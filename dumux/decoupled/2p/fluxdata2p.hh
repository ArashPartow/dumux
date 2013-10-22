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
#ifndef DUMUX_FLUXDATA2P_HH
#define DUMUX_FLUXDATA2P_HH

#include "2pproperties.hh"

/**
 * @file
 * @brief  Class storing data assigned to a cell-cell interfaces, so-called flux-data
 */

namespace Dumux
{
/*!
 * \ingroup IMPES
 */
//! Class storing data assigned to a cell-cell interfaces, so-called flux-data.
/*! Stores velocities and potentials at cell-cell interfaces. Further it provides methods which interpret stored phase potentials for upwind decisions.
 *
 * @tparam TypeTag The problem TypeTag
 */
template<class TypeTag>
class FluxData2P
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;

    enum
    {
        dim = GridView::dimension
    };

    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;

    enum
    {
        wPhaseIdx = Indices::wPhaseIdx, nPhaseIdx = Indices::nPhaseIdx
    };

    enum
    {
        numPhases = GET_PROP_VALUE(TypeTag, NumPhases)
    };

    typedef Dune::FieldVector<Scalar, dim> DimVector;
    typedef Dune::FieldVector<DimVector, 2 * dim> VelocityVector;

    VelocityVector velocity_[numPhases];
    Scalar upwindPotential_[2 * dim][numPhases];
    bool velocityMarker_[2 * dim];

public:

    //! Constructs a FluxData2P object
    FluxData2P()
    {
        for (int faceIdx = 0;  faceIdx < 2*dim; faceIdx++)
        {
            for (int phaseIdx = 0; phaseIdx < numPhases; phaseIdx++)
            {
                velocity_[phaseIdx][faceIdx] = DimVector(0.0);

                upwindPotential_[faceIdx][phaseIdx] = 0.0;
            }
            velocityMarker_[faceIdx] = false;
        }
    }

    ////////////////////////////////////////////////////////////
    // functions returning the vectors of the primary variables
    ////////////////////////////////////////////////////////////

    /*! \brief Returns the phase velocity vector at a cell-cell interface
     *
     * \param phaseIdx Index of a fluid phase
     * \param indexInInside Index of the cell-cell interface in this cell
     */
    const DimVector& velocity(int phaseIdx, int indexInInside)
    {
        return velocity_[phaseIdx][indexInInside];
    }

    /*! \brief Returns the phase velocity vector at a cell-cell interface
     *
     * \param phaseIdx Index of a fluid phase
     * \param indexInInside Index of the cell-cell interface in this cell
     */
    const DimVector& velocity(int phaseIdx, int indexInInside) const
    {
        return velocity_[phaseIdx][indexInInside];
    }

    /*! \brief Sets the phase velocity vector at a cell-cell interface
     *
     * \param phaseIdx Index of a fluid phase
     * \param indexInInside Index of the cell-cell interface in this cell
     * \param velocity Phase velocity vector which is stored
     */
    void setVelocity(int phaseIdx, int indexInInside, const DimVector& velocity)
    {
        velocity_[phaseIdx][indexInInside] = velocity;
    }

    /*! \brief Adds a phase velocity vector to the one previously stored.
     *
     * \param phaseIdx Index of a fluid phase
     * \param indexInInside Index of the cell-cell interface in this cell
     * \param velocity Phase velocity vector which is added
     */
    void addVelocity(int phaseIdx, int indexInInside, const DimVector& velocity)
    {
        velocity_[phaseIdx][indexInInside] += velocity;
    }

    //!Resets velocities and potentials
    void resetVelocity()
    {
        for (int i = 0; i < 2 * dim; i++)
        {
            for (int j = 0; j < numPhases; j++)
            {
                velocity_[j][i] = 0.;
                upwindPotential_[i][j] = 0.;
            }
            velocityMarker_[i] = false;
        }
    }

    /*! \brief Returns the total velocity vector at a cell-cell interface
     *
     * \param indexInInside Index of the cell-cell interface in this cell
     */
    DimVector velocityTotal(int indexInInside)
    {
        return velocity_[wPhaseIdx][indexInInside]
                + velocity_[nPhaseIdx][indexInInside];
    }

    /*! \brief Returns the total velocity vector at a cell-cell interface
     *
     * \param indexInInside Index of the cell-cell interface in this cell
     */
    DimVector velocityTotal(int indexInInside) const
    {
        return velocity_[wPhaseIdx][indexInInside]
                + velocity_[nPhaseIdx][indexInInside];
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
     * \param phaseIdx Index of a fluid phase
     * \param indexInInside Index of the cell-cell interface in this cell
     */
    bool isUpwindCell(int phaseIdx, int indexInInside)
    {
        return (upwindPotential_[indexInInside][phaseIdx] > 0.);
    }

    /*! \brief Checks for upwind direction
     *Returns <tt>true</tt> if the cell is the upwind cell, otherwise <tt>false</tt>
     *
     * \param phaseIdx Index of a fluid phase
     * \param indexInInside Index of the cell-cell interface in this cell
     */
    bool isUpwindCell(int phaseIdx, int indexInInside) const
    {
        return (upwindPotential_[indexInInside][phaseIdx] > 0.);
    }

    /*! \brief Returns the phase upwind potential at a cell-cell interface
     *
     * \param phaseIdx Index of a fluid phase
     * \param indexInInside Index of the cell-cell interface in this cell
     */
    Scalar upwindPotential(int phaseIdx, int indexInInside)
    {
        return upwindPotential_[indexInInside][phaseIdx];
    }

    /*! \brief Returns the phase upwind potential at a cell-cell interface
     *
     * \param phaseIdx Index of a fluid phase
     * \param indexInInside Index of the cell-cell interface in this cell
     */
    Scalar upwindPotential(int phaseIdx, int indexInInside) const
    {
        return upwindPotential_[indexInInside][phaseIdx];
    }

    /*! \brief Sets the phase upwind potential at a cell-cell interface
     *
     * \param phaseIdx Index of a fluid phase
     * \param indexInInside Index of the cell-cell interface in this cell
     * \param pot Phase upwind potential which is stored
     */
    void setUpwindPotential(int phaseIdx, int indexInInside, Scalar pot)
    {
        upwindPotential_[indexInInside][phaseIdx] = pot;
    }

    /*! \brief Adds a phase upwind potential to the one previously stored
     *
     * \param phaseIdx Index of a fluid phase
     * \param indexInInside Index of the cell-cell interface in this cell
     * \param pot Phase upwind potential which is added
     */
    void addUpwindPotential(int phaseIdx, int indexInInside, Scalar pot)
    {
        upwindPotential_[indexInInside][phaseIdx] += pot;
    }
};
}
#endif
