/* Copyright (c) 2006 by CodeSourcery.  All rights reserved.

   This file is available for license from CodeSourcery, Inc. under the terms
   of a commercial license and under the GPL.  It is not part of the VSIPL++
   reference implementation and is not available under the BSD license.
*/

#ifndef VSIP_OPT_SIMD_VMUL_HPP
#define VSIP_OPT_SIMD_VMUL_HPP

#if VSIP_IMPL_REF_IMPL
# error "vsip/opt files cannot be used as part of the reference impl."
#endif

#include <complex>
#include <vsip/support.hpp>
#include <vsip/opt/simd/simd.hpp>
#include <vsip/core/metaprogramming.hpp>

#define VSIP_IMPL_INLINE_LIBSIMD 0

namespace vsip
{
namespace impl
{
namespace simd
{

// Define value_types for which vmul is optimized.
//  - float
//  - double
//  - complex<float>
//  - complex<double>

template <typename T,
	  bool     IsSplit>
struct is_algorithm_supported<T, IsSplit, Alg_vmul>
{
  typedef typename scalar_of<T>::type scalar_type;
  static bool const value =
    Simd_traits<scalar_type>::is_accel &&
    (is_same<scalar_type, float>::value || is_same<scalar_type, double>::value);
};

// Class for vmul - vector element-wise multiplication.
template <typename T,
	  bool     Is_vectorized>
struct Vmul;

// Generic, non-vectorized implementation of vector element-wise multiply.
template <typename T>
struct Vmul<T, false>
{
  static void exec(T const *A, T const *B, T* R, length_type n)
  {
    while (n)
    {
      *R = *A * *B;
      R++; A++; B++;
      n--;
    }
  }
};

// Vectorized implementation of vector element-wise multiply for scalars
// (float, double, etc).
template <typename T>
struct Vmul<T, true>
{
  static void exec(T const *A, T const *B, T *R, length_type n)
  {
    typedef Simd_traits<T> simd;
    typedef typename simd::simd_type simd_type;

    // handle mis-aligned vectors
    if (simd::alignment_of(R) != simd::alignment_of(A) ||
	simd::alignment_of(R) != simd::alignment_of(B))
    {
      // PROFILE
      while (n)
      {
	*R = *A * *B;
	R++; A++; B++;
	n--;
      }
      return;
    }

    // clean up initial unaligned values
    while (n && simd::alignment_of(A) != 0)
    {
      *R = *A * *B;
      R++; A++; B++;
      n--;
    }
  
    if (n == 0) return;

    simd_type reg0;
    simd_type reg1;
    simd_type reg2;
    simd_type reg3;

    simd::enter();

    while (n >= 2*simd::vec_size)
    {
      n -= 2*simd::vec_size;

      reg0 = simd::load(A);
      reg1 = simd::load(B);
      
      reg2 = simd::load(A + simd::vec_size);
      reg3 = simd::load(B + simd::vec_size);
      
      reg1 = simd::mul(reg0, reg1);
      reg3 = simd::mul(reg2, reg3);
      
      simd::store(R,                  reg1);
      simd::store(R + simd::vec_size, reg3);
      
      A+=2*simd::vec_size; B+=2*simd::vec_size; R+=2*simd::vec_size;
    }
    
    simd::exit();

    while (n)
    {
      *R = *A * *B;
      R++; A++; B++;
      n--;
    }
  }
};



// Vectorized implementation of vector element-wise multiply for
// interleaved complex (complex<float>, complex<double>, etc).

template <typename T>
struct Vmul<std::complex<T>, true>
{
  static void exec(std::complex<T> const *A,
		   std::complex<T> const *B,
		   std::complex<T> *R,
		   length_type n)
  {
    typedef Simd_traits<T> simd;
    typedef typename simd::simd_type simd_type;
    
    // handle mis-aligned vectors
    if (simd::alignment_of((T*)R) != simd::alignment_of((T*)A) ||
	simd::alignment_of((T*)R) != simd::alignment_of((T*)B))
    {
      // PROFILE
      while (n)
      {
	*R = *A * *B;
	R++; A++; B++;
	n--;
      }
      return;
    }

    // clean up initial unaligned values
    while (n && simd::alignment_of((T*)A) != 0)
    {
      *R = *A * *B;
      R++; A++; B++;
      n--;
    }
  
    if (n == 0) return;

    simd::enter();

    while (n >= simd::vec_size)
    {
      n -= simd::vec_size;

      simd_type regA1 = simd::load((T*)A);
      simd_type regB1 = simd::load((T*)B);
      
      simd_type regA2 = simd::load((T*)A + simd::vec_size);
      simd_type regB2 = simd::load((T*)B + simd::vec_size);

      simd_type Ar = simd::real_from_interleaved(regA1, regA2);
      simd_type Ai = simd::imag_from_interleaved(regA1, regA2);

      simd_type Br = simd::real_from_interleaved(regB1, regB2);
      simd_type Bi = simd::imag_from_interleaved(regB1, regB2);
      
      simd_type ArBr = simd::mul(Ar, Br);
      simd_type AiBi = simd::mul(Ai, Bi);
      simd_type Rr   = simd::sub(ArBr, AiBi);
      
      simd_type ArBi = simd::mul(Ar, Bi);
      simd_type AiBr = simd::mul(Ai, Br);
      simd_type Ri   = simd::add(ArBi, AiBr);

      simd_type regR1 = simd::interleaved_lo_from_split(Rr, Ri);
      simd_type regR2 = simd::interleaved_hi_from_split(Rr, Ri);
      
      simd::store((T*)R,                  regR1);
      simd::store((T*)R + simd::vec_size, regR2);
      
      A+=simd::vec_size; B+=simd::vec_size; R+=simd::vec_size;
    }

    simd::exit();

    while (n)
    {
      *R = *A * *B;
      R++; A++; B++;
      n--;
    }
  }
};



// Generic, non-vectorized implementation of vector element-wise multiply for
// split complex (as represented by pair<float*, float*>, etc).

template <typename T>
struct Vmul<std::pair<T, T>, false>
{
  static void exec(std::pair<T const *, T const *> const& A,
		   std::pair<T const *, T const *> const& B,
		   std::pair<T*, T*> const& R,
		   length_type n)
  {
    T const* pAr = A.first;
    T const* pAi = A.second;

    T const* pBr = B.first;
    T const* pBi = B.second;

    T* pRr = R.first;
    T* pRi = R.second;

    while (n)
    {
      *pRr = *pAr * *pBr - *pAi * *pBi;
      *pRi = *pAr * *pBi + *pAi * *pBr;
      pRr++; pRi++;
      pAr++; pAi++;
      pBr++; pBi++;
      n--;
    }
  }
};

// Vectorized implementation of vector element-wise multiply for
// split complex (as represented by pair<float*, float*>, etc).
template <typename T>
struct Vmul<std::pair<T, T>, true>
{
  static void exec(std::pair<T const *, T const *> const& A,
		   std::pair<T const *, T const *> const& B,
		   std::pair<T*, T*> const& R,
		   length_type n)
  {
    typedef Simd_traits<T> simd;
    typedef typename simd::simd_type simd_type;
    
    T const* pAr = A.first;
    T const* pAi = A.second;

    T const* pBr = B.first;
    T const* pBi = B.second;

    T* pRr = R.first;
    T* pRi = R.second;

    // handle mis-aligned vectors
    if (simd::alignment_of(pRr) != simd::alignment_of(pRi) ||
	simd::alignment_of(pRr) != simd::alignment_of(pAr) ||
	simd::alignment_of(pRr) != simd::alignment_of(pAi) ||
	simd::alignment_of(pRr) != simd::alignment_of(pBr) ||
	simd::alignment_of(pRr) != simd::alignment_of(pBi))
    {
      // PROFILE
      while (n)
      {
	T rr = *pAr * *pBr - *pAi * *pBi;
	*pRi = *pAr * *pBi + *pAi * *pBr;
	*pRr = rr;
	pRr++; pRi++;
	pAr++; pAi++;
	pBr++; pBi++;
	n--;
      }
      return;
    }

    // clean up initial unaligned values
    while (n && simd::alignment_of(pRr) != 0)
    {
      T rr = *pAr * *pBr - *pAi * *pBi;
      *pRi = *pAr * *pBi + *pAi * *pBr;
      *pRr = rr;
      pRr++; pRi++;
      pAr++; pAi++;
      pBr++; pBi++;
      n--;
    }
  
    if (n == 0) return;

    simd::enter();

    while (n >= simd::vec_size)
    {
      n -= simd::vec_size;

      simd_type Ar = simd::load((T*)pAr);
      simd_type Ai = simd::load((T*)pAi);

      simd_type Br = simd::load((T*)pBr);
      simd_type Bi = simd::load((T*)pBi);
      
      simd_type ArBr = simd::mul(Ar, Br);
      simd_type AiBi = simd::mul(Ai, Bi);
      simd_type Rr   = simd::sub(ArBr, AiBi);
      
      simd_type ArBi = simd::mul(Ar, Bi);
      simd_type AiBr = simd::mul(Ai, Br);
      simd_type Ri   = simd::add(ArBi, AiBr);

      simd::store(pRr, Rr);
      simd::store(pRi, Ri);
      
      pRr += simd::vec_size; pRi += simd::vec_size;
      pAr += simd::vec_size; pAi += simd::vec_size;
      pBr += simd::vec_size; pBi += simd::vec_size;
    }

    simd::exit();

    while (n)
    {
      T rr = *pAr * *pBr - *pAi * *pBi;
      *pRi = *pAr * *pBi + *pAi * *pBr;
      *pRr = rr;
      pRr++; pRi++;
      pAr++; pAi++;
      pBr++; pBi++;
      n--;
    }
  }
};



// Depending on VSIP_IMPL_LIBSIMD_INLINE macro, either provide these
// functions inline, or provide non-inline functions in the libvsip.a.

#if VSIP_IMPL_INLINE_LIBSIMD

template <typename T>
inline void
vmul(T const *op1, T const *op2, T *res, length_type size)
{
  static bool const Is_vectorized = is_algorithm_supported<T, false, Alg_vmul>
                                      ::value;
  Vmul<T, Is_vectorized>::exec(op1, op2, res, size);
}

template <typename T>
inline void
vmul(std::pair<T const *,T const *>  op1,
     std::pair<T const *,T const *>  op2,
     std::pair<T*,T*>  res,
     length_type size)
{
  static bool const Is_vectorized = is_algorithm_supported<T, true, Alg_vmul>
                                      ::value;
  Vmul<std::pair<T,T>, Is_vectorized>::exec(op1, op2, res, size);
}

#else

template <typename T>
void
vmul(T const *op1, T const *op2, T *res, length_type size);

template <typename T>
void
vmul(std::pair<T const *,T const *>  op1,
     std::pair<T const *,T const *>  op2,
     std::pair<T*,T*>  res,
     length_type size);

#endif // VSIP_IMPL_INLINE_LIBSIMD


} // namespace vsip::impl::simd
} // namespace vsip::impl
} // namespace vsip

#endif // VSIP_IMPL_SIMD_VMUL_HPP
