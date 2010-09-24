/*********************************************************************
 *                
 * Copyright (C) 2001-2004,  Karlsruhe University
 *                
 * File path:     l4/types.h
 * Description:   Commonly used L4 types
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
 * $Id: types.h,v 1.41 2004/08/23 18:41:45 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __L4__TYPES_H__
#define __L4__TYPES_H__

#undef L4_32BIT
#undef L4_64BIT
#undef L4_BIG_ENDIAN
#undef L4_LITTLE_ENDIAN

/*
 * Determine which architecture dependent files to include
 */

#if !defined(__L4_ARCH__)
# if defined(__i386__)
#  define L4_ARCH_IA32
#  define __L4_ARCH__ ia32
# elif defined(__ia64__)
#  undef ia64
#  define L4_ARCH_IA64
#  define __L4_ARCH__ ia64
# elif defined(__PPC64__)
#  define L4_ARCH_POWERPC64
#  define __L4_ARCH__ powerpc64
# elif defined(__PPC__)
#  undef powerpc
#  define L4_ARCH_POWERPC
#  define __L4_ARCH__ powerpc
# elif defined(ARCH_ARM) || defined(__arm__) || defined(__thumb__) || defined(__ARMCC_VERSION) 
#  define L4_ARCH_ARM
#  define __L4_ARCH__ arm
# elif defined(__x86_64__)
#  define L4_ARCH_AMD64
#  define __L4_ARCH__ amd64
# elif defined(__alpha__)
#  define L4_ARCH_ALPHA
#  define __L4_ARCH__ alpha
# elif defined(__mips__)
#  define L4_ARCH_MIPS64
#  define __L4_ARCH__ mips64
# elif defined(__sparc__)
#  define L4_ARCH_SPARC64
#  define __L4_ARCH__ sparc64
# else
#  error Unknown hardware architecture.
# endif
#endif

#if !defined(__ARMCC_VERSION) && !defined(__GNUC__)
  #define __GNUC__
#endif

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION<210000)
  //this is ADS 1.2
  #define __L4_INC_ARCH(file) <l4/arm/##file##>
#else /* __GNUC__ */
#define __L4_INC_ARCH(file) <l4/__L4_ARCH__/file>
#endif

#include __L4_INC_ARCH(types.h)

#if defined (__ARMCC_VERSION) && (__ARMCC_VERSION<210000)
#if defined(__l4_cplusplus)
  #define __CPP "C"
#else
  #define __CPP
#endif
#define __attribute__(a) 
#endif



/*
 * All types used within <arch/syscalls.h> should be declared in this
 * file.
 */

typedef L4_Word_t		L4_Bool_t;

#if defined(__ARMCC_VERSION)
#define L4_INLINE		__inline
#else
#define L4_INLINE		static inline
#endif
#define L4_BITS_PER_WORD	(sizeof (L4_Word_t) * 8)

// XXX: magpie workaround
// # define __PLUS32	+ (sizeof (L4_Word_t) * 8 - 32)
#if defined(L4_64BIT)
# define __PLUS32	+ 32
# define __14		32
# define __18		32
#else
# define __PLUS32
# define __14		14
# define __18		18
#endif

#if defined(L4_BIG_ENDIAN)
#define L4_BITFIELD2(t,a,b)		t b; t a
#define L4_BITFIELD3(t,a,b,c)		t c; t b; t a
#define L4_BITFIELD4(t,a,b,c,d)		t d; t c; t b; t a
#define L4_BITFIELD5(t,a,b,c,d,e)	t e; t d; t c; t b; t a
#define L4_BITFIELD6(t,a,b,c,d,e,f)	t f; t e; t d; t c; t b; t a
#define L4_BITFIELD7(t,a,b,c,d,e,f,g)	t g; t f; t e; t d; t c; t b; t a
#define L4_BITFIELD8(t,a,b,c,d,e,f,g,h)	t h; t g; t f; t e; t d; t c; t b; t a
#define L4_BITFIELD9(t,a,b,c,d,e,f,g,h,i)	\
	t i; t h; t g; t f; t e; t d; t c; t b; t a
#define L4_SHUFFLE2(a,b)		b,a
#define L4_SHUFFLE3(a,b,c)		c,b,a
#define L4_SHUFFLE4(a,b,c,d)		d,c,b,a
#define L4_SHUFFLE5(a,b,c,d,e)		e,d,c,b,a
#define L4_SHUFFLE6(a,b,c,d,e,f)	f,e,d,c,b,a
#define L4_SHUFFLE7(a,b,c,d,e,f,g)	g,f,e,d,c,b,a
#else
#define L4_BITFIELD2(t,a,b)		t a; t b
#define L4_BITFIELD3(t,a,b,c)		t a; t b; t c
#define L4_BITFIELD4(t,a,b,c,d)		t a; t b; t c; t d
#define L4_BITFIELD5(t,a,b,c,d,e)	t a; t b; t c; t d; t e
#define L4_BITFIELD6(t,a,b,c,d,e,f)	t a; t b; t c; t d; t e; t f
#define L4_BITFIELD7(t,a,b,c,d,e,f,g)	t a; t b; t c; t d; t e; t f; t g
#define L4_BITFIELD8(t,a,b,c,d,e,f,g,h)	t a; t b; t c; t d; t e; t f; t g; t h
#define L4_BITFIELD9(t,a,b,c,d,e,f,g,h,i)	\
	t a; t b; t c; t d; t e; t f; t g; t h; t i
#define L4_SHUFFLE2(a,b)		a,b
#define L4_SHUFFLE3(a,b,c)		a,b,c
#define L4_SHUFFLE4(a,b,c,d)		a,b,c,d
#define L4_SHUFFLE5(a,b,c,d,e)		a,b,c,d,e
#define L4_SHUFFLE6(a,b,c,d,e,f)	a,b,c,d,e,f
#define L4_SHUFFLE7(a,b,c,d,e,f,g)	a,b,c,d,e,f,g
#endif

/*
 * Exregs controls
 */
#define L4_ExReg_sp		(1 << 3)
#define L4_ExReg_ip		(1 << 4)
#define L4_ExReg_sp_ip		(L4_ExReg_sp | L4_ExReg_ip)
#define L4_ExReg_flags		(1 << 5)
#define L4_ExReg_sp_ip_flags	(L4_ExReg_sp | L4_ExReg_ip | L4_ExReg_flags)
#define L4_ExReg_user		(1 << 6)
#define L4_ExReg_pager		(1 << 7)
#define L4_ExReg_Halt		(1 << 8 | 1 << 0)
#define L4_ExReg_Resume		(1 << 8)
#define L4_ExReg_AbortIPC	(1 << 1 | 1 << 2)
#define L4_ExReg_AbortRecvIPC	(1 << 1)
#define L4_ExReg_AbortSendIPC	(1 << 2)
#define L4_ExReg_Deliver	(1 << 9)
#define L4_ExReg_VolatileRegs	(1 << 10)
#define L4_ExReg_SavedRegs	(1 << 11)
#define L4_ExReg_RegsToMRs	(1 << 12)
#define L4_ExReg_SrcThread(x)	(((L4_GthreadId_t) { X : {L4_SHUFFLE2(0, L4_ThreadNo(x))}}).raw)

/*
 * Error codes
 */

#define L4_ErrOk		(0)
#define L4_ErrNoPrivilege	(1)
#define L4_ErrInvalidThread	(2)
#define L4_ErrInvalidSpace	(3)
#define L4_ErrInvalidScheduler	(4)
#define L4_ErrInvalidParam	(5)
#define L4_ErrUtcbArea		(6)
#define L4_ErrKipArea		(7)
#define L4_ErrNoMem		(8)
#define L4_ErrInvalidRedirector	(9)

/*
 * IPC Error Codes
 */
#define L4_ErrTimeout		(1)
#define L4_ErrNonExist		(2)
#define L4_ErrCanceled		(3)

#define L4_ErrMsgOverflow	(4)
#define L4_ErrNotAccepted	(5)
#define L4_ErrAborted		(7)

/*
 * IPC Error Phase, returned by L4_IpcError
 */
#define L4_ErrSendPhase         (0)
#define L4_ErrRecvPhase         (1)

L4_INLINE L4_Bool_t L4_IpcError (L4_Word_t err, L4_Word_t *err_code)
{
	L4_Bool_t phase;
	phase = err & 0x1;
	if (err_code) {
		*err_code = (err >> 1) & 0xf;
	}
	return phase;
}


/*
 * Fpages
 */

typedef union {
    L4_Word_t	raw;
    struct {
	L4_BITFIELD4(L4_Word_t, 
		rwx : 3,
		reserved : 1,
		s : 6,
		b : 22 __PLUS32);
    } X;
} L4_Fpage_t;

#define L4_Readable		(0x04)
#define L4_Writable		(0x02)
#define L4_eXecutable		(0x01)
#define L4_FullyAccessible	(0x07)
#define L4_ReadWriteOnly	(0x06)
#define L4_ReadeXecOnly		(0x05)
#define L4_NoAccess		(0x00)

#if defined(__ARMCC_VERSION)
  extern const L4_Fpage_t L4_Nilpage;
  extern L4_Fpage_t L4_CompleteAddressSpace;
#else
#ifdef _lint
/* We are playing tricks with the linter here because it doesn't like
   this as a #define at all. It would be nice to clean up the code
   but it doesn't really work */
static const L4_Fpage_t L4_Nilpage = { 0UL };
static const L4_Fpage_t L4_CompleteAddressSpace = { 1UL << 22 __PLUS32 };
#else
#define L4_Nilpage		((L4_Fpage_t) { raw : 0UL })
#define L4_CompleteAddressSpace	((L4_Fpage_t) { X : { L4_SHUFFLE4(0, 0, 1, 0) }})
#endif
#endif

#include __L4_INC_ARCH(specials.h)

L4_INLINE L4_Bool_t L4_IsNilFpage (L4_Fpage_t f)
{
    return f.raw == 0;
}

L4_INLINE L4_Word_t L4_Rights (L4_Fpage_t f)
{
    return f.X.rwx;
}

L4_INLINE L4_Fpage_t L4_Set_Rights (L4_Fpage_t * f, L4_Word_t rwx)
{
    f->X.rwx = rwx;
    return *f;
}

L4_INLINE L4_Fpage_t L4_FpageAddRights (L4_Fpage_t f, L4_Word_t rwx)
{
    f.X.rwx |= rwx;
    return f;
}

L4_INLINE L4_Fpage_t L4_FpageAddRightsTo (L4_Fpage_t * f, L4_Word_t rwx)
{
    f->X.rwx |= rwx;
    return *f;
}

L4_INLINE L4_Fpage_t L4_FpageRemoveRights (L4_Fpage_t f, L4_Word_t rwx)
{
    f.X.rwx &= ~rwx;
    return f;
}

L4_INLINE L4_Fpage_t L4_FpageRemoveRightsFrom (L4_Fpage_t * f, L4_Word_t rwx)
{
    f->X.rwx &= ~rwx;
    return *f;
}

#if defined(__l4_cplusplus)
static inline L4_Fpage_t operator + (const L4_Fpage_t & f, L4_Word_t rwx)
{
    return L4_FpageAddRights (f, rwx);
}

static inline L4_Fpage_t operator += (L4_Fpage_t & f, L4_Word_t rwx)
{
    return L4_FpageAddRightsTo (&f, rwx);
}

static inline L4_Fpage_t operator - (const L4_Fpage_t & f, L4_Word_t rwx)
{
    return L4_FpageRemoveRights (f, rwx);
}

static inline L4_Fpage_t operator -= (L4_Fpage_t & f, L4_Word_t rwx)
{
    return L4_FpageRemoveRightsFrom (&f, rwx);
}
#endif /* __l4_cplusplus */

L4_INLINE L4_Fpage_t L4_Fpage (L4_Word_t BaseAddress, L4_Word_t FpageSize)
{
    L4_Fpage_t fp;
    L4_Word_t msb = __L4_Msb (FpageSize);
    fp.raw = BaseAddress;
    fp.X.s = (1UL << msb) < FpageSize ? msb + 1 : msb;
    fp.X.rwx = L4_NoAccess;
    fp.X.reserved = 0;
    return fp;
}

L4_INLINE L4_Fpage_t L4_FpageLog2 (L4_Word_t BaseAddress, int FpageSize)
{
    L4_Fpage_t fp;
    fp.raw = BaseAddress;
    fp.X.s = FpageSize;
    fp.X.rwx = L4_NoAccess;
    fp.X.reserved = 0;
    return fp;
}

L4_INLINE L4_Word_t L4_Address (L4_Fpage_t f)
{
    return f.raw & ~((1UL << f.X.s) - 1);
}

L4_INLINE L4_Word_t L4_Size (L4_Fpage_t f)
{
    return f.X.s == 0 ? 0 : (1UL << f.X.s);
}

L4_INLINE L4_Word_t L4_SizeLog2 (L4_Fpage_t f)
{
    return f.X.s;
}


/*
 * Thread IDs
 */

typedef union {
    L4_Word_t	raw;
    struct {
	L4_BITFIELD2( L4_Word_t,
		version : __14,
		thread_no : __18);
    } X;
} L4_GthreadId_t;

typedef union {
    L4_Word_t		raw;
    L4_GthreadId_t	global;
} L4_ThreadId_t;

#if defined(__ARMCC_VERSION)
  extern L4_Word_t L4_ExReg_SrcThread(L4_ThreadId_t x);
#else
  #define L4_ExReg_SrcThread(x) (((L4_GthreadId_t) { X : {L4_SHUFFLE2(0, L4_ThreadNo(x))}}).raw)
#endif

#if defined(__ARMCC_VERSION)

extern const L4_ThreadId_t L4_nilthread;
extern const L4_ThreadId_t L4_anythread;
extern L4_ThreadId_t L4_anylocalthread;

#else /*  __GNUC__ */

#ifdef _lint
static const L4_ThreadId_t L4_nilthread = { 0UL };
static const L4_ThreadId_t L4_anythread	= { ~0UL};
#else
#define L4_nilthread		((L4_ThreadId_t) { raw : 0UL})
#define L4_anythread		((L4_ThreadId_t) { raw : ~0UL})
#define L4_waitnotify		((L4_ThreadId_t) { raw : -2UL})
#define L4_anylocalthread	((L4_ThreadId_t) { raw : -64UL})

#endif
#endif

L4_INLINE L4_ThreadId_t L4_GlobalId (L4_Word_t threadno, L4_Word_t version)
{
    L4_ThreadId_t t;
    t.global.X.thread_no = threadno;
    t.global.X.version = version;
    return t;
}

L4_INLINE L4_Word_t L4_Version (L4_ThreadId_t t)
{
    return t.global.X.version;
}

L4_INLINE L4_Word_t L4_ThreadNo (L4_ThreadId_t t)
{
    return t.global.X.thread_no;
}

L4_INLINE L4_Bool_t L4_IsThreadEqual (const L4_ThreadId_t l,
				      const L4_ThreadId_t r)
{
    return l.raw == r.raw;
}

L4_INLINE L4_Bool_t L4_IsThreadNotEqual (const L4_ThreadId_t l,
					 const L4_ThreadId_t r)
{
    return l.raw != r.raw;
}

#if defined(__l4_cplusplus)
static inline L4_Bool_t operator == (const L4_ThreadId_t & l,
				     const L4_ThreadId_t & r)
{
    return l.raw == r.raw;
}

static inline L4_Bool_t operator != (const L4_ThreadId_t & l,
				     const L4_ThreadId_t & r)
{
    return l.raw != r.raw;
}
#endif /* __l4_cplusplus */

L4_INLINE L4_Bool_t L4_IsNilThread (L4_ThreadId_t t)
{
    return t.raw == 0;
}


#undef __14
#undef __18
#undef __PLUS32

#endif /* !__L4__TYPES_H__ */
