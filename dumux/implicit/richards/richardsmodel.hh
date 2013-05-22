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
* \brief Adaption of the fully implicit scheme to the Richards model.
*/
#ifndef DUMUX_RICHARDS_MODEL_HH
#define DUMUX_RICHARDS_MODEL_HH

#include <dumux/implicit/common/implicitmodel.hh>
#include <dumux/implicit/common/implicitvelocityoutput.hh>

#include "richardslocalresidual.hh"
#include "richardsproblem.hh"

namespace Dumux
{
/*!
 * \ingroup RichardsModel
 *
 * \brief This model which implements a variant of the Richards'
 *        equation for quasi-twophase flow.
 *
 * In the unsaturated zone, Richards' equation
 \f[
 \frac{\partial\;\phi S_w \varrho_w}{\partial t}
 -
 \text{div} \left\lbrace
 \varrho_w \frac{k_{rw}}{\mu_w} \; \mathbf{K} \;
 \left( \text{\textbf{grad}}
 p_w - \varrho_w \textbf{g}
 \right)
 \right\rbrace
 =
 q_w,
 \f]
 * is frequently used to
 * approximate the water distribution above the groundwater level.
 * 
 * It can be derived from the two-phase equations, i.e.
 \f[
 \phi\frac{\partial S_\alpha \varrho_\alpha}{\partial t}
 -
 \text{div} \left\lbrace
 \varrho_\alpha \frac{k_{r\alpha}}{\mu_\alpha}\; \mathbf{K} \;
 \left( \text{\textbf{grad}}
 p_\alpha - \varrho_\alpha \textbf{g}
 \right)
 \right\rbrace
 =
 q_\alpha,
 \f]
 * where \f$\alpha \in \{w, n\}\f$ is the fluid phase,
 * \f$\kappa \in \{ w, a \}\f$ are the components,
 * \f$\rho_\alpha\f$ is the fluid density, \f$S_\alpha\f$ is the fluid
 * saturation, \f$\phi\f$ is the porosity of the soil,
 * \f$k_{r\alpha}\f$ is the relative permeability for the fluid,
 * \f$\mu_\alpha\f$ is the fluid's dynamic viscosity, \f$\mathbf{K}\f$ is the
 * intrinsic permeability, \f$p_\alpha\f$ is the fluid pressure and
 * \f$g\f$ is the potential of the gravity field.
 *
 * In contrast to the full two-phase model, the Richards model assumes
 * gas as the non-wetting fluid and that it exhibits a much lower
 * viscosity than the (liquid) wetting phase. (For example at
 * atmospheric pressure and at room temperature, the viscosity of air
 * is only about \f$1\%\f$ of the viscosity of liquid water.) As a
 * consequence, the \f$\frac{k_{r\alpha}}{\mu_\alpha}\f$ term
 * typically is much larger for the gas phase than for the wetting
 * phase. For this reason, the Richards model assumes that
 * \f$\frac{k_{rn}}{\mu_n}\f$ is infinitly large. This implies that
 * the pressure of the gas phase is equivalent to the static pressure
 * distribution and that therefore, mass conservation only needs to be
 * considered for the wetting phase.
 *
 * The model thus choses the absolute pressure of the wetting phase
 * \f$p_w\f$ as its only primary variable. The wetting phase
 * saturation is calculated using the inverse of the capillary
 * pressure, i.e.
 \f[
 S_w = p_c^{-1}(p_n - p_w)
 \f]
 * holds, where \f$p_n\f$ is a given reference pressure. Nota bene,
 * that the last step is assumes that the capillary
 * pressure-saturation curve can be uniquely inverted, so it is not
 * possible to set the capillary pressure to zero when using the
 * Richards model!
 */
template<class TypeTag >
class RichardsModel : public GET_PROP_TYPE(TypeTag, BaseModel)
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, VolumeVariables) VolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables) ElementVolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, SolutionVector) SolutionVector;

    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    enum {
        nPhaseIdx = Indices::nPhaseIdx,
        wPhaseIdx = Indices::wPhaseIdx
    };

    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    enum { dim = GridView::dimension };

    enum { isBox = GET_PROP_VALUE(TypeTag, ImplicitIsBox) };
    enum { dofCodim = isBox ? dim : 0 };

public:
    /*!
     * \brief All relevant primary and secondary of a given
     *        solution to an ouput writer.
     *
     * \param sol The current solution which ought to be written to disk
     * \param writer The Dumux::VtkMultiWriter which is be used to write the data
     */
    template <class MultiWriter>
    void addOutputVtkFields(const SolutionVector &sol, MultiWriter &writer)
    {
        typedef Dune::BlockVector<Dune::FieldVector<double, 1> > ScalarField;
        typedef Dune::BlockVector<Dune::FieldVector<double, dim> > VectorField;

        // get the number of degrees of freedom
        unsigned numDofs = this->numDofs();

        // create the required scalar fields
        ScalarField *pW = writer.allocateManagedBuffer(numDofs);
        ScalarField *pN = writer.allocateManagedBuffer(numDofs);
        ScalarField *pC = writer.allocateManagedBuffer(numDofs);
        ScalarField *Sw = writer.allocateManagedBuffer(numDofs);
        ScalarField *Sn = writer.allocateManagedBuffer(numDofs);
        ScalarField *rhoW = writer.allocateManagedBuffer(numDofs);
        ScalarField *rhoN = writer.allocateManagedBuffer(numDofs);
        ScalarField *mobW = writer.allocateManagedBuffer(numDofs);
        ScalarField *mobN = writer.allocateManagedBuffer(numDofs);
        ScalarField *poro = writer.allocateManagedBuffer(numDofs);
        ScalarField *Te = writer.allocateManagedBuffer(numDofs);
        VectorField *velocity = writer.template allocateManagedBuffer<double, dim>(numDofs);
        ImplicitVelocityOutput<TypeTag> velocityOutput(this->problem_());

        if (velocityOutput.enableOutput())
        {
            // initialize velocity field
            for (unsigned int i = 0; i < numDofs; ++i)
            {
                (*velocity)[i] = Scalar(0);
            }
        }

        unsigned numElements = this->gridView_().size(0);
        ScalarField *rank = writer.allocateManagedBuffer (numElements);

        ElementIterator elemIt = this->gridView_().template begin<0>();
        ElementIterator elemEndIt = this->gridView_().template end<0>();
        for (; elemIt != elemEndIt; ++elemIt)
        {
            int idx = this->problem_().model().elementMapper().map(*elemIt);
            (*rank)[idx] = this->gridView_().comm().rank();

            FVElementGeometry fvGeometry;
            fvGeometry.update(this->gridView_(), *elemIt);

            ElementVolumeVariables elemVolVars;
            elemVolVars.update(this->problem_(),
                               *elemIt,
                               fvGeometry,
                               false /* oldSol? */);

            for (int scvIdx = 0; scvIdx < fvGeometry.numScv; ++scvIdx)
            {
                int globalIdx = this->dofMapper().map(*elemIt, scvIdx, dofCodim);

                (*pW)[globalIdx] = elemVolVars[scvIdx].pressure(wPhaseIdx);
                (*pN)[globalIdx] = elemVolVars[scvIdx].pressure(nPhaseIdx);
                (*pC)[globalIdx] = elemVolVars[scvIdx].capillaryPressure();
                (*Sw)[globalIdx] = elemVolVars[scvIdx].saturation(wPhaseIdx);
                (*Sn)[globalIdx] = elemVolVars[scvIdx].saturation(nPhaseIdx);
                (*rhoW)[globalIdx] = elemVolVars[scvIdx].density(wPhaseIdx);
                (*rhoN)[globalIdx] = elemVolVars[scvIdx].density(nPhaseIdx);
                (*mobW)[globalIdx] = elemVolVars[scvIdx].mobility(wPhaseIdx);
                (*mobN)[globalIdx] = elemVolVars[scvIdx].mobility(nPhaseIdx);
                (*poro)[globalIdx] = elemVolVars[scvIdx].porosity();
                (*Te)[globalIdx] = elemVolVars[scvIdx].temperature();
            }

            // velocity output
            velocityOutput.calculateVelocity(*velocity, elemVolVars, fvGeometry, *elemIt, /*phaseIdx=*/0);
        }

        writer.attachDofData(*Sn, "Sn", isBox);
        writer.attachDofData(*Sw, "Sw", isBox);
        writer.attachDofData(*pN, "pn", isBox);
        writer.attachDofData(*pW, "pw", isBox);
        writer.attachDofData(*pC, "pc", isBox);
        writer.attachDofData(*rhoW, "rhoW", isBox);
        writer.attachDofData(*rhoN, "rhoN", isBox);
        writer.attachDofData(*mobW, "mobW", isBox);
        writer.attachDofData(*mobN, "mobN", isBox);
        writer.attachDofData(*poro, "porosity", isBox);
        writer.attachDofData(*Te, "temperature", isBox);
        if (velocityOutput.enableOutput())
        {
            writer.attachDofData(*velocity,  "velocity", isBox, dim);
        }
        writer.attachCellData(*rank, "process rank");
    }
};
}

#include "richardspropertydefaults.hh"

#endif
