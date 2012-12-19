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
 * \brief Base class for models using box discretization
 */
#ifndef DUMUX_BOX_MODEL_HH
#define DUMUX_BOX_MODEL_HH

#include <dumux/implicit/common/implicitmodel.hh>

namespace Dumux
{

/*!
 * \ingroup BoxModel
 *
 * \brief The base class for the vertex centered finite volume
 *        discretization scheme.
 */
template<class TypeTag>
class BoxModel : public ImplicitModel<TypeTag>
{
    typedef ImplicitModel<TypeTag> ParentType;

    // copying a model is not a good idea
    BoxModel(const BoxModel &);

public:
    /*!
     * \brief The constructor.
     */
    DUNE_DEPRECATED_MSG("Use ImplicitModel from dumux/implicit/common/implicitmodel.hh instead.")
    BoxModel() : ParentType()
    {}
};
}

#endif
