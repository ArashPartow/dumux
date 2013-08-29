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
 * \brief This file contains the calculation of all the fluxes over the surface of the
 * finite volume that make up the volume, the mass and the momentum balance
 * for the two-phase linear-elastic model.
 *
 * This means pressure, concentration and solid-displacement gradients, phase densities at
 * the integration point, etc.
 *
 * This class inherits from the two-phase model FluxVariables
 */
#ifndef DUMUX_EL2P_FLUX_VARIABLES_HH
#define DUMUX_EL2P_FLUX_VARIABLES_HH

#include <dune/pdelab/gridfunctionspace/localfunctionspace.hh>
#include <dumux/implicit/common/implicitdarcyfluxvariables.hh>
#include "el2pproperties.hh"

namespace Dumux
{

namespace Properties
{
// forward declaration of properties
NEW_PROP_TAG(SpatialParams);
}
/*!
 * \ingroup ElTwoPBoxModel
 * \ingroup ImplicitFluxVariables
 * \brief This template class contains the data which is required to
 *        calculate the fluxes over the surface of the
 *           finite volume that make up the volume, the mass and the momentum balance
 *           for the two-phase linear-elastic model.
 *
 * This means pressure, concentration and solid-displacement gradients, phase densities at
 * the integration point, etc.
 *
 */
    template<class TypeTag>
    class ElTwoPFluxVariables: public ImplicitDarcyFluxVariables<TypeTag>
    {
            typedef ImplicitDarcyFluxVariables<TypeTag> TwoPBase;

            typedef typename GET_PROP_TYPE(TypeTag, PTAG(Problem)) Problem;
            typedef typename GET_PROP_TYPE(TypeTag, SpatialParams) SpatialParams;
            typedef typename GET_PROP_TYPE(TypeTag, PTAG(ElementVolumeVariables)) ElementVolumeVariables;

            typedef typename GET_PROP_TYPE(TypeTag, PTAG(GridView)) GridView;
            typedef typename GridView::template Codim<0>::Entity Element;
                        enum
            {
                dim = GridView::dimension,
                dimWorld = GridView::dimensionworld
            };

            typedef typename GridView::ctype CoordScalar;
            typedef typename GET_PROP_TYPE(TypeTag, PTAG(Scalar)) Scalar;
            typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;
            typedef Dune::FieldVector<CoordScalar, dim> DimVector;
            typedef Dune::FieldMatrix<Scalar, dimWorld, dimWorld> DimMatrix;

            typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
            typedef typename FVElementGeometry::SubControlVolumeFace SCVFace;

            enum
            {
                numFluidPhases = GET_PROP_VALUE(TypeTag, PTAG(NumPhases)),
                numEq = GET_PROP_VALUE(TypeTag, PTAG(NumEq))
            };

        public:
        /*
         * \brief The constructor
         *
         * \param problem The problem
         * \param element The finite element
         * \param fvGeometry The finite-volume geometry in the fully implicit scheme
         * \param faceIdx The local index of the SCV (sub-control-volume) face
         * \param elemVolVars The volume variables of the current element
         * \param onBoundary A boolean variable to specify whether the flux variables
         * are calculated for interior SCV faces or boundary faces, default=false
         */
            ElTwoPFluxVariables(const Problem &problem,
                                const Element &element,
                                const FVElementGeometry &fvGeometry,
                                int faceIdx,
                                const ElementVolumeVariables &elemVolVars,
                                const bool onBoundary = false)
            : TwoPBase(problem, element, fvGeometry, faceIdx, elemVolVars),
              fvGeometry_(fvGeometry), faceIdx_(faceIdx)
            {
                dU_ = Scalar(0);
                timeDerivUNormal_ = Scalar(0);

                elTwoPGradients_(problem, element, elemVolVars);
                calculateDDt_(problem, element, elemVolVars);
                calculateK_(problem, element, elemVolVars);
            }
            ;

        public:
            /*!
             * \brief Return change of u [m] with time at integration point
             *        point.
             */
            Scalar dU(int dimIdx) const
            {
                return dU_[dimIdx];
            }

            /*!
             * \brief Return time derivative of u [m/s] in normal
             * direction at integration point
             */
            Scalar timeDerivUNormal() const
            {
                return timeDerivUNormal_;
            }

            /*
             * \brief Return the intrinsic permeability.
             */
            const DimMatrix &intrinsicPermeability() const
            {
                return K_;
            }

            /*
             * \brief Return the gradient of the potential for each phase.
             */
            DimVector potentialGrad(int phaseIdx) const
            {
                return this->gradPotential_[phaseIdx];
            }

            const SCVFace &face() const
            {
                return fvGeometry_.subContVolFace[faceIdx_];
            }

        protected:
            /*!
             * \brief Calculation of the solid displacement gradients.
             *
             *        \param problem The considered problem file
             *        \param element The considered element of the grid
             *        \param elemVolVars The parameters stored in the considered element
             */
            void elTwoPGradients_(const Problem &problem,
                            const Element &element,
                            const ElementVolumeVariables &elemVolVars)
            {
                typedef typename GET_PROP_TYPE(TypeTag, PTAG(GridFunctionSpace)) GridFunctionSpace;
                typedef Dune::PDELab::LocalFunctionSpace<GridFunctionSpace> LocalFunctionSpace;
                LocalFunctionSpace localFunctionSpace(problem.model().jacobianAssembler().gridFunctionSpace());
                localFunctionSpace.bind(element);
                // copy values of previous solution into prevSolutionValues Vector
                std::vector<Scalar> prevSolutionValues;
                localFunctionSpace.vread(problem.model().prevSol(), prevSolutionValues);
                // copy values of current solution into curSolutionValues Vector
                std::vector<Scalar> curSolutionValues;
                localFunctionSpace.vread(problem.model().curSol(), curSolutionValues);
                // type of function space for solid displacement vector
                typedef typename LocalFunctionSpace::template Child<1>::Type DisplacementLFS;
                const DisplacementLFS& displacementLFS = localFunctionSpace.template getChild<1>();
                // number of degrees of freedom for each displacement value (here number of element vertices)
                const unsigned int dispSize = displacementLFS.child(0).size();
                // type of function space of solid displacement value (one for each coordinate direction)
                typedef typename DisplacementLFS::template Child<0>::Type ScalarDispLFS;
                typedef typename ScalarDispLFS::Traits::FiniteElementType::Traits::LocalBasisType::Traits::RangeType RT_V;

                for(int coordDir = 0; coordDir < dim; ++coordDir) {
                    // get displacement function space for coordinate direction coordDir
                    const ScalarDispLFS & scalarDispLFS = displacementLFS.child(coordDir);
                    std::vector<RT_V> vShape(dispSize);
                    // evaluate shape functions of all element vertices for current integration point and write it into vector vShape
                    scalarDispLFS.finiteElement().localBasis().evaluateFunction(face().ipLocal, vShape);

                    dU_[coordDir] = 0;
                    // subtract previous displacement value from current displacement value for each coordinate direction
                    // coordDir and for each node i and interpolate values at integration point via the shape function vShape.
                    // TODO: evaluation of prevVolVars should also be possible--> check
                    for (size_t i = 0; i < dispSize; i++){
                        dU_[coordDir] += (elemVolVars[i].primaryVars()[(numEq - dim)+coordDir] - prevSolutionValues[scalarDispLFS.localIndex(i)])*vShape[i];
                    }
                }
            }

            /*!
             * \brief Calculation of the time derivative of solid displacement
             *        \param problem The considered problem file
             *        \param element The considered element of the grid
             *        \param elemVolVars The parameters stored in the considered element
             */
            void calculateDDt_(const Problem &problem,
                            const Element &element,
                            const ElementVolumeVariables &elemVolVars)
            {
                Scalar dt = problem.timeManager().timeStepSize();

                DimVector tmp(0.0);
                // calculate time derivative of solid displacement vector
                for (int coordDir = 0; coordDir < dim; ++coordDir)
                        tmp[coordDir] = dU(coordDir) / dt;

                // multiply time derivative of solid displacement vector with
                // normal vector of current scv-face
                timeDerivUNormal_ = tmp * face().normal;
            }

            /*!
             * \brief Calculation the harmonic mean of the intrinsic permeability tensor
             *        \param problem The considered problem file
             *        \param element The considered element of the grid
             *        \param elemVolVars The parameters stored in the considered element
             */
            void calculateK_(const Problem &problem,
                             const Element &element,
                             const ElementVolumeVariables &elemVolVars)
            {
                // calculate the mean intrinsic permeability
                const SpatialParams &spatialParams = problem.spatialParams();
                spatialParams.meanK(K_,
                                    spatialParams.intrinsicPermeability(element,
                                                                        fvGeometry_,
                                                                        face().i),
                                    spatialParams.intrinsicPermeability(element,
                                                                        fvGeometry_,
                                                                        face().j));
            }

            const FVElementGeometry &fvGeometry_;
            int faceIdx_;

            //! time derivative of solid displacement times normal vector at integration point
            Scalar timeDerivUNormal_;
            //! change of solid displacement with time at integration point
            GlobalPosition dU_;
            // intrinsic permeability
            DimMatrix K_;
    };

} // end namepace

#endif
