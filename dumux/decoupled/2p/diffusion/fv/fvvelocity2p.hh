// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2009 by Markus Wolff                                      *
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
#ifndef DUMUX_FVVELOCITY2P_HH
#define DUMUX_FVVELOCITY2P_HH

/**
 * @file
 * @brief  Velocity Field from a finite volume solution of a pressure equation.
 * @author Markus Wolff
 */

#include <dune/grid/common/gridenums.hh>
#include <dumux/decoupled/2p/diffusion/diffusionproperties2p.hh>

namespace Dumux
{
//! \ingroup FVPressure2p
//! \brief Determines the velocity from a finite volume solution of the  pressure equation of a sequential model (IMPES).
/*! Calculates phase velocities or total velocity from a known pressure field applying a finite volume discretization.
 * The wetting or the non-wetting phase pressure, or the global pressure has to be given as piecewise constant cell values.
 * The phase velocities are calculated following  Darcy's law as
 * \f[
 * \boldsymbol v_\alpha = \lambda_\alpha \boldsymbol K \left(\text{grad}\, p_\alpha + \rho_\alpha g  \text{grad}\, z \right),
 * \f]
 * where \f$ p_\alpha \f$ denotes the pressure of phase \f$ _\alpha \f$ (wetting or non-wetting), \f$ \boldsymbol K \f$ the absolute permeability, \f$ \lambda_\alpha \f$ the phase mobility, \f$ \rho_\alpha \f$ the phase density and \f$ g \f$ the gravity constant.
 * The total velocity is either calculated as sum of the phase velocities
 * \f[
 * \boldsymbol v_{total} = \boldsymbol v_{wetting}+\boldsymbol v_{non-wetting},
 * \f]
 * or with a given global pressure
 * \f[
 * \boldsymbol v_{total} = \lambda_{total} \boldsymbol K \left(\text{grad}\, p_{global} + \sum f_\alpha \rho_\alpha g  \text{grad}\, z\right).
 * \f]
 *
 * \tparam TypeTag The Type Tag
 */

template<class TypeTag>
class FVVelocity2P
{
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;

    typedef typename GET_PROP_TYPE(TypeTag, SpatialParams) SpatialParams;
    typedef typename SpatialParams::MaterialLaw MaterialLaw;

    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;

    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, FluidState) FluidState;

    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;
    typedef typename GET_PROP(TypeTag, SolutionTypes) SolutionTypes;
    typedef typename SolutionTypes::PrimaryVariables PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, CellData) CellData;

    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    typedef typename GridView::Traits::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<0>::EntityPointer ElementPointer;
    typedef typename GridView::Intersection Intersection;
    typedef typename GridView::IntersectionIterator IntersectionIterator;

    enum
    {
        dim = GridView::dimension, dimWorld = GridView::dimensionworld
    };

    typedef Dune::GenericReferenceElements<Scalar, dim> ReferenceElementContainer;

    enum
    {
        pw = Indices::pressureW,
        pn = Indices::pressureNW,
        pglobal = Indices::pressureGlobal,
        vw = Indices::velocityW,
        vn = Indices::velocityNW,
        vt = Indices::velocityTotal,
        Sw = Indices::saturationW,
        Sn = Indices::saturationNW,
        pressureIdx = Indices::pressureIdx,
        saturationIdx = Indices::saturationIdx,
        eqIdxPress = Indices::pressEqIdx,
        eqIdxSat = Indices::satEqIdx
    };
    enum
    {
        wPhaseIdx = Indices::wPhaseIdx, nPhaseIdx = Indices::nPhaseIdx, numPhases = GET_PROP_VALUE(TypeTag, NumPhases)
    };

    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;
    typedef Dune::FieldMatrix<Scalar, dim, dim> DimMatrix;

public:
    /*! \brief Constructs a FVVelocity2P object
     * \param problem A Problem class object
     */
    FVVelocity2P(Problem& problem) :
            problem_(problem), gravity_(problem.gravity())
    {
        if (GET_PROP_VALUE(TypeTag, EnableCompressibility) && velocityType_ == vt)
        {
            DUNE_THROW(Dune::NotImplemented,
                    "Total velocity - global pressure - model cannot be used with compressible fluids!");
        }
        if (velocityType_ != vw && velocityType_ != vn && velocityType_ != vt)
        {
            DUNE_THROW(Dune::NotImplemented, "Velocity type not supported!");
        }

        if (!compressibility_)
        {
            ElementIterator element = problem_.gridView().template begin<0> ();
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
        }
    }

    // Calculates the velocity at a cell-cell interface.
    void calculateVelocity(const Intersection&, CellData&);

    //Calculates the velocity at a boundary.
    void calculateVelocityOnBoundary(const Intersection&, CellData&);

    /*! \brief Indicates if velocity is reconstructed in the pressure step or in the transport step
     *
     * Returns true (In the standard finite volume discretization the velocity is calculated during the saturation transport.)
     */
    bool calculateVelocityInTransport()
    {
        return true;
    }

    /*! \brief Adds velocity output to the output file
     *
     * Adds the phase velocities or a total velocity (depending on the formulation) to the output.
     *
     * \tparam MultiWriter Class defining the output writer
     * \param writer The output writer (usually a <tt>VTKMultiWriter</tt> object)
     *
     */
    template<class MultiWriter>
    void addOutputVtkFields(MultiWriter &writer)
    {
        Dune::BlockVector < Dune::FieldVector<Scalar, dim> > &velocity = *(writer.template allocateManagedBuffer<Scalar,
                dim>(problem_.gridView().size(0)));
        Dune::BlockVector < Dune::FieldVector<Scalar, dim> > &velocitySecondPhase =
                *(writer.template allocateManagedBuffer<Scalar, dim>(problem_.gridView().size(0)));

        // compute update vector
        ElementIterator eItEnd = problem_.gridView().template end<0>();
        for (ElementIterator eIt = problem_.gridView().template begin<0>(); eIt != eItEnd; ++eIt)
        {
            // cell index
            int globalIdx = problem_.variables().index(*eIt);

            CellData& cellData = problem_.variables().cellData(globalIdx);

            Dune::FieldVector < Scalar, 2 * dim > fluxW(0);
            Dune::FieldVector < Scalar, 2 * dim > fluxNW(0);
            // run through all intersections with neighbors and boundary
            IntersectionIterator isItEnd = problem_.gridView().iend(*eIt);
            for (IntersectionIterator isIt = problem_.gridView().ibegin(*eIt); isIt != isItEnd; ++isIt)
            {
                int isIndex = isIt->indexInInside();

                fluxW[isIndex] += isIt->geometry().volume()
                        * (isIt->centerUnitOuterNormal() * cellData.fluxData().velocity(wPhaseIdx, isIndex));
                fluxNW[isIndex] += isIt->geometry().volume()
                        * (isIt->centerUnitOuterNormal() * cellData.fluxData().velocity(nPhaseIdx, isIndex));
            }

            Dune::FieldVector < Scalar, dim > refVelocity(0);
                refVelocity[0] = 0.5 * (fluxW[1] - fluxW[0]);
            if (dim > 1)
                refVelocity[1] = 0.5 * (fluxW[3] - fluxW[2]);
            if (dim == 3)
                refVelocity[2] = 0.5 * (fluxW[5] - fluxW[4]);

            const Dune::FieldVector<Scalar, dim>& localPos =
                    ReferenceElementContainer::general(eIt->geometry().type()).position(0, 0);

            // get the transposed Jacobian of the element mapping
            const DimMatrix& jacobianInv = eIt->geometry().jacobianInverseTransposed(localPos);
            DimMatrix jacobianT(jacobianInv);
            jacobianT.invert();

            // calculate the element velocity by the Piola transformation
            Dune::FieldVector < Scalar, dim > elementVelocity(0);
            jacobianT.umtv(refVelocity, elementVelocity);
            elementVelocity /= eIt->geometry().integrationElement(localPos);

            velocity[globalIdx] = elementVelocity;

            refVelocity = 0;
            refVelocity[0] = 0.5 * (fluxNW[1] - fluxNW[0]);
            if (dim > 1)
            refVelocity[1] = 0.5 * (fluxNW[3] - fluxNW[2]);
            if (dim == 3)
                refVelocity[2] = 0.5 * (fluxNW[5] - fluxNW[4]);

            // calculate the element velocity by the Piola transformation
            elementVelocity = 0;
            jacobianT.umtv(refVelocity, elementVelocity);
            elementVelocity /= eIt->geometry().integrationElement(localPos);

            velocitySecondPhase[globalIdx] = elementVelocity;
        }

        //switch velocities
        switch (velocityType_)
        {
        case vw:
        {
            writer.attachCellData(velocity, "wetting-velocity", dim);
            writer.attachCellData(velocitySecondPhase, "non-wetting-velocity", dim);
            break;
        }
        case vn:
        {
            writer.attachCellData(velocity, "non-wetting-velocity", dim);
            writer.attachCellData(velocitySecondPhase, "wetting-velocity", dim);
            break;
        }
        case vt:
        {
            writer.attachCellData(velocity, "total velocity", dim);
            break;
        }
        }

        return;
    }

private:
    Problem& problem_;
    const GlobalPosition& gravity_; //!< vector including the gravity constant
    Scalar density_[numPhases];
    Scalar viscosity_[numPhases];

    static const int velocityType_ = GET_PROP_VALUE(TypeTag, VelocityFormulation); //!< gives kind of velocity used (\f$ 0 = v_w\f$, \f$ 1 = v_n\f$, \f$ 2 = v_t\f$)
    static const bool compressibility_ = GET_PROP_VALUE(TypeTag, EnableCompressibility);
    static const int pressureType_ = GET_PROP_VALUE(TypeTag, PressureFormulation); //!< gives kind of pressure used (\f$p_w\f$, \f$p_n\f$, \f$p_{global}\f$)
    static const int saturationType_ = GET_PROP_VALUE(TypeTag, SaturationFormulation); //!< gives kind of saturation used (\f$S_w\f$, \f$S_n\f$)
};

/*! \brief Calculates the velocity at a cell-cell interface.
*
* Calculates the velocity at a cell-cell interface from a given pressure field.
*
* \param intersection Intersection of two grid cells
* \param cellData Object containing all model relevant cell data
*/
template<class TypeTag>
void FVVelocity2P<TypeTag>::calculateVelocity(const Intersection& intersection, CellData& cellData)
{
    ElementPointer elementI = intersection.inside();
    ElementPointer elementJ = intersection.outside();

    int globalIdxJ = problem_.variables().index(*elementJ);

    CellData& cellDataJ = problem_.variables().cellData(globalIdxJ);

    // get global coordinates of cell centers
    const GlobalPosition& globalPosI = elementI->geometry().center();
    const GlobalPosition& globalPosJ = elementJ->geometry().center();

    // get mobilities and fractional flow factors
    Scalar lambdaWI = cellData.mobility(wPhaseIdx);
    Scalar lambdaNWI = cellData.mobility(nPhaseIdx);
    Scalar fractionalWI = cellData.fracFlowFunc(wPhaseIdx);
    Scalar fractionalNWI = cellData.fracFlowFunc(nPhaseIdx);
    Scalar lambdaWJ = cellDataJ.mobility(wPhaseIdx);
    Scalar lambdaNWJ = cellDataJ.mobility(nPhaseIdx);
    Scalar fractionalWJ = cellDataJ.fracFlowFunc(wPhaseIdx);
    Scalar fractionalNWJ = cellDataJ.fracFlowFunc(nPhaseIdx);

    // get capillary pressure
    Scalar pcI = cellData.capillaryPressure();
    Scalar pcJ = cellDataJ.capillaryPressure();

    //get face index
    int isIndexI = intersection.indexInInside();
    int isIndexJ = intersection.indexInOutside();

    //get face normal
    const Dune::FieldVector<Scalar, dim>& unitOuterNormal = intersection.centerUnitOuterNormal();

    // distance vector between barycenters
    GlobalPosition distVec = globalPosJ - globalPosI;

    // compute distance between cell centers
    Scalar dist = distVec.two_norm();

    // compute vectorized permeabilities
    DimMatrix meanPermeability(0);

    problem_.spatialParams().meanK(meanPermeability, problem_.spatialParams().intrinsicPermeability(*elementI),
            problem_.spatialParams().intrinsicPermeability(*elementJ));

    Dune::FieldVector < Scalar, dim > permeability(0);
    meanPermeability.mv(unitOuterNormal, permeability);

    //calculate potential gradients
    Scalar potentialW = 0;
    Scalar potentialNW = 0;

    potentialW = cellData.fluxData().potential(wPhaseIdx, isIndexI);
    potentialNW = cellData.fluxData().potential(nPhaseIdx, isIndexI);

    if (compressibility_)
    {
        density_[wPhaseIdx] = (potentialW > 0.) ? cellData.density(wPhaseIdx) : cellDataJ.density(wPhaseIdx);
        density_[nPhaseIdx] = (potentialNW > 0.) ? cellData.density(nPhaseIdx) : cellDataJ.density(nPhaseIdx);

        density_[wPhaseIdx] =
                (potentialW == 0) ? 0.5 * (cellData.density(wPhaseIdx) + cellDataJ.density(wPhaseIdx)) :
                        density_[wPhaseIdx];
        density_[nPhaseIdx] =
                (potentialNW == 0) ? 0.5 * (cellData.density(nPhaseIdx) + cellDataJ.density(nPhaseIdx)) :
                        density_[nPhaseIdx];
    }

    if (pressureType_ == pglobal)
    {
        potentialW = (cellData.globalPressure() - cellDataJ.globalPressure()
                - 0.5 * (fractionalNWI + fractionalNWJ) * (pcI - pcJ));
        potentialNW = (cellData.globalPressure() - cellDataJ.globalPressure()
                + 0.5 * (fractionalWI + fractionalWJ) * (pcI - pcJ));
    }
    else
    {
        potentialW = (cellData.pressure(wPhaseIdx) - cellDataJ.pressure(wPhaseIdx));
        potentialNW = (cellData.pressure(nPhaseIdx) - cellDataJ.pressure(nPhaseIdx));
    }

    potentialW += density_[wPhaseIdx] * (distVec * gravity_); //delta z/delta x in unitOuterNormal[z]
    potentialNW += density_[nPhaseIdx] * (distVec * gravity_);

    //store potentials for further calculations (velocity, saturation, ...)
    cellData.fluxData().setPotential(wPhaseIdx, isIndexI, potentialW);
    cellData.fluxData().setPotential(nPhaseIdx, isIndexI, potentialNW);

    cellDataJ.fluxData().setPotential(wPhaseIdx, isIndexJ, -potentialW);
    cellDataJ.fluxData().setPotential(nPhaseIdx, isIndexJ, -potentialNW);

    //do the upwinding of the mobility depending on the phase potentials
    Scalar lambdaW = (potentialW > 0.) ? lambdaWI : lambdaWJ;
    lambdaW = (potentialW == 0) ? 0.5 * (lambdaWI + lambdaWJ) : lambdaW;
    Scalar lambdaNW = (potentialNW > 0.) ? lambdaNWI : lambdaNWJ;
    lambdaNW = (potentialNW == 0) ? 0.5 * (lambdaNWI + lambdaNWJ) : lambdaNW;

    if (compressibility_)
    {
        density_[wPhaseIdx] = (potentialW > 0.) ? cellData.density(wPhaseIdx) : cellDataJ.density(wPhaseIdx);
        density_[nPhaseIdx] = (potentialNW > 0.) ? cellData.density(nPhaseIdx) : cellDataJ.density(nPhaseIdx);

        density_[wPhaseIdx] =
                (potentialW == 0) ? 0.5 * (cellData.density(wPhaseIdx) + cellDataJ.density(wPhaseIdx)) :
                        density_[wPhaseIdx];
        density_[nPhaseIdx] =
                (potentialNW == 0) ? 0.5 * (cellData.density(nPhaseIdx) + cellDataJ.density(nPhaseIdx)) :
                        density_[nPhaseIdx];
    }

    //calculate the gravity term
    Dune::FieldVector < Scalar, dimWorld > velocityW(permeability);
    Dune::FieldVector < Scalar, dimWorld > velocityNW(permeability);
    Dune::FieldVector < Scalar, dimWorld > gravityTermW(unitOuterNormal);
    Dune::FieldVector < Scalar, dimWorld > gravityTermNW(unitOuterNormal);

    gravityTermW *= (gravity_ * permeability) * (lambdaW * density_[wPhaseIdx]);
    gravityTermNW *= (gravity_ * permeability) * (lambdaNW * density_[nPhaseIdx]);

    //calculate velocity depending on the pressure used -> use pc = pn - pw
    switch (pressureType_)
    {
    case pw:
    {
        velocityW *= lambdaW * (cellData.pressure(wPhaseIdx) - cellDataJ.pressure(wPhaseIdx)) / dist;
        velocityNW *= lambdaNW * (cellData.pressure(wPhaseIdx) - cellDataJ.pressure(wPhaseIdx)) / dist
                + 0.5 * (lambdaNWI + lambdaNWJ) * (pcI - pcJ) / dist;
        velocityW += gravityTermW;
        velocityNW += gravityTermNW;
        break;
    }
    case pn:
    {
        velocityW *= lambdaW * (cellData.pressure(nPhaseIdx) - cellDataJ.pressure(nPhaseIdx)) / dist
                - 0.5 * (lambdaWI + lambdaWJ) * (pcI - pcJ) / dist;
        velocityNW *= lambdaNW * (cellData.pressure(nPhaseIdx) - cellDataJ.pressure(nPhaseIdx)) / dist;
        velocityW += gravityTermW;
        velocityNW += gravityTermNW;
        break;
    }
    case pglobal:
    {
        velocityW *= (lambdaW + lambdaNW) * (cellData.globalPressure() - cellDataJ.globalPressure()) / dist;
        velocityW += gravityTermW;
        velocityW += gravityTermNW;
        velocityNW = 0;
        break;
    }
    }

    //store velocities
    cellData.fluxData().setVelocity(wPhaseIdx, isIndexI, velocityW);
    cellData.fluxData().setVelocity(nPhaseIdx, isIndexI, velocityNW);
    cellData.fluxData().setVelocityMarker(isIndexI);

    cellDataJ.fluxData().setVelocity(wPhaseIdx, isIndexJ, velocityW);
    cellDataJ.fluxData().setVelocity(nPhaseIdx, isIndexJ, velocityNW);
    cellDataJ.fluxData().setVelocityMarker(isIndexJ);

//                        printvector(std::cout, cellData.fluxData().velocity(), "velocity", "row", 4, 1, 3);
    return;
}

/*! \brief Calculates the velocity at a boundary.
*
* Calculates the velocity at a boundary from a given pressure field.
*
* \param intersection Boundary intersection
* \param cellData Object containing all model relevant cell data
*/
template<class TypeTag>
void FVVelocity2P<TypeTag>::calculateVelocityOnBoundary(const Intersection& intersection, CellData& cellData)
{
    ElementPointer element = intersection.inside();

    //get face index
    int isIndex = intersection.indexInInside();

    //get face normal
    const Dune::FieldVector<Scalar, dim>& unitOuterNormal = intersection.centerUnitOuterNormal();

    BoundaryTypes bcType;
    //get boundary type
    problem_.boundaryTypes(bcType, intersection);
    PrimaryVariables boundValues(0.0);

    if (bcType.isDirichlet(eqIdxPress))
    {
        problem_.dirichlet(boundValues, intersection);

        // get global coordinates of cell centers
        const GlobalPosition& globalPosI = element->geometry().center();

        // center of face in global coordinates
        const GlobalPosition& globalPosJ = intersection.geometry().center();

        // get mobilities and fractional flow factors
        Scalar lambdaWI = cellData.mobility(wPhaseIdx);
        Scalar lambdaNWI = cellData.mobility(nPhaseIdx);
        Scalar fractionalWI = cellData.fracFlowFunc(wPhaseIdx);
        Scalar fractionalNWI = cellData.fracFlowFunc(nPhaseIdx);

        // get capillary pressure
        Scalar pcI = cellData.capillaryPressure();

        // distance vector between barycenters
        GlobalPosition distVec = globalPosJ - globalPosI;

        // compute distance between cell centers
        Scalar dist = distVec.two_norm();

        //permeability vector at boundary
        // compute vectorized permeabilities
        DimMatrix meanPermeability(0);

        problem_.spatialParams().meanK(meanPermeability, problem_.spatialParams().intrinsicPermeability(*element));

        Dune::FieldVector < Scalar, dim > permeability(0);
        meanPermeability.mv(unitOuterNormal, permeability);

        //determine saturation at the boundary -> if no saturation is known directly at the boundary use the cell saturation
        Scalar satW = 0;
        Scalar satNW = 0;
        if (bcType.isDirichlet(eqIdxSat))
        {
            switch (saturationType_)
            {
            case Sw:
            {
                satW = boundValues[saturationIdx];
                satNW = 1 - boundValues[saturationIdx];
                break;
            }
            case Sn:
            {
                satW = 1 - boundValues[saturationIdx];
                satNW = boundValues[saturationIdx];
                break;
            }
            }
        }
        else
        {
            satW = cellData.saturation(wPhaseIdx);
            satNW = cellData.saturation(nPhaseIdx);
        }

        Scalar pressBound = boundValues[pressureIdx];
        Scalar pcBound = MaterialLaw::pC(problem_.spatialParams().materialLawParams(*element), satW);

        //determine phase pressures from primary pressure variable
        Scalar pressWBound = 0;
        Scalar pressNWBound = 0;
        if (pressureType_ == pw)
        {
            pressWBound = pressBound;
            pressNWBound = pressBound + pcBound;
        }
        else if (pressureType_ == pn)
        {
            pressWBound = pressBound - pcBound;
            pressNWBound = pressBound;
        }

        //get temperature at current position
        Scalar temperature = problem_.temperature(*element);

        Scalar densityWBound = density_[wPhaseIdx];
        Scalar densityNWBound = density_[nPhaseIdx];
        Scalar viscosityWBound = viscosity_[wPhaseIdx];
        Scalar viscosityNWBound = viscosity_[nPhaseIdx];

        if (compressibility_)
        {
            FluidState fluidState;
            fluidState.setSaturation(wPhaseIdx, satW);
            fluidState.setSaturation(nPhaseIdx, satNW);
            fluidState.setTemperature(temperature);
            fluidState.setPressure(wPhaseIdx, pressWBound);
            fluidState.setPressure(nPhaseIdx, pressNWBound);

            densityWBound = FluidSystem::density(fluidState, wPhaseIdx);
            densityNWBound = FluidSystem::density(fluidState, nPhaseIdx);
            viscosityWBound = FluidSystem::viscosity(fluidState, wPhaseIdx) / densityWBound;
            viscosityNWBound = FluidSystem::viscosity(fluidState, nPhaseIdx) / densityNWBound;
        }

        Scalar lambdaWBound = MaterialLaw::krw(problem_.spatialParams().materialLawParams(*element), satW)
                / viscosityWBound;
        Scalar lambdaNWBound = MaterialLaw::krn(problem_.spatialParams().materialLawParams(*element), satW)
                / viscosityNWBound;

        Scalar potentialW = 0;
        Scalar potentialNW = 0;

        potentialW = cellData.fluxData().potential(wPhaseIdx, isIndex);
        potentialNW = cellData.fluxData().potential(nPhaseIdx, isIndex);

        if (compressibility_)
        {
            density_[wPhaseIdx] = (potentialW > 0.) ? cellData.density(wPhaseIdx) : densityWBound;
            density_[wPhaseIdx] = (potentialW == 0) ? 0.5 * (cellData.density(wPhaseIdx) + densityWBound) : density_[wPhaseIdx];
            density_[nPhaseIdx] = (potentialNW > 0.) ? cellData.density(nPhaseIdx) : densityNWBound;
            density_[nPhaseIdx] = (potentialNW == 0) ? 0.5 * (cellData.density(nPhaseIdx) + densityNWBound) : density_[nPhaseIdx];
        }

        //calculate potential gradient
        if (pressureType_ == pglobal)
        {
            potentialW = (cellData.globalPressure() - pressBound - fractionalNWI * (pcI - pcBound));
            potentialNW = (cellData.globalPressure() - pressBound + fractionalWI * (pcI - pcBound));
        }
        else
        {
            potentialW = (cellData.pressure(wPhaseIdx) - pressWBound);
            potentialNW = (cellData.pressure(nPhaseIdx) - pressNWBound);
        }

        potentialW += density_[wPhaseIdx] * (distVec * gravity_);
        potentialNW += density_[nPhaseIdx] * (distVec * gravity_);

        //store potential gradients for further calculations
        cellData.fluxData().setPotential(wPhaseIdx, isIndex, potentialW);
        cellData.fluxData().setPotential(nPhaseIdx, isIndex, potentialNW);

        //do the upwinding of the mobility depending on the phase potentials
        Scalar lambdaW = (potentialW > 0.) ? lambdaWI : lambdaWBound;
        lambdaW = (potentialW == 0) ? 0.5 * (lambdaWI + lambdaWBound) : lambdaW;
        Scalar lambdaNW = (potentialNW > 0.) ? lambdaNWI : lambdaNWBound;
        lambdaNW = (potentialNW == 0) ? 0.5 * (lambdaNWI + lambdaNWBound) : lambdaNW;

        if (compressibility_)
        {
            density_[wPhaseIdx] = (potentialW > 0.) ? cellData.density(wPhaseIdx) : densityWBound;
            density_[wPhaseIdx] = (potentialW == 0) ? 0.5 * (cellData.density(wPhaseIdx) + densityWBound) : density_[wPhaseIdx];
            density_[nPhaseIdx] = (potentialNW > 0.) ? cellData.density(nPhaseIdx) : densityNWBound;
            density_[nPhaseIdx] = (potentialNW == 0) ? 0.5 * (cellData.density(nPhaseIdx) + densityNWBound) : density_[nPhaseIdx];
        }

        //calculate the gravity term
        Dune::FieldVector < Scalar, dimWorld > velocityW(permeability);
        Dune::FieldVector < Scalar, dimWorld > velocityNW(permeability);
        Dune::FieldVector < Scalar, dimWorld > gravityTermW(unitOuterNormal);
        Dune::FieldVector < Scalar, dimWorld > gravityTermNW(unitOuterNormal);

        gravityTermW *= (gravity_ * permeability) * (lambdaW * density_[wPhaseIdx]);
        gravityTermNW *= (gravity_ * permeability) * (lambdaNW * density_[nPhaseIdx]);

        //calculate velocity depending on the pressure used -> use pc = pn - pw
        switch (pressureType_)
        {
        case pw:
        {
            velocityW *= lambdaW * (cellData.pressure(wPhaseIdx) - pressBound) / dist;
            velocityNW *= lambdaNW * (cellData.pressure(wPhaseIdx) - pressBound) / dist
                    + 0.5 * (lambdaNWI + lambdaNWBound) * (pcI - pcBound) / dist;
            velocityW += gravityTermW;
            velocityNW += gravityTermNW;
            break;
        }
        case pn:
        {
            velocityW *= lambdaW * (cellData.pressure(nPhaseIdx) - pressBound) / dist
                    - 0.5 * (lambdaWI + lambdaWBound) * (pcI - pcBound) / dist;
            velocityNW *= lambdaNW * (cellData.pressure(nPhaseIdx) - pressBound) / dist;
            velocityW += gravityTermW;
            velocityNW += gravityTermNW;
            break;
        }
        case pglobal:
        {
            velocityW *= (lambdaW + lambdaNW) * (cellData.globalPressure() - pressBound) / dist;
            velocityW += gravityTermW;
            velocityW += gravityTermNW;
            velocityNW = 0;
            break;
        }
        }

        //store velocities
        cellData.fluxData().setVelocity(wPhaseIdx, isIndex, velocityW);
        cellData.fluxData().setVelocity(nPhaseIdx, isIndex, velocityNW);
        cellData.fluxData().setVelocityMarker(isIndex);

    } //end dirichlet boundary

    else if (bcType.isNeumann(eqIdxPress))
    {
        problem_.neumann(boundValues, intersection);

        Dune::FieldVector < Scalar, dimWorld > velocityW(unitOuterNormal);
        Dune::FieldVector < Scalar, dimWorld > velocityNW(unitOuterNormal);

        velocityW *= boundValues[wPhaseIdx];
        velocityNW *= boundValues[nPhaseIdx];

        if (!compressibility_)
        {
            velocityW /= density_[wPhaseIdx];
            velocityNW /= density_[nPhaseIdx];
        }

        //store potential gradients for further calculations
        cellData.fluxData().setPotential(wPhaseIdx, isIndex, boundValues[wPhaseIdx]);
        cellData.fluxData().setPotential(nPhaseIdx, isIndex, boundValues[nPhaseIdx]);

        cellData.fluxData().setVelocity(wPhaseIdx, isIndex, velocityW);
        cellData.fluxData().setVelocity(nPhaseIdx, isIndex, velocityNW);
        cellData.fluxData().setVelocityMarker(isIndex);
    } //end neumann boundary
    else
    {
        DUNE_THROW(Dune::NotImplemented, "No valid boundary condition type defined for pressure equation!");
    }

//                        printvector(std::cout, cellData.fluxData().velocity(), "velocity", "row", 4, 1, 3);
    return;
}
}
#endif
