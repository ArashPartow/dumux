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
 * \ingroup Properties
 * \ingroup BoxProperties
 * \ingroup BoxStokes2cModel
 *
 * \file
 *
 * \brief Defines the properties required for the compositional
 * Stokes box model.
 */
#ifndef DUMUX_STOKES2C_PROPERTY_DEFAULTS_HH
#define DUMUX_STOKES2C_PROPERTY_DEFAULTS_HH

#include "stokes2cproperties.hh"
#include "stokes2cfluxvariables.hh"
#include "stokes2cindices.hh"
#include "stokes2clocalresidual.hh"
#include "stokes2cmodel.hh"
#include "stokes2cvolumevariables.hh"

#include <dumux/material/fluidstates/compositionalfluidstate.hh>

namespace Dumux
{

namespace Properties
{
//////////////////////////////////////////////////////////////////
// Properties
//////////////////////////////////////////////////////////////////

SET_PROP(BoxStokes2c, NumEq) //!< set the number of equations
{
    typedef typename GET_PROP_TYPE(TypeTag, Grid) Grid;
    static const int dim = Grid::dimension;
 public:
    static constexpr int value = 2 + dim;
};

//! Use the stokes2c local jacobian operator
SET_TYPE_PROP(BoxStokes2c,
              LocalResidual,
              Stokes2cLocalResidual<TypeTag>);

//! the Model property
SET_TYPE_PROP(BoxStokes2c, Model, Stokes2cModel<TypeTag>);

//! the VolumeVariables property
SET_TYPE_PROP(BoxStokes2c, VolumeVariables, Stokes2cVolumeVariables<TypeTag>);

//! the FluxVariables property
SET_TYPE_PROP(BoxStokes2c, FluxVariables, Stokes2cFluxVariables<TypeTag>);

//! Set the Indices for the Stokes2c model.
SET_TYPE_PROP(BoxStokes2c, Indices, Stokes2cCommonIndices<TypeTag>);

//! Set the number of components to 2
SET_INT_PROP(BoxStokes2c, NumComponents, 2);

//! Choose the type of the employed fluid state
SET_PROP(BoxStokes2c, FluidState)
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
public:
    typedef Dumux::CompositionalFluidState<Scalar, FluidSystem> type;
};

//! Choose the considered phase (single-phase system); the gas phase is used
SET_PROP(BoxStokes2c, PhaseIdx)
{
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
public:
    static constexpr int value = FluidSystem::nPhaseIdx;
};

} // namespace Properties
} // namespace Dumux
#endif // DUMUX_STOKES2C_PROPERTY_DEFAULTS_HH
