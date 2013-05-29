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
 * \brief An assembler for the global Jacobian matrix for models using the cell centered discretization.
 */
#ifndef DUMUX_CC_ASSEMBLER_HH
#define DUMUX_CC_ASSEMBLER_HH

#include <dumux/implicit/common/implicitassembler.hh>

namespace Dumux {

/*!
 * \ingroup CCModel
 * \brief An assembler for the global Jacobian matrix for models using the cell centered discretization.
 */
template<class TypeTag>
class CCAssembler : public ImplicitAssembler<TypeTag>
{
    typedef ImplicitAssembler<TypeTag> ParentType;
    friend class ImplicitAssembler<TypeTag>;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, VertexMapper) VertexMapper;
    typedef typename GET_PROP_TYPE(TypeTag, JacobianMatrix) JacobianMatrix;
    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    enum{ dim = GridView::dimension };
    typedef typename GridView::IntersectionIterator IntersectionIterator;

public:
    CCAssembler(): ParentType() {}

private:
    // copying the jacobian assembler is not a good idea
    CCAssembler(const CCAssembler &);

    /*!
     * \brief Determine the colors of elements for partial
     *        reassembly given a relative tolerance.
     *
     * The following approach is used:
     *
     * - Set all elements to 'green'
     * - Mark all elements as 'red' which exhibit an relative error above
     *   the tolerance
     * - Mark all neighbors of 'red' elements also 'red'
     *
     * \param relTol The relative error below which an element won't be
     *               reassembled. Note that this specifies the
     *               worst-case relative error between the last
     *               linearization point and the current solution and
     *               _not_ the delta vector of the Newton iteration!
     */
    void computeColors_(Scalar relTol) 
    {
        if (!this->enablePartialReassemble_())
            return;

        ElementIterator eIt = this->gridView_().template begin<0>();
        ElementIterator eEndIt = this->gridView_().template end<0>();

        // mark the red elements and update the tolerance of the
        // linearization which actually will get achieved
        this->nextReassembleAccuracy_ = 0;
        for (; eIt != eEndIt; ++eIt) {
            int elemIdx = this->elementMapper_().map(*eIt);
            if (this->delta_[elemIdx] > relTol)
            {
                // mark element as red if discrepancy is larger than
                // the relative tolerance
                this->elementColor_[elemIdx] = ParentType::Red;
            }
            else
            {
                this->elementColor_[elemIdx] = ParentType::Green;
                this->nextReassembleAccuracy_ =
                    std::max(this->nextReassembleAccuracy_, this->delta_[elemIdx]);
            }
        }

        // mark the neighbors also red
        eIt = this->gridView_().template begin<0>();
        for (; eIt != eEndIt; ++eIt) {
            int elemIdx = this->elementMapper_().map(*eIt);
            if (this->elementColor_[elemIdx] == ParentType::Red) 
                continue; // element is red already!

            if (this->delta_[elemIdx] > relTol)
            {
                // also mark the neighbors 
               IntersectionIterator endIsIt = this->gridView_().iend(*eIt);
               for (IntersectionIterator isIt = this->gridView_().ibegin(*eIt); isIt != endIsIt; ++isIt)
               {
                   if (isIt->neighbor())
                   {
                       int neighborIdx = this->elementMapper_().map(*isIt->outside());
                       this->elementColor_[neighborIdx] = ParentType::Red;
                   }
               }
            }
        }
        
        // set the discrepancy of the red elements to zero
        for (unsigned int i = 0; i < this->delta_.size(); i++)
            if (this->elementColor_[i] == ParentType::Red)
                this->delta_[i] = 0;
    }
    
    // Construct the BCRS matrix for the global jacobian
    void createMatrix_()
    {
        int nElems = this->gridView_().size(0);

        // allocate raw matrix
        this->matrix_ = Dune::make_shared<JacobianMatrix>(nElems, nElems, JacobianMatrix::random);

        // find out the global indices of the neighboring elements of
        // each element
        typedef std::set<int> NeighborSet;
        std::vector<NeighborSet> neighbors(nElems);
        ElementIterator eIt = this->gridView_().template begin<0>();
        const ElementIterator eEndIt = this->gridView_().template end<0>();
        for (; eIt != eEndIt; ++eIt) {
            const Element &elem = *eIt;
            
            int globalI = this->elementMapper_().map(elem);
            neighbors[globalI].insert(globalI);

            // if the element is ghost,
            // all dofs just contain main-diagonal entries
            //if (elem.partitionType() == Dune::GhostEntity)
            //    continue;

            // loop over all neighbors
            IntersectionIterator isIt = this->gridView_().ibegin(elem);
            const IntersectionIterator &isEndIt = this->gridView_().iend(elem);
            for (; isIt != isEndIt; ++isIt)
            {
                if (isIt->neighbor())
                {
                    int globalJ = this->elementMapper_().map(*(isIt->outside()));
                    neighbors[globalI].insert(globalJ);
                }
            }
        }

        // allocate space for the rows of the matrix
        for (int i = 0; i < nElems; ++i) {
            this->matrix_->setrowsize(i, neighbors[i].size());
        }
        this->matrix_->endrowsizes();

        // fill the rows with indices. each element talks to all of its
        // neighbors and itself.
        for (int i = 0; i < nElems; ++i) {
            typename NeighborSet::iterator nIt = neighbors[i].begin();
            typename NeighborSet::iterator nEndIt = neighbors[i].end();
            for (; nIt != nEndIt; ++nIt) {
                this->matrix_->addindex(i, *nIt);
            }
        }
        this->matrix_->endindices();
    }
    
    // assemble a non-ghost element
    void assembleElement_(const Element &elem)
    {
        if (this->enablePartialReassemble_()) {
            int globalElemIdx = this->model_().elementMapper().map(elem);
            if (this->elementColor_[globalElemIdx] == ParentType::Green) {
                ++this->greenElems_;

                assembleGreenElement_(elem);
                return;
            }
        }

        this->model_().localJacobian().assemble(elem);

        int globalI = this->elementMapper_().map(elem);

        // update the right hand side
        this->residual_[globalI] = this->model_().localJacobian().residual(0);
        for (int j = 0; j < this->residual_[globalI].dimension; ++j)
            assert(std::isfinite(this->residual_[globalI][j]));
        if (this->enableJacobianRecycling_()) {
            this->storageTerm_[globalI] +=
                this->model_().localJacobian().storageTerm(0);
        }

        if (this->enableJacobianRecycling_())
            this->storageJacobian_[globalI] +=
                this->model_().localJacobian().storageJacobian(0);

        // update the diagonal entry 
        (*this->matrix_)[globalI][globalI] = this->model_().localJacobian().mat(0,0);

        IntersectionIterator isIt = this->gridView_().ibegin(elem);
        const IntersectionIterator &isEndIt = this->gridView_().iend(elem);
        for (int j = 0; isIt != isEndIt; ++isIt)
        {
            if (isIt->neighbor())
            {
                int globalJ = this->elementMapper_().map(*(isIt->outside()));
                (*this->matrix_)[globalI][globalJ] = this->model_().localJacobian().mat(0,++j);
            }
        }
    }

    // "assemble" a green element. green elements only get the
    // residual updated, but the jacobian is left alone...
    void assembleGreenElement_(const Element &elem)
    {
        this->model_().localResidual().eval(elem);

        int globalI = this->elementMapper_().map(elem);

        // update the right hand side
        this->residual_[globalI] += this->model_().localResidual().residual(0);
        if (this->enableJacobianRecycling_())
            this->storageTerm_[globalI] += this->model_().localResidual().storageTerm(0);
    }

    // "assemble" a ghost element
    void assembleGhostElement_(const Element &elem)
    {
        int globalI = this->elementMapper_().map(elem);

        // update the right hand side
        this->residual_[globalI] = 0.0;

        // update the diagonal entry 
        typedef typename JacobianMatrix::block_type BlockType;
        BlockType &J = (*this->matrix_)[globalI][globalI];
        for (int j = 0; j < BlockType::rows; ++j)
            J[j][j] = 1.0;
    }
};

} // namespace Dumux

#endif
