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
 * \brief Calculates the residual of models based on the box scheme element-wise.
 */
#ifndef DUMUX_STAGGERED_LOCAL_RESIDUAL_HH
#define DUMUX_STAGGERED_LOCAL_RESIDUAL_HH

#include <dumux/common/valgrind.hh>
#include <dumux/common/capabilities.hh>
#include <dumux/common/timeloop.hh>

namespace Dumux
{

namespace Properties
{
    NEW_PROP_TAG(ElementFaceVariables);
}
/*!
 * \ingroup CCModel
 * \ingroup StaggeredLocalResidual
 * \brief Element-wise calculation of the residual for models
 *        based on the fully implicit cell-centered scheme.
 *
 * \todo Please doc me more!
 */
template<class TypeTag>
class StaggeredLocalResidual
{
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);

    enum { numEq = GET_PROP_VALUE(TypeTag, NumEq) };

    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using Implementation = typename GET_PROP_TYPE(TypeTag, LocalResidual);
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using Element = typename GridView::template Codim<0>::Entity;
    using BoundaryTypes = typename GET_PROP_TYPE(TypeTag, BoundaryTypes);
    using ElementBoundaryTypes = typename GET_PROP_TYPE(TypeTag, ElementBoundaryTypes);
    using ElementVolumeVariables = typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables);
    using ElementFluxVariablesCache = typename GET_PROP_TYPE(TypeTag, ElementFluxVariablesCache);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using CellCenterSolutionVector = typename GET_PROP_TYPE(TypeTag, CellCenterSolutionVector);
    using FaceSolutionVector = typename GET_PROP_TYPE(TypeTag, FaceSolutionVector);
    using CellCenterPrimaryVariables = typename GET_PROP_TYPE(TypeTag, CellCenterPrimaryVariables);
    using FacePrimaryVariables = typename GET_PROP_TYPE(TypeTag, FacePrimaryVariables);
    using SolutionVector = typename GET_PROP_TYPE(TypeTag, SolutionVector);

    using CellCenterResidual = typename GET_PROP_TYPE(TypeTag, CellCenterPrimaryVariables);
    using FaceResidual = typename GET_PROP_TYPE(TypeTag, FacePrimaryVariables);
    using FaceResidualVector = typename GET_PROP_TYPE(TypeTag, FaceSolutionVector);
    using ElementFaceVariables = typename GET_PROP_TYPE(TypeTag, ElementFaceVariables);


    using DofTypeIndices = typename GET_PROP(TypeTag, DofTypeIndices);
    typename DofTypeIndices::CellCenterIdx cellCenterIdx;
    typename DofTypeIndices::FaceIdx faceIdx;

    enum {
         // grid and world dimension
        dim = GridView::dimension,
        dimWorld = GridView::dimensionworld
    };


    using TimeLoop = TimeLoopBase<Scalar>;

public:
    //! the constructor for stationary problems
    StaggeredLocalResidual() : prevSol_(nullptr) {}

    StaggeredLocalResidual(std::shared_ptr<TimeLoop> timeLoop)
    : timeLoop_(timeLoop)
    , prevSol_(nullptr)
    {}

     /*!
     * \name User interface
     * \note The following methods are usually expensive to evaluate
     *       They are useful for outputting residual information.
     */
    // \{

     /*!
     * \brief Compute the local residual, i.e. the deviation of the
     *        equations from zero.
     *
     * \param element The DUNE Codim<0> entity for which the residual
     *                ought to be calculated
     * \param fvGeometry The finite-volume geometry of the element
     * \param prevVolVars The volume averaged variables for all
     *                   sub-control volumes of the element at the previous
     *                   time level
     * \param curVolVars The volume averaged variables for all
     *                   sub-control volumes of the element at the current
     *                   time level
     * \param bcTypes The types of the boundary conditions for all
     *                vertices of the element
     */
    auto evalCellCenter(const Problem& problem,
                        const Element &element,
                        const FVElementGeometry& fvGeometry,
                        const ElementVolumeVariables& prevElemVolVars,
                        const ElementVolumeVariables& curElemVolVars,
                        const ElementFaceVariables& prevElemFaceVars,
                        const ElementFaceVariables& curElemFaceVars,
                        const ElementBoundaryTypes &bcTypes,
                        const ElementFluxVariablesCache& elemFluxVarsCache) const
    {
        // reset all terms
        CellCenterResidual residual;
        residual = 0.0;
        // ccStorageTerm_ = 0.0;

        asImp_().evalVolumeTermForCellCenter_(residual, problem, element, fvGeometry, prevElemVolVars, curElemVolVars, prevElemFaceVars, curElemFaceVars, bcTypes);
        asImp_().evalFluxesForCellCenter_(residual, problem, element, fvGeometry, curElemVolVars, curElemFaceVars, bcTypes, elemFluxVarsCache);
        asImp_().evalBoundaryForCellCenter_(residual, problem, element, fvGeometry, curElemVolVars, curElemFaceVars, bcTypes, elemFluxVarsCache);

        return residual;
    }

     /*!
     * \brief Compute the local residual, i.e. the deviation of the
     *        equations from zero.
     *
     * \param element The DUNE Codim<0> entity for which the residual
     *                ought to be calculated
     * \param fvGeometry The finite-volume geometry of the element
     * \param prevVolVars The volume averaged variables for all
     *                   sub-control volumes of the element at the previous
     *                   time level
     * \param curVolVars The volume averaged variables for all
     *                   sub-control volumes of the element at the current
     *                   time level
     * \param bcTypes The types of the boundary conditions for all
     *                vertices of the element
     */
    auto evalFace(const Problem& problem,
                  const Element &element,
                  const FVElementGeometry& fvGeometry,
                  const SubControlVolumeFace& scvf,
                  const ElementVolumeVariables& prevElemVolVars,
                  const ElementVolumeVariables& curElemVolVars,
                  const ElementFaceVariables& prevElemFaceVars,
                  const ElementFaceVariables& curElemFaceVars,
                  const ElementBoundaryTypes &bcTypes,
                  const ElementFluxVariablesCache& elemFluxVarsCache,
                  const bool resizeResidual = false) const
    {
        FaceResidual residual;

        asImp_().evalVolumeTermForFace_(residual, problem, element, fvGeometry, scvf, prevElemVolVars, curElemVolVars, prevElemFaceVars, curElemFaceVars, bcTypes);
        asImp_().evalFluxesForFace_(residual, problem, element, fvGeometry, scvf, curElemVolVars, curElemFaceVars, bcTypes, elemFluxVarsCache);
        asImp_().evalBoundaryForFace_(residual, problem, element, fvGeometry, scvf, curElemVolVars, curElemFaceVars, bcTypes, elemFluxVarsCache);

        return residual;
    }

    /*!
     * \brief Sets the solution from which to start the time integration. Has to be
     *        called prior to assembly for time-dependent problems.
     */
    void setPreviousSolution(const SolutionVector& u)
    { prevSol_ = &u; }

    /*!
     * \brief Return the solution that has been set as the previous one.
     */
    const SolutionVector& prevSol() const
    {
        assert(prevSol_ && "no solution set for storage term evaluation");
        return *prevSol_;
    }

    /*!
     * \brief If no solution has been set, we treat the problem as stationary.
     */
    bool isStationary() const
    { return !prevSol_; }


protected:

     /*!
     * \brief Evaluate the flux terms for cell center dofs
     */
    void evalFluxesForCellCenter_(CellCenterResidual& residual,
                                  const Problem& problem,
                                  const Element& element,
                                  const FVElementGeometry& fvGeometry,
                                  const ElementVolumeVariables& elemVolVars,
                                  const ElementFaceVariables& elemFaceVars,
                                  const ElementBoundaryTypes& bcTypes,
                                  const ElementFluxVariablesCache& elemFluxVarsCache) const
    {
        for (auto&& scvf : scvfs(fvGeometry))
        {
            if(!scvf.boundary())
                residual += asImp_().computeFluxForCellCenter(problem, element, fvGeometry, elemVolVars, elemFaceVars, scvf, elemFluxVarsCache);
        }
    }

     /*!
     * \brief Evaluate the flux terms for face dofs
     */
    void evalFluxesForFace_(FaceResidual& residual,
                            const Problem& problem,
                            const Element& element,
                            const FVElementGeometry& fvGeometry,
                            const SubControlVolumeFace& scvf,
                            const ElementVolumeVariables& elemVolVars,
                            const ElementFaceVariables& elemFaceVars,
                            const ElementBoundaryTypes& bcTypes,
                            const ElementFluxVariablesCache& elemFluxVarsCache) const
    {
        if(!scvf.boundary())
            residual += asImp_().computeFluxForFace(problem, element, scvf, fvGeometry, elemVolVars, elemFaceVars, elemFluxVarsCache);
    }

     /*!
     * \brief Evaluate boundary conditions
     */
    void evalBoundary_(const Problem& problem,
                       const Element& element,
                       const FVElementGeometry& fvGeometry,
                       const ElementVolumeVariables& elemVolVars,
                       const ElementFaceVariables& elemFaceVars,
                       const ElementBoundaryTypes& bcTypes,
                       const ElementFluxVariablesCache& elemFluxVarsCache)
    {
        DUNE_THROW(Dune::InvalidStateException,
           "The localResidual class does not provide "
           "a evalBoundary_() method.");
    }

     /*!
     * \brief Evaluate the volume term for a cell center dof for a stationary problem
     */
    template<class P = Problem>
    typename std::enable_if<Dumux::Capabilities::isStationary<P>::value, void>::type
    evalVolumeTermForCellCenter_(CellCenterResidual& residual,
                                 const Problem& problem,
                                 const Element &element,
                                 const FVElementGeometry& fvGeometry,
                                 const ElementVolumeVariables& prevElemVolVars,
                                 const ElementVolumeVariables& curElemVolVars,
                                 const ElementFaceVariables& prevFaceVars,
                                 const ElementFaceVariables& curFaceVars,
                                 const ElementBoundaryTypes &bcTypes) const
    {
        for(auto&& scv : scvs(fvGeometry))
        {
            const auto curExtrusionFactor = curElemVolVars[scv].extrusionFactor();

            // subtract the source term from the local rate
            CellCenterPrimaryVariables source = asImp_().computeSourceForCellCenter(problem, element, fvGeometry, curElemVolVars, curFaceVars, scv);
            source *= scv.volume()*curExtrusionFactor;
            residual -= source;
        }
    }

     /*!
     * \brief Evaluate the volume term for a face dof for a stationary problem
     */
    template<class P = Problem>
    typename std::enable_if<Dumux::Capabilities::isStationary<P>::value, void>::type
    evalVolumeTermForFace_(FaceResidual& residual,
                           const Problem& problem,
                           const Element &element,
                           const FVElementGeometry& fvGeometry,
                           const SubControlVolumeFace& scvf,
                           const ElementVolumeVariables& prevElemVolVars,
                           const ElementVolumeVariables& curElemVolVars,
                           const ElementFaceVariables& prevFaceVars,
                           const ElementFaceVariables& curFaceVars,
                           const ElementBoundaryTypes &bcTypes) const
    {
        // the source term:
        auto faceSource = asImp_().computeSourceForFace(problem, scvf, curElemVolVars, curFaceVars);
        const auto& scv = fvGeometry.scv(scvf.insideScvIdx());
        const auto curExtrusionFactor = curElemVolVars[scv].extrusionFactor();
        faceSource *= 0.5*scv.volume()*curExtrusionFactor;
        residual -= faceSource;
    }

     /*!
     * \brief Evaluate the volume term for a cell center dof for a transient problem
     */
    template<class P = Problem>
    typename std::enable_if<!Dumux::Capabilities::isStationary<P>::value, void>::type
    evalVolumeTermForCellCenter_(CellCenterResidual& residual,
                                 const Problem& problem,
                                 const Element &element,
                                 const FVElementGeometry& fvGeometry,
                                 const ElementVolumeVariables& prevElemVolVars,
                                 const ElementVolumeVariables& curElemVolVars,
                                 const ElementFaceVariables& prevFaceVars,
                                 const ElementFaceVariables& curFaceVars,
                                 const ElementBoundaryTypes &bcTypes) const
    {
        for(auto&& scv : scvs(fvGeometry))
        {
            const auto& curVolVars = curElemVolVars[scv];
            const auto& prevVolVars = prevElemVolVars[scv];

            // mass balance within the element. this is the
            // \f$\frac{m}{\partial t}\f$ term if using implicit
            // euler as time discretization.
            //
            // We might need a more explicit way for
            // doing the time discretization...
            auto prevCCStorage = asImp_().computeStorageForCellCenter(problem, scv, prevVolVars);
            auto curCCStorage = asImp_().computeStorageForCellCenter(problem, scv, curVolVars);

            prevCCStorage *= prevVolVars.extrusionFactor();
            curCCStorage *= curVolVars.extrusionFactor();

            CellCenterResidual storageTerm(0.0);

            storageTerm = std::move(curCCStorage);
            storageTerm -= std::move(prevCCStorage);
            storageTerm *= scv.volume();
            storageTerm /= timeLoop_->timeStepSize();

            // add the storage term to the residual
            residual += storageTerm;

            // subtract the source term from the local rate
            CellCenterPrimaryVariables source = asImp_().computeSourceForCellCenter(problem, element, fvGeometry, curElemVolVars, curFaceVars, scv);
            source *= scv.volume()*curVolVars.extrusionFactor();

            residual -= source;
        }
    }

     /*!
     * \brief Evaluate the volume term for a face dof for a transient problem
     */
    template<class P = Problem>
    typename std::enable_if<!Dumux::Capabilities::isStationary<P>::value, void>::type
    evalVolumeTermForFace_(FaceResidual& residual,
                           const Problem& problem,
                           const Element &element,
                           const FVElementGeometry& fvGeometry,
                           const SubControlVolumeFace& scvf,
                           const ElementVolumeVariables& prevElemVolVars,
                           const ElementVolumeVariables& curElemVolVars,
                           const ElementFaceVariables& prevFaceVars,
                           const ElementFaceVariables& curFaceVars,
                           const ElementBoundaryTypes &bcTypes) const
    {
        const auto& scv = fvGeometry.scv(scvf.insideScvIdx());
        const auto& curVolVars = curElemVolVars[scv];
        const auto& prevVolVars = prevElemVolVars[scv];
        auto prevFaceStorage = asImp_().computeStorageForFace(problem, scvf, prevVolVars, prevFaceVars);
        auto curFaceStorage = asImp_().computeStorageForFace(problem, scvf, curVolVars, curFaceVars);

        // the storage term
        residual = std::move(curFaceStorage);
        residual -= std::move(prevFaceStorage);
        residual *= (scv.volume()/2.0);
        residual /= timeLoop_->timeStepSize();
        // residuals[scvf.localFaceIdx()] += faceStorageTerms_[scvf.localFaceIdx()];

        // the source term:
        auto faceSource = asImp_().computeSourceForFace(problem, scvf, curElemVolVars, curFaceVars);
        faceSource *= 0.5*scv.volume()*curVolVars.extrusionFactor();
        residual -= faceSource;
    }

    Implementation &asImp_()
    { return *static_cast<Implementation*>(this); }

    const Implementation &asImp_() const
    { return *static_cast<const Implementation*>(this); }


    TimeLoop& timeLoop()
    { return *timeLoop_; }

    const TimeLoop& timeLoop() const
    { return *timeLoop_; }

    Implementation &asImp()
    { return *static_cast<Implementation*>(this); }

    const Implementation &asImp() const
    { return *static_cast<const Implementation*>(this); }

private:
    std::shared_ptr<TimeLoop> timeLoop_;
    const SolutionVector* prevSol_;

};

}

#endif   // DUMUX_CC_LOCAL_RESIDUAL_HH
