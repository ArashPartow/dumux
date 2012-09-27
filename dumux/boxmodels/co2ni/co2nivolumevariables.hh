// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2008-2009 by Melanie Darcis                                  *
 *   Copyright (C) 2008-2010 by Andreas Lauser                               *
 *   Copyright (C) 2008-2009 by Klaus Mosthaf                                *
 *   Copyright (C) 2008-2009 by Bernd Flemisch                               *
 *   Institute for Modelling Hydraulic and Environmental Systems             *
 *   University of Stuttgart, Germany                                        *
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
 *
 * \brief Contains the quantities which are constant within a
 *        finite volume in the non-isothermal two-phase, two-component
 *        model.
 */
#ifndef DUMUX_CO2NI_VOLUME_VARIABLES_HH
#define DUMUX_CO2NI_VOLUME_VARIABLES_HH

#include <dumux/boxmodels/co2/co2volumevariables.hh>

namespace Dumux
{

/*!
 * \ingroup TwoPTwoCNIModel
 * \ingroup BoxVolumeVariables
 * \brief Contains the quantities which are are constant within a
 *        finite volume in the non-isothermal two-phase, two-component
 *        model.
 */
template <class TypeTag>
class CO2NIVolumeVariables : public CO2VolumeVariables<TypeTag>
{
    //! \cond 0
    typedef CO2VolumeVariables<TypeTag> ParentType;
    typedef typename ParentType::FluidState FluidState;

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;

    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    enum { temperatureIdx = Indices::temperatureIdx };

    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GridView::template Codim<0>::Entity Element;
    //! \endcond

public:
    /*!
     * \brief Returns the total internal energy of a phase in the
     *        sub-control volume.
     *
     * \param phaseIdx The phase index
     */
    Scalar internalEnergy(const int phaseIdx) const
    { return this->fluidState_.internalEnergy(phaseIdx); };

    /*!
     * \brief Returns the total enthalpy of a phase in the sub-control
     *        volume.
     *
     * \param phaseIdx The phase index
     */
    Scalar enthalpy(const int phaseIdx) const
    { return this->fluidState_.enthalpy(phaseIdx); };

    /*!
     * \brief Returns the total heat capacity \f$\mathrm{[J/(K*m^3]}\f$ of the rock matrix in
     *        the sub-control volume.
     */
    Scalar heatCapacity() const
    { return heatCapacity_; };

protected:
    // this method gets called by the parent class. since this method
    // is protected, we are friends with our parent..
    friend class CO2VolumeVariables<TypeTag>;

    static Scalar temperature_(const PrimaryVariables &priVars,
                               const Problem& problem,
                               const Element &element,
                               const FVElementGeometry &fvGeometry,
                               const int scvIdx)
    {
        return priVars[temperatureIdx];
    }

    template<class ParameterCache>
    static Scalar enthalpy_(const FluidState& fluidState,
                            const ParameterCache& paramCache,
                            const int phaseIdx)
    {
        return FluidSystem::enthalpy(fluidState, paramCache, phaseIdx);
    }

    /*!
     * \brief Update all quantities for a given control volume.
     *
     * \param sol The solution primary variables
     * \param problem The problem
     * \param element The element
     * \param fvGeometry Evaluate function with solution of current or previous time step
     * \param scvIdx The local index of the SCV (sub-control volume)
     * \param isOldSol Evaluate function with solution of current or previous time step
     */
    void updateEnergy_(const PrimaryVariables &sol,
                       const Problem &problem,
                       const Element &element,
                       const FVElementGeometry &fvGeometry,
                       const int scvIdx,
                       bool isOldSol)
    {
        // copmute and set the heat capacity of the solid phase
        heatCapacity_ = problem.spatialParams().heatCapacity(element, fvGeometry, scvIdx);
        Valgrind::CheckDefined(heatCapacity_);
    };

    Scalar heatCapacity_;
};

} // end namepace

#endif
