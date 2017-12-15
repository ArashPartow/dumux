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
 * \brief The global object of flux var caches
 */
#ifndef DUMUX_DISCRETIZATION_CCTPFA_GLOBAL_FLUXVARSCACHE_HH
#define DUMUX_DISCRETIZATION_CCTPFA_GLOBAL_FLUXVARSCACHE_HH

#include <dumux/discretization/cellcentered/tpfa/fluxvariablescachefiller.hh>

namespace Dumux
{

/*!
 * \ingroup ImplicitModel
 * \brief Base class for the flux variables cache vector, we store one cache per face
 */
template<class TypeTag, bool EnableGridFluxVariablesCache>
class CCTpfaGridFluxVariablesCache;

/*!
 * \ingroup ImplicitModel
 * \brief Spezialization when caching globally
 */
template<class TypeTag>
class CCTpfaGridFluxVariablesCache<TypeTag, true>
{
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using FVGridGeometry = typename GET_PROP_TYPE(TypeTag, FVGridGeometry);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using SolutionVector = typename GET_PROP_TYPE(TypeTag, SolutionVector);
    using GridVolumeVariables = typename GET_PROP_TYPE(TypeTag, GridVolumeVariables);
    using ElementVolumeVariables = typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables);
    using IndexType = typename GridView::IndexSet::IndexType;
    using Element = typename GridView::template Codim<0>::Entity;
    using FluxVariablesCache = typename GET_PROP_TYPE(TypeTag, FluxVariablesCache);
    using ElementFluxVariablesCache = typename GET_PROP_TYPE(TypeTag, ElementFluxVariablesCache);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using FluxVariablesCacheFiller = CCTpfaFluxVariablesCacheFiller<TypeTag>;

public:
    // The constructor
    CCTpfaGridFluxVariablesCache(const Problem& problem) : problemPtr_(&problem) {}

    // When global caching is enabled, precompute transmissibilities and stencils for all the scv faces
    void update(const FVGridGeometry& fvGridGeometry,
                const GridVolumeVariables& gridVolVars,
                const SolutionVector& sol,
                bool forceUpdate = false)
    {
        // only do the update if fluxes are solution dependent or if update is forced
        if (FluxVariablesCacheFiller::isSolDependent || forceUpdate)
        {
            // instantiate helper class to fill the caches
            FluxVariablesCacheFiller filler(problem());

            fluxVarsCache_.resize(fvGridGeometry.numScvf());
            for (const auto& element : elements(fvGridGeometry.gridView()))
            {
                // Prepare the geometries within the elements of the stencil
                auto fvGeometry = localView(fvGridGeometry);
                fvGeometry.bind(element);

                auto elemVolVars = localView(gridVolVars);
                elemVolVars.bind(element, fvGeometry, sol);

                for (auto&& scvf : scvfs(fvGeometry))
                {
                    filler.fill(*this, fluxVarsCache_[scvf.index()], element, fvGeometry, elemVolVars, scvf, forceUpdate);
                }
            }
        }
    }

    void updateElement(const Element& element,
                       const FVElementGeometry& fvGeometry,
                       const ElementVolumeVariables& elemVolVars)
    {
        if (FluxVariablesCacheFiller::isSolDependent)
        {
            const auto globalI = fvGeometry.fvGridGeometry().elementMapper().index(element);

            // instantiate filler class
            FluxVariablesCacheFiller filler(problem());

            // update the caches inside this element
            for (const auto& scvf : scvfs(fvGeometry))
                filler.fill(*this, fluxVarsCache_[scvf.index()], element, fvGeometry, elemVolVars, scvf);

            // update the caches in the neighbors
            for (const auto& dataJ : fvGeometry.fvGridGeometry().connectivityMap()[globalI])
            {
                const auto elementJ = fvGeometry.fvGridGeometry().element(dataJ.globalJ);
                for (const auto scvfIdxJ : dataJ.scvfsJ)
                {
                    const auto& scvfJ = fvGeometry.scvf(scvfIdxJ);
                    filler.fill(*this, fluxVarsCache_[scvfJ.index()], elementJ, fvGeometry, elemVolVars, scvfJ);
                }
            }
        }
    }

    // access operators in the case of caching
    const FluxVariablesCache& operator [](const SubControlVolumeFace& scvf) const
    { return fluxVarsCache_[scvf.index()]; }

    FluxVariablesCache& operator [](const SubControlVolumeFace& scvf)
    { return fluxVarsCache_[scvf.index()]; }

    const Problem& problem() const
    { return *problemPtr_; }

    /*!
     * \brief Return a local restriction of this global object
     *        The local object is only functional after calling its bind/bindElement method
     *        This is a free function that will be found by means of ADL
     */
    friend inline ElementFluxVariablesCache localView(const CCTpfaGridFluxVariablesCache& global)
    { return ElementFluxVariablesCache(global); }

private:
    const Problem* problemPtr_;

    std::vector<FluxVariablesCache> fluxVarsCache_;
    std::vector<IndexType> globalScvfIndices_;
};

/*!
 * \ingroup ImplicitModel
 * \brief Spezialization when not using global caching
 */
template<class TypeTag>
class CCTpfaGridFluxVariablesCache<TypeTag, false>
{
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Element = typename GridView::template Codim<0>::Entity;
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using ElementFluxVariablesCache = typename GET_PROP_TYPE(TypeTag, ElementFluxVariablesCache);
    using FVGridGeometry = typename GET_PROP_TYPE(TypeTag, FVGridGeometry);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using SolutionVector = typename GET_PROP_TYPE(TypeTag, SolutionVector);
    using GridVolumeVariables = typename GET_PROP_TYPE(TypeTag, GridVolumeVariables);
    using ElementVolumeVariables = typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables);

public:
    // The constructor
    CCTpfaGridFluxVariablesCache(const Problem& problem) : problemPtr_(&problem) {}

    // When global flux variables caching is disabled, we don't need to update the cache
    void update(const FVGridGeometry& fvGridGeometry,
                const GridVolumeVariables& gridVolVars,
                const SolutionVector& sol,
                bool forceUpdate = false) {}

    void updateElement(const Element& element,
                       const FVElementGeometry& fvGeometry,
                       const ElementVolumeVariables& elemVolVars) {}

    const Problem& problem() const
    { return *problemPtr_; }

    /*!
     * \brief Return a local restriction of this global object
     *        The local object is only functional after calling its bind/bindElement method
     *        This is a free function that will be found by means of ADL
     */
    friend inline ElementFluxVariablesCache localView(const CCTpfaGridFluxVariablesCache& global)
    { return ElementFluxVariablesCache(global); }

private:
    const Problem* problemPtr_;
};

} // end namespace

#endif
