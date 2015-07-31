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
#ifndef DUMUX_STOKES_MODEL_HH
#define DUMUX_STOKES_MODEL_HH

/*!
 * \file
 * \brief Base class for all models which use the Stokes box model.
 */

#include <dune/common/version.hh>

#include <dumux/implicit/common/implicitmodel.hh>

#include "stokeslocalresidual.hh"
#include "stokesnewtoncontroller.hh"
#include "stokeslocaljacobian.hh"
#include "stokesproblem.hh"
#include "stokesproperties.hh"

namespace Dumux
{
/*!
 * \ingroup BoxStokesModel
 * \brief Adaptation of the box scheme to the Stokes model.
 *
 * This model implements laminar Stokes flow of a single fluid, solving the momentum balance equation
 * \f[
 *    \frac{\partial \left(\varrho_g {\boldsymbol{v}}_g\right)}{\partial t}
 *    + \text{div} \left( p_g {\bf {I}}
 *    - \mu_g \left( \textbf{grad}\, \boldsymbol{v}_g
 *                   + \textbf{grad}\, \boldsymbol{v}_g^T \right) \right)
 *    - \varrho_g {\bf g} = 0
 * \f]
 * By setting the property <code>EnableNavierStokes</code> to <code>true</code> the Navier-Stokes
 * equation can be solved. In this case an additional term
 * \f[
 *    + \text{div} \left( \varrho_g \boldsymbol{v}_g \boldsymbol{v}_g \right)
 * \f]
 * is added to the momentum balance equation.
 *
 * The mass balance equation:
 * \f[
 *    \frac{\partial \varrho_g}{\partial t}
 *    + \text{div} \left(\varrho_g {\boldsymbol{v}}_g\right) - q_g = 0
 * \f]
 *
 * This is discretized by a fully-coupled vertex-centered finite volume
 * (box) scheme in space and by the implicit Euler method in time.
 */
template<class TypeTag>
class StokesModel : public GET_PROP_TYPE(TypeTag, BaseModel)
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;

    enum {
        dim = GridView::dimension,
        dimWorld = GridView::dimensionworld
    };
    enum { numEq = GET_PROP_VALUE(TypeTag, NumEq) };

    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;

    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, ElementBoundaryTypes) ElementBoundaryTypes;
    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, SolutionVector) SolutionVector;

    typedef typename GET_PROP_TYPE(TypeTag, VolumeVariables) VolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables) ElementVolumeVariables;

public:
    /*!
     * \brief Calculate the fluxes across a certain layer in the domain.
     * The layer is situated perpendicular to the coordinate axis "coord" and cuts
     * the axis at the value "coordVal".
     *
     * \param globalSol The global solution vector
     * \param flux A vector to store the flux
     * \param axis The dimension, perpendicular to which the layer is situated
     * \param coordVal The (Scalar) coordinate on the axis, at which the layer is situated
     */
    void calculateFluxAcrossLayer(const SolutionVector &globalSol, Dune::FieldVector<Scalar, numEq> &flux, int axis, Scalar coordVal)
    {
        GlobalPosition globalI, globalJ;
        PrimaryVariables tmpFlux(0.0);

        FVElementGeometry fvGeometry;
        ElementVolumeVariables elemVolVars;

        // Loop over elements
        ElementIterator eIt = this->problem_.gridView().template begin<0>();
        ElementIterator eEndIt = this->problem_.gridView().template end<0>();
        for (; eIt != eEndIt; ++eIt)
        {
            if (eIt->partitionType() != Dune::InteriorEntity)
                continue;

            fvGeometry.update(this->gridView_(), *eIt);
            elemVolVars.update(this->problem_(), *eIt, fvGeometry);
            this->localResidual().evalFluxes(*eIt, elemVolVars);

            bool hasLeft = false;
            bool hasRight = false;
            for (int i = 0; i < fvGeometry.numScv; i++) {
                const GlobalPosition &globalPos = fvGeometry.subContVol[i].global;
                if (globalI[axis] < coordVal)
                    hasLeft = true;
                else if (globalI[axis] >= coordVal)
                    hasRight = true;
            }
            if (!hasLeft || !hasRight)
                continue;

            for (int i = 0; i < fvGeometry.numScv; i++) {
                const GlobalPosition &globalPos = fvGeometry.subContVol[i].global;
                if (globalI[axis] < coordVal)
                    flux += this->localResidual().residual(i);
            }
        }

        flux = this->problem_.gridView().comm().sum(flux);
    }

    //! \copydoc ImplicitModel::addOutputVtkFields
    template <class MultiWriter>
    void addOutputVtkFields(const SolutionVector &sol,
                            MultiWriter &writer)
    {
        typedef Dune::BlockVector<Dune::FieldVector<Scalar, 1> > ScalarField;
        typedef Dune::BlockVector<Dune::FieldVector<Scalar, dim> > VelocityField;

        // create the required scalar fields
        unsigned numVertices = this->gridView_().size(dim);
        ScalarField &pn = *writer.allocateManagedBuffer(numVertices);
        ScalarField &delP = *writer.allocateManagedBuffer(numVertices);
        ScalarField &rho = *writer.allocateManagedBuffer(numVertices);
        ScalarField &mu = *writer.allocateManagedBuffer(numVertices);
        VelocityField &velocity = *writer.template allocateManagedBuffer<Scalar, dim> (numVertices);

        unsigned numElements = this->gridView_().size(0);
        ScalarField &rank = *writer.allocateManagedBuffer(numElements);

        FVElementGeometry fvGeometry;
        VolumeVariables volVars;
        ElementBoundaryTypes elemBcTypes;

        ElementIterator eIt = this->gridView_().template begin<0>();
        ElementIterator eEndIt = this->gridView_().template end<0>();
        for (; eIt != eEndIt; ++eIt)
        {
#if DUNE_VERSION_NEWER(DUNE_COMMON, 2, 4)
            int eIdx = this->elementMapper().index(*eIt);
#else
            int eIdx = this->elementMapper().map(*eIt);
#endif
            rank[eIdx] = this->gridView_().comm().rank();

            fvGeometry.update(this->gridView_(), *eIt);
            elemBcTypes.update(this->problem_(), *eIt);

#if DUNE_VERSION_NEWER(DUNE_COMMON, 2, 4)
            int numLocalVerts = eIt->subEntities(dim);
#else
            int numLocalVerts = eIt->template count<dim>();
#endif
            for (int i = 0; i < numLocalVerts; ++i)
            {
#if DUNE_VERSION_NEWER(DUNE_COMMON, 2, 4)
                int vIdxGlobal = this->vertexMapper().subIndex(*eIt, i, dim);
#else
                int vIdxGlobal = this->vertexMapper().map(*eIt, i, dim);
#endif
                volVars.update(sol[vIdxGlobal],
                               this->problem_(),
                               *eIt,
                               fvGeometry,
                               i,
                               false);

                pn[vIdxGlobal] = volVars.pressure();
                delP[vIdxGlobal] = volVars.pressure() - 1e5;
                rho[vIdxGlobal] = volVars.density();
                mu[vIdxGlobal] = volVars.dynamicViscosity();
                velocity[vIdxGlobal] = volVars.velocity();
            }
        }
        writer.attachVertexData(pn, "P");
        writer.attachVertexData(delP, "delP");
        writer.attachVertexData(rho, "rho");
        writer.attachVertexData(mu, "mu");
        writer.attachVertexData(velocity, "v", dim);
    }
};
}

#include "stokespropertydefaults.hh"

#endif
