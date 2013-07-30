/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.    *
 *****************************************************************************/
#ifndef AWN_SURFACE_POLYNOMIAL_EDGE_ZERO_2ND_ORDER_HH
#define AWN_SURFACE_POLYNOMIAL_EDGE_ZERO_2ND_ORDER_HH

#include "awnsurfacepolynomialedgezero2ndorderparams.hh"

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
 *             specific interfacial  area to wetting phase saturation and capillary pressure.
 *
 */
template <class ParamsT>
class AwnSurfacePolynomialEdgeZero2ndOrder
{
public:
    typedef ParamsT Params;
    typedef typename Params::Scalar Scalar;

    /*!
     * \brief The awn surface
     *
     * the suggested (as estimated from pore network models) awn surface:
     * \f[
     a_{wn} = a_1 (S_{wr}-S_w)(1.-S_w) + a_2 (S_{wr}-S_w)(1.-S_w) p_c + a_3 (S_{wr}-S_w)(1.-S_w) p_c^2
     \f]
     * \param  params parameter container for the coefficients of the surface
     * \param  Sw Effective saturation of the wetting phase
     * \param  pc Capillary pressure
     */
    static Scalar interfacialArea(const Params &params, const Scalar Sw, const Scalar pc)
    {
        const Scalar a1     = params.a1();
        const Scalar a2     = params.a2();
        const Scalar a3     = params.a3();
        const Scalar Swr    = params.Swr();
        const Scalar factor = (Swr-Sw)*(1.-Sw) ;

        const Scalar aAlphaBeta = a1*factor + a2*factor*pc + a3*factor*pc*pc;
        return aAlphaBeta;
    }


    /*! \brief the derivative of specific interfacial area function w.r.t. capillary pressure
     *
     * \param  params parameter container for the coefficients of the surface
     * \param  Sw Effective saturation of the wetting phase
     * \param  pc Capillary pressure
     */
    static Scalar dawn_dpc (const Params &params, const Scalar Sw, const Scalar pc)
    {
        const Scalar Swr    = params.Swr();
        const Scalar a1     = params.a1();
        const Scalar a2     = params.a2();
        const Scalar a3     = params.a3();
        const Scalar value =  a2*(Swr-Sw)*(1-Sw) + a3*(Swr-Sw)*(1-Sw)*pc;
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
        const Scalar Swr                = params.Swr();
        const Scalar a1     = params.a1();
        const Scalar a2     = params.a2();
        const Scalar a3     = params.a3();
        const Scalar derivativeFactor   = (Sw-1.)+(Sw-Swr);
        const Scalar value = a1 * derivativeFactor + a2 * derivativeFactor * pc + a3 * derivativeFactor * pc*pc ;
        return value;
    }

};
} // namespace Dumux

#endif


