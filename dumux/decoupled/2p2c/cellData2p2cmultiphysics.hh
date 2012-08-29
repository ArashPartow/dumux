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
#ifndef DUMUX_ELEMENTDATA2P2C_MULTYPHYSICS_HH
#define DUMUX_ELEMENTDATA2P2C_MULTYPHYSICS_HH

#include "2p2cproperties.hh"
#include "cellData2p2c.hh"
#include <dumux/decoupled/2p2c/pseudo1p2cfluidstate.hh>

/**
 * @file
 * @brief  Storage container for discretized data for multiphysics models
 */

namespace Dumux
{
/*!
 * \ingroup multiphysics multiphase
 */
//! Storage container for discretized data for multiphysics models
/*! For multiphysics models, we divide the model in seperate sub-domains. Being a cell-based
 * information, this is also stored in the cellData. In addition, a simpler version of a
 * fluidState can be stored in cells being in the simpler subdomain.
 * Hence, acess functions either direct to the full fluidstate, or to the simple fluidstate.
 *
 * @tparam TypeTag The Type Tag
 */
template<class TypeTag>
class CellData2P2Cmultiphysics : public CellData2P2C<TypeTag>
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
//    typedef FluxData2P2C<TypeTag> FluxData;
    typedef typename GET_PROP_TYPE(TypeTag, FluidState) FluidState;
    typedef PseudoOnePTwoCFluidState<TypeTag> SimpleFluidState;

    enum
    {
        dim = GridView::dimension, dimWorld = GridView::dimensionworld
    };

    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;

    enum
    {
        wPhaseIdx = Indices::wPhaseIdx, nPhaseIdx = Indices::nPhaseIdx,
        wCompIdx = Indices::wPhaseIdx, nCompIdx = Indices::nPhaseIdx,
        contiWEqIdx = Indices::contiWEqIdx, contiNEqIdx = Indices::contiNEqIdx
    };
    enum
    {
        numPhases = GET_PROP_VALUE(TypeTag, NumPhases),
        numComponents = GET_PROP_VALUE(TypeTag, NumComponents)
    };
    enum
    {
        complex = 0, simple = 1
    };
private:
    int subdomain_;
    int fluidStateType_;
    SimpleFluidState* simpleFluidState_;

//    FluxData fluxData_;
public:

    //! Constructor for a local storage object
    CellData2P2Cmultiphysics() : CellData2P2C<TypeTag>(),
        subdomain_(2), fluidStateType_(complex), simpleFluidState_(0)
    {
    }

    /*! \name Acess to primary variables */
    //@{
    //! \copydoc Dumux::DecoupledTwoPTwoCFluidState::pressure()
    Scalar pressure(int phaseIdx)
    {
        if(fluidStateType_ == simple)
        {
            return simpleFluidState_->pressure(phaseIdx);
        }
        else
            return this->fluidState_->pressure(phaseIdx);
    }
    //! \copydoc Dumux::DecoupledTwoPTwoCFluidState::pressure()
    const Scalar pressure(int phaseIdx) const
    {
        if(fluidStateType_ == simple)
        {
            return simpleFluidState_->pressure(phaseIdx);
        }
        else
            return this->fluidState_->pressure(phaseIdx);
    }
    //! \copydoc Dumux::CellData2P2C::setPressure()
    void setPressure(int phaseIdx, Scalar value)
    {
        if(fluidStateType_ == simple)
            manipulateSimpleFluidState().setPressure(phaseIdx, value);
        else
            manipulateFluidState().setPressure(phaseIdx, value);
    }
    //@}


    //////////////////////////////////////////////////////////////
    // functions returning the vectors of secondary variables
    //////////////////////////////////////////////////////////////

    //! Return subdomain information
    /** Acess function to store subdomain information
     */
    int& subdomain()
    {
        return subdomain_;
    }
    //! Return subdomain information
    /** Acess function to get subdomain information
     */
    const int& subdomain() const
    {
        return subdomain_;
    }
    //! Specify subdomain information and fluidStateType
    /** This function is only called if
     */
    void setSubdomainAndFluidStateType(int index)
    {
        subdomain_ = index;
        if(index == 2)
            fluidStateType_ = complex;
        else
            fluidStateType_ = simple;
    }

    /*! \name Acess to secondary variables */
    //@{
    //! \copydoc Dumux::DecoupledTwoPTwoCFluidState::setSaturation()
    void setSaturation(int phaseIdx, Scalar value)
    {
        if(fluidStateType_ == simple)
        {
            // saturation is triggered by presentPhaseIdx
            manipulateSimpleFluidState().setPresentPhaseIdx((value==0.) ? nPhaseIdx : wPhaseIdx);
        }
        else
            manipulateFluidState().setSaturation(phaseIdx, value);
    }
    //! \copydoc Dumux::DecoupledTwoPTwoCFluidState::saturation()
    const Scalar saturation(int phaseIdx) const
    {
        if(fluidStateType_ == simple)
        {
            return simpleFluidState_->saturation(phaseIdx);
        }
        else
            return this->fluidState_->saturation(phaseIdx);
    }

    //! \copydoc Dumux::DecoupledTwoPTwoCFluidState::setViscosity()
    void setViscosity(int phaseIdx, Scalar value)
    {
        if(fluidStateType_ == simple)
        {
            assert(phaseIdx == simpleFluidState_->presentPhaseIdx());
            manipulateSimpleFluidState().setViscosity(phaseIdx, value);
        }
        else
            manipulateFluidState().setViscosity(phaseIdx, value);
    }
    //! \copydoc Dumux::DecoupledTwoPTwoCFluidState::viscosity()
    const Scalar viscosity(int phaseIdx) const
    {
        if(fluidStateType_ == simple)
        {
            if(phaseIdx != simpleFluidState_->presentPhaseIdx())
                return 0.; // This should only happend for output
            return simpleFluidState_->viscosity(phaseIdx);
        }
        else
            return this->fluidState_->viscosity(phaseIdx);
    }


    //! \copydoc Dumux::DecoupledTwoPTwoCFluidState::capillaryPressure()
    const Scalar capillaryPressure() const
    {
        if(fluidStateType_ == simple)
            return simpleFluidState_->pressure(nPhaseIdx) - simpleFluidState_->pressure(wPhaseIdx);
        else
            return this->fluidState_->pressure(nPhaseIdx) - this->fluidState_->pressure(wPhaseIdx);
    }

    //! \copydoc Dumux::DecoupledTwoPTwoCFluidState::density()
    const Scalar density(int phaseIdx) const
    {
        if(fluidStateType_ == simple)
        {
            return simpleFluidState_->density(phaseIdx);
        }
        else
            return this->fluidState_->density(phaseIdx);
    }

    //! \copydoc Dumux::DecoupledTwoPTwoCFluidState::massFraction()
    const Scalar massFraction(int phaseIdx, int compIdx) const
    {
        if(fluidStateType_ == simple)
        {
            return simpleFluidState_->massFraction(phaseIdx, compIdx);
        }
        else
            return this->fluidState_->massFraction(phaseIdx, compIdx);
    }

    //! \copydoc Dumux::DecoupledTwoPTwoCFluidState::moleFraction()
    const Scalar moleFraction(int phaseIdx, int compIdx) const
    {
        if(fluidStateType_ == simple)
        {
            return simpleFluidState_->moleFraction(phaseIdx, compIdx);
        }
        else
            return this->fluidState_->moleFraction(phaseIdx, compIdx);
    }
    //! \copydoc Dumux::DecoupledTwoPTwoCFluidState::temperature()
    const Scalar temperature(int phaseIdx) const
    {
        if(fluidStateType_ == simple)
        {
            return simpleFluidState_->temperature(phaseIdx);
        }
        else
            return this->fluidState_->temperature(phaseIdx);
    }

    //! \copydoc Dumux::DecoupledTwoPTwoCFluidState::phaseMassFraction()
    const Scalar phaseMassFraction(int phaseIdx) const
    {
        if(fluidStateType_ == simple)
        {
            if(phaseIdx == simpleFluidState_->presentPhaseIdx())
                return 1.; // phase is present => nu = 1
            else
                return 0.;
        }
        else
            return this->fluidState_->phaseMassFraction(phaseIdx);
    }
    //@}


//    FluxData& setFluxData()
//    {
//        return fluxData_;
//    }
//
//    const FluxData& fluxData() const
//    {
//        return fluxData_;
//    }
    //! Set a simple fluidstate for a cell in the simple domain
    /** Uses a simplified fluidstate with less storage capacity
     * and functionality.
     * Makes shure the fluidStateType_ flag is set appropriately in this
     * cell.
     * @param simpleFluidState A fluidstate storing a 1p2c mixture
     */
    void setSimpleFluidState(SimpleFluidState& simpleFluidState)
    {
        assert (this->subdomain() != 2);
        fluidStateType_ = simple;
        simpleFluidState_ = &simpleFluidState;
    }
    //! Manipulates a simple fluidstate for a cell in the simple domain
    SimpleFluidState& manipulateSimpleFluidState()
    {
        fluidStateType_ = simple;
        if(this->fluidState_)
        {
            delete this->fluidState_;
            this->fluidState_ = NULL;
        }

        if(!simpleFluidState_)
            simpleFluidState_ = new SimpleFluidState;
        return *simpleFluidState_;
    }
    //! Allows manipulation of the complex fluid state
    /** Fluidstate is stored as a pointer, initialized as a null-pointer.
     * Enshure that if no FluidState is present, a new one is created.
     * Also enshure that we are in the complex subdomain.
     */
    FluidState& manipulateFluidState()
    {
        fluidStateType_ = complex;
        if(simpleFluidState_)
        {
            delete simpleFluidState_;
            simpleFluidState_ = NULL;
        }

        if(!this->fluidState_)
            this->fluidState_ = new FluidState;
        return *this->fluidState_;
    }

    //! Returns the type of the fluidState
    const bool fluidStateType() const
    { return fluidStateType_;}

};
}
#endif
