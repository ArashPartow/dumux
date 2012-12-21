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
/*!
 * \file
 *
 * \ingroup IMPETtests
 * \brief Calculate errors for the diffusion test problem.
 */
#ifndef DUMUX_BENCHMARKRESULT_HH
#define DUMUX_BENCHMARKRESULT_HH

#include <dumux/decoupled/common/onemodelproblem.hh>

namespace Dumux
{
/*!
 * \brief Calculate errors for the diffusion test problem. 
 */
struct ResultEvaluation
{
public:
    double relativeL2Error;
    double ergrad;
    double ervell2;
    double uMin;
    double uMax;
    double flux0;
    double flux1;
    double fluy0;
    double fluy1;
    double sumf;
    double sumflux;
    double exactflux0;
    double exactflux1;
    double exactfluy0;
    double exactfluy1;
    double errflx0;
    double errflx1;
    double errfly0;
    double errfly1;
    double erflm;
    double ener1;

    /*!
     * \brief Calculate errors for the diffusion test problem. 
     * 
     * \param gridView the GridView for which the result should be evaluated
     * \param problem the Problem at hand
     * \param consecutiveNumbering indicates the order in which the 
     * velocities are stored in the flux data
     */
    template<class GridView, class Problem>
    void evaluate(const GridView& gridView,
            Problem& problem, bool consecutiveNumbering = false)
    {
        typedef typename GridView::Grid Grid;
        typedef typename Grid::ctype Scalar;
        enum {dim=Grid::dimension};
        typedef typename Grid::template Codim<0>::Entity Element;
        typedef typename Element::Geometry Geometry;
        typedef typename GridView::template Codim<0>::Iterator ElementIterator;
        typedef typename GridView::IntersectionIterator IntersectionIterator;
#if DUNE_VERSION_NEWER(DUNE_GRID, 2, 3)
        typedef typename Geometry::JacobianInverseTransposed JacobianInverseTransposed;
#else
        typedef typename Geometry::Jacobian JacobianInverseTransposed;    
#endif

        uMin = 1e100;
        uMax = -1e100;
        flux0 = 0;
        flux1 = 0;
        fluy0 = 0;
        fluy1 = 0;
        sumf = 0;
        sumflux = 0;
        exactflux0 = 0;
        exactflux1 = 0;
        exactfluy0 = 0;
        exactfluy1 = 0;
        erflm = 0;
        ener1 = 0;
        Scalar numerator = 0;
        Scalar denominator = 0;
        double numeratorGrad = 0;
        double denominatorGrad = 0;
        double numeratorFlux = 0;
        double denominatorFlux = 0;
        ElementIterator endEIt = gridView.template end<0>();
        for (ElementIterator eIt = gridView.template begin<0>(); eIt != endEIt; ++eIt)
        {
            const Element& element = *eIt;

            // element geometry
            const Geometry& geometry = element.geometry();

            Dune::GeometryType geomType = geometry.type();

            const Dune::FieldVector<Scalar,dim>& local = Dune::GenericReferenceElements<Scalar,dim>::general(geomType).position(0, 0);
            Dune::FieldVector<Scalar,dim> global = geometry.global(local);

            Scalar volume = geometry.volume();

            int eIdx = problem.variables().index(element);

            Scalar approxPressure = problem.variables().cellData(eIdx).globalPressure();
            Scalar exactPressure = problem.exact(global);

            numerator += volume*(approxPressure - exactPressure)*(approxPressure - exactPressure);
            denominator += volume*exactPressure*exactPressure;

            // update uMin and uMax
            uMin = std::min(uMin, approxPressure);
            uMax = std::max(uMax, approxPressure);

            typename Problem::PrimaryVariables sourceVec;
            problem.source(sourceVec, element);
            sumf += volume*sourceVec[0];

            // get the absolute permeability
            Dune::FieldMatrix<double,dim,dim> K = problem.spatialParams().intrinsicPermeability(element);

            int isIdx = -1;
            Dune::FieldVector<Scalar,2*dim> fluxVector;
            Dune::FieldVector<Scalar,dim> exactGradient;
            IntersectionIterator endis = gridView.iend(element);
            for (IntersectionIterator is = gridView.ibegin(element); is!=endis; ++is)
            {
                // local number of facet
                int faceIdx = is->indexInInside();

                if (consecutiveNumbering)
                    isIdx++;
                else
                    isIdx = faceIdx;

                Dune::FieldVector<double,dim> faceGlobal = is->geometry().center();
                double faceVol = is->geometry().volume();

                // get normal vector
                Dune::FieldVector<double,dim> unitOuterNormal = is->centerUnitOuterNormal();

                // get the exact gradient
                exactGradient = problem.exactGrad(faceGlobal);

                // get the negative exact velocity
                Dune::FieldVector<double,dim> KGrad(0);
                K.umv(exactGradient, KGrad);

                // calculate the exact normal velocity
                double exactFlux = KGrad*unitOuterNormal;

                // get the approximate normalvelocity
                double approximateFlux = problem.variables().cellData(eIdx).fluxData().velocityTotal(isIdx)*unitOuterNormal;

                // calculate the difference in the normal velocity
                double fluxDiff = exactFlux + approximateFlux;

                // update mean value error
                erflm = std::max(erflm, fabs(fluxDiff));

                numeratorFlux += volume*fluxDiff*fluxDiff;
                denominatorFlux += volume*exactFlux*exactFlux;

                // calculate the fluxes through the element faces
                exactFlux *= faceVol;
                approximateFlux *= faceVol;
                fluxVector[faceIdx] = approximateFlux;

                if (!is->neighbor()) {
                    if (fabs(faceGlobal[1]) < 1e-6) {
                        fluy0 += approximateFlux;
                        exactfluy0 += exactFlux;
                    }
                    else if (fabs(faceGlobal[1] - 1.0) < 1e-6) {
                        fluy1 += approximateFlux;
                        exactfluy1 += exactFlux;
                    }
                    else if (faceGlobal[0] < 1e-6) {
                        flux0 += approximateFlux;
                        exactflux0 += exactFlux;
                    }
                    else if (fabs(faceGlobal[0] - 1.0) < 1e-6) {
                        flux1 += approximateFlux;
                        exactflux1 += exactFlux;
                    }
                }
            }

            // calculate velocity on reference element
            Dune::FieldVector<Scalar,dim> refVelocity;
            if (geometry.corners() == 3) {
                refVelocity[0] = 1.0/3.0*(fluxVector[0] + fluxVector[2] - 2.0*fluxVector[1]);
                refVelocity[1] = 1.0/3.0*(fluxVector[0] + fluxVector[1] - 2.0*fluxVector[2]);
            }
            else {
                refVelocity[0] = 0.5*(fluxVector[1] - fluxVector[0]);
                refVelocity[1] = 0.5*(fluxVector[3] - fluxVector[2]);
            }

            // get the transposed Jacobian of the element mapping
            const JacobianInverseTransposed& jacobianInv = geometry.jacobianInverseTransposed(local);
            JacobianInverseTransposed jacobianT(jacobianInv);
            jacobianT.invert();

            // calculate the element velocity by the Piola transformation
            Dune::FieldVector<Scalar,dim> elementVelocity(0);
            jacobianT.umtv(refVelocity, elementVelocity);
            elementVelocity /= geometry.integrationElement(local);

            // get the approximate gradient
            Dune::FieldVector<Scalar,dim> approximateGradient;
            K.solve(approximateGradient, elementVelocity);

            // get the exact gradient
            exactGradient = problem.exactGrad(global);

            // the difference between exact and approximate gradient
            Dune::FieldVector<Scalar,dim> gradDiff(exactGradient);
            gradDiff += approximateGradient;

            // add to energy
            ener1 += volume*(approximateGradient*elementVelocity);

            numeratorGrad += volume*(gradDiff*gradDiff);
            denominatorGrad += volume*(exactGradient*exactGradient);
        }

        relativeL2Error = sqrt(numerator/denominator);
        ergrad = sqrt(numeratorGrad/denominatorGrad);
        ervell2 = sqrt(numeratorFlux/denominatorFlux);
        sumflux = flux0 + flux1 + fluy0 + fluy1 - sumf;
        errflx0 = fabs((flux0 + exactflux0)/exactflux0);
        errflx1 = fabs((flux1 + exactflux1)/exactflux1);
        errfly0 = fabs((fluy0 + exactfluy0)/exactfluy0);
        errfly1 = fabs((fluy1 + exactfluy1)/exactfluy1);
    }
};



}

#endif
