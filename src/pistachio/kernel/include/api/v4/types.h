/*********************************************************************
 *                
 * Copyright (C) 2002-2003,  Karlsruhe University
 *                
 * File path:     api/v4/types.h
 * Description:   General type declarations for V4 API
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
 * $Id: types.h,v 1.24 2003/09/24 19:04:24 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __API__V4__TYPES_H__
#define __API__V4__TYPES_H__

#include INC_API(config.h)

extern u64_t get_current_time();

typedef u16_t execution_unit_t;
typedef u16_t scheduler_domain_t;

#if defined (CONFIG_MDOMAINS) || defined(CONFIG_MUNITS)
class cpu_context_t
{	
public:
	inline void operator= (cpu_context_t a) { raw = a.raw; }
	inline bool operator== (const cpu_context_t rhs)	{ return (raw == rhs.raw); }
	inline bool operator!= (const cpu_context_t rhs)	{ return (raw != rhs.raw); }

	union {
	    struct {
		execution_unit_t unit;
		scheduler_domain_t domain;
	    };
	    word_t raw;
	};

	cpu_context_t root_context() 
	{
	    cpu_context_t root;
	    root.domain = domain;
	    root.unit = 0xFFFF;
	    return root;
	}
} ;
#else
typedef word_t cpu_context_t; 
#endif



#endif /* __API__V4__TYPES_H__ */
