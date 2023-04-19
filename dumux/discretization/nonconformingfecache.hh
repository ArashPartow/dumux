// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
//
// SPDX-FileCopyrightInfo: Copyright © DuMux Project contributors, see AUTHORS.md in root folder
// SPDX-License-Identifier: GPL-3.0-or-later
//
/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 3 of the License, or       *
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
 * \brief A finite element cache for the non-conforming FE spaces RT and CR
 */
#ifndef DUMUX_DISCRETIZATION_NONCONFORMING_FECACHE_HH
#define DUMUX_DISCRETIZATION_NONCONFORMING_FECACHE_HH

#include <memory>

#include <dune/common/exceptions.hh>
#include <dune/geometry/type.hh>

#include <dune/localfunctions/crouzeixraviart.hh>
#include <dune/localfunctions/rannacherturek.hh>
#include <dune/localfunctions/common/virtualinterface.hh>
#include <dune/localfunctions/common/virtualwrappers.hh>

namespace Dumux {

template< class CoordScalar, class Scalar, unsigned int dim>
class NonconformingFECache
{
    static_assert(dim == 2 || dim == 3, "Non-conforming FE spaces only implemented for 2D and 3D grids");

    // These are so-called non-conforming finite element spaces
    // the local basis is only continuous at given points on the faces
    using RT = Dune::RannacherTurekLocalFiniteElement<CoordScalar, Scalar, dim>;
    using CR = Dune::CrouzeixRaviartLocalFiniteElement<CoordScalar, Scalar, dim>;

public:
    using FiniteElementType = Dune::LocalFiniteElementVirtualInterface<typename RT::Traits::LocalBasisType::Traits>;

    NonconformingFECache()
    : rtBasis_(std::make_unique<Dune::LocalFiniteElementVirtualImp<RT>>(RT{}))
    , crBasis_(std::make_unique<Dune::LocalFiniteElementVirtualImp<CR>>(CR{}))
    {}

    //! Get local finite element for given GeometryType
    const FiniteElementType& get(const Dune::GeometryType& gt) const
    {
        if (gt.isSimplex())
            return *crBasis_;
        else if (gt.isCube())
            return *rtBasis_;
        else
            DUNE_THROW(Dune::NotImplemented,
                "Non-conforming local finite element for geometry type " << gt
            );
    }

private:
    std::unique_ptr<FiniteElementType> rtBasis_;
    std::unique_ptr<FiniteElementType> crBasis_;
};

} // end namespace Dumux

#endif
