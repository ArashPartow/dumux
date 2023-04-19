// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
//
// SPDX-FileCopyrightInfo: Copyright © DuMux Project contributors, see AUTHORS.md in root folder
// SPDX-License-Identifier: GPL-3.0-or-later
//
/**
 * \file
 * \ingroup OnePNCTests
 * \brief Definition of a problem for a 1p3c problem:
 *        Component transport of N2, CO2 and H2 using the Maxwell-Stefan diffusion law.
 */

#ifndef DUMUX_1P3C_TEST_PROBLEM_HH
#define DUMUX_1P3C_TEST_PROBLEM_HH
#include <dumux/common/properties.hh>
#include <dumux/common/parameters.hh>

#include <dumux/common/boundarytypes.hh>
#include <dumux/common/numeqvector.hh>
#include <dumux/porousmediumflow/problem.hh>

#include <dumux/io/gnuplotinterface.hh>
namespace Dumux {


/*!
 * \ingroup OnePNCTests
 * \brief Definition of a problem for a 1p3c problem:
 *        Component transport of N2, CO2 and H2.

 * The domain is closed on all sides. H2 constitutes the bulk gas phase.
 * Initially, there is N2 and CO2 in the left half of the domain,
 * while only N2 is present in the right half of the domain.
 * Over time, the concentrations will equilibrate.
 *
 * This problem uses the \ref OnePNCModel model and the Maxwell-Stefan diffusion law.
 *
 */
template <class TypeTag>
class MaxwellStefanOnePThreeCTestProblem : public PorousMediumFlowProblem<TypeTag>
{
    using ParentType = PorousMediumFlowProblem<TypeTag>;

    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using Indices = typename GetPropType<TypeTag, Properties::ModelTraits>::Indices;
    using GridView = typename GetPropType<TypeTag, Properties::GridGeometry>::GridView;
    using BoundaryTypes = Dumux::BoundaryTypes<GetPropType<TypeTag, Properties::ModelTraits>::numEq()>;
    using PrimaryVariables = GetPropType<TypeTag, Properties::PrimaryVariables>;
    using NumEqVector = Dumux::NumEqVector<PrimaryVariables>;
    using FluidSystem = GetPropType<TypeTag, Properties::FluidSystem>;
    using GridGeometry = GetPropType<TypeTag, Properties::GridGeometry>;
    using SolutionVector = GetPropType<TypeTag, Properties::SolutionVector>;
    using VolumeVariables = GetPropType<TypeTag, Properties::VolumeVariables>;

    //! property that defines whether mole or mass fractions are used
    static constexpr bool useMoles = getPropValue<TypeTag, Properties::UseMoles>();

    using Element = typename GridGeometry::GridView::template Codim<0>::Entity;
    using GlobalPosition = typename Element::Geometry::GlobalCoordinate;

public:
    MaxwellStefanOnePThreeCTestProblem(std::shared_ptr<const GridGeometry> gridGeometry)
    : ParentType(gridGeometry)
    {
        name_ = getParam<std::string>("Problem.Name");

        // stating in the terminal whether mole or mass fractions are used
        if (useMoles)
            std::cout<<"problem uses mole fractions" << '\n';
        else
            std::cout<<"problem uses mass fractions" << '\n';

        plotOutput_ = false;
    }

    /*!
     * \name Problem parameters
     */
    // \{

    /*!
     * \brief The problem name.
     * This is used as a prefix for files generated by the simulation.
     */
    const std::string& name() const
    { return name_; }

    //! Called after every time step
    void plotComponentsOverTime(const SolutionVector& curSol, Scalar time)
    {
        if (plotOutput_)
        {
            Scalar x_co2_left = 0.0;
            Scalar x_n2_left = 0.0;
            Scalar x_co2_right = 0.0;
            Scalar x_n2_right = 0.0;
            Scalar x_h2_left = 0.0;
            Scalar x_h2_right = 0.0;
            Scalar i = 0.0;
            Scalar j = 0.0;
            if (!(time < 0.0))
            {
                auto fvGeometry = localView(this->gridGeometry());
                for (const auto& element : elements(this->gridGeometry().gridView()))
                {
                    fvGeometry.bindElement(element);
                    const auto elemSol = elementSolution(element, curSol, this->gridGeometry());

                    for (auto&& scv : scvs(fvGeometry))
                    {
                        const auto& globalPos = scv.dofPosition();
                        VolumeVariables volVars;
                        volVars.update(elemSol, *this, element, scv);

                        if (globalPos[0] < 0.5)
                        {
                            x_co2_left += volVars.moleFraction(0,2);

                            x_n2_left += volVars.moleFraction(0,1);
                            x_h2_left += volVars.moleFraction(0,0);
                            i +=1;
                        }
                        else
                        {
                            x_co2_right += volVars.moleFraction(0,2);
                            x_n2_right += volVars.moleFraction(0,1);
                            x_h2_right += volVars.moleFraction(0,0);
                            j +=1;
                        }
                    }
                }
                x_co2_left /= i;
                x_n2_left /= i;
                x_h2_left /= i;
                x_co2_right /= j;
                x_n2_right /= j;
                x_h2_right /= j;

                // do a gnuplot
                x_.push_back(time); // in seconds
                y_.push_back(x_n2_left);
                y2_.push_back(x_n2_right);
                y3_.push_back(x_co2_left);
                y4_.push_back(x_co2_right);
                y5_.push_back(x_h2_left);
                y6_.push_back(x_h2_right);

                gnuplot_.resetPlot();
                gnuplot_.setXRange(0, std::min(time, 72000.0));
                gnuplot_.setYRange(0.4, 0.6);
                gnuplot_.setXlabel("time [s]");
                gnuplot_.setYlabel("mole fraction mol/mol");
                gnuplot_.addDataSetToPlot(x_, y_, "N2_left.dat", "w l t 'N_2 left'");
                gnuplot_.addDataSetToPlot(x_, y2_, "N2_right.dat", "w l t 'N_2 right'");
                gnuplot_.plot("mole_fraction_N2");

                gnuplot2_.resetPlot();
                gnuplot2_.setXRange(0, std::min(time, 72000.0));
                gnuplot2_.setYRange(0.0, 0.6);
                gnuplot2_.setXlabel("time [s]");
                gnuplot2_.setYlabel("mole fraction mol/mol");
                gnuplot2_.addDataSetToPlot(x_, y3_, "CO2_left.dat", "w l t 'CO_2 left'");
                gnuplot2_.addDataSetToPlot(x_, y4_, "CO2_right.dat", "w l t 'CO_2 right");
                gnuplot2_.plot("mole_fraction_CO2");

                gnuplot3_.resetPlot();
                gnuplot3_.setXRange(0, std::min(time, 72000.0));
                gnuplot3_.setYRange(0.0, 0.6);
                gnuplot3_.setXlabel("time [s]");
                gnuplot3_.setYlabel("mole fraction mol/mol");
                gnuplot3_.addDataSetToPlot(x_, y5_, "H2_left.dat", "w l t 'H_2 left'");
                gnuplot3_.addDataSetToPlot(x_, y6_, "H2_right.dat", "w l t 'H_2 right'");
                gnuplot3_.plot("mole_fraction_H2");
           }
        }
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
        values.setAllNeumann();
        return values;
    }

    /*!
     * \brief Evaluates the boundary conditions for a Neumann boundary segment.
     *
     * \param globalPos The position for which the bc type should be evaluated
     */
    NumEqVector neumannAtPos(const GlobalPosition& globalPos) const
    { return NumEqVector(0.0); }

    // \}

    /*!
     * \name Volume terms
     */
    // \{

    /*!
     * \brief Evaluates the initial value for a control volume.
     *
     * \param globalPos The position for which the initial condition should be evaluated
     */
    PrimaryVariables initialAtPos(const GlobalPosition &globalPos) const
    {
        PrimaryVariables initialValues(0.0);
        if (globalPos[0] < 0.5)
        {
           initialValues[Indices::pressureIdx] = 1e5;
           initialValues[FluidSystem::N2Idx] = 0.50086;
           initialValues[FluidSystem::CO2Idx] = 0.49914;
        }
        else
        {
           initialValues[Indices::pressureIdx] = 1e5;
           initialValues[FluidSystem::N2Idx] = 0.49879;
           initialValues[FluidSystem::CO2Idx] = 0.0;
        }
        return initialValues;
    }

    // \}

private:
    std::string name_;

    Dumux::GnuplotInterface<Scalar> gnuplot_;
    Dumux::GnuplotInterface<Scalar> gnuplot2_;
    Dumux::GnuplotInterface<Scalar> gnuplot3_;

    std::vector<Scalar> x_;
    std::vector<Scalar> y_;
    std::vector<Scalar> y2_;
    std::vector<Scalar> y3_;
    std::vector<Scalar> y4_;
    std::vector<Scalar> y5_;
    std::vector<Scalar> y6_;

    bool plotOutput_;
};

} // end namespace Dumux

#endif
