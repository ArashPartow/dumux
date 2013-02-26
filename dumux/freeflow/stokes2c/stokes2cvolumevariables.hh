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
 * \brief Contains the quantities which are constant within a
 *        finite volume in the compositional Stokes model.
 */
#ifndef DUMUX_STOKES2C_VOLUME_VARIABLES_HH
#define DUMUX_STOKES2C_VOLUME_VARIABLES_HH

#include <dumux/freeflow/stokes/stokesvolumevariables.hh>
#include "stokes2cproperties.hh"

namespace Dumux
{

/*!
 * \ingroup BoxStokes2cModel
 * \ingroup BoxVolumeVariables
 * \brief Contains the quantities which are are constant within a
 *        finite volume in the two-component Stokes box model.
 */
template <class TypeTag>
class Stokes2cVolumeVariables : public StokesVolumeVariables<TypeTag>
{
    typedef StokesVolumeVariables<TypeTag> ParentType;

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;

    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, FluidState) FluidState;
    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;

    enum {
        transportCompIdx = Indices::transportCompIdx,
        phaseCompIdx = Indices::phaseCompIdx
    };
    enum { numComponents = GET_PROP_VALUE(TypeTag, NumComponents) };
    enum { phaseIdx = GET_PROP_VALUE(TypeTag, PhaseIdx) };
    enum { transportEqIdx = Indices::transportEqIdx };
    enum { massOrMoleFracIdx = Indices::massOrMoleFracIdx };

public:
    /*!
     * \copydoc ImplicitVolumeVariables::update()
     */
    void update(const PrimaryVariables &priVars,
                const Problem &problem,
                const Element &element,
                const FVElementGeometry &fvGeometry,
                const int scvIdx,
                const bool isOldSol)
    {
        // set the mole fractions first
        completeFluidState(priVars, problem, element, fvGeometry, scvIdx, this->fluidState(), isOldSol);

        // update vertex data for the mass and momentum balance
        ParentType::update(priVars,
                           problem,
                           element,
                           fvGeometry,
                           scvIdx,
                           isOldSol);

        // Second instance of a parameter cache.
        // Could be avoided if diffusion coefficients also
        // became part of the fluid state.
        typename FluidSystem::ParameterCache paramCache;
        paramCache.updateAll(this->fluidState());

        diffCoeff_ = FluidSystem::binaryDiffusionCoefficient(this->fluidState(),
                                                             paramCache,
                                                             phaseIdx,
                                                             transportCompIdx,
                                                             phaseCompIdx);

        Valgrind::CheckDefined(diffCoeff_);
    };

    /*!
     * \copydoc ImplicitModel::completeFluidState()
     * \param isOldSol Specifies whether this is the previous solution or the current one
     */
    static void completeFluidState(const PrimaryVariables& priVars,
                                   const Problem& problem,
                                   const Element& element,
                                   const FVElementGeometry& fvGeometry,
                                   const int scvIdx,
                                   FluidState& fluidState,
                                   const bool isOldSol = false)
    {
        Scalar massFraction[numComponents];
        massFraction[transportCompIdx] = priVars[massOrMoleFracIdx];
        massFraction[phaseCompIdx] = 1 - massFraction[transportCompIdx];

        // calculate average molar mass of the gas phase
        Scalar M1 = FluidSystem::molarMass(transportCompIdx);
        Scalar M2 = FluidSystem::molarMass(phaseCompIdx);
        Scalar X2 = massFraction[phaseCompIdx];
        Scalar avgMolarMass = M1*M2/(M2 + X2*(M1 - M2));

        // convert mass to mole fractions and set the fluid state
        fluidState.setMoleFraction(phaseIdx, transportCompIdx, massFraction[transportCompIdx]*avgMolarMass/M1);
        fluidState.setMoleFraction(phaseIdx, phaseCompIdx, massFraction[phaseCompIdx]*avgMolarMass/M2);
    }

    /*!
     * \brief Returns the molar density \f$\mathrm{[mol/m^3]}\f$ of the fluid within the
     *        sub-control volume.
     */
    Scalar molarDensity() const
    { return this->fluidState_.density(phaseIdx) / this->fluidState_.averageMolarMass(phaseIdx); }

    /*!
     * \brief Returns the binary (mass) diffusion coefficient
     */
    Scalar diffusionCoeff() const
    { return diffCoeff_; }

protected:
    Scalar diffCoeff_; //!< Binary diffusion coefficient
};

} // end namespace

#endif
