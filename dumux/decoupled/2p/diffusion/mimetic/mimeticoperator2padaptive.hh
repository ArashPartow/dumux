// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/****************************************************************************
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

#ifndef DUMUX_MIMETICOPERATOR2PADAPTIVE_HH
#define DUMUX_MIMETICOPERATOR2PADAPTIVE_HH

/*!
 * \file
 *
 * \brief An assembler for the Jacobian matrix based on mimetic FD.
 */

#include"croperator2padaptive.hh"
#include <dumux/decoupled/2p/diffusion/diffusionproperties2p.hh>
#include "dumux/decoupled/common/mimetic/mimeticproperties.hh"

namespace Dumux
{
/*!
 * \ingroup Mimetic2P
 * @brief Levelwise assembler

  This class serves as a base class for local assemblers. It provides
  space and access to the local stiffness matrix. The actual assembling is done
  in a derived class via the virtual assemble method.

  The template parameters are:

  - Scalar The field type used in the elements of the stiffness matrix
*/
template<class TypeTag>
class MimeticOperatorAssemblerTwoPAdaptive : public CROperatorAssemblerTwoPAdaptive<TypeTag>
{
    typedef CROperatorAssemblerTwoPAdaptive<TypeTag> ParentType;

    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;

    enum
    {
        dim=GridView::dimension,
        dimWorld=GridView::dimensionworld
    };
    typedef typename GET_PROP_TYPE(TypeTag, LocalStiffness) LocalStiffness;

    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<0>::EntityPointer ElementPointer;
    typedef typename GridView::IntersectionIterator IntersectionIterator;

    typedef typename GET_PROP_TYPE(TypeTag, CellData) CellData;
    typedef typename GET_PROP_TYPE(TypeTag, MaterialLaw) MaterialLaw;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, FluidState) FluidState;
    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;
    typedef typename GET_PROP(TypeTag, SolutionTypes)::PrimaryVariables PrimaryVariables;

    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;

    enum
    {
        pw = Indices::pressureW,
        pn = Indices::pressureNw,
        pressureType = GET_PROP_VALUE(TypeTag, PressureFormulation),
        wPhaseIdx = Indices::wPhaseIdx,
        nPhaseIdx = Indices::nPhaseIdx,
        saturationIdx = Indices::saturationIdx,
        satEqIdx = Indices::satEqIdx
    };

    typedef Dune::FieldVector<Scalar, dimWorld> FieldVector;
public:

    MimeticOperatorAssemblerTwoPAdaptive (const GridView& gridView)
    : ParentType(gridView)
    {}

    template<class Vector>
    void calculatePressure(LocalStiffness& loc, Vector& u, Problem& problem)
    {
        Dune::DynamicVector<Scalar> velocityW(2*dim);
        Dune::DynamicVector<Scalar> velocityNw(2*dim);
        Dune::DynamicVector<Scalar> pressTraceW(2*dim);
        Dune::DynamicVector<Scalar> pressTraceNw(2*dim);

        // run over all level elements
        ElementIterator eIt = this->gridView_.template begin<0>();
        ElementIterator eItEnd = this->gridView_.template end<0>();

        FluidState fluidState;
        fluidState.setPressure(wPhaseIdx, problem.referencePressure(*eIt));
        fluidState.setPressure(nPhaseIdx, problem.referencePressure(*eIt));
        fluidState.setTemperature(problem.temperature(*eIt));
        fluidState.setSaturation(wPhaseIdx, 1.);
        fluidState.setSaturation(nPhaseIdx, 0.);
        Scalar densityDiff = FluidSystem::density(fluidState, nPhaseIdx) - FluidSystem::density(fluidState, wPhaseIdx);
        Scalar viscosityW = FluidSystem::viscosity(fluidState, wPhaseIdx);
        Scalar viscosityNw = FluidSystem::viscosity(fluidState, nPhaseIdx);

        //reset velocity
        for (int i = 0; i < problem.gridView().size(0); i++)
        {
            problem.variables().cellData(i).fluxData().resetVelocity();
        }

        for (; eIt != eItEnd; ++eIt)
        {
            int globalIdx = problem.variables().index(*eIt);

            unsigned int numFaces = this->intersectionMapper_.size(globalIdx);

            // get local to global id map and pressure traces
            velocityW.resize(numFaces);
            velocityNw.resize(numFaces);
            pressTraceW.resize(numFaces);
            pressTraceNw.resize(numFaces);

            CellData& cellData = problem.variables().cellData(globalIdx);
            FieldVector globalPos = eIt->geometry().center();

            int intersectionIdx = -1;
            // get local to global id map and pressure traces
            IntersectionIterator isIt = problem.gridView().template ibegin(*eIt);
            const IntersectionIterator &isItEnd = problem.gridView().template iend(*eIt);
            for (; isIt != isItEnd; ++isIt)
            {
                ++intersectionIdx;

                int globalIdxFace = this->intersectionMapper_.map(*eIt, intersectionIdx);

                Scalar pcPotFace = (problem.bBoxMax() - isIt->geometry().center()) * problem.gravity() * densityDiff;

                switch (pressureType)
                {
                case pw:
                {
                    pressTraceW[intersectionIdx] = u[globalIdxFace];
                    pressTraceNw[intersectionIdx] = u[globalIdxFace] + pcPotFace;
                    break;
                }
                case pn:
                {
                    pressTraceNw[intersectionIdx] = u[globalIdxFace];
                    pressTraceW[intersectionIdx] = u[globalIdxFace] - pcPotFace;

                    break;
                }
                }
            }

            switch (pressureType)
            {
            case pw:
            {
                Scalar potW = loc.constructPressure(*eIt, pressTraceW);
                Scalar gravPot = (problem.bBoxMax() - globalPos) * problem.gravity() * densityDiff;
                Scalar potNw = potW + gravPot;

                cellData.setPotential(wPhaseIdx, potW);
                cellData.setPotential(nPhaseIdx, potNw);

                gravPot = (problem.bBoxMax() - globalPos) * problem.gravity() * FluidSystem::density(fluidState, wPhaseIdx);

                cellData.setPressure(wPhaseIdx, potW - gravPot);

                gravPot = (problem.bBoxMax() - globalPos) * problem.gravity() * FluidSystem::density(fluidState, nPhaseIdx);

                cellData.setPressure(nPhaseIdx, potNw - gravPot);

                break;
            }
            case pn:
            {
                Scalar potNw = loc.constructPressure(*eIt, pressTraceNw);
                Scalar  gravPot = (problem.bBoxMax() - globalPos) * problem.gravity() * densityDiff;
                Scalar potW = potNw - gravPot;

                cellData.setPotential(nPhaseIdx, potNw);
                cellData.setPotential(wPhaseIdx, potW);

                gravPot = (problem.bBoxMax() - globalPos) * problem.gravity() * FluidSystem::density(fluidState, wPhaseIdx);

                cellData.setPressure(wPhaseIdx, potW - gravPot);

                gravPot = (problem.bBoxMax() - globalPos) * problem.gravity() * FluidSystem::density(fluidState, nPhaseIdx);

                cellData.setPressure(nPhaseIdx, potNw - gravPot);

                break;
            }
            }

            //velocity reconstruction: !!! The velocity which is not reconstructed from the primary pressure variable can be slightly wrong and not conservative!!!!
            // -> Should not be used for transport!!
            loc.constructVelocity(*eIt, velocityW, pressTraceW, cellData.potential(wPhaseIdx));
            loc.constructVelocity(*eIt, velocityNw, pressTraceNw, cellData.potential(nPhaseIdx));

            intersectionIdx = -1;
            isIt = problem.gridView().template ibegin(*eIt);
            for (; isIt != isItEnd; ++isIt)
            {
                ++intersectionIdx;
                int idxInInside = isIt->indexInInside();

                cellData.fluxData().addUpwindPotential(wPhaseIdx, idxInInside, velocityW[intersectionIdx]);
                cellData.fluxData().addUpwindPotential(nPhaseIdx, idxInInside, velocityNw[intersectionIdx]);

                Scalar mobilityW = 0;
                Scalar mobilityNw = 0;

                if (isIt->neighbor())
                {
                    int neighborIdx = problem.variables().index(*(isIt->outside()));

                    CellData& cellDataNeighbor = problem.variables().cellData(neighborIdx);

                    mobilityW =
                            (velocityW[intersectionIdx] >= 0.) ? cellData.mobility(wPhaseIdx) :
                                    cellDataNeighbor.mobility(wPhaseIdx);
                    mobilityNw =
                            (velocityNw[intersectionIdx] >= 0.) ? cellData.mobility(nPhaseIdx) :
                                    cellDataNeighbor.mobility(nPhaseIdx);

                    if (velocityW[intersectionIdx] >= 0.)
                    {
                        FieldVector velocity(isIt->centerUnitOuterNormal());
                        velocity *= mobilityW/(mobilityW+mobilityNw) * velocityW[intersectionIdx];
                        cellData.fluxData().addVelocity(wPhaseIdx, idxInInside, velocity);
                        cellDataNeighbor.fluxData().addVelocity(wPhaseIdx, isIt->indexInOutside(), velocity);
                    }
                    if (velocityNw[intersectionIdx] >= 0.)
                    {
                        FieldVector velocity(isIt->centerUnitOuterNormal());
                        velocity *= mobilityNw/(mobilityW+mobilityNw) * velocityNw[intersectionIdx];
                        cellData.fluxData().addVelocity(nPhaseIdx, idxInInside, velocity);
                        cellDataNeighbor.fluxData().addVelocity(nPhaseIdx, isIt->indexInOutside(), velocity);
                    }

                    cellData.fluxData().setVelocityMarker(idxInInside);
                }
                else
                {
                    BoundaryTypes bctype;
                    problem.boundaryTypes(bctype, *isIt);
                    if (bctype.isDirichlet(satEqIdx))
                    {
                        PrimaryVariables boundValues(0.0);
                        problem.dirichlet(boundValues, *isIt);

                        if (velocityW[intersectionIdx] >= 0.)
                        {
                            mobilityW = cellData.mobility(wPhaseIdx);
                        }
                        else
                        {
                            mobilityW = MaterialLaw::krw(problem.spatialParams().materialLawParams(*eIt),
                                boundValues[saturationIdx]) / viscosityW;
                        }

                        if (velocityNw[intersectionIdx] >= 0.)
                        {
                            mobilityNw = cellData.mobility(nPhaseIdx);
                        }
                        else
                        {
                            mobilityNw = MaterialLaw::krn(problem.spatialParams().materialLawParams(*eIt),
                                boundValues[saturationIdx]) / viscosityNw;
                        }
                    }
                    else
                    {
                        mobilityW = cellData.mobility(wPhaseIdx);
                        mobilityNw = cellData.mobility(nPhaseIdx);
                    }

                    FieldVector velocity(isIt->centerUnitOuterNormal());
                    velocity *= mobilityW/(mobilityW+mobilityNw) * velocityW[intersectionIdx];
                    cellData.fluxData().addVelocity(wPhaseIdx, idxInInside, velocity);


                    velocity = isIt->centerUnitOuterNormal();
                    velocity *= mobilityNw/(mobilityW+mobilityNw) * velocityNw[intersectionIdx];
                    cellData.fluxData().addVelocity(nPhaseIdx, idxInInside, velocity);
                    cellData.fluxData().setVelocityMarker(idxInInside);
                }
            }
        }
    }
};
}
#endif
