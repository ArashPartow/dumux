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
 * \brief Definition of a problem, for the linear elastic 1p2c problem:
 * Component transport of nitrogen dissolved in the water phase with a linear
 * elastic solid matrix.
 */
#ifndef DUMUX_EL1P2CPROBLEM_HH
#define DUMUX_EL1P2CPROBLEM_HH

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <iostream>

#include <dune/grid/yaspgrid.hh>
#include <dumux/geomechanics/el1p2c/model.hh>
#include <dumux/porousmediumflow/implicit/problem.hh>

#include <dumux/material/fluidsystems/h2on2liquidphasefluidsystem.hh>
#include "el1p2cspatialparams.hh"
#include <dumux/linear/amgbackend.hh>

namespace Dumux
{
    template<class TypeTag>
    class El1P2CProblem;

    namespace Properties
    {
    NEW_TYPE_TAG(El1P2CProblem, INHERITS_FROM(BoxElasticOnePTwoC));

    // Set the grid type
    SET_TYPE_PROP(El1P2CProblem, Grid, Dune::YaspGrid<3>);

    // Set the problem property
    SET_TYPE_PROP(El1P2CProblem, Problem, Dumux::El1P2CProblem<TTAG(El1P2CProblem)>);

    // Set fluid configuration
    SET_PROP(El1P2CProblem, FluidSystem)
    { private:
        typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    public:
        typedef Dumux::FluidSystems::H2ON2LiquidPhase<Scalar, false> type;
    };

    // Set the soil properties
    SET_TYPE_PROP(El1P2CProblem, SpatialParams, Dumux::El1P2CSpatialParams<TypeTag>);

    //Define whether mole(true) or mass (false) fractions are used
    SET_BOOL_PROP(El1P2CProblem, UseMoles, false);

    // Include stabilization term to prevent pressure oscillations
    SET_BOOL_PROP(El1P2CProblem, ImplicitWithStabilization, true);

    // use the algebraic multigrid
    SET_TYPE_PROP(El1P2CProblem, LinearSolver, Dumux::AMGBackend<TypeTag> );
}

/*!
 * \ingroup ElOnePTwoCBoxProblems
 * \brief Problem definition for a one-phase two-component transport process
 * in an elastic deformable matrix.
 *
 * The 3D domain given in el1p2c.dgf spans from (0,0,0) to (10,10,10).
 *
 * Dirichlet boundary conditions (p=101300, X=0.0, u=0.0) are applied at all boundaries.
 * In the center of the cube at the location (5,5,5) water with dissolved nitrogen is injected.
 * The injection leads to a pressure build-up which results in solid displacement (ux, uy, uz [m])
 * and effective stress changes.
 *
 */
template<class TypeTag = TTAG( El1P2CProblem)>
class El1P2CProblem: public ImplicitPorousMediaProblem<TypeTag>
{
    typedef ImplicitPorousMediaProblem<TypeTag> ParentType;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, TimeManager) TimeManager;

    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    enum
    {
        // Grid and world dimension
        dim = GridView::dimension,
        dimWorld = GridView::dimensionworld,
    };

    enum {
        // balance equation indices
        transportEqIdx = Indices::transportEqIdx
    };

    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;

    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<dim>::Entity Vertex;
    typedef typename GridView::Intersection Intersection;

    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;

    public:
    El1P2CProblem(TimeManager &timeManager, const GridView &gridView)
    : ParentType(timeManager, gridView)
    {}

    /*!
     * \brief The problem name.
     *
     * This is used as a prefix for files generated by the simulation.
     */
    const char *name() const
    {   return "el1p2c";}

    /*!
     * \brief Returns the temperature within the domain.
     */
    Scalar temperature() const
    {
        return 273.15 + 10; // in K
    };

    /*!
     * \name Boundary conditions
     */
    // \{

    /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary segment.
     *
     * \param values The boundary types for the conservation equations
     * \param vertex The vertex on the boundary for which the
     *               conditions needs to be specified
     */
    void boundaryTypes(BoundaryTypes &values, const Vertex &vertex) const
    {
        values.setAllDirichlet();
     }

    /*!
     * \brief Evaluate the boundary conditions for a dirichlet
     *        control volume.
     *
     * \param values The dirichlet values for the primary variables
     * \param vertex The vertex representing the "half volume on the boundary"
     *
     * For this method, the \a values parameter stores primary variables.
     */
    void dirichlet(PrimaryVariables &values, const Vertex &vertex) const
    {
        values = 0.0;
        values[0] = 101300;
    }

    /*!
     * \brief Evaluate the boundary conditions for a neumann
     *        boundary segment.
     *
     * For this method, the \a values parameter stores the mass flux
     * in normal direction of each phase. Negative values mean influx.
     */
    void neumann(PrimaryVariables &values,
                    const Element &element,
                    const FVElementGeometry &fvGeometry,
                    const Intersection &intersection,
                    int scvIdx,
                    int boundaryFaceIdx) const
    {
        values = 0.0;
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
     * For this method, the \a values parameter stores the rate mass
     * generated or annihilate per volume unit. Positive values mean
     * that mass is created, negative ones mean that it vanishes.
     */
    void sourceAtPos(PrimaryVariables &values,
                     const GlobalPosition &globalPos) const
    {
        values = Scalar(0.0);

            if(globalPos[0] < 6 && globalPos[0] > 4
                    && globalPos[1] < 6 && globalPos[1] > 4
                    && globalPos[2] < 6 && globalPos[2] > 4)
            {
                values[0] = 1.e-3;
                values[1] = 1.e-4;
            }
    }

    /*!
     * \brief Evaluate the initial value for a control volume.
     *
     * For this method, the \a values parameter stores primary
     * variables.
     */
    void initial(PrimaryVariables &values,
                    const Element &element,
                    const FVElementGeometry &fvGeometry,
                    int scvIdx) const
    {
        values = 0.0;
        values[0] = 101300;
    }

    static constexpr Scalar eps_ = 3e-6;
};
} //end namespace

#endif
