/*
  Implementation based on C99 Section 7.15 Variable arguments
*/

#ifndef _STDARG_H_
#define _STDARG_H_

#ifdef _lint
/* 
   If we are dealing with a linter lets provide some type correct data.
*/
typedef long va_list;
#define va_arg(ap, type) (type) 0
#define va_copy(dest, src) 
#define va_end(ap) 
#define va_start(ap, parmN) 

#else

typedef __builtin_va_list va_list;

#define va_arg(ap, type) __builtin_va_arg((ap), type)
#define va_copy(dest, src) __builtin_va_copy((ap), type)
#define va_end(ap) __builtin_va_end((ap))
#define va_start(ap, parmN) __builtin_stdarg_start((ap), (parmN))

#endif

#endif /* _STDARG_H_ */
