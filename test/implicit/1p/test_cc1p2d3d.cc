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
 * \brief test for the one-phase CC model
 */
#include "config.h"
#include "1ptestproblem.hh"
#include <dumux/common/start.hh>

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
                    errorMessageOut += "\n\nThe list of mandatory arguments for this program is:\n"
                                        "\t-TimeManager.TEnd               End of the simulation [s] \n"
                                        "\t-TimeManager.DtInitial          Initial timestep size [s] \n"
                                        "\t-Grid.File                      Name of the file containing the grid \n"
                                        "\t                                definition in DGF format\n"
                                        "\t-SpatialParams.LensLowerLeftX   x-coordinate of the lower left corner of the lens [m] \n"
                                        "\t-SpatialParams.LensLowerLeftY   y-coordinate of the lower left corner of the lens [m] \n"
                                        "\t-SpatialParams.LensUpperRightX  x-coordinate of the upper right corner of the lens [m] \n"
                                        "\t-SpatialParams.LensUpperRightY  y-coordinate of the upper right corner of the lens [m] \n"
                                        "\t-SpatialParams.Permeability     Permeability of the domain [m^2] \n"
                                        "\t-SpatialParams.PermeabilityLens Permeability of the lens [m^2] \n";

        std::cout << errorMessageOut
                  << "\n";
    }
}

int main(int argc, char** argv)
{
#if HAVE_ALBERTA
    typedef TTAG(OnePTwoDThreeDTestCCProblem) ProblemTypeTag;
    return Dumux::start<ProblemTypeTag>(argc, argv, usage);
#else
#warning External grid Alberta needed to run this example.
    std::cerr << "Test skipped, it needs Alberta!" << std::endl;
    return 77;
#endif
}
