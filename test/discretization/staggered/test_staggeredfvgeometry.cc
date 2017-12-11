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
 * \brief Test for finite volume element geometry, sub control volume, and sub
          control volume faces
 */
#include <config.h>

#include <iostream>
#include <utility>
#include <iomanip>

#include <dune/common/test/iteratortest.hh>
#include <dune/grid/utility/structuredgridfactory.hh>
#include <dune/grid/yaspgrid.hh>

#include <dumux/common/properties.hh>
#include <dumux/discretization/staggered/properties.hh>

namespace Dumux
{

//! Dummy flux variables class so that we can update the connectivity map
class MockFluxVariables
{
public:

  template<class Map, class Element, class FvGeometry, class Scvf>
  void computeCellCenterToCellCenterStencil(Map& map,
                                            const Element& element,
                                            const FvGeometry& fvGeometry,
                                            const Scvf& scvf)
  {}

  template<class Map, class Element, class FvGeometry, class Scvf>
  void computeCellCenterToFaceStencil(Map& map,
                                      const Element& element,
                                      const FvGeometry& fvGeometry,
                                      const Scvf& scvf)
  {}

  template<class Map, class FvGeometry, class Scvf>
  void computeFaceToCellCenterStencil(Map& map,
                                      const FvGeometry& fvGeometry,
                                      const Scvf& scvf)
  {}

  template<class Map, class FvGeometry, class Scvf>
  void computeFaceToFaceStencil(Map& map,
                                const FvGeometry& fvGeometry,
                                const Scvf& scvf)
  {}

};

namespace Properties
{
NEW_TYPE_TAG(TestFVGeometry, INHERITS_FROM(StaggeredModel));

SET_TYPE_PROP(TestFVGeometry, Grid, Dune::YaspGrid<2>);

SET_TYPE_PROP(TestFVGeometry, FluxVariables, MockFluxVariables);

SET_BOOL_PROP(TestFVGeometry, EnableFVGridGeometryCache, true);
}

}

template<class T>
class NoopFunctor {
public:
  NoopFunctor() {}
  void operator()(const T& t){}
};

int main (int argc, char *argv[]) try
{
    // maybe initialize mpi
    Dune::MPIHelper::instance(argc, argv);

    std::cout << "Checking the FVGeometries, SCVs and SCV faces" << std::endl;

    // aliases
    using TypeTag = TTAG(TestFVGeometry);
    using Grid = typename GET_PROP_TYPE(TypeTag, Grid);
    using GridView = typename Grid::LeafGridView;

    constexpr int dim = GridView::dimension;
    constexpr int dimworld = GridView::dimensionworld;

    using GlobalPosition = typename Dune::FieldVector<Grid::ctype, dimworld>;
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using FVGridGeometry = typename GET_PROP_TYPE(TypeTag, FVGridGeometry);

    // make a grid
    GlobalPosition lower(0.0);
    GlobalPosition upper(1.0);
    std::array<unsigned int, dim> els{{2, 4}};
    std::shared_ptr<Grid> grid = Dune::StructuredGridFactory<Grid>::createCubeGrid(lower, upper, els);
    auto leafGridView = grid->leafGridView();

    FVGridGeometry fvGridGeometry(leafGridView);
    fvGridGeometry.update();

    // iterate over elements. For every element get fv geometry and loop over scvs and scvfaces
    for (const auto& element : elements(leafGridView))
    {
        auto eIdx = fvGridGeometry.elementMapper().index(element);
        std::cout << std::endl << "Checking fvGeometry of element " << eIdx << std::endl;
        auto fvGeometry = localView(fvGridGeometry);
        fvGeometry.bind(element);

        auto range = scvs(fvGeometry);
        NoopFunctor<SubControlVolume> op;
        if(0 != testForwardIterator(range.begin(), range.end(), op))
            DUNE_THROW(Dune::Exception, "Iterator does not fulfill the forward iterator concept");

        for (auto&& scv : scvs(fvGeometry))
        {
            std::cout << "-- scv " << scv.indexInElement() << " center at: " << scv.center() << " , volume: " << scv.volume()  << std::endl;
        }

        auto range2 = scvfs(fvGeometry);
        NoopFunctor<SubControlVolumeFace> op2;
        if(0 != testForwardIterator(range2.begin(), range2.end(), op2))
            DUNE_THROW(Dune::Exception, "Iterator does not fulfill the forward iterator concept");

        for (auto&& scvf : scvfs(fvGeometry))
        {
            std::cout << "-- scvf " << scvf.index() << " ip at: " << scvf.ipGlobal() << " normal: " << scvf.unitOuterNormal();
            if (scvf.boundary()) std::cout << " (on boundary).";
            std::cout << std::endl;
        }
    }
}
// //////////////////////////////////
//   Error handler
// /////////////////////////////////
catch (Dune::Exception e) {

    std::cout << e << std::endl;
    return 1;
}
