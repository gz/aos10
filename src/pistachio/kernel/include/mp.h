/*********************************************************************
 *                
 * Copyright (C) 2002-2003,  Karlsruhe University
 *                
 * File path:     mp.h
 * Description:   Multi Processor support
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
 * $Id: intctrl.h,v 1.9 2003/09/24 19:04:24 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __GENERIC__MP_H__
#define __GENERIC__MP_H__
#include <debug.h>	/* for UNIMPLEMENTED() */

/* 
 * This class is:
 *  - an interface description
 *  - a porting helper, the real mp_t can be derived from it
 *   
 *   NOTE: If we are not building an MP kernel, then we instansiate a empty version, with
 *   no over-ridden members that will give us uniprocessor support by default
 */
#include INC_API(types.h)

static int generic_execution_units[1] = {1};

class generic_mp_t {
 public:

    /* Does what ever is necessary to find the amount of packages/cores/hwthreads. */
    void probe() { 
	execution_units = generic_execution_units;
    }

    void print_info(){
	printf("domain pairs:");	
	for(int i = 0; i < 1; i++)
	    printf(" (%d, %d)", i, execution_units[i]);
	printf("\n");
    }
    
    /* These may be hardcoded for a particular platform or include specific code to find the corerct values */
    inline int get_num_scheduler_domains()	{ return 1; }
    
    inline void startup_context(cpu_context_t context)	{  }
    inline void stop_context(cpu_context_t context)	{  }
    inline void interrupt_context(cpu_context_t c) {printf("NILL INT CONTEXT\n");}
    
 protected:
    int * execution_units;
};


#if defined(CONFIG_MDOMAINS) || defined(CONFIG_MUNITS)
/* If we are a multi processing system of any variety, include the platform specific version to override the above
 * implementation
 *
 */
	#include INC_PLAT(mp.h)

#define ON_CONFIG_MDOMAINS(x) do { x; } while(0)
#define ON_CONFIG_MUNITS(x) do { x; } while(0)

#else

/* Handle the single processor case */
inline cpu_context_t get_current_context(){
	return 0;
}

class mp_t : public generic_mp_t {};

#define ON_CONFIG_MDOMAINS(x) 
#define ON_CONFIG_MUNITS(x) 

#endif

inline mp_t* get_mp() { 
	extern mp_t mp;
	return &mp; 
}




#endif /* !__GENERIC__MP_H__ */
