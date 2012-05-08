// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2011 by Bernd Flemisch                                    *
 *   Copyright (C) 2011 by Markus Wolff                                      *
 *   Copyright (C) 2011 by Klaus Mosthaf                                     *
 *   Institute for Modelling Hydraulic and Environmental Systems             *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@iws.uni-stuttgart.de                          *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \file
 * \brief Dumux solver backend
 */
#ifndef DUMUX_SOLVER_BACKEND_HH
#define DUMUX_SOLVER_BACKEND_HH

#include <dune/istl/solvers.hh>
#include <dune/istl/superlu.hh>

#include <dumux/common/parameters.hh>
#include <dumux/boxmodels/common/boxproperties.hh>
#include <dumux/decoupled/common/decoupledproperties.hh>
#include <dumux/linear/linearsolverproperties.hh>

namespace Dumux
{

/*!
 * \ingroup Linear
 * \brief A general solver backend allowing arbitrary preconditioners and solvers.
 */
template <class TypeTag>
class IterativePrecondSolverBackend
{
public:

  IterativePrecondSolverBackend()
  {}

  template<class Preconditioner, class Solver, class Matrix, class Vector>
  bool solve(const Matrix& A, Vector& x, const Vector& b)
  {
      int verbosity = GET_PARAM_FROM_GROUP(TypeTag, int, LinearSolver, Verbosity);
    const int maxIter = GET_PARAM_FROM_GROUP(TypeTag, double, LinearSolver, MaxIterations);
    const double residReduction = GET_PARAM_FROM_GROUP(TypeTag, double, LinearSolver, ResidualReduction);

    Vector bTmp(b);

    const double relaxation = GET_PARAM(TypeTag, double, PreconditionerRelaxation);
    const int precondIter = GET_PARAM(TypeTag, int, PreconditionerIterations);

    Preconditioner precond(A, precondIter, relaxation);

    typedef Dune::MatrixAdapter<Matrix, Vector, Vector> MatrixAdapter;
    MatrixAdapter operatorA(A);

    Solver solver(operatorA, precond, residReduction, maxIter, verbosity);

    solver.apply(x, bTmp, result_);

    return result_.converged;
  }

  // solve with RestartedGMRes (needs restartGMRes as additional argument)
  template<class Preconditioner, class Solver, class Matrix, class Vector>
  bool solve(const Matrix& A, Vector& x, const Vector& b, const int restartGMRes)
  {
    int verbosity = GET_PARAM_FROM_GROUP(TypeTag, int, LinearSolver, Verbosity);
    const int maxIter = GET_PARAM_FROM_GROUP(TypeTag, double, LinearSolver, MaxIterations);
    const double residReduction = GET_PARAM_FROM_GROUP(TypeTag, double, LinearSolver, ResidualReduction);

    Vector bTmp(b);

    const double relaxation = GET_PARAM(TypeTag, double, PreconditionerRelaxation);
    const int precondIter = GET_PARAM(TypeTag, int, PreconditionerIterations);

    Preconditioner precond(A, precondIter, relaxation);

    typedef Dune::MatrixAdapter<Matrix, Vector, Vector> MatrixAdapter;
    MatrixAdapter operatorA(A);

    Solver solver(operatorA, precond, residReduction, restartGMRes, maxIter, verbosity);

    solver.apply(x, bTmp, result_);

    return result_.converged;
  }

  const Dune::InverseOperatorResult& result() const
  {
    return result_;
  }

private:
  Dune::InverseOperatorResult result_;
};

/*!
 * \ingroup Linear
 * \brief Sequential ILUn-preconditioned BiCSTAB solver.
 */
template <class TypeTag>
class ILUnBiCGSTABBackend: public IterativePrecondSolverBackend<TypeTag>
{
  typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
  typedef IterativePrecondSolverBackend<TypeTag> ParentType;
public:

  ILUnBiCGSTABBackend(const Problem& problem)
  {}

  template<class Matrix, class Vector>
  bool solve(const Matrix& A, Vector& x, const Vector& b)
  {
    typedef Dune::SeqILUn<Matrix, Vector, Vector> Preconditioner;
    typedef Dune::BiCGSTABSolver<Vector> Solver;

    return ParentType::template solve<Preconditioner, Solver>(A, x, b);
  }
};

/*!
 * \ingroup Linear
 * \brief Sequential SOR-preconditioned BiCSTAB solver.
 */
template <class TypeTag>
class SORBiCGSTABBackend: public IterativePrecondSolverBackend<TypeTag>
{
  typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
  typedef IterativePrecondSolverBackend<TypeTag> ParentType;
public:

  SORBiCGSTABBackend(const Problem& problem)
  {}

  template<class Matrix, class Vector>
  bool solve(const Matrix& A, Vector& x, const Vector& b)
  {
    typedef Dune::SeqSOR<Matrix, Vector, Vector> Preconditioner;
    typedef Dune::BiCGSTABSolver<Vector> Solver;

    return ParentType::template solve<Preconditioner, Solver>(A, x, b);
  }
};

/*!
 * \ingroup Linear
 * \brief Sequential SSOR-preconditioned BiCSTAB solver.
 */
template <class TypeTag>
class SSORBiCGSTABBackend: public IterativePrecondSolverBackend<TypeTag>
{
  typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
  typedef IterativePrecondSolverBackend<TypeTag> ParentType;
public:

  SSORBiCGSTABBackend(const Problem& problem)
  {}

  template<class Matrix, class Vector>
  bool solve(const Matrix& A, Vector& x, const Vector& b)
  {
    typedef Dune::SeqSSOR<Matrix, Vector, Vector> Preconditioner;
    typedef Dune::BiCGSTABSolver<Vector> Solver;

    return ParentType::template solve<Preconditioner, Solver>(A, x, b);
  }
};

/*!
 * \ingroup Linear
 * \brief Sequential GS-preconditioned BiCSTAB solver.
 */
template <class TypeTag>
class GSBiCGSTABBackend: public IterativePrecondSolverBackend<TypeTag>
{
  typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
  typedef IterativePrecondSolverBackend<TypeTag> ParentType;
public:

  GSBiCGSTABBackend(const Problem& problem)
  {}

  template<class Matrix, class Vector>
  bool solve(const Matrix& A, Vector& x, const Vector& b)
  {
    typedef Dune::SeqGS<Matrix, Vector, Vector> Preconditioner;
    typedef Dune::BiCGSTABSolver<Vector> Solver;

    return ParentType::template solve<Preconditioner, Solver>(A, x, b);
  }
};

/*!
 * \ingroup Linear
 * \brief Sequential Jacobi-preconditioned BiCSTAB solver.
 */
template <class TypeTag>
class JacBiCGSTABBackend: public IterativePrecondSolverBackend<TypeTag>
{
  typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
  typedef IterativePrecondSolverBackend<TypeTag> ParentType;
public:

  JacBiCGSTABBackend(const Problem& problem)
  {}

  template<class Matrix, class Vector>
  bool solve(const Matrix& A, Vector& x, const Vector& b)
  {
    typedef Dune::SeqJac<Matrix, Vector, Vector> Preconditioner;
    typedef Dune::BiCGSTABSolver<Vector> Solver;

    return ParentType::template solve<Preconditioner, Solver>(A, x, b);
  }
};

/*!
 * \ingroup Linear
 * \brief Sequential ILUn-preconditioned CG solver.
 */
template <class TypeTag>
class ILUnCGBackend: public IterativePrecondSolverBackend<TypeTag>
{
  typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
  typedef IterativePrecondSolverBackend<TypeTag> ParentType;
public:

  ILUnCGBackend(const Problem& problem)
  {}

  template<class Matrix, class Vector>
  bool solve(const Matrix& A, Vector& x, const Vector& b)
  {
    typedef Dune::SeqILUn<Matrix, Vector, Vector> Preconditioner;
    typedef Dune::CGSolver<Vector> Solver;

    return ParentType::template solve<Preconditioner, Solver>(A, x, b);
  }
};

/*!
 * \ingroup Linear
 * \brief Sequential SOR-preconditioned CG solver.
 */
template <class TypeTag>
class SORCGBackend: public IterativePrecondSolverBackend<TypeTag>
{
  typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
  typedef IterativePrecondSolverBackend<TypeTag> ParentType;
public:

  SORCGBackend(const Problem& problem)
  {}

  template<class Matrix, class Vector>
  bool solve(const Matrix& A, Vector& x, const Vector& b)
  {
    typedef Dune::SeqSOR<Matrix, Vector, Vector> Preconditioner;
    typedef Dune::CGSolver<Vector> Solver;

    return ParentType::template solve<Preconditioner, Solver>(A, x, b);
  }
};

/*!
 * \ingroup Linear
 * \brief Sequential SSOR-preconditioned CG solver.
 */
template <class TypeTag>
class SSORCGBackend: public IterativePrecondSolverBackend<TypeTag>
{
  typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
  typedef IterativePrecondSolverBackend<TypeTag> ParentType;
public:

  SSORCGBackend(const Problem& problem)
  {}

  template<class Matrix, class Vector>
  bool solve(const Matrix& A, Vector& x, const Vector& b)
  {
    typedef Dune::SeqSSOR<Matrix, Vector, Vector> Preconditioner;
    typedef Dune::CGSolver<Vector> Solver;

    return ParentType::template solve<Preconditioner, Solver>(A, x, b);
  }
};

/*!
 * \ingroup Linear
 * \brief Sequential GS-preconditioned CG solver.
 */
template <class TypeTag>
class GSCGBackend: public IterativePrecondSolverBackend<TypeTag>
{
  typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
  typedef IterativePrecondSolverBackend<TypeTag> ParentType;
public:

  GSCGBackend(const Problem& problem)
  {}

  template<class Matrix, class Vector>
  bool solve(const Matrix& A, Vector& x, const Vector& b)
  {
    typedef Dune::SeqGS<Matrix, Vector, Vector> Preconditioner;
    typedef Dune::CGSolver<Vector> Solver;

    return ParentType::template solve<Preconditioner, Solver>(A, x, b);
  }
};

/*!
 * \ingroup Linear
 * \brief Sequential Jacobi-preconditioned CG solver.
 */
template <class TypeTag>
class JacCGBackend: public IterativePrecondSolverBackend<TypeTag>
{
  typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
  typedef IterativePrecondSolverBackend<TypeTag> ParentType;
public:

  JacCGBackend(const Problem& problem)
  {}

  template<class Matrix, class Vector>
  bool solve(const Matrix& A, Vector& x, const Vector& b)
  {
    typedef Dune::SeqJac<Matrix, Vector, Vector> Preconditioner;
    typedef Dune::CGSolver<Vector> Solver;

    return ParentType::template solve<Preconditioner, Solver>(A, x, b);
  }
};

/*!
 * \ingroup Linear
 * \brief Sequential SSOR-preconditioned GMRes solver.
 */
template <class TypeTag>
class SSORRestartedGMResBackend: public IterativePrecondSolverBackend<TypeTag>
{
  typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
  typedef IterativePrecondSolverBackend<TypeTag> ParentType;
public:

  SSORRestartedGMResBackend(const Problem& problem)
  {}

  template<class Matrix, class Vector>
  bool solve(const Matrix& A, Vector& x, const Vector& b)
  {
    typedef Dune::SeqSSOR<Matrix, Vector, Vector> Preconditioner;
    typedef Dune::RestartedGMResSolver<Vector> Solver;
    const int restart = GET_PARAM(TypeTag, int, GMResRestart);

    return ParentType::template solve<Preconditioner, Solver>(A, x, b, restart);
  }
};

/*!
 * \ingroup Linear
 * \brief Base class for backend combinations of linear solvers and a ILU0 preconditioner
 */
template <class TypeTag>
class ILU0SolverBackend
{
public:

  ILU0SolverBackend()
  {}

  template<class Preconditioner, class Solver, class Matrix, class Vector>
  bool solve(const Matrix& A, Vector& x, const Vector& b)
  {
    int verbosity = GET_PARAM_FROM_GROUP(TypeTag, int, LinearSolver, Verbosity);
    const int maxIter = GET_PARAM_FROM_GROUP(TypeTag, double, LinearSolver, MaxIterations);
    const double residReduction = GET_PARAM_FROM_GROUP(TypeTag, double, LinearSolver, ResidualReduction);

    Vector bTmp(b);

    const double relaxation = GET_PARAM(TypeTag, double, PreconditionerRelaxation);

    Preconditioner precond(A, relaxation);

    typedef Dune::MatrixAdapter<Matrix, Vector, Vector> MatrixAdapter;
    MatrixAdapter operatorA(A);

    Solver solver(operatorA, precond, residReduction, maxIter, verbosity);

    solver.apply(x, bTmp, result_);

    return result_.converged;
  }

  // solve with RestartedGMRes (needs restartGMRes as additional argument)
  template<class Preconditioner, class Solver, class Matrix, class Vector>
  bool solve(const Matrix& A, Vector& x, const Vector& b, const int restartGMRes)
  {
    int verbosity = GET_PARAM_FROM_GROUP(TypeTag, int, LinearSolver, Verbosity);
    const int maxIter = GET_PARAM_FROM_GROUP(TypeTag, double, LinearSolver, MaxIterations);
    const double residReduction = GET_PARAM_FROM_GROUP(TypeTag, double, LinearSolver, ResidualReduction);

    Vector bTmp(b);

    const double relaxation = GET_PARAM(TypeTag, double, PreconditionerRelaxation);
    const int precondIter = GET_PARAM(TypeTag, int, PreconditionerIterations);

    Preconditioner precond(A, relaxation);

    typedef Dune::MatrixAdapter<Matrix, Vector, Vector> MatrixAdapter;
    MatrixAdapter operatorA(A);

    Solver solver(operatorA, precond, residReduction, restartGMRes, maxIter, verbosity);

    solver.apply(x, bTmp, result_);

    return result_.converged;
  }

  const Dune::InverseOperatorResult& result() const
  {
    return result_;
  }

private:
  Dune::InverseOperatorResult result_;
};

/*!
 * \ingroup Linear
 * \brief Sequential ILU0-preconditioned BiCGSTAB solver.
 */
template <class TypeTag>
class ILU0BiCGSTABBackend : public ILU0SolverBackend<TypeTag>
{
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef ILU0SolverBackend<TypeTag> ParentType;
  public:

  ILU0BiCGSTABBackend(const Problem& problem)
  {}

  template<class Matrix, class Vector>
  bool solve(const Matrix& A, Vector& x, const Vector& b)
  {
      typedef Dune::SeqILU0<Matrix, Vector, Vector> Preconditioner;
      typedef Dune::BiCGSTABSolver<Vector> Solver;

      return ParentType::template solve<Preconditioner, Solver>(A, x, b);
  }
};

/*!
 * \ingroup Linear
 * \brief Sequential ILU0-preconditioned CG solver.
 */
template <class TypeTag>
class ILU0CGBackend : public ILU0SolverBackend<TypeTag>
{
  typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
  typedef ILU0SolverBackend<TypeTag> ParentType;
public:

  ILU0CGBackend(const Problem& problem)
  {}

  template<class Matrix, class Vector>
  bool solve(const Matrix& A, Vector& x, const Vector& b)
  {
      typedef Dune::SeqILU0<Matrix, Vector, Vector> Preconditioner;
      typedef Dune::CGSolver<Vector> Solver;

      return ParentType::template solve<Preconditioner, Solver>(A, x, b);
  }
};

/*!
 * \ingroup Linear
 * \brief Sequential ILU0-preconditioned GMRes solver.
 */
template <class TypeTag>
class ILU0RestartedGMResBackend : public ILU0SolverBackend<TypeTag>
{
  typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
  typedef ILU0SolverBackend<TypeTag> ParentType;
public:

  ILU0RestartedGMResBackend(const Problem& problem)
  {}

  template<class Matrix, class Vector>
  bool solve(const Matrix& A, Vector& x, const Vector& b)
  {
      typedef Dune::SeqILU0<Matrix, Vector, Vector> Preconditioner;
      typedef Dune::RestartedGMResSolver<Vector> Solver;
      const int restart = GET_PARAM(TypeTag, int, GMResRestart);

      return ParentType::template solve<Preconditioner, Solver>(A, x, b, restart);
  }
};

#if HAVE_SUPERLU
template <class TypeTag>
class SuperLUBackend
{
  typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;

public:

  SuperLUBackend(const Problem& problem)
  : problem_(problem)
  {}

  template<class Matrix, class Vector>
  bool solve(const Matrix& A, Vector& x, const Vector& b)
  {
    Vector bTmp(b);

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    enum {blockSize = GET_PROP_VALUE(TypeTag, LinearSolverBlockSize)};
    typedef typename Dune::FieldMatrix<Scalar, blockSize, blockSize> MatrixBlock;
    typedef typename Dune::BCRSMatrix<MatrixBlock> ISTLMatrix;

    int verbosity = GET_PARAM_FROM_GROUP(TypeTag, int, LinearSolver, Verbosity);
    Dune::SuperLU<ISTLMatrix> solver(A, verbosity > 0);

    solver.apply(x, bTmp, result_);

    int size = x.size();
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < blockSize; j++)
        {
            if (isnan(x[i][j]) || isinf(x[i][j]))
            {
                result_.converged = false;
                break;
            }
        }
    }

    return result_.converged;
  }

  const Dune::InverseOperatorResult& result() const
  {
    return result_;
  }

private:
  Dune::InverseOperatorResult result_;
  const Problem& problem_;
};
#endif

}
#endif
