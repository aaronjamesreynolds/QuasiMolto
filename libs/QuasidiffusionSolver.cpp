// File: QuasidiffusionSolver.cpp
// Purpose: Solve RZ quasidiffusion equations  
// Date: February 05, 2020

#include <iostream>
#include <cmath>
#include <vector>
#include <iomanip>
#include <armadillo>
#include "Mesh.h"
#include "Materials.h"
#include "Material.h"
#include "MMS.h"
#include "QuasidiffusionSolver.h"
#include "../TPLs/yaml-cpp/include/yaml-cpp/yaml.h"
#include "../TPLs/eigen-git-mirror/Eigen/Eigen"

using namespace std; 
using namespace arma;

//==============================================================================
/// QuasidiffusionSolver object constructor
///
/// @param [in] myMesh Mesh object for the simulation
/// @param [in] myMaterials Materials object for the simulation
/// @param [in] myInput YAML input object for the simulation
QDSolver::QDSolver(Mesh * myMesh,\
  Materials * myMaterials,\
  YAML::Node * myInput)	      
{
  // Point to variables for mesh and input file
  mesh = myMesh;
  input = myInput;
  materials = myMaterials;

  // temporary variables for initialization
  int nUnknowns;

  // calculate number of unknowns  
  energyGroups = materials->nGroups;
  nR = mesh->rCornerCent.size();
  nZ = mesh->zCornerCent.size();
  nGroupUnknowns = 5*(nZ*nR) + 2*nZ + 2*nR;
  nUnknowns = energyGroups*nGroupUnknowns;

  // initialize size of linear system
  A.resize(nUnknowns,nUnknowns);
  x.resize(nUnknowns);
  b.resize(nUnknowns);

};

//==============================================================================

//==============================================================================
/// Form a portion of the linear system that belongs to SGQD 
/// @param [in] SGQD quasidiffusion energy group to build portion of linear 
///   for
void QDSolver::formLinearSystem(SingleGroupQD * SGQD)	      
{

  cout << "linear system formed" << endl;

};

//==============================================================================

//==============================================================================
/// Assert the flux boundary condition on the north face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
void QDSolver::assertNFluxBC(int iR,int iZ,int iEq,int energyGroup,\
  SingleGroupQD * SGQD)
{
  int nIndex = NFluxIndex(iR,iZ,energyGroup);
  
  A.insert(iEq,nIndex) = 1.0;
  b(iEq) = SGQD->nFluxBC(iR);
};
//==============================================================================

//==============================================================================
/// Assert the flux boundary condition on the south face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
void QDSolver::assertSFluxBC(int iR,int iZ,int iEq,int energyGroup,\
  SingleGroupQD * SGQD)
{
  int sIndex = SFluxIndex(iR,iZ,energyGroup);
  
  A.insert(iEq,sIndex) = 1.0;
  b(iEq) = SGQD->sFluxBC(iR);
};
//==============================================================================

//==============================================================================
/// Assert the flux boundary condition on the west face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
void QDSolver::assertWFluxBC(int iR,int iZ,int iEq,int energyGroup,\
  SingleGroupQD * SGQD)
{
  int wIndex = WFluxIndex(iR,iZ,energyGroup);
  
  A.insert(iEq,wIndex) = 1.0;
  b(iEq) = SGQD->wFluxBC(iZ);
};
//==============================================================================

//==============================================================================
/// Assert the flux boundary condition on the east face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
void QDSolver::assertEFluxBC(int iR,int iZ,int iEq,int energyGroup,\
  SingleGroupQD * SGQD)
{
  int eIndex = EFluxIndex(iR,iZ,energyGroup);
  
  A.insert(iEq,eIndex) = 1.0;
  b(iEq) = SGQD->eFluxBC(iZ);
};
//==============================================================================

//==============================================================================
/// Assert the current boundary condition on the north face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
void QDSolver::assertNCurrentBC(int iR,int iZ,int iEq,int energyGroup,\
  SingleGroupQD * SGQD)
{
  int nIndex = NCurrentIndex(iR,iZ,energyGroup);
  
  A.insert(iEq,nIndex) = 1.0;
  b(iEq) = SGQD->nCurrentZBC(iR);
};
//==============================================================================

//==============================================================================
/// Assert the current boundary condition on the south face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
void QDSolver::assertSCurrentBC(int iR,int iZ,int iEq,int energyGroup,\
  SingleGroupQD * SGQD)
{
  int sIndex = SCurrentIndex(iR,iZ,energyGroup);
  
  A.insert(iEq,sIndex) = 1.0;
  b(iEq) = SGQD->sCurrentZBC(iR);
};
//==============================================================================

//==============================================================================
/// Assert the current boundary condition on the west face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
void QDSolver::assertWCurrentBC(int iR,int iZ,int iEq,int energyGroup,\
  SingleGroupQD * SGQD)
{
  int wIndex = WCurrentIndex(iR,iZ,energyGroup);
  
  A.insert(iEq,wIndex) = 1.0;
  b(iEq) = SGQD->wCurrentRBC(iZ);
};
//==============================================================================

//==============================================================================
/// Assert the current boundary condition on the east face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
void QDSolver::assertECurrentBC(int iR,int iZ,int iEq,int energyGroup,\
  SingleGroupQD * SGQD)
{
  int eIndex = ECurrentIndex(iR,iZ,energyGroup);
  
  A.insert(iEq,eIndex) = 1.0;
  b(iEq) = SGQD->eCurrentRBC(iZ);
};
//==============================================================================


//==============================================================================
/// Form a portion of the linear system that belongs to SGQD 
/// @param [in] SGQD quasidiffusion energy group to build portion of linear 
///   for
double QDSolver::calcIntegratingFactor(int iR,int iZ,double rEval,\
  SingleGroupQD * SGQD)	      
{
  double EzzL,ErrL,ErzL,G,rUp,rDown,rAvg,g0,g1,ratio,hEval;
  int p;
  
  // get local Eddington factors 
  ErrL = SGQD->Err(iZ,iR);
  EzzL = SGQD->Ezz(iZ,iR);
  ErzL = SGQD->Erz(iZ,iR);

  // evaluate G
  G = 1 + (ErrL+EzzL-1)/ErrL;

  // get boundaries of cell and calculate volume average 
  rUp = mesh->rEdge(iR+1); rDown = mesh->rEdge(iR);
  rAvg = calcVolAvgR(rDown,rUp);

  if (iR == 0){

    // use a special expression for cells that share a boundary with
    // the z-axis
    p = 2;
    ratio = (pow(rUp,p+1)-pow(rAvg,p+1))/(pow(rAvg,p)-pow(rUp,p));
    g1 = G/(pow(rAvg,p) * (rAvg + ratio));
    g0 = g1 * ratio;

    hEval = exp((g0*pow(rEval,p)/p)+g1*(pow(rEval,p+1))/(p+1));

  } else {

    // use the typical expression
    hEval = pow(rEval,G);

  }

};

//==============================================================================

//==============================================================================
/// Return global index of average flux at indices iR and iZ 
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [out] index global index for average flux in cell at (iR,iZ)
int QDSolver::CFluxIndex(int iR,int iZ,int energyGroup)
{
  int offset = energyGroup*nGroupUnknowns;
  int index = iR + nR * (iZ);
  return index;
};

//==============================================================================

//==============================================================================
/// Return global index of west face flux at indices iR and iZ 
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [out] index global index for west face flux in cell at (iR,iZ)
int QDSolver::WFluxIndex(int iR,int iZ,int energyGroup)
{
  int offset = energyGroup*nGroupUnknowns + nR*nZ;
  int westIndex = offset + (iR) + (nR + 1) * iZ;

  return westIndex;
};

//==============================================================================

//==============================================================================
/// Return global index of east face flux at indices iR and iZ 
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [out] index global index for east face flux in cell at (iR,iZ)
int QDSolver::EFluxIndex(int iR,int iZ,int energyGroup)
{
  int offset = energyGroup*nGroupUnknowns + nR*nZ;
  int eastIndex = offset + (iR + 1) + (nR + 1) * iZ;

  return eastIndex;
};

//==============================================================================

//==============================================================================
/// Return global index of north face flux at indices iR and iZ 
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [out] index global index for north face flux in cell at (iR,iZ)
int QDSolver::NFluxIndex(int iR,int iZ,int energyGroup)
{
  int offset = energyGroup*nGroupUnknowns + nR*nZ + (nR+1)*nZ;
  int northIndex = offset + (iZ) + (nZ + 1) * iR;

  return northIndex;
};

//==============================================================================

//==============================================================================
/// Return global index of south face flux at indices iR and iZ 
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [out] index global index for south face flux in cell at (iR,iZ)
int QDSolver::SFluxIndex(int iR,int iZ,int energyGroup)
{
  int offset = energyGroup*nGroupUnknowns + nR*nZ + (nR+1)*nZ;
  int southIndex = offset + (iZ + 1) + (nZ + 1) * iR;

  return southIndex;
};

//==============================================================================

//==============================================================================
/// Return global index of west face current at indices iR and iZ 
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [out] index global index for west face current in cell at (iR,iZ)
int QDSolver::WCurrentIndex(int iR,int iZ,int energyGroup)
{
  int offset = energyGroup*nGroupUnknowns + nR*nZ + (nR+1)*nZ + (nZ+1)*nR;
  int westIndex = offset + (iR) + (nR + 1) * iZ;

  return westIndex;
};

//==============================================================================

//==============================================================================
/// Return global index of east face current at indices iR and iZ 
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [out] index global index for east face current in cell at (iR,iZ)
int QDSolver::ECurrentIndex(int iR,int iZ,int energyGroup)
{
  int offset = energyGroup*nGroupUnknowns + nR*nZ + (nR+1)*nZ + (nZ+1)*nR;
  int eastIndex = offset + (iR + 1) + (nR + 1) * iZ;

  return eastIndex;
};

//==============================================================================

//==============================================================================
/// Return global index of north face current at indices iR and iZ 
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [out] index global index for north face current in cell at (iR,iZ)
int QDSolver::NCurrentIndex(int iR,int iZ,int energyGroup)
{
  int offset = energyGroup*nGroupUnknowns + nR*nZ + 2*(nR+1)*nZ + (nZ+1)*nR;
  int northIndex = offset + (iZ) + (nZ + 1) * iR;

  return northIndex;
};

//==============================================================================

//==============================================================================
/// Return global index of south face current at indices iR and iZ 
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [out] index global index for south face current in cell at (iR,iZ)
int QDSolver::SCurrentIndex(int iR,int iZ,int energyGroup)
{
  int offset = energyGroup*nGroupUnknowns + nR*nZ + 2*(nR+1)*nZ + (nZ+1)*nR;
  int southIndex = offset + (iZ + 1) + (nZ + 1) * iR;

  return southIndex;
};

//==============================================================================

//==============================================================================
/// Return global index of south face current at indices iR and iZ 
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [out] index global index for south face current in cell at (iR,iZ)
vector<int> QDSolver::indices(int iR,int iZ,int energyGroup)
{
  vector<int> indices;
  indices.push_back(CFluxIndex(iR,iZ,energyGroup));
  indices.push_back(WFluxIndex(iR,iZ,energyGroup));
  indices.push_back(EFluxIndex(iR,iZ,energyGroup));
  indices.push_back(NFluxIndex(iR,iZ,energyGroup));
  indices.push_back(SFluxIndex(iR,iZ,energyGroup));
  indices.push_back(WCurrentIndex(iR,iZ,energyGroup));
  indices.push_back(ECurrentIndex(iR,iZ,energyGroup));
  indices.push_back(NCurrentIndex(iR,iZ,energyGroup));
  indices.push_back(SCurrentIndex(iR,iZ,energyGroup));

  return indices;
};

//==============================================================================


//==============================================================================
/// Return geometry parameters for the cell located at (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [out] gParams vector containing volume and surfaces areas of the 
///   west, east, north, and south faces, in that order.
vector<double> QDSolver::calcGeoParams(int iR,int iZ)
{
  vector<double> gParams;
  double rDown,rUp,zDown,zUp,volume,wFaceSA,eFaceSA,nFaceSA,sFaceSA;
  
  // get boundaries of this cell
  rDown = mesh->rEdge(iR); rUp = mesh->rEdge(iR+1);
  zDown = mesh->zEdge(iR); zUp = mesh->zEdge(iR+1);

  // calculate geometry parameters
  volume = M_PI*(rUp*rUp-rDown*rDown)*(zUp-zDown);
  nFaceSA = M_PI*(rUp*rUp-rDown*rDown); 
  sFaceSA = nFaceSA;
  eFaceSA = 2*M_PI*rUp*(zUp-zDown);
  wFaceSA = 2*M_PI*rDown*(zUp-zDown);

  // add parameters to the vector and return it
  gParams.push_back(volume);
  gParams.push_back(wFaceSA);
  gParams.push_back(eFaceSA);
  gParams.push_back(nFaceSA);
  gParams.push_back(sFaceSA);

  return gParams;
};
//==============================================================================

//==============================================================================
/// Return volume-averaged radial coordinate for cell with boundaries rDown and
///   rUp
/// @param [in] rDown location of left cell edge
/// @param [in] rUp location of right cell edge
/// @param [out] volAvgR volume-averaged radial coordinate 
double QDSolver::calcVolAvgR(double rDown,double rUp)
{
  // calculate volume-averaged radius
  double volAvgR = (2/3)*(pow(rUp,3) - pow(rDown,3))/(pow(rUp,2)-pow(rDown,2));

  return volAvgR;
};
//==============================================================================




