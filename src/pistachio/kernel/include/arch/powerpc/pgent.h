/****************************************************************************
 *
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *
 * File path:	arch/powerpc/pgent.h
 * Description:	The page table entry.  Compatible with the generic linear
 * 		page table walker.
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
 * $Id: pgent.h,v 1.9 2005/01/18 13:28:21 cvansch Exp $
 *
 ***************************************************************************/

#ifndef __ARCH__POWERPC___PGENT_H__
#define __ARCH__POWERPC___PGENT_H__

class space_t;
class mapnode_t;
class pgent_t;

class base_pgent_t
{
private:

    enum pgsize_e {
	size_none = 0
    };

    // Linknode access 

    inline word_t get_linknode( space_t * s, pgsize_e pgsize );
    inline void set_linknode( space_t * s, pgsize_e pgsize, word_t val );

public:

    // Predicates

    inline bool is_valid( space_t * s, pgsize_e pgsize );
    inline bool is_writable( space_t * s, pgsize_e pgsize );
    inline bool is_readable( space_t * s, pgsize_e pgsize );
    inline bool is_executable( space_t * s, pgsize_e pgsize );
    inline bool is_subtree( space_t * s, pgsize_e pgsize );
    inline bool is_kernel( space_t * s, pgsize_e pgsize );
    /* inline bool is_kernel_writeable( space_t * s, pgsize_e pgsize ); */

    // Retrieval

    inline addr_t address( space_t * s, pgsize_e pgsize );
    inline pgent_t * subtree( space_t * s, pgsize_e pgsize );
    inline mapnode_t * mapnode( space_t * s, pgsize_e pgsize, addr_t vaddr );
    inline addr_t vaddr( space_t * s, pgsize_e pgsize, mapnode_t * map );
    inline word_t reference_bits( space_t *s, pgsize_e pgsize, addr_t vaddr );
    inline memattrib_e get_attributes( space_t * s, pgsize_e pgsize );

    // Modification

    inline void flush( space_t * s, pgsize_e pgsize, bool kernel, addr_t vaddr);
    inline void clear( space_t * s, pgsize_e pgsize, bool kernel, addr_t vaddr);
    inline void make_subtree( space_t * s, pgsize_e pgsize, bool kernel );
    inline void remove_subtree( space_t * s, pgsize_e pgsize, bool kernel );
    inline void set_entry( space_t * s, pgsize_e pgsize,
	    addr_t paddr, bool readable, bool writable,
	    bool executable, bool kernel=false, memattrib_e attrib );
    inline void set_linknode( space_t * s, pgsize_e pgsize,
	    mapnode_t * map, addr_t vaddr );
    inline void update_rights( space_t *s, pgsize_e pgsize, word_t rwx );
    inline void revoke_rights( space_t *s, pgsize_e pgsize, word_t rwx );
    inline void reset_reference_bits( space_t *s, pgsize_e pgsize );
    inline void update_reference_bits( space_t *s, pgsize_e pgsz, word_t rwx );
    inline void set_accessed( space_t *s, pgsize_e pgsize, word_t flag );
    inline void set_dirty( space_t *s, pgsize_e pgsize, word_t flag );
    inline void set_attributes( space_t *s, pgsize_e pgsize, memattrib_e attrib );

    // Movement

    inline pgent_t * next( space_t * s, pgsize_e pgsize, word_t num );

    // Debug

    void dump_misc (space_t * s, pgsize_e pgsize);
};

#include INC_CPU(pgent.h)

#endif /* __ARCH__POWERPC___PGENT_H__ */
