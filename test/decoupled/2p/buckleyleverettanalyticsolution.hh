// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
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
#ifndef DUMUX_BUCKLEYLEVERETT_ANALYTICAL_HH
#define DUMUX_BUCKLEYLEVERETT_ANALYTICAL_HH

#include <dumux/decoupled/2p/2pproperties.hh>
#include <dumux/material/fluidmatrixinteractions/2p/linearmaterial.hh>
#include <dumux/material/fluidmatrixinteractions/2p/efftoabslaw.hh>

/**
 * @file
 * @brief  Analytical solution of the buckley-leverett problem
 * @author Markus Wolff
 */

namespace Dumux
{
/**
 * \ingroup fracflow
 * @brief IMplicit Pressure Explicit Saturation (IMPES) scheme for the solution of
 * the Buckley-Leverett problem
 */

template<typename Scalar, typename Law>
struct CheckMaterialLaw
{
    static bool isLinear()
    {
        return false;
    }
};

template<typename Scalar>
struct CheckMaterialLaw<Scalar, LinearMaterial<Scalar> >
{
    static bool isLinear()
    {
        return true;
    }
};

template<typename Scalar>
struct CheckMaterialLaw<Scalar, EffToAbsLaw< LinearMaterial<Scalar> > >
{
    static bool isLinear()
    {
        return true;
    }
};

template<class TypeTag> class BuckleyLeverettAnalytic
{
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, Grid) Grid;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;

    typedef typename GET_PROP_TYPE(TypeTag, SpatialParams) SpatialParams;
    typedef typename SpatialParams::MaterialLaw MaterialLaw;
    typedef typename MaterialLaw::Params MaterialLawParams;

    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, FluidState) FluidState;

    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    typedef typename GET_PROP_TYPE(TypeTag, CellData) CellData;

    enum
    {
        dim = GridView::dimension, dimworld = GridView::dimensionworld
    };
    enum
    {
        wPhaseIdx = Indices::wPhaseIdx,
        nPhaseIdx = Indices::nPhaseIdx,
        satEqIdx = Indices::satEqIdx,
    };

    typedef Dune::BlockVector<Dune::FieldVector<Scalar, 1> > BlockVector;
    typedef typename GridView::Traits::template Codim<0>::Entity Element;
    typedef typename GridView::Traits::template Codim<0>::EntityPointer ElementPointer;
    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    typedef Dune::FieldVector<Scalar, dimworld> GlobalPosition;

private:

    // functions needed for analytical solution

    void initializeAnalytic()
    {
    	int size = problem_.gridView().size(0);
        analyticSolution_.resize(size);
        analyticSolution_ = 0;
        errorGlobal_.resize(size);
        errorGlobal_ = 0;
        errorLocal_.resize(size);
        errorLocal_ = 0;

        return;
    }
    /*!
     * \brief DOC ME!
     * \param materialLawParams DOC ME!
     */
    void prepareAnalytic()
    {
    	ElementIterator dummyElement = problem_.gridView().template begin<0>();
        const MaterialLawParams& materialLawParams(problem_.spatialParams().materialLawParams(*dummyElement));

        swr_ = materialLawParams.Swr();
        snr_ = materialLawParams.Snr();
        Scalar porosity = problem_.spatialParams().porosity(*dummyElement);

        FluidState fluidState;
        fluidState.setTemperature(problem_.temperature(*dummyElement));
        fluidState.setPressure(wPhaseIdx, problem_.referencePressure(*dummyElement));
        fluidState.setPressure(nPhaseIdx, problem_.referencePressure(*dummyElement));
        Scalar viscosityW = FluidSystem::viscosity(fluidState, wPhaseIdx);
        Scalar viscosityNW = FluidSystem::viscosity(fluidState, nPhaseIdx);


        if (CheckMaterialLaw<Scalar, MaterialLaw>::isLinear() && viscosityW == viscosityNW)
        {
        	std::pair<Scalar, Scalar> entry;
        	entry.first = 1 - snr_;

        	entry.second = vTot_  / porosity;

        	frontParams_.push_back(entry);
        }
        else
        {
        Scalar sw0 = swr_;
        Scalar fw0 = MaterialLaw::krw(materialLawParams, sw0)/viscosityW;
        fw0 /= (fw0 + MaterialLaw::krn(materialLawParams, sw0)/viscosityNW);
        Scalar sw1 = sw0 + deltaS_;
        Scalar fw1 = MaterialLaw::krw(materialLawParams, sw1)/viscosityW;
        fw1 /= (fw1 + MaterialLaw::krn(materialLawParams, sw1)/viscosityNW);
        Scalar tangentSlopeOld = (fw1 - fw0)/(sw1 - sw0);
        sw1 += deltaS_;
        fw1 = MaterialLaw::krw(materialLawParams, sw1)/viscosityW;
        fw1 /= (fw1 + MaterialLaw::krn(materialLawParams, sw1)/viscosityNW);
        Scalar tangentSlopeNew = (fw1 - fw0)/(sw1 - sw0);

        while (tangentSlopeNew >= tangentSlopeOld && sw1 < (1.0 - snr_))
        {
        	tangentSlopeOld = tangentSlopeNew;
            sw1 += deltaS_;
            fw1 = MaterialLaw::krw(materialLawParams, sw1)/viscosityW;
            fw1 /= (fw1 + MaterialLaw::krn(materialLawParams, sw1)/viscosityNW);
            tangentSlopeNew = (fw1 - fw0)/(sw1 - sw0);
        }

        sw0 = sw1 - deltaS_;
        fw0 = MaterialLaw::krw(materialLawParams, sw0)/viscosityW;
                fw0 /= (fw0 + MaterialLaw::krn(materialLawParams, sw0)/viscosityNW);
        Scalar sw2 = sw1 + deltaS_;
        Scalar fw2 = MaterialLaw::krw(materialLawParams, sw2)/viscosityW;
        fw2 /= (fw2 + MaterialLaw::krn(materialLawParams, sw2)/viscosityNW);
        while (sw1 <= (1.0 - snr_))
        {
        	std::pair<Scalar, Scalar> entry;
        	entry.first = sw1;

        	Scalar dfwdsw = (fw2 - fw0)/(sw2 - sw0);

        	entry.second = vTot_  / porosity * dfwdsw;

        	frontParams_.push_back(entry);

        	sw0 = sw1;
        	sw1 = sw2;
        	fw0 = fw1;
        	fw1 = fw2;

        	sw2 += deltaS_;
        	fw2 = MaterialLaw::krw(materialLawParams, sw2)/viscosityW;
        	fw2 /= (fw2 + MaterialLaw::krn(materialLawParams, sw2)/viscosityNW);
        }
        }

        return;
    }

    void calcSatError()
    {
        Scalar globalVolume = 0;
        Scalar errorNorm = 0.0;

        ElementIterator eItEnd = problem_.gridView().template end<0> ();
        for (ElementIterator eIt = problem_.gridView().template begin<0> (); eIt != eItEnd; ++eIt)
        {
            // get entity
            ElementPointer element = *eIt;
            int index = problem_.variables().index(*element);

            Scalar sat = problem_.variables().cellData(index).saturation(wPhaseIdx);

            Scalar volume = element->geometry().volume();

            Scalar error = analyticSolution_[index] - sat;

            errorLocal_[index] = error;

            if (sat > swr_ + 1e-6)
            {
            globalVolume += volume;
            errorNorm += (volume*volume*error*error);
            }
        }

        if (globalVolume > 0.0 && errorNorm > 0.0)
        {
        	errorNorm = std::sqrt(errorNorm)/globalVolume;
        	errorGlobal_ = errorNorm;
        }
        else
        {
        	errorGlobal_ = 0;
        }

        return;
    }

    void updateExSol()
    {
    	Scalar time = problem_.timeManager().time() + problem_.timeManager().timeStepSize();

        // position of the fluid front

    	Scalar xMax =  frontParams_[0].second * time;

        // iterate over vertices and get analytic saturation solution
        ElementIterator eItEnd = problem_.gridView().template end<0> ();
        for (ElementIterator eIt = problem_.gridView().template begin<0> (); eIt != eItEnd; ++eIt)
        {
            // get global coordinate of cell center
            GlobalPosition globalPos = eIt->geometry().center();

            int index = problem_.variables().index(*eIt);

            if (globalPos[0] > xMax)
            	analyticSolution_[index] = swr_;
            else
            {
            	int size = frontParams_.size();
            	if (size == 1)
            	{
            		analyticSolution_[index] = frontParams_[0].first;
            	}
            	else
            	{
                    // find x_f next to global coordinate of the vertex
                    int xnext = 0;
                    for (int i = 0; i < size-1; i++)
                    {
                    	Scalar x = frontParams_[i].second * time;
                    	Scalar xMinus = frontParams_[i+1].second * time;
                        if (globalPos[0] <= x && globalPos[0] > xMinus)
                        {
                        	analyticSolution_[index] = frontParams_[i].first - (frontParams_[i].first - frontParams_[i+1].first)/(x - xMinus) * (x - globalPos[0]);
                            break;
                        }
                    }

            	}
            }
        }

        // call function to calculate the saturation error
        calcSatError();

        return;
    }

public:
    void calculateAnalyticSolution()
    {
        initializeAnalytic();

        updateExSol();
    }

    BlockVector AnalyticSolution() const
    {
        return analyticSolution_;
    }

    //Write saturation and pressure into file
    template<class MultiWriter>
    void addOutputVtkFields(MultiWriter &writer)
    {
    	int size = problem_.gridView().size(0);
        BlockVector *analyticSolution = writer.allocateManagedBuffer (size);
        BlockVector *errorGlobal = writer.allocateManagedBuffer (size);
        BlockVector *errorLocal = writer.allocateManagedBuffer (size);

        *analyticSolution = analyticSolution_;
        *errorGlobal = errorGlobal_;
        *errorLocal = errorLocal_;

        writer.attachCellData(*analyticSolution, "saturation (exact solution)");
        writer.attachCellData(*errorGlobal, "global error");
        writer.attachCellData(*errorLocal, "local error");

        return;
    }

    //! Construct an IMPES object.
    BuckleyLeverettAnalytic(Problem& problem) :
        problem_(problem), analyticSolution_(0), errorGlobal_(0), errorLocal_(0), frontParams_(0), deltaS_(1e-3)
    {}

    void initialize(Scalar vTot)
    {
    	vTot_ = vTot;
        initializeAnalytic();
        prepareAnalytic();
    }


private:
    Problem& problem_;

    BlockVector analyticSolution_;
    BlockVector errorGlobal_;
    BlockVector errorLocal_;

    std::vector<std::pair<Scalar, Scalar> > frontParams_;
    const Scalar deltaS_;

    Scalar swr_;
    Scalar snr_;
    Scalar vTot_;



    int tangentIdx_;
};
}
#endif
