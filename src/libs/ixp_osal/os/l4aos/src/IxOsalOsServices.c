/**
 * @file IxOsalOsServices.c (l4aos)
 *
 * @brief Implementation for Irq, Mem, sleep. 
 * 
 * 
 * @par
 * IXP400 SW Release version 2.1
 * 
 * -- Copyright Notice --
 * 
 * @par
 * Copyright (c) 2001-2005, Intel Corporation.
 * All rights reserved.
 * 
 * @par
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * 
 * @par
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * 
 * @par
 * -- End of Copyright Notice --
 */

#include "IxOsal.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <l4.h>		// from sos
#include <libsos.h>	// from sos

#include "IxOsalOsIxp425Irq.h"

typedef struct IxOsalIRQEntry
{
    void (*fRoutine) (void *);
    void *fParameter;
    uint32_t fFlags;
} IxOsalIRQEntry;

enum IxOsalIRQEntryFlags {
    kIxOsalIntOccurred = 0x00000001,
    kIxOsalIntEnabled  = 0x00000002,
    kIxOsalIntActive   = (kIxOsalIntOccurred|kIxOsalIntEnabled),
    kIxOsalIntReserved = 0x00000004,
};

#define NR_INTS	32
#define L4_INTERRUPT	((L4_Word_t) -1)

static char *traceHeaders[] = {
    "%lx",
    "%lx[fatal] ",
    "%lx[error] ",
    "%lx[warning] ",
    "%lx[message] ",
    "%lx[debug1] ",
    "%lx[debug2] ",
    "%lx[debug3] ",
    "%lx[all]"
};

/* by default trace all but debug message */
static IxOsalLogLevel sIxOsalCurrLogLevel = IX_OSAL_LOG_LVL_MESSAGE;

static IxOsalSemaphore sIxOsalMemAllocLock;
static IxOsalSemaphore sIxOsalLogLock;

static IxOsalSemaphore sIxOsalOsIrqLock;
static bool sIxOsalOsServicesInited;
static bool sIxOsalIntLocked, sIxOsalIntUnlocked;
static L4_Word_t sIxOsalMaxSysNum;

static IxOsalIRQEntry sIxOsalIrqInfo[NR_INTS];

// Only used from the IxOsalOsIxp400.c:ixOsalOemInit() routine
extern void ixOsalOSServicesInit(void);
void ixOsalOSServicesInit(void)
{
    if (sIxOsalOsServicesInited)
	return;

    sIxOsalOsServicesInited = true;

    void *kip = L4_GetKernelInterface();
    sIxOsalMaxSysNum = L4_ThreadIdUserBase(kip);

    ixOsalSemaphoreInit(&sIxOsalOsIrqLock, 1);
    ixOsalSemaphoreInit(&sIxOsalMemAllocLock, 1);
    ixOsalSemaphoreInit(&sIxOsalLogLock, 1);
}

extern void ixOsalOSServicesFinaliseInit(void);
void ixOsalOSServicesFinaliseInit(void)
{
    // Block out timer1 & timestamp interrupts
    int i = sizeof(sIxOsalIrqInfo)/sizeof(sIxOsalIrqInfo[0]);
    do {
	if (!sIxOsalIrqInfo[--i].fRoutine)
	    sIxOsalIrqInfo[i].fFlags = kIxOsalIntReserved;
    } while (i);
}

/*
 * General interrupt handler
 */
extern void ixOsalOSServicesServiceInterrupt(L4_ThreadId_t *tP, int *sendP);
void ixOsalOSServicesServiceInterrupt(L4_ThreadId_t *tidP, int *sendP)
{
    L4_ThreadId_t toTid = L4_nilthread;
    L4_ThreadId_t fromTid = *tidP;

	L4_Word_t irqIndex = L4_ThreadNo(fromTid);
	IxOsalIRQEntry *irq;

	toTid = fromTid;	// Acknowledge message by default
	ixOsalSemaphoreWait(&sIxOsalOsIrqLock, IX_OSAL_WAIT_FOREVER);
	if (irqIndex >= sIxOsalMaxSysNum)
	    sIxOsalIntUnlocked = true;
	else {
	    // Interrupt message
	    irq = &sIxOsalIrqInfo[irqIndex];
	    assert(irqIndex < NR_INTS);
	    assert( !(irq->fFlags & kIxOsalIntOccurred) );
	    assert( !(irq->fFlags & kIxOsalIntReserved) );

	    irq->fFlags |= kIxOsalIntOccurred;	// Interrupt occured

	    // Call the interrupt routine if enabled
	    if (sIxOsalIntLocked || !(irq->fFlags & kIxOsalIntEnabled))
		toTid = L4_nilthread;
	    else {
		(*irq->fRoutine)(irq->fParameter);
		irq->fFlags &= ~kIxOsalIntOccurred;	// Clear occured bit
		if ( !(irq->fFlags & kIxOsalIntEnabled) )
		    toTid = L4_nilthread; // If disabled don't reply to int
	    }
	}

	if (sIxOsalIntUnlocked) {
	    // Must be an unlock message from user land not an interrupt
	    // Probably should check the tag's label too
	    for (irqIndex = 0; irqIndex < NR_INTS; irqIndex++) {
		irq = &sIxOsalIrqInfo[irqIndex];
		// Check that the interrupt is enabled
		if (!irq->fRoutine
		||  (irq->fFlags & kIxOsalIntActive) != kIxOsalIntActive)
		    continue;

		(*irq->fRoutine)(irq->fParameter);
		irq->fFlags &= ~kIxOsalIntOccurred;	// Clear occured bit

		if (irq->fFlags & kIxOsalIntEnabled) {
		// Acknowledge this interrupt now that it is enabled
		L4_Set_MsgTag(L4_Niltag); // Clear the tag down for sending
		L4_MsgTag_t tag = L4_Reply(L4_GlobalId(irqIndex, 1));
		    assert(!L4_IpcFailed(tag));
		}
	    }
	    sIxOsalIntUnlocked = false;	// Done it
	}
	ixOsalSemaphorePost(&sIxOsalOsIrqLock);
    *sendP = !L4_IsNilThread(toTid);
    *tidP = toTid;
    L4_Set_MsgTag(L4_Niltag);	// Clear the tag down for sending
}

/**************************************
 * Irq services 
 *************************************/

PUBLIC IX_STATUS
ixOsalIrqBind(UINT32 vector, IxOsalVoidFnVoidPtr routine, void *parameter)
{
    assert(sIxOsalOsServicesInited);
    IxOsalIRQEntry *irq = &sIxOsalIrqInfo[vector];

    assert( !(irq->fFlags & kIxOsalIntReserved) );
    if (vector >= NR_INTS || irq->fRoutine) {
        ixOsalLog(IX_OSAL_LOG_LVL_ERROR, IX_OSAL_LOG_DEV_STDOUT,
            "%s: illegal %d %p.\n", LOG_FUNCTION, vector,
	    (uintptr_t) irq->fRoutine, 0, 0, 0);
        return IX_FAIL;
    }

    irq->fRoutine   = routine;
    irq->fParameter = parameter;
    irq->fFlags     = kIxOsalIntEnabled;
    L4_AssociateInterrupt(L4_GlobalId(vector, 1), root_thread_g);
    return IX_SUCCESS;
}

PUBLIC IX_STATUS
ixOsalIrqUnbind(UINT32 vector)
{
    assert(sIxOsalOsServicesInited);
    IxOsalIRQEntry *irq = &sIxOsalIrqInfo[vector];

    assert( !(irq->fFlags & kIxOsalIntReserved) );
    if (vector >= NR_INTS || !irq->fRoutine) {
        ixOsalLog (IX_OSAL_LOG_LVL_ERROR, IX_OSAL_LOG_DEV_STDOUT,
            "%s: illegal %d.\n", LOG_FUNCTION, vector,
	    (uintptr_t) irq->fRoutine, 0, 0, 0);
        return IX_FAIL;
    }
    else {
	L4_DeassociateInterrupt(L4_GlobalId(vector, 1));
	ixOsalSemaphoreWait(&sIxOsalOsIrqLock, IX_OSAL_WAIT_FOREVER);
	irq->fRoutine = NULL;
	irq->fFlags   = 0;
	ixOsalSemaphorePost(&sIxOsalOsIrqLock);
    }

    return IX_SUCCESS;
}

/* XXX gvdl: Doesn't disable thread scheduling */
PUBLIC UINT32
ixOsalIrqLock()
{
    UINT32 ret;

    assert(sIxOsalOsServicesInited);
    if (L4_IsThreadEqual(L4_Myself(), root_thread_g)) {
	ret = sIxOsalIntLocked;
	sIxOsalIntLocked = true;
	sIxOsalIntUnlocked = false;
    }
    else {
	ixOsalSemaphoreWait(&sIxOsalOsIrqLock, IX_OSAL_WAIT_FOREVER);
	ret = sIxOsalIntLocked;
	sIxOsalIntLocked = true;
	sIxOsalIntUnlocked = false;
	ixOsalSemaphorePost(&sIxOsalOsIrqLock);
    }

    return ret;
}

/* Enable interrupts and task scheduling,
 * input parameter: irqEnable status returned
 * by ixOsalIrqLock().
 */
PUBLIC void
ixOsalIrqUnlock(UINT32 lockKey)
{
    assert(sIxOsalOsServicesInited);

    if (L4_IsThreadEqual(L4_Myself(), root_thread_g)) {
	if (!lockKey && sIxOsalIntLocked)
	    sIxOsalIntUnlocked = true;
	sIxOsalIntLocked = lockKey;
    }
    else {
	ixOsalSemaphoreWait(&sIxOsalOsIrqLock, IX_OSAL_WAIT_FOREVER);
	bool reenable = sIxOsalIntLocked && !lockKey;
	if (reenable)
	    sIxOsalIntLocked = false;
	ixOsalSemaphorePost(&sIxOsalOsIrqLock);

	if (reenable) {
	    L4_MsgTag_t tag = L4_Niltag;
	    L4_Set_MsgTag(L4_MsgTagAddLabel(tag, L4_INTERRUPT));
	    tag = L4_Call(root_thread_g);
	    assert(!L4_IpcFailed(tag));
	}
    }
}

// Not a supported function
PUBLIC UINT32
ixOsalIrqLevelSet(UINT32 level)
{
    assert(!"ixOsalIrqLevelSet");
    return 0;
}

PUBLIC void
ixOsalIrqEnable(UINT32 irqLevel)
{
    assert(sIxOsalOsServicesInited);

    if (irqLevel >= NR_INTS) {
        // Not supported 
        ixOsalLog(IX_OSAL_LOG_LVL_MESSAGE, IX_OSAL_LOG_DEV_STDOUT,
            "%s: IRQ %d not supported\n", LOG_FUNCTION, irqLevel, 0, 0, 0, 0);
	return;
    }

    IxOsalIRQEntry *irq = &sIxOsalIrqInfo[irqLevel];
    if (L4_IsThreadEqual(L4_Myself(), root_thread_g))
	irq->fFlags |= kIxOsalIntEnabled;
    else {
	ixOsalSemaphoreWait(&sIxOsalOsIrqLock, IX_OSAL_WAIT_FOREVER);
	bool reenable = !(irq->fFlags & kIxOsalIntEnabled);
	irq->fFlags |= kIxOsalIntEnabled;
	ixOsalSemaphorePost(&sIxOsalOsIrqLock);

	// Let the irq thread know
	if (reenable) {
	    L4_MsgTag_t tag = L4_Niltag;
	    L4_Set_MsgTag(L4_MsgTagAddLabel(tag, L4_INTERRUPT));
	    tag = L4_Call(root_thread_g);
	    assert(!L4_IpcFailed(tag));
	}
    }
}

PUBLIC void
ixOsalIrqDisable(UINT32 irqLevel)
{
    assert(sIxOsalOsServicesInited);

    if (irqLevel < NR_INTS) {
	ixOsalSemaphoreWait(&sIxOsalOsIrqLock, IX_OSAL_WAIT_FOREVER);
	sIxOsalIrqInfo[irqLevel].fFlags &= ~kIxOsalIntEnabled;
	ixOsalSemaphorePost(&sIxOsalOsIrqLock);
    }
    else {
	// Not supported 
	ixOsalLog(IX_OSAL_LOG_LVL_MESSAGE, IX_OSAL_LOG_DEV_STDOUT,
	    "%s: IRQ %d not supported\n", LOG_FUNCTION, irqLevel, 0, 0, 0, 0);
    }
}

/*********************
 * Log function
 *********************/

INT32
ixOsalLog (IxOsalLogLevel level,
    IxOsalLogDevice device,
    char *format, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6)
{
    if (!sIxOsalLogLock)
	ixOsalOSServicesInit();
	
    INT32 ret = IX_OSAL_LOG_ERROR;

    ixOsalSemaphoreWait(&sIxOsalLogLock, IX_OSAL_WAIT_FOREVER);
    do {
	// Return -1 for custom display devices
	if (device != IX_OSAL_LOG_DEV_STDOUT
	&&  device != IX_OSAL_LOG_DEV_STDERR) {
	    printf("%s: only IX_OSAL_LOG_DEV_STDOUT and "
		   "IX_OSAL_LOG_DEV_STDERR are supported\n", __FUNCTION__);
	    break;
	}

	if (level <= sIxOsalCurrLogLevel && level != IX_OSAL_LOG_LVL_NONE) {
	    if (level == IX_OSAL_LOG_LVL_USER)
		ret = printf("%lx", L4_ThreadNo(L4_Myself()));
	    else
		ret = printf(traceHeaders[level - 1], L4_ThreadNo(L4_Myself()));

	    ret += printf(format, arg1, arg2, arg3, arg4, arg5, arg6);
	    break;
	}
    } while(0);
    ixOsalSemaphorePost(&sIxOsalLogLock);

    return ret;
}

PUBLIC UINT32
ixOsalLogLevelSet (UINT32 level)
{
    UINT32 oldLevel;

    // Check value first
    if (IX_OSAL_LOG_LVL_NONE <= level && level <= IX_OSAL_LOG_LVL_ALL) {
	oldLevel = sIxOsalCurrLogLevel;
	sIxOsalCurrLogLevel = level;
	return oldLevel;
    }
    else {
        ixOsalLog (IX_OSAL_LOG_LVL_MESSAGE, IX_OSAL_LOG_DEV_STDOUT,
            "ixOsalLogLevelSet: Log Level is between %d and %d \n",
            IX_OSAL_LOG_LVL_NONE, IX_OSAL_LOG_LVL_ALL, 0, 0, 0, 0);
        return IX_OSAL_LOG_LVL_NONE;
    }
}

/**************************************
 * Task services 
 *************************************/

PUBLIC void
ixOsalBusySleep(UINT32 microseconds)
{
    // never busy sleep on an L4 system
    assert(microseconds < 32 * 1000 * 1000);
    sos_usleep(microseconds);
}

PUBLIC void
ixOsalSleep (UINT32 milliseconds)
{
    assert(milliseconds < 32 * 1000);
    sos_usleep(milliseconds * 1000);
}

/**************************************
 * Memory functions 
 *************************************/

void *
ixOsalMemAlloc(UINT32 size)
{
    ixOsalSemaphoreWait(&sIxOsalMemAllocLock, IX_OSAL_WAIT_FOREVER);
    void *mem = malloc(size);
    ixOsalSemaphorePost(&sIxOsalMemAllocLock);
    return mem;
}

void
ixOsalMemFree(void *ptr)
{
    ixOsalSemaphoreWait(&sIxOsalMemAllocLock, IX_OSAL_WAIT_FOREVER);
    IX_OSAL_ASSERT(ptr);
    free(ptr);
    ixOsalSemaphorePost(&sIxOsalMemAllocLock);
}

/* 
 * Copy count bytes from src to dest ,
 * returns pointer to the dest mem zone.
 */
void *
ixOsalMemCopy(void *dest, void *src, UINT32 count)
{
    IX_OSAL_ASSERT(dest);
    IX_OSAL_ASSERT(src);
    return memcpy(dest, src, count);
}

/* 
 * Fills a memory zone with a given constant byte,
 * returns pointer to the memory zone.
 */
void *
ixOsalMemSet(void *ptr, UINT8 filler, UINT32 count)
{
    IX_OSAL_ASSERT(ptr);
    return memset (ptr, filler, count);
}

/*****************************
 *
 *  Time
 *
 *****************************/

/* Retrieve current system time */
void
ixOsalTimeGet (IxOsalTimeval * ptime)
{
    assert(!"ixOsalTimeGet");
}

/* Timestamp is implemented in OEM */
PUBLIC UINT32
ixOsalTimestampGet (void)
{
    return IX_OSAL_OEM_TIMESTAMP_GET();
}

/* OEM-specific implementation */
PUBLIC UINT32
ixOsalTimestampResolutionGet (void)
{
    return IX_OSAL_OEM_TIMESTAMP_RESOLUTION_GET();
}

PUBLIC UINT32
ixOsalSysClockRateGet (void)
{
    return IX_OSAL_OEM_SYS_CLOCK_RATE_GET();
}

PUBLIC void
ixOsalYield (void)
{
    L4_Yield();
}


/*****************************
 *
 *  Memory mapping functions
 *
 *****************************/
void IxOsalOsMapMemory(IxOsalMemoryMap *map)
{
    L4_Word_t virt = IX_OSAL_IXP400_VIRT_OFFSET(map->physicalAddress);

    ixOsalLog (IX_OSAL_LOG_LVL_DEBUG1, IX_OSAL_LOG_DEV_STDOUT,
	"%s: Mapping(%lx,%lx) => %lx\n",
	LOG_FUNCTION, map->physicalAddress, map->size, virt, 0, 0);

    L4_Fpage_t targetFpage = L4_Fpage(virt, map->size);
    L4_Set_Rights(&targetFpage, L4_ReadWriteOnly);
    L4_PhysDesc_t phys = L4_PhysDesc(map->physicalAddress, L4_IOMemory);
    assert(L4_MapFpage(root_thread_g, targetFpage, phys));
    map->virtualAddress = virt;
}

void IxOsalOsUnmapMemory(IxOsalMemoryMap *map)
{
    L4_Word_t virt = map->virtualAddress;
    map->virtualAddress = 0;
    L4_Fpage_t targetFpage = L4_Fpage(virt, map->size);
    assert(L4_UnmapFpage(root_thread_g, targetFpage));
}


