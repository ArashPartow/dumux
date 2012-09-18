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
* \brief Adaption of the box scheme to the Richards model.
*/
#ifndef DUMUX_RICHARDS_MODEL_HH
#define DUMUX_RICHARDS_MODEL_HH

#include <dumux/boxmodels/common/boxmodel.hh>

#include "richardslocalresidual.hh"
#include "richardsproblem.hh"

namespace Dumux
{
/*!
 * \ingroup RichardsModel
 *
 * \brief This model which implements a variant of the Richards
 *        equation for quasi-twophase flow.
 *
 * In the unsaturated zone, Richards' equation is frequently used to
 * approximate the water distribution above the groundwater level. It
 * can be derived from the two-phase equations, i.e.
 \f[
 \frac{\partial\;\phi S_\alpha \rho_\alpha}{\partial t}
 -
 \text{div} \left\{
 \rho_\alpha \frac{k_{r\alpha}}{\mu_\alpha}\; \mathbf{K}
 \textbf{grad}\left[
 p_\alpha - g\rho_\alpha
 \right]
 \right\}
 =
 q_\alpha,
 \f]
 * where \f$\alpha \in \{w, n\}\f$ is the fluid phase,
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
    typedef typename GET_PROP_TYPE(TypeTag, SolutionVector) SolutionVector;

    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    enum {
        nPhaseIdx = Indices::nPhaseIdx,
        wPhaseIdx = Indices::wPhaseIdx
    };

    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    enum { dim = GridView::dimension };

public:
    /*!
     * \brief Returns the relative weight of a primary variable for
     *        calculating relative errors.
     *
     * \param vertIdx The global index of the vertex in question
     * \param pvIdx The index of the primary variable
     */
    Scalar primaryVarWeight(const int vertIdx, const int pvIdx) const
    {
        if (Indices::pwIdx == pvIdx)
            return 1e-6;
        return 1;
    }

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

        // create the required scalar fields
        unsigned numVertices = this->problem_().gridView().size(dim);
        ScalarField *pW = writer.allocateManagedBuffer(numVertices);
        ScalarField *pN = writer.allocateManagedBuffer(numVertices);
        ScalarField *pC = writer.allocateManagedBuffer(numVertices);
        ScalarField *Sw = writer.allocateManagedBuffer(numVertices);
        ScalarField *Sn = writer.allocateManagedBuffer(numVertices);
        ScalarField *rhoW = writer.allocateManagedBuffer(numVertices);
        ScalarField *rhoN = writer.allocateManagedBuffer(numVertices);
        ScalarField *mobW = writer.allocateManagedBuffer(numVertices);
        ScalarField *mobN = writer.allocateManagedBuffer(numVertices);
        ScalarField *poro = writer.allocateManagedBuffer(numVertices);
        ScalarField *Te = writer.allocateManagedBuffer(numVertices);

        unsigned numElements = this->gridView_().size(0);
        ScalarField *rank =
                writer.allocateManagedBuffer (numElements);

        FVElementGeometry fvElemGeom;
        VolumeVariables volVars;

        ElementIterator elemIt = this->gridView_().template begin<0>();
        ElementIterator elemEndIt = this->gridView_().template end<0>();
        for (; elemIt != elemEndIt; ++elemIt)
        {
            int idx = this->problem_().model().elementMapper().map(*elemIt);
            (*rank)[idx] = this->gridView_().comm().rank();

            fvElemGeom.update(this->gridView_(), *elemIt);

            int numVerts = elemIt->template count<dim> ();
            for (int i = 0; i < numVerts; ++i)
            {
                int globalIdx = this->vertexMapper().map(*elemIt, i, dim);
                volVars.update(sol[globalIdx],
                               this->problem_(),
                               *elemIt,
                               fvElemGeom,
                               i,
                               false);

                (*pW)[globalIdx] = volVars.pressure(wPhaseIdx);
                (*pN)[globalIdx] = volVars.pressure(nPhaseIdx);
                (*pC)[globalIdx] = volVars.capillaryPressure();
                (*Sw)[globalIdx] = volVars.saturation(wPhaseIdx);
                (*Sn)[globalIdx] = volVars.saturation(nPhaseIdx);
                (*rhoW)[globalIdx] = volVars.density(wPhaseIdx);
                (*rhoN)[globalIdx] = volVars.density(nPhaseIdx);
                (*mobW)[globalIdx] = volVars.mobility(wPhaseIdx);
                (*mobN)[globalIdx] = volVars.mobility(nPhaseIdx);
                (*poro)[globalIdx] = volVars.porosity();
                (*Te)[globalIdx] = volVars.temperature();
            }
        }

        writer.attachVertexData(*Sn, "Sn");
        writer.attachVertexData(*Sw, "Sw");
        writer.attachVertexData(*pN, "pn");
        writer.attachVertexData(*pW, "pw");
        writer.attachVertexData(*pC, "pc");
        writer.attachVertexData(*rhoW, "rhoW");
        writer.attachVertexData(*rhoN, "rhoN");
        writer.attachVertexData(*mobW, "mobW");
        writer.attachVertexData(*mobN, "mobN");
        writer.attachVertexData(*poro, "porosity");
        writer.attachVertexData(*Te, "temperature");
        writer.attachCellData(*rank, "process rank");
    }
};
}

#include "richardspropertydefaults.hh"

#endif
