// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2008-2009 by Markus Wolff                                 *
 *   Copyright (C) 2008-2009 by Andreas Lauser                               *
 *   Copyright (C) 2008 by Bernd Flemisch                                    *
 *   Institute of Hydraulic Engineering                                      *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@iws.uni-stuttgart.de                          *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \ingroup IMPES
 * \ingroup Properties
 */
/*!
 * \file
 *
 * \brief Defines the properties required for (immiscible) twophase sequential models.
 */

#ifndef DUMUX_MIMETICPROPERTIES2PADAPTIVE_DECOUPLED_HH
#define DUMUX_MIMETICPROPERTIES2PADAPTIVE_DECOUPLED_HH

//Dumux-includes
#include <dumux/decoupled/2p/diffusion/diffusionproperties2p.hh>
#include <dumux/decoupled/common/mimetic/mimeticproperties.hh>
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

//! The type tag for the two-phase problems
NEW_TYPE_TAG(MimeticPressureTwoPAdaptive, INHERITS_FROM(PressureTwoP, Mimetic))
;

//////////////////////////////////////////////////////////////////
// Property tags
//////////////////////////////////////////////////////////////////
}
}

#include <dumux/decoupled/2p/diffusion/mimetic/mimeticpressure2padaptive.hh>
#include <dumux/decoupled/2p/diffusion/mimetic/mimetic2padaptive.hh>

namespace Dumux
{
namespace Properties
{
SET_TYPE_PROP(MimeticPressureTwoPAdaptive, PressureModel, MimeticPressure2PAdaptive<TypeTag>);
SET_TYPE_PROP(MimeticPressureTwoPAdaptive, LocalStiffness, MimeticTwoPLocalStiffnessAdaptive<TypeTag>);
}
}

#endif
