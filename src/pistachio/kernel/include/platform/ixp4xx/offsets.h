/*********************************************************************
 *
 * Copyright (C) 2004,  National ICT Australia (NICTA)
 *
 * File path:     platform/ixp4xx/offsets.h
 * Description:   Offsets for IXP4XX
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

#ifndef __PLATFORM__IXP4XX__OFFSETS_H__
#define __PLATFORM__IXP4XX__OFFSETS_H__

#define		PHYS_ADDR_BASE		(CONFIG_RAMSTART + 0x00100000)

#define		XSCALE_DEV_PHYS		0xc8000000

/* These must correspond with glue/v4-arm/config.h */
/* VIRT_ADDR_BASE must be 0xF0000000 */
#define		VIRT_ADDR_BASE		0xF0000000
#define		VIRT_ADDR_RAM		0xF0000000
/* UNCACHED_ADDR_BASE must be 0xF4000000 */
#define		PGTABLE_ADDR_BASE	0xF4000000
#define		KERNEL_RW_OFFSET	(VIRT_ADDR_BASE-PHYS_ADDR_BASE)
#define		KERNEL_RO_OFFSET	(VIRT_ADDR_BASE-PHYS_ADDR_BASE)
#define		PGTABLE_OFFSET		(PGTABLE_ADDR_BASE-PHYS_ADDR_BASE)

#endif /*__PLATFORM__IXP4XX__OFFSETS_H__*/
