/**
 * @file IxOsalOs.h
 *
 * @brief linux-specific defines 
 *
 * Design Notes:
 *
 * @par
 * IXP400 SW Release version 2.1
 * 
 * -- Copyright Notice --
 * 
 * @par
 * Copyright (c) 2001-2005, Intel Corporation.
 * All rights reserved.
 * 
 * @par
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * 
 * @par
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * 
 * @par
 * -- End of Copyright Notice --
 */

#ifndef IxOsalOs_H
#define IxOsalOs_H

#ifndef IX_OSAL_CACHED
#error "Uncached memory not supported in linux environment"
#endif

#ifdef IX_OSAL_OS_LINUX_VERSION_2_6
#include <linux/dma-mapping.h>
#include <asm/io.h>
#else
#include <linux/cache.h>
#endif /* IX_OSAL_OS_LINUX_VERSION_2_6 */
#include <linux/mm.h>
#include <linux/config.h>
#include <asm/pgalloc.h>

/**
 * Linux implementations of macros.
 */

#define IX_OSAL_OS_MMU_VIRT_TO_PHYS(addr) ((addr) ? virt_to_phys((void*)(addr)) : 0)

#define IX_OSAL_OS_MMU_PHYS_TO_VIRT(addr)  ((addr) ? phys_to_virt((unsigned int)(addr)) : 0)

#ifdef IX_OSAL_OS_LINUX_VERSION_2_6
#define IX_OSAL_OS_CACHE_INVALIDATE(addr, size)  \
    (consistent_sync((void*)addr, (size_t) size, DMA_FROM_DEVICE))

#define IX_OSAL_OS_CACHE_FLUSH(addr, size) \
    (consistent_sync((void*)addr, (size_t) size, DMA_TO_DEVICE))

#else
#define IX_OSAL_OS_CACHE_INVALIDATE(addr, size)  \
    (invalidate_dcache_range((__u32)addr, (__u32)addr + size )) 

#define IX_OSAL_OS_CACHE_FLUSH(addr, size) \
    (clean_dcache_range((__u32)addr, (__u32)addr + size))
#endif /* IX_OSAL_OS_LINUX_VERSION_2_6 */

/* Cache preload not available*/
#define IX_OSAL_OS_CACHE_PRELOAD(addr,size) {}

#define printf	printk /* For backword compatibility, needs to move to better location */

#endif /* IxOsalOs_H */
