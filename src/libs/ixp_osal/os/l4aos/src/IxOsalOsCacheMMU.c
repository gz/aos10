/**
 * @file IxOsalOsCacheMMU.c (l4aos)
 *
 * @brief Cache MemAlloc and MemFree.
 * 
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

#include "IxOsal.h"

// Direct access to libsos.c for the dedicated allocator
extern void *sos_malloc(uint32_t size);

static unsigned sAlloced;

/* based on Intel's linux implementation */

/* Arbitrary numbers to detect memory corruption */
#define IX_OSAL_OS_MAGIC_ALLOC_NUMBER   0xBABEFACE
#define IX_OSAL_OS_MAGIC_DEALLOC_NUMBER 0xecafebab

/* Number of information words maintained behind the user buffer */
#define IX_OSAL_OS_NUM_INFO_WORDS	4

/* Macro to round up a size to a multiple of a cache line */
#define IX_OSAL_OS_CL_ROUND_UP(s) \
    (((s) + (IX_OSAL_CACHE_LINE_SIZE - 1)) & ~(IX_OSAL_CACHE_LINE_SIZE - 1))

/* 
 * Allocate on a cache line boundary (null pointers are
 * not affected by this operation). This operation is NOT cache safe.
 */
void *
ixOsalCacheDmaMalloc(UINT32 size)
{
    UINT32 *userPtr, *dp;

    /* The minimum allocation size is 32 */
    if (size < IX_OSAL_CACHE_LINE_SIZE)
	size = IX_OSAL_CACHE_LINE_SIZE;

    /*
     * dp         userPtr                           end of last cache line
     * ______________________________________________________
     * |   |  |  |                                 |         |
     * |Ptr|Sz|Ma|      USER BUFFER                |         |
     * |___|__|__|_________________________________|_________|
     * 
     * dp: The pointer returned by kmalloc. This may not be 32 byte aligned
     * userPtr: The pointer returned to the user. This is guaranteed
     *          to be 32 byte aligned
     * Sz: Amount of memory allocated
     * Ma: Arbitrary number 0xBABEFACE that allows to check against
     *     memory corruption
     * Ptr: This 4-byte field records the value of dp. This info is
     *      needed in order to deallocate the buffer
     */

    /*
     * Ensure that the size is rounded up to a multiple of a cache line
     * and add to it a cache line for storing internal information
     */
    size  = IX_OSAL_OS_CL_ROUND_UP(size + IX_OSAL_CACHE_LINE_SIZE);
    dp = sos_malloc(size);
    if (!dp) {
	ixOsalLog(IX_OSAL_LOG_LVL_ERROR, IX_OSAL_LOG_DEV_STDOUT,
		  "%s():  Fail to alloc memory of size %lu\n",
		  (uintptr_t) __FUNCTION__, size, 0, 0, 0, 0);
	return NULL;
    }

    /* Pass to the user a pointer that is cache line aligned */
    userPtr = dp + IX_OSAL_OS_NUM_INFO_WORDS;
    userPtr = (UINT32 *) IX_OSAL_OS_CL_ROUND_UP((UINT32) userPtr);

    /* It is imperative that the user pointer be 32 byte aligned */
    IX_OSAL_ENSURE(((UINT32) userPtr % IX_OSAL_CACHE_LINE_SIZE) == 0,
		   "Error memory allocated is not 32 byte aligned\n");
    
    /* Store the allocated pointer 3 words behind the client's pointer */
    userPtr[-3] = (UINT32) dp;
    /* Store the allocated pointer 2 words behind the client's pointer */
    userPtr[-2] = size;
    /* Store the allocation identifier 1 word behind the client's pointer */
    userPtr[-1] = IX_OSAL_OS_MAGIC_ALLOC_NUMBER;

    ixOsalLog(IX_OSAL_LOG_LVL_DEBUG3, IX_OSAL_LOG_DEV_STDOUT,
	      "%s(%lu):  size %ld\n",
	      (uintptr_t) __FUNCTION__, sAlloced, size, 0, 0, 0);
    sAlloced += size;
    return (void *) userPtr;
}

void
ixOsalCacheDmaFree(void *ptr)
{
    UINT32 *clientPtr = ptr;

    IX_OSAL_ENSURE(clientPtr, "Null pointer being freed");

    /* Make sure that the pointer passed in belongs to us */
    if (clientPtr[-1] != IX_OSAL_OS_MAGIC_ALLOC_NUMBER) {
	ixOsalLog(IX_OSAL_LOG_LVL_ERROR, IX_OSAL_LOG_DEV_STDOUT,
		  "%s():  Memory being freed is invalid \n",
		  (uintptr_t) __FUNCTION__, 0, 0, 0, 0, 0);
	return;
    }

    /* Detect multiple deallocation */
    clientPtr[-1] = IX_OSAL_OS_MAGIC_DEALLOC_NUMBER;

#if 0
    /* Rewind ptr to retrieve requested-size information */
    ixOsalMemFree((UINT32 *) clientPtr[-3]);
#else
    static unsigned freed;
    ixOsalLog(IX_OSAL_LOG_LVL_DEBUG3, IX_OSAL_LOG_DEV_STDOUT,
	      "%s(%lu):  Memory freed %lu size %ld\n",
	      (uintptr_t) __FUNCTION__, sAlloced, freed, clientPtr[-2], 0, 0);
    freed += clientPtr[-2];
#endif
}
