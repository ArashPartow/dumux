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
/**
 * \file
 * \brief Definition of a 3pni problem:
 *        Component transport of nitrogen dissolved in the water phase.
 */
#ifndef DUMUX_3PNI_CONDUCTION_PROBLEM_HH
#define DUMUX_3PNI_CONDUCTION_PROBLEM_HH

#include <math.h>

#include <dumux/porousmediumflow/problem.hh>
#include <dumux/porousmediumflow/3p/implicit/model.hh>
#include <dumux/implicit/cellcentered/tpfa/properties.hh>
#include <dumux/implicit/cellcentered/mpfa/properties.hh>

#include <dumux/material/fluidsystems/h2oairmesitylene.hh>
#include <dumux/material/components/h2o.hh>
#include <dumux/material/fluidmatrixinteractions/3p/thermalconductivitysomerton3p.hh>
#include "3pnispatialparams.hh"


namespace Dumux
{

template <class TypeTag>
class ThreePNIConductionProblem;

namespace Properties
{
NEW_TYPE_TAG(ThreePNIConductionProblem, INHERITS_FROM(ThreePNI, ThreePNISpatialParams));
NEW_TYPE_TAG(ThreePNIConductionBoxProblem, INHERITS_FROM(BoxModel, ThreePNIConductionProblem));
NEW_TYPE_TAG(ThreePNIConductionCCProblem, INHERITS_FROM(CCTpfaModel, ThreePNIConductionProblem));
NEW_TYPE_TAG(ThreePNIConductionCCMpfaProblem, INHERITS_FROM(CCMpfaModel, ThreePNIConductionProblem));

// Set the grid type
SET_TYPE_PROP(ThreePNIConductionProblem, Grid, Dune::YaspGrid<2>);

// Set the problem property
SET_TYPE_PROP(ThreePNIConductionProblem, Problem, ThreePNIConductionProblem<TypeTag>);


// Set the fluid system
SET_TYPE_PROP(ThreePNIConductionProblem,
              FluidSystem,
              FluidSystems::H2OAirMesitylene<typename GET_PROP_TYPE(TypeTag, Scalar)>);

// Set the spatial parameters
SET_TYPE_PROP(ThreePNIConductionProblem,
              SpatialParams,
              ThreePNISpatialParams<TypeTag>);

}


/*!
 * \ingroup ThreePModel
 * \ingroup ImplicitTestProblems
 *
 * \brief Test for the ThreePModel in combination with the NI model for a conduction problem:
 * The simulation domain is a tube where with an elevated temperature on the left hand side.
 *
 * Initially the domain is fully saturated with water at a constant temperature.
 * On the left hand side there is a Dirichlet boundary condition with an increased temperature and on the right hand side
 * a Dirichlet boundary with constant pressure, saturation and temperature is applied.
 *
 * The results are compared to an analytical solution for a diffusion process:
  \f[
     T =T_{high} + (T_{init} - T_{high})erf \left(0.5\sqrt{\frac{x^2 S_{total}}{t \lambda_{eff}}}\right)
 \f]
 *
 * The result of the analytical solution is written into the vtu files.
 * This problem uses the \ref ThreePModel and \ref NIModel model.
 *
 * To run the simulation execute the following line in shell: <br>
 * <tt>./test_box3pniconduction -ParameterFile ./test_box3pniconduction.input</tt> or <br>
 * <tt>./test_cc3pniconduction -ParameterFile ./test_cc3pniconduction.input</tt>
 */
template <class TypeTag>
class ThreePNIConductionProblem : public PorousMediumFlowProblem<TypeTag>
{
    using ParentType = PorousMediumFlowProblem<TypeTag>;

    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using FVGridGeometry = typename GET_PROP_TYPE(TypeTag, FVGridGeometry);
    using PrimaryVariables = typename GET_PROP_TYPE(TypeTag, PrimaryVariables);
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);
    using BoundaryTypes = typename GET_PROP_TYPE(TypeTag, BoundaryTypes);
    using TimeManager = typename GET_PROP_TYPE(TypeTag, TimeManager);
    using ThermalConductivityModel = typename GET_PROP_TYPE(TypeTag, ThermalConductivityModel);
    using VolumeVariables = typename GET_PROP_TYPE(TypeTag, VolumeVariables);
    using ElementVolumeVariables = typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using IapwsH2O = H2O<Scalar>;
    using VtkOutputModule = typename GET_PROP_TYPE(TypeTag, VtkOutputModule);

    // copy some indices for convenience
    using Indices = typename GET_PROP_TYPE(TypeTag, Indices);
    enum {
        // world dimension
        dimWorld = GridView::dimensionworld
    };

    enum { isBox = GET_PROP_VALUE(TypeTag, ImplicitIsBox) };
    enum { dofCodim = isBox ? dimWorld : 0 };

    enum {
        // index of the primary variables
        pressureIdx = Indices::pressureIdx,
        swIdx = Indices::swIdx,
        snIdx = Indices::snIdx,
        temperatureIdx = Indices::temperatureIdx,
        wPhaseIdx = Indices::wPhaseIdx
    };

    using Element = typename GridView::template Codim<0>::Entity;
    using Intersection = typename GridView::Intersection;
    using GlobalPosition = Dune::FieldVector<Scalar, dimWorld>;

public:
    ThreePNIConductionProblem(std::shared_ptr<const FVGridGeometry> fvGridGeometry)
        : ParentType(fvGridGeometry)
    {
        //initialize fluid system
        FluidSystem::init();

        name_ = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag,
                                             std::string,
                                             Problem,
                                             Name);
        outputInterval_ = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag,
                  int, Problem, OutputInterval);

        temperatureHigh_ = 300.0;

    }


    bool shouldWriteOutput() const
    {
        return
            this->timeManager().timeStepIndex() == 0 ||
            this->timeManager().timeStepIndex() % outputInterval_ == 0 ||
            this->timeManager().episodeWillBeFinished() ||
            this->timeManager().willBeFinished();
    }


     /*!
     * \brief Adds additional VTK output data to the VTKWriter. Function is called by the output module on every write.
     */
    void addVtkOutputFields(VtkOutputModule& outputModule) const
    {
        auto& temperatureExact = outputModule.createScalarField("temperatureExact", dofCodim);

        const auto someElement = *(elements(this->gridView()).begin());
        const auto someElemSol = this->model().elementSolution(someElement, this->model().curSol());
        const auto someInitSol = initialAtPos(someElement.geometry().center());

        auto someFvGeometry = localView(this->model().fvGridGeometry());
        someFvGeometry.bindElement(someElement);
        const auto someScv = *(scvs(someFvGeometry).begin());

        VolumeVariables volVars;
        volVars.update(someElemSol, *this, someElement, someScv);

        const auto porosity = this->spatialParams().porosity(someElement, someScv, someElemSol);
        const auto densityW = volVars.density(wPhaseIdx);
        const auto heatCapacityW = IapwsH2O::liquidHeatCapacity(someInitSol[temperatureIdx], someInitSol[pressureIdx]);
        const auto densityS = this->spatialParams().solidDensity(someElement, someScv, someElemSol);
        const auto heatCapacityS = this->spatialParams().solidHeatCapacity(someElement, someScv, someElemSol);
        const auto storage = densityW*heatCapacityW*porosity + densityS*heatCapacityS*(1 - porosity);
        const auto effectiveThermalConductivity = ThermalConductivityModel::effectiveThermalConductivity(volVars, this->spatialParams(),
                                                                                                         someElement, someFvGeometry, someScv);
        using std::max;
        Scalar time = max(this->timeManager().time() + this->timeManager().timeStepSize(), 1e-10);

        for (const auto& element : elements(this->gridView()))
        {
            auto fvGeometry = localView(this->model().fvGridGeometry());
            fvGeometry.bindElement(element);

            for (auto&& scv : scvs(fvGeometry))
            {
                auto globalIdx = scv.dofIndex();
                const auto& globalPos = scv.dofPosition();
                using std::erf;
                using std::sqrt;
                temperatureExact[globalIdx] = temperatureHigh_ + (someInitSol[temperatureIdx] - temperatureHigh_)
                                              *erf(0.5*sqrt(globalPos[0]*globalPos[0]*storage/time/effectiveThermalConductivity));
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
        if(globalPos[0] < eps_ || globalPos[0] > this->bBoxMax()[0] - eps_)
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
     * \brief Evaluate the boundary conditions for a dirichlet
     *        boundary segment.
     *
     * \param globalPos The position for which the bc type should be evaluated
     */
    PrimaryVariables dirichletAtPos(const GlobalPosition &globalPos) const
    {
        PrimaryVariables values = initialAtPos(globalPos);

        if (globalPos[0] < eps_)
            values[temperatureIdx] = temperatureHigh_;
        return values;
    }

    /*!
     * \brief Evaluate the boundary conditions for a neumann
     *        boundary segment.
     *
     * \param element The finite element
     * \param fvGeometry The finite-volume geometry in the box scheme
     * \param elemVolVars The element volume variables
     * \param scvf The subcontrolvolume face
     *  Negative values mean influx.
     */
    PrimaryVariables neumann(const Element &element,
                             const FVElementGeometry& fvGeometry,
                             const ElementVolumeVariables& elemVolVars,
                             const SubControlVolumeFace& scvf) const
    {
        return PrimaryVariables(0.0);
    }

    // \}

    /*!
     * \name Volume terms
     */
    // \{

    /*!
     * \brief Evaluate the source term for all phases within a given
     *        sub-control-volume.
     *
     * \param globalPos The position for which the source should be evaluated
     *
     * Returns the rate mass of a component is generated or annihilate
     * per volume unit. Positive values mean that mass is created,
     * negative ones mean that it vanishes.
     *
     * The units must be according to either using mole or mass fractions. (mole/(m^3*s) or kg/(m^3*s))
     */
    PrimaryVariables sourceAtPos(const GlobalPosition &globalPos) const
    {
        return PrimaryVariables(0.0);
    }

    /*!
     * \brief Evaluate the initial value for a control volume.
     *
     * \param globalPos The position for which the initial condition should be evaluated
     *
     */
    PrimaryVariables initialAtPos(const GlobalPosition &globalPos) const
    {
        PrimaryVariables values;
        values[pressureIdx] = 1e5; // initial condition for the pressure
        values[swIdx] = 1.;  // initial condition for the wetting phase saturation
        values[snIdx] = 1e-5;  // initial condition for the non-wetting phase saturation
        values[temperatureIdx] = 290.;
        return values;
    }

    // \}

private:

    Scalar temperatureHigh_;
    static constexpr Scalar eps_ = 1e-6;
    std::string name_;
    int outputInterval_;
};

} //end namespace
#endif
