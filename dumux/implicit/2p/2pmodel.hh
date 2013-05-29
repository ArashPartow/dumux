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
* \brief Adaption of the fully implicit scheme to the two-phase flow model.
*/

#ifndef DUMUX_TWOP_MODEL_HH
#define DUMUX_TWOP_MODEL_HH

#include <dumux/implicit/common/implicitvelocityoutput.hh>
#include "2pproperties.hh"

namespace Dumux
{

/*!
 * \ingroup TwoPBoxModel
 * \brief A two-phase, isothermal flow model using the fully implicit scheme.
 *
 * This model implements two-phase flow of two immiscible fluids
 * \f$\alpha \in \{ w, n \}\f$ using a standard multiphase Darcy
 * approach as the equation for the conservation of momentum, i.e.
 \f[
 v_\alpha = - \frac{k_{r\alpha}}{\mu_\alpha} \textbf{K}
 \left(\textbf{grad}\, p_\alpha - \varrho_{\alpha} {\textbf g} \right)
 \f]
 *
 * By inserting this into the equation for the conservation of the
 * phase mass, one gets
 \f[
 \phi \frac{\partial \varrho_\alpha S_\alpha}{\partial t}
 -
 \text{div} \left\{
 \varrho_\alpha \frac{k_{r\alpha}}{\mu_\alpha} \mbox{\bf K} \left(\textbf{grad}\, p_\alpha - \varrho_{\alpha} \mbox{\bf g} \right)
 \right\} - q_\alpha = 0 \;,
 \f]
 *
 * All equations are discretized using a vertex-centered finite volume (box)
 * or cell-centered finite volume scheme as spatial
 * and the implicit Euler method as time discretization.
 *
 * By using constitutive relations for the capillary pressure \f$p_c =
 * p_n - p_w\f$ and relative permeability \f$k_{r\alpha}\f$ and taking
 * advantage of the fact that \f$S_w + S_n = 1\f$, the number of
 * unknowns can be reduced to two. Currently the model supports
 * choosing either \f$p_w\f$ and \f$S_n\f$ or \f$p_n\f$ and \f$S_w\f$
 * as primary variables. The formulation which ought to be used can be
 * specified by setting the <tt>Formulation</tt> property to either
 * <tt>TwoPCommonIndices::pWsN</tt> or <tt>TwoPCommonIndices::pNsW</tt>. By
 * default, the model uses \f$p_w\f$ and \f$S_n\f$.
 */
template<class TypeTag >
class TwoPModel : public GET_PROP_TYPE(TypeTag, BaseModel)
{
    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, VolumeVariables) VolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, SolutionVector) SolutionVector;
    typedef typename GET_PROP_TYPE(TypeTag, FluxVariables) FluxVariables;
    typedef typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables) ElementVolumeVariables;

    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    enum {
        nPhaseIdx = Indices::nPhaseIdx,
        wPhaseIdx = Indices::wPhaseIdx,
        pressureIdx = Indices::pressureIdx,
        numPhases = GET_PROP_VALUE(TypeTag, NumPhases)
    };

    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    typedef typename GridView::ctype CoordScalar;
    enum {
        dim = GridView::dimension,
        dimWorld = GridView::dimensionworld
    };

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef Dune::FieldVector<Scalar, numPhases> PhasesVector;
    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;

    enum { isBox = GET_PROP_VALUE(TypeTag, ImplicitIsBox) };
    enum { dofCodim = isBox ? dim : 0 };

public:
    /*!
     * \brief Append all quantities of interest which can be derived
     *        from the solution of the current time step to the VTK
     *        writer.
     *
     *        \param sol The global solution vector
     *        \param writer The writer for multi-file VTK datasets
     */
    template<class MultiWriter>
    void addOutputVtkFields(const SolutionVector &sol,
                            MultiWriter &writer)
    {
        typedef Dune::BlockVector<Dune::FieldVector<double, 1> > ScalarField;
        typedef Dune::BlockVector<Dune::FieldVector<double, dim> > VectorField;

        // get the number of degrees of freedom
        unsigned numDofs = this->numDofs();

        // create the required scalar fields
        ScalarField *pw = writer.allocateManagedBuffer(numDofs);
        ScalarField *pn = writer.allocateManagedBuffer(numDofs);
        ScalarField *pc = writer.allocateManagedBuffer(numDofs);
        ScalarField *sw = writer.allocateManagedBuffer(numDofs);
        ScalarField *sn = writer.allocateManagedBuffer(numDofs);
        ScalarField *rhoW = writer.allocateManagedBuffer(numDofs);
        ScalarField *rhoN = writer.allocateManagedBuffer(numDofs);
        ScalarField *mobW = writer.allocateManagedBuffer(numDofs);
        ScalarField *mobN = writer.allocateManagedBuffer(numDofs);
        ScalarField *poro = writer.allocateManagedBuffer(numDofs);
        ScalarField *Te = writer.allocateManagedBuffer(numDofs);
        VectorField *velocityN = writer.template allocateManagedBuffer<double, dim>(numDofs);
        VectorField *velocityW = writer.template allocateManagedBuffer<double, dim>(numDofs);
        ImplicitVelocityOutput<TypeTag> velocityOutput(this->problem_());

        if (velocityOutput.enableOutput()) // check if velocity output is demanded
        {
            // initialize velocity fields
            for (unsigned int i = 0; i < numDofs; ++i)
            {
                (*velocityN)[i] = Scalar(0);
                (*velocityW)[i] = Scalar(0);
            }
        }

        unsigned numElements = this->gridView_().size(0);
        ScalarField *rank = writer.allocateManagedBuffer(numElements);

        ElementIterator eIt = this->gridView_().template begin<0>();
        ElementIterator eEndIt = this->gridView_().template end<0>();
        for (; eIt != eEndIt; ++eIt)
        {
            int idx = this->elementMapper().map(*eIt);
            (*rank)[idx] = this->gridView_().comm().rank();

            FVElementGeometry fvGeometry;
            fvGeometry.update(this->gridView_(), *eIt);

            ElementVolumeVariables elemVolVars;
            elemVolVars.update(this->problem_(),
                               *eIt,
                               fvGeometry,
                               false /* oldSol? */);

            for (int scvIdx = 0; scvIdx < fvGeometry.numScv; ++scvIdx)
            {
                int globalIdx = this->dofMapper().map(*eIt, scvIdx, dofCodim);

                (*pw)[globalIdx] = elemVolVars[scvIdx].pressure(wPhaseIdx);
                (*pn)[globalIdx] = elemVolVars[scvIdx].pressure(nPhaseIdx);
                (*pc)[globalIdx] = elemVolVars[scvIdx].capillaryPressure();
                (*sw)[globalIdx] = elemVolVars[scvIdx].saturation(wPhaseIdx);
                (*sn)[globalIdx] = elemVolVars[scvIdx].saturation(nPhaseIdx);
                (*rhoW)[globalIdx] = elemVolVars[scvIdx].density(wPhaseIdx);
                (*rhoN)[globalIdx] = elemVolVars[scvIdx].density(nPhaseIdx);
                (*mobW)[globalIdx] = elemVolVars[scvIdx].mobility(wPhaseIdx);
                (*mobN)[globalIdx] = elemVolVars[scvIdx].mobility(nPhaseIdx);
                (*poro)[globalIdx] = elemVolVars[scvIdx].porosity();
                (*Te)[globalIdx] = elemVolVars[scvIdx].temperature();
            }

            // velocity output
            velocityOutput.calculateVelocity(*velocityW, elemVolVars, fvGeometry, *eIt, wPhaseIdx);
            velocityOutput.calculateVelocity(*velocityN, elemVolVars, fvGeometry, *eIt, nPhaseIdx);
        }

        writer.attachDofData(*sn, "sn", isBox);
        writer.attachDofData(*sw, "sw", isBox);
        writer.attachDofData(*pn, "pn", isBox);
        writer.attachDofData(*pw, "pw", isBox);
        writer.attachDofData(*pc, "pc", isBox);
        writer.attachDofData(*rhoW, "rhoW", isBox);
        writer.attachDofData(*rhoN, "rhoN", isBox);
        writer.attachDofData(*mobW, "mobW", isBox);
        writer.attachDofData(*mobN, "mobN", isBox);
        writer.attachDofData(*poro, "porosity", isBox);
        writer.attachDofData(*Te, "temperature", isBox);

        if (velocityOutput.enableOutput()) // check if velocity output is demanded
        {
            writer.attachDofData(*velocityW,  "velocityW", isBox, dim);
            writer.attachDofData(*velocityN,  "velocityN", isBox, dim);
        }

        writer.attachCellData(*rank, "process rank");
    }
};
}

#include "2ppropertydefaults.hh"

#endif
