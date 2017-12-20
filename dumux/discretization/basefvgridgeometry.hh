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
 * \file
 * \ingroup Discretization
 * \brief Base class for all finite volume grid geometries
 */
#ifndef DUMUX_DISCRETIZATION_BASE_FV_GRID_GEOMETRY_HH
#define DUMUX_DISCRETIZATION_BASE_FV_GRID_GEOMETRY_HH

#include <dune/common/version.hh>
#include <dune/grid/common/mcmgmapper.hh>

#include <dumux/common/properties.hh>
#include <dumux/common/entitymap.hh>
#include <dumux/common/geometry/boundingboxtree.hh>
#include <dumux/common/geometry/geometricentityset.hh>

namespace Dumux
{

/*!
 * \ingroup Discretization
 * \brief Base class for all finite volume grid geometries
 */
template<class TypeTag>
class BaseFVGridGeometry
{
    using Implementation = typename GET_PROP_TYPE(TypeTag, FVGridGeometry);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using ElementMapper = typename GET_PROP_TYPE(TypeTag, ElementMapper);
    using VertexMapper = typename GET_PROP_TYPE(TypeTag, VertexMapper);
    using ElementMap = EntityMap<GridView, 0>;
    using ElementSet = GridViewGeometricEntitySet<GridView, 0>;
    using BoundingBoxTree = Dumux::BoundingBoxTree<ElementSet>;

    static const int dim = GridView::dimension;
    static const int dimWorld = GridView::dimensionworld;
    using CoordScalar = typename GridView::ctype;
    using GlobalPosition = Dune::FieldVector<CoordScalar, dimWorld>;

public:
    //! Constructor computes the bouding box of the entire domain, for e.g. setting boundary conditions
    BaseFVGridGeometry(const GridView& gridView)
    : gridView_(gridView)
#if DUNE_VERSION_NEWER(DUNE_COMMON,2,6)
    , elementMapper_(gridView, Dune::mcmgElementLayout())
    , vertexMapper_(gridView, Dune::mcmgVertexLayout())
#else
    , elementMapper_(gridView)
    , vertexMapper_(gridView)
#endif
    , bBoxMin_(std::numeric_limits<double>::max())
    , bBoxMax_(-std::numeric_limits<double>::max())
    {
        computeGlobalBoundingBox_();
    }

    /*!
     * \brief Return a local restriction of this global object.
     *        The local object is only functional after calling its bind/bindElement method.
     *        This is a free function that will be found by means of ADL
     */
    friend inline FVElementGeometry localView(const Implementation& fvGridGeometry)
    { return FVElementGeometry(fvGridGeometry); }

    /*!
     * \brief Update all fvElementGeometries (do this again after grid adaption)
     */
    void update()
    {
        //! Update the mappers
        vertexMapper_.update();
        elementMapper_.update();

        //! Compute the bouding box of the entire domain, for e.g. setting boundary conditions
        computeGlobalBoundingBox_();

        //! reset bounding box tree and the element map until requested the next time
        boundingBoxTree_.release();
        elementMap_.reset();
    }

    /*!
     * \brief Return the gridView this grid geometry object lives on
     */
    const GridView& gridView() const
    { return gridView_; }

    /*!
     * \brief Returns the mapper for vertices to indices for constant grids.
     */
    const VertexMapper &vertexMapper() const
    { return vertexMapper_; }

    /*!
     * \brief Returns the mapper for elements to indices for constant grids.
     */
    const ElementMapper &elementMapper() const
    { return elementMapper_; }

    /*!
     * \brief Returns the mapper for vertices to indices for possibly adaptive grids.
     */
    VertexMapper &vertexMapper()
    { return vertexMapper_; }

    /*!
     * \brief Returns the mapper for elements to indices for possibly adaptive grids.
     */
    ElementMapper &elementMapper()
    { return elementMapper_; }

    /*!
     * \brief Returns the bounding box tree of the grid
     */
    const BoundingBoxTree& boundingBoxTree() const
    {
        if(!boundingBoxTree_)
        {
            elementMap(); // make sure the element map is built
            boundingBoxTree_ = std::make_unique<BoundingBoxTree>
                                    ( std::make_shared<ElementSet>(gridView_, elementMapper(), elementMap_) );
        }

        return *boundingBoxTree_;
    }

    /*!
     * \brief Returns the element index to element map
     */
    const ElementMap& elementMap() const
    {
        if(!elementMap_)
            elementMap_ = std::make_shared<ElementMap>(gridView_.grid(), elementMapper_);

        return *elementMap_;
    }

    /*!
     * \brief The coordinate of the corner of the GridView's bounding
     *        box with the smallest values.
     */
    const GlobalPosition &bBoxMin() const
    { return bBoxMin_; }

    /*!
     * \brief The coordinate of the corner of the GridView's bounding
     *        box with the largest values.
     */
    const GlobalPosition &bBoxMax() const
    { return bBoxMax_; }

private:

    //! Compute the bouding box of the entire domain, for e.g. setting boundary conditions
    void computeGlobalBoundingBox_()
    {
        // calculate the bounding box of the local partition of the grid view
        for (const auto& vertex : vertices(gridView_))
        {
            for (int i=0; i<dimWorld; i++)
            {
                using std::min;
                using std::max;
                bBoxMin_[i] = min(bBoxMin_[i], vertex.geometry().corner(0)[i]);
                bBoxMax_[i] = max(bBoxMax_[i], vertex.geometry().corner(0)[i]);
            }
        }

        // communicate to get the bounding box of the whole domain
        if (gridView_.comm().size() > 1)
        {
            for (int i = 0; i < dimWorld; ++i)
            {
                bBoxMin_[i] = gridView_.comm().min(bBoxMin_[i]);
                bBoxMax_[i] = gridView_.comm().max(bBoxMax_[i]);
            }
        }
    }

    //! the process grid view
    const GridView gridView_;

    //! entity mappers
    ElementMapper elementMapper_;
    VertexMapper vertexMapper_;

    //! the bounding box tree of the grid view for effecient element intersections
    mutable std::unique_ptr<BoundingBoxTree> boundingBoxTree_;

    //! a map from element index to elements (needed in the bounding box tree and for assembling cell-centered discretization)
    mutable std::shared_ptr<ElementMap> elementMap_;

    //! the bounding box of the whole domain
    GlobalPosition bBoxMin_;
    GlobalPosition bBoxMax_;
};

} // end namespace Dumux

#endif
