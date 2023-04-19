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
 * \ingroup NavierStokesModel
 *
 * \copydoc Dumux::NavierStokesVolumeVariables
 */
#ifndef DUMUX_NAVIERSTOKES_MOMENTUM_DIAMOND_VOLUME_VARIABLES_HH
#define DUMUX_NAVIERSTOKES_MOMENTUM_DIAMOND_VOLUME_VARIABLES_HH

#warning "This header is deprecated and will be removed after 3.7"

#include <dumux/freeflow/navierstokes/momentum/cvfe/volumevariables.hh>

namespace Dumux {

template<class TypeTag>
using NavierStokesMomentumDiamondVolumeVariables [[deprecated("Use NavierStokesMomentumCVFEVolumeVariables")]] = NavierStokesMomentumCVFEVolumeVariables<TypeTag>;

} // end namespace Dumux

#endif
