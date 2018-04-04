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
 * \ingroup StaggeredDiscretization
 * \brief Traits class to be used in conjunction with the StaggeredGridFaceVariables.
 */
#ifndef DUMUX_DISCRETIZATION_STAGGERED_GRID_VARIABLES_TRAITS_HH
#define DUMUX_DISCRETIZATION_STAGGERED_GRID_VARIABLES_TRAITS_HH

#include <dumux/discretization/staggered/elementvolumevariables.hh>

namespace Dumux {

/*!
 * \ingroup StaggeredDiscretization
 * \brief Traits class to be used for the StaggeredGridVolumeVariables.
 *
 * \tparam P The problem type
 * \tparam VV The volume variables type
 * \tparam I The indices type
 * TODO: Remove the indices (get out of volvar) and do a default class like in tpfa
 */
template<class P, class VV, class I>
struct StaggeredGridVolumeVariablesTraits
{
    template<class GridVolumeVariables, bool enableCache>
    using LocalView = StaggeredElementVolumeVariables<GridVolumeVariables, enableCache>;

    using Problem = P;
    using VolumeVariables = VV;
    using Indices = I;
};

} // end namespace Dumux

#endif
