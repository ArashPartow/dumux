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
#ifndef DUMUX_FVMPFAL2OFABOUNDPRESSURE2P_HH
#define DUMUX_FVMPFAL2OFABOUNDPRESSURE2P_HH

// dumux environment
#include <dumux/decoupled/common/fv/fvpressure.hh>
#include <dumux/decoupled/common/fv/mpfa/mpfalinteractionvolume.hh>
#include <dumux/decoupled/2p/diffusion/diffusionproperties2p.hh>
#include <dumux/decoupled/common/fv/mpfa/fvmpfaproperties.hh>

/**
 * @file
 * @brief Finite volume MPFA L-method discretization of a two-phase pressure equation of the sequential IMPES model.
 */

namespace Dumux
{
//! \ingroup FVPressure2p
/*! \brief Finite volume MPFA L-method discretization of a two-phase flow pressure equation of the sequential IMPES model.
 *
 * Finite volume MPFA L-method discretization of the equations
 * \f[ - \text{div}\, \boldsymbol v_t = - \text{div}\, (\lambda_t \boldsymbol K \textbf{grad}\, \Phi_w + f_n \lambda_t \boldsymbol K \textbf{grad}\, \Phi_{cap}   ) = 0, \f]
 * or
 * \f[ - \text{div}\, \boldsymbol v_t = - \text{div}\, (\lambda_t \boldsymbol K \textbf{grad}\, \Phi_n - f_w \lambda_t \boldsymbol K \textbf{grad}\, \Phi_{cap}   ) = 0. \f]
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
class FVMPFAL2PFABoundPressure2P: public FVPressure<TypeTag>
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
        pressEqIdx = Indices::pressureEqIdx,
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

public:
    /*! \brief Type of the interaction volume objects
     *
     * Type of the interaction volume objects used to store the geometric information which is needed
     * to calculated the transmissibility matrices of one MPFA interaction volume.
     *
     */
    typedef Dumux::FVMPFALInteractionVolume<TypeTag> InteractionVolume;
private:

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

    //! Constructs a FVMPFAL2PFABoundPressure2P object
    /**
     * \param problem A problem class object
     */
    FVMPFAL2PFABoundPressure2P(Problem& problem) :
            ParentType(problem), problem_(problem), R_(0),
                    gravity_(problem.gravity()),
                    maxError_(0.), timeStep_(1.)
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

        // evaluate matrix R
        if (dim == 2)
        {
            R_[0][1] = 1;
            R_[1][0] = -1;
        }

        ErrorTermFactor_ = GET_PARAM_FROM_GROUP(TypeTag, Scalar, Impet, ErrorTermFactor);
        ErrorTermLowerBound_ = GET_PARAM_FROM_GROUP(TypeTag, Scalar, Impet, ErrorTermLowerBound);
        ErrorTermUpperBound_ = GET_PARAM_FROM_GROUP(TypeTag, Scalar, Impet, ErrorTermUpperBound);

        density_[wPhaseIdx] = 0.;
        density_[nPhaseIdx] = 0.;
        viscosity_[wPhaseIdx] = 0.;
        viscosity_[nPhaseIdx] = 0.;

        vtkOutputLevel_ = GET_PARAM_FROM_GROUP(TypeTag, int, Vtk, OutputLevel);
    }

private:
    Problem& problem_;
    DimMatrix R_;

protected:

    // Calculates tranmissibility matrix
    bool calculateTransmissibility(
            Dune::FieldMatrix<Scalar,dim,2*dim-dim+1>& transmissibility,
            InteractionVolume& interactionVolume,
            std::vector<DimVector >& lambda,
            int idx1, int idx2, int idx3, int idx4);

    GlobalInteractionVolumeVector interactionVolumes_;//!< Global Vector of interaction volumes
    InnerBoundaryVolumeFaces innerBoundaryVolumeFaces_; //!< Vector marking faces which intersect the boundary

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

};

template<class TypeTag>
void FVMPFAL2PFABoundPressure2P<TypeTag>::initializeMatrix()
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
                DUNE_THROW(Dune::NotImplemented, "GridType can not be used with MPFAL implementation!");
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
                DUNE_THROW(Dune::NotImplemented, "GridType can not be used with MPFAL implementation!");
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
void FVMPFAL2PFABoundPressure2P<TypeTag>::storeInteractionVolumeInfo()
{
    BoundaryTypes bcType;

    // run through all elements
    ElementIterator eItEnd = problem_.gridView().template end<0>();
    for (ElementIterator eIt = problem_.gridView().template begin<0>(); eIt != eItEnd; ++eIt)
    {
        // get common geometry information for the following computation

        int globalIdx1 = problem_.variables().index(*eIt);

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
                DUNE_THROW(Dune::NotImplemented, "GridType can not be used with MPFAL implementation!");
                break;
            }
            }

            int indexInInside12 = isIt12->indexInInside();
            int indexInInside14 = isIt14->indexInInside();

//            std::cout<<"indexInInside12 = "<<indexInInside12<<"\n";
//            std::cout<<"indexInInside14 = "<<indexInInside14<<"\n";

            // get the intersection node /bar^{x_3} between 'isIt12' and 'isIt14', denoted as 'corner1234'
            // initialization of corner1234

            const ReferenceElement& referenceElement = ReferenceElementContainer::general(eIt->geometry().type());

            GlobalPosition corner1234(0);

            int globalVertIdx1234 = 0;

            // get the global coordinate and global vertex index of corner1234
            for (int i = 0; i < isIt12->geometry().corners(); ++i)
            {
                bool finished = false;

                int localVertIdx12corner = referenceElement.subEntity(indexInInside12, 1, i, dim);
//                std::cout<<"localVertIdx12corner = "<<localVertIdx12corner<<"\n";
//
//                int globalVertIdx12corner = problem_.variables().index(
//                        *((*eIt).template subEntity < dim > (localVertIdx12corner)));
                int globalVertIdx12corner = problem_.variables().vertexMapper().map(*eIt, localVertIdx12corner, dim);
//                std::cout<<"globalVertIdx12corner = "<<globalVertIdx12corner<<"\n";


                for (int j = 0; j < isIt14->geometry().corners(); ++j)
                {
                    int localVertIdx14corner = referenceElement.subEntity(indexInInside14, 1, j, dim);
//                    std::cout<<"localVertIdx14corner = "<<localVertIdx14corner<<"\n";

//                    int globalVertIdx14corner = problem_.variables().index(
//                            *((*eIt).template subEntity < dim > (localVertIdx14corner)));
                    int globalVertIdx14corner = problem_.variables().vertexMapper().map(*eIt, localVertIdx14corner, dim);
//                    std::cout<<"globalVertIdx14corner = "<<globalVertIdx14corner<<"\n";

                    if (globalVertIdx12corner == globalVertIdx14corner)
                    {
                        corner1234 = eIt->geometry().corner(localVertIdx12corner);
//                        std::cout<<"corner1234 = "<<corner1234<<"\n";
//                        std::cout<<"globalIdx = "<<globalIdx1<<"\n";

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

            interactionVolumes_[globalVertIdx1234].setCenterPosition(corner1234);

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

            interactionVolumes_[globalVertIdx1234].setNormal(unitOuterNormal12, 0, 0);
            interactionVolumes_[globalVertIdx1234].setNormal(unitOuterNormal14, 0, 1);
            //get the normals of from cell 2 and 4
            unitOuterNormal14 *= -1;
            unitOuterNormal12 *= -1;
            interactionVolumes_[globalVertIdx1234].setFaceArea(faceVol12, 0, 0);
            interactionVolumes_[globalVertIdx1234].setFaceArea(faceVol41, 0, 1);
            interactionVolumes_[globalVertIdx1234].setFacePosition(globalPosFace12, 0, 0);
            interactionVolumes_[globalVertIdx1234].setFacePosition(globalPosFace41, 0, 1);

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
                interactionVolumes_[globalVertIdx1234].setFacePosition(globalPosFace12, 1, 1);

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
                    interactionVolumes_[globalVertIdx1234].setFacePosition(globalPosFace41, 3, 0);

                    // cell 3
                    GlobalPosition globalPos3(0);

                    GlobalPosition globalPosFace23(0);
                    GlobalPosition globalPosFace34(0);

                    IntersectionIterator isIt2End = problem_.gridView().iend(*elementPointer2);
                    IntersectionIterator isIt4End = problem_.gridView().iend(*elementPointer4);
                    for (IntersectionIterator isIt23 = problem_.gridView().ibegin(*elementPointer2); isIt23 != isIt2End;
                            ++isIt23)
                            {
                        bool finished = false;

                        for (IntersectionIterator isIt43 = problem_.gridView().ibegin(*elementPointer4);
                                isIt43 != isIt4End; ++isIt43)
                                {
                            if (isIt23->neighbor() && isIt43->neighbor())
                            {
                                ElementPointer elementPointer32 = isIt23->outside();
                                ElementPointer elementPointer34 = isIt43->outside();

                                // find the common neighbor cell between cell 2 and cell 3, except cell 1
                                if (elementPointer32 == elementPointer34 && elementPointer32 != eIt)
                                {
                                    //store pointer 3
                                    interactionVolumes_[globalVertIdx1234].setSubVolumeElement(elementPointer32, 2);

                                    interactionVolumes_[globalVertIdx1234].setIndexOnElement(isIt23->indexInInside(), 1,
                                            0);
                                    interactionVolumes_[globalVertIdx1234].setIndexOnElement(isIt23->indexInOutside(), 2,
                                            1);
                                    interactionVolumes_[globalVertIdx1234].setIndexOnElement(isIt43->indexInInside(), 3,
                                            1);
                                    interactionVolumes_[globalVertIdx1234].setIndexOnElement(isIt43->indexInOutside(), 2,
                                            0);

                                    // get global coordinate of neighbor cell 4 center
                                    globalPos3 = elementPointer32->geometry().center();

                                    globalPosFace23 = isIt23->geometry().center();
                                    globalPosFace34 = isIt43->geometry().center();

                                    Scalar faceVol23 = isIt23->geometry().volume() / 2.0;
                                    Scalar faceVol34 = isIt43->geometry().volume() / 2.0;

                                    // get outer normal vector scaled with half volume of face : for numbering of n see Aavatsmark, Eigestad
                                    DimVector unitOuterNormal23 = isIt23->centerUnitOuterNormal();

                                    DimVector unitOuterNormal43 = isIt43->centerUnitOuterNormal();

                                    interactionVolumes_[globalVertIdx1234].setNormal(unitOuterNormal23, 1, 0);
                                    unitOuterNormal23 *= -1;
                                    interactionVolumes_[globalVertIdx1234].setNormal(unitOuterNormal23, 2, 1);
                                    interactionVolumes_[globalVertIdx1234].setNormal(unitOuterNormal43, 3, 1);
                                    unitOuterNormal43 *= -1;
                                    interactionVolumes_[globalVertIdx1234].setNormal(unitOuterNormal43, 2, 0);
                                    interactionVolumes_[globalVertIdx1234].setFaceArea(faceVol23, 1, 0);
                                    interactionVolumes_[globalVertIdx1234].setFaceArea(faceVol23, 2, 1);
                                    interactionVolumes_[globalVertIdx1234].setFaceArea(faceVol34, 2, 0);
                                    interactionVolumes_[globalVertIdx1234].setFaceArea(faceVol34, 3, 1);
                                    interactionVolumes_[globalVertIdx1234].setFacePosition(globalPosFace23, 1, 0);
                                    interactionVolumes_[globalVertIdx1234].setFacePosition(globalPosFace23, 2, 1);
                                    interactionVolumes_[globalVertIdx1234].setFacePosition(globalPosFace34, 2, 0);
                                    interactionVolumes_[globalVertIdx1234].setFacePosition(globalPosFace34, 3, 1);

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
                                    interactionVolumes_[globalVertIdx1234].setFacePosition(globalPosFace23, 1, 0);

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
                    interactionVolumes_[globalVertIdx1234].setFacePosition(globalPosFace41, 3, 0);

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
                                    interactionVolumes_[globalVertIdx1234].setFacePosition(globalPosFace34, 3, 1);

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
void FVMPFAL2PFABoundPressure2P<TypeTag>::assemble()
{
    // initialization: set global matrix this->A_ to zero
    this->A_ = 0;
    this->f_ = 0;

    // run through all vertices
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
            this->f_[globalIdx1] += volume1 / (4.0) * (source[wPhaseIdx] / density_[wPhaseIdx] + source[nPhaseIdx] / density_[nPhaseIdx]);
            problem_.source(source, *elementPointer2);
            this->f_[globalIdx2] += volume2 / (4.0) * (source[wPhaseIdx] / density_[wPhaseIdx] + source[nPhaseIdx] / density_[nPhaseIdx]);
            problem_.source(source, *elementPointer3);
            this->f_[globalIdx3] += volume3 / (4.0) * (source[wPhaseIdx] / density_[wPhaseIdx] + source[nPhaseIdx] / density_[nPhaseIdx]);
            problem_.source(source, *elementPointer4);
            this->f_[globalIdx4] += volume4 / (4.0) * (source[wPhaseIdx] / density_[wPhaseIdx] + source[nPhaseIdx] / density_[nPhaseIdx]);

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

            std::vector<DimVector > lambda(2*dim);

            lambda[0][0] = lambdaTotal1;
            lambda[0][1] = lambdaTotal1;
            lambda[1][0] = lambdaTotal2;
            lambda[1][1] = lambdaTotal2;
            lambda[2][0] = lambdaTotal3;
            lambda[2][1] = lambdaTotal3;
            lambda[3][0] = lambdaTotal4;
            lambda[3][1] = lambdaTotal4;

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

            Dune::FieldVector<Scalar, 2 * dim> pcFlux(0);

            Scalar pcPotential12 = 0;
            Scalar pcPotential14 = 0;
            Scalar pcPotential32 = 0;
            Scalar pcPotential34 = 0;

            DimVector Tu(0);
            Dune::FieldVector<Scalar, 2 * dim - dim + 1> u(0);
            Dune::FieldMatrix<Scalar,dim,2*dim-dim+1> T(0);

            bool rightTriangle = calculateTransmissibility(T, interactionVolume, lambda, 0, 1, 2, 3);

            if (rightTriangle)
            {
                if (innerBoundaryVolumeFaces_[globalIdx1][interactionVolume.getIndexOnElement(0, 0)])
                {
                    T *= 2;
                }
                this->A_[globalIdx1][globalIdx2] += T[1][0];
                this->A_[globalIdx1][globalIdx3] += T[1][1];
                this->A_[globalIdx1][globalIdx1] += T[1][2];

                this->A_[globalIdx2][globalIdx2] -= T[1][0];
                this->A_[globalIdx2][globalIdx3] -= T[1][1];
                this->A_[globalIdx2][globalIdx1] -= T[1][2];

                u[0] = pc[1];
                u[1] = pc[2];
                u[2] = pc[0];

                T.mv(u, Tu);

                pcFlux[0] = Tu[1];
                pcPotential12 = Tu[1];
            }
            else
            {
                if (innerBoundaryVolumeFaces_[globalIdx1][interactionVolume.getIndexOnElement(0, 0)])
                {
                    T *= 2;
                }
                this->A_[globalIdx1][globalIdx1] += T[1][0];
                this->A_[globalIdx1][globalIdx4] += T[1][1];
                this->A_[globalIdx1][globalIdx2] += T[1][2];

                this->A_[globalIdx2][globalIdx1] -= T[1][0];
                this->A_[globalIdx2][globalIdx4] -= T[1][1];
                this->A_[globalIdx2][globalIdx2] -= T[1][2];

                u[0] = pc[0];
                u[1] = pc[3];
                u[2] = pc[1];

                T.mv(u, Tu);

                pcFlux[0] = Tu[1];
                pcPotential12 = Tu[1];
            }

            rightTriangle = calculateTransmissibility(T, interactionVolume, lambda, 1, 2, 3, 0);

            if (rightTriangle)
            {
                if (innerBoundaryVolumeFaces_[globalIdx2][interactionVolume.getIndexOnElement(1, 0)])
                {
                    T *= 2;
                }
                this->A_[globalIdx2][globalIdx3] += T[1][0];
                this->A_[globalIdx2][globalIdx4] += T[1][1];
                this->A_[globalIdx2][globalIdx2] += T[1][2];

                this->A_[globalIdx3][globalIdx3] -= T[1][0];
                this->A_[globalIdx3][globalIdx4] -= T[1][1];
                this->A_[globalIdx3][globalIdx2] -= T[1][2];

                u[0] = pc[2];
                u[1] = pc[3];
                u[2] = pc[1];

                T.mv(u, Tu);

                pcFlux[1] = Tu[1];
                pcPotential32 = -Tu[1];
            }
            else
            {
                if (innerBoundaryVolumeFaces_[globalIdx2][interactionVolume.getIndexOnElement(1, 0)])
                {
                    T *= 2;
                }
                this->A_[globalIdx2][globalIdx2] += T[1][0];
                this->A_[globalIdx2][globalIdx1] += T[1][1];
                this->A_[globalIdx2][globalIdx3] += T[1][2];

                this->A_[globalIdx3][globalIdx2] -= T[1][0];
                this->A_[globalIdx3][globalIdx1] -= T[1][1];
                this->A_[globalIdx3][globalIdx3] -= T[1][2];

                u[0] = pc[1];
                u[1] = pc[0];
                u[2] = pc[2];

                T.mv(u, Tu);

                pcFlux[1] = Tu[1];
                pcPotential32 = -Tu[1];
            }

            rightTriangle = calculateTransmissibility(T, interactionVolume, lambda, 2, 3, 0, 1);

            if (rightTriangle)
            {
                if (innerBoundaryVolumeFaces_[globalIdx3][interactionVolume.getIndexOnElement(2, 0)])
                {
                    T *= 2;
                }
                this->A_[globalIdx3][globalIdx4] += T[1][0];
                this->A_[globalIdx3][globalIdx1] += T[1][1];
                this->A_[globalIdx3][globalIdx3] += T[1][2];

                this->A_[globalIdx4][globalIdx4] -= T[1][0];
                this->A_[globalIdx4][globalIdx1] -= T[1][1];
                this->A_[globalIdx4][globalIdx3] -= T[1][2];

                u[0] = pc[3];
                u[1] = pc[0];
                u[2] = pc[2];

                T.mv(u, Tu);

                pcFlux[2] = Tu[1];
                pcPotential34 = Tu[1];
            }
            else
            {
                if (innerBoundaryVolumeFaces_[globalIdx3][interactionVolume.getIndexOnElement(2, 0)])
                {
                    T *= 2;
                }
                this->A_[globalIdx3][globalIdx3] += T[1][0];
                this->A_[globalIdx3][globalIdx2] += T[1][1];
                this->A_[globalIdx3][globalIdx4] += T[1][2];

                this->A_[globalIdx4][globalIdx3] -= T[1][0];
                this->A_[globalIdx4][globalIdx2] -= T[1][1];
                this->A_[globalIdx4][globalIdx4] -= T[1][2];

                u[0] = pc[2];
                u[1] = pc[1];
                u[2] = pc[3];

                T.mv(u, Tu);

                pcFlux[2] = Tu[1];
                pcPotential34 = Tu[1];
            }

            rightTriangle = calculateTransmissibility(T, interactionVolume, lambda, 3, 0, 1, 2);

            if (rightTriangle)
            {
                if (innerBoundaryVolumeFaces_[globalIdx4][interactionVolume.getIndexOnElement(3, 0)])
                {
                    T *= 2;
                }
                this->A_[globalIdx4][globalIdx1] += T[1][0];
                this->A_[globalIdx4][globalIdx2] += T[1][1];
                this->A_[globalIdx4][globalIdx4] += T[1][2];

                this->A_[globalIdx1][globalIdx1] -= T[1][0];
                this->A_[globalIdx1][globalIdx2] -= T[1][1];
                this->A_[globalIdx1][globalIdx4] -= T[1][2];

                u[0] = pc[0];
                u[1] = pc[1];
                u[2] = pc[3];

                T.mv(u, Tu);

                pcFlux[3] = Tu[1];
                pcPotential14 = -Tu[1];
            }
            else
            {
                if (innerBoundaryVolumeFaces_[globalIdx4][interactionVolume.getIndexOnElement(3, 0)])
                {
                    T *= 2;
                }
                this->A_[globalIdx4][globalIdx4] += T[1][0];
                this->A_[globalIdx4][globalIdx3] += T[1][1];
                this->A_[globalIdx4][globalIdx1] += T[1][2];

                this->A_[globalIdx1][globalIdx4] -= T[1][0];
                this->A_[globalIdx1][globalIdx3] -= T[1][1];
                this->A_[globalIdx1][globalIdx1] -= T[1][2];

                u[0] = pc[3];
                u[1] = pc[2];
                u[2] = pc[0];

                T.mv(u, Tu);

                pcFlux[3] = Tu[1];
                pcPotential14 = -Tu[1];
            }

            if (pc[0] == 0 && pc[1] == 0 && pc[2] == 0 && pc[3] == 0)
            {
                continue;
            }

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
                        this->f_[globalIdx1] -= (pcFluxReal[0] - pcFluxReal[3]);
                        this->f_[globalIdx2] -= (pcFluxReal[1] - pcFluxReal[0]);
                        this->f_[globalIdx3] -= (pcFluxReal[2] - pcFluxReal[1]);
                        this->f_[globalIdx4] -= (pcFluxReal[3] - pcFluxReal[2]);

                    }
                    break;
                }
                case pn:
                {
                    if (i == wPhaseIdx)
                    {
                        //add capillary pressure term to right hand side
                        this->f_[globalIdx1] += (pcFluxReal[0] - pcFluxReal[3]);
                        this->f_[globalIdx2] += (pcFluxReal[1] - pcFluxReal[0]);
                        this->f_[globalIdx3] += (pcFluxReal[2] - pcFluxReal[1]);
                        this->f_[globalIdx4] += (pcFluxReal[3] - pcFluxReal[2]);
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
                this->f_[globalIdx] += volume / (4.0) * (source[wPhaseIdx] / density_[wPhaseIdx] + source[nPhaseIdx] / density_[nPhaseIdx]);

                this->f_[globalIdx] += evaluateErrorTerm_(cellData) * volume / (4.0);

                //get mobilities of the phases
                Dune::FieldVector<Scalar, numPhases> lambda(cellData.mobility(wPhaseIdx));
                lambda[nPhaseIdx] = cellData.mobility(nPhaseIdx);

                Scalar pc = cellData.capillaryPressure();

                Scalar gravityDiff = (problem_.bboxMax() - globalPos) * gravity_ * (density_[nPhaseIdx] - density_[wPhaseIdx]);

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
                            Scalar J = interactionVolume.getNeumannValues(intVolFaceIdx)[wPhaseIdx] / density_[wPhaseIdx];
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

    // only do more if we have more than one process
    if (problem_.gridView().comm().size() > 1)
    {
        // set ghost and overlap element entries
        ElementIterator eItEnd = problem_.gridView().template end<0>();
        for (ElementIterator eIt = problem_.gridView().template begin<0>(); eIt != eItEnd; ++eIt)
        {
            if (eIt->partitionType() == Dune::InteriorEntity)
                continue;
            
            // get the global index of the cell
            int globalIdxI = problem_.variables().index(*eIt);

            this->A_[globalIdxI] = 0.0;
            this->A_[globalIdxI][globalIdxI] = 1.0;
            this->f_[globalIdxI] = this->pressure()[globalIdxI];
        }
    }
    
    return;
}

/*! \brief Calculates tranmissibility matrix
 *
 * Calculates tranmissibility matrix of an L-shape for a certain flux face.
 * Automatically selects one of the two possible L-shape (left, or right).
 *
 * \param transmissibility Matrix for the resulting transmissibility
 * \param interactionVolume The interaction volume object (includes geometric information)
 * \param lambda Mobilities of cells 1-4
 * \param idx1 Index of cell 1 of the L-stencil
 * \param idx2 Index of cell 2 of the L-stencil
 * \param idx3 Index of cell 3 of the L-stencil
 * \param idx4 Index of cell 4 of the L-stencil
 */
template<class TypeTag>
bool FVMPFAL2PFABoundPressure2P<TypeTag>::calculateTransmissibility(
        Dune::FieldMatrix<Scalar,dim,2*dim-dim+1>& transmissibility,
        InteractionVolume& interactionVolume,
        std::vector<DimVector >& lambda,
        int idx1, int idx2, int idx3, int idx4)
{
    ElementPointer& elementPointer1 = interactionVolume.getSubVolumeElement(idx1);
    ElementPointer& elementPointer2 = interactionVolume.getSubVolumeElement(idx2);
    ElementPointer& elementPointer3 = interactionVolume.getSubVolumeElement(idx3);
    ElementPointer& elementPointer4 = interactionVolume.getSubVolumeElement(idx4);

    // get global coordinate of cell centers
    const GlobalPosition& globalPos1 = elementPointer1->geometry().center();
    const GlobalPosition& globalPos2 = elementPointer2->geometry().center();
    const GlobalPosition& globalPos3 = elementPointer3->geometry().center();
    const GlobalPosition& globalPos4 = elementPointer4->geometry().center();

    const GlobalPosition& globalPosCenter = interactionVolume.getCenterPosition();

    const DimMatrix& K1 = problem_.spatialParams().intrinsicPermeability(*elementPointer1);
    const DimMatrix& K2 = problem_.spatialParams().intrinsicPermeability(*elementPointer2);
    const DimMatrix& K3 = problem_.spatialParams().intrinsicPermeability(*elementPointer3);
    const DimMatrix& K4 = problem_.spatialParams().intrinsicPermeability(*elementPointer4);

    const GlobalPosition& globalPosFace12 = interactionVolume.getFacePosition(idx1,0);
    const GlobalPosition& globalPosFace23 = interactionVolume.getFacePosition(idx2,0);

    // compute normal vectors nu1-nu7 in triangle R for first half edge
    DimVector nu1R1(0);
    R_.mv(globalPosFace12-globalPos2 ,nu1R1);

    DimVector nu2R1(0);
    R_.mv(globalPos2-globalPosFace23, nu2R1);

    DimVector nu3R1(0);
    R_.mv(globalPosFace23-globalPos3, nu3R1);

    DimVector nu4R1(0);
    R_.mv(globalPos3-globalPosCenter, nu4R1);

    DimVector nu5R1(0);
    R_.mv(globalPosCenter-globalPos1, nu5R1);

    DimVector nu6R1(0);
    R_.mv(globalPos1-globalPosFace12, nu6R1);

    DimVector nu7R1(0);
    R_.mv(globalPosCenter-globalPos2, nu7R1);

    // compute T, i.e., the area of quadrilateral made by normal vectors 'nu'
    DimVector Rnu2R1(0);
    R_.mv(nu2R1, Rnu2R1);
    Scalar T1R1 = nu1R1 * Rnu2R1;

    DimVector Rnu4R1(0);
    R_.mv(nu4R1, Rnu4R1);
    Scalar T2R1 = nu3R1 * Rnu4R1;

    DimVector Rnu6R1(0);
    R_.mv(nu6R1, Rnu6R1);
    Scalar T3R1 = nu5R1 * Rnu6R1;

    // compute components needed for flux calculation, denoted as 'omega' and 'chi'
    DimVector K2nu1R1(0);
    K2.mv(nu1R1, K2nu1R1);
    DimVector K2nu2R1(0);
    K2.mv(nu2R1, K2nu2R1);
    DimVector K4nu3R1(0);
    K3.mv(nu3R1, K4nu3R1);
    DimVector K4nu4R1(0);
    K3.mv(nu4R1, K4nu4R1);
    DimVector K1nu5R1(0);
    K1.mv(nu5R1, K1nu5R1);
    DimVector K1nu6R1(0);
    K1.mv(nu6R1, K1nu6R1);

    DimVector Rnu1R1(0);
    R_.mv(nu1R1, Rnu1R1);

    DimVector &outerNormaln1R1 =  interactionVolume.getNormal(idx2, 0);
    DimVector &outerNormaln2 =  interactionVolume.getNormal(idx1, 0);

//    std::cout<<"outerNormaln1R1 = "<<outerNormaln1R1<<"\n";
//    std::cout<<"outerNormaln2 = "<<outerNormaln2<<"\n";

    Scalar omega111R1 = lambda[idx2][0] * (outerNormaln1R1 * K2nu1R1) * interactionVolume.getFaceArea(idx2, 0)/T1R1;
    Scalar omega112R1 = lambda[idx2][0] * (outerNormaln1R1 * K2nu2R1) * interactionVolume.getFaceArea(idx2, 0)/T1R1;
    Scalar omega211R1 = lambda[idx2][1] * (outerNormaln2 * K2nu1R1) * interactionVolume.getFaceArea(idx2, 1)/T1R1;
    Scalar omega212R1 = lambda[idx2][1] * (outerNormaln2 * K2nu2R1) * interactionVolume.getFaceArea(idx2, 1)/T1R1;
    Scalar omega123R1 = lambda[idx3][1] * (outerNormaln1R1 * K4nu3R1) * interactionVolume.getFaceArea(idx3, 1)/T2R1;
    Scalar omega124R1 = lambda[idx3][1] * (outerNormaln1R1 * K4nu4R1) * interactionVolume.getFaceArea(idx3, 1)/T2R1;
    Scalar omega235R1 = lambda[idx1][0] * (outerNormaln2 * K1nu5R1) * interactionVolume.getFaceArea(idx1, 0)/T3R1;
    Scalar omega236R1 = lambda[idx1][0] * (outerNormaln2 * K1nu6R1) * interactionVolume.getFaceArea(idx1, 0)/T3R1;
    Scalar chi711R1 = (nu7R1 * Rnu1R1)/T1R1;
    Scalar chi712R1 = (nu7R1 * Rnu2R1)/T1R1;

    // compute transmissibility matrix TR1 = CA^{-1}B+D
    DimMatrix C(0), A(0);
    Dune::FieldMatrix<Scalar,dim,2*dim-dim+1> D(0), B(0);

    // evaluate matrix C, D, A, B
    C[0][0] = -omega111R1;
    C[0][1] = -omega112R1;
    C[1][0] = -omega211R1;
    C[1][1] = -omega212R1;

    D[0][0] = omega111R1 + omega112R1;
    D[1][0] = omega211R1 + omega212R1;

    A[0][0] = omega111R1 - omega124R1 - omega123R1*chi711R1;
    A[0][1] = omega112R1 - omega123R1*chi712R1;
    A[1][0] = omega211R1 - omega236R1*chi711R1;
    A[1][1] = omega212R1 - omega235R1 - omega236R1*chi712R1;

    B[0][0] = omega111R1 + omega112R1 + omega123R1*(1.0 - chi711R1 - chi712R1);
    B[0][1] = -omega123R1 - omega124R1;
    B[1][0] = omega211R1 + omega212R1 + omega236R1*(1.0 - chi711R1 - chi712R1);
    B[1][2] = -omega235R1 - omega236R1;

//    std::cout<<"AR = "<<A<<"\n";

    // compute TR1
    A.invert();
    D += B.leftmultiply(C.rightmultiply(A));
    Dune::FieldMatrix<Scalar,dim,2*dim-dim+1> TR1(D);


    // 2.use triangle L to compute the transmissibility of half edge
    const GlobalPosition& globalPosFace14 = interactionVolume.getFacePosition(idx1,1);

//    std::cout<<"globalPosFace14 = "<<globalPosFace14<<"\n";

    // compute normal vectors nu1-nu7 in triangle L for first half edge
    DimVector nu1L1(0);
    R_.mv(globalPosFace12-globalPos1 ,nu1L1);

    DimVector nu2L1(0);
    R_.mv(globalPos1-globalPosFace14, nu2L1);

    DimVector nu3L1(0);
    R_.mv(globalPosFace14-globalPos4, nu3L1);

    DimVector nu4L1(0);
    R_.mv(globalPos4-globalPosCenter, nu4L1);

    DimVector nu5L1(0);
    R_.mv(globalPosCenter-globalPos2, nu5L1);

    DimVector nu6L1(0);
    R_.mv(globalPos2-globalPosFace12, nu6L1);

    DimVector nu7L1(0);
    R_.mv(globalPosCenter-globalPos1, nu7L1);

    // compute T, i.e., the area of quadrilateral made by normal vectors 'nu'
    DimVector Rnu2L1(0);
    R_.mv(nu2L1, Rnu2L1);
    Scalar T1L1 = nu1L1 * Rnu2L1;

    DimVector Rnu4L1(0);
    R_.mv(nu4L1, Rnu4L1);
    Scalar T2L1 = nu3L1 * Rnu4L1;

    DimVector Rnu6L1(0);
    R_.mv(nu6L1, Rnu6L1);
    Scalar T3L1 = nu5L1 * Rnu6L1;

    // compute components needed for flux calculation, denoted as 'omega' and 'chi'
    DimVector K1nu1L1(0);
    K1.mv(nu1L1, K1nu1L1);
    DimVector K1nu2L1(0);
    K1.mv(nu2L1, K1nu2L1);
    DimVector K3nu3L1(0);
    K4.mv(nu3L1, K3nu3L1);
    DimVector K3nu4L1(0);
    K4.mv(nu4L1, K3nu4L1);
    DimVector K2nu5L1(0);
    K2.mv(nu5L1, K2nu5L1);
    DimVector K2nu6L1(0);
    K2.mv(nu6L1, K2nu6L1);

    DimVector Rnu1L1(0);
    R_.mv(nu1L1, Rnu1L1);

    DimVector &outerNormaln1L1 =  interactionVolume.getNormal(idx1, 1);
//    std::cout<<"outerNormaln1L1 = "<<outerNormaln1L1<<"\n";

    Scalar omega111L1 = lambda[idx1][1] * (outerNormaln1L1 * K1nu1L1)* interactionVolume.getFaceArea(idx1, 1)/T1L1;
    Scalar omega112L1 = lambda[idx1][1] * (outerNormaln1L1 * K1nu2L1)* interactionVolume.getFaceArea(idx1, 1)/T1L1;
    Scalar omega211L1 = lambda[idx1][0] * (outerNormaln2 * K1nu1L1)* interactionVolume.getFaceArea(idx1, 0)/T1L1;
    Scalar omega212L1 = lambda[idx1][0] * (outerNormaln2 * K1nu2L1)* interactionVolume.getFaceArea(idx1, 0)/T1L1;
    Scalar omega123L1 = lambda[idx4][0] * (outerNormaln1L1 * K3nu3L1)* interactionVolume.getFaceArea(idx4, 0)/T2L1;
    Scalar omega124L1 = lambda[idx4][0] * (outerNormaln1L1 * K3nu4L1)* interactionVolume.getFaceArea(idx4, 0)/T2L1;
    Scalar omega235L1 = lambda[idx2][1] * (outerNormaln2 * K2nu5L1)* interactionVolume.getFaceArea(idx2, 1)/T3L1;
    Scalar omega236L1 = lambda[idx2][1] * (outerNormaln2 * K2nu6L1)* interactionVolume.getFaceArea(idx2, 1)/T3L1;
    Scalar chi711L1 = (nu7L1 * Rnu1L1)/T1L1;
    Scalar chi712L1 = (nu7L1 * Rnu2L1)/T1L1;

    // compute transmissibility matrix TL1 = CA^{-1}B+D
    C = 0;
    A = 0;
    D = 0;
    B = 0;

    // evaluate matrix C, D, A, B
    C[0][0] = -omega111L1;
    C[0][1] = -omega112L1;
    C[1][0] = -omega211L1;
    C[1][1] = -omega212L1;

    D[0][0] = omega111L1 + omega112L1;
    D[1][0] = omega211L1 + omega212L1;

    A[0][0] = omega111L1 - omega124L1 - omega123L1*chi711L1;
    A[0][1] = omega112L1 - omega123L1*chi712L1;
    A[1][0] = omega211L1 - omega236L1*chi711L1;
    A[1][1] = omega212L1 - omega235L1 - omega236L1*chi712L1;

    B[0][0] = omega111L1 + omega112L1 + omega123L1*(1.0 - chi711L1 - chi712L1);
    B[0][1] = -omega123L1 - omega124L1;
    B[1][0] = omega211L1 + omega212L1 + omega236L1*(1.0 - chi711L1 - chi712L1);
    B[1][2] = -omega235L1 - omega236L1;

//    std::cout<<"AL = "<<A<<"\n";

    // compute TL1
    A.invert();
    D += B.leftmultiply(C.rightmultiply(A));
    Dune::FieldMatrix<Scalar,dim,2*dim-dim+1> TL1(D);

    Scalar sR = std::abs(TR1[1][2] - TR1[1][0]);
    Scalar sL = std::abs(TL1[1][0] - TL1[1][2]);

    // 3.decide which triangle (which transmissibility coefficients) to use
    if (sR <= sL)
    {
        transmissibility = TR1;
//        if (isnan(transmissibility.frobenius_norm()))
//        {
//                    std::cout<<"right transmissibility = "<<transmissibility<<"\n";
//                std::cout<<"globalPos1 = "<<globalPos1<<"\n";
//                std::cout<<"globalPos2 = "<<globalPos2<<"\n";
//                std::cout<<"globalPos3 = "<<globalPos3<<"\n";
//                std::cout<<"globalPos4 = "<<globalPos4<<"\n";
//                std::cout<<"globalPosCenter = "<<globalPosCenter<<"\n";
//                std::cout<<"outerNormaln1L1 = "<<outerNormaln1L1<<"\n";
//                std::cout<<"globalPosFace14 = "<<globalPosFace14<<"\n";
//                std::cout<<"outerNormaln1R1 = "<<outerNormaln1R1<<"\n";
//                std::cout<<"outerNormaln2 = "<<outerNormaln2<<"\n";
//                std::cout<<"globalPosFace12 = "<<globalPosFace12<<"\n";
//                std::cout<<"globalPosFace23 = "<<globalPosFace23<<"\n";
//                std::cout<<"perm1 = "<<K1<<"\n";
//                std::cout<<"perm2 = "<<K2<<"\n";
//                std::cout<<"perm3 = "<<K3<<"\n";
//                std::cout<<"perm4 = "<<K4<<"\n";
//                std::cout<<"lambda = ";
//                for (int i = 0; i < lambda.size(); i++)
//                {
//                    std::cout<<lambda[i]<<" ";
//                }
//                std::cout<<"\n";
//        }
//        std::cout<<"transmissibility = "<<transmissibility<<"\n";
        return true;
    }
    else
    {
        transmissibility = TL1;
//        if (isnan(transmissibility.frobenius_norm()))
//        {
//                    std::cout<<"left transmissibility = "<<transmissibility<<"\n";
//                std::cout<<"globalPos1 = "<<globalPos1<<"\n";
//                std::cout<<"globalPos2 = "<<globalPos2<<"\n";
//                std::cout<<"globalPos3 = "<<globalPos3<<"\n";
//                std::cout<<"globalPos4 = "<<globalPos4<<"\n";
//                std::cout<<"globalPosCenter = "<<globalPosCenter<<"\n";
//                std::cout<<"outerNormaln1L1 = "<<outerNormaln1L1<<"\n";
//                std::cout<<"globalPosFace14 = "<<globalPosFace14<<"\n";
//                std::cout<<"outerNormaln1R1 = "<<outerNormaln1R1<<"\n";
//                std::cout<<"outerNormaln2 = "<<outerNormaln2<<"\n";
//                std::cout<<"globalPosFace12 = "<<globalPosFace12<<"\n";
//                std::cout<<"globalPosFace23 = "<<globalPosFace23<<"\n";
//                std::cout<<"perm1 = "<<K1<<"\n";
//                std::cout<<"perm2 = "<<K2<<"\n";
//                std::cout<<"perm3 = "<<K3<<"\n";
//                std::cout<<"perm4 = "<<K4<<"\n";
//                std::cout<<"lambda = ";
//                for (int i = 0; i < lambda.size(); i++)
//                {
//                    std::cout<<lambda[i]<<" ";
//                }
//                std::cout<<"\n";
//        }
//        std::cout<<"left transmissibility = "<<transmissibility<<"\n";
        return false;
    }
}

/*! \brief Updates constitutive relations and stores them in the variable class
 *
 * Stores mobility, fractional flow function and capillary pressure for all grid cells.
 */
template<class TypeTag>
void FVMPFAL2PFABoundPressure2P<TypeTag>::updateMaterialLaws()
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
