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
 *        finite volume in the Stokes box model.
 */
#ifndef DUMUX_STOKES_VOLUME_VARIABLES_HH
#define DUMUX_STOKES_VOLUME_VARIABLES_HH

#include "stokesproperties.hh"
#include "dumux/implicit/common/implicitvolumevariables.hh"

#include <dumux/material/fluidstates/immisciblefluidstate.hh>

namespace Dumux
{

/*!
 * \ingroup BoxStokesModel
 * \ingroup BoxVolumeVariables
 * \brief Contains the quantities which are are constant within a
 *        finite volume in the Stokes box model.
 */
template <class TypeTag>
class StokesVolumeVariables : public BoxVolumeVariables<TypeTag>
{
    typedef BoxVolumeVariables<TypeTag> ParentType;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;

    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GET_PROP_TYPE(TypeTag, VolumeVariables) Implementation;
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;

    enum {
        dim = GridView::dimension,

        momentumXIdx = Indices::momentumXIdx,
        lastMomentumIdx = Indices::lastMomentumIdx,
        pressureIdx = Indices::pressureIdx
    };

    enum { phaseIdx = GET_PROP_VALUE(TypeTag, PhaseIdx) };

    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, FluidState) FluidState;

    typedef Dune::FieldVector<Scalar, dim> DimVector;

public:
    /*!
     * \copydoc BoxVolumeVariables::update()
     */
    void update(const PrimaryVariables &priVars,
                const Problem &problem,
                const Element &element,
                const FVElementGeometry &fvGeometry,
                const int scvIdx,
                const bool isOldSol)
    {
        ParentType::update(priVars,
                           problem,
                           element,
                           fvGeometry,
                           scvIdx,
                           isOldSol);

        completeFluidState(priVars, problem, element, fvGeometry, scvIdx, fluidState_, isOldSol);

        for (int dimIdx=momentumXIdx; dimIdx<=lastMomentumIdx; ++dimIdx)
            velocity_[dimIdx] = priVars[dimIdx];
    }

    /*!
     * \copydoc BoxModel::completeFluidState()
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
        Scalar temperature = Implementation::temperature_(priVars, problem,
                                                          element, fvGeometry, scvIdx);
        fluidState.setTemperature(temperature);
        fluidState.setPressure(phaseIdx, priVars[pressureIdx]);

        // create NullParameterCache and do dummy update
        typename FluidSystem::ParameterCache paramCache;
        paramCache.updateAll(fluidState);

        fluidState.setDensity(phaseIdx,
                              FluidSystem::density(fluidState,
                                                   paramCache,
                                                   phaseIdx));
        fluidState.setViscosity(phaseIdx,
                                FluidSystem::viscosity(fluidState,
                                                       paramCache,
                                                       phaseIdx));

        // compute and set the enthalpy
        Scalar h = Implementation::enthalpy_(fluidState, paramCache, phaseIdx);
        fluidState.setEnthalpy(phaseIdx, h);

//        int globalVertIdx = problem.model().dofMapper().map(element, scvIdx, dim);
    }

    /*!
     * \brief Returns the phase state for the control-volume.
     */
    const FluidState &fluidState() const
    { return fluidState_; }
    FluidState &fluidState()
    { return fluidState_; }

    /*!
     * \brief Returns the mass density \f$\mathrm{[kg/m^3]}\f$ of the fluid within the
     *        sub-control volume.
     */
    Scalar density() const
    { return fluidState_.density(phaseIdx); }

    /*!
     * \brief Returns the molar density \f$\mathrm{[mol/m^3]}\f$ of the fluid within the
     *        sub-control volume.
     * \deprecated Use molarDensity() from Stokes2c model because it does not belong
     *             to this model.
     */
    DUNE_DEPRECATED_MSG("Use molarDensity() from Stokes2c model")
    Scalar molarDensity() const
    { return fluidState_.density(phaseIdx) / fluidState_.averageMolarMass(phaseIdx); }

    /*!
     * \brief Returns the fluid pressure \f$\mathrm{[Pa]}\f$ within
     *        the sub-control volume.
     */
    Scalar pressure() const
    { return fluidState_.pressure(phaseIdx); }

    /*!
     * \brief Returns temperature\f$\mathrm{[T]}\f$ inside the sub-control volume.
     */
    Scalar temperature() const
    { return fluidState_.temperature(phaseIdx); }

    /*!
     * \brief Returns the dynamic viscosity \f$ \mathrm{[Pa s]} \f$ of the fluid in
     *        the sub-control volume.
     */
    Scalar viscosity() const
    { return fluidState_.viscosity(phaseIdx); }

    /*!
     * \brief Returns the velocity vector in the sub-control volume.
     */
    const DimVector &velocity() const
    { return velocity_; }

protected:
    template<class ParameterCache>
    static Scalar enthalpy_(const FluidState& fluidState,
                            const ParameterCache& paramCache,
                            int phaseIdx)
    {
        return 0;
    }

    static Scalar temperature_(const PrimaryVariables &priVars,
                            const Problem& problem,
                            const Element &element,
                            const FVElementGeometry &fvGeometry,
                            const int scvIdx)
    {
        return problem.boxTemperature(element, fvGeometry, scvIdx);
    }

    DimVector velocity_;
    FluidState fluidState_;

private:
    Implementation &asImp()
    { return *static_cast<Implementation*>(this); }
    const Implementation &asImp() const
    { return *static_cast<const Implementation*>(this); }
};

}

#endif
