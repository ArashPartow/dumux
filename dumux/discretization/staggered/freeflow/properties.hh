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
 * \brief Defines a type tag and some properties for free-flow models using the staggered scheme.
 */

#ifndef DUMUX_STAGGERD_FREE_FLOW_PROPERTIES_HH
#define DUMUX_STAGGERD_FREE_FLOW_PROPERTIES_HH

#include <dumux/common/properties.hh>
#include <dumux/discretization/staggered/properties.hh>
#include <dumux/freeflow/navierstokes/staggered/boundarytypes.hh>

#include "subcontrolvolumeface.hh"

namespace Dumux
{

namespace Properties
{

NEW_PROP_TAG(NumEq);

//! Type tag for the staggered scheme.
NEW_TYPE_TAG(StaggeredFreeFlowModel, INHERITS_FROM(StaggeredModel));

// UNSET_PROP(FreeFlow, PrimaryVariables);
UNSET_PROP(FreeFlow, NumEqVector);

SET_INT_PROP(StaggeredFreeFlowModel, NumEqFace, 1); //!< set the number of equations to 1

SET_PROP(StaggeredFreeFlowModel, NumEqCellCenter)
{
private:
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    static constexpr auto dim = GridView::dimension;
    static constexpr auto numEq = GET_PROP_VALUE(TypeTag, NumEq);
public:
    // static_assert(true, "fuuu");
    // static_assert(dim == 2, "fuuuDim");
    // static constexpr int value = GET_PROP_VALUE(TypeTag, NumEq) - dim;
    static constexpr int value = 1;
    // static_assert(value == 1, "fuuuVal");

};


//! The default sub-controlvolume face
SET_PROP(StaggeredFreeFlowModel, SubControlVolumeFace)
{
private:
    using Grid = typename GET_PROP_TYPE(TypeTag, Grid);
    static constexpr int dim = Grid::dimension;
    static constexpr int dimWorld = Grid::dimensionworld;

    struct ScvfGeometryTraits
    {
        using GridIndexType = typename Grid::LeafGridView::IndexSet::IndexType;
        using LocalIndexType = unsigned int;
        using Scalar = typename Grid::ctype;
        using Geometry = typename Grid::template Codim<1>::Geometry;
        using GlobalPosition = Dune::FieldVector<Scalar, dim>;
    };

public:
    using type = FreeFlowStaggeredSubControlVolumeFace<ScvfGeometryTraits>;
};

//! The default geometry helper required for the stencils, etc.
SET_PROP(StaggeredFreeFlowModel, StaggeredGeometryHelper)
{
private:
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
public:
    using type = FreeFlowStaggeredGeometryHelper<GridView>;
};

//! The variables living on the faces
SET_TYPE_PROP(StaggeredFreeFlowModel, FaceVariables, StaggeredFaceVariables<TypeTag>);

//! A container class used to specify values for boundary/initial conditions
SET_PROP(StaggeredFreeFlowModel, PrimaryVariables)
{
private:
    using CellCenterBoundaryValues = typename GET_PROP_TYPE(TypeTag, CellCenterPrimaryVariables);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using FaceBoundaryValues = Dune::FieldVector<typename GET_PROP_TYPE(TypeTag, Scalar),
                                                 GridView::dimension>;
public:
    using type = StaggeredPrimaryVariables<TypeTag, CellCenterBoundaryValues, FaceBoundaryValues>;
};

//! A container class used to specify values for sources and Neumann BCs
SET_PROP(StaggeredFreeFlowModel, NumEqVector)
{
private:
    using CellCenterBoundaryValues = typename GET_PROP_TYPE(TypeTag, CellCenterPrimaryVariables);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using FaceBoundaryValues = Dune::FieldVector<typename GET_PROP_TYPE(TypeTag, Scalar),
                                                 GridView::dimension>;
public:
    using type = StaggeredPrimaryVariables<TypeTag, CellCenterBoundaryValues, FaceBoundaryValues>;
};

//! Boundary types at a single degree of freedom
SET_PROP(StaggeredFreeFlowModel, BoundaryTypes)
{
private:
    static constexpr auto size = GET_PROP_VALUE(TypeTag, NumEqCellCenter) + GET_PROP_VALUE(TypeTag, NumEqFace);
public:
    using type = StaggeredFreeFlowBoundaryTypes<size>;
};

} // namespace Properties
} // namespace Dumux

#endif
