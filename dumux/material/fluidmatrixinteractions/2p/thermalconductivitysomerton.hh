// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
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
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \file
 *
 * \brief   Relation for the saturation-dependent effective thermal conductivity
 */
#ifndef THERMALCONDUCTIVITY_SOMERTON_HH
#define THERMALCONDUCTIVITY_SOMERTON_HH

#include <algorithm>

namespace Dumux
{
/*!
 * \ingroup fluidmatrixinteractionslaws
 *
 * \brief Relation for the saturation-dependent effective thermal conductivity
 *
 *  The Somerton method computes the thermal conductivity of dry and the wet soil material
 *  and uses a root function of the wetting saturation to compute the
 *  effective thermal conductivity for a two-phase fluidsystem. The individual thermal
 *  conductivities are calculated as geometric mean of the thermal conductivity of the porous
 *  material and of the respective fluid phase.
 *
 * The material law is:
 * \f[
 \lambda_\text{eff} = \lambda_{\text{dry}} + \sqrt{(S_w)} \left(\lambda_\text{wet} - \lambda_\text{dry}\right)
 \f]
 *
 * with
 * \f[
 \lambda_\text{wet} = \lambda_{solid}^{\left(1-\phi\right)}*\lambda_w^\phi
 \f]
 * and
 *
 * \f[
 \lambda_\text{dry} = \lambda_{solid}^{\left(1-\phi\right)}*\lambda_n^\phi.
 \f]
 *
 */
template<class Scalar>
class ThermalConductivitySomerton
{
public:
    /*!
     * \brief Returns the effective thermal conductivity \f$[W/(m K)]\f$ after Somerton (1974).
     *
     * \param sw The saturation of the wetting phase
     * \param lambdaW the thermal conductivity of the wetting phase
     * \param lambdaN the thermal conductivity of the non-wetting phase
     * \param lambdaSolid the thermal conductivity of the solid phase
     * \param porosity The porosity
     *
     * \return Effective thermal conductivity \f$[W/(m K)]\f$ after Somerton (1974)
     *
     * This gives an interpolation of the effective thermal conductivities of a porous medium
     * filled with the non-wetting phase and a porous medium filled with the wetting phase.
     * These two effective conductivities are computed as geometric mean of the solid and the
     * fluid conductivities and interpolated with the square root of the wetting saturation.
     * See f.e. Ebigbo, A.: Thermal Effects of Carbon Dioxide Sequestration in the Subsurface, Diploma thesis.
     */
    static Scalar effectiveThermalConductivity(const Scalar sw,
                                               const Scalar lambdaW,
                                               const Scalar lambdaN,
                                               const Scalar lambdaSolid,
                                               const Scalar porosity)
    {
        const Scalar satW = std::max<Scalar>(0.0, sw);
        // geometric means
        const Scalar lSat = std::pow(lambdaSolid, (1.0 - porosity)) * std::pow(lambdaW, porosity);
        const Scalar lDry = std::pow(lambdaSolid, (1.0 - porosity)) * std::pow(lambdaN, porosity);

        return lDry + std::sqrt(satW) * (lSat - lDry);
    }
};
}
#endif
