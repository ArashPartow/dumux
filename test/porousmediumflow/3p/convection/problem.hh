// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
//
// SPDX-FileCopyrightInfo: Copyright © DuMux Project contributors, see AUTHORS.md in root folder
// SPDX-License-Identifier: GPL-3.0-or-later
//
/**
 * \file
 * \ingroup ThreePTests
 * \brief Test for the ThreePModel in combination with the NI model for a convection problem.
 */

#ifndef DUMUX_3PNI_CONVECTION_PROBLEM_HH
#define DUMUX_3PNI_CONVECTION_PROBLEM_HH

#include <algorithm>
#include <cmath>

#include <dumux/common/boundarytypes.hh>
#include <dumux/common/parameters.hh>
#include <dumux/common/properties.hh>
#include <dumux/common/numeqvector.hh>

#include <dumux/porousmediumflow/problem.hh>
#include <dumux/material/components/h2o.hh>

namespace Dumux {

/*!
 * \ingroup ThreePTests
 *
 * \brief Test for the ThreePModel in combination with the NI model for a convection problem.
 *
 * The simulation domain is a tube where water with an elevated temperature is injected
 * at a constant rate on the left hand side.
 *
 * Initially the domain is fully saturated with water at a constant temperature.
 * On the left hand side water is injected at a constant rate and on the right
 * hand side a Dirichlet boundary with constant pressure, saturation and
 * temperature is applied.
 *
 * The results are compared to an analytical solution where a retarded front
 * velocity is calculated as follows:
  \f[
     v_{Front}=\frac{q S_{water}}{\phi S_{total}}
 \f]
 */
template <class TypeTag>
class ThreePNIConvectionProblem : public PorousMediumFlowProblem<TypeTag>
{
    using ParentType = PorousMediumFlowProblem<TypeTag>;

    using GridView = typename GetPropType<TypeTag, Properties::GridGeometry>::GridView;
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using FVElementGeometry = typename GetPropType<TypeTag, Properties::GridGeometry>::LocalView;
    using GridGeometry = GetPropType<TypeTag, Properties::GridGeometry>;
    using PrimaryVariables = GetPropType<TypeTag, Properties::PrimaryVariables>;
    using NumEqVector = Dumux::NumEqVector<PrimaryVariables>;
    using FluidSystem = GetPropType<TypeTag, Properties::FluidSystem>;
    using BoundaryTypes = Dumux::BoundaryTypes<GetPropType<TypeTag, Properties::ModelTraits>::numEq()>;

    using GridVariables = GetPropType<TypeTag, Properties::GridVariables>;
    using ElementVolumeVariables = typename GridVariables::GridVolumeVariables::LocalView;
    using ElementFluxVariablesCache = typename GridVariables::GridFluxVariablesCache::LocalView;
    using VolumeVariables = typename GridVariables::GridVolumeVariables::VolumeVariables;

    using SolutionVector = GetPropType<TypeTag, Properties::SolutionVector>;
    using SubControlVolumeFace = typename FVElementGeometry::SubControlVolumeFace;
    using IapwsH2O = Components::H2O<Scalar>;

    // copy some indices for convenience
    using Indices = typename GetPropType<TypeTag, Properties::ModelTraits>::Indices;
    enum {
        // index of the primary variables
        pressureIdx = Indices::pressureIdx,
        swIdx = Indices::swIdx,
        snIdx = Indices::snIdx,
        temperatureIdx = Indices::temperatureIdx,
        wPhaseIdx = FluidSystem::wPhaseIdx,
        conti0EqIdx = Indices::conti0EqIdx,
        energyEqIdx = Indices::energyEqIdx
    };

    enum { dimWorld = GridView::dimensionworld };

    using Element = typename GridView::template Codim<0>::Entity;
    using GlobalPosition = typename SubControlVolumeFace::GlobalPosition;

public:
    ThreePNIConvectionProblem(std::shared_ptr<const GridGeometry> gridGeometry)
    : ParentType(gridGeometry)
    {
        //initialize fluid system
        FluidSystem::init();

        name_ = getParam<std::string>("Problem.Name");
        outputInterval_ = getParam<int>("Problem.OutputInterval");
        darcyVelocity_ = getParam<Scalar>("Problem.DarcyVelocity");

        temperatureHigh_ = 291.;
        temperatureLow_ = 290.;
        pressureHigh_ = 2e5;
        pressureLow_ = 1e5;

        temperatureExact_.resize(this->gridGeometry().numDofs());
    }

    //! Get exact temperature vector for output
    const std::vector<Scalar>& getExactTemperature()
    {
        return temperatureExact_;
    }

    //! Update the analytical temperature
    void updateExactTemperature(const SolutionVector& curSol, Scalar time)
    {
        const auto someElement = *(elements(this->gridGeometry().gridView()).begin());

        const auto someElemSol = elementSolution(someElement, curSol, this->gridGeometry());
        const auto someInitSol = initialAtPos(someElement.geometry().center());

        const auto someFvGeometry = localView(this->gridGeometry()).bindElement(someElement);
        const auto someScv = *(scvs(someFvGeometry).begin());

        VolumeVariables volVars;
        volVars.update(someElemSol, *this, someElement, someScv);

        const auto porosity = this->spatialParams().porosity(someElement, someScv, someElemSol);
        const auto densityW = volVars.density(wPhaseIdx);
        const auto heatCapacityW = IapwsH2O::liquidHeatCapacity(someInitSol[temperatureIdx], someInitSol[pressureIdx]);
        const auto storageW =  densityW*heatCapacityW*porosity;
        const auto densityS = volVars.solidDensity();
        const auto heatCapacityS = volVars.solidHeatCapacity();
        const auto storageTotal = storageW + densityS*heatCapacityS*(1 - porosity);
        std::cout << "storage: " << storageTotal << '\n';

        using std::max;
        time = max(time, 1e-10);
        const Scalar retardedFrontVelocity = darcyVelocity_*storageW/storageTotal/porosity;
        std::cout << "retarded velocity: " << retardedFrontVelocity << '\n';
        auto fvGeometry = localView(this->gridGeometry());
        for (const auto& element : elements(this->gridGeometry().gridView()))
        {
            fvGeometry.bindElement(element);
            for (auto&& scv : scvs(fvGeometry))
            {
                auto dofIdxGlobal = scv.dofIndex();
                auto dofPosition = scv.dofPosition();
                temperatureExact_[dofIdxGlobal] = (dofPosition[0] < retardedFrontVelocity*time) ? temperatureHigh_ : temperatureLow_;
            }
        }
    }

    /*!
     * \name Problem parameters
     */
    // \{

    /*!
     * \brief The problem name.
     *
     * This is used as a prefix for files generated by the simulation.
     */
    const std::string& name() const
    {
        return name_;
    }

    // \}

    /*!
     * \name Boundary conditions
     */
    // \{

    /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary segment.
     *
     * \param globalPos The position for which the bc type should be evaluated
     */
    BoundaryTypes boundaryTypesAtPos(const GlobalPosition &globalPos) const
    {
        BoundaryTypes values;
        if(globalPos[0] > this->gridGeometry().bBoxMax()[0] - eps_)
        {
            values.setAllDirichlet();
        }
        else
        {
            values.setAllNeumann();
        }
        return values;
    }

    /*!
     * \brief Evaluates the boundary conditions for a Dirichlet boundary segment.
     *
     * \param globalPos The position for which the bc type should be evaluated
     *
     */
    PrimaryVariables dirichletAtPos(const GlobalPosition &globalPos) const
    {
        return initialAtPos(globalPos);
    }

    /*!
     * \brief Evaluates the boundary conditions for a Neumann boundary segment.
     *
     * \param element The finite element
     * \param fvGeometry The finite-volume geometry in the box scheme
     * \param elemVolVars The element volume variables
     * \param elemFluxVarsCache Flux variables caches for all faces in stencil
     * \param scvf The subcontrolvolume face
     * Negative values mean influx.
     */
    NumEqVector neumann(const Element &element,
                        const FVElementGeometry& fvGeometry,
                        const ElementVolumeVariables& elemVolVars,
                        const ElementFluxVariablesCache& elemFluxVarsCache,
                        const SubControlVolumeFace& scvf) const
    {
        NumEqVector values(0.0);
        const auto globalPos = scvf.ipGlobal();
        const auto& volVars = elemVolVars[scvf.insideScvIdx()];

        if(globalPos[0] < eps_)
        {
            values[conti0EqIdx] = -darcyVelocity_*volVars.density(wPhaseIdx);
            values[energyEqIdx] = -darcyVelocity_*volVars.density(wPhaseIdx)
                                     *IapwsH2O::liquidEnthalpy(temperatureHigh_, volVars.pressure(wPhaseIdx));
        }
        return values;
    }

    // \}

    /*!
     * \name Volume terms
     */
    // \{

    /*!
     * \brief Evaluates the initial value for a control volume.
     *
     * \param globalPos The position for which the initial condition should be evaluated
     *
     */
    PrimaryVariables initialAtPos(const GlobalPosition &globalPos) const
    {
        PrimaryVariables values;
        values[pressureIdx] = pressureLow_; // initial condition for the pressure
        values[swIdx] = 1.0;  // initial condition for the wetting phase saturation
        values[snIdx] = 1e-10;  // initial condition for the nonwetting phase saturation
        values[temperatureIdx] = temperatureLow_;
        return values;
    }

    // \}

private:
    Scalar temperatureHigh_;
    Scalar temperatureLow_;
    Scalar pressureHigh_;
    Scalar pressureLow_;
    Scalar darcyVelocity_;
    static constexpr Scalar eps_ = 1e-6;
    std::string name_;
    int outputInterval_;
    std::vector<Scalar> temperatureExact_;
};

} // end namespace Dumux

#endif
