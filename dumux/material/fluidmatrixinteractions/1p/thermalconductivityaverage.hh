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
 * \brief simple effective thermal conductivity
 */
#ifndef THERMALCONDUCTIVITY_AVERAGE_HH
#define THERMALCONDUCTIVITY_AVERAGE_HH

#include <algorithm>


namespace Dumux
{
/*!
 * \ingroup fluidmatrixinteractionslaws
 *
 * \brief Relation for a simple effective thermal conductivity
 */
template<class Scalar>
class ThermalConductivityAverage
{
public:
    /*!
     * \brief simple effective thermal conductivity \f$[W/(m K)]\f$
     *
     * \param volVars volume variables
     * \param spatialParams spatial parameters
     * \param element element (to be passed to spatialParams)
     * \param fvGeometry fvGeometry (to be passed to spatialParams)
     * \param scvIdx scvIdx (to be passed to spatialParams)
     *
     * \return effective thermal conductivity \f$[W/(m K)]\f$
     */
    template<class VolumeVariables, class SpatialParams, class Element, class FVGeometry>
    static Scalar effectiveThermalConductivity(const VolumeVariables& volVars,
                                               const SpatialParams& spatialParams,
                                               const Element& element,
                                               const FVGeometry& fvGeometry,
                                               int scvIdx)
    {
        //Get the thermal conductivities and the porosity from the volume variables
        Scalar lambdaW = volVars.thermalConductivityFluid(0);
        Scalar lambdaSolid = volVars.thermalConductivitySolid();
        Scalar porosity = volVars.porosity();

        return lambdaSolid*(1-porosity) + lambdaW*porosity;
    }
};
}
#endif
