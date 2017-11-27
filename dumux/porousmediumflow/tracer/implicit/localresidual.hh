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
 * \brief Element-wise calculation of the local residual for problems
 *        using fully implicit tracer model.
 */
#ifndef DUMUX_TRACER_LOCAL_RESIDUAL_HH
#define DUMUX_TRACER_LOCAL_RESIDUAL_HH

namespace Dumux
{

/*!
 * \ingroup Implicit
 * \ingroup TracerLocalResidual
 * \brief Element-wise calculation of the local residual for problems
 *        using fully implicit tracer model.
 *
 */
template<class TypeTag>
class TracerLocalResidual: public GET_PROP_TYPE(TypeTag, BaseLocalResidual)
{
    using ParentType = typename GET_PROP_TYPE(TypeTag, BaseLocalResidual);
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using PrimaryVariables = typename GET_PROP_TYPE(TypeTag, PrimaryVariables);
    using FluxVariables = typename GET_PROP_TYPE(TypeTag, FluxVariables);
    using ElementFluxVariablesCache = typename GET_PROP_TYPE(TypeTag, ElementFluxVariablesCache);
    using Indices = typename GET_PROP_TYPE(TypeTag, Indices);
    using BoundaryTypes = typename GET_PROP_TYPE(TypeTag, BoundaryTypes);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Element = typename GridView::template Codim<0>::Entity;
    using ElementVolumeVariables = typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables);
    using VolumeVariables = typename GET_PROP_TYPE(TypeTag, VolumeVariables);
    using EnergyLocalResidual = typename GET_PROP_TYPE(TypeTag, EnergyLocalResidual);
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);

    static const int numComponents = GET_PROP_VALUE(TypeTag, NumComponents);
    static const bool useMoles = GET_PROP_VALUE(TypeTag, UseMoles);

public:
    using ParentType::ParentType;

    /*!
     * \brief Evaluate the amount of all conservation quantities
     *        (e.g. phase mass) within a sub-control volume.
     *
     * The result should be averaged over the volume (e.g. phase mass
     * inside a sub control volume divided by the volume)
     *
     *  \param scv The sub control volume
     *  \param volVars The primary and secondary varaibles on the scv
     *  \param useMoles If mole or mass fractions are used
     */
    PrimaryVariables computeStorage(const Problem& problem,
                                    const SubControlVolume& scv,
                                    const VolumeVariables& volVars) const
    {
        PrimaryVariables storage(0.0);

        // formulation with mole balances
        if (useMoles)
        {
            for (int compIdx = 0; compIdx < numComponents; ++compIdx)
                storage[compIdx] += volVars.porosity()
                                    * volVars.molarDensity(0)
                                    * volVars.moleFraction(0, compIdx);
        }
        // formulation with mass balances
        else
        {
            for (int compIdx = 0; compIdx < numComponents; ++compIdx)
                storage[compIdx] += volVars.porosity()
                                    * volVars.density(0)
                                    * volVars.massFraction(0, compIdx);
        }

        return storage;
    }

    /*!
     * \brief Evaluates the total flux of all conservation quantities
     *        over a face of a sub-control volume.
     *
     * \param element The element
     * \param fvGeometry The finite volume geometry context
     * \param elemVolVars The volume variables for all flux stencil elements
     * \param scvf The sub control volume face
     * \param elemFluxVarsCache The cache related to flux compuation
     * \param useMoles If mole or mass fractions are used
     */
    PrimaryVariables computeFlux(const Problem& problem,
                                 const Element& element,
                                 const FVElementGeometry& fvGeometry,
                                 const ElementVolumeVariables& elemVolVars,
                                 const SubControlVolumeFace& scvf,
                                 const ElementFluxVariablesCache& elemFluxVarsCache) const
    {
        FluxVariables fluxVars;
        fluxVars.init(problem, element, fvGeometry, elemVolVars, scvf, elemFluxVarsCache);

        // get upwind weights into local scope
        PrimaryVariables flux(0.0);
        const auto diffusiveFluxes = fluxVars.molecularDiffusionFlux(0);

        // formulation with mole balances
        if (useMoles)
        {
            for (int compIdx = 0; compIdx < numComponents; ++compIdx)
            {
                // the physical quantities for which we perform upwinding
                auto upwindTerm = [compIdx](const VolumeVariables& volVars)
                { return volVars.molarDensity()*volVars.moleFraction(0, compIdx); };

                // advective fluxes
                flux[compIdx] += fluxVars.advectiveFlux(0, upwindTerm);
                // diffusive fluxes
                flux[compIdx] += diffusiveFluxes[compIdx];
            }
        }
        // formulation with mass balances
        else
        {
            for (int compIdx = 0; compIdx < numComponents; ++compIdx)
            {
                // the physical quantities for which we perform upwinding
                auto upwindTerm = [compIdx](const VolumeVariables& volVars)
                { return volVars.density()*volVars.massFraction(0, compIdx); };

                // advective fluxes
                flux[compIdx] += fluxVars.advectiveFlux(0, upwindTerm);
                // diffusive fluxes
                flux[compIdx] += diffusiveFluxes[compIdx]*FluidSystem::molarMass(compIdx);
            }
        }

        return flux;
    }

    template<class PartialDerivativeMatrix>
    void addStorageDerivatives(PartialDerivativeMatrix& partialDerivatives,
                               const Problem& problem,
                               const Element& element,
                               const FVElementGeometry& fvGeometry,
                               const VolumeVariables& curVolVars,
                               const SubControlVolume& scv) const
    {
        const auto porosity = curVolVars.porosity();
        const auto rho = useMoles ? curVolVars.molarDensity() : curVolVars.density();
        const auto d_storage = scv.volume()*porosity*rho/this->timeLoop().timeStepSize();

        for (int compIdx = 0; compIdx < numComponents; ++compIdx)
            partialDerivatives[compIdx][compIdx] += d_storage;
    }

    template<class PartialDerivativeMatrix>
    void addSourceDerivatives(PartialDerivativeMatrix& partialDerivatives,
                              const Problem& problem,
                              const Element& element,
                              const FVElementGeometry& fvGeometry,
                              const VolumeVariables& curVolVars,
                              const SubControlVolume& scv) const
    {
        // TODO maybe forward to the problem? -> necessary for reaction terms
    }

    template<class PartialDerivativeMatrices, class T = TypeTag>
    std::enable_if_t<!GET_PROP_VALUE(T, ImplicitIsBox), void>
    addFluxDerivatives(PartialDerivativeMatrices& derivativeMatrices,
                       const Problem& problem,
                       const Element& element,
                       const FVElementGeometry& fvGeometry,
                       const ElementVolumeVariables& curElemVolVars,
                       const ElementFluxVariablesCache& elemFluxVarsCache,
                       const SubControlVolumeFace& scvf) const
    {
        // advective term: we do the same for all tracer components
        auto rho = [](const VolumeVariables& volVars)
        { return useMoles ? volVars.molarDensity() : volVars.density(); };

        // the volume flux
        const auto volFlux = problem.spatialParams().volumeFlux(element, fvGeometry, curElemVolVars, scvf);

        // the upwind weight
        static const Scalar upwindWeight = GET_PARAM_FROM_GROUP(TypeTag, Scalar, Implicit, UpwindWeight);

        // get the inside and outside volvars
        const auto& insideVolVars = curElemVolVars[scvf.insideScvIdx()];
        const auto& outsideVolVars = curElemVolVars[scvf.outsideScvIdx()];

        const auto insideWeight = std::signbit(volFlux) ? (1.0 - upwindWeight) : upwindWeight;
        const auto outsideWeight = 1.0 - insideWeight;
        const auto advDerivII = volFlux*rho(insideVolVars)*insideWeight;
        const auto advDerivIJ = volFlux*rho(outsideVolVars)*outsideWeight;

        // diffusive term
        const auto& fluxCache = elemFluxVarsCache[scvf];
        const auto rhoMolar = 0.5*(insideVolVars.molarDensity() + outsideVolVars.molarDensity());

        for (int compIdx = 0; compIdx < numComponents; ++compIdx)
        {
            // diffusive term
            const auto diffDeriv = useMoles ? rhoMolar*fluxCache.diffusionTij(/*phaseIdx=*/0, compIdx)
                                            : rhoMolar*fluxCache.diffusionTij(/*phaseIdx=*/0, compIdx)*FluidSystem::molarMass(compIdx);

            derivativeMatrices[scvf.insideScvIdx()][compIdx][compIdx] += (advDerivII + diffDeriv);
            derivativeMatrices[scvf.outsideScvIdx()][compIdx][compIdx] += (advDerivIJ - diffDeriv);
        }
    }

    template<class JacobianMatrix, class T = TypeTag>
    std::enable_if_t<GET_PROP_VALUE(T, ImplicitIsBox), void>
    addFluxDerivatives(JacobianMatrix& A,
                       const Problem& problem,
                       const Element& element,
                       const FVElementGeometry& fvGeometry,
                       const ElementVolumeVariables& curElemVolVars,
                       const ElementFluxVariablesCache& elemFluxVarsCache,
                       const SubControlVolumeFace& scvf) const
    {

        // advective term: we do the same for all tracer components
        auto rho = [](const VolumeVariables& volVars)
        { return useMoles ? volVars.molarDensity() : volVars.density(); };

        // the volume flux
        const auto volFlux = problem.spatialParams().volumeFlux(element, fvGeometry, curElemVolVars, scvf);

        // the upwind weight
        static const Scalar upwindWeight = GET_PARAM_FROM_GROUP(TypeTag, Scalar, Implicit, UpwindWeight);

        // get the inside and outside volvars
        const auto& insideVolVars = curElemVolVars[scvf.insideScvIdx()];
        const auto& outsideVolVars = curElemVolVars[scvf.outsideScvIdx()];

        const auto insideWeight = std::signbit(volFlux) ? (1.0 - upwindWeight) : upwindWeight;
        const auto outsideWeight = 1.0 - insideWeight;
        const auto advDerivII = volFlux*rho(insideVolVars)*insideWeight;
        const auto advDerivIJ = volFlux*rho(outsideVolVars)*outsideWeight;

        // diffusive term
        using DiffusionType = typename GET_PROP_TYPE(T, MolecularDiffusionType);
        const auto ti = DiffusionType::calculateTransmissibilities(problem,
                                                                   element,
                                                                   fvGeometry,
                                                                   curElemVolVars,
                                                                   scvf,
                                                                   elemFluxVarsCache[scvf],
                                                                   /*phaseIdx=*/0);
        const auto& insideScv = fvGeometry.scv(scvf.insideScvIdx());
        const auto& outsideScv = fvGeometry.scv(scvf.outsideScvIdx());

        for (int compIdx = 0; compIdx < numComponents; ++compIdx)
        {
            for (const auto& scv : scvs(fvGeometry))
            {
                // diffusive term
                const auto diffDeriv = useMoles ? ti[compIdx][scv.indexInElement()]
                                                : ti[compIdx][scv.indexInElement()]*FluidSystem::molarMass(compIdx);
                A[insideScv.dofIndex()][scv.dofIndex()][compIdx][compIdx] += diffDeriv;
                A[outsideScv.dofIndex()][scv.dofIndex()][compIdx][compIdx] -= diffDeriv;
            }

            A[insideScv.dofIndex()][insideScv.dofIndex()][compIdx][compIdx] += advDerivII;
            A[insideScv.dofIndex()][outsideScv.dofIndex()][compIdx][compIdx] += advDerivIJ;
            A[outsideScv.dofIndex()][outsideScv.dofIndex()][compIdx][compIdx] -= advDerivII;
            A[outsideScv.dofIndex()][insideScv.dofIndex()][compIdx][compIdx] -= advDerivIJ;
        }
    }

    template<class PartialDerivativeMatrices>
    void addCCDirichletFluxDerivatives(PartialDerivativeMatrices& derivativeMatrices,
                                       const Problem& problem,
                                       const Element& element,
                                       const FVElementGeometry& fvGeometry,
                                       const ElementVolumeVariables& curElemVolVars,
                                       const ElementFluxVariablesCache& elemFluxVarsCache,
                                       const SubControlVolumeFace& scvf) const
    {
        // do the same as for inner facets
        addFluxDerivatives(derivativeMatrices, problem, element, fvGeometry,
                           curElemVolVars, elemFluxVarsCache, scvf);
    }

    template<class PartialDerivativeMatrices>
    void addRobinFluxDerivatives(PartialDerivativeMatrices& derivativeMatrices,
                                 const Problem& problem,
                                 const Element& element,
                                 const FVElementGeometry& fvGeometry,
                                 const ElementVolumeVariables& curElemVolVars,
                                 const ElementFluxVariablesCache& elemFluxVarsCache,
                                 const SubControlVolumeFace& scvf) const
    {
        // TODO maybe forward to the problem?
    }
};

} // end namespace Dumux

#endif
