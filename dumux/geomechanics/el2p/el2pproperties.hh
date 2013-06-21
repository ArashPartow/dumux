//$Id: el2pproperties.hh 10663 2013-05-14 16:22:40Z melanie $
/*****************************************************************************
 *   Copyright (C) 2009 by Melanie Darcis                                    *
 *   Copyright (C) 2009 by Andreas Lauser                                    *
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
 * \file
 *
 * \brief Defines the properties required for the two phase linear-elastic model.
 *
 * This class inherits from the properties of the two-phase model and
 * from the properties of the simple linear-elastic model
 */

#ifndef DUMUX_ELASTIC2P_PROPERTIES_HH
#define DUMUX_ELASTIC2P_PROPERTIES_HH

#include <dumux/implicit/box/boxproperties.hh>
#include <dumux/implicit/2p/2pproperties.hh>

namespace Dumux
{
////////////////////////////////
// properties
////////////////////////////////
namespace Properties
{
//////////////////////////////////////////////////////////////////
// Type tags
//////////////////////////////////////////////////////////////////

//! The type tag for the twophase model with a linear elastic matrix
NEW_TYPE_TAG(BoxElasticTwoP, INHERITS_FROM(BoxModel));

//////////////////////////////////////////////////////////////////
// Property tags
//////////////////////////////////////////////////////////////////

NEW_PROP_TAG(NumPhases);   //!< Number of fluid phases in the system
NEW_PROP_TAG(NumComponents);   //!< Number of fluid components in the system
NEW_PROP_TAG(Indices); //!< Enumerations for the linear elasticity model
NEW_PROP_TAG(SpatialParams); //!< The type of the soil properties object
NEW_PROP_TAG(MaterialLaw);   //!< The material law which ought to be used (extracted from the soil)
NEW_PROP_TAG(MaterialLawParams); //!< The context material law (extracted from the soil)
NEW_PROP_TAG(WettingPhase); //!< The wetting phase for two-phase models
NEW_PROP_TAG(NonwettingPhase); //!< The non-wetting phase for two-phase models
NEW_PROP_TAG(UpwindAlpha);   //!< DEPRECATED The default value of the upwind parameter
NEW_PROP_TAG(ImplicitMobilityUpwindWeight);   //!< The default value of the upwind parameter
NEW_PROP_TAG(EnableGravity); //!< DEPRECATED Returns whether gravity is considered in the problem
NEW_PROP_TAG(ProblemEnableGravity); //!< Returns whether gravity is considered in the problem
NEW_PROP_TAG(FluidSystem); //!< The fluid systems including the information about the phases
NEW_PROP_TAG(FluidState); //!< The phases state
NEW_PROP_TAG(DisplacementGridFunctionSpace); //!< grid function space for the displacement
NEW_PROP_TAG(PressureGridFunctionSpace); //!< grid function space for the pressure, saturation, ...
NEW_PROP_TAG(GridOperatorSpace); //!< The grid operator space
NEW_PROP_TAG(GridOperator); //!< The grid operator space
NEW_PROP_TAG(PressureFEM); //!< FE space used for pressure, saturation, ...
NEW_PROP_TAG(DisplacementFEM); //!< FE space used for displacement
NEW_PROP_TAG(VtkRockMechanicsSignConvention); //!< Returns whether the output should be written according to rock mechanics sign convention (compressive stresses > 0)

//! Specifies the grid function space used for sub-problems
NEW_PROP_TAG(GridFunctionSpace);

//! Specifies the grid operator used for sub-problems
NEW_PROP_TAG(GridOperator);

//! Specifies the grid operator space used for sub-problems
NEW_PROP_TAG(GridOperatorSpace);

//! Specifies the type of the constraints
NEW_PROP_TAG(Constraints);

//! Specifies the type of the constraints transformation
NEW_PROP_TAG(ConstraintsTrafo);

//! Specifies the local finite element space
NEW_PROP_TAG(LocalFEMSpace);

//! Specifies the local operator
NEW_PROP_TAG(LocalOperator);

}

}

#endif

