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
 * \brief Adaption of the fully implicit scheme to the three-phase flow model.
 *
 * The model is designed for simulating three fluid phases with water, gas, and
 * a liquid contaminant (NAPL - non-aqueous phase liquid)
 */
#ifndef DUMUX_3P_MODEL_HH
#define DUMUX_3P_MODEL_HH

/*!
 * \file
 * \ingroup ThreePModel
 * \brief Adaption of the fully implicit scheme to the three-phase flow model.
 *
 * This model implements three-phase flow of three fluid phases
 * \f$\alpha \in \{ water, gas, NAPL \}\f$
 * The standard multiphase Darcy
 * approach is used as the equation for the conservation of momentum.
 *
 * By inserting this into the equations for the conservation of the
 * components, the well-known multiphase flow equation is obtained.
 *
 * All equations are discretized using a vertex-centered finite volume (box)
 * or cell-centered finite volume scheme as spatial
 * and the implicit Euler method as time discretization.
 *
 * The model uses commonly applied auxiliary conditions like
 * \f$S_w + S_n + S_g = 1\f$ for the saturations.
 * Furthermore, the phase pressures are related to each other via
 * capillary pressures between the fluid phases, which are functions of
 * the saturation, e.g. according to the approach of Parker et al.
 *
 * The used primary variables are gas phase pressure \f$p_g\f$,
 * water saturation \f$S_w\f$ and NAPL saturation \f$S_n\f$.
 */

#include <dumux/common/properties.hh>

#include <dumux/material/spatialparams/fv.hh>
#include <dumux/material/fluidstates/immiscible.hh>
#include <dumux/material/fluidmatrixinteractions/3p/thermalconductivitysomerton3p.hh>

#include <dumux/porousmediumflow/properties.hh>
#include <dumux/porousmediumflow/immiscible/localresidual.hh>
#include <dumux/porousmediumflow/nonisothermal/model.hh>

#include "indices.hh"
#include "volumevariables.hh"
#include "vtkoutputfields.hh"

namespace Dumux {
namespace Properties {

//////////////////////////////////////////////////////////////////
// Type tags
//////////////////////////////////////////////////////////////////
NEW_TYPE_TAG(ThreeP, INHERITS_FROM(PorousMediumFlow));
NEW_TYPE_TAG(ThreePNI, INHERITS_FROM(ThreeP, NonIsothermal));

//////////////////////////////////////////////////////////////////
// Properties for the isothermal 3p model
//////////////////////////////////////////////////////////////////
/*!
 * \brief Set the property for the number of fluid phases.
 *
 * We just forward the number from the fluid system and use an static
 * assert to make sure it is 3.
 */
SET_PROP(ThreeP, NumPhases)
{
 private:
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;

 public:
    static const int value = FluidSystem::numPhases;
    static_assert(value == 3,
                  "Only fluid systems with 3 phases are supported by the 3p model!");
};

SET_PROP(ThreeP, NumComponents)
{
 private:
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;

 public:
    static const int value = FluidSystem::numComponents;
    static_assert(value == 3,
                  "Only fluid systems with 3 components are supported by the 3p model!");
};

SET_INT_PROP(ThreeP, NumEq, 3); //!< set the number of equations to 3

//! The local residual function of the conservation equations
SET_TYPE_PROP(ThreeP, LocalResidual, ImmiscibleLocalResidual<TypeTag>);

//! Enable advection
SET_BOOL_PROP(ThreeP, EnableAdvection, true);

//! disable molecular diffusion for the 3p model
SET_BOOL_PROP(ThreeP, EnableMolecularDiffusion, false);

//! Isothermal model by default
SET_BOOL_PROP(ThreeP, EnableEnergyBalance, false);

//! the VolumeVariables property
SET_TYPE_PROP(ThreeP, VolumeVariables, ThreePVolumeVariables<TypeTag>);

//! The indices required by the isothermal 3p model
SET_TYPE_PROP(ThreeP, Indices, ThreePIndices<TypeTag,/*PVOffset=*/0>);

//! The spatial parameters to be employed.
//! Use FVSpatialParams by default.
SET_TYPE_PROP(ThreeP, SpatialParams, FVSpatialParams<TypeTag>);

SET_PROP(ThreeP, FluidState)
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
public:
    typedef ImmiscibleFluidState<Scalar, FluidSystem> type;
};

//! Set the vtk output fields specific to the ThreeP model
SET_TYPE_PROP(ThreeP, VtkOutputFields, ThreePVtkOutputFields<TypeTag>);


/////////////////////////////////////////////////
// Properties for the non-isothermal 3p model
/////////////////////////////////////////////////

//! Somerton is used as default model to compute the effective thermal heat conductivity
SET_PROP(ThreePNI, ThermalConductivityModel)
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
public:
    typedef ThermalConductivitySomerton<Scalar, Indices> type;
};

//////////////////////////////////////////////////////////////////
// Property values for isothermal model required for the general non-isothermal model
//////////////////////////////////////////////////////////////////

//set isothermal VolumeVariables
SET_TYPE_PROP(ThreePNI, IsothermalVolumeVariables, ThreePVolumeVariables<TypeTag>);

//set isothermal LocalResidual
SET_TYPE_PROP(ThreePNI, IsothermalLocalResidual, ImmiscibleLocalResidual<TypeTag>);

//set isothermal output fields
SET_TYPE_PROP(ThreePNI, IsothermalVtkOutputFields, ThreePVtkOutputFields<TypeTag>);

//set isothermal Indices
SET_TYPE_PROP(ThreePNI, IsothermalIndices, ThreePIndices<TypeTag,/*PVOffset=*/0>);

//set isothermal NumEq
SET_INT_PROP(ThreePNI, IsothermalNumEq, 3);

} // end namespace Properties
} // end namespace Dumux

#endif
