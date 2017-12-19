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
 * \ingroup CCTpfaDiscretization
 * \brief The finite volume geometry (scvs and scvfs) for cell-centered TPFA models on a grid view
 *        This builds up the sub control volumes and sub control volume faces
 *        for each element of the grid partition.
 */
#ifndef DUMUX_DISCRETIZATION_CCTPFA_FV_GRID_GEOMETRY_HH
#define DUMUX_DISCRETIZATION_CCTPFA_FV_GRID_GEOMETRY_HH

#include <dune/common/version.hh>

#include <dumux/discretization/basefvgridgeometry.hh>
#include <dumux/discretization/cellcentered/tpfa/fvelementgeometry.hh>
#include <dumux/discretization/cellcentered/connectivitymap.hh>

namespace Dumux
{

/*!
 * \ingroup CCTpfaDiscretization
 * \brief The finite volume geometry (scvs and scvfs) for cell-centered TPFA models on a grid view
 *        This builds up the sub control volumes and sub control volume faces
 * \note This class is specialized for versions with and without caching the fv geometries on the grid view
 */
template<class TypeTag, bool EnableFVGridGeometryCache>
class CCTpfaFVGridGeometry
{};

/*!
 * \ingroup CCTpfaDiscretization
 * \brief The finite volume geometry (scvs and scvfs) for cell-centered TPFA models on a grid view
 *        This builds up the sub control volumes and sub control volume faces
 * \note For caching enabled we store the fv geometries for the whole grid view which is memory intensive but faster
 */
template<class TypeTag>
class CCTpfaFVGridGeometry<TypeTag, true> : public BaseFVGridGeometry<TypeTag>
{
    using ParentType = BaseFVGridGeometry<TypeTag>;
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using IndexType = typename GridView::IndexSet::IndexType;
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using ElementMapper = typename GET_PROP_TYPE(TypeTag, ElementMapper);
    using Element = typename GridView::template Codim<0>::Entity;
    using ConnectivityMap = CCSimpleConnectivityMap<TypeTag>;

    static const int dim = GridView::dimension;
    static const int dimWorld = GridView::dimensionworld;
    using CoordScalar = typename GridView::ctype;
    using GlobalPosition = Dune::FieldVector<CoordScalar, dimWorld>;

    //! The local class needs access to the scv, scvfs and the fv element geometry
    //! as they are globally cached
    friend typename GET_PROP_TYPE(TypeTag, FVElementGeometry);

public:
    //! Constructor
    CCTpfaFVGridGeometry(const GridView& gridView)
    : ParentType(gridView)
    {}

    //! the element mapper is the dofMapper
    //! this is convenience to have better chance to have the same main files for box/tpfa/mpfa...
    const ElementMapper& dofMapper() const
    { return this->elementMapper(); }

    //! The total number of sub control volumes
    std::size_t numScv() const
    {
        return scvs_.size();
    }

    //! The total number of sub control volume faces
    std::size_t numScvf() const
    {
        return scvfs_.size();
    }

    //! The total number of boundary sub control volume faces
    std::size_t numBoundaryScvf() const
    {
        return numBoundaryScvf_;
    }

    //! The total number of degrees of freedom
    std::size_t numDofs() const
    { return this->gridView().size(0); }

    //! Get an element from a sub control volume contained in it
    Element element(const SubControlVolume& scv) const
    { return this->elementMap()[scv.elementIndex()]; }

    //! Get an element from a global element index
    Element element(IndexType eIdx) const
    { return this->elementMap()[eIdx]; }

    //! update all fvElementGeometries (do this again after grid adaption)
    void update()
    {
        ParentType::update();

        // clear containers (necessary after grid refinement)
        scvs_.clear();
        scvfs_.clear();
        scvfIndicesOfScv_.clear();
        flipScvfIndices_.clear();

        // determine size of containers
        IndexType numScvs = numDofs();
        IndexType numScvf = 0;
        for (const auto& element : elements(this->gridView()))
            numScvf += element.subEntities(1);

        // reserve memory
        scvs_.resize(numScvs);
        scvfs_.reserve(numScvf);
        scvfIndicesOfScv_.resize(numScvs);

        // Build the scvs and scv faces
        IndexType scvfIdx = 0;
        numBoundaryScvf_ = 0;
        for (const auto& element : elements(this->gridView()))
        {
            const auto eIdx = this->elementMapper().index(element);
            scvs_[eIdx] = SubControlVolume(element.geometry(), eIdx);

            // the element-wise index sets for finite volume geometry
            std::vector<IndexType> scvfsIndexSet;
            scvfsIndexSet.reserve(element.subEntities(1));

            // for network grids there might be multiple intersection with the same geometryInInside
            // we indentify those by the indexInInside for now (assumes conforming grids at branching facets)
            std::vector<std::vector<IndexType>> outsideIndices;
            if (dim < dimWorld)
            {
                outsideIndices.resize(element.subEntities(1));
                for (const auto& intersection : intersections(this->gridView(), element))
                {
                    if (intersection.neighbor())
                    {
                        const auto& outside = intersection.outside();
                        const auto nIdx = this->elementMapper().index(outside);
                        outsideIndices[intersection.indexInInside()].push_back(nIdx);
                    }

                }
            }

            for (const auto& intersection : intersections(this->gridView(), element))
            {
                // TODO check if intersection is on interior boundary
                const auto isInteriorBoundary = false;

                // inner sub control volume faces
                if (intersection.neighbor() && !isInteriorBoundary)
                {
                    if (dim == dimWorld)
                    {
                        const auto nIdx = this->elementMapper().index(intersection.outside());
                        scvfs_.emplace_back(intersection,
                                            intersection.geometry(),
                                            scvfIdx,
                                            std::vector<IndexType>({eIdx, nIdx}),
                                            false);
                        scvfsIndexSet.push_back(scvfIdx++);
                    }
                    // this is for network grids
                    // (will be optimized away of dim == dimWorld)
                    else
                    {
                        auto indexInInside = intersection.indexInInside();
                        // check if we already handled this facet
                        if (outsideIndices[indexInInside].empty())
                            continue;
                        else
                        {
                            std::vector<IndexType> scvIndices({eIdx});
                            scvIndices.insert(scvIndices.end(), outsideIndices[indexInInside].begin(), outsideIndices[indexInInside].end());
                            scvfs_.emplace_back(intersection,
                                                intersection.geometry(),
                                                scvfIdx,
                                                scvIndices,
                                                false);
                            scvfsIndexSet.push_back(scvfIdx++);
                            outsideIndices[indexInInside].clear();
                        }
                    }
                }
                // boundary sub control volume faces
                else if (intersection.boundary() || isInteriorBoundary)
                {
                    scvfs_.emplace_back(intersection,
                                        intersection.geometry(),
                                        scvfIdx,
                                        std::vector<IndexType>({eIdx, this->gridView().size(0) + numBoundaryScvf_++}),
                                        true);
                    scvfsIndexSet.push_back(scvfIdx++);
                }
            }

            // Save the scvf indices belonging to this scv to build up fv element geometries fast
            scvfIndicesOfScv_[eIdx] = scvfsIndexSet;
        }

        // Make the flip index set for network and surface grids
        if (dim < dimWorld)
        {
            flipScvfIndices_.resize(scvfs_.size());
            for (auto&& scvf : scvfs_)
            {
                if (scvf.boundary())
                    continue;

                flipScvfIndices_[scvf.index()].resize(scvf.numOutsideScvs());
                const auto insideScvIdx = scvf.insideScvIdx();
                // check which outside scvf has the insideScvIdx index in its outsideScvIndices
                for (unsigned int i = 0; i < scvf.numOutsideScvs(); ++i)
                    flipScvfIndices_[scvf.index()][i] = findFlippedScvfIndex_(insideScvIdx, scvf.outsideScvIdx(i));
            }
        }

        // build the connectivity map for an effecient assembly
        connectivityMap_.update(*this);
    }

    //! Get a sub control volume with a global scv index
    const SubControlVolume& scv(IndexType scvIdx) const
    {
        return scvs_[scvIdx];
    }

    //! Get a sub control volume face with a global scvf index
    const SubControlVolumeFace& scvf(IndexType scvfIdx) const
    {
        return scvfs_[scvfIdx];
    }

    //! Get the scvf on the same face but from the other side
    //! Note that e.g. the normals might be different in the case of surface grids
    const SubControlVolumeFace& flipScvf(IndexType scvfIdx, unsigned int outsideScvfIdx = 0) const
    {
        return scvfs_[flipScvfIndices_[scvfIdx][outsideScvfIdx]];
    }

    //! Get the sub control volume face indices of an scv by global index
    const std::vector<IndexType>& scvfIndicesOfScv(IndexType scvIdx) const
    {
        return scvfIndicesOfScv_[scvIdx];
    }

    /*!
     * \brief Returns the connectivity map of which dofs have derivatives with respect
     *        to a given dof.
     */
    const ConnectivityMap &connectivityMap() const
    { return connectivityMap_; }

private:
    // find the scvf that has insideScvIdx in its outsideScvIdx list and outsideScvIdx as its insideScvIdx
    IndexType findFlippedScvfIndex_(IndexType insideScvIdx, IndexType outsideScvIdx)
    {
        // go over all potential scvfs of the outside scv
        for (auto outsideScvfIndex : scvfIndicesOfScv_[outsideScvIdx])
        {
            const auto& outsideScvf = this->scvf(outsideScvfIndex);
            for (int j = 0; j < outsideScvf.numOutsideScvs(); ++j)
                if (outsideScvf.outsideScvIdx(j) == insideScvIdx)
                    return outsideScvf.index();
        }

        DUNE_THROW(Dune::InvalidStateException, "No flipped version of this scvf found!");
    }

    //! connectivity map for efficient assembly
    ConnectivityMap connectivityMap_;

    //! containers storing the global data
    std::vector<SubControlVolume> scvs_;
    std::vector<SubControlVolumeFace> scvfs_;
    std::vector<std::vector<IndexType>> scvfIndicesOfScv_;
    IndexType numBoundaryScvf_;

    //! needed for embedded surface and network grids (dim < dimWorld)
    std::vector<std::vector<IndexType>> flipScvfIndices_;
};

/*!
 * \ingroup CCTpfaDiscretization
 * \brief The finite volume geometry (scvs and scvfs) for cell-centered TPFA models on a grid view
 *        This builds up the sub control volumes and sub control volume faces
 * \note For caching disabled we store only some essential index maps to build up local systems on-demand in
 *       the corresponding FVElementGeometry
 */
template<class TypeTag>
class CCTpfaFVGridGeometry<TypeTag, false>  : public BaseFVGridGeometry<TypeTag>
{
    using ParentType = BaseFVGridGeometry<TypeTag>;
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using IndexType = typename GridView::IndexSet::IndexType;
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using ElementMapper = typename GET_PROP_TYPE(TypeTag, ElementMapper);
    using Element = typename GridView::template Codim<0>::Entity;
    using ConnectivityMap = CCSimpleConnectivityMap<TypeTag>;

    static const int dim = GridView::dimension;
    static const int dimWorld = GridView::dimensionworld;

    using CoordScalar = typename GridView::ctype;
    using GlobalPosition = Dune::FieldVector<CoordScalar, dimWorld>;

public:
    //! Constructor
    CCTpfaFVGridGeometry(const GridView& gridView)
    : ParentType(gridView)
    {}

    //! the element mapper is the dofMapper
    //! this is convenience to have better chance to have the same main files for box/tpfa/mpfa...
    const ElementMapper& dofMapper() const
    { return this->elementMapper(); }

    //! The total number of sub control volumes
    std::size_t numScv() const
    {
        return numScvs_;
    }

    //! The total number of sub control volume faces
    std::size_t numScvf() const
    {
        return numScvf_;
    }

    //! The total number of boundary sub control volume faces
    std::size_t numBoundaryScvf() const
    {
        return numBoundaryScvf_;
    }

    //! The total number of degrees of freedom
    std::size_t numDofs() const
    { return this->gridView().size(0); }

    // Get an element from a sub control volume contained in it
    Element element(const SubControlVolume& scv) const
    { return this->elementMap()[scv.elementIndex()]; }

    // Get an element from a global element index
    Element element(IndexType eIdx) const
    { return this->elementMap()[eIdx]; }

    //! update all fvElementGeometries (do this again after grid adaption)
    void update()
    {
        ParentType::update();

        // clear local data
        scvfIndicesOfScv_.clear();
        neighborVolVarIndices_.clear();

        // reserve memory or resize the containers
        numScvs_ = numDofs();
        numScvf_ = 0;
        numBoundaryScvf_ = 0;
        scvfIndicesOfScv_.resize(numScvs_);
        neighborVolVarIndices_.resize(numScvs_);

        // Build the SCV and SCV face
        for (const auto& element : elements(this->gridView()))
        {
            const auto eIdx = this->elementMapper().index(element);

            // the element-wise index sets for finite volume geometry
            auto numLocalFaces = element.subEntities(1);
            std::vector<IndexType> scvfsIndexSet;
            std::vector<std::vector<IndexType>> neighborVolVarIndexSet;
            scvfsIndexSet.reserve(numLocalFaces);
            neighborVolVarIndexSet.reserve(numLocalFaces);

            // for network grids there might be multiple intersection with the same geometryInInside
            // we indentify those by the indexInInside for now (assumes conforming grids at branching facets)
            std::vector<std::vector<IndexType>> outsideIndices;
            if (dim < dimWorld)
            {
                outsideIndices.resize(numLocalFaces);
                for (const auto& intersection : intersections(this->gridView(), element))
                {
                    if (intersection.neighbor())
                    {
                        const auto& outside = intersection.outside();
                        const auto nIdx = this->elementMapper().index(outside);
                        outsideIndices[intersection.indexInInside()].push_back(nIdx);
                    }

                }
            }

            for (const auto& intersection : intersections(this->gridView(), element))
            {
                // TODO check if intersection is on interior boundary
                const auto isInteriorBoundary = false;

                // inner sub control volume faces
                if (intersection.neighbor() && !isInteriorBoundary)
                {
                    if (dim == dimWorld)
                    {
                        scvfsIndexSet.push_back(numScvf_++);
                        const auto nIdx = this->elementMapper().index(intersection.outside());
                        neighborVolVarIndexSet.push_back({nIdx});
                    }
                    // this is for network grids
                    // (will be optimized away of dim == dimWorld)
                    else
                    {
                        auto indexInInside = intersection.indexInInside();
                        // check if we already handled this facet
                        if (outsideIndices[indexInInside].empty())
                            continue;
                        else
                        {
                            scvfsIndexSet.push_back(numScvf_++);
                            neighborVolVarIndexSet.push_back(outsideIndices[indexInInside]);
                            outsideIndices[indexInInside].clear();
                        }
                    }
                }
                // boundary sub control volume faces
                else if (intersection.boundary() || isInteriorBoundary)
                {
                    scvfsIndexSet.push_back(numScvf_++);
                    neighborVolVarIndexSet.push_back({numScvs_ + numBoundaryScvf_++});
                }
            }

            // store the sets of indices in the data container
            scvfIndicesOfScv_[eIdx] = scvfsIndexSet;
            neighborVolVarIndices_[eIdx] = neighborVolVarIndexSet;
        }

        // build the connectivity map for an effecient assembly
        connectivityMap_.update(*this);
    }

    const std::vector<IndexType>& scvfIndicesOfScv(IndexType scvIdx) const
    { return scvfIndicesOfScv_[scvIdx]; }

    //! Return the neighbor volVar indices for all scvfs in the scv with index scvIdx
    const std::vector<std::vector<IndexType>>& neighborVolVarIndices(IndexType scvIdx) const
    { return neighborVolVarIndices_[scvIdx]; }

    /*!
     * \brief Returns the connectivity map of which dofs have derivatives with respect
     *        to a given dof.
     */
    const ConnectivityMap &connectivityMap() const
    { return connectivityMap_; }

private:

    //! Information on the global number of geometries
    IndexType numScvs_;
    IndexType numScvf_;
    IndexType numBoundaryScvf_;

    //! connectivity map for efficient assembly
    ConnectivityMap connectivityMap_;

    //! vectors that store the global data
    std::vector<std::vector<IndexType>> scvfIndicesOfScv_;
    std::vector<std::vector<std::vector<IndexType>>> neighborVolVarIndices_;
};

} // end namespace Dumux

#endif
