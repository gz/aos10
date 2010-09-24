/*********************************************************************
 *                
 * Copyright (C) 2002-2004,  Karlsruhe University
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *                
 * File path:     api/v4/space.cc
 * Description:   architecture independent parts of space_t
 *                
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *                
 * $Id: space.cc,v 1.63 2004/12/09 01:19:57 cvansch Exp $
 *                
 ********************************************************************/

#include <l4.h>
#include <debug.h>
#include <kmemory.h>
#include <kdb/tracepoints.h>
#include INC_API(space.h)
#include INC_API(tcb.h)
#include INC_API(kernelinterface.h)
#include INC_API(schedule.h)
#include INC_API(syscalls.h)
#include INC_API(threadstate.h)
#include INC_GLUE(syscalls.h)


space_t * roottask_space;

DECLARE_TRACEPOINT (PAGEFAULT_USER);
DECLARE_TRACEPOINT (PAGEFAULT_KERNEL);
DECLARE_TRACEPOINT (SYSCALL_SPACE_CONTROL);

DECLARE_KMEM_GROUP (kmem_space);

#ifdef CONFIG_DEBUG
space_t * global_spaces_list UNIT("kdebug") = NULL;
spinlock_t spaces_list_lock;
#endif


void space_t::handle_pagefault(addr_t addr, addr_t ip, access_e access, bool kernel)
{
    tcb_t * current = get_current_tcb();
    bool user_area = is_user_area(addr);

    if (EXPECT_TRUE( user_area || (!kernel) ))
    {
	TRACEPOINT_TB (PAGEFAULT_USER,
		       printf("%t: user %s pagefault at %p, ip=%p\n", 
			      current,
			      access == space_t::write     ? "write" :
			      access == space_t::read	   ? "read"  :
			      access == space_t::execute   ? "execute" :
			      access == space_t::readwrite ? "read/write" :
			      "unknown", 
			      addr, ip),
		       "user page fault at %x (current=%t, ip=%x, access=%x)",
		       (word_t)addr, (word_t)current, (word_t)ip, access);

	/* #warning root-check in default pagefault handler. */
	/* VU: with software loaded tlbs that could be handled elsewhere...*/
	if (EXPECT_TRUE( !is_roottask_space(current->get_space()) ))
	{
	    if (kernel)
	    {
		printf("kernel access raised user pagefault @ %p, ip=%p, "
		       "space=%p\n", addr, ip, this);
		enter_kdebug("kpf");
	    }

	    // if we have a user fault we may have a stale partner
	    current->set_partner(threadid_t::nilthread());

	    current->send_pagefault_ipc (addr, ip, access);
	}
	else
	{
	    if (user_area){
		map_roottask(addr);
	    }
	    else
	    {
		printf("root task accessed kernel space @ %p, ip=%p - deny\n",
		       addr, ip);
		enter_kdebug("root kpf");
	    }
	}
	return;
    }
    else
    {
	/* fault in kernel area */
	TRACEPOINT_TB (PAGEFAULT_KERNEL,
		       printf ("%t: kernel pagefault in space "
			       "%p @ %p, ip=%p, type=%x\n",
			       current, this, addr, ip, access),
		       "kernel page fault at %x "
		       "(current=%t, ip=%x, space=%p)",
		       (word_t)addr, (word_t)current, (word_t)ip, (word_t)this);

	if (sync_kernel_space(addr))
	    return;

	if (is_tcb_area(addr))
	{
	    tcb_t * tcb = (tcb_t*)addr_align(addr, KTCB_SIZE);
	    /* write access to tcb area? */
	    if (access == space_t::write)
	    {
		enter_kdebug("ktcb write fault");
	    }
	    else
	    {
		map_dummy_tcb(tcb);
	    }
	    return;
	}
    }
    TRACEF("unhandled pagefault @ %p, %p\n", addr, ip);
    enter_kdebug("unhandled pagefault");

	thread_state_lock();
    current->set_state(thread_state_t::halted);
	thread_state_unlock();
    if (current != get_idle_tcb())
	current->switch_to_idle();
    printf("wrong access - unable to recover\n");
    spin_forever(1);
}



SYS_SPACE_CONTROL (threadid_t space_tid, word_t control, fpage_t kip_area,
		   fpage_t utcb_area)
{
    TRACEPOINT (SYSCALL_SPACE_CONTROL,
		printf("SYS_SPACE_CONTROL: space=%t, control=%p, kip_area=%p, "
		       "utcb_area=%p\n",  TID (space_tid),
		       control, kip_area.raw, utcb_area.raw));

    // Check privilege
    if (EXPECT_FALSE (! is_privileged_space(get_current_space())))
    {
	get_current_tcb ()->set_error_code (ENO_PRIVILEGE);
	return_space_control(0, 0);
    }

    // Check for valid space id
    if (EXPECT_FALSE (! space_tid.is_global() || space_tid.is_nilthread()))
    {
	get_current_tcb ()->set_error_code (EINVALID_SPACE);
	return_space_control(0, 0);
    }

    tcb_t * space_tcb = get_current_space()->get_tcb(space_tid);
    if (EXPECT_FALSE (space_tcb->get_global_id() != space_tid))
    {
	get_current_tcb ()->set_error_code (EINVALID_SPACE);
	return_space_control(0, 0);
    }

    space_t * space = space_tcb->get_space();

    if (! space->is_initialized ())
    {
	/*
	 * Space does not exist.  Create it.
	 */

#if (KIP_KIP_AREA != 0)	    /* KIP is user allocated */
	if ((get_kip()->utcb_info.get_minimal_size() > utcb_area.get_size()) ||
	    (!space->is_user_area(utcb_area)))
	{
	    // Invalid UTCB area
	    get_current_tcb ()->set_error_code (EUTCB_AREA);
	    return_space_control(0, 0);
	}
	else if ((get_kip()->kip_area_info.get_size() > kip_area.get_size()) ||
		 (! space->is_user_area(kip_area)) ||
		 kip_area.is_overlapping (utcb_area))
	{
	    // Invalid KIP area
	    get_current_tcb ()->set_error_code (EKIP_AREA);
	    return_space_control(0, 0);
	}
#else	/* KIP is kernel allocated */
	if (kip_area.raw != 0)
	{
	    // Invalid KIP area
	    get_current_tcb ()->set_error_code (EKIP_AREA);
	    return_space_control(0, 0);
	}
	else if (utcb_area.raw != 0)
	{
	    // Invalid UTCB area
	    get_current_tcb ()->set_error_code (EUTCB_AREA);
	    return_space_control(0, 0);
	}
#endif
	else
	{
	    /* ok, everything seems fine, now setup the space */
	    space->init(utcb_area, kip_area);
	}
    }

    word_t old_control = space->space_control (control);

    return_space_control (1, old_control);

    UNREACHABLE(spin_forever();)
}

/**
 * allocate_space: allocates a new space_t
 */
#if !defined(CONFIG_ARCH_ALPHA) && !defined(CONFIG_ARCH_ARM)
space_t * allocate_space()
{
    space_t * space = (space_t*)kmem.alloc(kmem_space, sizeof(space_t));
    if (!space)
	return NULL;

    space->enqueue_spaces();
    return space;
}

/**
 * free_space: frees a previously allocated space
 */
void free_space(space_t * space)
{
    ASSERT(DEBUG, space);
#ifndef ARCH_IA32
    space->get_asid()->release();
#endif
    space->dequeue_spaces();	
    kmem.free(kmem_space, (addr_t)space, sizeof(space_t));
}
#endif /* !defined(CONFIG_ARCH_ALPHA) && !defined(CONFIG_ARCH_ARM) */

void space_t::free (void)
{
    arch_free ();

    // Unmap everything, including KIP and UTCB
    fpage_t fp = fpage_t::complete ();
    unmap_fpage (fp, true);
}
