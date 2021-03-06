/*********************************************************************
 *
 * Copyright (C) 2006,  National ICT Australia
 *
 * File path:    glue/v4-arm/cache.h
 * Description: Cache control API class definitions
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
 *********************************************************************/
#ifndef __GLUE__V4_ARM__CACHE_H__
#define __GLUE__V4_ARM__CACHE_H__

#include INC_CPU(cache.h)

class cache_t {
    public:
	static void flush_all_clean_invalidate (void);
	static void flush_all_invalidate (void);
	static void flush_all_clean (void);

	static void flush_range_attribute(addr_t start, addr_t end, word_t attr);
};

INLINE void cache_t::flush_all_clean_invalidate(void)
{
    arm_cache::cache_flush();
}

INLINE void cache_t::flush_range_attribute(addr_t start, addr_t end, word_t attr)
{
    word_t size = (word_t)end - (word_t)start;
    arm_cache::cache_flush_range_attr(start, size, attr);
}

#endif /* __GLUE__V4_ARM__CACHE_H__ */
