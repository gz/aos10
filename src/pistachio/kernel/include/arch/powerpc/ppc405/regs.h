/****************************************************************************
 *                
 * Copyright (C) 2005, National ICT Australia
 *                
 * File path:	arch/powerpc/ppc405/regs.h
 * Description:	CPU specific registers inclusing SPR register numbers.
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
 * $Id: ppc_registers.h,v 1.7 2003/09/24 19:04:30 skoglund Exp $
 *
 ***************************************************************************/

#ifndef __ARCH__POWERPC__PPC405__REGS_H__
#define __ARCH__POWERPC__PPC405__REGS_H__

/*
 * SPRN - SPR access numbers
 */
#define SPR_XER		1
#define SPR_LR		8
#define SPR_CTR		9
#define SPR_SRR0	26
#define SPR_SRR1	27
#define SPR_SPRG4u	260
#define SPR_SPRG5u	261
#define SPR_SPRG6u	262
#define SPR_SPRG7u	263
#define SPR_SPRG0	272
#define SPR_SPRG1	273
#define SPR_SPRG2	274
#define SPR_SPRG3	275
#define SPR_SPRG4	276
#define SPR_SPRG5	277
#define SPR_SPRG6	278
#define SPR_SPRG7	279
#define SPR_PVR		287
#define SPR_MCSR	572
#define SPR_CCR1	888
#define SPR_PID		945
#define SPR_CCR0	947
#define SPR_IAC3	948
#define SPR_IAC4	949
#define SPR_DVC1	950
#define SPR_DVC1	951
#define SPR_SGR		953
#define SPR_DCWR	954
#define SPR_SLER	955
#define SPR_SU0R	956
#define SPR_DBCR1	957
#define SPR_ICDBDR	979
#define SPR_ESR		980
#define SPR_DEAR	981
#define SPR_EVPR	982
#define SPR_PIT		987
#define SPR_DBSR	1008
#define SPR_DBCR0	1010
#define SPR_IAC1	1012
#define SPR_IAC2	1013
#define SPR_DAC1	1014
#define SPR_DAC2	1015
#define SPR_DCCR	1018
#define SPR_ICCR	1019

#ifndef ASSEMBLY

#endif	/* ASSEMBLY */

#endif /* __ARCH__POWERPC__PPC405__REGS_H__ */

