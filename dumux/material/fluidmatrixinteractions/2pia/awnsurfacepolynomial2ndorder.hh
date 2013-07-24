/*****************************************************************************
 *   Copyright (C) 2008 by Philipp Nuske                                     *
 *   Institute for Modelling Hydraulic and Environmental Systems             *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@iws.uni-stuttgart.de                          *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \file awnSurfacePylynomial2ndOrder.hh Specification of the material params
 *       specific interfacial area surface as a function of wetting phase Saturation
 *       and capillara pressure.
 */


#ifndef AWN_SURFACE_POLYNOMIAL_2ND_ORDER_HH
#define AWN_SURFACE_POLYNOMIAL_2ND_ORDER_HH

#include "awnsurfacepolynomial2ndorderparams.hh"

#include <dune/common/exceptions.hh>


#include <algorithm>

#include <math.h>
#include <assert.h>

namespace Dumux
{
/*!
 * \ingroup material
 *
 * \brief Implementation of the polynomial of second order relating
 *             specific interfacial  area to wetting phase saturation and capillary pressure as suggested by Joakar-Niasar(2007).
 */
template <class ParamsT>
class AwnSurfacePolynomial2ndOrder
{
public:
    typedef ParamsT Params;
    typedef typename Params::Scalar Scalar;

    /*!
     * \brief The awn surface
     *
     * the suggested (as estimated from pore network models) awn surface:
     * \f[
     a_{wn} = a_{00} + a_{10}S_w + a_{20}S_w^2 + a_{11} S_w p_c +  a_{01} p_c + a_{02}p_c^2
     \f]
     * \param  params parameter container for the coefficients of the surface
     * \param  Sw Effective saturation of the wetting phase
     * \param  pc Capillary pressure
     */
    static Scalar interfacialArea(const Params &params, const Scalar Sw, const Scalar pc)
    {
        const Scalar a00 = params.a00();
        const Scalar a10 = params.a10();
        const Scalar a20 = params.a20();
        const Scalar a11 = params.a11();
        const Scalar a01 = params.a01();
        const Scalar a02 = params.a02();
        const Scalar aAlphaBeta = a00 + a10 * Sw + a20 * std::pow(Sw,2) + a11*Sw*pc +  a01*pc + a02*std::pow(pc,2);
        return aAlphaBeta;
    }


    /*! \brief the derivative of specific interfacial area function w.r.t. capillary pressure
     *
     * \param  params parameter container for the coefficients of the surface
     * \param  Sw Effective saturation of the wetting phase
     * \param  pc Capillary pressure
     */
    static Scalar dawn_dpc (const Params & params, const Scalar Sw, const Scalar pc)
    {
        const Scalar value = params.a11()*Sw + params.a01() + 2.0*params.a02() * pc;
        return value;
    }

    /*! \brief the derivative of specific interfacial area function w.r.t. saturation
     *
     * \param  params parameter container for the coefficients of the surface
     * \param  Sw Effective saturation of the wetting phase
     * \param  pc Capillary pressure
     */
    static Scalar dawn_dsw (const Params &params, const Scalar Sw, const Scalar pc)
    {
        const Scalar value = params.a11()*pc + params.a10() + 2.0*params.a20()*Sw;
        return value;
    }

};
}

#endif


