/****************************************************************************
 *
 * Copyright (C) 2002, Karlsruhe University
 *
 * File path:	glue/v4-powerpc/kip_sc.S
 * Description:	Defines the user space syscall entry points.  
 *		They are prefixes for the actual system calls.  The linker is
 *		responsible for aligning these entry points with their
 *		respective system calls.
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
 * $Id: kip_sc.S,v 1.30 2004/06/04 17:29:41 joshua Exp $
 *
 ***************************************************************************/

#include <l4.h>
#include INC_ARCH(frame.h)
#include INC_GLUE(abi.h)

#include <tcb_layout.h>
#include <asmsyms.h>

#define SC_REG(x)	    (OFS_SYSCALL_##x + EABI_STACK_SIZE)
#define MR_OFFSET(n)	    (OFS_UTCB_MR + 4*(n))
#define USER_MR_OFFSET(n)   (MR_OFFSET(n) - OFS_UTCB_MR)
#define TCB_STATE_OFFSET(r) (TOTAL_TCB_SIZE-SYSCALL_FRAME_SIZE+OFS_SYSCALL_##r)

/**
 * syscall_entry: Switches from user-level to kernel-level for system
 *   calls, and spills relevant user state.
 */
.macro	syscall_entry
	/* Promote privilege level and switch to the kernel stack.	*/
	sc
	/* Save state. */
	mflr	%r27
	stw	%r29, SC_REG(R1)(%r1)	/* Preserve the user sp.	*/
	stw	%r30, SC_REG(R30)(%r1)	/* Preserve r30.		*/
	stw	%r31, SC_REG(R31)(%r1)	/* Preserve r31.		*/
	stw	%r28, SC_REG(SRR1)(%r1)	/* Preserve srr1.		*/
	stw	%r27, SC_REG(SRR0)(%r1)	/* Preserve return address.	*/
	stw	%r2,  SC_REG(R2)(%r1)	/* Preserve r2 (utcb pointer).	*/
.endm

.macro	stack_alloc size
	addi	%r1, %r1, -\size
.endm

/**
 * stack_store: Stores the contents of a register on the stack,
 *   and stores in the register the memory location of the
 *   value on the stack.
 */
.macro	stack_store reg, off
	stw	\reg, \off (%r1)
	addi	\reg, %r1, \off
.endm

/**
 * stack_store2: Stores the contents of a src register on the stack,
 *   and stores in a dst register the memory location of the value
 *   on the stack.
 */
.macro	stack_store2 dst, src, off
	stw	\src, \off (%r1)
	addi	\dst, %r1, \off
.endm

/**
 * grab_sym: Encodes a symbol in the instruction stream as
 *   two 16-bit immediates, and combines into a dst register.
 */
.macro	grab_sym dst, sym
	lis	\dst, \sym @ha
	la	\dst, \sym @l(\dst)
.endm

/**
 * stackify1: Moves a register to the stack, while recording the
 *   stack location in the register.  Allocates space on the stack.
 */
.macro	stackify1 reg1
#if defined(CONFIG_SYSV_ABI)
	stack_alloc	8
	stack_store	\reg1, 8
#endif
.endm

/**
 * stackify2: Moves two registers to the stack, while recording their
 *   stack locations in the registers.  Allocates space on the stack.
 */
.macro	stackify2 reg1, reg2
#if defined(CONFIG_SYSV_ABI)
	stack_alloc	8
	stack_store	\reg1, 8
	stack_store	\reg2, 12
#endif
.endm

/**
 * stackify3: Moves three registers to the stack, while recording their
 *   stack locations in the registers.  Allocates space on the stack.
 */
.macro	stackify3 reg1, reg2, reg3
#if defined(CONFIG_SYSV_ABI)
	stack_alloc	16
	stack_store	\reg1, 8
	stack_store	\reg2, 12
	stack_store	\reg3, 16
#endif
.endm

/**
 * stackify4: Moves four registers to the stack, while recording their
 *   stack locations in the registers.  Allocates space on the stack.
 */
.macro	stackify4 reg1, reg2, reg3, reg4
#if defined(CONFIG_SYSV_ABI)
	stack_alloc	16
	stack_store	\reg1, 8
	stack_store	\reg2, 12
	stack_store	\reg3, 16
	stack_store	\reg4, 20
#endif
.endm


#define KIP_SC_ENTRY(sect_name,label)	\
	.section sect_name ;		\
	.align	2 ;			\
	.globl	_sc_##label ;		\
	_sc_##label :


.macro	spill_register_MRs_to_user_UTCB
	stw	IPC_ABI_MR0, USER_MR_OFFSET(0) (ABI_UTCB)
	stw	IPC_ABI_MR1, USER_MR_OFFSET(1) (ABI_UTCB)
	stw	IPC_ABI_MR2, USER_MR_OFFSET(2) (ABI_UTCB)
	stw	IPC_ABI_MR3, USER_MR_OFFSET(3) (ABI_UTCB)
	stw	IPC_ABI_MR4, USER_MR_OFFSET(4) (ABI_UTCB)
	stw	IPC_ABI_MR5, USER_MR_OFFSET(5) (ABI_UTCB)
	stw	IPC_ABI_MR6, USER_MR_OFFSET(6) (ABI_UTCB)
	stw	IPC_ABI_MR7, USER_MR_OFFSET(7) (ABI_UTCB)
	stw	IPC_ABI_MR8, USER_MR_OFFSET(8) (ABI_UTCB)
	stw	IPC_ABI_MR9, USER_MR_OFFSET(9) (ABI_UTCB)
.endm

/*****************************************************************************/

/* Define the IPC paths. */
#include "fastpath.S"

/*****************************************************************************/

KIP_SC_ENTRY(".sc_schedule", schedule)
	syscall_entry
	stackify1 SCHEDULE_ABI_DEST

/*****************************************************************************/

KIP_SC_ENTRY(".sc_xchg_registers", xchg_registers)

	/* Does the control register mention the pager TID?
	 * If so, we convert it into a global TID.
	 */
	andi.	%r11, EXREG_ABI_CONTROL, 1 << EXREG_ABI_CONTROL_P    /* Check the p-bit of the control register. */
	beq	1f			/* If zero, skip the globalize. */
1:

	syscall_entry
	stackify2 EXREG_ABI_DEST, EXREG_ABI_PAGER

/*****************************************************************************/

KIP_SC_ENTRY(".sc_unmap", unmap)

	/* Fixup the parameters for the C function calling ABI (we are still
	 * in user mode). */
	spill_register_MRs_to_user_UTCB
	mr	%r3, UNMAP_ABI_CONTROL

	/* Enter the kernel and handle the system call. */
	syscall_entry

/*****************************************************************************/

KIP_SC_ENTRY(".sc_memory_ctrl", memory_ctrl)

	/* Fixup the parameters for the C function calling ABI (we are still
	 * in user mode). */
	spill_register_MRs_to_user_UTCB
	mr	%r3, MEM_ABI_CONTROL
	mr	%r4, MEM_ABI_ATTR0
	mr	%r5, MEM_ABI_ATTR1
	mr	%r6, MEM_ABI_ATTR2
	mr	%r7, MEM_ABI_ATTR3

	/* Enter the kernel and handle the system call. */
	syscall_entry

/*****************************************************************************/

KIP_SC_ENTRY(".sc_processor_ctrl", processor_ctrl)
	syscall_entry

/*****************************************************************************/

KIP_SC_ENTRY(".sc_thread_ctrl", thread_ctrl)
	syscall_entry
	stackify4 TCTRL_ABI_DEST, TCTRL_ABI_SPACE, TCTRL_ABI_SCHEDULER, TCTRL_ABI_PAGER

/*****************************************************************************/

KIP_SC_ENTRY(".sc_space_ctrl", space_ctrl)
	syscall_entry
	stackify3 SPACE_ABI_SPACE, SPACE_ABI_KIP, SPACE_ABI_UTCB

/*****************************************************************************/

KIP_SC_ENTRY(".sc_perf", perf)
	syscall_entry
	lwz	%r11, SC_REG(SRR0) (%r1)
	lwz	%r12, SC_REG(SRR1) (%r1)
	lwz	%r31, SC_REG(R31)  (%r1)
	lwz	%r30, SC_REG(R30)  (%r1)
	lwz	%r2,  SC_REG(R2)   (%r1)
	lwz	%r1,  SC_REG(R1)   (%r1)
	mtsrr0	%r11
	mtsrr1	%r12
	rfi

/*****************************************************************************/

KIP_SC_ENTRY(".sc_thread_switch", thread_switch)
	syscall_entry
	stackify1 TSWITCH_ABI_DEST

