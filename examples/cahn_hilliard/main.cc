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

// # Problem, test properties/traits and main program flow (`main.cc`)
// In this example the file `main.cc` contains the problem class `CahnHilliardTestProblem`,
// properties and traits specific to the test case as well as the main program flow in the form of
// `main` function.
//
#include <config.h>
// ## Problem
// The __problem class__ defines boundary conditions and extends the storage term defined in the
// model's localresidual by the derivative of the free energy.
//
// [[content]]
// ### Include headers
//
// [[codeblock]]
// use the property system and runtime parameters
#include <dumux/common/properties.hh>
#include <dumux/common/parameters.hh>
// common DuMux vector for discretized equations
#include <dumux/common/numeqvector.hh>
// types of boundary conditions to use
#include <dumux/common/boundarytypes.hh>
// generic problem for finite volume simulations
#include <dumux/common/fvproblem.hh>
// [[/codeblock]]
//
// ### The problem class `CahnHilliardTestProblem`
// In this abstract problem we inherit from the generic `FVProblem`.
//
// [[codeblock]]
namespace Dumux {

template<class TypeTag>
class CahnHilliardTestProblem : public FVProblem<TypeTag>
{
// [[/codeblock]]
// [[details]] alias definitions and local variables
// [[codeblock]]
    using ParentType = FVProblem<TypeTag>;
    using GridGeometry = GetPropType<TypeTag, Properties::GridGeometry>;
    using FVElementGeometry = typename GridGeometry::LocalView;
    using SubControlVolume = typename GridGeometry::SubControlVolume;
    using GridView = typename GetPropType<TypeTag, Properties::GridGeometry>::GridView;
    using Element = typename GridView::template Codim<0>::Entity;
    using GlobalPosition = typename Element::Geometry::GlobalCoordinate;

    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using PrimaryVariables = GetPropType<TypeTag, Properties::PrimaryVariables>;
    using NumEqVector = Dumux::NumEqVector<PrimaryVariables>;
    using BoundaryTypes = Dumux::BoundaryTypes<GetPropType<TypeTag, Properties::ModelTraits>::numEq()>;
    using Indices = typename GetPropType<TypeTag, Properties::ModelTraits>::Indices;
// [[/codeblock]]
// [[/details]]
// [[codeblock]]
public:
    CahnHilliardTestProblem(std::shared_ptr<const GridGeometry> gridGeometry)
    : ParentType(gridGeometry)
    {
        mobility_ = getParam<Scalar>("Problem.Mobility");
        surfaceTension_ = getParam<Scalar>("Problem.SurfaceTension");
        energyScale_ = getParam<Scalar>("Problem.EnergyScale");
    }
    // [[/codeblock]]
    //
    // ### Problem source term
    //
    // Here we implement the derivative of the free energy, setting a source for the equation for
    // the chemical potential. The `computeSource` function in the local residual adds the terms
    // defined here.
    //
    // [[codeblock]]
    template<class ElementVolumeVariables>
    NumEqVector source(const Element &element,
                       const FVElementGeometry& fvGeometry,
                       const ElementVolumeVariables& elemVolVars,
                       const SubControlVolume &scv) const
    {
        NumEqVector values(0.0);
        const auto& c = elemVolVars[scv].concentration();
        values[Indices::chemicalPotentialEqIdx] = -energyScale_*2.0*c*(2.0*c*c - 3*c + 1);
        return values;
    }
    // [[/codeblock]]
    //
    // ### Boundary conditions
    //
    // For the boundary we choose boundary flux (or Neumann) conditions for all equations and on
    // every part of the boundary, specifying zero flux everywhere for both equations.
    //
    // [[codeblock]]
    BoundaryTypes boundaryTypesAtPos(const GlobalPosition& globalPos) const
    {
        BoundaryTypes values;
        values.setAllNeumann();
        return values;
    }

    NumEqVector neumannAtPos(const GlobalPosition& globalPos) const
    { return { 0.0, 0.0 }; }
    // [[/codeblock]]
    //
    // [[details]] coefficients and private variables
    // The problem class offers access to the mobility and surface tension coefficients as read from
    // the parameter file by default `params.input`.
    //
    // [[codeblock]]
    Scalar mobility() const
    { return mobility_; }

    Scalar surfaceTension() const
    { return surfaceTension_; }

private:
    Scalar mobility_;
    Scalar surfaceTension_;
    Scalar energyScale_;
};

} // end namespace Dumux
// [[/codeblock]]
// [[/details]]
// [[/content]]
//
// ## Test case properties/traits
// Within the `Dumux::Properties` namespace we specialize properties and traits to the considered
// test case by using the test's TypeTag.
//
// [[content]]
//
// ### Include headers
//
// [[codeblock]]
// Include the grid to be used
#include <dune/grid/yaspgrid.hh>
// The header for the box discretization scheme
#include <dumux/discretization/box.hh>
// The model header including the model traits and properties
#include "model.hh"
// [[/codeblock]]
//
// ### TypeTag `CahnHilliardTest`
//
// We define a type tag for the test case, allowing us to further specify properties and traits. To
// use those set for the Cahn-Hilliard model we inherit from its type tag.
//
// [[codeblock]]
namespace Dumux::Properties {

// Inheriting properties of the Cahn-Hilliard model and the box finite volume discretization
namespace TTag {
struct CahnHilliardTest { using InheritsFrom = std::tuple<CahnHilliardModel, BoxModel>; };
} // end namespace TTag
// [[/codeblock]]
//
// ### Test properties
//
// We specify a grid to be used in the test, select our problem class and enable caching.
//
// [[codeblock]]
// Set the grid type
template<class TypeTag>
struct Grid<TypeTag, TTag::CahnHilliardTest>
{ using type = Dune::YaspGrid<2>; };

// Select the problem class defined above
template<class TypeTag>
struct Problem<TypeTag, TTag::CahnHilliardTest>
{ using type = CahnHilliardTestProblem<TypeTag>; };

// Enable caching
template<class TypeTag>
struct EnableGridVolumeVariablesCache<TypeTag, TTag::CahnHilliardTest>
{ static constexpr bool value = true; };

template<class TypeTag>
struct EnableGridFluxVariablesCache<TypeTag, TTag::CahnHilliardTest>
{ static constexpr bool value = true; };

template<class TypeTag>
struct EnableGridGeometryCache<TypeTag, TTag::CahnHilliardTest>
{ static constexpr bool value = true; };

} // end namespace Dumux::Properties
// [[/codeblock]]
// [[/content]]
//
// ## The main program flow
// The main program flow in the `main` function is often the only content of `main.cc`. It sets up
// the simulation framework, initializes runtime parameters, creates the grid and storage vectors
// for the variables, primary and secondary. It specifies and constructs and assembler, which
// assembles the discretized residual and system matrix (Jacobian of the model residual), as well as
// linear and nonlinear solvers that solve the resulting linear system and handle the convergence of
// nonlinear iterations. The time loop controls the time stepping, with adaptive time step size in
// coordination with the nonlinear solver.
//
// [[content]]
//
// ### Include headers
//
// [[codeblock]]
// standard header to generate random initial data
#include <random>
// common DuMux header for parallelization
#include <dumux/common/initialize.hh>
// headers to use the property system and runtime parameters
#include <dumux/common/properties.hh>
#include <dumux/common/parameters.hh>
// module for VTK output, to write out fields of interest
#include <dumux/io/vtkoutputmodule.hh>
// gridmanager for the grid used in the test
#include <dumux/io/grid/gridmanager_yasp.hh>
// headers for linear and non-linear solvers as well as the assembler
#include <dumux/linear/linearsolvertraits.hh>
#include <dumux/linear/linearalgebratraits.hh>
#include <dumux/linear/istlsolvers.hh>
#include <dumux/nonlinear/newtonsolver.hh>
#include <dumux/assembly/fvassembler.hh>
// [[/codeblock]]
//
// ### Creating the initial solution
//
// We define a helper struct and function to handle communication for parallel runs.
// For our initial conditions we create a random field of values around a mean of 0.42.
// The random values are created with an offset based on the processor rank for communication
// purposes, which is removed afterwards. For more information see the description of the
// diffusion example
// [examples/diffusion/doc/main.md](https://git.iws.uni-stuttgart.de/dumux-repositories/dumux/-/blob/master/examples/diffusion/doc/main.md)
//
// [[codeblock]]
struct MinScatter
{
    template<class A, class B>
    static void apply(A& a, const B& b)
    { a[0] = std::min(a[0], b[0]); }
};

template<class SolutionVector, class GridGeometry>
SolutionVector createInitialSolution(const GridGeometry& gg)
{
    SolutionVector sol(gg.numDofs());

    // Generate random number and add processor offset
    // For sequential runs `rank` always returns `0`.
    std::mt19937 gen(0); // seed is 0 for deterministic results
    std::uniform_real_distribution<> dis(0.0, 1.0);
    for (int n = 0; n < sol.size(); ++n)
    {
        sol[n][0] = 0.42 + 0.02*(0.5-dis(gen)) + gg.gridView().comm().rank();
        sol[n][1] = 0.0;
    }

    // Take the value of the processor with the minimum rank and subtract the rank offset
    if (gg.gridView().comm().size() > 1)
    {
        Dumux::VectorCommDataHandle<
            typename GridGeometry::VertexMapper,
            SolutionVector,
            GridGeometry::GridView::dimension,
            MinScatter
        > minHandle(gg.vertexMapper(), sol);
        gg.gridView().communicate(minHandle, Dune::All_All_Interface, Dune::ForwardCommunication);

        // Remove processor offset
        for (int n = 0; n < sol.size(); ++n)
            sol[n][0] -= std::floor(sol[n][0]);
    }
    return sol;
}
// [[/codeblock]]
//
// ### The main function
//
// The main function takes the command line arguments, optionally specifying an input file of
// parameters and/or individual key-value pairs of runtime parameters.
//
// [[codeblock]]
int main(int argc, char** argv)
{
    using namespace Dumux;

    // define the type tag for this problem
    using TypeTag = Properties::TTag::CahnHilliardTest;
    // [[/codeblock]]
    //
    // We initialize parallelization backends as well as runtime parameters
    //
    // [[codeblock]]
    // maybe initialize MPI and/or multithreading backend
    Dumux::initialize(argc, argv);

    // initialize parameter tree
    Parameters::init(argc, argv);
    // [[/codeblock]]
    //
    // ### Grid setup
    //
    // Set up the grid as well as a grid geometry to access the (sub-)control-volumes and their
    // faces
    //
    // [[codeblock]]
    // initialize the grid
    GridManager<GetPropType<TypeTag, Properties::Grid>> gridManager;
    gridManager.init();

    // we compute on the leaf grid view
    const auto& leafGridView = gridManager.grid().leafGridView();

    // create the finite volume grid geometry
    using GridGeometry = GetPropType<TypeTag, Properties::GridGeometry>;
    auto gridGeometry = std::make_shared<GridGeometry>(leafGridView);
    // [[/codeblock]]
    //
    // ### Problem setup
    //
    // We instantiate also the problem class according to the test properties
    //
    // [[codeblock]]
    // the problem (initial and boundary conditions)
    using Problem = GetPropType<TypeTag, Properties::Problem>;
    auto problem = std::make_shared<Problem>(gridGeometry);
    // [[/codeblock]]
    //
    // ### Applying initial conditions
    //
    // After writing the initial data to the storage for previous and current time-step, we
    // initialize the grid variables, also computing secondary variables.
    //
    // [[codeblock]]
    // the solution vector
    using SolutionVector = GetPropType<TypeTag, Properties::SolutionVector>;
    auto sol = createInitialSolution<SolutionVector>(*gridGeometry);
    // copy the vector to store state of previous time step
    auto oldSol = sol;

    // the grid variables
    using GridVariables = GetPropType<TypeTag, Properties::GridVariables>;
    auto gridVariables = std::make_shared<GridVariables>(problem, gridGeometry);
    gridVariables->init(sol);
    // [[/codeblock]]
    //
    // ### Initialize VTK output
    //
    // [[codeblock]]
    // initialize the vtk output module
    VtkOutputModule<GridVariables, SolutionVector> vtkWriter(*gridVariables, sol, problem->name());
    vtkWriter.addVolumeVariable([](const auto& vv){ return vv.concentration(); }, "c");
    vtkWriter.addVolumeVariable([](const auto& vv){ return vv.chemicalPotential(); }, "mu");
    vtkWriter.write(0.0);
    // [[/codeblock]]
    //
    // ### Set up time loop
    //
    // [[codeblock]]
    // get some time loop parameters
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    const auto tEnd = getParam<Scalar>("TimeLoop.TEnd");
    const auto dt = getParam<Scalar>("TimeLoop.InitialTimeStepSize");
    const auto maxDt = getParam<Scalar>("TimeLoop.MaxTimeStepSize");

    // instantiate time loop
    auto timeLoop = std::make_shared<CheckPointTimeLoop<Scalar>>(0.0, dt, tEnd);
    timeLoop->setMaxTimeStepSize(maxDt);
    // [[/codeblock]]
    //
    // ### Assembler, linear and nonlinear solver
    //
    // [[codeblock]]
    // the assembler with time loop for a transient problem
    using Assembler = FVAssembler<TypeTag, DiffMethod::numeric>;
    auto assembler = std::make_shared<Assembler>(problem, gridGeometry, gridVariables, timeLoop, oldSol);

    // the linear solver
    using LinearSolver = SSORBiCGSTABIstlSolver<
        LinearSolverTraits<GridGeometry>,
        LinearAlgebraTraitsFromAssembler<Assembler>
    >;
    auto linearSolver = std::make_shared<LinearSolver>(gridGeometry->gridView(), gridGeometry->dofMapper());

    // the solver
    using Solver = Dumux::NewtonSolver<Assembler, LinearSolver>;
    Solver solver(assembler, linearSolver);
    // [[/codeblock]]
    //
    // ### Time loop
    //
    // [[codeblock]]
    // time loop
    timeLoop->start(); do
    {
        // assemble & solve
        solver.solve(sol, *timeLoop);

        // make the new solution the old solution
        oldSol = sol;
        gridVariables->advanceTimeStep();

        // advance to the time loop to the next step
        timeLoop->advanceTimeStep();

        // write VTK output
        vtkWriter.write(timeLoop->time());

        // report statistics of this time step
        timeLoop->reportTimeStep();

        // set new dt as suggested by the Newton solver
        timeLoop->setTimeStepSize(solver.suggestTimeStepSize(timeLoop->timeStepSize()));

    } while (!timeLoop->finished());
    // [[/codeblock]]
    //
    // ### Finalize
    //
    // [[codeblock]]
    timeLoop->finalize(leafGridView.comm());

    return 0;
}
// [[/codeblock]]
// [[/content]]
