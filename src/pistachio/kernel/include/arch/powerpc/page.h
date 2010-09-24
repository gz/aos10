/*********************************************************************
 *                
 * Copyright (C) 2002-2004,  University of New South Wales
 * Copyright (C) 2005,  National ICT Australia
 *                
 * File path:     arch/powerpc/page.h
 * Created:       11/12/2005 by Carl van Schaik
 * Description:   Generic PowerPC MM defines
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

#ifndef __ARCH__POWERPC__PAGE_H__
#define __ARCH__POWERPC__PAGE_H__

#include <config.h>
#include INC_CPU(page.h)

/* HW_PGSHIFTS must match pgsize_e (pgent.h)
 * This specifies all page/level sizes in the pagetable */
#define HW_PGSHIFTS		CPU_HW_PAGESHIFTS

/* HW_VALID_PGSIZES specifies only pagesizes valid for
 * the CPUs MMU */
#define HW_VALID_PGSIZES	CPU_HW_VALID_PGSIZES

/* Mapping database supported page sizes */
#define MDB_PGSHIFTS		CPU_MDB_PGSHIFTS
#define MDB_NUM_PGSIZES		CPU_MDB_NUM_PGSIZES

/* Basic page sizes etc. */
#define PAGE_BITS_1K		(10)
#define PAGE_BITS_2K		(11)
#define PAGE_BITS_4K		(12)
#define PAGE_BITS_8K		(13)
#define PAGE_BITS_16K		(14)
#define PAGE_BITS_32K		(15)
#define PAGE_BITS_64K		(16)
#define PAGE_BITS_128K		(17)
#define PAGE_BITS_256K		(18)
#define PAGE_BITS_512K		(29)
#define PAGE_BITS_1M		(20)
#define PAGE_BITS_2M		(21)
#define PAGE_BITS_4M		(22)
#define PAGE_BITS_8M		(23)
#define PAGE_BITS_16M		(24)
#define PAGE_BITS_32M		(25)
#define PAGE_BITS_64M		(26)
#define PAGE_BITS_128M		(27)
#define PAGE_BITS_256M		(28)
#define PAGE_BITS_512M		(29)
#define PAGE_BITS_1G		(30)
#define PAGE_BITS_2G		(31)
#define PAGE_BITS_4G		(32)

#define PAGE_SIZE_1K		(1ul << PAGE_BITS_1K)
#define PAGE_SIZE_2K		(1ul << PAGE_BITS_2K)
#define PAGE_SIZE_4K		(1ul << PAGE_BITS_4K)
#define PAGE_SIZE_8K		(1ul << PAGE_BITS_8K)
#define PAGE_SIZE_16K		(1ul << PAGE_BITS_16K)
#define PAGE_SIZE_32K		(1ul << PAGE_BITS_32K)
#define PAGE_SIZE_64K		(1ul << PAGE_BITS_64K)
#define PAGE_SIZE_128K		(1ul << PAGE_BITS_128K)
#define PAGE_SIZE_256K		(1ul << PAGE_BITS_256K)
#define PAGE_SIZE_512K		(1ul << PAGE_BITS_512K)
#define PAGE_SIZE_1M		(1ul << PAGE_BITS_1M)
#define PAGE_SIZE_2M		(1ul << PAGE_BITS_2M)
#define PAGE_SIZE_4M		(1ul << PAGE_BITS_4M)
#define PAGE_SIZE_8M		(1ul << PAGE_BITS_8M)
#define PAGE_SIZE_16M		(1ul << PAGE_BITS_16M)
#define PAGE_SIZE_32M		(1ul << PAGE_BITS_32M)
#define PAGE_SIZE_64M		(1ul << PAGE_BITS_64M)
#define PAGE_SIZE_128M		(1ul << PAGE_BITS_128M)
#define PAGE_SIZE_256M		(1ul << PAGE_BITS_256M)
#define PAGE_SIZE_512M		(1ul << PAGE_BITS_512M)
#define PAGE_SIZE_1G		(1ul << PAGE_BITS_1G)
#define PAGE_SIZE_2G		(1ul << PAGE_BITS_2G)

#define PAGE_MASK_1K		(~(PAGE_SIZE_1K - 1))
#define PAGE_MASK_2K		(~(PAGE_SIZE_2K - 1))
#define PAGE_MASK_4K		(~(PAGE_SIZE_4K - 1))
#define PAGE_MASK_8K		(~(PAGE_SIZE_8K - 1))
#define PAGE_MASK_16K		(~(PAGE_SIZE_16K - 1))
#define PAGE_MASK_32K		(~(PAGE_SIZE_32K - 1))
#define PAGE_MASK_64K		(~(PAGE_SIZE_64K - 1))
#define PAGE_MASK_128K		(~(PAGE_SIZE_128K - 1))
#define PAGE_MASK_256K		(~(PAGE_SIZE_256K - 1))
#define PAGE_MASK_512K		(~(PAGE_SIZE_512K - 1))
#define PAGE_MASK_1M		(~(PAGE_SIZE_1M - 1))
#define PAGE_MASK_2M		(~(PAGE_SIZE_2M - 1))
#define PAGE_MASK_4M		(~(PAGE_SIZE_4M - 1))
#define PAGE_MASK_8M		(~(PAGE_SIZE_8M - 1))
#define PAGE_MASK_16M		(~(PAGE_SIZE_16M  - 1))
#define PAGE_MASK_32M		(~(PAGE_SIZE_32M  - 1))
#define PAGE_MASK_64M		(~(PAGE_SIZE_64M  - 1))
#define PAGE_MASK_128M		(~(PAGE_SIZE_128M - 1))
#define PAGE_MASK_256M		(~(PAGE_SIZE_256M - 1))
#define PAGE_MASK_512M		(~(PAGE_SIZE_512M - 1))
#define PAGE_MASK_1G		(~(PAGE_SIZE_1G - 1))
#define PAGE_MASK_2G		(~(PAGE_SIZE_2G - 1))

#endif /* __ARCH__POWERPC__PAGE_H__ */
