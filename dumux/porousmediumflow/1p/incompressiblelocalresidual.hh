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
 * \brief Element-wise calculation of the residual and its derivatives
 *        for a single-phase, incompressible, test problem.
 */
#ifndef DUMUX_1P_INCOMPRESSIBLE_LOCAL_RESIDUAL_HH
#define DUMUX_1P_INCOMPRESSIBLE_LOCAL_RESIDUAL_HH

#include <dumux/discretization/methods.hh>
#include <dumux/porousmediumflow/immiscible/localresidual.hh>

namespace Dumux
{

template<class TypeTag>
class OnePIncompressibleLocalResidual : public ImmiscibleLocalResidual<TypeTag>
{
    using ParentType = ImmiscibleLocalResidual<TypeTag>;
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using PrimaryVariables = typename GET_PROP_TYPE(TypeTag, PrimaryVariables);
    using VolumeVariables = typename GET_PROP_TYPE(TypeTag, VolumeVariables);
    using ElementVolumeVariables = typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables);
    using ElementResidualVector = typename GET_PROP_TYPE(TypeTag, ElementSolutionVector);
    using FluxVariables = typename GET_PROP_TYPE(TypeTag, FluxVariables);
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);
    using ElementFluxVariablesCache = typename GET_PROP_TYPE(TypeTag, ElementFluxVariablesCache);
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Element = typename GridView::template Codim<0>::Entity;
    using EnergyLocalResidual = typename GET_PROP_TYPE(TypeTag, EnergyLocalResidual);
    using Indices = typename GET_PROP_TYPE(TypeTag, Indices);
    // first index for the mass balance
    enum { conti0EqIdx = Indices::conti0EqIdx };
    enum { pressureIdx = Indices::pressureIdx };

    static const int numPhases = GET_PROP_VALUE(TypeTag, NumPhases);

public:
    using ParentType::ParentType;

    template<class PartialDerivativeMatrix>
    void addStorageDerivatives(PartialDerivativeMatrix& partialDerivatives,
                               const Problem& problem,
                               const Element& element,
                               const FVElementGeometry& fvGeometry,
                               const VolumeVariables& curVolVars,
                               const SubControlVolume& scv) const {}

    template<class PartialDerivativeMatrix>
    void addSourceDerivatives(PartialDerivativeMatrix& partialDerivatives,
                              const Problem& problem,
                              const Element& element,
                              const FVElementGeometry& fvGeometry,
                              const VolumeVariables& curVolVars,
                              const SubControlVolume& scv) const {}

    //! flux derivatives for the cell-centered tpfa scheme
    template<class PartialDerivativeMatrices, class T = TypeTag>
    std::enable_if_t<GET_PROP_VALUE(T, DiscretizationMethod) == DiscretizationMethods::CCTpfa, void>
    addFluxDerivatives(PartialDerivativeMatrices& derivativeMatrices,
                       const Problem& problem,
                       const Element& element,
                       const FVElementGeometry& fvGeometry,
                       const ElementVolumeVariables& curElemVolVars,
                       const ElementFluxVariablesCache& elemFluxVarsCache,
                       const SubControlVolumeFace& scvf) const
    {
        static_assert(!FluidSystem::isCompressible(0),
                      "1p/incompressiblelocalresidual.hh: Only incompressible fluids are allowed!");
        static_assert(FluidSystem::viscosityIsConstant(0),
                      "1p/incompressiblelocalresidual.hh: Only fluids with constant viscosities are allowed!");

        // we know the "upwind factor" is constant, get inner one here and compute derivatives
        static const Scalar up = curElemVolVars[scvf.insideScvIdx()].density()
                                 / curElemVolVars[scvf.insideScvIdx()].viscosity();
        const auto deriv = elemFluxVarsCache[scvf].advectionTij()*up;

        // add partial derivatives to the respective given matrices
        derivativeMatrices[scvf.insideScvIdx()][conti0EqIdx][pressureIdx] += deriv;
        derivativeMatrices[scvf.outsideScvIdx()][conti0EqIdx][pressureIdx] -= deriv;
    }

    //! flux derivatives for the cell-centered mpfa scheme
    template<class PartialDerivativeMatrices, class T = TypeTag>
    std::enable_if_t<GET_PROP_VALUE(T, DiscretizationMethod) == DiscretizationMethods::CCMpfa, void>
    addFluxDerivatives(PartialDerivativeMatrices& derivativeMatrices,
                       const Problem& problem,
                       const Element& element,
                       const FVElementGeometry& fvGeometry,
                       const ElementVolumeVariables& curElemVolVars,
                       const ElementFluxVariablesCache& elemFluxVarsCache,
                       const SubControlVolumeFace& scvf) const
    {
        static_assert(!FluidSystem::isCompressible(0),
                      "1p/incompressiblelocalresidual.hh: Only incompressible fluids are allowed!");
        static_assert(FluidSystem::viscosityIsConstant(0),
                      "1p/incompressiblelocalresidual.hh: Only fluids with constant viscosities are allowed!");

        const auto& fluxVarsCache = elemFluxVarsCache[scvf];
        const auto& volVarIndices = fluxVarsCache.advectionVolVarsStencil();
        const auto& tij = fluxVarsCache.advectionTij();

        // we know the "upwind factor" is constant, get inner one here and compute derivatives
        static const Scalar up = curElemVolVars[scvf.insideScvIdx()].density()
                                 / curElemVolVars[scvf.insideScvIdx()].viscosity();

        // add partial derivatives to the respective given matrices
        for (unsigned int i = 0; i < volVarIndices.size();++i)
        {
            if (fluxVarsCache.advectionSwitchFluxSign())
                derivativeMatrices[volVarIndices[i]][conti0EqIdx][pressureIdx] -= tij[i]*up;
            else
                derivativeMatrices[volVarIndices[i]][conti0EqIdx][pressureIdx] += tij[i]*up;
        }
    }

    //! flux derivatives for the box scheme
    template<class JacobianMatrix, class T = TypeTag>
    std::enable_if_t<GET_PROP_VALUE(T, DiscretizationMethod) == DiscretizationMethods::Box, void>
    addFluxDerivatives(JacobianMatrix& A,
                       const Problem& problem,
                       const Element& element,
                       const FVElementGeometry& fvGeometry,
                       const ElementVolumeVariables& curElemVolVars,
                       const ElementFluxVariablesCache& elemFluxVarsCache,
                       const SubControlVolumeFace& scvf) const
    {
        static_assert(!FluidSystem::isCompressible(0),
                      "1p/incompressiblelocalresidual.hh: Only incompressible fluids are allowed!");
        static_assert(FluidSystem::viscosityIsConstant(0),
                      "1p/incompressiblelocalresidual.hh: Only fluids with constant viscosities are allowed!");

        using AdvectionType = typename GET_PROP_TYPE(T, AdvectionType);
        const auto ti = AdvectionType::calculateTransmissibilities(problem,
                                                                   element,
                                                                   fvGeometry,
                                                                   curElemVolVars,
                                                                   scvf,
                                                                   elemFluxVarsCache[scvf]);

        const auto& insideScv = fvGeometry.scv(scvf.insideScvIdx());
        const auto& outsideScv = fvGeometry.scv(scvf.outsideScvIdx());

        // we know the "upwind factor" is constant, get inner one here and compute derivatives
        static const Scalar up = curElemVolVars[scvf.insideScvIdx()].density()
                                 / curElemVolVars[scvf.insideScvIdx()].viscosity();
        for (const auto& scv : scvs(fvGeometry))
        {
            auto d = up*ti[scv.indexInElement()];
            A[insideScv.dofIndex()][scv.dofIndex()][conti0EqIdx][pressureIdx] += d;
            A[outsideScv.dofIndex()][scv.dofIndex()][conti0EqIdx][pressureIdx] -= d;
        }
    }

    //! Dirichlet flux derivatives for the cell-centered tpfa scheme
    template<class PartialDerivativeMatrices, class T = TypeTag>
    std::enable_if_t<GET_PROP_VALUE(T, DiscretizationMethod) == DiscretizationMethods::CCTpfa, void>
    addCCDirichletFluxDerivatives(PartialDerivativeMatrices& derivativeMatrices,
                                  const Problem& problem,
                                  const Element& element,
                                  const FVElementGeometry& fvGeometry,
                                  const ElementVolumeVariables& curElemVolVars,
                                  const ElementFluxVariablesCache& elemFluxVarsCache,
                                  const SubControlVolumeFace& scvf) const
    {
        // we know the "upwind factor" is constant, get inner one here
        static const Scalar up = curElemVolVars[scvf.insideScvIdx()].density()
                                 / curElemVolVars[scvf.insideScvIdx()].viscosity();
        const auto deriv = elemFluxVarsCache[scvf].advectionTij()*up;

        // compute and add partial derivative to the respective given matrices
        derivativeMatrices[scvf.insideScvIdx()][conti0EqIdx][pressureIdx] += deriv;
    }

    //! Dirichlet flux derivatives for the cell-centered mpfa scheme
    template<class PartialDerivativeMatrices, class T = TypeTag>
    std::enable_if_t<GET_PROP_VALUE(T, DiscretizationMethod) == DiscretizationMethods::CCMpfa, void>
    addCCDirichletFluxDerivatives(PartialDerivativeMatrices& derivativeMatrices,
                                  const Problem& problem,
                                  const Element& element,
                                  const FVElementGeometry& fvGeometry,
                                  const ElementVolumeVariables& curElemVolVars,
                                  const ElementFluxVariablesCache& elemFluxVarsCache,
                                  const SubControlVolumeFace& scvf) const
    {
        const auto& fluxVarsCache = elemFluxVarsCache[scvf];
        const auto& volVarIndices = fluxVarsCache.advectionVolVarsStencil();
        const auto& tij = fluxVarsCache.advectionTij();

        // we know the "upwind factor" is constant, get inner one here and compute derivatives
        static const Scalar up = curElemVolVars[scvf.insideScvIdx()].density()
                                 / curElemVolVars[scvf.insideScvIdx()].viscosity();

        // add partial derivatives to the respective given matrices
        for (unsigned int i = 0; i < volVarIndices.size();++i)
        {
            if (fluxVarsCache.advectionSwitchFluxSign())
                derivativeMatrices[volVarIndices[i]][conti0EqIdx][pressureIdx] -= tij[i]*up;
            else
                derivativeMatrices[volVarIndices[i]][conti0EqIdx][pressureIdx] += tij[i]*up;
        }
    }

    template<class PartialDerivativeMatrices>
    void addRobinFluxDerivatives(PartialDerivativeMatrices& derivativeMatrices,
                                 const Problem& problem,
                                 const Element& element,
                                 const FVElementGeometry& fvGeometry,
                                 const ElementVolumeVariables& curElemVolVars,
                                 const ElementFluxVariablesCache& elemFluxVarsCache,
                                 const SubControlVolumeFace& scvf) const
    {
        //! Robin-type boundary conditions are problem-specific.
        //! We can't put a general implementation here - users defining Robin-type BCs
        //! while using analytical Jacobian assembly must overload this function!
    }
};

} // end namespace Dumux

#endif
