/****************************************************************************
 *                
 * Copyright (C) 2002, Karlsruhe University
 * Copyright (C) 2005, National ICT Australia
 *                
 * File path:	arch/powerpc/ppc506/msr.h
 * Description:	Macros which describe the Machine State Register, and
 * 		functions which manipulate the register for PPC405.
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
 * $Id: msr.h,v 1.14 2003/09/24 19:04:30 skoglund Exp $
 *
 ***************************************************************************/

#ifndef __ARCH__POWERPC__PPC405__MSR_H__
#define __ARCH__POWERPC__PPC405__MSR_H__

#ifdef ASSEMBLY
#define __MASK(x)       (1<<(x))
#else
#define __MASK(x)       (1ul<<(x))
#endif

#define MSR_AP_BIT	25	/* auxiliary processor available    */
#define MSR_APE_BIT	19	/* APU exception enable		    */
#define MSR_WE_BIT	18	/* wait state enable		    */
#define MSR_CE_BIT	17	/* critical interrupt enable	    */
#define MSR_EE_BIT	15	/* extern interrupt enable	    */
#define MSR_PR_BIT	14	/* privilege level		    */
#define MSR_FP_BIT	13	/* floating point available	    */
#define MSR_ME_BIT	12	/* machine check enable		    */
#define MSR_FE0_BIT	11	/* floating point exception mode 0  */
#define MSR_DWE_BIT	10	/* debug wait enable 		    */
#define MSR_DE_BIT	 9	/* debug interrupts enable	    */
#define MSR_FE1_BIT	 8	/* floating point exception mode 1  */
#define MSR_IR_BIT	 5	/* instruction address translation  */
#define MSR_DR_BIT	 4	/* data address translation	    */

#define MSR_AP		__MASK(MSR_AP_BIT)	/* auxiliary processor available    */
#define MSR_APE		__MASK(MSR_APE_BIT)	/* APU exception enable		    */
#define MSR_WE		__MASK(MSR_WE_BIT)	/* wait state enable		    */
#define MSR_CE		__MASK(MSR_CE_BIT)	/* critical interrupt enable	    */
#define MSR_EE		__MASK(MSR_EE_BIT)	/* extern interrupt enable	    */
#define MSR_PR		__MASK(MSR_PR_BIT)	/* privilege level		    */
#define MSR_FP		__MASK(MSR_FP_BIT)	/* floating point available	    */
#define MSR_ME		__MASK(MSR_ME_BIT)	/* machine check enable		    */
#define MSR_FE0		__MASK(MSR_FE0_BIT)	/* floating point exception mode 0  */
#define MSR_DWE		__MASK(MSR_DWE_BIT)	/* debug wait enable 		    */
#define MSR_DE		__MASK(MSR_DE_BIT)	/* debug interrupts enable	    */
#define MSR_FE1		__MASK(MSR_FE1_BIT)	/* floating point exception mode 1  */
#define MSR_IR		__MASK(MSR_IR_BIT)	/* instruction address translation  */
#define MSR_DR		__MASK(MSR_DR_BIT)	/* data address translation	    */

#define MSR_AP_AVAILABLE    MSR_AP

#define MSR_APE_DISABLED    0
#define MSR_APE_ENABLED	    MSR_APE

#define MSR_CE_DISABLED	    0
#define MSR_CE_ENABLED	    MSR_CE

#define MSR_EE_DISABLED	    0
#define MSR_EE_ENABLED	    MSR_EE

#define MSR_PR_KERNEL	    0
#define MSR_PR_USER	    MSR_PR

#define MSR_FP_DISABLED	    0
#define MSR_FP_ENABLED	    MSR_FP

#define MSR_ME_DISABLED	    0
#define MSR_ME_ENABLED	    MSR_ME

#define MSR_DE_DISABLED	    0
#define MSR_DE_ENABLED	    MSR_DE

#define MSR_IR_DISABLED	    0
#define MSR_IR_ENABLED	    MSR_IR

#define MSR_DR_DISABLED	    0
#define MSR_DR_ENABLED	    MSR_DR

#define MSR_KERNEL_INIT (MSR_PR_KERNEL	    \
	| MSR_APE_DISABLED  | MSR_FP_DISABLED	| MSR_CE_ENABLED    | MSR_EE_DISABLED	\
	| MSR_ME_ENABLED    | MSR_DE_DISABLED	| MSR_IR_ENABLED    | MSR_DR_ENABLED)

#define MSR_KERNEL_MODE (MSR_KERNEL_INIT)

#define MSR_USER_MODE	(MSR_PR_USER	    \
	| MSR_APE_DISABLED  | MSR_FP_DISABLED	| MSR_CE_ENABLED    | MSR_EE_ENABLED	\
	| MSR_ME_ENABLED    | MSR_DE_DISABLED	| MSR_IR_ENABLED    | MSR_DR_ENABLED)

#define MSR_REAL_MODE	(MSR_PR_KERNEL	    \
	| MSR_APE_DISABLED  | MSR_FP_DISABLED	| MSR_CE_ENABLED    | MSR_EE_DISABLED	\
	| MSR_ME_ENABLED    | MSR_DE_DISABLED	| MSR_IR_DISABLED   | MSR_DR_DISABLED)

#define MSR_USER_MASK	(MSR_FE0 | MSR_FE1)


#ifndef ASSEMBLY
INLINE word_t ppc_get_msr( void )
{
    word_t msr;
    asm volatile("mfmsr %0" : "=r" (msr) );
    return msr;
}

INLINE void ppc_set_msr( word_t msr )
{
    asm volatile("mtmsr %0" : : "r" (msr) );
}

INLINE bool ppc_is_real_mode( word_t msr )
{
    return !((msr & MSR_IR_ENABLED) || (msr & MSR_DR_ENABLED));
}

INLINE bool ppc_is_kernel_mode( word_t msr )
{
    return ((msr & MSR_PR) == MSR_PR_KERNEL);
}

INLINE void ppc_enable_interrupts()
{
    word_t msr = ppc_get_msr();
    ppc_set_msr( msr | MSR_EE );
}

INLINE void ppc_disable_interrupts()
{
    word_t msr = ppc_get_msr();
    ppc_set_msr( msr & (~MSR_EE) );
}
#endif	/* ASSEMBLY */

#endif	/* __ARCH__POWERPC__PPC405__MSR_H__ */

