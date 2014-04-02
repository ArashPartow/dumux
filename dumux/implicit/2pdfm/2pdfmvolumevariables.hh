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
 * \brief Contains the quantities which are constant within a
 *        finite volume in the two-phase discrete fracture-matrix model.
 */
#ifndef DUMUX_MODELS_2PDFM_VOLUME_VARIABLES_HH
#define DUMUX_MODELS_2PDFM_VOLUME_VARIABLES_HH

#include <dune/common/fvector.hh>

#include <dumux/implicit/2p/2pvolumevariables.hh>

#include "2pdfmproperties.hh"

namespace Dumux
{
/*!
 * \ingroup TwoPDFMModel
 * \ingroup ImplicitVolumeVariables
 * \brief Contains the quantities which are are constant within a
 *        finite volume in the two-phase discrete fracture-matrix model.
 */
template <class TypeTag>
class TwoPDFMVolumeVariables : public TwoPVolumeVariables<TypeTag>
{
    typedef TwoPVolumeVariables<TypeTag> ParentType;

    typedef typename GET_PROP_TYPE(TypeTag, VolumeVariables) Implementation;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, FluidState) FluidState;
    typedef typename GET_PROP_TYPE(TypeTag, MaterialLaw) MaterialLaw;
    typedef typename GET_PROP_TYPE(TypeTag, MaterialLawParams) MaterialLawParams;
    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;
    typedef typename FVElementGeometry::SubControlVolumeFace SCVFace;

    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    enum {
        pressureIdx = Indices::pressureIdx,
        saturationIdx = Indices::saturationIdx,
        wPhaseIdx = Indices::wPhaseIdx,
        nPhaseIdx = Indices::nPhaseIdx,
        numPhases = GET_PROP_VALUE(TypeTag, NumPhases),
        formulation = GET_PROP_VALUE(TypeTag, Formulation)
    };

    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GET_PROP_TYPE(TypeTag, Grid) GridType;
    typedef typename GridType::ctype DT;

    enum {
            dim = GridView::dimension,
            dimWorld = GridView::dimensionworld
    };
    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;

#if DUNE_VERSION_NEWER(DUNE_GRID, 2, 3)
    typedef typename Dune::ReferenceElements<DT, dim> ReferenceElements;
    typedef typename Dune::ReferenceElement<DT, dim> ReferenceElement;
#else
    typedef typename Dune::GenericReferenceElements<DT, dim> ReferenceElements;
    typedef typename Dune::GenericReferenceElement<DT, dim> ReferenceElement;
#endif

public:
    /*!
     * \copydoc ImplicitVolumeVariables::update
     */
    void update(const PrimaryVariables &priVars,
                const Problem &problem,
                const Element &element,
                const FVElementGeometry &fvGeometry,
                int scvIdx,
                bool isOldSol)
    {
        ParentType::update(priVars,
                           problem,
                           element,
                           fvGeometry,
                           scvIdx,
                           isOldSol);

        this->completeFluidState(priVars, problem, element, fvGeometry, scvIdx, fluidState_);

        const MaterialLawParams &materialParams =
            problem.spatialParams().materialLawParams(element, fvGeometry, scvIdx);

        mobilityMatrix_[wPhaseIdx] =
            MaterialLaw::krw(materialParams, fluidState_.saturation(wPhaseIdx))
            / fluidState_.viscosity(wPhaseIdx);

        mobilityMatrix_[nPhaseIdx] =
            MaterialLaw::krn(materialParams, fluidState_.saturation(wPhaseIdx))
            / fluidState_.viscosity(nPhaseIdx);

        // porosity
        porosityMatrix_ = problem.spatialParams().porosity(element, fvGeometry, scvIdx);

        // energy related quantities not belonging to the fluid state
        asImp_().updateEnergy_(priVars, problem, element, fvGeometry, scvIdx, isOldSol);
        asImp_().updateFracture(priVars, problem, element, fvGeometry, scvIdx, isOldSol);
    }

    /*!
     * \brief Construct the volume variables for all fracture vertices.
     *
     * \param priVars Primary variables.
     * \param problem The problem which needs to be simulated.
     * \param element The DUNE Codim<0> entity for which the volume variables ought to be calculated
     * \param fvGeometry The finite volume geometry of the element
     * \param scvIdx Sub-control volume index
     * \param isOldSol Tells whether the model's previous or current solution should be used.
     */
    void updateFracture(const PrimaryVariables &priVars,
                        const Problem &problem,
                        const Element &element,
                        const FVElementGeometry &fvGeometry,
                        int scvIdx,
                        bool isOldSol)
    {
        PrimaryVariables varsFracture;
        const MaterialLawParams &materialParamsMatrix =
                    problem.spatialParams().materialLawParams(element, fvGeometry, scvIdx);
        Scalar pressure[numPhases];
        Scalar pMatrix[numPhases];
        Scalar pFract[numPhases];

        satNMatrix_  = priVars[saturationIdx];
        satWMatrix_  = 1.0 - satNMatrix_;
        satN_ = satNMatrix_;
        satW_ = satWMatrix_;

        pcMatrix_ = MaterialLaw::pc(materialParamsMatrix, satWMatrix_);
        pc_ = pcMatrix_;
        //pressures
        pMatrix[wPhaseIdx] = priVars[pressureIdx];
        pMatrix[nPhaseIdx] = pMatrix[wPhaseIdx] + pcMatrix_;
        //Initialize pFract with the same values as the ones in the matrix
        pFract[wPhaseIdx] = pMatrix[wPhaseIdx];
        pFract[nPhaseIdx] = satNMatrix_;

        varsFracture[pressureIdx] = pFract[wPhaseIdx];
        varsFracture[saturationIdx] = pFract[wPhaseIdx];

        this->completeFluidState(priVars, problem, element, fvGeometry, scvIdx, fluidStateFracture_);

        //Checks if the node is on a fracture
        isNodeOnFracture_ = problem.spatialParams().isVertexFracture(element, scvIdx);

        ///////////////////////////////////////////////////////////////////////////////
        if (isNodeOnFracture_)
        {
            const MaterialLawParams &materialParamsFracture =
                    problem.spatialParams().materialLawParamsFracture(element, fvGeometry, scvIdx);

            satNFracture_ = priVars[saturationIdx];
            satWFracture_ = 1 - satNFracture_;
            pcFracture_ = MaterialLaw::pc(materialParamsFracture, satWFracture_);
            pFract[wPhaseIdx] = priVars[pressureIdx];
            pFract[nPhaseIdx] = pFract[wPhaseIdx] + pcFracture_;
            pEntryMatrix_ = MaterialLaw::pc(materialParamsMatrix, 1);

            //use interface condition - extended capillary pressure inteface condition
            if (problem.useInterfaceCondition())
            {
                interfaceCondition(materialParamsMatrix);
            }
            pc_ = pcFracture_;
            satW_ = satWFracture_; //for plotting we are interested in the saturations of the fracture
            satN_ = satNFracture_;
            mobilityFracture_[wPhaseIdx] =
                    MaterialLaw::krw(materialParamsFracture, fluidStateFracture_.saturation(wPhaseIdx))
                    / fluidStateFracture_.viscosity(wPhaseIdx);

            mobilityFracture_[nPhaseIdx] =
                    MaterialLaw::krn(materialParamsFracture, fluidStateFracture_.saturation(wPhaseIdx))
                        / fluidStateFracture_.viscosity(nPhaseIdx);

            // derivative resulted from BrooksCorey pc_Sw formulation
            dsm_dsf_ = (1 - problem.spatialParams().swrm_) / (1 - problem.spatialParams().swrf_)
                    * pow((problem.spatialParams().pdm_/ problem.spatialParams().pdf_),problem.spatialParams().lambdaM_)
                    * (problem.spatialParams().lambdaM_ / problem.spatialParams().lambdaF_)
                    * pow((satWFracture_ - problem.spatialParams().swrf_ ) / (1 - problem.spatialParams().swrf_),
                            (problem.spatialParams().lambdaM_ / problem.spatialParams().lambdaF_) - 1);
        }// end if (node)
        ///////////////////////////////////////////////////////////////////////////////
        else
        {
            /* the values of pressure and saturation of the fractures in the volumes where
                there are no fracture are set unphysical*/
            satNFracture_ = -1;
            satWFracture_ = -1;
            pcFracture_ = -1e100;
            pFract[wPhaseIdx] = -1e100;
            pFract[nPhaseIdx] = -1e100;
            pEntryMatrix_ = -1e100;
            mobilityFracture_[wPhaseIdx] = 0.0;
            mobilityFracture_[nPhaseIdx] = 0.0;
        }
        ///////////////////////////////////////////////////////////////////////////////
        pressure[wPhaseIdx] = priVars[pressureIdx];
        pressure[nPhaseIdx] = pressure[wPhaseIdx] + pc_;

        porosityFracture_ = problem.spatialParams().porosityFracture(element,
                                                                  fvGeometry,
                                                                  scvIdx);
    }

    /*!
     * \brief Extended capillary pressure saturation interface condition
     *
     * \param materialParamsMatrix the material law o calculate the sw as inverse of capillary pressure function
     *
     * This method is called by updateFracture
     */
    void interfaceCondition(const MaterialLawParams &materialParamsMatrix)
    {
        /*2nd condition Niessner, J., R. Helmig, H. Jakobs, and J.E. Roberts. 2005, eq.10
        * if the capillary pressure in the fracture is smaller than the entry pressure
        * in the matrix than in the matrix
        * */
        if (pcFracture_ <= pEntryMatrix_)
        {
            satWMatrix_ = 1.0;
            satNMatrix_ = 1 - satWMatrix_;
        }
        //3rd condition Niessner, J., R. Helmig, H. Jakobs, and J.E. Roberts. 2005, eq.10
        else
        {
            /*
             * Inverse capillary pressure function SwM = pcM^(-1)(pcF(SwF))
             */
            satWMatrix_ = MaterialLaw::sw(materialParamsMatrix, pcFracture_);
            satNMatrix_ = 1 - satWMatrix_;
        }
    }

    /*!
     * \brief Calculates the volume of the fracture inside the SCV
     */
    Scalar calculateSCVFractureVolume ( const Problem &problem,
                                        const Element &element,
                                        const FVElementGeometry &fvGeometry,
                                        int scvIdx)
    {
        Scalar volSCVFracture;
        Dune::GeometryType geomType = element.geometry().type();
        const ReferenceElement &refElement = ReferenceElements::general(geomType);

        for (int faceIdx=0; faceIdx<refElement.size(1); faceIdx++)
        {
            SCVFace face = fvGeometry.subContVolFace[faceIdx];
            int i=face.i;
            int j=face.j;

            if (problem.spatialParams().isEdgeFracture(element, faceIdx)
                && (i == scvIdx || j == scvIdx))
            {
                Scalar fracture_width = problem.spatialParams().fractureWidth();

                const GlobalPosition global_i = element.geometry().corner(i);
                const GlobalPosition global_j = element.geometry().corner(j);
                GlobalPosition diff_ij = global_j;
                diff_ij -=global_i;
                //fracture length in the subcontrol volume is half d_ij
                Scalar fracture_length = 0.5*diff_ij.two_norm();

                volSCVFracture += 0.5 * fracture_length * fracture_width;
            }
        }
        return volSCVFracture;
    }

    /*!
     * \brief Returns the effective saturation fracture of a given phase within
     *        the control volume.
     *
     * \param phaseIdx The phase index
     */
    Scalar saturationFracture(int phaseIdx) const
    {
        if (phaseIdx == wPhaseIdx)
            return satWFracture_;
        else
            return satNFracture_;
    }
    Scalar saturationMatrix(int phaseIdx) const
    {
         if (phaseIdx == wPhaseIdx)
             return satWMatrix_;
         else
             return satNMatrix_;
    }

    /*!
     * \brief Returns the effective mobility of a given phase within
     *        the control volume.
     *
     * \param phaseIdx The phase index
     */
    Scalar mobility(int phaseIdx) const
    { return mobilityMatrix_[phaseIdx]; }

    /*!
     * \brief Returns the effective mobility of a given phase within
     *        the control volume.
     *
     * \param phaseIdx The phase index
     */
    Scalar mobilityFracture(int phaseIdx) const
    { return mobilityFracture_[phaseIdx]; }

    /*!
     * \brief Returns the average porosity within the matrix control volume.
     */
    Scalar porosity() const
    { return porosityMatrix_; }

    /*!
     * \brief Returns the average porosity within the fracture.
     */
    Scalar porosityFracture() const
    { return porosityFracture_; }

    /*!
     * \brief Returns the average permeability within the fracture.
     */
    Scalar permeabilityFracture() const
    { return permeabilityFracture_; }

    /*!
     * \brief Returns the derivative dsm/dsf
     */
    Scalar dsm_dsf() const
    { return dsm_dsf_;}

protected:
    FluidState fluidState_;
    FluidState fluidStateFracture_;
    Scalar porosityMatrix_;
    Scalar porosityFracture_;
    Scalar permeability_;
    Scalar permeabilityFracture_;
    Scalar mobilityMatrix_[numPhases];
    Scalar mobilityFracture_[numPhases];

    Scalar satW_;
    Scalar satWFracture_;
    Scalar satWMatrix_;
    Scalar satN_;
    Scalar satNFracture_;
    Scalar satNMatrix_;

    Scalar pc_;
    Scalar pcFracture_;
    Scalar pcMatrix_;
    Scalar pEntryMatrix_;
    Scalar dsm_dsf_;

    bool isNodeOnFracture_;

private:
    Implementation &asImp_()
    { return *static_cast<Implementation*>(this); }

    const Implementation &asImp_() const
    { return *static_cast<const Implementation*>(this); }
};
} // end namespace

#endif // DUMUX_MODELS_2PDFM_VOLUME_VARIABLES_HH
