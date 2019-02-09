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
 * \ingroup FacetCoupling
 * \copydoc Dumux::BoxFacetCouplingDarcysLaw
 */
#ifndef DUMUX_DISCRETIZATION_BOX_FACET_COUPLING_DARCYS_LAW_HH
#define DUMUX_DISCRETIZATION_BOX_FACET_COUPLING_DARCYS_LAW_HH

#include <dune/common/exceptions.hh>
#include <dune/common/fvector.hh>
#include <dune/common/float_cmp.hh>

#include <dumux/common/math.hh>
#include <dumux/common/parameters.hh>
#include <dumux/common/properties.hh>

#include <dumux/discretization/method.hh>
#include <dumux/flux/box/darcyslaw.hh>

namespace Dumux {

/*!
 * \ingroup FacetCoupling
 * \brief Darcy's law for the box scheme scheme in the context of coupled models
 *        where coupling occurs across the facets of the bulk domain elements
 *        with a lower-dimensional domain living on these facets.
 */
template<class Scalar, class FVGridGeometry>
class BoxFacetCouplingDarcysLaw
{
    using DefaultBoxDarcysLaw = BoxDarcysLaw<Scalar, FVGridGeometry>;

    using FVElementGeometry = typename FVGridGeometry::LocalView;
    using SubControlVolume = typename FVElementGeometry::SubControlVolume;
    using SubControlVolumeFace = typename FVElementGeometry::SubControlVolumeFace;
    using GridView = typename FVGridGeometry::GridView;
    using Element = typename GridView::template Codim<0>::Entity;
    using CoordScalar = typename GridView::ctype;

    static constexpr int dim = GridView::dimension;
    static constexpr int dimWorld = GridView::dimensionworld;

public:

    template<class Problem, class ElementVolumeVariables, class ElementFluxVarsCache>
    static Scalar flux(const Problem& problem,
                       const Element& element,
                       const FVElementGeometry& fvGeometry,
                       const ElementVolumeVariables& elemVolVars,
                       const SubControlVolumeFace& scvf,
                       const int phaseIdx,
                       const ElementFluxVarsCache& elemFluxVarCache)
    {
        // if this scvf is not on an interior boundary, use the standard law
        if (!scvf.interiorBoundary())
            return DefaultBoxDarcysLaw::flux(problem, element, fvGeometry, elemVolVars, scvf, phaseIdx, elemFluxVarCache);

        static const Scalar xi = getParamFromGroup<Scalar>(problem.paramGroup(), "FacetCoupling.Xi", 1.0);
        if ( !Dune::FloatCmp::eq(xi, 1.0, 1e-6) )
            DUNE_THROW(Dune::NotImplemented, "Xi != 1.0 cannot be used with the Box-Facet-Coupling scheme");

        // get some references for convenience
        const auto& fluxVarCache = elemFluxVarCache[scvf];
        const auto& shapeValues = fluxVarCache.shapeValues();
        const auto& insideScv = fvGeometry.scv(scvf.insideScvIdx());
        const auto& insideVolVars = elemVolVars[insideScv];

        // evaluate user-defined interior boundary types
        const auto bcTypes = problem.interiorBoundaryTypes(element, scvf);

        static const bool enableGravity = getParamFromGroup<bool>(problem.paramGroup(), "Problem.EnableGravity");

        // on interior Neumann boundaries, evaluate the flux using the facet permeability
        if (bcTypes.hasOnlyNeumann())
        {
            // compute point on element whose connection vector to
            // the scvf integration point is parallel to the normal
            const auto& elemGeometry = element.geometry();
            const auto d1 = scvf.ipGlobal() - insideScv.dofPosition();
            const auto d2 = elemGeometry.center() - insideScv.dofPosition();

            const auto d1Norm = d1.two_norm();
            const auto d2Norm = d2.two_norm();

            using std::tan;
            using std::acos;
            const auto angle = acos( (d1*d2)/d1Norm/d2Norm );
            const auto dm = tan(angle)*d1Norm;

            auto pos = scvf.unitOuterNormal();
            pos *= -1.0*dm;
            pos += scvf.ipGlobal();

            const auto posLocal = elemGeometry.local(pos);
            std::vector< Dune::FieldVector<Scalar, 1> > insideShapeValues;
            fvGeometry.fvGridGeometry().feCache().get(elemGeometry.type()).localBasis().evaluateFunction(posLocal, insideShapeValues);

            Scalar rho = 0.0;
            Scalar pInside = 0.0;
            for (const auto& scv : scvs(fvGeometry))
            {
                const auto& volVars = elemVolVars[scv];
                rho += volVars.density(phaseIdx)*shapeValues[scv.indexInElement()][0];
                pInside += volVars.pressure(phaseIdx)*insideShapeValues[scv.indexInElement()][0];
            }

            // compute tpfa flux such that flux and pressure continuity holds
            const auto& facetVolVars = problem.couplingManager().getLowDimVolVars(element, scvf);

            // On surface grids, use sqrt of aperture as distance measur
            using std::sqrt;
            const auto df = dim == dimWorld ? 0.5*facetVolVars.extrusionFactor()
                                            : 0.5*sqrt(facetVolVars.extrusionFactor());

            const auto tm = 1.0/dm*vtmv(scvf.unitOuterNormal(), insideVolVars.permeability(), scvf.unitOuterNormal());
            const auto tf = 1.0/df*vtmv(scvf.unitOuterNormal(), facetVolVars.permeability(), scvf.unitOuterNormal());

            auto flux = tm*tf/(tm+tf)*(pInside - facetVolVars.pressure(phaseIdx))
                        *scvf.area()*insideVolVars.extrusionFactor();

            if (enableGravity)
                flux -= rho*scvf.area()*insideVolVars.extrusionFactor()
                        *vtmv(scvf.unitOuterNormal(), insideVolVars.permeability(), problem.gravityAtPos(scvf.center()));

            return flux;
        }

        // on interior Dirichlet boundaries use the facet pressure and evaluate flux
        else if (bcTypes.hasOnlyDirichlet())
        {
            // create vector with nodal pressures
            std::vector<Scalar> pressures(element.subEntities(dim));
            for (const auto& scv : scvs(fvGeometry))
                pressures[scv.localDofIndex()] = elemVolVars[scv].pressure(phaseIdx);

            // substitute with facet pressures for those scvs touching this facet
            for (const auto& scvfJ : scvfs(fvGeometry))
                if (scvfJ.interiorBoundary() && scvfJ.facetIndexInElement() == scvf.facetIndexInElement())
                    pressures[ fvGeometry.scv(scvfJ.insideScvIdx()).localDofIndex() ]
                             = problem.couplingManager().getLowDimVolVars(element, scvfJ).pressure(phaseIdx);

            // evaluate gradP - rho*g at integration point
            Scalar rho(0.0);
            Dune::FieldVector<Scalar, dimWorld> gradP(0.0);
            for (const auto& scv : scvs(fvGeometry))
            {
                rho += elemVolVars[scv].density(phaseIdx)*shapeValues[scv.indexInElement()][0];
                gradP.axpy(pressures[scv.localDofIndex()], fluxVarCache.gradN(scv.indexInElement()));
            }

            if (enableGravity)
                gradP.axpy(-rho, problem.gravityAtPos(scvf.center()));

            // apply matrix permeability and return the flux
            return -1.0*scvf.area()
                       *insideVolVars.extrusionFactor()
                       *vtmv(scvf.unitOuterNormal(), insideVolVars.permeability(), gradP);
        }

        // mixed boundary types are not supported
        else
            DUNE_THROW(Dune::NotImplemented, "Mixed boundary types are not supported");
    }

    // compute transmissibilities ti for analytical jacobians
    template<class Problem, class ElementVolumeVariables, class FluxVarCache>
    static std::vector<Scalar> calculateTransmissibilities(const Problem& problem,
                                                           const Element& element,
                                                           const FVElementGeometry& fvGeometry,
                                                           const ElementVolumeVariables& elemVolVars,
                                                           const SubControlVolumeFace& scvf,
                                                           const FluxVarCache& fluxVarCache)
    {
        DUNE_THROW(Dune::NotImplemented, "transmissibilty computation for BoxFacetCouplingDarcysLaw");
    }
};

} // end namespace Dumux

#endif
