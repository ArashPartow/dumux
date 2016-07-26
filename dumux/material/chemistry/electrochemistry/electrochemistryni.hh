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
 *
 * \brief Electrochemical model for a fuel cell application.
 */

#ifndef DUMUX_ELECTROCHEMISTRY_NI_HH
#define DUMUX_ELECTROCHEMISTRY_NI_HH

#include <dumux/common/basicproperties.hh>
#include <dumux/material/constants.hh>
#include <dumux/material/chemistry/electrochemistry/electrochemistry.hh>

namespace Dumux
{

namespace Properties
{
NEW_PROP_TAG(FluidSystem);
NEW_PROP_TAG(Indices);
NEW_PROP_TAG(VolumeVariables);
NEW_PROP_TAG(PrimaryVariables);
}

/*!
* \brief
* Class calculating source terms and current densities for fuel cells
* with the electrochemical models suggested by Ochs (2008) \cite ochs2008 or Acosta (2006) \cite A3:acosta:2006
* for the non-isothermal case
*/
template <class TypeTag, ElectroChemistryModel electroChemistryModel>
class ElectroChemistryNI : public ElectroChemistry<TypeTag, electroChemistryModel>
{
    typedef ElectroChemistry<TypeTag, electroChemistryModel> ParentType;

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, VolumeVariables) VolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;

    typedef Constants<Scalar> Constant;

    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    enum {
        //indices of the components
        wCompIdx = FluidSystem::wCompIdx, //major component of the liquid phase
        nCompIdx = FluidSystem::nCompIdx, //major component of the gas phase
        O2Idx = wCompIdx + 2
    };
    enum { //equation indices
            conti0EqIdx = Indices::conti0EqIdx,
            contiH2OEqIdx = conti0EqIdx + wCompIdx,
            contiO2EqIdx = conti0EqIdx + wCompIdx + 2,
            energyEqIdx = FluidSystem::numComponents, //energy equation
    };

    enum { isBox = GET_PROP_VALUE(TypeTag, ImplicitIsBox) };
    enum { dofCodim = isBox ? GridView::dimension : 0 };

    using GlobalPosition = typename Dune::FieldVector<Scalar, GridView::dimensionworld>;
    using CellVector = typename Dune::FieldVector<Scalar, GridView::dimension>;

public:
    /*!
    * \brief Calculates reaction sources with an electrochemical model approach.
    *
    * \param values The primary variable vector
    * \param currentDensity The current density
    *
    * For this method, the \a values parameter stores source values
    */
    static void reactionSource(PrimaryVariables &values,
                               Scalar currentDensity)
    {
        //correction to account for actually relevant reaction area
        //current density has to be devided by the half length of the box
        //\todo Do we have multiply with the electrochemically active surface area (ECSA) here instead?
        static Scalar gridYMax = GET_RUNTIME_PARAM(TypeTag, GlobalPosition, Grid.UpperRight)[1];
        static Scalar nCellsY = GET_RUNTIME_PARAM(TypeTag, CellVector, Grid.Cells)[1];

        // Warning: This assumes the reaction layer is always just one cell (cell-centered) or half a box (box) thick
        const auto lengthBox = gridYMax/nCellsY;
        if (isBox)
            currentDensity *= 2.0/lengthBox;
        else
            currentDensity *= 1.0/lengthBox;

        static Scalar transportNumberH2O = GET_RUNTIME_PARAM(TypeTag, Scalar, ElectroChemistry.TransportNumberH20);
        static Scalar thermoneutralVoltage = GET_RUNTIME_PARAM(TypeTag, Scalar, ElectroChemistry.ThermoneutralVoltage);
        static Scalar cellVoltage = GET_RUNTIME_PARAM(TypeTag, Scalar, ElectroChemistry.CellVoltage);

        //calculation of flux terms with faraday equation
        values[contiH2OEqIdx] = currentDensity/(2*Constant::F);                  //reaction term in reaction layer
        values[contiH2OEqIdx] += currentDensity/Constant::F*transportNumberH2O;  //osmotic term in membrane
        values[contiO2EqIdx]  = -currentDensity/(4*Constant::F);                 //O2-equation
        values[energyEqIdx] = (thermoneutralVoltage - cellVoltage)*currentDensity; //energy equation
    }

    DUNE_DEPRECATED_MSG("First compute the currentDensity with calculateCurrentDensity(const VolumeVariables&) and then use the method reactionSource(PrimaryVariables&, Scalar) instead")
    static void reactionSource(PrimaryVariables &values,
                               const VolumeVariables &volVars)
    {
        reactionSource(values, ParentType::calculateCurrentDensity(volVars));
    }
};

}// end namespace

#endif
