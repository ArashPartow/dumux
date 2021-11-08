// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
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
/**
 * \file
 * \ingroup OnePNCTests
 * \brief Definition of a problem for the 1pnc problem:
 * Component transport of nitrogen dissolved in the water phase.
 */

#ifndef DUMUX_1P2C_TEST_PROBLEM_PROPERTIES_HH
#define DUMUX_1P2C_TEST_PROBLEM_PROPERTIES_HH

#if HAVE_DUNE_UGGRID
#include <dune/grid/uggrid.hh>
#endif
#include <dune/grid/yaspgrid.hh>

#include <dumux/common/math.hh>

#include <dumux/discretization/cctpfa.hh>
#include <dumux/discretization/ccmpfa.hh>
#include <dumux/discretization/box.hh>
#include <dumux/discretization/evalsolution.hh>
#include <dumux/discretization/evalgradients.hh>
#include <dumux/porousmediumflow/1pnc/model.hh>
#include <dumux/material/fluidmatrixinteractions/dispersiontensors/fulltensor.hh>
#include <dumux/material/fluidmatrixinteractions/dispersiontensors/scheidegger.hh>

#include <dumux/material/fluidsystems/h2on2.hh>
#include <dumux/material/fluidsystems/1padapter.hh>

#include "problem.hh"
#include "spatialparams.hh"

namespace Dumux::Properties {

// Create new type tags
namespace TTag {
struct DispersionTest { using InheritsFrom = std::tuple<BoxModel>; };
struct OnePNCDispersionTestBox { using InheritsFrom = std::tuple<DispersionTest, OnePNC>; };
} // end namespace TTag

// Set the grid type
#if HAVE_DUNE_UGGRID
template<class TypeTag>
struct Grid<TypeTag, TTag::DispersionTest> { using type = Dune::UGGrid<2>; };
#else
template<class TypeTag>
struct Grid<TypeTag, TTag::DispersionTest> { using type = Dune::YaspGrid<2>; };
#endif

// Set the problem property
template<class TypeTag>
struct Problem<TypeTag, TTag::DispersionTest> { using type = OnePNCDispersionProblem<TypeTag>; };

// Set fluid configuration
template<class TypeTag>
struct FluidSystem<TypeTag, TTag::DispersionTest>
{
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using H2ON2 = FluidSystems::H2ON2<Scalar, FluidSystems::H2ON2DefaultPolicy</*simplified=*/true>>;
    using type = FluidSystems::OnePAdapter<H2ON2, H2ON2::liquidPhaseIdx>;
};

// Set the spatial parameters
template<class TypeTag>
struct SpatialParams<TypeTag, TTag::DispersionTest>
{
    using GridGeometry = GetPropType<TypeTag, Properties::GridGeometry>;
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using type = OnePNCTestSpatialParams<GridGeometry, Scalar>;
};

// Define whether mole(true) or mass (false) fractions are used
template<class TypeTag>
struct UseMoles<TypeTag, TTag::DispersionTest> { static constexpr bool value = true; };

template<class TypeTag>
struct EnableCompositionalDispersion<TypeTag, TTag::DispersionTest> { static constexpr bool value = true; };

template<class TypeTag>

template<class TypeTag>
struct DispersionFluxType<TypeTag, TTag::DispersionTest> { using type = OnePDiffusiveDispersionFlux<TypeTag>; };

template<class TypeTag>
struct DispersionTensorType<TypeTag, TTag::DispersionTest>
{
#if SCHEIDEGGER
    using type = ScheideggersDispersionTensor<TypeTag>;
#else
    using type = FullDispersionTensor<TypeTag>;
#endif
};

} // end namespace Dumux::Properties

#endif
