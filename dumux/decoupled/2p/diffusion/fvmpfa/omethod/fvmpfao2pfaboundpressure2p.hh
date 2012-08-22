/*****************************************************************************
 *   Copyright (C) 2010 by Markus Wolff                                      *
 *   Institute for Modelling Hydraulic and Environmental Systems             *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@mathematik.uni-stuttgart.de                   *
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
#ifndef DUMUX_FVMPFAO2OFABOUNDPRESSURE2P_HH
#define DUMUX_FVMPFAO2OFABOUNDPRESSURE2P_HH

// dumux environment
#include <dumux/decoupled/common/fv/fvpressure.hh>
#include <dumux/decoupled/common/fv/mpfa/mpfaointeractionvolume.hh>
#include <dumux/decoupled/2p/diffusion/diffusionproperties2p.hh>
#include <dumux/decoupled/common/fv/mpfa/fvmpfaproperties.hh>

/**
 * @file
 * @brief  Finite volume MPFA O-method discretization of a two-phase pressure equation of the sequential IMPES model.
 * @author Markus Wolff
 */

namespace Dumux
{
//! \ingroup FVPressure2p
/*! \brief Finite volume MPFA O-method discretization of a two-phase flow pressure equation of the sequential IMPES model.
 *
 * Finite volume MPFA O-method discretization of the equations
 * \f[ - \text{div}\, \boldsymbol v_t = - \text{div}\, (\lambda_t \boldsymbol K \text{grad}\, \Phi_w + f_n \lambda_t \boldsymbol K \text{grad}\, \Phi_{cap}   ) = 0, \f]
 * or
 * \f[ - \text{div}\, \boldsymbol v_t = - \text{div}\, (\lambda_t \boldsymbol K \text{grad}\, \Phi_n - f_w \lambda_t \boldsymbol K \text{grad}\, \Phi_{cap}   ) = 0. \f]
 *  At Dirichlet boundaries a two-point flux approximation is used.
 * \f[ \Phi = g \;  \text{on} \; \Gamma_1, \quad \text{and} \quad
 * -\text{div}\, \boldsymbol v_t \cdot \mathbf{n} = J \;  \text{on}  \; \Gamma_2. \f]
 *  Here, \f$ \Phi_\alpha \f$ denotes the potential of phase \f$ \alpha \f$, \f$ \boldsymbol K \f$ the intrinsic permeability,
 * \f$ \lambda_t \f$ the total mobility, \f$ f_\alpha \f$ the phase fractional flow function.
 *
 * More details on the equations can be found in H. Hoteit, A. Firoozabadi.
 * Numerical modeling of thwo-phase flow in heterogeneous permeable media with different capillarity pressures. Adv Water Res (31), 2008
 *
 *  Remark1: only for 2-D quadrilateral grid
 *
 *  Remark2: implemented for UGGrid, ALUGrid, or SGrid/YaspGrid
 *
 *\tparam TypeTag The problem Type Tag
 */
template<class TypeTag>
class FVMPFAO2PFABoundPressure2P: public FVPressure<TypeTag>
{
    typedef FVPressure<TypeTag> ParentType;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;

    enum
    {
        dim = GridView::dimension, dimWorld = GridView::dimensionworld
    };

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;

    typedef Dune::GenericReferenceElements<Scalar, dim> ReferenceElementContainer;
    typedef Dune::GenericReferenceElement<Scalar, dim> ReferenceElement;

    typedef typename GET_PROP_TYPE(TypeTag, SpatialParams) SpatialParams;
    typedef typename SpatialParams::MaterialLaw MaterialLaw;

    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;

    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, FluidState) FluidState;

    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;
    typedef typename GET_PROP_TYPE(TypeTag, CellData) CellData;
    typedef typename GET_PROP(TypeTag, SolutionTypes) SolutionTypes;
    typedef typename SolutionTypes::PrimaryVariables PrimaryVariables;
    typedef typename SolutionTypes::ScalarSolution ScalarSolutionType;

    typedef typename GET_PROP_TYPE(TypeTag, GridTypeIndices) GridTypeIndices;

    enum
    {
        pw = Indices::pressureW,
        pn = Indices::pressureNW,
        pglobal = Indices::pressureGlobal,
        Sw = Indices::saturationW,
        Sn = Indices::saturationNW,
        vw = Indices::velocityW,
        vn = Indices::velocityNW,
        vt = Indices::velocityTotal
    };
    enum
    {
        wPhaseIdx = Indices::wPhaseIdx,
        nPhaseIdx = Indices::nPhaseIdx,
        pressureIdx = Indices::pressureIdx,
        saturationIdx = Indices::saturationIdx,
        pressEqIdx = Indices::pressEqIdx,
        satEqIdx = Indices::satEqIdx,
        numPhases = GET_PROP_VALUE(TypeTag, NumPhases)
    };
    enum
    {
        globalCorner = 2,
        globalEdge = 3,
        neumannNeumann = 0,
        dirichletDirichlet = 1,
        dirichletNeumann = 2,
        neumannDirichlet = 3
    };

    typedef typename GridView::Traits::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    typedef typename GridView::template Codim<dim>::Iterator VertexIterator;
    typedef typename GridView::Grid Grid;
    typedef typename GridView::template Codim<0>::EntityPointer ElementPointer;
    typedef typename GridView::IntersectionIterator IntersectionIterator;

    typedef Dune::FieldVector<Scalar, dim> LocalPosition;
    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;
    typedef Dune::FieldMatrix<Scalar, dim, dim> DimMatrix;

    typedef Dune::FieldVector<Scalar, dim> DimVector;

    typedef Dumux::FVMPFAOInteractionVolume<TypeTag> InteractionVolume;

    typedef std::vector<InteractionVolume> GlobalInteractionVolumeVector;
    typedef std::vector<Dune::FieldVector<bool, 2 * dim> > InnerBoundaryVolumeFaces;

    //initializes the matrix to store the system of equations
    friend class FVPressure<TypeTag>;
    void initializeMatrix();

    void storeInteractionVolumeInfo();

    //function which assembles the system of equations to be solved
    void assemble();
public:

    //constitutive functions are initialized and stored in the variables object
    void updateMaterialLaws();

    /*! \brief Updates interaction volumes
     *
     * Globally rebuilds the MPFA interaction volumes.
     *
     */
    void updateInteractionVolumeInfo()
    {
        interactionVolumes_.clear();
        innerBoundaryVolumeFaces_.clear();

        interactionVolumes_.resize(problem_.gridView().size(dim));
        innerBoundaryVolumeFaces_.resize(problem_.gridView().size(0), Dune::FieldVector<bool, 2 * dim>(false));

        storeInteractionVolumeInfo();
    }

    /*! \brief Initializes the pressure model
     *
     * \copydetails ParentType::initialize()
     *
     */
    void initialize()
    {
        ParentType::initialize();

        ElementIterator element = problem_.gridView().template begin<0>();
        FluidState fluidState;
        fluidState.setPressure(wPhaseIdx, problem_.referencePressure(*element));
        fluidState.setPressure(nPhaseIdx, problem_.referencePressure(*element));
        fluidState.setTemperature(problem_.temperature(*element));
        fluidState.setSaturation(wPhaseIdx, 1.);
        fluidState.setSaturation(nPhaseIdx, 0.);
        density_[wPhaseIdx] = FluidSystem::density(fluidState, wPhaseIdx);
        density_[nPhaseIdx] = FluidSystem::density(fluidState, nPhaseIdx);
        viscosity_[wPhaseIdx] = FluidSystem::viscosity(fluidState, wPhaseIdx);
        viscosity_[nPhaseIdx] = FluidSystem::viscosity(fluidState, nPhaseIdx);

        updateMaterialLaws();

        interactionVolumes_.resize(problem_.gridView().size(dim));
        innerBoundaryVolumeFaces_.resize(problem_.gridView().size(0), Dune::FieldVector<bool, 2 * dim>(false));

        storeInteractionVolumeInfo();

        assemble();
        this->solve();

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
        //error bounds for error term for incompressible models to correct unphysical saturation over/undershoots due to saturation transport
        timeStep_ = problem_.timeManager().timeStepSize();
        maxError_ = 0.0;
        int size = problem_.gridView().size(0);
        for (int i = 0; i < size; i++)
        {
            Scalar sat = 0;
            switch (saturationType_)
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
                maxError_ = std::max(maxError_, (sat - 1.0) / timeStep_);
            }
            if (sat < 0.0)
            {
                maxError_ = std::max(maxError_, (-sat) / timeStep_);
            }
        }

        assemble();
        this->solve();

        storePressureSolution();

        return;
    }

    /*! \brief Globally stores the pressure solution
     *
     */
    void storePressureSolution()
    {
        // iterate through leaf grid an evaluate c0 at cell center
        ElementIterator eItEnd = problem_.gridView().template end<0>();
        for (ElementIterator eIt = problem_.gridView().template begin<0>(); eIt != eItEnd; ++eIt)
        {
            storePressureSolution(*eIt);
        }
    }

    /*! \brief Stores the pressure solution of a cell
     *
     * \param element Dune grid element
     */
    void storePressureSolution(const Element& element)
    {
        int globalIdx = problem_.variables().index(element);
        CellData& cellData = problem_.variables().cellData(globalIdx);
        const GlobalPosition& globalPos = element.geometry().center();

        switch (pressureType_)
        {
        case pw:
        {
            Scalar potW = this->pressure()[globalIdx];
            Scalar potPc = cellData.capillaryPressure()
                    + (problem_.bboxMax() - globalPos) * gravity_ * (density_[nPhaseIdx] - density_[wPhaseIdx]);

            cellData.setPressure(wPhaseIdx, potW);
            cellData.setPressure(nPhaseIdx, potW + potPc);

            break;
        }
        case pn:
        {
            Scalar potNW = this->pressure()[globalIdx];
            Scalar potPc = cellData.capillaryPressure()
                    + (problem_.bboxMax() - globalPos) * gravity_ * (density_[nPhaseIdx] - density_[wPhaseIdx]);

            cellData.setPressure(nPhaseIdx, potNW);
            cellData.setPressure(wPhaseIdx, potNW - potPc);

            break;
        }
        }
        cellData.fluxData().resetVelocity();
    }

    /*! \brief Adds pressure output to the output file
     *
     * Adds the pressure, the potential and the capillary pressure to the output.
     * If the VtkOutputLevel is equal to zero (default) only primary variables are written,
     * if it is larger than zero also secondary variables are written.
     *
     * \tparam MultiWriter Class defining the output writer
     * \param writer The output writer (usually a <tt>VTKMultiWriter</tt> object)
     *
     */
    template<class MultiWriter>
    void addOutputVtkFields(MultiWriter &writer)
    {
        int size = problem_.gridView().size(0);
        ScalarSolutionType *potential = writer.allocateManagedBuffer(size);

        (*potential) = this->pressure();

        if (pressureType_ == pw)
        {
            writer.attachCellData(*potential, "wetting potential");
        }

        if (pressureType_ == pn)
        {
            writer.attachCellData(*potential, "nonwetting potential");
        }

        if (vtkOutputLevel_ > 0)
        {
            ScalarSolutionType *pressure = writer.allocateManagedBuffer(size);
            ScalarSolutionType *pressureSecond = writer.allocateManagedBuffer(size);
            ScalarSolutionType *potentialSecond = writer.allocateManagedBuffer(size);
            ScalarSolutionType *pC = writer.allocateManagedBuffer(size);

            ElementIterator eItBegin = problem_.gridView().template begin<0>();
            ElementIterator eItEnd = problem_.gridView().template end<0>();
            for (ElementIterator eIt = eItBegin; eIt != eItEnd; ++eIt)
            {
                int idx = problem_.variables().index(*eIt);
                CellData& cellData = problem_.variables().cellData(idx);

                (*pC)[idx] = cellData.capillaryPressure();

                if (pressureType_ == pw)
                {
                    (*pressure)[idx] = this->pressure()[idx][0]
                    - density_[wPhaseIdx] * (gravity_ * (problem_.bboxMax() - eIt->geometry().center()));
                    (*potentialSecond)[idx] = cellData.pressure(nPhaseIdx);
                    (*pressureSecond)[idx] = (*pressure)[idx] + cellData.capillaryPressure();
                }

                if (pressureType_ == pn)
                {
                    (*pressure)[idx] = this->pressure()[idx][0]
                    - density_[nPhaseIdx] * (gravity_ * (problem_.bboxMax() - eIt->geometry().center()));
                    (*potentialSecond)[idx] = cellData.pressure(wPhaseIdx);
                    (*pressureSecond)[idx] = (*pressure)[idx] - cellData.capillaryPressure();
                }
            }

            if (pressureType_ == pw)
            {
                writer.attachCellData(*pressure, "wetting pressure");
                writer.attachCellData(*pressureSecond, "nonwetting pressure");
                writer.attachCellData(*potentialSecond, "nonwetting potential");
            }

            if (pressureType_ == pn)
            {
                writer.attachCellData(*pressure, "nonwetting pressure");
                writer.attachCellData(*pressureSecond, "wetting pressure");
                writer.attachCellData(*potentialSecond, "wetting potential");
            }

            writer.attachCellData(*pC, "capillary pressure");
        }

        return;
    }

    //! Constructs a FVMPFAO2PFABoundPressure2P object
    /**
     * \param problem A problem class object
     */
    FVMPFAO2PFABoundPressure2P(Problem& problem) :
            ParentType(problem), problem_(problem), gravity_(problem.gravity()), maxError_(
                    0.), timeStep_(1.)
    {
        if (pressureType_ != pw && pressureType_ != pn)
        {
            DUNE_THROW(Dune::NotImplemented, "Pressure type not supported!");
        }
        if (saturationType_ != Sw && saturationType_ != Sn)
        {
            DUNE_THROW(Dune::NotImplemented, "Saturation type not supported!");
        }
        if (GET_PROP_VALUE(TypeTag, EnableCompressibility))
        {
            DUNE_THROW(Dune::NotImplemented, "Compressibility not supported!");
        }
        if (dim != 2)
        {
            DUNE_THROW(Dune::NotImplemented, "Dimension not supported!");
        }

        ErrorTermFactor_ = GET_PARAM(TypeTag, Scalar, ErrorTermFactor);
        ErrorTermLowerBound_ = GET_PARAM(TypeTag, Scalar, ErrorTermLowerBound);
        ErrorTermUpperBound_ = GET_PARAM(TypeTag, Scalar, ErrorTermUpperBound);

        density_[wPhaseIdx] = 0.;
        density_[nPhaseIdx] = 0.;
        viscosity_[wPhaseIdx] = 0.;
        viscosity_[nPhaseIdx] = 0.;

        vtkOutputLevel_ = GET_PARAM_FROM_GROUP(TypeTag, int, Vtk, OutputLevel);
    }

private:
    Problem& problem_;

    /* Volume correction term to correct for unphysical saturation overshoots/undershoots.
     * These can occur if the estimated time step for the explicit transport was too large. Correction by an artificial source term allows to correct
     * this errors due to wrong time-stepping without losing mass conservation. The error term looks as follows:
     * \f[
     *  q_{error} = \begin{cases}
     *          S < 0 & a_{error} \frac{S}{\Delta t} V \\
     *          S > 1 & a_{error} \frac{(S - 1)}{\Delta t} V \\
     *          0 \le S \le 1 & 0
     *      \end{cases}
     *  \f]
     *  where \f$a_{error}\f$ is a weighting factor (default: \f$a_{error} = 0.5\f$)
    */
    Scalar evaluateErrorTerm_(CellData& cellData)
    {
        //error term for incompressible models to correct unphysical saturation over/undershoots due to saturation transport
        // error reduction routine: volumetric error is damped and inserted to right hand side
        Scalar sat = 0;
        switch (saturationType_)
        {
        case Sw:
            sat = cellData.saturation(wPhaseIdx);
            break;
        case Sn:
            sat = cellData.saturation(nPhaseIdx);
            break;
        }

        Scalar error = (sat > 1.0) ? sat - 1.0 : 0.0;
        if (sat < 0.0)
        {
            error = sat;
        }
        error /= timeStep_;

        Scalar errorAbs = std::abs(error);

        if ((errorAbs * timeStep_ > 1e-6) && (errorAbs > ErrorTermLowerBound_ * maxError_)
                && (!problem_.timeManager().willBeFinished()))
        {
            return ErrorTermFactor_ * error;
        }
        return 0.0;
    }

protected:
    GlobalInteractionVolumeVector interactionVolumes_;
    InnerBoundaryVolumeFaces innerBoundaryVolumeFaces_;

private:
    const GlobalPosition& gravity_; //!< vector including the gravity constant

    Scalar maxError_;
    Scalar timeStep_;
    Scalar ErrorTermFactor_; //!< Handling of error term: relaxation factor
    Scalar ErrorTermLowerBound_; //!< Handling of error term: lower bound for error dampening
    Scalar ErrorTermUpperBound_; //!< Handling of error term: upper bound for error dampening

    Scalar density_[numPhases];
    Scalar viscosity_[numPhases];

    int vtkOutputLevel_;

    static constexpr Scalar threshold_ = 1e-15;
    static const int pressureType_ = GET_PROP_VALUE(TypeTag, PressureFormulation); //!< gives kind of pressure used (\f$ 0 = p_w\f$, \f$ 1 = p_n\f$, \f$ 2 = p_{global}\f$)
    static const int saturationType_ = GET_PROP_VALUE(TypeTag, SaturationFormulation); //!< gives kind of saturation used (\f$ 0 = S_w\f$, \f$ 1 = S_n\f$)
    static const int velocityType_ = GET_PROP_VALUE(TypeTag, VelocityFormulation); //!< gives kind of velocity used (\f$ 0 = v_w\f$, \f$ 1 = v_n\f$, \f$ 2 = v_t\f$)
};

template<class TypeTag>
void FVMPFAO2PFABoundPressure2P<TypeTag>::initializeMatrix()
{
    // determine matrix row sizes
    ElementIterator eItBegin = problem_.gridView().template begin<0>();
    ElementIterator eItEnd = problem_.gridView().template end<0>();
    for (ElementIterator eIt = eItBegin; eIt != eItEnd; ++eIt)
    {
        // cell index
        int globalIdxI = problem_.variables().index(*eIt);

        // initialize row size
        int rowSize = 1;

        // run through all intersections with neighbors
        IntersectionIterator isItBegin = problem_.gridView().ibegin(*eIt);
        IntersectionIterator isItEnd = problem_.gridView().iend(*eIt);
        for (IntersectionIterator isIt = isItBegin; isIt != isItEnd; ++isIt)
        {
            IntersectionIterator tempisIt = isIt;
            IntersectionIterator tempisItBegin = isItBegin;

            // 'nextIsIt' iterates over next codimension 1 intersection neighboring with 'isIt'
            IntersectionIterator nextIsIt = ++tempisIt;

            // get 'nextIsIt'
            switch (GET_PROP_VALUE(TypeTag, GridImplementation))
            {
            // for SGrid
            case GridTypeIndices::sGrid:
            {
                if (nextIsIt == isItEnd)
                    nextIsIt = isItBegin;
                else
                {
                    nextIsIt = ++tempisIt;

                    if (nextIsIt == isItEnd)
                    {
                        nextIsIt = ++tempisItBegin;
                    }
                }

                break;
            }
                // for YaspGrid
            case GridTypeIndices::yaspGrid:
            {
                if (nextIsIt == isItEnd)
                {
                    nextIsIt = isItBegin;
                }
                else
                {
                    nextIsIt = ++tempisIt;

                    if (nextIsIt == isItEnd)
                    {
                        nextIsIt = ++tempisItBegin;
                    }
                }

                break;
            }
                // for ALUGrid
            case GridTypeIndices::aluGrid:
            {
                if (nextIsIt == isItEnd)
                    nextIsIt = isItBegin;

                break;
            }
                // for UGGrid
            case GridTypeIndices::ugGrid:
            {
                if (nextIsIt == isItEnd)
                    nextIsIt = isItBegin;

                break;
            }
            default:
            {
                DUNE_THROW(Dune::NotImplemented, "GridType can not be used with MPFAO implementation!");
                break;
            }
            }

            if (isIt->neighbor())
                rowSize++;

            if (isIt->neighbor() && nextIsIt->neighbor())
            {
                // access the common neighbor of isIt's and nextIsIt's outside
                ElementPointer outside = isIt->outside();
                ElementPointer nextisItoutside = nextIsIt->outside();

                IntersectionIterator innerisItEnd = problem_.gridView().iend(*outside);
                IntersectionIterator innernextisItEnd = problem_.gridView().iend(*nextisItoutside);

                for (IntersectionIterator innerisIt = problem_.gridView().ibegin(*outside); innerisIt != innerisItEnd;
                        ++innerisIt)
                    for (IntersectionIterator innernextisIt = problem_.gridView().ibegin(*nextisItoutside);
                            innernextisIt != innernextisItEnd; ++innernextisIt)
                    {
                        if (innerisIt->neighbor() && innernextisIt->neighbor())
                        {
                            ElementPointer innerisItoutside = innerisIt->outside();
                            ElementPointer innernextisItoutside = innernextisIt->outside();

                            if (innerisItoutside == innernextisItoutside && innerisItoutside != isIt->inside())
                            {
                                rowSize++;
                            }
                        }
                    }
            }

        } // end of 'for' IntersectionIterator

        // set number of indices in row globalIdxI to rowSize
        this->A_.setrowsize(globalIdxI, rowSize);

    } // end of 'for' ElementIterator

    // indicate that size of all rows is defined
    this->A_.endrowsizes();

    // determine position of matrix entries
    for (ElementIterator eIt = eItBegin; eIt != eItEnd; ++eIt)
    {
        // cell index
        int globalIdxI = problem_.variables().index(*eIt);

        // add diagonal index
        this->A_.addindex(globalIdxI, globalIdxI);

        // run through all intersections with neighbors
        IntersectionIterator isItBegin = problem_.gridView().ibegin(*eIt);
        IntersectionIterator isItEnd = problem_.gridView().iend(*eIt);
        for (IntersectionIterator isIt = isItBegin; isIt != isItEnd; ++isIt)
        {
            IntersectionIterator tempisIt = isIt;
            IntersectionIterator tempisItBegin = isItBegin;

            // 'nextIsIt' iterates over next codimension 1 intersection neighboring with 'isIt'
            // sequence of next is anticlockwise of 'isIt'
            IntersectionIterator nextIsIt = ++tempisIt;

            // get 'nextIsIt'
            switch (GET_PROP_VALUE(TypeTag, GridImplementation))
            {
            // for SGrid
            case GridTypeIndices::sGrid:
            {
                if (nextIsIt == isItEnd)
                {
                    nextIsIt = isItBegin;
                }
                else
                {
                    nextIsIt = ++tempisIt;

                    if (nextIsIt == isItEnd)
                    {
                        nextIsIt = ++tempisItBegin;
                    }
                }

                break;
            }
                // for YaspGrid
            case GridTypeIndices::yaspGrid:
            {
                if (nextIsIt == isItEnd)
                {
                    nextIsIt = isItBegin;
                }
                else
                {
                    nextIsIt = ++tempisIt;

                    if (nextIsIt == isItEnd)
                    {
                        nextIsIt = ++tempisItBegin;
                    }
                }

                break;
            }
                // for ALUGrid
            case GridTypeIndices::aluGrid:
            {
                if (nextIsIt == isItEnd)
                    nextIsIt = isItBegin;

                break;
            }
                // for UGGrid
            case GridTypeIndices::ugGrid:
            {
                if (nextIsIt == isItEnd)
                    nextIsIt = isItBegin;

                break;
            }
            default:
            {
                DUNE_THROW(Dune::NotImplemented, "GridType can not be used with MPFAO implementation!");
                break;
            }
            }

            if (isIt->neighbor())
            {
                // access neighbor
                ElementPointer outside = isIt->outside();
                int globalIdxJ = problem_.variables().index(*outside);

                // add off diagonal index
                // add index (row,col) to the matrix
                this->A_.addindex(globalIdxI, globalIdxJ);
            }

            if (isIt->neighbor() && nextIsIt->neighbor())
            {
                // access the common neighbor of isIt's and nextIsIt's outside
                ElementPointer outside = isIt->outside();
                ElementPointer nextisItoutside = nextIsIt->outside();

                IntersectionIterator innerisItEnd = problem_.gridView().iend(*outside);
                IntersectionIterator innernextisItEnd = problem_.gridView().iend(*nextisItoutside);

                for (IntersectionIterator innerisIt = problem_.gridView().ibegin(*outside); innerisIt != innerisItEnd;
                        ++innerisIt)
                    for (IntersectionIterator innernextisIt = problem_.gridView().ibegin(*nextisItoutside);
                            innernextisIt != innernextisItEnd; ++innernextisIt)
                    {
                        if (innerisIt->neighbor() && innernextisIt->neighbor())
                        {
                            ElementPointer innerisItoutside = innerisIt->outside();
                            ElementPointer innernextisItoutside = innernextisIt->outside();

                            if (innerisItoutside == innernextisItoutside && innerisItoutside != isIt->inside())
                            {
                                int globalIdxJ = problem_.variables().index(*innerisItoutside);

                                this->A_.addindex(globalIdxI, globalIdxJ);
                            }
                        }
                    }
            }
        } // end of 'for' IntersectionIterator
    } // end of 'for' ElementIterator

    // indicate that all indices are defined, check consistency
    this->A_.endindices();

    return;
}
//                 Indices used in a interaction volume of the MPFA-o method
//                 ___________________________________________________
//                 |                        |                        |
//                 | nuxy: cell geometry    |       nxy: face normal |
//                 |     vectors (see MPFA) |                        |
//                 |                        |                        |
//                 |            4-----------3-----------3            |
//                 |            | --> nu43  |  nu34 <-- |            |
//                 |            | |nu41    1|--> n43   ||nu32        |
//                 |            | v   ^     |0     ^   v|            |
//                 |____________4__0__|n14__|__n23_|_1__2____________|
//                 |            |    1    0 |     0     |
//                 |            | ^         |1   nu23 ^ |            |
//                 |            | |nu14    0|--> n12  | |            |
//                 |            | -->nu12   |   nu21<-- |            |
//                 |            1-----------1-----------2            |
//                 |          elementnumber |inter-                  |
//                 |                        |face-                   |
//                 |                        |number                  |
//                 |________________________|________________________|

// only for 2-D general quadrilateral
template<class TypeTag>
void FVMPFAO2PFABoundPressure2P<TypeTag>::storeInteractionVolumeInfo()
{
    // introduce matrix R for vector rotation and R is initialized as zero matrix
    DimMatrix R(0);

    // evaluate matrix R
    if (dim == 2)
    {
        R[0][1] = 1;
        R[1][0] = -1;
    }

    BoundaryTypes bcType;

    // run through all elements
    ElementIterator eItEnd = problem_.gridView().template end<0>();
    for (ElementIterator eIt = problem_.gridView().template begin<0>(); eIt != eItEnd; ++eIt)
    {
        // get common geometry information for the following computation

        int globalIdx1 = problem_.variables().index(*eIt);
        // get global coordinate of cell 1 center
        const GlobalPosition& globalPos1 = eIt->geometry().center();

        // get absolute permeability of neighbor cell 1
        DimMatrix K1(problem_.spatialParams().intrinsicPermeability(*eIt));

        IntersectionIterator isIt12Begin = problem_.gridView().ibegin(*eIt);
        IntersectionIterator isIt12End = problem_.gridView().iend(*eIt);
        for (IntersectionIterator isIt12 = isIt12Begin; isIt12 != isIt12End; ++isIt12)
        {
            // intersection iterator 'nextIsIt' is used to get geometry information
            IntersectionIterator tempIsIt = isIt12;
            IntersectionIterator tempIsItBegin = isIt12Begin;

            IntersectionIterator isIt14 = ++tempIsIt;

            //get isIt14
            switch (GET_PROP_VALUE(TypeTag, GridImplementation))
            {
            // for SGrid
            case GridTypeIndices::sGrid:
            {
                if (isIt14 == isIt12End)
                {
                    isIt14 = isIt12Begin;
                }
                else
                {
                    isIt14 = ++tempIsIt;

                    if (isIt14 == isIt12End)
                    {
                        isIt14 = ++tempIsItBegin;
                    }
                }

                break;
            }
                // for YaspGrid
            case GridTypeIndices::yaspGrid:
            {
                if (isIt14 == isIt12End)
                {
                    isIt14 = isIt12Begin;
                }
                else
                {
                    isIt14 = ++tempIsIt;

                    if (isIt14 == isIt12End)
                    {
                        isIt14 = ++tempIsItBegin;
                    }
                }

                break;
            }
                // for ALUGrid
            case GridTypeIndices::aluGrid:
            {
                if (isIt14 == isIt12End)
                    isIt14 = isIt12Begin;

                break;
            }
                // for UGGrid
            case GridTypeIndices::ugGrid:
            {
                if (isIt14 == isIt12End)
                    isIt14 = isIt12Begin;

                break;
            }
            default:
            {
                DUNE_THROW(Dune::NotImplemented, "GridType can not be used with MPFAO implementation!");
                break;
            }
            }

            int indexInInside12 = isIt12->indexInInside();
            int indexInInside14 = isIt14->indexInInside();

            // get the intersection node /bar^{x_3} between 'isIt12' and 'isIt14', denoted as 'corner1234'
            // initialization of corner1234

            const ReferenceElement& referenceElement = ReferenceElementContainer::general(eIt->geometry().type());

            GlobalPosition corner1234(0);

            int globalVertIdx1234 = 0;

            // get the global coordinate and global vertex index of corner1234
            for (int i = 0; i < isIt12->geometry().corners(); ++i)
            {
                bool finished = false;

                const GlobalPosition& isIt12corner = isIt12->geometry().corner(i);

                int localVertIdx12corner = referenceElement.subEntity(indexInInside12, dim - 1, i, dim);

                int globalVertIdx12corner = problem_.variables().index(
                        *((*eIt).template subEntity < dim > (localVertIdx12corner)));

                for (int j = 0; j < isIt14->geometry().corners(); ++j)
                {
                    int localVertIdx14corner = referenceElement.subEntity(indexInInside14, dim - 1, j, dim);

                    int globalVertIdx14corner = problem_.variables().index(
                            *((*eIt).template subEntity < dim > (localVertIdx14corner)));

                    if (globalVertIdx12corner == globalVertIdx14corner)
                    {
                        corner1234 = isIt12corner;

                        globalVertIdx1234 = globalVertIdx12corner;

                        finished = true;
                        break;
                    }
                }

                if (finished)
                {
                    break;
                }
            }

            if (interactionVolumes_[globalVertIdx1234].isStored())
            {
                continue;
            }
            else
            {
                interactionVolumes_[globalVertIdx1234].setStored();
                //                std::cout << "vertIdx = " << globalVertIdx1234 << "\n";
            }

            //store pointer 1
            interactionVolumes_[globalVertIdx1234].setSubVolumeElement(*eIt, 0);
            interactionVolumes_[globalVertIdx1234].setIndexOnElement(isIt12->indexInInside(), 0, 0);
            interactionVolumes_[globalVertIdx1234].setIndexOnElement(isIt14->indexInInside(), 0, 1);

            // center of face in global coordinates, i.e., the midpoint of edge 'isIt12'
            const GlobalPosition& globalPosFace12 = isIt12->geometry().center();

            // get face volume
            Scalar faceVol12 = isIt12->geometry().volume() / 2.0;

            // get outer normal vector scaled with half volume of face 'isIt12'
            DimVector unitOuterNormal12 = isIt12->centerUnitOuterNormal();

            // center of face in global coordinates, i.e., the midpoint of edge 'isIt14'
            const GlobalPosition& globalPosFace41 = isIt14->geometry().center();

            // get face volume
            Scalar faceVol41 = isIt14->geometry().volume() / 2.0;

            // get outer normal vector scaled with half volume of face 'isIt14': for numbering of n see Aavatsmark, Eigestad
            DimVector unitOuterNormal14 = isIt14->centerUnitOuterNormal();

            // compute normal vectors nu14,nu12
            DimVector nu14(0);
            R.mv(globalPos1 - globalPosFace12, nu14);

            DimVector nu12(0);
            R.mv(globalPosFace41 - globalPos1, nu12);

            interactionVolumes_[globalVertIdx1234].setPermTimesNu(nu12, K1, 0, 0);
            interactionVolumes_[globalVertIdx1234].setPermTimesNu(nu14, K1, 0, 1);
            interactionVolumes_[globalVertIdx1234].setNormal(unitOuterNormal12, 0, 0);
            interactionVolumes_[globalVertIdx1234].setNormal(unitOuterNormal14, 0, 1);
            interactionVolumes_[globalVertIdx1234].setFaceArea(faceVol12, 0, 0);
            interactionVolumes_[globalVertIdx1234].setFaceArea(faceVol41, 0, 1);

            // compute dF1, the area of quadrilateral made by normal vectors 'nu'
            DimVector Rnu12(0);
            R.umv(nu12, Rnu12);
            interactionVolumes_[globalVertIdx1234].setDF(fabs(nu14 * Rnu12), 0);

            // handle interior face
            if (isIt12->neighbor())
            {
                // access neighbor cell 2 of 'isIt12'
                ElementPointer elementPointer2 = isIt12->outside();

                int globalIdx2 = problem_.variables().index(*elementPointer2);

                //store pointer 2
                interactionVolumes_[globalVertIdx1234].setSubVolumeElement(elementPointer2, 1);
                interactionVolumes_[globalVertIdx1234].setIndexOnElement(isIt12->indexInOutside(), 1, 1);

                interactionVolumes_[globalVertIdx1234].setNormal(unitOuterNormal12, 1, 1);
                interactionVolumes_[globalVertIdx1234].setFaceArea(faceVol12, 1, 1);

                // get global coordinate of neighbor cell 2 center
                const GlobalPosition& globalPos2 = elementPointer2->geometry().center();

                // get absolute permeability of neighbor cell 2
                DimMatrix K2(problem_.spatialParams().intrinsicPermeability(*elementPointer2));

                // 'isIt14' is an interior face
                if (isIt14->neighbor())
                {
                    // neighbor cell 3
                    // access neighbor cell 3
                    ElementPointer elementPointer4 = isIt14->outside();

                    //store pointer 4
                    interactionVolumes_[globalVertIdx1234].setSubVolumeElement(elementPointer4, 3);
                    interactionVolumes_[globalVertIdx1234].setIndexOnElement(isIt14->indexInOutside(), 3, 0);

                    interactionVolumes_[globalVertIdx1234].setNormal(unitOuterNormal14, 3, 0);
                    interactionVolumes_[globalVertIdx1234].setFaceArea(faceVol41, 3, 0);

                    // get basic information of cell 1,2's neighbor cell 3,4

                    // get global coordinate of neighbor cell 4 center
                    const GlobalPosition& globalPos4 = elementPointer4->geometry().center();

                    // get absolute permeability of neighbor cell 2
                    DimMatrix K4(problem_.spatialParams().intrinsicPermeability(*elementPointer4));

                    // cell 3
                    GlobalPosition globalPos3(0);
                    int globalIdx3 = 0;

                    GlobalPosition globalPosFace23(0);
                    GlobalPosition globalPosFace34(0);

                    IntersectionIterator isIt2End = problem_.gridView().iend(*elementPointer2);
                    IntersectionIterator isIt4End = problem_.gridView().iend(*elementPointer4);
                    for (IntersectionIterator isIt2 = problem_.gridView().ibegin(*elementPointer2); isIt2 != isIt2End;
                            ++isIt2)
                    {
                        bool finished = false;

                        for (IntersectionIterator isIt4 = problem_.gridView().ibegin(*elementPointer4);
                                isIt4 != isIt4End; ++isIt4)
                        {
                            if (isIt2->neighbor() && isIt4->neighbor())
                            {
                                ElementPointer elementPointer32 = isIt2->outside();
                                ElementPointer elementPointer34 = isIt4->outside();

                                // find the common neighbor cell between cell 2 and cell 3, except cell 1
                                if (elementPointer32 == elementPointer34 && elementPointer32 != eIt)
                                {
                                    //store pointer 3
                                    interactionVolumes_[globalVertIdx1234].setSubVolumeElement(elementPointer32, 2);

                                    interactionVolumes_[globalVertIdx1234].setIndexOnElement(isIt2->indexInInside(), 1,
                                            0);
                                    interactionVolumes_[globalVertIdx1234].setIndexOnElement(isIt2->indexInOutside(), 2,
                                            1);
                                    interactionVolumes_[globalVertIdx1234].setIndexOnElement(isIt4->indexInInside(), 3,
                                            1);
                                    interactionVolumes_[globalVertIdx1234].setIndexOnElement(isIt4->indexInOutside(), 2,
                                            0);

                                    // access neighbor cell 4
                                    globalIdx3 = problem_.variables().index(*elementPointer32);

                                    // get global coordinate of neighbor cell 4 center
                                    globalPos3 = elementPointer32->geometry().center();

                                    globalPosFace23 = isIt2->geometry().center();
                                    globalPosFace34 = isIt4->geometry().center();

                                    Scalar faceVol23 = isIt2->geometry().volume() / 2.0;
                                    Scalar faceVol34 = isIt4->geometry().volume() / 2.0;

                                    // get outer normal vector scaled with half volume of face : for numbering of n see Aavatsmark, Eigestad
                                    DimVector unitOuterNormal23 = isIt2->centerUnitOuterNormal();

                                    DimVector unitOuterNormal43 = isIt4->centerUnitOuterNormal();

                                    interactionVolumes_[globalVertIdx1234].setNormal(unitOuterNormal23, 1, 0);
                                    interactionVolumes_[globalVertIdx1234].setNormal(unitOuterNormal23, 2, 1);
                                    interactionVolumes_[globalVertIdx1234].setNormal(unitOuterNormal43, 2, 0);
                                    interactionVolumes_[globalVertIdx1234].setNormal(unitOuterNormal43, 3, 1);
                                    interactionVolumes_[globalVertIdx1234].setFaceArea(faceVol23, 1, 0);
                                    interactionVolumes_[globalVertIdx1234].setFaceArea(faceVol23, 2, 1);
                                    interactionVolumes_[globalVertIdx1234].setFaceArea(faceVol34, 2, 0);
                                    interactionVolumes_[globalVertIdx1234].setFaceArea(faceVol34, 3, 1);

                                    // get absolute permeability of neighbor cell 2
                                    DimMatrix K3(
                                            problem_.spatialParams().intrinsicPermeability(*elementPointer32));

                                    // compute normal vectors nu23, nu21; nu32, nu34; nu41, nu43;
                                    DimVector nu23(0);
                                    R.umv(globalPosFace12 - globalPos2, nu23);

                                    DimVector nu21(0);
                                    R.umv(globalPosFace23 - globalPos2, nu21);

                                    DimVector nu32(0);
                                    R.umv(globalPosFace34 - globalPos3, nu32);

                                    DimVector nu34(0);
                                    R.umv(globalPos3 - globalPosFace23, nu34);

                                    DimVector nu41(0);
                                    R.umv(globalPos4 - globalPosFace34, nu41);

                                    DimVector nu43(0);
                                    R.umv(globalPos4 - globalPosFace41, nu43);

                                    interactionVolumes_[globalVertIdx1234].setPermTimesNu(nu23, K2, 1, 0);
                                    interactionVolumes_[globalVertIdx1234].setPermTimesNu(nu21, K2, 1, 1);
                                    interactionVolumes_[globalVertIdx1234].setPermTimesNu(nu34, K3, 2, 0);
                                    interactionVolumes_[globalVertIdx1234].setPermTimesNu(nu32, K3, 2, 1);
                                    interactionVolumes_[globalVertIdx1234].setPermTimesNu(nu41, K4, 3, 0);
                                    interactionVolumes_[globalVertIdx1234].setPermTimesNu(nu43, K4, 3, 1);

                                    // compute dF2, dF3, dF4 i.e., the area of quadrilateral made by normal vectors 'nu'
                                    DimVector Rnu21(0);
                                    R.umv(nu21, Rnu21);
                                    interactionVolumes_[globalVertIdx1234].setDF(fabs(nu23 * Rnu21), 1);

                                    DimVector Rnu34(0);
                                    R.umv(nu34, Rnu34);
                                    interactionVolumes_[globalVertIdx1234].setDF(fabs(nu32 * Rnu34), 2);

                                    DimVector Rnu43(0);
                                    R.umv(nu43, Rnu43);
                                    interactionVolumes_[globalVertIdx1234].setDF(fabs(nu41 * Rnu43), 3);

                                    finished = true;

                                    break;
                                }
                            }
                        }
                        if (finished)
                        {
                            break;
                        }
                    }
                }

                // 'isIt14' is on the boundary
                else
                {
                    problem_.boundaryTypes(bcType, *isIt14);
                    PrimaryVariables boundValues(0.0);

                    interactionVolumes_[globalVertIdx1234].setBoundary(bcType, 3);
                    if (bcType.isNeumann(pressEqIdx))
                    {
                        problem_.neumann(boundValues, *isIt14);
                        boundValues *= faceVol41;
                        interactionVolumes_[globalVertIdx1234].setNeumannCondition(boundValues, 3);
                    }
                    if (bcType.hasDirichlet())
                    {
                        problem_.dirichlet(boundValues, *isIt14);
                        interactionVolumes_[globalVertIdx1234].setDirichletCondition(boundValues, 3);
                    }

                    // get common geometry information for the following computation
                    // get the information of the face 'isIt23' between cell2 and cell4 (locally numbered)

                    // center of face in global coordinates, i.e., the midpoint of edge 'isIt23'
                    GlobalPosition globalPosFace23(0);

                    // get face volume
                    Scalar faceVol23 = 0;

                    // get outer normal vector scaled with half volume of face 'isIt23'
                    DimVector unitOuterNormal23(0);

                    bool finished = false;

                    IntersectionIterator isIt2End = problem_.gridView().iend(*elementPointer2);
                    for (IntersectionIterator isIt2 = problem_.gridView().ibegin(*elementPointer2); isIt2 != isIt2End;
                            ++isIt2)
                    {
                        if (isIt2->boundary())
                        {
                            for (int i = 0; i < isIt2->geometry().corners(); ++i)
                            {
                                int localVertIdx2corner = referenceElement.subEntity(isIt2->indexInInside(), dim - 1, i,
                                        dim);

                                int globalVertIdx2corner = problem_.variables().index(
                                        *((*elementPointer2).template subEntity < dim > (localVertIdx2corner)));

                                if (globalVertIdx2corner == globalVertIdx1234)
                                {
                                    interactionVolumes_[globalVertIdx1234].setIndexOnElement(isIt2->indexInInside(), 1,
                                            0);

                                    globalPosFace23 = isIt2->geometry().center();

                                    faceVol23 = isIt2->geometry().volume() / 2.0;

                                    unitOuterNormal23 = isIt2->centerUnitOuterNormal();

                                    interactionVolumes_[globalVertIdx1234].setNormal(unitOuterNormal23, 1, 0);
                                    interactionVolumes_[globalVertIdx1234].setFaceArea(faceVol23, 1, 0);

                                    problem_.boundaryTypes(bcType, *isIt2);
                                    PrimaryVariables boundValues(0.0);

                                    interactionVolumes_[globalVertIdx1234].setBoundary(bcType, 1);
                                    if (bcType.isNeumann(pressEqIdx))
                                    {
                                        problem_.neumann(boundValues, *isIt2);
                                        boundValues *= faceVol23;
                                        interactionVolumes_[globalVertIdx1234].setNeumannCondition(boundValues, 1);
                                    }
                                    if (bcType.hasDirichlet())
                                    {
                                        problem_.dirichlet(boundValues, *isIt2);
                                        interactionVolumes_[globalVertIdx1234].setDirichletCondition(boundValues, 1);
                                    }

                                    interactionVolumes_[globalVertIdx1234].setOutsideFace(2);

                                    innerBoundaryVolumeFaces_[globalIdx1][isIt12->indexInInside()] = true;
                                    innerBoundaryVolumeFaces_[globalIdx2][isIt12->indexInOutside()] = true;

                                    // compute normal vectors nu23, nu21;
                                    DimVector nu23(0);
                                    R.umv(globalPosFace12 - globalPos2, nu23);

                                    DimVector nu21(0);
                                    R.umv(globalPosFace23 - globalPos2, nu21);

                                    interactionVolumes_[globalVertIdx1234].setPermTimesNu(nu23, K2, 1, 0);
                                    interactionVolumes_[globalVertIdx1234].setPermTimesNu(nu21, K2, 1, 1);

                                    // compute dF2 i.e., the area of quadrilateral made by normal vectors 'nu'
                                    DimVector Rnu21(0);
                                    R.umv(nu21, Rnu21);
                                    interactionVolumes_[globalVertIdx1234].setDF(fabs(nu23 * Rnu21), 1);

                                    finished = true;

                                    break;
                                }
                            }
                        }
                        if (finished)
                        {
                            break;
                        }
                    }
                    if (!finished)
                    {
                        DUNE_THROW(
                                Dune::NotImplemented,
                                "fvmpfao2pfaboundpressure2p.hh, l. 997: boundary shape not available as interaction volume shape");
                    }
                }
            }

            // handle boundary face 'isIt12'
            else
            {
                problem_.boundaryTypes(bcType, *isIt12);
                PrimaryVariables boundValues(0.0);

                interactionVolumes_[globalVertIdx1234].setBoundary(bcType, 0);
                if (bcType.isNeumann(pressEqIdx))
                {
                    problem_.neumann(boundValues, *isIt12);
                    boundValues *= faceVol12;
                    interactionVolumes_[globalVertIdx1234].setNeumannCondition(boundValues, 0);
                }
                if (bcType.hasDirichlet())
                {
                    problem_.dirichlet(boundValues, *isIt12);
                    interactionVolumes_[globalVertIdx1234].setDirichletCondition(boundValues, 0);
                }

                // 'isIt14' is on boundary
                if (isIt14->boundary())
                {
                    problem_.boundaryTypes(bcType, *isIt14);
                    PrimaryVariables boundValues(0.0);

                    interactionVolumes_[globalVertIdx1234].setBoundary(bcType, 3);
                    if (bcType.isNeumann(pressEqIdx))
                    {
                        problem_.neumann(boundValues, *isIt14);
                        boundValues *= faceVol41;
                        interactionVolumes_[globalVertIdx1234].setNeumannCondition(boundValues, 3);
                    }
                    if (bcType.hasDirichlet())
                    {
                        problem_.dirichlet(boundValues, *isIt14);
                        interactionVolumes_[globalVertIdx1234].setDirichletCondition(boundValues, 3);
                    }

                    interactionVolumes_[globalVertIdx1234].setOutsideFace(1);
                    interactionVolumes_[globalVertIdx1234].setOutsideFace(2);
                }

                // 'isIt14' is inside
                else
                {
                    // neighbor cell 3
                    // access neighbor cell 3
                    ElementPointer elementPointer4 = isIt14->outside();
                    interactionVolumes_[globalVertIdx1234].setIndexOnElement(isIt14->indexInOutside(), 3, 0);

                    //store pointer 4
                    interactionVolumes_[globalVertIdx1234].setSubVolumeElement(elementPointer4, 3);

                    interactionVolumes_[globalVertIdx1234].setNormal(unitOuterNormal14, 3, 0);
                    interactionVolumes_[globalVertIdx1234].setFaceArea(faceVol41, 3, 0);

                    // get global coordinate of neighbor cell 3 center
                    const GlobalPosition& globalPos4 = elementPointer4->geometry().center();

                    int globalIdx4 = problem_.variables().index(*elementPointer4);

                    bool finished = false;

                    // get the information of the face 'isIt34' between cell3 and cell4 (locally numbered)
                    IntersectionIterator isIt4End = problem_.gridView().iend(*elementPointer4);
                    for (IntersectionIterator isIt4 = problem_.gridView().ibegin(*elementPointer4); isIt4 != isIt4End;
                            ++isIt4)
                    {
                        if (isIt4->boundary())
                        {
                            for (int i = 0; i < isIt4->geometry().corners(); ++i)
                            {
                                int localVertIdx4corner = referenceElement.subEntity(isIt4->indexInInside(), dim - 1, i,
                                        dim);

                                int globalVertIdx4corner = problem_.variables().index(
                                        *((*elementPointer4).template subEntity < dim > (localVertIdx4corner)));

                                if (globalVertIdx4corner == globalVertIdx1234)
                                {
                                    interactionVolumes_[globalVertIdx1234].setIndexOnElement(isIt4->indexInInside(), 3,
                                            1);

                                    const GlobalPosition& globalPosFace34 = isIt4->geometry().center();

                                    Scalar faceVol34 = isIt4->geometry().volume() / 2.0;

                                    DimVector unitOuterNormal43 = isIt4->centerUnitOuterNormal();

                                    interactionVolumes_[globalVertIdx1234].setNormal(unitOuterNormal43, 3, 1);
                                    interactionVolumes_[globalVertIdx1234].setFaceArea(faceVol34, 3, 1);

                                    problem_.boundaryTypes(bcType, *isIt4);
                                    PrimaryVariables boundValues(0.0);

                                    interactionVolumes_[globalVertIdx1234].setBoundary(bcType, 2);
                                    if (bcType.isNeumann(pressEqIdx))
                                    {
                                        problem_.neumann(boundValues, *isIt4);
                                        boundValues *= faceVol34;
                                        interactionVolumes_[globalVertIdx1234].setNeumannCondition(boundValues, 2);
                                    }
                                    if (bcType.hasDirichlet())
                                    {
                                        problem_.dirichlet(boundValues, *isIt4);
                                        interactionVolumes_[globalVertIdx1234].setDirichletCondition(boundValues, 2);
                                    }

                                    interactionVolumes_[globalVertIdx1234].setOutsideFace(1);

                                    innerBoundaryVolumeFaces_[globalIdx1][isIt14->indexInInside()] = true;
                                    innerBoundaryVolumeFaces_[globalIdx4][isIt14->indexInOutside()] = true;

                                    // get absolute permeability of neighbor cell 2
                                    DimMatrix K4(
                                            problem_.spatialParams().intrinsicPermeability(*elementPointer4));

                                    // compute normal vectors nu41, nu43;
                                    DimVector nu41(0);
                                    R.umv(globalPos4 - globalPosFace34, nu41);

                                    DimVector nu43(0);
                                    R.umv(globalPos4 - globalPosFace41, nu43);

                                    interactionVolumes_[globalVertIdx1234].setPermTimesNu(nu41, K4, 3, 0);
                                    interactionVolumes_[globalVertIdx1234].setPermTimesNu(nu43, K4, 3, 1);

                                    // compute dF1, dF3 i.e., the area of quadrilateral made by normal vectors 'nu'
                                    DimVector Rnu43(0);
                                    R.umv(nu43, Rnu43);
                                    interactionVolumes_[globalVertIdx1234].setDF(fabs(nu41 * Rnu43), 3);

                                    finished = true;

                                    break;
                                }
                            }
                        }
                        if (finished)
                        {
                            break;
                        }
                    }
                    if (!finished)
                    {
                        DUNE_THROW(
                                Dune::NotImplemented,
                                "fvmpfao2pfaboundpressure2p.hh, l. 1164: boundary shape not available as interaction volume shape");
                    }
                }
            }

        } // end all intersections
    } // end grid traversal

    return;
}

// only for 2-D general quadrilateral
template<class TypeTag>
void FVMPFAO2PFABoundPressure2P<TypeTag>::assemble()
{
    // initialization: set global matrix this->A_ to zero
    this->A_ = 0;
    this->f_ = 0;

    // run through all elements
    VertexIterator vItEnd = problem_.gridView().template end<dim>();
    for (VertexIterator vIt = problem_.gridView().template begin<dim>(); vIt != vItEnd; ++vIt)
    {
        int globalVertIdx = problem_.variables().index(*vIt);

        InteractionVolume& interactionVolume = interactionVolumes_[globalVertIdx];

        if (interactionVolume.isInnerVolume())
        {

            ElementPointer& elementPointer1 = interactionVolume.getSubVolumeElement(0);
            ElementPointer& elementPointer2 = interactionVolume.getSubVolumeElement(1);
            ElementPointer& elementPointer3 = interactionVolume.getSubVolumeElement(2);
            ElementPointer& elementPointer4 = interactionVolume.getSubVolumeElement(3);

            // get global coordinate of cell centers
            const GlobalPosition& globalPos1 = elementPointer1->geometry().center();
            const GlobalPosition& globalPos2 = elementPointer2->geometry().center();
            const GlobalPosition& globalPos3 = elementPointer3->geometry().center();
            const GlobalPosition& globalPos4 = elementPointer4->geometry().center();

            // cell volumes
            Scalar volume1 = elementPointer1->geometry().volume();
            Scalar volume2 = elementPointer2->geometry().volume();
            Scalar volume3 = elementPointer3->geometry().volume();
            Scalar volume4 = elementPointer4->geometry().volume();

            // cell index
            int globalIdx1 = problem_.variables().index(*elementPointer1);
            int globalIdx2 = problem_.variables().index(*elementPointer2);
            int globalIdx3 = problem_.variables().index(*elementPointer3);
            int globalIdx4 = problem_.variables().index(*elementPointer4);

            //get the cell Data
            CellData& cellData1 = problem_.variables().cellData(globalIdx1);
            CellData& cellData2 = problem_.variables().cellData(globalIdx2);
            CellData& cellData3 = problem_.variables().cellData(globalIdx3);
            CellData& cellData4 = problem_.variables().cellData(globalIdx4);

            // evaluate right hand side
            PrimaryVariables source(0.0);
            problem_.source(source, *elementPointer1);
            this->f_[globalIdx1] += volume1 / (4.0)
                    * (source[wPhaseIdx] / density_[wPhaseIdx] + source[nPhaseIdx] / density_[nPhaseIdx]);
            problem_.source(source, *elementPointer2);
            this->f_[globalIdx2] += volume2 / (4.0)
                    * (source[wPhaseIdx] / density_[wPhaseIdx] + source[nPhaseIdx] / density_[nPhaseIdx]);
            problem_.source(source, *elementPointer3);
            this->f_[globalIdx3] += volume3 / (4.0)
                    * (source[wPhaseIdx] / density_[wPhaseIdx] + source[nPhaseIdx] / density_[nPhaseIdx]);
            problem_.source(source, *elementPointer4);
            this->f_[globalIdx4] += volume4 / (4.0)
                    * (source[wPhaseIdx] / density_[wPhaseIdx] + source[nPhaseIdx] / density_[nPhaseIdx]);

            this->f_[globalIdx1] += evaluateErrorTerm_(cellData1) * volume1 / (4.0);
            this->f_[globalIdx2] += evaluateErrorTerm_(cellData2) * volume2 / (4.0);
            this->f_[globalIdx3] += evaluateErrorTerm_(cellData3) * volume3 / (4.0);
            this->f_[globalIdx4] += evaluateErrorTerm_(cellData4) * volume4 / (4.0);

            //get mobilities of the phases
            Dune::FieldVector<Scalar, numPhases> lambda1(cellData1.mobility(wPhaseIdx));
            lambda1[nPhaseIdx] = cellData1.mobility(nPhaseIdx);

            //compute total mobility of cell 1
            Scalar lambdaTotal1 = lambda1[wPhaseIdx] + lambda1[nPhaseIdx];

            //get mobilities of the phases
            Dune::FieldVector<Scalar, numPhases> lambda2(cellData2.mobility(wPhaseIdx));
            lambda2[nPhaseIdx] = cellData2.mobility(nPhaseIdx);

            //compute total mobility of cell 1
            Scalar lambdaTotal2 = lambda2[wPhaseIdx] + lambda2[nPhaseIdx];

            //get mobilities of the phases
            Dune::FieldVector<Scalar, numPhases> lambda3(cellData3.mobility(wPhaseIdx));
            lambda3[nPhaseIdx] = cellData3.mobility(nPhaseIdx);

            //compute total mobility of cell 1
            Scalar lambdaTotal3 = lambda3[wPhaseIdx] + lambda3[nPhaseIdx];

            //get mobilities of the phases
            Dune::FieldVector<Scalar, numPhases> lambda4(cellData4.mobility(wPhaseIdx));
            lambda4[nPhaseIdx] = cellData4.mobility(nPhaseIdx);

            //compute total mobility of cell 1
            Scalar lambdaTotal4 = lambda4[wPhaseIdx] + lambda4[nPhaseIdx];

            Scalar gn12nu14 = interactionVolume.getNTKrKNu_by_dF(lambdaTotal1, 0, 0, 1);
            Scalar gn12nu12 = interactionVolume.getNTKrKNu_by_dF(lambdaTotal1, 0, 0, 0);
            Scalar gn14nu14 = interactionVolume.getNTKrKNu_by_dF(lambdaTotal1, 0, 1, 1);
            Scalar gn14nu12 = interactionVolume.getNTKrKNu_by_dF(lambdaTotal1, 0, 1, 0);
            Scalar gn12nu23 = interactionVolume.getNTKrKNu_by_dF(lambdaTotal2, 1, 1, 0);
            Scalar gn12nu21 = interactionVolume.getNTKrKNu_by_dF(lambdaTotal2, 1, 1, 1);
            Scalar gn23nu23 = interactionVolume.getNTKrKNu_by_dF(lambdaTotal2, 1, 0, 0);
            Scalar gn23nu21 = interactionVolume.getNTKrKNu_by_dF(lambdaTotal2, 1, 0, 1);
            Scalar gn43nu32 = interactionVolume.getNTKrKNu_by_dF(lambdaTotal3, 2, 0, 1);
            Scalar gn43nu34 = interactionVolume.getNTKrKNu_by_dF(lambdaTotal3, 2, 0, 0);
            Scalar gn23nu32 = interactionVolume.getNTKrKNu_by_dF(lambdaTotal3, 2, 1, 1);
            Scalar gn23nu34 = interactionVolume.getNTKrKNu_by_dF(lambdaTotal3, 2, 1, 0);
            Scalar gn43nu41 = interactionVolume.getNTKrKNu_by_dF(lambdaTotal4, 3, 1, 0);
            Scalar gn43nu43 = interactionVolume.getNTKrKNu_by_dF(lambdaTotal4, 3, 1, 1);
            Scalar gn14nu41 = interactionVolume.getNTKrKNu_by_dF(lambdaTotal4, 3, 0, 0);
            Scalar gn14nu43 = interactionVolume.getNTKrKNu_by_dF(lambdaTotal4, 3, 0, 1);

            // compute transmissibility matrix T = CA^{-1}B+F
            Dune::FieldMatrix<Scalar, 2 * dim, 2 * dim> C(0), F(0), A(0), B(0);

            // evaluate matrix C, F, A, B
            C[0][0] = -gn12nu12;
            C[0][3] = -gn12nu14;
            C[1][0] = gn23nu21;
            C[1][1] = -gn23nu23;
            C[2][1] = gn43nu32;
            C[2][2] = gn43nu34;
            C[3][2] = -gn14nu43;
            C[3][3] = gn14nu41;

            F[0][0] = gn12nu12 + gn12nu14;
            F[1][1] = -gn23nu21 + gn23nu23;
            F[2][2] = -gn43nu34 - gn43nu32;
            F[3][3] = gn14nu43 - gn14nu41;

            A[0][0] = gn12nu12 + gn12nu21;
            A[0][1] = -gn12nu23;
            A[0][3] = gn12nu14;
            A[1][0] = -gn23nu21;
            A[1][1] = gn23nu23 + gn23nu32;
            A[1][2] = gn23nu34;
            A[2][1] = -gn43nu32;
            A[2][2] = -gn43nu34 - gn43nu43;
            A[2][3] = gn43nu41;
            A[3][0] = -gn14nu12;
            A[3][2] = gn14nu43;
            A[3][3] = -gn14nu41 - gn14nu14;

            B[0][0] = gn12nu12 + gn12nu14;
            B[0][1] = gn12nu21 - gn12nu23;
            B[1][1] = -gn23nu21 + gn23nu23;
            B[1][2] = gn23nu34 + gn23nu32;
            B[2][2] = -gn43nu34 - gn43nu32;
            B[2][3] = -gn43nu43 + gn43nu41;
            B[3][0] = -gn14nu12 - gn14nu14;
            B[3][3] = gn14nu43 - gn14nu41;

            // compute T
            A.invert();
            F += C.rightmultiply(B.leftmultiply(A));
            Dune::FieldMatrix<Scalar, 2 * dim, 2 * dim> T(F);

            //                        std::cout<<"Tpress = "<<T<<"\n";

            // assemble the global matrix this->A_ and right hand side f
            this->A_[globalIdx1][globalIdx1] += T[0][0] + T[3][0];
            this->A_[globalIdx1][globalIdx2] += T[0][1] + T[3][1];
            this->A_[globalIdx1][globalIdx3] += T[0][2] + T[3][2];
            this->A_[globalIdx1][globalIdx4] += T[0][3] + T[3][3];

            this->A_[globalIdx2][globalIdx1] += -T[0][0] + T[1][0];
            this->A_[globalIdx2][globalIdx2] += -T[0][1] + T[1][1];
            this->A_[globalIdx2][globalIdx3] += -T[0][2] + T[1][2];
            this->A_[globalIdx2][globalIdx4] += -T[0][3] + T[1][3];

            this->A_[globalIdx3][globalIdx1] -= T[1][0] + T[2][0];
            this->A_[globalIdx3][globalIdx2] -= T[1][1] + T[2][1];
            this->A_[globalIdx3][globalIdx3] -= T[1][2] + T[2][2];
            this->A_[globalIdx3][globalIdx4] -= T[1][3] + T[2][3];

            this->A_[globalIdx4][globalIdx1] += T[2][0] - T[3][0];
            this->A_[globalIdx4][globalIdx2] += T[2][1] - T[3][1];
            this->A_[globalIdx4][globalIdx3] += T[2][2] - T[3][2];
            this->A_[globalIdx4][globalIdx4] += T[2][3] - T[3][3];

            if (innerBoundaryVolumeFaces_[globalIdx1][interactionVolume.getIndexOnElement(0, 0)])
            {
                this->A_[globalIdx1][globalIdx1] += T[0][0];
                this->A_[globalIdx1][globalIdx2] += T[0][1];
                this->A_[globalIdx1][globalIdx3] += T[0][2];
                this->A_[globalIdx1][globalIdx4] += T[0][3];
            }
            if (innerBoundaryVolumeFaces_[globalIdx1][interactionVolume.getIndexOnElement(0, 1)])
            {
                this->A_[globalIdx1][globalIdx1] += T[3][0];
                this->A_[globalIdx1][globalIdx2] += T[3][1];
                this->A_[globalIdx1][globalIdx3] += T[3][2];
                this->A_[globalIdx1][globalIdx4] += T[3][3];

            }
            if (innerBoundaryVolumeFaces_[globalIdx2][interactionVolume.getIndexOnElement(1, 0)])
            {
                this->A_[globalIdx2][globalIdx1] += T[1][0];
                this->A_[globalIdx2][globalIdx2] += T[1][1];
                this->A_[globalIdx2][globalIdx3] += T[1][2];
                this->A_[globalIdx2][globalIdx4] += T[1][3];
            }
            if (innerBoundaryVolumeFaces_[globalIdx2][interactionVolume.getIndexOnElement(1, 1)])
            {
                this->A_[globalIdx2][globalIdx1] += -T[0][0];
                this->A_[globalIdx2][globalIdx2] += -T[0][1];
                this->A_[globalIdx2][globalIdx3] += -T[0][2];
                this->A_[globalIdx2][globalIdx4] += -T[0][3];
            }
            if (innerBoundaryVolumeFaces_[globalIdx3][interactionVolume.getIndexOnElement(2, 0)])
            {
                this->A_[globalIdx3][globalIdx1] -= T[2][0];
                this->A_[globalIdx3][globalIdx2] -= T[2][1];
                this->A_[globalIdx3][globalIdx3] -= T[2][2];
                this->A_[globalIdx3][globalIdx4] -= T[2][3];
            }
            if (innerBoundaryVolumeFaces_[globalIdx3][interactionVolume.getIndexOnElement(2, 1)])
            {
                this->A_[globalIdx3][globalIdx1] -= T[1][0];
                this->A_[globalIdx3][globalIdx2] -= T[1][1];
                this->A_[globalIdx3][globalIdx3] -= T[1][2];
                this->A_[globalIdx3][globalIdx4] -= T[1][3];
            }
            if (innerBoundaryVolumeFaces_[globalIdx4][interactionVolume.getIndexOnElement(3, 0)])
            {
                this->A_[globalIdx4][globalIdx1] += -T[3][0];
                this->A_[globalIdx4][globalIdx2] += -T[3][1];
                this->A_[globalIdx4][globalIdx3] += -T[3][2];
                this->A_[globalIdx4][globalIdx4] += -T[3][3];
            }
            if (innerBoundaryVolumeFaces_[globalIdx4][interactionVolume.getIndexOnElement(3, 1)])
            {
                this->A_[globalIdx4][globalIdx1] += T[2][0];
                this->A_[globalIdx4][globalIdx2] += T[2][1];
                this->A_[globalIdx4][globalIdx3] += T[2][2];
                this->A_[globalIdx4][globalIdx4] += T[2][3];
            }

            //add capillary pressure and gravity terms to right-hand-side
            //calculate capillary pressure velocity
            Dune::FieldVector<Scalar, 2 * dim> pc(0);
            pc[0] = cellData1.capillaryPressure();
            pc[1] = cellData2.capillaryPressure();
            pc[2] = cellData3.capillaryPressure();
            pc[3] = cellData4.capillaryPressure();

            Dune::FieldVector<Scalar, 2 * dim> gravityDiff(0);

//            std::cout<<"maxPos = "<<problem_.bboxMax()<<"\n";

            gravityDiff[0] = (problem_.bboxMax() - globalPos1) * gravity_ * (density_[nPhaseIdx] - density_[wPhaseIdx]);
            gravityDiff[1] = (problem_.bboxMax() - globalPos2) * gravity_ * (density_[nPhaseIdx] - density_[wPhaseIdx]);
            gravityDiff[2] = (problem_.bboxMax() - globalPos3) * gravity_ * (density_[nPhaseIdx] - density_[wPhaseIdx]);
            gravityDiff[3] = (problem_.bboxMax() - globalPos4) * gravity_ * (density_[nPhaseIdx] - density_[wPhaseIdx]);

            pc += gravityDiff;

            if (pc[0] == 0 && pc[1] == 0 && pc[2] == 0 && pc[3] == 0)
            {
                continue;
            }

            Dune::FieldVector<Scalar, 2 * dim> pcFlux(0);

            T.mv(pc, pcFlux);

            //            std::cout<<"pcFlux = "<<pcFlux<<"\n";

            Scalar pcPotential12 = pcFlux[0];
            Scalar pcPotential14 = pcFlux[3];
            Scalar pcPotential32 = -pcFlux[1];
            Scalar pcPotential34 = -pcFlux[2];

            //            std::cout<<"pcPotential12 = "<<pcPotential12<<"\n";

            //compute mobilities of face 1
            Dune::FieldVector<Scalar, numPhases> lambda12Upw(0.0);
            lambda12Upw[wPhaseIdx] = (pcPotential12 >= 0) ? lambda1[wPhaseIdx] : lambda2[wPhaseIdx];
            lambda12Upw[nPhaseIdx] = (pcPotential12 >= 0) ? lambda1[nPhaseIdx] : lambda2[nPhaseIdx];

            //compute mobilities of face 4
            Dune::FieldVector<Scalar, numPhases> lambda14Upw(0.0);
            lambda14Upw[wPhaseIdx] = (pcPotential14 >= 0) ? lambda1[wPhaseIdx] : lambda4[wPhaseIdx];
            lambda14Upw[nPhaseIdx] = (pcPotential14 >= 0) ? lambda1[nPhaseIdx] : lambda4[nPhaseIdx];

            //compute mobilities of face 2
            Dune::FieldVector<Scalar, numPhases> lambda32Upw(0.0);
            lambda32Upw[wPhaseIdx] = (pcPotential32 >= 0) ? lambda3[wPhaseIdx] : lambda2[wPhaseIdx];
            lambda32Upw[nPhaseIdx] = (pcPotential32 >= 0) ? lambda3[nPhaseIdx] : lambda2[nPhaseIdx];

            //compute mobilities of face 3
            Dune::FieldVector<Scalar, numPhases> lambda34Upw(0.0);
            lambda34Upw[wPhaseIdx] = (pcPotential34 >= 0) ? lambda3[wPhaseIdx] : lambda4[wPhaseIdx];
            lambda34Upw[nPhaseIdx] = (pcPotential34 >= 0) ? lambda3[nPhaseIdx] : lambda4[nPhaseIdx];

            for (int i = 0; i < numPhases; i++)
            {
                Scalar lambdaT12 = lambda12Upw[wPhaseIdx] + lambda12Upw[nPhaseIdx];
                Scalar lambdaT14 = lambda14Upw[wPhaseIdx] + lambda14Upw[nPhaseIdx];
                Scalar lambdaT32 = lambda32Upw[wPhaseIdx] + lambda32Upw[nPhaseIdx];
                Scalar lambdaT34 = lambda34Upw[wPhaseIdx] + lambda34Upw[nPhaseIdx];
                Scalar fracFlow12 = (lambdaT12 > threshold_) ? lambda12Upw[i] / (lambdaT12) : 0.0;
                Scalar fracFlow14 = (lambdaT14 > threshold_) ? lambda14Upw[i] / (lambdaT14) : 0.0;
                Scalar fracFlow32 = (lambdaT32 > threshold_) ? lambda32Upw[i] / (lambdaT32) : 0.0;
                Scalar fracFlow34 = (lambdaT34 > threshold_) ? lambda34Upw[i] / (lambdaT34) : 0.0;

                Dune::FieldVector<Scalar, 2 * dim> pcFluxReal(pcFlux);

                pcFluxReal[0] *= fracFlow12;
                pcFluxReal[1] *= fracFlow32;
                pcFluxReal[2] *= fracFlow34;
                pcFluxReal[3] *= fracFlow14;

//                if (isnan(pcFluxReal.two_norm()))
//                                std::cout<<"pcFlux = "<<pcFlux<<"\n";

                switch (pressureType_)
                {
                case pw:
                {
                    if (i == nPhaseIdx)
                    {
                        //add capillary pressure term to right hand side
                        this->f_[globalIdx1] -= (pcFluxReal[0] + pcFluxReal[3]);
                        this->f_[globalIdx2] -= (pcFluxReal[1] - pcFluxReal[0]);
                        this->f_[globalIdx3] -= (-pcFluxReal[2] - pcFluxReal[1]);
                        this->f_[globalIdx4] -= (-pcFluxReal[3] + pcFluxReal[2]);

                        if (innerBoundaryVolumeFaces_[globalIdx1][interactionVolume.getIndexOnElement(0, 0)])
                        {
                            this->f_[globalIdx1] -= pcFluxReal[0];
                        }
                        if (innerBoundaryVolumeFaces_[globalIdx1][interactionVolume.getIndexOnElement(0, 1)])
                        {
                            this->f_[globalIdx1] -= pcFluxReal[3];
                        }
                        if (innerBoundaryVolumeFaces_[globalIdx2][interactionVolume.getIndexOnElement(1, 0)])
                        {
                            this->f_[globalIdx2] -= pcFluxReal[1];
                        }
                        if (innerBoundaryVolumeFaces_[globalIdx2][interactionVolume.getIndexOnElement(1, 1)])
                        {
                            this->f_[globalIdx2] += pcFluxReal[0];
                        }
                        if (innerBoundaryVolumeFaces_[globalIdx3][interactionVolume.getIndexOnElement(2, 0)])
                        {
                            this->f_[globalIdx3] += pcFluxReal[2];
                        }
                        if (innerBoundaryVolumeFaces_[globalIdx3][interactionVolume.getIndexOnElement(2, 1)])
                        {
                            this->f_[globalIdx3] += pcFluxReal[1];
                        }
                        if (innerBoundaryVolumeFaces_[globalIdx4][interactionVolume.getIndexOnElement(3, 0)])
                        {
                            this->f_[globalIdx4] += pcFluxReal[3];
                        }
                        if (innerBoundaryVolumeFaces_[globalIdx4][interactionVolume.getIndexOnElement(3, 1)])
                        {
                            this->f_[globalIdx4] -= pcFluxReal[2];
                        }
                    }
                    break;
                }
                case pn:
                {
                    if (i == wPhaseIdx)
                    {
                        //add capillary pressure term to right hand side
                        this->f_[globalIdx1] += (pcFluxReal[0] + pcFluxReal[1]);
                        this->f_[globalIdx2] += (pcFluxReal[1] - pcFluxReal[0]);
                        this->f_[globalIdx3] += (-pcFluxReal[2] - pcFluxReal[1]);
                        this->f_[globalIdx4] += (-pcFluxReal[3] + pcFluxReal[2]);

                        if (innerBoundaryVolumeFaces_[globalIdx1][interactionVolume.getIndexOnElement(0, 0)])
                        {
                            this->f_[globalIdx1] += pcFluxReal[0];
                        }
                        if (innerBoundaryVolumeFaces_[globalIdx1][interactionVolume.getIndexOnElement(0, 1)])
                        {
                            this->f_[globalIdx1] += pcFluxReal[3];
                        }
                        if (innerBoundaryVolumeFaces_[globalIdx2][interactionVolume.getIndexOnElement(1, 0)])
                        {
                            this->f_[globalIdx2] += pcFluxReal[1];
                        }
                        if (innerBoundaryVolumeFaces_[globalIdx2][interactionVolume.getIndexOnElement(1, 1)])
                        {
                            this->f_[globalIdx2] -= pcFluxReal[0];
                        }
                        if (innerBoundaryVolumeFaces_[globalIdx3][interactionVolume.getIndexOnElement(2, 0)])
                        {
                            this->f_[globalIdx3] -= pcFluxReal[2];
                        }
                        if (innerBoundaryVolumeFaces_[globalIdx3][interactionVolume.getIndexOnElement(2, 1)])
                        {
                            this->f_[globalIdx3] -= pcFluxReal[1];
                        }
                        if (innerBoundaryVolumeFaces_[globalIdx4][interactionVolume.getIndexOnElement(3, 0)])
                        {
                            this->f_[globalIdx4] -= pcFluxReal[3];
                        }
                        if (innerBoundaryVolumeFaces_[globalIdx4][interactionVolume.getIndexOnElement(3, 1)])
                        {
                            this->f_[globalIdx4] += pcFluxReal[2];
                        }
                    }
                    break;
                }
                }
            }
        }

        // at least one face on boundary!
        else
        {
            for (int elemIdx = 0; elemIdx < 2 * dim; elemIdx++)
            {
                bool isOutside = false;
                for (int faceIdx = 0; faceIdx < dim; faceIdx++)
                {
                    int intVolFaceIdx = interactionVolume.getFaceIndexFromSubVolume(elemIdx, faceIdx);
                    if (interactionVolume.isOutsideFace(intVolFaceIdx))
                    {
                        isOutside = true;
                        break;
                    }
                }
                if (isOutside)
                {
                    continue;
                }

                ElementPointer& elementPointer = interactionVolume.getSubVolumeElement(elemIdx);

                // get global coordinate of cell centers
                const GlobalPosition& globalPos = elementPointer->geometry().center();

                // cell volumes
                Scalar volume = elementPointer->geometry().volume();

                // cell index
                int globalIdx = problem_.variables().index(*elementPointer);

                //get the cell Data
                CellData& cellData = problem_.variables().cellData(globalIdx);

                //permeability vector at boundary
                DimMatrix permeability(problem_.spatialParams().intrinsicPermeability(*elementPointer));

                // evaluate right hand side
                PrimaryVariables source(0);
                problem_.source(source, *elementPointer);
                this->f_[globalIdx] += volume / (4.0)
                        * (source[wPhaseIdx] / density_[wPhaseIdx] + source[nPhaseIdx] / density_[nPhaseIdx]);

                this->f_[globalIdx] += evaluateErrorTerm_(cellData) * volume / (4.0);

                //get mobilities of the phases
                Dune::FieldVector<Scalar, numPhases> lambda(cellData.mobility(wPhaseIdx));
                lambda[nPhaseIdx] = cellData.mobility(nPhaseIdx);

                Scalar pc = cellData.capillaryPressure();

                Scalar gravityDiff = (problem_.bboxMax() - globalPos) * gravity_
                        * (density_[nPhaseIdx] - density_[wPhaseIdx]);

                pc += gravityDiff; //minus because of gravity definition!

                for (int faceIdx = 0; faceIdx < dim; faceIdx++)
                {
                    int intVolFaceIdx = interactionVolume.getFaceIndexFromSubVolume(elemIdx, faceIdx);

                    if (interactionVolume.isBoundaryFace(intVolFaceIdx))
                    {

                        if (interactionVolume.getBoundaryType(intVolFaceIdx).isDirichlet(pressEqIdx))
                        {
                            int boundaryFaceIdx = interactionVolume.getIndexOnElement(elemIdx, faceIdx);

                            const ReferenceElement& referenceElement = ReferenceElementContainer::general(
                                    elementPointer->geometry().type());

                            const LocalPosition& localPos = referenceElement.position(boundaryFaceIdx, 1);

                            const GlobalPosition& globalPosFace = elementPointer->geometry().global(localPos);

                            DimVector distVec(globalPosFace - globalPos);
                            Scalar dist = distVec.two_norm();
                            DimVector unitDistVec(distVec);
                            unitDistVec /= dist;

                            Scalar faceArea = interactionVolume.getFaceArea(elemIdx, faceIdx);

                            // get pc and lambda at the boundary
                            Scalar satWBound = cellData.saturation(wPhaseIdx);
                            //check boundary sat at face 1
                            if (interactionVolume.getBoundaryType(intVolFaceIdx).isDirichlet(satEqIdx))
                            {
                                Scalar satBound = interactionVolume.getDirichletValues(intVolFaceIdx)[saturationIdx];
                                switch (saturationType_)
                                {
                                case Sw:
                                {
                                    satWBound = satBound;
                                    break;
                                }
                                case Sn:
                                {
                                    satWBound = 1 - satBound;
                                    break;
                                }
                                }

                            }

                            Scalar pcBound = MaterialLaw::pC(
                                    problem_.spatialParams().materialLawParams(*elementPointer), satWBound);

                            Scalar gravityDiffBound = (problem_.bboxMax() - globalPosFace) * gravity_
                                    * (density_[nPhaseIdx] - density_[wPhaseIdx]);

                            pcBound += gravityDiffBound;

                            Dune::FieldVector<Scalar, numPhases> lambdaBound(
                                    MaterialLaw::krw(problem_.spatialParams().materialLawParams(*elementPointer),
                                            satWBound));
                            lambdaBound[nPhaseIdx] = MaterialLaw::krn(
                                    problem_.spatialParams().materialLawParams(*elementPointer), satWBound);
                            lambdaBound[wPhaseIdx] /= viscosity_[wPhaseIdx];
                            lambdaBound[nPhaseIdx] /= viscosity_[nPhaseIdx];

                            Scalar potentialBound = interactionVolume.getDirichletValues(intVolFaceIdx)[pressureIdx];
                            Scalar gdeltaZ = (problem_.bboxMax()-globalPosFace) * gravity_;

                            //calculate potential gradients
                            Scalar potentialW = 0;
                            Scalar potentialNW = 0;
                            switch (pressureType_)
                            {
                            case pw:
                            {
                                potentialBound += density_[wPhaseIdx]*gdeltaZ;
                                potentialW = (cellData.pressure(wPhaseIdx) - potentialBound) / dist;
                                potentialNW = (cellData.pressure(nPhaseIdx) - potentialBound - pcBound) / dist;
                                break;
                            }
                            case pn:
                            {
                                potentialBound += density_[nPhaseIdx]*gdeltaZ;
                                potentialW = (cellData.pressure(wPhaseIdx) - potentialBound + pcBound) / dist;
                                potentialNW = (cellData.pressure(nPhaseIdx) - potentialBound) / dist;
                                break;
                            }
                            }

                            Scalar lambdaTotal = (potentialW >= 0.) ? lambda[wPhaseIdx] : lambdaBound[wPhaseIdx];
                            lambdaTotal += (potentialNW >= 0.) ? lambda[nPhaseIdx] : lambdaBound[nPhaseIdx];

                            DimVector permTimesNormal(0);
                            permeability.mv(unitDistVec, permTimesNormal);

                            //calculate current matrix entry
                            Scalar entry = lambdaTotal * (unitDistVec * permTimesNormal) / dist * faceArea;

                            //calculate right hand side
                            Scalar pcFlux = 0;

                            switch (pressureType_)
                            {
                            case pw:
                            {
                                // calculate capillary pressure gradient
                                DimVector pCGradient = unitDistVec;
                                pCGradient *= (pc - pcBound) / dist;

                                //add capillary pressure term to right hand side
                                pcFlux = 0.5 * (lambda[nPhaseIdx] + lambdaBound[nPhaseIdx])
                                        * (permTimesNormal * pCGradient) * faceArea;

                                break;
                            }
                            case pn:
                            {
                                // calculate capillary pressure gradient
                                DimVector pCGradient = unitDistVec;
                                pCGradient *= (pc - pcBound) / dist;

                                //add capillary pressure term to right hand side
                                pcFlux = 0.5 * (lambda[wPhaseIdx] + lambdaBound[wPhaseIdx])
                                        * (permTimesNormal * pCGradient) * faceArea;

                                break;

                            }
                            }

                            // set diagonal entry and right hand side entry
                            this->A_[globalIdx][globalIdx] += entry;
                            this->f_[globalIdx] += entry * potentialBound;

                            if (pc == 0 && pcBound == 0)
                            {
                                continue;
                            }

                            for (int i = 0; i < numPhases; i++)
                            {
                                switch (pressureType_)
                                {
                                case pw:
                                {
                                    if (i == nPhaseIdx)
                                    {
                                        //add capillary pressure term to right hand side
                                        this->f_[globalIdx] -= pcFlux;
                                    }
                                    break;
                                }
                                case pn:
                                {
                                    if (i == wPhaseIdx)
                                    {
                                        //add capillary pressure term to right hand side
                                        this->f_[globalIdx] += pcFlux;
                                    }
                                    break;
                                }
                                }
                            }

                        }
                        else if (interactionVolume.getBoundaryType(intVolFaceIdx).isNeumann(pressEqIdx))
                        {
                            Scalar J = interactionVolume.getNeumannValues(intVolFaceIdx)[wPhaseIdx]
                                    / density_[wPhaseIdx];
                            J += interactionVolume.getNeumannValues(intVolFaceIdx)[nPhaseIdx] / density_[nPhaseIdx];
                            this->f_[globalIdx] -= J;
                        }
                        else
                        {
                            std::cout << "interactionVolume.getBoundaryType(intVolFaceIdx).isNeumann(pressEqIdx)"
                                    << interactionVolume.getBoundaryType(intVolFaceIdx).isNeumann(pressEqIdx) << "\n";
                            DUNE_THROW(Dune::NotImplemented,
                                    "No valid boundary condition type defined for pressure equation!");
                        }
                    }
                }
            }

        } // end boundaries

    } // end vertex iterator

    return;
}


/*! \brief Updates constitutive relations and stores them in the variable class
 *
 * Stores mobility, fractional flow function and capillary pressure for all grid cells.
 */
template<class TypeTag>
void FVMPFAO2PFABoundPressure2P<TypeTag>::updateMaterialLaws()
{
    // iterate through leaf grid an evaluate c0 at cell center
    ElementIterator eItEnd = problem_.gridView().template end<0>();
    for (ElementIterator eIt = problem_.gridView().template begin<0>(); eIt != eItEnd; ++eIt)
    {
        int globalIdx = problem_.variables().index(*eIt);

        CellData& cellData = problem_.variables().cellData(globalIdx);

        Scalar satW = cellData.saturation(wPhaseIdx);

        Scalar pc = MaterialLaw::pC(problem_.spatialParams().materialLawParams(*eIt), satW);

        cellData.setCapillaryPressure(pc);

        // initialize mobilities
        Scalar mobilityW = MaterialLaw::krw(problem_.spatialParams().materialLawParams(*eIt), satW)
                / viscosity_[wPhaseIdx];
        Scalar mobilityNW = MaterialLaw::krn(problem_.spatialParams().materialLawParams(*eIt), satW)
                / viscosity_[nPhaseIdx];

        // initialize mobilities
        cellData.setMobility(wPhaseIdx, mobilityW);
        cellData.setMobility(nPhaseIdx, mobilityNW);

        //initialize fractional flow functions
        cellData.setFracFlowFunc(wPhaseIdx, mobilityW / (mobilityW + mobilityNW));
        cellData.setFracFlowFunc(nPhaseIdx, mobilityNW / (mobilityW + mobilityNW));
    }
    return;
}

}
// end of Dune namespace
#endif
