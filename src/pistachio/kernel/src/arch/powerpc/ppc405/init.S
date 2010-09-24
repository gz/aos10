/*********************************************************************
 *
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *
 * File path:     arch/powerpc/ppc405/init.S
 * Description:   IBM PPC405 startup code
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
#include INC_CPU(msr.h)
#include INC_CPU(regs.h)
#include INC_GLUE(asm.h)
#include INC_ARCH(regdef.h)

	.section .init

	.globl	_start
_start:
	grab_sym	sp,	init_stack_top	/* Setup stack			*/
	grab_sym	r11,	KERNEL_OFFSET	/* Get the kernel offset	*/

	sub	sp,	sp,	r11		/* We are will run in real mode, adjust stack */

	grab_sym	r3,	_realmode_start	/* Get address to jump to	*/
	sub	r3,	r3,	r11		/* Sub vitual address offset	*/

	grab_sym    r4,	MSR_REAL_MODE		/* Get the real-mode MSR	*/

	mtsrr0	r3				/* Set address to return to	*/
	mtsrr1	r4				/* Set MSR to load on return	*/

	rfi					/* Return to real-mode		*/

	/* From here we are sure we are in real-mode */
_realmode_start:
	subi	sp,	sp,	20		/* Make stack frame		*/

	li	r3,	0
	mtspr	SPR_SGR,	r3		/* Disable guarded storage (for perf)	*/
	mtspr	SPR_SLER,	r3		/* Set CPU to big endian	*/
	mtspr	SPR_SU0R,	r3		/* Disable storage compression	*/

	/* Invalidate and disable i-cache */
	iccci	r0,	r3			/* invalidate i-cache		*/

	li	r3,	0
	mtspr	SPR_ICCR,	r3		/* disable real-mode caching	*/
	isync					/* synchronize			*/

	/* Invalidate and disable d-cache */
	mfspr	r10,	SPR_PVR			/* Get PVR for cache size	*/
	extrwi	r10,	r10,	6,	16	/* Extract PVR:CAS		*/
	// XXX can't find sufficient docs on CAS, assuming 32k cache to be safe

	li	r3,	0
	mtspr	SPR_DCCR,   r3			/* Disable data cache		*/
	mtspr	SPR_DCWR,   r3			/* Reset to write-back		*/

	li	r3,	0
	li	r10,	(32768/POWERPC_CACHE_WAYS/POWERPC_CACHE_LINE_SIZE)
	mtctr	r10
idc_loop:
	dccci	r0,	r3			/* invalidate line		*/
	addi	r3,	r3,	POWERPC_CACHE_LINE_SIZE
	bdnz	idc_loop
	sync

// XXX SPR init


	/* Call L4 init code */
	bl	l4_powerpc_init

	/* Spin forever */
1:
	b	1b

	.section .init.stack
init_stack_bot:
	.space	0x800
init_stack_top:
