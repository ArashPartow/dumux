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
#ifndef DUMUX_FVMPFAL2PFABOUND3DPRESSURE2P_HH
#define DUMUX_FVMPFAL2PFABOUND3DPRESSURE2P_HH

// dumux environment
#include <dumux/decoupled/2p/diffusion/diffusionproperties2p.hh>
#include <dumux/decoupled/common/fv/fvpressure.hh>
#include "fvmpfal3dinteractionvolumecontainer.hh"
#include "fvmpfal3dtransmissibilitycalculator.hh"

/**
 * @file
 * @brief  3-d finite Volume-MPFAL implementation of a two-phase pressure equation
 * @brief  Remark1: only for 3-D hexahedrons of quadrilaterals.
 */

namespace Dumux
{
//! \ingroup FVPressure2p
/*! \brief 3-d finite volume MPFA L-method discretization of a two-phase flow pressure equation of the sequential IMPES model
 *
 * Finite Volume-MPFAL-Implementation of the equation
 * \f$ - \text{div}\, \mathbf{v}_t = - \text{div}\, (\lambda_t \mathbf{K} \text{grad}\,
 * \Phi_w + f_n \lambda_t \mathbf{K} \text{grad}\, \Phi_{cap}   ) = 0, \f$,
 * or
 * \f$ - \text{div}\, \mathbf{v}_t = - \text{div}\, (\lambda_t \mathbf{K} \text{grad}\,
 * \Phi_n - f_w \lambda_t \mathbf{K} \text{grad}\, \Phi_{cap}   ) = 0, \f$.
 *
 * \f$ \Phi = g \f$ on \f$ \Gamma_1 \f$, and
 * \f$ - \text{div} \, \mathbf{v}_t \cdot \mathbf{n} = J \f$
 * on \f$ \Gamma_2 \f$.
 *
 * Here, \f$ \Phi_\alpha \f$ denotes the potential of phase \f$ \alpha \f$, \f$ \mathbf{K} \f$ the intrinsic permeability,
 * \f$ \lambda_t \f$ the total mobility, \f$ f_\alpha \f$ the phase fractional flow function.
 *
 * More details on the equations can be found in
 *
 * Wolff 2013: http://elib.uni-stuttgart.de/opus/volltexte/2013/8661/
 *
 * M. Wolff, Y. Cao, B. Flemisch, R. Helmig, and B. Wohlmuth (2013a). Multi-point flux
 * approximation L-method in 3D: numerical convergence and application to two-phase
 * flow through porous media. In P. Bastian, J. Kraus, R. Scheichl, and M. Wheeler,
 * editors, Simulation of Flow in Porous Media - Applications in Energy and Environment. De Gruyter.
 *
 * M. Wolff, B. Flemisch, R. Helmig, I. Aavatsmark.
 * Treatment of tensorial relative permeabilities with multipoint flux approximation.
 * International Journal of Numerical Analysis and Modeling (9), pp. 725-744, 2012.
 *
 * Remark1: only for 3-D hexahedrons of quadrilaterals.
 *
 * \tparam TypeTag The problem Type Tag
 */
template<class TypeTag>
class FvMpfaL3dPressure2p: public FVPressure<TypeTag>
{
    typedef FVPressure<TypeTag> ParentType;
    typedef typename GET_PROP_TYPE(TypeTag, PressureModel) Implementation;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;

    enum
        {
            dim = GridView::dimension, dimWorld = GridView::dimensionworld
        };

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;

#if DUNE_VERSION_NEWER(DUNE_GRID, 2, 3)
    typedef typename Dune::ReferenceElements<Scalar, dim> ReferenceElements;
    typedef typename Dune::ReferenceElement<Scalar, dim> ReferenceElement;
#else
    typedef typename Dune::GenericReferenceElements<Scalar, dim> ReferenceElements;
    typedef typename Dune::GenericReferenceElement<Scalar, dim> ReferenceElement;
#endif

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

    enum
        {
            pw = Indices::pressureW,
            pn = Indices::pressureNw,
            pglobal = Indices::pressureGlobal,
            sw = Indices::saturationW,
            sn = Indices::saturationNw,
            vw = Indices::velocityW,
            vn = Indices::velocityNw,
            vt = Indices::velocityTotal
        };
    enum
        {
            wPhaseIdx = Indices::wPhaseIdx,
            nPhaseIdx = Indices::nPhaseIdx,
            pressureIdx = Indices::pressureIdx,
            saturationIdx = Indices::saturationIdx,
            pressureEqIdx = Indices::pressureEqIdx,
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
    enum
        {
            innerEdgeFace = 2,
            innerSideFace = 1
        };

    typedef typename GridView::Traits::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    typedef typename GridView::template Codim<dim>::Iterator VertexIterator;
    typedef typename GridView::Grid Grid;
    typedef typename Element::Geometry Geometry;
    typedef typename GridView::template Codim<0>::EntityPointer ElementPointer;
    typedef typename GridView::IntersectionIterator IntersectionIterator;

    typedef Dune::FieldVector<Scalar, dim> LocalPosition;
    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;
    typedef Dune::FieldMatrix<Scalar, dim, dim> DimMatrix;

    typedef Dune::FieldVector<Scalar, dim> DimVector;

    typedef typename GET_PROP_TYPE(TypeTag, MPFAInteractionVolumeContainer) InteractionVolumeContainer;
    typedef  Dumux::FvMpfaL3dTransmissibilityCalculator<TypeTag> TransmissibilityCalculator;
public:
    //!< Type including methods for calculation of MPFA transmissibilities
    typedef typename TransmissibilityCalculator::TransmissibilityType TransmissibilityType;
    //!< Type for storing interaction volume information
    typedef typename GET_PROP_TYPE(TypeTag, MPFAInteractionVolume) InteractionVolume;
protected:
    //initializes the matrix to store the system of equations
    friend class FVPressure<TypeTag>;
    void initializeMatrix();
    void initializeMatrixRowSize();
    void initializeMatrixIndices();

    //function which assembles the system of equations to be solved
    void assemble();
    void assembleInnerInteractionVolume(InteractionVolume& interactionVolume);
    void assembleBoundaryInteractionVolume(InteractionVolume& interactionVolume);

public:

    //constitutive functions are initialized and stored in the variables object
    void updateMaterialLaws();

    /*! \brief Initializes the pressure model
     *
     * \copydetails ParentType::initialize()
     */
    void initialize(bool solveTwice = true)
    {
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

        interactionVolumes_.initialize();
        ParentType::initialize();

        updateMaterialLaws();

        asImp_().assemble();
        this->solve();

        storePressureSolution();

        return;
    }

    //! \brief Globally stores the pressure solution
    void storePressureSolution()
    {
        // iterate through leaf grid an evaluate c0 at cell center
        ElementIterator eEndIt = problem_.gridView().template end<0>();
        for (ElementIterator eIt = problem_.gridView().template begin<0>(); eIt != eEndIt; ++eIt)
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

        switch (pressureType_)
        {
        case pw:
        {
            Scalar potW = this->pressure()[globalIdx];

            Scalar gravityDiff = (problem_.bBoxMax() - element.geometry().center()) * gravity_;
            Scalar potPc = cellData.capillaryPressure()
                    + gravityDiff * (density_[nPhaseIdx] - density_[wPhaseIdx]);

            cellData.setPotential(wPhaseIdx, potW);
            cellData.setPotential(nPhaseIdx, potW + potPc);

            Scalar pressW = potW - gravityDiff * density_[wPhaseIdx];

            cellData.setPressure(wPhaseIdx, pressW);
            cellData.setPressure(nPhaseIdx, pressW + cellData.capillaryPressure());

            break;
        }
        case pn:
        {
            Scalar potNw = this->pressure()[globalIdx];

            Scalar gravityDiff = (problem_.bBoxMax() - element.geometry().center()) * gravity_;
            Scalar potPc = cellData.capillaryPressure()
                    + gravityDiff * (density_[nPhaseIdx] - density_[wPhaseIdx]);

            cellData.setPotential(nPhaseIdx, potNw);
            cellData.setPotential(wPhaseIdx, potNw - potPc);

            Scalar pressNw = potNw - gravityDiff * density_[nPhaseIdx];

            cellData.setPressure(wPhaseIdx, pressNw - cellData.capillaryPressure());
            cellData.setPressure(nPhaseIdx, pressNw);

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
        int size = problem_.gridView().size(0);

        //error bounds for error term for incompressible models to correct unphysical saturation
        //over/undershoots due to saturation transport
        timeStep_ = problem_.timeManager().timeStepSize();
        maxError_ = 0.0;
        for (int i = 0; i < size; i++)
        {
            CellData& cellData = problem_.variables().cellData(i);
            Scalar sat = 0;
            switch (saturationType_)
            {
            case sw:
                sat = cellData.saturation(wPhaseIdx);
                break;
            case sn:
                sat = cellData.saturation(nPhaseIdx);
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

            switch (pressureType_)
            {
            case pw:
                this->pressure()[i] = cellData.pressure(wPhaseIdx);
                break;
            case pn:
                this->pressure()[i] = cellData.pressure(nPhaseIdx);
                break;
            }
        }

        asImp_().assemble();

        this->solve();

        storePressureSolution();

        return;
    }

    /* \brief Volume correction term to correct for unphysical saturation overshoots/undershoots.
     *
     * These can occur if the estimated time step for the explicit transport was too large.
     * Correction by an artificial source term allows to correct
     * this errors due to wrong time-stepping without losing mass conservation. The error term looks as follows:
     * \f[
     *  q_{error} = \begin{cases}
     *          S < 0 & \frac{S}{\Delta t} V \\
     *          S > 1 & \frac{(S - 1)}{\Delta t} V \\
     *          0 \le S \le 1 & 0
     *      \end{cases}
     *  \f]
     *
     *  \param cellData The IMPES <tt>CellData</tt> object of the current cell.
     *
     *  \return The scalar value of the error term.
    */
    Scalar evaluateErrorTerm(CellData& cellData)
    {
        //error term for incompressible models to correct unphysical saturation over/undershoots due to saturation transport
        // error reduction routine: volumetric error is damped and inserted to right hand side
        Scalar sat = 0;
        switch (saturationType_)
        {
        case sw:
            sat = cellData.saturation(wPhaseIdx);
            break;
        case sn:
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
            ScalarSolutionType *pc = writer.allocateManagedBuffer(size);

            ElementIterator eItBegin = problem_.gridView().template begin<0>();
            ElementIterator eEndIt = problem_.gridView().template end<0>();
            for (ElementIterator eIt = eItBegin; eIt != eEndIt; ++eIt)
            {
                int idx = problem_.variables().index(*eIt);
                CellData& cellData = problem_.variables().cellData(idx);

                (*pc)[idx] = cellData.capillaryPressure();

                if (pressureType_ == pw)
                {
                    (*pressure)[idx] = cellData.pressure(wPhaseIdx);
                    (*potentialSecond)[idx] = cellData.potential(nPhaseIdx);
                    (*pressureSecond)[idx] = cellData.pressure(nPhaseIdx);
                }

                if (pressureType_ == pn)
                {
                    (*pressure)[idx] = cellData.pressure(nPhaseIdx);
                    (*potentialSecond)[idx] = cellData.potential(wPhaseIdx);
                    (*pressureSecond)[idx] = cellData.pressure(wPhaseIdx);
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

            writer.attachCellData(*pc, "capillary pressure");
        }
    }

    //! Returns the global container of the stored interaction volumes
    InteractionVolumeContainer& interactionVolumes()
    {
        return interactionVolumes_;
    }

    //! Returns the transmissibility calculator
    /*!
     *  Object including methods for the MPFA transmissibility calculation
     */
    TransmissibilityCalculator& transmissibilityCalculator()
    {
        return transmissibilityCalculator_;
    }

    //! Constructs a FvMpfaL3dPressure2p object
    /**
     * \param problem A problem class object
     */
    FvMpfaL3dPressure2p(Problem& problem) :
        ParentType(problem), problem_(problem),
        interactionVolumes_(problem), transmissibilityCalculator_(problem),
        gravity_(problem.gravity()),
        maxError_(0.), timeStep_(1.)
    {
        if (pressureType_ != pw && pressureType_ != pn)
        {
            DUNE_THROW(Dune::NotImplemented, "Pressure type not supported!");
        }
        if (saturationType_ != sw && saturationType_ != sn)
        {
            DUNE_THROW(Dune::NotImplemented, "Saturation type not supported!");
        }
        if (GET_PROP_VALUE(TypeTag, EnableCompressibility))
        {
            DUNE_THROW(Dune::NotImplemented, "Compressibility not supported!");
        }
        if (dim != 3)
        {
            DUNE_THROW(Dune::NotImplemented, "Dimension not supported!");
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

protected:
    InteractionVolumeContainer interactionVolumes_;//!<Global container of the stored interaction volumes
    //!<The transmissibility calculator including methods for the MPFA transmissibility calculation
    TransmissibilityCalculator transmissibilityCalculator_;

private:
    //! Returns the implementation of the problem (i.e. static polymorphism)
    Implementation &asImp_()
    {   return *static_cast<Implementation *>(this);}

    //! \copydoc Dumux::IMPETProblem::asImp_()
    const Implementation &asImp_() const
    {   return *static_cast<const Implementation *>(this);}

    const GlobalPosition& gravity_; //!< vector including the gravity constant

    Scalar maxError_;
    Scalar timeStep_;
    Scalar ErrorTermFactor_;//!< Handling of error term: relaxation factor
    Scalar ErrorTermLowerBound_;//!< Handling of error term: lower bound for error dampening
    Scalar ErrorTermUpperBound_;//!< Handling of error term: upper bound for error dampening

    Scalar density_[numPhases];
    Scalar viscosity_[numPhases];

    int vtkOutputLevel_;

    static constexpr Scalar threshold_ = 1e-15;
    //!< gives kind of pressure used (\f$ 0 = p_w\f$, \f$ 1 = p_n\f$, \f$ 2 = p_{global}\f$)
    static const int pressureType_ = GET_PROP_VALUE(TypeTag, PressureFormulation);
    //!< gives kind of saturation used (\f$ 0 = S_w\f$, \f$ 1 = S_n\f$)
    static const int saturationType_ = GET_PROP_VALUE(TypeTag, SaturationFormulation);
    //!< gives kind of velocity used (\f$ 0 = v_w\f$, \f$ 1 = v_n\f$, \f$ 2 = v_t\f$)
    static const int velocityType_ = GET_PROP_VALUE(TypeTag, VelocityFormulation);
};

//! Initializes the sparse matrix for the pressure solution
template<class TypeTag>
void FvMpfaL3dPressure2p<TypeTag>::initializeMatrix()
{
    initializeMatrixRowSize();
    this->A_.endrowsizes();
    initializeMatrixIndices();
    this->A_.endindices();
}

//! Initializes the row size of the sparse matrix for the pressure solution
template<class TypeTag>
void FvMpfaL3dPressure2p<TypeTag>::initializeMatrixRowSize()
{
    // determine matrix row sizes
    ElementIterator eEndIt = problem_.gridView().template end<0>();
    // determine position of matrix entries
    for (ElementIterator eIt = problem_.gridView().template begin<0>(); eIt != eEndIt; ++eIt)
    {
        // cell index
        int globalIdxI = problem_.variables().index(*eIt);

        std::set<int> neighborIndices;

        int numVertices = eIt->geometry().corners();

        for (int vIdx = 0; vIdx < numVertices; vIdx++)
        {
            int globalVertIdx = problem_.variables().vertexMapper().map(*eIt, vIdx, dim);

            InteractionVolume& interactionVolume = interactionVolumes_.interactionVolume(globalVertIdx);

            for (int subVolumeIdx = 0; subVolumeIdx < InteractionVolume::subVolumeTotalNum; subVolumeIdx++)
            {
                if (interactionVolume.hasSubVolumeElement(subVolumeIdx))
                {
                    const ElementPointer& neighbor = interactionVolume.getSubVolumeElement(subVolumeIdx);
                    int globalIdxJ = problem_.variables().index(*neighbor);

                    neighborIndices.insert(globalIdxJ);
                }
            }
        }

        this->A_.setrowsize(globalIdxI, neighborIndices.size());
    } // end of 'for' ElementIterator

    return;
}

//! Initializes the indices of the sparse matrix for the pressure solution
template<class TypeTag>
void FvMpfaL3dPressure2p<TypeTag>::initializeMatrixIndices()
{
    // determine matrix row sizes
    ElementIterator eEndIt = problem_.gridView().template end<0>();
    // determine position of matrix entries
    for (ElementIterator eIt = problem_.gridView().template begin<0>(); eIt != eEndIt; ++eIt)
    {
        // cell index
        int globalIdxI = problem_.variables().index(*eIt);

        // add diagonal index
        this->A_.addindex(globalIdxI, globalIdxI);

        int numVertices = eIt->geometry().corners();

        for (int vIdx = 0; vIdx < numVertices; vIdx++)
        {
            int globalVertIdx = problem_.variables().vertexMapper().map(*eIt, vIdx, dim);

            InteractionVolume& interactionVolume = interactionVolumes_.interactionVolume(globalVertIdx);
            for (int subVolumeIdx = 0; subVolumeIdx < InteractionVolume::subVolumeTotalNum; subVolumeIdx++)
            {
                if (interactionVolume.hasSubVolumeElement(subVolumeIdx))
                {
                    ElementPointer neighbor = interactionVolume.getSubVolumeElement(subVolumeIdx);
                    int globalIdxJ = problem_.variables().index(*neighbor);

                    this->A_.addindex(globalIdxI, globalIdxJ);
                }
            }
        }
    } // end of 'for' ElementIterator
    return;
}

//! assembles the global matrix and rhs vector for the pressure solution
template<class TypeTag>
void FvMpfaL3dPressure2p<TypeTag>::assemble()
{
    // initialization: set global matrix this->A_ to zero
    this->A_ = 0;
    this->f_ = 0;

    // run through all vertices
    VertexIterator vEndIt = problem_.gridView().template end<dim>();
    for (VertexIterator vIt = problem_.gridView().template begin<dim>(); vIt != vEndIt; ++vIt)
    {
        int globalVertIdx = problem_.variables().index(*vIt);

        InteractionVolume& interactionVolume = interactionVolumes_.interactionVolume(globalVertIdx);

        // inner interactionvolume
        if (interactionVolume.isInnerVolume())
        {
            assembleInnerInteractionVolume(interactionVolume);
        }

        // at least one face on boundary! (boundary interactionvolume)
        else
        {
            assembleBoundaryInteractionVolume(interactionVolume);
        } // end boundaries

    } // end vertex iterator

    return;
}

//! assembles the matrix entries of one interaction volume into the global matrix
template<class TypeTag>
void FvMpfaL3dPressure2p<TypeTag>::assembleInnerInteractionVolume(InteractionVolume& interactionVolume)
{
    ElementPointer& elementPointer1 = interactionVolume.getSubVolumeElement(0);
    ElementPointer& elementPointer2 = interactionVolume.getSubVolumeElement(1);
    ElementPointer& elementPointer3 = interactionVolume.getSubVolumeElement(2);
    ElementPointer& elementPointer4 = interactionVolume.getSubVolumeElement(3);
    ElementPointer& elementPointer5 = interactionVolume.getSubVolumeElement(4);
    ElementPointer& elementPointer6 = interactionVolume.getSubVolumeElement(5);
    ElementPointer& elementPointer7 = interactionVolume.getSubVolumeElement(6);
    ElementPointer& elementPointer8 = interactionVolume.getSubVolumeElement(7);

    // get global coordinate of cell centers
    const GlobalPosition& globalPos1 = elementPointer1->geometry().center();
    const GlobalPosition& globalPos2 = elementPointer2->geometry().center();
    const GlobalPosition& globalPos3 = elementPointer3->geometry().center();
    const GlobalPosition& globalPos4 = elementPointer4->geometry().center();
    const GlobalPosition& globalPos5 = elementPointer5->geometry().center();
    const GlobalPosition& globalPos6 = elementPointer6->geometry().center();
    const GlobalPosition& globalPos7 = elementPointer7->geometry().center();
    const GlobalPosition& globalPos8 = elementPointer8->geometry().center();

    // cell volumes
    Scalar volume1 = elementPointer1->geometry().volume();
    Scalar volume2 = elementPointer2->geometry().volume();
    Scalar volume3 = elementPointer3->geometry().volume();
    Scalar volume4 = elementPointer4->geometry().volume();
    Scalar volume5 = elementPointer5->geometry().volume();
    Scalar volume6 = elementPointer6->geometry().volume();
    Scalar volume7 = elementPointer7->geometry().volume();
    Scalar volume8 = elementPointer8->geometry().volume();

    // cell index
    int globalIdx1 = problem_.variables().index(*elementPointer1);
    int globalIdx2 = problem_.variables().index(*elementPointer2);
    int globalIdx3 = problem_.variables().index(*elementPointer3);
    int globalIdx4 = problem_.variables().index(*elementPointer4);
    int globalIdx5 = problem_.variables().index(*elementPointer5);
    int globalIdx6 = problem_.variables().index(*elementPointer6);
    int globalIdx7 = problem_.variables().index(*elementPointer7);
    int globalIdx8 = problem_.variables().index(*elementPointer8);

    //get the cell Data
    CellData& cellData1 = problem_.variables().cellData(globalIdx1);
    CellData& cellData2 = problem_.variables().cellData(globalIdx2);
    CellData& cellData3 = problem_.variables().cellData(globalIdx3);
    CellData& cellData4 = problem_.variables().cellData(globalIdx4);
    CellData& cellData5 = problem_.variables().cellData(globalIdx5);
    CellData& cellData6 = problem_.variables().cellData(globalIdx6);
    CellData& cellData7 = problem_.variables().cellData(globalIdx7);
    CellData& cellData8 = problem_.variables().cellData(globalIdx8);

    // get mobilities of the phases
    Dune::FieldVector<Scalar, numPhases> lambda1(cellData1.mobility(wPhaseIdx));
    lambda1[nPhaseIdx] = cellData1.mobility(nPhaseIdx);

    // compute total mobility of cell 1
    Scalar lambdaTotal1 = lambda1[wPhaseIdx] + lambda1[nPhaseIdx];

    // get mobilities of the phases
    Dune::FieldVector<Scalar, numPhases> lambda2(cellData2.mobility(wPhaseIdx));
    lambda2[nPhaseIdx] = cellData2.mobility(nPhaseIdx);

    // compute total mobility of cell 2
    Scalar lambdaTotal2 = lambda2[wPhaseIdx] + lambda2[nPhaseIdx];

    // get mobilities of the phases
    Dune::FieldVector<Scalar, numPhases> lambda3(cellData3.mobility(wPhaseIdx));
    lambda3[nPhaseIdx] = cellData3.mobility(nPhaseIdx);

    // compute total mobility of cell 3
    Scalar lambdaTotal3 = lambda3[wPhaseIdx] + lambda3[nPhaseIdx];

    // get mobilities of the phases
    Dune::FieldVector<Scalar, numPhases> lambda4(cellData4.mobility(wPhaseIdx));
    lambda4[nPhaseIdx] = cellData4.mobility(nPhaseIdx);

    // compute total mobility of cell 4
    Scalar lambdaTotal4 = lambda4[wPhaseIdx] + lambda4[nPhaseIdx];

    // get mobilities of the phases
    Dune::FieldVector<Scalar, numPhases> lambda5(cellData5.mobility(wPhaseIdx));
    lambda5[nPhaseIdx] = cellData5.mobility(nPhaseIdx);

    // compute total mobility of cell 5
    Scalar lambdaTotal5 = lambda5[wPhaseIdx] + lambda5[nPhaseIdx];

    // get mobilities of the phases
    Dune::FieldVector<Scalar, numPhases> lambda6(cellData6.mobility(wPhaseIdx));
    lambda6[nPhaseIdx] = cellData6.mobility(nPhaseIdx);

    // compute total mobility of cell 6
    Scalar lambdaTotal6 = lambda6[wPhaseIdx] + lambda6[nPhaseIdx];

    // get mobilities of the phases
    Dune::FieldVector<Scalar, numPhases> lambda7(cellData7.mobility(wPhaseIdx));
    lambda7[nPhaseIdx] = cellData7.mobility(nPhaseIdx);

    // compute total mobility of cell 7
    Scalar lambdaTotal7 = lambda7[wPhaseIdx] + lambda7[nPhaseIdx];

    // get mobilities of the phases
    Dune::FieldVector<Scalar, numPhases> lambda8(cellData8.mobility(wPhaseIdx));
    lambda8[nPhaseIdx] = cellData8.mobility(nPhaseIdx);

    // compute total mobility of cell 8
    Scalar lambdaTotal8 = lambda8[wPhaseIdx] + lambda8[nPhaseIdx];

    std::vector<DimVector> lambda(8);
    lambda[0][0] = lambdaTotal1;
    lambda[0][1] = lambdaTotal1;
    lambda[0][2] = lambdaTotal1;
    lambda[1][0] = lambdaTotal2;
    lambda[1][1] = lambdaTotal2;
    lambda[1][2] = lambdaTotal2;
    lambda[2][0] = lambdaTotal3;
    lambda[2][1] = lambdaTotal3;
    lambda[2][2] = lambdaTotal3;
    lambda[3][0] = lambdaTotal4;
    lambda[3][1] = lambdaTotal4;
    lambda[3][2] = lambdaTotal4;
    lambda[4][0] = lambdaTotal5;
    lambda[4][1] = lambdaTotal5;
    lambda[4][2] = lambdaTotal5;
    lambda[5][0] = lambdaTotal6;
    lambda[5][1] = lambdaTotal6;
    lambda[5][2] = lambdaTotal6;
    lambda[6][0] = lambdaTotal7;
    lambda[6][1] = lambdaTotal7;
    lambda[6][2] = lambdaTotal7;
    lambda[7][0] = lambdaTotal8;
    lambda[7][1] = lambdaTotal8;
    lambda[7][2] = lambdaTotal8;

    // add capillary pressure and gravity terms to right-hand-side
    // calculate capillary pressure velocity
    Dune::FieldVector<Scalar, 8> pc(0);
    pc[0] = cellData1.capillaryPressure();
    pc[1] = cellData2.capillaryPressure();
    pc[2] = cellData3.capillaryPressure();
    pc[3] = cellData4.capillaryPressure();
    pc[4] = cellData5.capillaryPressure();
    pc[5] = cellData6.capillaryPressure();
    pc[6] = cellData7.capillaryPressure();
    pc[7] = cellData8.capillaryPressure();

    Dune::FieldVector<Scalar, 8> gravityDiff(0);

    gravityDiff[0] = (problem_.bBoxMax() - globalPos1) * gravity_ * (density_[nPhaseIdx] - density_[wPhaseIdx]);
    gravityDiff[1] = (problem_.bBoxMax() - globalPos2) * gravity_ * (density_[nPhaseIdx] - density_[wPhaseIdx]);
    gravityDiff[2] = (problem_.bBoxMax() - globalPos3) * gravity_ * (density_[nPhaseIdx] - density_[wPhaseIdx]);
    gravityDiff[3] = (problem_.bBoxMax() - globalPos4) * gravity_ * (density_[nPhaseIdx] - density_[wPhaseIdx]);
    gravityDiff[4] = (problem_.bBoxMax() - globalPos5) * gravity_ * (density_[nPhaseIdx] - density_[wPhaseIdx]);
    gravityDiff[5] = (problem_.bBoxMax() - globalPos6) * gravity_ * (density_[nPhaseIdx] - density_[wPhaseIdx]);
    gravityDiff[6] = (problem_.bBoxMax() - globalPos7) * gravity_ * (density_[nPhaseIdx] - density_[wPhaseIdx]);
    gravityDiff[7] = (problem_.bBoxMax() - globalPos8) * gravity_ * (density_[nPhaseIdx] - density_[wPhaseIdx]);

    pc += gravityDiff;

    Dune::FieldVector<Dune::FieldVector<Scalar, 3>, 8> pcFlux(Dune::FieldVector<Scalar, 3>(0));

    Scalar pcPotential0 = 0;
    Scalar pcPotential1 = 0;
    Scalar pcPotential2 = 0;
    Scalar pcPotential3 = 0;
    Scalar pcPotential4 = 0;
    Scalar pcPotential5 = 0;
    Scalar pcPotential6 = 0;
    Scalar pcPotential7 = 0;
    Scalar pcPotential8 = 0;
    Scalar pcPotential9 = 0;
    Scalar pcPotential10 = 0;
    Scalar pcPotential11 = 0;

    //                interactionVolume.printInteractionVolumeInfo();
    // evaluate right hand side
    PrimaryVariables source(0.0);
    problem_.source(source, *elementPointer1);
    this->f_[globalIdx1] += volume1 / (8.0)
        * (source[wPhaseIdx] / density_[wPhaseIdx] + source[nPhaseIdx] / density_[nPhaseIdx]);
    problem_.source(source, *elementPointer2);
    this->f_[globalIdx2] += volume2 / (8.0)
        * (source[wPhaseIdx] / density_[wPhaseIdx] + source[nPhaseIdx] / density_[nPhaseIdx]);
    problem_.source(source, *elementPointer3);
    this->f_[globalIdx3] += volume3 / (8.0)
        * (source[wPhaseIdx] / density_[wPhaseIdx] + source[nPhaseIdx] / density_[nPhaseIdx]);
    problem_.source(source, *elementPointer4);
    this->f_[globalIdx4] += volume4 / (8.0)
        * (source[wPhaseIdx] / density_[wPhaseIdx] + source[nPhaseIdx] / density_[nPhaseIdx]);
    problem_.source(source, *elementPointer5);
    this->f_[globalIdx5] += volume5 / (8.0)
        * (source[wPhaseIdx] / density_[wPhaseIdx] + source[nPhaseIdx] / density_[nPhaseIdx]);
    problem_.source(source, *elementPointer6);
    this->f_[globalIdx6] += volume6 / (8.0)
        * (source[wPhaseIdx] / density_[wPhaseIdx] + source[nPhaseIdx] / density_[nPhaseIdx]);
    problem_.source(source, *elementPointer7);
    this->f_[globalIdx7] += volume7 / (8.0)
        * (source[wPhaseIdx] / density_[wPhaseIdx] + source[nPhaseIdx] / density_[nPhaseIdx]);
    problem_.source(source, *elementPointer8);
    this->f_[globalIdx8] += volume8 / (8.0)
        * (source[wPhaseIdx] / density_[wPhaseIdx] + source[nPhaseIdx] / density_[nPhaseIdx]);

    this->f_[globalIdx1] += evaluateErrorTerm(cellData1) * volume1 / (8.0);
    this->f_[globalIdx2] += evaluateErrorTerm(cellData2) * volume2 / (8.0);
    this->f_[globalIdx3] += evaluateErrorTerm(cellData3) * volume3 / (8.0);
    this->f_[globalIdx4] += evaluateErrorTerm(cellData4) * volume4 / (8.0);
    this->f_[globalIdx5] += evaluateErrorTerm(cellData5) * volume5 / (8.0);
    this->f_[globalIdx6] += evaluateErrorTerm(cellData6) * volume6 / (8.0);
    this->f_[globalIdx7] += evaluateErrorTerm(cellData7) * volume7 / (8.0);
    this->f_[globalIdx8] += evaluateErrorTerm(cellData8) * volume8 / (8.0);

    DimVector Tu(0);
    Dune::FieldVector<Scalar, 2 * dim - dim + 1> u(0);
    TransmissibilityType T(0);
    TransmissibilityType TSecond(0);

    // calculate the flux through the subvolumeface 1 (subVolumeFaceIdx = 0)
    int caseL = transmissibilityCalculator_.transmissibility(T, interactionVolume, lambda, 0, 1, 2, 3, 4,
                                                             5);

    TSecond = T;
    T *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx1, 0, 0);
    TSecond *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx2, 1, 1);

    if (caseL == 1)
    {
        this->A_[globalIdx1][globalIdx1] += T[0][0];
        this->A_[globalIdx1][globalIdx2] += T[0][1];
        this->A_[globalIdx1][globalIdx3] += T[0][2];
        this->A_[globalIdx1][globalIdx5] += T[0][3];

        this->A_[globalIdx2][globalIdx1] -= TSecond[0][0];
        this->A_[globalIdx2][globalIdx2] -= TSecond[0][1];
        this->A_[globalIdx2][globalIdx3] -= TSecond[0][2];
        this->A_[globalIdx2][globalIdx5] -= TSecond[0][3];

        u[0] = pc[0];
        u[1] = pc[1];
        u[2] = pc[2];
        u[3] = pc[4];

        T.mv(u, Tu);

        pcFlux[0][0] = Tu[0];
        pcPotential0 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[1][1]= Tu[0];
    }
    else if (caseL == 2)
    {
        this->A_[globalIdx1][globalIdx1] += T[0][0];
        this->A_[globalIdx1][globalIdx2] += T[0][1];
        this->A_[globalIdx1][globalIdx4] += T[0][2];
        this->A_[globalIdx1][globalIdx6] += T[0][3];

        this->A_[globalIdx2][globalIdx1] -= TSecond[0][0];
        this->A_[globalIdx2][globalIdx2] -= TSecond[0][1];
        this->A_[globalIdx2][globalIdx4] -= TSecond[0][2];
        this->A_[globalIdx2][globalIdx6] -= TSecond[0][3];

        u[0] = pc[0];
        u[1] = pc[1];
        u[2] = pc[3];
        u[3] = pc[5];

        T.mv(u, Tu);
        pcFlux[0][0] = Tu[0];
        pcPotential0 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[1][1]= Tu[0];
    }
    else if (caseL == 3)
    {
        this->A_[globalIdx1][globalIdx1] += T[0][0];
        this->A_[globalIdx1][globalIdx2] += T[0][1];
        this->A_[globalIdx1][globalIdx4] += T[0][2];
        this->A_[globalIdx1][globalIdx5] += T[0][3];

        this->A_[globalIdx2][globalIdx1] -= TSecond[0][0];
        this->A_[globalIdx2][globalIdx2] -= TSecond[0][1];
        this->A_[globalIdx2][globalIdx4] -= TSecond[0][2];
        this->A_[globalIdx2][globalIdx5] -= TSecond[0][3];

        u[0] = pc[0];
        u[1] = pc[1];
        u[2] = pc[3];
        u[3] = pc[4];

        T.mv(u, Tu);
        pcFlux[0][0] = Tu[0];
        pcPotential0 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[1][1]= Tu[0];
    }
    else
    {
        this->A_[globalIdx1][globalIdx1] += T[0][0];
        this->A_[globalIdx1][globalIdx2] += T[0][1];
        this->A_[globalIdx1][globalIdx3] += T[0][2];
        this->A_[globalIdx1][globalIdx6] += T[0][3];

        this->A_[globalIdx2][globalIdx1] -= TSecond[0][0];
        this->A_[globalIdx2][globalIdx2] -= TSecond[0][1];
        this->A_[globalIdx2][globalIdx3] -= TSecond[0][2];
        this->A_[globalIdx2][globalIdx6] -= TSecond[0][3];

        u[0] = pc[0];
        u[1] = pc[1];
        u[2] = pc[2];
        u[3] = pc[5];

        T.mv(u, Tu);
        pcFlux[0][0] = Tu[0];
        pcPotential0 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[1][1]= Tu[0];
    }

    // calculate the flux through the subvolumeface 2 (subVolumeFaceIdx = 1)
    caseL = transmissibilityCalculator_.transmissibility(T, interactionVolume, lambda, 1, 3, 0, 2, 5, 7);

    TSecond = T;
    T *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx2, 1, 0);
    TSecond *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx4, 3, 1);

    if (caseL == 1)
    {
        this->A_[globalIdx2][globalIdx2] += T[0][0];
        this->A_[globalIdx2][globalIdx4] += T[0][1];
        this->A_[globalIdx2][globalIdx1] += T[0][2];
        this->A_[globalIdx2][globalIdx6] += T[0][3];

        this->A_[globalIdx4][globalIdx2] -= TSecond[0][0];
        this->A_[globalIdx4][globalIdx4] -= TSecond[0][1];
        this->A_[globalIdx4][globalIdx1] -= TSecond[0][2];
        this->A_[globalIdx4][globalIdx6] -= TSecond[0][3];

        u[0] = pc[1];
        u[1] = pc[3];
        u[2] = pc[0];
        u[3] = pc[5];

        T.mv(u, Tu);
        pcFlux[1][0] = Tu[0];
        pcPotential1 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[3][1] = Tu[0];
    }
    else if (caseL == 2)
    {
        this->A_[globalIdx2][globalIdx2] += T[0][0];
        this->A_[globalIdx2][globalIdx4] += T[0][1];
        this->A_[globalIdx2][globalIdx3] += T[0][2];
        this->A_[globalIdx2][globalIdx8] += T[0][3];

        this->A_[globalIdx4][globalIdx2] -= TSecond[0][0];
        this->A_[globalIdx4][globalIdx4] -= TSecond[0][1];
        this->A_[globalIdx4][globalIdx3] -= TSecond[0][2];
        this->A_[globalIdx4][globalIdx8] -= TSecond[0][3];

        u[0] = pc[1];
        u[1] = pc[3];
        u[2] = pc[2];
        u[3] = pc[7];

        T.mv(u, Tu);
        pcFlux[1][0] = Tu[0];
        pcPotential1 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[3][1] = Tu[0];
    }
    else if (caseL == 3)
    {
        this->A_[globalIdx2][globalIdx2] += T[0][0];
        this->A_[globalIdx2][globalIdx4] += T[0][1];
        this->A_[globalIdx2][globalIdx3] += T[0][2];
        this->A_[globalIdx2][globalIdx6] += T[0][3];

        this->A_[globalIdx4][globalIdx2] -= TSecond[0][0];
        this->A_[globalIdx4][globalIdx4] -= TSecond[0][1];
        this->A_[globalIdx4][globalIdx3] -= TSecond[0][2];
        this->A_[globalIdx4][globalIdx6] -= TSecond[0][3];

        u[0] = pc[1];
        u[1] = pc[3];
        u[2] = pc[2];
        u[3] = pc[5];

        T.mv(u, Tu);
        pcFlux[1][0] = Tu[0];
        pcPotential1 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[3][1] = Tu[0];
    }
    else
    {
        this->A_[globalIdx2][globalIdx2] += T[0][0];
        this->A_[globalIdx2][globalIdx4] += T[0][1];
        this->A_[globalIdx2][globalIdx1] += T[0][2];
        this->A_[globalIdx2][globalIdx8] += T[0][3];

        this->A_[globalIdx4][globalIdx2] -= TSecond[0][0];
        this->A_[globalIdx4][globalIdx4] -= TSecond[0][1];
        this->A_[globalIdx4][globalIdx1] -= TSecond[0][2];
        this->A_[globalIdx4][globalIdx8] -= TSecond[0][3];

        u[0] = pc[1];
        u[1] = pc[3];
        u[2] = pc[0];
        u[3] = pc[7];

        T.mv(u, Tu);
        pcFlux[1][0] = Tu[0];
        pcPotential1 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[3][1] = Tu[0];
    }

    // calculate the flux through the subvolumeface 3 (subVolumeFaceIdx = 2)
    caseL = transmissibilityCalculator_.transmissibility(T, interactionVolume, lambda, 3, 2, 1, 0, 7, 6);

    TSecond = T;
    T *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx4, 3, 0);
    TSecond *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx3, 2, 1);

    if (caseL == 1)
    {
        this->A_[globalIdx4][globalIdx4] += T[0][0];
        this->A_[globalIdx4][globalIdx3] += T[0][1];
        this->A_[globalIdx4][globalIdx2] += T[0][2];
        this->A_[globalIdx4][globalIdx8] += T[0][3];

        this->A_[globalIdx3][globalIdx4] -= TSecond[0][0];
        this->A_[globalIdx3][globalIdx3] -= TSecond[0][1];
        this->A_[globalIdx3][globalIdx2] -= TSecond[0][2];
        this->A_[globalIdx3][globalIdx8] -= TSecond[0][3];

        u[0] = pc[3];
        u[1] = pc[2];
        u[2] = pc[1];
        u[3] = pc[7];

        T.mv(u, Tu);
        pcPotential2 = Tu[0];
        pcFlux[3][0] = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[2][1] = Tu[0];
    }
    else if (caseL == 2)
    {
        this->A_[globalIdx4][globalIdx4] += T[0][0];
        this->A_[globalIdx4][globalIdx3] += T[0][1];
        this->A_[globalIdx4][globalIdx1] += T[0][2];
        this->A_[globalIdx4][globalIdx7] += T[0][3];

        this->A_[globalIdx3][globalIdx4] -= TSecond[0][0];
        this->A_[globalIdx3][globalIdx3] -= TSecond[0][1];
        this->A_[globalIdx3][globalIdx1] -= TSecond[0][2];
        this->A_[globalIdx3][globalIdx7] -= TSecond[0][3];

        u[0] = pc[3];
        u[1] = pc[2];
        u[2] = pc[0];
        u[3] = pc[6];

        T.mv(u, Tu);
        pcPotential2 = Tu[0];
        pcFlux[3][0] = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[2][1] = Tu[0];
    }
    else if (caseL == 3)
    {
        this->A_[globalIdx4][globalIdx4] += T[0][0];
        this->A_[globalIdx4][globalIdx3] += T[0][1];
        this->A_[globalIdx4][globalIdx1] += T[0][2];
        this->A_[globalIdx4][globalIdx8] += T[0][3];

        this->A_[globalIdx3][globalIdx4] -= TSecond[0][0];
        this->A_[globalIdx3][globalIdx3] -= TSecond[0][1];
        this->A_[globalIdx3][globalIdx1] -= TSecond[0][2];
        this->A_[globalIdx3][globalIdx8] -= TSecond[0][3];

        u[0] = pc[3];
        u[1] = pc[2];
        u[2] = pc[0];
        u[3] = pc[7];

        T.mv(u, Tu);
        pcPotential2 = Tu[0];
        pcFlux[3][0] = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[2][1] = Tu[0];
    }
    else
    {
        this->A_[globalIdx4][globalIdx4] += T[0][0];
        this->A_[globalIdx4][globalIdx3] += T[0][1];
        this->A_[globalIdx4][globalIdx2] += T[0][2];
        this->A_[globalIdx4][globalIdx7] += T[0][3];

        this->A_[globalIdx3][globalIdx4] -= TSecond[0][0];
        this->A_[globalIdx3][globalIdx3] -= TSecond[0][1];
        this->A_[globalIdx3][globalIdx2] -= TSecond[0][2];
        this->A_[globalIdx3][globalIdx7] -= TSecond[0][3];

        u[0] = pc[3];
        u[1] = pc[2];
        u[2] = pc[1];
        u[3] = pc[6];

        T.mv(u, Tu);
        pcPotential2 = Tu[0];
        pcFlux[3][0] = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[2][1] = Tu[0];
    }

    // calculate the flux through the subvolumeface 4 (subVolumeFaceIdx = 3)
    caseL = transmissibilityCalculator_.transmissibility(T, interactionVolume, lambda, 2, 0, 3, 1, 6, 4);

    TSecond = T;
    T *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx3, 2, 0);
    TSecond *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx1, 0, 1);

    if (caseL == 1)
    {
        this->A_[globalIdx3][globalIdx3] += T[0][0];
        this->A_[globalIdx3][globalIdx1] += T[0][1];
        this->A_[globalIdx3][globalIdx4] += T[0][2];
        this->A_[globalIdx3][globalIdx7] += T[0][3];

        this->A_[globalIdx1][globalIdx3] -= TSecond[0][0];
        this->A_[globalIdx1][globalIdx1] -= TSecond[0][1];
        this->A_[globalIdx1][globalIdx4] -= TSecond[0][2];
        this->A_[globalIdx1][globalIdx7] -= TSecond[0][3];

        u[0] = pc[2];
        u[1] = pc[0];
        u[2] = pc[3];
        u[3] = pc[6];

        T.mv(u, Tu);
        pcFlux[2][0] = Tu[0];
        pcPotential3 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[0][1] = Tu[0];
    }
    else if (caseL == 2)
    {
        this->A_[globalIdx3][globalIdx3] += T[0][0];
        this->A_[globalIdx3][globalIdx1] += T[0][1];
        this->A_[globalIdx3][globalIdx2] += T[0][2];
        this->A_[globalIdx3][globalIdx5] += T[0][3];

        this->A_[globalIdx1][globalIdx3] -= TSecond[0][0];
        this->A_[globalIdx1][globalIdx1] -= TSecond[0][1];
        this->A_[globalIdx1][globalIdx2] -= TSecond[0][2];
        this->A_[globalIdx1][globalIdx5] -= TSecond[0][3];

        u[0] = pc[2];
        u[1] = pc[0];
        u[2] = pc[1];
        u[3] = pc[4];

        T.mv(u, Tu);
        pcFlux[2][0] = Tu[0];
        pcPotential3 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[0][1] = Tu[0];
    }
    else if (caseL == 3)
    {
        this->A_[globalIdx3][globalIdx3] += T[0][0];
        this->A_[globalIdx3][globalIdx1] += T[0][1];
        this->A_[globalIdx3][globalIdx2] += T[0][2];
        this->A_[globalIdx3][globalIdx7] += T[0][3];

        this->A_[globalIdx1][globalIdx3] -= TSecond[0][0];
        this->A_[globalIdx1][globalIdx1] -= TSecond[0][1];
        this->A_[globalIdx1][globalIdx2] -= TSecond[0][2];
        this->A_[globalIdx1][globalIdx7] -= TSecond[0][3];

        u[0] = pc[2];
        u[1] = pc[0];
        u[2] = pc[1];
        u[3] = pc[6];

        T.mv(u, Tu);
        pcFlux[2][0] = Tu[0];
        pcPotential3 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[0][1] = Tu[0];
    }
    else
    {
        this->A_[globalIdx3][globalIdx3] += T[0][0];
        this->A_[globalIdx3][globalIdx1] += T[0][1];
        this->A_[globalIdx3][globalIdx4] += T[0][2];
        this->A_[globalIdx3][globalIdx5] += T[0][3];

        this->A_[globalIdx1][globalIdx3] -= TSecond[0][0];
        this->A_[globalIdx1][globalIdx1] -= TSecond[0][1];
        this->A_[globalIdx1][globalIdx4] -= TSecond[0][2];
        this->A_[globalIdx1][globalIdx5] -= TSecond[0][3];

        u[0] = pc[2];
        u[1] = pc[0];
        u[2] = pc[3];
        u[3] = pc[4];

        T.mv(u, Tu);
        pcFlux[2][0] = Tu[0];
        pcPotential3 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[0][1] = Tu[0];
    }

    // calculate the flux through the subvolumeface 5 (subVolumeFaceIdx = 4)
    caseL = transmissibilityCalculator_.transmissibility(T, interactionVolume, lambda, 5, 4, 7, 6, 1, 0);

    TSecond = T;
    T *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx6, 5, 2);
    TSecond *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx5, 4, 1);

    if (caseL == 1)
    {
        this->A_[globalIdx6][globalIdx6] += T[0][0];
        this->A_[globalIdx6][globalIdx5] += T[0][1];
        this->A_[globalIdx6][globalIdx8] += T[0][2];
        this->A_[globalIdx6][globalIdx2] += T[0][3];

        this->A_[globalIdx5][globalIdx6] -= TSecond[0][0];
        this->A_[globalIdx5][globalIdx5] -= TSecond[0][1];
        this->A_[globalIdx5][globalIdx8] -= TSecond[0][2];
        this->A_[globalIdx5][globalIdx2] -= TSecond[0][3];

        u[0] = pc[5];
        u[1] = pc[4];
        u[2] = pc[7];
        u[3] = pc[1];

        T.mv(u, Tu);
        pcFlux[5][2] = Tu[0];
        pcPotential4 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[4][1] = Tu[0];
    }
    else if (caseL == 2)
    {
        this->A_[globalIdx6][globalIdx6] += T[0][0];
        this->A_[globalIdx6][globalIdx5] += T[0][1];
        this->A_[globalIdx6][globalIdx7] += T[0][2];
        this->A_[globalIdx6][globalIdx1] += T[0][3];

        this->A_[globalIdx5][globalIdx6] -= TSecond[0][0];
        this->A_[globalIdx5][globalIdx5] -= TSecond[0][1];
        this->A_[globalIdx5][globalIdx7] -= TSecond[0][2];
        this->A_[globalIdx5][globalIdx1] -= TSecond[0][3];

        u[0] = pc[5];
        u[1] = pc[4];
        u[2] = pc[6];
        u[3] = pc[0];

        T.mv(u, Tu);
        pcFlux[5][2] = Tu[0];
        pcPotential4 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[4][1] = Tu[0];
    }
    else if (caseL == 3)
    {
        this->A_[globalIdx6][globalIdx6] += T[0][0];
        this->A_[globalIdx6][globalIdx5] += T[0][1];
        this->A_[globalIdx6][globalIdx7] += T[0][2];
        this->A_[globalIdx6][globalIdx2] += T[0][3];

        this->A_[globalIdx5][globalIdx6] -= TSecond[0][0];
        this->A_[globalIdx5][globalIdx5] -= TSecond[0][1];
        this->A_[globalIdx5][globalIdx7] -= TSecond[0][2];
        this->A_[globalIdx5][globalIdx2] -= TSecond[0][3];

        u[0] = pc[5];
        u[1] = pc[4];
        u[2] = pc[6];
        u[3] = pc[1];

        T.mv(u, Tu);
        pcFlux[5][2] = Tu[0];
        pcPotential4 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[4][1] = Tu[0];
    }
    else
    {
        this->A_[globalIdx6][globalIdx6] += T[0][0];
        this->A_[globalIdx6][globalIdx5] += T[0][1];
        this->A_[globalIdx6][globalIdx8] += T[0][2];
        this->A_[globalIdx6][globalIdx1] += T[0][3];

        this->A_[globalIdx5][globalIdx6] -= TSecond[0][0];
        this->A_[globalIdx5][globalIdx5] -= TSecond[0][1];
        this->A_[globalIdx5][globalIdx8] -= TSecond[0][2];
        this->A_[globalIdx5][globalIdx1] -= TSecond[0][3];

        u[0] = pc[5];
        u[1] = pc[4];
        u[2] = pc[7];
        u[3] = pc[0];

        T.mv(u, Tu);
        pcFlux[5][2] = Tu[0];
        pcPotential4 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[4][1] = Tu[0];
    }

    // calculate the flux through the subvolumeface 6 (subVolumeFaceIdx = 5)
    caseL = transmissibilityCalculator_.transmissibility(T, interactionVolume, lambda, 7, 5, 6, 4, 3, 1);

    TSecond = T;
    T *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx8, 7, 2);
    TSecond *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx6, 5, 1);


    if (caseL == 1)
    {
        this->A_[globalIdx8][globalIdx8] += T[0][0];
        this->A_[globalIdx8][globalIdx6] += T[0][1];
        this->A_[globalIdx8][globalIdx7] += T[0][2];
        this->A_[globalIdx8][globalIdx4] += T[0][3];

        this->A_[globalIdx6][globalIdx8] -= TSecond[0][0];
        this->A_[globalIdx6][globalIdx6] -= TSecond[0][1];
        this->A_[globalIdx6][globalIdx7] -= TSecond[0][2];
        this->A_[globalIdx6][globalIdx4] -= TSecond[0][3];

        u[0] = pc[7];
        u[1] = pc[5];
        u[2] = pc[6];
        u[3] = pc[3];

        T.mv(u, Tu);
        pcFlux[7][2] = Tu[0];
        pcPotential5 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[5][1] = Tu[0];
    }
    else if (caseL == 2)
    {
        this->A_[globalIdx8][globalIdx8] += T[0][0];
        this->A_[globalIdx8][globalIdx6] += T[0][1];
        this->A_[globalIdx8][globalIdx5] += T[0][2];
        this->A_[globalIdx8][globalIdx2] += T[0][3];

        this->A_[globalIdx6][globalIdx8] -= TSecond[0][0];
        this->A_[globalIdx6][globalIdx6] -= TSecond[0][1];
        this->A_[globalIdx6][globalIdx5] -= TSecond[0][2];
        this->A_[globalIdx6][globalIdx2] -= TSecond[0][3];

        u[0] = pc[7];
        u[1] = pc[5];
        u[2] = pc[4];
        u[3] = pc[1];

        T.mv(u, Tu);
        pcFlux[7][2] = Tu[0];
        pcPotential5 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[5][1] = Tu[0];
    }
    else if (caseL == 3)
    {
        this->A_[globalIdx8][globalIdx8] += T[0][0];
        this->A_[globalIdx8][globalIdx6] += T[0][1];
        this->A_[globalIdx8][globalIdx5] += T[0][2];
        this->A_[globalIdx8][globalIdx4] += T[0][3];

        this->A_[globalIdx6][globalIdx8] -= TSecond[0][0];
        this->A_[globalIdx6][globalIdx6] -= TSecond[0][1];
        this->A_[globalIdx6][globalIdx5] -= TSecond[0][2];
        this->A_[globalIdx6][globalIdx4] -= TSecond[0][3];

        u[0] = pc[7];
        u[1] = pc[5];
        u[2] = pc[4];
        u[3] = pc[3];

        T.mv(u, Tu);
        pcFlux[7][2] = Tu[0];
        pcPotential5 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[5][1] = Tu[0];
    }
    else
    {
        this->A_[globalIdx8][globalIdx8] += T[0][0];
        this->A_[globalIdx8][globalIdx6] += T[0][1];
        this->A_[globalIdx8][globalIdx7] += T[0][2];
        this->A_[globalIdx8][globalIdx2] += T[0][3];

        this->A_[globalIdx6][globalIdx8] -= TSecond[0][0];
        this->A_[globalIdx6][globalIdx6] -= TSecond[0][1];
        this->A_[globalIdx6][globalIdx7] -= TSecond[0][2];
        this->A_[globalIdx6][globalIdx2] -= TSecond[0][3];

        u[0] = pc[7];
        u[1] = pc[5];
        u[2] = pc[6];
        u[3] = pc[1];

        T.mv(u, Tu);
        pcFlux[7][2] = Tu[0];
        pcPotential5 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[5][1] = Tu[0];
    }

    // calculate the flux through the subvolumeface 7 (subVolumeFaceIdx = 6)
    caseL = transmissibilityCalculator_.transmissibility(T, interactionVolume, lambda, 6, 7, 4, 5, 2, 3);

    TSecond = T;
    T *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx7, 6, 2);
    TSecond *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx8, 7, 1);

    if (caseL == 1)
    {
        this->A_[globalIdx7][globalIdx7] += T[0][0];
        this->A_[globalIdx7][globalIdx8] += T[0][1];
        this->A_[globalIdx7][globalIdx5] += T[0][2];
        this->A_[globalIdx7][globalIdx3] += T[0][3];

        this->A_[globalIdx8][globalIdx7] -= TSecond[0][0];
        this->A_[globalIdx8][globalIdx8] -= TSecond[0][1];
        this->A_[globalIdx8][globalIdx5] -= TSecond[0][2];
        this->A_[globalIdx8][globalIdx3] -= TSecond[0][3];

        u[0] = pc[6];
        u[1] = pc[7];
        u[2] = pc[4];
        u[3] = pc[2];

        T.mv(u, Tu);
        pcFlux[6][2] = Tu[0];
        pcPotential6 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[7][1] = Tu[0];
    }
    else if (caseL == 2)
    {
        this->A_[globalIdx7][globalIdx7] += T[0][0];
        this->A_[globalIdx7][globalIdx8] += T[0][1];
        this->A_[globalIdx7][globalIdx6] += T[0][2];
        this->A_[globalIdx7][globalIdx4] += T[0][3];

        this->A_[globalIdx8][globalIdx7] -= TSecond[0][0];
        this->A_[globalIdx8][globalIdx8] -= TSecond[0][1];
        this->A_[globalIdx8][globalIdx6] -= TSecond[0][2];
        this->A_[globalIdx8][globalIdx4] -= TSecond[0][3];

        u[0] = pc[6];
        u[1] = pc[7];
        u[2] = pc[5];
        u[3] = pc[3];

        T.mv(u, Tu);
        pcFlux[6][2] = Tu[0];
        pcPotential6 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[7][1] = Tu[0];
    }
    else if (caseL == 3)
    {
        this->A_[globalIdx7][globalIdx7] += T[0][0];
        this->A_[globalIdx7][globalIdx8] += T[0][1];
        this->A_[globalIdx7][globalIdx6] += T[0][2];
        this->A_[globalIdx7][globalIdx3] += T[0][3];

        this->A_[globalIdx8][globalIdx7] -= TSecond[0][0];
        this->A_[globalIdx8][globalIdx8] -= TSecond[0][1];
        this->A_[globalIdx8][globalIdx6] -= TSecond[0][2];
        this->A_[globalIdx8][globalIdx3] -= TSecond[0][3];

        u[0] = pc[6];
        u[1] = pc[7];
        u[2] = pc[5];
        u[3] = pc[2];

        T.mv(u, Tu);
        pcFlux[6][2] = Tu[0];
        pcPotential6 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[7][1] = Tu[0];
    }
    else
    {
        this->A_[globalIdx7][globalIdx7] += T[0][0];
        this->A_[globalIdx7][globalIdx8] += T[0][1];
        this->A_[globalIdx7][globalIdx5] += T[0][2];
        this->A_[globalIdx7][globalIdx4] += T[0][3];

        this->A_[globalIdx8][globalIdx7] -= TSecond[0][0];
        this->A_[globalIdx8][globalIdx8] -= TSecond[0][1];
        this->A_[globalIdx8][globalIdx5] -= TSecond[0][2];
        this->A_[globalIdx8][globalIdx4] -= TSecond[0][3];

        u[0] = pc[6];
        u[1] = pc[7];
        u[2] = pc[4];
        u[3] = pc[3];

        T.mv(u, Tu);
        pcFlux[6][2] = Tu[0];
        pcPotential6 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[7][1] = Tu[0];
    }

    // calculate the flux through the subvolumeface 8 (subVolumeFaceIdx = 7)
    caseL = transmissibilityCalculator_.transmissibility(T, interactionVolume, lambda, 4, 6, 5, 7, 0, 2);

    TSecond = T;
    T *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx5, 4, 2);
    TSecond *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx7, 6, 1);

    if (caseL == 1)
    {
        this->A_[globalIdx5][globalIdx5] += T[0][0];
        this->A_[globalIdx5][globalIdx7] += T[0][1];
        this->A_[globalIdx5][globalIdx6] += T[0][2];
        this->A_[globalIdx5][globalIdx1] += T[0][3];

        this->A_[globalIdx7][globalIdx5] -= TSecond[0][0];
        this->A_[globalIdx7][globalIdx7] -= TSecond[0][1];
        this->A_[globalIdx7][globalIdx6] -= TSecond[0][2];
        this->A_[globalIdx7][globalIdx1] -= TSecond[0][3];

        u[0] = pc[4];
        u[1] = pc[6];
        u[2] = pc[5];
        u[3] = pc[0];

        T.mv(u, Tu);
        pcFlux[4][2] = Tu[0];
        pcPotential7 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[6][1] = Tu[0];
    }
    else if (caseL == 2)
    {
        this->A_[globalIdx5][globalIdx5] += T[0][0];
        this->A_[globalIdx5][globalIdx7] += T[0][1];
        this->A_[globalIdx5][globalIdx8] += T[0][2];
        this->A_[globalIdx5][globalIdx3] += T[0][3];

        this->A_[globalIdx7][globalIdx5] -= TSecond[0][0];
        this->A_[globalIdx7][globalIdx7] -= TSecond[0][1];
        this->A_[globalIdx7][globalIdx8] -= TSecond[0][2];
        this->A_[globalIdx7][globalIdx3] -= TSecond[0][3];

        u[0] = pc[4];
        u[1] = pc[6];
        u[2] = pc[7];
        u[3] = pc[2];

        T.mv(u, Tu);
        pcFlux[4][2] = Tu[0];
        pcPotential7 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[6][1] = Tu[0];
    }
    else if (caseL == 3)
    {
        this->A_[globalIdx5][globalIdx5] += T[0][0];
        this->A_[globalIdx5][globalIdx7] += T[0][1];
        this->A_[globalIdx5][globalIdx8] += T[0][2];
        this->A_[globalIdx5][globalIdx1] += T[0][3];

        this->A_[globalIdx7][globalIdx5] -= TSecond[0][0];
        this->A_[globalIdx7][globalIdx7] -= TSecond[0][1];
        this->A_[globalIdx7][globalIdx8] -= TSecond[0][2];
        this->A_[globalIdx7][globalIdx1] -= TSecond[0][3];

        u[0] = pc[4];
        u[1] = pc[6];
        u[2] = pc[7];
        u[3] = pc[0];

        T.mv(u, Tu);
        pcFlux[4][2] = Tu[0];
        pcPotential7 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[6][1] = Tu[0];
    }
    else
    {
        this->A_[globalIdx5][globalIdx5] += T[0][0];
        this->A_[globalIdx5][globalIdx7] += T[0][1];
        this->A_[globalIdx5][globalIdx6] += T[0][2];
        this->A_[globalIdx5][globalIdx3] += T[0][3];

        this->A_[globalIdx7][globalIdx5] -= TSecond[0][0];
        this->A_[globalIdx7][globalIdx7] -= TSecond[0][1];
        this->A_[globalIdx7][globalIdx6] -= TSecond[0][2];
        this->A_[globalIdx7][globalIdx3] -= TSecond[0][3];

        u[0] = pc[4];
        u[1] = pc[6];
        u[2] = pc[5];
        u[3] = pc[2];

        T.mv(u, Tu);
        pcFlux[4][2] = Tu[0];
        pcPotential7 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[6][1] = Tu[0];
    }

    // calculate the flux through the subvolumeface 9 (subVolumeFaceIdx = 8)
    caseL = transmissibilityCalculator_.transmissibility(T, interactionVolume, lambda, 4, 0, 6, 2, 5, 1);

    TSecond = T;
    T *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx5, 4, 0);
    TSecond *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx1, 0, 2);

    if (caseL == 1)
    {
        this->A_[globalIdx5][globalIdx5] += T[0][0];
        this->A_[globalIdx5][globalIdx1] += T[0][1];
        this->A_[globalIdx5][globalIdx7] += T[0][2];
        this->A_[globalIdx5][globalIdx6] += T[0][3];

        this->A_[globalIdx1][globalIdx5] -= TSecond[0][0];
        this->A_[globalIdx1][globalIdx1] -= TSecond[0][1];
        this->A_[globalIdx1][globalIdx7] -= TSecond[0][2];
        this->A_[globalIdx1][globalIdx6] -= TSecond[0][3];

        u[0] = pc[4];
        u[1] = pc[0];
        u[2] = pc[6];
        u[3] = pc[5];

        T.mv(u, Tu);
        pcFlux[4][0] = Tu[0];
        pcPotential8 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[0][2] = Tu[0];
    }
    else if (caseL == 2)
    {
        this->A_[globalIdx5][globalIdx5] += T[0][0];
        this->A_[globalIdx5][globalIdx1] += T[0][1];
        this->A_[globalIdx5][globalIdx3] += T[0][2];
        this->A_[globalIdx5][globalIdx2] += T[0][3];

        this->A_[globalIdx1][globalIdx5] -= TSecond[0][0];
        this->A_[globalIdx1][globalIdx1] -= TSecond[0][1];
        this->A_[globalIdx1][globalIdx3] -= TSecond[0][2];
        this->A_[globalIdx1][globalIdx2] -= TSecond[0][3];

        u[0] = pc[4];
        u[1] = pc[0];
        u[2] = pc[2];
        u[3] = pc[1];

        T.mv(u, Tu);
        pcFlux[4][0] = Tu[0];
        pcPotential8 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[0][2] = Tu[0];
    }
    else if (caseL == 3)
    {
        this->A_[globalIdx5][globalIdx5] += T[0][0];
        this->A_[globalIdx5][globalIdx1] += T[0][1];
        this->A_[globalIdx5][globalIdx3] += T[0][2];
        this->A_[globalIdx5][globalIdx6] += T[0][3];

        this->A_[globalIdx1][globalIdx5] -= TSecond[0][0];
        this->A_[globalIdx1][globalIdx1] -= TSecond[0][1];
        this->A_[globalIdx1][globalIdx3] -= TSecond[0][2];
        this->A_[globalIdx1][globalIdx6] -= TSecond[0][3];

        u[0] = pc[4];
        u[1] = pc[0];
        u[2] = pc[2];
        u[3] = pc[5];

        T.mv(u, Tu);
        pcFlux[4][0] = Tu[0];
        pcPotential8 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[0][2] = Tu[0];
    }
    else
    {
        this->A_[globalIdx5][globalIdx5] += T[0][0];
        this->A_[globalIdx5][globalIdx1] += T[0][1];
        this->A_[globalIdx5][globalIdx7] += T[0][2];
        this->A_[globalIdx5][globalIdx2] += T[0][3];

        this->A_[globalIdx1][globalIdx5] -= TSecond[0][0];
        this->A_[globalIdx1][globalIdx1] -= TSecond[0][1];
        this->A_[globalIdx1][globalIdx7] -= TSecond[0][2];
        this->A_[globalIdx1][globalIdx2] -= TSecond[0][3];

        u[0] = pc[4];
        u[1] = pc[0];
        u[2] = pc[6];
        u[3] = pc[1];

        T.mv(u, Tu);
        pcFlux[4][0] = Tu[0];
        pcPotential8 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[0][2] = Tu[0];
    }

    // calculate the flux through the subvolumeface 10 (subVolumeFaceIdx = 9)
    caseL = transmissibilityCalculator_.transmissibility(T, interactionVolume, lambda, 1, 5, 3, 7, 0, 4);

    TSecond = T;
    T *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx2, 1, 2);
    TSecond *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx6, 5, 0);

    if (caseL == 1)
    {
        this->A_[globalIdx2][globalIdx2] += T[0][0];
        this->A_[globalIdx2][globalIdx6] += T[0][1];
        this->A_[globalIdx2][globalIdx4] += T[0][2];
        this->A_[globalIdx2][globalIdx1] += T[0][3];

        this->A_[globalIdx6][globalIdx2] -= TSecond[0][0];
        this->A_[globalIdx6][globalIdx6] -= TSecond[0][1];
        this->A_[globalIdx6][globalIdx4] -= TSecond[0][2];
        this->A_[globalIdx6][globalIdx1] -= TSecond[0][3];

        u[0] = pc[1];
        u[1] = pc[5];
        u[2] = pc[3];
        u[3] = pc[0];

        T.mv(u, Tu);
        pcFlux[1][2] = Tu[0];
        pcPotential9 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[5][0] = Tu[0];
    }
    else if (caseL == 2)
    {
        this->A_[globalIdx2][globalIdx2] += T[0][0];
        this->A_[globalIdx2][globalIdx6] += T[0][1];
        this->A_[globalIdx2][globalIdx8] += T[0][2];
        this->A_[globalIdx2][globalIdx5] += T[0][3];

        this->A_[globalIdx6][globalIdx2] -= TSecond[0][0];
        this->A_[globalIdx6][globalIdx6] -= TSecond[0][1];
        this->A_[globalIdx6][globalIdx8] -= TSecond[0][2];
        this->A_[globalIdx6][globalIdx5] -= TSecond[0][3];

        u[0] = pc[1];
        u[1] = pc[5];
        u[2] = pc[7];
        u[3] = pc[4];

        T.mv(u, Tu);
        pcFlux[1][2] = Tu[0];
        pcPotential9 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[5][0] = Tu[0];
    }
    else if (caseL == 3)
    {
        this->A_[globalIdx2][globalIdx2] += T[0][0];
        this->A_[globalIdx2][globalIdx6] += T[0][1];
        this->A_[globalIdx2][globalIdx8] += T[0][2];
        this->A_[globalIdx2][globalIdx1] += T[0][3];

        this->A_[globalIdx6][globalIdx2] -= TSecond[0][0];
        this->A_[globalIdx6][globalIdx6] -= TSecond[0][1];
        this->A_[globalIdx6][globalIdx8] -= TSecond[0][2];
        this->A_[globalIdx6][globalIdx1] -= TSecond[0][3];

        u[0] = pc[1];
        u[1] = pc[5];
        u[2] = pc[7];
        u[3] = pc[0];

        T.mv(u, Tu);
        pcFlux[1][2] = Tu[0];
        pcPotential9 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[5][0] = Tu[0];
    }
    else
    {
        this->A_[globalIdx2][globalIdx2] += T[0][0];
        this->A_[globalIdx2][globalIdx6] += T[0][1];
        this->A_[globalIdx2][globalIdx4] += T[0][2];
        this->A_[globalIdx2][globalIdx5] += T[0][3];

        this->A_[globalIdx6][globalIdx2] -= TSecond[0][0];
        this->A_[globalIdx6][globalIdx6] -= TSecond[0][1];
        this->A_[globalIdx6][globalIdx4] -= TSecond[0][2];
        this->A_[globalIdx6][globalIdx5] -= TSecond[0][3];

        u[0] = pc[1];
        u[1] = pc[5];
        u[2] = pc[3];
        u[3] = pc[4];

        T.mv(u, Tu);
        pcFlux[1][2] = Tu[0];
        pcPotential9 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[5][0] = Tu[0];
    }

    // calculate the flux through the subvolumeface 11 (subVolumeFaceIdx = 10)
    caseL = transmissibilityCalculator_.transmissibility(T, interactionVolume, lambda, 7, 3, 5, 1, 6, 2);

    TSecond = T;
    T *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx8, 7, 0);
    TSecond *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx4, 3, 2);

    if (caseL == 1)
    {
        this->A_[globalIdx8][globalIdx8] += T[0][0];
        this->A_[globalIdx8][globalIdx4] += T[0][1];
        this->A_[globalIdx8][globalIdx6] += T[0][2];
        this->A_[globalIdx8][globalIdx7] += T[0][3];

        this->A_[globalIdx4][globalIdx8] -= TSecond[0][0];
        this->A_[globalIdx4][globalIdx4] -= TSecond[0][1];
        this->A_[globalIdx4][globalIdx6] -= TSecond[0][2];
        this->A_[globalIdx4][globalIdx7] -= TSecond[0][3];

        u[0] = pc[7];
        u[1] = pc[3];
        u[2] = pc[5];
        u[3] = pc[6];

        T.mv(u, Tu);
        pcFlux[7][0] = Tu[0];
        pcPotential10 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[3][2] = Tu[0];
    }
    else if (caseL == 2)
    {
        this->A_[globalIdx8][globalIdx8] += T[0][0];
        this->A_[globalIdx8][globalIdx4] += T[0][1];
        this->A_[globalIdx8][globalIdx2] += T[0][2];
        this->A_[globalIdx8][globalIdx3] += T[0][3];

        this->A_[globalIdx4][globalIdx8] -= TSecond[0][0];
        this->A_[globalIdx4][globalIdx4] -= TSecond[0][1];
        this->A_[globalIdx4][globalIdx2] -= TSecond[0][2];
        this->A_[globalIdx4][globalIdx3] -= TSecond[0][3];

        u[0] = pc[7];
        u[1] = pc[3];
        u[2] = pc[1];
        u[3] = pc[2];

        T.mv(u, Tu);
        pcFlux[7][0] = Tu[0];
        pcPotential10 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[3][2] = Tu[0];
    }
    else if (caseL == 3)
    {
        this->A_[globalIdx8][globalIdx8] += T[0][0];
        this->A_[globalIdx8][globalIdx4] += T[0][1];
        this->A_[globalIdx8][globalIdx2] += T[0][2];
        this->A_[globalIdx8][globalIdx7] += T[0][3];

        this->A_[globalIdx4][globalIdx8] -= TSecond[0][0];
        this->A_[globalIdx4][globalIdx4] -= TSecond[0][1];
        this->A_[globalIdx4][globalIdx2] -= TSecond[0][2];
        this->A_[globalIdx4][globalIdx7] -= TSecond[0][3];

        u[0] = pc[7];
        u[1] = pc[3];
        u[2] = pc[1];
        u[3] = pc[6];

        T.mv(u, Tu);
        pcFlux[7][0] = Tu[0];
        pcPotential10 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[3][2] = Tu[0];
    }
    else
    {
        this->A_[globalIdx8][globalIdx8] += T[0][0];
        this->A_[globalIdx8][globalIdx4] += T[0][1];
        this->A_[globalIdx8][globalIdx6] += T[0][2];
        this->A_[globalIdx8][globalIdx3] += T[0][3];

        this->A_[globalIdx4][globalIdx8] -= TSecond[0][0];
        this->A_[globalIdx4][globalIdx4] -= TSecond[0][1];
        this->A_[globalIdx4][globalIdx6] -= TSecond[0][2];
        this->A_[globalIdx4][globalIdx3] -= TSecond[0][3];

        u[0] = pc[7];
        u[1] = pc[3];
        u[2] = pc[5];
        u[3] = pc[2];

        T.mv(u, Tu);
        pcFlux[7][0] = Tu[0];
        pcPotential10 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[3][2] = Tu[0];
    }

    // calculate the flux through the subvolumeface 12 (subVolumeFaceIdx = 11)
    caseL = transmissibilityCalculator_.transmissibility(T, interactionVolume, lambda, 2, 6, 0, 4, 3, 7);

    TSecond = T;
    T *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx3, 2, 2);
    TSecond *= interactionVolumes_.faceAreaFactor(interactionVolume, globalIdx7, 6, 0);

    if (caseL == 1)
    {
        this->A_[globalIdx3][globalIdx3] += T[0][0];
        this->A_[globalIdx3][globalIdx7] += T[0][1];
        this->A_[globalIdx3][globalIdx1] += T[0][2];
        this->A_[globalIdx3][globalIdx4] += T[0][3];

        this->A_[globalIdx7][globalIdx3] -= TSecond[0][0];
        this->A_[globalIdx7][globalIdx7] -= TSecond[0][1];
        this->A_[globalIdx7][globalIdx1] -= TSecond[0][2];
        this->A_[globalIdx7][globalIdx4] -= TSecond[0][3];

        u[0] = pc[2];
        u[1] = pc[6];
        u[2] = pc[0];
        u[3] = pc[3];

        T.mv(u, Tu);
        pcFlux[2][2] = Tu[0];
        pcPotential11 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[6][0] = Tu[0];
    }
    else if (caseL == 2)
    {
        this->A_[globalIdx3][globalIdx3] += T[0][0];
        this->A_[globalIdx3][globalIdx7] += T[0][1];
        this->A_[globalIdx3][globalIdx5] += T[0][2];
        this->A_[globalIdx3][globalIdx8] += T[0][3];

        this->A_[globalIdx7][globalIdx3] -= TSecond[0][0];
        this->A_[globalIdx7][globalIdx7] -= TSecond[0][1];
        this->A_[globalIdx7][globalIdx5] -= TSecond[0][2];
        this->A_[globalIdx7][globalIdx8] -= TSecond[0][3];

        u[0] = pc[2];
        u[1] = pc[6];
        u[2] = pc[4];
        u[3] = pc[7];

        T.mv(u, Tu);
        pcFlux[2][2] = Tu[0];
        pcPotential11 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[6][0] = Tu[0];
    }
    else if (caseL == 3)
    {
        this->A_[globalIdx3][globalIdx3] += T[0][0];
        this->A_[globalIdx3][globalIdx7] += T[0][1];
        this->A_[globalIdx3][globalIdx5] += T[0][2];
        this->A_[globalIdx3][globalIdx4] += T[0][3];

        this->A_[globalIdx7][globalIdx3] -= TSecond[0][0];
        this->A_[globalIdx7][globalIdx7] -= TSecond[0][1];
        this->A_[globalIdx7][globalIdx5] -= TSecond[0][2];
        this->A_[globalIdx7][globalIdx4] -= TSecond[0][3];

        u[0] = pc[2];
        u[1] = pc[6];
        u[2] = pc[4];
        u[3] = pc[3];

        T.mv(u, Tu);
        pcFlux[2][2] = Tu[0];
        pcPotential11 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[6][0] = Tu[0];
    }
    else
    {
        this->A_[globalIdx3][globalIdx3] += T[0][0];
        this->A_[globalIdx3][globalIdx7] += T[0][1];
        this->A_[globalIdx3][globalIdx1] += T[0][2];
        this->A_[globalIdx3][globalIdx8] += T[0][3];

        this->A_[globalIdx7][globalIdx3] -= TSecond[0][0];
        this->A_[globalIdx7][globalIdx7] -= TSecond[0][1];
        this->A_[globalIdx7][globalIdx1] -= TSecond[0][2];
        this->A_[globalIdx7][globalIdx8] -= TSecond[0][3];

        u[0] = pc[2];
        u[1] = pc[6];
        u[2] = pc[0];
        u[3] = pc[7];

        T.mv(u, Tu);
        pcFlux[2][2] = Tu[0];
        pcPotential11 = Tu[0];

        TSecond.mv(u, Tu);
        pcFlux[6][0] = Tu[0];
    }

    if (pc[0] == 0 && pc[1] == 0 && pc[2] == 0 && pc[3] == 0 && pc[4] == 0 && pc[5] == 0 && pc[6] == 0
        && pc[7] == 0)
    {
        return;
    }

    // compute mobilities of subvolumeface 1 (subVolumeFaceIdx = 0)
    Dune::FieldVector<Scalar, numPhases> lambda0Upw(0.0);
    lambda0Upw[wPhaseIdx] = (pcPotential0 >= 0) ? lambda1[wPhaseIdx] : lambda2[wPhaseIdx];
    lambda0Upw[nPhaseIdx] = (pcPotential0 >= 0) ? lambda1[nPhaseIdx] : lambda2[nPhaseIdx];

    // compute mobilities of subvolumeface 2 (subVolumeFaceIdx = 1)
    Dune::FieldVector<Scalar, numPhases> lambda1Upw(0.0);
    lambda1Upw[wPhaseIdx] = (pcPotential1 >= 0) ? lambda2[wPhaseIdx] : lambda4[wPhaseIdx];
    lambda1Upw[nPhaseIdx] = (pcPotential1 >= 0) ? lambda2[nPhaseIdx] : lambda4[nPhaseIdx];

    // compute mobilities of subvolumeface 3 (subVolumeFaceIdx = 2)
    Dune::FieldVector<Scalar, numPhases> lambda2Upw(0.0);
    lambda2Upw[wPhaseIdx] = (pcPotential2 >= 0) ? lambda4[wPhaseIdx] : lambda3[wPhaseIdx];
    lambda2Upw[nPhaseIdx] = (pcPotential2 >= 0) ? lambda4[nPhaseIdx] : lambda3[nPhaseIdx];

    // compute mobilities of subvolumeface 4 (subVolumeFaceIdx = 3)
    Dune::FieldVector<Scalar, numPhases> lambda3Upw(0.0);
    lambda3Upw[wPhaseIdx] = (pcPotential3 >= 0) ? lambda3[wPhaseIdx] : lambda1[wPhaseIdx];
    lambda3Upw[nPhaseIdx] = (pcPotential3 >= 0) ? lambda3[nPhaseIdx] : lambda1[nPhaseIdx];

    // compute mobilities of subvolumeface 5 (subVolumeFaceIdx = 4)
    Dune::FieldVector<Scalar, numPhases> lambda4Upw(0.0);
    lambda4Upw[wPhaseIdx] = (pcPotential4 >= 0) ? lambda6[wPhaseIdx] : lambda5[wPhaseIdx];
    lambda4Upw[nPhaseIdx] = (pcPotential4 >= 0) ? lambda6[nPhaseIdx] : lambda5[nPhaseIdx];

    // compute mobilities of subvolumeface 6 (subVolumeFaceIdx = 5)
    Dune::FieldVector<Scalar, numPhases> lambda5Upw(0.0);
    lambda5Upw[wPhaseIdx] = (pcPotential5 >= 0) ? lambda8[wPhaseIdx] : lambda6[wPhaseIdx];
    lambda5Upw[nPhaseIdx] = (pcPotential5 >= 0) ? lambda8[nPhaseIdx] : lambda6[nPhaseIdx];

    // compute mobilities of subvolumeface 7 (subVolumeFaceIdx = 6)
    Dune::FieldVector<Scalar, numPhases> lambda6Upw(0.0);
    lambda6Upw[wPhaseIdx] = (pcPotential6 >= 0) ? lambda7[wPhaseIdx] : lambda8[wPhaseIdx];
    lambda6Upw[nPhaseIdx] = (pcPotential6 >= 0) ? lambda7[nPhaseIdx] : lambda8[nPhaseIdx];

    // compute mobilities of subvolumeface 8 (subVolumeFaceIdx = 7)
    Dune::FieldVector<Scalar, numPhases> lambda7Upw(0.0);
    lambda7Upw[wPhaseIdx] = (pcPotential7 >= 0) ? lambda5[wPhaseIdx] : lambda7[wPhaseIdx];
    lambda7Upw[nPhaseIdx] = (pcPotential7 >= 0) ? lambda5[nPhaseIdx] : lambda7[nPhaseIdx];

    // compute mobilities of subvolumeface 9 (subVolumeFaceIdx = 8)
    Dune::FieldVector<Scalar, numPhases> lambda8Upw(0.0);
    lambda8Upw[wPhaseIdx] = (pcPotential8 >= 0) ? lambda5[wPhaseIdx] : lambda1[wPhaseIdx];
    lambda8Upw[nPhaseIdx] = (pcPotential8 >= 0) ? lambda5[nPhaseIdx] : lambda1[nPhaseIdx];

    // compute mobilities of subvolumeface 10 (subVolumeFaceIdx = 9)
    Dune::FieldVector<Scalar, numPhases> lambda9Upw(0.0);
    lambda9Upw[wPhaseIdx] = (pcPotential9 >= 0) ? lambda2[wPhaseIdx] : lambda6[wPhaseIdx];
    lambda9Upw[nPhaseIdx] = (pcPotential9 >= 0) ? lambda2[nPhaseIdx] : lambda6[nPhaseIdx];

    // compute mobilities of subvolumeface 11 (subVolumeFaceIdx = 10)
    Dune::FieldVector<Scalar, numPhases> lambda10Upw(0.0);
    lambda10Upw[wPhaseIdx] = (pcPotential10 >= 0) ? lambda8[wPhaseIdx] : lambda4[wPhaseIdx];
    lambda10Upw[nPhaseIdx] = (pcPotential10 >= 0) ? lambda8[nPhaseIdx] : lambda4[nPhaseIdx];

    // compute mobilities of subvolumeface 12 (subVolumeFaceIdx = 11)
    Dune::FieldVector<Scalar, numPhases> lambda11Upw(0.0);
    lambda11Upw[wPhaseIdx] = (pcPotential11 >= 0) ? lambda3[wPhaseIdx] : lambda7[wPhaseIdx];
    lambda11Upw[nPhaseIdx] = (pcPotential11 >= 0) ? lambda3[nPhaseIdx] : lambda7[nPhaseIdx];

    for (int i = 0; i < numPhases; i++)
    {
        Scalar lambdaT0 = lambda0Upw[wPhaseIdx] + lambda0Upw[nPhaseIdx];
        Scalar lambdaT1 = lambda1Upw[wPhaseIdx] + lambda1Upw[nPhaseIdx];
        Scalar lambdaT2 = lambda2Upw[wPhaseIdx] + lambda2Upw[nPhaseIdx];
        Scalar lambdaT3 = lambda3Upw[wPhaseIdx] + lambda3Upw[nPhaseIdx];
        Scalar lambdaT4 = lambda4Upw[wPhaseIdx] + lambda4Upw[nPhaseIdx];
        Scalar lambdaT5 = lambda5Upw[wPhaseIdx] + lambda5Upw[nPhaseIdx];
        Scalar lambdaT6 = lambda6Upw[wPhaseIdx] + lambda6Upw[nPhaseIdx];
        Scalar lambdaT7 = lambda7Upw[wPhaseIdx] + lambda7Upw[nPhaseIdx];
        Scalar lambdaT8 = lambda8Upw[wPhaseIdx] + lambda8Upw[nPhaseIdx];
        Scalar lambdaT9 = lambda9Upw[wPhaseIdx] + lambda9Upw[nPhaseIdx];
        Scalar lambdaT10 = lambda10Upw[wPhaseIdx] + lambda10Upw[nPhaseIdx];
        Scalar lambdaT11 = lambda11Upw[wPhaseIdx] + lambda11Upw[nPhaseIdx];
        Scalar fracFlow0 = (lambdaT0 > threshold_) ? lambda0Upw[i] / (lambdaT0) : 0.0;
        Scalar fracFlow1 = (lambdaT1 > threshold_) ? lambda1Upw[i] / (lambdaT1) : 0.0;
        Scalar fracFlow2 = (lambdaT2 > threshold_) ? lambda2Upw[i] / (lambdaT2) : 0.0;
        Scalar fracFlow3 = (lambdaT3 > threshold_) ? lambda3Upw[i] / (lambdaT3) : 0.0;
        Scalar fracFlow4 = (lambdaT4 > threshold_) ? lambda4Upw[i] / (lambdaT4) : 0.0;
        Scalar fracFlow5 = (lambdaT5 > threshold_) ? lambda5Upw[i] / (lambdaT5) : 0.0;
        Scalar fracFlow6 = (lambdaT6 > threshold_) ? lambda6Upw[i] / (lambdaT6) : 0.0;
        Scalar fracFlow7 = (lambdaT7 > threshold_) ? lambda7Upw[i] / (lambdaT7) : 0.0;
        Scalar fracFlow8 = (lambdaT8 > threshold_) ? lambda8Upw[i] / (lambdaT8) : 0.0;
        Scalar fracFlow9 = (lambdaT9 > threshold_) ? lambda9Upw[i] / (lambdaT9) : 0.0;
        Scalar fracFlow10 = (lambdaT10 > threshold_) ? lambda10Upw[i] / (lambdaT10) : 0.0;
        Scalar fracFlow11 = (lambdaT11 > threshold_) ? lambda11Upw[i] / (lambdaT11) : 0.0;

        switch (pressureType_)
        {
        case pw:
            {
                if (i == nPhaseIdx)
                {
                    // add capillary pressure term to right hand side
                    this->f_[globalIdx1] -= (fracFlow0 * pcFlux[0][0] - fracFlow3 * pcFlux[0][1] - fracFlow8 * pcFlux[0][2]);
                    this->f_[globalIdx2] -= (fracFlow1 * pcFlux[1][0] - fracFlow0 * pcFlux[1][1] + fracFlow9 * pcFlux[1][2]);
                    this->f_[globalIdx3] -= (fracFlow3 * pcFlux[2][0] - fracFlow2 * pcFlux[2][1] + fracFlow11 * pcFlux[2][2]);
                    this->f_[globalIdx4] -= (fracFlow2 * pcFlux[3][0] - fracFlow1 * pcFlux[3][1] - fracFlow10 * pcFlux[3][2]);
                    this->f_[globalIdx5] -= (fracFlow8 * pcFlux[4][0] - fracFlow4 * pcFlux[4][1] + fracFlow7 * pcFlux[4][2]);
                    this->f_[globalIdx6] -= (-fracFlow9 * pcFlux[5][0] - fracFlow5 * pcFlux[5][1] + fracFlow4 * pcFlux[5][2]);
                    this->f_[globalIdx7] -= (-fracFlow11 * pcFlux[6][0] - fracFlow7 * pcFlux[6][1] + fracFlow6 * pcFlux[6][2]);
                    this->f_[globalIdx8] -= (fracFlow10 * pcFlux[7][0] - fracFlow6 * pcFlux[7][1] + fracFlow5 * pcFlux[7][2]);
                }
                break;
            }
        case pn:
            {
                if (i == wPhaseIdx)
                {
                    // add capillary pressure term to right hand side
                    this->f_[globalIdx1] += (fracFlow0 * pcFlux[0][0] - fracFlow3 * pcFlux[0][1] - fracFlow8 * pcFlux[0][2]);
                    this->f_[globalIdx2] += (fracFlow1 * pcFlux[1][0] - fracFlow0 * pcFlux[1][1] + fracFlow9 * pcFlux[1][2]);
                    this->f_[globalIdx3] += (fracFlow3 * pcFlux[2][0] - fracFlow2 * pcFlux[2][1] + fracFlow11 * pcFlux[2][2]);
                    this->f_[globalIdx4] += (fracFlow2 * pcFlux[3][0] - fracFlow1 * pcFlux[3][1] - fracFlow10 * pcFlux[3][2]);
                    this->f_[globalIdx5] += (fracFlow8 * pcFlux[4][0] - fracFlow4 * pcFlux[4][1] + fracFlow7 * pcFlux[4][2]);
                    this->f_[globalIdx6] += (-fracFlow9 * pcFlux[5][0] - fracFlow5 * pcFlux[5][1] + fracFlow4 * pcFlux[5][2]);
                    this->f_[globalIdx7] += (-fracFlow11 * pcFlux[6][0] - fracFlow7 * pcFlux[6][1] + fracFlow6 * pcFlux[6][2]);
                    this->f_[globalIdx8] += (fracFlow10 * pcFlux[7][0] - fracFlow6 * pcFlux[7][1] + fracFlow5 * pcFlux[7][2]);
                }
                break;
            }
        }
    }
}

template<class TypeTag>
void FvMpfaL3dPressure2p<TypeTag>::assembleBoundaryInteractionVolume(InteractionVolume& interactionVolume)
{
    for (int elemIdx = 0; elemIdx < 8; elemIdx++)
    {
        if (!interactionVolume.hasSubVolumeElement(elemIdx))
        {
            continue;
        }
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

        // permeability vector at boundary
        DimMatrix permeability(problem_.spatialParams().intrinsicPermeability(*elementPointer));

        // evaluate right hand side
        PrimaryVariables source(0);
        problem_.source(source, *elementPointer);
        this->f_[globalIdx] += volume / (8.0)
            * (source[wPhaseIdx] / density_[wPhaseIdx] + source[nPhaseIdx] / density_[nPhaseIdx]);

        this->f_[globalIdx] += evaluateErrorTerm(cellData) * volume / (8.0);

        //get mobilities of the phases
        Dune::FieldVector<Scalar, numPhases> lambda(cellData.mobility(wPhaseIdx));
        lambda[nPhaseIdx] = cellData.mobility(nPhaseIdx);

        Scalar pc = cellData.capillaryPressure();

        Scalar gravityDiff = (problem_.bBoxMax() - globalPos) * gravity_
            * (density_[nPhaseIdx] - density_[wPhaseIdx]);

        pc += gravityDiff; //minus because of gravity definition!

        for (int faceIdx = 0; faceIdx < dim; faceIdx++)
        {
            int intVolFaceIdx = interactionVolume.getFaceIndexFromSubVolume(elemIdx, faceIdx);

            if (interactionVolume.isBoundaryFace(intVolFaceIdx))
            {
                if (interactionVolume.getBoundaryType(intVolFaceIdx).isDirichlet(pressureEqIdx))
                {
                    const GlobalPosition& globalPosFace = interactionVolume.getFacePosition(elemIdx, faceIdx);

                    DimVector distVec(globalPosFace - globalPos);
                    Scalar dist = distVec.two_norm();
                    DimVector& normal = interactionVolume.getNormal(elemIdx, faceIdx);

                    Scalar faceArea = interactionVolume.getFaceArea(elemIdx, faceIdx);

                    // get pc and lambda at the boundary
                    Scalar satWBound = cellData.saturation(wPhaseIdx);
                    //check boundary sat at face 1
                    if (interactionVolume.getBoundaryType(intVolFaceIdx).isDirichlet(satEqIdx))
                    {
                        Scalar satBound = interactionVolume.getDirichletValues(intVolFaceIdx)[saturationIdx];
                        switch (saturationType_)
                        {
                        case sw:
                            {
                                satWBound = satBound;
                                break;
                            }
                        case sn:
                            {
                                satWBound = 1 - satBound;
                                break;
                            }
                        }

                    }

                    Scalar pcBound = MaterialLaw::pc(
                                                     problem_.spatialParams().materialLawParams(*elementPointer), satWBound);

                    Scalar gravityDiffBound = (problem_.bBoxMax() - globalPosFace) * gravity_
                        * (density_[nPhaseIdx] - density_[wPhaseIdx]);

                    pcBound += gravityDiffBound;

                    Dune::FieldVector<Scalar, numPhases>
                      lambdaBound(MaterialLaw::krw(problem_.spatialParams().materialLawParams(*elementPointer),satWBound));
                    lambdaBound[nPhaseIdx] = MaterialLaw::krn(
                                                              problem_.spatialParams().materialLawParams(*elementPointer), satWBound);
                    lambdaBound[wPhaseIdx] /= viscosity_[wPhaseIdx];
                    lambdaBound[nPhaseIdx] /= viscosity_[nPhaseIdx];

                    Scalar potentialBound = interactionVolume.getDirichletValues(intVolFaceIdx)[pressureIdx];
                    Scalar gdeltaZ = (problem_.bBoxMax()-globalPosFace) * gravity_;

                    //calculate potential gradients
                    Scalar potentialDiffW = 0;
                    Scalar potentialDiffNw = 0;
                    switch (pressureType_)
                    {
                    case pw:
                        {
                            potentialBound += density_[wPhaseIdx]*gdeltaZ;
                            potentialDiffW = (cellData.potential(wPhaseIdx) - potentialBound) / dist;
                            potentialDiffNw = (cellData.potential(nPhaseIdx) - potentialBound - pcBound) / dist;
                            break;
                        }
                    case pn:
                        {
                            potentialBound += density_[nPhaseIdx]*gdeltaZ;
                            potentialDiffW = (cellData.potential(wPhaseIdx) - potentialBound + pcBound) / dist;
                            potentialDiffNw = (cellData.potential(nPhaseIdx) - potentialBound) / dist;
                            break;
                        }
                    }

                    Scalar lambdaTotal = (potentialDiffW >= 0.) ? lambda[wPhaseIdx] : lambdaBound[wPhaseIdx];
                    lambdaTotal += (potentialDiffNw >= 0.) ? lambda[nPhaseIdx] : lambdaBound[nPhaseIdx];

                    DimVector permTimesNormal(0);
                    permeability.mv(normal, permTimesNormal);
                    Scalar scalarPerm = permTimesNormal.two_norm();

                    //calculate current matrix entry
                    Scalar entry = lambdaTotal * scalarPerm / dist * faceArea;

                    // set diagonal entry and right hand side entry
                    this->A_[globalIdx][globalIdx] += entry;
                    this->f_[globalIdx] += entry * potentialBound;

                    if (pc == 0 && pcBound == 0)
                    {
                        continue;
                    }

                    //calculate right hand side
                    Scalar pcFlux = 0;

                    switch (pressureType_)
                    {
                    case pw:
                        {
                            //add capillary pressure term to right hand side
                            pcFlux = 0.5 * (lambda[nPhaseIdx] + lambdaBound[nPhaseIdx])
                                * scalarPerm * (pc - pcBound) / dist * faceArea;

                            break;
                        }
                    case pn:
                        {
                            //add capillary pressure term to right hand side
                            pcFlux = 0.5 * (lambda[wPhaseIdx] + lambdaBound[wPhaseIdx])
                                * scalarPerm * (pc - pcBound) / dist * faceArea;

                            break;
                        }
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
                else if (interactionVolume.getBoundaryType(intVolFaceIdx).isNeumann(pressureEqIdx))
                {
                    Scalar J = interactionVolume.getNeumannValues(intVolFaceIdx)[wPhaseIdx]
                        / density_[wPhaseIdx];
                    J += interactionVolume.getNeumannValues(intVolFaceIdx)[nPhaseIdx] / density_[nPhaseIdx];
                    this->f_[globalIdx] -= J;
                }
                else
                {
                    std::cout << "interactionVolume.getBoundaryType(intVolFaceIdx).isNeumann(pressureEqIdx)"
                              << interactionVolume.getBoundaryType(intVolFaceIdx).isNeumann(pressureEqIdx) << "\n";
                    DUNE_THROW(Dune::NotImplemented,
                               "No valid boundary condition type defined for pressure equation!");
                }
            }
        }
    }
}

//constitutive functions are updated once if new saturations are calculated and stored in the variables object
template<class TypeTag>
void FvMpfaL3dPressure2p<TypeTag>::updateMaterialLaws()
{
    // iterate through leaf grid an evaluate c0 at cell center
    ElementIterator eEndIt = problem_.gridView().template end<0>();
    for (ElementIterator eIt = problem_.gridView().template begin<0>(); eIt != eEndIt; ++eIt)
    {
        int globalIdx = problem_.variables().index(*eIt);

        CellData& cellData = problem_.variables().cellData(globalIdx);

        Scalar satW = cellData.saturation(wPhaseIdx);

        Scalar pc = MaterialLaw::pc(problem_.spatialParams().materialLawParams(*eIt), satW);

        cellData.setCapillaryPressure(pc);

        // initialize mobilities
        Scalar mobilityW = MaterialLaw::krw(problem_.spatialParams().materialLawParams(*eIt), satW)
            / viscosity_[wPhaseIdx];
        Scalar mobilityNw = MaterialLaw::krn(problem_.spatialParams().materialLawParams(*eIt), satW)
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
// end of Dune namespace
#endif
