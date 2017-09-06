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
 * \brief The primary variable switch for the extended Richards model
 */
#ifndef DUMUX_RICHARDS_PRIMARY_VARIABLE_SWITCH_HH
#define DUMUX_RICHARDS_PRIMARY_VARIABLE_SWITCH_HH

#include <dune/common/exceptions.hh>
#include <dumux/material/constants.hh>
#include <dumux/porousmediumflow/compositional/primaryvariableswitch.hh>

namespace Dumux
{
/*!
 * \ingroup TwoPTwoCModel
 * \brief The primary variable switch controlling the phase presence state variable
 */
template<class TypeTag>
class ExtendedRichardsPrimaryVariableSwitch : public PrimaryVariableSwitch<TypeTag>
{
    friend typename Dumux::PrimaryVariableSwitch<TypeTag>;
    using ParentType = Dumux::PrimaryVariableSwitch<TypeTag>;

    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using IndexType = typename GridView::IndexSet::IndexType;
    using GlobalPosition = Dune::FieldVector<Scalar, GridView::dimensionworld>;

    using PrimaryVariables = typename GET_PROP_TYPE(TypeTag, PrimaryVariables);
    using VolumeVariables = typename GET_PROP_TYPE(TypeTag, VolumeVariables);
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);
    using Indices = typename GET_PROP_TYPE(TypeTag, Indices);

    enum {
        switchIdx = Indices::switchIdx,

        wPhaseIdx = Indices::wPhaseIdx,
        nPhaseIdx = Indices::nPhaseIdx,

        wCompIdx = FluidSystem::wCompIdx,

        wPhaseOnly = Indices::wPhaseOnly,
        nPhaseOnly = Indices::nPhaseOnly,
        bothPhases = Indices::bothPhases
    };

    static constexpr bool useMoles = GET_PROP_VALUE(TypeTag, UseMoles);

    static constexpr bool enableWaterDiffusionInAir
        = GET_PROP_VALUE(TypeTag, EnableWaterDiffusionInAir);
    static constexpr bool useKelvinVaporPressure
        = GET_PROP_VALUE(TypeTag, UseKelvinEquation);

protected:

    // perform variable switch at a degree of freedom location
    bool update_(PrimaryVariables& priVars,
                 const VolumeVariables& volVars,
                 IndexType dofIdxGlobal,
                 const GlobalPosition& globalPos)
    {
        static const bool usePriVarSwitch = GET_PARAM_FROM_GROUP(TypeTag, bool, Problem, UsePrimaryVariableSwitch);
        if (!usePriVarSwitch)
            return false;

        assert(enableWaterDiffusionInAir && "The Richards primary variable switch only works with water diffusion in air enabled!");

        // evaluate primary variable switch
        bool wouldSwitch = false;
        int phasePresence = priVars.state();
        int newPhasePresence = phasePresence;

        // check if a primary var switch is necessary
        if (phasePresence == nPhaseOnly)
        {
            // if the mole fraction of water is larger than the one
            // predicted by a liquid-vapor equilibrium
            Scalar xnw = volVars.moleFraction(nPhaseIdx, wCompIdx);
            Scalar xnwPredicted = FluidSystem::H2O::vaporPressure(volVars.temperature())
                                  / volVars.pressure(nPhaseIdx);
            if (useKelvinVaporPressure)
            {
                using std::exp;
                xnwPredicted *= exp(-volVars.capillaryPressure()
                                     * FluidSystem::H2O::molarMass() / volVars.density(wPhaseIdx)
                                     / Constants<Scalar>::R / volVars.temperature());
            }

            Scalar xwMax = 1.0;
            if (xnw / xnwPredicted > xwMax)
                wouldSwitch = true;
            if (this->wasSwitched_[dofIdxGlobal])
                xwMax *= 1.01;

            // if the ratio of predicted mole fraction to current mole fraction is larger than
            // 100%, wetting phase appears
            if (xnw / xnwPredicted > xwMax)
            {
                // wetting phase appears
                std::cout << "wetting phase appears at vertex " << dofIdxGlobal
                          << ", coordinates: " << globalPos << ", xnw / xnwPredicted * 100: "
                          << xnw / xnwPredicted * 100 << "%"
                          << ", at x_n^w: " << priVars[switchIdx] << std::endl;
                newPhasePresence = bothPhases;
                priVars[switchIdx] = 0.0;
            }
        }
        else if (phasePresence == bothPhases)
        {
            Scalar Smin = 0.0;
            if (this->wasSwitched_[dofIdxGlobal])
                Smin = -0.01;

            if (volVars.saturation(wPhaseIdx) <= Smin)
            {
                wouldSwitch = true;
                // wetting phase disappears
                newPhasePresence = nPhaseOnly;
                priVars[switchIdx] = volVars.moleFraction(nPhaseIdx, wCompIdx);

                std::cout << "Wetting phase disappears at vertex " << dofIdxGlobal
                          << ", coordinates: " << globalPos << ", sw: "
                          << volVars.saturation(wPhaseIdx)
                          << ", x_n^w: " << priVars[switchIdx] << std::endl;
            }
        }
        else if (phasePresence == wPhaseOnly)
        {
            DUNE_THROW(Dune::NotImplemented, "Water phase only phase presence!");
        }

        priVars.setState(newPhasePresence);
        this->wasSwitched_[dofIdxGlobal] = wouldSwitch;
        return phasePresence != newPhasePresence;
    }
};

} // end namespace Dumux

#endif
