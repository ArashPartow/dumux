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
#ifndef DUMUX_FVPRESSURE1P_HH
#define DUMUX_FVPRESSURE1P_HH


// dumux environment
#include <dumux/decoupled/common/fv/fvpressure.hh>
#include <dumux/decoupled/1p/1pproperties.hh>

/**
 * \file
 * \brief  Single Phase Finite Volume Model
 */

namespace Dumux
{

//! \ingroup FV1p
//! \brief Single Phase Finite Volume Model
/*! This model solves equations of the form
 * \f[
 *  \text{div}\, \boldsymbol v = q.
 * \f]
 * The velocity \f$ \boldsymbol v \f$ is the single phase Darcy velocity:
 * \f[
 *  \boldsymbol v = -\frac{1}{\mu} \boldsymbol K \left(\textbf{grad}\, p + \rho \, g  \, \textbf{grad}\, z\right),
 * \f]
 * where \f$ p \f$ is the pressure, \f$ \boldsymbol K \f$ the absolute permeability, \f$ \mu \f$ the viscosity, \f$ \rho \f$ the density, and \f$ g \f$ the gravity constant,
 * and \f$ q \f$ is the source term.
 * At the boundary, \f$ p = p_D \f$ on \f$ \Gamma_{Dirichlet} \f$, and \f$ \boldsymbol v \cdot \boldsymbol n = q_N\f$
 * on \f$ \Gamma_{Neumann} \f$.
 *
 * \tparam TypeTag The Type Tag
 *
 */
template<class TypeTag> class FVPressure1P: public FVPressure<TypeTag>
{
    typedef FVPressure<TypeTag> ParentType;

    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;

    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;

    typedef typename GET_PROP_TYPE(TypeTag, Fluid) Fluid;

    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;
    typedef typename GET_PROP(TypeTag, SolutionTypes) SolutionTypes;
    typedef typename SolutionTypes::PrimaryVariables PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, CellData) CellData;
    typedef typename SolutionTypes::ScalarSolution ScalarSolutionType;

    enum
    {
        dim = GridView::dimension, dimWorld = GridView::dimensionworld
    };

    enum
    {
        pressEqIdx = Indices::pressureEqIdx // only one equation!
    };

    enum
    {
        rhs = ParentType::rhs, matrix = ParentType::matrix
    };

    typedef typename GridView::Traits::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    typedef typename GridView::template Codim<0>::EntityPointer ElementPointer;
    typedef typename GridView::Intersection Intersection;

    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;
    typedef Dune::FieldMatrix<Scalar, dim, dim> DimMatrix;


public:
    // Function which calculates the source entry
    void getSource(Dune::FieldVector<Scalar, 2>&, const Element&, const CellData&, const bool);
    // Function which calculates the storage entry
    //! \cond \private
    void getStorage(Dune::FieldVector<Scalar, 2>& entry, const Element& element, const CellData& cellData, const bool first)
    {
        entry = 0;
    }
    //! \endcond
    // Function which calculates the flux entry
    void getFlux(Dune::FieldVector<Scalar, 2>&, const Intersection&, const CellData&, const bool);
    // Function which calculates the boundary flux entry
    void getFluxOnBoundary(Dune::FieldVector<Scalar, 2>&,
    const Intersection&, const CellData&, const bool);

    /*! \brief Initializes the pressure model
     *
     * \copydetails ParentType::initialize()
     *
     * \param solveTwice indicates if more than one iteration is allowed to get an initial pressure solution
     */
    void initialize(bool solveTwice = true)
    {
        ParentType::initialize();
        this->assemble(true);
        this->solve();
        if (solveTwice)
        {
            this->assemble(false);
            this-> solve();
        }
        storePressureSolution();
        return;
    }

    /*! \brief Pressure update
     *
     * \copydetails ParentType::update()
     *
     */
    void update()
    {
        ParentType::update();
        storePressureSolution();
    }

    /*! \brief Globally stores the pressure solution
     *
     */
    void storePressureSolution()
    {
        int size = problem_.gridView().size(0);
        for (int i = 0; i < size; i++)
        {
            CellData& cellData = problem_.variables().cellData(i);
            storePressureSolution(i, cellData);
            cellData.fluxData().resetVelocity();
        }
    }

    /*! \brief Stores the pressure solution of a cell
     *
     * \param globalIdx Global cell index
     * \param cellData A CellData object
     */
    void storePressureSolution(int globalIdx, CellData& cellData)
    {
            Scalar press = this->pressure()[globalIdx];

            cellData.setPressure(press);
    }

    /*! \brief Adds pressure output to the output file
     *
     * Adds pressures to the output.
     *
     * \tparam MultiWriter Class defining the output writer
     * \param writer The output writer (usually a <tt>VTKMultiWriter</tt> object)
     *
     */
    template<class MultiWriter>
    void addOutputVtkFields(MultiWriter &writer)
    {
        ScalarSolutionType *pressure = writer.allocateManagedBuffer (
                problem_.gridView().size(0));

        *pressure = this->pressure();

        writer.attachCellData(*pressure, "pressure");

        return;
    }

    //! Constructs a FVPressure1P object
    /**
     * \param problem A problem class object
     */
    FVPressure1P(Problem& problem) :
        ParentType(problem), problem_(problem),
        gravity_(
        problem.gravity())
    {
        ElementIterator element = problem_.gridView().template begin<0> ();
        Scalar temperature = problem_.temperature(*element);
        Scalar referencePress = problem_.referencePressure(*element);

        density_ = Fluid::density(temperature, referencePress);
        viscosity_ = Fluid::viscosity(temperature, referencePress);
    }

private:
    Problem& problem_;
    const GlobalPosition& gravity_; //!< vector including the gravity constant
    Scalar density_;
    Scalar viscosity_;
};

/*! \brief Function which calculates the source entry
 *
 * \copydetails FVPressure::getSource(EntryType&,const Element&,const CellData&,const bool)
 *
 * Source the fluid phase has to be added as mass flux (\f$\text{kg}/(\text{m}^3 \text{s}\f$).
 */
template<class TypeTag>
void FVPressure1P<TypeTag>::getSource(Dune::FieldVector<Scalar, 2>& entry, const Element& element
        , const CellData& cellData, const bool first)
{
    // cell volume, assume linear map here
    Scalar volume = element.geometry().volume();

    // get sources from problem
    PrimaryVariables sourcePhase(0.0);
    problem_.source(sourcePhase, element);
        sourcePhase /= density_;

    entry[rhs] = volume * sourcePhase;

    return;
}

/*! \brief Function which calculates the flux entry
 *
 * \copydetails FVPressure::getFlux(EntryType&,const Intersection&,const CellData&,const bool)
 *
 */
template<class TypeTag>
void FVPressure1P<TypeTag>::getFlux(Dune::FieldVector<Scalar, 2>& entry, const Intersection& intersection
        , const CellData& cellData, const bool first)
{
    ElementPointer elementI = intersection.inside();
    ElementPointer elementJ = intersection.outside();

    // get global coordinates of cell centers
    const GlobalPosition& globalPosI = elementI->geometry().center();
    const GlobalPosition& globalPosJ = elementJ->geometry().center();

    //get face normal
    const Dune::FieldVector<Scalar, dim>& unitOuterNormal = intersection.centerUnitOuterNormal();

    // get face area
    Scalar faceArea = intersection.geometry().volume();

    // distance vector between barycenters
    GlobalPosition distVec = globalPosJ - globalPosI;

    // compute distance between cell centers
    Scalar dist = distVec.two_norm();

    // compute vectorized permeabilities
    DimMatrix meanPermeability(0);

    problem_.spatialParams().meanK(meanPermeability, problem_.spatialParams().intrinsicPermeability(*elementI),
            problem_.spatialParams().intrinsicPermeability(*elementJ));

    Dune::FieldVector<Scalar, dim> permeability(0);
    meanPermeability.mv(unitOuterNormal, permeability);

    permeability/=viscosity_;

    //calculate current matrix entry
    entry[matrix] = ((permeability * unitOuterNormal) / dist) * faceArea;

    //calculate right hand side
    entry[rhs] = density_ * (permeability * gravity_) * faceArea;

    return;
}

/*! \brief Function which calculates the flux entry at a boundary
 *
 * \copydetails FVPressure::getFluxOnBoundary(EntryType&,const Intersection&,const CellData&,const bool)
 *
 * Dirichlet boundary condition is a pressure,
 * Neumann boundary condition is the phase mass flux [\f$\text{kg}/(\text{m}^2 \text{s}\f$]
 */
template<class TypeTag>
void FVPressure1P<TypeTag>::getFluxOnBoundary(Dune::FieldVector<Scalar, 2>& entry,
const Intersection& intersection, const CellData& cellData, const bool first)
{
    ElementPointer element = intersection.inside();

    // get global coordinates of cell centers
    const GlobalPosition& globalPosI = element->geometry().center();

    // center of face in global coordinates
    const GlobalPosition& globalPosJ = intersection.geometry().center();

    //get face normal
    const Dune::FieldVector<Scalar, dim>& unitOuterNormal = intersection.centerUnitOuterNormal();

    // get face area
    Scalar faceArea = intersection.geometry().volume();

    // distance vector between barycenters
    GlobalPosition distVec = globalPosJ - globalPosI;

    // compute distance between cell centers
    Scalar dist = distVec.two_norm();

    BoundaryTypes bcType;
    problem_.boundaryTypes(bcType, intersection);
    PrimaryVariables boundValues(0.0);

    if (bcType.isDirichlet(pressEqIdx))
    {
        problem_.dirichlet(boundValues, intersection);

        //permeability vector at boundary
        // compute vectorized permeabilities
        DimMatrix meanPermeability(0);

        problem_.spatialParams().meanK(meanPermeability,
                problem_.spatialParams().intrinsicPermeability(*element));

        Dune::FieldVector<Scalar, dim> permeability(0);
        meanPermeability.mv(unitOuterNormal, permeability);

        permeability/= viscosity_;

        //get dirichlet pressure boundary condition
        Scalar pressBound = boundValues;

        //calculate current matrix entry
        entry[matrix] = ((permeability * unitOuterNormal) / dist) * faceArea;
        entry[rhs] = entry[matrix] * pressBound;

        //calculate right hand side
        entry[rhs] -= density_ * (permeability * gravity_) * faceArea;

    }
    //set neumann boundary condition
    else if (bcType.isNeumann(pressEqIdx))
    {
        problem_.neumann(boundValues, intersection);
        Scalar J = boundValues /= density_;

        entry[rhs] = -(J * faceArea);
    }
    else
    {
        DUNE_THROW(Dune::NotImplemented, "No valid boundary condition type defined for pressure equation!");
    }

    return;
}

}
#endif
