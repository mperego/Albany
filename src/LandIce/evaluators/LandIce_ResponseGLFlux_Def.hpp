//*****************************************************************//
//    Albany 3.0:  Copyright 2016 Sandia Corporation               //
//    This Software is released under the BSD license detailed     //
//    in the file "license.txt" in the top-level Albany directory  //
//*****************************************************************//

#include <fstream>
#include "Teuchos_TestForException.hpp"
#include "Phalanx_DataLayout.hpp"
#include "Teuchos_CommHelpers.hpp"
#include "PHAL_Utilities.hpp"

template<typename EvalT, typename Traits>
LandIce::ResponseGLFlux<EvalT, Traits>::
ResponseGLFlux(Teuchos::ParameterList& p, const Teuchos::RCP<Albany::Layouts>& dl)
{
  // get and validate Response parameter list
  Teuchos::ParameterList* plist = p.get<Teuchos::ParameterList*>("Parameter List");
  Teuchos::RCP<Teuchos::ParameterList> paramList = p.get<Teuchos::RCP<Teuchos::ParameterList> >("Parameters From Problem");
  Teuchos::RCP<ParamLib> paramLib = paramList->get< Teuchos::RCP<ParamLib> > ("Parameter Library");
  scaling = plist->get<double>("Scaling Coefficient", 1.0);

  basalSideName = paramList->get<std::string> ("Basal Side Name");

  const std::string& avg_vel_name     = paramList->get<std::string>("Averaged Vertical Velocity Side Variable Name");
  const std::string& thickness_name   = paramList->get<std::string>("Thickness Side Variable Name");
  const std::string& bed_name         = paramList->get<std::string>("Bed Topography Side Variable Name");
  const std::string& coords_name      = paramList->get<std::string>("Coordinate Vector Side Variable Name");

  TEUCHOS_TEST_FOR_EXCEPTION (dl->side_layouts.find(basalSideName)==dl->side_layouts.end(), std::runtime_error,
                              "Error! Basal side data layout not found.\n");

  Teuchos::RCP<Albany::Layouts> dl_basal = dl->side_layouts.at(basalSideName);

  avg_vel        = decltype(avg_vel)(avg_vel_name, dl_basal->node_vector);
  thickness      = decltype(thickness)(thickness_name, dl_basal->node_scalar);
  bed            = decltype(bed)(bed_name, dl_basal->node_scalar);
  coords         = decltype(coords)(coords_name, dl_basal->vertices_vector);

  cell_topo = paramList->get<Teuchos::RCP<const CellTopologyData> >("Cell Topology");
  Teuchos::RCP<const Teuchos::ParameterList> reflist = this->getValidResponseParameters();
  plist->validateParameters(*reflist, 0);

  // Get Dimensions
  numSideNodes = dl_basal->node_scalar->dimension(2);
  numSideDims  = dl_basal->vertices_vector->dimension(3);

  // add dependent fields
  this->addDependentField(avg_vel);
  this->addDependentField(thickness);
  this->addDependentField(bed);
  this->addDependentField(coords);

  this->setName("Response Surface Mass Balance Mismatch" + PHX::typeAsString<EvalT>());

  using PHX::MDALayout;

  rho_w = paramList->sublist("LandIce Physical Parameters List",true).get<double>("Water Density");
  rho_i = paramList->sublist("LandIce Physical Parameters List",true).get<double>("Ice Density");

  // Setup scatter evaluator
  p.set("Stand-alone Evaluator", false);
  std::string local_response_name = "Local Response SMB Mismatch";
  std::string global_response_name = "Global SMB Mismatch";
  int worksetSize = dl_basal->node_scalar->dimension(0);
  int responseSize = 1;
  auto local_response_layout = Teuchos::rcp(new MDALayout<Cell, Dim>(worksetSize, responseSize));
  auto global_response_layout = Teuchos::rcp(new MDALayout<Dim>(responseSize));
  PHX::Tag<ScalarT> local_response_tag(local_response_name, local_response_layout);
  PHX::Tag<ScalarT> global_response_tag(global_response_name, global_response_layout);
  p.set("Local Response Field Tag", local_response_tag);
  p.set("Global Response Field Tag", global_response_tag);
  PHAL::SeparableScatterScalarResponse<EvalT, Traits>::setup(p, dl);
}

// **********************************************************************
template<typename EvalT, typename Traits>
void LandIce::ResponseGLFlux<EvalT, Traits>::postRegistrationSetup(typename Traits::SetupData d, PHX::FieldManager<Traits>& fm)
{
  PHAL::SeparableScatterScalarResponse<EvalT, Traits>::postRegistrationSetup(d, fm);
  gl_func = Kokkos::createDynRankView(bed.get_view(), "gl_func", numSideNodes);
  H = Kokkos::createDynRankView(bed.get_view(), "H", 2);
  x = Kokkos::createDynRankView(bed.get_view(), "x", 2);
  y = Kokkos::createDynRankView(bed.get_view(), "y", 2);
  velx = Kokkos::createDynRankView(avg_vel.get_view(), "velx", 2);
  vely = Kokkos::createDynRankView(avg_vel.get_view(), "vely", 2);
}

// **********************************************************************
template<typename EvalT, typename Traits>
void LandIce::ResponseGLFlux<EvalT, Traits>::preEvaluate(typename Traits::PreEvalData workset) {
  PHAL::set(this->global_response_eval, 0.0);


  // Do global initialization
  PHAL::SeparableScatterScalarResponse<EvalT, Traits>::preEvaluate(workset);
}

// **********************************************************************
template<typename EvalT, typename Traits>
void LandIce::ResponseGLFlux<EvalT, Traits>::evaluateFields(typename Traits::EvalData workset)
{
  if (workset.sideSets == Teuchos::null)
    TEUCHOS_TEST_FOR_EXCEPTION(true, std::logic_error, "Side sets defined in input file but not properly specified on the mesh" << std::endl);

  // Zero out local response
  PHAL::set(this->local_response_eval, 0.0);

  if (workset.sideSets->find(basalSideName) != workset.sideSets->end())
  {
    const std::vector<Albany::SideStruct>& sideSet = workset.sideSets->at(basalSideName);
    for (auto const& it_side : sideSet)
    {
      // Get the local data of side and cell
      const int cell = it_side.elem_LID;
      const int side = it_side.side_local_id;


  //    for (int inode=0; inode<numSideNodes; ++inode)
    //    t+=avg_vel(cell,side,inode,0);//+avg_vel(cell,side,inode,1);
        //t+=thickness(cell,side,inode);


///*    
      for (int inode=0; inode<numSideNodes; ++inode)
        gl_func(inode) = rho_i*thickness(cell,side,inode)+rho_w*bed(cell,side,inode);

      bool isGLCell = false;

      for (int inode=1; inode<numSideNodes; ++inode)
        isGLCell = isGLCell || (gl_func(0)*gl_func(inode) <=0);

      if(!isGLCell)
        continue;

      ParamScalarT maxTheta(0);
      int maxTheta_index, maxTheta_counter;
  //*
    for (int inode=0, counter=0; (inode<numSideNodes) && (counter<2); ++inode) {
        int inode1 = (inode+1)%numSideNodes;
        ParamScalarT gl0 = gl_func(inode), gl1 = gl_func(inode1);
        if(gl0*gl1 <= 0) {
          ParamScalarT theta = gl0/(gl0-gl1);
          //std::cout << counter << " "  << inode << " " << inode1 <<std::endl;
          H(counter) = thickness(cell,side,inode1)*theta + thickness(cell,side,inode)*(1-theta);
          x(counter) = coords(cell,side,inode1,0)*theta + coords(cell,side,inode,0)*(1-theta);
          y(counter) = coords(cell,side,inode1,1)*theta + coords(cell,side,inode,1)*(1-theta);
          velx(counter) = avg_vel(cell,side,inode1,0)*theta + avg_vel(cell,side,inode,0)*(1-theta);
          vely(counter) = avg_vel(cell,side,inode1,1)*theta + avg_vel(cell,side,inode,1)*(1-theta);
          if(maxTheta < theta) {
            maxTheta = theta;
            maxTheta_index = inode;
            maxTheta_counter = counter;
          }
          ++counter;
        }
      }
//*/
       int sign = 1;//(y[1]-y[0])*(coords(cell,side,maxTheta_index,0)-x[maxTheta_counter])-(x[1]-x[0])*(coords(cell,side,maxTheta_index,1)-y[maxTheta_counter])*gl_func(maxTheta_index)<0;
      ScalarT t = 0.5*sign*((H(0)*velx(0)+H(1)*velx(1))*(y(1)-y(0))-(H(0)*vely(0)+H(1)*vely(1))*(x(1)-x(0)));
      t = abs(t);
     //t = 0.25*sign*(H[0]+H[1])*((velx[0]+velx[1])*(y[1]-y[0])-(vely[0]+vely[1])*(x[1]-x[0]));

      //  t += pow((avg_vel(cell,side,inode)-SMB(cell,side,qp))/SMBRMS(cell,side,qp),2) * w_measure_2d(cell,side,qp);
//*/
      this->local_response_eval(cell, 0) += t*scaling;
      //std::cout << this->local_response(cell, 0) << std::endl;
      this->global_response_eval(0) += t*scaling;
    }
  }

  // Do any local-scattering necessary
  PHAL::SeparableScatterScalarResponse<EvalT, Traits>::evaluateFields(workset);
  PHAL::SeparableScatterScalarResponse<EvalT, Traits>::evaluate2DFieldsDerivativesDueToExtrudedSolution(workset,basalSideName, cell_topo);
}

// **********************************************************************
template<typename EvalT, typename Traits>
void LandIce::ResponseGLFlux<EvalT, Traits>::postEvaluate(typename Traits::PostEvalData workset) {
  //amb Deal with op[], pointers, and reduceAll.
  PHAL::reduceAll<ScalarT>(*workset.comm, Teuchos::REDUCE_SUM,
                           this->global_response_eval);

  // Do global scattering
  PHAL::SeparableScatterScalarResponse<EvalT, Traits>::postEvaluate(workset);
}

// **********************************************************************
template<typename EvalT, typename Traits>
Teuchos::RCP<const Teuchos::ParameterList> LandIce::ResponseGLFlux<EvalT, Traits>::getValidResponseParameters() const {
  Teuchos::RCP<Teuchos::ParameterList> validPL = rcp(new Teuchos::ParameterList("Valid ResponseGLFlux Params"));
  Teuchos::RCP<const Teuchos::ParameterList> baseValidPL = PHAL::SeparableScatterScalarResponse<EvalT, Traits>::getValidResponseParameters();
  validPL->setParameters(*baseValidPL);

  validPL->set<std::string>("Name", "", "Name of response function");
  validPL->set<double>("Scaling Coefficient", 1.0, "Coefficient that scales the response");
  validPL->set<Teuchos::RCP<const CellTopologyData> >("Cell Topology",Teuchos::RCP<const CellTopologyData>(),"Cell Topology Data");
  validPL->set<int>("Cubature Degree", 3, "degree of cubature used to compute the velocity mismatch");
  validPL->set<int>("Phalanx Graph Visualization Detail", 0, "Make dot file to visualize phalanx graph");
  validPL->set<std::string>("Description", "", "Description of this response used by post processors");
  validPL->set<std::string> ("Basal Side Name", "", "Name of the side set corresponding to the ice-bedrock interface");

  return validPL;
}
// **********************************************************************

