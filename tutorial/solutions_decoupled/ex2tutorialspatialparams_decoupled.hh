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
 * \brief spatial parameters for the sequential tutorial
 */
#ifndef DUMUX_EX2TUTORIAL_SPATIAL_PARAMS_DECOUPLED_HH
#define DUMUX_EX2TUTORIAL_SPATIAL_PARAMS_DECOUPLED_HH


#include <dumux/material/spatialparams/fvspatialparams.hh>
#include <dumux/material/fluidmatrixinteractions/2p/linearmaterial.hh>
#include <dumux/material/fluidmatrixinteractions/2p/regularizedbrookscorey.hh>
#include <dumux/material/fluidmatrixinteractions/2p/efftoabslaw.hh>

namespace Dumux
{

//forward declaration
template<class TypeTag>
class Ex2TutorialSpatialParamsDecoupled;

namespace Properties
{
// The spatial parameters TypeTag
NEW_TYPE_TAG(Ex2TutorialSpatialParamsDecoupled);

// Set the spatial parameters
SET_TYPE_PROP(Ex2TutorialSpatialParamsDecoupled, SpatialParams,
        Dumux::Ex2TutorialSpatialParamsDecoupled<TypeTag>); /*@\label{tutorial-decoupled:set-spatialparameters}@*/

// Set the material law
SET_PROP(Ex2TutorialSpatialParamsDecoupled, MaterialLaw)
{
private:
    // material law typedefs
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef RegularizedBrooksCorey<Scalar> RawMaterialLaw;
public:
    typedef EffToAbsLaw<RawMaterialLaw> type;
};
}

//! Definition of the spatial parameters for the decoupled tutorial

template<class TypeTag>
class Ex2TutorialSpatialParamsDecoupled: public FVSpatialParams<TypeTag>
{
    typedef FVSpatialParams<TypeTag> ParentType;
    typedef typename GET_PROP_TYPE(TypeTag, Grid) Grid;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename Grid::ctype CoordScalar;

    enum
        {dim=Grid::dimension, dimWorld=Grid::dimensionworld, numEq=1};
    typedef typename Grid::Traits::template Codim<0>::Entity Element;

    typedef Dune::FieldVector<CoordScalar, dimWorld> GlobalPosition;
    typedef Dune::FieldVector<CoordScalar, dim> LocalPosition;
    typedef Dune::FieldMatrix<Scalar,dim,dim> FieldMatrix;

public:
    typedef typename GET_PROP_TYPE(TypeTag, MaterialLaw) MaterialLaw;
    typedef typename MaterialLaw::Params MaterialLawParams;

    /**
     * \brief Intrinsic permeability tensor K \f$[m^2]\f$ depending
     *        on the position in the domain
     *
     *  \param globalPos The global position in the domain.
     *
     *  Alternatively, the function intrinsicPermeabilityAtPos(const GlobalPosition& globalPos) could be
     *  defined, where globalPos is the vector including the global coordinates of the finite volume.
     */
    const FieldMatrix& intrinsicPermeabilityAtPos(const GlobalPosition& globalPos) const
    {
    	if(((globalPos[0]>25)and(globalPos[0]<75))and((globalPos[1]>15)and(globalPos[1]<35)))
            return K1;
    	else
    		return K2;
    }

    /** 
     * \brief Define the porosity \f$[-]\f$ of the porous medium depending
     *        on the position in the domain
     *
     *  \param globalPos The global position in the domain.
     *
     *  Alternatively, the function porosityAtPos(const GlobalPosition& globalPos) could be
     *  defined, where globalPos is the vector including the global coordinates of the finite volume.
     */
    double porosityAtPos(const GlobalPosition& globalPos) const
    {
    	if(((globalPos[0]>25)and(globalPos[0]<75))and((globalPos[1]>15)and(globalPos[1]<35)))
    	return 0.15;
    	else
        return 0.3;
    }

    /*! \brief Return the parameter object for the material law (i.e. Brooks-Corey)
     *         depending on the position in the domain
     *
     *  \param globalPos The global position in the domain.
     *
     *  Alternatively, the function materialLawParamsAtPos(const GlobalPosition& globalPos)
     *  could be defined, where globalPos is the vector including the global coordinates of
     *  the finite volume.
     */
    const MaterialLawParams& materialLawParamsAtPos(const GlobalPosition& globalPos) const
    {
    	if(((globalPos[0]>25)and(globalPos[0]<75))and((globalPos[1]>15)and(globalPos[1]<35)))
    	return materialParams2;
    	else
    	return materialParams1;
    }

    //! Constructor
    Ex2TutorialSpatialParamsDecoupled(const GridView& gridView)
    : ParentType(gridView), K1(0), K2(0)
    {
        for (int i = 0; i < dim; i++)
                K1[i][i] = 1e-8;
        for (int i = 0; i < dim; i++) 
        	    K2[i][i] = 1e-9;


    	//set residual saturations
            materialParams1.setSwr(0.0);                /*@\label{tutorial-coupled:setLawParams}@*/
            materialParams1.setSnr(0.0);
    	    materialParams2.setSwr(0.0);                /*@\label{tutorial-coupled:setLawParams}@*/
            materialParams2.setSnr(0.0);

            //parameters of Brooks & Corey Law
    	    materialParams1.setPe(100.0);
            materialParams1.setLambda(1.8);
    	    materialParams2.setPe(500.0);
            materialParams2.setLambda(2.0);
    }

private:
    MaterialLawParams materialLawParams_;
    FieldMatrix K1;
    FieldMatrix K2;

    MaterialLawParams materialParams1;                 /*@\label{tutorial-coupled:matParamsObject}@*/
    MaterialLawParams materialParams2;
   };

} // end namespace
#endif
