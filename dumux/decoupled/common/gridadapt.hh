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
 * \brief Base class for h-adaptive sequential models.
 */
#ifndef DUMUX_GIRDADAPT_HH
#define DUMUX_GIRDADAPT_HH

#include "decoupledproperties.hh"
#include "gridadaptproperties.hh"

namespace Dumux
{

/*!\ingroup IMPET
 * @brief Standard Module for h-adaptive simulations
 *
 * This class is created by the problem class with the template
 * parameters <TypeTag, true> and provides basic functionality
 * for adaptive methods:
 *
 * A standard implementation adaptGrid() will prepare everything
 * to calculate the next pressure field on the new grid.
 */
template<class TypeTag, bool adaptive>
class GridAdapt
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar)   Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Problem)  Problem;


    //*******************************
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GridView::Grid                         Grid;
    typedef typename Grid::LeafGridView                     LeafGridView;
    typedef typename LeafGridView::template Codim<0>::Iterator LeafIterator;
    typedef typename GridView::IntersectionIterator         LeafIntersectionIterator;
    typedef typename Grid::template Codim<0>::Entity         Element;
    typedef typename Grid::template Codim<0>::EntityPointer         ElementPointer;

    typedef typename GET_PROP_TYPE(TypeTag, AdaptionIndicator) AdaptionIndicator;
    typedef typename GET_PROP_TYPE(TypeTag, AdaptionInitializationIndicator) AdaptionInitializationIndicator;

public:
    /*!
     * Constructor for h-adaptive simulations (adaptive grids)
     * @param problem The problem
     */
    GridAdapt (Problem& problem)
    : problem_(problem), adaptionIndicator_(problem), marked_(0), coarsened_(0)
    {
        levelMin_ = GET_PARAM_FROM_GROUP(TypeTag, int, GridAdapt, MinLevel);
        levelMax_ = GET_PARAM_FROM_GROUP(TypeTag, int, GridAdapt, MaxLevel);
        adaptationInterval_ = GET_PARAM_FROM_GROUP(TypeTag, int, GridAdapt, AdaptionInterval);

        if (levelMin_ < 0)
            Dune::dgrave <<  __FILE__<< ":" <<__LINE__
            << " :  Dune cannot coarsen to gridlevels smaller 0! "<< std::endl;
    }

    void init()
    {
        adaptionIndicator_.init();

        if (!GET_PARAM_FROM_GROUP(TypeTag, bool, GridAdapt, EnableInitializationIndicator))
            return;

        AdaptionInitializationIndicator adaptionInitIndicator(problem_, adaptionIndicator_);

        int maxIter = 2*levelMax_;
        int iter = 0;
        while (iter <= maxIter)
        {
            adaptGrid(adaptionInitIndicator);

            if (marked_ == 0 && coarsened_ == 0)
            {
                break;
            }

            problem_.model().initialize();
            iter++;
        }
    }

    /*!
     * @brief Standard method to adapt the grid
     *
     * This method is called in preTimeStep() of the problem if
     * adaptive grids are used in the simulation.
     *
     * It uses a standard procedure for adaptivity:
     * 1) Determine the refinement indicator
     * 2) Mark the elements
     * 3) Store primary variables in a map
     * 4) Adapt the grid, adapt variables sizes, update mappers
     * 5) Reconstruct primary variables, regain secondary variables
     */
    void adaptGrid()
    {
        adaptGrid(adaptionIndicator_) ;
    }

    template<class Indicator>
    void adaptGrid(Indicator& indicator)
    {
        // reset internal counter for marked elements
        marked_ = coarsened_ = 0;

        // check for adaption interval: Adapt only at certain time step indices
        if (problem_.timeManager().timeStepIndex() % adaptationInterval_ != 0)
            return;

        /**** 1) determine refining parameter if standard is used ***/
        // if not, the indicatorVector and refinement Bounds have to
        // specified by the problem through setIndicator()
        indicator.calculateIndicator();

        /**** 2) mark elements according to indicator     *********/
        markElements(indicator);

        // abort if nothing in grid is marked
        if (marked_==0 && coarsened_ == 0)
            return;
        else
            Dune::dinfo << marked_ << " cells have been marked_ to be refined, "
                << coarsened_ << " to be coarsened." << std::endl;

        /****  2b) Do pre-adaption step    *****/
        problem_.grid().preAdapt();
        problem_.preAdapt();

        /****  3) Put primary variables in a map         *********/
        problem_.variables().storePrimVars(problem_);

        /****  4) Adapt Grid and size of variable vectors    *****/
        problem_.grid().adapt();

//        forceRefineRatio(1);

        // update mapper to new cell idice
        problem_.variables().elementMapper().update();

        // adapt size of vectors (
        problem_.variables().adaptVariableSize(problem_.variables().elementMapper().size());

        /****  5) (Re-)construct primary variables to new grid **/
        problem_.variables().reconstructPrimVars(problem_);

        // delete markers in grid
        problem_.grid().postAdapt();

        // write out new grid
//        Dune::VTKWriter<LeafGridView> vtkwriter(problem_.gridView());
//        vtkwriter.write("latestgrid",Dune::VTK::appendedraw);
        return;
    }

    /*!
     * Mark Elements for grid refinement according to applied Indicator
     * @return Total ammount of marked cells
     */
    template<class Indicator>
    int markElements(Indicator& indicator)
    {
        std::map<int, int> coarsenMarker;
        const typename Grid::Traits::LocalIdSet& idSet(problem_.grid().localIdSet());

        for (LeafIterator eIt = problem_.gridView().template begin<0>();
                    eIt!=problem_.gridView().template end<0>(); ++eIt)
        {
            // Verfeinern?
            if (indicator.refine(*eIt) && eIt->level() < levelMax_)
            {
                problem_.grid().mark( 1,  *eIt);
                ++marked_;

                // this also refines the neighbor elements
                checkNeighborsRefine_(*eIt);
            }
            if (indicator.coarsen(*eIt) && eIt->hasFather())
            {
                int idx = idSet.id(*(eIt->father()));
                std::map<int, int>::iterator it = coarsenMarker.find(idx);
                if (it != coarsenMarker.end())
                {
                    it->second++;
                }
                else
                {
                    coarsenMarker[idx] = 1;
                }
            }
        }
        // coarsen
        for (LeafIterator eIt = problem_.gridView().template begin<0>();
                    eIt!=problem_.gridView().template end<0>(); ++eIt)
        {
            if (indicator.coarsen(*eIt) 
                && eIt->level() > levelMin_ 
                && problem_.grid().getMark(*eIt) == 0
                && coarsenMarker[idSet.id(*(eIt->father()))] == eIt->geometry().corners())
            {
                // check if coarsening is possible
                bool coarsenPossible = true;
                LeafIntersectionIterator isend = problem_.gridView().iend(*eIt);
                for(LeafIntersectionIterator is = problem_.gridView().ibegin(*eIt); is != isend; ++is)
                {
                    if(is->neighbor())
                    {
                        ElementPointer outside = is->outside();
                        if ((problem_.grid().getMark(*outside) > 0)
                            || outside.level()>eIt.level()))
                        {
                            coarsenPossible = false;
                        }
                    }
                }

                if(coarsenPossible)
                {
                    problem_.grid().mark( -1, *eIt );
                    ++coarsened_;
                }
            }
        }

        return marked_;
    }

    bool wasAdapted()
    {
        return (marked_ != 0 || coarsened_ != 0);
    }

    /*!
     * Sets minimum and maximum refinement levels
     *
     * @param levMin minimum level for coarsening
     * @param levMax maximum level for refinement
     */
    void setLevels(int levMin, int levMax)
    {
        if (levMin < 0)
            Dune::dgrave <<  __FILE__<< ":" <<__LINE__
            << " :  Dune cannot coarsen to gridlevels smaller 0! "<< std::endl;
        levelMin_ = levMin;
        levelMax_ = levMax;
    }

    /*!
     * Gets maximum refinement level
     *
     * @return levelMax_ maximum level for refinement
     */
    const int getMaxLevel() const
    {
        return levelMax_;
    }
    /*!
     * Gets minimum refinement level
     *
     * @return levelMin_ minimum level for coarsening
     */
    const int getMinLevel() const
    {
        return levelMin_;
    }

    AdaptionIndicator& adaptionIndicator()
    {
        return adaptionIndicator_;
    }

    AdaptionIndicator& adaptionIndicator() const
    {
        return adaptionIndicator_;
    }

private:
    /*!
     * @brief Method ensuring the refinement ratio of 2:1
     *
     * For any given entity, a loop over the neighbors checks weather the
     * entities refinement would require that any of the neighbors has
     * to be refined, too.
     * This is done recursively over all levels of the grid.
     *
     * @param entity Element of interest that is to be refined
     * @param level level of the refined entity: it is at least 1
     * @return true if everything was successful
     */
    bool checkNeighborsRefine_(const Element &entity, int level = 1)
    {
        // this also refines the neighbor elements
        LeafIntersectionIterator isend = problem_.gridView().iend(entity);
        for(LeafIntersectionIterator is = problem_.gridView().ibegin(entity); is != isend; ++is)
        {
            if(!is->neighbor())
                continue;

            ElementPointer outside = is->outside();
            if ((outside.level()<levelMax_)
                    && (outside.level()<entity.level()))
                {
                    problem_.grid().mark(1, *outside);
                    ++marked_;

                    if(level != levelMax_)
                        checkNeighborsRefine_(*outside, ++level);
                }
        }
        return true;
    };


    /*!
     * \brief Enforces a given refine ratio after grid was adapted
     *
     * If the refine ratio is not taken into consideration during
     * marking, then this method ensures a certain ratio.
     *
     * @param maxLevelDelta The maximum level difference (refine ratio)
     *             between neighbors.
     */
    void forceRefineRatio(int maxLevelDelta = 1)
    {
        LeafGridView leafView = problem_.gridView();
        // delete all existing marks
        problem_.grid().postAdapt();
        bool done;
        do
        {
            // run through all cells
            done=true;
            for (LeafIterator eIt = leafView.template begin<0>();
                        eIt!=leafView.template end<0>(); ++eIt)
            {
                // run through all neighbor-cells (intersections)
                LeafIntersectionIterator isItend = leafView.iend(*eIt);
                for (LeafIntersectionIterator isIt = leafView.ibegin(*eIt); isIt!= isItend; ++isIt)
                {
                    const typename LeafIntersectionIterator::Intersection intersection = *isIt;
                    if(!intersection.neighbor())
                        continue;

                    ElementPointer outside =intersection.outside();
                    if (eIt.level()+maxLevelDelta<outside.level())
                            {
                            ElementPointer entity =eIt;
                            problem_.grid().mark( 1, *entity );
                            done=false;
                            }
                }
            }
            if (done==false)
            {
                // adapt the grid
                problem_.grid().adapt();
                // delete marks
                problem_.grid().postAdapt();
            }
        }
        while (done!=true);
    }

    // private Variables
    Problem& problem_;
    AdaptionIndicator adaptionIndicator_;

    int marked_;
    int coarsened_;

    int levelMin_;
    int levelMax_;

    int adaptationInterval_;
};

/*!
 * @brief Class for NON-adaptive simulations
 *
 * This class provides empty methods for non-adaptive simulations
 * for compilation reasons. If adaptivity is desired, create the
 * class with template arguments <TypeTag, true> instead.
 */
template<class TypeTag>
class GridAdapt<TypeTag, false>
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar)   Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Problem)     Problem;
    typedef typename GET_PROP(TypeTag, SolutionTypes) SolutionTypes;
    typedef typename SolutionTypes::ScalarSolution ScalarSolutionType;

public:
    void init()
    {};
    void adaptGrid()
    {};
    bool wasAdapted()
    {
        return false;
    }
    void setLevels(int, int)
    {};
    void setTolerance(int, int)
    {};
    const void setIndicator(const ScalarSolutionType&,
            const Scalar&, const Scalar&)
    {};
    GridAdapt (Problem& problem)
    {}
};

}
#endif /* DUMUX_GIRDADAPT_HH */
