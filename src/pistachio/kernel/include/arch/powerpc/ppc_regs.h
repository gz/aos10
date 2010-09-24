/*********************************************************************
 *                
 * Copyright (C) 2002, Karlsruhe University
 * Copyright (C) 2005, National ICT Australia
 *                
 * File path:     arch/powerpc/ppc_regs.h
 * Created:       10/12/2005 by Carl van Schaik
 * Description:   PowerPC Register Definitions
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
 * $Id: regdef.h,v 1.5 2003/09/24 19:04:29 skoglund Exp $
 *                
 ********************************************************************/

#ifndef __ARCH__POWERPC__PPC_REGS_H__
#define __ARCH__POWERPC__PPC_REGS_H__

#include INC_CPU(regs.h)

#ifndef ASSEMBLY

/* Functions to access SPRs from C */

INLINE word_t ppc_get_sprg( const int which )
{
    word_t val;
    asm volatile( "mfsprg %0, %1" : "=r" (val) : "i" (which) );
    return val;
}

INLINE void ppc_set_sprg( const int which, word_t val )
{
    asm volatile( "mtsprg %0, %1" : : "i" (which), "r" (val) );
}

INLINE void ppc_set_srr0( word_t val )
{
    asm volatile( "mtsrr0 %0" : : "r" (val) );
}

INLINE void ppc_set_srr1( word_t val )
{
    asm volatile( "mtsrr1 %0" : : "r" (val) );
}

INLINE word_t ppc_get_srr0( void )
{
    word_t val;
    asm volatile( "mfsrr0 %0" : "=r" (val) );
    return val;
}

INLINE word_t ppc_get_srr1( void )
{
    word_t val;
    asm volatile( "mfsrr1 %0" : "=r" (val) );
    return val;
}

INLINE word_t ppc_get_tbl( void )
{
    word_t val;
    asm volatile( "mftbl %0" : "=r" (val) );
    return val;
}

INLINE void ppc_set_tbl( word_t val )
{
    asm volatile( "mttbl %0" : : "r" (val) );
}

INLINE word_t ppc_get_tbu( void )
{
    word_t val;
    asm volatile( "mftbu %0" : "=r" (val) );
    return val;
}

INLINE void ppc_set_tbu( word_t val )
{
    asm volatile( "mttbu %0" : : "r" (val) );
}

#endif /* !ASSEMBLY */

#endif /* __ARCH__POWERPC__PPC_REGS_H__ */
