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
 * \brief Base class for a sub control volume face
 */
#ifndef DUMUX_DISCRETIZATION_BOX_SUBCONTROLVOLUMEFACE_HH
#define DUMUX_DISCRETIZATION_BOX_SUBCONTROLVOLUMEFACE_HH

#include <utility>
#include <dune/geometry/type.hh>
#include <dune/common/version.hh>
#include <dumux/discretization/subcontrolvolumefacebase.hh>
#include <dumux/discretization/box/boxgeometryhelper.hh>

namespace Dumux
{

/*!
 * \ingroup Discretization
 * \brief Class for a sub control volume face in the box method, i.e a part of the boundary
 *        of a sub control volume we compute fluxes on. We simply use the base class here.
 */
template<class ScvfGeometryTraits>
class BoxSubControlVolumeFace
: public SubControlVolumeFaceBase<BoxSubControlVolumeFace<ScvfGeometryTraits>, ScvfGeometryTraits>
{
    using ParentType = SubControlVolumeFaceBase<BoxSubControlVolumeFace<ScvfGeometryTraits>, ScvfGeometryTraits>;
    using GridIndexType = typename ScvfGeometryTraits::GridIndexType;
    using LocalIndexType = typename ScvfGeometryTraits::LocalIndexType;
    using Scalar = typename ScvfGeometryTraits::Scalar;
    using GlobalPosition = typename ScvfGeometryTraits::GlobalPosition;
    using CornerStorage = typename ScvfGeometryTraits::CornerStorage;
    using Geometry = typename ScvfGeometryTraits::Geometry;

public:
    //! The default constructor
    BoxSubControlVolumeFace() = default;

    //! Constructor for inner scvfs
    template<class GeometryHelper, class Element>
    BoxSubControlVolumeFace(const GeometryHelper& geometryHelper,
                            const Element& element,
                            const typename Element::Geometry& elemGeometry,
                            GridIndexType scvfIndex,
                            std::vector<LocalIndexType>&& scvIndices,
                            bool boundary = false)
    : corners_(geometryHelper.getScvfCorners(scvfIndex)),
      center_(0.0),
      unitOuterNormal_(geometryHelper.normal(corners_, scvIndices)),
      area_(geometryHelper.scvfArea(corners_)),
      scvfIndex_(scvfIndex),
      scvIndices_(std::move(scvIndices)),
      boundary_(boundary)
    {
        for (const auto& corner : corners_)
            center_ += corner;
        center_ /= corners_.size();
    }

    //! Constructor for boundary scvfs
    template<class GeometryHelper, class Intersection>
    BoxSubControlVolumeFace(const GeometryHelper& geometryHelper,
                            const Intersection& intersection,
                            const typename Intersection::Geometry& isGeometry,
                            LocalIndexType indexInIntersection,
                            GridIndexType scvfIndex,
                            std::vector<LocalIndexType>&& scvIndices,
                            bool boundary = false)
    : corners_(geometryHelper.getBoundaryScvfCorners(isGeometry, indexInIntersection)),
      center_(0.0),
      unitOuterNormal_(intersection.centerUnitOuterNormal()),
      area_(geometryHelper.scvfArea(corners_)),
      scvfIndex_(scvfIndex),
      scvIndices_(std:: move(scvIndices)),
      boundary_(boundary)
    {
        for (const auto& corner : corners_)
            center_ += corner;
        center_ /= corners_.size();
    }

    //! The center of the sub control volume face
    const GlobalPosition& center() const
    {
        return center_;
    }

    //! The integration point for flux evaluations in global coordinates
    const GlobalPosition& ipGlobal() const
    {
        return center_;
    }

    //! The area of the sub control volume face
    Scalar area() const
    {
        return area_;
    }

    //! returns bolean if the sub control volume face is on the boundary
    bool boundary() const
    {
        return boundary_;
    }

    const GlobalPosition& unitOuterNormal() const
    {
        return unitOuterNormal_;
    }

    //! index of the inside sub control volume for spatial param evaluation
    LocalIndexType insideScvIdx() const
    {
        return scvIndices_[0];
    }

    //! index of the outside sub control volume for spatial param evaluation
    // This results in undefined behaviour if boundary is true
    LocalIndexType outsideScvIdx() const
    {
        assert(!boundary());
        return scvIndices_[1];
    }

    //! The global index of this sub control volume face
    GridIndexType index() const
    {
        return scvfIndex_;
    }

    const GlobalPosition& corner(unsigned int localIdx) const
    {
        assert(localIdx < corners_.size() && "provided index exceeds the number of corners");
        return corners_[localIdx];
    }

    //! The geometry of the sub control volume face
    Geometry geometry() const
    {
#if DUNE_VERSION_NEWER(DUNE_COMMON,2,6)
        return Geometry(Dune::GeometryTypes::cube(Geometry::mydimension), corners_);
#else
        return Geometry(Dune::GeometryType(Dune::GeometryType::cube, Geometry::mydimension), corners_);
#endif
    }

private:
    CornerStorage corners_;
    GlobalPosition center_;
    GlobalPosition unitOuterNormal_;
    Scalar area_;
    GridIndexType scvfIndex_;
    std::vector<LocalIndexType> scvIndices_;
    bool boundary_;
};

} // end namespace Dumux

#endif
