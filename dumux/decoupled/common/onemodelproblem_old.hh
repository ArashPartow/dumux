// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2010 by Markus Wolff                                      *
 *   Copyright (C) 2009 by Andreas Lauser                                    *
 *   Institute of Hydraulic Engineering                                      *
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

#ifndef DUMUX_ONE_MODEL_PROBLEM_HH
#define DUMUX_ONE_MODEL_PROBLEM_HH

#include <dumux/decoupled/common/decoupledproperties_old.hh>
#include <dumux/io/vtkmultiwriter.hh>
#include <dumux/io/restart.hh>


/**
 * @file
 * @brief  Base class for definition of an decoupled diffusion (pressure) or transport problem
 * @author Markus Wolff
 */

namespace Dumux
{

/*! \ingroup IMPET
 *
 * @brief Base class for definition of an decoupled diffusion (pressure) or transport problem
 *
 * @tparam TypeTag The Type Tag
 */
template<class TypeTag>
class OneModelProblem
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Implementation;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;

    typedef typename GET_PROP_TYPE(TypeTag, TimeManager) TimeManager;

    typedef Dumux::VtkMultiWriter<GridView>  VtkMultiWriter;

    typedef typename GET_PROP_TYPE(TypeTag, Variables) Variables;

    typedef typename GET_PROP_TYPE(TypeTag, Model) Model;

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;

    typedef typename GET_PROP(TypeTag, SolutionTypes) SolutionTypes;
    typedef typename SolutionTypes::VertexMapper VertexMapper;
    typedef typename SolutionTypes::ElementMapper ElementMapper;
    typedef typename SolutionTypes::ScalarSolution Solution;

    enum
    {
        dim = GridView::dimension,
        dimWorld = GridView::dimensionworld
    };
    enum
    {
        wetting = 0, nonwetting = 1
    };

    typedef Dune::FieldVector<Scalar,dimWorld> GlobalPosition;
    typedef typename GridView::template Codim<dim>::Iterator VertexIterator;
    typedef typename GridView::Traits::template Codim<0>::Entity Element;
    typedef typename GridView::Intersection Intersection;

    typedef typename SolutionTypes::PrimaryVariables PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;

    // private!! copy constructor
    OneModelProblem(const OneModelProblem&)
    {}

public:

    //! Constructs an object of type OneModelProblemProblem
    /*!
     *  \tparam TypeTag The TypeTag
     *  \tparam verbose Output level for Dumux::TimeManager
     */
    OneModelProblem(const GridView &gridView, bool verbose = true)
        : gridView_(gridView),
          bboxMin_(std::numeric_limits<double>::max()),
          bboxMax_(-std::numeric_limits<double>::max()),
          deleteTimeManager_(true),
          variables_(gridView),
          outputInterval_(1)
    {
        // calculate the bounding box of the grid view
        VertexIterator vIt = gridView.template begin<dim>();
        const VertexIterator vEndIt = gridView.template end<dim>();
        for (; vIt!=vEndIt; ++vIt) {
            for (int i=0; i<dim; i++) {
                bboxMin_[i] = std::min(bboxMin_[i], vIt->geometry().center()[i]);
                bboxMax_[i] = std::max(bboxMax_[i], vIt->geometry().center()[i]);
            }
        }

        timeManager_ = new TimeManager(verbose);

        model_ = new Model(asImp_()) ;

        resultWriter_ = NULL;
    }

    //! Constructs an object of type OneModelProblemProblem
    /*!
     *  \tparam TypeTag The TypeTag
     *  \tparam verbose Output level for Dumux::TimeManager
     */
    OneModelProblem(TimeManager &timeManager, const GridView &gridView)
        : gridView_(gridView),
          bboxMin_(std::numeric_limits<double>::max()),
          bboxMax_(-std::numeric_limits<double>::max()),
          timeManager_(&timeManager),
          deleteTimeManager_(false),
          variables_(gridView),
          outputInterval_(1)
    {
        // calculate the bounding box of the grid view
        VertexIterator vIt = gridView.template begin<dim>();
        const VertexIterator vEndIt = gridView.template end<dim>();
        for (; vIt!=vEndIt; ++vIt) {
            for (int i=0; i<dim; i++) {
                bboxMin_[i] = std::min(bboxMin_[i], vIt->geometry().center()[i]);
                bboxMax_[i] = std::max(bboxMax_[i], vIt->geometry().center()[i]);
            }
        }

        model_ = new Model(asImp_()) ;

        resultWriter_ = NULL;
    }

    //! destructor
    virtual ~OneModelProblem ()
    {
        delete model_;
        delete resultWriter_;
        if (deleteTimeManager_)
            delete timeManager_;
    }

    /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary segment.
     *
     * \param bcTypes The boundary types for the conservation equations
     * \param intersection The intersection for which the boundary type is set
     */
    void boundaryTypes(BoundaryTypes &bcTypes,
                       const Intersection &intersection) const
    {
        // forward it to the method which only takes the global coordinate
        asImp_().boundaryTypesAtPos(bcTypes, intersection.geometry().center());
    }

    /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary segment.
     *
     * \param bcTypes The boundary types for the conservation equations
     * \param globalPos The position of the center of the boundary intersection
     */
    void boundaryTypesAtPos(BoundaryTypes &bcTypes,
                       const GlobalPosition &globalPos) const
    {
        // Throw an exception (there is no reasonable default value
        // for Dirichlet conditions)
        DUNE_THROW(Dune::InvalidStateException,
                   "The problem does not provide "
                   "a boundaryTypesAtPos() method.");
    }

    /*!
     * \brief Evaluate the boundary conditions for a dirichlet
     *        control volume.
     *
     * \param values The dirichlet values for the primary variables
     * \param intersection The boundary intersection
     *
     * For this method, the \a values parameter stores primary variables.
     */
    void dirichlet(PrimaryVariables &values,
            const Intersection &intersection) const
    {
        // forward it to the method which only takes the global coordinate
        asImp_().dirichletAtPos(values, intersection.geometry().center());
    }

    /*!
     * \brief Evaluate the boundary conditions for a dirichlet
     *        control volume.
     *
     * \param values The dirichlet values for the primary variables
     * \param globalPos The position of the center of the boundary intersection
     *
     * For this method, the \a values parameter stores primary variables.
     */
    void dirichletAtPos(PrimaryVariables &values,
            const GlobalPosition &globalPos) const
    {
        // Throw an exception (there is no reasonable default value
        // for Dirichlet conditions)
        DUNE_THROW(Dune::InvalidStateException,
                   "The problem specifies that some boundary "
                   "segments are dirichlet, but does not provide "
                   "a dirichletAtPos() method.");
    }

    /*!
     * \brief Evaluate the boundary conditions for a neumann
     *        boundary segment.
     *
     * \param values The neumann values for the conservation equations [kg / (m^2 *s )]
     * \param intersection The boundary intersection
     *
     * For this method, the \a values parameter stores the mass flux
     * in normal direction of each phase. Negative values mean influx.
     */
    void neumann(PrimaryVariables &values,
            const Intersection &intersection) const
    {
        // forward it to the interface with only the global position
        asImp_().neumannAtPos(values, intersection.geometry().center());
    }

    /*!
     * \brief Evaluate the boundary conditions for a neumann
     *        boundary segment.
     *
     * \param values The neumann values for the conservation equations [kg / (m^2 *s )]
     * \param globalPos The position of the center of the boundary intersection
     *
     * For this method, the \a values parameter stores the mass flux
     * in normal direction of each phase. Negative values mean influx.
     */
    void neumannAtPos(PrimaryVariables &values,
            const GlobalPosition &globalPos) const
    {
        // Throw an exception (there is no reasonable default value
        // for Neumann conditions)
        DUNE_THROW(Dune::InvalidStateException,
                   "The problem specifies that some boundary "
                   "segments are neumann, but does not provide "
                   "a neumannAtPos() method.");
    }

    /*!
     * \brief Evaluate the source term
     *
     * \param values The source and sink values for the conservation equations
     * \param element The element
     *
     * For this method, the \a values parameter stores the rate mass
     * generated or annihilate per volume unit. Positive values mean
     * that mass is created, negative ones mean that it vanishes.
     */
    void source(PrimaryVariables &values,
                const Element &element) const
    {
        // forward to generic interface
        asImp_().sourceAtPos(values, element.geometry().center());
    }

    /*!
     * \brief Evaluate the source term for all phases within a given
     *        sub-control-volume.
     *
     * \param values The source and sink values for the conservation equations
     * \param globalPos The position of the center of the finite volume
     *            for which the source term ought to be
     *            specified in global coordinates
     *
     * For this method, the \a values parameter stores the rate mass
     * generated or annihilate per volume unit. Positive values mean
     * that mass is created, negative ones mean that it vanishes.
     */
    void sourceAtPos(PrimaryVariables &values,
            const GlobalPosition &globalPos) const
    {         // Throw an exception (there is no initial condition)
        DUNE_THROW(Dune::InvalidStateException,
                   "The problem does not provide "
                   "a sourceAtPos() method.");
        }

    /*!
     * \brief Evaluate the initial value for a control volume.
     *
     * \param values The initial values for the primary variables
     * \param element The element
     *
     * For this method, the \a values parameter stores primary
     * variables.
     */
    void initial(PrimaryVariables &values,
                 const Element &element) const
    {
        // forward to generic interface
        asImp_().initialAtPos(values, element.geometry().center());
    }

    /*!
     * \brief Evaluate the initial value for a control volume.
     *
     * \param values The dirichlet values for the primary variables
     * \param globalPos The position of the center of the finite volume
     *            for which the initial values ought to be
     *            set (in global coordinates)
     *
     * For this method, the \a values parameter stores primary variables.
     */
    void initialAtPos(PrimaryVariables &values,
            const GlobalPosition &globalPos) const
    {
        // Throw an exception (there is no initial condition)
        DUNE_THROW(Dune::InvalidStateException,
                   "The problem does not provide "
                   "a initialAtPos() method.");
    }

    /*!
     * \brief Called by the Dumux::TimeManager in order to
     *        initialize the problem.
     */
    void init()
    {
        // set the initial condition of the model
        model().initialize();
    }

    /*!
     * \brief Called by Dumux::TimeManager just before the time
     *        integration.
     */
    void preTimeStep()
    { };

    /*!
     * \brief Called by Dumux::TimeManager in order to do a time
     *        integration on the model.
     */
    void timeIntegration()
    { };

    /*!
     * \brief Called by Dumux::TimeManager whenever a solution for a
     *        timestep has been computed and the simulation time has
     *        been updated.
     *
     * This is used to do some janitorial tasks like writing the
     * current solution to disk.
     */
    void postTimeStep()
    { };

    /*!
     * \brief Called by the time manager after everything which can be
     *        done about the current time step is finished and the
     *        model should be prepared to do the next time integration.
     */
    void advanceTimeLevel()
    {}

    /*!
     * \brief Returns the current time step size [seconds].
     */
    Scalar timeStepSize() const
    { return timeManager().timeStepSize(); }

    /*!
     * \brief Sets the current time step size [seconds].
     */
    void setTimeStepSize(Scalar dt)
    { timeManager().setTimeStepSize(dt); }

    /*!
     * \brief Called by Dumux::TimeManager whenever a solution for a
     *        timestep has been computed and the simulation time has
     *        been updated.
     */
    Scalar nextTimeStepSize(Scalar dt)
    { return timeManager().timeStepSize();}

    /*!
     * \brief Returns true if a restart file should be written to
     *        disk.
     *
     * The default behaviour is to write one restart file every 5 time
     * steps. This file is intented to be overwritten by the
     * implementation.
     */
    bool shouldWriteRestartFile() const
    {
        return
            timeManager().timeStepIndex() > 0 &&
            (timeManager().timeStepIndex() % 5 == 0);
    }

    /*!
     * \brief Sets the interval for Output
     *
     * The default is 1 -> Output every time step
     */
    void setOutputInterval(int interval)
    { outputInterval_ = interval; }

    /*!
     * \brief Returns true if the current solution should be written to
     *        disk (i.e. as a VTK file)
     *
     * The default behaviour is to write out every the solution for
     * very time step. This file is intented to be overwritten by the
     * implementation.
     */
    bool shouldWriteOutput() const
    {
        if (this->timeManager().timeStepIndex() % outputInterval_ == 0 || this->timeManager().willBeFinished())
        {
            return true;
        }
        return false;
    }

    void addOutputVtkFields()
    {}

    //! Write the fields current solution into an VTK output file.
    void writeOutput(bool verbose = true)
    {
        if (verbose && gridView().comm().rank() == 0)
            std::cout << "Writing result file for current time step\n";
        if (!resultWriter_)
            resultWriter_ = new VtkMultiWriter(gridView(), asImp_().name());
        resultWriter_->beginWrite(timeManager().time() + timeManager().timeStepSize());
        model().addOutputVtkFields(*resultWriter_);
        asImp_().addOutputVtkFields();
        resultWriter_->endWrite();
    }

    /*!
     * \brief Called when the end of an simulation episode is reached.
     */
    void episodeEnd()
    {
        std::cerr << "The end of an episode is reached, but the problem "
                  << "does not override the episodeEnd() method. "
                  << "Doing nothing!\n";
    };

    // \}

    /*!
     * \brief The problem name.
     *
     * This is used as a prefix for files generated by the simulation.
     * It could be either overwritten by the problem files, or simply
     * declared over the setName() function in the application file.
     */
    const char *name() const
    {
        return simname_.c_str();
    }

    /*!
     * \brief Set the problem name.
     *
     * This function sets the simulation name, which should be called before
     * the application problem is declared! If not, the default name "sim"
     * will be used.
     */
    void setName(const char *newName)
    {
        simname_ = newName;
    }

    /*!
     * \brief The GridView which used by the problem.
     */
    const GridView &gridView() const
    { return gridView_; }

    /*!
     * \brief Returns the mapper for vertices to indices.
     */
    const VertexMapper &vertexMapper() const
    { return variables_.vertexMapper(); }

    /*!
     * \brief Returns the mapper for elements to indices.
     */
    const ElementMapper &elementMapper() const
    { return variables_.elementMapper(); }

    /*!
     * \brief The coordinate of the corner of the GridView's bounding
     *        box with the smallest values.
     */
    const GlobalPosition &bboxMin() const
    { return bboxMin_; }

    /*!
     * \brief The coordinate of the corner of the GridView's bounding
     *        box with the largest values.
     */
    const GlobalPosition &bboxMax() const
    { return bboxMax_; }

    /*!
     * \brief Returns TimeManager object used by the simulation
     */
    TimeManager &timeManager()
    { return *timeManager_; }

    /*!
     * \brief \copybrief Dumux::OneModelProblem::timeManager()
     */
    const TimeManager &timeManager() const
    { return *timeManager_; }

    /*!
     * \brief Returns variables object.
     */
    Variables& variables ()
    { return variables_; }

    /*!
     * \brief \copybrief Dumux::OneModelProblem::variables()
     */
    const Variables& variables () const
    { return variables_; }

    /*!
     * \brief Returns numerical model used for the problem.
     */
    Model &model()
    { return *model_; }

    /*!
     * \brief \copybrief Dumux::OneModelProblem::model()
     */
    const Model &model() const
    { return *model_; }
    // \}


    /*!
     * \name Restart mechanism
     */
    // \{

    /*!
     * \brief This method writes the complete state of the problem
     *        to the harddisk.
     *
     * The file will start with the prefix returned by the name()
     * method, has the current time of the simulation clock in it's
     * name and uses the extension <tt>.drs</tt>. (Dumux ReStart
     * file.)  See Dumux::Restart for details.
     */
    void serialize()
    {
        typedef Dumux::Restart Restarter;

        Restarter res;
        res.serializeBegin(asImp_());
        std::cerr << "Serialize to file " << res.fileName() << "\n";

        timeManager().serialize(res);
        resultWriter().serialize(res);
        model().serialize(res);

        res.serializeEnd();
    }

    /*!
     * \brief This method restores the complete state of the problem
     *        from disk.
     *
     * It is the inverse of the serialize() method.
     */
    void restart(double tRestart)
    {
        typedef Dumux::Restart Restarter;

        Restarter res;
        res.deserializeBegin(asImp_(), tRestart);
        std::cerr << "Deserialize from file " << res.fileName() << "\n";

        timeManager().deserialize(res);
        resultWriter().deserialize(res);
        model().deserialize(res);

        res.deserializeEnd();
    };
    //! Use restart() function instead!
    void deserialize(double tRestart) DUNE_DEPRECATED
    { restart(tRestart);}

    // \}

protected:
    VtkMultiWriter& resultWriter()
    {
        if (!resultWriter_)
            resultWriter_ = new VtkMultiWriter(gridView_, asImp_().name());
        return *resultWriter_;
    }

    VtkMultiWriter& resultWriter() const
    {
        if (!resultWriter_)
            resultWriter_ = new VtkMultiWriter(gridView_, asImp_().name());
        return *resultWriter_;
    }

private:
    //! Returns the implementation of the problem (i.e. static polymorphism)
    Implementation &asImp_()
    { return *static_cast<Implementation *>(this); }

    //! \brief \copybrief Dumux::OneModelProblem::asImp_()
    const Implementation &asImp_() const
    { return *static_cast<const Implementation *>(this); }

    std::string simname_; // a string for the name of the current simulation,
                                  // which could be set by means of an program argument,
                                 // for example.
    const GridView gridView_;

    GlobalPosition bboxMin_;
    GlobalPosition bboxMax_;

    TimeManager *timeManager_;
    bool deleteTimeManager_;

    Variables variables_;

    Model* model_;

    VtkMultiWriter *resultWriter_;
    int outputInterval_;
};

}
#endif
