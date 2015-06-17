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
 * \file
 *
 * \brief Non-isothermal two-phase two-component porous-medium subproblem
 *        with coupling at the top boundary.
 */
#ifndef DUMUX_2P2CNISUB_PROBLEM_HH
#define DUMUX_2P2CNISUB_PROBLEM_HH

#include <dumux/implicit/2p2c/2p2cindices.hh>
#include <dumux/implicit/common/implicitporousmediaproblem.hh>
#include <dumux/material/fluidmatrixinteractions/2p/thermalconductivityjohansen.hh>
#include <dumux/material/fluidmatrixinteractions/2p/thermalconductivitysomerton.hh>
#include <dumux/multidomain/common/subdomainpropertydefaults.hh>
#include <dumux/multidomain/common/multidomainlocaloperator.hh>
#include <dumux/multidomain/couplinglocalresiduals/2p2cnicouplinglocalresidual.hh>

#include "2cnizeroeq2p2cnispatialparameters.hh"

namespace Dumux
{
template <class TypeTag>
class TwoPTwoCNISubProblem;

namespace Properties
{
NEW_TYPE_TAG(TwoPTwoCNISubProblem,
             INHERITS_FROM(BoxTwoPTwoCNI, SubDomain, TwoCNIZeroEqTwoPTwoCNISpatialParams));

// Set the problem property
SET_TYPE_PROP(TwoPTwoCNISubProblem, Problem, TwoPTwoCNISubProblem<TTAG(TwoPTwoCNISubProblem)>);

// Use the 2p2cni local jacobian operator for the 2p2cniCoupling model
SET_TYPE_PROP(TwoPTwoCNISubProblem, LocalResidual, TwoPTwoCNICouplingLocalResidual<TypeTag>);

// Choose pn and Sw as primary variables
SET_INT_PROP(TwoPTwoCNISubProblem, Formulation, TwoPTwoCFormulation::pnsw);

// The gas component balance (air) is replaced by the total mass balance
SET_PROP(TwoPTwoCNISubProblem, ReplaceCompEqIdx)
{
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    static const int value = Indices::contiNEqIdx;
};

// Use the fluid system from the coupled problem
SET_TYPE_PROP(TwoPTwoCNISubProblem,
              FluidSystem,
              typename GET_PROP_TYPE(typename GET_PROP_TYPE(TypeTag, MultiDomainTypeTag), FluidSystem));

// Johanson is used as model to compute the effective thermal heat conductivity
SET_TYPE_PROP(TwoPTwoCNISubProblem, ThermalConductivityModel,
              ThermalConductivityJohansen<typename GET_PROP_TYPE(TypeTag, Scalar)>);

// Use formulation based on mass fractions
SET_BOOL_PROP(TwoPTwoCNISubProblem, UseMoles, false);

// Enable/disable velocity output
SET_BOOL_PROP(TwoPTwoCNISubProblem, VtkAddVelocity, true);

// Enable gravity
SET_BOOL_PROP(TwoPTwoCNISubProblem, ProblemEnableGravity, true);
}

/*!
 * \ingroup ImplicitTestProblems
 * \ingroup MultidomainProblems
 * \brief Non-isothermal two-phase two-component porous-medium subproblem
 *        with coupling at the top boundary.
 *
 * \todo update description
 *
 * This sub problem uses the \ref TwoPTwoCModel. It is part of the 2p2cni model and
 * is combined with the zeroeq2cnisubproblem for the free flow domain.
 */
template <class TypeTag = TTAG(TwoPTwoCNISubProblem) >
class TwoPTwoCNISubProblem : public ImplicitPorousMediaProblem<TypeTag>
{
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GridView::Grid Grid;

    typedef TwoPTwoCNISubProblem<TypeTag> ThisType;
    typedef ImplicitPorousMediaProblem<TypeTag> ParentType;

    // copy some indices for convenience
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;

    // the type tag of the coupled problem
    typedef typename GET_PROP_TYPE(TypeTag, MultiDomainTypeTag) CoupledTypeTag;

    enum { numEq = GET_PROP_VALUE(TypeTag, NumEq) };
    enum { numPhases = GET_PROP_VALUE(TypeTag, NumPhases) };
    enum { numComponents = GET_PROP_VALUE(TypeTag, NumComponents) };
    enum { // the equation indices
        contiTotalMassIdx = Indices::contiNEqIdx,
        contiWEqIdx = Indices::contiWEqIdx,
        energyEqIdx = Indices::energyEqIdx
    };
    enum { // the indices of the primary variables
        pressureIdx = Indices::pressureIdx,
        switchIdx = Indices::switchIdx,
        temperatureIdx = Indices::temperatureIdx
    };
    enum {
        wPhaseOnly = Indices::wPhaseOnly,
        nPhaseOnly = Indices::nPhaseOnly,
        bothPhases = Indices::bothPhases
    };
    enum {
        wPhaseIdx = Indices::wPhaseIdx,
        nPhaseIdx = Indices::nPhaseIdx
    };
    enum { // grid and world dimension
        dim = GridView::dimension,
        dimWorld = GridView::dimensionworld
    };

    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;
    typedef typename GET_PROP_TYPE(TypeTag, TimeManager) TimeManager;

    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<dim>::Entity Vertex;
    typedef typename GridView::Intersection Intersection;

    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;

    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;

public:
    /*!
     * \brief The sub-problem for the porous-medium subdomain
     *
     * \param timeManager The TimeManager which is used by the simulation
     * \param gridView The simulation's idea about physical space
     */
    TwoPTwoCNISubProblem(TimeManager &timeManager, const GridView &gridView)
        : ParentType(timeManager, gridView)
    {
        Scalar noDarcyX1 = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, Grid, NoDarcyX1);
        Scalar noDarcyX2 = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, Grid, NoDarcyX2);
        Scalar xMin = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, Grid, XMin);
        Scalar xMax = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, Grid, XMax);

        bBoxMin_[0] = std::max(xMin,noDarcyX1);
        bBoxMax_[0] = std::min(xMax,noDarcyX2);
        bBoxMin_[1] = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, Grid, YMin);
        bBoxMax_[1] = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, Grid, InterfacePos);
        runUpDistanceX1_ = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, Grid, RunUpDistanceX1); // first part of the interface without coupling
        runUpDistanceX2_ = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, Grid, RunUpDistanceX2); // second part of the interface without coupling

        refTemperature_ = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, PorousMedium, RefTemperaturePM);
        refPressure_ = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, PorousMedium, RefPressurePM);
        refSw_ = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, PorousMedium, RefSw);

        freqMassOutput_ = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, int, Output, FreqMassOutput);

        storageLastTimestep_ = Scalar(0);
        lastMassOutputTime_ = Scalar(0);

        outfile.open("storage.out");
        outfile << "Time[s]" << ";"
                << "TotalMassChange[kg/(s*mDepth)]" << ";"
                << "WaterMassChange[kg/(s*mDepth))]" << ";"
                << "IntEnergyChange[J/(m^3*s*mDepth)]" << ";"
                << "WaterMass[kg/mDepth]" << ";"
                << "WaterMassLoss[kg/mDepth]" << ";"
                << "EvaporationRate[mm/s]"
                << std::endl;
    }

    //! \brief The destructor
    ~TwoPTwoCNISubProblem()
    {
        outfile.close();
    }

    /*!
     * \name Problem parameters
     */
    // \{

    //! \copydoc Dumux::ImplicitProblem::name()
    const std::string &name() const
    { return GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, std::string, Output, NamePM); }

    //! \copydoc Dumux::ImplicitProblem::init()
    void init()
    {
        ParentType::init();
        this->model().globalStorage(storageLastTimestep_);
    }

    // \}

    /*!
     * \name Boundary conditions
     */
    // \{

    //! \copydoc Dumux::ImplicitProblem::boundaryTypesAtPos()
    void boundaryTypesAtPos(BoundaryTypes &values,
                            const GlobalPosition &globalPos) const
    {
        values.setAllNeumann();

        if (onLowerBoundary_(globalPos))
        {
            values.setDirichlet(temperatureIdx, energyEqIdx);
        }

        if (onUpperBoundary_(globalPos))
        {
            if (globalPos[0] > runUpDistanceX1_ - eps_
                && globalPos[0] < runUpDistanceX2_ + eps_)
            {
                values.setAllCouplingInflow();
            }
            else
                values.setAllNeumann();
        }
    }

    //! \copydoc Dumux::ImplicitProblem::dirichletAtPos()
    void dirichletAtPos(PrimaryVariables &values, const GlobalPosition &globalPos) const
    {
        initial_(values, globalPos);
    }

    //! \copydoc Dumux::ImplicitProblem::neumannAtPos()
    void neumannAtPos(PrimaryVariables &values, const GlobalPosition &globalPos) const
    {
        values = 0.;
    }

    // \}

    /*!
     * \name Volume terms
     */
    // \{
    //! \copydoc Dumux::ImplicitProblem::sourceAtPos()
    void sourceAtPos(PrimaryVariables &values, const GlobalPosition &globalPos) const
    {
        values = 0.;
    }

    //! \copydoc Dumux::ImplicitProblem::initialAtPos()
    void initialAtPos(PrimaryVariables &values, const GlobalPosition &globalPos) const
    {
        initial_(values, globalPos);
    }

    /*!
     * \brief Return the initial phase state inside a control volume.
     *
     * \param vertex The vertex
     * \param globalIdx The index of the global vertex
     * \param globalPos The global position
     */
    int initialPhasePresence(const Vertex &vertex,
                             const int &globalIdx,
                             const GlobalPosition &globalPos) const
    {
        return bothPhases;
    }

    /*!
     * \brief Called by the time manager after the time integration to
     *        do some post processing on the solution.
     */
    void postTimeStep()
    {
        // Calculate masses
        PrimaryVariables storage;

        this->model().globalStorage(storage);
        const Scalar time = this->timeManager().time() +  this->timeManager().timeStepSize();

        static Scalar initialWaterContent = 0.0;                                                                                                                                                                                                                                                                                                                  ;
        if (this->timeManager().time() <  this->timeManager().timeStepSize() + 1e-10)
            initialWaterContent = storage[contiWEqIdx];

        // Write mass balance information for rank 0
        if (this->gridView().comm().rank() == 0)
        {
            if (this->timeManager().timeStepIndex() % freqMassOutput_ == 0
                || this->timeManager().episodeWillBeOver())
            {
                PrimaryVariables storageChange(0.);
                storageChange = storageLastTimestep_ - storage;

                assert(time - lastMassOutputTime_ != 0);
                storageChange /= (time - lastMassOutputTime_);

                std::cout << "Time[s]: " << time
                          << " TotalMass[kg]: " << storage[contiTotalMassIdx]
                          << " WaterMass[kg]: " << storage[contiWEqIdx]
                          << " IntEnergy[J/m^3]: " << storage[energyEqIdx]
                          << " WaterMassChange[kg/s]: " << storageChange[contiWEqIdx]
                          << std::endl;
                if (this->timeManager().time() != 0.)
                    outfile << time << ";"
                            << storageChange[contiTotalMassIdx] << ";"
                            << storageChange[contiWEqIdx] << ";"
                            << storageChange[energyEqIdx] << ";"
                            << storage[contiWEqIdx] << ";"
                            << initialWaterContent - storage[contiWEqIdx] << ";"
                            << storageChange[contiWEqIdx] / (bBoxMax_[0]-bBoxMin_[0])
                            << std::endl;

                storageLastTimestep_ = storage;
                lastMassOutputTime_ = time;
            }
        }
    }

    /*!
     * \brief Determine if we are on a corner of the grid
     *
     * \param globalPos The global position
     */
    bool isCornerPoint(const GlobalPosition &globalPos)
    {
        return ((onLeftBoundary_(globalPos) && onLowerBoundary_(globalPos))
                || (onLeftBoundary_(globalPos) && onUpperBoundary_(globalPos))
                || (onRightBoundary_(globalPos) && onLowerBoundary_(globalPos))
                || (onRightBoundary_(globalPos) && onUpperBoundary_(globalPos)));
    }

    /*!
     * \brief Returns whether the position is an interface corner point
     *
     * This function is required in case of mortar coupling otherwise it should return false
     *
     * \param globalPos The global position
     */
    bool isInterfaceCornerPoint(const GlobalPosition &globalPos) const
    { return false; }

    // \}

private:
    /*!
     * \brief Internal method for the initial condition
     *        (reused for the dirichlet conditions!)
     */
    void initial_(PrimaryVariables &values,
                  const GlobalPosition &globalPos) const
    {
        values[pressureIdx] = refPressure_
                              + 1000. * this->gravity()[1] * (globalPos[1] - bBoxMax_[1]);
        values[switchIdx] = refSw_;
        values[temperatureIdx] = refTemperature_;
    }

    bool onLeftBoundary_(const GlobalPosition &globalPos) const
    { return globalPos[0] < bBoxMin_[0] + eps_; }

    bool onRightBoundary_(const GlobalPosition &globalPos) const
    { return globalPos[0] > bBoxMax_[0] - eps_; }

    bool onLowerBoundary_(const GlobalPosition &globalPos) const
    { return globalPos[1] < bBoxMin_[1] + eps_; }

    bool onUpperBoundary_(const GlobalPosition &globalPos) const
    { return globalPos[1] > bBoxMax_[1] - eps_; }

    bool onBoundary_(const GlobalPosition &globalPos) const
    {
        return (onLeftBoundary_(globalPos) || onRightBoundary_(globalPos)
                || onLowerBoundary_(globalPos) || onUpperBoundary_(globalPos));
    }

    static constexpr Scalar eps_ = 1e-8;
    GlobalPosition bBoxMin_;
    GlobalPosition bBoxMax_;

    int freqMassOutput_;

    PrimaryVariables storageLastTimestep_;
    Scalar lastMassOutputTime_;

    Scalar refTemperature_;
    Scalar refPressure_;
    Scalar refSw_;

    Scalar runUpDistanceX1_;
    Scalar runUpDistanceX2_;
    std::ofstream outfile;
};
} //end namespace Dumux

#endif // DUMUX_TWOPTWOCNI_SUBPROBLEM_HH
