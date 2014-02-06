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
#ifndef DUMUX_FV_TRANSPORT_PROPERTIES_2P_HH
#define DUMUX_FV_TRANSPORT_PROPERTIES_2P_HH

#include <dumux/decoupled/2p/transport/transportproperties2p.hh>

/*!
 * \ingroup FVSaturation2p
 * \ingroup IMPETProperties
 */
/*!
 * \file
 * \brief Specifies the properties for immiscible 2p transport
 */
namespace Dumux
{
namespace Properties
{
// \{

//////////////////////////////////////////////////////////////////
// Type tags tags
//////////////////////////////////////////////////////////////////

//! The type tag for two-phase problems using a standard finite volume model
NEW_TYPE_TAG(FVTransportTwoP, INHERITS_FROM(TransportTwoP));

//////////////////////////////////////////////////////////////////
// Property tags
//////////////////////////////////////////////////////////////////
NEW_PROP_TAG( PrecomputedConstRels );
//! < Bool property which tells the transport model if it should use constitutive relations which
//!   are precomputed at the begin of the time step or if it should recompute the relations
}
}

#include "evalcflfluxdefault.hh"
#include "dumux/decoupled/2p/transport/fv/diffusivepart.hh"
#include "dumux/decoupled/2p/transport/fv/convectivepart.hh"
#include <dumux/decoupled/2p/transport/fv/fvsaturation2p.hh>

namespace Dumux
{
namespace Properties
{
//! Set the default implementation of the cfl-condition
SET_TYPE_PROP(FVTransportTwoP, EvalCflFluxFunction, EvalCflFluxDefault<TypeTag>);
//! Set the default implementation of a diffusive flux -> diffusive flux dissabled
SET_TYPE_PROP(FVTransportTwoP, CapillaryFlux, DiffusivePart<TypeTag>);
//! Set the default implementation of an additional convective flux -> additional convective flux dissabled
SET_TYPE_PROP(FVTransportTwoP, GravityFlux, ConvectivePart<TypeTag>);
//! \brief Set PrecomputedConstRels flag <tt>true</tt> as default
SET_BOOL_PROP( FVTransportTwoP, PrecomputedConstRels, true);
//! Set finite volume implementation of the two-phase saturation equation as default saturation model
SET_TYPE_PROP(FVTransportTwoP, TransportModel, Dumux::FVSaturation2P<TypeTag>);
}
}

#endif
