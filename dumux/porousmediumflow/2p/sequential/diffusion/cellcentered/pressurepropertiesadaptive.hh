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
 * \ingroup FVPressure2p
 * \ingroup IMPETProperties
 */
/*!
 * \file
 *
 * \brief Defines the properties required for finite volume pressure models in a two-phase sequential model.
 */

#ifndef DUMUX_FVPRESSUREPORPERTIES2P_ADAPTIVE_SEQUENTIAL_HH
#define DUMUX_FVPRESSUREPORPERTIES2P_ADAPTIVE_SEQUENTIAL_HH

//Dumux-includes
#include <dumux/porousmediumflow/2p/sequential/diffusion/properties.hh>

namespace Dumux
{

////////////////////////////////
// forward declarations
////////////////////////////////


////////////////////////////////
// properties
////////////////////////////////
namespace Properties
{
//////////////////////////////////////////////////////////////////
// Type tags
//////////////////////////////////////////////////////////////////

//! The type tag for two-phase problems using a grid-adaptive finite volume model
NEW_TYPE_TAG(FVPressureTwoPAdaptive, INHERITS_FROM(PressureTwoP));

//////////////////////////////////////////////////////////////////
// Property tags
//////////////////////////////////////////////////////////////////

}
}

#include "velocityadaptive.hh"
#include "pressureadaptive.hh"

namespace Dumux
{
namespace Properties
{
//////////////////////////////////////////////////////////////////
// Properties
//////////////////////////////////////////////////////////////////

//! Set velocity reconstruction implementation for grid-adaptive cell centered finite volume schemes as default
SET_TYPE_PROP( FVPressureTwoPAdaptive, Velocity, Dumux::FVVelocity2PAdaptive<TypeTag> );
//! Set finite volume implementation of the two-phase pressure equation which allows hanging nodes as default pressure model
SET_TYPE_PROP(FVPressureTwoPAdaptive, PressureModel, Dumux::FVPressure2PAdaptive<TypeTag>);
//! Allow assembling algorithm for the pressure matrix to assemble only from one side of a cell-cell interface
SET_BOOL_PROP(FVPressureTwoPAdaptive, VisitFacesOnlyOnce, true);
}

}

#endif
