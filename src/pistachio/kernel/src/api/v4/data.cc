/*********************************************************************
 *                
 * Copyright (C) 2006,  National ICT Australia
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
#include INC_API(tcb.h)
#include INC_API(smp.h)
#include INC_API(schedule.h)

/* We declare the global data that the kernel uses here, as the number needed will vary depending
 * upon the mp properties of the platform - should this be in arch or glue instead?
 */

ALIGNED(32) scheduler_t scheduler[CONFIG_NUM_DOMAINS] UNIT("cpulocal");

#if defined (CONFIG_MDOMAINS) || defined (CONFIG_MUNITS)
ALIGNED(sizeof(utcb_t))		utcb_t 		__idle_utcb_1	[CONFIG_NUM_UNITS];
ALIGNED(sizeof(whole_tcb_t)) 	whole_tcb_t 	__idle_tcb_1	[CONFIG_NUM_UNITS];

utcb_t *	__idle_utcb	[CONFIG_NUM_DOMAINS] = {__idle_utcb_1};
whole_tcb_t * 	__idle_tcb	[CONFIG_NUM_DOMAINS] = {__idle_tcb_1};
#else
ALIGNED(sizeof(utcb_t))		utcb_t 		__idle_utcb;
ALIGNED(sizeof(whole_tcb_t)) 	whole_tcb_t 	__idle_tcb;
#endif

#if defined (CONFIG_MDOMAINS) || defined (CONFIG_MUNITS)
cpu_mb_t cpu_mailboxes[CONFIG_NUM_UNITS];
#endif

spinlock_t		state_lock[CONFIG_NUM_DOMAINS];

