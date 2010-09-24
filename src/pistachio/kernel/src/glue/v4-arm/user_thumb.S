/*********************************************************************
 *
 * Copyright (C) 2003-2005,  National ICT Australia (NICTA)
 *
 * File path:     glue/v4-arm/user_thumb.S
 * Description:   Kernel entry points for thumb mode syscalls
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
 * $Id: user.S,v 1.19 2004/12/01 23:57:01 cvansch Exp $
 *
 ********************************************************************/

#include <l4.h>
#include INC_ARCH(asm.h)
#include INC_ARCH(thread.h)
#include INC_GLUE(syscalls.h)

	CODE16

#if defined(__GNUC__)
#define SYSCALL_UNIMPLEMENTED(name)		\
.section    .user_thumb.##name;			\
BEGIN_PROC_KIP(user_thumb_##name);		\
        mov   pc, lr;				\
END_PROC_KIP(user_thumb_##name)

#define SYSCALL(name)				\
.section	.user_thumb.##name;		\
BEGIN_PROC_KIP(user_thumb_##name);		\
	mov	ip,	sp;			\
	mov	r7,	#(~(0xffffff00 + SYSCALL_##name));	\
	mvn	r7,	r7;			\
	mov	sp,	r7;			\
	swi	SYSCALL_##name;			\
END_PROC_KIP(user_thumb_##name)

#define SYSCALL_MRS(name)			\
.section	.user_thumb.##name;		\
BEGIN_PROC_KIP(user_thumb_##name);		\
	mov	r7,	r8;			\
	mov	r8,	r0;			\
	ldr	r0,	=0xff000ff0;		\
	ldr	r0,	[r0];			\
	add	r0,	r0,	#64;		\
	stmia	r0!,	{r3-r7};		\
	push	{lr};				\
	ldr	r0,	=(0xffffff00 + SYSCALL_##name);	\
	mov	ip,	sp;			\
	mov	sp,	r0;			\
	mov	r0,	r8;			\
	mov	lr,	pc;			\
	swi	SYSCALL_##name;			\
	ldr	r2,	=0xff000ff0;		\
	ldmia	r2!,	{r3-r7};		\
	mov	r8,	r7;			\
	pop	{pc};				\
END_PROC_KIP(user_thumb_##name)
#elif defined(__RVCT__)
	MACRO
	syscall_unimplemented $name, $aname
	AREA	$aname, CODE, READWRITE
	EXPORT	$name
$name
	jump	lr
	ALIGN
	MEND
#define SYSCALL_UNIMPLEMENTED(name) syscall_unimplemented user_thumb_##name, |.##user_thumb##.##name|

	MACRO
	syscall	$name, $num, $aname
	AREA	$aname, CODE, READWRITE
	EXPORT	$name
$name
	mov	ip,	sp
	mov	r7,	#(~(0xffffff00 + $num))
	mvn	r7,	r7
	mov	sp,	r7
	mov	r7,	r8
	swi	0x14
	ALIGN
	MEND
#define SYSCALL(name) syscall user_thumb_##name, SYSCALL_##name, |.##user_thumb##.##name|

	MACRO
	syscall_mrs $name, $num, $aname
	AREA	$aname, CODE, READWRITE
	EXPORT	$name
$name
	mov	r7,	r8
	mov	r8,	r0
	ldr	r0,	=0xff000ff0
	ldr	r0,	[r0]
	add	r0,	r0,	#64
	stmia	r0!,	{r3-r7}
	push	{lr}
	ldr	r0,	=(0xffffff00 + $num)
	mov	ip,	sp
	mov	sp,	r0
	mov	r0,	r8
	mov	lr,	pc
	swi	0x14
	ldr	r2,	=0xff000ff0
	ldmia	r2!,	{r3-r7}
	mov	r8,	r7
	pop	{pc}
	ALIGN
	MEND
#define SYSCALL_MRS(name) syscall_mrs user_thumb_##name, SYSCALL_##name, |.##user_thumb##.##name|
#endif

	SYSCALL(ipc)
	SYSCALL_MRS(lipc)
	SYSCALL(thread_switch)
	SYSCALL(space_control)
	SYSCALL_MRS(map_control)
	SYSCALL(exchange_registers)
	SYSCALL(thread_control)
	SYSCALL(schedule)
	SYSCALL_MRS(cache_control)

/* ProcessorControl currently unimplemented */


	SYSCALL_UNIMPLEMENTED(processor_control)
    
	END
