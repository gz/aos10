/*********************************************************************
 *                
 * Copyright (C) 2005,  National ICT Australai
 *                
 * File path:     l4/map.h
 * Description:   Interfaces for mappings in map control
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
#ifndef __L4__MAP_H__
#define __L4__MAP_H__

#include <l4/types.h>

#if defined(L4_64BIT)
# define __PLUS32	+ 32
#else
# define __PLUS32
#endif

/*
 * Physical descriptor from MapControl item
 */
typedef union {
    L4_Word_t	raw;
    struct {
	L4_BITFIELD2( L4_Word_t,
		attr	: 6,
		base	: 26 __PLUS32
	);
    } X;
} L4_PhysDesc_t;

L4_INLINE L4_PhysDesc_t L4_PhysDesc (L4_Word64_t PhysAddr, L4_Word_t Attribute)
{
    L4_PhysDesc_t phys;

    phys.X.base = PhysAddr >> 10;
    phys.X.attr = Attribute;
    return phys;
}

/*
 * MapControl controls
 */
#define L4_MapCtrl_Read		(1UL << (L4_BITS_PER_WORD - 2))
#define L4_MapCtrl_Modify	(1UL << (L4_BITS_PER_WORD - 1))

#undef __PLUS32

#endif /* !__L4__MAP_H__ */
