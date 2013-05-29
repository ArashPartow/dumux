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
 * \brief Convert intersection boundary types to vertex boundary types 
 */
#ifndef DUMUX_INTERSECTIONTOVERTEXBC_HH
#define DUMUX_INTERSECTIONTOVERTEXBC_HH

#include <dumux/implicit/box/boxproperties.hh>

namespace Dumux
{

/*!
 * \ingroup BoxModel
 * \brief Convert intersection boundary types to vertex boundary types 
 */
template<typename TypeTag> 
class IntersectionToVertexBC
{
    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;

    enum {
        numEq = GET_PROP_VALUE(TypeTag, NumEq),
        dim = GridView::dimension,
    };

    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    typedef typename GridView::IntersectionIterator IntersectionIterator;
    typedef typename GridView::template Codim<dim>::Entity Vertex;

    typedef typename Dune::GenericReferenceElements<Scalar, dim> ReferenceElements;
    typedef typename Dune::GenericReferenceElement<Scalar, dim> ReferenceElement;

public:
    IntersectionToVertexBC(const Problem& problem)
    : problem_(problem)
    {
	vertexBC.resize(problem.vertexMapper().size());
        for (int vertIdx = 0; vertIdx < vertexBC.size(); vertIdx++)
            vertexBC[vertIdx].setAllNeumann();

        ElementIterator eIt = problem.gridView().template begin<0>();
        const ElementIterator eEndIt = problem.gridView().template end<0>();
        for (; eIt != eEndIt; ++eIt) {
            Dune::GeometryType geoType = eIt->geometry().type();
            const ReferenceElement &refElement = ReferenceElements::general(geoType);

            IntersectionIterator isIt = problem.gridView().ibegin(*eIt);
            IntersectionIterator isEndIt = problem.gridView().iend(*eIt);
            for (; isIt != isEndIt; ++isIt) {
                if (!isIt->boundary())
                    continue;

                BoundaryTypes bcTypes;
		problem.boundaryTypes(bcTypes, *isIt);

		if (!bcTypes.hasDirichlet())
                    continue;

                int faceIdx = isIt->indexInInside();
                int numFaceVerts = refElement.size(faceIdx, 1, dim);
                for (int faceVertIdx = 0; faceVertIdx < numFaceVerts; ++faceVertIdx)
                {
                    int elemVertIdx = refElement.subEntity(faceIdx, 1, faceVertIdx, dim);
                    int globalVertIdx = problem.vertexMapper().map(*eIt, elemVertIdx, dim);

                    for (int eqIdx = 0; eqIdx < numEq; eqIdx++)
                      if (bcTypes.isDirichlet(eqIdx))
                          vertexBC[globalVertIdx].setDirichlet(eqIdx);
                }
            }
        }
    }

    void boundaryTypes(BoundaryTypes& values, const Vertex& vertex) const
    {
        values.setAllNeumann();

	int vertIdx = problem_.vertexMapper().map(vertex);
	const BoundaryTypes& bcTypes = vertexBC[vertIdx];

        for (int eqIdx = 0; eqIdx < numEq; eqIdx++)
            if (bcTypes.isDirichlet(eqIdx))
                values.setDirichlet(eqIdx);
    }

private: 
    const Problem& problem_;
    std::vector<BoundaryTypes> vertexBC;
};
}

#endif

