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
 *  \file
 *
 * \ingroup TwoPNCModel
 * \brief Adaption of the fully implicit scheme to the
 *        two-phase n-component fully implicit model.
 *
 * This model implements two-phase n-component flow of two compressible and
 * partially miscible fluids \f$\alpha \in \{ w, n \}\f$ composed of the n components
 * \f$\kappa \in \{ w, n,\cdots \}\f$ in combination with mineral precipitation and dissolution.
 * The solid phases. The standard multiphase Darcy
 * approach is used as the equation for the conservation of momentum:
 * \f[
 v_\alpha = - \frac{k_{r\alpha}}{\mu_\alpha} \mbox{\bf K}
 \left(\text{grad}\, p_\alpha - \varrho_{\alpha} \mbox{\bf g} \right)
 * \f]
 *
 * By inserting this into the equations for the conservation of the
 * components, one gets one transport equation for each component
 * \f{eqnarray}
 && \frac{\partial (\sum_\alpha \varrho_\alpha X_\alpha^\kappa \phi S_\alpha )}
 {\partial t}
 - \sum_\alpha  \text{div} \left\{ \varrho_\alpha X_\alpha^\kappa
 \frac{k_{r\alpha}}{\mu_\alpha} \mbox{\bf K}
 (\text{grad}\, p_\alpha - \varrho_{\alpha}  \mbox{\bf g}) \right\}
 \nonumber \\ \nonumber \\
    &-& \sum_\alpha \text{div} \left\{{\bf D_{\alpha, pm}^\kappa} \varrho_{\alpha} \text{grad}\, X^\kappa_{\alpha} \right\}
 - \sum_\alpha q_\alpha^\kappa = 0 \qquad \kappa \in \{w, a,\cdots \} \, ,
 \alpha \in \{w, g\}
 \f}
 *
 * The solid or mineral phases are assumed to consist of a single component.
 * Their mass balance consist only of a storage and a source term:
 *  \f$\frac{\partial \varrho_\lambda \phi_\lambda )} {\partial t}
 *  = q_\lambda\f$
 *
 * All equations are discretized using a vertex-centered finite volume (box)
 * or cell-centered finite volume scheme as
 * spatial and the implicit Euler method as time discretization.
 *
 * By using constitutive relations for the capillary pressure \f$p_c =
 * p_n - p_w\f$ and relative permeability \f$k_{r\alpha}\f$ and taking
 * advantage of the fact that \f$S_w + S_n = 1\f$ and \f$X^\kappa_w + X^\kappa_n = 1\f$, the number of
 * unknowns can be reduced to number of components.
 *
 * The used primary variables are, like in the two-phase model, either \f$p_w\f$ and \f$S_n\f$
 * or \f$p_n\f$ and \f$S_w\f$. The formulation which ought to be used can be
 * specified by setting the <tt>Formulation</tt> property to either
 * TwoPTwoCIndices::pwsn or TwoPTwoCIndices::pnsw. By
 * default, the model uses \f$p_w\f$ and \f$S_n\f$.
 *
 * Moreover, the second primary variable depends on the phase state, since a
 * primary variable switch is included. The phase state is stored for all nodes
 * of the system. The model is uses mole fractions.
 *Following cases can be distinguished:
 * <ul>
 *  <li> Both phases are present: The saturation is used (either \f$S_n\f$ or \f$S_w\f$, dependent on the chosen <tt>Formulation</tt>),
 *      as long as \f$ 0 < S_\alpha < 1\f$</li>.
 *  <li> Only wetting phase is present: The mole fraction of, e.g., air in the wetting phase \f$x^a_w\f$ is used,
 *      as long as the maximum mole fraction is not exceeded (\f$x^a_w<x^a_{w,max}\f$)</li>
 *  <li> Only non-wetting phase is present: The mole fraction of, e.g., water in the non-wetting phase, \f$x^w_n\f$, is used,
 *      as long as the maximum mole fraction is not exceeded (\f$x^w_n<x^w_{n,max}\f$)</li>
 * </ul>
 *
 * For the other components, the mole fraction \f$x^\kappa_w\f$ is the primary variable.
 */

#ifndef DUMUX_2PNC_MODEL_HH
#define DUMUX_2PNC_MODEL_HH

#include <dumux/common/properties.hh>

#include <dumux/material/spatialparams/implicit.hh>
#include <dumux/material/fluidmatrixinteractions/diffusivitymillingtonquirk.hh>
#include <dumux/material/fluidmatrixinteractions/2p/thermalconductivitysomerton.hh>

#include <dumux/porousmediumflow/properties.hh>
#include <dumux/porousmediumflow/compositional/localresidual.hh>
#include <dumux/porousmediumflow/compositional/switchableprimaryvariables.hh>
#include <dumux/porousmediumflow/nonisothermal/model.hh>

#include "indices.hh"
#include "volumevariables.hh"
#include "primaryvariableswitch.hh"
#include "vtkoutputfields.hh"

namespace Dumux
{

namespace Properties
{
//////////////////////////////////////////////////////////////////
// Type tags
//////////////////////////////////////////////////////////////////
NEW_TYPE_TAG(TwoPNC, INHERITS_FROM(PorousMediumFlow));
NEW_TYPE_TAG(TwoPNCNI, INHERITS_FROM(TwoPNC, NonIsothermal));

//////////////////////////////////////////////////////////////////
// Properties for the isothermal 2pnc model
//////////////////////////////////////////////////////////////////
SET_TYPE_PROP(TwoPNC, PrimaryVariables, SwitchablePrimaryVariables<TypeTag, int>);          //! The primary variables vector for the 2pnc model
SET_TYPE_PROP(TwoPNC, PrimaryVariableSwitch, TwoPNCPrimaryVariableSwitch<TypeTag>);         //! The primary variable switch for the 2pnc model
SET_TYPE_PROP(TwoPNC, VolumeVariables, TwoPNCVolumeVariables<TypeTag>);                     //! the VolumeVariables property
SET_TYPE_PROP(TwoPNC, Indices, TwoPNCIndices <TypeTag, /*PVOffset=*/0>);                    //! The indices required by the isothermal 2pnc model
SET_TYPE_PROP(TwoPNC, SpatialParams, FVSpatialParams<TypeTag>);                       //! Use the FVSpatialParams by default
SET_TYPE_PROP(TwoPNC, VtkOutputFields, TwoPNCVtkOutputFields<TypeTag>);                     //! Set the vtk output fields specific to the TwoPNC model
SET_TYPE_PROP(TwoPNC, LocalResidual, CompositionalLocalResidual<TypeTag>);                  //! Use the compositional local residual

SET_INT_PROP(TwoPNC, NumComponents, GET_PROP_TYPE(TypeTag, FluidSystem)::numComponents);    //! Use the number of components of the fluid system
SET_INT_PROP(TwoPNC, ReplaceCompEqIdx, GET_PROP_TYPE(TypeTag, FluidSystem)::numComponents); //! Per default, no component mass balance is replaced
SET_INT_PROP(TwoPNC, NumEq, GET_PROP_TYPE(TypeTag, FluidSystem)::numComponents);            //! We solve one equation per component
SET_INT_PROP(TwoPNC, Formulation, TwoPNCFormulation::pwsn);                                 //! Default formulation is pw-Sn, overwrite if necessary

SET_BOOL_PROP(TwoPNC, SetMoleFractionsForWettingPhase, true);  //! Set the primary variables mole fractions for the wetting or non-wetting phase
SET_BOOL_PROP(TwoPNC, EnableAdvection, true);                  //! Enable advection
SET_BOOL_PROP(TwoPNC, EnableMolecularDiffusion, true);         //! Enable molecular diffusion
SET_BOOL_PROP(TwoPNC, EnableEnergyBalance, false);             //! This is the isothermal variant of the model
SET_BOOL_PROP(TwoPNC, UseMoles, true);                         //! Use mole fractions in the balance equations by default


//! Use the model after Millington (1961) for the effective diffusivity
SET_TYPE_PROP(TwoPNC, EffectiveDiffusivityModel, DiffusivityMillingtonQuirk<typename GET_PROP_TYPE(TypeTag, Scalar)>);

//! The major components belonging to the existing phases, e.g. 2 for water and air being the major components in a liquid-gas-phase system
SET_PROP(TwoPNC, NumMajorComponents)
{
private:
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);

public:
    static const int value = FluidSystem::numPhases;
    static_assert(value == 2, "The model is restricted to two phases, thus number of major components must also be two.");
};

//! We use the number of phases of the fluid system. Make sure it is 2!
SET_PROP(TwoPNC, NumPhases)
{
private:
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);

public:
    static const int value = FluidSystem::numPhases;
    static_assert(value == 2, "Only fluid systems with 2 fluid phases are supported by the 2p-nc model!");
};

//! This model uses the compositional fluid state
SET_PROP(TwoPNC, FluidState)
{
private:
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);
public:
    using type = CompositionalFluidState<Scalar, FluidSystem>;
};

/////////////////////////////////////////////////
// Properties for the non-isothermal 2pnc model
/////////////////////////////////////////////////
SET_TYPE_PROP(TwoPNCNI, IsothermalVolumeVariables, TwoPNCVolumeVariables<TypeTag>);     //! set isothermal VolumeVariables
SET_TYPE_PROP(TwoPNCNI, IsothermalLocalResidual, CompositionalLocalResidual<TypeTag>);  //! set isothermal LocalResidual
SET_TYPE_PROP(TwoPNCNI, IsothermalIndices, TwoPNCIndices<TypeTag, /*PVOffset=*/0>);     //! set isothermal Indices
SET_TYPE_PROP(TwoPNCNI, IsothermalVtkOutputFields, TwoPNCVtkOutputFields<TypeTag>);     //! set isothermal output fields

//! Somerton is used as default model to compute the effective thermal heat conductivity
SET_PROP(TwoPNCNI, ThermalConductivityModel)
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
public:
    typedef ThermalConductivitySomerton<Scalar, Indices> type;
};

//! set isothermal NumEq
SET_PROP(TwoPNCNI, IsothermalNumEq)
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(FluidSystem)) FluidSystem;

public:
    static const int value = FluidSystem::numComponents;
};

} // end namespace Properties
} // end namespace Dumux

#endif
