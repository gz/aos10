/*********************************************************************
 *
 * Copyright (C) 2006,  National ICT Australia
 *
 * File path:    api/v4/cache.h
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
#ifndef __API__V4__CACHE_H__
#define __API__V4__CACHE_H__

enum cacheop_e {
    cop_arch	    = 0,
    cop_flush_fp    = 1,
    cop_flush_I_all = 4,
    cop_flush_D_all = 5,
    cop_flush_all   = 6,
    cop_lock	    = 8,
    cop_unlock	    = 9,
};

enum cacheattr_e {
    attr_invalidate_i	= 0x9,
    attr_invalidate_d	= 0xa,
    attr_invalidate_id	= 0xb,
    attr_clean_i	= 0x5,
    attr_clean_d	= 0x6,
    attr_clean_id	= 0x7,
    attr_clean_inval_i	= 0xd,
    attr_clean_inval_d	= 0xe,
    attr_clean_inval_id	= 0xf,
};

class cache_control_t
{
public:
    inline word_t highest_item()
	{ return x.n; }

    inline cacheop_e operation()
	{ return (cacheop_e)x.op; }

    inline word_t cache_level_mask()
	{ return x.lx; }

    inline void operator = (word_t raw)
	{ this->raw = raw; }
private:
    union {
	word_t raw;
	struct {
	    BITFIELD4(word_t,
		    n		: 6,
		    op		: 6,
		    lx		: 6,
		    __res	: BITS_WORD - 18);
	} x;
    };
};

#endif /* __API__V4__CACHE_H__ */
