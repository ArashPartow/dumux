// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
//
// SPDX-FileCopyrightInfo: Copyright © DuMux Project contributors, see AUTHORS.md in root folder
// SPDX-License-Identifier: GPL-3.0-or-later
//
/*!
 * \file
 * \brief This file contains the data which is required to calculate
 *        diffusive heat fluxes with Fourier's law.
 */
#ifndef DUMUX_TEST_MULTIDOMAIN_DUALNETWORK_FLUID_FOURIERS_LAW_HH
#define DUMUX_TEST_MULTIDOMAIN_DUALNETWORK_FLUID_FOURIERS_LAW_HH

#include <dumux/common/math.hh>

namespace Dumux::PoreNetwork {

template<bool isFluid>
struct FluidOrGrainPyramidFouriersLaw
{
    template<class Problem, class Element, class FVElementGeometry,
             class ElementVolumeVariables, class ElementFluxVariablesCache>
    static auto flux(const Problem& problem,
                     const Element& element,
                     const FVElementGeometry& fvGeometry,
                     const ElementVolumeVariables& elemVolVars,
                     const typename FVElementGeometry::SubControlVolumeFace& scvf,
                     const ElementFluxVariablesCache& elemFluxVarsCache)
    {
        const auto& insideScv = fvGeometry.scv(scvf.insideScvIdx());
        const auto& outsideScv = fvGeometry.scv(scvf.outsideScvIdx());
        const auto& insideVolVars = elemVolVars[insideScv];
        const auto& outsideVolVars = elemVolVars[outsideScv];
        const auto deltaT = insideVolVars.temperature() - outsideVolVars.temperature();
        return transmissibility(problem, element, fvGeometry, elemVolVars, scvf, elemFluxVarsCache) * deltaT;
    }

    template<class Problem, class Element, class FVElementGeometry,
             class ElementVolumeVariables, class ElementFluxVariablesCache>
    static auto transmissibility(const Problem& problem,
                                 const Element& element,
                                 const FVElementGeometry& fvGeometry,
                                 const ElementVolumeVariables& elemVolVars,
                                 const typename FVElementGeometry::SubControlVolumeFace& scvf,
                                 const ElementFluxVariablesCache& elemFluxVarsCache)
    {
        using Scalar = typename ElementVolumeVariables::VolumeVariables::PrimaryVariables::value_type;
        const auto& insideScv = fvGeometry.scv(scvf.insideScvIdx());
        const auto& outsideScv = fvGeometry.scv(scvf.outsideScvIdx());
        const auto& insideVolVars = elemVolVars[insideScv];
        const auto& outsideVolVars = elemVolVars[outsideScv];
        const auto& fluxVarsCache = elemFluxVarsCache[scvf];

        const auto getConductivity = [&](const auto& volVars)
        {
            if constexpr (isFluid)
                return volVars.fluidThermalConductivity(0);
            else
                return volVars.solidThermalConductivity();
        };

        const auto insideThermalConducitivity = getConductivity(insideVolVars);
        const auto outsideThermalConducitivity = getConductivity(outsideVolVars);

        const auto eIdx = fvGeometry.gridGeometry().elementMapper().index(element);
        const auto throatCenter = problem.spatialParams().throatCenter(eIdx);
        const auto distanceInside = (insideScv.dofPosition() - throatCenter).two_norm();
        const auto distanceOutside = (outsideScv.dofPosition() - throatCenter).two_norm();

        const auto throatArea = [&]()
        {
            static const Scalar givenArea = getParamFromGroup<Scalar>(problem.paramGroup(), "Problem.ThroatArea", 0.0);
            if (givenArea > 0.0)
                return givenArea;

            if constexpr (isFluid)
                return fluxVarsCache.throatCrossSectionalArea(0);
            else
                return fluxVarsCache.grainContactArea();
        }();


        const Scalar pyramidFrustumTopArea = [&]()
        {
            static const bool realArea = getParamFromGroup<bool>(problem.paramGroup(), "Problem.UseRealThroatAreaInPyramid", true);
            if (realArea)
                return throatArea;
            else
            {
                // use inscribed throat diameter as square side length
                const Scalar baseLength = fluxVarsCache.throatInscribedRadius() * 2.0;
                return baseLength*baseLength;
            }
        }();

        const auto getPyramidFrustumBaseArea = [&](const auto& volVars, const Scalar distance)
        {
            static const Scalar givenArea = getParamFromGroup<Scalar>(problem.paramGroup(), "Problem.PoreArea", 0.0);
            if (givenArea > 0.0)
                return givenArea;

            static const bool realVolume = getParamFromGroup<bool>(problem.paramGroup(), "Problem.UseVolumeEqualPyramid", true);
            if (realVolume)
            {
                using std::sqrt;
                const Scalar vol = 0.5 * volVars.poreVolume();
                const Scalar baseLenTop = sqrt(pyramidFrustumTopArea);
                const Scalar height = distance;
                // see https://en.wikipedia.org/wiki/Moscow_Mathematical_Papyrus
                const Scalar baseLenBot = 0.5*sqrt(3.0) * sqrt(-(baseLenTop*baseLenTop*height-4.0*vol)/height) -0.5*baseLenTop;
                return baseLenBot*baseLenBot;
            }
            else
                return volVars.poreVolume() / (2.0*distance);
        };

        using std::sqrt;
        const auto baseAreaInside = getPyramidFrustumBaseArea(insideVolVars, distanceInside);
        const auto baseAreaOutside = getPyramidFrustumBaseArea(outsideVolVars, distanceOutside);
        const auto topArea = pyramidFrustumTopArea;
        const auto insideTransmissibility = insideThermalConducitivity * sqrt(baseAreaInside*topArea) / distanceInside;
        const auto outsideTransmissibility = outsideThermalConducitivity * sqrt(baseAreaOutside*topArea) / distanceOutside;

        return 1.0 / (1.0/insideTransmissibility + 1.0/outsideTransmissibility);
    }
};

template<bool isFluid>
struct FixedFactorFouriersLaw
{
    template<class Problem, class Element, class FVElementGeometry,
             class ElementVolumeVariables, class ElementFluxVariablesCache>
    static auto flux(const Problem& problem,
                     const Element& element,
                     const FVElementGeometry& fvGeometry,
                     const ElementVolumeVariables& elemVolVars,
                     const typename FVElementGeometry::SubControlVolumeFace& scvf,
                     const ElementFluxVariablesCache& elemFluxVarsCache)
    {
        const auto& insideScv = fvGeometry.scv(scvf.insideScvIdx());
        const auto& outsideScv = fvGeometry.scv(scvf.outsideScvIdx());
        const auto& insideVolVars = elemVolVars[insideScv];
        const auto& outsideVolVars = elemVolVars[outsideScv];
        const auto deltaT = insideVolVars.temperature() - outsideVolVars.temperature();
        return transmissibility(problem, element, fvGeometry, elemVolVars, scvf, elemFluxVarsCache) * deltaT;
    }

    template<class Problem, class Element, class FVElementGeometry,
             class ElementVolumeVariables, class ElementFluxVariablesCache>
    static auto transmissibility(const Problem& problem,
                                 const Element& element,
                                 const FVElementGeometry& fvGeometry,
                                 const ElementVolumeVariables& elemVolVars,
                                 const typename FVElementGeometry::SubControlVolumeFace& scvf,
                                 const ElementFluxVariablesCache& elemFluxVarsCache)
    {
        using Scalar = typename ElementVolumeVariables::VolumeVariables::PrimaryVariables::value_type;

        const auto& insideScv = fvGeometry.scv(scvf.insideScvIdx());
        const auto& outsideScv = fvGeometry.scv(scvf.outsideScvIdx());
        const auto& insideVolVars = elemVolVars[insideScv];
        const auto& outsideVolVars = elemVolVars[outsideScv];

        const auto getConductivity = [&](const auto& volVars)
        {
            if constexpr (isFluid)
                return volVars.fluidThermalConductivity(0);
            else
                return volVars.solidThermalConductivity();
        };

        const auto insideThermalConducitivity = getConductivity(insideVolVars);
        const auto outsideThermalConducitivity = getConductivity(outsideVolVars);

        const auto eIdx = fvGeometry.gridGeometry().elementMapper().index(element);
        const auto throatCenter = problem.spatialParams().throatCenter(eIdx);
        const auto distanceInside = (insideScv.dofPosition() - throatCenter).two_norm();
        const auto distanceOutside = (outsideScv.dofPosition() - throatCenter).two_norm();

        static const Scalar fixedFactor = getParamFromGroup<Scalar>(problem.paramGroup(), "Problem.FixedFourierFactor");
        const auto insideTransmissibility = insideThermalConducitivity*4*distanceInside*fixedFactor;
        const auto outsideTransmissibility = outsideThermalConducitivity*4*distanceOutside*fixedFactor;

        return 1.0 / (1.0/insideTransmissibility + 1.0/outsideTransmissibility);
    }
};

template<bool isFluid>
struct FancyFactorFouriersLaw
{
    template<class Problem, class Element, class FVElementGeometry,
             class ElementVolumeVariables, class ElementFluxVariablesCache>
    static auto flux(const Problem& problem,
                     const Element& element,
                     const FVElementGeometry& fvGeometry,
                     const ElementVolumeVariables& elemVolVars,
                     const typename FVElementGeometry::SubControlVolumeFace& scvf,
                     const ElementFluxVariablesCache& elemFluxVarsCache)
    {
        const auto& insideScv = fvGeometry.scv(scvf.insideScvIdx());
        const auto& outsideScv = fvGeometry.scv(scvf.outsideScvIdx());
        const auto& insideVolVars = elemVolVars[insideScv];
        const auto& outsideVolVars = elemVolVars[outsideScv];
        const auto deltaT = insideVolVars.temperature() - outsideVolVars.temperature();
        return transmissibility(problem, element, fvGeometry, elemVolVars, scvf, elemFluxVarsCache) * deltaT;
    }

    template<class Problem, class Element, class FVElementGeometry,
             class ElementVolumeVariables, class ElementFluxVariablesCache>
    static auto transmissibility(const Problem& problem,
                                 const Element& element,
                                 const FVElementGeometry& fvGeometry,
                                 const ElementVolumeVariables& elemVolVars,
                                 const typename FVElementGeometry::SubControlVolumeFace& scvf,
                                 const ElementFluxVariablesCache& elemFluxVarsCache)
    {
        using Scalar = typename ElementVolumeVariables::VolumeVariables::PrimaryVariables::value_type;

        const auto& insideScv = fvGeometry.scv(scvf.insideScvIdx());
        const auto& outsideScv = fvGeometry.scv(scvf.outsideScvIdx());
        const auto& insideVolVars = elemVolVars[insideScv];
        const auto& outsideVolVars = elemVolVars[outsideScv];

        const auto getConductivity = [&](const auto& volVars)
        {
            if constexpr (isFluid)
                return volVars.fluidThermalConductivity(0);
            else
                return volVars.solidThermalConductivity();
        };

        const auto insideThermalConducitivity = getConductivity(insideVolVars);
        const auto outsideThermalConducitivity = getConductivity(outsideVolVars);

        const auto eIdx = fvGeometry.gridGeometry().elementMapper().index(element);

        const auto distance = [&](const auto& scv)
        {
            static const bool useThroatCenter =  getParamFromGroup<bool>(problem.paramGroup(), "Problem.UseThroatCenter", true);
            if (useThroatCenter)
            {
                const auto throatCenter = problem.spatialParams().throatCenter(eIdx);
                return (scv.dofPosition() - throatCenter).two_norm();
            }
            else
            {
                static const Scalar R = getParam<Scalar>("Problem.SphereRadius", 50e-6);
                static const Scalar overlapFactor = getParam<Scalar>("Problem.OverlapFactor");
                static const auto dx = overlapFactor*R;
                return dx;
            }
        };

        const Scalar distanceInside = distance(insideScv);
        const Scalar distanceOutside = distance(outsideScv);

        assert(distanceInside > 0.0);
        assert(distanceOutside > 0.0);

        static const Scalar liquidThermalConductivity = getParam<Scalar>("2.Component.LiquidThermalConductivity");
        static const Scalar solidThermalConductivity = getParam<Scalar>("1.Component.SolidThermalConductivity");
        static const Scalar kappa = liquidThermalConductivity / solidThermalConductivity;
        static const Scalar kappaFactor = isFluid ? kappa : 1.0/kappa;

        const Scalar ApInside = insideVolVars.poreVolume()/(2.0*distanceInside);
        const Scalar ApOutside = outsideVolVars.poreVolume()/(2.0*distanceOutside);

        Scalar effectiveAreaInside = 0.0;
        Scalar effectiveAreaOutside = 0.0;
        Scalar At = 0.0;

        const auto effectiveArea = [kappaFactor = kappaFactor](const Scalar At, const Scalar Cinf, const Scalar C0)
        {
            return At*(Cinf + ((C0 - Cinf)*(Cinf - 1.0))/((Cinf - 1.0) + kappaFactor*(1.0 - C0)));
        };

        static const Scalar useExactThroatAreaSphere = getParam<bool>("Problem.UseExactThroatAreaSphere", false);
        if (useExactThroatAreaSphere)
        {
            static const Scalar R = getParamFromGroup<Scalar>(problem.paramGroup(), "Problem.SphereRadius", 50e-6);
            const auto As = [](Scalar x, Scalar dx, Scalar R) { return M_PI*(R - x)*(R + x); };
            const auto Asq = [](Scalar x, Scalar dx, Scalar R) { return 4*dx*dx; };
            const auto Asemicirc = [](Scalar x, Scalar dx, Scalar R) {
                const auto r = std::sqrt((R - x)*(R + x));
                return (r*r)*std::acos(dx/r) - dx*std::sqrt(r*r - dx*dx);
            };
            const auto A1s = [&](Scalar x, Scalar dx, Scalar R) { return As(x, dx, R) - 4*Asemicirc(x, dx, R); };
            const auto A2s = [&](Scalar x, Scalar dx, Scalar R) { return As(x, dx, R); };
            const auto A1f = [&](Scalar x, Scalar dx, Scalar R) { return Asq(x, dx, R) - A1s(x, dx, R); };
            // const auto A2f = [&](Scalar x, Scalar dx, Scalar R) { return Asq(x, dx, R) - A2s(x, dx, R); }; // TODO unused?

            At = isFluid ? A1f(0, distanceInside, R) : A2s(distanceInside, distanceInside, R);
            const Scalar C0 = isFluid ? 0.1 : 0.45;
            const Scalar Cinf = isFluid ? ApInside/At : ApInside/At*1.45;
            effectiveAreaInside = effectiveArea(At, Cinf, C0);
            effectiveAreaOutside = effectiveAreaInside;
            assert(std::isnormal(effectiveAreaInside));
        }
        else
        {
            At = [&]
            {
                if constexpr (isFluid)
                    return elemFluxVarsCache[scvf].throatCrossSectionalArea(0);
                else
                    return elemFluxVarsCache[scvf].grainContactArea();
            }();

            static const Scalar C0 = isFluid ? getParamFromGroup<Scalar>(problem.paramGroup(), "Problem.C0Fluid")
                                   : getParamFromGroup<Scalar>(problem.paramGroup(), "Problem.C0Solid");
            static const Scalar CinfFactor = isFluid ? getParamFromGroup<Scalar>(problem.paramGroup(), "Problem.CInfFactorFluid")
                                           : getParamFromGroup<Scalar>(problem.paramGroup(), "Problem.CInfFactorSolid");
            using std::max;
            const Scalar CinfInside = max(ApInside/At*CinfFactor, 1.0);
            const Scalar CinfOutside = max(ApOutside/At*CinfFactor, 1.0);
            effectiveAreaInside = effectiveArea(At, CinfInside, C0);
            effectiveAreaOutside = effectiveArea(At, CinfOutside, C0);
        }

        assert(std::isnormal(effectiveAreaInside));
        assert(effectiveAreaInside > 0.0);
        assert(std::isnormal(effectiveAreaOutside));
        assert(effectiveAreaOutside > 0.0);

        const Scalar tInside = std::sqrt(effectiveAreaInside*At)/distanceInside;
        const Scalar tOutside = std::sqrt(effectiveAreaOutside*At)/distanceOutside;

        const auto insideTransmissibility = insideThermalConducitivity*tInside;
        const auto outsideTransmissibility = outsideThermalConducitivity*tOutside;
        const auto result = 1.0 / (1.0/insideTransmissibility + 1.0/outsideTransmissibility);

        if (!std::isnormal(result))
            DUNE_THROW(Dune::InvalidStateException, "Error in heat conductivity. Check your grid and your factors.");

        return result;
    }
};

template<class BaseLaw>
struct ScalingFouriersLaw
{
    template<class Problem, class Element, class FVElementGeometry,
             class ElementVolumeVariables, class ElementFluxVariablesCache>
    static auto flux(const Problem& problem,
                     const Element& element,
                     const FVElementGeometry& fvGeometry,
                     const ElementVolumeVariables& elemVolVars,
                     const typename FVElementGeometry::SubControlVolumeFace& scvf,
                     const ElementFluxVariablesCache& elemFluxVarsCache)
    {
        const auto& insideScv = fvGeometry.scv(scvf.insideScvIdx());
        const auto& outsideScv = fvGeometry.scv(scvf.outsideScvIdx());
        const auto& insideVolVars = elemVolVars[insideScv];
        const auto& outsideVolVars = elemVolVars[outsideScv];
        using Scalar = typename ElementVolumeVariables::VolumeVariables::PrimaryVariables::value_type;
        const auto deltaT = insideVolVars.temperature() - outsideVolVars.temperature();

        static const bool useScaling = getParamFromGroup<bool>(problem.paramGroup(), "Problem.UseFourierScaling", true);
        if (!useScaling)
            return transmissibility(problem, element, fvGeometry, elemVolVars, scvf, elemFluxVarsCache) * deltaT;


        Scalar factor = 1.0;
        const auto& bBoxMin = problem.gridGeometry().bBoxMin();
        const auto& bBoxMax = problem.gridGeometry().bBoxMax();
        const auto throatCenter = element.geometry().center();

        for (int i = 0; i < throatCenter.size(); ++i)
        {
            constexpr Scalar eps = 1e-8; // TODO
            if (throatCenter[i] < bBoxMin[i] + eps || throatCenter[i] > bBoxMax[i] - eps)
                factor *= 0.5;
        }

        static const Scalar baseTransmissibility = problem.getInternalReferenceHeatTransmissibility();
        return baseTransmissibility * factor * deltaT;
    }

    template<class Problem, class Element, class FVElementGeometry,
             class ElementVolumeVariables, class ElementFluxVariablesCache>
    static auto transmissibility(const Problem& problem,
                                 const Element& element,
                                 const FVElementGeometry& fvGeometry,
                                 const ElementVolumeVariables& elemVolVars,
                                 const typename FVElementGeometry::SubControlVolumeFace& scvf,
                                 const ElementFluxVariablesCache& elemFluxVarsCache)
    {
        return BaseLaw::transmissibility(problem, element, fvGeometry, elemVolVars, scvf, elemFluxVarsCache);
    }
};

template<bool isFluid>
struct FlexibleFouriersLaw
{
    enum class Mode {pyramid, fixedFactor, fancyFactor, tpfa};

    template<class Problem, class Element, class FVElementGeometry,
             class ElementVolumeVariables, class ElementFluxVariablesCache>
    static auto flux(const Problem& problem,
                     const Element& element,
                     const FVElementGeometry& fvGeometry,
                     const ElementVolumeVariables& elemVolVars,
                     const typename FVElementGeometry::SubControlVolumeFace& scvf,
                     const ElementFluxVariablesCache& elemFluxVarsCache)
    {
        const auto& insideScv = fvGeometry.scv(scvf.insideScvIdx());
        const auto& outsideScv = fvGeometry.scv(scvf.outsideScvIdx());
        const auto& insideVolVars = elemVolVars[insideScv];
        const auto& outsideVolVars = elemVolVars[outsideScv];
        const auto deltaT = insideVolVars.temperature() - outsideVolVars.temperature();
        return transmissibility(problem, element, fvGeometry, elemVolVars, scvf, elemFluxVarsCache) * deltaT;
    }

    template<class Problem, class Element, class FVElementGeometry,
             class ElementVolumeVariables, class ElementFluxVariablesCache>
    static auto transmissibility(const Problem& problem,
                                 const Element& element,
                                 const FVElementGeometry& fvGeometry,
                                 const ElementVolumeVariables& elemVolVars,
                                 const typename FVElementGeometry::SubControlVolumeFace& scvf,
                                 const ElementFluxVariablesCache& elemFluxVarsCache)
    {
        static const Mode mode = [&]()
        {
            const auto m = getParamFromGroup<std::string>(problem.paramGroup(), "Problem.ThroatConductionType");
            if (m == "Pyramid")
                return Mode::pyramid;
            else if (m == "FancyFactor")
                return Mode::fancyFactor;
            else if (m == "FixedFactor")
                return Mode::fixedFactor;
            else
                return Mode::tpfa;
        }();

        if (mode == Mode::pyramid)
            return FluidOrGrainPyramidFouriersLaw<isFluid>::transmissibility(problem, element, fvGeometry, elemVolVars, scvf, elemFluxVarsCache);
        else if (mode == Mode::fixedFactor)
            return FixedFactorFouriersLaw<isFluid>::transmissibility(problem, element, fvGeometry, elemVolVars, scvf, elemFluxVarsCache);
        else if (mode == Mode::fancyFactor)
            return FancyFactorFouriersLaw<isFluid>::transmissibility(problem, element, fvGeometry, elemVolVars, scvf, elemFluxVarsCache);
        else return 0.0; // TODO implement TPFA
    }
};


} // end namespace Dumux::PoreNetwork

#endif
