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
 * \brief The primary variable and equation indices for the MpNc model.
 */
#ifndef DUMUX_MPNC_INDICES_HH
#define DUMUX_MPNC_INDICES_HH

#include <dumux/common/properties.hh>

namespace Dumux
{

/*!
 * \ingroup MPNCModel
 * \ingroup ImplicitIndices
 * \brief Enumerates the formulations which the MpNc model accepts.
 */
struct MpNcPressureFormulation
{
    enum {
        mostWettingFirst,
        leastWettingFirst
    };
};

/*!
 * \ingroup MPNCModel
 * \ingroup ImplicitIndices
 * \brief The primary variable and equation indices for the MpNc model.
 */
template <class TypeTag, int BasePVOffset = 0>
class MPNCIndices
{
     using FluidSystem  = typename GET_PROP_TYPE(TypeTag, FluidSystem);
     enum { numPhases = FluidSystem::numPhases };
     static const int numEqBalance = GET_PROP_VALUE(TypeTag, NumEqBalance);
public:

        // Phase indices
    static const int wPhaseIdx = FluidSystem::wPhaseIdx; //!< Index of the wetting phase
    static const int nPhaseIdx = FluidSystem::nPhaseIdx; //!< Index of the non-wetting phase
    /*!
     * \brief The number of primary variables / equations.
     */
    // temperature + Mass Balance  + constraints for switch stuff
    static const unsigned int numPrimaryVars = numEqBalance ;

    /*!
     * \brief Index of the saturation of the first phase in a vector
     *        of primary variables.
     *
     * The following (numPhases - 1) primary variables represent the
     * saturations for the phases [1, ..., numPhases - 1]
     */
    static const unsigned int s0Idx = numEqBalance - numPhases;

    /*!
     * \brief Index of the first phase' pressure in a vector of
     *        primary variables.
     */
    static const unsigned int p0Idx = numEqBalance  - 1;

    /*!
     * \brief Index of the first phase NCP equation.
     *
     * The index for the remaining phases are consecutive.
     */
    static const unsigned int phase0NcpIdx =  numEqBalance - numPhases;

    static const unsigned int fug0Idx = BasePVOffset + 0;
    static const unsigned int conti0EqIdx = BasePVOffset + 0;
    static const unsigned int moleFrac00Idx = BasePVOffset + 0;
};

}

#endif
