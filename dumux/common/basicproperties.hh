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
 *        fully coupled and decoupled models
 */
#ifndef DUMUX_BASIC_PROPERTIES_HH
#define DUMUX_BASIC_PROPERTIES_HH

#include <dune/common/parametertree.hh>

#include <dumux/common/propertysystem.hh>
#include <dumux/common/parameters.hh>
#include <dumux/common/dgfgridcreator.hh>

namespace Dumux
{
namespace Properties
{
///////////////////////////////////
// Type tag definitions:
//
// NumericModel
// |
// +-> ImplicitModel
// |
// \-> ExplicitModel
///////////////////////////////////

//! Type tag for all models.
NEW_TYPE_TAG(NumericModel);

//! Type tag for all fully coupled models.
NEW_TYPE_TAG(ImplicitModel, INHERITS_FROM(NumericModel));

//! Type tag for all decoupled models.
NEW_TYPE_TAG(ExplicitModel, INHERITS_FROM(NumericModel));


///////////////////////////////////
// Property names which are always available:
//
// Scalar
///////////////////////////////////

//! Property to specify the type of scalar values.
NEW_PROP_TAG(Scalar);

//! Property which provides a Dune::ParameterTree.
NEW_PROP_TAG(ParameterTree);

//! Property which defines the group that is queried for parameters by default
NEW_PROP_TAG(ModelParameterGroup);

//! Property which provides a GridCreator (manages grids)
NEW_PROP_TAG(GridCreator);

//! Property to define the output level
NEW_PROP_TAG(VtkOutputLevel);

///////////////////////////////////
// Default values for properties:
//
// Scalar -> double
///////////////////////////////////

//! Set the default type of scalar values to double
SET_TYPE_PROP(NumericModel, Scalar, double);

//! Set the ParameterTree property
SET_PROP(NumericModel, ParameterTree)
{
    typedef Dune::ParameterTree type;

    static Dune::ParameterTree &tree()
    {
        static Dune::ParameterTree obj_;
        return obj_;
    };

    static Dune::ParameterTree &compileTimeParams()
    {
        static Dune::ParameterTree obj_;
        return obj_;
    };


    static Dune::ParameterTree &runTimeParams()
    {
        static Dune::ParameterTree obj_;
        return obj_;
    };

    static Dune::ParameterTree &deprecatedRunTimeParams()
    {
        static Dune::ParameterTree obj_;
        return obj_;
    };

    static Dune::ParameterTree &unusedNewRunTimeParams()
    {
        static Dune::ParameterTree obj_;
        return obj_;
    };
};

//! use the global group as default for the model's parameter group
SET_STRING_PROP(NumericModel, ModelParameterGroup, "");

//! Use the DgfGridCreator by default
SET_TYPE_PROP(NumericModel, GridCreator, Dumux::DgfGridCreator<TypeTag>);

//! Set default output level to 0 -> only primary variables are added to output
SET_INT_PROP(NumericModel, VtkOutputLevel, 0);

} // namespace Properties
} // namespace Dumux

#endif
