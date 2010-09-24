/*********************************************************************
 *                
 * Copyright (C) 2002, 2003-2004, Karlsruhe University
 * Copyright (C) 2004-2005,  National ICT Australia (NICTA)
 *                
 * File path:     arch/arm/pgent.h
 * Description:   Generic page table manipluation for ARM
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
 * $Id: pgent.h,v 1.23 2004/12/07 06:58:39 cvansch Exp $
 *                
 ********************************************************************/
#ifndef __ARCH__ARM__PGENT_H__
#define __ARCH__ARM__PGENT_H__

#include <kmemory.h>
#include INC_ARCH(fass.h)
#include INC_ARCH(page.h)
#include INC_ARCH(ptab.h)
#include INC_GLUE(hwspace.h)	/* virt<->phys */
#include INC_CPU(cache.h)

#if defined(CONFIG_ARM_TINY_PAGES)
#error Tiny pages not working
#endif

EXTERN_KMEM_GROUP (kmem_pgtab);

class mapnode_t;
class space_t;

template<typename T> INLINE T phys_to_page_table(T x)
{
    return (T) ((u32_t) x + PGTABLE_OFFSET);
}

template<typename T> INLINE T virt_to_page_table(T x)
{
    return (T) ((u32_t) x - VIRT_ADDR_BASE + PGTABLE_ADDR_BASE);
}

template<typename T> INLINE T page_table_to_virt(T x)
{
    return (T) ((u32_t) x - PGTABLE_ADDR_BASE + VIRT_ADDR_BASE);
}

template<typename T> INLINE T page_table_to_phys(T x)
{
    return (T) ((u32_t) x - PGTABLE_OFFSET);
}

INLINE bool has_tiny_pages (space_t * space)
{
#if defined(CONFIG_ARM_TINY_PAGES)
    return true;
#else
    return false;
#endif
}

INLINE word_t arm_l2_ptab_size (space_t * space)
{
    return has_tiny_pages (space) ? ARM_L2_SIZE_TINY : ARM_L2_SIZE_NORMAL;
}


class pgent_t
{
public:
    union {
	arm_l1_desc_t	l1;
	arm_l2_desc_t	l2;
	u32_t		raw;
    };

public:
    enum pgsize_e {
#if defined(CONFIG_ARM_TINY_PAGES)
	size_1k,
	size_4k,
	size_min = size_1k,
#else
	size_4k = 0,
	size_min = size_4k,
#endif
	size_64k = 1,
	size_1m = 2,
	min_tree = size_1m,
	size_4g = 3,
	size_max = size_1m,
    };


private:


    void sync_64k (space_t * s)
	{
	    for (word_t i = 1; i < (arm_l2_ptab_size (s) >> 6); i++)
		((u32_t *) this)[i] = raw;
	}

    void sync_64k (space_t * s, word_t val)
	{
	    for (word_t i = 1; i < (arm_l2_ptab_size (s) >> 6); i++)
		((u32_t *) this)[i] = val;
	}

    void sync_4k (space_t * s)
	{
	    if (has_tiny_pages (s))
		for (word_t i = 1; i < 4; i++)
		    ((u32_t *) this)[i] = raw;
	}

public:

    // Predicates

    bool is_valid (space_t * s, pgsize_e pgsize)
	{
	    switch (pgsize)
	    {
	    case size_1m:
		return (l1.fault.zero != 0) || (l1.section.two == 2);
	    case size_64k:
		return true;
	    case size_4k:
		if (has_tiny_pages(s)) {
		    return true;
		} else {
		    return (l2.small.two == 2);
		}
#if defined(CONFIG_ARM_TINY_PAGES)
	    case size_1k:
		return (l2.tiny.three == 3);
#endif
	    default:
		return false;
	    }
	}

    bool is_writable (space_t * s, pgsize_e pgsize)
	{
	    return pgsize == size_1m ?
		    l1.section.ap == arm_l1_desc_t::rw :
		    /* 64k, 4k, 1k have ap0 in the same location */
		    l2.tiny.ap == arm_l1_desc_t::rw;
	}

    /* in is_readable() and is_executable(), the fields are identical for
     * all pagesizes, hence the optimization to not check pgsize */
    bool is_readable (space_t * s, pgsize_e pgsize)
	{ return l1.section.ap >= arm_l1_desc_t::ro; }

    bool is_executable (space_t * s, pgsize_e pgsize)
	{ return l1.section.ap >= arm_l1_desc_t::ro; }

    bool is_subtree (space_t * s, pgsize_e pgsize)
	{
	    switch (pgsize)
	    {
	    case size_1m:
		return (l1.section.two != 2) && (l1.fault.zero != 0);
	    case size_64k:
		return (l2.large.one != 1);
	    case size_4k:
		if (has_tiny_pages(s)) {
		    return (l2.small.two != 2);
		}
	    default:
		return false;
	    }
	}

    bool is_kernel (space_t * s, pgsize_e pgsize)
	{ return l1.section.ap <= arm_l1_desc_t::none; }

    // Retrieval

    arm_domain_t get_domain(void)
        {
            return l1.section.domain;
        }

    addr_t address (space_t * s, pgsize_e pgsize)
	{
#if defined(CONFIG_ARM_TINY_PAGES)
	    if (pgsize == size_1k)
		return l1.tiny.address();
#endif
	    /* The address for 1M, 64k, 4k is always padded to 20 bits on ARMv5 */
	    return (addr_t)(raw & ~(0xfff));
	}
	
    pgent_t * subtree (space_t * s, pgsize_e pgsize)
	{
	    if (pgsize == size_1m)
	    {
		if (has_tiny_pages (s))
		    return (pgent_t *) phys_to_page_table(l1.address_finetable());
		else
		    return (pgent_t *) phys_to_page_table(l1.address_coarsetable());
	    }
	    else
		return this;
	}

    word_t reference_bits (space_t * s, pgsize_e pgsize, addr_t vaddr)
	{ return 7; }

    // Modification

private:

    void cpd_sync (space_t * s);

public:

    void set_domain(arm_domain_t domain)
        {
            l1.section.domain = domain;
        }

    void clear (space_t * s, pgsize_e pgsize, bool kernel, addr_t vaddr)
        {
            clear(s, pgsize, kernel);
        }

    void clear (space_t * s, pgsize_e pgsize, bool kernel )
	{
	    raw = 0;
	    if (EXPECT_FALSE(pgsize == size_64k))
	    {
		sync_64k (s, 0);
	    }
#ifdef CONFIG_ENABLE_FASS
	    else if (EXPECT_FALSE(pgsize == size_1m))
	    {
		cpd_sync(s);
	    }
#endif
#if defined(CONFIG_ARM_TINY_PAGES)
	    else if (EXPECT_TRUE(pgsize == size_4k))
	    {
		sync_4k (s);
	    }
#endif
	}

    void flush (space_t * s, pgsize_e pgsize, bool kernel, addr_t vaddr)
	{
	}

    bool make_subtree (space_t * s, pgsize_e pgsize, bool kernel)
	{
	    if (pgsize == size_1m)
	    {
		if (has_tiny_pages (s))
		{
		    addr_t base = kmem.alloc (kmem_pgtab, ARM_L2_SIZE_TINY);
		    if (base == NULL)
			return false;
		    l1.raw = 0;
		    l1.fine_table.three = 3;
		    l1.fine_table.base_address = (word_t)virt_to_ram(base) >> 12;
		    arm_cache::cache_flush_d_ent(base, ARM_L2_SIZE_TINY_BITS);
		}
		else
		{
		    addr_t base = kmem.alloc (kmem_pgtab, ARM_L2_SIZE_NORMAL);
		    if (base == NULL)
			return false;
		    l1.raw = 0;
		    l1.coarse_table.one = 1;
		    l1.coarse_table.base_address = (word_t)virt_to_ram(base) >> 10;
		    arm_cache::cache_flush_d_ent(base, ARM_L2_SIZE_NORMAL_BITS);
		}
#ifdef CONFIG_ENABLE_FASS
		cpd_sync(s);
#endif
	    }
	    else
	    {
		if (!l2.is_valid())
		    raw = (pgsize << 16);
	    }
	    return true;
	}

    void remove_subtree (space_t * s, pgsize_e pgsize, bool kernel)
	{
	    if (pgsize == size_1m)
	    {
		if (has_tiny_pages (s))
		    kmem.free (kmem_pgtab, phys_to_virt (
			       (addr_t) (l1.fine_table.base_address << 12)),
			       ARM_L2_SIZE_TINY);
		else
		    kmem.free (kmem_pgtab, phys_to_virt (
			       (addr_t) (l1.coarse_table.base_address << 10)),
			       ARM_L2_SIZE_NORMAL);
	    }

	    clear (s, pgsize, kernel);
	}


#define QUAD(x)	(x | (x<<2) | (x<<4) | (x<<6))

    void set_entry (space_t * s, pgsize_e pgsize,
                           addr_t paddr, bool readable,
                           bool writable, bool executable,
                           bool kernel, memattrib_e attrib)
	{
	    word_t ap;

	    if (EXPECT_FALSE(kernel))
		ap = writable ? QUAD(arm_l1_desc_t::none) : QUAD(arm_l1_desc_t::special);
	    else
		ap = writable ? QUAD(arm_l1_desc_t::rw) : QUAD(arm_l1_desc_t::ro);

	    if (EXPECT_TRUE(pgsize == size_4k))
	    {
		l2.raw = 0;
		l2.small.two = 2;
		l2.small.attrib = attrib;
		l2.small.ap = ap;
		l2.small.base_address = (word_t) paddr >> 12;
	    }
	    else if (pgsize == size_1m)
	    {
		l1.raw = 0;
		l1.section.two = 2;
		l1.section.attrib = attrib;
		l1.section.domain = 0;
		l1.section.ap = ap;
		l1.section.base_address = (word_t) paddr >> 20;
#ifdef CONFIG_ENABLE_FASS
		cpd_sync(s);
#endif
	    }
#if defined(CONFIG_ARM_TINY_PAGES)
	    else if (pgsize == size_1k)
	    {
		l2.raw = 0;
		l2.tiny.three = 3;
		l2.tiny.attrib = attrib;
		l2.tiny.ap = ap;
		l2.tiny.base_address = (word_t) paddr >> 10;
	    }
#endif
	    else
	    {	/* size_64k */
		l2.raw = 0;
		l2.large.one = 1;
		l2.large.attrib = attrib;
		l2.large.ap = ap;
		l2.large.base_address = (word_t) paddr >> 16;
		sync_64k (s);
	    }
	}

    /* For init code, must be inline. Sets kernel and cached */
    inline void set_entry_1m (space_t * s, addr_t paddr, bool readable,
                           bool writable, bool executable, memattrib_e attrib)
	{
	    word_t ap;

	    ap = writable ? arm_l1_desc_t::none : arm_l1_desc_t::special;

	    l1.raw = 0;
	    l1.section.two = 2;
	    l1.section.attrib = attrib;
	    l1.section.domain = 0;
	    l1.section.ap = ap;
	    l1.section.base_address = (word_t) paddr >> 20;
	}

    inline void set_entry (space_t * s, pgsize_e pgsize,
                           addr_t paddr, bool readable,
                           bool writable, bool executable,
                           bool kernel)
	{
	    set_entry(s, pgsize, paddr, readable, writable, executable, kernel, l4default);
	}

    void revoke_rights (space_t * s, pgsize_e pgsize, word_t rwx)
	{
	    word_t ap = pgsize == size_1m ? l1.section.ap : l2.tiny.ap;

	    if ((rwx & 2) && ap == arm_l1_desc_t::rw)
	    {
		ap = QUAD(arm_l1_desc_t::ro);

		switch (pgsize)
		{
		case size_1m:
		    l1.section.ap = ap;
#ifdef CONFIG_ENABLE_FASS
		    cpd_sync(s);
#endif
		    break;
		case size_64k:
		    l2.large.ap = ap;
		    sync_64k (s);
		    break;
		case size_4k:
		    l2.small.ap = ap;
		    sync_4k (s);
		    break;
#if defined(CONFIG_ARM_TINY_PAGES)
		case size_1k:
		    l2.tiny.ap = ap;
		    break;
#endif
		default:
		    break;
		}
	    }
	}

    memattrib_e get_attributes (space_t * s, pgsize_e pgsize)
        {
	    /* bits in the same position for all page sizes */
	    return (memattrib_e)l2.small.attrib;
        }


    void update_attributes(space_t * s, pgsize_e pgsize, memattrib_e attrib)
	{
	    /* bits in the same position for all page sizes */
	    l1.section.attrib = attrib;

	    switch (pgsize) {
	    case size_1m:
#ifdef CONFIG_ENABLE_FASS
		cpd_sync(s);
		break;
#endif
#if defined(CONFIG_ARM_TINY_PAGES)
	    case size_1k:
		break;
#endif
	    case size_64k:
		sync_64k (s);
		break;
	    case size_4k:
		sync_4k (s);
		break;
	    default:
		break;
	    }
	}

    void update_rights (space_t * s, pgsize_e pgsize, word_t rwx)
	{
	    word_t ap = pgsize == size_1m ? l1.section.ap : l2.tiny.ap;

	    if ((rwx & 2) && ap == arm_l1_desc_t::ro)
	    {
		ap = QUAD(arm_l1_desc_t::rw);

		switch (pgsize)
		{
		case size_1m:
		    l1.section.ap = ap;
#ifdef CONFIG_ENABLE_FASS
		    cpd_sync(s);
#endif
		    break;
		case size_64k:
		    l2.large.ap = ap;
		    sync_64k (s);
		    break;
		case size_4k:
		    l2.small.ap = ap;
		    sync_4k (s);
		    break;
#if defined(CONFIG_ARM_TINY_PAGES)
		case size_1k:
		    l2.tiny.ap = ap;
		    break;
#endif
		default:
		    break;
		}
	    }
	}

    void reset_reference_bits (space_t * s, pgsize_e pgsize)
	{ }

    void update_reference_bits (space_t * s, pgsize_e pgsize, word_t rwx)
	{ }

    // Movement

    pgent_t * next (space_t * s, pgsize_e pgsize, word_t num)
	{
#if defined(CONFIG_ARM_TINY_PAGES)
	    if (pgsize == size_4k)
	    {
		return this + (num * 4);
	    } else
#endif
	    if (pgsize == size_64k)
	    {
		return this + (num * (arm_l2_ptab_size (s) >> 6));
	    }
	    return this + num;
	}

    // Operators

    bool operator != (const pgent_t rhs)
	{
	    return this->raw != rhs.raw;
	}

    // Debug

    void dump_misc (space_t * s, pgsize_e pgsize)
	{
	    printf(" %s",
		    get_attributes(s, pgsize) == cached ? "WB" :
		    get_attributes(s, pgsize) == uncached ? "NC" :
		    get_attributes(s, pgsize) == writethrough ? "WT" :
		    get_attributes(s, pgsize) == writecombine ? "WC" :
		    "??");
	}
};

#endif /* !__ARCH__ARM__PGENT_H__ */
