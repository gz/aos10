/*******************************************************************************
* Filename:    src/arch-powerpc64/ofsetup.c                                   *
* Description: Elf-loader - ELF file kernel/application bootstraper, source    *
*              file defining the architecture specific functions for the PPC64 *
*              architecture.                                                   *
* Authors:     Carl van Schaik.                                                *
* Created:     2005-06-01                                                      *
********************************************************************************
*                                                                              *
* Australian Public Licence B (OZPLB)                                          *
*                                                                              *
* Version 1-0                                                                  *
*                                                                              *
* Copyright (c) 2004 - 2005 University of New South Wales, Australia           *
*                                                                              *
* All rights reserved.                                                         *
*                                                                              *
* Developed by: Operating Systems, Embedded and                                *
*               Distributed Systems Group (DiSy)                               *
*               University of New South Wales                                  *
*               http://www.disy.cse.unsw.edu.au                                *
*                                                                              *
* Permission is granted by University of New South Wales, free of charge, to   *
* any person obtaining a copy of this software and any associated              *
* documentation files (the "Software") to deal with the Software without       *
* restriction, including (without limitation) the rights to use, copy,         *
* modify, adapt, merge, publish, distribute, communicate to the public,        *
* sublicense, and/or sell, lend or rent out copies of the Software, and        *
* to permit persons to whom the Software is furnished to do so, subject        *
* to the following conditions:                                                 *
*                                                                              *
*     * Redistributions of source code must retain the above copyright         *
*       notice, this list of conditions and the following disclaimers.         *
*                                                                              *
*     * Redistributions in binary form must reproduce the above                *
*       copyright notice, this list of conditions and the following            *
*       disclaimers in the documentation and/or other materials provided       *
*       with the distribution.                                                 *
*                                                                              *
*     * Neither the name of University of New South Wales, nor the names of    *
*        its contributors, may be used to endorse or promote products derived  *
*       from this Software without specific prior written permission.          *
*                                                                              *
* EXCEPT AS EXPRESSLY STATED IN THIS LICENCE AND TO THE FULL EXTENT            *
* PERMITTED BY APPLICABLE LAW, THE SOFTWARE IS PROVIDED "AS-IS", AND           *
* NATIONAL ICT AUSTRALIA AND ITS CONTRIBUTORS MAKE NO REPRESENTATIONS,         *
* WARRANTIES OR CONDITIONS OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING          *
* BUT NOT LIMITED TO ANY REPRESENTATIONS, WARRANTIES OR CONDITIONS             *
* REGARDING THE CONTENTS OR ACCURACY OF THE SOFTWARE, OR OF TITLE,             *
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NONINFRINGEMENT,          *
* THE ABSENCE OF LATENT OR OTHER DEFECTS, OR THE PRESENCE OR ABSENCE OF        *
* ERRORS, WHETHER OR NOT DISCOVERABLE.                                         *
*                                                                              *
* TO THE FULL EXTENT PERMITTED BY APPLICABLE LAW, IN NO EVENT SHALL            *
* NATIONAL ICT AUSTRALIA OR ITS CONTRIBUTORS BE LIABLE ON ANY LEGAL            *
* THEORY (INCLUDING, WITHOUT LIMITATION, IN AN ACTION OF CONTRACT,             *
* NEGLIGENCE OR OTHERWISE) FOR ANY CLAIM, LOSS, DAMAGES OR OTHER               *
* LIABILITY, INCLUDING (WITHOUT LIMITATION) LOSS OF PRODUCTION OR              *
* OPERATION TIME, LOSS, DAMAGE OR CORRUPTION OF DATA OR RECORDS; OR LOSS       *
* OF ANTICIPATED SAVINGS, OPPORTUNITY, REVENUE, PROFIT OR GOODWILL, OR         *
* OTHER ECONOMIC LOSS; OR ANY SPECIAL, INCIDENTAL, INDIRECT,                   *
* CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES, ARISING OUT OF OR IN           *
* CONNECTION WITH THIS LICENCE, THE SOFTWARE OR THE USE OF OR OTHER            *
* DEALINGS WITH THE SOFTWARE, EVEN IF NATIONAL ICT AUSTRALIA OR ITS            *
* CONTRIBUTORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH CLAIM, LOSS,       *
* DAMAGES OR OTHER LIABILITY.                                                  *
*                                                                              *
* If applicable legislation implies representations, warranties, or            *
* conditions, or imposes obligations or liability on University of New South   *
* Wales or one of its contributors in respect of the Software that             *
* cannot be wholly or partly excluded, restricted or modified, the             *
* liability of University of New South Wales or the contributor is limited, to *
* the full extent permitted by the applicable legislation, at its              *
* option, to:                                                                  *
* a.  in the case of goods, any one or more of the following:                  *
* i.  the replacement of the goods or the supply of equivalent goods;          *
* ii.  the repair of the goods;                                                *
* iii. the payment of the cost of replacing the goods or of acquiring          *
*  equivalent goods;                                                           *
* iv.  the payment of the cost of having the goods repaired; or                *
* b.  in the case of services:                                                 *
* i.  the supplying of the services again; or                                  *
* ii.  the payment of the cost of having the services supplied again.          *
*                                                                              *
* The construction, validity and performance of this licence is governed       *
* by the laws in force in New South Wales, Australia.                          *
*                                                                              *
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <openfirmware/openfirmware.h>
#include <openfirmware/devicetree.h>

/* libc - sys-baremetal crt0.S calls this */
void of_entry(uintptr_t, uintptr_t, void *);

uintptr_t treebuf[0x10000];

of_tree_t device_tree;
size_t tree_size;

void * firmware_entry;

void of_entry(uintptr_t initrd_start, uintptr_t initrd_size, void *entry)
{
	uintptr_t *treestart = treebuf;

	firmware_entry = entry;

	of_init(entry);

	of_puts( "\r\n[==== Kenge PowerPC64 OpenFirmware elf-loader ====]\r\n" );

	/* Align device tree on a page boundary */
	if ((uintptr_t)treestart & 0xfff)
		treestart = (uintptr_t *)(((uintptr_t)treestart + 0xfff) & (~0xffful));

	of_tree_init( &device_tree, treestart );

	tree_size = of_populate_device_tree( &device_tree );
}

