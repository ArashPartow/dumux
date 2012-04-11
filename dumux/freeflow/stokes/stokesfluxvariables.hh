// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2010-2011 by Katherina Baber, Klaus Mosthaf               *
 *   Copyright (C) 2008-2009 by Bernd Flemisch, Andreas Lauser               *
 *   Institute for Modelling Hydraulic and Environmental Systems             *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@iws.uni-stuttgart.de                          *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \file
 *
 * \brief This file contains the data which is required to calculate
 *        the fluxes of the Stokes model over a face of a finite volume.
 *
 * This means pressure gradients, phase densities at the integration point, etc.
 */
#ifndef DUMUX_STOKES_FLUX_VARIABLES_HH
#define DUMUX_STOKES_FLUX_VARIABLES_HH

#include <dumux/common/math.hh>
#include <dumux/common/valgrind.hh>

#include "stokesproperties.hh"

namespace Dumux
{

/*!
 * \ingroup BoxStokesModel
 * \ingroup BoxFluxVariables
 * \brief This template class contains the data which is required to
 *        calculate the mass and momentum fluxes over the face of a
 *        sub-control volume for the Stokes box model.
 *
 * This means pressure gradients, phase densities, viscosities, etc.
 * at the integration point of the sub-control-volume face
 */
template <class TypeTag>
class StokesFluxVariables
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;

    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables) ElementVolumeVariables;

    enum { dim = GridView::dimension };

    typedef typename GridView::template Codim<0>::Entity Element;
    typedef Dune::FieldVector<Scalar, dim> FieldVector;
    typedef Dune::FieldVector<Scalar, dim> VelocityVector;
    typedef Dune::FieldVector<Scalar, dim> ScalarGradient;
    typedef Dune::FieldMatrix<Scalar, dim, dim> VectorGradient;

    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename FVElementGeometry::SubControlVolumeFace SCVFace;

public:
    StokesFluxVariables(const Problem &problem,
                        const Element &element,
                        const FVElementGeometry &elemGeom,
                        int faceIdx,
                        const ElementVolumeVariables &elemVolVars,
                        bool onBoundary = false)
        : fvGeom_(elemGeom), onBoundary_(onBoundary), faceIdx_(faceIdx)
    {
        calculateValues_(problem, element, elemVolVars);
        determineUpwindDirection_(elemVolVars);
    };

protected:
    void calculateValues_(const Problem &problem,
                          const Element &element,
                          const ElementVolumeVariables &elemVolVars)
    {
        // calculate gradients and secondary variables at IPs
        FieldVector tmp(0.0);

        densityAtIP_ = Scalar(0);
        molarDensityAtIP_ = Scalar(0);
        viscosityAtIP_ = Scalar(0);
        pressureAtIP_ = Scalar(0);
        normalVelocityAtIP_ = Scalar(0);
        velocityAtIP_ = Scalar(0);
        pressureGradAtIP_ = Scalar(0);
        velocityGradAtIP_ = Scalar(0);
//        velocityDivAtIP_ = Scalar(0);

        for (int idx = 0;
             idx < fvGeom_.numVertices;
             idx++) // loop over adjacent vertices
        {
            // phase density and viscosity at IP
            densityAtIP_ += elemVolVars[idx].density() *
                face().shapeValue[idx];
            molarDensityAtIP_ += elemVolVars[idx].molarDensity()*
                face().shapeValue[idx];
            viscosityAtIP_ += elemVolVars[idx].viscosity() *
                face().shapeValue[idx];
            pressureAtIP_ += elemVolVars[idx].pressure() *
                face().shapeValue[idx];

            // velocity at the IP (fluxes)
            VelocityVector velocityTimesShapeValue = elemVolVars[idx].velocity();
            velocityTimesShapeValue *= face().shapeValue[idx];
            velocityAtIP_ += velocityTimesShapeValue;

            // the pressure gradient
            tmp = face().grad[idx];
            tmp *= elemVolVars[idx].pressure();
            pressureGradAtIP_ += tmp;
            // take gravity into account
            tmp = problem.gravity();
            tmp *= densityAtIP_;
            // pressure gradient including influence of gravity
            pressureGradAtIP_ -= tmp;

            // the velocity gradients and divergence
            for (int dimIdx = 0; dimIdx<dim; ++dimIdx)
            {
                tmp = face().grad[idx];
                tmp *= elemVolVars[idx].velocity()[dimIdx];
                velocityGradAtIP_[dimIdx] += tmp;

//                velocityDivAtIP_ += face().grad[idx][dimIdx]*elemVolVars[idx].velocity()[dimIdx];
            }
        }

        normalVelocityAtIP_ = velocityAtIP_ * face().normal;

        Valgrind::CheckDefined(densityAtIP_);
        Valgrind::CheckDefined(viscosityAtIP_);
        Valgrind::CheckDefined(normalVelocityAtIP_);
        Valgrind::CheckDefined(velocityAtIP_);
        Valgrind::CheckDefined(pressureGradAtIP_);
        Valgrind::CheckDefined(velocityGradAtIP_);
//        Valgrind::CheckDefined(velocityDivAtIP_);
    };

    void determineUpwindDirection_(const ElementVolumeVariables &elemVolVars)
    {

        // set the upstream and downstream vertices
        upstreamIdx_ = face().i;
        downstreamIdx_ = face().j;

        if (normalVelocityAtIP() < 0)
            std::swap(upstreamIdx_, downstreamIdx_);
    };

public:
    /*!
     * \brief The face of the current sub-control volume. This may be either
     *        an inner sub-control-volume face or a face on the boundary.
     */
    const SCVFace &face() const
    {
        if (onBoundary_)
            return fvGeom_.boundaryFace[faceIdx_];
        else
            return fvGeom_.subContVolFace[faceIdx_];
    }

    /*!
     * \brief Return the average volume of the upstream and the downstream sub-control volume;
     *        this is required for the stabilization.
     */
    const Scalar averageSCVVolume() const
    {
        return 0.5*(fvGeom_.subContVol[upstreamIdx_].volume +
                fvGeom_.subContVol[downstreamIdx_].volume);
    }

    /*!
     * \brief Return the pressure \f$\mathrm{[Pa]}\f$ at the integration
     *        point.
     */
    Scalar pressureAtIP() const
    { return pressureAtIP_; }

    /*!
     * \brief Return the mass density \f$ \mathrm{[kg/m^3]} \f$ at the integration
     *        point.
     */
    Scalar densityAtIP() const
    { return densityAtIP_; }

    /*!
     * \brief Return the molar density \f$ \mathrm{[mol/m^3]} \f$ at the integration point.
     */
    const Scalar molarDensityAtIP() const
    { return molarDensityAtIP_; }

    /*!
     * \brief Return the viscosity \f$ \mathrm{[m^2/s]} \f$ at the integration
     *        point.
     */
    Scalar viscosityAtIP() const
    { return viscosityAtIP_; }

    /*!
     * \brief Return the velocity \f$ \mathrm{[m/s]} \f$ at the integration
     *        point multiplied by the normal and the area.
     */
    Scalar normalVelocityAtIP() const
    { return normalVelocityAtIP_; }

    /*!
     * \brief Return the pressure gradient at the integration point.
     */
    const ScalarGradient &pressureGradAtIP() const
    { return pressureGradAtIP_; }

    /*!
     * \brief Return the velocity vector at the integration point.
     */
    const VelocityVector &velocityAtIP() const
    { return velocityAtIP_; }

    /*!
     * \brief Return the velocity gradient at the integration
     *        point of a face.
     */
    const VectorGradient &velocityGradAtIP() const
    { return velocityGradAtIP_; }

//    /*!
//     * \brief Return the divergence of the normal velocity at the
//     *        integration point.
//     */
//    Scalar velocityDivAtIP() const
//    { return velocityDivAtIP_; }

    /*!
     * \brief Return the local index of the upstream sub-control volume.
     */
    int upstreamIdx() const
    { return upstreamIdx_; }

    /*!
     * \brief Return the local index of the downstream sub-control volume.
     */
    int downstreamIdx() const
    { return downstreamIdx_; }

    /*!
     * \brief Indicates if a face is on a boundary. Used for in the
     *        face() method (e.g. for outflow boundary conditions).
     */
    bool onBoundary() const
    { return onBoundary_; }

protected:
    const FVElementGeometry &fvGeom_;
    const bool onBoundary_;

    // values at the integration point
    Scalar densityAtIP_;
    Scalar molarDensityAtIP_;
    Scalar viscosityAtIP_;
    Scalar pressureAtIP_;
    Scalar normalVelocityAtIP_;
//    Scalar velocityDivAtIP_;
    VelocityVector velocityAtIP_;

    // gradients at the IPs
    ScalarGradient pressureGradAtIP_;
    VectorGradient velocityGradAtIP_;

    // local index of the upwind vertex
    int upstreamIdx_;
    // local index of the downwind vertex
    int downstreamIdx_;
    // the index of the considered face
    int faceIdx_;
};

} // end namepace

#endif
