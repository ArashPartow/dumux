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
 * \brief This file contains the data which is required to calculate
 *        all fluxes of fluid phases over a face of a finite volume.
 *
 * This means pressure and temperature gradients, phase densities at
 * the integration point, etc.
 */
#ifndef DUMUX_2P_FLUX_VARIABLES_HH
#define DUMUX_2P_FLUX_VARIABLES_HH

#warning This file is deprecated. Use dumux/boxmodels/common/boxdarcyfluxvariables.hh instead. 

#include "2pproperties.hh"

#include <dumux/boxmodels/common/boxdarcyfluxvariables.hh>
#include <dumux/common/parameters.hh>
#include <dumux/common/math.hh>

namespace Dumux
{

/*!
 * \ingroup TwoPBoxModel
 * \ingroup BoxFluxVariables
 * \brief This template class contains the data which is required to
 *        calculate the fluxes of the fluid phases over a face of a
 *        finite volume for the two-phase model.
 *
 * This means pressure and concentration gradients, phase densities at
 * the intergration point, etc.
 */
template <class TypeTag>
class TwoPFluxVariables : public BoxDarcyFluxVariables<TypeTag>
{
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, SpatialParams) SpatialParams;
    typedef typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables) ElementVolumeVariables;

    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GridView::template Codim<0>::Entity Element;
    enum {
        dim = GridView::dimension,
        dimWorld = GridView::dimensionworld,
        numPhases = GET_PROP_VALUE(TypeTag, NumPhases)
    };

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef Dune::FieldMatrix<Scalar, dimWorld, dimWorld> Tensor;
    typedef Dune::FieldVector<Scalar, dimWorld> Vector;

    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename FVElementGeometry::SubControlVolumeFace SCVFace;

public:
    /*
     * \brief The constructor
     *
     * \param problem The problem
     * \param element The finite element
     * \param fvGeometry The finite-volume geometry in the box scheme
     * \param faceIdx The local index of the SCV (sub-control-volume) face
     * \param elemDat The volume variables of the current element
     * \param onBoundary A boolean variable to specify whether the flux variables
     * are calculated for interior SCV faces or boundary faces, default=false
     */
    TwoPFluxVariables(const Problem &problem,
                 const Element &element,
                 const FVElementGeometry &fvGeometry,
                 int faceIdx,
                 const ElementVolumeVariables &elemVolVars,
                 const bool onBoundary = false)
        : BoxDarcyFluxVariables<TypeTag>(problem, element, fvGeometry, faceIdx, elemVolVars, onBoundary), 
          fvGeometry_(fvGeometry), onBoundary_(onBoundary)
    {
        faceIdx_ = faceIdx;

        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            potentialGrad_[phaseIdx] = Scalar(0);
        }

        calculateGradients_(problem, element, elemVolVars);
        calculateK_(problem, element, elemVolVars);
    };

public:
    /*
     * \brief Return the intrinsic permeability.
     */
    const Tensor &intrinsicPermeability() const
    { return K_; }

    /*!
     * \brief Return the pressure potential gradient.
     *
     * \param phaseIdx The index of the fluid phase
     */
    const Vector &potentialGrad(int phaseIdx) const
    { return potentialGrad_[phaseIdx]; }

    /*!
     * \brief Return the local index of the downstream control volume
     *        for a given phase as a function of the normal flux.
     *
     * \param normalFlux The normal flux i.e. the given intrinsic permeability
     *                   times the pressure potential gradient and SCV face normal.
     */
    int downstreamIdx(Scalar normalFlux) const
    { return (normalFlux >= 0)?face().j:face().i; }

    /*!
     * \brief Return the local index of the upstream control volume
     *        for a given phase as a function of the normal flux.
     *
     * \param normalFlux The normal flux i.e. the given intrinsic permeability
     *                   times the pressure potential gradient and SCV face normal.
     */
    int upstreamIdx(Scalar normalFlux) const
    { return (normalFlux > 0)?face().i:face().j; }

    /*!
     * \brief Return the SCV (sub-control-volume) face
    */
    const SCVFace &face() const
    {
        if (onBoundary_)
            return fvGeometry_.boundaryFace[faceIdx_];
        else
            return fvGeometry_.subContVolFace[faceIdx_];
    }

protected:
    const FVElementGeometry &fvGeometry_;
    int faceIdx_;
    const bool onBoundary_;

    // gradients
    Vector potentialGrad_[numPhases];

    // intrinsic permeability
    Tensor K_;

private:
    void calculateGradients_(const Problem &problem,
                             const Element &element,
                             const ElementVolumeVariables &elemVolVars)
    {
        // calculate gradients
        for (int idx = 0;
             idx < fvGeometry_.numFAP;
             idx++) // loop over adjacent vertices
        {
            // FE gradient at vertex idx
            const Vector &feGrad = face().grad[idx];

            // index for the element volume variables 
            int volVarsIdx = face().fapIndices[idx];
	    
            // compute sum of pressure gradients for each phase
            for (int phaseIdx = 0; phaseIdx < numPhases; phaseIdx++)
            {
                // the pressure gradient
                Vector tmp(feGrad);
                tmp *= elemVolVars[volVarsIdx].pressure(phaseIdx);
                potentialGrad_[phaseIdx] += tmp;
            }
        }

        ///////////////
        // correct the pressure gradients by the gravitational acceleration
        ///////////////
        if (GET_PARAM_FROM_GROUP(TypeTag, bool, Problem, EnableGravity))
        {
            // estimate the gravitational acceleration at a given SCV face
            // using the arithmetic mean
            Vector g(problem.boxGravity(element, fvGeometry_, face().i));
            g += problem.boxGravity(element, fvGeometry_, face().j);
            g /= 2;

            for (int phaseIdx = 0; phaseIdx < numPhases; phaseIdx++)
            {
                // calculate the phase density at the integration point. we
                // only do this if the wetting phase is present in both cells
                Scalar SI = elemVolVars[face().i].saturation(phaseIdx);
                Scalar SJ = elemVolVars[face().j].saturation(phaseIdx);
                Scalar rhoI = elemVolVars[face().i].density(phaseIdx);
                Scalar rhoJ = elemVolVars[face().j].density(phaseIdx);
                Scalar fI = std::max(0.0, std::min(SI/1e-5, 0.5));
                Scalar fJ = std::max(0.0, std::min(SJ/1e-5, 0.5));
                if (fI + fJ == 0)
                    // doesn't matter because no wetting phase is present in
                    // both cells!
                    fI = fJ = 0.5;
                Scalar density = (fI*rhoI + fJ*rhoJ)/(fI + fJ);

                // make gravity acceleration a force
                Vector f(g);
                f *= density;

                // calculate the final potential gradient
                potentialGrad_[phaseIdx] -= f;
            }
        }
    }

    void calculateK_(const Problem &problem,
                     const Element &element,
                     const ElementVolumeVariables &elemVolVars)
    {
        const SpatialParams &spatialParams = problem.spatialParams();
        // calculate the intrinsic permeability
        spatialParams.meanK(K_,
                            spatialParams.intrinsicPermeability(element,
                                                                fvGeometry_,
                                                                face().i),
                            spatialParams.intrinsicPermeability(element,
                                                                fvGeometry_,
                                                                face().j));
    }
};

} // end namepace

#endif
