//*****************************************************************//
//    Albany 3.0:  Copyright 2016 Sandia Corporation               //
//    This Software is released under the BSD license detailed     //
//    in the file "license.txt" in the top-level Albany directory  //
//*****************************************************************//
#include "Albany_ResponseUtilities.hpp"
#include "Albany_Utils.hpp"

#include "QCAD_ResponseFieldIntegral.hpp"
#include "QCAD_ResponseFieldValue.hpp"
#include "QCAD_ResponseFieldAverage.hpp"
#include "QCAD_ResponseSaveField.hpp"
#include "QCAD_ResponseCenterOfMass.hpp"
#include "PHAL_ResponseFieldIntegral.hpp"
#include "PHAL_ResponseThermalEnergy.hpp"
#include "Adapt_ElementSizeField.hpp"
#include "PHAL_ResponseSquaredL2Difference.hpp"
#include "PHAL_ResponseSquaredL2DifferenceSide.hpp"
#include "PHAL_SaveNodalField.hpp"
#ifdef ALBANY_LANDICE
#include "LandIce_ResponseSurfaceVelocityMismatch.hpp"
#include "LandIce_ResponseSMBMismatch.hpp"
#include "LandIce_ResponseGLFlux.hpp"
#include "LandIce_ResponseBoundarySquaredL2Norm.hpp"
#endif
#if defined(ALBANY_LCM)
#include "IPtoNodalField.hpp"
#include "ProjectIPtoNodalField.hpp"
#endif
#ifdef ALBANY_ATO
#include "ATO_StiffnessObjective.hpp"
#include "ATO_InterfaceEnergy.hpp"
#include "ATO_InternalEnergyResponse.hpp"
#include "ATO_TensorPNormResponse.hpp"
#include "ATO_TensorAverageResponse.hpp"
#include "ATO_HomogenizedConstantsResponse.hpp"
#include "ATO_ModalObjective.hpp"
#endif
#ifdef ALBANY_AERAS
#include "Aeras_ShallowWaterResponseL2Error.hpp"
#include "Aeras_ShallowWaterResponseL2Norm.hpp"
#endif

template<typename EvalT, typename Traits>
Albany::ResponseUtilities<EvalT,Traits>::ResponseUtilities(
  Teuchos::RCP<Albany::Layouts> dl_) :
  dl(dl_)
{
}

template<typename EvalT, typename Traits>
Teuchos::RCP<const PHX::FieldTag>
Albany::ResponseUtilities<EvalT,Traits>::constructResponses(
  PHX::FieldManager<PHAL::AlbanyTraits>& fm,
  Teuchos::ParameterList& responseParams,
  Teuchos::RCP<Teuchos::ParameterList> paramsFromProblem,
  Albany::StateManager& stateMgr,
  const Albany::MeshSpecsStruct* meshSpecs)
{
  using Teuchos::RCP;
  using Teuchos::rcp;
  using Teuchos::ParameterList;
  using PHX::DataLayout;

  std::string responseName = responseParams.get<std::string>("Name");
  RCP<ParameterList> p = rcp(new ParameterList);
  p->set<ParameterList*>("Parameter List", &responseParams);
  p->set<RCP<ParameterList> >("Parameters From Problem", paramsFromProblem);
  RCP<PHX::Evaluator<Traits>> res_ev;

  if (responseName == "Field Integral")
  {
    res_ev = rcp(new QCAD::ResponseFieldIntegral<EvalT,Traits>(*p, dl));
  }
  else if (responseName == "Field Value")
  {
    res_ev = rcp(new QCAD::ResponseFieldValue<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Field Average")
  {
    res_ev = rcp(new QCAD::ResponseFieldAverage<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Squared L2 Difference Source ST Target ST")
  {
    res_ev = rcp(new PHAL::ResponseSquaredL2DifferenceSST_TST<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Squared L2 Difference Source ST Target MST")
  {
    res_ev =rcp(new PHAL::ResponseSquaredL2DifferenceSST_TMST<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Squared L2 Difference Source ST Target PST")
  {
    res_ev = rcp(new PHAL::ResponseSquaredL2DifferenceSST_TPST<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Squared L2 Difference Source PST Target ST")
  {
    res_ev = rcp(new PHAL::ResponseSquaredL2DifferenceSPST_TST<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Squared L2 Difference Source PST Target MST")
  {
    res_ev = rcp(new PHAL::ResponseSquaredL2DifferenceSPST_TMST<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Squared L2 Difference Source PST Target PST")
  {
    res_ev = rcp(new PHAL::ResponseSquaredL2DifferenceSPST_TPST<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Squared L2 Difference Source MST Target ST")
  {
    res_ev = rcp(new PHAL::ResponseSquaredL2DifferenceSMST_TST<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Squared L2 Difference Source MST Target MST")
  {
    res_ev = rcp(new PHAL::ResponseSquaredL2DifferenceSMST_TMST<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Squared L2 Difference Source MST Target PST")
  {
    res_ev = rcp(new PHAL::ResponseSquaredL2DifferenceSideSMST_TPST<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Squared L2 Difference Side Source ST Target ST")
  {
    res_ev = rcp(new PHAL::ResponseSquaredL2DifferenceSideSST_TST<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Squared L2 Difference Side Source ST Target MST")
  {
    res_ev =rcp(new PHAL::ResponseSquaredL2DifferenceSideSST_TMST<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Squared L2 Difference Side Source ST Target PST")
  {
    res_ev = rcp(new PHAL::ResponseSquaredL2DifferenceSideSST_TPST<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Squared L2 Difference Side Source PST Target ST")
  {
    res_ev = rcp(new PHAL::ResponseSquaredL2DifferenceSideSPST_TST<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Squared L2 Difference Side Source PST Target MST")
  {
    res_ev = rcp(new PHAL::ResponseSquaredL2DifferenceSideSPST_TMST<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Squared L2 Difference Side Source PST Target PST")
  {
    res_ev = rcp(new PHAL::ResponseSquaredL2DifferenceSideSPST_TPST<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Squared L2 Difference Side Source MST Target ST")
  {
    res_ev = rcp(new PHAL::ResponseSquaredL2DifferenceSideSMST_TST<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Squared L2 Difference Side Source MST Target MST")
  {
    res_ev = rcp(new PHAL::ResponseSquaredL2DifferenceSideSMST_TMST<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Squared L2 Difference Side Source MST Target PST")
  {
    res_ev = rcp(new PHAL::ResponseSquaredL2DifferenceSMST_TPST<EvalT,Traits>(*p,dl));
  }
#ifdef ALBANY_LANDICE
  else if (responseName == "Surface Velocity Mismatch")
  {
    res_ev = rcp(new LandIce::ResponseSurfaceVelocityMismatch<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Surface Mass Balance Mismatch")
  {
    res_ev = rcp(new LandIce::ResponseSMBMismatch<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Grounding Line Flux")
  {
    res_ev = rcp(new LandIce::ResponseGLFlux<EvalT,Traits>(*p,dl));
  }
  else if (responseName == "Boundary Squared L2 Norm")
  {
    res_ev = rcp(new LandIce::ResponseBoundarySquaredL2Norm<EvalT,Traits>(*p,dl));
  }
#endif
  else if (responseName == "Center Of Mass")
  {
    res_ev = rcp(new QCAD::ResponseCenterOfMass<EvalT,Traits>(*p, dl));
  }
  else if (responseName == "Save Field")
  {
    p->set< Albany::StateManager* >("State Manager Ptr", &stateMgr );

    res_ev = rcp(new QCAD::ResponseSaveField<EvalT,Traits>(*p, dl));
  }
  else if (responseName == "PHAL Field Integral")
  {
    res_ev = rcp(new PHAL::ResponseFieldIntegral<EvalT,Traits>(*p, dl));
  }
  else if (responseName == "PHAL Field IntegralT")
  {
    // Leave this for backward compatibility
    std::cout << "WARNING: 'PHAL Field IntegralT' is deprecated. You can just use 'PHAL Field Integral' (no T at the end).\n";
    res_ev = rcp(new PHAL::ResponseFieldIntegral<EvalT,Traits>(*p, dl));
  }
  else if (responseName == "PHAL Thermal Energy")
  {
    res_ev = rcp(new PHAL::ResponseThermalEnergy<EvalT,Traits>(*p, dl));
  }
  else if (responseName == "PHAL Thermal EnergyT")
  {
    // Leave this for backward compatibility
    std::cout << "WARNING: 'PHAL Thermal EnergyT' is deprecated. You can just use 'PHAL Thermal Energy' (no T at the end).\n";
    res_ev = rcp(new PHAL::ResponseThermalEnergy<EvalT,Traits>(*p, dl));
  }

#ifdef ALBANY_AERAS
  else if (responseName == "Aeras Shallow Water L2 Error")
  {
    res_ev = rcp(new Aeras::ShallowWaterResponseL2Error<EvalT,Traits>(*p, dl));
  }
  else if (responseName == "Aeras Shallow Water L2 Norm")
  {
    res_ev = rcp(new Aeras::ShallowWaterResponseL2Norm<EvalT,Traits>(*p, dl));
  }
#endif

  else if (responseName == "Element Size Field")
  {
    p->set< Albany::StateManager* >("State Manager Ptr", &stateMgr );
    p->set< RCP<DataLayout> >("Dummy Data Layout", dl->dummy);
    p->set<std::string>("Coordinate Vector Name", "Coord Vec");
    p->set<std::string>("Weights Name",  "Weights");

    res_ev = rcp(new Adapt::ElementSizeField<EvalT,Traits>(*p, dl));
  }
  else if (responseName == "Save Nodal Fields")
  {
    p->set< Albany::StateManager* >("State Manager Ptr", &stateMgr );

    res_ev = rcp(new PHAL::SaveNodalField<EvalT,Traits>(*p, dl));
  }
  else if (responseName == "Modal Objective")
  {
#ifdef ALBANY_ATO
    p->set< Albany::StateManager* >("State Manager Ptr", &stateMgr );

    res_ev = rcp(new ATO::ModalObjective<EvalT,Traits>(*p, dl));
#else
    TEUCHOS_TEST_FOR_EXCEPTION(
      true, Teuchos::Exceptions::InvalidParameter,
      std::endl << "Error!  Response function " << responseName <<
      " not available!" << std::endl << "Albany/ATO not enabled." <<
      std::endl);
#endif
  }

  else if (responseName == "Stiffness Objective")
  {
#ifdef ALBANY_ATO
    p->set< Albany::StateManager* >("State Manager Ptr", &stateMgr );
    res_ev = rcp(new ATO::StiffnessObjective<EvalT,Traits>(*p, dl, meshSpecs));
#else
    TEUCHOS_TEST_FOR_EXCEPTION(
      true, Teuchos::Exceptions::InvalidParameter,
      std::endl << "Error!  Response function " << responseName <<
      " not available!" << std::endl << "Albany/ATO not enabled." <<
      std::endl);
#endif
  }

  else if (responseName == "Interface Energy")
  {
#ifdef ALBANY_ATO
    p->set< Albany::StateManager* >("State Manager Ptr", &stateMgr );
    res_ev = rcp(new ATO::InterfaceEnergy<EvalT,Traits>(*p, dl, meshSpecs));
#else
    TEUCHOS_TEST_FOR_EXCEPTION(
      true, Teuchos::Exceptions::InvalidParameter,
      std::endl << "Error!  Response function " << responseName <<
      " not available!" << std::endl << "Albany/ATO not enabled." <<
      std::endl);
#endif
  }

  else if (responseName == "Tensor PNorm Objective")
  {
#ifdef ALBANY_ATO
    p->set< Albany::StateManager* >("State Manager Ptr", &stateMgr );
    res_ev = rcp(new ATO::TensorPNormResponse<EvalT,Traits>(*p, dl, meshSpecs));
#else
    TEUCHOS_TEST_FOR_EXCEPTION(
      true, Teuchos::Exceptions::InvalidParameter,
      std::endl << "Error!  Response function " << responseName <<
      " not available!" << std::endl << "Albany/ATO not enabled." <<
      std::endl);
#endif
  }

  else if (responseName == "Tensor Average Response")
  {
#ifdef ALBANY_ATO
    p->set< Albany::StateManager* >("State Manager Ptr", &stateMgr );
    res_ev = rcp(new ATO::TensorAverageResponse<EvalT,Traits>(*p, dl));
#else
    TEUCHOS_TEST_FOR_EXCEPTION(
      true, Teuchos::Exceptions::InvalidParameter,
      std::endl << "Error!  Response function " << responseName <<
      " not available!" << std::endl << "Albany/ATO not enabled." <<
      std::endl);
#endif
  }

  else if (responseName == "Homogenized Constants Response")
  {
#ifdef ALBANY_ATO
    p->set< Albany::StateManager* >("State Manager Ptr", &stateMgr );
    res_ev = rcp(new ATO::HomogenizedConstantsResponse<EvalT,Traits>(*p, dl));
#else
    TEUCHOS_TEST_FOR_EXCEPTION(
      true, Teuchos::Exceptions::InvalidParameter,
      std::endl << "Error!  Response function " << responseName <<
      " not available!" << std::endl << "Albany/ATO not enabled." <<
      std::endl);
#endif
  }

  else if (responseName == "Internal Energy Objective")
  {
#ifdef ALBANY_ATO
    p->set< Albany::StateManager* >("State Manager Ptr", &stateMgr );

    res_ev = rcp(new ATO::InternalEnergyResponse<EvalT,Traits>(*p, dl, meshSpecs));
#else
    TEUCHOS_TEST_FOR_EXCEPTION(
      true, Teuchos::Exceptions::InvalidParameter,
      std::endl << "Error!  Response function " << responseName <<
      " not available!" << std::endl << "Albany/ATO not enabled." <<
      std::endl);
#endif
  }

#if defined(ALBANY_LCM)
  else if (responseName == "IP to Nodal Field")
  {
    p->set< Albany::StateManager* >("State Manager Ptr", &stateMgr );
    p->set< RCP<DataLayout> >("Dummy Data Layout", dl->dummy);
    //p->set<std::string>("Stress Name", "Cauchy_Stress");
    //p->set<std::string>("Weights Name",  "Weights");

    res_ev = rcp(new LCM::IPtoNodalField<EvalT,Traits>(*p, dl, meshSpecs));
  }
  else if (responseName == "Project IP to Nodal Field")
  {
    p->set< Albany::StateManager* >("State Manager Ptr", &stateMgr );
    p->set< RCP<DataLayout> >("Dummy Data Layout", dl->dummy);
    p->set<std::string>("BF Name", "BF");
    p->set<std::string>("Weighted BF Name", "wBF");
    p->set<std::string>("Coordinate Vector Name", "Coord Vec");

    res_ev = rcp(new LCM::ProjectIPtoNodalField<EvalT,Traits>(*p, dl, meshSpecs));
  }
#endif

  else
    TEUCHOS_TEST_FOR_EXCEPTION(
      true, Teuchos::Exceptions::InvalidParameter,
      std::endl << "Error!  Unknown response function " << responseName <<
      "!" << std::endl << "Supplied parameter list is " <<
      std::endl << responseParams);

  // Register the evaluator
  fm.template registerEvaluator<EvalT>(res_ev);

  // Fetch the response tag. Usually it is the tag of the first evaluated field
  Teuchos::RCP<const PHX::FieldTag> ev_tag = res_ev->evaluatedFields()[0];

  // The response tag is not the same of the evaluated field tag for PHAL::ScatterScalarResponse
  Teuchos::RCP<PHAL::ScatterScalarResponseBase<EvalT,Traits>> sc_resp;
  sc_resp = Teuchos::rcp_dynamic_cast<PHAL::ScatterScalarResponseBase<EvalT,Traits>>(res_ev);
  if (sc_resp!=Teuchos::null)
  {
    ev_tag = sc_resp->getResponseFieldTag();
  }

  // Require the response tag;
  fm.requireField<EvalT>(*ev_tag);

  return ev_tag;
}
