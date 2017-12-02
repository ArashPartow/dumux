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
 * \brief Base class for a sub control volume
 */
#ifndef DUMUX_DISCRETIZATION_CC_SUBCONTROLVOLUME_HH
#define DUMUX_DISCRETIZATION_CC_SUBCONTROLVOLUME_HH

#include <dumux/discretization/subcontrolvolumebase.hh>
#include <dune/common/std/optional.hh>

namespace Dumux
{
template<class ScvGeometryTraits>
class CCSubControlVolume : public SubControlVolumeBase<CCSubControlVolume<ScvGeometryTraits>, ScvGeometryTraits>
{
    using ParentType = SubControlVolumeBase<CCSubControlVolume<ScvGeometryTraits>, ScvGeometryTraits>;
    using Geometry = typename ScvGeometryTraits::Geometry;
    using GridIndexType = typename ScvGeometryTraits::GridIndexType;
    using LocalIndexType = typename ScvGeometryTraits::LocalIndexType;
    using Scalar = typename ScvGeometryTraits::Scalar;
    using GlobalPosition = typename ScvGeometryTraits::GlobalPosition;

public:
    //! state the traits public and thus export all types
    using Traits = ScvGeometryTraits;

    // the default constructor
    CCSubControlVolume() = default;

    // the contructor in the cc case
    CCSubControlVolume(Geometry&& geometry,
                       GridIndexType elementIndex)
    : ParentType()
    , geometry_(std::move(geometry))
    , center_(geometry_.value().center())
    , elementIndex_(elementIndex)
    {}

    //! The copy constrcutor
    CCSubControlVolume(const CCSubControlVolume& other) = default;

    //! The move constrcutor
    CCSubControlVolume(CCSubControlVolume&& other) = default;

    //! The copy assignment operator
    CCSubControlVolume& operator=(const CCSubControlVolume& other)
    {
        // We want to use the default copy/move assignment.
        // But since geometry is not copy assignable :( we
        // have to construct it again
        geometry_.reset();
        geometry_.emplace(other.geometry_.value());
        center_ = other.center_;
        elementIndex_ = other.elementIndex_;
        return *this;
    }

    //! The move assignment operator
    CCSubControlVolume& operator=(CCSubControlVolume&& other) noexcept
    {
        // We want to use the default copy/move assignment.
        // But since geometry is not copy assignable :( we
        // have to construct it again
        geometry_.reset();
        geometry_.emplace(std::move(other.geometry_.value()));
        center_ = std::move(other.center_);
        elementIndex_ = std::move(other.elementIndex_);
        return *this;
    }

    //! The center of the sub control volume
    const GlobalPosition& center() const
    {
        return center_;
    }

    //! The volume of the sub control volume
    Scalar volume() const
    {
        return geometry().volume();
    }

    //! The geometry of the sub control volume
    // e.g. for integration
    const Geometry& geometry() const
    {
        assert((geometry_));
        return geometry_.value();
    }

    //! The index of the dof this scv is embedded in (the global index of this scv)
    GridIndexType dofIndex() const
    {
        return elementIndex();
    }

    //! The global index of this scv
    LocalIndexType indexInElement() const
    {
        return 0;
    }

    // The position of the dof this scv is embedded in
    const GlobalPosition& dofPosition() const
    {
        return center_;
    }

    //! The global index of the element this scv is embedded in
    GridIndexType elementIndex() const
    {
        return elementIndex_;
    }

    //! Return the corner for the given local index
    GlobalPosition corner(LocalIndexType localIdx) const
    {
        assert(localIdx < geometry().corners() && "provided index exceeds the number of corners");
        return geometry().corner(localIdx);
    }

private:
    // Work around the fact that geometry is not default constructible
    Dune::Std::optional<Geometry> geometry_;
    GlobalPosition center_;
    GridIndexType elementIndex_;
};

} // end namespace Dumux

#endif
