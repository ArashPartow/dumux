// $Id: richardsproperties.hh 3357 2010-03-25 13:02:05Z lauser $
/*****************************************************************************
 *   Copyright (C) 2009 by Andreas Lauser                                    *
 *   Institute of Hydraulic Engineering                                      *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@iws.uni-stuttgart.de                          *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version, as long as this copyright notice    *
 *   is included in its original form.                                       *
 *                                                                           *
 *   This program is distributed WITHOUT ANY WARRANTY.                       *
 *****************************************************************************/
/*!
 * \file
 *
 * \brief Contains the properties for the Richards BOX model.
 */
#ifndef DUMUX_RICHARDS_PROPERTIES_DATA_HH
#define DUMUX_RICHARDS_PROPERTIES_DATA_HH

namespace Dumux
{
/*!
 * \addtogroup RichardsBoxModel
 */
// \{
////////////////////////////////
// forward declarations
////////////////////////////////
template<class TypeTag>
class RichardsBoxModel;

template<class TypeTag>
class RichardsBoxJacobian;

template <class TypeTag>
class RichardsVertexData;

template <class TypeTag>
class RichardsElementData;

template <class TypeTag>
class RichardsFluxData;

/*!
 * \brief Indices for the single phase model.
 */
struct RichardsIndices
{
    //! index of the wetting phase pressure
    static const int pW = 0;
};

///////////////////////////////////////////////////////////////////////////
// properties for the isothermal richards model
///////////////////////////////////////////////////////////////////////////
namespace Properties {

//////////////////////////////////////////////////////////////////
// Type tags
//////////////////////////////////////////////////////////////////

//! The type tag for problems discretized using the isothermal
//! richards model
NEW_TYPE_TAG(BoxRichards, INHERITS_FROM(BoxScheme));

//////////////////////////////////////////////////////////////////
// Property tags
//////////////////////////////////////////////////////////////////

NEW_PROP_TAG(NumPhases);   //!< Number of fluid phases in the system
NEW_PROP_TAG(RichardsIndices); //!< Enumerations for the richards models
NEW_PROP_TAG(Soil); //!< The type of the soil properties object
NEW_PROP_TAG(WettingPhase); //!< The wetting phase for two-phase models
NEW_PROP_TAG(NonwettingPhase); //!< The non-wetting phase for two-phase models
NEW_PROP_TAG(EnableGravity); //!< Returns whether gravity is considered in the problem
NEW_PROP_TAG(MobilityUpwindAlpha); //!< The value of the upwind parameter for the mobility

//////////////////////////////////////////////////////////////////
// Properties
//////////////////////////////////////////////////////////////////
SET_INT_PROP(BoxRichards, NumEq, 1);
SET_INT_PROP(BoxRichards, NumPhases, 2);

//! Use the 2p local jacobian operator for the 2p model
SET_TYPE_PROP(BoxRichards,
              LocalJacobian,
              RichardsBoxJacobian<TypeTag>);

//! the Model property
SET_TYPE_PROP(BoxRichards, Model, RichardsBoxModel<TypeTag>);

//! the VertexData property
SET_TYPE_PROP(BoxRichards, VertexData, RichardsVertexData<TypeTag>);

//! the ElementData property
SET_TYPE_PROP(BoxRichards, ElementData, RichardsElementData<TypeTag>);

//! the FluxData property
SET_TYPE_PROP(BoxRichards, FluxData, RichardsFluxData<TypeTag>);

//! the weight of the upwind vertex for the mobility
SET_SCALAR_PROP(BoxRichards,
                MobilityUpwindAlpha,
                1.0);

//! The indices required by the isothermal single-phase model
SET_TYPE_PROP(BoxRichards, RichardsIndices, Dumux::RichardsIndices);

// \}
};

} // end namepace

#endif
