/*********************************************************************
 *                
 * Copyright (C) 2006,  National ICT Australia
 *                
 * File path:     l4/cache.h
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
 ********************************************************************/
/* Creator: Carl van Schaik */
#ifndef __L4__CACHE_H__
#define __L4__CACHE_H__

#include <l4/types.h>
#include __L4_INC_ARCH(syscalls.h)


/*
 * Cache control options
 */

#define L4_CacheCtl_ArchSpecifc		(0UL << 6)
#define L4_CacheCtl_FlushRanges		(1UL << 6)
#define L4_CacheCtl_FlushIcache		(4UL << 6)
#define L4_CacheCtl_FlushDcache		(5UL << 6)
#define L4_CacheCtl_FlushCache		(6UL << 6)
#define L4_CacheCtl_CacheLock		(8UL << 6)
#define L4_CacheCtl_CacheUnLock		(9UL << 6)

#define L4_CacheCtl_MaskL1		( 1UL << 12)
#define L4_CacheCtl_MaskL2		( 2UL << 12)
#define L4_CacheCtl_MaskL3		( 4UL << 12)
#define L4_CacheCtl_MaskL4		( 8UL << 12)
#define L4_CacheCtl_MaskL5		(16UL << 12)
#define L4_CacheCtl_MaskL6		(32UL << 12)
#define L4_CacheCtl_MaskAllLs		(63UL << 12)

/*
 * Cache control flush attributes
 */
#define L4_CacheFlush_AttrI		(1UL << (L4_BITS_PER_WORD-4))
#define L4_CacheFlush_AttrD		(1UL << (L4_BITS_PER_WORD-3))
#define L4_CacheFlush_AttrClean		(1UL << (L4_BITS_PER_WORD-2))
#define L4_CacheFlush_AttrInvalidate	(1UL << (L4_BITS_PER_WORD-1))
#define L4_CacheFlush_AttrAll		(L4_CacheFlush_AttrI | L4_CacheFlush_AttrD |    \
					 L4_CacheFlush_AttrClean | L4_CacheFlush_AttrInvalidate)

L4_INLINE L4_Word_t L4_CacheFlushAll(void)
{
    return L4_CacheControl(L4_nilthread, L4_CacheCtl_FlushCache | L4_CacheCtl_MaskAllLs);
}

/*
 * Flush cache range (start..end-1)
 */
L4_INLINE L4_Word_t L4_CacheFlushRange(L4_ThreadId_t s, L4_Word_t start, L4_Word_t end)
{
    L4_Word_t size = (end - start);

    size = size & ((1UL << (L4_BITS_PER_WORD-4)) - 1);
    size |= L4_CacheFlush_AttrAll;
    L4_LoadMR (0, start);
    L4_LoadMR (1, size);

    return L4_CacheControl(s, L4_CacheCtl_FlushRanges | L4_CacheCtl_MaskAllLs | 0);
}

/*
 * Flush I-cache range (start..end-1)
 */
L4_INLINE L4_Word_t L4_CacheFlushIRange(L4_ThreadId_t s, L4_Word_t start, L4_Word_t end)
{
    L4_Word_t size = (end - start);

    size = size & ((1UL << (L4_BITS_PER_WORD-4)) - 1);
    size |= L4_CacheFlush_AttrI | L4_CacheFlush_AttrClean | L4_CacheFlush_AttrInvalidate;
    L4_LoadMR (0, start);
    L4_LoadMR (1, size);

    return L4_CacheControl(s, L4_CacheCtl_FlushRanges | L4_CacheCtl_MaskAllLs | 0);
}

/*
 * Flush-clean-invalidate D-cache range (start..end-1)
 */
L4_INLINE L4_Word_t L4_CacheFlushDRange(L4_ThreadId_t s, L4_Word_t start, L4_Word_t end)
{
    L4_Word_t size = (end - start);

    size = size & ((1UL << (L4_BITS_PER_WORD-4)) - 1);
    size |= L4_CacheFlush_AttrD | L4_CacheFlush_AttrClean | L4_CacheFlush_AttrInvalidate;
    L4_LoadMR (0, start);
    L4_LoadMR (1, size);

    return L4_CacheControl(s, L4_CacheCtl_FlushRanges | L4_CacheCtl_MaskAllLs | 0);
}

/*
 * Flush-Invalidate cache range (start..end-1)
 */
L4_INLINE L4_Word_t L4_CacheFlushRangeInvalidate(L4_ThreadId_t s, L4_Word_t start, L4_Word_t end)
{
    L4_Word_t size = (end - start);

    size = size & ((1UL << (L4_BITS_PER_WORD-4)) - 1);
    size |= L4_CacheFlush_AttrD | L4_CacheFlush_AttrI | L4_CacheFlush_AttrInvalidate;
    L4_LoadMR (0, start);
    L4_LoadMR (1, size);

    return L4_CacheControl(s, L4_CacheCtl_FlushRanges | L4_CacheCtl_MaskAllLs | 0);
}

#endif /* !__L4__CACHE_H__ */
