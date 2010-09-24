/*********************************************************************
 *                
 * Copyright (C) 2001, 2002, 2003,  Karlsruhe University
 *                
 * File path:     l4/misc.h
 * Description:   Miscelaneous interfaces
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
 * $Id: misc.h,v 1.4 2003/11/17 06:21:55 cvansch Exp $
 *                
 ********************************************************************/
#ifndef __L4__MISC_H__
#define __L4__MISC_H__

#include <l4/types.h>
#include __L4_INC_ARCH(syscalls.h)


/*
 * Memory attributes
 */

#define L4_DefaultMemory	0
#define L4_CachedMemory		1
#define L4_UncachedMemory	2
#define L4_WriteBackMemory	3
#define L4_WriteThroughMemory	4
#define L4_CoherentMemory	5
#define L4_IOMemory		6
#define L4_IOCombinedMemory	7


#endif /* !__L4__MISC_H__ */
