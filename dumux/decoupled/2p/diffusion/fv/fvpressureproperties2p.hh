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

#ifndef DUMUX_FVPRESSUREPORPERTIES2P_DECOUPLED_HH
#define DUMUX_FVPRESSUREPORPERTIES2P_DECOUPLED_HH

//Dumux-includes
#include <dumux/decoupled/2p/diffusion/diffusionproperties2p.hh>

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

//! The type tag for two-phase problems using a standard finite volume model
NEW_TYPE_TAG(FVPressureTwoP, INHERITS_FROM(PressureTwoP));

//////////////////////////////////////////////////////////////////
// Property tags
//////////////////////////////////////////////////////////////////

}
}

#include "fvvelocity2p.hh"
#include "fvpressure2p.hh"

namespace Dumux
{
namespace Properties
{
//////////////////////////////////////////////////////////////////
// Properties
//////////////////////////////////////////////////////////////////
//! Set velocity reconstruction implementation standard cell centered finite volume schemes as default
SET_TYPE_PROP( FVPressureTwoP, Velocity, Dumux::FVVelocity2P<TypeTag> );
//! Set finite volume implementation of the two-phase pressure equation as default pressure model
SET_TYPE_PROP(FVPressureTwoP, PressureModel, Dumux::FVPressure2P<TypeTag>);
//! Allow assembling algorithm for the pressure matrix to assemble only from one side of a cell-cell interface
SET_BOOL_PROP(FVPressureTwoP, VisitFacesOnlyOnce, true);

}

}

#endif
