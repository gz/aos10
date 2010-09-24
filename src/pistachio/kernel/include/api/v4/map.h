/*********************************************************************
 *
 * Copyright (C) 2005-2006,  National ICT Australia
 *
 * File path:    api/v4/map.h
 * Description:  Memory mapping API class definitions
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
#ifndef __API__V4__MAP_H__
#define __API__V4__MAP_H__

class phys_desc_t
{
public:
    inline word_t get_raw()
	{ return raw; }

    inline void clear()
	{ raw = 0; }

    inline u64_t get_base()
	{ return ((u64_t)x.base) << 10; }

    inline void set_base(u64_t base)
	{ x.base = base >> 10; }

    inline l4attrib_e get_attributes()
	{ return (l4attrib_e)x.attr; }

    inline void set_attributes(l4attrib_e attr)
	{ x.attr = attr; }

    inline void operator = (word_t raw)
	{ this->raw = raw; }
private:
    union {
	word_t raw;
	struct {
	    BITFIELD2(word_t,
		    attr	: 6,
		    base	: BITS_WORD - 6);
	} x;
    };
};

class perm_desc_t
{
public:
    inline word_t get_raw()
	{ return raw; }

    inline void clear()
	{ raw = 0; }

    inline void set_perms(word_t rwx)
	{ x.rwx = rwx; }

    inline void set_reference(word_t RWX)
	{ x.RWX = RWX; }

    inline void set_distinct()
	{ x.d = 1; }

    inline void operator = (word_t raw)
	{ this->raw = raw; }
private:
    union {
	word_t raw;
	struct {
	    BITFIELD4(word_t,
		    rwx		: 4,
		    RWX		: 4,
		    __res	: BITS_WORD - 9,
		    d		: 1);
	} x;
    };
};

class map_control_t
{
public:
    inline word_t is_modify()
	{ return x.m; }

    inline word_t is_read()
	{ return x.r; }

    inline word_t highest_item()
	{ return x.n; }

    inline void operator = (word_t raw)
	{ this->raw = raw; }
private:
    union {
	word_t raw;
	struct {
	    BITFIELD4(word_t,
		    n		: 6,
		    __res	: BITS_WORD - 8,
		    r		: 1,
		    m		: 1);
	} x;
    };
};

#endif /* __API__V4__MAP_H__ */
