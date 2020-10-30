// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 3 of the License, or       *
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
 * \ingroup MPNCTests
 * \brief Spatial parameters for the kinetic test-case of the mpnc model.
 *
 * "Poor-mans" coupling of free-flow and porous medium.
 */
#ifndef DUMUX_EVAPORATION_ATMOSPHERE_SPATIALPARAMS_HH
#define DUMUX_EVAPORATION_ATMOSPHERE_SPATIALPARAMS_HH

#include <dumux/porousmediumflow/properties.hh>
#include <dumux/material/spatialparams/fvnonequilibrium.hh>
#include <dumux/material/fluidmatrixinteractions/2p/linearmaterial.hh>
#include <dumux/material/fluidmatrixinteractions/2p/regularizedbrookscorey.hh>
#include <dumux/material/fluidmatrixinteractions/2p/efftoabslaw.hh>
#include <dumux/material/fluidmatrixinteractions/2p/vangenuchtenoftemperature.hh>
#include <dumux/material/fluidmatrixinteractions/2p/regularizedvangenuchten.hh>
#include <dumux/common/parameters.hh>

// material laws for interfacial area
#include <dumux/material/fluidmatrixinteractions/2pia/efftoabslawia.hh>
#include <dumux/material/fluidmatrixinteractions/2pia/awnsurfacepolynomial2ndorder.hh>
#include <dumux/material/fluidmatrixinteractions/2pia/awnsurfacepcmaxfct.hh>
#include <dumux/material/fluidmatrixinteractions/2pia/awnsurfaceexpswpcto3.hh>

namespace Dumux {

/**
 * \brief Definition of the spatial parameters for the evaporation atmosphere Problem (using a "poor man's coupling")
 */
template<class GridGeometry, class Scalar>
class EvaporationAtmosphereSpatialParams
: public FVNonEquilibriumSpatialParams<GridGeometry, Scalar,
                                       EvaporationAtmosphereSpatialParams<GridGeometry, Scalar>>
{
    using GridView = typename GridGeometry::GridView;
    using FVElementGeometry = typename GridGeometry::LocalView;
    using SubControlVolume = typename FVElementGeometry::SubControlVolume;
    using Element = typename GridView::template Codim<0>::Entity;
    using ThisType = EvaporationAtmosphereSpatialParams<GridGeometry, Scalar>;
    using ParentType = FVNonEquilibriumSpatialParams<GridGeometry, Scalar, ThisType>;

    using GlobalPosition = typename Element::Geometry::GlobalCoordinate;

    static constexpr auto dimWorld = GridView::dimensionworld;
public:
    //! Export the type used for the permeability
    using PermeabilityType = Scalar;
    //! Export the material law type used
    using MaterialLaw = EffToAbsLaw<RegularizedBrooksCorey<Scalar>>;
    //! Convenience aliases of the law parameters
    using MaterialLawParams = typename MaterialLaw::Params;

    //! Export the types used for interfacial area calculations
    using EffectiveIALawAws = AwnSurfacePolynomial2ndOrder<Scalar>;
    using EffectiveIALawAwn = AwnSurfacePcMaxFct<Scalar>;
    using EffectiveIALawAns = AwnSurfaceExpSwPcTo3<Scalar>;
    using AwnSurface = EffToAbsLawIA<EffectiveIALawAwn, MaterialLawParams>;
    using AwsSurface = EffToAbsLawIA<EffectiveIALawAws, MaterialLawParams>;
    using AnsSurface = EffToAbsLawIA<EffectiveIALawAns, MaterialLawParams>;

    using AwnSurfaceParams = typename AwnSurface::Params;
    using AwsSurfaceParams = typename AwsSurface::Params;
    using AnsSurfaceParams = typename AnsSurface::Params;

    EvaporationAtmosphereSpatialParams(std::shared_ptr<const GridGeometry> gridGeometry)
    : ParentType(gridGeometry)
    {
        heightPM_               = getParam<std::vector<Scalar>>("Grid.Positions1")[1];
        heightDomain_           = getParam<std::vector<Scalar>>("Grid.Positions1")[2];

        porosityPM_                 = getParam<Scalar>("SpatialParams.PorousMedium.porosity");
        intrinsicPermeabilityPM_    = getParam<Scalar>("SpatialParams.PorousMedium.permeability");

        porosityFF_                 = getParam<Scalar>("SpatialParams.FreeFlow.porosity");
        intrinsicPermeabilityFF_    = getParam<Scalar>("SpatialParams.FreeFlow.permeability");

        aWettingNonwettingA1_ = getParam<Scalar>("SpatialParams.soil.aWettingNonwettingA1");
        aWettingNonwettingA2_ = getParam<Scalar>("SpatialParams.soil.aWettingNonwettingA2");
        aWettingNonwettingA3_ = getParam<Scalar>("SpatialParams.soil.aWettingNonwettingA3");

        aNonwettingSolidA1_ = getParam<Scalar>("SpatialParams.soil.aNonwettingSolidA1");
        aNonwettingSolidA2_ = getParam<Scalar>("SpatialParams.soil.aNonwettingSolidA2");
        aNonwettingSolidA3_ = getParam<Scalar>("SpatialParams.soil.aNonwettingSolidA3");

        BCPd_           = getParam<Scalar>("SpatialParams.soil.BCPd");
        BClambda_       = getParam<Scalar>("SpatialParams.soil.BClambda");
        Swr_            = getParam<Scalar>("SpatialParams.soil.Swr");
        Snr_            = getParam<Scalar>("SpatialParams.soil.Snr");

        characteristicLengthFF_   = getParam<Scalar>("SpatialParams.FreeFlow.meanPoreSize");
        characteristicLengthPM_   = getParam<Scalar>("SpatialParams.PorousMedium.meanPoreSize");

        factorEnergyTransfer_ = getParam<Scalar>("SpatialParams.PorousMedium.factorEnergyTransfer");
        factorMassTransfer_ = getParam<Scalar>("SpatialParams.PorousMedium.factorMassTransfer");

        // residual saturations
        materialParamsFF_.setSwr(0.0);
        materialParamsFF_.setSnr(0.00);

        materialParamsPM_.setSwr(Swr_);
        materialParamsPM_.setSnr(Snr_);

        // pc / kr parameters
        materialParamsPM_.setLambda(BClambda_);
        materialParamsPM_.setPe(BCPd_);

        // for making pc == 0 in the FF
        materialParamsFF_.setLambda(42.);
        materialParamsFF_.setPe(0.);

        // determine maximum capillary pressure for wetting-nonwetting surface
        /* Of course physically there is no such thing as a maximum capillary pressure.
         * The parametrization (VG/BC) goes to infinity and physically there is only one pressure
         * for single phase conditions.
         * Here, this is used for fitting the interfacial area surface: the capillary pressure,
         * where the interfacial area is zero.
         * Technically this value is obtained as the capillary pressure of saturation zero.
         * This value of course only exists for the case of a regularized pc-Sw relation.
         */
        using TwoPLaw = EffToAbsLaw<RegularizedBrooksCorey<Scalar>>;
        const auto pcMax = TwoPLaw::pc(materialParamsPM_, /*sw = */0.0);
        aWettingNonwettingSurfaceParams_.setPcMax(pcMax);

        // wetting-non wetting: surface which goes to zero on the edges, but is a polynomial
        aWettingNonwettingSurfaceParams_.setA1(aWettingNonwettingA1_);
        aWettingNonwettingSurfaceParams_.setA2(aWettingNonwettingA2_);
        aWettingNonwettingSurfaceParams_.setA3(aWettingNonwettingA3_);

        // nonwetting-solid
        aNonwettingSolidSurfaceParams_.setA1(aNonwettingSolidA1_);
        aNonwettingSolidSurfaceParams_.setA2(aNonwettingSolidA2_);
        aNonwettingSolidSurfaceParams_.setA3(aNonwettingSolidA3_);

        // dummys for free flow: no interface where there is only one phase
        aWettingNonwettingSurfaceParamsFreeFlow_.setA1(0.);
        aWettingNonwettingSurfaceParamsFreeFlow_.setA2(0.);
        aWettingNonwettingSurfaceParamsFreeFlow_.setA3(0.);
        aWettingNonwettingSurfaceParamsFreeFlow_.setPcMax(42.); // not needed because it is anyways zero;

        // dummys for free flow: no interface where there is only one phase
        aNonwettingSolidSurfaceParamsFreeFlow_.setA1(0.);
        aNonwettingSolidSurfaceParamsFreeFlow_.setA2(0.);
        aNonwettingSolidSurfaceParamsFreeFlow_.setA3(0.);
    }

    template<class ElementSolution>
    PermeabilityType permeability(const Element& element,
                                  const SubControlVolume& scv,
                                  const ElementSolution& elemSol) const
    {
        const  auto & globalPos = scv.dofPosition();
        if (inFF_(globalPos))
            return intrinsicPermeabilityFF_ ;
        else if (inPM_(globalPos))
            return intrinsicPermeabilityPM_ ;
        else
            DUNE_THROW(Dune::InvalidStateException, "You should not be here: x=" << globalPos[0] << " y= "<< globalPos[dimWorld-1]);
    }

    /*!
     * \brief Function for defining the porosity.
     *        That is possibly solution dependent.
     *
     * \param element The current element
     * \param scv The sub-control volume inside the element.
     * \param elemSol The solution at the dofs connected to the element.
     * \return The porosity
     */
    template<class ElementSolution>
    Scalar porosity(const Element& element,
                    const SubControlVolume& scv,
                    const ElementSolution& elemSol) const
    {
        const auto& globalPos =  scv.dofPosition();

        if (inFF_(globalPos))
            return porosityFF_ ;
        else if (inPM_(globalPos))
            return porosityPM_ ;
        else
            DUNE_THROW(Dune::InvalidStateException, "You should not be here: x=" << globalPos[0] << " y= "<< globalPos[dimWorld-1]);
    }

    template<class ElementSolution>
    const MaterialLawParams& materialLawParams(const Element& element,
                                               const SubControlVolume& scv,
                                               const ElementSolution& elemSol) const
    { return materialLawParamsAtPos(scv.dofPosition()); }

    const MaterialLawParams& materialLawParamsAtPos(const GlobalPosition& globalPos) const
    {
        if (inFF_(globalPos))
            return materialParamsFF_;
        else if (inPM_(globalPos))
            return materialParamsPM_;
        else DUNE_THROW(Dune::InvalidStateException, "You should not be here: x=" << globalPos[0] << " y= "<< globalPos[dimWorld-1]);
    }

    /*!\brief Returns a reference to the container object for the
     *        parametrization of the surface between wetting and nonwetting phase.
     *
     * The position is determined based on the coordinate of
     * the vertex belonging to the considered sub-control volume.
     *
     * \param element The finite element
     * \param scv The sub-control volume
     * \param elemSol The element solution
     */
    template<class ElementSolution>
    const AwnSurfaceParams& aWettingNonwettingSurfaceParams(const Element &element,
                                                            const SubControlVolume &scv,
                                                            const ElementSolution &elemSol) const
    {
        const auto& globalPos =  scv.dofPosition();
        if (inFF_(globalPos) )
            return aWettingNonwettingSurfaceParamsFreeFlow_  ;
        else if (inPM_(globalPos))
            return aWettingNonwettingSurfaceParams_ ;
        else DUNE_THROW(Dune::InvalidStateException, "You should not be here: x=" << globalPos[0] << " y= "<< globalPos[dimWorld-1]);
    }

    /*!\brief Returns a reference to the container object for the
     *        parametrization of the surface between nonwetting and solid phase.
     *
     *        The position is determined based on the coordinate of
     *        the vertex belonging to the considered sub-control volume.
     * \param element The finite element
     * \param scv The sub-control volume
     * \param elemSol The element solution
     */
    template<class ElementSolution>
    const AnsSurfaceParams& aNonwettingSolidSurfaceParams(const Element &element,
                                                          const SubControlVolume &scv,
                                                          const ElementSolution &elemSol) const
    {
        const auto& globalPos =  scv.dofPosition();
        if (inFF_(globalPos) )
            return aNonwettingSolidSurfaceParamsFreeFlow_  ;
        else if (inPM_(globalPos))
            return aNonwettingSolidSurfaceParams_ ;
        else DUNE_THROW(Dune::InvalidStateException, "You should not be here: x=" << globalPos[0] << " y= "<< globalPos[dimWorld-1]);
    }

    /*!\brief Returns a reference to the container object for the
     *        parametrization of the surface between wetting and solid phase.
     *
     *        The position is determined based on the coordinate of
     *        the vertex belonging to the considered sub-control volume.
     * \param element The finite element
     * \param scv The sub-control volume
     * \param elemSol The element solution
     */
    template<class ElementSolution>
    const AwsSurfaceParams& aWettingSolidSurfaceParams(const Element &element,
                                                       const SubControlVolume &scv,
                                                       const ElementSolution &elemSol) const
    {
        DUNE_THROW(Dune::NotImplemented, "wetting-solid-interface surface params");
    }

    /*!
     * \brief Returns the characteristic length for the mass transfer.
     * \param globalPos The position in global coordinates.
     */
    const Scalar characteristicLengthAtPos(const  GlobalPosition & globalPos) const
    {
        if (inFF_(globalPos) )
            return characteristicLengthFF_ ;
        else if (inPM_(globalPos))
            return characteristicLengthPM_ ;
        else DUNE_THROW(Dune::InvalidStateException, "You should not be here: x=" << globalPos[0] << " y= "<< globalPos[dimWorld-1]);
    }

    /*!
     * \brief Return the pre factor the the energy transfer
     * \param globalPos The position in global coordinates.
     */
    const Scalar factorEnergyTransferAtPos(const  GlobalPosition & globalPos) const
    {
        if (inFF_(globalPos) )
            return factorEnergyTransfer_ ;
        else if (inPM_(globalPos))
            return factorEnergyTransfer_ ;
        else DUNE_THROW(Dune::InvalidStateException, "You should not be here: x=" << globalPos[0] << " y= "<< globalPos[dimWorld-1]);
    }

    /*!
     * \brief Return the pre factor the the mass transfer
     * \param globalPos The position in global coordinates.
     */
    const Scalar factorMassTransferAtPos(const  GlobalPosition & globalPos) const
    {
        if (inFF_(globalPos) )
            return factorMassTransfer_ ;
        else if (inPM_(globalPos))
            return factorMassTransfer_ ;
        else DUNE_THROW(Dune::InvalidStateException, "You should not be here: x=" << globalPos[0] << " y= "<< globalPos[dimWorld-1]);
    }

    /*!
     * \brief Function for defining which phase is to be considered as the wetting phase.
     *
     * \param globalPos The global position
     * \return The wetting phase index
     */
    template<class FluidSystem>
    int wettingPhaseAtPos(const GlobalPosition& globalPos) const
    {
        return FluidSystem::phase0Idx;
    }

    /*!
     * \brief Returns whether the tested position is in the porous medium part of the domain
     *
     * This setting ensures that the boundary between the two domains has porous
     * medium properties. This is desirable, because I want to observe the
     * boundary of the porous domain.
     * However, the position has to be the coordinate of the vertex and not the
     * integration point of the boundary flux. If the position is the ip of the
     * Neumann flux this leads to a situation
     * where the vertex belongs to porous medium and there is nonetheless
     * injection over the boundary. That does not work.
     * -> be careful with neumannAtPos
     */
    bool inPM_(const GlobalPosition & globalPos) const
    { return ( (globalPos[dimWorld-1] > 0. - eps_) and (globalPos[dimWorld-1] < (heightPM_ + eps_) ) );   }

    /*!
     * \brief Returns whether the tested position is above PM / "free flow" in the domain.
     *
     * This setting ensures that the boundary between the two domains has porous
     * medium properties. This is desirable, because I want to observe the
     * boundary of the porous domain.
     * However, the position has to be the coordinate of the vertex and not the
     * integration point of the boundary flux. If the position is the ip of the
     * Neumann flux this leads to a situation where the vertex belongs to porous
     * medium and there is nonetheless injection over the boundary.
     * That does not work.
     * -> be careful with neumannAtPos
     */
    bool inFF_(const GlobalPosition & globalPos) const
    { return ( (globalPos[dimWorld-1] < heightDomain_ + eps_) and (globalPos[dimWorld-1] > (heightPM_ + eps_) ) );   }

    /*! \brief access function for the depth / height of the porous medium */
    const Scalar heightPM() const
    { return heightPM_; }

private:
    static constexpr Scalar eps_  = 1e-6;
    Scalar heightDomain_ ;

    AwnSurfaceParams aWettingNonwettingSurfaceParams_;
    AnsSurfaceParams aNonwettingSolidSurfaceParams_ ;
    AwnSurfaceParams aWettingNonwettingSurfaceParamsFreeFlow_;
    AnsSurfaceParams aNonwettingSolidSurfaceParamsFreeFlow_ ;

    // Porous Medium Domain
    Scalar intrinsicPermeabilityPM_ ;
    Scalar porosityPM_ ;
    Scalar heightPM_ ;
    Scalar factorEnergyTransfer_ ;
    Scalar factorMassTransfer_ ;
    Scalar characteristicLengthPM_ ;
    MaterialLawParams materialParamsPM_ ;

    // Free Flow Domain
    Scalar porosityFF_ ;
    Scalar intrinsicPermeabilityFF_ ;
    Scalar characteristicLengthFF_ ;
    MaterialLawParams materialParamsFF_ ;

    // interfacial area parameters
    Scalar aWettingNonwettingA1_ ;
    Scalar aWettingNonwettingA2_ ;
    Scalar aWettingNonwettingA3_ ;

    Scalar aNonwettingSolidA1_;
    Scalar aNonwettingSolidA2_;
    Scalar aNonwettingSolidA3_;

    // capillary pressures parameters
    Scalar BCPd_ ;
    Scalar BClambda_ ;
    Scalar Swr_ ;
    Scalar Snr_ ;
    std::vector<Scalar> gridVector_;
};

} // end namespace Dumux

#endif
