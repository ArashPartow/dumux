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
 * \brief Helper class to get information required for mpfa scheme.
 */
#ifndef DUMUX_DISCRETIZATION_CC_MPFA_HELPER_HH
#define DUMUX_DISCRETIZATION_CC_MPFA_HELPER_HH

#include "methods.hh"

namespace Dumux
{

/*!
 * \brief Mpfa method-specific implementation of the helper class (dimension-dependent)
 */
template<class TypeTag, MpfaMethods Method, int dim, int dimWorld>
class MpfaMethodHelper;

/*!
 * \brief Dimension-specific implementation of the helper class (common for all methods)
 */
template<class TypeTag, int dim, int dimWorld>
class MpfaDimensionHelper;

/*!
 * \brief Specialization for dim == 2 & dimWorld == 2
 */
template<class TypeTag>
class MpfaDimensionHelper<TypeTag, /*dim*/2, /*dimWorld*/2>
{
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using ScvfCornerVector = typename SubControlVolumeFace::Traits::CornerStorage;
    using InteractionVolume = typename GET_PROP_TYPE(TypeTag, PrimaryInteractionVolume);
    using ScvBasis = typename InteractionVolume::Traits::ScvBasis;

    //! We know that dim = 2 & dimworld = 2. However, the specialization for
    //! dim = 2 & dimWorld = 3 reuses some methods of this class, thus, we
    //! obtain the world dimension from the grid view here to get the
    //! GlobalPosition vector right. Be picky about the dimension though.
    static_assert(GridView::dimension == 2, "The chosen mpfa helper expects a grid view with dim = 2");
    static const int dim = 2;
    static const int dimWorld = GridView::dimensionworld;
    using GlobalPosition = Dune::FieldVector<Scalar, dimWorld>;

    //! Container to store the positions of intersections required for scvf
    //! corner computation. In 2d, these are the center plus the two corners
    using ScvfPositionsOnIntersection = std::array<GlobalPosition, 3>;

public:
    /*!
     * \brief Calculates the inner normal vectors to a given scv basis.
     *
     * \param scvBasis The basis of an scv
     */
    static ScvBasis calculateInnerNormals(const ScvBasis& scvBasis)
    {
        static const Dune::FieldMatrix<Scalar, dim, dim> R = {{0.0, 1.0}, {-1.0, 0.0}};

        ScvBasis innerNormals;
        R.mv(scvBasis[1], innerNormals[0]);
        R.mv(scvBasis[0], innerNormals[1]);

        // adjust sign depending on basis being a RHS
        if (!isRightHandSystem(scvBasis))
            innerNormals[0] *= -1.0;
        else
            innerNormals[1] *= -1.0;

        return innerNormals;
    }

    /*!
     * \brief Calculates the determinant of an scv basis.
     *        This is equal to the cross product for dim = dimWorld = 2
     *
     * \param scvBasis The basis of an scv
     */
    static Scalar calculateDetX(const ScvBasis& scvBasis)
    {
        using std::abs;
        return abs(crossProduct<Scalar>(scvBasis[0], scvBasis[1]));
    }

    // returns the global number of scvfs in the grid
    static std::size_t getGlobalNumScvf(const GridView& gridView)
    { return gridView.size(Dune::GeometryTypes::triangle)*6 + gridView.size(Dune::GeometryTypes::quadrilateral)*8; }

    /*!
     * \brief Checks whether or not a given scv basis forms a right hand system.
     *
     * \param scvBasis The basis of an scv
     */
    static bool isRightHandSystem(const ScvBasis& scvBasis)
    { return !std::signbit(crossProduct<Scalar>(scvBasis[0], scvBasis[1])); }

    /*!
     * \brief Returns a vector containing the positions on a given intersection
     *        that are relevant for scvf corner computation.
     *        Ordering -> 1: facet center, 2: the two facet corners
     *
     * \param eg Geometry of the element the facet is embedded in
     * \param refElement Reference element of the element the facet is embedded in
     * \param indexInInside The local index of the facet in the element
     * \param numCorners The number of corners on the facet
     */
    template<class ElementGeometry, class ReferenceElement>
    static ScvfPositionsOnIntersection computeScvfCornersOnIntersection(const ElementGeometry& eg,
                                                                        const ReferenceElement& refElement,
                                                                        unsigned int indexInElement,
                                                                        unsigned int numCorners)
    {
        ScvfPositionsOnIntersection p;

        // compute facet center and corners
        p[0] = 0.0;
        for (unsigned int c = 0; c < numCorners; ++c)
        {
            p[c+1] = eg.global(refElement.position(refElement.subEntity(indexInElement, 1, c, dim), dim));
            p[0] += p[c+1];
        }
        p[0] /= numCorners;

        return p;
    }

    /*!
     * \brief Returns the corners of the sub control volume face constructed
     *        in a corner (vertex) of an intersection.
     *
     * \param scvfCornerPositions Container with all scvf corners of the intersection
     * \param numIntersectionCorners Number of corners of the intersection (required in 3d)
     * \param cornerIdx Local vertex index on the intersection
     */
    static ScvfCornerVector getScvfCorners(const ScvfPositionsOnIntersection& p,
                                           unsigned int numIntersectionCorners,
                                           unsigned int cornerIdx)
    {
        // make sure the given input is admissible
        assert(cornerIdx < 2 && "provided index exceeds the number of corners of facets in 2d");

        // create & return the scvf corner vector
        if (cornerIdx == 0)
            return ScvfCornerVector({p[0], p[1]});
        else
            return ScvfCornerVector({p[0], p[2]});
    }

    /*!
     * \brief Calculates the area of an scvf.
     *
     * \param scvfCorners Container with the corners of the scvf
     */
    static Scalar getScvfArea(const ScvfCornerVector& scvfCorners)
    { return (scvfCorners[1]-scvfCorners[0]).two_norm(); }

    /*!
     * \brief Calculates the number of scvfs in a given element geometry type.
     *
     * \param gt The element geometry type
     */
    static std::size_t getNumLocalScvfs(const Dune::GeometryType gt)
    {
        if (gt == Dune::GeometryTypes::triangle)
            return 6;
        else if (gt == Dune::GeometryTypes::quadrilateral)
            return 8;
        else
            DUNE_THROW(Dune::NotImplemented, "Mpfa for 2d geometry type " << gt);
    }
};

/*!
 * \brief Specialization for dim == 2 & dimWorld == 2. Reuses some
 *        functionality of the specialization for dim = dimWorld = 2
 */
template<class TypeTag>
class MpfaDimensionHelper<TypeTag, /*dim*/2, /*dimWorld*/3> : public MpfaDimensionHelper<TypeTag, 2, 2>
{
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using InteractionVolume = typename GET_PROP_TYPE(TypeTag, PrimaryInteractionVolume);
    using ScvBasis = typename InteractionVolume::Traits::ScvBasis;

public:

    /*!
     * \brief Calculates the inner normal vectors to a given scv basis.
     *
     * \param scvBasis The basis of an scv
     */
    static ScvBasis calculateInnerNormals(const ScvBasis& scvBasis)
    {
        // compute vector normal to the basis plane
        const auto normal = [&] () {
                                        auto n = crossProduct<Scalar>(scvBasis[0], scvBasis[1]);
                                        n /= n.two_norm();
                                        return n;
                                    } ();

        // compute inner normals using the normal vector
        ScvBasis innerNormals;
        innerNormals[0] = crossProduct<Scalar>(scvBasis[1], normal);
        innerNormals[1] = crossProduct<Scalar>(normal, scvBasis[0]);

        return innerNormals;
    }

    /*!
     * \brief Calculates the determinant of an scv basis.
     *        For dim = 2 < dimWorld = 3 this is actually not the determinant of the
     *        basis but it is simply the area of the parallelofram spanned by the
     *        basis vectors.
     *
     * \param scvBasis The basis of an scv
     */
    static Scalar calculateDetX(const ScvBasis& scvBasis)
    {
        using std::abs;
        return abs(crossProduct<Scalar>(scvBasis[0], scvBasis[1]).two_norm());
    }

    /*!
     * \brief Checks whether or not a given scv basis forms a right hand system.
     *        Note that for dim = 2 < dimWorld = 3 the bases forming a right hand system
     *        are not unique.
     *
     * \param scvBasis The basis of an scv
     */
    static bool isRightHandSystem(const ScvBasis& scvBasis)
    { return true; }
};

/*!
 * \brief Specialization for dim == 3 & dimWorld == 3.
 */
template<class TypeTag>
class MpfaDimensionHelper<TypeTag, /*dim*/3, /*dimWorld*/3>
{
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using ScvfCornerVector = typename SubControlVolumeFace::Traits::CornerStorage;
    using InteractionVolume = typename GET_PROP_TYPE(TypeTag, PrimaryInteractionVolume);
    using ScvBasis = typename InteractionVolume::Traits::ScvBasis;

    // Be picky about the dimensions
    static_assert(GridView::dimension == 3 && GridView::dimensionworld == 3,
                  "The chosen mpfa helper expects a grid view with dim = 3 & dimWorld = 3");
    static const int dim = 3;
    static const int dimWorld = 3;
    using GlobalPosition = Dune::FieldVector<Scalar, dimWorld>;

    // container to store the positions of intersections required for
    // scvf corner computation. Maximum number of points needed is 9
    // for the supported geometry types (quadrilateral facet)
    using ScvfPositionsOnIntersection = std::array<GlobalPosition, 9>;

public:

    /*!
     * \brief Calculates the inner normal vectors to a given scv basis.
     *
     * \param scvBasis The basis of an scv
     */
    static ScvBasis calculateInnerNormals(const ScvBasis& scvBasis)
    {
        ScvBasis innerNormals;

        innerNormals[0] = crossProduct<Scalar>(scvBasis[1], scvBasis[2]);
        innerNormals[1] = crossProduct<Scalar>(scvBasis[2], scvBasis[0]);
        innerNormals[2] = crossProduct<Scalar>(scvBasis[0], scvBasis[1]);

        if (!isRightHandSystem(scvBasis))
        {
            innerNormals[0] *= -1.0;
            innerNormals[1] *= -1.0;
            innerNormals[2] *= -1.0;
        }

        return innerNormals;
    }

    /*!
     * \brief Calculates the determinant of an scv basis.
     *        This is equal to the cross product for dim = dimWorld = 2
     *
     * \param scvBasis The basis of an scv
     */
    static Scalar calculateDetX(const ScvBasis& scvBasis)
    {
        using std::abs;
        return abs(tripleProduct<Scalar>(scvBasis[0], scvBasis[1], scvBasis[2]));
    }

    /*!
     * \brief Returns the total number of scvfs in a given grid view. This number can be used to
     *        to resize e.g. geometry vectors during initialization. However, if generic element
     *        geometry types are used by the grid, this method should be overloaded as it currently
     *        only works for basic geometry types. Furthermore, on locally refined grids the computed
     *        value will be smaller than the actual number of scvfs, thus the name "estimate"
     *        TODO: return accurate value also for locally refined grids?
     *
     * \param gridView The grid view to be checked
     */
    static std::size_t estimateNumScvf(const GridView& gridView)
    {
        Dune::GeometryType simplex, pyramid, prism, cube;
        simplex.makeTetrahedron();
        pyramid.makePyramid();
        prism.makePrism();
        cube.makeHexahedron();

        // This assumes the grid to be entirely made up of triangles and quadrilaterals
        assert(gridView.size(simplex)
               + gridView.size(pyramid)
               + gridView.size(prism)
               + gridView.size(cube) == gridView.size(0)
               && "Current implementation of mpfa schemes only supports simplices, pyramids, prisms & cubes in 3d.");

        return gridView.size(simplex)*12 + gridView.size(pyramid)*16 + gridView.size(prism)*18 + gridView.size(cube)*24;
    }

    /*!
     * \brief Checks whether or not a given scv basis forms a right hand system.
     *
     * \param scvBasis The basis of an scv
     */
    static bool isRightHandSystem(const ScvBasis& scvBasis)
    { return !std::signbit(tripleProduct<Scalar>(scvBasis[0], scvBasis[1], scvBasis[2])); }

    /*!
     * \brief Returns a vector containing the positions on a given intersection
     *        that are relevant for scvf corner computation.
     *        Ordering -> 1: facet center, 2: facet corners, 3: edge centers
     *
     * \param eg Geometry of the element the facet is embedded in
     * \param refElement Reference element of the element the facet is embedded in
     * \param indexInInside The local index of the facet in the element
     * \param numCorners The number of corners on the facet
     */
    template<class ElementGeometry, class ReferenceElement>
    static ScvfPositionsOnIntersection computeScvfCornersOnIntersection(const ElementGeometry& eg,
                                                                        const ReferenceElement& refElement,
                                                                        unsigned int indexInElement,
                                                                        unsigned int numCorners)
    {
        ScvfPositionsOnIntersection p;

        // compute facet center and corners
        p[0] = 0.0;
        for (unsigned int c = 0; c < numCorners; ++c)
        {
            p[c+1] = eg.global(refElement.position(refElement.subEntity(indexInElement, 1, c, dim), dim));
            p[0] += p[c+1];
        }
        p[0] /= numCorners;

        // proceed according to number of corners
        switch (numCorners)
        {
            case 3: // triangle
            {
                // add edge midpoints, there are 3 for triangles
                p[numCorners+1] = p[2] + p[1];
                p[numCorners+1] /= 2;
                p[numCorners+2] = p[3] + p[1];
                p[numCorners+2] /= 2;
                p[numCorners+3] = p[3] + p[2];
                p[numCorners+3] /= 2;
            }
            case 4: // quadrilateral
            {
                // add edge midpoints, there are 4 for quadrilaterals
                p[numCorners+1] = p[3] + p[1];
                p[numCorners+1] /= 2;
                p[numCorners+2] = p[4] + p[2];
                p[numCorners+2] /= 2;
                p[numCorners+3] = p[2] + p[1];
                p[numCorners+3] /= 2;
                p[numCorners+4] = p[4] + p[3];
                p[numCorners+4] /= 2;
            }
            default:
                DUNE_THROW(Dune::NotImplemented, "Mpfa scvf corners for dim = " << dim
                                                              << ", dimWorld = " << dimWorld
                                                              << ", corners = " << numCorners);
        }

        return p;
    }

    /*!
     * \brief Returns the corners of the sub control volume face constructed
     *        in a corner (vertex) of an intersection.
     *
     * \param scvfCornerPositions Container with all scvf corners of the intersection
     * \param numIntersectionCorners Number of corners of the intersection
     * \param cornerIdx Local vertex index on the intersection
     */
    static ScvfCornerVector getScvfCorners(const ScvfPositionsOnIntersection& p,
                                           unsigned int numIntersectionCorners,
                                           unsigned int cornerIdx)
    {
        // proceed according to number of corners
        // we assume the ordering according to the above method computeScvfCornersOnIntersection()
        switch (numIntersectionCorners)
        {
            case 3: // triangle
            {
                //! Only build the maps the first time we encounter a triangle
                static const std::uint8_t vo = 1; //! vertex offset in point vector p
                static const std::uint8_t eo = 4; //! edge offset in point vector p
                static const std::uint8_t map[3][4] =
                {
                    {0, eo+1, eo+0, vo+0},
                    {0, eo+0, eo+2, vo+1},
                    {0, eo+2, eo+1, vo+2}
                };

                return ScvfCornerVector( {p[map[cornerIdx][0]],
                                          p[map[cornerIdx][1]],
                                          p[map[cornerIdx][2]],
                                          p[map[cornerIdx][3]]} );
            }
            case 4: // quadrilateral
            {
                //! Only build the maps the first time we encounter a quadrilateral
                static const std::uint8_t vo = 1; //! vertex offset in point vector p
                static const std::uint8_t eo = 5; //! face offset in point vector p
                static const std::uint8_t map[4][4] =
                {
                    {0, eo+0, eo+2, vo+0},
                    {0, eo+2, eo+1, vo+1},
                    {0, eo+3, eo+0, vo+2},
                    {0, eo+1, eo+3, vo+3}
                };

                return ScvfCornerVector( {p[map[cornerIdx][0]],
                                          p[map[cornerIdx][1]],
                                          p[map[cornerIdx][2]],
                                          p[map[cornerIdx][3]]} );
            }
            default:
                DUNE_THROW(Dune::NotImplemented, "Mpfa scvf corners for dim = " << dim
                                                              << ", dimWorld = " << dimWorld
                                                              << ", corners = " << numIntersectionCorners);
        }
    }

    /*!
     * \brief Calculates the area of an scvf.
     *
     * \param scvfCorners Container with the corners of the scvf
     */
    static Scalar getScvfArea(const ScvfCornerVector& scvfCorners)
    {
        // after Wolfram alpha quadrilateral area
        return 0.5*Dumux::crossProduct(scvfCorners[3]-scvfCorners[0], scvfCorners[2]-scvfCorners[1]).two_norm();
    }

    /*!
     * \brief Calculates the number of scvfs in a given element geometry type.
     *
     * \param gt The element geometry type
     */
    static std::size_t getNumLocalScvfs(const Dune::GeometryType gt)
    {
        if (gt == Dune::GeometryType(Dune::GeometryType::simplex, 3))
            return 12;
        else if (gt == Dune::GeometryType(Dune::GeometryType::pyramid, 3))
            return 16;
        else if (gt == Dune::GeometryType(Dune::GeometryType::prism, 3))
            return 18;
        else if (gt == Dune::GeometryType(Dune::GeometryType::cube, 3))
            return 24;
        else
            DUNE_THROW(Dune::NotImplemented, "Mpfa for 3d geometry type " << gt);
    }
};

/*!
 * \ingroup Mpfa
 * \brief Helper class to get the required information on an interaction volume.
 *        Specializations depending on the method and dimension are provided.
 */
template<class TypeTag, MpfaMethods Method, int dim, int dimWorld>
class CCMpfaHelperImplementation : public MpfaDimensionHelper<TypeTag, dim, dimWorld>,
                                   public MpfaMethodHelper<TypeTag, Method, dim, dimWorld>
{
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);

    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Element = typename GridView::template Codim<0>::Entity;
    using IndexType = typename GridView::IndexSet::IndexType;

    using VertexMapper = typename GET_PROP_TYPE(TypeTag, VertexMapper);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using ScvfCornerVector = typename SubControlVolumeFace::Traits::CornerStorage;
    using InteractionVolume = typename GET_PROP_TYPE(TypeTag, PrimaryInteractionVolume);
    using LocalIndexType = typename InteractionVolume::Traits::LocalIndexType;
    using ScvBasis = typename InteractionVolume::Traits::ScvBasis;

    using GlobalPosition = Dune::FieldVector<Scalar, dimWorld>;
    using DimWorldMatrix = Dune::FieldMatrix<Scalar, dimWorld, dimWorld>;
    using CoordScalar = typename GridView::ctype;
    using ReferenceElements = typename Dune::ReferenceElements<CoordScalar, dim>;

public:
    /*!
     * \brief Calculates the integration point on an scvf.
     *
     * \param scvfCorners Container with the corners of the scvf
     * \param q Parameterization of the integration point on the scvf
     */
    static GlobalPosition getScvfIntegrationPoint(const ScvfCornerVector& scvfCorners, Scalar q)
    {
        // scvfs in 3d are always quadrilaterals
        // ordering -> first corner: facet center, last corner: vertex
        if (q == 0.0) return scvfCorners[0];
        const auto d = [&] () { auto tmp = scvfCorners.back() - scvfCorners.front(); tmp *= q; return tmp; } ();
        return scvfCorners[0] + d;
    }

    // returns a vector which maps true to each vertex on processor boundaries and false otherwise
    // TODO: Does this really NOT work with ghosts? Adjust to make it work with ghosts or rename the function!
    static std::vector<bool> findGhostVertices(const GridView& gridView, const VertexMapper& vertexMapper)
    {
        std::vector<bool> ghostVertices(gridView.size(dim), false);

        // if not run in parallel, skip the rest
        if (Dune::MPIHelper::getCollectiveCommunication().size() == 1)
            return ghostVertices;

        // mpfa methods can not yet handle ghost cells
        if (gridView.ghostSize(0) > 0)
            DUNE_THROW(Dune::InvalidStateException, "Mpfa methods in parallel do not work with ghost cells. Use overlap cells instead.");

        // mpfa methods have to have overlapping cells
        if (gridView.overlapSize(0) == 0)
            DUNE_THROW(Dune::InvalidStateException, "Grid no overlaping cells. This is required by mpfa methods in parallel.");

        for (const auto& element : elements(gridView))
        {
            for (const auto& is : intersections(gridView, element))
            {
                if (!is.neighbor() && !is.boundary())
                {
                    const auto& refElement = ReferenceElements::general(element.geometry().type());
                    for (int isVertex = 0; isVertex < is.geometry().corners(); ++isVertex)
                    {
                        const auto vIdxLocal = refElement.subEntity(is.indexInInside(), 1, isVertex, dim);
                        const auto vIdxGlobal = vertexMapper.subIndex(element, vIdxLocal, dim);

                        ghostVertices[vIdxGlobal] = true;
                    }
                }
            }
        }

        return ghostVertices;
    }

    //! returns whether or not a value exists in a vector
    template<typename V1, typename V2>
    static bool vectorContainsValue(const std::vector<V1>& vector, const V2 value)
    { return std::find(vector.begin(), vector.end(), value) != vector.end(); }

    // calculates the product of a transposed vector n, a Matrix M and another vector v (n^T M v)
    static Scalar nT_M_v(const GlobalPosition& n, const DimWorldMatrix& M, const GlobalPosition& v)
    {
        GlobalPosition tmp;
        M.mv(v, tmp);
        return n*tmp;
    }

    // calculates the product of a transposed vector n, a Scalar M and another vector v (n^T M v)
    static Scalar nT_M_v(const GlobalPosition& n, const Scalar M, const GlobalPosition& v)
    {
        return M*(n*v);
    }
};

/*!
 * \ingroup Mpfa
 * \brief Helper class for the mpfa methods for the construction of the interaction regions etc.
 *        It inherits from dimension-, dimensionworld- and method-specific implementations.
 */
template<class TypeTag>
using CCMpfaHelper = CCMpfaHelperImplementation<TypeTag,
                                                GET_PROP_VALUE(TypeTag, MpfaMethod),
                                                GET_PROP_TYPE(TypeTag, GridView)::dimension,
                                                GET_PROP_TYPE(TypeTag, GridView)::dimensionworld>;

} // end namespace

// The implemented helper classes need to be included here
// #include <dumux/discretization/cellcentered/mpfa/lmethod/helper.hh>
#include <dumux/discretization/cellcentered/mpfa/omethod/helper.hh>
// #include <dumux/discretization/cellcentered/mpfa/omethodfps/helper.hh>

#endif
