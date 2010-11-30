/*
 *  This file is part of libpsht.
 *
 *  libpsht is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  libpsht is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libpsht; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
 *  libpsht is being developed at the Max-Planck-Institut fuer Astrophysik
 *  and financially supported by the Deutsches Zentrum fuer Luft- und Raumfahrt
 *  (DLR).
 */

/*! \file ylmgen_c.h
 *  Code for efficient calculation of Y_lm(phi=0,theta)
 *
 *  Copyright (C) 2005-2010 Max-Planck-Society
 *  \author Martin Reinecke
 */

#ifndef PLANCK_YLMGEN_C_H
#define PLANCK_YLMGEN_C_H

#include "sse_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef double ylmgen_dbl2[2];

typedef struct
  {
  double fsmall, fbig, eps, cth_crit;
  int lmax, mmax, m_cur, ith, m_crit;
  /*! The index of the first non-negligible Y_lm value. */
  int firstl;
  double *cf, *mfac, *t1fac, *t2fac, *cth, *sth, *logsth;
  ylmgen_dbl2 *recfac;
  double *lamfact;
  /*! Points to an array of size [0..lmax] containing the Y_lm values. */
  double *ylm;
  /*! Points to an array of size [0..lmax] containing the lambda_w
      and lambda_x values for spin>0 transforms. */
  ylmgen_dbl2 **lambda_wx;
  int *lwx_uptodate;
  int ylm_uptodate;

#ifdef PLANCK_HAVE_SSE2
  int ith1, ith2;
  /*! Points to an array of size [0..lmax] containing the Y_lm values. */
  v2df *ylm_sse2;
  /*! Points to an array of size [0..lmax] containing the lambda_w
      and lambda_x values for spin>0 transforms. */
  v2df2 **lambda_wx_sse2;
  int *lwx_uptodate_sse2;
  int ylm_uptodate_sse2;
#endif

  int recfac_uptodate, lamfact_uptodate;
  } Ylmgen_C;

/*! Creates a generator which will calculate Y_lm(theta,phi=0)
    up to \a l=l_max and \a m=m_max. It may regard Y_lm whose absolute
    magnitude is smaller than \a epsilon as zero. */
void Ylmgen_init (Ylmgen_C *gen, int l_max, int m_max, double epsilon);

/*! Passes am array \a theta of \a nth colatitudes that will be used in
    subsequent calls. The individual angles will be referenced by their
    index in the array, starting with 0.
    \note The array can be freed or reused after the call. */
void Ylmgen_set_theta (Ylmgen_C *gen, const double *theta, int nth);

/*! Deallocates a generator previously initialised by Ylmgen_init(). */
void Ylmgen_destroy (Ylmgen_C *gen);

/*! Prepares the object for the calculation at \a theta and \a m. */
void Ylmgen_prepare (Ylmgen_C *gen, int ith, int m);

/*! Recalculates (if necessary) the Y_lm values. */
void Ylmgen_recalc_Ylm (Ylmgen_C *gen);
/*! Recalculates (if necessary) the lambda_w and lambda_x values for spin >0
    transforms. */
void Ylmgen_recalc_lambda_wx (Ylmgen_C *gen, int spin);

#ifdef PLANCK_HAVE_SSE2
/*! Prepares the object for the calculation at \a theta, \a theta2 and \a m. */
void Ylmgen_prepare_sse2 (Ylmgen_C *gen, int ith1, int ith2, int m);

/*! Recalculates (if necessary) the Y_lm values. */
void Ylmgen_recalc_Ylm_sse2 (Ylmgen_C *gen);
/*! Recalculates (if necessary) the lambda_w and lambda_x values for spin >0
    transforms. */
void Ylmgen_recalc_lambda_wx_sse2 (Ylmgen_C *gen, int spin);
#endif

#ifdef __cplusplus
}
#endif

#endif
