
/********************************************************************* *                
 *
 * Copyright (C) 2003-2004,  National ICT Australia (NICTA)
 *                
 * File path:     generic/init.cc
 * Description:   
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
#include <debug.h>



/**
 * Entry point from ASM into C kernel
 * Precondition: MMU and page tables are disabled
 * Warning: Do not use local variables in startup_system()
 */


#include INC_API(kernelinterface.h)
#include INC_API(tcb.h)
#include INC_API(smp.h)
#include INC_API(schedule.h)
#include INC_API(interrupt.h)
#include INC_GLUE(space.h)
#include INC_GLUE(memory.h)
#include INC_GLUE(intctrl.h)
#include INC_GLUE(config.h)
#include INC_GLUE(hwspace.h)
#include INC_GLUE(timer.h)

#include <mp.h>
mp_t mp;

//#define TRACE_INIT printf

extern void idle_thread();
extern void init_all_threads();
#if defined(ARCH_MIPS64)
extern void SECTION (".init") finalize_cpu_init (word_t cpu_id);
#endif

void create_idle_thread(){
    get_idle_tcb()->create_kernel_thread(NILTHREAD, get_idle_utcb());

    /* set idle-magic */
    get_idle_tcb()->myself_global.set_raw((word_t)0x1d1e1d1e1d1e1d1eULL);
	thread_state_lock();
	get_idle_tcb()->set_state(thread_state_t::running);
	thread_state_unlock();
}


#if defined(CONFIG_MUNITS)

/* We set this flag here, since now we are running on the idle thread's (unique) stack
 * and this avoids a race where the next context is brought up and starts using the boot
 * stack before we are finished with it.
 */
void  SECTION (".init") context_stub(void){
	get_mp()->context_initialized = true;
}


/* This is the c language entrypoint after arch specific assembler routines bootstrap a context  */
extern "C" void SECTION (".init") startup_context (void)
{
    cpu_context_t context = get_current_context();
    
    create_idle_thread();
    get_idle_tcb()->notify(idle_thread);
    get_idle_tcb()->notify(context_stub);
    
    
    /* initialize the kernel's timer source */
    get_timer()->init_cpu();
    init_xcpu_handling (context);

//    get_current_scheduler()->init (false);
    get_current_scheduler()->start (context);

    /* make sure we don't fall off the edge */
    spin_forever(1);
}
#endif

extern "C" void SECTION(".init") startup_scheduler()
{
    TRACE_INIT("Initialising multiprocessing support...\n");
    /* initialise the mp class */
    get_mp()->probe();
    get_mp()->print_info();

 
#if defined(CONFIG_MDOMAINS) || defined(CONFIG_MUNITS)
    init_xcpu_handling (get_current_context());
#endif

    TRACE_INIT("Initialising scheduler(s)/context(s)...\n");
    extern scheduler_t scheduler[];	// ewww...
    for(int i = 0; i < get_mp()->get_num_scheduler_domains(); i++){
	scheduler[i].init(false);
    }

    create_idle_thread();
    get_idle_tcb ()->notify (idle_thread);

#if defined(CONFIG_MDOMAINS) || defined(CONFIG_MUNITS)
    /* initialize other processors */
    get_mp()->init_other_contexts();
#endif    

    get_idle_tcb()->notify (init_all_threads);

#ifdef ARCH_MIPS64
    get_idle_tcb ()->notify (finalize_cpu_init, 0);
#endif

    /* get the thing going - we should never return */
    get_current_scheduler()->start(get_current_context());

    ASSERT(ALWAYS, !"Scheduler start fell through!!!\n");
    
    /* make sure we don't fall off the edge */
    spin_forever(1);
}


