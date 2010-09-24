/*********************************************************************
 *
 * Copyright (C) 2003-2005,  National ICT Australia (NICTA)
 *
 * File path:     arch/arm/thread.h
 * Description:   Thread switch and interrupt stack frames
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
 * $Id: thread.h,v 1.15 2004/12/02 00:13:52 cvansch Exp $
 *
 ********************************************************************/

#ifndef __ARCH__ARM__THREAD_H__
#define __ARCH__ARM__THREAD_H__

#define ARM_SYSCALL_STACK_SIZE	16
#define ARM_USER_FLAGS_MASK	UL(0xf8000020)

#define CPSR_MODE_MASK		0x1f
#define CPSR_USER_MODE		0x10
#define CPSR_THUMB_BIT		0x20
#define CPSR_I_BIT		0x80
#define CPSR_F_BIT		0x40


#if !defined(__ASSEMBLER__)

class arm_switch_stack_t {
    public:
	u32_t r4;   /* -0 - */
	u32_t r5;   /*  4   */
	u32_t r11;  /*  8   */
	u32_t lr;   /*  12  */
};
#endif

#define SS_R4	    0
#define SS_R5	    4
#define SS_R11	    8
#define SS_LR	    12

#if !defined(__ASSEMBLER__)

/*
 * IMPORTANT: A full exception context is indicated by setting the
 * bottom bit of the pc.  This means you need to be very careful at
 * places where the pc is being modified or used.  In particular, when
 * returning from an exception, the bottom bit needs to be masked out.
 */

/* Must match #defines below */
class arm_irq_context_t {
    public:
	u32_t klr;	/*  0   */
	u32_t r0;	/*  4   */
	u32_t r1;	/* -8-  */
	u32_t r2;	/*  12  */
	u32_t r3;	/*  16  */
	u32_t r4;	/*  20  */
	u32_t r5;	/* -24- */
	u32_t r6;	/*  28  */
	u32_t r7;	/*  32  */
	u32_t r8;	/*  36  */
	u32_t r9;	/* -40- */
	u32_t r10;	/*  44  */
	u32_t r11;	/*  48  */
	u32_t r12;	/*  52  */
	u32_t sp;	/* -56- */
	u32_t lr;	/*  60  */
	u32_t pc;	/*  64  */
	u32_t cpsr;	/*  68  */
};

/* Must match #defines below */
class arm_syscall_context_t {
    public:
	u32_t sp;	/*  0   */
	u32_t lr;	/*  4   */
	u32_t pc;	/*  8   */
	u32_t cpsr;	/*  12  */
};
#endif

#define PT_KLR		0
#define PT_R0		4
#define PT_R1		8
#define PT_R2		12
#define PT_R3		16
#define PT_R4		20
#define PT_R5		24
#define PT_R6		28
#define PT_R7		32
#define PT_R8		36
#define PT_R9		40
#define PT_R10		44
#define PT_R11		48
#define PT_R12		52
#define PT_SP		56
#define PT_LR		60
#define PT_PC		64
#define PT_CPSR		68

#define PT_SIZE		72

#if defined(__GNUC__)
#define SAVE_ALL_INT_TMP		\
    sub     sp,     sp,     #PT_SIZE;	\
    stmib   sp,	    {r0-r14}^;		\
    ldr     r0,     tmp_spsr;		\
    str	    lr,	    [sp, #PT_PC];	\
    str     r0,     [sp, #PT_CPSR];

#ifdef CONFIG_ENABLE_FASS
#define SAVE_ALL_INT_TMP_LINKED_DACR	\
    sub     sp,     sp,     #PT_SIZE;	\
    stmib   sp,	    {r0-r14}^;		\
    ldr     r0,     tmp_spsr;		\
    ldr	    ip,	    kernel_access;	\
    str	    lr,	    [sp, #PT_PC];	\
    str     r0,     [sp, #PT_CPSR];
#else
#define SAVE_ALL_INT_TMP_LINKED_DACR	SAVE_ALL_INT_TMP
#endif

#define RESTORE_ALL			\
    ldr     lr,     [sp, #PT_CPSR];	\
    ldmib   sp,     {r0-r14}^;		\
    nop;				\
    msr     spsr,   lr;			\
    ldr	    lr,	    [sp, #PT_PC];	\
    add     sp,     sp,     #PT_SIZE;

#define RESTORE_ALL_ABT			\
    ldr     lr,     [sp, #PT_CPSR];	\
    ldmib   sp,     {r0-r14}^;		\
    nop;				\
    str	    lr,	    tmp_spsr;		\
    ldr	    lr,	    [sp, #PT_PC];	\
    add     sp,     sp,     #PT_SIZE;	\
    str	    lr,	    tmp_r14;

#ifdef CONFIG_ENABLE_FASS
#define SET_KERNEL_DACR			\
    ldr	    ip,	    kernel_access;	\
    mcr	    p15, 0, ip, c3, c0;
#else
#define SET_KERNEL_DACR
#endif

#ifdef CONFIG_ENABLE_FASS
#define SET_KERNEL_DACR_LINKED		\
    mcr	    p15, 0, ip, c3, c0;
#else
#define SET_KERNEL_DACR_LINKED
#endif

// dacr = (0x00000001 | (1 << (2 * current_domain)));
#ifdef CONFIG_ENABLE_FASS
#define SET_USER_DACR			\
    ldr	    ip,	    arm_current_ptr;	\
    ldr	    ip,	    [ip];		\
    mov	    lr,	    #1;			\
    mov	    ip,	    ip, lsl #1;		\
    orr	    lr,	    lr, lr, lsl ip;	\
    mcr	    p15,    0, lr, c3, c0;

/* If inside the kernel, don't modify DACR */
#define SET_USER_DACR_K			\
    ldr	    ip,	    [sp, #PT_CPSR];	\
    tst	    ip,	    #0x0f;		\
    bne	    1f;				\
    SET_USER_DACR			\
1:

#else
#define SET_USER_DACR
#define SET_USER_DACR_K
#endif

#elif defined(__RVCT__)
#if defined(ASSEMBLY)
    MACRO
    SAVE_ALL_INT_TMP
    sub     sp,     sp,     #PT_SIZE
    stmib   sp,	    {r0-r14}^
    ldr     r0,     tmp_spsr
    str	    lr,	    [sp, #PT_PC]
    str     r0,     [sp, #PT_CPSR]
    MEND

#ifdef CONFIG_ENABLE_FASS
    MACRO
    SAVE_ALL_INT_TMP_LINKED_DACR
    sub     sp,     sp,     #PT_SIZE
    stmib   sp,	    {r0-r14}^
    ldr     r0,     tmp_spsr
    ldr	    ip,	    kernel_access
    str	    lr,	    [sp, #PT_PC]
    str     r0,     [sp, #PT_CPSR]
    MEND
#else
#define SAVE_ALL_INT_TMP_LINKED_DACR	SAVE_ALL_INT_TMP
#endif

    MACRO
    SAVE_ALL_INT_MRS
    str	    lr,	    [sp, #-4]!
    stmdb   sp,	    {r0-r14}^
    nop
    mrs     lr,     spsr
    sub     sp,     sp,     #(PT_SIZE-4)
    str     lr,     [sp, #PT_CPSR]
    MEND

    MACRO
    RESTORE_ALL
    ldr     lr,     [sp, #PT_CPSR]
    ldmib   sp,     {r0-r14}^
    nop
    msr     spsr_cxsf,   lr
    ldr	    lr,	    [sp, #PT_PC]
    add     sp,     sp,     #PT_SIZE
    MEND

    MACRO
    RESTORE_ALL_ABT
    ldr     lr,     [sp, #PT_CPSR]
    ldmib   sp,     {r0-r14}^
    nop
    str	    lr,	    tmp_spsr
    ldr	    lr,	    [sp, #PT_PC]
    add     sp,     sp,     #PT_SIZE
    str	    lr,	    tmp_r14
    MEND

#ifdef CONFIG_ENABLE_FASS
    MACRO
    SET_KERNEL_DACR
    ldr	    ip,	    kernel_access
    mcr	    p15, 0, ip, c3, c0
    MEND
#else
#define SET_KERNEL_DACR
#endif

#ifdef CONFIG_ENABLE_FASS
    MACRO
    SET_KERNEL_DACR_LINKED
    mcr	    p15, 0, ip, c3, c0
    MEND
#else
#define SET_KERNEL_DACR_LINKED
#endif

// dacr = (0x00000001 | (1 << (2 * current_domain)));
#ifdef CONFIG_ENABLE_FASS
    MACRO
    SET_USER_DACR
    ldr	    ip,	    =current_domain
    ldr	    ip,	    [ip]
    mov	    lr,	    #1
    mov	    ip,	    ip, lsl #1
    orr	    lr,	    lr, lr, lsl ip
    mcr	    p15,    0, lr, c3, c0
    MEND

/* If inside the kernel, don't modify DACR */
    MACRO
    SET_USER_DACR_K
    ldr	    ip,	    [sp, #PT_CPSR]
    tst	    ip,	    #0x0f
    bne	    %F1
    SET_USER_DACR
1
    MEND
#else
#define SET_USER_DACR
#define SET_USER_DACR_K
#endif
#endif
#endif

#endif /* __ARCH__ARM__THREAD_H__ */
