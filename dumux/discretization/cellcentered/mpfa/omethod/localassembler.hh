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
 * \ingroup CCMpfaDiscretization
 * \brief Class for the assembly of the local systems of equations
 *        involved in the transmissibility computaion in the mpfa-o scheme.
 */
#ifndef DUMUX_DISCRETIZATION_CC_MPFA_O_LOCAL_ASSEMBLER_HH
#define DUMUX_DISCRETIZATION_CC_MPFA_O_LOCAL_ASSEMBLER_HH

#include <dumux/common/math.hh>
#include <dumux/common/properties.hh>

#include <dumux/discretization/cellcentered/mpfa/methods.hh>
#include <dumux/discretization/cellcentered/mpfa/localassembler.hh>
#include <dumux/discretization/cellcentered/mpfa/computetransmissibility.hh>
#include <dumux/discretization/cellcentered/mpfa/interactionvolume.hh>

namespace Dumux
{

/*!
 * \ingroup CCMpfaDiscretization
 * \brief Specialization of the interaction volume-local
 *        assembler class for the mpfa-o scheme.
 */
template< class TypeTag >
class InteractionVolumeAssemblerImpl< CCMpfaInteractionVolumeImplementation<TypeTag, MpfaMethods::oMethod> >
      : public InteractionVolumeAssemblerBase< CCMpfaInteractionVolumeImplementation<TypeTag, MpfaMethods::oMethod> >
{
    using InteractionVolume = CCMpfaInteractionVolumeImplementation<TypeTag, MpfaMethods::oMethod>;
    using ParentType = InteractionVolumeAssemblerBase< InteractionVolume >;

    using Traits = typename InteractionVolume::Traits;
    using LocalIndexType = typename Traits::LocalIndexType;
    using Matrix = typename Traits::Matrix;
    using Vector = typename Traits::Vector;

    static constexpr int dim = Traits::LocalScvType::myDimension;
    static constexpr int dimWorld = Traits::LocalScvType::worldDimension;

    // obtain the number of phases via the property system
    static constexpr int numPhases = GET_PROP_VALUE(TypeTag, NumPhases);

public:
    //! Use the constructor of the base class
    using ParentType::ParentType;

    /*!
     * \brief Assembles the transmissibility matrix
     *        within an interaction volume for the mpfa-o scheme.
     *
     * \param T The transmissibility matrix to be assembled
     * \param iv The mpfa-o interaction volume
     * \param getTensor Lambda to evaluate the scv-wise tensors
     */
    template< class GetTensorFunction >
    void assemble(Matrix& T, InteractionVolume& iv, const GetTensorFunction& getTensor)
    {
        // assemble D into T directly
        assembleLocalMatrices_(iv.A(), iv.B(), iv.C(), T, iv, getTensor);

        // maybe solve the local system
        if (iv.numUnknowns() > 0)
        {
            // T = C*A^-1*B + D
            iv.A().invert();
            iv.C().rightmultiply(iv.A());
            T += multiplyMatrices(iv.C(), iv.B());
        }
    }

    /*!
     * \brief Assembles the interaction volume-local transmissibility
     *        matrix for surface grids. The transmissibilities associated
     *        with "outside" faces are stored in a separate container.
     *
     * \param outsideTij tij on "outside" faces to be assembled
     * \param T The transmissibility matrix tij to be assembled
     * \param iv The mpfa-o interaction volume
     * \param getTensor Lambda to evaluate the scv-wise tensors
     */
    template< class OutsideTijContainer, class GetTensorFunction >
    void assemble(OutsideTijContainer& outsideTij, Matrix& T, InteractionVolume& iv, const GetTensorFunction& getTensor)
    {
        // assemble D into T directly
        assembleLocalMatrices_(iv.A(), iv.B(), iv.C(), T, iv, getTensor);

        // maybe solve the local system
        if (iv.numUnknowns() > 0)
        {
            // T = C*A^-1*B + D
            iv.A().invert();
            iv.B().leftmultiply(iv.A());
            T += multiplyMatrices(iv.C(), iv.B());

            // compute outside transmissibilities
            for (const auto& localFaceData : iv.localFaceData())
            {
                // continue only for "outside" faces
                if (!localFaceData.isOutside())
                    continue;

                const auto localScvIdx = localFaceData.ivLocalInsideScvIndex();
                const auto localScvfIdx = localFaceData.ivLocalScvfIndex();
                const auto idxInOutside = localFaceData.scvfLocalOutsideScvfIndex();
                const auto& posLocalScv = iv.localScv(localScvIdx);
                const auto& wijk = iv.omegas()[localScvfIdx][idxInOutside+1];

                // store the calculated transmissibilities in the data handle
                auto& tij = outsideTij[localScvfIdx][idxInOutside];
                tij = 0.0;

                // add contributions from all local directions
                for (LocalIndexType localDir = 0; localDir < dim; localDir++)
                {
                    // the scvf corresponding to this local direction in the scv
                    const auto& curLocalScvf = iv.localScvf(posLocalScv.scvfIdxLocal(localDir));

                    // on interior faces the coefficients of the AB matrix come into play
                    if (!curLocalScvf.isDirichlet())
                    {
                        auto tmp = iv.B()[curLocalScvf.localDofIndex()];
                        tmp *= wijk[localDir];
                        tij -= tmp;
                    }
                    else
                        tij[curLocalScvf.localDofIndex()] -= wijk[localDir];

                    // add entry from the scv unknown
                    tij[localScvIdx] += wijk[localDir];
                }
            }
        }
    }

    /*!
     * \brief Assemble the transmissibility matrix within an interaction
     *        volume for the mpfa-o scheme to be used for advective flux
     *        computation in the case that gravity is to be considered in
     *        the local system of equations.
     *
     * \param T The transmissibility matrix to be assembled
     * \param g Container to assemble gravity per scvf & phase
     * \param CA Matrix to store matrix product C*A^-1
     * \param iv The mpfa-o interaction volume
     * \param getTensor Lambda to evaluate the scv-wise tensors
     */
    template< class GravityContainer, class GetTensorFunction >
    void assembleWithGravity(Matrix& T,
                             GravityContainer& g,
                             Matrix& CA,
                             InteractionVolume& iv,
                             const GetTensorFunction& getTensor)
    {
        // assemble D into T & C into CA directly
        assembleLocalMatrices_(iv.A(), iv.B(), CA, T, iv, getTensor);

        // maybe solve the local system
        if (iv.numUnknowns() > 0)
        {
            // T = C*A^-1*B + D
            iv.A().invert();
            CA.rightmultiply(iv.A());
            T += multiplyMatrices(CA, iv.B());
        }

        // assemble gravitational acceleration container (enforce usage of mpfa-o type version)
        assembleGravity(g, iv, CA, getTensor);
    }

    /*!
     * \brief Assembles the interaction volume-local transmissibility
     *        matrix in the case that gravity is to be considered in the
     *        local system of equations. This specialization is to be used
     *        on surface grids, where the gravitational flux contributions
     *        on "outside" faces are stored in a separate container.
     *
     * \param outsideTij tij on "outside" faces to be assembled
     * \param T The transmissibility matrix to be assembled
     * \param outsideG Container to assemble gravity on "outside" faces
     * \param g Container to assemble gravity per scvf & phase
     * \param CA Matrix to store matrix product C*A^-1
     * \param A Matrix to store the inverse A^-1
     * \param iv The mpfa-o interaction volume
     * \param getTensor Lambda to evaluate the scv-wise tensors
     */
    template< class GravityContainer,
              class OutsideGravityContainer,
              class OutsideTijContainer,
              class GetTensorFunction >
    void assembleWithGravity(OutsideTijContainer& outsideTij,
                             Matrix& T,
                             OutsideGravityContainer& outsideG,
                             GravityContainer& g,
                             Matrix& CA,
                             Matrix& A,
                             InteractionVolume& iv,
                             const GetTensorFunction& getTensor)
    {
        // assemble D into T directly
        assembleLocalMatrices_(iv.A(), iv.B(), iv.C(), T, iv, getTensor);

        // maybe solve the local system
        if (iv.numUnknowns() > 0)
        {
            // T = C*A^-1*B + D
            iv.A().invert();
            iv.B().leftmultiply(iv.A());
            T += multiplyMatrices(iv.C(), iv.B());
            A = iv.A();
            CA = iv.C().rightmultiply(A);

            // compute outside transmissibilities
            for (const auto& localFaceData : iv.localFaceData())
            {
                // continue only for "outside" faces
                if (!localFaceData.isOutside())
                    continue;

                const auto localScvIdx = localFaceData.ivLocalInsideScvIndex();
                const auto localScvfIdx = localFaceData.ivLocalScvfIndex();
                const auto idxInOutside = localFaceData.scvfLocalOutsideScvfIndex();
                const auto& posLocalScv = iv.localScv(localScvIdx);
                const auto& wijk = iv.omegas()[localScvfIdx][idxInOutside+1];

                // store the calculated transmissibilities in the data handle
                auto& tij = outsideTij[localScvfIdx][idxInOutside];
                tij = 0.0;

                // add contributions from all local directions
                for (LocalIndexType localDir = 0; localDir < dim; localDir++)
                {
                    // the scvf corresponding to this local direction in the scv
                    const auto& curLocalScvf = iv.localScvf(posLocalScv.scvfIdxLocal(localDir));

                    // on interior faces the coefficients of the AB matrix come into play
                    if (!curLocalScvf.isDirichlet())
                    {
                        auto tmp = iv.B()[curLocalScvf.localDofIndex()];
                        tmp *= wijk[localDir];
                        tij -= tmp;
                    }
                    else
                        tij[curLocalScvf.localDofIndex()] -= wijk[localDir];

                    // add entry from the scv unknown
                    tij[localScvIdx] += wijk[localDir];
                }
            }
        }

        assembleGravity(g, outsideG, iv, CA, A, getTensor);
    }

    /*!
     * \brief Assembles the vector of primary (cell) unknowns and (maybe)
     *        Dirichlet boundary conditions within an interaction volume.
     *
     * \param u The vector to be filled with the cell unknowns
     * \param iv The mpfa-o interaction volume
     * \param getU Lambda to obtain the desired cell/Dirichlet value from grid index
     */
    template< class GetU >
    void assemble(Vector& u, const InteractionVolume& iv, const GetU& getU)
    {
        // resize given container
        u.resize(iv.numKnowns());

        // put the cell pressures first

        for (LocalIndexType i = 0; i < iv.numScvs(); ++i)
            u[i] = getU( iv.localScv(i).globalScvIndex() );

        // Dirichlet BCs come afterwards
        unsigned int i = iv.numScvs();
        for (const auto& data : iv.dirichletData())
            u[i++] = getU( data.volVarIndex() );
    }

    /*!
     * \brief Assemble the gravitational flux contributions on the scvfs within
     *        an mpfa-o interaction volume.
     *
     * \note  For each face, the gravity term in the form of \f$\rho \mathbf{n K g}\f$ is
     *        evaluated. Thus, make sure to only call this with a lambda that returns the
     *        hydraulic conductivity.
     *
     * \param g Container to assemble gravity per scvf & phase
     * \param iv The mpfa-o interaction volume
     * \param CA Projection matrix transforming the gravity terms in the local system of
     *        equations to the entire set of faces within the interaction volume
     * \param getTensor Lambda to evaluate scv-wise hydraulic conductivities
     */
    template< class GravityContainer, class GetTensorFunction >
    void assembleGravity(GravityContainer& g,
                         const InteractionVolume& iv,
                         const Matrix& CA,
                         const GetTensorFunction& getTensor)
    {
        // we require the CA matrix and the g vector to have the correct size already
        assert(g.size() == numPhases && "Provided gravity container does not have NumPhases entries");
        assert(g[0].size() == iv.numFaces() && "Gravitation vector g does not have the correct size");
        assert(CA.rows() == iv.numFaces() && CA.cols() == iv.numUnknowns() && "Matrix CA does not have the correct size");

        //! For each face, we...
        //! - arithmetically average the phase densities
        //! - compute the term \f$ \alpha := A \rho \ \mathbf{n}^T \mathbf{K} \mathbf{g} \f$ in each neighboring cell
        //! - compute \f$ \alpha^* = \alpha_{outside} - \alpha_{inside} \f$
        using Scalar = typename Vector::value_type;

        std::array< Vector, numPhases > sum_alphas;
        for (unsigned int pIdx = 0; pIdx < numPhases; ++pIdx)
        {
            g[pIdx] = 0.0;
            sum_alphas[pIdx].resize(iv.numUnknowns(), 0.0);
        }

        for (LocalIndexType faceIdx = 0; faceIdx < iv.numFaces(); ++faceIdx)
        {
            // gravitational acceleration on this face
            const auto& curLocalScvf = iv.localScvf(faceIdx);
            const auto& curGlobalScvf = this->fvGeometry().scvf(curLocalScvf.globalScvfIndex());
            const auto gravity = this->problem().gravityAtPos(curGlobalScvf.ipGlobal());

            // get permeability tensor in "positive" sub volume
            const auto& neighborScvIndices = curLocalScvf.neighboringLocalScvIndices();
            const auto& posLocalScv = iv.localScv(neighborScvIndices[0]);
            const auto& posGlobalScv = this->fvGeometry().scv(posLocalScv.globalScvIndex());
            const auto& posVolVars = this->elemVolVars()[posGlobalScv];
            const auto& posElement = iv.element(neighborScvIndices[0]);
            const auto tensor = getTensor(this->problem(), posElement, posVolVars, this->fvGeometry(), posGlobalScv);

            // This function should never be called for surface grids,
            // for which there is the specialization of this function below
            assert(neighborScvIndices.size() <= 2 && "Scvf seems to have more than one outside scv!");

            std::array< Scalar, numPhases > rho;
            const auto alpha_inside = posVolVars.extrusionFactor()*vtmv(curGlobalScvf.unitOuterNormal(), tensor, gravity);
            if (!curLocalScvf.isDirichlet())
            {
                for (unsigned int pIdx = 0; pIdx < numPhases; ++pIdx)
                    rho[pIdx] = posVolVars.density(pIdx);

                if (!curGlobalScvf.boundary())
                {
                    // obtain outside tensor
                    const auto& negLocalScv = iv.localScv( neighborScvIndices[1] );
                    const auto& negGlobalScv = this->fvGeometry().scv(negLocalScv.globalScvIndex());
                    const auto& negVolVars = this->elemVolVars()[negGlobalScv];
                    const auto& negElement = iv.element( neighborScvIndices[1] );
                    const auto negTensor = getTensor(this->problem(), negElement, negVolVars, this->fvGeometry(), negGlobalScv);

                    const auto sum_alpha = negVolVars.extrusionFactor()
                                           * vtmv(curGlobalScvf.unitOuterNormal(), negTensor, gravity)
                                           - alpha_inside;

                    const auto localDofIdx = curLocalScvf.localDofIndex();
                    for (unsigned int pIdx = 0; pIdx < numPhases; ++pIdx)
                    {
                        rho[pIdx] = 0.5*( rho[pIdx] + negVolVars.density(pIdx) );
                        sum_alphas[pIdx][localDofIdx] = sum_alpha*rho[pIdx]*curGlobalScvf.area();
                    }
                }
                else
                {
                    const auto localDofIdx = curLocalScvf.localDofIndex();
                    for (unsigned int pIdx = 0; pIdx < numPhases; ++pIdx)
                        sum_alphas[pIdx][localDofIdx] -= alpha_inside*rho[pIdx]*curGlobalScvf.area();
                }
            }
            // use Dirichlet BC densities
            else
            {
                const auto& dirichletVolVars = this->elemVolVars()[curGlobalScvf.outsideScvIdx()];
                for (unsigned int pIdx = 0; pIdx < numPhases; ++pIdx)
                    rho[pIdx] = dirichletVolVars.density(pIdx);
            }

            // add "inside" alpha to gravity container
            for (unsigned int pIdx = 0; pIdx < numPhases; ++pIdx)
                g[pIdx][faceIdx] += alpha_inside*rho[pIdx]*curGlobalScvf.area();
        }

        // g += CA*sum_alphas
        for (unsigned int pIdx = 0; pIdx < numPhases; ++pIdx)
            CA.umv(sum_alphas[pIdx], g[pIdx]);
    }

    /*!
     * \brief Assembles the gravitational flux contributions on the scvfs within an mpfa-o
     *        interaction volume. This specialization is to be used on surface grids, where the
     *        gravitational flux contributions on "outside" faces are stored in a separate container.
     *
     * \note  For each face, the gravity term in the form of \f$\rho \mathbf{n K g}\f$ is
     *        evaluated. Thus, make sure to only call this with a lambda that returns the
     *        hydraulic conductivity.
     *
     * \param g Container to store gravity per scvf & phase
     * \param outsideG Container to store gravity per "outside" scvf & phase
     * \param iv The mpfa-o interaction volume
     * \param CA Projection matrix transforming the gravity terms in the local system of
     *        equations to the entire set of faces within the interaction volume
     * \param A Matrix needed for the "reconstruction" of face unknowns as a function of gravity
     * \param getTensor Lambda to evaluate scv-wise hydraulic conductivities
     */
    template< class GravityContainer,
              class OutsideGravityContainer,
              class GetTensorFunction >
    void assembleGravity(GravityContainer& g,
                         OutsideGravityContainer& outsideG,
                         const InteractionVolume& iv,
                         const Matrix& CA,
                         const Matrix& A,
                         const GetTensorFunction& getTensor)
    {
        // we require the CA matrix and the gravity containers to have the correct size already
        assert(CA.rows() == iv.numFaces() && CA.cols() == iv.numUnknowns() && "Matrix CA does not have the correct size");
        assert(g.size() == numPhases && "Provided gravity container does not have NumPhases entries");
        assert(outsideG.size() == numPhases && "Provided outside gravity container does not have NumPhases entries");
        assert(std::all_of(g.cbegin(), g.cend(), [&iv](const auto& d) { return d.size() == iv.numFaces(); })
               && "Gravitation vector g does not have the correct size");
        assert(std::all_of(outsideG.cbegin(), outsideG.cend(), [&iv](const auto& d) { return d.size() == iv.numFaces(); })
               && "Outside gravity container does not have the correct size");

        //! For each face, we...
        //! - arithmetically average the phase densities
        //! - compute the term \f$ \alpha := \mathbf{A} \rho \ \mathbf{n}^T \mathbf{K} \mathbf{g} \f$ in each neighboring cell
        //! - compute \f$ \alpha^* = \sum{\alpha_{outside, i}} - \alpha_{inside} \f$
        using Scalar = typename Vector::value_type;

        // reset everything to zero
        std::array< Vector, numPhases > sum_alphas;
        for (unsigned int pIdx = 0; pIdx < numPhases; ++pIdx)
        {
            g[pIdx] = 0.0;
            std::for_each(outsideG[pIdx].begin(), outsideG[pIdx].end(), [] (auto& v) { v = 0.0; });
            sum_alphas[pIdx].resize(iv.numUnknowns(), 0.0);
        }

        for (LocalIndexType faceIdx = 0; faceIdx < iv.numFaces(); ++faceIdx)
        {
            // gravitational acceleration on this face
            const auto& curLocalScvf = iv.localScvf(faceIdx);
            const auto& curGlobalScvf = this->fvGeometry().scvf(curLocalScvf.globalScvfIndex());
            const auto gravity = this->problem().gravityAtPos(curGlobalScvf.ipGlobal());

            // get permeability tensor in "positive" sub volume
            const auto& neighborScvIndices = curLocalScvf.neighboringLocalScvIndices();
            const auto& posLocalScv = iv.localScv(neighborScvIndices[0]);
            const auto& posGlobalScv = this->fvGeometry().scv(posLocalScv.globalScvIndex());
            const auto& posVolVars = this->elemVolVars()[posGlobalScv];
            const auto& posElement = iv.element(neighborScvIndices[0]);
            const auto tensor = getTensor(this->problem(), posElement, posVolVars, this->fvGeometry(), posGlobalScv);

            const auto alpha_inside = posVolVars.extrusionFactor()*vtmv(curGlobalScvf.unitOuterNormal(), tensor, gravity);
            const auto numOutsideFaces = curGlobalScvf.boundary() ? 0 : curGlobalScvf.numOutsideScvs();
            std::vector< Scalar > alpha_outside(numOutsideFaces);
            std::array< Scalar, numPhases > rho;

            if (!curLocalScvf.isDirichlet())
            {
                for (unsigned int pIdx = 0; pIdx < numPhases; ++pIdx)
                    rho[pIdx] = posVolVars.density(pIdx);

                // arithmetically average density on inside faces
                const auto localDofIdx = curLocalScvf.localDofIndex();
                if (!curGlobalScvf.boundary())
                {
                    for (unsigned int idxInOutside = 0; idxInOutside < curGlobalScvf.numOutsideScvs(); ++idxInOutside)
                    {
                        // obtain outside tensor
                        const auto& negLocalScv = iv.localScv( neighborScvIndices[idxInOutside] );
                        const auto& negGlobalScv = this->fvGeometry().scv(negLocalScv.globalScvIndex());
                        const auto& negVolVars = this->elemVolVars()[negGlobalScv];
                        const auto& negElement = iv.element( neighborScvIndices[idxInOutside] );
                        const auto negTensor = getTensor(this->problem(), negElement, negVolVars, this->fvGeometry(), negGlobalScv);

                        const auto& flipScvf = this->fvGeometry().flipScvf(curGlobalScvf.index(), idxInOutside);
                        alpha_outside[idxInOutside] = negVolVars.extrusionFactor()
                                                      * vtmv(flipScvf.unitOuterNormal(), negTensor, gravity);

                        for (unsigned int pIdx = 0; pIdx < numPhases; ++pIdx)
                        {
                            rho[pIdx] += negVolVars.density(pIdx);
                            sum_alphas[pIdx][localDofIdx] -= alpha_outside[idxInOutside];
                        }
                    }
                }

                for (unsigned int pIdx = 0; pIdx < numPhases; ++pIdx)
                {
                    rho[pIdx] /= numOutsideFaces + 1;
                    sum_alphas[pIdx][localDofIdx] -= alpha_inside;
                    sum_alphas[pIdx][localDofIdx] *= rho[pIdx]*curGlobalScvf.area();
                }
            }
            // use Dirichlet BC densities
            else
            {
                const auto& dirichletVolVars = this->elemVolVars()[curGlobalScvf.outsideScvIdx()];
                for (unsigned int pIdx = 0; pIdx < numPhases; ++pIdx)
                    rho[pIdx] = dirichletVolVars.density(pIdx);
            }

            // add "inside" & "outside" alphas to gravity containers
            for (unsigned int pIdx = 0; pIdx < numPhases; ++pIdx)
            {
                g[pIdx][faceIdx] += alpha_inside*rho[pIdx]*curGlobalScvf.area();
                unsigned int i = 0;
                for (const auto& alpha : alpha_outside)
                    outsideG[pIdx][faceIdx][i++] -= alpha*rho[pIdx]*curGlobalScvf.area();
            }
        }

        // g += CA*sum_alphas
        // outsideG = wikj*A^-1*sum_alphas + outsideG
        for (unsigned int pIdx = 0; pIdx < numPhases; ++pIdx)
        {
            CA.umv(sum_alphas[pIdx], g[pIdx]);

            Vector AG(iv.numUnknowns());
            A.mv(sum_alphas[pIdx], AG);

            // compute gravitational accelerations
            for (const auto& localFaceData : iv.localFaceData())
            {
                // continue only for "outside" faces
                if (!localFaceData.isOutside())
                    continue;

                const auto localScvIdx = localFaceData.ivLocalInsideScvIndex();
                const auto localScvfIdx = localFaceData.ivLocalScvfIndex();
                const auto idxInOutside = localFaceData.scvfLocalOutsideScvfIndex();
                const auto& posLocalScv = iv.localScv(localScvIdx);
                const auto& wijk = iv.omegas()[localScvfIdx][idxInOutside+1];

                // add contributions from all local directions
                for (LocalIndexType localDir = 0; localDir < dim; localDir++)
                {
                    // the scvf corresponding to this local direction in the scv
                    const auto& curLocalScvf = iv.localScvf(posLocalScv.scvfIdxLocal(localDir));

                    // on interior faces the coefficients of the AB matrix come into play
                    if (!curLocalScvf.isDirichlet())
                        outsideG[pIdx][localScvfIdx][idxInOutside] -= wijk[localDir]*AG[curLocalScvf.localDofIndex()];
                }
            }
        }
    }

private:
    /*!
     * \brief Assemble the matrices involved in the flux expressions
     *        across the scvfs inside an interaction volume as well as those involved
     *        in the interaction volume-local system of equations resulting from flux
     *        and solution continuity across the scvfs.
     *
     *        Flux expressions: \f$\mathbf{f} = \mathbf{C} \bar{\mathbf{u}} + \mathbf{D} \mathbf{u}\f$.
     *
     *        Continuity equations: \f$\mathbf{A} \, \bar{\mathbf{u}} = \mathbf{B} \, \mathbf{u}\f$.
     *
     * \note  The matrices are expected to have been resized beforehand.
     *
     * \param A The A matrix of the iv-local equation system
     * \param B The B matrix of the iv-local equation system
     * \param C The C matrix of the iv-local flux expressions
     * \param D The D matrix of the iv-local flux expressions
     * \param iv The mpfa-o interaction volume
     * \param getTensor Lambda to evaluate the scv-wise tensors
     */
    template< class GetTensorFunction >
    void assembleLocalMatrices_(Matrix& A, Matrix& B,  Matrix& C, Matrix& D,
                                InteractionVolume& iv,
                                const GetTensorFunction& getTensor)
    {
        // Matrix D is assumed to have the right size already
        assert(D.rows() == iv.numFaces() && D.cols() == iv.numKnowns() && "Matrix D does not have the correct size");

        // if only Dirichlet faces are present in the iv,
        // the matrices A, B & C are undefined and D = T
        if (iv.numUnknowns() == 0)
        {
            // reset matrix beforehand
            D = 0.0;

            // Loop over all the faces, in this case these are all dirichlet boundaries
            for (LocalIndexType faceIdx = 0; faceIdx < iv.numFaces(); ++faceIdx)
            {
                const auto& curLocalScvf = iv.localScvf(faceIdx);
                const auto& curGlobalScvf = this->fvGeometry().scvf(curLocalScvf.globalScvfIndex());
                const auto& neighborScvIndices = curLocalScvf.neighboringLocalScvIndices();

                // get tensor in "positive" sub volume
                const auto& posLocalScv = iv.localScv(neighborScvIndices[0]);
                const auto& posGlobalScv = this->fvGeometry().scv(posLocalScv.globalScvIndex());
                const auto& posVolVars = this->elemVolVars()[posGlobalScv];
                const auto& posElement = iv.element(neighborScvIndices[0]);
                const auto tensor = getTensor(this->problem(), posElement, posVolVars, this->fvGeometry(), posGlobalScv);

                // the omega factors of the "positive" sub volume
                const auto wijk = computeMpfaTransmissibility(posLocalScv, curGlobalScvf, tensor, posVolVars.extrusionFactor());

                const auto posScvLocalDofIdx = posLocalScv.localDofIndex();
                for (LocalIndexType localDir = 0; localDir < dim; localDir++)
                {
                    const auto& otherLocalScvf = iv.localScvf( posLocalScv.scvfIdxLocal(localDir) );
                    const auto otherLocalDofIdx = otherLocalScvf.localDofIndex();
                    D[faceIdx][otherLocalDofIdx] -= wijk[localDir];
                    D[faceIdx][posScvLocalDofIdx] += wijk[localDir];
                }
            }
        }
        else
        {
            // we require the matrices A,B,C to have the correct size already
            assert(A.rows() == iv.numUnknowns() && A.cols() == iv.numUnknowns() && "Matrix A does not have the correct size");
            assert(B.rows() == iv.numUnknowns() && B.cols() == iv.numKnowns() && "Matrix B does not have the correct size");
            assert(C.rows() == iv.numFaces() && C.cols() == iv.numKnowns() && "Matrix C does not have the correct size");

            // reset matrices
            A = 0.0;
            B = 0.0;
            C = 0.0;
            D = 0.0;

            auto& wijk = iv.omegas();
            for (LocalIndexType faceIdx = 0; faceIdx < iv.numFaces(); ++faceIdx)
            {
                const auto& curLocalScvf = iv.localScvf(faceIdx);
                const auto& curGlobalScvf = this->fvGeometry().scvf(curLocalScvf.globalScvfIndex());
                const auto curIsDirichlet = curLocalScvf.isDirichlet();
                const auto curLocalDofIdx = curLocalScvf.localDofIndex();

                // get tensor in "positive" sub volume
                const auto& neighborScvIndices = curLocalScvf.neighboringLocalScvIndices();
                const auto& posLocalScv = iv.localScv(neighborScvIndices[0]);
                const auto& posGlobalScv = this->fvGeometry().scv(posLocalScv.globalScvIndex());
                const auto& posVolVars = this->elemVolVars()[posGlobalScv];
                const auto& posElement = iv.element(neighborScvIndices[0]);
                const auto tensor = getTensor(this->problem(), posElement, posVolVars, this->fvGeometry(), posGlobalScv);

                // the omega factors of the "positive" sub volume
                wijk[faceIdx][0] = computeMpfaTransmissibility(posLocalScv, curGlobalScvf, tensor, posVolVars.extrusionFactor());

                // go over the coordinate directions in the positive sub volume
                for (unsigned int localDir = 0; localDir < dim; localDir++)
                {
                    const auto& otherLocalScvf = iv.localScvf( posLocalScv.scvfIdxLocal(localDir) );
                    const auto otherLocalDofIdx = otherLocalScvf.localDofIndex();

                    // if we are not on a Dirichlet face, add entries associated with unknown face pressures
                    // i.e. in matrix C and maybe A (if current face is not a Dirichlet face)
                    if (!otherLocalScvf.isDirichlet())
                    {
                        C[faceIdx][otherLocalDofIdx] -= wijk[faceIdx][0][localDir];
                        if (!curIsDirichlet)
                            A[curLocalDofIdx][otherLocalDofIdx] -= wijk[faceIdx][0][localDir];
                    }
                    // the current face is a Dirichlet face and creates entries in D & maybe B
                    else
                    {
                        D[faceIdx][otherLocalDofIdx] -= wijk[faceIdx][0][localDir];
                        if (!curIsDirichlet)
                            B[curLocalDofIdx][otherLocalDofIdx] += wijk[faceIdx][0][localDir];
                    }

                    // add entries related to pressures at the scv centers (dofs)
                    const auto posScvLocalDofIdx = posLocalScv.localDofIndex();
                    D[faceIdx][posScvLocalDofIdx] += wijk[faceIdx][0][localDir];

                    if (!curIsDirichlet)
                        B[curLocalDofIdx][posScvLocalDofIdx] -= wijk[faceIdx][0][localDir];
                }

                // If we are on an interior face, add values from negative sub volume
                if (!curGlobalScvf.boundary())
                {
                    // loop over all the outside neighbors of this face and add entries
                    for (unsigned int idxInOutside = 0; idxInOutside < curGlobalScvf.numOutsideScvs(); ++idxInOutside)
                    {
                        const auto idxOnScvf = idxInOutside+1;
                        const auto& negLocalScv = iv.localScv( neighborScvIndices[idxOnScvf] );
                        const auto& negGlobalScv = this->fvGeometry().scv(negLocalScv.globalScvIndex());
                        const auto& negVolVars = this->elemVolVars()[negGlobalScv];
                        const auto& negElement = iv.element( neighborScvIndices[idxOnScvf] );
                        const auto negTensor = getTensor(this->problem(), negElement, negVolVars, this->fvGeometry(), negGlobalScv);

                        // On surface grids, use outside face for "negative" transmissibility calculation
                        const auto& scvf = dim < dimWorld ? this->fvGeometry().flipScvf(curGlobalScvf.index(), idxInOutside)
                                                          : curGlobalScvf;
                        wijk[faceIdx][idxOnScvf] = computeMpfaTransmissibility(negLocalScv, scvf, negTensor, negVolVars.extrusionFactor());

                        // flip sign on surface grids (since we used the "outside" normal)
                        if (dim < dimWorld)
                            wijk[faceIdx][idxOnScvf] *= -1.0;

                        // go over the coordinate directions in the positive sub volume
                        for (int localDir = 0; localDir < dim; localDir++)
                        {
                            const auto otherLocalScvfIdx = negLocalScv.scvfIdxLocal(localDir);
                            const auto& otherLocalScvf = iv.localScvf(otherLocalScvfIdx);
                            const auto otherLocalDofIdx = otherLocalScvf.localDofIndex();

                            if (!otherLocalScvf.isDirichlet())
                                A[curLocalDofIdx][otherLocalDofIdx] += wijk[faceIdx][idxOnScvf][localDir];
                            else
                                B[curLocalDofIdx][otherLocalDofIdx] -= wijk[faceIdx][idxOnScvf][localDir];

                            // add entries to matrix B
                            B[curLocalDofIdx][negLocalScv.localDofIndex()] += wijk[faceIdx][idxOnScvf][localDir];
                        }
                    }
                }
            }
        }
    }
};

} // end namespace

#endif