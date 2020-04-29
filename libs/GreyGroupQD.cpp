// File: GreyGroupQD.cpp     
// Purpose: Contains information and builds linear system for grey group qd
// Date: April 9, 2020

#include "GreyGroupQD.h"
#include "GreyGroupSolver.h"
#include "MultiPhysicsCoupledQD.h"

using namespace std;

//==============================================================================
/// GreyGroupQD class object constructor
///
/// @param [in] blankType blank for this material
GreyGroupQD::GreyGroupQD(Materials * myMaterials,\
  Mesh * myMesh,\
  YAML::Node * myInput,\
  MultiPhysicsCoupledQD * myMPQD)
{
  // Assign pointers for materials, mesh, and input objects
  mpqd = myMPQD;
  materials = myMaterials;
  mesh = myMesh;
  input = myInput;

  GGSolver = std::make_shared<GreyGroupSolver>(this,mesh,materials,input);

  // initialize Eddington factors to diffusion physics
  Err.setOnes(mesh->zCornerCent.size(),mesh->rCornerCent.size());
  Err = (1.0/3.0)*Err;
  Ezz.setOnes(mesh->zCornerCent.size(),mesh->rCornerCent.size());
  Ezz = (1.0/3.0)*Ezz;
  Erz.setOnes(mesh->zCornerCent.size(),mesh->rCornerCent.size());
  Erz = 0.0*Erz;
  
  // initialize previous Eddington factors
  ErrPrev = Err;
  EzzPrev = Ezz;
  ErzPrev = Erz;
  
  // initialize source 
  q.setOnes(mesh->zCornerCent.size(),mesh->rCornerCent.size());
  q = 0.0*q;

  // initialize flux and current matrices
  sFlux.setZero(mesh->zCornerCent.size(),mesh->rCornerCent.size());
  sFluxR.setZero(mesh->zCornerCent.size(),mesh->rCornerCent.size()+1);
  sFluxZ.setZero(mesh->zCornerCent.size()+1,mesh->rCornerCent.size());
  currentR.setZero(mesh->zCornerCent.size(),mesh->rCornerCent.size()+1);
  currentZ.setZero(mesh->zCornerCent.size()+1,mesh->rCornerCent.size());
  
  // initialize boundary conditions
  wFluxBC.setOnes(mesh->dzsCorner.size());
  eFluxBC.setOnes(mesh->dzsCorner.size());
  nFluxBC.setOnes(mesh->drsCorner.size());
  sFluxBC.setOnes(mesh->drsCorner.size());
  wCurrentRBC.setZero(mesh->dzsCorner.size());
  eCurrentRBC.setZero(mesh->dzsCorner.size());
  nCurrentZBC.setZero(mesh->drsCorner.size());
  sCurrentZBC.setZero(mesh->drsCorner.size());
  eInwardFluxBC.setZero(mesh->dzsCorner.size());
  nInwardFluxBC.setZero(mesh->drsCorner.size());
  sInwardFluxBC.setZero(mesh->drsCorner.size());
  eInwardCurrentBC.setZero(mesh->dzsCorner.size());
  nInwardCurrentBC.setZero(mesh->drsCorner.size());
  sInwardCurrentBC.setZero(mesh->drsCorner.size());
  eOutwardCurrToFluxRatioBC.setZero(mesh->dzsCorner.size());
  nOutwardCurrToFluxRatioBC.setZero(mesh->drsCorner.size());
  sOutwardCurrToFluxRatioBC.setZero(mesh->drsCorner.size());
  eAbsCurrentBC.setZero(mesh->dzsCorner.size());
  nAbsCurrentBC.setZero(mesh->drsCorner.size());
  sAbsCurrentBC.setZero(mesh->drsCorner.size());


};
//==============================================================================

//==============================================================================
/// Build linear system for QD equations
///
void GreyGroupQD::buildLinearSystem()
{

  GGSolver->formLinearSystem(); // Assuming this is the first set of equations
                                 //   equations to be built  
};
//==============================================================================


