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
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.    *
 *****************************************************************************/
/*!
 * \file
 *
 * \brief This file contains the data which is required to calculate
 *        all fluxes of fluid phases over a face of a finite volume in the
 *        two phase discrete fracture-matrix model.
 */
#ifndef DUMUX_BOXMODELS_2PDFM_FLUX_VARIABLES_HH
#define DUMUX_BOXMODELS_2PDFM_FLUX_VARIABLES_HH

#include <dumux/common/math.hh>
#include <dumux/common/parameters.hh>
#include <dumux/common/valgrind.hh>
#include <dumux/boxmodels/common/boxdarcyfluxvariables.hh>

#include "2pdfmproperties.hh"

namespace Dumux
{

/*!
 * \ingroup TwoPDFMBoxModel
 * \ingroup BoxDFMFluxVariables
 * \brief Contains the data which is required to calculate the fluxes of 
 *        the fluid phases over a face of a finite volume for the two-phase
 *        discrete fracture-matrix model.
 *
 * This means pressure and concentration gradients, phase densities at
 * the intergration point, etc.
 */
template <class TypeTag>
class TwoPDFMFluxVariables : public BoxDarcyFluxVariables<TypeTag>
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
    typedef Dune::FieldVector<Scalar, dimWorld> Vector;

    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename FVElementGeometry::SubControlVolumeFace SCVFace;
    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;
    typedef Dune::FieldVector<Scalar, dim> LocalPosition;
    typedef Dune::FieldVector<Scalar, numPhases> PhasesVector;

public:
    /*!
     * \brief The constructor
     *
     * \param problem The problem
     * \param element The finite element
     * \param fvGeometry The finite-volume geometry in the box scheme
     * \param faceIdx The local index of the SCV (sub-control-volume) face
     * \param elemVolVars The volume variables of the current element
     * \param onBoundary A boolean variable to specify whether the flux variables
     * are calculated for interior SCV faces or boundary faces, default=false
     */
    TwoPDFMFluxVariables(const Problem &problem,
                 const Element &element,
                 const FVElementGeometry &fvGeometry,
                 int faceIdx,
                 const ElementVolumeVariables &elemVolVars,
                 const bool onBoundary = false)
        : BoxDarcyFluxVariables<TypeTag>(problem, element, fvGeometry, faceIdx, elemVolVars, onBoundary), 
          fvGeometry_(fvGeometry), faceIdx_(faceIdx), onBoundary_(onBoundary)
    {
        faceSCV_ = &this->face();

        for (int phase = 0; phase < numPhases; ++phase)
        {
            potentialGradFracture_[phase] = 0.0;
        }

        calculateGradientsInFractures_(problem, element, elemVolVars, faceIdx);
        calculateVelocitiesFracture_(problem, element, elemVolVars, faceIdx);
    };

public:
    /*!
     * \brief Calculates the velocities in the lower-dimenstional fracture.
     * 
     * \param problem The problem
     * \param element The finite element
     * \param elemVolVars The volume variables of the current element
     * \param faceIdx The local index of the SCV (sub-control-volume) face
     */
    void calculateVelocitiesFracture_(const Problem &problem,
                                      const Element &element,
                                      const ElementVolumeVariables &elemVolVars,
                                      int faceIdx)
    {
        isFracture_ = problem.spatialParams().isEdgeFracture(element, faceIdx);
        fractureWidth_ = problem.spatialParams().fractureWidth(element, faceIdx);

        Scalar KFracture, KFi, KFj; //permeabilities of the fracture
        if (isFracture_)
        {
            KFi = problem.spatialParams().
                intrinsicPermeabilityFracture(element, this->fvGeometry_, this->face().i);
            KFj = problem.spatialParams().
                intrinsicPermeabilityFracture(element, this->fvGeometry_, this->face().j);
        }
        else
        {
            KFi = 0;
            KFj = 0;
        }

        KFracture = Dumux::harmonicMean(KFi, KFj);

        // temporary vector for the Darcy velocity
        Scalar vDarcyFracture = 0;
        for (int phase=0; phase < numPhases; phase++)
        {
            vDarcyFracture = - (KFracture * potentialGradFracture_[phase]);

            Valgrind::CheckDefined(KFracture);
            Valgrind::CheckDefined(fractureWidth_);
            vDarcyFracture_[phase] = (vDarcyFracture * fractureWidth_);
            Valgrind::CheckDefined(vDarcyFracture_[phase]);
        }

        // set the upstream and downstream vertices
        for (int phase = 0; phase < numPhases; ++phase)
        {
            upstreamFractureIdx[phase] = faceSCV_->i;
            downstreamFractureIdx[phase] = faceSCV_->j;

            if (vDarcyFracture_[phase] < 0)
            {
                std::swap(upstreamFractureIdx[phase],
                          downstreamFractureIdx[phase]);
            }
        }
    }

    /*!
     * \brief Return the pressure potential gradient in the lower dimensional fracture.
     *
     * \param phaseIdx The index of the fluid phase
     */
    const Scalar &potentialGradFracture(int phaseIdx) const
    {
        return potentialGradFracture_[phaseIdx];
    }


    PhasesVector vDarcyFracture_;

    int upstreamFractureIdx[numPhases];
    int downstreamFractureIdx[numPhases];
protected:
    // gradients
    Scalar potentialGradFracture_[numPhases];
    const FVElementGeometry &fvGeometry_;
    int faceIdx_;
    const bool onBoundary_;
    bool isFracture_;
    Scalar fractureWidth_;
    const SCVFace *faceSCV_;

private:
    /*!
     * \brief Calculates the gradients in the lower-dimenstional fracture.
     * 
     * \param problem The problem
     * \param element The finite element
     * \param elemVolVars The volume variables of the current element
     * \param faceIdx The local index of the SCV (sub-control-volume) face
     */
    void calculateGradientsInFractures_(const Problem &problem,
                                        const Element &element,
                                        const ElementVolumeVariables &elemVolVars,
                                        int faceIdx)
    {
        // calculate gradients, loop over adjacent vertices
        for (int idx = 0; idx < this->fvGeometry_.numFAP; idx++)
        {
            int i = this->face().i;
            int j = this->face().j;

            // compute sum of pressure gradients for each phaseGlobalPosition
            for (int phase = 0; phase < numPhases; phase++)
            {
                const GlobalPosition localIdx_i = element.geometry().corner(i);
                const GlobalPosition localIdx_j = element.geometry().corner(j);

                isFracture_ = problem.spatialParams().isEdgeFracture(element, faceIdx);

                if (isFracture_)
                {
                    GlobalPosition diff_ij = localIdx_j;
                    diff_ij -= localIdx_i;
                    potentialGradFracture_[phase] =
                        (elemVolVars[j].pressure(phase) - elemVolVars[i].pressure(phase))
                        / diff_ij.two_norm();
                }
                else
                {
                    potentialGradFracture_[phase] = 0;
                }
            }
        }
    }
};

} // end namepace

#endif // DUMUX_BOXMODELS_2PDFM_FLUX_VARIABLES_HH
