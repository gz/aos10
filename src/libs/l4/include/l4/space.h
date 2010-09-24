/*********************************************************************
 *                
 * Copyright (C) 2001-2004,  Karlsruhe University
 *                
 * File path:     l4/space.h
 * Description:   Interfaces for handling address spaces/mappings
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
 * $Id: space.h,v 1.12 2004/03/12 17:50:56 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __L4__SPACE_H__
#define __L4__SPACE_H__

#include <l4/types.h>
#include <l4/map.h>
#include <l4/kip.h>
#include __L4_INC_ARCH(syscalls.h)

#include __L4_INC_ARCH(kdebug.h)

/*
 * Derived functions
 */

L4_INLINE L4_Word_t L4_MapFpage (L4_ThreadId_t s, L4_Fpage_t f, L4_PhysDesc_t p)
{
    L4_LoadMR (0, p.raw);
    L4_LoadMR (1, f.raw);

    return L4_MapControl(s, L4_MapCtrl_Modify);
}

L4_INLINE L4_Word_t L4_MapFpages (L4_ThreadId_t s,
	L4_Word_t n, L4_Fpage_t * fpages, L4_PhysDesc_t * descs)
{
    int i;
    for (i = 0; i < n; i++) {
	if ((fpages[i].raw & 0x7) == 0)
	    L4_KDB_Enter("eek");
	L4_LoadMR (2*i, descs[i].raw);
	L4_LoadMR (2*i + 1, fpages[i].raw);
    }
    return L4_MapControl(s, L4_MapCtrl_Modify | (n - 1));
}

L4_INLINE L4_Word_t L4_UnmapFpage (L4_ThreadId_t s, L4_Fpage_t f)
{
    L4_LoadMR (1, f.raw & (~0xFUL));

    return L4_MapControl(s, L4_MapCtrl_Modify);
}

L4_INLINE L4_Word_t L4_UnmapFpages (L4_ThreadId_t s,
	L4_Word_t n, L4_Fpage_t * fpages)
{
    int i;
    for (i = 0; i < n; i++) {
	L4_LoadMR (2*i+1, fpages[i].raw & (~0xfUL));
    }
    return L4_MapControl(s, L4_MapCtrl_Modify | (n - 1));
}

#if defined(__l4_cplusplus)
L4_INLINE L4_Fpage_t L4_Unmap (L4_ThreadId_t s, L4_Fpage_t f)
{
    return L4_UnmapFpage (s, f);
}

L4_INLINE L4_Word_t L4_Unmap (L4_ThreadId_td s, L4_Word_t n,
	L4_Fpage_t * fpages, L4_PhysDesc_t * descs)
{
    return L4_UnmapFpages (s, n, fpages, descs);
}
#endif

L4_INLINE L4_Word_t L4_GetStatus (L4_ThreadId_t s, L4_Fpage_t * f, L4_PhysDesc_t * p)
{
    L4_Word_t r;
    L4_LoadMR (1, f->raw);
    r = L4_MapControl(s, L4_MapCtrl_Read);
    if (p)
	L4_StoreMR (0, &p->raw);
    L4_StoreMR (1, &f->raw);
    return r;
}

L4_INLINE L4_Bool_t L4_WasWritten (L4_Fpage_t f)
{
    return (f.raw & L4_Writable) != 0;
}

L4_INLINE L4_Bool_t L4_WasReferenced (L4_Fpage_t f)
{
    return (f.raw & L4_Readable) != 0;
}

L4_INLINE L4_Bool_t L4_WaseXecuted (L4_Fpage_t f)
{
    return (f.raw & L4_eXecutable) != 0;
}



#endif /* !__L4__SPACE_H__ */
