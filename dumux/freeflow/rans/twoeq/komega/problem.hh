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
 * \ingroup KOmega TwoEq Model
 * \copydoc Dumux::KomegaProblem
 */
#ifndef DUMUX_KOMEGA_PROBLEM_HH
#define DUMUX_KOMEGA_PROBLEM_HH

#include <dumux/common/properties.hh>
#include <dumux/common/staggeredfvproblem.hh>
#include <dumux/discretization/localview.hh>
#include <dumux/discretization/staggered/elementsolution.hh>
#include <dumux/discretization/methods.hh>
#include <dumux/freeflow/rans/problem.hh>

#include "model.hh"
#include "models.hh"

namespace Dumux
{

/*!
 * \ingroup KOmega TwoEq Model
 * \brief K-Omega turbulence model problem base class.
 *
 * This implements the 2-equation k-omega turbulence model developed in Wilcox08 and Wilcox88
 */
template<class TypeTag>
class KOmegaProblem : public RANSProblem<TypeTag>
{
    using ParentType = RANSProblem<TypeTag>;
    using Implementation = typename GET_PROP_TYPE(TypeTag, Problem);

    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);

    using FVGridGeometry = typename GET_PROP_TYPE(TypeTag, FVGridGeometry);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVGridGeometry)::LocalView;
    using SubControlVolumeFace = typename FVElementGeometry::SubControlVolumeFace;

    using VolumeVariables = typename GET_PROP_TYPE(TypeTag, VolumeVariables);
    using SolutionVector = typename GET_PROP_TYPE(TypeTag, SolutionVector);
    using PrimaryVariables = typename GET_PROP_TYPE(TypeTag, PrimaryVariables);
    using CellCenterPrimaryVariables = typename GET_PROP_TYPE(TypeTag, CellCenterPrimaryVariables);
    using Indices = typename GET_PROP_TYPE(TypeTag, ModelTraits)::Indices;

    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Grid = typename GridView::Grid;
    enum {
        dim = Grid::dimension,
      };
    using GlobalPosition = Dune::FieldVector<Scalar, dim>;
    using DimVector = Dune::FieldVector<Scalar, dim>;
    using DimMatrix = Dune::FieldMatrix<Scalar, dim, dim>;

public:
    KOmegaProblem(std::shared_ptr<const FVGridGeometry> fvGridGeometry) : ParentType(fvGridGeometry)
    {
        kOmegaModel_ = getParamFromGroup<int>(GET_PROP_VALUE(TypeTag, ModelParameterGroup), "KOmega.Model", 0);
        if (kOmegaModel_ != KOmegaModels::wilcox08
            && kOmegaModel_ != KOmegaModels::wilcox88)
        {
            DUNE_THROW(Dune::NotImplemented, "This k-omega model is not implemented: " << kOmegaModel_);
        }
        useStoredEddyViscosity_ = getParamFromGroup<bool>(GET_PROP_VALUE(TypeTag, ModelParameterGroup), "RANS.UseStoredEddyViscosity", true);
    }

    /*!
     * \brief Correct size of the static (solution independent) wall variables
     */
    void updateStaticWallProperties()
    {
        ParentType::updateStaticWallProperties();

        // update size and initial values of the global vectors
        storedDissipation_.resize(this->fvGridGeometry().elementMapper().size(), 0.0);
        storedDissipationGradient_.resize(this->fvGridGeometry().elementMapper().size(), DimMatrix(0.0));
        storedDynamicEddyViscosity_.resize(this->fvGridGeometry().elementMapper().size(), 0.0);
        storedTurbulentKineticEnergy_.resize(this->fvGridGeometry().elementMapper().size(), 0.0);
        storedTurbulentKineticEnergyGradient_.resize(this->fvGridGeometry().elementMapper().size(), 0.0);
    }

    /*!
     * \brief Update the dynamic (solution dependent) relations to the walls
     *
     * \param curSol The solution vector.
     */
    void updateDynamicWallProperties(const SolutionVector& curSol)
    {
        ParentType::updateDynamicWallProperties(curSol);

        for (const auto& element : elements(this->fvGridGeometry().gridView()))
        {
            unsigned int elementID = this->fvGridGeometry().elementMapper().index(element);

            auto fvGeometry = localView(this->fvGridGeometry());
            fvGeometry.bindElement(element);
            for (auto&& scv : scvs(fvGeometry))
            {
                const int dofIdx = scv.dofIndex();
                const auto& cellCenterPriVars = curSol[FVGridGeometry::cellCenterIdx()][dofIdx];
                PrimaryVariables priVars = makePriVarsFromCellCenterPriVars<PrimaryVariables>(cellCenterPriVars);
                auto elemSol = elementSolution<typename FVGridGeometry::LocalView>(std::move(priVars));
                // NOTE: first update the turbulence quantities
                storedDissipation_[elementID] = elemSol[0][Indices::dissipationEqIdx];
                storedTurbulentKineticEnergy_[elementID] = elemSol[0][Indices::turbulentKineticEnergyEqIdx];
                // NOTE: then update the volVars
                VolumeVariables volVars;
                volVars.update(elemSol, asImp_(), element, scv);
                volVars.calculateEddyViscosity();
                storedKinematicEddyViscosity_[elementID] = volVars.kinematicEddyViscosity();
            }
        }
    }

    //! \brief Returns the \$f \beta_{\omega} \$f constant
    const Scalar betaOmega() const
    {
          if(kOmegaModel_ == KOmegaModels::wilcox88)
              return 0.0750;
          else if (kOmegaModel_ == KOmegaModels::wilcox08)
              return 0.0708;
          else
              DUNE_THROW(Dune::NotImplemented, "This Model is not implemented.");
    }

    /*!
     * \brief Returns the index of the k-omega model applied
     */
    const int kOmegaModel() const
    { return kOmegaModel_; }

public:
    std::vector<Scalar> storedDissipation_;
    std::vector<Scalar> storedDynamicEddyViscosity_;
    std::vector<Scalar> storedTurbulentKineticEnergy_;
    std::vector<DimMatrix> storedDissipationGradient_;
    std::vector<DimMatrix> storedTurbulentKineticEnergyGradient_;
    int kOmegaModel_;
    bool useStoredEddyViscosity_;

private:
    //! Returns the implementation of the problem (i.e. static polymorphism)
    Implementation &asImp_()
    { return *static_cast<Implementation *>(this); }

    //! \copydoc asImp_()
    const Implementation &asImp_() const
    { return *static_cast<const Implementation *>(this); }
};

}

#endif
