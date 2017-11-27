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
 * \brief Boundary types gathered on an element
 */
#ifndef DUMUX_CC_ELEMENT_BOUNDARY_TYPES_HH
#define DUMUX_CC_ELEMENT_BOUNDARY_TYPES_HH

#include <dumux/common/valgrind.hh>

namespace Dumux
{

/*!
 * \ingroup CCModel
 * \ingroup ImplicitBoundaryTypes
 * \brief This class stores an array of BoundaryTypes objects
 */
template<class TypeTag>
class CCElementBoundaryTypes
{
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using Element = typename GridView::template Codim<0>::Entity;

public:

    /*!
     * \brief Update the boundary types for all vertices of an element.
     *
     * \param problem The problem object which needs to be simulated
     * \param element The DUNE Codim<0> entity for which the boundary
     *                types should be collected
     */
    void update(const Problem &problem,
                const Element &element,
                const FVElementGeometry &fvGeometry)
    {}
};

} // namespace Dumux

#endif
