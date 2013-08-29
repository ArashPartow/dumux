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
 * \brief Base class for all models which use the one-phase two-component linear elasticity model.
 *        Adaption of the fully implicit scheme to the one-phase two-component linear elasticity model.
 */
#ifndef DUMUX_ELASTIC1P2C_MODEL_HH
#define DUMUX_ELASTIC1P2C_MODEL_HH

#include "el1p2cproperties.hh"
#include <dumux/common/eigenvalues.hh>

namespace Dumux {
/*!
 * \ingroup ElOnePTwoCBoxModel
 * \brief Adaption of the fully implicit scheme to the one-phase two-component linear elasticity model.
 *
 * This model implements a one-phase flow of an incompressible fluid, that consists of two components.
 * The deformation of the solid matrix is described with a quasi-stationary momentum balance equation.
 * The influence of the pore fluid is accounted for through the effective stress concept (Biot 1941).
 * The total stress acting on a rock is partially supported by the rock matrix and partially supported
 * by the pore fluid. The effective stress represents the share of the total stress which is supported
 * by the solid rock matrix and can be determined as a function of the strain according to Hooke's law.
 *
 * As an equation for the conservation of momentum within the fluid phase Darcy's approach is used:
 \f[
 v = - \frac{\textbf K}{\mu}
 \left(\textbf{grad}\, p - \varrho_w {\textbf g} \right)
 \f]
 *
 * Gravity can be enabled or disabled via the property system.
 * By inserting this into the volume balance of the solid-fluid mixture, one gets
 \f[
 \frac{\partial \text{div} \textbf{u}}{\partial t} - \text{div} \left\{
   \frac{\textbf K}{\mu}  \left(\textbf{grad}\, p - \varrho_w {\textbf g} \right)\right\} = q \;,
 \f]
 *
 * The transport of the components \f$\kappa \in \{ w, a \}\f$ is described by the following equation:
 \f[
 \frac{ \partial \phi_{eff} X^\kappa}{\partial t}
 - \text{div} \left\lbrace
 X^\kappa \frac{{\textbf K}}{\mu} \left( \textbf{grad}\, p - \varrho_w {\textbf g} \right)
 + D^\kappa_\text{pm} \frac{M^\kappa}{M_\alpha} \textbf{grad} x^\kappa
 - \phi_{eff} X^\kappa \frac{\partial \boldsymbol{u}}{\partial t}
 \right\rbrace = q.
 \f]
 *
 * If the model encounters stability problems, a stabilization term can be switched on. The stabilization
 * term is defined in Aguilar et al (2008):
 \f[
 \beta \text{div} \textbf{grad} \frac{\partial p}{\partial t}
 \f]
 with \f$\beta\f$:
 \f[
 \beta = h^2 / 4(\lambda + 2 \mu)
 \f]
 * where \f$h\f$ is the discretization length.
 *
 * The balance equations
 * with the stabilization term are given below:
 \f[
 \frac{\partial \text{div} \textbf{u}}{\partial t} - \text{div} \left\{
   \frac{\textbf K}{\mu}  \left(\textbf{grad}\, p - \varrho_w {\textbf g} \right)
   + \varrho_w \beta \textbf{grad} \frac{\partial p}{\partial t}
   \right\} = q \;,
 \f]
 *
 * The transport of the components \f$\kappa \in \{ w, a \}\f$ is described by the following equation:
 *
 \f[
 \frac{ \partial \phi_{eff} X^\kappa}{\partial t}
 - \text{div} \left\lbrace
 X^\kappa \frac{{\textbf K}}{\mu} \left( \textbf{grad}\, p - \varrho_w {\textbf g} \right)
 + \varrho_w X^\kappa \beta \textbf{grad} \frac{\partial p}{\partial t}
 + D^\kappa_\text{pm} \frac{M^\kappa}{M_\alpha} \textbf{grad} x^\kappa
 - \phi_{eff} X^\kappa \frac{\partial \boldsymbol{u}}{\partial t}
 \right\rbrace = q.
 \f]
 *
 *
 * The quasi-stationary momentum balance equation is:
 \f[
 \text{div}\left( \boldsymbol{\sigma'}- p \boldsymbol{I} \right) + \left( \phi_{eff} \varrho_w + (1 - \phi_{eff}) * \varrho_s  \right)
  {\textbf g} = 0 \;,
 \f]
 * with the effective stress:
 \f[
  \boldsymbol{\sigma'} = 2\,G\,\boldsymbol{\epsilon} + \lambda \,\text{tr} (\boldsymbol{\epsilon}) \, \boldsymbol{I}.
 \f]
 *
 * and the strain tensor \f$\boldsymbol{\epsilon}\f$ as a function of the solid displacement gradient \f$\textbf{grad} \boldsymbol{u}\f$:
 \f[
  \boldsymbol{\epsilon} = \frac{1}{2} \, (\textbf{grad} \boldsymbol{u} + \textbf{grad}^T \boldsymbol{u}).
 \f]
 *
 * Here, the rock mechanics sign convention is switch off which means compressive stresses are < 0 and tensile stresses are > 0.
 * The rock mechanics sign convention can be switched on for the vtk output via the property system.
 *
 * The effective porosity is calculated as a function of the solid displacement:
 \f[
      \phi_{eff} = \frac{\phi_{init} + \text{div} \boldsymbol{u}}{1 + \text{div}}
 \f]
 * All equations are discretized using a vertex-centered finite volume (box)
 * or cell-centered finite volume scheme as spatial
 * and the implicit Euler method as time discretization.
 *
 * The primary variables are the pressure \f$p\f$ and the mole or mass fraction of dissolved component \f$x\f$ and the solid
 * displacement vector \f$\boldsymbol{u}\f$.
 */


template<class TypeTag>
class ElOnePTwoCModel: public GET_PROP_TYPE(TypeTag, BaseModel)
{
    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, FluxVariables) FluxVariables;
    typedef typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables) ElementVolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, ElementBoundaryTypes) ElementBoundaryTypes;
    typedef typename GET_PROP_TYPE(TypeTag, SolutionVector) SolutionVector;

    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;

    enum {
        dim = GridView::dimension,
        dimWorld = GridView::dimensionworld
    };

    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;


    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef Dune::FieldVector<Scalar, dim> DimVector;
    typedef Dune::FieldMatrix<Scalar, dim, dim> DimMatrix;

public:
    /*!
     * \brief \copybrief ImplicitModel::addOutputVtkFields
     *
     * Specialization for the ElOnePTwoCBoxModel, add one-phase two-component
     * properties, solid displacement, stresses, effective properties and the
     * process rank to the VTK writer.
     */
    template<class MultiWriter>
    void addOutputVtkFields(const SolutionVector &sol, MultiWriter &writer) {

        // check whether compressive stresses are defined to be positive
        // (rockMechanicsSignConvention_ == true) or negative
        rockMechanicsSignConvention_ =  GET_PARAM_FROM_GROUP(TypeTag, bool, Vtk, RockMechanicsSignConvention);

        typedef Dune::BlockVector<Dune::FieldVector<Scalar, 1> > ScalarField;
        typedef Dune::BlockVector<Dune::FieldVector<Scalar, dim> > VectorField;

        // create the required scalar and vector fields
        unsigned numScv = this->gridView_().size(dim);
        unsigned numElements = this->gridView_().size(0);

        // create the required fields for vertex data
        ScalarField &pressure = *writer.allocateManagedBuffer(numScv);
        ScalarField &moleFraction0 = *writer.allocateManagedBuffer(numScv);
        ScalarField &moleFraction1 = *writer.allocateManagedBuffer(numScv);
        ScalarField &massFraction0 = *writer.allocateManagedBuffer(numScv);
        ScalarField &massFraction1 = *writer.allocateManagedBuffer(numScv);
        VectorField &displacement = *writer.template allocateManagedBuffer<Scalar, dim>(numScv);
        ScalarField &density = *writer.allocateManagedBuffer(numScv);
        ScalarField &viscosity = *writer.allocateManagedBuffer(numScv);
        ScalarField &porosity = *writer.allocateManagedBuffer(numScv);
        ScalarField &Kx = *writer.allocateManagedBuffer(numScv);

        // create the required fields for element data
        // effective stresses
        VectorField &effStressX = *writer.template allocateManagedBuffer<Scalar,
                dim>(numElements);
        VectorField &effStressY = *writer.template allocateManagedBuffer<Scalar,
                dim>(numElements);
        VectorField &effStressZ = *writer.template allocateManagedBuffer<Scalar,
                dim>(numElements);
        // total stresses
        VectorField &totalStressX = *writer.template allocateManagedBuffer<
                Scalar, dim>(numElements);
        VectorField &totalStressY = *writer.template allocateManagedBuffer<
                Scalar, dim>(numElements);
        VectorField &totalStressZ = *writer.template allocateManagedBuffer<
                Scalar, dim>(numElements);

        // principal stresses
        ScalarField &principalStress1 = *writer.allocateManagedBuffer(
                numElements);
        ScalarField &principalStress2 = *writer.allocateManagedBuffer(
                numElements);
        ScalarField &principalStress3 = *writer.allocateManagedBuffer(
                numElements);

        ScalarField &effPorosity = *writer.allocateManagedBuffer(numElements);
        ScalarField &cellPorosity = *writer.allocateManagedBuffer(numElements);
        ScalarField &cellKx = *writer.allocateManagedBuffer(numElements);
        ScalarField &cellPressure = *writer.allocateManagedBuffer(numElements);

        // initialize cell stresses, cell-wise hydraulic parameters and cell pressure with zero
        for (unsigned int elemIdx = 0; elemIdx < numElements; ++elemIdx) {
            effStressX[elemIdx] = Scalar(0.0);
            if (dim >= 2)
                effStressY[elemIdx] = Scalar(0.0);
            if (dim >= 3)
                effStressZ[elemIdx] = Scalar(0.0);

            totalStressX[elemIdx] = Scalar(0.0);
            if (dim >= 2)
                totalStressY[elemIdx] = Scalar(0.0);
            if (dim >= 3)
                totalStressZ[elemIdx] = Scalar(0.0);

            principalStress1[elemIdx] = Scalar(0.0);
            if (dim >= 2)
                principalStress2[elemIdx] = Scalar(0.0);
            if (dim >= 3)
                principalStress3[elemIdx] = Scalar(0.0);

            effPorosity[elemIdx] = Scalar(0.0);
            cellPorosity[elemIdx] = Scalar(0.0);
            cellKx[elemIdx] = Scalar(0.0);
            cellPressure[elemIdx] = Scalar(0.0);
        }
        ScalarField &rank = *writer.allocateManagedBuffer(numElements);


        FVElementGeometry fvGeometry;
        ElementVolumeVariables elemVolVars;
        ElementBoundaryTypes elemBcTypes;

        // initialize start and end of element iterator
        ElementIterator elemIt = this->gridView_().template begin<0>();
        ElementIterator endit = this->gridView_().template end<0>();
        // loop over all elements (cells)
        for (; elemIt != endit; ++elemIt)
        {
            unsigned int elemIdx = this->problem_().model().elementMapper().map(*elemIt);
            rank[elemIdx] = this->gridView_().comm().rank();

            fvGeometry.update(this->gridView_(), *elemIt);
            elemBcTypes.update(this->problem_(), *elemIt, fvGeometry);
            elemVolVars.update(this->problem_(), *elemIt, fvGeometry, false);

            // loop over all local vertices of the cell
            int numScv = elemIt->template count<dim>();

            for (int scvIdx = 0; scvIdx < numScv; ++scvIdx)
            {
                unsigned int globalIdx = this->dofMapper().map(*elemIt, scvIdx, dim);

                pressure[globalIdx] = elemVolVars[scvIdx].pressure();
                moleFraction0[globalIdx] = elemVolVars[scvIdx].moleFraction(0);
                moleFraction1[globalIdx] = elemVolVars[scvIdx].moleFraction(1);
                massFraction0[globalIdx] = elemVolVars[scvIdx].massFraction(0);
                massFraction1[globalIdx] = elemVolVars[scvIdx].massFraction(1);
                // in case of rock mechanics sign convention solid displacement is
                // defined to be negative if it points in positive coordinate direction
                if(rockMechanicsSignConvention_){
                    DimVector tmpDispl;
                    tmpDispl = Scalar(0);
                    tmpDispl -= elemVolVars[scvIdx].displacement();
                    displacement[globalIdx] = tmpDispl;
                    }

                else
                    displacement[globalIdx] = elemVolVars[scvIdx].displacement();

                density[globalIdx] = elemVolVars[scvIdx].density();
                viscosity[globalIdx] = elemVolVars[scvIdx].viscosity();
                porosity[globalIdx] = elemVolVars[scvIdx].porosity();
                Kx[globalIdx] =    this->problem_().spatialParams().intrinsicPermeability(
                                *elemIt, fvGeometry, scvIdx)[0][0];
                // calculate cell quantities by adding up scv quantities and dividing through numScv
                cellPorosity[elemIdx] += elemVolVars[scvIdx].porosity()    / numScv;
                cellKx[elemIdx] += this->problem_().spatialParams().intrinsicPermeability(
                                *elemIt, fvGeometry, scvIdx)[0][0] / numScv;
                cellPressure[elemIdx] += elemVolVars[scvIdx].pressure()    / numScv;
            };

            // calculate cell quantities for variables which are defined at the integration point
            Scalar tmpEffPoro;
            DimMatrix tmpEffStress;
            tmpEffStress = Scalar(0);
            tmpEffPoro = Scalar(0);

            // loop over all scv-faces of the cell
            for (int faceIdx = 0; faceIdx < fvGeometry.numScvf; faceIdx++) {

                //prepare the flux calculations (set up and prepare geometry, FE gradients)
                FluxVariables fluxVars(this->problem_(),
                                    *elemIt, fvGeometry,
                                    faceIdx,
                                    elemVolVars);

                // divide by number of scv-faces and sum up edge values
                tmpEffPoro = fluxVars.effPorosity() / fvGeometry.numScvf;
                tmpEffStress = fluxVars.sigma();
                tmpEffStress /= fvGeometry.numScvf;

                effPorosity[elemIdx] += tmpEffPoro;

                // in case of rock mechanics sign convention compressive stresses
                // are defined to be positive
                if(rockMechanicsSignConvention_){
                    effStressX[elemIdx] -= tmpEffStress[0];
                    if (dim >= 2) {
                        effStressY[elemIdx] -= tmpEffStress[1];
                    }
                    if (dim >= 3) {
                        effStressZ[elemIdx] -= tmpEffStress[2];
                    }
                }
                else{
                    effStressX[elemIdx] += tmpEffStress[0];
                    if (dim >= 2) {
                        effStressY[elemIdx] += tmpEffStress[1];
                    }
                    if (dim >= 3) {
                        effStressZ[elemIdx] += tmpEffStress[2];
                    }
                }
            }

            // calculate total stresses
            // in case of rock mechanics sign convention compressive stresses
            // are defined to be positive and total stress is calculated by adding the pore pressure
            if(rockMechanicsSignConvention_){
                totalStressX[elemIdx][0] = effStressX[elemIdx][0]    + cellPressure[elemIdx];
                totalStressX[elemIdx][1] = effStressX[elemIdx][1];
                totalStressX[elemIdx][2] = effStressX[elemIdx][2];
                if (dim >= 2) {
                    totalStressY[elemIdx][0] = effStressY[elemIdx][0];
                    totalStressY[elemIdx][1] = effStressY[elemIdx][1]    + cellPressure[elemIdx];
                    totalStressY[elemIdx][2] = effStressY[elemIdx][2];
                }
                if (dim >= 3) {
                    totalStressZ[elemIdx][0] = effStressZ[elemIdx][0];
                    totalStressZ[elemIdx][1] = effStressZ[elemIdx][1];
                    totalStressZ[elemIdx][2] = effStressZ[elemIdx][2]    + cellPressure[elemIdx];
                }
            }
            else{
                totalStressX[elemIdx][0] = effStressX[elemIdx][0]    - cellPressure[elemIdx];
                totalStressX[elemIdx][1] = effStressX[elemIdx][1];
                totalStressX[elemIdx][2] = effStressX[elemIdx][2];
                if (dim >= 2) {
                    totalStressY[elemIdx][0] = effStressY[elemIdx][0];
                    totalStressY[elemIdx][1] = effStressY[elemIdx][1]    - cellPressure[elemIdx];
                    totalStressY[elemIdx][2] = effStressY[elemIdx][2];
                }
                if (dim >= 3) {
                    totalStressZ[elemIdx][0] = effStressZ[elemIdx][0];
                    totalStressZ[elemIdx][1] = effStressZ[elemIdx][1];
                    totalStressZ[elemIdx][2] = effStressZ[elemIdx][2]    - cellPressure[elemIdx];
                }
            }
        }

        // calculate principal stresses i.e. the eigenvalues of the total stress tensor
        Scalar a1, a2, a3;
        DimMatrix totalStress;
        DimVector eigenValues;
        a1=Scalar(0);
        a2=Scalar(0);
        a3=Scalar(0);

        for (unsigned int elemIdx = 0; elemIdx < numElements; elemIdx++)
        {
            eigenValues = Scalar(0);
            totalStress = Scalar(0);

            totalStress[0] = totalStressX[elemIdx];
            if (dim >= 2)
                totalStress[1] = totalStressY[elemIdx];
            if (dim >= 3)
                totalStress[2] = totalStressZ[elemIdx];

            calculateEigenValues<dim>(eigenValues, totalStress);


            for (int i = 0; i < dim; i++)
                {
                    if (isnan(eigenValues[i]))
                        eigenValues[i] = 0.0;
                }

            // sort principal stresses: principalStress1 >= principalStress2 >= principalStress3
            if (dim == 2) {
                a1 = eigenValues[0];
                a2 = eigenValues[1];

                if (a1 >= a2) {
                    principalStress1[elemIdx] = a1;
                    principalStress2[elemIdx] = a2;
                } else {
                    principalStress1[elemIdx] = a2;
                    principalStress2[elemIdx] = a1;
                }
            }

            if (dim == 3) {
                a1 = eigenValues[0];
                a2 = eigenValues[1];
                a3 = eigenValues[2];

                if (a1 >= a2) {
                    if (a1 >= a3) {
                        principalStress1[elemIdx] = a1;
                        if (a2 >= a3) {
                            principalStress2[elemIdx] = a2;
                            principalStress3[elemIdx] = a3;
                        }
                        else //a3 > a2
                        {
                            principalStress2[elemIdx] = a3;
                            principalStress3[elemIdx] = a2;
                        }
                    }
                    else // a3 > a1
                    {
                        principalStress1[elemIdx] = a3;
                        principalStress2[elemIdx] = a1;
                        principalStress3[elemIdx] = a2;
                    }
                } else // a2>a1
                {
                    if (a2 >= a3) {
                        principalStress1[elemIdx] = a2;
                        if (a1 >= a3) {
                            principalStress2[elemIdx] = a1;
                            principalStress3[elemIdx] = a3;
                        }
                        else //a3>a1
                        {
                            principalStress2[elemIdx] = a3;
                            principalStress3[elemIdx] = a1;
                        }
                    }
                    else //a3>a2
                    {
                        principalStress1[elemIdx] = a3;
                        principalStress2[elemIdx] = a2;
                        principalStress3[elemIdx] = a1;
                    }
                }
            }

        }

        writer.attachVertexData(pressure, "P");

        char nameMoleFraction0[42], nameMoleFraction1[42];
        snprintf(nameMoleFraction0, 42, "x_%s", FluidSystem::componentName(0));
        snprintf(nameMoleFraction1, 42, "x_%s", FluidSystem::componentName(1));
        writer.attachVertexData(moleFraction0, nameMoleFraction0);
        writer.attachVertexData(moleFraction1, nameMoleFraction1);

        char nameMassFraction0[42], nameMassFraction1[42];
        snprintf(nameMassFraction0, 42, "X_%s", FluidSystem::componentName(0));
        snprintf(nameMassFraction1, 42, "X_%s", FluidSystem::componentName(1));
        writer.attachVertexData(massFraction0, nameMassFraction0);
        writer.attachVertexData(massFraction1, nameMassFraction1);

        writer.attachVertexData(displacement, "u", dim);
        writer.attachVertexData(density, "rho");
        writer.attachVertexData(viscosity, "mu");
        writer.attachVertexData(porosity, "porosity");
        writer.attachVertexData(Kx, "Kx");
        writer.attachCellData(cellPorosity, "porosity");
        writer.attachCellData(cellKx, "Kx");
        writer.attachCellData(effPorosity, "effective porosity");

        writer.attachCellData(totalStressX, "total stresses X", dim);
        if (dim >= 2)
            writer.attachCellData(totalStressY, "total stresses Y", dim);
        if (dim >= 3)
            writer.attachCellData(totalStressZ, "total stresses Z", dim);

        writer.attachCellData(effStressX, "effective stress changes X", dim);
        if (dim >= 2)
            writer.attachCellData(effStressY, "effective stress changes Y",    dim);
        if (dim >= 3)
            writer.attachCellData(effStressZ, "effective stress changes Z",    dim);

        writer.attachCellData(principalStress1, "principal stress 1");
        if (dim >= 2)
            writer.attachCellData(principalStress2, "principal stress 2");
        if (dim >= 3)
            writer.attachCellData(principalStress3, "principal stress 3");

        writer.attachCellData(cellPressure, "P");

        writer.attachCellData(rank, "rank");

    }
private:
    bool rockMechanicsSignConvention_;

};
}
#include "el1p2cpropertydefaults.hh"
#endif
