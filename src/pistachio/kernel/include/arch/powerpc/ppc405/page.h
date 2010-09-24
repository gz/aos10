/*********************************************************************
 *                
 * Copyright (C) 2002-2004,  University of New South Wales
 * Copyright (C) 2005,  National ICT Australia
 *                
 * File path:     arch/powerpc/ppc405/page.h
 * Created:       11/12/2005 by Carl van Schaik
 * Description:   PPC405 MMU defines
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

#ifndef __ARCH__POWERPC__PPC405__PAGE_H__
#define __ARCH__POWERPC__PPC405__PAGE_H__

/* HW_PGSHIFTS must match pgsize_e (pgent.h)
 * This specifies all page/level sizes in the pagetable */
#define CPU_HW_PAGESHIFTS	{   \
    PAGE_BITS_1K, PAGE_BITS_4K, PAGE_BITS_16K, PAGE_BITS_256K,	\
    PAGE_BITS_1M, PAGE_BITS_4M, PAGE_BITS_16M, PAGE_BITS_4G }

#define	HW_NUM_PAGESHIFTS   8


#define MMU_OPTIMIZED_1K

#ifdef MMU_OPTIMIZED_1K

/* 32-bit Pagetable structure
 *
 * (32) 4G	[ 64 entries (6-bits)]
 *  (24) 16M	[ 64 entries (6-bits)]
 *   (20) 1M	[ 64 entries (6-bits)]
 *    (14) 16k	[ 16 entries (4-bits)]
 *     (10) 1k	[    entry only	     ]
 */

#else

/* 32-bit Pagetable structure
 *
 * (32) 4G	[ 64 entries (6-bits)]
 *  (24) 16M	[ 64 entries (6-bits)]
 *   (20) 1M	[256 entries (8-bits)]
 *    (12) 4k	[  4 entries (2-bits)]
 *     (10) 1k	[    entry only	     ]
 */

#endif

/* HW_VALID_PGSIZES specifies only pagesizes valid for
 * the CPUs MMU */
#define CPU_HW_VALID_PGSIZES	(   \
	PAGE_SIZE_1K | PAGE_SIZE_4K | PAGE_SIZE_16K | PAGE_SIZE_256K |	\
	PAGE_SIZE_1M | PAGE_SIZE_4M | PAGE_SIZE_16M )

/* Mapping database supported page sizes */
#define CPU_MDB_PGSHIFTS	CPU_HW_PAGESHIFTS
#define CPU_MDB_NUM_PGSIZES	(HW_NUM_PAGESHIFTS-1)


#endif /* __ARCH__POWERPC__PPC405__PAGE_H__ */
