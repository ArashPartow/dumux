// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2008,2009 by Andreas Lauser                               *
 *   Copyright (C) 2008,2009 by Melanie Darcis                               *
 *                                                                           *
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
 *
 * \brief Contains the quantities to calculate the energy flux in the
 *        MpNc box model.
 */
#ifndef DUMUX_MPNC_ENERGY_FLUX_VARIABLES_HH
#define DUMUX_MPNC_ENERGY_FLUX_VARIABLES_HH

#include <dune/common/fmatrix.hh>
#include <dune/common/fvector.hh>

#include <dumux/boxmodels/mpnc/mpncproperties.hh>
#include <dumux/common/spline.hh>

namespace Dumux
{

template <class TypeTag, bool enableEnergy/*=false*/, bool kineticEnergyTransfer/*=false*/>
class MPNCFluxVariablesEnergy
{
    static_assert(!(kineticEnergyTransfer && !enableEnergy),
                  "No kinetic energy transfer may only be enabled "
                  "if energy is enabled in general.");
    static_assert(!kineticEnergyTransfer,
                  "No kinetic energy transfer module included, "
                  "but kinetic energy transfer enabled.");

    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, VolumeVariables) VolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables) ElementVolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, FluxVariables) FluxVariables;

public:
    MPNCFluxVariablesEnergy()
    {
    }

    void update(const Problem &problem,
                const Element &element,
                const FVElementGeometry &fvGeometry,
                const unsigned int faceIdx,
                const FluxVariables &fluxVars,
                const ElementVolumeVariables &elemVolVars)
    {};
};

template <class TypeTag>
class MPNCFluxVariablesEnergy<TypeTag, /*enableEnergy=*/true,  /*kineticEnergyTransfer=*/false>
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, VolumeVariables) VolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables) ElementVolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, FluxVariables) FluxVariables;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, FluidState) FluidState;
    typedef typename FluidSystem::ParameterCache ParameterCache;
    typedef typename GridView::ctype CoordScalar;
    typedef typename GridView::template Codim<0>::Entity Element;

    enum{dim = GridView::dimension};
    enum{dimWorld = GridView::dimensionworld};
    enum{nPhaseIdx = FluidSystem::nPhaseIdx};
    enum{wPhaseIdx = FluidSystem::wPhaseIdx};
    enum{numPhases = GET_PROP_VALUE(TypeTag, NumPhases)};

    typedef Dune::FieldVector<CoordScalar, dimWorld>  DimVector;
    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;

public:
    MPNCFluxVariablesEnergy()
    {}

    void update(const Problem & problem,
                const Element &element,
                const FVElementGeometry &fvGeometry,
                const unsigned int faceIdx,
                const FluxVariables &fluxVars,
                const ElementVolumeVariables &elemVolVars)
    {
        // calculate temperature gradient using finite element
        // gradients
        DimVector tmp(0.0);
        DimVector temperatureGradient(0.);
        for (int scvIdx = 0; scvIdx < fvGeometry.numVertices; scvIdx++)
        {
            tmp = this->face().grad[scvIdx];
            tmp *= elemVolVars[scvIdx].fluidState().temperature(/*phaseIdx=*/0);
            temperatureGradient += tmp;
        }

        // project the heat flux vector on the face's normal vector
        temperatureGradientNormal_ = temperatureGradient * this->face().normal;


        lambdaPm_ = lumpedLambdaPm(problem,
                                   element,
                                   fvGeometry,
                                   faceIdx,
                                   elemVolVars) ;

    }

    Scalar lumpedLambdaPm(const Problem &problem,
                          const Element &element,
                          const FVElementGeometry & fvGeometry,
                          const unsigned int faceIdx,
                          const ElementVolumeVariables & elemVolVars)
    {
         // arithmetic mean of the liquid saturation and the porosity
         const unsigned int i = this->face().i;
         const unsigned int j = this->face().j;

         const FluidState &fsI = elemVolVars[i].fluidState();
         const FluidState &fsJ = elemVolVars[j].fluidState();
         const Scalar Swi = fsI.saturation(wPhaseIdx);
         const Scalar Swj = fsJ.saturation(wPhaseIdx);

         typename FluidSystem::ParameterCache paramCacheI, paramCacheJ;
         paramCacheI.updateAll(fsI);
         paramCacheJ.updateAll(fsJ);

         const Scalar Sw = std::max<Scalar>(0.0, 0.5*(Swi + Swj));

         //        const Scalar lambdaDry = 0.583; // W / (K m) // works, orig
         //        const Scalar lambdaWet = 1.13; // W / (K m) // works, orig

         const Scalar lambdaSoilI = problem.spatialParams().soilThermalConductivity(element, fvGeometry, i);
         const Scalar lambdaSoilJ = problem.spatialParams().soilThermalConductivity(element, fvGeometry, i);
         const Scalar lambdaDry = 0.5 * (lambdaSoilI + FluidSystem::thermalConductivity(fsI, paramCacheI, nPhaseIdx)); // W / (K m)
         const Scalar lambdaWet = 0.5 * (lambdaSoilJ + FluidSystem::thermalConductivity(fsJ, paramCacheJ, wPhaseIdx)) ; // W / (K m)

         // the heat conductivity of the matrix. in general this is a
         // tensorial value, but we assume isotropic heat conductivity.
         // This is the Sommerton approach with lambdaDry =
         // lambdaSn100%.  Taken from: H. Class: "Theorie und
         // numerische Modellierung nichtisothermer Mehrphasenprozesse
         // in NAPL-kontaminierten poroesen Medien", PhD Thesis, University of
         // Stuttgart, Institute of Hydraulic Engineering, p. 57

         Scalar result;
         if (Sw < 0.1) {
             // regularization
             Dumux::Spline<Scalar> sp(0, 0.1, // x1, x2
                                      0, sqrt(0.1), // y1, y2
                                      5*0.5/sqrt(0.1), 0.5/sqrt(0.1)); // m1, m2
             result = lambdaDry + sp.eval(Sw)*(lambdaWet - lambdaDry);
         }
         else
             result = lambdaDry + std::sqrt(Sw)*(lambdaWet - lambdaDry);

         return result;
    }

    /*!
     * \brief The lumped / average conductivity of solid plus phases \f$[W/mK]\f$.
     */
    Scalar lambdaPm() const
    { return lambdaPm_; }

    /*!
     * \brief The normal of the gradient of temperature .
     */
    Scalar temperatureGradientNormal() const
    {
        return temperatureGradientNormal_;
    }

private:
    Scalar lambdaPm_;
    Scalar temperatureGradientNormal_;
};

} // end namepace

#endif
