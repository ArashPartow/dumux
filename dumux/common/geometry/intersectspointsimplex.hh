/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 3 of the License, or       *
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
 * \brief Detect if a point intersects a simplex (including boundary)
 */
#ifndef DUMUX_INTERSECTS_POINT_SIMPLEX_HH
#define DUMUX_INTERSECTS_POINT_SIMPLEX_HH

#include <cmath>
#include <dune/common/fvector.hh>
#include <dumux/common/math.hh>

namespace Dumux {

//! Find out whether a point is inside a tetrahedron (p0, p1, p2, p3) (dimworld is 3)
template<class ctype, int dimworld, typename std::enable_if_t<(dimworld == 3), int> = 0>
bool intersectsPointSimplex(const Dune::FieldVector<ctype, dimworld>& point,
                            const Dune::FieldVector<ctype, dimworld>& p0,
                            const Dune::FieldVector<ctype, dimworld>& p1,
                            const Dune::FieldVector<ctype, dimworld>& p2,
                            const Dune::FieldVector<ctype, dimworld>& p3)
{
    // Algorithm from http://www.blackpawn.com/texts/pointinpoly/
    // See also "Real-Time Collision Detection" by Christer Ericson.
    using GlobalPosition = Dune::FieldVector<ctype, dimworld>;
    static constexpr ctype eps_ = 1.0e-7;

    // put the tetrahedron points in an array
    const GlobalPosition *p[4] = {&p0, &p1, &p2, &p3};

    // iterate over all faces
    for (int i = 0; i < 4; ++i)
    {
        // compute all the vectors from vertex (local index 0) to the other points
        const GlobalPosition v1 = *p[(i + 1)%4] - *p[i];
        const GlobalPosition v2 = *p[(i + 2)%4] - *p[i];
        const GlobalPosition v3 = *p[(i + 3)%4] - *p[i];
        const GlobalPosition v = point - *p[i];
        // compute the normal to the facet (cross product)
        GlobalPosition n1 = crossProduct(v1, v2);
        n1 /= n1.two_norm();
        // find out on which side of the plane v and v3 are
        const auto t1 = n1.dot(v);
        const auto t2 = n1.dot(v3);
        // If the point is not exactly on the plane the
        // points have to be on the same side
        const auto eps = eps_ * v1.two_norm();
        if ((t1 > eps || t1 < -eps) && std::signbit(t1) != std::signbit(t2))
            return false;
    }
    return true;
}

//! Find out whether a point is inside a triangle (p0, p1, p2) (dimworld is 3)
template<class ctype, int dimworld, typename std::enable_if_t<(dimworld == 3), int> = 0>
bool intersectsPointSimplex(const Dune::FieldVector<ctype, dimworld>& point,
                            const Dune::FieldVector<ctype, dimworld>& p0,
                            const Dune::FieldVector<ctype, dimworld>& p1,
                            const Dune::FieldVector<ctype, dimworld>& p2)
{
    // Algorithm from http://www.blackpawn.com/texts/pointinpoly/
    // See also "Real-Time Collision Detection" by Christer Ericson.
    using GlobalPosition = Dune::FieldVector<ctype, dimworld>;
    static constexpr ctype eps_ = 1.0e-7;

    // compute the vectors of the edges and the vector point-p0
    const GlobalPosition v1 = p0 - p2;
    const GlobalPosition v2 = p1 - p0;
    const GlobalPosition v3 = p2 - p1;
    const GlobalPosition v = point - p0;

    // compute the normal of the triangle
    const GlobalPosition n = crossProduct(v1, v2);

    // first check if we are in the plane of the triangle
    // if not we can return early
    const double t = v.dot(n);
    using std::abs;
    if (abs(t) > v1.two_norm()*eps_) // take |v1| as scale
        return false;

    // compute the normal to the triangle made of point and first edge
    // the dot product of this normal and the triangle normal has to
    // be positive because we defined the edges in the right orientation
    const GlobalPosition n1 = crossProduct(v, v1);
    const double t1 = n.dot(n1);
    if (t1 < 0) return false;

    const GlobalPosition n2 = crossProduct(v, v2);
    const double t2 = n.dot(n2);
    if (t2 < 0) return false;

    const GlobalPosition n3 = crossProduct(v, v3);
    const double t3 = n.dot(n3);
    if (t3 < 0) return false;

    return true;
}

//! Find out whether a point is inside a triangle (p0, p1, p2) (dimworld is 2)
template<class ctype, int dimworld, typename std::enable_if_t<(dimworld == 2), int> = 0>
bool intersectsPointSimplex(const Dune::FieldVector<ctype, dimworld>& point,
                            const Dune::FieldVector<ctype, dimworld>& p0,
                            const Dune::FieldVector<ctype, dimworld>& p1,
                            const Dune::FieldVector<ctype, dimworld>& p2)
{
    static constexpr ctype eps_ = 1.0e-7;

    // Use barycentric coordinates
    const ctype A = 0.5*(-p1[1]*p2[0] + p0[1]*(p2[0] - p1[0])
                         +p1[0]*p2[1] + p0[0]*(p1[1] - p2[1]));
    const ctype sign = std::copysign(1.0, A);
    const ctype s = sign*(p0[1]*p2[0] + point[0]*(p2[1]-p0[1])
                         -p0[0]*p2[1] + point[1]*(p0[0]-p2[0]));
    const ctype t = sign*(p0[0]*p1[1] + point[0]*(p0[1]-p1[1])
                         -p0[1]*p1[0] + point[1]*(p1[0]-p0[0]));
    const ctype eps = sign*A*eps_;

    return (s > -eps
            && t > -eps
            && (s + t) < 2*A*sign + eps);
}

//! Find out whether a point is inside an interval (p0, p1) (dimworld is 3)
template<class ctype, int dimworld, typename std::enable_if_t<(dimworld == 3), int> = 0>
bool intersectsPointSimplex(const Dune::FieldVector<ctype, dimworld>& point,
                            const Dune::FieldVector<ctype, dimworld>& p0,
                            const Dune::FieldVector<ctype, dimworld>& p1)
{
    using GlobalPosition = Dune::FieldVector<ctype, dimworld>;
    static constexpr ctype eps_ = 1.0e-7;

    // compute the vectors between p0 and the other points
    const GlobalPosition v1 = p1 - p0;
    const GlobalPosition v2 = point - p0;

    // check if point and p0 are the same
    const ctype v1norm = v1.two_norm();
    const ctype v2norm = v2.two_norm();
    if (v2norm < v1norm*eps_)
        return true;

    // if not check if p0 and p1 are the same
    // then we know that point is not in the interval
    if (v1norm < eps_)
        return false;

    // if the cross product is zero the points are on a line
    const GlobalPosition n = crossProduct(v1, v2);

    // early return if the vector length is larger than zero
    if (n.two_norm() > v1norm*eps_)
        return false;

    // we know the points are aligned
    // if the dot product is positive and the length in range
    // the point is in the interval
    return (v1.dot(v2) > 0.0 && v2norm < v1norm*(1 + eps_));
}

//! Find out whether a point is inside an interval (p0, p1) (dimworld is 2)
template<class ctype, int dimworld, typename std::enable_if_t<(dimworld == 2), int> = 0>
bool intersectsPointSimplex(const Dune::FieldVector<ctype, dimworld>& point,
                            const Dune::FieldVector<ctype, dimworld>& p0,
                            const Dune::FieldVector<ctype, dimworld>& p1)
{
    using GlobalPosition = Dune::FieldVector<ctype, dimworld>;
    static constexpr ctype eps_ = 1.0e-7;

    // compute the vectors between p0 and the other points
    const GlobalPosition v1 = p1 - p0;
    const GlobalPosition v2 = point - p0;

    // check if point and p0 are the same
    const ctype v1norm = v1.two_norm();
    const ctype v2norm = v2.two_norm();
    if (v2norm < v1norm*eps_)
        return true;

    // if not check if p0 and p1 are the same
    // then we know that point is not in the interval
    if (v1norm < eps_)
        return false;

    // if the cross product is zero the points are on a line
    const ctype n = crossProduct(v1, v2);

    // early return if the cross product is larger than zero
    using std::abs;
    if (abs(n) > v1norm*eps_)
        return false;

    // we know the points are aligned
    // if the dot product is positive and the length in range
    // the point is in the interval
    return (v1.dot(v2) > 0.0 && v2norm < v1norm*(1 + eps_));
}

//! Find out whether a point is inside an interval (p0, p1) (dimworld is 1)
template<class ctype, int dimworld, typename std::enable_if_t<(dimworld == 1), int> = 0>
bool intersectsPointSimplex(const Dune::FieldVector<ctype, dimworld>& point,
                            const Dune::FieldVector<ctype, dimworld>& p0,
                            const Dune::FieldVector<ctype, dimworld>& p1)
{
    static constexpr ctype eps_ = 1.0e-7;

    // sort the interval so interval[1] is the end and interval[0] the start
    const ctype *interval[2] = {&p0[0], &p1[0]};
    if (*interval[0] > *interval[1])
        std::swap(interval[0], interval[1]);

    const ctype v1 = point[0] - *interval[0];
    const ctype v2 = *interval[1] - *interval[0]; // always positive

    // the coordinates are the same
    using std::abs;
    if (abs(v1) < v2*eps_)
        return true;

    // the point doesn't coincide with p0
    // so if p0 and p1 are equal it's not inside
    if (v2 < 1.0e-30)
        return false;

    // the point is inside if the length is
    // smaller than the interval length and the
    // sign of v1 & v2 are the same
    using std::signbit;
    return (!signbit(v1) && abs(v1) < v2*(1.0 + eps_));
}

} // end namespace Dumux

#endif