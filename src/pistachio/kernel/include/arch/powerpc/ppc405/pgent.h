/****************************************************************************
 *
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *
 * File path:	arch/powerpc/ppc405/pgent.h
 * Description:	The page table entry.  Compatible with the generic linear
 * 		page table walker.
 * Author:	Carl van Schaik
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

#ifndef __ARCH__POWERPC__PPC405__PGENT_H__
#define __ARCH__POWERPC__PPC405__PGENT_H__

class pgent_t : public base_pgent_t
{
public:
    /* pagetable entry format */
    union {
	word_t raw;
	struct {
	    BITFIELD4(word_t,
		subtree		: 28,		/* Pointer to subtree			*/
		__res		: 2,
		is_valid	: 1,		/* Entry is valid (0 for subtrees)	*/
		is_subtree	: 1		/* Entry is subtree (0 for entry)	*/
	    )
	} tree;
	struct {
	    BITFIELD10(word_t,
		rpn		: 22,		/* Real page number			*/
		ex		: 1,		/* Execute permissions			*/
		wr		: 1,		/* Write permissions			*/
		endian		: 1,		/* 0 = big, 1 = little endian	^a	*/
		u0		: 1,		/* user defined attribute (U0)	^a	*/
		zone		: 2,		/* Current zone (we use 0..2)		*/
		w		: 1,		/* 0 - writeback, 1 - write-through	*/
		i		: 1,		/* 0 - cached, 1 - noncached		*/
		is_valid	: 1,		/* Entry is valid		^b	*/
		g		: 1		/* Guarded (IO memory)			*/
		/* ^a : These fields are in the 'ZSEL' of the TLB, zero before inserting
		 * ^b : This field in the 'm' of the TLB, zero before inserting		*/
	    )
	    /* At some stage it may be useful to have a zone page table which contains
	     * extra zone bits for tlbs and provide a interface to zones for user	*/
	} entry;
    };

    enum pgsize_e {
	size_1k	    = 0,
	size_4k	    = 1,
	size_16k    = 2,
	size_64k    = 3,
	size_256k   = 4,
	size_1m	    = 5,
	size_4m	    = 6,
	size_16m    = 7,
	size_4g	    = 8,
	size_max    = size_16m
    };
};

#endif /* __ARCH__POWERPC__PPC405__PGENT_H__ */
