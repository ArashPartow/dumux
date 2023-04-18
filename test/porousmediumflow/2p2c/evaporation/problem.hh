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
 * \ingroup TwoPTwoCTests
 * \brief Evaporation problem where two components with constant properties mix and evaporate.
 */

#ifndef DUMUX_EVAPORATION_CONSTANT_COMPONENT_PROBLEM_HH
#define DUMUX_EVAPORATION_CONSTANT_COMPONENT_PROBLEM_HH

#include <dumux/common/properties.hh>
#include <dumux/common/parameters.hh>
#include <dumux/common/boundarytypes.hh>
#include <dumux/common/numeqvector.hh>

#include <dumux/porousmediumflow/problem.hh>

namespace Dumux {

/*!
 * \ingroup TwoPTwoCModel
 * \brief Evaporation problem where two components with constant properties mix and evaporate.
 *
 */
template <class TypeTag >
class EvaporationConstantComponentProblem : public PorousMediumFlowProblem<TypeTag>
{
    using ParentType = PorousMediumFlowProblem<TypeTag>;

    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using GridGeometry = GetPropType<TypeTag, Properties::GridGeometry>;
    using GridView = typename GridGeometry::GridView;
    using GridVariables = GetPropType<TypeTag, Properties::GridVariables>;
    using ElementVolumeVariables = typename GridVariables::GridVolumeVariables::LocalView;
    using ElementFluxVariablesCache = typename GridVariables::GridFluxVariablesCache::LocalView;
    using FVElementGeometry = typename GetPropType<TypeTag, Properties::GridGeometry>::LocalView;
    using SubControlVolumeFace = typename FVElementGeometry::SubControlVolumeFace;
    using FluidSystem = GetPropType<TypeTag, Properties::FluidSystem>;
    using ModelTraits = GetPropType<TypeTag, Properties::ModelTraits>;
    using Indices = typename ModelTraits::Indices;

    // primary variable indices
    enum
    {
        pressureIdx = Indices::pressureIdx,
        switchIdx = Indices::switchIdx,
        temperatureIdx = Indices::temperatureIdx,
        energyEqIdx = Indices::energyEqIdx
    };

    // equation indices
    enum
    {
        contiH2OEqIdx = Indices::conti0EqIdx + FluidSystem::comp0Idx,
        contiN2EqIdx = Indices::conti0EqIdx + FluidSystem::comp1Idx
    };

    using PrimaryVariables = GetPropType<TypeTag, Properties::PrimaryVariables>;
    using NumEqVector = Dumux::NumEqVector<PrimaryVariables>;
    using BoundaryTypes = Dumux::BoundaryTypes<GetPropType<TypeTag, Properties::ModelTraits>::numEq()>;
    using Element = typename GridView::template Codim<0>::Entity;
    using GlobalPosition = typename Element::Geometry::GlobalCoordinate;

    //! Property that defines whether mole or mass fractions are used
    static constexpr bool useMoles = ModelTraits::useMoles();

public:
    EvaporationConstantComponentProblem(std::shared_ptr<const GridGeometry> gridGeometry)
    : ParentType(gridGeometry)
    {
        FluidSystem::init();

        name_ = getParam<std::string>("Problem.Name");

        //stating in the console whether mole or mass fractions are used
        if(useMoles)
            std::cout << "The problem uses mole-fractions" << std::endl;
        else
            std::cout << "The problem uses mass-fractions" << std::endl;
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
     *        used for which equation on a given boundary segment.
     *
     * \param globalPos The position for which the bc type should be evaluated
     */
    BoundaryTypes boundaryTypesAtPos(const GlobalPosition &globalPos) const
    {
        BoundaryTypes bcTypes;
        if(globalPos[0] < eps_)
            bcTypes.setAllDirichlet();
        else
            bcTypes.setAllNeumann();

        return bcTypes;
    }

    /*!
     * \brief Evaluates the boundary conditions for a Neumann boundary segment.
     *
     * \param element The finite element
     * \param fvGeometry The finite volume geometry of the element
     * \param elemVolVars The element volume variables
     * \param elemFluxVarsCache Flux variables caches for all faces in stencil
     * \param scvf The sub-control volume face
     *
     * Negative values mean influx.
     */
    NumEqVector neumann(const Element& element,
                        const FVElementGeometry& fvGeometry,
                        const ElementVolumeVariables& elemVolVars,
                        const ElementFluxVariablesCache& elemFluxVarsCache,
                        const SubControlVolumeFace& scvf) const
    {
        NumEqVector values(0.0);
        const auto& globalPos = scvf.ipGlobal();
        const auto& volVars = elemVolVars[scvf.insideScvIdx()];
        Scalar boundaryLayerThickness = 0.0016;
        //right side
        if (globalPos[0] > this->gridGeometry().bBoxMax()[0] - eps_)
        {
            Scalar massFracInside = volVars.massFraction(FluidSystem::phase1Idx, FluidSystem::comp0Idx);
            Scalar massFracRef = 0.0;
            Scalar evaporationRate = volVars.diffusionCoefficient(FluidSystem::phase1Idx, FluidSystem::comp1Idx, FluidSystem::comp0Idx)
                                     * (massFracInside - massFracRef)
                                     / boundaryLayerThickness
                                     * volVars.density(FluidSystem::phase1Idx);
            values[Indices::conti0EqIdx] = evaporationRate;
            values[Indices::conti0EqIdx+1] = -evaporationRate;

            values[Indices::energyEqIdx] = FluidSystem::enthalpy(volVars.fluidState(), FluidSystem::phase1Idx) * evaporationRate;
            values[Indices::energyEqIdx] += FluidSystem::thermalConductivity(volVars.fluidState(), FluidSystem::phase1Idx)
                                            * (volVars.temperature() - 293.15)/boundaryLayerThickness;
        }
        return values;
    }

    /*!
     * \brief Evaluates the boundary conditions for a Dirichlet boundary segment.
     *
     * \param globalPos The position for which the Dirichlet condition should be evaluated
     */
    PrimaryVariables dirichletAtPos(const GlobalPosition &globalPos) const
    {
        PrimaryVariables priVars(0.0);
        priVars.setState(Indices::bothPhases);
        priVars[pressureIdx] = 1.e5;
        priVars[switchIdx] = 0.6; //sn
        priVars[temperatureIdx] = 298.15;

        return priVars;

    }



    /*!
     * \brief Evaluates the initial value for a control volume.
     *
     * \param globalPos The position for which the initial condition should be evaluated
     *
     * For this method, the \a values parameter stores primary
     * variables.
     */
    PrimaryVariables initialAtPos(const GlobalPosition &globalPos) const
    {
        PrimaryVariables priVars(0.0);
        priVars.setState(Indices::bothPhases);
        priVars[pressureIdx] = 1e5;
        priVars[switchIdx] = 0.6; //sn
        priVars[temperatureIdx] = 293.15;
        return priVars;
    }

private:

    static constexpr Scalar eps_ = 1e-2;
    std::string name_;
};

} // end namespace Dumux

#endif
