/*********************************************************************
 *
 * Copyright (C) 2004-2006,  National ICT Australia (NICTA)
 *
 * File path:     platform/pxa/console.h
 * Description:   PXA XSCALE console constants
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

#ifndef __PLATFORM__PXA__CONSOLE_H_
#define __PLATFORM__PXA__CONSOLE_H_

#include INC_CPU(cpu.h)

/* Console port */
#if defined(CONFIG_FFUART)

#define CONSOLE_POFFSET		0x100000
#define CONSOLE_VOFFSET		0x000000

#elif defined(CONFIG_STUART)

#define CONSOLE_POFFSET		0x700000
#define CONSOLE_VOFFSET		0x000000
#error STUART has a bug and doesnt work.

#elif defined(CONFIG_BTUART)
#error Please look up the offsets for BTUART and update this file

#elif defined(CONFIG_HWUART)
#error Please look up the offsets for HWUART and update this file

#else
#error Invalid UART defined
#endif


#endif /* __PLATFORM__PXA__CONSOLE_H_ */
