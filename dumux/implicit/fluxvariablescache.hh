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
 * \brief Base class for the flux variables
 */
#ifndef DUMUX_IMPLICIT_FLUXVARIABLESCACHE_HH
#define DUMUX_IMPLICIT_FLUXVARIABLESCACHE_HH

#include <dumux/implicit/properties.hh>

namespace Dumux
{

namespace Properties
{
NEW_PROP_TAG(NumPhases);
NEW_PROP_TAG(NumComponents);
}

/*!
 * \ingroup ImplicitModel
 * \brief The flux variables cache class
 *        stores the transmissibilities and stencils for the darcy flux calculation on a scv face
 */
template<class TypeTag, bool isBox>
class FluxVariablesCache
{};

// specialization for the Box Method
template<class TypeTag>
class FluxVariablesCache<TypeTag, true>
{
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using AdvectionType = typename GET_PROP_TYPE(TypeTag, AdvectionType);
    using Element = typename GridView::template Codim<0>::Entity;
    using IndexType = typename GridView::IndexSet::IndexType;
    using Stencil = std::vector<IndexType>;
    using TransmissibilityVector = std::vector<Scalar>;

public:
    void update(const Problem& problem,
                const Element& element,
                const SubControlVolumeFace &scvFace)
    {
        darcyStencil_ = AdvectionType::stencil(problem, scvFace);
        tij_ = AdvectionType::calculateTransmissibilities(problem, scvFace);
    }

    const Stencil& stencil() const
    { return darcyStencil_; }

    const Stencil& darcyStencil() const
    { return darcyStencil_; }

    const TransmissibilityVector& tij() const
    { return tij_; }

private:
    Stencil darcyStencil_;
    TransmissibilityVector tij_;
};

// specialization for cell centered methods
template<class TypeTag>
class FluxVariablesCache<TypeTag, false> : public FluxVariablesCache<TypeTag, true>
{
    using ParentType = FluxVariablesCache<TypeTag, true>;
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using VolumeVariables = typename GET_PROP_TYPE(TypeTag, VolumeVariables);
    using FluxVariables = typename GET_PROP_TYPE(TypeTag, FluxVariables);
    using Element = typename GridView::template Codim<0>::Entity;
    using IndexType = typename GridView::IndexSet::IndexType;
    using Stencil = std::vector<IndexType>;

public:
    void update(const Problem& problem,
                const Element& element,
                const SubControlVolumeFace &scvFace)
    {
        ParentType::update(problem, element, scvFace);
        FluxVariables fluxVars;
        stencil_ = fluxVars.computeFluxStencil(problem, scvFace);
        if (GET_PROP_VALUE(TypeTag, ConstantBoundaryConditions))
            boundaryVolVars_ = std::make_unique<VolumeVariables>(std::move(fluxVars.getBoundaryVolumeVariables(problem, element, scvFace)));
    }

    const Stencil& stencil() const
    { return stencil_; }

    const VolumeVariables& boundaryVolumeVariables() const
    { return *boundaryVolVars_.get(); }

private:
    Stencil stencil_;
    std::unique_ptr<VolumeVariables> boundaryVolVars_;
};

} // end namespace

#endif