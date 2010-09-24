/****************************************************************************
 *                
 * Copyright (C) 2002, 2003, Karlsruhe University
 * Copyright (C) 2005, National ICT Australia
 *                
 * File path:	glue/v4-powerpc/config.h
 * Description:	Configuration of the PowerPC architecture.
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
 * $Id: config.h,v 1.41 2003/10/29 09:05:51 joshua Exp $
 *
 ***************************************************************************/

#ifndef __GLUE__V4_POWERPC__CONFIG_H__
#define __GLUE__V4_POWERPC__CONFIG_H__

#include INC_PLAT(offsets.h)
#include INC_ARCH(cache.h)
#include INC_ARCH(page.h)
#include INC_CPU(tlb.h)

#define CACHE_LINE_SIZE		(POWERPC_CACHE_LINE_SIZE)

//#define KERNEL_PAGE_SIZE	(POWERPC_PAGE_SIZE)

#define KTCB_BITS		(11)
#define KTCB_SIZE		(1 << KTCB_BITS)
#define KTCB_MASK		(~(KTCB_SIZE-1))

#define KTCB_AREA_BITS		28	/* 256 MB for all KTCBs */
#define VALID_THREADNO_BITS	(KTCB_AREA_BITS - KTCB_BITS)
#define KTCB_THREADNO_MASK	((1 << VALID_THREADNO_BITS)-1)

#define UTCB_BITS		8	/* 256 byte UTCBs */
#define UTCB_SIZE		(1 << UTCB_BITS)
#define UTCB_MASK		(~(UTCB_SIZE - 1))

/* At some stage make UTCB pagesize configurable */
#if ((PAGE_SIZE_4K & HW_VALID_PGSIZES) == 0)
#error "FIXME: CPU does not support 4k pages."
#else
#define KIP_UTCB_SIZE		(PAGE_BITS_4K)
#define UTCB_AREA_PAGE_SIZE	(PAGE_SIZE_4K)
#endif

/**
 * number of message registers
 */
#define IPC_NUM_MR		32

/****************************************************************************
 *
 * Division of the kernel's 1 gig address space.
 *
 ****************************************************************************/

/* 224MB area for kernel code and data		*/
#define KERNEL_AREA_START	(KERNEL_OFFSET)
#define KERNEL_AREA_SIZE	0x0E000000
#define KERNEL_AREA_END		(KERNEL_AREA_START + KERNEL_AREA_SIZE)

/* 32MB area for devices (eg: PIC, serial)	*/
#define DEVICE_AREA_START	(KERNEL_AREA_END)
#define DEVICE_AREA_SIZE	0x02000000
#define DEVICE_AREA_END		(DEVICE_AREA_START + DEVICE_AREA_SIZE)

#if ((DEVICE_AREA_END - KERNEL_AREA_START) != 0x10000000)
#error "Kernel/device area size changed."
#endif

/* Machine specific memory region provided	*/
#define PPC_MACH_AREA_START	DEVICE_AREA_END
/* PPC_MACH_AREA_SIZE	provided by platform/cpu */
#ifndef PPC_MACH_AREA_SIZE
#define PPC_MACH_AREA_SIZE	0
#endif
#define PPC_MACH_AREA_END	(PPC_MACH_AREA_START + PPC_MACH_AREA_SIZE)

#if ((PPC_MACH_AREA_END & ((1 << KTCB_AREA_BITS)-1)) != 0)
#error "PPC_MACH_AREA_END incorrectly aligned."
#endif

#define KTCB_AREA_START		(PPC_MACH_AREA_END)
#define KTCB_AREA_SIZE		(1 << KTCB_AREA_BITS)
#define KTCB_AREA_END		(KTCB_AREA_START + KTCB_AREA_SIZE)

#if (KTCB_AREA_END != 0x100000000)
# error "KTCB_AREA_END is not at 4GB."
#endif

/****************************************************************************
 *
 * Division of the user's address space.
 *
 ****************************************************************************/

#define USER_AREA_START		0x00000000
#define USER_AREA_END		(KERNEL_OFFSET)

#define ROOT_UTCB_START		(USER_AREA_END - 0x20000)
#define ROOT_KIP_START		(USER_AREA_END - 0x10000)

/****************************************************************************
 *
 * KIP configuration.
 *
 ****************************************************************************/

/* big endian, 32-bit */
#define KIP_API_FLAGS		{SHUFFLE2(endian:1, word_size:0)}
#define KIP_UTCB_INFO		{SHUFFLE3(multiplier:1, alignment:UTCB_BITS, size:KIP_UTCB_SIZE)}
/* kip */
#if ((PAGE_SIZE_4K & HW_VALID_PGSIZES) == 0)
#error "FIXME: CPU does not support 4k pages."
#endif

#define KIP_KIP_AREA		PAGE_BITS_4K
#define KIP_ARCH_PAGEINFO	{SHUFFLE2(rwx:PAGE_PROT, size_mask:HW_VALID_PGSIZES >> 10)}
#define KIP_MIN_MEMDESCS	(16)

/****************************************************************************
 *
 * Other.
 *
 ****************************************************************************/

#include INC_PLAT(timer.h)

/* The special pupose register for the cpu spill. */
#define SPRG_CPU		0
/* The special purpose register for the current TCB. */
#define SPRG_CURRENT_TCB	1
/* Temporary special purpose registers. */
#define SPRG_TMP0		2
#define SPRG_TMP1		3

#endif /* !__GLUE__V4_POWERPC__CONFIG_H__ */
