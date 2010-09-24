/****************************************************************************
 *
 *      $Id: $
 *
 *      Description: Simple operating system l4 helper functions
 *
 *      Author:		    Godfrey van der Linden
 *      Original Author:    Ben Leslie
 *
 ****************************************************************************/

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <l4/types.h>

#include <l4/kip.h>
#include <l4/ipc.h>
#include <l4/thread.h>
#include <l4/schedule.h>

#include <nfs.h>
#include <serial.h>

#include "libsos.h"

// Globals accessable everywhere
L4_ThreadId_t root_thread_g;

// Hack externs from timer.c
extern void utimer_init(void);
extern void utimer_sleep(uint32_t microseconds);

#define verbose 1

#define ONE_MEG	    (1 * 1024 * 1024)

extern void _start(void);

static L4_Word_t sSosMemoryTop, sSosMemoryBot;

static L4_Fpage_t kip_fpage_s, utcb_fpage_s;
static void *kip_s, *binfo_s;
static L4_Word_t utcb_base_s;
static L4_Word_t last_thread_s;

/* Initialise the L4 Environment library */
int
libsos_init(void)
{
    // Find out my ID
    root_thread_g = L4_Myself();
    
    // Announce ourself to the world
    puts("\n*********************************");

    // Find out the kernel information page
    void *kip = kip_s = L4_GetKernelInterface();
    last_thread_s = L4_ThreadIdUserBase(kip);

    sos_logf("SOS Starting. My thread id is %lx, next %lx\n\n",
	    root_thread_g.raw, last_thread_s);
    
    // and get the correct area
    kip_fpage_s.raw = L4_KipAreaSizeLog2(kip);
    dprintf(0, "KIP at %p l2sz %lx\n", kip, kip_fpage_s.raw);
    if (kip_fpage_s.raw)
	kip_fpage_s = L4_FpageLog2((L4_Word_t) kip, kip_fpage_s.raw);
    else
	kip_fpage_s.raw = 0;	// No userland kip map
    
    // get my localid to find the utcb base
    utcb_base_s  = (L4_Word_t) L4_GetUtcbBase();    // XXX gvdl: inder or cast?
    assert(!(utcb_base_s & (L4_UtcbAreaSize(kip) - 1)));

    utcb_fpage_s.raw = L4_UtcbAreaSizeLog2(kip);
    if (utcb_fpage_s.raw) {
	utcb_fpage_s = L4_FpageLog2(utcb_base_s, utcb_fpage_s.raw);
	dprintf(0, "UTCB at %lx l2sz %lx\n", utcb_base_s, utcb_fpage_s.raw);
    }
    else {  // No userland utcb map
	utcb_fpage_s.raw = 0;
	utcb_base_s      = 0;
	dprintf(0, "UTCB under kernel control\n");
    }

    binfo_s = (void *) L4_BootInfo(kip);
    dprintf(1, "BootInfo at %p\n", binfo_s);

    utimer_init();

    return 0;
}

//
// Find appropriate chunks of memory
// 

static inline const char *
strtypemem(L4_Word_t type)
{
    static const char *types[] = {
	"Undefined",		// L4_UndefinedMemoryType		(0x0)
	"Conventional",		// L4_ConventionalMemoryType		(0x1)
	"Reserved",		// L4_ReservedMemoryType		(0x2)
	"Dedicated",		// L4_DedicatedMemoryType		(0x3)
	"NUMA",			// L4_NUMAMemoryType			(0x4)
	"Global",		// L4_GlobalMemoryType			(0x5)
	"Error",		//					(0x6)
	"Error",		//					(0x7)
	"Error",		//					(0x8)
	"Error",		//					(0x9)
	"Error",		//					(0xa)
	"Tracebuffer",		// L4_TracebufferMemoryType		(0xb)
	"Error",		//					(0xc)
	"Error",		//					(0xd)
	"BootLoaderSpecific",	// L4_BootLoaderSpecificMemoryType	(0xe)
	"ArchitectureSpecific",	// L4_ArchitectureSpecificMemoryType	(0xf)
    };
#   define num_strtypemem (sizeof(types) / sizeof(types[0]))
    if (type < num_strtypemem)
	return types[type];
    else
	return "Error";
#   undef num_strtypemem
}

static int
memdesc_info(L4_KernelInterfacePage_t *kip, int pos, 
	     L4_Word_t *lowP, L4_Word_t *highP, L4_Word_t *typeP)
{
    L4_MemoryDesc_t *memdesc = L4_MemoryDesc(kip, pos);
    assert(memdesc != NULL);

    // get the info out
    // note that L4 uses inclusive addresses, we prefer to C style exlusive
    // so add 1 to the high memory pointer
    L4_Word_t high = L4_MemoryDescHigh(memdesc) + 1;
    L4_Word_t low  = L4_MemoryDescLow(memdesc);
    assert(high > low);

    L4_Word_t type = memdesc->x.type;
    *lowP = low; *highP = high; *typeP = type;

    /* return whether physical */
    return !memdesc->x.v;
}

/* Find the largest available chunk of physical RAM */
void
sos_find_memory(L4_Word_t *lowP, L4_Word_t *highP)
{
    int i;
    L4_KernelInterfacePage_t *kip = kip_s;
    L4_Word_t mem_bot = 0, mem_top = 0, maxsize = 0;

    // find the biggest block of conventional physical memory first
    for (i = 0; i < kip->MemoryInfo.n; i++) {
	L4_Word_t lowmem, highmem, typemem, sizemem;
	int is_phys = memdesc_info(kip, i, &lowmem, &highmem, &typemem);

	dprintf(3, "mem(%d%c): %08lx-%08lx %s\n", i, (is_phys)?'p':'v',
		lowmem, highmem, strtypemem(typemem));

	if (is_phys && L4_ConventionalMemoryType == typemem) {
	    sizemem = highmem - lowmem;
	    if (sizemem > maxsize) {
		mem_bot = lowmem, mem_top = highmem;
	    }
	}
    }
    dprintf(2, "Biggest conventional memory %lx-%lx\n", mem_bot, mem_top);

    // Now remove dedicated areas of physical memory from the range
    for (i = 0; i < kip->MemoryInfo.n; i++) {
	L4_Word_t lowmem, highmem, typemem;

	int is_phys = memdesc_info(kip, i, &lowmem, &highmem, &typemem);
	if (is_phys && L4_ConventionalMemoryType != typemem) {
	    /* calculate remaining sizes on each side */
	    L4_Word_t size_bot = 0, size_top = 0;
	    if (mem_bot <= lowmem && lowmem < mem_top)
		size_bot = lowmem - mem_bot;
	    if (mem_bot < highmem && highmem <= mem_top)
		size_top = mem_top - highmem;
	    if (size_bot > size_top)
		mem_top = lowmem;
	    else if (size_top > 0)
		mem_bot = highmem;
	}
    }

    sSosMemoryTop = mem_top;
    sSosMemoryBot = sSosMemoryTop - ONE_MEG;

    *highP = sSosMemoryBot;
    *lowP  = mem_bot;
}

//
// L4 Debug output functions
//

// pretty print an L4 error code
void
sos_print_error(L4_Word_t ec)
{
    L4_Word_t e, p;

    /* mmm, seedy */
    e = (ec >> 1) & 7;
    p = ec & 1;

    // XXX gvdl: get page number when doc is available
    sos_logf("IPC %s error, code is 0x%lx (see p55)\n", p? "receive":"send", e);
    if (e == 4)
	sos_logf("  Message overflow error\n");
}

void sos_print_l4memory(void *addr, L4_Word_t len)
{
    len = ((len + 15) & ~15);	// Round up to next 16
    unsigned int *wp = &((unsigned int *) addr)[len / sizeof(unsigned)];
    do {
	wp -= 4;
	sos_logf("%8p: %08x %08x %08x %08x\n", wp, wp[3], wp[2], wp[1], wp[0]);
    } while (wp != addr);

}

// pretty print an L4 fpage
void
sos_print_fpage(L4_Fpage_t fpage)
{
    sos_logf("fpage(%lx - %lx)",
	    L4_Address(fpage), L4_Address(fpage) + L4_Size(fpage));
}

void sos_logf(const char *msg, ...)
{
    va_list alist;

    va_start(alist, msg);
    vprintf(msg, alist);
    va_end(alist);
}


//
// Thread and task ID handling function
//

// A simple, i.e. broken, kernel thread id allocator
L4_ThreadId_t
sos_get_new_tid(void)
{
    return L4_GlobalId(++last_thread_s, 1);
}

// Create a new thread
static inline L4_ThreadId_t
create_thread(L4_ThreadId_t tid, L4_ThreadId_t scheduler)
{
    L4_Word_t utcb_location = utcb_base_s;
    if (utcb_location)
	utcb_location += L4_UtcbSize(kip_s) * L4_ThreadNo(tid);

    // Create active thread
    int res = L4_ThreadControl(tid,
			       root_thread_g,	// address space
			       scheduler,	// scheduler
			       root_thread_g,	// pager
			       L4_anythread,	// send redirector
			       L4_anythread,	// receive redirector
			       (void *) utcb_location);
    if (!res) {
	sos_logf("ERROR(%lu): ThreadControl(%lx) utcb %lx\n",
		L4_ErrorCode(), tid.raw, utcb_location);
	tid = L4_nilthread;
    }
    return tid;
}

// Create and start a sos thread in the rootserver
L4_ThreadId_t
sos_thread_new_priority(L4_Word_t prio, void *entry, void *stack)
{
    L4_ThreadId_t tid = sos_get_new_tid();
    L4_ThreadId_t sched = (prio)? root_thread_g : L4_anythread;

    // This bit creates the thread, but it won't execute any code yet 
    tid = create_thread(tid, sched);
    if (!L4_IsNilThread(tid)) {
	if (prio && !L4_Set_Priority(tid, prio)) {
	    sos_logf("sos: failed to set priority for %lx\n", tid.raw);
	    sos_print_error(L4_ErrorCode());
	}

	// Send an ipc to thread thread to start it up
	L4_Start_SpIp(tid, (L4_Word_t) stack, (L4_Word_t) entry);
    }

    return tid;
}

// Create and start a sos thread in the rootserver
L4_ThreadId_t
sos_thread_new(void *entrypoint, void *stack)
{
    return sos_thread_new_priority(/* prio */ 0, entrypoint, stack);
}

// Create and start a new task
L4_ThreadId_t
sos_task_new(L4_Word_t task, L4_ThreadId_t pager, 
	     void *entrypoint, void *stack)
{
    // HACK: Workaround for compiler bug, volatile qualifier stops the internal
    // compiler error.
    volatile uint32_t taskId = task << THREADBITS;
    int res;

    // Create an inactive thread
    L4_ThreadId_t tid = L4_GlobalId(taskId, 1);
    res = L4_ThreadControl(tid, tid, root_thread_g,
	    L4_nilthread, L4_anythread, L4_anythread, (void *) -1);
    if (!res)
	return ((L4_ThreadId_t) { raw : -1});

    // Setup space
    L4_Word_t dummy;
    res = L4_SpaceControl(tid, 0, kip_fpage_s, utcb_fpage_s, &dummy);
    if (!res)
	return ((L4_ThreadId_t) { raw : -2});

    // Activate thread
    res = L4_ThreadControl(tid, tid, root_thread_g,
	    pager, L4_anythread, L4_anythread, (void *) utcb_base_s);
    if (!res)
	return ((L4_ThreadId_t) { raw : -3});

    L4_Start_SpIp(tid, (L4_Word_t) stack, (L4_Word_t) entrypoint);

    return tid;
}

L4_BootRec_t *
sos_get_binfo_rec(int index)
{
    static struct { int index; L4_BootRec_t *rec; } last = { -1 };
    void *binfo = binfo_s;
    L4_BootRec_t *rec;

    if ( !(0 <= index && index < (int) L4_BootInfo_Entries(binfo)) )
	return NULL;				// index -- Out of range 
    else if (!index)
	rec = L4_BootInfo_FirstEntry(binfo_s);	// Initial index
    else if (last.index + 1 == index)
	rec = L4_BootRec_Next(last.rec);	// Cached last value
    else {	// Cache is invalid -- need to start from beginning
	int i;

	rec = L4_BootInfo_FirstEntry(binfo_s);
	for (i = 1; i <= index; i++)		// index has been range checked
	    rec = L4_BootRec_Next(rec);
    }

    last.index = index;
    last.rec   = rec;

    return rec;
}

// Memory for the ixp400 networking layers
extern void *sos_malloc(uint32_t size);
void *sos_malloc(uint32_t size)
{
    if (sSosMemoryBot + size < sSosMemoryTop) {
	L4_Word_t bot = sSosMemoryBot;
	sSosMemoryBot += size;
	return (void *) bot;
    }
    else
	return (void *) 0;
}

void sos_usleep(uint32_t microseconds)
{
    utimer_sleep(microseconds);	// M4 must change to your timer
}
