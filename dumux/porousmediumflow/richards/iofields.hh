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
 * \ingroup RichardsModel
 * \brief Adds I/O fields specific to the Richards model.
 */

#ifndef DUMUX_RICHARDS_IO_FIELDS_HH
#define DUMUX_RICHARDS_IO_FIELDS_HH

#include <dumux/common/parameters.hh>
#include <dumux/common/typetraits/state.hh>
#include <dumux/io/name.hh>

namespace Dumux {

/*!
 * \ingroup RichardsModel
 * \brief Adds I/O fields specific to the Richards model.
 */
class RichardsIOFields
{
public:
    template <class OutputModule>
    static void initOutputModule(OutputModule& out)
    {
        using VV = typename OutputModule::VolumeVariables;
        using FS = typename VV::FluidSystem;

        out.addVolumeVariable([](const auto& v){ return v.saturation(FS::phase0Idx); },
                              IOName::saturation<FS>(FS::phase0Idx));
        out.addVolumeVariable([](const auto& v){ return v.saturation(FS::phase1Idx); },
                              IOName::saturation<FS>(FS::phase1Idx));
        out.addVolumeVariable([](const auto& v){ return v.pressure(FS::phase0Idx); },
                              IOName::pressure<FS>(FS::phase0Idx));
        out.addVolumeVariable([](const auto& v){ return v.pressure(FS::phase1Idx); },
                              IOName::pressure<FS>(FS::phase1Idx));
        out.addVolumeVariable([](const auto& v){ return v.capillaryPressure(); },
                              IOName::capillaryPressure());
        out.addVolumeVariable([](const auto& v){ return v.density(FS::phase0Idx); },
                              IOName::density<FS>(FS::phase0Idx));
        out.addVolumeVariable([](const auto& v){ return v.mobility(FS::phase0Idx); },
                              IOName::mobility<FS>(FS::phase0Idx));
        out.addVolumeVariable([](const auto& v){ return v.relativePermeability(FS::phase0Idx); },
                              IOName::relativePermeability<FS>(FS::phase0Idx));
        out.addVolumeVariable([](const auto& v){ return v.porosity(); },
                              IOName::porosity());

        static const bool gravity = getParamFromGroup<bool>(out.paramGroup(), "Problem.EnableGravity");

        if(gravity)
            out.addVolumeVariable([](const auto& v){ return v.pressureHead(FS::phase0Idx); },
                                  IOName::pressureHead());
        out.addVolumeVariable([](const auto& v){ return v.waterContent(FS::phase0Idx); },
                              IOName::waterContent());
    }

    template<class ModelTraits, class FluidSystem, class SolidSystem = void>
    static std::string primaryVariableName(int pvIdx, int state)
    {
        return IOName::pressure<FluidSystem>(FluidSystem::phase0Idx);
    }
};

} // end namespace Dumux

#endif
