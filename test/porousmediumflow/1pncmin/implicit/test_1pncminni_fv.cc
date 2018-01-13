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
 * \brief Test for the 1pncminni model
 */
#include <config.h>
#include <iostream>

#include <dune/common/parallel/mpihelper.hh>

#include <dumux/common/properties.hh>
#include <dumux/common/parameters.hh>
#include <dumux/common/dumuxmessage.hh>

#include <dumux/nonlinear/newtonmethod.hh>
#include <dumux/nonlinear/newtoncontroller.hh>
#include <dumux/linear/seqsolverbackend.hh>

#include <dumux/assembly/fvassembler.hh>

#include <dumux/io/vtkoutputmodule.hh>

#include "thermochemproblem.hh"

/*!
 * \brief Provides an interface for customizing error messages associated with
 *        reading in parameters.
 *
 * \param progName  The name of the program, that was tried to be started.
 * \param errorMsg  The error message that was issued by the start function.
 *                  Comprises the thing that went wrong and a general help message.
 */
void usage(const char *progName, const std::string &errorMsg)
{
    if (errorMsg.size() > 0) {
        std::string errorMessageOut = "\nUsage: ";
                    errorMessageOut += progName;
                    errorMessageOut += " [options]\n";
                    errorMessageOut += errorMsg;
                    errorMessageOut += "\n\nThe list of mandatory options for this program is:\n"
                                        "\t-TimeLoop.TEnd                  End of the simulation [s] \n"
                                        "\t-TimeLoop.DtInitial            Initial timestep size [s] \n"
                                        "\t-Grid.UpperRight                Upper right corner coordinates\n"
                                        "\t-Grid.Cells                     Number of cells in respective coordinate directions\n"
                                        "\t-Problem.Name                   Name for the vtk files \n"
                                        "\t-Problem.PressureInitial        Initial Pressure [Pa] \n"
                                        "\t-Problem.TemperatureInitial     Initial Temperature [K] \n"
                                        "\t-Problem.VaporInitial           Initial vapor mole fraction [-] \n"
                                        "\t-Problem.CaOInitial             Initial volumefraction of CaO [-] \n"
                                        "\t-Problem.CaO2H2Initial          Initial volumefraction of Ca(OH)2 [-] \n"
                                        "\t-Problem.BoundaryPressure       Pressure at the boundary [Pa] \n"
                                        "\t-Problem.BoundaryTemperature    Temperature at the boundary [K] \n"
                                        "\t-Problem.BoundaryMoleFraction   Vapor molefraction at the boundary [K] \n";

        std::cout << errorMessageOut
                  << "\n";
    }
}

int main(int argc, char** argv) try
{
    using namespace Dumux;

    // define the type tag for this problem
    using TypeTag = TTAG(TYPETAG);

    ////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////

    // initialize MPI, finalize is done automatically on exit
    const auto& mpiHelper = Dune::MPIHelper::instance(argc, argv);

    // print dumux start message
    if (mpiHelper.rank() == 0)
        DumuxMessage::print(/*firstCall=*/true);

    // parse command line arguments and input file
    Parameters::init(argc, argv, usage);

    //////////////////////////////////////////////////////////////////////
    // try to create a grid (from the given grid file or the input file)
    /////////////////////////////////////////////////////////////////////

    using GridCreator = typename GET_PROP_TYPE(TypeTag, GridCreator);
    GridCreator::makeGrid();
    GridCreator::loadBalance();

    ////////////////////////////////////////////////////////////
    // run instationary non-linear problem on this grid
    ////////////////////////////////////////////////////////////

    // we compute on the leaf grid view
    const auto& leafGridView = GridCreator::grid().leafGridView();

    // create the finite volume grid geometry
    using FVGridGeometry = typename GET_PROP_TYPE(TypeTag, FVGridGeometry);
    auto fvGridGeometry = std::make_shared<FVGridGeometry>(leafGridView);
    fvGridGeometry->update();

    // the problem (initial and boundary conditions)
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    auto problem = std::make_shared<Problem>(fvGridGeometry);

    // the solution vector
    using SolutionVector = typename GET_PROP_TYPE(TypeTag, SolutionVector);
    SolutionVector x(fvGridGeometry->numDofs());
    problem->applyInitialSolution(x);
    auto xOld = x;

    // the grid variables
    using GridVariables = typename GET_PROP_TYPE(TypeTag, GridVariables);
    auto gridVariables = std::make_shared<GridVariables>(problem, fvGridGeometry);
    gridVariables->init(x, xOld);

    // get some time loop parameters
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    auto tEnd = getParam<Scalar>("TimeLoop.TEnd");
    auto dt = getParam<Scalar>("TimeLoop.DtInitial");
    auto maxDivisions = getParam<int>("TimeLoop.MaxTimeStepDivisions");
    auto maxDt = getParam<Scalar>("TimeLoop.MaxTimeStepSize");

    // intialize the vtk output module
    VtkOutputModule<TypeTag> vtkWriter(*problem, *fvGridGeometry, *gridVariables, x, problem->name());
    using VtkOutputFields = typename GET_PROP_TYPE(TypeTag, VtkOutputFields);
    VtkOutputFields::init(vtkWriter);

    // Add model specific output fields
    vtkWriter.addField(problem->getPerm(), "permeability");
    vtkWriter.addField(problem->getPoro(), "porosity");
    vtkWriter.addField(problem->getRRate(), "reactionRate");

    // update the output fields before writing initial solution
    problem->updateVtkOutput(x);
    vtkWriter.write(0.0);

    // instantiate time loop
    auto timeLoop = std::make_shared<TimeLoop<Scalar>>(0.0, dt, tEnd);
    timeLoop->setMaxTimeStepSize(maxDt);

    // the assembler with time loop for instationary problem
    using Assembler = FVAssembler<TypeTag, DiffMethod::numeric>;
    auto assembler = std::make_shared<Assembler>(problem, fvGridGeometry, gridVariables, timeLoop);

    // the linear solver
    using LinearSolver = ILU0BiCGSTABBackend;
    auto linearSolver = std::make_shared<LinearSolver>();

    // the non-linear solver
    using NewtonController = Dumux::NewtonController<Scalar>;
    auto newtonController = std::make_shared<NewtonController>(timeLoop);
    NewtonMethod<NewtonController, Assembler, LinearSolver> nonLinearSolver(newtonController, assembler, linearSolver);

    // time loop
    timeLoop->start(); do
    {
        // set time for problem for implicit Euler scheme
        problem->setTimeStepSize( timeLoop->timeStepSize() );

        // set previous solution for storage evaluations
        assembler->setPreviousSolution(xOld);

        // try solving the non-linear system
        for (int i = 0; i < maxDivisions; ++i)
        {
            // linearize & solve
            auto converged = nonLinearSolver.solve(x);

            if (converged)
                break;

            if (!converged && i == maxDivisions-1)
                DUNE_THROW(Dune::MathError,
                        "Newton solver didn't converge after "
                        << maxDivisions
                        << " time-step divisions. dt="
                        << timeLoop->timeStepSize()
                        << ".\nThe solutions of the current and the previous time steps "
                        << "have been saved to restart files.");
        }

    // make the new solution the old solution
    xOld = x;
    gridVariables->advanceTimeStep();

    // advance to the time loop to the next step
    timeLoop->advanceTimeStep();

    // update the output fields before write
    problem->updateVtkOutput(x);

    // write vtk output
    vtkWriter.write(timeLoop->time());

    // report statistics of this time step
    timeLoop->reportTimeStep();

    // set new dt as suggested by newton controller
    timeLoop->setTimeStepSize(newtonController->suggestTimeStepSize(timeLoop->timeStepSize()));

    } while (!timeLoop->finished());

    // finalize, print dumux message to say goodbye
    timeLoop->finalize(leafGridView.comm());
    if (mpiHelper.rank() == 0)
        DumuxMessage::print(/*firstCall=*/false);

    return 0;

}
catch (Dumux::ParameterException &e)
{
    std::cerr << std::endl << e << " ---> Abort!" << std::endl;
    return 1;
}
catch (Dune::DGFException & e)
{
    std::cerr << "DGF exception thrown (" << e <<
                "). Most likely, the DGF file name is wrong "
                "or the DGF file is corrupted, "
                "e.g. missing hash at end of file or wrong number (dimensions) of entries."
                << " ---> Abort!" << std::endl;
    return 2;
}
catch (Dune::Exception &e)
{
    std::cerr << "Dune reported error: " << e << " ---> Abort!" << std::endl;
    return 3;
}
catch (...)
{
    std::cerr << "Unknown exception thrown! ---> Abort!" << std::endl;
    return 4;
}
