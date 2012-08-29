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
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
#ifndef DUMUX_FVVELOCITY_HH
#define DUMUX_FVVELOCITY_HH

// dumux environment
#include "dumux/common/math.hh"
#include <dumux/decoupled/common/pressureproperties.hh>
#include "fvvelocitydefault.hh"

/**
 * @file
 * @brief  Finite volume velocity reconstruction
 */

namespace Dumux
{

/*! \ingroup IMPET
 *
 * \brief Base class for finite volume velocity reconstruction
 *
 * Provides a basic frame for calculating a global velocity field.
 * The definition of the local velocity calculation as well as the storage or other postprocessing has to be provided by the local velocity implementation.
 * This local implementation has to have the form of VelocityDefault.
 *
 * \tparam TypeTag The Type Tag
 * \tparam Velocity The implementation of the local velocity calculation
 */
template<class TypeTag, class Velocity> class FVVelocity
{
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;

    typedef typename GET_PROP_TYPE(TypeTag, CellData) CellData;

    enum
    {
        dim = GridView::dimension, dimWorld = GridView::dimensionworld
    };

    // typedefs to abbreviate several dune classes...
    typedef typename GridView::Traits::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    typedef typename GridView::IntersectionIterator IntersectionIterator;
    typedef typename GridView::Intersection Intersection;


public:

    //!Initialize velocity implementation
    void initialize()
    {
        velocity_.initialize();
    }

    //function which iterates through the grid and calculates the global velocity field
    void calculateVelocity();

    /*! \brief Adds velocity output to the output file
     *
     * \tparam MultiWriter Class defining the output writer
     * \param writer The output writer (usually a <tt>VTKMultiWriter</tt> object)
     *
     */
    template<class MultiWriter>
    void addOutputVtkFields(MultiWriter &writer)
    {
        velocity_.addOutputVtkFields(writer);
    }

    //! Constructs a FVVelocity object
    /**
     * \param problem A problem class object
     */
    FVVelocity(Problem& problem) :
        problem_(problem), velocity_(problem)
    {}

private:
    Problem& problem_;
    Velocity velocity_;
};


/*! \brief Function which reconstructs a global velocity field
 *
 * Iterates through the grid and calls the local calculateVelocity(...) or calculateVelocityOnBoundary(...) functions which have to be provided by the local velocity implementation (see e.g. VelocityDefault )
 */
template<class TypeTag, class Velocity>
void FVVelocity<TypeTag, Velocity>::calculateVelocity()
{
    Dune::FieldVector<Scalar, dim> vel(0.);

    ElementIterator eItEnd = problem_.gridView().template end<0> ();
    for (ElementIterator eIt = problem_.gridView().template begin<0> (); eIt != eItEnd; ++eIt)
    {
        // cell information
        int globalIdxI = problem_.variables().index(*eIt);
        CellData& cellDataI = problem_.variables().cellData(globalIdxI);

        /*****  flux term ***********/
        // iterate over all faces of the cell
        IntersectionIterator isItEnd = problem_.gridView().iend(*eIt);
        for (IntersectionIterator isIt = problem_.gridView().ibegin(*eIt); isIt != isItEnd; ++isIt)
        {
            /************* handle interior face *****************/
            if (isIt->neighbor())
            {
                int isIndex = isIt->indexInInside();

                if (!cellDataI.fluxData().haveVelocity(isIndex))
                    velocity_.calculateVelocity(*isIt, cellDataI);
            }   // end neighbor

            /************* boundary face ************************/
            else
            {
                velocity_.calculateVelocityOnBoundary(*isIt, cellDataI);
            }
        } //end interfaces loop
    } // end grid traversal

    return;
}

}//end namespace Dumux
#endif
