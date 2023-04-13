// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
//
// SPDX-FileCopyrightInfo: Copyright © DuMux Project contributors, see AUTHORS.md in root folder
// SPDX-License-Identifier: GPL-3.0-or-later
//
/*!
 * \file
 * \ingroup PNMOnePNCModel
 * \copydoc Dumux::PoreNetwork::OnePNCIOFields
 */
#ifndef DUMUX_PNM_ONEP_NC_IO_FIELDS_HH
#define DUMUX_PNM_ONEP_NC_IO_FIELDS_HH

#include <dumux/porousmediumflow/1pnc/iofields.hh>
#include <dumux/porenetwork/common/iofields.hh>

namespace Dumux::PoreNetwork {

/*!
 * \ingroup PNMOnePNCModel
 * \brief Adds output fields specific to the PNM 1pnc model
 */
template<class FluidSystem>
class OnePNCIOFields
{
public:
    template<class OutputModule>
    static void initOutputModule(OutputModule& out)
    {
        Dumux::OnePNCIOFields::initOutputModule(out);
        CommonIOFields::initOutputModule(out);

        out.addFluxVariable([](const auto& fluxVars, const auto& fluxVarsCache)
                              { return fluxVarsCache.transmissibility(0); }, "transmissibility");

        auto volumeFlux = [](const auto& fluxVars, const auto& fluxVarsCache)
        {
            auto upwindTerm = [](const auto& volVars) { return volVars.mobility(0); };
            using std::abs;
            return abs(fluxVars.advectiveFlux(0, upwindTerm));
        };
        out.addFluxVariable(volumeFlux, "volumeFlux");
    }
};

} // end namespace Dumux::PoreNetwork

#endif
