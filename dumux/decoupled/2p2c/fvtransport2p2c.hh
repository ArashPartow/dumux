// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2010 by Markus Wolff, Benjamin Faigle                     *
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
#ifndef DUMUX_FVTRANSPORT2P2C_HH
#define DUMUX_FVTRANSPORT2P2C_HH

#include <dune/grid/common/gridenums.hh>
#include <dumux/decoupled/2p2c/2p2cproperties.hh>
#include <dumux/common/math.hh>

/**
 * @file
 * @brief  Finite Volume discretization of the component transport equation
 * @author Markus Wolff, Jochen Fritz, Benjamin Faigle
 */

namespace Dumux
{
//! Miscible Transport step in a Finite Volume discretization
/*! \ingroup multiphase
 *  The finite volume model for the solution of the transport equation for compositional
 *  two-phase flow.
 *  \f[
      \frac{\partial C^\kappa}{\partial t} = - \nabla \cdot \left( \sum_{\alpha} X^{\kappa}_{\alpha} \varrho_{alpha} \bf{v}_{\alpha}\right) + q^{\kappa},
 *  \f]
 *  where \f$ \bf{v}_{\alpha} = - \lambda_{\alpha} \bf{K} \left(\nabla p_{\alpha} + \rho_{\alpha} \bf{g} \right) \f$.
 *  \f$ p_{\alpha} \f$ denotes the phase pressure, \f$ \bf{K} \f$ the absolute permeability, \f$ \lambda_{\alpha} \f$ the phase mobility,
 *  \f$ \rho_{\alpha} \f$ the phase density and \f$ \bf{g} \f$ the gravity constant and \f$ C^{\kappa} \f$ the total Component concentration.
 *  The whole flux contribution for each cell is subdivided into a storage term, a flux term and a source term.
 *  Corresponding functions (<tt>getFlux()</tt> and <tt>getFluxOnBoundary()</tt>) are provided,
 *  internal sources are directly treated.
 *
 *
 *  \tparam TypeTag The Type Tag
 */
template<class TypeTag>
class FVTransport2P2C
{
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;

    typedef typename GET_PROP_TYPE(TypeTag, SpatialParameters) SpatialParameters;
    typedef typename SpatialParameters::MaterialLaw MaterialLaw;

    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;

    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, FluidState) FluidState;

    typedef typename GET_PROP_TYPE(TypeTag, CellData) CellData;

    typedef typename GET_PROP_TYPE(TypeTag, TransportSolutionType) TransportSolutionType;

    enum
    {
        dim = GridView::dimension, dimWorld = GridView::dimensionworld
    };
    enum
    {
        pw = Indices::pressureW,
        pn = Indices::pressureNW,
        Sw = Indices::saturationW,
        Sn = Indices::saturationNW
    };
    enum
    {
        wPhaseIdx = Indices::wPhaseIdx, nPhaseIdx = Indices::nPhaseIdx,
        wCompIdx = Indices::wPhaseIdx, nCompIdx = Indices::nPhaseIdx,
        contiWEqIdx=Indices::contiWEqIdx, contiNEqIdx=Indices::contiNEqIdx
    };

    typedef typename GridView::Traits::template Codim<0>::Entity Element;
    typedef typename GridView::Grid Grid;
    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    typedef typename GridView::template Codim<0>::EntityPointer ElementPointer;
    typedef typename GridView::IntersectionIterator IntersectionIterator;
    typedef typename GridView::Intersection Intersection;

    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;
    typedef Dune::FieldMatrix<Scalar,dim,dim> DimMatrix;
    typedef Dune::FieldVector<Scalar, 2> PhaseVector;
    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;

    //! Acess function for the current problem
    Problem& problem()
    {return problem_;};

public:
    virtual void update(const Scalar t, Scalar& dt, TransportSolutionType& updateVec, bool impes);

    void updateTransportedQuantity(TransportSolutionType& updateVector);

    // Function which calculates the flux update
    void getFlux(Dune::FieldVector<Scalar, 2>&, Dune::FieldVector<Scalar, 2>&,
            const Intersection&, CellData&);

    // Function which calculates the boundary flux update
    void getFluxOnBoundary(Dune::FieldVector<Scalar, 2>&, Dune::FieldVector<Scalar, 2>&,
                            const Intersection&, const CellData&);

    void evalBoundary(GlobalPosition,const Intersection&,FluidState &, PhaseVector &);

    //! Set the initial values before the first pressure equation
    /*!
     * This method is called before first pressure equation is solved from Dumux::IMPET.
     */
    void initialize()
    {
        totalConcentration_[wCompIdx].resize(problem_.gridView().size(0));
        totalConcentration_[nCompIdx].resize(problem_.gridView().size(0));
    };

    //! \brief Write data files
     /*  \param writer applied VTK-writer */
    template<class MultiWriter>
    void addOutputVtkFields(MultiWriter &writer)
    {
        typedef typename GET_PROP(TypeTag, SolutionTypes)::ScalarSolution ScalarSolutionType;
        int size = problem_.gridView().size(0);
        ScalarSolutionType *totalC1PV = writer.allocateManagedBuffer(size);
        ScalarSolutionType *totalC2PV = writer.allocateManagedBuffer(size);
        *totalC1PV = this->totalConcentration_[wCompIdx];
        *totalC2PV = this->totalConcentration_[nCompIdx];
        writer.attachCellData(*totalC1PV, "total Concentration w-Comp");
        writer.attachCellData(*totalC2PV, "total Concentration n-Comp");
    }

    //! Function needed for restart option of the transport model: Write out
    void serializeEntity(std::ostream &outstream, const Element &element)
    {
        int globalIdx = problem().variables().index(element);
        outstream << totalConcentration_[wCompIdx][globalIdx]
                  << "  " << totalConcentration_[nCompIdx][globalIdx];
    }
    //! Function needed for restart option of the transport model: Read in
    void deserializeEntity(std::istream &instream, const Element &element)
    {
        int globalIdx = problem().variables().index(element);
        instream >>  totalConcentration_[wCompIdx][globalIdx]
                 >> totalConcentration_[nCompIdx][globalIdx];
    }

    /*! \name Access functions for protected variables  */
    //@{
    //! Return the vector of the transported quantity
    /*! For an immiscible IMPES scheme, this is the saturation. For Miscible simulations, however,
     *  the total concentration of all components is transported.
     */
    TransportSolutionType& transportedQuantity() DUNE_DEPRECATED
    {
        return totalConcentration_;
    }
    //! \copydoc transportedQuantity()
    void getTransportedQuantity(TransportSolutionType& transportedQuantity)
    {
        transportedQuantity = totalConcentration_;
    }
    //! \copydoc transportedQuantity()
    Scalar& totalConcentration(int compIdx, int globalIdx)
    {
        return totalConcentration_[compIdx][globalIdx][0];
    }
    //@}
    //! Constructs a FVTransport2P2C object
    /*!
     * Currently, the miscible transport scheme can not be applied with a global pressure / total velocity
     * formulation.
     *
     * \param problem a problem class object
     */
    FVTransport2P2C(Problem& problem) :
        totalConcentration_(0.),problem_(problem), switchNormals(false)
    {
        totalConcentration_.resize(GET_PROP_VALUE(TypeTag, NumComponents));
        totalConcentration_[wCompIdx].resize(problem.gridView().size(0));
        totalConcentration_[nCompIdx].resize(problem.gridView().size(0));

        restrictFluxInTransport_ = GET_PARAM(TypeTag,bool, RestrictFluxInTransport);
    }

    virtual ~FVTransport2P2C()
    {
    }

protected:
    TransportSolutionType totalConcentration_;
    Problem& problem_;
    bool impet_;

    static const int pressureType = GET_PROP_VALUE(TypeTag, PressureFormulation); //!< gives kind of pressure used (\f$ 0 = p_w \f$, \f$ 1 = p_n \f$, \f$ 2 = p_{global} \f$)
    bool restrictFluxInTransport_;
    bool switchNormals;
};

//! \brief Calculate the update vector and determine timestep size
/*!
 *  This method calculates the update vector \f$ u \f$ of the discretized equation
 *  \f[
       C^{\kappa , new} = C^{\kappa , old} + u,
 *  \f]
 *  where \f$ u = \sum_{element faces} \boldsymbol{v}_{\alpha} * \varrho_{\alpha} * X^{\kappa}_{\alpha} * \boldsymbol{n} * A_{element face} \f$,
 *  \f$ \boldsymbol{n} \f$ is the face normal and \f$ A_{element face} \f$ is the face area.
 *
 *  In addition to the \a update vector, the recommended time step size \a dt is calculated
 *  employing a CFL condition.
 *  This method = old concentrationUpdate()
 *
 *  \param t Current simulation time \f$\mathrm{[s]}\f$
 *  \param[out] dt Time step size \f$\mathrm{[s]}\f$
 *  \param[out] updateVec Update vector, or update estimate for secants, resp. Here in \f$\mathrm{[kg/m^3]}\f$
 *  \param impet Flag that determines if it is a real impet step or an update estimate for volume derivatives
 */
template<class TypeTag>
void FVTransport2P2C<TypeTag>::update(const Scalar t, Scalar& dt,
		TransportSolutionType& updateVec, bool impet = false)
{
    // initialize dt very large
    dt = 1E100;
    // store if we do update Estimate for flux functions
    impet_ = impet;

    // resize update vector and set to zero
    updateVec.resize(GET_PROP_VALUE(TypeTag, NumComponents));
    updateVec[wCompIdx].resize(problem_.gridView().size(0));
    updateVec[nCompIdx].resize(problem_.gridView().size(0));
    updateVec[wCompIdx] = 0;
    updateVec[nCompIdx] = 0;

    // Cell which restricts time step size
    int restrictingCell = -1;

    PhaseVector entries(0.), timestepFlux(0.);
    // compute update vector
    ElementIterator eItEnd = problem().gridView().template end<0> ();
    for (ElementIterator eIt = problem().gridView().template begin<0> (); eIt != eItEnd; ++eIt)
    {
        // get cell infos
        int globalIdxI = problem().variables().index(*eIt);
        CellData& cellDataI = problem().variables().cellData(globalIdxI);

        // some variables for time step calculation
        double sumfactorin = 0;
        double sumfactorout = 0;

        // run through all intersections with neighbors and boundary
        IntersectionIterator isItEnd = problem().gridView().iend(*eIt);
        for (IntersectionIterator isIt = problem().gridView().ibegin(*eIt); isIt != isItEnd; ++isIt)
        {

            /****** interior face   *****************/
            if (isIt->neighbor())
                getFlux(entries, timestepFlux, *isIt, cellDataI);

            /******  Boundary Face   *****************/
            if (isIt->boundary())
                getFluxOnBoundary(entries, timestepFlux, *isIt, cellDataI);

            // add to update vector
            updateVec[wCompIdx][globalIdxI] += entries[wCompIdx];
            updateVec[nCompIdx][globalIdxI] += entries[nCompIdx];

            // for time step calculation
            sumfactorin += timestepFlux[0];
            sumfactorout += timestepFlux[1];

        }// end all intersections

        /***********     Handle source term     ***************/
        PrimaryVariables q(NAN);
        problem().source(q, *eIt);
        updateVec[wCompIdx][globalIdxI] += q[contiWEqIdx];
        updateVec[nCompIdx][globalIdxI] += q[contiNEqIdx];

        // account for porosity in fluxes for time-step
        sumfactorin = std::max(sumfactorin,sumfactorout)
                        / problem().spatialParams().porosity(*eIt);

        if ( 1./sumfactorin < dt)
        {
            dt = 1./sumfactorin;
            restrictingCell= globalIdxI;
        }
    } // end grid traversal
    if(impet)
        Dune::dinfo << "Timestep restricted by CellIdx " << restrictingCell << " leads to dt = "<<dt * GET_PARAM(TypeTag, Scalar, CFLFactor)<< std::endl;
    return;
}
/*	Updates the transported quantity once an update is calculated.
 *  This method updates both, the internal transport solution vector and the entries in the cellData.
 *  \param updateVec Update vector, or update estimate for secants, resp. Here in \f$\mathrm{[kg/m^3]}\f$
 *
 */
template<class TypeTag>
void FVTransport2P2C<TypeTag>::updateTransportedQuantity(TransportSolutionType& updateVector)
{
    Scalar dt = problem().timeManager().timeStepSize();
    // loop thorugh all elements
    for (int i = 0; i< problem().gridView().size(0); i++)
    {
        CellData& cellDataI = problem().variables().cellData(i);
        for(int compIdx = 0; compIdx < GET_PROP_VALUE(TypeTag, NumComponents); compIdx++)
        {
            totalConcentration_[compIdx][i] += (updateVector[compIdx][i]*=dt);
            cellDataI.setTotalConcentration(compIdx, totalConcentration_[compIdx][i]);
        }
    }
}

//! Get flux at an interface between two cells
/** The flux thorugh \f$ \gamma{ij} \f$  is calculated according to the underlying pressure field,
 * calculated by the pressure model.
 *  \f[ - A_{\gamma_{ij}} \cdot \mathbf{K} \cdot \mathbf{u} \cdot (\mathbf{n}_{\gamma_{ij}} \cdot \mathbf{u})
      \sum_{\alpha} \varrho_{\alpha} \lambda_{\alpha} \sum_{\kappa} X^{\kappa}_{\alpha}
    \left( \frac{p_{\alpha,j}^t - p^{t}_{\alpha,i}}{\Delta x} + \varrho_{\alpha} \mathbf{g}\right) \f]
 * Here, \f$ \mathbf{u} \f$ is the normalized vector connecting the cell centers, and \f$ \mathbf{n}_{\gamma_{ij}} \f$
 * represents the normal of the face \f$ \gamma{ij} \f$. Due to the nature of the Primay Variable, the (volume-)specific
 * total mass concentration, this represents a mass flux per cell volume.
 * \param fluxEntries The flux entries, mass influx from cell \f$j\f$ to \f$i\f$.
 * \param timestepFlux flow velocities for timestep estimation
 * \param intersection The intersection
 * \param cellDataI The cell data for cell \f$i\f$
 */
template<class TypeTag>
void FVTransport2P2C<TypeTag>::getFlux(Dune::FieldVector<Scalar, 2>& fluxEntries,
                                        Dune::FieldVector<Scalar, 2>& timestepFlux,
                                        const Intersection& intersection,
                                        CellData& cellDataI)
{
    fluxEntries = 0.;
    timestepFlux = 0.;
    // cell information
    ElementPointer elementPtrI= intersection.inside();
    int globalIdxI = problem().variables().index(*elementPtrI);

    // get position
    const GlobalPosition globalPos = elementPtrI->geometry().center();
    const GlobalPosition& gravity_ = problem().gravity();
    // cell volume, assume linear map here
    Scalar volume = elementPtrI->geometry().volume();

    // get values of cell I
    Scalar pressI = problem().pressureModel().pressure(globalIdxI);
    Scalar pcI = cellDataI.capillaryPressure();
    DimMatrix K_I(problem().spatialParams().intrinsicPermeability(*elementPtrI));

    Scalar SwmobI = std::max((cellDataI.saturation(wPhaseIdx)
                            - problem().spatialParams().materialLawParams(*elementPtrI).Swr())
                            , 1e-2);
    Scalar SnmobI = std::max((cellDataI.saturation(nPhaseIdx)
                                - problem().spatialParams().materialLawParams(*elementPtrI).Snr())
                            , 1e-2);

    Scalar densityWI (0.), densityNWI(0.);
    densityWI= cellDataI.density(wPhaseIdx);
    densityNWI = cellDataI.density(nPhaseIdx);

    // face properties
    GlobalPosition unitOuterNormal = intersection.centerUnitOuterNormal();
    if (switchNormals)
        unitOuterNormal *= -1.0;
    Scalar faceArea = intersection.geometry().volume();

    // create vector for timestep and for update
    Dune::FieldVector<Scalar, 2> factor (0.);
    Dune::FieldVector<Scalar, 2> updFactor (0.);

    Scalar potentialW(0.), potentialNW(0.);

    // access neighbor
    ElementPointer neighborPtr = intersection.outside();
    int globalIdxJ = problem().variables().index(*neighborPtr);
    CellData& cellDataJ = problem().variables().cellData(globalIdxJ);

    // neighbor cell center in global coordinates
    const GlobalPosition& globalPosNeighbor = neighborPtr->geometry().center();

    // distance vector between barycenters
    GlobalPosition distVec = globalPosNeighbor - globalPos;
    // compute distance between cell centers
    Scalar dist = distVec.two_norm();

    GlobalPosition unitDistVec(distVec);
    unitDistVec /= dist;

    // phase densities in neighbor
    Scalar densityWJ (0.), densityNWJ(0.);
    densityWJ = cellDataJ.density(wPhaseIdx);
    densityNWJ = cellDataJ.density(nPhaseIdx);

    // average phase densities with central weighting
    double densityW_mean = (densityWI + densityWJ) * 0.5;
    double densityNW_mean = (densityNWI + densityNWJ) * 0.5;

    double pressJ = problem().pressureModel().pressure(globalIdxJ);
    Scalar pcJ = cellDataJ.capillaryPressure();

    // compute mean permeability
    DimMatrix meanK_(0.);
    Dumux::harmonicMeanMatrix(meanK_,
            K_I,
            problem().spatialParams().intrinsicPermeability(*neighborPtr));
    Dune::FieldVector<Scalar,dim> K(0);
    meanK_.umv(unitDistVec,K);

    // determine potentials for upwind
    switch (pressureType)
    {
    case pw:
    {
        potentialW = (K * unitOuterNormal) * (pressI - pressJ) / (dist);
        potentialNW = (K * unitOuterNormal) * (pressI - pressJ + pcI - pcJ) / (dist);
        break;
    }
    case pn:
    {
        potentialW = (K * unitOuterNormal) * (pressI - pressJ - pcI + pcJ) / (dist);
        potentialNW = (K * unitOuterNormal) * (pressI - pressJ) / (dist);
        break;
    }
    }
    // add gravity term
    potentialNW +=  (K * gravity_)  * (unitOuterNormal * unitDistVec) * densityNW_mean;
    potentialW +=  (K * gravity_)  * (unitOuterNormal * unitDistVec) * densityW_mean;

    // upwind mobility
    double lambdaW(0.), lambdaNW(0.);
    if(!impet_ or !restrictFluxInTransport_) // perform a simple uwpind scheme
    {
        if (potentialW >= 0.)
        {
            lambdaW = cellDataI.mobility(wPhaseIdx);
            cellDataI.setUpwindCell(intersection.indexInInside(), contiWEqIdx, true);

        }
        else
        {
            lambdaW = cellDataJ.mobility(wPhaseIdx);
            cellDataI.setUpwindCell(intersection.indexInInside(), contiWEqIdx, false);
        }

        if (potentialNW >= 0.)
        {
            lambdaNW = cellDataI.mobility(nPhaseIdx);
            cellDataI.setUpwindCell(intersection.indexInInside(), contiNEqIdx, true);
        }
        else
        {
            lambdaNW = cellDataJ.mobility(nPhaseIdx);
            cellDataI.setUpwindCell(intersection.indexInInside(), contiNEqIdx, false);
        }
    }
    else // if new potentials indicate same flow direction as in P.E., perform upwind
    {
        if (potentialW >= 0. && cellDataI.isUpwindCell(intersection.indexInInside(), contiWEqIdx))
            lambdaW = cellDataI.mobility(wPhaseIdx);
        else if (potentialW < 0. && !cellDataI.isUpwindCell(intersection.indexInInside(), contiWEqIdx))
            lambdaW = cellDataJ.mobility(wPhaseIdx);
        else    // potential of w-phase does not coincide with that of P.E.
        {
            bool isUpwindCell = cellDataI.isUpwindCell(intersection.indexInInside(), contiWEqIdx);
            //check if harmonic weithing is necessary
            if (!isUpwindCell && !(cellDataI.mobility(wPhaseIdx) != 0. && cellDataJ.mobility(wPhaseIdx) == 0.)) // check if outflow induce neglected phase flux
                lambdaW = cellDataI.mobility(wPhaseIdx);
            else if (isUpwindCell && !(cellDataJ.mobility(wPhaseIdx) != 0. && cellDataI.mobility(wPhaseIdx) == 0.)) // check if inflow induce neglected phase flux
                lambdaW = cellDataJ.mobility(wPhaseIdx);
            else
            {
                //a) perform harmonic averageing
                fluxEntries[wCompIdx] -= potentialW * faceArea / volume
                        * harmonicMean(cellDataI.massFraction(wPhaseIdx, wCompIdx) * cellDataI.mobility(wPhaseIdx) * cellDataI.density(wPhaseIdx),
                                cellDataJ.massFraction(wPhaseIdx, wCompIdx) * cellDataJ.mobility(wPhaseIdx) * cellDataJ.density(wPhaseIdx));
                fluxEntries[nCompIdx] -= potentialW * faceArea / volume
                        * harmonicMean(cellDataI.massFraction(wPhaseIdx, nCompIdx) * cellDataI.mobility(wPhaseIdx) * cellDataI.density(wPhaseIdx),
                                cellDataJ.massFraction(wPhaseIdx, nCompIdx) * cellDataJ.mobility(wPhaseIdx) * cellDataJ.density(wPhaseIdx));
                // b) timestep control
                // for timestep control : influx
                timestepFlux[0] += std::max(0.,
                        - potentialW * faceArea / volume * harmonicMean(cellDataI.density(wPhaseIdx),cellDataJ.density(wPhaseIdx)));
                // outflux
                timestepFlux[1] += std::max(0.,
                        potentialW * faceArea / volume * harmonicMean(cellDataI.density(wPhaseIdx),cellDataJ.density(wPhaseIdx)));

                //c) stop further standard calculations
                potentialW = 0;

                //d) output (only for one side)
                if(potentialW >= 0.)
                    Dune::dinfo << "harmonicMean flux of phase w used from cell" << globalIdxI<< " into " << globalIdxJ <<"\n";
            }
        }

        if (potentialNW >= 0. && cellDataI.isUpwindCell(intersection.indexInInside(), contiNEqIdx))
            lambdaNW = cellDataI.mobility(nPhaseIdx);
        else if (potentialNW < 0. && !cellDataI.isUpwindCell(intersection.indexInInside(), contiNEqIdx))
            lambdaNW = cellDataJ.mobility(nPhaseIdx);
        else    // potential of n-phase does not coincide with that of P.E.
        {
            bool isUpwindCell = cellDataI.isUpwindCell(intersection.indexInInside(), contiNEqIdx);
            //check if harmonic weithing is necessary
            if (!isUpwindCell && !(cellDataI.mobility(nPhaseIdx) != 0. && cellDataJ.mobility(nPhaseIdx) == 0.)) // check if outflow induce neglected phase flux
                lambdaNW = cellDataI.mobility(nPhaseIdx);
            else if (isUpwindCell && !(cellDataJ.mobility(nPhaseIdx) != 0. && cellDataI.mobility(nPhaseIdx) == 0.)) // check if inflow induce neglected phase flux
                lambdaNW = cellDataJ.mobility(nPhaseIdx);
            else
            {
                //a) perform harmonic averageing
                fluxEntries[wCompIdx] -= potentialNW * faceArea / volume
                        * harmonicMean(cellDataI.massFraction(nPhaseIdx, wCompIdx) * cellDataI.mobility(nPhaseIdx) * cellDataI.density(nPhaseIdx),
                                cellDataJ.massFraction(nPhaseIdx, wCompIdx) * cellDataJ.mobility(nPhaseIdx) * cellDataJ.density(nPhaseIdx));
                fluxEntries[nCompIdx] -= potentialNW * faceArea / volume
                        * harmonicMean(cellDataI.massFraction(nPhaseIdx, nCompIdx) * cellDataI.mobility(nPhaseIdx) * cellDataI.density(nPhaseIdx),
                                cellDataJ.massFraction(nPhaseIdx, nCompIdx) * cellDataJ.mobility(nPhaseIdx) * cellDataJ.density(nPhaseIdx));
                // b) timestep control
                // for timestep control : influx
                timestepFlux[0] += std::max(0.,
                        - potentialNW * faceArea / volume * harmonicMean(cellDataI.density(nPhaseIdx),cellDataJ.density(nPhaseIdx)));
                // outflux
                timestepFlux[1] += std::max(0.,
                        potentialNW * faceArea / volume * harmonicMean(cellDataI.density(nPhaseIdx),cellDataJ.density(nPhaseIdx)));

                //c) stop further standard calculations
                potentialNW = 0;

                //d) output (only for one side)
                if(potentialNW >= 0.)
                    Dune::dinfo << "harmonicMean flux of phase n used from cell" << globalIdxI<< " into " << globalIdxJ <<"\n";
            }
        }
    }

    // calculate and standardized velocity
    double velocityJIw = std::max((-lambdaW * potentialW) * faceArea / volume, 0.0);
    double velocityIJw = std::max(( lambdaW * potentialW) * faceArea / volume, 0.0);
    double velocityJIn = std::max((-lambdaNW * potentialNW) * faceArea / volume, 0.0);
    double velocityIJn = std::max(( lambdaNW * potentialNW) * faceArea / volume, 0.0);

    // for timestep control : influx
    timestepFlux[0] += velocityJIw + velocityJIn;

    double foutw = velocityIJw/SwmobI;
    double foutn = velocityIJn/SnmobI;
    if (std::isnan(foutw) || std::isinf(foutw) || foutw < 0) foutw = 0;
    if (std::isnan(foutn) || std::isinf(foutn) || foutn < 0) foutn = 0;
    timestepFlux[1] += foutw + foutn;

    fluxEntries[wCompIdx] +=
        velocityJIw * cellDataJ.massFraction(wPhaseIdx, wCompIdx) * densityWJ
        - velocityIJw * cellDataI.massFraction(wPhaseIdx, wCompIdx) * densityWI
        + velocityJIn * cellDataJ.massFraction(nPhaseIdx, wCompIdx) * densityNWJ
        - velocityIJn * cellDataI.massFraction(nPhaseIdx, wCompIdx) * densityNWI;
    fluxEntries[nCompIdx] +=
        velocityJIw * cellDataJ.massFraction(wPhaseIdx, nCompIdx) * densityWJ
        - velocityIJw * cellDataI.massFraction(wPhaseIdx, nCompIdx) * densityWI
        + velocityJIn * cellDataJ.massFraction(nPhaseIdx, nCompIdx) * densityNWJ
        - velocityIJn * cellDataI.massFraction(nPhaseIdx, nCompIdx) * densityNWI;

    return;
}
//! Get flux on Boundary
/** The flux thorugh \f$ \gamma{ij} \f$  is calculated according to the underlying pressure field,
 * calculated by the pressure model.
 *  \f[ - A_{\gamma_{ij}} \cdot \mathbf{K} \cdot \mathbf{u} \cdot (\mathbf{n}_{\gamma_{ij}} \cdot \mathbf{u})
      \sum_{\alpha} \varrho_{\alpha} \lambda_{\alpha} \sum_{\kappa} X^{\kappa}_{\alpha}
    \left( \frac{p_{\alpha,j}^t - p^{t}_{\alpha,i}}{\Delta x} + \varrho_{\alpha} \mathbf{g}\right) \f]
 * Here, \f$ \mathbf{u} \f$ is the normalized vector connecting the cell centers, and \f$ \mathbf{n}_{\gamma_{ij}} \f$
 * represents the normal of the face \f$ \gamma{ij} \f$.
 * \param fluxEntries The flux entries, mass influx from cell \f$j\f$ to \f$i\f$.
 * \param timestepFlux flow velocities for timestep estimation
 * \param intersection The intersection
 * \param cellDataI The cell data for cell \f$i\f$
 */
template<class TypeTag>
void FVTransport2P2C<TypeTag>::getFluxOnBoundary(Dune::FieldVector<Scalar, 2>& fluxEntries,
                                                    Dune::FieldVector<Scalar, 2>& timestepFlux,
                                                    const Intersection& intersection,
                                                    const CellData& cellDataI)
{
    // cell information
    ElementPointer elementPtrI= intersection.inside();
    int globalIdxI = problem().variables().index(*elementPtrI);

    // get position
    const GlobalPosition globalPos = elementPtrI->geometry().center();

    // cell volume, assume linear map here
    Scalar volume = elementPtrI->geometry().volume();
    const GlobalPosition& gravity_ = problem().gravity();
    // get values of cell I
    Scalar pressI = problem().pressureModel().pressure(globalIdxI);
    Scalar pcI = cellDataI.capillaryPressure();
    DimMatrix K_I(problem().spatialParams().intrinsicPermeability(*elementPtrI));

    Scalar SwmobI = std::max((cellDataI.saturation(wPhaseIdx)
                            - problem().spatialParams().materialLawParams(*elementPtrI).Swr())
                            , 1e-2);
    Scalar SnmobI = std::max((cellDataI.saturation(nPhaseIdx)
                                - problem().spatialParams().materialLawParams(*elementPtrI).Snr())
                            , 1e-2);

    Scalar densityWI (0.), densityNWI(0.);
    densityWI= cellDataI.density(wPhaseIdx);
    densityNWI = cellDataI.density(nPhaseIdx);

    // face properties
    GlobalPosition unitOuterNormal = intersection.centerUnitOuterNormal();
    if (switchNormals)
        unitOuterNormal *= -1.0;
    Scalar faceArea = intersection.geometry().volume();

    // create vector for timestep and for update
    Dune::FieldVector<Scalar, 2> factor (0.);
    Dune::FieldVector<Scalar, 2> updFactor (0.);

    Scalar potentialW(0.), potentialNW(0.);
    // center of face in global coordinates
    const GlobalPosition& globalPosFace = intersection.geometry().center();

    // distance vector between barycenters
    GlobalPosition distVec = globalPosFace - globalPos;
    // compute distance between cell centers
    Scalar dist = distVec.two_norm();

    GlobalPosition unitDistVec(distVec);
    unitDistVec /= dist;

    //instantiate a fluid state
    FluidState BCfluidState;

    //get boundary type
    BoundaryTypes bcTypes;
    problem().boundaryTypes(bcTypes, intersection);

    /**********         Dirichlet Boundary        *************/
    if (bcTypes.isDirichlet(contiWEqIdx)) // if contiWEq is Dirichlet, so is contiNEq
    {
        //get dirichlet pressure boundary condition
        PhaseVector pressBound(0.);
        Scalar pcBound (0.);

        // read boundary values
        this->evalBoundary(globalPosFace,
                        intersection,
                        BCfluidState,
                        pressBound);

        // determine fluid properties at the boundary
        Scalar densityWBound = BCfluidState.density(wPhaseIdx);
        Scalar densityNWBound = BCfluidState.density(nPhaseIdx);
        Scalar viscosityWBound = FluidSystem::viscosity(BCfluidState, wPhaseIdx);
        Scalar viscosityNWBound = FluidSystem::viscosity(BCfluidState, nPhaseIdx);
        if(GET_PROP_VALUE(TypeTag, EnableCapillarity))
            pcBound = BCfluidState.capillaryPressure();
        // average
        double densityW_mean = (densityWI + densityWBound) / 2;
        double densityNW_mean = (densityNWI + densityNWBound) / 2;

        // prepare K
        Dune::FieldVector<Scalar,dim> K(0);
        K_I.umv(unitDistVec,K);

        //calculate potential gradient
        switch (pressureType)
        {
            case pw:
            {
                potentialW = (K * unitOuterNormal) *
                        (pressI - pressBound[wPhaseIdx]) / dist;
                potentialNW = (K * unitOuterNormal) *
                        (pressI + pcI - pressBound[wPhaseIdx] - pcBound)
                        / dist;
                break;
            }
            case pn:
            {
                potentialW = (K * unitOuterNormal) *
                        (pressI - pcI - pressBound[nPhaseIdx] + pcBound)
                        / dist;
                potentialNW = (K * unitOuterNormal) *
                        (pressI - pressBound[nPhaseIdx]) / dist;
                break;
            }
        }
        potentialW += (K * gravity_)  * (unitOuterNormal * unitDistVec) * densityW_mean;;
        potentialNW += (K * gravity_)  * (unitOuterNormal * unitDistVec) * densityNW_mean;;

        // do upwinding for lambdas
        double lambdaW, lambdaNW;
        if (potentialW >= 0.)
            lambdaW = cellDataI.mobility(wPhaseIdx);
        else
            {
            if(GET_PROP_VALUE(TypeTag, BoundaryMobility)==Indices::satDependent)
                lambdaW = BCfluidState.saturation(wPhaseIdx) / viscosityWBound;
            else
                lambdaW = MaterialLaw::krw(
                        problem().spatialParams().materialLawParams(*elementPtrI), BCfluidState.saturation(wPhaseIdx))
                        / viscosityWBound;
            }
        if (potentialNW >= 0.)
            lambdaNW = cellDataI.mobility(nPhaseIdx);
        else
            {
            if(GET_PROP_VALUE(TypeTag, BoundaryMobility)==Indices::satDependent)
                lambdaNW = BCfluidState.saturation(nPhaseIdx) / viscosityNWBound;
            else
                lambdaNW = MaterialLaw::krn(
                        problem().spatialParams().materialLawParams(*elementPtrI), BCfluidState.saturation(wPhaseIdx))
                        / viscosityNWBound;
            }
        // calculate and standardized velocity
        double velocityJIw = std::max((-lambdaW * potentialW) * faceArea / volume, 0.0);
        double velocityIJw = std::max(( lambdaW * potentialW) * faceArea / volume, 0.0);
        double velocityJIn = std::max((-lambdaNW * potentialNW) * faceArea / volume, 0.0);
        double velocityIJn = std::max(( lambdaNW * potentialNW) * faceArea / volume, 0.0);

        // for timestep control
        timestepFlux[0] = velocityJIw + velocityJIn;

        double foutw = velocityIJw/SwmobI;
        double foutn = velocityIJn/SnmobI;
        if (std::isnan(foutw) || std::isinf(foutw) || foutw < 0) foutw = 0;
        if (std::isnan(foutn) || std::isinf(foutn) || foutn < 0) foutn = 0;
        timestepFlux[1] = foutw + foutn;

        fluxEntries[wCompIdx] =
            + velocityJIw * BCfluidState.massFraction(wPhaseIdx, wCompIdx) * densityWBound
            - velocityIJw * cellDataI.massFraction(wPhaseIdx, wCompIdx) * densityWI
            + velocityJIn * BCfluidState.massFraction(nPhaseIdx, wCompIdx) * densityNWBound
            - velocityIJn * cellDataI.massFraction(nPhaseIdx, wCompIdx) * densityNWI ;
        fluxEntries[nCompIdx] =
            velocityJIw * BCfluidState.massFraction(wPhaseIdx, nCompIdx) * densityWBound
            - velocityIJw * cellDataI.massFraction(wPhaseIdx, nCompIdx) * densityWI
            + velocityJIn * BCfluidState.massFraction(nPhaseIdx, nCompIdx) * densityNWBound
            - velocityIJn * cellDataI.massFraction(nPhaseIdx, nCompIdx) * densityNWI ;
    }//end dirichlet boundary
    else if (bcTypes.isNeumann(contiWEqIdx))
    {
        // Convention: outflow => positive sign : has to be subtracted from update vec
        PrimaryVariables J(NAN);
        problem().neumann(J, intersection);
        fluxEntries[wCompIdx] = - J[contiWEqIdx] * faceArea / volume;
        fluxEntries[nCompIdx] = - J[contiNEqIdx] * faceArea / volume;

        // for timestep control
        #define cflIgnoresNeumann
        #ifdef cflIgnoresNeumann
        timestepFlux[0] = 0;
        timestepFlux[1] = 0;
        #else
        double inflow = updFactor[wCompIdx] / densityW + updFactor[nCompIdx] / densityNW;
        if (inflow>0)
            {
            timestepFlux[0] = updFactor[wCompIdx] / densityW
                        + updFactor[nCompIdx] / densityNW;    // =factor in
            timestepFlux[1] = -(updFactor[wCompIdx] / densityW /SwmobI
                        + updFactor[nCompIdx] / densityNW / SnmobI);    // =factor out
            }
        else
        {
            timestepFlux[0] = -(updFactor[wCompIdx] / densityW
                            + updFactor[nCompIdx] / densityNW);    // =factor in
            timestepFlux[1] = updFactor[wCompIdx] / densityW /SwmobI
                        + updFactor[nCompIdx] / densityNW / SnmobI;    // =factor out
        }
        #endif
    }//end neumann boundary
    return;
}

//! evaluate the boundary conditions
/*!
 *  As the transport primary variable in this formulation is the total component
 *  concentration, \f$ C^{\kappa} \f$ it seems natural that the boundary values
 *  are also total concentrations. However, as for the initial conditions, it is
 *  possible to define boundaries by means of a saturation. This choice determines
 *  which version of flash calculation is necessary to get to the composition at
 *  the boundary.
 *  \param globalPosFace Face of the current boundary
 *  \param intersection The current intersection
 *  \param BCfluidState FluidState object that is used for the boundary
 *  \param pressBound Boundary values of phase pressures
 */
template<class TypeTag>
void FVTransport2P2C<TypeTag>::evalBoundary(GlobalPosition globalPosFace,
                                            const Intersection& intersection,
                                            FluidState &BCfluidState,
                                            PhaseVector &pressBound)
{
    const ElementPointer eIt= intersection.inside();
    // read boundary values
    PrimaryVariables primaryVariablesOnBoundary(0.);
    problem().dirichlet(primaryVariablesOnBoundary, intersection);

    // read boundary type
    typename Indices::BoundaryFormulation bcType;
    problem().boundaryFormulation(bcType, intersection);
    if (bcType == Indices::saturation)
    {
        Scalar satBound = primaryVariablesOnBoundary[contiWEqIdx];
        if(GET_PROP_VALUE(TypeTag, EnableCapillarity))
        {
            Scalar pcBound = MaterialLaw::pC(problem().spatialParams().materialLawParams(*eIt),
                    satBound);
            switch (pressureType)
            {
            case pw:
            {
                pressBound[wPhaseIdx] = primaryVariablesOnBoundary[Indices::pressureEqIdx];
                pressBound[nPhaseIdx] = primaryVariablesOnBoundary[Indices::pressureEqIdx] + pcBound;
                break;
            }
            case pn:
            {
                pressBound[wPhaseIdx] = primaryVariablesOnBoundary[Indices::pressureEqIdx] - pcBound;
                pressBound[nPhaseIdx] = primaryVariablesOnBoundary[Indices::pressureEqIdx];
                break;
            }
            }
        }
        else // capillarity neglected
            pressBound[wPhaseIdx] = pressBound[nPhaseIdx] = primaryVariablesOnBoundary[Indices::pressureEqIdx];


        BCfluidState.satFlash(satBound, pressBound, problem().spatialParams().porosity(*eIt),
                            problem().temperatureAtPos(globalPosFace));

    }
    else if (bcType == Indices::concentration)
    {
        // saturation and hence pc and hence corresponding pressure unknown
        pressBound[wPhaseIdx] = pressBound[nPhaseIdx] = primaryVariablesOnBoundary[Indices::pressureEqIdx];
        Scalar Z1Bound = primaryVariablesOnBoundary[contiWEqIdx];
        BCfluidState.update(Z1Bound, pressBound, problem().spatialParams().porosity(*eIt),
                            problem().temperatureAtPos(globalPosFace));

        if(GET_PROP_VALUE(TypeTag, EnableCapillarity))
        {
            Scalar pcBound = MaterialLaw::pC(problem().spatialParams().materialLawParams(*eIt),
                    BCfluidState.saturation(wPhaseIdx));
            int maxiter = 3;
            //start iteration loop
            for(int iter=0; iter < maxiter; iter++)
            {
                //prepare pressures to enter flash calculation
                switch (pressureType)
                {
                case pw:
                {
                    pressBound[wPhaseIdx] = primaryVariablesOnBoundary[Indices::pressureEqIdx];
                    pressBound[nPhaseIdx] = primaryVariablesOnBoundary[Indices::pressureEqIdx]
                                                                                        + pcBound;
                    break;
                }
                case pn:
                {
                    pressBound[wPhaseIdx] = primaryVariablesOnBoundary[Indices::pressureEqIdx]
                                                                                         - pcBound;
                    pressBound[nPhaseIdx] = primaryVariablesOnBoundary[Indices::pressureEqIdx];
                    break;
                }
                }

                //store old pc
                Scalar oldPc = pcBound;
                //update with better pressures
                BCfluidState.update(Z1Bound, pressBound, problem().spatialParams().porosity(*eIt),
                                    problem().temperatureAtPos(globalPosFace));
                pcBound = MaterialLaw::pC(problem().spatialParams().materialLawParams(*eIt),
                        BCfluidState.saturation(wPhaseIdx));
                // TODO: get right criterion, do output for evaluation
                //converge criterion
                if (abs(oldPc-pcBound)<10)
                    iter = maxiter;
            }
        }
    }
    else
        DUNE_THROW(Dune::NotImplemented, "Boundary Formulation neither Concentration nor Saturation??");
}

}
#endif
