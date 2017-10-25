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
 * \brief Global flux variable cache
 */
#ifndef DUMUX_DISCRETIZATION_BOX_ELEMENT_FLUXVARSCACHE_HH
#define DUMUX_DISCRETIZATION_BOX_ELEMENT_FLUXVARSCACHE_HH

#include <dumux/implicit/properties.hh>

namespace Dumux
{

/*!
 * \ingroup ImplicitModel
 * \brief Base class for the flux variables cache vector, we store one cache per face
 */
template<class TypeTag, bool EnableGlobalFluxVariablesCache>
class BoxElementFluxVariablesCache
{};

/*!
 * \ingroup ImplicitModel
 * \brief Base class for the flux variables cache vector, we store one cache per face
 */
template<class TypeTag>
class BoxElementFluxVariablesCache<TypeTag, true>
{
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using IndexType = typename GridView::IndexSet::IndexType;
    using Element = typename GridView::template Codim<0>::Entity;
    using FluxVariablesCache = typename GET_PROP_TYPE(TypeTag, FluxVariablesCache);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using ElementVolumeVariables = typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables);
    using GlobalFluxVariablesCache = typename GET_PROP_TYPE(TypeTag, GlobalFluxVariablesCache);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);

public:
    BoxElementFluxVariablesCache(const GlobalFluxVariablesCache& global)
    : globalFluxVarsCachePtr_(&global) {}

    // Function is called by the BoxLocalJacobian prior to flux calculations on the element.
    // We assume the FVGeometries to be bound at this point
    void bind(const Element& element,
              const FVElementGeometry& fvGeometry,
              const ElementVolumeVariables& elemVolVars)
    {
        bindElement(element, fvGeometry, elemVolVars);
    }

    void bindElement(const Element& element,
                     const FVElementGeometry& fvGeometry,
                     const ElementVolumeVariables& elemVolVars)
    {
        eIdx_ = fvGeometry.fvGridGeometry().elementMapper().index(element);
    }

    void bindScvf(const Element& element,
                  const FVElementGeometry& fvGeometry,
                  const ElementVolumeVariables& elemVolVars,
                  const SubControlVolumeFace& scvf)
    {
        bindElement(element, fvGeometry, elemVolVars);
    }

    // access operator
    const FluxVariablesCache& operator [](const SubControlVolumeFace& scvf) const
    { return globalFluxVarsCache().cache(eIdx_, scvf.index()); }

    //! The global object we are a restriction of
    const GlobalFluxVariablesCache& globalFluxVarsCache() const
    {  return *globalFluxVarsCachePtr_; }

private:
    const GlobalFluxVariablesCache* globalFluxVarsCachePtr_;
    // currently bound element
    IndexType eIdx_;
};

/*!
 * \ingroup ImplicitModel
 * \brief Base class for the flux variables cache vector, we store one cache per face
 */
template<class TypeTag>
class BoxElementFluxVariablesCache<TypeTag, false>
{
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using IndexType = typename GridView::IndexSet::IndexType;
    using Element = typename GridView::template Codim<0>::Entity;
    using FluxVariablesCache = typename GET_PROP_TYPE(TypeTag, FluxVariablesCache);
    using ElementVolumeVariables = typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using GlobalFluxVariablesCache = typename GET_PROP_TYPE(TypeTag, GlobalFluxVariablesCache);

public:
    BoxElementFluxVariablesCache(const GlobalFluxVariablesCache& global)
    : globalFluxVarsCachePtr_(&global) {}

    // Function is called by the BoxLocalJacobian prior to flux calculations on the element.
    // We assume the FVGeometries to be bound at this point
    void bind(const Element& element,
              const FVElementGeometry& fvGeometry,
              const ElementVolumeVariables& elemVolVars)
    {
        bindElement(element, fvGeometry, elemVolVars);
    }

    void bindElement(const Element& element,
                     const FVElementGeometry& fvGeometry,
                     const ElementVolumeVariables& elemVolVars)
    {
        // temporary resizing of the cache
        fluxVarsCache_.resize(fvGeometry.numScvf());
        for (auto&& scvf : scvfs(fvGeometry))
            (*this)[scvf].update(globalFluxVarsCache().problem(), element, fvGeometry, elemVolVars, scvf);
    }

    void bindScvf(const Element& element,
                  const FVElementGeometry& fvGeometry,
                  const ElementVolumeVariables& elemVolVars,
                  const SubControlVolumeFace& scvf)
    {
        fluxVarsCache_.resize(fvGeometry.numScvf());
        (*this)[scvf].update(globalFluxVarsCache().problem(), element, fvGeometry, elemVolVars, scvf);
    }

    // access operator
    const FluxVariablesCache& operator [](const SubControlVolumeFace& scvf) const
    { return fluxVarsCache_[scvf.index()]; }

    // access operator
    FluxVariablesCache& operator [](const SubControlVolumeFace& scvf)
    { return fluxVarsCache_[scvf.index()]; }

    //! The global object we are a restriction of
    const GlobalFluxVariablesCache& globalFluxVarsCache() const
    {  return *globalFluxVarsCachePtr_; }

private:
    const GlobalFluxVariablesCache* globalFluxVarsCachePtr_;
    std::vector<FluxVariablesCache> fluxVarsCache_;
};

} // end namespace Dumux

#endif
