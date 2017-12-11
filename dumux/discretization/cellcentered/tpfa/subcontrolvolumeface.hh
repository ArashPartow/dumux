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
#ifndef DUMUX_DISCRETIZATION_CC_TPFA_SUBCONTROLVOLUMEFACE_HH
#define DUMUX_DISCRETIZATION_CC_TPFA_SUBCONTROLVOLUMEFACE_HH

#include <utility>
#include <dune/geometry/type.hh>
#include <dumux/discretization/subcontrolvolumefacebase.hh>

namespace Dumux
{

/*!
 * \ingroup Discretization
 * \brief Class for a sub control volume face in the box method, i.e a part of the boundary
 *        of a sub control volume we compute fluxes on. We simply use the base class here.
 */
template<class ScvfGeometryTraits>
class CCTpfaSubControlVolumeFace : public SubControlVolumeFaceBase<CCTpfaSubControlVolumeFace<ScvfGeometryTraits>,ScvfGeometryTraits>
{
    using ParentType = SubControlVolumeFaceBase<CCTpfaSubControlVolumeFace<ScvfGeometryTraits>, ScvfGeometryTraits>;
    using GridIndexType = typename ScvfGeometryTraits::GridIndexType;
    using Scalar = typename ScvfGeometryTraits::Scalar;
    using GlobalPosition = typename ScvfGeometryTraits::GlobalPosition;
    using CornerStorage = typename ScvfGeometryTraits::CornerStorage;
    using Geometry = typename ScvfGeometryTraits::Geometry;

public:
    //! state the traits public and thus export all types
    using Traits = ScvfGeometryTraits;

    // the default constructor
    CCTpfaSubControlVolumeFace() = default;

    /*!
     * \brief Constructor with intersection
     *
     * \param is The intersection
     * \param isGeometry The geometry of the intersection
     * \param scvfIndex The global index of this scv face
     * \param scvIndices The inside/outside scv indices connected to this face
     * \param isBoundary Bool to specify whether or not the scvf is on an interior or the domain boundary
     */
    template <class Intersection>
    CCTpfaSubControlVolumeFace(const Intersection& is,
                               const typename Intersection::Geometry& isGeometry,
                               GridIndexType scvfIndex,
                               const std::vector<GridIndexType>& scvIndices,
                               bool isBoundary)
    : ParentType()
    , geomType_(isGeometry.type())
    , area_(isGeometry.volume())
    , center_(isGeometry.center())
    , unitOuterNormal_(is.centerUnitOuterNormal())
    , scvfIndex_(scvfIndex)
    , scvIndices_(scvIndices)
    , boundary_(isBoundary)
    {
        corners_.resize(isGeometry.corners());
        for (int i = 0; i < isGeometry.corners(); ++i)
            corners_[i] = isGeometry.corner(i);
    }

    //! The center of the sub control volume face
    const GlobalPosition& center() const
    {
        return center_;
    }

    //! The integration point for flux evaluations in global coordinates
    const GlobalPosition& ipGlobal() const
    {
        // Return center for now
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

    //! The unit outer normal of the sub control volume face
    const GlobalPosition& unitOuterNormal() const
    {
        return unitOuterNormal_;
    }

    //! index of the inside sub control volume for spatial param evaluation
    GridIndexType insideScvIdx() const
    {
        return scvIndices_[0];
    }

    //! index of the outside sub control volume for spatial param evaluation
    // This results in undefined behaviour if boundary is true
    GridIndexType outsideScvIdx(int i = 0) const
    {
        return scvIndices_[i+1];
    }

    //! The number of outside scvs connection via this scv face
    std::size_t numOutsideScvs() const
    {
        return scvIndices_.size()-1;
    }

    //! The global index of this sub control volume face
    GridIndexType index() const
    {
        return scvfIndex_;
    }

    //! return the i-th corner of this sub control volume face
    const GlobalPosition& corner(int i) const
    {
        assert(i < corners_.size() && "provided index exceeds the number of corners");
        return corners_[i];
    }

    //! The geometry of the sub control volume face
    Geometry geometry() const
    {
        return Geometry(geomType_, corners_);
    }

private:
    Dune::GeometryType geomType_;
    CornerStorage corners_;
    Scalar area_;
    GlobalPosition center_;
    GlobalPosition unitOuterNormal_;
    GridIndexType scvfIndex_;
    std::vector<GridIndexType> scvIndices_;
    bool boundary_;
};

} // end namespace

#endif
