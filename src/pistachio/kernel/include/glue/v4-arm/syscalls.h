/*********************************************************************
 *                
 * Copyright (C) 2003-2005,  National ICT Australia (NICTA)
 *                
 * File path:     glue/v4-arm/syscalls.h
 * Description:   
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
 * $Id: syscalls.h,v 1.15 2004/12/09 01:00:07 cvansch Exp $
 *                
 ********************************************************************/
#ifndef __GLUE__V4_ARM__SYSCALLS_H__
#define __GLUE__V4_ARM__SYSCALLS_H__

#include INC_ARCH(asm.h)
#include INC_GLUE(config.h)

#define USER_UTCB_REF		(*(word_t *)(USER_KIP_PAGE + 0xff0))

#define L4_TRAP_KPUTC		0x000000a0
#define L4_TRAP_KGETC		0x000000a4
#define L4_TRAP_KGETC_NB	0x000000a8
#define L4_TRAP_KDEBUG		0x000000ac
#define L4_TRAP_GETUTCB		0x000000b0
#define L4_TRAP_KIP		0x000000b4
#define L4_TRAP_KSET_THRD_NAME  0x000000b8
#define L4_TRAP_GETCOUNTER      0x000000bc
#define L4_TRAP_GETNUMTPS       0x000000c0
#define L4_TRAP_GETTPNAME       0x000000c4
#define L4_TRAP_TCCTRL          0x000000c8

#define L4_TRAP_PMU_RESET       0x000000cc
#define L4_TRAP_PMU_STOP        0x000000d0
#define L4_TRAP_PMU_READ        0x000000d4
#define L4_TRAP_PMU_CONFIG      0x000000d8

#define L4_TRAP_GETTICK		0x000000e0

#define SYSCALL_ipc		    0x0
#define SYSCALL_thread_switch	    0x4
#define SYSCALL_thread_control	    0x8
#define SYSCALL_exchange_registers  0xc
#define SYSCALL_schedule	    0x10
#define SYSCALL_map_control	    0x14
#define SYSCALL_space_control	    0x18
#define SYSCALL_processor_control   0x1c
#define SYSCALL_cache_control	    0x20
#define SYSCALL_reserved	    0x24
#define SYSCALL_lipc		    0x28

/* Upper bound on syscall number */
#define SYSCALL_limit		SYSCALL_lipc

/*
 * Syscall declaration wrappers.
 */

#define SYS_IPC(to, from)					\
  SYS_IPC_RETURN_TYPE /*SYSCALL_ATTR ("ipc")*/			\
  sys_ipc (to, from)

#define SYS_THREAD_CONTROL(dest, space, scheduler, pager,	\
		send_redirector, recv_redirector, utcb)		\
  SYS_THREAD_CONTROL_RETURN_TYPE SYSCALL_ATTR ("thread_control")\
  sys_thread_control (dest, space, scheduler, pager,		\
		      send_redirector, recv_redirector, utcb)

#define SYS_SPACE_CONTROL(space, control, kip_area, utcb_area)	\
  SYS_SPACE_CONTROL_RETURN_TYPE SYSCALL_ATTR ("space_control")	\
  sys_space_control (space, control, kip_area, utcb_area)

#define SYS_SCHEDULE(dest, ts_len, total_quantum,		\
		     hw_thread_bitmask, processor_control, prio)\
  SYS_SCHEDULE_RETURN_TYPE SYSCALL_ATTR ("schedule")		\
  sys_schedule (dest, ts_len, total_quantum, hw_thread_bitmask, \
	  processor_control, prio)

#define SYS_EXCHANGE_REGISTERS(dest, control, usp, uip, uflags,	\
                        uhandle, pager)				\
  SYS_EXCHANGE_REGISTERS_RETURN_TYPE SYSCALL_ATTR ("exchange_registers")\
  sys_exchange_registers (dest, control, usp, uip,		\
                          uflags, uhandle, pager)

#define SYS_THREAD_SWITCH(dest)					\
  SYS_THREAD_SWITCH_RETURN_TYPE SYSCALL_ATTR ("thread_switch")	\
  sys_thread_switch (dest)

#define SYS_MAP_CONTROL(space, control)				\
  SYS_MAP_CONTROL_RETURN_TYPE SYSCALL_ATTR ("map_control")	\
  sys_map_control (space, control)

#define SYS_PROCESSOR_CONTROL(processor_no, internal_frequency,	\
                              external_frequency, voltage)	\
  void SYSCALL_ATTR ("processor_control")                       \
  sys_processor_control (processor_no, internal_frequency,	\
                         external_frequency, voltage)

#define SYS_CACHE_CONTROL(space, control)			\
  SYS_CACHE_CONTROL_RETURN_TYPE SYSCALL_ATTR ("cache_control")	\
  sys_cache_control (space, control)

#if defined(__GNUC__)
/*
 * attributes for system call functions
 */
#define SYSCALL_ATTR(x) __attribute__((noreturn))

#define SYS_IPC_RETURN_TYPE			threadid_t
#define SYS_THREAD_CONTROL_RETURN_TYPE		void
#define SYS_EXCHANGE_REGISTERS_RETURN_TYPE	void
#define SYS_MAP_CONTROL_RETURN_TYPE		void
#define SYS_SPACE_CONTROL_RETURN_TYPE		void
#define SYS_SCHEDULE_RETURN_TYPE		void
#define SYS_CACHE_CONTROL_RETURN_TYPE		void
#define SYS_THREAD_SWITCH_RETURN_TYPE		void


/**
 * Preload registers and return from sys_ipc
 * @param from The FROM value after the system call
 */
#define return_ipc(from)	return (from);

/**
 * Preload registers and return from sys_thread_control
 * @param result The RESULT value after the system call
 */
#define return_thread_control(result)					\
{									\
    register word_t rslt    ASM_REG("r0") = result;			\
    char * context = (char *) get_current_tcb()->get_stack_top () -	\
		ARM_SYSCALL_STACK_SIZE;					\
									\
    __asm__ __volatile__ (						\
	CHECK_ARG("r0", "%2")						\
	"mov	sp,	%1	\n"					\
	"mov	pc,	%0	\n"					\
	:: "r"	(__builtin_return_address(0)),				\
	   "r"	(context), "r" (rslt)					\
    );									\
    while (1);								\
}

/**
 * Preload registers and return from sys_exchange_registers
 * @param result The RESULT value after the system call
 * @param control The CONTROL value after the system call
 * @param sp The SP value after the system call
 * @param ip The IP value after the system call
 * @param flags The FLAGS value after the system call
 * @param pager The PAGER value after the system call
 * @param handle The USERDEFINEDHANDLE value after the system call
 */
#define return_exchange_registers(result, control, sp, ip, flags, pager, handle)\
{									\
    register word_t rslt    ASM_REG("r0") = (result).get_raw();		\
    register word_t ctrl    ASM_REG("r1") = control;			\
    register word_t sp_r    ASM_REG("r2") = sp;				\
    register word_t ip_r    ASM_REG("r3") = ip;				\
    register word_t flg	    ASM_REG("r4") = flags;			\
    register word_t hdl	    ASM_REG("r5") = handle;			\
    register word_t pgr	    ASM_REG("r6") = (pager).get_raw();		\
    char * context = (char *) get_current_tcb()->get_stack_top () -	\
		ARM_SYSCALL_STACK_SIZE;					\
									\
    __asm__ __volatile__ (						\
	CHECK_ARG("r0", "%2")						\
	CHECK_ARG("r1", "%3")						\
	CHECK_ARG("r2", "%4")						\
	CHECK_ARG("r3", "%5")						\
	CHECK_ARG("r4", "%6")						\
	CHECK_ARG("r5", "%8")						\
	CHECK_ARG("r6", "%7")						\
	"mov	sp,	%1	\n"					\
	"mov	pc,	%0	\n"					\
	:: "r"	(__builtin_return_address(0)),				\
	   "r"	(context),						\
	   "r" (rslt), "r" (ctrl), "r" (sp_r),				\
	   "r" (ip_r), "r" (flg), "r" (pgr), "r" (hdl)			\
    );									\
    while (1);								\
}

/**
 * Return from sys_thread_switch
 */
#define return_thread_switch()						\
{									\
    char * context = (char *) get_current_tcb()->get_stack_top () -	\
		ARM_SYSCALL_STACK_SIZE;					\
									\
    __asm__ __volatile__ (						\
	"mov	sp,	%1	\n"					\
	"mov	pc,	%0	\n"					\
	:: "r"	(__builtin_return_address(0)),				\
	   "r"	(context)						\
    );									\
    while (1);								\
}


/**
 * Return from sys_map_control
 * @param result The RESULT value after the system call
 */
#define return_map_control(result)					\
{									\
    register word_t rslt    ASM_REG("r0") = result;			\
    char * context = (char *) get_current_tcb()->get_stack_top () -	\
		ARM_SYSCALL_STACK_SIZE;					\
									\
    __asm__ __volatile__ (						\
	CHECK_ARG("r0", "%2")						\
	"mov	sp,	%1	\n"					\
	"mov	pc,	%0	\n"					\
	:: "r"	(__builtin_return_address(0)),				\
	   "r"	(context),						\
	   "r" (rslt)							\
    );									\
    while (1);								\
}


/**
 * Preload registers and return from sys_space_control
 * @param result The RESULT value after the system call
 * @param control The CONTROL value after the system call
 */
#define return_space_control(result, control) {                         \
    register word_t rslt    ASM_REG("r0") = result;			\
    register word_t cntrl   ASM_REG("r1") = control;			\
    char * context = (char *) get_current_tcb()->get_stack_top () -	\
		ARM_SYSCALL_STACK_SIZE;					\
									\
    __asm__ __volatile__ (						\
	CHECK_ARG("r0", "%2")						\
	CHECK_ARG("r1", "%3")						\
	"mov	sp,	%1	\n"					\
	"mov	pc,	%0	\n"					\
	:: "r"	(__builtin_return_address(0)),				\
	   "r"	(context),						\
	   "r" (rslt), "r" (cntrl)					\
    );									\
    while (1);								\
}

/**
 * Preload registers and return from sys_schedule
 * @param result The RESULT value after the system call
 */
#define return_schedule(result, rem_ts, rem_total) {			\
    register word_t rslt    ASM_REG("r0") = result;			\
    register word_t remts   ASM_REG("r1") = rem_ts;			\
    register word_t remtot  ASM_REG("r2") = rem_total;			\
    char * context = (char *) get_current_tcb()->get_stack_top () -	\
		ARM_SYSCALL_STACK_SIZE;					\
									\
    __asm__ __volatile__ (						\
	CHECK_ARG("r0", "%2")						\
	CHECK_ARG("r1", "%3")						\
	CHECK_ARG("r2", "%4")						\
	"mov	sp,	%1	\n"					\
	"mov	pc,	%0	\n"					\
	:: "r"	(__builtin_return_address(0)),				\
	   "r"	(context),						\
	   "r" (rslt), "r" (remts), "r" (remtot)			\
    );									\
    while (1);								\
}


/**
 * Return from sys_processor_control
 */
#define return_processor_control()					\
{									\
    char * context = (char *) get_current_tcb()->get_stack_top () -	\
		ARM_SYSCALL_STACK_SIZE;					\
									\
    __asm__ __volatile__ (						\
	"mov	sp,	%1	\n"					\
	"mov	pc,	%0	\n"					\
	:: "r"	(__builtin_return_address(0)),				\
	   "r"	(context)						\
    );									\
    while (1);								\
}

/**
 * Return from sys_cache_control
 */
#define return_cache_control(result)					\
{									\
    register word_t rslt    ASM_REG("r0") = result;			\
    char * context = (char *) get_current_tcb()->get_stack_top () -	\
		ARM_SYSCALL_STACK_SIZE;					\
									\
    __asm__ __volatile__ (						\
	CHECK_ARG("r0", "%2")						\
	"mov	sp,	%1	\n"					\
	"mov	pc,	%0	\n"					\
	:: "r"	(__builtin_return_address(0)),				\
	   "r"	(context),						\
	   "r" (rslt)							\
    );									\
    while (1);								\
}

#elif defined(__RVCT__)
/*
 * attributes for system call functions
 */
#define SYSCALL_ATTR(x)

#define SYS_IPC_RETURN_TYPE			threadid_t
#define SYS_THREAD_CONTROL_RETURN_TYPE		word_t
#define SYS_EXCHANGE_REGISTERS_RETURN_TYPE	void
#define SYS_MAP_CONTROL_RETURN_TYPE		word_t
#define SYS_SPACE_CONTROL_RETURN_TYPE		result2_t __value_in_regs
#define SYS_SCHEDULE_RETURN_TYPE		result3_t __value_in_regs
#define SYS_CACHE_CONTROL_RETURN_TYPE		word_t
#define SYS_THREAD_SWITCH_RETURN_TYPE		void

#if !defined(ASSEMBLY)
struct result2_t {
	word_t r0, r1;
};

struct result3_t {
	word_t r0, r1, r2;
};

void asm_return_exchange_registers(word_t result, word_t control, word_t sp, word_t ip, word_t flags, word_t handle, word_t pager, addr_t context, addr_t return_address);
#endif

/**
 * Return from sys_ipc
 */
#define return_ipc(from)		return (from)

/**
 * Return from sys_thread_control
 */
#define return_thread_control(result) 	return (result)
#define return_exchange_registers(result, control, sp, ip, flags, pager, handle) \
    asm_return_exchange_registers(result.get_raw(), control, sp, ip, flags, handle, pager.get_raw(), (char *)get_current_tcb()->get_stack_top() - ARM_IPC_STACK_SIZE - (4*4), __builtin_return_address(0))

/**
 * Return from sys_thread_switch
 */
#define return_thread_switch()		return

/**
 * Return from sys_map_control
 */
#define return_map_control(result)	return (result)

/**
 * Return from sys_space_control
 */
#define return_space_control(result, control) \
    return (result2_t){ (result), (control) }

/**
 * Return from sys_schedule
 */
#define return_schedule(result, rem_ts, rem_total) \
    return (result3_t){ (result), (rem_ts), (rem_total) }

/**
 * Return from sys_processor_control
 */
#define return_processor_control()      return

/**
 * Return from sys_cache_control
 */
#define return_cache_control(result)	return (result)

#endif

#endif /* __GLUE__V4_ARM__SYSCALLS_H__ */
