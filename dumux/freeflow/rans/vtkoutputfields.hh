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
 * \ingroup RANSModel
 * \copydoc Dumux::RANSVtkOutputFields
 */
#ifndef DUMUX_RANS_VTK_OUTPUT_FIELDS_HH
#define DUMUX_RANS_VTK_OUTPUT_FIELDS_HH

#include <dumux/freeflow/navierstokes/vtkoutputfields.hh>

namespace Dumux
{

/*!
 * \ingroup RANSModel
 * \brief Adds vtk output fields for the Reynolds-Averaged Navier-Stokes model
 */
template<class FVGridGeometry>
class RANSVtkOutputFields : public NavierStokesVtkOutputFields<FVGridGeometry>
{
    enum { dim = FVGridGeometry::GridView::dimension };

public:
    //! Initialize the Navier-Stokes specific vtk output fields.
    template <class VtkOutputModule>
    static void init(VtkOutputModule& vtk)
    {
        NavierStokesVtkOutputFields<FVGridGeometry>::init(vtk);

        vtk.addVolumeVariable([](const auto& v){ return v.velocity()[0] / v.velocityMaximum()[0]; }, "v_x/v_x,max");
        vtk.addVolumeVariable([](const auto& v){ return v.velocityGradients()[0]; }, "dv_x/dx_");
        if (dim > 1)
            vtk.addVolumeVariable([](const auto& v){ return v.velocityGradients()[1]; }, "dv_y/dx_");
        if (dim > 2)
            vtk.addVolumeVariable([](const auto& v){ return v.velocityGradients()[2]; }, "dv_z/dx_");
        vtk.addVolumeVariable([](const auto& v){ return v.pressure() - 1e5; }, "p_rel");
        vtk.addVolumeVariable([](const auto& v){ return v.density(); }, "rho");
        vtk.addVolumeVariable([](const auto& v){ return v.viscosity() / v.density(); }, "nu");
        vtk.addVolumeVariable([](const auto& v){ return v.dynamicEddyViscosity() / v.density(); }, "nu_t");
        vtk.addVolumeVariable([](const auto& v){ return v.wallDistance(); }, "l_w");
        vtk.addVolumeVariable([](const auto& v){ return v.yPlus(); }, "y^+");
        vtk.addVolumeVariable([](const auto& v){ return v.uPlus(); }, "u^+");
    }
};

} // end namespace Dumux

#endif
