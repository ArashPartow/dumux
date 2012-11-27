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
 *
 * \brief Defines the indices used by the non-isotherm two-phase BOX model.
 */
#ifndef DUMUX_2PNI_INDICES_HH
#define DUMUX_2PNI_INDICES_HH

#include <dumux/implicit/2p/2pindices.hh>

namespace Dumux
{
// \{

/*!
 * \ingroup TwoPNIModel
 * \ingroup BoxIndices
 * \brief Enumerations for the non-isothermal two-phase model
 */
template <class TypeTag, int PVOffset = 0>
class TwoPNIIndices : public TwoPIndices<TypeTag, PVOffset>
{
public:
    static const int temperatureIdx = PVOffset + 2; //! The primary variable index for temperature
    static const int energyEqIdx = PVOffset + 2; //! The equation index of the energy equation
};
// \}
}

#endif
