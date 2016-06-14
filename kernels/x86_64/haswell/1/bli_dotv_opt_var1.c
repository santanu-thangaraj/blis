/*

   BLIS    
   An object-based framework for developing high-performance BLAS-like
   libraries.

   Copyright (C) 2014, The University of Texas at Austin

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    - Neither the name of The University of Texas at Austin nor the names
      of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "blis.h"
#include "immintrin.h"

#if 0

typedef union
{
    __m256d v;
    double  d[4];
} v4df_t;


void bli_ddotv_opt_var1
     (
       conj_t           conjx,
       conj_t           conjy,
       dim_t            n,
       double* restrict x, inc_t incx,
       double* restrict y, inc_t incy,
       double* restrict rho,
       cntx_t*          cntx
     )
{
	double*  restrict x_cast   = x;
	double*  restrict y_cast   = y;
	double*  restrict rho_cast = rho;
	dim_t             i;

	dim_t             n_run;
	dim_t             n_left;

	double*  restrict x1;
	double*  restrict y1;
	double            rho1;
	double            x1c, y1c;

	v4df_t            rho1v;
	v4df_t            x1v, y1v;

	v4df_t            rho2v;
	v4df_t            x2v, y2v;

	const dim_t       n_elem_per_reg = 4;
	const dim_t       n_iter_unroll  = 2;

	bool_t            use_ref = FALSE;

	// If the vector lengths are zero, set rho to zero and return.
	if ( bli_zero_dim1( n ) )
	{
		PASTEMAC(d,set0s)( *rho_cast );
		return;
	}

	// If there is anything that would interfere with our use of aligned
	// vector loads/stores, call the reference implementation.
	if ( incx != 1 || incy != 1 )
	{
		use_ref = TRUE;
	}

	// Call the reference implementation if needed.
	if ( use_ref == TRUE )
	{
		BLIS_DDOTV_KERNEL_REF( conjx,
		                       conjy,
		                       n,
		                       x, incx,
		                       y, incy,
		                       rho,
		                       cntx );
		return;
	}

	n_run       = ( n ) / (n_elem_per_reg * n_iter_unroll);
	n_left      = ( n ) % (n_elem_per_reg * n_iter_unroll);

	x1 = x_cast;
	y1 = y_cast;

	PASTEMAC(d,set0s)( rho1 );


	rho1v.v = _mm256_setzero_pd();
	rho2v.v = _mm256_setzero_pd();


	for ( i = 0; i < n_run; ++i )
	{
		x1v.v = _mm256_loadu_pd( ( double* )x1 );
		y1v.v = _mm256_loadu_pd( ( double* )y1 );

		rho1v.v += x1v.v * y1v.v;

		x2v.v = _mm256_loadu_pd( ( double* )(x1 + n_elem_per_reg));
		y2v.v = _mm256_loadu_pd( ( double* )(y1 + n_elem_per_reg));

		rho2v.v += x2v.v * y2v.v;

		//x1 += 2*incx;
		//y1 += 2*incy;
		x1 += (n_elem_per_reg * n_iter_unroll);
		y1 += (n_elem_per_reg * n_iter_unroll);
	}

	rho1 += rho1v.d[0] + rho1v.d[1] + rho1v.d[2] + rho1v.d[3] +
			rho2v.d[0] + rho2v.d[1] + rho2v.d[2] + rho2v.d[3];

	if ( n_left > 0 )
	{
		for ( i = 0; i < n_left; ++i )
		{
			x1c = *x1;
			y1c = *y1;

			rho1 += x1c * y1c;

			x1 += incx;
			y1 += incy;
		}
	}

	PASTEMAC(d,copys)( rho1, *rho_cast );
}
#else

#include "pmmintrin.h"
typedef union
{
    __m128d v;
    double  d[2];
} v2df_t;


void bli_ddotv_opt_var1
     (
       conj_t           conjx,
       conj_t           conjy,
       dim_t            n,
       double* restrict x, inc_t incx,
       double* restrict y, inc_t incy,
       double* restrict rho,
       cntx_t*          cntx
     )
{
	double*  restrict x_cast   = x;
	double*  restrict y_cast   = y;
	double*  restrict rho_cast = rho;
	dim_t             i;

	dim_t             n_pre;
	dim_t             n_run;
	dim_t             n_left;

	double*  restrict x1;
	double*  restrict y1;
	double            rho1;
	double            x1c, y1c;

	v2df_t            rho1v;
	v2df_t            x1v, y1v;

	v2df_t            rho2v;
	v2df_t            x2v, y2v;

	v2df_t            rho3v;
	v2df_t            x3v, y3v;

	v2df_t            rho4v;
	v2df_t            x4v, y4v;
	bool_t            use_ref = FALSE;

	// If the vector lengths are zero, set rho to zero and return.
	if ( bli_zero_dim1( n ) )
	{
		PASTEMAC(d,set0s)( *rho_cast );
		return;
	}

	n_pre = 0;

	// If there is anything that would interfere with our use of aligned
	// vector loads/stores, call the reference implementation.
	if ( incx != 1 || incy != 1 )
	{
		use_ref = TRUE;
	}
	else if ( bli_is_unaligned_to( x, 16 ) ||
	          bli_is_unaligned_to( y, 16 ) )
	{
		use_ref = TRUE;

		if ( bli_is_unaligned_to( x, 16 ) &&
		     bli_is_unaligned_to( y, 16 ) )
		{
			use_ref = FALSE;
			n_pre = 1;
		}
	}

	// Call the reference implementation if needed.
	if ( use_ref == TRUE )
	{
		BLIS_DDOTV_KERNEL_REF( conjx,
		                       conjy,
		                       n,
		                       x, incx,
		                       y, incy,
		                       rho,
		                       cntx );
		return;
	}

	n_run       = ( n - n_pre ) / 8;
	n_left      = ( n - n_pre ) % 8;

	x1 = x_cast;
	y1 = y_cast;

	PASTEMAC(d,set0s)( rho1 );

	if ( n_pre == 1 )
	{
		x1c = *x1;
		y1c = *y1;

		rho1 += x1c * y1c;

		x1 += incx;
		y1 += incy;
	}

	rho1v.v = _mm_setzero_pd();
	rho2v.v = _mm_setzero_pd();
	rho3v.v = _mm_setzero_pd();
	rho4v.v = _mm_setzero_pd();

	for ( i = 0; i < n_run; ++i )
	{
		x1v.v = _mm_load_pd( ( double* )x1 );
		y1v.v = _mm_load_pd( ( double* )y1 );

		rho1v.v += x1v.v * y1v.v;

		x2v.v = _mm_load_pd( ( double* )(x1 + 2) );
		y2v.v = _mm_load_pd( ( double* )(y1 + 2) );

		rho2v.v += x2v.v * y2v.v;

		x3v.v = _mm_load_pd( ( double* )(x1 + 4) );
		y3v.v = _mm_load_pd( ( double* )(y1 + 4) );

		rho3v.v += x3v.v * y3v.v;

		x4v.v = _mm_load_pd( ( double* )(x1 + 6) );
		y4v.v = _mm_load_pd( ( double* )(y1 + 6) );

		rho4v.v += x4v.v * y4v.v;
		//x1 += 2*incx;
		//y1 += 2*incy;
		x1 += 8;
		y1 += 8;
	}

	rho1 += rho1v.d[0] + rho1v.d[1] + rho2v.d[0] + rho2v.d[1] +
			rho3v.d[0] + rho3v.d[1] + rho4v.d[0] + rho4v.d[1];

	if ( n_left > 0 )
	{
		for ( i = 0; i < n_left; ++i )
		{
			x1c = *x1;
			y1c = *y1;

			rho1 += x1c * y1c;

			x1 += incx;
			y1 += incy;
		}
	}

	PASTEMAC(d,copys)( rho1, *rho_cast );
}
#endif
typedef union
{
    __m256 v;
    float  f[8];
} v8ff_t;


void bli_sdotv_opt_var1
     (
       conj_t           conjx,
       conj_t           conjy,
       dim_t            n,
       float* restrict x, inc_t incx,
	   float* restrict y, inc_t incy,
	   float* restrict rho,
       cntx_t*          cntx
     )
{
	float*  restrict x_cast   = x;
	float*  restrict y_cast   = y;
	float*  restrict rho_cast = rho;
	dim_t             i;

	dim_t             n_run;
	dim_t             n_left;

	float*  restrict x1;
	float*  restrict y1;
	float            rho1;
	float            x1c, y1c;

	v8ff_t            rho1v;
	v8ff_t            x1v, y1v;

	const dim_t       n_elem_per_reg = 8;
	const dim_t       n_iter_unroll  = 1;

	bool_t            use_ref = FALSE;

	// If the vector lengths are zero, set rho to zero and return.
	if ( bli_zero_dim1( n ) )
	{
		PASTEMAC(s,set0s)( *rho_cast );
		return;
	}

	// If there is anything that would interfere with our use of aligned
	// vector loads/stores, call the reference implementation.
	if ( incx != 1 || incy != 1 )
	{
		use_ref = TRUE;
	}

	// Call the reference implementation if needed.
	if ( use_ref == TRUE )
	{
		BLIS_SDOTV_KERNEL_REF( conjx,
		                       conjy,
		                       n,
		                       x, incx,
		                       y, incy,
		                       rho,
		                       cntx );
		return;
	}

	n_run       = ( n ) / (n_elem_per_reg * n_iter_unroll);
	n_left      = ( n ) % (n_elem_per_reg * n_iter_unroll);

	x1 = x_cast;
	y1 = y_cast;

	PASTEMAC(s,set0s)( rho1 );


	rho1v.v = _mm256_setzero_ps();

	for ( i = 0; i < n_run; ++i )
	{
		x1v.v = _mm256_loadu_ps( ( float* )x1 );
		y1v.v = _mm256_loadu_ps( ( float* )y1 );

		rho1v.v += x1v.v * y1v.v;

		//x1 += 2*incx;
		//y1 += 2*incy;
		x1 += (n_elem_per_reg * n_iter_unroll);
		y1 += (n_elem_per_reg * n_iter_unroll);
	}

	rho1 += rho1v.f[0] + rho1v.f[1] + rho1v.f[2] + rho1v.f[3] +
			rho1v.f[4] + rho1v.f[5] + rho1v.f[6] + rho1v.f[7];

	if ( n_left > 0 )
	{
		for ( i = 0; i < n_left; ++i )
		{
			x1c = *x1;
			y1c = *y1;

			rho1 += x1c * y1c;

			x1 += incx;
			y1 += incy;
		}
	}

	PASTEMAC(s,copys)( rho1, *rho_cast );
}
