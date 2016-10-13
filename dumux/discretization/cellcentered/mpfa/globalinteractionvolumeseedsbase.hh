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
 * \brief Base class for the global interaction volumes of the mpfa-o method.
 */
#ifndef DUMUX_DISCRETIZATION_MPFA_GLOBALINTERACTIONVOLUMESEEDS_BASE_HH
#define DUMUX_DISCRETIZATION_MPFA_GLOBALINTERACTIONVOLUMESEEDS_BASE_HH

#include <dumux/implicit/cellcentered/mpfa/properties.hh>

namespace Dumux
{
/*!
 * \ingroup MpfaO
 * \brief Base class for the creation and storage of the interaction volume seeds for mpfa methods.
 */
template<class TypeTag>
class CCMpfaGlobalInteractionVolumeSeedsBase
{
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using Helper = typename GET_PROP_TYPE(TypeTag, MpfaHelper);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using InteractionVolume = typename GET_PROP_TYPE(TypeTag, InteractionVolume);
    using InteractionVolumeSeed = typename InteractionVolume::Seed;
    using BoundaryInteractionVolume = typename GET_PROP_TYPE(TypeTag, BoundaryInteractionVolume);
    using BoundaryInteractionVolumeSeed = typename BoundaryInteractionVolume::Seed;
    using Element = typename GridView::template Codim<0>::Entity;

    using IndexType = typename GridView::IndexSet::IndexType;
    using LocalIndexType = typename InteractionVolume::LocalIndexType;

    static const int dim = GridView::dimension;
    static const int numEq = GET_PROP_VALUE(TypeTag, NumEq);

public:
    CCMpfaGlobalInteractionVolumeSeedsBase(const GridView gridView) : gridView_(gridView) {}

    // initializes the interaction volumes or the seeds
    void update(const Problem& problem)
    {
        problemPtr_ = &problem;
        seeds_.clear();
        boundarySeeds_.clear();

        // -1 indicates that the scvf has not been handled yet
        auto numScvf = problem_().model().globalFvGeometry().numScvf();
        scvfIndexMap_.resize(numScvf, -1);

        // detect and handle the boundary first
        initializeBoundarySeeds_();
        initializeInteriorSeeds_();
    }

    const InteractionVolumeSeed& seed(const SubControlVolumeFace& scvf) const
    { return seeds_[scvfIndexMap_[scvf.index()]]; }

    const BoundaryInteractionVolumeSeed& boundarySeed(const SubControlVolumeFace& scvf) const
    { return boundarySeeds_[scvfIndexMap_[scvf.index()]]; }

private:
    void initializeBoundarySeeds_()
    {
        boundarySeeds_.reserve(problem_().model().globalFvGeometry().numBoundaryScvf());
        IndexType boundarySeedIndex = 0;
        for (const auto& element : elements(gridView_))
        {
            if (!element.hasBoundaryIntersections())
                continue;

            auto fvGeometry = localView(problem_().model().globalFvGeometry());
            fvGeometry.bind(element);
            for (const auto& scvf : scvfs(fvGeometry))
            {
                // skip the rest if we already handled this face or if face is not on boundary
                if (scvfIndexMap_[scvf.index()] != -1 || !scvf.boundary())
                    continue;

                // the boundary interaction volume seed
                auto seed = Helper::makeBoundaryInteractionVolumeSeed(problem_(), element, fvGeometry, scvf);

                // update the index map entries for the global scv faces in the interaction volume
                for (const auto& localScvfSeed : seed.scvfSeeds())
                    for (const auto scvfIdxGlobal : localScvfSeed.globalScvfIndices())
                        scvfIndexMap_[scvfIdxGlobal] = boundarySeedIndex;

                // store interaction volume and increment counter
                boundarySeeds_.emplace_back(std::move(seed));
                boundarySeedIndex++;
            }
        }

        // shrink boundary seed vector to actual size
        boundarySeeds_.shrink_to_fit();
    }

    void initializeInteriorSeeds_()
    {
        const auto& globalFvGeometry = problem_().model().globalFvGeometry();
        seeds_.reserve(globalFvGeometry.numScvf() - globalFvGeometry.numBoundaryScvf());

        IndexType seedIndex = 0;
        for (const auto& element : elements(gridView_))
        {
            auto fvGeometry = localView(globalFvGeometry);
            fvGeometry.bind(element);
            for (const auto& scvf : scvfs(fvGeometry))
            {
                if (scvfIndexMap_[scvf.index()] != -1)
                    continue;

                // the inner interaction volume seed
                auto seed = Helper::makeInnerInteractionVolumeSeed(problem_(), element, fvGeometry, scvf);

                // update the index map entries for the global scv faces in the interaction volume
                for (const auto& localScvf : seed.scvfSeeds())
                    for (const auto scvfIdxGlobal : localScvf.globalScvfIndices())
                        scvfIndexMap_[scvfIdxGlobal] = seedIndex;

                // store interaction volume and increment counter
                seeds_.emplace_back(std::move(seed));
                seedIndex++;
            }
        }

        // shrink seed vector to actual size
        seeds_.shrink_to_fit();
    }

    const Problem& problem_() const
    { return *problemPtr_; }

    const Problem* problemPtr_;
    GridView gridView_;
    std::vector<IndexType> scvfIndexMap_;
    std::vector<InteractionVolumeSeed> seeds_;
    std::vector<BoundaryInteractionVolumeSeed> boundarySeeds_;
};
} // end namespace


#endif