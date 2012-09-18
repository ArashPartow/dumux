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
 * \ingroup Properties
 * \ingroup BoxProperties
 * \ingroup BoxModel
 * \file
 *
 * \brief Default properties for box models
 */
#ifndef DUMUX_BOX_PROPERTY_DEFAULTS_HH
#define DUMUX_BOX_PROPERTY_DEFAULTS_HH

#include <dumux/nonlinear/newtonmethod.hh>
#include <dumux/nonlinear/newtoncontroller.hh>

#include "boxassembler.hh"
#include "boxmodel.hh"
#include "boxfvelementgeometry.hh"
#include "boxelementboundarytypes.hh"
#include "boxlocaljacobian.hh"
#include "boxlocalresidual.hh"
#include "boxelementvolumevariables.hh"
#include "boxvolumevariables.hh"

#include <dumux/common/boundarytypes.hh>
#include <dumux/common/timemanager.hh>

#include "boxproperties.hh"

#include <limits>

namespace Dumux {

// forward declaration
template<class TypeTag>
class BoxModel;

namespace Properties {
//////////////////////////////////////////////////////////////////
// Some defaults for very fundamental properties
//////////////////////////////////////////////////////////////////

//! Set the default type for the time manager
SET_TYPE_PROP(BoxModel, TimeManager, Dumux::TimeManager<TypeTag>);

//////////////////////////////////////////////////////////////////
// Properties
//////////////////////////////////////////////////////////////////

//! Use the leaf grid view if not defined otherwise
SET_TYPE_PROP(BoxModel,
              GridView,
              typename GET_PROP_TYPE(TypeTag, Grid)::LeafGridView);

//! Set the default for the FVElementGeometry
SET_TYPE_PROP(BoxModel, FVElementGeometry, Dumux::BoxFVElementGeometry<TypeTag>);

//! Disable evaluation of shape function gradients at the sub-control volume center by default
// The shape function gradients at the sub-control volume center are currently only
// needed for the stokes and the linear elastic models
SET_BOOL_PROP(BoxModel, EvalGradientsAtSCVCenter, false);

//! Set the default for the ElementBoundaryTypes
SET_TYPE_PROP(BoxModel, ElementBoundaryTypes, Dumux::BoxElementBoundaryTypes<TypeTag>);

//! use the plain newton method for the box scheme by default
SET_TYPE_PROP(BoxModel, NewtonMethod, Dumux::NewtonMethod<TypeTag>);

//! use the plain newton controller for the box scheme by default
SET_TYPE_PROP(BoxModel, NewtonController, Dumux::NewtonController<TypeTag>);

//! Mapper for the grid view's vertices.
SET_TYPE_PROP(BoxModel,
              VertexMapper,
              Dune::MultipleCodimMultipleGeomTypeMapper<typename GET_PROP_TYPE(TypeTag, GridView),
                                                        Dune::MCMGVertexLayout>);

//! Mapper for the grid view's elements.
SET_TYPE_PROP(BoxModel,
              ElementMapper,
              Dune::MultipleCodimMultipleGeomTypeMapper<typename GET_PROP_TYPE(TypeTag, GridView),
                                                        Dune::MCMGElementLayout>);

//! Mapper for the degrees of freedoms.
SET_TYPE_PROP(BoxModel, DofMapper, typename GET_PROP_TYPE(TypeTag, VertexMapper));

//! Set the BaseLocalResidual to BoxLocalResidual
SET_TYPE_PROP(BoxModel, BaseLocalResidual, Dumux::BoxLocalResidual<TypeTag>);

//! Set the BaseModel to BoxModel
SET_TYPE_PROP(BoxModel, BaseModel, Dumux::BoxModel<TypeTag>);

//! The local jacobian operator for the box scheme
SET_TYPE_PROP(BoxModel, LocalJacobian, Dumux::BoxLocalJacobian<TypeTag>);

/*!
 * \brief The type of a solution for the whole grid at a fixed time.
 */
SET_TYPE_PROP(BoxModel,
              SolutionVector,
              Dune::BlockVector<typename GET_PROP_TYPE(TypeTag, PrimaryVariables)>);

/*!
 * \brief The type of a solution for a whole element.
 */
SET_TYPE_PROP(BoxModel,
              ElementSolutionVector,
              Dune::BlockVector<typename GET_PROP_TYPE(TypeTag, PrimaryVariables)>);

/*!
 * \brief A vector of primary variables.
 */
SET_TYPE_PROP(BoxModel,
              PrimaryVariables,
              Dune::FieldVector<typename GET_PROP_TYPE(TypeTag, Scalar),
                                GET_PROP_VALUE(TypeTag, NumEq)>);

/*!
 * \brief The volume variable class.
 *
 * This should almost certainly be overloaded by the model...
 */
SET_TYPE_PROP(BoxModel, VolumeVariables, Dumux::BoxVolumeVariables<TypeTag>);

/*!
 * \brief An array of secondary variable containers.
 */
SET_TYPE_PROP(BoxModel, ElementVolumeVariables, Dumux::BoxElementVolumeVariables<TypeTag>);

/*!
 * \brief Boundary types at a single degree of freedom.
 */
SET_TYPE_PROP(BoxModel,
              BoundaryTypes,
              Dumux::BoundaryTypes<GET_PROP_VALUE(TypeTag, NumEq)>);

/*!
 * \brief Assembler for the global jacobian matrix.
 */
SET_TYPE_PROP(BoxModel, JacobianAssembler, Dumux::BoxAssembler<TypeTag>);

//! use an unlimited time step size by default
#if 0
// requires GCC 4.6 and above to call the constexpr function of
// numeric_limits
SET_SCALAR_PROP(BoxModel, MaxTimeStepSize, std::numeric_limits<Scalar>::infinity());//DEPRECATED
#else
SET_SCALAR_PROP(BoxModel, MaxTimeStepSize, 1e100);//DEPRECATED
#endif
SET_SCALAR_PROP(BoxModel, TimeManagerMaxTimeStepSize, GET_PROP_VALUE(TypeTag, MaxTimeStepSize));

//! use forward differences to calculate the jacobian by default
SET_INT_PROP(BoxModel, ImplicitNumericDifferenceMethod, GET_PROP_VALUE(TypeTag, NumericDifferenceMethod));
SET_INT_PROP(BoxModel, NumericDifferenceMethod, +1);//DEPRECATED

//! do not use hints by default
SET_BOOL_PROP(BoxModel, ImplicitEnableHints, GET_PROP_VALUE(TypeTag, EnableHints));
SET_BOOL_PROP(BoxModel, EnableHints, false);//DEPRECATED

// disable jacobian matrix recycling by default
SET_BOOL_PROP(BoxModel, ImplicitEnableJacobianRecycling, GET_PROP_VALUE(TypeTag, EnableJacobianRecycling));
SET_BOOL_PROP(BoxModel, EnableJacobianRecycling, false);//DEPRECATED

// disable partial reassembling by default
SET_BOOL_PROP(BoxModel, ImplicitEnablePartialReassemble, GET_PROP_VALUE(TypeTag, EnablePartialReassemble));
SET_BOOL_PROP(BoxModel, EnablePartialReassemble, false);//DEPRECATED

// disable two-point-flux by default
SET_BOOL_PROP(BoxModel, ImplicitUseTwoPointFlux, false);

//! Set the type of a global jacobian matrix from the solution types
SET_PROP(BoxModel, JacobianMatrix)
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    enum { numEq = GET_PROP_VALUE(TypeTag, NumEq) };
    typedef typename Dune::FieldMatrix<Scalar, numEq, numEq> MatrixBlock;
public:
    typedef typename Dune::BCRSMatrix<MatrixBlock> type;
};

// use the stabilized BiCG solver preconditioned by the ILU-0 by default
SET_TYPE_PROP(BoxModel, LinearSolver, Dumux::BoxBiCGStabILU0Solver<TypeTag> );

// if the deflection of the newton method is large, we do not
// need to solve the linear approximation accurately. Assuming
// that the initial value for the delta vector u is quite
// close to the final value, a reduction of 6 orders of
// magnitude in the defect should be sufficient...
SET_SCALAR_PROP(BoxModel, LinearSolverResidualReduction, 1e-6);

//! set the default number of maximum iterations for the linear solver
SET_INT_PROP(BoxModel, LinearSolverMaxIterations, 250);

//! set number of equations of the mathematical model as default
SET_INT_PROP(BoxModel, LinearSolverBlockSize, GET_PROP_VALUE(TypeTag, NumEq));

} // namespace Properties
} // namespace Dumux

#endif
