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
/*!
 * \file
 * \ingroup TwoPNCTests
 * \brief Definition of a problem for a surfactant model.
 */
#ifndef DUMUX_TEST_2P3C_SURFACTANT_PROBLEM_HH
#define DUMUX_TEST_2P3C_SURFACTANT_PROBLEM_HH

#include <dumux/common/properties.hh>
#include <dumux/common/parameters.hh>
#include <dumux/common/boundarytypes.hh>
#include <dumux/common/numeqvector.hh>

#include <dumux/porousmediumflow/problem.hh>
#include <dumux/porousmediumflow/2p/formulation.hh>

namespace Dumux {

/*!
 * \ingroup TwoPNCTests
 * \brief Problem for a surfactant model.
 *
 */

template <class TypeTag>
class TestSurfactantProblem
: public PorousMediumFlowProblem<TypeTag>
{
    using ParentType = PorousMediumFlowProblem<TypeTag>;
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using FluidSystem = GetPropType<TypeTag, Properties::FluidSystem>;

    using GridGeometry = GetPropType<TypeTag, Properties::GridGeometry>;
    using GridView = typename GridGeometry::GridView;
    using FVElementGeometry = typename GridGeometry::LocalView;
    using SubControlVolume = typename GridGeometry::SubControlVolume;
    using SubControlVolumeFace = typename GridGeometry::SubControlVolumeFace;

    static constexpr int dim = GridView::dimension;

    using PrimaryVariables = GetPropType<TypeTag, Properties::PrimaryVariables>;
    using NumEqVector = Dumux::NumEqVector<PrimaryVariables>;
    using BoundaryTypes = Dumux::BoundaryTypes<GetPropType<TypeTag, Properties::ModelTraits>::numEq()>;
    using Element = typename GridView::Traits::template Codim<0>::Entity;
    using GlobalPosition = typename Element::Geometry::GlobalCoordinate;
    using FluidState = GetPropType<TypeTag, Properties::FluidState>;

public:
    TestSurfactantProblem(std::shared_ptr<const GridGeometry> gridGeometry)
    : ParentType(gridGeometry)
    {
        const auto maxSurfC = getParam<Scalar>("Problem.InjectionSurfactantConcentration");
        injectionFluidState_.setMoleFraction(0, FluidSystem::surfactantCompIdx, maxSurfC);
        this->spatialParams().setMaxSurfactantConcentration(maxSurfC);

        initialPressure_ = getParam<Scalar>("Problem.InitialPressure");
        initialSw_ = getParam<Scalar>("Problem.InitialSw");
        productionWellPressure_ = getParam<Scalar>("Problem.ProductionWellPressure");
        injectionWellPressure_ = getParam<Scalar>("Problem.InjectionWellPressure");

        name_ = getParam<std::string>("Problem.Name");
    }

    /*!
     * \brief The problem name.
     *
     * This is used as a prefix for files generated by the simulation.
     */
    const std::string& name() const
    { return name_; }

    /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary segment
     */

    BoundaryTypes boundaryTypes(const Element& element, const SubControlVolume& scv) const
    {
        BoundaryTypes bcTypes;
        bcTypes.setAllNeumann();
        return bcTypes;
    }

    /*!
     * \brief Evaluates the boundary conditions for a Neumann boundary segment.
     */

    template<class ElementVolumeVariables, class ElementFluxVariablesCache>
    NumEqVector neumann(const Element& element,
                        const FVElementGeometry& fvGeometry,
                        const ElementVolumeVariables& elemVolVars,
                        const ElementFluxVariablesCache& elemFluxVarsCache,
                        const SubControlVolumeFace& scvf) const
    {
        NumEqVector values(0.0);

        const auto& volVars = elemVolVars[scvf.insideScvIdx()];
        const Scalar pw = volVars.pressure(0);
        const auto K = this->spatialParams().permeability(element);

        const auto middleX = 0.5*(this->gridGeometry().bBoxMin()[0] + this->gridGeometry().bBoxMax()[0]);
        if (scvf.ipGlobal()[0] < middleX)
        {
            using std::max;

            const Scalar dp0dn = max(0.0, (injectionWellPressure_ - volVars.pressure(0)));

            const Scalar injectionViscosity = FluidSystem::viscosity(injectionFluidState_, 0);

            // m3 / (m2 s) or kg / (m2 s)
            const auto flux = K * (1 / injectionViscosity) * dp0dn; // volume per second
            const auto moleflux = flux * FluidSystem::molarDensity(injectionFluidState_, 0);

            for (int j = 0; j < FluidSystem::numComponents; j++)
                values[j] = -moleflux * injectionFluidState_.moleFraction(0, j);

        }

        else
        {
            using std::min;
            const auto dp0dn = min(0.0, productionWellPressure_ - pw);
            const auto dp1dn = dp0dn;

            const Scalar fluxOilphase = K * volVars.mobility(1) * dp1dn; // volume per second
            const Scalar fluxWaterphase = K * volVars.mobility(0) * dp0dn; // volume per second

            values[FluidSystem::oilCompIdx] = -volVars.molarDensity(1) * fluxOilphase;
            values[FluidSystem::waterCompIdx] = -volVars.molarDensity(0) * fluxWaterphase;

            const Scalar f = volVars.moleFraction(0, FluidSystem::surfactantCompIdx);
            values[FluidSystem::surfactantCompIdx] = - volVars.molarDensity(0) * volVars.mobility(0) * K * dp0dn * f;
            values[FluidSystem::waterCompIdx] -= values[FluidSystem::surfactantCompIdx];
        }

        return values;
    }

    /*!
     * \brief Evaluates the initial values for a control volume.
     *
     * \param globalPos The global position
     */

    PrimaryVariables initialAtPos(const GlobalPosition& globalPos) const
    {
        using ModelTraits = GetPropType<TypeTag, Properties::ModelTraits>;
        static_assert(ModelTraits::priVarFormulation() == TwoPFormulation::p0s1, "must have p0s1 formulation");

        PrimaryVariables values(0);

        values[0] = initialPressure_;
        values[1] = 1.0 - initialSw_;
        values.setState(ModelTraits::Indices::bothPhases);
        values[FluidSystem::surfactantCompIdx] = 0;

        return values;
    }

private:
    Scalar productionWellPressure_;
    Scalar injectionWellPressure_;
    Scalar initialPressure_;
    Scalar initialSw_;

    FluidState injectionFluidState_;
    std::string name_;
};

} // end namespace Dumux

#endif