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
 * \ingroup TracerTests
 * \brief  A 2p problem with multiple tracer bands in a porous groundwater reservoir with a lens
 */
#ifndef DUMUX_TWOP_TRACER_TEST_PROBLEM_HH
#define DUMUX_TWOP_TRACER_TEST_PROBLEM_HH

#include <dumux/common/boundarytypes.hh>

#include <dumux/porousmediumflow/problem.hh>

namespace Dumux {
/*!
 * \ingroup TracerTests
 *
 * \brief Definition of a problem, for the tracer problem:
 * A lens of contaminant tracer is diluted by diffusion and a base groundwater flow
 *
 * This problem uses the \ref TracerModel model.
 *
 * To run the simulation execute the following line in shell:
 * <tt>./test_2ptracer -ParameterFile ./params.input</tt> or
 * <tt>./test_2ptracer -ParameterFile ./params.input</tt>
 */
template <class TypeTag>
class TwoPTracerTestProblem : public PorousMediumFlowProblem<TypeTag>
{
    using ParentType = PorousMediumFlowProblem<TypeTag>;

    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using Indices = typename GetPropType<TypeTag, Properties::ModelTraits>::Indices;
    using GridView = typename GetPropType<TypeTag, Properties::GridGeometry>::GridView;
    using GridGeometry = GetPropType<TypeTag, Properties::GridGeometry>;
    using BoundaryTypes = Dumux::BoundaryTypes<GetPropType<TypeTag, Properties::ModelTraits>::numEq()>;
    using PrimaryVariables = GetPropType<TypeTag, Properties::PrimaryVariables>;
    using FluidSystem = GetPropType<TypeTag, Properties::FluidSystem>;
    using SpatialParams = GetPropType<TypeTag, Properties::SpatialParams>;

    //! property that defines whether mole or mass fractions are used
    static constexpr bool useMoles = getPropValue<TypeTag, Properties::UseMoles>();

    using Element = typename GridGeometry::GridView::template Codim<0>::Entity;
    using GlobalPosition = typename Element::Geometry::GlobalCoordinate;

public:
    TwoPTracerTestProblem(std::shared_ptr<const GridGeometry> gridGeometry)
    : ParentType(gridGeometry)
    {
        // stating in the console whether mole or mass fractions are used
        if(useMoles)
            std::cout<<"problem uses mole fractions" << '\n';
        else
            std::cout<<"problem uses mass fractions" << '\n';

        stripeWidth_ = getParam<Scalar>("Problem.StripeWidth", 0.125);
    }

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
        if (onLeftBoundary_(globalPos) || onRightBoundary_(globalPos))
            values.setAllDirichlet();
        else
            values.setAllNeumann();
        return values;
    }
    // \}

    /*!
     * \name Volume terms
     */
    // \{

    /*!
     * \brief Evaluates the boundary conditions for a Dirichlet boundary segment.
     *
     * \param globalPos The global position
     */
    PrimaryVariables dirichletAtPos(const GlobalPosition &globalPos) const
    {
        PrimaryVariables initialValues(0.0);
        return initialValues;
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
        PrimaryVariables initialValues(0.0);

        if (onStripe1_(globalPos) || onStripe2_(globalPos) || onStripe3_(globalPos))
        {
            if (useMoles)
                initialValues = 1e-9;
            else
                initialValues = 1e-9*FluidSystem::molarMass(0)/this->spatialParams().fluidMolarMass(globalPos);
        }
        return initialValues;
    }

private:
    static constexpr Scalar eps_ = 1e-6;
    Scalar stripeWidth_;

    bool onUpperBoundary_(const GlobalPosition &globalPos) const
    {
        return globalPos[1] > this->gridGeometry().bBoxMax()[1] - 0.1 - eps_;
    }

    bool onLeftBoundary_(const GlobalPosition &globalPos) const
    {
        return globalPos[0] < this->gridGeometry().bBoxMin()[0] + eps_;
    }

    bool onRightBoundary_(const GlobalPosition &globalPos) const
    {
        return globalPos[0] > this->gridGeometry().bBoxMax()[0] - eps_;
    }

    bool onStripe1_(const GlobalPosition &globalPos) const
    {
       const auto xMax = this->gridGeometry().bBoxMax()[0];
       return  (
           ( (xMax /4.0 - stripeWidth_*0.5) < globalPos[0] + eps_ ) &&
           ( (xMax/4.0 + stripeWidth_*0.5) > globalPos[0] + eps_  )
       );
    }

    bool onStripe2_(const GlobalPosition &globalPos) const
    {
        const auto xMax = this->gridGeometry().bBoxMax()[0];
        return  (
            ( (2.0 * xMax /4.0 - stripeWidth_*0.5) < globalPos[0] + eps_ ) &&
            ( (2.0 * xMax/4.0 + stripeWidth_*0.5) > globalPos[0] + eps_ )
        );
    }

    bool onStripe3_(const GlobalPosition &globalPos) const
    {
        const auto xMax = this->gridGeometry().bBoxMax()[0];
        return  (
            ( (3.0 * xMax /4.0 - stripeWidth_*0.5) < globalPos[0] + eps_ ) &&
            ( (3.0 * xMax/4.0 + stripeWidth_*0.5) > globalPos[0] + eps_ )
        );
    }

};

} //end namespace Dumux

#endif
