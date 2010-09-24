/*********************************************************************
 *                
 * Copyright (C) 2002-2004,  Karlsruhe University
 * Copyright (C) 2005,  National ICT Australia
 *                
 * File path:     generic/linear_ptab_walker.cc
 * Description:   Linear page table manipulation
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
 * $Id: linear_ptab_walker.cc,v 1.53 2004/11/08 17:23:42 uhlig Exp $
 *                
 ********************************************************************/
#ifndef __GENERIC__LINEAR_PTAB_WALKER_CC__
#define __GENERIC__LINEAR_PTAB_WALKER_CC__

#include <l4.h>
#include INC_ARCH(pgent.h)
#include INC_API(fpage.h)
#include INC_API(tcb.h)
#include INC_API(space.h)
#include INC_GLUE(schedule.h)

#include <kdb/tracepoints.h>
#include <linear_ptab.h>


DECLARE_TRACEPOINT (FPAGE_MAP);
DECLARE_TRACEPOINT (FPAGE_OVERMAP);
DECLARE_TRACEPOINT (FPAGE_UNMAP);

DECLARE_KMEM_GROUP (kmem_pgtab);

#define TRACE_MAP(x...)
//#define TRACE_MAP(x...)		printf(x);
#define TRACE_UNMAP(x...)
//#define TRACE_UNMAP(x...)	printf(x);
#define TRACE_READ(x...)
//#define TRACE_READ(x...)	printf(x);

const word_t hw_pgshifts[] = HW_PGSHIFTS;

class mapnode_t;


/*
 * Helper functions.
 */

static inline addr_t address (fpage_t fp, word_t size) PURE;
static inline addr_t address (fpage_t fp, word_t size)
{
    return (addr_t) (fp.raw & ~((1UL << size) - 1));
}

static inline word_t dbg_pgsize (word_t sz)
{
    return (sz >= GB (1) ? sz >> 30 : sz >= MB (1) ? sz >> 20 : sz >> 10);
}

static inline char dbg_szname (word_t sz)
{
    return (sz >= GB (1) ? 'G' : sz >= MB (1) ? 'M' : 'K');
}


/**
 * Map fpage to another address space. The access bits in the fpage
 * indicate which access rights the mappings in the destination address
 * space should have.
 *
 * @param base		physical address and page attributes
 * @param dest_fp	fpage description
 */
bool space_t::map_fpage(phys_desc_t base, fpage_t dest_fp)
{
    word_t t_num = dest_fp.get_size_log2 ();
    pgent_t::pgsize_e pgsize, t_size;
    addr_t t_addr, p_addr;
    pgent_t * tpg;
    bool flush = false;

    pgent_t * r_tpg[pgent_t::size_max];
    word_t r_tnum[pgent_t::size_max];

    u64_t phys = base.get_base();

    if (t_num < page_shift(pgent_t::size_min))
	return true;
    if (t_num > page_shift(pgent_t::size_max+1))
	t_num = page_shift(pgent_t::size_max+1);

    t_addr = address (dest_fp, t_num);

    memattrib_e page_attributes	= to_memattrib(base.get_attributes());
    /*
     * Mask phys to correct page size
     */
    phys &= ~(((u64_t)1 << t_num)-1);
    // XXX Add PAE support
    p_addr = (addr_t)(word_t)phys;

    /*
     * Find pagesize to use, and number of pages to map.
     */
    for (pgsize = pgent_t::size_max; page_shift(pgsize) > t_num; pgsize --) { }

    t_num = 1UL << (t_num - page_shift(pgsize));
    t_size =  pgent_t::size_max;

    tpg = this->pgent (page_table_index (t_size, t_addr));

    space_t::begin_update ();

    TRACEPOINT (FPAGE_MAP,
		word_t tsz = page_size (pgsize);
		word_t msz = dest_fp.get_size();
		printf ("map_fpage (fpg=%p  t_spc=%p) : "
		        "addr=%p, phys=%p  map=%d%cB, sz=%d%cB\n",
			dest_fp.raw, this, t_addr, p_addr,
			dbg_pgsize (msz), dbg_szname (msz),
			dbg_pgsize (tsz), dbg_szname (tsz)));

    /*
     * Lookup destination pagetable, inserting levels when needed,
     * then insert mappings.
     */
    while (t_num > 0)
    {
	TRACE_MAP("  . LOOP: t_num = %d, t_size = %d, pgsize = %d, tpg = %p\n",
		t_num, t_size, pgsize, tpg);

	if (EXPECT_FALSE(t_size > pgsize))
	{
	    /*
	     * We are currently working on too large pages.
	     */
	    t_size --;
	    r_tpg[t_size] = tpg->next (this, t_size+1, 1);
	    r_tnum[t_size] = t_num - 1;
map_next_level:

	    if (EXPECT_FALSE(! tpg->is_valid (this, t_size+1)))
	    {
		/*
		 * Subtree does not exist. Create one.
		 */
		if (!tpg->make_subtree (this, t_size+1, false))
		    goto map_fpage_fail;
		TRACE_MAP("   - made subtree on tpg = %p\n", tpg);
	    }
	    else if (EXPECT_FALSE(! tpg->is_subtree (this, t_size+1)))
	    {
		t_size ++;
		/*
		 * A larger mapping exists where a smaller mapping
		 * is being inserted.
		 */
		if ( /* Make sure that we do not overmap KIP,UTCB or kernel area */
			!this->is_mappable(t_addr, addr_offset (t_addr, page_size (t_size)))
		   )
		{
		    goto map_next_pgentry;
		}

		/*
		 * Delete the larger mapping.
		 */
		tpg->clear(this, t_size, false, t_addr);
		TRACE_MAP("   - removed blocking superpage, tpg = %p, size %d, addr %p\n",
			tpg, t_size, t_addr);

		/* We might have to flush some TLB entries */
		this->flush_tlbent (get_current_space (), t_addr, page_shift (t_size));

		/* restart with mapping removed */
		continue;
	    }
	    /*
	     * Entry is valid and a subtree.
	     */

	    /*
	     * Just follow tree and continue
	     */
	    tpg = tpg->subtree (this, t_size+1)->next
		(this, t_size, page_table_index (t_size, t_addr));
	    continue;
	}
	else if (EXPECT_FALSE(tpg->is_valid (this, t_size) &&

		/* Check if we're simply extending access rights */
		(tpg->is_subtree (this, t_size) ||
		 (tpg->address (this, t_size) != p_addr)) &&

		/* Make sure that we do not overmap KIP,UTCB or kernel area */
		this->is_mappable(t_addr, addr_offset (t_addr, page_size (t_size)))
		))
	{
	    /*
	     * We are doing overmapping.  Need to remove existing
	     * mapping or subtree from address space.
	     */
	    TRACEPOINT (FPAGE_OVERMAP,
			word_t tsz = page_size (t_size);
			printf ("overmapping: "
				"taddr=%p tsz=%d%cB "
				"(%s)\n",
				t_addr, dbg_pgsize (tsz), dbg_szname (tsz),
				tpg->is_subtree (this, t_size) ?
				"subtree" : "single map"));

	    word_t num = 1;
	    pgent_t::pgsize_e s_size = t_size;
	    addr_t vaddr = t_addr;

	    while (num > 0)
	    {
		TRACE_MAP("    ovr - tpg = %p, t_size = %d, v_addr = %p\n", tpg, t_size, vaddr);
		if (! tpg->is_valid (this, t_size))
		{
		    /* Skip invalid entries. */
		}
		else if (tpg->is_subtree (this, t_size))
		{
		    /* We have to search each single page in the subtree. */
		    t_size--;

		    r_tpg[t_size] = tpg;
		    r_tnum[t_size] = num - 1;

		    tpg = tpg->subtree (this, t_size+1);
		    num = page_table_size (t_size);
		    continue;
		}
		else
		{
		    tpg->clear(this, t_size, false, vaddr);
		    /* We might have to flush some TLB entries */
		    if (! this->does_tlbflush_pay (page_shift (t_size)))
		    {
			this->flush_tlbent (get_current_space (), vaddr, page_shift (t_size));
		    }
		}

		if (t_size < s_size)
		{
		    /* Skip to next entry. */
		    vaddr = addr_offset (vaddr, page_size (t_size));
		    tpg = tpg->next (this, t_size, 1);
		}

		num--;
		while (num == 0 && t_size < s_size)
		{
		    /* Recurse up and remove subtree. */
		    tpg = r_tpg[t_size];
		    num = r_tnum[t_size];
		    t_size++;
		    tpg->remove_subtree (this, t_size, false);
		    if (t_size < s_size)
			tpg = tpg->next (this, t_size, 1);
		};

	    }

	    /* We might have to flush the TLB after removing mappings. */
	    if (space_t::does_tlbflush_pay (page_shift (t_size)))
	    {
		flush = true;
	    }
	}
	else if (EXPECT_FALSE(tpg->is_valid (this, t_size) &&
		    tpg->is_subtree (this, t_size)))
	{
	    /* Target mappings are of smaller page size.
	     * Recurse to lower level
	     */
	    goto map_recurse_down;
	}

	if (EXPECT_FALSE(!is_page_size_valid (t_size)))
	{
	    /*
	     * Pagesize is ok but is not a valid hardware pagesize or
	     * target mappings are of smaller page size.
	     * Need to recurse to a smaller size.
	     */
map_recurse_down:
	    t_size--;
	    r_tpg[t_size] = tpg->next (this, t_size+1, 1);
	    r_tnum[t_size] = t_num - 1;

	    t_num = page_table_size (t_size);
	    goto map_next_level;
	}

	/*
	 * If we get here `tpg' will be the page table entry that we
	 * are going to change.
	 */

	if (EXPECT_TRUE(this->is_mappable (t_addr)))
	{
	    bool valid = tpg->is_valid (this, t_size);

	    /*
	     * This is where the real work is done.
	     */
	    TRACE_MAP("  * set entry: %p: t_size %d, v_addr = %p, p_addr = %p, r=%d,w=%d,x=%d  attr=%d\n",
		tpg, t_size, t_addr, p_addr, dest_fp.is_read(),
		dest_fp.is_write(), dest_fp.is_execute(), base.get_attributes());

	    tpg->set_entry (this, t_size, p_addr,
		    dest_fp.is_read(), dest_fp.is_write(),
		    dest_fp.is_execute(), false,
		    page_attributes);

	    if (EXPECT_FALSE(valid && !flush))
	    {
		if (!space_t::does_tlbflush_pay (page_shift (t_size)))
		    flush_tlbent (get_current_space (), t_addr, page_shift (t_size));
		else
		    flush = true;
	    }
	}

	/* Move on to the next map entry */
map_next_pgentry:
	preempt_enable();

	t_addr = addr_offset (t_addr, page_size (t_size));
	p_addr = addr_offset (p_addr, page_size (t_size));
	t_num--;

	preempt_disable();

	if (EXPECT_TRUE(t_num > 0))
	{
	    /* Go to next entry */
	    tpg = tpg->next (this, t_size, 1);
	    continue;
	}
	else if (t_size < pgsize)
	{
	    do {
		/* Recurse up */
		tpg = r_tpg[t_size];
		t_num = r_tnum[t_size];
		t_size++;
	    } while (t_size < pgsize && t_num == 0);
	}
    }

    if (flush)
	this->flush_tlb (get_current_space ());

    space_t::end_update ();
    return true;

map_fpage_fail:

    space_t::end_update ();
    this->unmap_fpage (dest_fp, false);
    return false;
}

/**
 * Unmaps the indicated fpage.
 *
 * @param fpage''       fpage to unmap
 * @param unmap_all'    also unmap kernel mappings (i.e., UTCB and KIP)
 *
 * @returns_
 */
void space_t::unmap_fpage(fpage_t fpage, bool unmap_all)
{
    pgent_t::pgsize_e size, pgsize;
    pgent_t * pg;
    addr_t vaddr;
    word_t num;
    bool flush = false;

    pgent_t *r_pg[pgent_t::size_max];
    word_t r_num[pgent_t::size_max];

    num = fpage.get_size_log2 ();
    vaddr = address (fpage, num);

    if (num < hw_pgshifts[0])
    {
	ASSERT(DEBUG, !"invalid fpage");
	return;
    }

    TRACEPOINT (FPAGE_UNMAP,
		printf ("unmap_fpage (fpg=%p  t_spc=%p)\n",
			fpage.raw, this));

    /*
     * Some architectures may not support a complete virtual address
     * space.  Enforce unmaps to only cover the supported space.
     */

    if (num > page_shift(pgent_t::size_max+1))
	num = page_shift(pgent_t::size_max+1);

    /*
     * Find pagesize to use, and number of pages to map.
     */

    for (pgsize = pgent_t::size_max; page_shift(pgsize) > num; pgsize--) {}

    num = 1UL << (num - page_shift(pgsize));
    size = pgent_t::size_max;
    pg = this->pgent (page_table_index (size, vaddr));

    space_t::begin_update ();

    while (num)
    {
	TRACE_UNMAP("  . LOOP: num = %d, size = %d, pgsize = %d, pg = %p, vaddr = %p\n", num, size, pgsize, pg, vaddr);
	if (! is_user_area (vaddr))
	    /* Do not mess with kernel area. */
	    break;

	bool valid = pg->is_valid (this, size);
	bool subtree = pg->is_subtree(this, size);

	if (EXPECT_FALSE(size > pgsize))
	{
	    /* We are operating on too large page sizes. */
	    if (!valid)
		break;
	    else if (subtree)
	    {
		size--;
		pg = pg->subtree (this, size+1)->next
		    (this, size, page_table_index (size, vaddr));
		continue;
	    }
	    else
	    {
		/*
		 * A larger mapping exists where a smaller page
		 * is being removed.
		 */
		if ( /* Make sure that we do not remove KIP,UTCB or kernel area */
			!this->is_mappable(vaddr, addr_offset (vaddr, page_size (size)))
		   )
		{
		    goto unmap_next_pgentry;
		}

		/*
		 * Delete the larger mapping.
		 */
		TRACE_UNMAP("   - remove superpage, pg = %p, size %d, addr %p\n",
			pg, size, vaddr);
		pg->clear(this, size, false, vaddr);

		/* We might have to flush some TLB entries */
		flush = true;

		/* restart with mapping removed */
		continue;
	    }
	}

	if (EXPECT_FALSE(!valid))
	    goto unmap_next_pgentry;

	if (EXPECT_FALSE(subtree))
	{
	    if ( (size == pgent_t::min_tree) &&
		    (unmap_all ||
		     /* Make sure that we do not remove KIP and UTCB area */
		     this->is_mappable(vaddr, addr_offset (vaddr, page_size (size))))
	       )
	    {
		/* Just remove the subtree. */
		pg->remove_subtree (this, size, false);
		flush = true;
		goto unmap_next_pgentry;
	    }
	    else
	    {
		/* We have to flush each single entry in the subtree. */
		size--;
		r_pg[size] = pg;
		r_num[size] = num - 1;

		pg = pg->subtree (this, size+1);
		num = page_table_size (size);
		continue;
	    }
	}

	if (EXPECT_TRUE(is_mappable (vaddr) || unmap_all))
	{
	    TRACE_UNMAP("   - remove %p: vaddr = %p\n", pg, vaddr);
	    pg->clear (this, size, true, vaddr);

	    if (EXPECT_FALSE(!flush))
	    {
		if (!space_t::does_tlbflush_pay (page_shift (size)))
		    flush_tlbent (get_current_space (), vaddr, page_shift (size));
		else
		    flush = true;
	    }
	}

unmap_next_pgentry:

	preempt_enable();

	pg = pg->next (this, size, 1);
	vaddr = addr_offset (vaddr, page_size (size));
	num--;

	preempt_disable();

	/* Delete all subtrees below fpage size */
	while (num == 0 && size < pgsize)
	{
	    /* Recurse up */
	    pg = r_pg[size];
	    num = r_num[size];
	    size++;

	    fpage_t fp;
	    fp.set ((word_t) vaddr - page_size (size), page_shift(size), false, false, false);

	    if (unmap_all || is_mappable (fp))
	    {
		TRACE_UNMAP("   - rm tree %p: size = %x\n", pg, page_size(size));
		pg->remove_subtree (this, size, false);
	    }

	    pg = pg->next (this, size, 1);
	}
    }

    if (flush)
    {
	preempt_enable();
	flush_tlb (get_current_space ());
	preempt_disable();
    }

    space_t::end_update ();
}

void space_t::read_fpage(fpage_t fpage, phys_desc_t *base, perm_desc_t *perm)
{
    pgent_t::pgsize_e size, pgsize;
    pgent_t * pg;
    addr_t vaddr;
    word_t num, rwx = 0, RWX = 0;
    word_t count = 0;

    pgent_t *r_pg[pgent_t::size_max];
    word_t r_num[pgent_t::size_max];

    num = fpage.get_size_log2 ();
    vaddr = address (fpage, num);

    if (num < hw_pgshifts[0])
    {
	ASSERT(DEBUG, !"invalid fpage");
	return;
    }

    /*
     * Some architectures may not support a complete virtual address
     * space.  Enforce unmaps to only cover the supported space.
     */

    if (num > page_shift(pgent_t::size_max+1))
	num = page_shift(pgent_t::size_max+1);

    /*
     * Find pagesize to use, and number of pages.
     */

    for (pgsize = pgent_t::size_max; page_shift(pgsize) > num; pgsize--) {}

    num = 1UL << (num - page_shift(pgsize));
    size = pgent_t::size_max;
    pg = this->pgent (page_table_index (size, vaddr));

    space_t::begin_update ();

    while (num)
    {
	TRACE_READ("  . LOOP: num = %d, size = %d, pgsize = %d, pg = %p\n", num, size, pgsize, pg);
	if (! is_user_area (vaddr))
	    /* Do not mess with kernel area. */
	    break;

	if (size > pgsize)
	{
	    /* We are operating on too large page sizes. */
	    if (! pg->is_valid (this, size))
		break;
	    else if (pg->is_subtree (this, size))
	    {
		size--;
		pg = pg->subtree (this, size+1)->next
		    (this, size, page_table_index (size, vaddr));
		continue;
	    }
	    else /* implicitly is_valid() */
	    {
		/* fpage specifies a smaller page than exists */
		RWX = pg->reference_bits(this, size, vaddr);

		rwx = (pg->is_readable(this, size) << 2) |
		      (pg->is_writable(this, size) << 1) |
		      (pg->is_executable(this, size) << 0);

		count ++;
		break;
	    }
	}

	if (! pg->is_valid (this, size))
	    goto read_next_pgentry;

	if (pg->is_subtree (this, size))
	{
	    /* We have to flush each single page in the subtree. */
	    size--;
	    r_pg[size] = pg;
	    r_num[size] = num - 1;

	    pg = pg->subtree (this, size+1);
	    num = page_table_size (size);
	    continue;
	}

	if (is_mappable (vaddr))
	{
	    TRACE_READ("   - read %p: vaddr = %p\n", pg, vaddr);
	    rwx |= (pg->is_readable(this, size) << 2) |
		   (pg->is_writable(this, size) << 1) |
		   (pg->is_executable(this, size) << 0);
	    RWX |= pg->reference_bits(this, size, vaddr);
	    if (!count && base)
	    {
		phys_desc_t b;
		b.clear();
		b.set_base((word_t)pg->address(this, size));
		b.set_attributes(to_l4attrib(pg->get_attributes(this, size)));
		*base = b;
		count++;
	    }
	    /* FIXME: do this properly */
	    /* compensate for multiple entries needed to support superpages */
	    if(base && base->get_base() != ((word_t)pg->address(this, size)))
		count ++;
	}

read_next_pgentry:

	preempt_enable();

	pg = pg->next (this, size, 1);
	vaddr = addr_offset (vaddr, page_size (size));
	num--;

	preempt_disable();

	while (num == 0 && size < pgsize)
	{
	    /* Recurse up */
	    pg = r_pg[size];
	    num = r_num[size];
	    size++;

	    pg = pg->next (this, size, 1);
	}
    }

    space_t::end_update ();

    perm->set_reference(RWX);
    perm->set_perms(rwx);

    if (count == 1)
	perm->set_distinct();	// mark this as distinct
    return;
}

/**
 * Read word from address space.  Parses page tables to find physical
 * address of mapping and reads the indicated word directly from
 * kernel-mapped physical memory.
 *
 * @param vaddr		virtual address to read
 * @param contents	returned contents in given virtual location
 *
 * @return true if mapping existed, false otherwise
 */
bool space_t::readmem (addr_t vaddr, word_t * contents)
{
    pgent_t * pg;
    pgent_t::pgsize_e pgsize;

    if (! lookup_mapping (vaddr, &pg, &pgsize))
	return false;

    addr_t paddr = pg->address (this, pgsize);
    paddr = addr_offset (paddr, (word_t) vaddr & page_mask (pgsize));
    addr_t paddr1 = addr_mask (paddr, ~(sizeof (word_t) - 1));

    if (paddr1 == paddr)
    {
	// Word access is properly aligned.
	*contents = readmem_phys (paddr);
    }
    else
    {
	// Word access not properly aligned.  Need to perform two
	// separate accesses.
	addr_t paddr2 = addr_offset (paddr1, sizeof (word_t));
	word_t mask = ~page_mask (pgsize);

	if (addr_mask (paddr1, mask) != addr_mask (paddr2, mask))
	{
	    // Word access crosses page boundary.
	    vaddr = addr_offset (vaddr, sizeof (word_t));
	    if (! lookup_mapping (vaddr, &pg, &pgsize))
		return false;
	    paddr2 = pg->address (this, pgsize);
	    paddr2 = addr_offset (paddr2, (word_t) vaddr & page_mask (pgsize));
	    paddr2 = addr_mask (paddr2, ~(sizeof (word_t) - 1));
	}

	word_t idx = ((word_t) vaddr) & (sizeof (word_t) - 1);

#if defined(CONFIG_BIGENDIAN)
	*contents =
	    (readmem_phys (paddr1) << (idx * 8)) |
	    (readmem_phys (paddr2) >> ((sizeof (word_t) - idx) * 8));
#else
	*contents =
	    (readmem_phys (paddr1) >> (idx * 8)) |
	    (readmem_phys (paddr2) << ((sizeof (word_t) - idx) * 8));
#endif
    }

    return true;
}


/**
 * Lookup mapping in address space.  Parses the page table to find a
 * mapping for the indicated virtual address.  Also returns located
 * page table entry and page size in R_PG and R_SIZE (if non-nil).
 *
 * @param vaddr		virtual address
 * @param r_pg		returned page table entry for mapping
 * @param r_size	returned page size for mapping
 *
 * @return true if mapping exists, false otherwise
 */
bool space_t::lookup_mapping (addr_t vaddr, pgent_t ** r_pg,
			      pgent_t::pgsize_e * r_size)
{
    pgent_t * pg = this->pgent (page_table_index (pgent_t::size_max, vaddr));
    pgent_t::pgsize_e pgsize = pgent_t::size_max;

    for (;;)
    {
	if (pg->is_valid (this, pgsize))
	{
	    if (pg->is_subtree (this, pgsize))
	    {
		// Recurse into subtree
		if (pgsize == 0)
		    return false;

		pg = pg->subtree (this, pgsize)->next
		    (this, pgsize-1, page_table_index (pgsize-1, vaddr));
		pgsize--;
	    }
	    else
	    {
		// Return mapping
		if (r_pg)
		    *r_pg = pg;
		if (r_size)
		    *r_size = pgsize;
		return true;
	    }
	}
	else
	    // No valid mapping or subtree
	    return false;
    }

    /* NOTREACHED */
    return false;
}

#endif /* !__GENERIC__LINEAR_PTAB_WALKER_CC__ */
