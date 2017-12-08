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
 * \file
 *
 * \brief Defines a type tags and some fundamental properties for
 *        all models
 */
#ifndef DUMUX_BASIC_PROPERTIES_HH
#define DUMUX_BASIC_PROPERTIES_HH

#include <dune/common/fvector.hh>

#include <dumux/common/balanceequationopts.hh>
#include <dumux/common/properties.hh>

#include <dumux/io/defaultvtkoutputfields.hh>

//! Forward declare
namespace Dune { class ParameterTree; }

namespace Dumux
{
namespace Properties
{
//! Type tag for numeric models.
NEW_TYPE_TAG(ModelProperties);

//! Set the default type of scalar values to double
SET_TYPE_PROP(ModelProperties, Scalar, double);

//! Set the default vector with size number of equations to a field vector
SET_TYPE_PROP(ModelProperties, NumEqVector, Dune::FieldVector<typename GET_PROP_TYPE(TypeTag, Scalar), GET_PROP_VALUE(TypeTag, NumEq)>);

//! Set the default primary variable vector to a vector of size of number of equations
SET_TYPE_PROP(ModelProperties, PrimaryVariables, typename GET_PROP_TYPE(TypeTag, NumEqVector));

//! use the global group as default for the model's parameter group
SET_STRING_PROP(ModelProperties, ModelParameterGroup, "");

//! do not specific any model-specific default parameters here
SET_PROP(ModelProperties, ModelDefaultParameters)
{
    static void defaultParams(Dune::ParameterTree& tree, const std::string& group = "") { }
};

//! Set the default to a function throwing a NotImplemented error
SET_TYPE_PROP(ModelProperties, VtkOutputFields, DefaultVtkOutputFields);

//! Set the default class for the balance equation options
SET_TYPE_PROP(ModelProperties, BalanceEqOpts, BalanceEquationOptions<TypeTag>);

} // namespace Properties
} // namespace Dumux

#endif
