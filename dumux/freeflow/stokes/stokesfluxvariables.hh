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
    typedef Dune::FieldVector<Scalar, dim> DimVector;
    typedef Dune::FieldMatrix<Scalar, dim, dim> DimMatrix;

    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename FVElementGeometry::SubControlVolumeFace SCVFace;

public:
    StokesFluxVariables(const Problem &problem,
                        const Element &element,
                        const FVElementGeometry &fvGeometry,
                        const int faceIdx,
                        const ElementVolumeVariables &elemVolVars,
                        const bool onBoundary = false)
        : fvGeometry_(fvGeometry), onBoundary_(onBoundary), faceIdx_(faceIdx)
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
        DimVector tmp(0.0);

        density_ = Scalar(0);
        molarDensity_ = Scalar(0);
        viscosity_ = Scalar(0);
        pressure_ = Scalar(0);
        normalvelocity_ = Scalar(0);
        velocity_ = Scalar(0);
        pressureGrad_ = Scalar(0);
        velocityGrad_ = Scalar(0);
//        velocityDiv_ = Scalar(0);

        for (int idx = 0;
             idx < fvGeometry_.numVertices;
             idx++) // loop over adjacent vertices
        {
            // phase density and viscosity at IP
            density_ += elemVolVars[idx].density() *
                face().shapeValue[idx];
            molarDensity_ += elemVolVars[idx].molarDensity()*
                face().shapeValue[idx];
            viscosity_ += elemVolVars[idx].viscosity() *
                face().shapeValue[idx];
            pressure_ += elemVolVars[idx].pressure() *
                face().shapeValue[idx];

            // velocity at the IP (fluxes)
            DimVector velocityTimesShapeValue = elemVolVars[idx].velocity();
            velocityTimesShapeValue *= face().shapeValue[idx];
            velocity_ += velocityTimesShapeValue;

            // the pressure gradient
            tmp = face().grad[idx];
            tmp *= elemVolVars[idx].pressure();
            pressureGrad_ += tmp;
            // take gravity into account
            tmp = problem.gravity();
            tmp *= density_;
            // pressure gradient including influence of gravity
            pressureGrad_ -= tmp;

            // the velocity gradients and divergence
            for (int dimIdx = 0; dimIdx<dim; ++dimIdx)
            {
                tmp = face().grad[idx];
                tmp *= elemVolVars[idx].velocity()[dimIdx];
                velocityGrad_[dimIdx] += tmp;

//                velocityDiv_ += face().grad[idx][dimIdx]*elemVolVars[idx].velocity()[dimIdx];
            }
        }

        normalvelocity_ = velocity_ * face().normal;

        Valgrind::CheckDefined(density_);
        Valgrind::CheckDefined(viscosity_);
        Valgrind::CheckDefined(normalvelocity_);
        Valgrind::CheckDefined(velocity_);
        Valgrind::CheckDefined(pressureGrad_);
        Valgrind::CheckDefined(velocityGrad_);
//        Valgrind::CheckDefined(velocityDiv_);
    };

    void determineUpwindDirection_(const ElementVolumeVariables &elemVolVars)
    {

        // set the upstream and downstream vertices
        upstreamIdx_ = face().i;
        downstreamIdx_ = face().j;

        if (normalVelocity() < 0)
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
            return fvGeometry_.boundaryFace[faceIdx_];
        else
            return fvGeometry_.subContVolFace[faceIdx_];
    }

    /*!
     * \brief Return the average volume of the upstream and the downstream sub-control volume;
     *        this is required for the stabilization.
     */
    const Scalar averageSCVVolume() const
    {
        return 0.5*(fvGeometry_.subContVol[upstreamIdx_].volume +
                fvGeometry_.subContVol[downstreamIdx_].volume);
    }

    /*!
     * \brief Return the pressure \f$\mathrm{[Pa]}\f$ at the integration
     *        point.
     */
    Scalar pressure() const
    { return pressure_; }

    /*!
     * \brief Return the pressure \f$\mathrm{[Pa]}\f$ at the integration
     *        point.
     */
    DUMUX_DEPRECATED_MSG("use pressure() instead")
    Scalar pressureAtIP() const
    { return pressure(); }

    /*!
     * \brief Return the mass density \f$ \mathrm{[kg/m^3]} \f$ at the integration
     *        point.
     */
    Scalar density() const
    { return density_; }

    /*!
     * \brief Return the mass density \f$ \mathrm{[kg/m^3]} \f$ at the integration
     *        point.
     */
    DUMUX_DEPRECATED_MSG("use density() instead")
    Scalar densityAtIP() const
    { return density(); }

    /*!
     * \brief Return the molar density \f$ \mathrm{[mol/m^3]} \f$ at the integration point.
     */
    const Scalar molarDensity() const
    { return molarDensity_; }

    /*!
     * \brief Return the molar density \f$ \mathrm{[mol/m^3]} \f$ at the integration point.
     */
    DUMUX_DEPRECATED_MSG("use molarDensity() instead")
    const Scalar molarDensityAtIP() const
    { return molarDensity(); }

    /*!
     * \brief Return the dynamic viscosity \f$ \mathrm{[Pa s]} \f$ at the integration
     *        point.
     */
    Scalar viscosity() const
    { return viscosity_; }

    /*!
     * \brief Return the dynamic viscosity \f$ \mathrm{[Pa s]} \f$ at the integration
     *        point.
     */
    DUMUX_DEPRECATED_MSG("use viscosity() instead")
    Scalar viscosityAtIP() const
    { return viscosity(); }

    /*!
     * \brief Return the velocity \f$ \mathrm{[m/s]} \f$ at the integration
     *        point multiplied by the normal and the area.
     */
    Scalar normalVelocity() const
    { return normalvelocity_; }

    /*!
     * \brief Return the velocity \f$ \mathrm{[m/s]} \f$ at the integration
     *        point multiplied by the normal and the area.
     */
    DUMUX_DEPRECATED_MSG("use normalVelocity() instead")
    Scalar normalVelocityAtIP() const
    { return normalVelocity(); }

    /*!
     * \brief Return the pressure gradient at the integration point.
     */
    const DimVector &pressureGrad() const
    { return pressureGrad_; }

    /*!
     * \brief Return the pressure gradient at the integration point.
     */
    DUMUX_DEPRECATED_MSG("use pressureGrad() instead")
    const DimVector &pressureGradAtIP() const
    { return pressureGrad(); }

    /*!
     * \brief Return the velocity vector at the integration point.
     */
    const DimVector &velocity() const
    { return velocity_; }

    /*!
     * \brief Return the velocity vector at the integration point.
     */
    DUMUX_DEPRECATED_MSG("use velocity() instead")
    const DimVector &velocityAtIP() const
    { return velocity(); }

    /*!
     * \brief Return the velocity gradient at the integration
     *        point of a face.
     */
    const DimMatrix &velocityGrad() const
    { return velocityGrad_; }

    /*!
     * \brief Return the velocity gradient at the integration
     *        point of a face.
     */
    DUMUX_DEPRECATED_MSG("use velocityGrad() instead")
    const DimMatrix &velocityGradAtIP() const
    { return velocityGrad(); }

//    /*!
//     * \brief Return the divergence of the normal velocity at the
//     *        integration point.
//     */
//    Scalar velocityDiv() const
//    { return velocityDiv_; }

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
    const FVElementGeometry &fvGeometry_;
    const bool onBoundary_;

    // values at the integration point
    Scalar density_;
    Scalar molarDensity_;
    Scalar viscosity_;
    Scalar pressure_;
    Scalar normalvelocity_;
//    Scalar velocityDiv_;
    DimVector velocity_;

    // gradients at the IPs
    DimVector pressureGrad_;
    DimMatrix velocityGrad_;

    // local index of the upwind vertex
    int upstreamIdx_;
    // local index of the downwind vertex
    int downstreamIdx_;
    // the index of the considered face
    int faceIdx_;
};

} // end namepace

#endif
