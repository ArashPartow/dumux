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

#ifndef DUMUX_MIMETICPRESSURE2PADAPTIVE_HH
#define DUMUX_MIMETICPRESSURE2PADAPTIVE_HH

/*!
 * \file
 *
 * \brief Model for the pressure equation discretized by mimetic FD.
 */
// dumux environment
#include <dumux/porousmediumflow/sequential/mimetic/properties.hh>
#include <dumux/porousmediumflow/2p/sequential/diffusion/mimetic/operatoradaptive.hh>
#include <dumux/porousmediumflow/2p/sequential/diffusion/mimetic/mimeticadaptive.hh>

namespace Dumux
{

/*! \ingroup Mimetic2p
 *
 * \brief mimetic method for the pressure equation
 *
 * Provides a mimetic implementation for the evaluation
 * of equations of the form
 * \f[\text{div}\, \boldsymbol{v}_{total} = q.\f]
 * The definition of the total velocity \f$\boldsymbol{v}_total\f$ depends on the kind of pressure chosen.
 * This could be a wetting (w) phase pressure leading to
 * \f[ - \text{div}\,  \left[\lambda \boldsymbol{K} \left(\text{grad}\, p_w + f_n \text{grad}\, p_c
 *     + \sum f_\alpha \rho_\alpha g  \text{grad}\, z\right)\right] = q, \f]
 * a non-wetting (n) phase pressure yielding
 * \f[ - \text{div}\,  \left[\lambda \boldsymbol{K}  \left(\text{grad}\, p_n - f_w \text{grad}\, p_c
 *     + \sum f_\alpha \rho_\alpha g  \text{grad}\, z\right)\right] = q, \f]
 * or a global pressure leading to
 * \f[ - \text{div}\, \left[\lambda \boldsymbol{K} \left(\text{grad}\, p_{global}
 *     + \sum f_\alpha \rho_\alpha g  \text{grad}\, z\right)\right] = q.\f]
 * Here, \f$p\f$ denotes a pressure, \f$\boldsymbol{K}\f$ the absolute permeability,
 * \f$\lambda\f$ the total mobility, possibly depending on the
 * saturation,\f$f\f$ the fractional flow function of a phase, \f$\rho\f$ a phase density,
 * \f$g\f$ the gravity constant and \f$q\f$ the source term.
 * For all cases, \f$p = p_D\f$ on \f$\Gamma_{Neumann}\f$, and \f$\boldsymbol{v}_{total}  = q_N\f$
 * on \f$\Gamma_{Dirichlet}\f$.
 *
 *\tparam TypeTag The Type Tag
 */
template<class TypeTag> class MimeticPressure2PAdaptive
{
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;

    typedef typename GET_PROP_TYPE(TypeTag, SpatialParams) SpatialParams;
    typedef typename SpatialParams::MaterialLaw MaterialLaw;

    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;

    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, FluidState) FluidState;

    enum
    {
        dim = GridView::dimension, dimWorld = GridView::dimensionworld
    };
    enum
    {
        pw = Indices::pressureW,
        pn = Indices::pressureNw,
        pGlobal = Indices::pressureGlobal,
        Sw = Indices::saturationW,
        Sn = Indices::saturationNw,
        vw = Indices::velocityW,
        vn = Indices::velocityNw,
        //! gives kind of pressure used (\f$ 0 = p_w\f$, \f$ 1 = p_n\f$, \f$ 2 = p_{global}\f$)
        pressureType = GET_PROP_VALUE(TypeTag, PressureFormulation),
        //! gives kind of saturation used (\f$ 0 = S_w\f$, \f$ 1 = S_n\f$)
        saturationType = GET_PROP_VALUE(TypeTag, SaturationFormulation),
    };
    enum
    {
        wPhaseIdx = Indices::wPhaseIdx,
        nPhaseIdx = Indices::nPhaseIdx,
        numPhases = GET_PROP_VALUE(TypeTag, NumPhases)
    };

    typedef typename GridView::Traits::template Codim<0>::Entity Element;
    typedef typename GridView::Grid Grid;

    typedef typename Element::Geometry Geometry;
    typedef typename Geometry::JacobianTransposed JacobianTransposed;

    typedef Dune::FieldVector<Scalar, dim> DimVector;

    typedef typename GET_PROP_TYPE(TypeTag, LocalStiffness) LocalStiffness;
    typedef Dune::BlockVector< Dune::FieldVector<Scalar, 1> > TraceType;
    typedef MimeticOperatorAssemblerTwoPAdaptive<TypeTag> OperatorAssembler;

    typedef typename GET_PROP_TYPE(TypeTag, CellData) CellData;
    typedef typename GET_PROP(TypeTag, SolutionTypes) SolutionTypes;
    typedef typename SolutionTypes::ScalarSolution ScalarSolutionType;

    typedef typename GET_PROP_TYPE(TypeTag, PressureCoefficientMatrix) Matrix;
    typedef typename GET_PROP_TYPE(TypeTag, PressureRHSVector) Vector;

    typedef typename Dune::ReferenceElements<Scalar, dim> ReferenceElements;
    typedef typename Dune::ReferenceElement<Scalar, dim> ReferenceElement;

    //initializes the matrix to store the system of equations
    void initializeMatrix();

    //function which assembles the system of equations to be solved
    void assemble(bool first)
    {
        Scalar timeStep = problem_.timeManager().timeStepSize();
        Scalar maxError = 0.0;
        int size = problem_.gridView().size(0);
        for (int i = 0; i < size; i++)
        {
            Scalar sat = 0;
            switch (saturationType)
            {
            case Sw:
                sat = problem_.variables().cellData(i).saturation(wPhaseIdx);
                break;
            case Sn:
                sat = problem_.variables().cellData(i).saturation(nPhaseIdx);
                break;
            }
            if (sat > 1.0)
            {
                maxError = std::max(maxError, (sat - 1.0) / timeStep);
            }
            if (sat < 0.0)
            {
                maxError = std::max(maxError, (-sat) / timeStep);
            }
        }

        lstiff_.setErrorInfo(maxError, timeStep);
        A_.assemble(lstiff_, pressTrace_, f_);
        return;
    }

    //solves the system of equations to get the spatial distribution of the pressure
    void solve();

    void postprocess()
    {
            A_.calculatePressure(lstiff_, pressTrace_, problem_);

        return;
    }

public:
    //constitutive functions are initialized and stored in the variables object
    void updateMaterialLaws();

    void initialize(bool solveTwice = true)
    {
        const auto element = *problem_.gridView().template begin<0>();
        FluidState fluidState;
        fluidState.setPressure(wPhaseIdx, problem_.referencePressure(element));
        fluidState.setPressure(nPhaseIdx, problem_.referencePressure(element));
        fluidState.setTemperature(problem_.temperature(element));
        fluidState.setSaturation(wPhaseIdx, 1.);
        fluidState.setSaturation(nPhaseIdx, 0.);
        density_[wPhaseIdx] = FluidSystem::density(fluidState, wPhaseIdx);
        density_[nPhaseIdx] = FluidSystem::density(fluidState, nPhaseIdx);
        viscosity_[wPhaseIdx] = FluidSystem::viscosity(fluidState, wPhaseIdx);
        viscosity_[nPhaseIdx] = FluidSystem::viscosity(fluidState, nPhaseIdx);

        updateMaterialLaws();
        adapt();
        lstiff_.initialize();
        lstiff_.reset();

        assemble(true);
        solve();
        postprocess();

        return;
    }

    void adapt()
    {
        A_.adapt();
        pressTrace_.resize(A_.intersectionMapper().size());
        f_.resize(A_.intersectionMapper().size());
        pressTrace_ = 0.0;
        f_ = 0;
        lstiff_.adapt();
    }

    void updateVelocity()
    {
        updateMaterialLaws();
        postprocess();
    }

    void update()
    {
        if (problem_.gridAdapt().wasAdapted())
        {
            adapt();
        }

        lstiff_.reset();
        assemble(false);

        solve();

        postprocess();

        return;
    }


    //! \brief Write data files
     /*  \param name file name */
    template<class MultiWriter>
    void addOutputVtkFields(MultiWriter &writer)
    {
        int size = problem_.gridView().size(0);
        ScalarSolutionType *potential = writer.allocateManagedBuffer(size);
        ScalarSolutionType *pressure = 0;
        ScalarSolutionType *pressureSecond = 0;
        ScalarSolutionType *potentialSecond = 0;
        Dune::BlockVector < DimVector > *velocityWetting = 0;
        Dune::BlockVector < DimVector > *velocityNonwetting = 0;

        if (vtkOutputLevel_ > 0)
        {
            pressure = writer.allocateManagedBuffer(size);
            pressureSecond = writer.allocateManagedBuffer(size);
            potentialSecond = writer.allocateManagedBuffer(size);
            velocityWetting = writer.template allocateManagedBuffer<Scalar, dim>(size);
            velocityNonwetting = writer.template allocateManagedBuffer<Scalar, dim>(size);
        }


            for (const auto& element : elements(problem_.gridView()))
            {
                int eIdxGlobal = problem_.variables().index(element);
                CellData& cellData = problem_.variables().cellData(eIdxGlobal);

                if (pressureType == pw)
                {
                    (*potential)[eIdxGlobal] = cellData.potential(wPhaseIdx);
                }

                if (pressureType == pn)
                {
                    (*potential)[eIdxGlobal] = cellData.potential(nPhaseIdx);
                }

                if (vtkOutputLevel_ > 0)
                {

                if (pressureType == pw)
                {
                    (*pressure)[eIdxGlobal] = cellData.pressure(wPhaseIdx);
                    (*potentialSecond)[eIdxGlobal] = cellData.potential(nPhaseIdx);
                    (*pressureSecond)[eIdxGlobal] = cellData.pressure(nPhaseIdx);
                }

                if (pressureType == pn)
                {
                    (*pressure)[eIdxGlobal] = cellData.pressure(nPhaseIdx);
                    (*potentialSecond)[eIdxGlobal] = cellData.potential(wPhaseIdx);
                    (*pressureSecond)[eIdxGlobal] = cellData.pressure(wPhaseIdx);
                }

                const typename Element::Geometry& geometry = element.geometry();
                // get corresponding reference element
                typedef Dune::ReferenceElements<Scalar, dim> ReferenceElements;
                const Dune::ReferenceElement< Scalar , dim > & refElement =
                        ReferenceElements::general( geometry.type() );
                const int numberOfFaces=refElement.size(1);

                std::vector<Scalar> fluxW(numberOfFaces,0);
                std::vector<Scalar> fluxNw(numberOfFaces,0);

                // run through all intersections with neighbors and boundary
                for (const auto& intersection : intersections(problem_.gridView(), element))
                {
                    int isIndex = intersection.indexInInside();

                    fluxW[isIndex] += intersection.geometry().volume()
                        * (intersection.centerUnitOuterNormal() * cellData.fluxData().velocity(wPhaseIdx, isIndex));
                    fluxNw[isIndex] += intersection.geometry().volume()
                        * (intersection.centerUnitOuterNormal() * cellData.fluxData().velocity(nPhaseIdx, isIndex));
                }

                // calculate velocity on reference element as the Raviart-Thomas-0
                // interpolant of the fluxes
                Dune::FieldVector<Scalar, dim> refVelocity;
                // simplices
                if (refElement.type().isSimplex()) {
                    for (int dimIdx = 0; dimIdx < dim; dimIdx++)
                    {
                        refVelocity[dimIdx] = -fluxW[dim - 1 - dimIdx];
                        for (int fIdx = 0; fIdx < dim + 1; fIdx++)
                        {
                            refVelocity[dimIdx] += fluxW[fIdx]/(dim + 1);
                        }
                    }
                }
                // cubes
                else if (refElement.type().isCube()){
                    for (int i = 0; i < dim; i++)
                        refVelocity[i] = 0.5 * (fluxW[2*i + 1] - fluxW[2*i]);
                }
                // 3D prism and pyramids
                else {
                    DUNE_THROW(Dune::NotImplemented, "velocity output for prism/pyramid not implemented");
                }

                const DimVector& localPos = refElement.position(0, 0);

                // get the transposed Jacobian of the element mapping
                const JacobianTransposed jacobianT = geometry.jacobianTransposed(localPos);

                // calculate the element velocity by the Piola transformation
                DimVector elementVelocity(0);
                jacobianT.umtv(refVelocity, elementVelocity);
                elementVelocity /= geometry.integrationElement(localPos);

                (*velocityWetting)[eIdxGlobal] = elementVelocity;

                // calculate velocity on reference element as the Raviart-Thomas-0
                // interpolant of the fluxes
                // simplices
                if (refElement.type().isSimplex()) {
                    for (int dimIdx = 0; dimIdx < dim; dimIdx++)
                    {
                        refVelocity[dimIdx] = -fluxNw[dim - 1 - dimIdx];
                        for (int fIdx = 0; fIdx < dim + 1; fIdx++)
                        {
                            refVelocity[dimIdx] += fluxNw[fIdx]/(dim + 1);
                        }
                    }
                }
                // cubes
                else if (refElement.type().isCube()){
                    for (int i = 0; i < dim; i++)
                        refVelocity[i] = 0.5 * (fluxNw[2*i + 1] - fluxNw[2*i]);
                }
                // 3D prism and pyramids
                else {
                    DUNE_THROW(Dune::NotImplemented, "velocity output for prism/pyramid not implemented");
                }

                // calculate the element velocity by the Piola transformation
                elementVelocity = 0;
                jacobianT.umtv(refVelocity, elementVelocity);
                elementVelocity /= geometry.integrationElement(localPos);

                (*velocityNonwetting)[eIdxGlobal] = elementVelocity;
                }
            }

            if (pressureType == pw)
            {
                writer.attachCellData(*potential, "wetting potential");
            }

            if (pressureType == pn)
            {
                writer.attachCellData(*potential, "nonwetting potential");
            }

            if (vtkOutputLevel_ > 0)
            {
            if (pressureType == pw)
            {
                writer.attachCellData(*pressure, "wetting pressure");
                writer.attachCellData(*pressureSecond, "nonwetting pressure");
                writer.attachCellData(*potentialSecond, "nonwetting potential");
            }

            if (pressureType == pn)
            {
                writer.attachCellData(*pressure, "nonwetting pressure");
                writer.attachCellData(*pressureSecond, "wetting pressure");
                writer.attachCellData(*potentialSecond, "wetting potential");
            }

            writer.attachCellData(*velocityWetting, "wetting-velocity", dim);
            writer.attachCellData(*velocityNonwetting, "non-wetting-velocity", dim);
            }
    }

    /*! \name general methods for serialization, output */
    //@{
    // serialization methods
    //! Function needed for restart option.
    void serializeEntity(std::ostream &outstream, const Element &element)
    {
        int numFaces = element.subEntities(1);
        for (int i=0; i < numFaces; i++)
        {
            int isIdxGlobal = A_.intersectionMapper().subIndex(element, i);
            outstream << pressTrace_[isIdxGlobal][0];
        }
    }

    void deserializeEntity(std::istream &instream, const Element &element)
    {
        int numFaces = element.subEntities(1);
        for (int i=0; i < numFaces; i++)
        {
            int isIdxGlobal = A_.intersectionMapper().subIndex(element, i);
            instream >> pressTrace_[isIdxGlobal][0];
        }
    }
    //@}

    //! Constructs a MimeticPressure2PAdaptive object
    /**
     * \param problem The Dumux problem
     */
    MimeticPressure2PAdaptive(Problem& problem) :
    problem_(problem),
    A_(problem.gridView()), lstiff_(problem_, false, problem_.gridView(), A_.intersectionMapper())
    {
        if (pressureType != pw && pressureType != pn)
        {
            DUNE_THROW(Dune::NotImplemented, "Pressure type not supported!");
        }
        if (saturationType != Sw && saturationType != Sn)
        {
            DUNE_THROW(Dune::NotImplemented, "Saturation type not supported!");
        }
        if (GET_PROP_VALUE(TypeTag, EnableCompressibility))
        {
            DUNE_THROW(Dune::NotImplemented, "Compressibility not supported!");
        }

        density_[wPhaseIdx] = 0.0;
        density_[nPhaseIdx] = 0.0;
        viscosity_[wPhaseIdx] = 0.0;
        viscosity_[nPhaseIdx] = 0.0;

        vtkOutputLevel_ = GET_PARAM_FROM_GROUP(TypeTag, int, Vtk, OutputLevel);
    }

private:
    Problem& problem_;
    TraceType pressTrace_; //!< vector of pressure traces
    TraceType f_;
    OperatorAssembler A_;
    LocalStiffness lstiff_;

    Scalar density_[numPhases];
    Scalar viscosity_[numPhases];

    int vtkOutputLevel_;
};

//solves the system of equations to get the spatial distribution of the pressure
template<class TypeTag>
void MimeticPressure2PAdaptive<TypeTag>::solve()
{
    typedef typename GET_PROP_TYPE(TypeTag, LinearSolver) Solver;

    int verboseLevelSolver = GET_PARAM_FROM_GROUP(TypeTag, int, LinearSolver, Verbosity);

    if (verboseLevelSolver)
    std::cout << "MimeticPressure2PAdaptive: solve for pressure" << std::endl;

    Solver solver(problem_);
    solver.solve(*A_, pressTrace_, f_);

//                                    printmatrix(std::cout, *A_, "global stiffness matrix", "row", 11, 3);
//                                    printvector(std::cout, f_, "right hand side", "row", 200, 1, 3);
//                printvector(std::cout, pressTrace_, "pressure", "row", 200, 1, 3);
    return;
}

//constitutive functions are updated once if new saturations are calculated and stored in the variables object
template<class TypeTag>
void MimeticPressure2PAdaptive<TypeTag>::updateMaterialLaws()
{
    // iterate through leaf grid an evaluate c0 at cell center
    for (const auto& element : elements(problem_.gridView()))
    {
        int eIdxGlobal = problem_.variables().index(element);

        CellData& cellData = problem_.variables().cellData(eIdxGlobal);

        Scalar satW = cellData.saturation(wPhaseIdx);

        // initialize mobilities
        Scalar mobilityW = MaterialLaw::krw(problem_.spatialParams().materialLawParams(element), satW)
                / viscosity_[wPhaseIdx];
        Scalar mobilityNw = MaterialLaw::krn(problem_.spatialParams().materialLawParams(element), satW)
                / viscosity_[nPhaseIdx];

        // initialize mobilities
        cellData.setMobility(wPhaseIdx, mobilityW);
        cellData.setMobility(nPhaseIdx, mobilityNw);

        //initialize fractional flow functions
        cellData.setFracFlowFunc(wPhaseIdx, mobilityW / (mobilityW + mobilityNw));
        cellData.setFracFlowFunc(nPhaseIdx, mobilityNw / (mobilityW + mobilityNw));
    }
    return;
}



}
#endif