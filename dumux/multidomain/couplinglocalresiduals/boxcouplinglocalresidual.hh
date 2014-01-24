// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2011 by Klaus Mosthaf                                     *
 *   Copyright (C) 2011 by Bernd Flemisch                                    *
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
 * \brief Calculates the residual of models based on the box scheme element-wise.
 */
#ifndef DUMUX_BOX_COUPLING_LOCAL_RESIDUAL_HH
#define DUMUX_BOX_COUPLING_LOCAL_RESIDUAL_HH

#include <dumux/implicit/box/boxlocalresidual.hh>

namespace Dumux
{
/*!
 * \ingroup BoxModel
 * \ingroup BoxLocalResidual
 * \brief Element-wise calculation of the residual matrix for models
 *        based on the box scheme .
 *
 * \todo Please doc me more!
 */
template<class TypeTag>
class BoxCouplingLocalResidual : public BoxLocalResidual<TypeTag>
{
private:
    typedef BoxCouplingLocalResidual<TypeTag> ThisType;
    typedef BoxLocalResidual<TypeTag> ParentType;
    typedef typename GET_PROP_TYPE(TypeTag, LocalResidual) Implementation;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, Model) Model;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;

    enum {
        numEq = GET_PROP_VALUE(TypeTag, NumEq),

        dim = GridView::dimension,
        dimWorld = GridView::dimensionworld
    };

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GridView::Grid::ctype CoordScalar;

    typedef Dune::FieldVector<Scalar, dim>  LocalPosition;
    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;

    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    typedef typename GridView::template Codim<dim>::Entity Vertex;
    typedef typename GridView::template Codim<dim>::EntityPointer VertexPointer;

    typedef typename Dune::GenericReferenceElements<CoordScalar, dim> ReferenceElements;
    typedef typename Dune::GenericReferenceElement<CoordScalar, dim> ReferenceElement;

    typedef typename GridView::IntersectionIterator IntersectionIterator;
    typedef typename Element::Geometry Geometry;
    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;

    typedef typename GET_PROP_TYPE(TypeTag, VertexMapper) VertexMapper;
    typedef typename GET_PROP_TYPE(TypeTag, ElementSolutionVector) ElementSolutionVector;
    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;
    typedef typename GET_PROP_TYPE(TypeTag, ElementBoundaryTypes) ElementBoundaryTypes;
    typedef typename GET_PROP_TYPE(TypeTag, VolumeVariables) VolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables) ElementVolumeVariables;

    typedef Dune::FieldMatrix<Scalar, numEq, numEq>  MatrixBlock;
    typedef Dune::Matrix<MatrixBlock> LocalBlockMatrix;

    // copying the local residual class is not a good idea
    BoxCouplingLocalResidual(const BoxCouplingLocalResidual &);

public:
    BoxCouplingLocalResidual()
    { }

    ~BoxCouplingLocalResidual()
    { }

    /*!
     * \brief Compute the local residual, i.e. the deviation of the
     *        equations from zero. Gets a solution vector computed by PDELab
     *
     * \param element The DUNE Codim<0> entity for which the residual
     *                ought to be calculated
     * \param elementSolVector The local solution for the element using PDELab ordering
     */
    template<typename ElemSolVectorType>
    void evalPDELab(const Element &element,
                    const FVElementGeometry& fvGeometry,
                    const ElemSolVectorType& elementSolVector,
                    ElementVolumeVariables& volVarsPrev,
                    ElementVolumeVariables& volVarsCur)
    {
        this->elemPtr_ = &element;
        this->fvElemGeomPtr_ = &fvGeometry;

        volVarsPrev.update(this->problem_(),
                           element,
                           fvGeometry,
                           true /* oldSol? */);
        volVarsCur.updatePDELab(this->problem_(),
                          element,
                          fvGeometry,
                          elementSolVector);
        ElementBoundaryTypes bcTypes;
        bcTypes.update(this->problem_(), element, fvGeometry);

        asImp_().evalPDELab(element, fvGeometry, volVarsPrev, volVarsCur, bcTypes);
    }

    /*!
     * \brief Compute the local residual, i.e. the deviation of the
     *        equations from zero without taking boundary conditions into account.
     *        This is required for the flux calculation at the interface
     *        (called from the coupled problem). Calls evalPDELab with the
     *        required removal of the stabilization at the boundary (stokes).
     *
     * \param element The DUNE Codim<0> entity for which the residual
     *                ought to be calculated
     */
    void evalNoBoundary(const Element &element,
                        const FVElementGeometry fvGeometry,
                        ElementVolumeVariables& volVarsPrev,
                        ElementVolumeVariables& volVarsCur)
    {
        volVarsPrev.update(this->problem_(),
                           element,
                           fvGeometry,
                           true /* oldSol? */);
        volVarsCur.update(this->problem_(),
                          element,
                          fvGeometry,
                          false /* oldSol? */);

        ElementBoundaryTypes bcTypes;
        bcTypes.update(this->problem_(), element, fvGeometry);

        asImp_().evalPDELab(element, fvGeometry, volVarsPrev, volVarsCur, bcTypes);
    }

    /*!
     * \brief Compute the local residual, i.e. the deviation of the
     *        equations from zero. Calls evalBoundaryPDELab,
     *        where the stabilization of the mass balance (stokes)
     *        is removed. No further boundary conditions are employed.
     *
     * \param element The DUNE Codim<0> entity for which the residual
     *                ought to be calculated
     * \param fvGeometry The finite-volume geometry of the element
     * \param prevVolVars The volume averaged variables for all
     *                   sub-control volumes of the element at the previous
     *                   time level
     * \param curVolVars The volume averaged variables for all
     *                   sub-control volumes of the element at the current
     *                   time level
     * \param bcTypes The types of the boundary conditions for all
     *                vertices of the element
     */
    void evalPDELab(const Element &element,
              const FVElementGeometry &fvGeometry,
              const ElementVolumeVariables &prevVolVars,
              const ElementVolumeVariables &curVolVars,
              const ElementBoundaryTypes &bcTypes)
    {
        const int numVerts = fvGeometry.numScv;
#if HAVE_VALGRIND
        for (int i=0; i < numVerts; i++) {
            Valgrind::CheckDefined(prevVolVars[i]);
            Valgrind::CheckDefined(curVolVars[i]);
        }
#endif // HAVE_VALGRIND

        this->elemPtr_ = &element;
        this->fvElemGeomPtr_ = &fvGeometry;
        this->bcTypesPtr_ = &bcTypes;
        this->prevVolVarsPtr_ = &prevVolVars;
        this->curVolVarsPtr_ = &curVolVars;

        // reset residual
        this->residual_.resize(numVerts);
        this->storageTerm_.resize(numVerts);

        this->residual_ = 0;
        this->storageTerm_ = 0;

        asImp_().evalFluxes_();
        asImp_().evalVolumeTerms_();

        // evaluate the boundary (modified version)
        asImp_().evalBoundaryPDELab_();

#if HAVE_VALGRIND
        for (int i=0; i < numVerts; i++)
            Valgrind::CheckDefined(this->residual_[i]);
#endif // HAVE_VALGRIND
    }

protected:
    /*!
     * /brief Empty method, has to be overwritten if required.
     *        Called e.g. for the removal of the stabilization of the
     *        stokes model.
     *
     */
    void evalBoundaryPDELab_()
    { }

    Implementation &asImp_()
    {
        assert(static_cast<Implementation*>(this) != 0);
        return *static_cast<Implementation*>(this);
    }

    const Implementation &asImp_() const
    {
        assert(static_cast<const Implementation*>(this) != 0);
        return *static_cast<const Implementation*>(this);
    }
};

}

#endif
