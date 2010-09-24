/*********************************************************************
 *                
 * Copyright (C) 2003-2006,  National ICT Australia (NICTA)
 *                
 * File path:     api/v4/cache.cc
 * Description:   Cache control implementation
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
#include INC_API(config.h)
#include INC_API(tcb.h)
#include INC_API(thread.h)
#include INC_API(cache.h)
#include INC_GLUE(cache.h)
#include INC_GLUE(schedule.h)

#include <kdb/tracepoints.h>

DECLARE_TRACEPOINT(SYSCALL_CACHE_CONTROL);

SYS_CACHE_CONTROL (threadid_t space, word_t control)
{
    cache_control_t ctrl;
    tcb_t * current = get_current_tcb();

    ctrl = control;

    TRACEPOINT (SYSCALL_CACHE_CONTROL,
		printf ("SYSCALL_CACHE_CONTROL: current=%p control=%lx, space=%t\n",
			current, control, TID(space));
	    );

    preempt_enable();

    switch (ctrl.operation())
    {
    case cop_flush_all:
	cache_t::flush_all_clean_invalidate();
	break;
    case cop_flush_fp:
	{
	    word_t count = ctrl.highest_item()+1;
	    tcb_t * tcb = current; // XXX space

	    if ((count*2) > IPC_NUM_MR)
	    {
		current->set_error_code (EINVALID_PARAM);
		goto error_out_preempt;
	    }

	    for (word_t i = 0; i < count; i++)
	    {
		word_t start = tcb->get_mr(i*2);
		word_t size = tcb->get_mr(i*2 + 1);;
		word_t attr = size >> (BITS_WORD-4);
		size &= (1UL << (BITS_WORD-4)) - 1;

		if (EXPECT_FALSE((attr & 0xe) == 0xa))
		{
		    /* XXX we don't support d-cache invalidate currently
		     * Invalidate MUST be priviledged OR must do carefull checks so that
		     * you can't invalidate cache lines from a different address space
		     */
		    // Check privilege
		    if (! is_privileged_space(get_current_space()))
		    {
			current->set_error_code (ENO_PRIVILEGE);
			goto error_out_preempt;
		    }
		}

		cache_t::flush_range_attribute((addr_t)start, (addr_t)(start+size), attr);
	    }
	    break;
	}
    default:
	current->set_error_code (EINVALID_PARAM);
	goto error_out_preempt;
    }

    preempt_disable();
    return_cache_control(1);

error_out_preempt:
    preempt_disable();
    return_cache_control(0);
}
