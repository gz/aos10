/*********************************************************************
 *
 * Copyright (C) 2003-2004,  National ICT Australia (NICTA)
 *
 * File path:     lib/l4/arm-syscalls.c
 * Description:   syscall gate relocation
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
 * $Id: arm-syscalls.c,v 1.4 2004/06/04 08:21:29 htuch Exp $
 *
 ********************************************************************/

#include <l4/kip.h>

#if defined( __ARMCC_VERSION)
#include <l4/thread.h>
#endif

#if !defined(NULL)
#define NULL 0
#endif

__L4_Ipc_t __L4_Ipc = NULL;
__L4_Lipc_t __L4_Lipc = NULL;
__L4_Schedule_t __L4_Schedule = NULL;
__L4_ThreadSwitch_t __L4_ThreadSwitch = NULL;
__L4_ExchangeRegisters_t __L4_ExchangeRegisters = NULL;

__L4_MapControl_t __L4_MapControl = NULL;
__L4_ThreadControl_t __L4_ThreadControl = NULL;
__L4_SpaceControl_t __L4_SpaceControl = NULL;
__L4_ProcessorControl_t __L4_ProcessorControl = NULL;
__L4_CacheControl_t __L4_CacheControl = NULL;

#if defined( __ARMCC_VERSION)

L4_ThreadId_t L4_anylocalthread;
L4_Time_t L4_ZeroTime;

const L4_MsgTag_t L4_Niltag = {0UL};
const L4_Time_t L4_Never = {0UL};
const L4_ThreadId_t L4_nilthread = {0UL};
const L4_ThreadId_t L4_anythread = {~0UL};

const L4_Acceptor_t L4_UntypedWordsAcceptor = {0};
const L4_Acceptor_t L4_StringItemsAcceptor  = {1};
const L4_Acceptor_t L4_AsynchItemsAcceptor  = {2};
const L4_Acceptor_t L4_NoSyncItemsAcceptor  = {4};
const L4_Acceptor_t L4_AsynchOnlyItemsAcceptor = {6};

const L4_Fpage_t L4_Nilpage = {0};
L4_Fpage_t L4_CompleteAddressSpace;

#endif

#if defined( __ARMCC_VERSION)
L4_Word_t L4_ExReg_SrcThread(L4_ThreadId_t x)
{
  L4_GthreadId_t gtid;
  gtid.X.version   = 0;
  gtid.X.thread_no = L4_ThreadNo(x);
  return gtid.raw;
}
#endif

void __L4_Init( void );

void __L4_Init( void )
{
    L4_KernelInterfacePage_t *kip;
    
#if defined( __ARMCC_VERSION)
    L4_anylocalthread.global.raw = (~0UL) >> 6 << 6;

    L4_CompleteAddressSpace.X.rwx      = 0;
    L4_CompleteAddressSpace.X.reserved = 0;
    L4_CompleteAddressSpace.X.s        = 1;
    L4_CompleteAddressSpace.X.b        = 0;
#endif

    kip = L4_KernelInterface( NULL, NULL, NULL );

#if (defined(__thumb) || defined(__thumb__))
/* calculate the offset of the thumb syscalls */
#define KIP_RELOC(a) (*(&a + (&kip->ArchSyscall3 - &kip->SpaceControl + 1)) + (L4_Word_t) kip)
#else
#define KIP_RELOC(a) (a + (L4_Word_t) kip)
#endif

    __L4_Ipc = (__L4_Ipc_t) KIP_RELOC(kip->Ipc);
    __L4_Lipc = (__L4_Lipc_t) KIP_RELOC(kip->Lipc);
    __L4_Schedule = (__L4_Schedule_t) KIP_RELOC(kip->Schedule);
    __L4_ThreadSwitch = (__L4_ThreadSwitch_t) KIP_RELOC(kip->ThreadSwitch);
    __L4_ExchangeRegisters = (__L4_ExchangeRegisters_t) KIP_RELOC(kip->ExchangeRegisters);

    __L4_MapControl = (__L4_MapControl_t) KIP_RELOC(kip->MapControl);
    __L4_ThreadControl = (__L4_ThreadControl_t) KIP_RELOC(kip->ThreadControl);
    __L4_SpaceControl = (__L4_SpaceControl_t) KIP_RELOC(kip->SpaceControl);
    __L4_ProcessorControl = (__L4_ProcessorControl_t) KIP_RELOC(kip->ProcessorControl );
    __L4_CacheControl = (__L4_CacheControl_t) KIP_RELOC(kip->CacheControl);

#undef KIP_RELOC
}

