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
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/

/*!
 * \file
 *
 * \brief Defines the indices required for the two-phase box model.
 */
#ifndef DUMUX_BOX_2P_INDICES_HH
#define DUMUX_BOX_2P_INDICES_HH

#include "2pproperties.hh"

namespace Dumux
{
// \{

/*!
 * \ingroup TwoPBoxModel
 * \ingroup BoxIndices
 * \brief The common indices for the isothermal two-phase model.
 */

struct TwoPFormulation
{
    static const int pwSn = 0; //!< Pw and Sn as primary variables
    static const int pnSw = 1; //!< Pn and Sw as primary variables
};

template <class TypeTag>
struct TwoPCommonIndices
{
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;

    // Phase indices
    static const int wPhaseIdx = FluidSystem::wPhaseIdx; //!< Index of the wetting phase
    static const int nPhaseIdx = FluidSystem::nPhaseIdx; //!< Index of the non-wetting phase
};

/*!
 * \brief The indices for the \f$p_w-S_n\f$ formulation of the
 *        isothermal two-phase model.
 *
 * \tparam TypeTag The problem type tag
 * \tparam formulation The formulation, either pwSn or pnSw
 * \tparam PVOffset The first index in a primary variable vector.
 */
template <class TypeTag, 
          int formulation = TwoPFormulation::pwSn, 
          int PVOffset = 0>
struct TwoPIndices 
: public TwoPCommonIndices<TypeTag>, TwoPFormulation
{
    // Primary variable indices
    static const int pressureIdx = PVOffset + 0; //!< Index for wetting/non-wetting phase pressure (depending on formulation) in a solution vector
    static const int saturationIdx = PVOffset + 1; //!< Index of the saturation of the non-wetting/wetting phase

    // indices of the primary variables
    static const int pwIdx = PVOffset + 0; //!< Pressure index of the wetting phase
    static const int SnIdx = PVOffset + 1; //!< Saturation index of the wetting phase

    // indices of the equations
    static const int contiWEqIdx = PVOffset + 0; //!< Index of the continuity equation of the wetting phase
    static const int contiNEqIdx = PVOffset + 1; //!< Index of the continuity equation of the non-wetting phase
};

/*!
 * \brief The indices for the \f$p_w-S_n\f$ formulation of the
 *        isothermal two-phase model.
 *
 * \tparam PVOffset The first index in a primary variable vector.
 */
template <class TypeTag, int PVOffset>
struct TwoPIndices<TypeTag, TwoPFormulation::pnSw, PVOffset>
: public TwoPCommonIndices<TypeTag>, TwoPFormulation
{
    // Primary variable indices
    static const int pressureIdx = PVOffset + 0; //!< Index for wetting/non-wetting phase pressure (depending on formulation) in a solution vector
    static const int saturationIdx = PVOffset + 1; //!< Index of the saturation of the non-wetting/wetting phase

    // indices of the primary variables
    static const int pnIdx = PVOffset + 0; //!< Pressure index of the wetting phase
    static const int SwIdx = PVOffset + 1; //!< Saturation index of the wetting phase

    // indices of the equations
    static const int contiNEqIdx = PVOffset + 0; //!< Index of the continuity equation of the non-wetting phase
    static const int contiWEqIdx = PVOffset + 1; //!< Index of the continuity equation of the wetting phase
};

// \}
} // namespace Dumux


#endif
