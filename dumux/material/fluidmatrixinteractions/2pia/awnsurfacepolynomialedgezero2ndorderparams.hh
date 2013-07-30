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
#ifndef AWN_SURFACE_POLYNOMIAL_EDGE_ZERO_2ND_ORDER_PARAMS_HH
#define AWN_SURFACE_POLYNOMIAL_EDGE_ZERO_2ND_ORDER_PARAMS_HH

/*!
 * \file
 * Specification of the material params for the interfacial area surface
 * parameters
 */

namespace Dumux
{
/*!
 * \brief implementation of interfacial area surface params
 */
template<class ScalarT>
class AwnSurfacePolynomialEdgeZero2ndOrderParams
{
public:
    typedef ScalarT Scalar;

    AwnSurfacePolynomialEdgeZero2ndOrderParams()
    {}

    AwnSurfacePolynomialEdgeZero2ndOrderParams(const Scalar a1,
                                               const Scalar a2,
                                               const Scalar a3)
    {
        setA1(a1);
        setA2(a2);
        setA3(a3);
    }

    /*!
     * \brief Return the \f$a_{1}\f$ shape parameter of awn surface.
     */
    const Scalar a1() const
    { return a1_; }

    /*!
     * \brief Return the \f$a_{2}\f$ shape parameter of awn surface.
     */
    const Scalar a2() const
    { return a2_; }

    /*!
     * \brief Return the \f$a_{3}\f$ shape parameter of awn surface.
     */
    const Scalar a3() const
    { return a3_; }

    /*!
     * \brief Return the \f$a_{3}\f$ shape parameter of awn surface.
     */
    const Scalar Swr() const
    { return Swr_; }

    /*!
     * \brief Set the residual water saturation
     */
    void setSwr(const Scalar v)
    { Swr_ = v; }

    /*!
     * \brief Set the \f$a_{1}\f$ shape parameter.
     */
    void setA1(const Scalar v)
    { a1_ = v; }

    /*!
     * \brief Set the \f$a_{1}\f$ shape parameter.
     */
    void setA2(const Scalar v)
    { a2_ = v; }

    /*!
     * \brief Set the \f$a_{1}\f$ shape parameter.
     */
    void setA3(const Scalar v)
    { a3_ = v; }


private:
    Scalar Swr_;
    Scalar a1_;
    Scalar a2_;
    Scalar a3_;
};
} // namespace Dumux

#endif
