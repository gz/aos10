/*********************************************************************
 *                
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *                
 * File path:     api/v4/map.cc
 * Description:   mapping implementation
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
 ********************************************************************/

#include <l4.h>
#include <debug.h>
#include <kmemory.h>
#include <kdb/tracepoints.h>
#include INC_API(map.h)
#include INC_API(tcb.h)
#include INC_API(syscalls.h)


DECLARE_TRACEPOINT (SYSCALL_MAP_CONTROL);

SYS_MAP_CONTROL (threadid_t space_tid, word_t control)
{
    map_control_t ctrl;
    ctrl = control;

    TRACEPOINT (SYSCALL_MAP_CONTROL,
		printf("SYS_MAP_CONTROL: space=%t, control=%p "
		       "(n = %d, %c%c)\n", TID (space_tid),
		       control, ctrl.highest_item(),
		       ctrl.is_modify() ? 'M' : 'm',
		       ctrl.is_read() ? 'R' : 'r'));

    // Check privilege
    if (EXPECT_FALSE (! is_privileged_space(get_current_space())))
    {
	get_current_tcb ()->set_error_code (ENO_PRIVILEGE);
	return_map_control(0);
    }

    // Check for valid space id
    if (EXPECT_FALSE (! space_tid.is_global() || space_tid.is_nilthread()))
    {
	get_current_tcb ()->set_error_code (EINVALID_SPACE);
	return_map_control(0);
    }

    tcb_t * space_tcb = get_current_space()->get_tcb(space_tid);
    if (EXPECT_FALSE (space_tcb->get_global_id() != space_tid))
    {
	get_current_tcb ()->set_error_code (EINVALID_SPACE);
	return_map_control(0);
    }

    word_t count = ctrl.highest_item()+1;

    if ((count*2) > IPC_NUM_MR)
    {
	get_current_tcb ()->set_error_code (EINVALID_PARAM);
	return_map_control(0);
    }

    tcb_t * tcb = get_current_tcb();
    space_t * space = space_tcb->get_space();

    for (word_t i = 0; i < count; i++)
    {
	phys_desc_t tmp_base;
	perm_desc_t tmp_perm;
	/* XXX check for message overflow !! */

	// Read the existing pagetable
	if (ctrl.is_read())
	{
	    fpage_t fpg;

	    tmp_perm.clear();
	    fpg.raw = tcb->get_mr(i*2+1);

	    space->read_fpage(fpg, &tmp_base, &tmp_perm);
//	    printf("read map (%t) %p (%x) = %lx (%d)\n", TID(space_tid),
//		    fpg.get_address(), fpg.get_size(),
//		    (word_t)tmp_base.get_base(), tmp_base.get_attributes());
	}

	// Modify the page table
	if (ctrl.is_modify())
	{
	    phys_desc_t base;
	    fpage_t fpg;

	    base = tcb->get_mr(i*2);
	    fpg.raw = tcb->get_mr(i*2+1);

	    if (fpg.get_rwx() == 0)
	    {
//		printf("unmmap (%t) %p (%x)\n", TID(space_tid),
//			fpg.get_address(), fpg.get_size());
		space->unmap_fpage(fpg, false);
	    }
	    else
	    {
//		printf("map (%t) %p -> %p (%x)\n", TID(space_tid),
//			(addr_t)(word_t)base.get_base(), fpg.get_address(),
//			fpg.get_size());
		if (EXPECT_FALSE(!space->map_fpage(base, fpg)))
		{
		    get_current_tcb ()->set_error_code (ENO_MEM);
		    return_map_control(0);
		}
	    }
	}

	// Write back data to MRs
	if (ctrl.is_read())
	{
	    tcb->set_mr(i*2, tmp_base.get_raw());
	    tcb->set_mr(i*2+1, tmp_perm.get_raw());
	}

    }
    return_map_control(1);
}

