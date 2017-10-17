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
 * \brief Base class for the upwind scheme
 */
#ifndef DUMUX_DISCRETIZATION_UPWINDSCHEME_HH
#define DUMUX_DISCRETIZATION_UPWINDSCHEME_HH

#include <dumux/implicit/properties.hh>
#include <dumux/discretization/methods.hh>

namespace Dumux
{

namespace Properties
{
// forward declaration
NEW_PROP_TAG(ImplicitUpwindWeight);
}

//! Forward declaration of the upwind scheme implementation
template<class TypeTag, DiscretizationMethods Method>
class UpwindSchemeImplementation;


//! Upwind scheme for the box method
template<class TypeTag>
class UpwindSchemeImplementation<TypeTag, DiscretizationMethods::Box>
{
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);

public:
    // applies a simple upwind scheme to the precalculated advective flux
    template<class FluxVariables, class UpwindTermFunction>
    static Scalar apply(const FluxVariables& fluxVars,
                        const UpwindTermFunction& upwindTerm,
                        Scalar flux, int phaseIdx)
    {
        // retrieve the upwind weight for the mass conservation equations. Use the value
        // specified via the property system as default, and overwrite
        // it by the run-time parameter from the Dune::ParameterTree
        static const Scalar upwindWeight = GET_PARAM_FROM_GROUP(TypeTag, Scalar, Implicit, UpwindWeight);

        const auto& insideVolVars = fluxVars.elemVolVars()[fluxVars.scvFace().insideScvIdx()];
        const auto& outsideVolVars = fluxVars.elemVolVars()[fluxVars.scvFace().outsideScvIdx()];
        if (std::signbit(flux))
            return flux*(upwindWeight*upwindTerm(outsideVolVars)
                         + (1.0 - upwindWeight)*upwindTerm(insideVolVars));
        else
            return flux*(upwindWeight*upwindTerm(insideVolVars)
                         + (1.0 - upwindWeight)*upwindTerm(outsideVolVars));
    }
};

//! Upwind scheme for the cell-centered TPFA scheme
template<class TypeTag>
class UpwindSchemeImplementation<TypeTag, DiscretizationMethods::CCTpfa>
{
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using AdvectionType = typename GET_PROP_TYPE(TypeTag, AdvectionType);

    static constexpr int dim = GridView::dimension;
    static constexpr int dimWorld = GridView::dimensionworld;

public:
    // For surface and network grids (dim < dimWorld) we have to do a special upwind scheme
    template<class FluxVariables, class UpwindTermFunction, int d = dim, int dw = dimWorld>
    static typename std::enable_if<(d < dw), Scalar>::type
    apply(const FluxVariables& fluxVars,
          const UpwindTermFunction& upwindTerm,
          Scalar flux, int phaseIdx)
    {
        // retrieve the upwind weight for the mass conservation equations. Use the value
        // specified via the property system as default, and overwrite
        // it by the run-time parameter from the Dune::ParameterTree
        static const Scalar upwindWeight = GET_PARAM_FROM_GROUP(TypeTag, Scalar, Implicit, UpwindWeight);

        // the volume variables of the inside sub-control volume
        const auto& insideVolVars = fluxVars.elemVolVars()[fluxVars.scvFace().insideScvIdx()];

        // check if this is a branching point
        if (fluxVars.scvFace().numOutsideScvs() > 1)
        {
            // more complicated upwind scheme
            // we compute a flux-weighted average of all inflowing branches
            Scalar branchingPointUpwindTerm = 0.0;
            Scalar sumUpwindFluxes = 0.0;

            // if the inside flux is positive (outflow) do fully upwind and return flux
            if (!std::signbit(flux))
                return upwindTerm(insideVolVars)*flux;
            else
                sumUpwindFluxes += flux;

            for (unsigned int i = 0; i < fluxVars.scvFace().numOutsideScvs(); ++i)
            {
                 // compute the outside flux
                const auto outsideScvIdx = fluxVars.scvFace().outsideScvIdx(i);
                const auto outsideElement = fluxVars.fvGeometry().fvGridGeometry().element(outsideScvIdx);
                const auto& flippedScvf = fluxVars.fvGeometry().flipScvf(fluxVars.scvFace().index(), i);

                const auto outsideFlux = AdvectionType::flux(fluxVars.problem(),
                                                             outsideElement,
                                                             fluxVars.fvGeometry(),
                                                             fluxVars.elemVolVars(),
                                                             flippedScvf,
                                                             phaseIdx,
                                                             fluxVars.elemFluxVarsCache());

                if (!std::signbit(outsideFlux))
                    branchingPointUpwindTerm += upwindTerm(fluxVars.elemVolVars()[outsideScvIdx])*outsideFlux;
                else
                    sumUpwindFluxes += outsideFlux;
            }

            // the flux might be zero
            if (sumUpwindFluxes != 0.0)
                branchingPointUpwindTerm /= -sumUpwindFluxes;
            else
                branchingPointUpwindTerm = 0.0;

            // upwind scheme (always do fully upwind at branching points)
            // a weighting here would lead to an error since the derivation is based on a fully upwind scheme
            // TODO How to implement a weight of e.g. 0.5
            if (std::signbit(flux))
                return flux*branchingPointUpwindTerm;
            else
                return flux*upwindTerm(insideVolVars);
        }
        // non-branching points and boundaries
        else
        {
            // upwind scheme
            const auto& outsideVolVars = fluxVars.elemVolVars()[fluxVars.scvFace().outsideScvIdx()];
            if (std::signbit(flux))
                return flux*(upwindWeight*upwindTerm(outsideVolVars)
                             + (1.0 - upwindWeight)*upwindTerm(insideVolVars));
            else
                return flux*(upwindWeight*upwindTerm(insideVolVars)
                             + (1.0 - upwindWeight)*upwindTerm(outsideVolVars));
        }
    }

    // For grids with dim == dimWorld we use a simple upwinding scheme
    template<class FluxVariables, class UpwindTermFunction, int d = dim, int dw = dimWorld>
    static typename std::enable_if<(d == dw), Scalar>::type
    apply(const FluxVariables& fluxVars,
          const UpwindTermFunction& upwindTerm,
          Scalar flux, int phaseIdx)
    {
        // retrieve the upwind weight for the mass conservation equations. Use the value
        // specified via the property system as default, and overwrite
        // it by the run-time parameter from the Dune::ParameterTree
        static const Scalar upwindWeight = getParamFromGroup<Scalar>(GET_PROP_VALUE(TypeTag, ModelParameterGroup), "Implicit.UpwindWeight");

        const auto& insideVolVars = fluxVars.elemVolVars()[fluxVars.scvFace().insideScvIdx()];
        const auto& outsideVolVars = fluxVars.elemVolVars()[fluxVars.scvFace().outsideScvIdx()];
        if (std::signbit(flux))
            return flux*(upwindWeight*upwindTerm(outsideVolVars)
                         + (1.0 - upwindWeight)*upwindTerm(insideVolVars));
        else
            return flux*(upwindWeight*upwindTerm(insideVolVars)
                         + (1.0 - upwindWeight)*upwindTerm(outsideVolVars));
    }
};

//! Specialization for cell-centered MPFA schemes
template<class TypeTag>
class UpwindSchemeImplementation<TypeTag, DiscretizationMethods::CCMpfa>
: public UpwindSchemeImplementation<TypeTag, DiscretizationMethods::CCTpfa> {};

/*!
 * \ingroup Discretization
 * \brief The upwind scheme used for the advective fluxes.
 *        This depends on the chosen discretization method.
 */
template<class TypeTag>
using UpwindScheme = UpwindSchemeImplementation<TypeTag, GET_PROP_VALUE(TypeTag, DiscretizationMethod)>;


} // end namespace

#endif
