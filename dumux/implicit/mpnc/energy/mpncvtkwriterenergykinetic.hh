/*****************************************************************************
 *   Copyright (C) 2011 by Philipp Nuske                                     *
 *   Institute for Modelling Hydraulic and Environmental Systems             *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@iws.uni-stuttgart.de                          *
 *                                                                           *
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
/*!
 * \file
 *
 * \brief VTK writer module for the energy related quantities of the
 *        MpNc model.
 */
#ifndef DUMUX_MPNC_VTK_WRITER_ENERGY_KINETIC_HH
#define DUMUX_MPNC_VTK_WRITER_ENERGY_KINETIC_HH

#include <dumux/implicit/mpnc/mpncvtkwritermodule.hh>
#include "mpncvtkwriterenergy.hh"

namespace Dumux
{
/*!
 * \ingroup MPNCModel
 *
 * \brief VTK writer module for the energy related quantities of the
 *        MpNc model.
 *
 * This is the specialization for the case with an energy equation and
 * no local thermal equilibrium. (i.e. _including_ kinetic energy transfer)
 */
template<class TypeTag>
class MPNCVtkWriterEnergy<TypeTag, /*enableEnergy = */ true, /* enableKineticEnergy = */ true >
    : public MPNCVtkWriterModule<TypeTag>
{
    typedef MPNCVtkWriterModule<TypeTag> ParentType;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables) ElementVolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, ElementBoundaryTypes) ElementBoundaryTypes;
    typedef typename GET_PROP_TYPE(TypeTag, VolumeVariables) VolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GridView::template Codim<0>::Entity Element;

    enum { dim = GridView::dimension };
    enum { numPhases = GET_PROP_VALUE(TypeTag, NumPhases) };
    enum { numEnergyEqs = Indices::numPrimaryEnergyVars};
    enum { velocityAveragingInModel = GET_PROP_VALUE(TypeTag, VelocityAveragingInModel) };

    enum { temperatureOutput    = GET_PROP_VALUE(TypeTag, VtkAddTemperatures) };
    enum { enthalpyOutput       = GET_PROP_VALUE(TypeTag, VtkAddEnthalpies) };
    enum { internalEnergyOutput = GET_PROP_VALUE(TypeTag, VtkAddInternalEnergies) };
    enum { reynoldsOutput       = GET_PROP_VALUE(TypeTag, VtkAddReynolds) };
    enum { prandtlOutput        = GET_PROP_VALUE(TypeTag, VtkAddPrandtl) };
    enum { nusseltOutput        = GET_PROP_VALUE(TypeTag, VtkAddNusselt) };
    enum { interfacialAreaOutput= GET_PROP_VALUE(TypeTag, VtkAddInterfacialArea) };
    enum { velocityOutput       = GET_PROP_VALUE(TypeTag, VtkAddVelocities) };

    typedef typename ParentType::ScalarVector ScalarVector;
    typedef typename ParentType::PhaseVector PhaseVector;
    typedef typename ParentType::ComponentVector ComponentVector;
    typedef std::array<ScalarVector, numEnergyEqs> EnergyEqVector;

    typedef Dune::FieldVector<Scalar, dim> DimVector;
    typedef Dune::BlockVector<DimVector> DimField;
    typedef std::array<DimField, numPhases> PhaseDimField;


public:
    MPNCVtkWriterEnergy(const Problem &problem)
        : ParentType(problem)
    {
    }

    /*!
     * \brief Allocate memory for the scalar fields we would like to
     *        write to the VTK file.
     */
    template <class MultiWriter>
    void allocBuffers(MultiWriter &writer)
    {
        resizeTemperaturesBuffer_(temperature_);
        this->resizeScalarBuffer_(TwMinusTn_);
        this->resizeScalarBuffer_(TnMinusTs_);
        this->resizeScalarBuffer_(awn_);
        this->resizeScalarBuffer_(aws_);
        this->resizeScalarBuffer_(ans_);
        this->resizePhaseBuffer_(enthalpy_);
        this->resizePhaseBuffer_(internalEnergy_);
        this->resizePhaseBuffer_(reynoldsNumber_);
        this->resizePhaseBuffer_(prandtlNumber_);
        this->resizePhaseBuffer_(nusseltNumber_);


        if (velocityAveragingInModel and not velocityOutput/*only one of the two output options, otherwise paraview segfaults due to two times the same field name*/) {
            Scalar numVertices = this->problem_.gridView().size(dim);
            for (int phaseIdx = 0; phaseIdx < numPhases; ++ phaseIdx) {
                velocity_[phaseIdx].resize(numVertices);
                velocity_[phaseIdx] = 0;
            }
        }
    }

    /*!
     * \brief Modify the internal buffers according to the volume
     *        variables seen on an element
     *
     *        \param element The finite element
     *        \param fvGeometry The finite-volume geometry in the fully implicit scheme
     *        \param elemVolVars The volume variables of the current element
     *        \param elemBcTypes
     */
    void processElement(const Element & element,
                        const FVElementGeometry & fvGeometry,
                        const ElementVolumeVariables & elemVolVars,
                        const ElementBoundaryTypes & elemBcTypes)
    {
        int numLocalVertices = element.geometry().corners();
        for (int localVertexIdx = 0; localVertexIdx < numLocalVertices; ++localVertexIdx) {
            const unsigned int globalIdx = this->problem_.vertexMapper().map(element, localVertexIdx, dim);
            const VolumeVariables &volVars = elemVolVars[localVertexIdx];

            for (int phaseIdx = 0; phaseIdx < numPhases; ++ phaseIdx) {
                enthalpy_[phaseIdx][globalIdx]          = volVars.fluidState().enthalpy(phaseIdx);
                internalEnergy_[phaseIdx][globalIdx]    = volVars.fluidState().internalEnergy(phaseIdx);
                reynoldsNumber_[phaseIdx][globalIdx]    = volVars.reynoldsNumber(phaseIdx);
                prandtlNumber_[phaseIdx][globalIdx]     = volVars.prandtlNumber(phaseIdx);
                nusseltNumber_[phaseIdx][globalIdx]             = volVars.nusseltNumber(phaseIdx);
            }

            // because numPhases only counts liquid phases
            for (int phaseIdx = 0; phaseIdx < numEnergyEqs; ++ phaseIdx) {
                temperature_[phaseIdx][globalIdx] = volVars.temperature(phaseIdx);
                Valgrind::CheckDefined(temperature_[phaseIdx][globalIdx]);
            }

            const unsigned int wPhaseIdx = FluidSystem::wPhaseIdx;
            const unsigned int nPhaseIdx = FluidSystem::nPhaseIdx;
            const unsigned int sPhaseIdx = FluidSystem::sPhaseIdx;

            TwMinusTn_[globalIdx]  = volVars.temperature(wPhaseIdx) - volVars.temperature(nPhaseIdx);
            TnMinusTs_[globalIdx]  = volVars.temperature(nPhaseIdx) - volVars.temperature(sPhaseIdx);

            awn_[globalIdx]          = volVars.interfacialArea(wPhaseIdx, nPhaseIdx);
            aws_[globalIdx]          = volVars.interfacialArea(wPhaseIdx, sPhaseIdx);
            ans_[globalIdx]          = volVars.interfacialArea(nPhaseIdx, sPhaseIdx);

            if (velocityAveragingInModel and not velocityOutput/*only one of the two output options, otherwise paraview segfaults due to two times the same field name*/){
                int numVertices = this->problem_.gridView().size(dim); // numVertices for vertexCentereed, numVolumes for volume centered
                for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx)
                    for (int I = 0; I < numVertices; ++I)
                        velocity_[phaseIdx][I] = this->problem_.model().volumeDarcyVelocity(phaseIdx, I);
            }
        }
    }

    /*!
     * \brief Add all buffers to the VTK output writer.
     */
    template <class MultiWriter>
    void commitBuffers(MultiWriter & writer)
    {

        if(interfacialAreaOutput){
            this->commitScalarBuffer_(writer, "awn" , awn_);
            this->commitScalarBuffer_(writer, "aws" , aws_);
            this->commitScalarBuffer_(writer, "ans" , ans_);
        }

        if (temperatureOutput){
            this->commitTemperaturesBuffer_(writer, "T_%s", temperature_);
            this->commitScalarBuffer_(writer, "TwMinusTn" , TwMinusTn_);
            this->commitScalarBuffer_(writer, "TnMinusTs" , TnMinusTs_);
        }

        if (enthalpyOutput)
            this->commitPhaseBuffer_(writer, "h_%s", enthalpy_);
        if (internalEnergyOutput)
            this->commitPhaseBuffer_(writer, "u_%s", internalEnergy_);
        if (reynoldsOutput)
            this->commitPhaseBuffer_(writer, "reynoldsNumber_%s", reynoldsNumber_);
        if (prandtlOutput)
            this->commitPhaseBuffer_(writer, "prandtlNumber_%s", prandtlNumber_);
        if (nusseltOutput)
            this->commitPhaseBuffer_(writer, "nusseltNumber_%s", nusseltNumber_);
        if (velocityAveragingInModel and not velocityOutput/*only one of the two output options, otherwise paraview segfaults due to two timies the same field name*/){
            for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
                // commit the phase velocity
                std::ostringstream oss;
                oss << "velocity_" << FluidSystem::phaseName(phaseIdx);
                writer.attachVertexData(velocity_[phaseIdx],
                                        oss.str(),
                                        dim);
            }
        }
    }

private:
    /*!
     * \brief Allocate the space for a buffer storing temperatures.
     *         This is one more entry than (fluid) phases.
     */
    void resizeTemperaturesBuffer_(EnergyEqVector & buffer,
                                   bool vertexCentered = true)
    {
        Scalar n; // numVertices for vertexCentereed, numVolumes for volume centered
        if (vertexCentered)
            n = this->problem_.gridView().size(dim);
        else
            n = this->problem_.gridView().size(0);

        for (int energyEqIdx = 0; energyEqIdx < numEnergyEqs; ++energyEqIdx) {
            buffer[energyEqIdx].resize(n);
            std::fill(buffer[energyEqIdx].begin(), buffer[energyEqIdx].end(), 0.0);
        }
    }

/*!
     * \brief Add a buffer for the three tmeperatures (fluids+solid) to the VTK result file.
     */
    template <class MultiWriter>
    void commitTemperaturesBuffer_(MultiWriter & writer,
                                   const char *pattern,
                                   EnergyEqVector & buffer,
                                   bool vertexCentered = true)
    {
        for (int energyEqIdx = 0; energyEqIdx < numEnergyEqs; ++energyEqIdx) {
            std::ostringstream oss;
            oss << "T_" << FluidSystem::phaseName(energyEqIdx);

            if (vertexCentered)
                writer.attachVertexData(buffer[energyEqIdx], oss.str(), 1);
            else
                writer.attachCellData(buffer[energyEqIdx], oss.str(), 1);
        }
    }

    EnergyEqVector temperature_ ;
    ScalarVector TwMinusTn_;
    ScalarVector TnMinusTs_;
    PhaseVector enthalpy_ ;
    PhaseVector internalEnergy_ ;
    PhaseVector reynoldsNumber_ ;
    PhaseVector prandtlNumber_ ;
    PhaseVector nusseltNumber_ ;

    PhaseDimField  velocity_;

    ScalarVector awn_;
    ScalarVector aws_;
    ScalarVector ans_;
};

}

#endif
