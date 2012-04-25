// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2011 by Markus Wolff                                      *
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
#ifndef DUMUX_FLUXDATA1P_HH
#define DUMUX_FLUXDATA1P_HH

#include "1pproperties.hh"

/**
 * @file
 * @brief  Class storing data assigned to a cell-cell interfaces, so-called flux-data
 * @author Markus Wolff
 */

namespace Dumux
{
/*!
 * \ingroup OnePhase
 */
//! Class storing data assigned to a cell-cell interfaces, so-called flux-data.
/*! Stores velocities and potentials at cell-cell interfaces. Further it provides methods which interpret stored phase potentials for upwind decisions.
*
* @tparam TypeTag The problem TypeTag
*/
template<class TypeTag>
class FluxData1P
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GridView::IntersectionIterator IntersectionIterator;
    typedef typename GridView::Traits::template Codim<0>::Entity Element;

    enum
    {
        dim = GridView::dimension, dimWorld = GridView::dimensionworld
    };

    typedef Dune::FieldVector<Scalar, dim> DimVector;
    typedef Dune::FieldVector<DimVector, 2 * dim> VelocityVector;

    VelocityVector velocity_;
    Scalar potential_[2 * dim];
    bool velocityMarker_[2 * dim];

public:

    //! Constructs a FluxData1P object
    FluxData1P()
    {
        for (int face = 0;  face < 2*dim; face++)
        {
            velocity_[face] = DimVector(0.0);
            potential_[face] = 0.0;
            velocityMarker_[face] = false;
        }
    }

    ////////////////////////////////////////////////////////////
    // functions returning the vectors of the primary variables
    ////////////////////////////////////////////////////////////

    /*! \brief Returns the velocity vector at a cell-cell interface
     *
     * \param indexInInside Index of the cell-cell interface in this cell
     */
    const DimVector& velocity(int indexInInside)
    {
        return velocity_[indexInInside];
    }
    /*! \brief Returns the velocity vector at a cell-cell interface
     *
     * \param indexInInside Index of the cell-cell interface in this cell
     */
    const DimVector& velocity(int indexInInside) const
    {
        return velocity_[indexInInside];
    }
    /*! \brief Sets the velocity vector at a cell-cell interface
     *
     * \param indexInInside Index of the cell-cell interface in this cell
     * \param velocity Velocity vector which is stored
     */
    void setVelocity(int indexInInside, DimVector& velocity)
    {
        velocity_[indexInInside] = velocity;
    }
    //!Resets velocities and potentials
    void resetVelocity()
    {
        for (int i = 0; i < 2 * dim; i++)
        {
            velocity_[i] = 0.;
            potential_[i] = 0.;
            velocityMarker_[i] = false;
        }
    }

    /*! \brief Sets the velocity marker at a cell-cell interface
     * This marker can be used to check if a velocity has already been stored for this interface
     *
     * \param indexInInside Index of the cell-cell interface in this cell
     */
    void setVelocityMarker(int indexInInside)
    {
        velocityMarker_[indexInInside] = true;
    }

    /*! \brief Check the velocity marker
     * Returns <tt>true</tt> if a velocity marker was set, otherwise <tt>false</tt>
     *
     * \param indexInInside Index of the cell-cell interface in this cell
     */
    bool haveVelocity(int indexInInside)
    {
        return velocityMarker_[indexInInside];
    }

    //!Resets the velocity marker
    void resetVelocityMarker()
    {
        for (int i = 0; i < 2*dim; i++)
            velocityMarker_[i] = false;
    }

    /*! \brief Checks for upwind direction
     *Returns <tt>true</tt> if the cell is the upwind cell, otherwise <tt>false</tt>
     *
     * \param indexInInside Index of the cell-cell interface in this cell
     */
    bool isUpwindCell(int indexInInside)
    {
        return (potential_[indexInInside] >= 0.);
    }

    /*! \brief Checks for upwind direction
     *Returns <tt>true</tt> if the cell is the upwind cell, otherwise <tt>false</tt>
     *
     * \param indexInInside Index of the cell-cell interface in this cell
     */
    bool isUpwindCell(int indexInInside) const
    {
        return (potential_[indexInInside] >= 0.);
    }

    /*! \brief Returns the potential at a cell-cell interface
     *
     * \param indexInInside Index of the cell-cell interface in this cell
     */
    Scalar potential(int indexInInside)
    {
        return potential_[indexInInside];
    }

    /*! \brief Returns the potential at a cell-cell interface
     *
     * \param indexInInside Index of the cell-cell interface in this cell
     */
    Scalar potential(int indexInInside) const
    {
        return potential_[indexInInside];
    }

    /*! \brief Sets the potential at a cell-cell interface
     *
     * \param indexInInside Index of the cell-cell interface in this cell
     * \param pot Phase potential which is stored
     */
    void setPotential(int indexInInside, Scalar pot)
    {
        potential_[indexInInside] = pot;
    }

};
}
#endif
