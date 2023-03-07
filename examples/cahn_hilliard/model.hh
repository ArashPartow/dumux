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

#ifndef DUMUX_EXAMPLES_CAHN_HILLIARD_MODEL_HH
#define DUMUX_EXAMPLES_CAHN_HILLIARD_MODEL_HH

// # Volume variables, local residual and model traits (`model.hh`)
// In this example the file `model.hh` contains the classes `CahnHilliardModelVolumeVariables` and
// `CahnHilliardModelLocalResidual` as well as general model traits and properties.
//
// ## VolumeVariables
//
// The volume variables store the local element volume variables, both primary and secondary.
//
// [[content]]
//
// ### The `CahnHilliardModelVolumeVariables` class
//
// [[codeblock]]
namespace Dumux {

template <class Traits>
class CahnHilliardModelVolumeVariables
{
    using Scalar = typename Traits::PrimaryVariables::value_type;
    static_assert(Traits::PrimaryVariables::dimension == Traits::ModelTraits::numEq());
public:
    //! export the type used for the primary variables
    using PrimaryVariables = typename Traits::PrimaryVariables;
    //! export the indices type
    using Indices = typename Traits::ModelTraits::Indices;
    // [[/codeblock]]
    //
    // ### Update variables
    //
    // The `update` function stores the local primary variables of the current solution and
    // potentially recomputes secondary variables.
    //
    // [[codeblock]]
    /*!
     * \brief Update all quantities for a given control volume
     */
    template<class ElementSolution, class Problem, class Element, class SubControlVolume>
    void update(const ElementSolution& elemSol,
                const Problem& problem,
                const Element& element,
                const SubControlVolume& scv)
    {
        priVars_ = elemSol[scv.indexInElement()];
    }
    // [[/codeblock]]
    //
    // ### Access functions
    //
    // Named and generic functions to access different primary variables
    //
    // [[codeblock]]
    Scalar concentration() const
    { return priVars_[Indices::concentrationIdx]; }

    Scalar chemicalPotential() const
    { return priVars_[Indices::chemicalPotentialIdx]; }

    Scalar priVar(const int pvIdx) const
    { return priVars_[pvIdx]; }

    const PrimaryVariables& priVars() const
    { return priVars_; }
    // [[/codeblock]]
    //
    // ### Extrusion factor
    //
    // The volumevariables are also expected to return the extrusion factor
    //
    // [[codeblock]]
    // for compatibility with more general models
    Scalar extrusionFactor() const
    { return 1.0; }
    // [[/codeblock]]
    //
    // ### Storage of local variables
    //
    // [[codeblock]]
private:
    PrimaryVariables priVars_;
};

} // end namespace Dumux
// [[/codeblock]]
// [[/content]]
//
// ## LocalResidual
//
// The local residual defines the discretized and integrated partial differential equation through
// terms for storage, fluxes and sources, with the residual given as
// d/dt storage + div(fluxes) - sources = 0
// The individual terms can be further adjusted or replaced in the more specific problem.
//
// [[content]]
//
// ### Include headers
//
// [[codeblock]]
#include <dumux/common/math.hh>
// use the property system
#include <dumux/common/properties.hh>
// common DuMux vector for discretized equations
#include <dumux/common/numeqvector.hh>
// [[/codeblock]]
//
// ### The local residual class `CahnHilliardModelLocalResidual` inherits from a base class set in
// the model properties, depending on the discretization scheme.
//
// [[codeblock]]
namespace Dumux {

template<class TypeTag>
class CahnHilliardModelLocalResidual
: public GetPropType<TypeTag, Properties::BaseLocalResidual>
{
    // the base local residual is selected depending on the chosen discretization scheme
    using ParentType = GetPropType<TypeTag, Properties::BaseLocalResidual>;
    // [[/codeblock]]
    //
    // [[details]] alias definitions
    // [[codeblock]]
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using Problem = GetPropType<TypeTag, Properties::Problem>;

    using NumEqVector = Dumux::NumEqVector<GetPropType<TypeTag, Properties::PrimaryVariables>>;
    using VolumeVariables = typename GetPropType<TypeTag, Properties::GridVolumeVariables>::VolumeVariables;

    using ElementVolumeVariables = typename GetPropType<TypeTag, Properties::GridVolumeVariables>::LocalView;
    using ElementFluxVariablesCache = typename GetPropType<TypeTag, Properties::GridFluxVariablesCache>::LocalView;
    using FVElementGeometry = typename GetPropType<TypeTag, Properties::GridGeometry>::LocalView;

    using SubControlVolume = typename FVElementGeometry::SubControlVolume;
    using SubControlVolumeFace = typename FVElementGeometry::SubControlVolumeFace;
    using GridView = typename GetPropType<TypeTag, Properties::GridGeometry>::GridView;
    using Element = typename GridView::template Codim<0>::Entity;

    using ModelTraits = GetPropType<TypeTag, Properties::ModelTraits>;
    using Indices = typename ModelTraits::Indices;
    static constexpr int dimWorld = GridView::dimensionworld;
public:
    using ParentType::ParentType;
    // [[/codeblock]]
    // [[/details]]
    //
    // ### The storage term
    // The function `computeStorage` receives the volumevariables at the previous or current time
    // step and computes the value of the storage terms.
    // In this case the mass balance equation is a conservation equation of the concentration and
    // the equation for the chemical potential does not have a storage term.
    //
    // [[codeblock]]
    /*!
     * \brief Evaluate the rate of change of all conserved quantities
     */
    NumEqVector computeStorage(const Problem& problem,
                               const SubControlVolume& scv,
                               const VolumeVariables& volVars) const
    {
        NumEqVector storage;
        storage[Indices::massBalanceEqIdx] = volVars.concentration();
        storage[Indices::chemicalPotentialEqIdx] = 0.0;
        return storage;
    }
    // [[/codeblock]]
    //
    // ### The flux terms
    // `computeFlux` computes the fluxes over a subcontrolvolumeface, including the integration over
    // the area of the face.
    //
    // [[codeblock]]
    /*!
     * \brief Evaluate the fluxes over a face of a sub control volume
     * Here we evaluate the flow rate, F1 = -M∇mu·n A, F2 = -gamma∇c·n A
     *
     * TODO: why is this called flux, if we expect it to be integrated already?
     * computeFluxIntegral?
     */
    NumEqVector computeFlux(const Problem& problem,
                            const Element& element,
                            const FVElementGeometry& fvGeometry,
                            const ElementVolumeVariables& elemVolVars,
                            const SubControlVolumeFace& scvf,
                            const ElementFluxVariablesCache& elemFluxVarsCache) const
    {
        const auto& fluxVarCache = elemFluxVarsCache[scvf];
        Dune::FieldVector<Scalar, dimWorld> gradConcentration(0.0);
        Dune::FieldVector<Scalar, dimWorld> gradChemicalPotential(0.0);
        for (const auto& scv : scvs(fvGeometry))
        {
            const auto& volVars = elemVolVars[scv];
            gradConcentration.axpy(volVars.concentration(), fluxVarCache.gradN(scv.indexInElement()));
            gradChemicalPotential.axpy(volVars.chemicalPotential(), fluxVarCache.gradN(scv.indexInElement()));
        }

        const auto M = problem.mobility();
        const auto gamma = problem.surfaceTension();

        NumEqVector flux;
        flux[Indices::massBalanceEqIdx] = -1.0*vtmv(scvf.unitOuterNormal(), M, gradChemicalPotential)*scvf.area();
        flux[Indices::chemicalPotentialEqIdx] = -1.0*vtmv(scvf.unitOuterNormal(), gamma, gradConcentration)*scvf.area();
        return flux;
    }
    // [[/codeblock]]
    //
    // ### Source terms
    //
    // `computeSource` defines the sources terms at a sub control volume.
    // We implement a model-specific source term for the chemical potential equation before
    // deferring further implementation to the problem where we add the derivative of the free
    // energy. The default implementation of this function also defers the calculation to the
    // problem.
    //
    // [[codeblock]]
    /*!
     * \brief Calculate the source term of the equation
     */
    NumEqVector computeSource(const Problem& problem,
                              const Element& element,
                              const FVElementGeometry& fvGeometry,
                              const ElementVolumeVariables& elemVolVars,
                              const SubControlVolume &scv) const
    {
        NumEqVector source(0.0);

        source[Indices::chemicalPotentialEqIdx] = elemVolVars[scv].chemicalPotential();

        // add contributions from problem (e.g. double well potential)
        source += problem.source(element, fvGeometry, elemVolVars, scv);

        return source;
    }
};

} // end namespace Dumux
// [[/codeblock]]
// [[/content]]
//
// ## Model properties/traits
//
// We set some general model traits and properties, using a TypeTag `CahnHilliardModel`.
// For this type tag we define a `ModelTraits` struct and set a number of properties by specifying
// further structs within the `Dumux::Properties` namespace.
//
// [[content]]
//
// ### Include the header for the property system and common properties and switch to the
// `Properties` namespace.
//
// [[codeblock]]
#include <dumux/common/properties.hh>

namespace Dumux::Properties {
// [[/codeblock]]
//
// ### Define the type tag
//
// [[codeblock]]
namespace TTag {
struct CahnHilliardModel {};
} // end namespace TTag
// [[/codeblock]]
//
// ### Basic model properties
//
// Define some general properties to be used for this modedl such as a datatype for scalars and the
// default vector for the primary variables. Here we can also use the model traits we specify below.
//
// [[codeblock]]
//! Set the default type of scalar values to double
template<class TypeTag>
struct Scalar<TypeTag, TTag:: CahnHilliardModel >
{ using type = double; };

//! Set the default primary variable vector to a vector of size of number of equations
template<class TypeTag>
struct PrimaryVariables<TypeTag, TTag:: CahnHilliardModel >
{
    using type = Dune::FieldVector<
        GetPropType<TypeTag, Properties::Scalar>,
        GetPropType<TypeTag, Properties::ModelTraits>::numEq()
    >;
};
// [[/codeblock]]
//
// ### Model traits
//
// We specify general traits of the implemented model, defining indices (often in `indices.hh`)
// and the number of equations in the model.
//
// [[codeblock]]
//! Set the model traits property
template<class TypeTag>
struct ModelTraits<TypeTag, TTag::CahnHilliardModel>
{
    struct Traits
    {
        struct Indices
        {
            static constexpr int concentrationIdx = 0;
            static constexpr int chemicalPotentialIdx = 1;

            static constexpr int massBalanceEqIdx = 0;
            static constexpr int chemicalPotentialEqIdx = 1;
        };

        static constexpr int numEq() { return 2; }
    };

    using type = Traits;
};
// [[/codeblock]]
//
// ### Further model properties
//
// Define further properties of the model, selecting the local residual and volumevariables defined
// above.
//
// [[codeblock]]
template<class TypeTag>
struct LocalResidual<TypeTag, TTag::CahnHilliardModel>
{ using type = CahnHilliardModelLocalResidual<TypeTag>; };

//! Set the volume variables property
template<class TypeTag>
struct VolumeVariables<TypeTag, TTag::CahnHilliardModel>
{
    struct Traits
    {
        using PrimaryVariables = GetPropType<TypeTag, Properties::PrimaryVariables>;
        using ModelTraits = GetPropType<TypeTag, Properties::ModelTraits>;
    };

    using type = CahnHilliardModelVolumeVariables<Traits>;
};

} // end namespace Dumux::Properties
// [[/codeblock]]
// [[/content]]

#endif
