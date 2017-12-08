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
 * \brief Element-wise calculation of the local residual for problems using a
 *        compositional fully implicit model that also considers solid phases.
 */
#ifndef DUMUX_COMPOSITIONAL_MINERALIZATION_LOCAL_RESIDUAL_HH
#define DUMUX_COMPOSITIONAL_MINERALIZATION_LOCAL_RESIDUAL_HH

#include <dumux/porousmediumflow/compositional/localresidual.hh>

namespace Dumux
{
/*!
 * \ingroup Mineralization
 * \ingroup LocalResidual
 * \brief Element-wise calculation of the local residual for problems
 *        using a one/two-phase n-component mineralization fully implicit model.
 */
template<class TypeTag>
class MineralizationLocalResidual: public CompositionalLocalResidual<TypeTag>
{
    using ParentType = CompositionalLocalResidual<TypeTag>;
    using ResidualVector = typename GET_PROP_TYPE(TypeTag, NumEqVector);
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using VolumeVariables = typename GET_PROP_TYPE(TypeTag, VolumeVariables);
    using Indices = typename GET_PROP_TYPE(TypeTag, Indices);
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);

    static constexpr int numPhases = GET_PROP_VALUE(TypeTag, NumPhases);
    static constexpr int numSPhases = GET_PROP_VALUE(TypeTag, NumSPhases);
    static constexpr int numComponents = GET_PROP_VALUE(TypeTag, NumComponents);
    static constexpr bool useMoles = GET_PROP_VALUE(TypeTag, UseMoles);

    enum { conti0EqIdx = Indices::conti0EqIdx };

public:
    using ParentType::ParentType;

    /*!
     * \brief Evaluate the amount of all conservation quantities
     *        (e.g. phase mass) within a sub-control volume.
     *
     * The result should be averaged over the volume (e.g. phase mass
     * inside a sub control volume divided by the volume)
     *
     *  \param storage The mass of the component within the sub-control volume
     *  \param scvIdx The SCV (sub-control-volume) index
     *  \param usePrevSol Evaluate function with solution of current or previous time step
     */
    ResidualVector computeStorage(const Problem& problem,
                                  const SubControlVolume& scv,
                                  const VolumeVariables& volVars) const
    {
        auto storage = ParentType::computeStorage(problem, scv, volVars);

        const auto massOrMoleDensity = [](const auto& volVars, const int phaseIdx)
        { return useMoles ? volVars.molarDensity(phaseIdx) : volVars.density(phaseIdx); };

        // compute storage term of all components within all fluid phases
        for (int phaseIdx = numPhases; phaseIdx < numPhases + numSPhases; ++phaseIdx)
        {
            auto eqIdx = conti0EqIdx + numComponents-numPhases + phaseIdx;
            storage[eqIdx] += volVars.precipitateVolumeFraction(phaseIdx)
                             * massOrMoleDensity(volVars, phaseIdx);
        }

        return storage;
    }
};

} // end namespace Dumux

#endif