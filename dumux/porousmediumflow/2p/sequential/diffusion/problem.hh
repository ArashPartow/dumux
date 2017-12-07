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
 * \brief Base class for stationary solution of a two-phase diffusion/pressure equation
 */
#ifndef DUMUX_DIFFUSIONPROBLEM_2P_HH
#define DUMUX_DIFFUSIONPROBLEM_2P_HH

#include <dumux/porousmediumflow/sequential/onemodelproblem.hh>
#include "properties.hh"

namespace Dumux
{
/*!
 * \ingroup IMPETproblems
 * \ingroup Pressure2p
 * \brief  Base class for stationary solution of a two-phase diffusion/pressure equation
 *
 * \tparam TypeTag The problem TypeTag
 */
template<class TypeTag>
class DiffusionProblem2P: public OneModelProblem<TypeTag>
{
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Implementation;
    typedef OneModelProblem<TypeTag> ParentType;

    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GridView::Grid Grid;typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;

    typedef typename GET_PROP_TYPE(TypeTag, TimeManager) TimeManager;
    typedef typename GET_PROP_TYPE(TypeTag, PressureModel) PressureModel;

    // material properties
    typedef typename GET_PROP_TYPE(TypeTag, SpatialParams) SpatialParams;

    typedef typename GridView::Traits::template Codim<0>::Entity Element;

    enum
    {
        dim = Grid::dimension, dimWorld = Grid::dimensionworld
    };

    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;

    // private!! copy constructor
    DiffusionProblem2P(const DiffusionProblem2P&)
    {}

public:
    /*!
     * \brief Constructs a DiffusionProblem2P object
     *
     * \param timeManager the time manager
     * \param gridView The grid view
     */
    DiffusionProblem2P(TimeManager &timeManager, const GridView &gridView)
    : ParentType(timeManager, gridView), gravity_(0)
    {
        spatialParams_ = std::make_shared<SpatialParams>(gridView);
        gravity_ = 0;
        if (getParam<bool>("Problem.EnableGravity"))
            gravity_[dim - 1] = -9.81;

        pressModel_ = std::make_shared<PressureModel>(asImp_());
    }
    /*!
     * \brief Constructs a DiffusionProblem2P object
     *
     * \param timeManager the time manager
     * \param gridView The grid view
     * \param spatialParams SpatialParams instantiation
     */
    DiffusionProblem2P(TimeManager &timeManager, const GridView &gridView, SpatialParams &spatialParams)
    : ParentType(timeManager, gridView), gravity_(0)
    {
        spatialParams_ = Dune::stackobject_to_shared_ptr<SpatialParams>(spatialParams);
        gravity_ = 0;
        if (getParam<bool>("Problem.EnableGravity"))
            gravity_[dim - 1] = -9.81;

        pressModel_ = std::make_shared<PressureModel>(asImp_());
    }

    /*!
     * \brief Constructs a DiffusionProblem2P object
     *
     * \param gridView The grid view
     */
    DiffusionProblem2P(const GridView &gridView)
    : ParentType(gridView, false), gravity_(0)
    {
        spatialParams_ = std::make_shared<SpatialParams>(gridView);
        gravity_ = 0;
        if (getParam<bool>("Problem.EnableGravity"))
            gravity_[dim - 1] = -9.81;

        pressModel_ = std::make_shared<PressureModel>(asImp_());
    }
    /*!
     * \brief Constructs a DiffusionProblem2P object
     *
     * \param gridView The grid view
     * \param spatialParams SpatialParams instantiation
     */
    DiffusionProblem2P(const GridView &gridView, SpatialParams &spatialParams)
    : ParentType(gridView, false), gravity_(0)
    {
        spatialParams_ = Dune::stackobject_to_shared_ptr<SpatialParams>(spatialParams);
        gravity_ = 0;
        if (getParam<bool>("Problem.EnableGravity"))
            gravity_[dim - 1] = -9.81;

        pressModel_ = std::make_shared<PressureModel>(asImp_());
    }

    /*!
     * \name Problem parameters
     */
    // \{

    /* \brief Time integration function called by the time manager
     *
     * For stationary diffusion problems this function just finishes the simulation.
     */
    void timeIntegration()
    {
        //end simulation -> no time dependent problem!
        this->timeManager().setFinished();

        return;
    }

    /*!
     * \brief Returns the temperature within the domain.
     *
     * \param element The element
     *
     */
    Scalar temperature(const Element& element) const
    {
        return this->asImp_().temperatureAtPos(element.geometry().center());
    }

    /*!
     * \brief Returns the temperature within the domain.
     *
     * \param globalPos The position of the center of an element
     *
     */
    Scalar temperatureAtPos(const GlobalPosition& globalPos) const
    {
        // Throw an exception (there is no initial condition)
        DUNE_THROW(Dune::InvalidStateException,
                   "The problem does not provide "
                   "a temperatureAtPos() method.");
    }

    /*!
     * \brief Returns the reference pressure for evaluation of constitutive relations.
     *
     * \param element The element
     *
     */
    Scalar referencePressure(const Element& element) const
    {
        return this->asImp_().referencePressureAtPos(element.geometry().center());
    }

    /*!
     * \brief Returns the reference pressure for evaluation of constitutive relations.
     *
     * \param globalPos The position of the center of an element
     *
     */
    Scalar referencePressureAtPos(const GlobalPosition& globalPos) const
    {
        // Throw an exception (there is no initial condition)
        DUNE_THROW(Dune::InvalidStateException,
                   "The problem does not provide "
                   "a referencePressureAtPos() method.");
    }

    /*!
     * \brief Returns the acceleration due to gravity.
     *
     * If the <tt>EnableGravity</tt> property is true, this means
     * \f$\boldsymbol{g} = ( 0,\dots,\ -9.81)^T \f$, else \f$\boldsymbol{g} = ( 0,\dots, 0)^T \f$
     */
    const GlobalPosition &gravity() const
    {
        return gravity_;
    }

    /*!
     * \brief Returns the spatial parameters object.
     */
    SpatialParams &spatialParams()
    {
        return *spatialParams_;
    }

    /*!
     * \brief Returns the spatial parameters object.
     */
    const SpatialParams &spatialParams() const
    {
        return *spatialParams_;
    }

    /*!
     * \brief Returns the pressure model used for the problem.
     */
    PressureModel &pressureModel()
    { return *pressModel_; }

    //! \copydoc IMPETProblem::pressureModel()
    const PressureModel &pressureModel() const
    { return *pressModel_; }

    // \}

private:
    //! Returns the implementation of the problem (i.e. static polymorphism)
    Implementation &asImp_()
    { return *static_cast<Implementation *>(this); }

    //! \copydoc IMPETProblem::asImp_()
    const Implementation &asImp_() const
    { return *static_cast<const Implementation *>(this); }

    GlobalPosition gravity_;

    // fluids and material properties
    std::shared_ptr<SpatialParams> spatialParams_;
    bool newSpatialParams_;
    std::shared_ptr<PressureModel> pressModel_;
};

}

#endif
