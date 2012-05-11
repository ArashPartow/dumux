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
#ifndef DUMUX_FLUXDATA2P2C_HH
#define DUMUX_FLUXDATA2P2C_HH

#include <dumux/decoupled/common/decoupledproperties.hh>

/**
 * @file
 * @brief  Class including the variables and data of discretized data of the constitutive relations
 * @author Markus Wolff
 */

namespace Dumux
{
/*!
 * \ingroup IMPES
 */
//! Class including the variables and data of discretized data of the constitutive relations.
/*! The variables of two-phase flow, which are one pressure and one saturation are stored in this class.
 * Additionally, a velocity needed in the transport part of the decoupled two-phase flow is stored, as well as discretized data of constitutive relationships like
 * mobilities, fractional flow functions and capillary pressure. Thus, they have to be callculated just once in every time step or every iteration step.
 *
 * @tparam TypeTag The Type Tag
 */
template<class TypeTag>
class FluxData2P2C
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Scalar)) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(GridView)) GridView;
    typedef typename GridView::IntersectionIterator IntersectionIterator;
    typedef typename GridView::Traits::template Codim<0>::Entity Element;

    enum
    {
        dim = GridView::dimension, dimWorld = GridView::dimensionworld
    };

    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Indices)) Indices;

    enum
    {
        wPhaseIdx = Indices::wPhaseIdx, nPhaseIdx = Indices::nPhaseIdx
    };

    enum
    {
        numEquations = GET_PROP_VALUE(TypeTag, PTAG(NumEq))
    };

    typename Dune::BlockVector<typename Dune::FieldVector<bool, numEquations>> isUpwindCell_;

public:

    //! Constructs a VariableClass object
    /**
     *  @param gridView a DUNE gridview object corresponding to diffusion and transport equation
     */

    FluxData2P2C()
    {
        isUpwindCell_.resize(2 * dim);
        for (int i = 0; i<2*dim; i++)
        {
            isUpwindCell_[i] = false;
        }
    }
    void resize(int size)
    {
        isUpwindCell_.resize(size);
    }
    /** functions returning upwind information **/
    const bool& isUpwindCell(int indexInInside, int equationIdx) const
    {
        return isUpwindCell_[indexInInside][equationIdx];
    }

    void setUpwindCell(int indexInInside, int equationIdx, bool value)
    {
        isUpwindCell_[indexInInside][equationIdx] = value;
    }

};
}
#endif


/*** in transport module:
 *     // upwind mobility
    double lambdaW, lambdaNW;
    if (potentialW >= 0.)
    {
        lambdaW = cellDataI.mobility(wPhaseIdx);
        cellDataI.setUpwindCell(intersection.indexInInside(), contiWEqIdx, true);
        cellDataJ.setUpwindCell(intersection.indexInOutside(), contiWEqIdx, false);
    }
    else
    {
        lambdaW = cellDataJ.mobility(wPhaseIdx);
        cellDataJ.setUpwindCell(intersection.indexInOutside(), contiWEqIdx, true);
        cellDataI.setUpwindCell(intersection.indexInInside(), contiWEqIdx, false);
    }

    if (potentialNW >= 0.)
    {
        lambdaNW = cellDataI.mobility(nPhaseIdx);
        cellDataI.setUpwindCell(intersection.indexInInside(), contiNEqIdx, true);
        cellDataJ.setUpwindCell(intersection.indexInOutside(), contiNEqIdx, false);
    }
    else
    {
        lambdaW = cellDataJ.mobility(nPhaseIdx);
        cellDataJ.setUpwindCell(intersection.indexInOutside(), contiNEqIdx, true);
        cellDataI.setUpwindCell(intersection.indexInInside(), contiNEqIdx, false);
    }


    in cellData:

    //! Acess to flux data
    bool& isUpwindCell(int indexInInside, int equationIdx) const
    {
        return fluxData_.isUpwindCell(indexInInside, equationIdx);
    }

    void setUpwindCell(int indexInInside, int equationIdx, bool value)
    {
        fluxData_.setUpwindCell(indexInInside, equationIdx, value);
    }
 */

