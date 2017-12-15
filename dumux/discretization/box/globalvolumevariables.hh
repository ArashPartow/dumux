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
 * \brief The global volume variables class
 */
#ifndef DUMUX_DISCRETIZATION_BOX_GLOBAL_VOLUMEVARIABLES_HH
#define DUMUX_DISCRETIZATION_BOX_GLOBAL_VOLUMEVARIABLES_HH

#include <dumux/discretization/box/elementvolumevariables.hh>

namespace Dumux
{

/*!
 * \ingroup ImplicitModel
 * \brief Base class for the volume variables vector
 */
template<class TypeTag, bool enableGridVolVarsCache>
class BoxGridVolumeVariables
{};

// specialization in case of storing the volume variables
template<class TypeTag>
class BoxGridVolumeVariables<TypeTag,/*enableGlobalVolVarCache*/true>
{
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using SolutionVector = typename GET_PROP_TYPE(TypeTag, SolutionVector);
    using VolumeVariables = typename GET_PROP_TYPE(TypeTag, VolumeVariables);
    using ElementVolumeVariables = typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables);
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using IndexType = typename GridView::IndexSet::IndexType;
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using FVGridGeometry = typename GET_PROP_TYPE(TypeTag, FVGridGeometry);
    using ElementSolutionVector = typename GET_PROP_TYPE(TypeTag, ElementSolutionVector);

    static const int dim = GridView::dimension;
    using Element = typename GridView::template Codim<0>::Entity;

public:
    BoxGridVolumeVariables(const Problem& problem) : problemPtr_(&problem) {}

    void update(const FVGridGeometry& fvGridGeometry, const SolutionVector& sol)
    {
        volumeVariables_.resize(fvGridGeometry.gridView().size(0));
        for (const auto& element : elements(fvGridGeometry.gridView()))
        {
            auto eIdx = fvGridGeometry.elementMapper().index(element);

            auto fvGeometry = localView(fvGridGeometry);
            fvGeometry.bindElement(element);

            // get the element solution
            ElementSolutionVector elemSol(element, sol, fvGeometry);

            // update the volvars of the element
            volumeVariables_[eIdx].resize(fvGeometry.numScv());
            for (auto&& scv : scvs(fvGeometry))
                volumeVariables_[eIdx][scv.indexInElement()].update(elemSol, problem(), element, scv);
        }
    }

    /*!
     * \brief Return a local restriction of this global object
     *        The local object is only functional after calling its bind/bindElement method
     *        This is a free function that will be found by means of ADL
     */
    friend inline ElementVolumeVariables localView(const BoxGridVolumeVariables& global)
    { return ElementVolumeVariables(global); }

    const VolumeVariables& volVars(const IndexType eIdx, const IndexType scvIdx) const
    { return volumeVariables_[eIdx][scvIdx]; }

    VolumeVariables& volVars(const IndexType eIdx, const IndexType scvIdx)
    { return volumeVariables_[eIdx][scvIdx]; }

    const Problem& problem() const
    { return *problemPtr_; }

private:
    const Problem* problemPtr_;
    std::vector<std::vector<VolumeVariables>> volumeVariables_;
};


// Specialization when the current volume variables are not stored
template<class TypeTag>
class BoxGridVolumeVariables<TypeTag, /*enableGlobalVolVarCache*/false>
{
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using SolutionVector = typename GET_PROP_TYPE(TypeTag, SolutionVector);
    using VolumeVariables = typename GET_PROP_TYPE(TypeTag, VolumeVariables);
    using ElementVolumeVariables = typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables);
    using FVGridGeometry = typename GET_PROP_TYPE(TypeTag, FVGridGeometry);

public:
    BoxGridVolumeVariables(const Problem& problem) : problemPtr_(&problem) {}

    void update(const FVGridGeometry& fvGridGeometry, const SolutionVector& sol) {}

    /*!
     * \brief Return a local restriction of this global object
     *        The local object is only functional after calling its bind/bindElement method
     *        This is a free function that will be found by means of ADL
     */
    friend inline ElementVolumeVariables localView(const BoxGridVolumeVariables& global)
    { return ElementVolumeVariables(global); }

    const Problem& problem() const
    { return *problemPtr_;}

private:
    const Problem* problemPtr_;
};

} // end namespace Dumux

#endif
