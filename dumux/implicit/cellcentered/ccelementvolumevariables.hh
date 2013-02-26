// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2010 by Andreas Lauser                                    *
 *   Institute for Modelling Hydraulic and Environmental Systems             *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@iws.uni-stuttgart.de                          *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \file
 * \brief Volume variables gathered on an element
 */
#ifndef DUMUX_CC_ELEMENT_VOLUME_VARIABLES_HH
#define DUMUX_CC_ELEMENT_VOLUME_VARIABLES_HH

#include "ccproperties.hh"


namespace Dumux
{

/*!
 * \ingroup CCModel
 * \brief This class stores an array of VolumeVariables objects, one
 *        volume variables object for each of the element's vertices
 */
template<class TypeTag>
class CCElementVolumeVariables : public std::vector<typename GET_PROP_TYPE(TypeTag, VolumeVariables) >
{
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, SolutionVector) SolutionVector;
    typedef typename GET_PROP_TYPE(TypeTag, VertexMapper) VertexMapper;
    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;

    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GridView::template Codim<0>::Entity Element;

    enum { dim = GridView::dimension };
    enum { numEq = GET_PROP_VALUE(TypeTag, NumEq) };

public:
    /*!
     * \brief The constructor.
     */
    CCElementVolumeVariables()
    { }

    /*!
     * \brief Construct the volume variables for all of vertices of an element.
     *
     * \param problem The problem which needs to be simulated.
     * \param element The DUNE Codim<0> entity for which the volume variables ought to be calculated
     * \param fvElemGeom The finite volume geometry of the element
     * \param oldSol Tells whether the model's previous or current solution should be used.
     */
    void update(const Problem &problem,
                const Element &element,
                const FVElementGeometry &fvElemGeom,
                bool oldSol)
    {
        const SolutionVector &globalSol =
            oldSol?
            problem.model().prevSol():
            problem.model().curSol();

        int numNeighbors = fvElemGeom.numNeighbors;
        this->resize(numNeighbors);
        
        for (int i = 0; i < numNeighbors; i++)
        {
            const Element& neighbor = *(fvElemGeom.neighbors[i]);
            
            const PrimaryVariables &solI
                    = globalSol[problem.elementMapper().map(neighbor)];

            FVElementGeometry neighborFVGeom;
            neighborFVGeom.updateInner(neighbor);
            
            (*this)[i].update(solI,
                              problem,
                              neighbor,
                              neighborFVGeom,
                              /*scvIdx=*/0,
                              oldSol);
        }
    }
};

} // namespace Dumux

#endif
