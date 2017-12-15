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
 * \brief A newton solver specific to the Richards problem.
 */
#ifndef DUMUX_RICHARDS_NEWTON_CONTROLLER_HH
#define DUMUX_RICHARDS_NEWTON_CONTROLLER_HH

#include <dumux/common/properties.hh>
#include <dumux/nonlinear/newtoncontroller.hh>

namespace Dumux {
/*!
 * \ingroup Newton
 * \brief A Richards model specific controller for the newton solver.
 *
 * This controller 'knows' what a 'physically meaningful' solution is
 * and can thus do update smarter than the plain Newton controller.
 */
template <class TypeTag>
class RichardsNewtonController : public NewtonController<TypeTag>
{
    using ParentType = NewtonController<TypeTag>;
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using SolutionVector = typename GET_PROP_TYPE(TypeTag, SolutionVector);
    using SpatialParams = typename GET_PROP_TYPE(TypeTag, SpatialParams);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using MaterialLaw = typename GET_PROP_TYPE(TypeTag, MaterialLaw);
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Communicator = typename GridView::CollectiveCommunication;
    using ElementSolution =  typename GET_PROP_TYPE(TypeTag, ElementSolutionVector);

    using Indices = typename GET_PROP_TYPE(TypeTag, Indices);
    enum { pressureIdx = Indices::pressureIdx };

public:
    /*!
     * \brief Constructor for stationary problems
     */
    RichardsNewtonController(const Communicator& comm)
    : ParentType(comm)
    {}

    /*!
     * \brief Constructor for stationary problems
     */
    RichardsNewtonController(const Communicator& comm, std::shared_ptr<TimeLoop<Scalar>> timeLoop)
    : ParentType(comm, timeLoop)
    {}

    /*!
     * \brief Update the current solution of the newton method
     *
     * This is basically the step
     * \f[ u^{k+1} = u^k - \Delta u^k \f]
     *
     * \param uCurrentIter The solution after the current Newton iteration \f$ u^{k+1} \f$
     * \param uLastIter The solution after the last Newton iteration \f$ u^k \f$
     * \param deltaU The vector of differences between the last
     *               iterative solution and the next one \f$ \Delta u^k \f$
     */
    template<class JacobianAssembler, class SolutionVector>
    void newtonUpdate(JacobianAssembler& assembler,
                      SolutionVector &uCurrentIter,
                      const SolutionVector &uLastIter,
                      const SolutionVector &deltaU)
    {
        ParentType::newtonUpdate(assembler, uCurrentIter, uLastIter, deltaU);
        const std::string group = GET_PROP_VALUE(TypeTag, ModelParameterGroup);
        if (!this->useLineSearch_ && getParamFromGroup<bool>(group, "Newton.EnableChop"))
        {
            // do not clamp anything after 5 iterations
            if (this->numSteps_ > 4)
                return;

            // clamp saturation change to at most 20% per iteration
            const auto& fvGridGeometry = assembler.fvGridGeometry();
            for (const auto& element : elements(fvGridGeometry.gridView()))
            {
                auto fvGeometry = localView(fvGridGeometry);
                fvGeometry.bindElement(element);

                for (auto&& scv : scvs(fvGeometry))
                {
                    auto dofIdxGlobal = scv.dofIndex();

                    // calculate the old wetting phase saturation
                    const auto& spatialParams = assembler.problem().spatialParams();
                    const ElementSolution elemSol(element, uCurrentIter, fvGridGeometry);
                    const auto& materialLawParams = spatialParams.materialLawParams(element, scv, elemSol);
                    const Scalar pcMin = MaterialLaw::pc(materialLawParams, 1.0);
                    const Scalar pw = uLastIter[dofIdxGlobal][pressureIdx];
                    using std::max;
                    const Scalar pn = max(assembler.problem().nonWettingReferencePressure(), pw + pcMin);
                    const Scalar pcOld = pn - pw;
                    const Scalar SwOld = max(0.0, MaterialLaw::sw(materialLawParams, pcOld));

                    // convert into minimum and maximum wetting phase
                    // pressures
                    const Scalar pwMin = pn - MaterialLaw::pc(materialLawParams, SwOld - 0.2);
                    const Scalar pwMax = pn - MaterialLaw::pc(materialLawParams, SwOld + 0.2);

                    // clamp the result
                    using std::min; using std::max;
                    uCurrentIter[dofIdxGlobal][pressureIdx] = max(pwMin, min(uCurrentIter[dofIdxGlobal][pressureIdx], pwMax));
                }
            }
        }
    }
};

} // end namespace Dumux

#endif
