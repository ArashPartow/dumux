// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2010 by Markus Wolff                                 *
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
#ifndef DUMUX_EVALCFLFLUX_COATS_HH
#define DUMUX_EVALCFLFLUX_COATS_HH

/**
 * @file
 * @brief  CFL-flux-function to evaluate a CFL-Condition after Coats 2003
 * @author Markus Wolff
 */

#include "evalcflfluxdefault.hh"

namespace Dumux
{
/*!\ingroup IMPES
 * @brief  CFL-flux-function to evaluate a CFL-Condition after Coats 2003
 *
 * tparam TypeTag The problem TypeTag
 */
template<class TypeTag>
class EvalCflFluxCoats: public EvalCflFluxDefault<TypeTag>
{
private:
    typedef EvalCflFluxDefault<TypeTag> ParentType;typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;

    typedef typename GET_PROP_TYPE(TypeTag, SpatialParams) SpatialParams;
    typedef typename SpatialParams::MaterialLaw MaterialLaw;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, FluidState) FluidState;
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;

    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;
    typedef typename GET_PROP(TypeTag, SolutionTypes) SolutionTypes;
    typedef typename SolutionTypes::PrimaryVariables PrimaryVariables;

    typedef typename GET_PROP_TYPE(TypeTag, CellData) CellData;

    enum
    {
        dim = GridView::dimension, dimWorld = GridView::dimensionworld
    };
    enum
    {
        wPhaseIdx = Indices::wPhaseIdx, nPhaseIdx = Indices::nPhaseIdx,
        eqIdxPress = Indices::pressEqIdx,
        eqIdxSat = Indices::satEqIdx
    };

    enum
    {
        pw = Indices::pressureW,
        pn = Indices::pressureNW,
        pglobal = Indices::pressureGlobal,
        vw = Indices::velocityW,
        vn = Indices::velocityNW,
        vt = Indices::velocityTotal,
        Sw = Indices::saturationW,
        Sn = Indices::saturationNW
    };

    typedef typename GridView::Traits::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<0>::EntityPointer ElementPointer;
    typedef typename GridView::Intersection Intersection;

    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;
    typedef Dune::FieldMatrix<Scalar, dim, dim> DimMatrix;

public:

    /*! \brief adds a flux to the cfl-criterion evaluation
     *
     * \copydetails EvalCflFlux::addFlux(Scalar&,Scalar&,Scalar&,Scalar&,Scalar,const Element&,int)
     */
    void addFlux(Scalar& lambdaW, Scalar& lambdaNW, Scalar& viscosityW, Scalar& viscosityNW, Scalar flux,
            const Element& element, int phaseIdx = -1)
    {
//        if (hasHangingNode_)
//        {
//            return;
//        }

        ParentType::addFlux(lambdaW, lambdaNW, viscosityW, viscosityNW, flux, element, phaseIdx);
    }

    /*! \brief adds a flux to the cfl-criterion evaluation
     *
     * \copydetails EvalCflFlux::addFlux(Scalar&,Scalar&,Scalar&,Scalar&,Scalar,const Intersection&,int)
     */
    void addFlux(Scalar& lambdaW, Scalar& lambdaNW, Scalar& viscosityW, Scalar& viscosityNW, Scalar flux,
            const Intersection& intersection, int phaseIdx = -1)
    {
        Scalar parentMob = 0.5;
        Scalar parentVis = 1;
        //        ParentType::addFlux(lambdaW, lambdaNW, viscosityW, viscosityNW, flux, intersection, phaseIdx);
        ParentType::addFlux(parentMob, parentMob, parentVis, parentVis, flux, intersection, phaseIdx);

        if (hasHangingNode_)
        {
            return;
        }

        ElementPointer element = intersection.inside();

        //coordinates of cell center
        const GlobalPosition& globalPos = element->geometry().center();

        // cell index
        int globalIdxI = problem_.variables().index(*element);

        CellData& cellDataI = problem_.variables().cellData(globalIdxI);

        int indexInInside = intersection.indexInInside();

        Scalar satI = cellDataI.saturation(wPhaseIdx);
        Scalar lambdaWI = cellDataI.mobility(wPhaseIdx);
        Scalar lambdaNWI = cellDataI.mobility(nPhaseIdx);

        Scalar dPcdSI = MaterialLaw::dpC_dSw(problem_.spatialParams().materialLawParams(*element), satI);


        const GlobalPosition& unitOuterNormal = intersection.centerUnitOuterNormal();

        if (intersection.neighbor())
        {
            ElementPointer neighbor = intersection.outside();

            const GlobalPosition& globalPosNeighbor = neighbor->geometry().center();

            // distance vector between barycenters
            Dune::FieldVector < Scalar, dimWorld > distVec = globalPosNeighbor - globalPos;

            // compute distance between cell centers
//            Scalar dist = distVec.two_norm();
            Scalar dist = std::abs(distVec*unitOuterNormal);

            int globalIdxJ = problem_.variables().index(*neighbor);

            CellData& cellDataJ = problem_.variables().cellData(globalIdxJ);

            //calculate potential gradients
            if (element.level() < neighbor.level())
            {
                hasHangingNode_ = true;
                return;
            }

            Scalar satJ = cellDataJ.saturation(wPhaseIdx);
            Scalar lambdaWJ = cellDataI.mobility(wPhaseIdx);
            Scalar lambdaNWJ = cellDataI.mobility(nPhaseIdx);

            Scalar dPcdSJ = MaterialLaw::dpC_dSw(problem_.spatialParams().materialLawParams(*neighbor), satJ);

            // compute vectorized permeabilities
            DimMatrix meanPermeability(0);

            problem_.spatialParams().meanK(meanPermeability,
                    problem_.spatialParams().intrinsicPermeability(*element),
                    problem_.spatialParams().intrinsicPermeability(*neighbor));

            Dune::FieldVector < Scalar, dim > permeability(0);
            meanPermeability.mv(unitOuterNormal, permeability);

            Scalar transmissibility = (unitOuterNormal * permeability) * intersection.geometry().volume() / dist;

            Scalar satUpw = 0;
            if (cellDataI.fluxData().isUpwindCell(wPhaseIdx, indexInInside))
            {
                satUpw = std::max(satI, 0.0);
            }
            else
            {
                satUpw = std::max(satJ, 0.0);
            }

            Scalar dS = eps_;

            Scalar satPlus = satUpw + eps_;
            Scalar satMinus = satUpw;
            if (satMinus - eps_ > 0.0)
            {
                satMinus -= eps_;
                dS += eps_;
            }

            Scalar dLambdaWdS = MaterialLaw::krw(problem_.spatialParams().materialLawParams(*neighbor), std::abs(satPlus)) / viscosityW;
            dLambdaWdS -= MaterialLaw::krw(problem_.spatialParams().materialLawParams(*neighbor), std::abs(satMinus)) / viscosityW;
            dLambdaWdS /= (dS);

            if (cellDataI.fluxData().isUpwindCell(nPhaseIdx, indexInInside))
            {
                satUpw = std::max(1 - satI, 0.0);
            }
            else
            {
                satUpw = std::max(1 - satJ, 0.0);
            }

            dS = eps_;

            satPlus = satUpw + eps_;
            satMinus = satUpw;
            if (satMinus - eps_ > 0.0)
            {
                satMinus -= eps_;
                dS += eps_;
            }

            Scalar dLambdaNWdS = MaterialLaw::krn(problem_.spatialParams().materialLawParams(*neighbor), satPlus) / viscosityNW;
            dLambdaNWdS -= MaterialLaw::krn(problem_.spatialParams().materialLawParams(*neighbor), satMinus) / viscosityNW;
            dLambdaNWdS /= (dS);

            Scalar lambdaWCap = 0.5 * (lambdaWI + lambdaWJ);
            Scalar lambdaNWCap = 0.5 * (lambdaNWI + lambdaNWJ);

            if (phaseIdx == wPhaseIdx)
            {
                if (flux != 0)
                {
                    cflFluxFunction_ += lambdaNW * dLambdaWdS * std::abs(flux) / (lambdaW * (lambdaW + lambdaNW));
                }

                cflFluxFunction_ -= transmissibility * lambdaWCap * lambdaNWCap * (dPcdSI + dPcdSJ) / (lambdaW
                        + lambdaNW);
            }
            else if (phaseIdx == nPhaseIdx)
            {
                if (flux != 0)
                {
                    cflFluxFunction_ -= lambdaW * dLambdaNWdS * std::abs(flux) / (lambdaNW * (lambdaW + lambdaNW));
                }
            }
            else
            {
                if (flux != 0)
                {
                    switch (saturationType_)
                    {
                    case Sw:
                    {
                    cflFluxFunction_ += dLambdaWdS / (dLambdaWdS + dLambdaNWdS) * std::abs(flux);
                    break;
                    }
                    case Sn:
                    {
                    cflFluxFunction_ +=  dLambdaNWdS / (dLambdaWdS + dLambdaNWdS) * std::abs(flux);
                    break;
                    }
                    }
                }
            }
        }
        else
        {
            // center of face in global coordinates
            GlobalPosition globalPosFace = intersection.geometry().center();

            //get boundary type
            BoundaryTypes bcType;
            problem_.boundaryTypes(bcType, intersection);

            // distance vector between barycenters
            Dune::FieldVector < Scalar, dimWorld > distVec = globalPosFace - globalPos;

            // compute distance between cell centers
            Scalar dist = distVec.two_norm();

            //permeability vector at boundary
            // compute vectorized permeabilities
            DimMatrix meanPermeability(0);

            problem_.spatialParams().meanK(meanPermeability,
                    problem_.spatialParams().intrinsicPermeability(*element));

            Dune::FieldVector<Scalar, dim> permeability(0);
            meanPermeability.mv(unitOuterNormal, permeability);

            Scalar satWBound =  cellDataI.saturation(wPhaseIdx);
            if (bcType.isDirichlet(eqIdxSat))
            {
                PrimaryVariables bcValues;
                problem_.dirichlet(bcValues, intersection);
                switch (saturationType_)
                {
                case Sw:
                {
                    satWBound = bcValues[eqIdxSat];
                    break;
                }
                case Sn:
                {
                    satWBound = 1 - bcValues[eqIdxSat];
                    break;
                }
                default:
                {
                    DUNE_THROW(Dune::RangeError, "saturation type not implemented");
                    break;
                }
                }

            }

            Scalar dPcdSBound = MaterialLaw::dpC_dSw(problem_.spatialParams().materialLawParams(*element), satWBound);

            Scalar lambdaWBound = 0;
            Scalar lambdaNWBound = 0;

            Scalar temperature = problem_.temperature(*element);
            Scalar referencePressure = problem_.referencePressure(*element);
            FluidState fluidState;
            fluidState.setPressure(wPhaseIdx, referencePressure);
            fluidState.setPressure(nPhaseIdx, referencePressure);
            fluidState.setTemperature(temperature);

            Scalar viscosityWBound = FluidSystem::viscosity(fluidState, wPhaseIdx);
            Scalar viscosityNWBound =
                    FluidSystem::viscosity(fluidState, nPhaseIdx);
            lambdaWBound = MaterialLaw::krw(problem_.spatialParams().materialLawParams(*element), satWBound) / viscosityWBound;
            lambdaNWBound = MaterialLaw::krn(problem_.spatialParams().materialLawParams(*element), satWBound) / viscosityNWBound;

            Scalar transmissibility = (unitOuterNormal * permeability) * intersection.geometry().volume() / dist;

            Scalar satUpw = 0;
            if (cellDataI.fluxData().isUpwindCell(wPhaseIdx, indexInInside))
            {
                satUpw = std::max(satI, 0.0);
            }
            else
            {
                satUpw = std::max(satWBound, 0.0);
            }

            Scalar dS = eps_;

            Scalar satPlus = satUpw + eps_;
            Scalar satMinus = satUpw;
            if (satMinus - eps_ > 0.0)
            {
                satMinus -= eps_;
                dS += eps_;
            }

            Scalar dLambdaWdS = MaterialLaw::krw(problem_.spatialParams().materialLawParams(*element), satPlus) / viscosityW;
            dLambdaWdS -= MaterialLaw::krw(problem_.spatialParams().materialLawParams(*element), satMinus) / viscosityW;
            dLambdaWdS /= (dS);

            if (cellDataI.fluxData().isUpwindCell(nPhaseIdx, indexInInside))
            {
                satUpw = std::max(1 - satI, 0.0);
            }
            else
            {
                satUpw = std::max(1 - satWBound, 0.0);
            }

            dS = eps_;

            satPlus = satUpw + eps_;
            satMinus = satUpw;
            if (satMinus - eps_ > 0.0)
            {
                satMinus -= eps_;
                dS += eps_;
            }

            Scalar dLambdaNWdS = MaterialLaw::krn(problem_.spatialParams().materialLawParams(*element), satPlus) / viscosityNW;
            dLambdaNWdS -= MaterialLaw::krn(problem_.spatialParams().materialLawParams(*element), satMinus) / viscosityNW;
            dLambdaNWdS /= (dS);

            Scalar lambdaWCap = 0.5 * (lambdaWI + lambdaWBound);
            Scalar lambdaNWCap = 0.5 * (lambdaNWI + lambdaNWBound);

            if (phaseIdx == wPhaseIdx)
            {
                if (flux != 0)
                {
                    cflFluxFunction_ += lambdaNW * dLambdaWdS * std::abs(flux) / (lambdaW * (lambdaW + lambdaNW));
                }

                cflFluxFunction_ -= transmissibility * lambdaWCap * lambdaNWCap * (dPcdSI + dPcdSBound) / (lambdaW
                        + lambdaNW);
            }
            else if (phaseIdx == nPhaseIdx)
            {
                if (flux != 0)
                {
                    cflFluxFunction_ -= lambdaW * dLambdaNWdS * std::abs(flux) / (lambdaNW * (lambdaW + lambdaNW));
                }
            }
            else
            {
                if (flux != 0)
                {
                    switch (saturationType_)
                    {
                    case Sw:
                    {
                    cflFluxFunction_ += dLambdaWdS / (dLambdaWdS + dLambdaNWdS) * std::abs(flux);
                    break;
                    }
                    case Sn:
                    {
                    cflFluxFunction_ +=  dLambdaNWdS / (dLambdaWdS + dLambdaNWdS) * std::abs(flux);
                    break;
                    }
                    }
                }
            }
        }
    }

    /*! \brief Returns the CFL flux-function
     *
     * \copydetails EvalCflFlux::getCFLFluxFunction(const Element&)
     */
    Scalar getCFLFluxFunction(const Element& element)
    {
        Scalar cflFluxDefault = 1 / ParentType::getCFLFluxFunction(element);

        if (std::isnan(cflFluxFunction_) || std::isinf(cflFluxFunction_) || cflFluxFunction_ > 100 * cflFluxDefault)
        {
            cflFluxFunction_ = 0;
        }

//        if (cflFluxDefault > 1)
//            std::cout << "cflFluxDefault = " << cflFluxDefault << "\n";
//
//        if (cflFluxFunction_ > 1)
//            std::cout << "cflFluxFunction_ = " << cflFluxFunction_ << "\n";

        Scalar returnValue = std::max(cflFluxFunction_, cflFluxDefault);
        reset();

        if (returnValue > 0 && !hasHangingNode_)
        {
            return (returnValue == cflFluxDefault) ? 0.95 / returnValue : 1 / returnValue;
        }
        else
        {
            return cflFluxDefault;
        }
    }

    /*! \brief  Returns the CFL time-step
     *
     * \copydetails EvalCflFlux::getDt(const Element&)
     */
    Scalar getDt(const Element& element)
    {
        Scalar porosity = problem_.spatialParams().porosity(element);
        if (porosity > 1e-6)
            return (getCFLFluxFunction(element) * problem_.spatialParams().porosity(element) * element.geometry().volume());
        else
            return (getCFLFluxFunction(element) * element.geometry().volume());
    }

    /*! \brief Constructs an EvalCflFluxDefault object
     *
     * \param problem A problem type object
     */
    EvalCflFluxCoats(Problem& problem) :
        ParentType(problem), problem_(problem), eps_(5e-3)
    {
        reset();
    }

protected:
    //! resets the accumulated CFL-fluxes to zero
    void reset()
    {
        ParentType::reset();
        cflFluxFunction_ = 0;
        hasHangingNode_ = false;
    }

private:


    Problem& problem_;//problem data
    Scalar cflFluxFunction_;
    bool hasHangingNode_;
    static const int pressureType_ = GET_PROP_VALUE(TypeTag, PressureFormulation);
    static const int velocityType_ = GET_PROP_VALUE(TypeTag, VelocityFormulation);
    static const int saturationType_ = GET_PROP_VALUE(TypeTag, SaturationFormulation);
    const Scalar eps_;
};

}

#endif
