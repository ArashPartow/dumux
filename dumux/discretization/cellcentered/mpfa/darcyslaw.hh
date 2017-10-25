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
 * \brief This file contains the data which is required to calculate
 *        volume and mass fluxes of fluid phases over a face of a finite volume by means
 *        of the Darcy approximation. Specializations are provided for the different discretization methods.
 */
#ifndef DUMUX_DISCRETIZATION_CC_MPFA_DARCYS_LAW_HH
#define DUMUX_DISCRETIZATION_CC_MPFA_DARCYS_LAW_HH

#include <dumux/implicit/cellcentered/mpfa/properties.hh>

namespace Dumux
{

namespace Properties
{
// forward declaration of properties
NEW_PROP_TAG(ProblemEnableGravity);
NEW_PROP_TAG(MpfaHelper);
}

/*!
 * \ingroup DarcysLaw
 * \brief Specialization of Darcy's Law for the CCMpfa method.
 */
template <class TypeTag>
class DarcysLawImplementation<TypeTag, DiscretizationMethods::CCMpfa>
{
    using Implementation = typename GET_PROP_TYPE(TypeTag, AdvectionType);

    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Element = typename GridView::template Codim<0>::Entity;
    using MpfaHelper = typename GET_PROP_TYPE(TypeTag, MpfaHelper);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using VolumeVariables = typename GET_PROP_TYPE(TypeTag, VolumeVariables);
    using ElementVolumeVariables = typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables);
    using ElementFluxVariablesCache = typename GET_PROP_TYPE(TypeTag, ElementFluxVariablesCache);
    using FluxVariablesCache = typename GET_PROP_TYPE(TypeTag, FluxVariablesCache);
    using ElementSolutionVector = typename GET_PROP_TYPE(TypeTag, ElementSolutionVector);

    // Always use the dynamic type for vectors (compatibility with the boundary)
    using PrimaryInteractionVolume = typename GET_PROP_TYPE(TypeTag, PrimaryInteractionVolume);
    using CoefficientVector = typename PrimaryInteractionVolume::Traits::DynamicVector;
    using DataHandle = typename PrimaryInteractionVolume::Traits::DataHandle;

    static constexpr int numPhases = GET_PROP_VALUE(TypeTag, NumPhases);
    static constexpr int dim = GridView::dimension;
    static constexpr int dimWorld = GridView::dimensionworld;

    //! The cache used in conjunction with the mpfa Darcy's Law
    class MpfaDarcysLawCache
    {
        // We always use the dynamic types here to be compatible on the boundary
        using Stencil = typename PrimaryInteractionVolume::Traits::DynamicGlobalIndexContainer;
        using DirichletDataContainer = typename PrimaryInteractionVolume::DirichletDataContainer;

    public:
        //! update cached objects
        template<class InteractionVolume>
        void updateAdvection(const InteractionVolume& iv, const DataHandle& dataHandle, const SubControlVolumeFace &scvf)
        {
            const auto& localFaceData = iv.getLocalFaceData(scvf);

            // update the quantities that are equal for all phases
            advectionSwitchFluxSign_ = localFaceData.isOutside();
            advectionVolVarsStencil_ = &dataHandle.volVarsStencil();
            advectionDirichletData_ = &dataHandle.dirichletData();

            // the transmissibilities on surface grids have to be obtained from the outside
            if (dim == dimWorld)
                advectionTij_ = &dataHandle.T()[localFaceData.ivLocalScvfIndex()];
            else
                advectionTij_ = localFaceData.isOutside() ?
                                &dataHandle.outsideTij()[localFaceData.ivLocalOutsideScvfIndex()] :
                                &dataHandle.T()[localFaceData.ivLocalScvfIndex()];
        }

        //! Returns the stencil for advective scvf flux computation
        const Stencil& advectionVolVarsStencil() const
        { return *advectionVolVarsStencil_; }

        //! Returns the transmissibilities associated with the volume variables
        //! All phases flow through the same rock, thus, tij are equal for all phases
        const CoefficientVector& advectionTij() const
        { return *advectionTij_; }

        //! On faces that are "outside" w.r.t. a face in the interaction volume,
        //! we have to take the negative value of the fluxes, i.e. multiply by -1.0
        bool advectionSwitchFluxSign() const
        { return advectionSwitchFluxSign_; }

        //! Returns the data on dirichlet boundary conditions affecting
        //! the flux computation on this face
        const DirichletDataContainer& advectionDirichletData() const
        { return *advectionDirichletData_; }

    private:
        bool advectionSwitchFluxSign_;
        const Stencil* advectionVolVarsStencil_;
        const CoefficientVector* advectionTij_;
        const DirichletDataContainer* advectionDirichletData_;
    };

    //! Class that fills the cache corresponding to mpfa Darcy's Law
    class MpfaDarcysLawCacheFiller
    {
    public:
        //! Function to fill an MpfaDarcysLawCache of a given scvf
        //! This interface has to be met by any advection-related cache filler class
        template<class FluxVariablesCacheFiller>
        static void fill(FluxVariablesCache& scvfFluxVarsCache,
                         const Problem& problem,
                         const Element& element,
                         const FVElementGeometry& fvGeometry,
                         const ElementVolumeVariables& elemVolVars,
                         const SubControlVolumeFace& scvf,
                         const FluxVariablesCacheFiller& fluxVarsCacheFiller)
        {
            // get interaction volume from the flux vars cache filler & upate the cache
            if (fvGeometry.fvGridGeometry().vertexUsesSecondaryInteractionVolume(scvf.vertexIndex()))
                scvfFluxVarsCache.updateAdvection(fluxVarsCacheFiller.secondaryInteractionVolume(),
                                                  fluxVarsCacheFiller.dataHandle(),
                                                  scvf);
            else
                scvfFluxVarsCache.updateAdvection(fluxVarsCacheFiller.primaryInteractionVolume(),
                                                  fluxVarsCacheFiller.dataHandle(),
                                                  scvf);
        }
    };

public:
    // state the discretization method this implementation belongs to
    static const DiscretizationMethods myDiscretizationMethod = DiscretizationMethods::CCMpfa;

    // state the type for the corresponding cache and its filler
    using Cache = MpfaDarcysLawCache;
    using CacheFiller = MpfaDarcysLawCacheFiller;

    static Scalar flux(const Problem& problem,
                       const Element& element,
                       const FVElementGeometry& fvGeometry,
                       const ElementVolumeVariables& elemVolVars,
                       const SubControlVolumeFace& scvf,
                       const unsigned int phaseIdx,
                       const ElementFluxVariablesCache& elemFluxVarsCache)
    {
        static const bool gravity = GET_PARAM_FROM_GROUP(TypeTag, bool, Problem, EnableGravity);

        const auto& fluxVarsCache = elemFluxVarsCache[scvf];
        const auto& tij = fluxVarsCache.advectionTij();

        // Calculate the interface density for gravity evaluation
        const auto rho = interpolateDensity(fvGeometry, elemVolVars, scvf, fluxVarsCache, phaseIdx);

        // Variable for the flux to be computed
        Scalar scvfFlux(0.0);

        // index counter to get corresponding transmissibilities
        unsigned int i = 0;

        // add contributions from cell-centered unknowns
        for (const auto volVarIdx : fluxVarsCache.advectionVolVarsStencil())
        {
            const auto& volVars = elemVolVars[volVarIdx];
            Scalar h = volVars.pressure(phaseIdx);

            // if gravity is enabled, add gravitational acceleration
            if (gravity)
            {
                // gravitational acceleration in the center of the actual element
                const auto x = fvGeometry.scv(volVarIdx).center();
                const auto g = problem.gravityAtPos(x);
                h -= rho*(g*x);
            }

            scvfFlux += tij[i++]*h;
        }

        // add contributions from possible dirichlet boundary conditions
        for (const auto& d : fluxVarsCache.advectionDirichletData())
        {
            const auto& volVars = elemVolVars[d.volVarIndex()];
            Scalar h = volVars.pressure(phaseIdx);

            // maybe add gravitational acceleration
            if (gravity)
            {
                const auto x = d.ipGlobal();
                const auto g = problem.gravityAtPos(x);
                h -= rho*(g*x);
            }

            scvfFlux += tij[i++]*h;
        }

        // return the flux (maybe adjust the sign)
        return fluxVarsCache.advectionSwitchFluxSign() ? -scvfFlux : scvfFlux;
    }

private:
    static Scalar interpolateDensity(const FVElementGeometry& fvGeometry,
                                     const ElementVolumeVariables& elemVolVars,
                                     const SubControlVolumeFace& scvf,
                                     const FluxVariablesCache& fluxVarsCache,
                                     const unsigned int phaseIdx)
    {
        static const bool gravity = GET_PARAM_FROM_GROUP(TypeTag, bool, Problem, EnableGravity);

        if (!gravity)
            return Scalar(0.0);
        else
        {
            // use arithmetic mean of the densities around the scvf
            if (!scvf.boundary())
            {
                Scalar rho = elemVolVars[scvf.insideScvIdx()].density(phaseIdx);
                for (auto outsideIdx : scvf.outsideScvIndices())
                    rho += elemVolVars[outsideIdx].density(phaseIdx);
                return rho/(scvf.outsideScvIndices().size()+1);
            }
            else
                return elemVolVars[scvf.outsideScvIdx()].density(phaseIdx);
        }
    }
};

} // end namespace

#endif
