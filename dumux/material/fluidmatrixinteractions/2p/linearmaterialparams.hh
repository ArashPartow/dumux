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
 * \brief   Parameters for the linear capillary pressure and
 *          relative permeability <-> saturation relations
 */
#ifndef LINEAR_MATERIAL_PARAMS_HH
#define LINEAR_MATERIAL_PARAMS_HH

namespace Dumux
{
/*!
 * \brief Reference implementation of params for the linear material
 *        law.
 *
 *        \ingroup fluidmatrixinteractionsparams
 */
template<class ScalarT>
class LinearMaterialParams
{
public:
    typedef ScalarT Scalar;

    LinearMaterialParams()
    {}

    LinearMaterialParams(Scalar entryPc, Scalar maxPc)
    {
        setEntryPc(entryPc);
        setMaxPc(maxPc);
    };


    /*!
     * \brief Return the entry pressure for the linear material law.
     *
     * The entry pressure is reached at \f$\overline S_w = 1\f$
     */
    Scalar entryPc() const
    { return entryPc_; }

    DUNE_DEPRECATED_MSG("use entryPc() (uncapitalized 'c') instead")
    Scalar entryPC()
    {
        return entryPc();
    }

    /*!
     * \brief Set the entry pressure for the linear material law.
     *
     * The entry pressure is reached at \f$ \overline S_w = 1\f$
     */
    void setEntryPc(Scalar v)
    { entryPc_ = v; }

    DUNE_DEPRECATED_MSG("use setEntryPc() (uncapitalized 'c') instead")
    Scalar setEntryPC(Scalar v)
    {
        return setEntryPc(v);
    }

    /*!
     * \brief Return the maximum capillary pressure for the linear material law.
     *
     * The maximum capillary pressure is reached at \f$ \overline S_w = 0\f$
     */
    Scalar maxPc() const
    { return maxPc_; }

    DUNE_DEPRECATED_MSG("use maxPc() (uncapitalized 'c') instead")
    Scalar maxPC()
    {
        return maxPc();
    }

    /*!
     * \brief Set the maximum capillary pressure for the linear material law.
     *
     * The maximum capillary pressure is reached at \f$ \overline S_w = 0\f$
     */
    void setMaxPc(Scalar v)
    { maxPc_ = v; }

    DUNE_DEPRECATED_MSG("use setMaxPc() (uncapitalized 'c') instead")
    Scalar setMaxPC(Scalar v)
    {
        return setMaxPc(v);
    }


private:
    Scalar entryPc_;
    Scalar maxPc_;
};
} // namespace Dumux

#endif
