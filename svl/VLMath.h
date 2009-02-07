/*
	File:			VLMath.h

	Function:		Various math definitions for VL
					
	Author(s):		Andrew Willmott

	Copyright:		Copyright (c) 1995-1996, Andrew Willmott
 */

#ifndef __VL_MATH__
#define __VL_MATH__

// --- Inlines ----------------------------------------------------------------

#ifdef __SGI__
#include <ieeefp.h>
#define vl_finite(X) finite(X)
#elif __GCC__
#define vl_finite(X) finite(X)
#else
#define vl_finite(X) _finite(X)
#endif

inline Float sqrlen(Float r)
{ return(sqr(r)); }
inline Double sqrlen(Double r)
{ return(sqr(r)); }

inline Float mix(Float a, Float b, Float s)
{ return((1.0 - s) * a + s * b); }
inline Float mix(Double a, Double b, Double s)
{ return((1.0 - s) * a + s * b); }

inline Void SetReal(Float &a, Double b)
{ a = b; }
inline Void SetReal(Double &a, Double b)
{ a = b; }

template <class S, class T> Void ConvertVec(const S &u, T &v)
{
	for (Int i = 0; i < u.Elts(); i++)
		v[i] = u[i];
}

#endif
