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
#ifndef DUMUX_FVPRESSUREVELOCITY1P_HH
#define DUMUX_FVPRESSUREVELOCITY1P_HH


// dumux environment
#include "fvpressure1p.hh"
#include <dumux/decoupled/1p/1pproperties.hh>
#include <dumux/decoupled/common/fv/fvvelocity.hh>

/**
 * @file
 * @brief  Single Phase Finite Volume Model
 */

namespace Dumux
{

//! \ingroup FV1p
//! \brief Single Phase Finite Volume Model
/*! This model solves equations of the form
 * \f[
 *  \textbf{div}\, \boldsymbol v = q.
 * \f]
 * The velocity \f$ \boldsymbol v \f$ is the single phase Darcy velocity:
 * \f[
 *  \boldsymbol v = -\frac{1}{\mu} \boldsymbol K \left(\textbf{grad}\, p + \rho \, g  \, \textbf{grad}\, z\right),
 * \f]
 * where \f$ p \f$ is the pressure, \f$ \boldsymbol K \f$ the absolute permeability, \f$ \mu \f$ the viscosity, \f$ \rho \f$ the density, and \f$ g \f$ the gravity constant,
 * and \f$ q \f$ is the source term.
 * At the boundary, \f$ p = p_D \f$ on \f$ \Gamma_{Dirichlet} \f$, and \f$ \boldsymbol v \cdot \boldsymbol n = q_N\f$
 * on \f$ \Gamma_{Neumann} \f$.
 *
 * @tparam TypeTag The Type Tag
 *
 */
template<class TypeTag> class FVPressureVelocity1P: public FVPressure1P<TypeTag>
{
    typedef FVPressure1P<TypeTag> ParentType;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
public:
    /*! \brief Initializes the pressure model
     *
     * \copydetails ParentType::initialize()
     *
     * \param solveTwice indicates if more than one iteration is allowed to get an initial pressure solution
     */
    void initialize()
    {
        ParentType::initialize(false);
        velocity_.calculateVelocity();
    }

    /*! \brief Pressure update
     *
     * \copydetails ParentType::update()
     *
     */
    void update()
    {
        ParentType::update();
        velocity_.calculateVelocity();
    }

    /*! \brief Adds velocity output to the output file
     *
     * Adds the velocities to the output.
     *
     * \tparam MultiWriter Class defining the output writer
     * \param writer The output writer (usually a <tt>VTKMultiWriter</tt> object)
     *
     */
    template<class MultiWriter>
    void addOutputVtkFields(MultiWriter &writer)
    {
    	ParentType::addOutputVtkFields(writer);
    	velocity_.addOutputVtkFields(writer);
    }

    //! Constructs a FVPressure1P object
    /**
     * \param problem A problem class object
     */
    FVPressureVelocity1P(Problem& problem) :
        ParentType(problem), velocity_(problem)
    {}

private:
    Dumux::FVVelocity<TypeTag, typename GET_PROP_TYPE(TypeTag, Velocity) > velocity_;
};

}
#endif
