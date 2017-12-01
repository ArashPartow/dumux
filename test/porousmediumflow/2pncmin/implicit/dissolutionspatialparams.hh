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
#ifndef DUMUX_INJECTION_SPATIAL_PARAMETERS_HH
#define DUMUX_INJECTION_SPATIAL_PARAMETERS_HH

#include <dumux/material/spatialparams/implicit.hh>
#include <dumux/material/fluidmatrixinteractions/2p/linearmaterial.hh>
#include <dumux/material/fluidmatrixinteractions/2p/regularizedbrookscorey.hh>
#include <dumux/material/fluidmatrixinteractions/2p/efftoabslaw.hh>
#include <dumux/material/fluidmatrixinteractions/porosityprecipitation.hh>
#include <dumux/material/fluidmatrixinteractions/permeabilitykozenycarman.hh>

namespace Dumux
{
//forward declaration
template<class TypeTag>
class DissolutionSpatialparams;

namespace Properties
{
// The spatial parameters TypeTag
NEW_TYPE_TAG(DissolutionSpatialparams);

// Set the spatial parameters
SET_TYPE_PROP(DissolutionSpatialparams, SpatialParams, DissolutionSpatialparams<TypeTag>);

// Set the material Law
SET_PROP(DissolutionSpatialparams, MaterialLaw)
{
private:
    // define the material law which is parameterized by effective saturations
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
public:
    // define the material law parameterized by absolute saturations
    using type = EffToAbsLaw<RegularizedBrooksCorey<Scalar>>;
};

} // end namespace Properties

/**
 * \brief Definition of the spatial parameters for the brine-co2 problem
 *
 */
template<class TypeTag>
class DissolutionSpatialparams : public ImplicitSpatialParams<TypeTag>
{
    using ThisType = DissolutionSpatialparams<TypeTag>;
    using ParentType = ImplicitSpatialParams<TypeTag>;
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using MaterialLawParams = typename GET_PROP_TYPE(TypeTag, MaterialLawParams);
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);
    using ElementSolutionVector = typename GET_PROP_TYPE(TypeTag, ElementSolutionVector);
    using CoordScalar = typename GridView::ctype;
    enum {
        dim=GridView::dimension,
        dimWorld=GridView::dimensionworld,
    };

    using Indices = typename GET_PROP_TYPE(TypeTag, Indices);
    enum {
        wPhaseIdx = FluidSystem::wPhaseIdx,
        nPhaseIdx = FluidSystem::nPhaseIdx,
    };

    using GlobalPosition = Dune::FieldVector<CoordScalar, dimWorld>;
    using Tensor = Dune::FieldMatrix<CoordScalar, dimWorld, dimWorld>;
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using Element = typename GridView::template Codim<0>::Entity;

    using PorosityLaw = PorosityPrecipitation<TypeTag>;
    using PermeabilityLaw = PermeabilityKozenyCarman<TypeTag>;

public:
    // type used for the permeability (i.e. tensor or scalar)
    using PermeabilityType = Tensor;

    DissolutionSpatialparams(const Problem& problem)
    : ParentType(problem)
    {
        solubilityLimit_     = getParam<Scalar>("SpatialParams.SolubilityLimit", 0.26);
        initialPorosity_     = getParam<Scalar>("SpatialParams.Porosity", 0.11);
        initialPermeability_ = getParam<Scalar>("SpatialParams.Permeability", 2.23e-14);
        irreducibleLiqSat_   = getParam<Scalar>("SpatialParams.IrreducibleLiqSat", 0.2);
        irreducibleGasSat_   = getParam<Scalar>("SpatialParams.IrreducibleGasSat", 1e-3);
        pEntry1_             = getParam<Scalar>("SpatialParams.Pentry1", 500);
        bcLambda1_           = getParam<Scalar>("SpatialParams.BCLambda1", 2);

        // residual saturations
        materialParams_.setSwr(irreducibleLiqSat_);
        materialParams_.setSnr(irreducibleGasSat_);

        // parameters of Brooks & Corey Law
        materialParams_.setPe(pEntry1_);
        materialParams_.setLambda(bcLambda1_);

        // set main diagonal entries of the permeability tensor to a value
        // setting to one value means: isotropic, homogeneous
        for (int i = 0; i < dim; i++)  //TODO make this nice and dependend on PermeabilityType!
            initK_[i][i] = initialPermeability_;

        //! Intitialize the parameter laws
        poroLaw_.init(*this);
        permLaw_.init(*this);
    }

    /*! Intrinsic permeability tensor K \f$[m^2]\f$ depending
     *  on the position in the domain
     *
     *  \param element The finite volume element
     *  \param scv The sub-control volume
     *
     *  Solution dependent permeability function
     */
    PermeabilityType permeability(const Element& element,
                        const SubControlVolume& scv,
                        const ElementSolutionVector& elemSol) const
    { return permLaw_.evaluatePermeability(element, scv, elemSol); }

    /*!
     *  \brief Define the minimum porosity \f$[-]\f$ distribution
     *
     *  \param element The finite element
     *  \param scv The sub-control volume
     */
    Scalar minPorosity(const Element& element, const SubControlVolume &scv) const
    { return 1e-5; }

    /*!
     *  \brief Define the initial porosity \f$[-]\f$ distribution
     *
     *  \param element The finite element
     *  \param scv The sub-control volume
     */
    Scalar initialPorosity(const Element& element, const SubControlVolume &scv) const
    { return initialPorosity_; }

    /*!
     *  \brief Define the initial permeability \f$[m^2]\f$ distribution
     *
     *  \param element The finite element
     *  \param scv The sub-control volume
     */
    PermeabilityType initialPermeability(const Element& element, const SubControlVolume &scv) const
    { return initK_; }

    /*!
     *  \brief Define the minimum porosity \f$[-]\f$ after clogging caused by mineralization
     *
     *  \param element The finite element
     *  \param scv The sub-control volume
     */
    Scalar porosity(const Element& element,
                    const SubControlVolume& scv,
                    const ElementSolutionVector& elemSol) const
    { return poroLaw_.evaluatePorosity(element, scv, elemSol); }


    Scalar solidity(const SubControlVolume &scv) const
    { return 1.0 - porosityAtPos(scv.center()); }

    Scalar solubilityLimit() const
    { return solubilityLimit_; }

    Scalar theta(const SubControlVolume &scv) const
    { return 10.0; }

    // return the brooks-corey context depending on the position
    const MaterialLawParams& materialLawParamsAtPos(const GlobalPosition& globalPos) const
    { return materialParams_; }

private:
    MaterialLawParams materialParams_;

    PorosityLaw poroLaw_;
    PermeabilityLaw permLaw_;
    Scalar solubilityLimit_;
    Scalar initialPorosity_;
    Scalar initialPermeability_;
    PermeabilityType initK_= 0.0;
    Scalar irreducibleLiqSat_;
    Scalar irreducibleGasSat_;
    Scalar pEntry1_;
    Scalar bcLambda1_;
};

} // end namespace Dumux

#endif
