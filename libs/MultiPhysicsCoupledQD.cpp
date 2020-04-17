// File: MultiPhysicsCoupledQD.cpp     
// Purpose: Contains precursor, heat, and grey group quasidiffusion objects. 
//   Also owns and is responsible for solving the coupled system each of 
//   of those object contribute to.
// Date: April 9, 2020

#include "MultiPhysicsCoupledQD.h"
#include "HeatTransfer.h"
#include "MultiGroupPrecursor.h"

using namespace std;

//==============================================================================
/// MultiPhysicsCoupledQD class object constructor
///
/// @param [in] blankType blank for this material
MultiPhysicsCoupledQD::MultiPhysicsCoupledQD(Materials * myMats,\
  Mesh * myMesh,\
  YAML::Node * myInput) 
{
  // Assign inputs to their member variables
  mats = myMats;
  mesh = myMesh;
  input = myInput;

  // Initialize multiphysics objects
  heat = new HeatTransfer(mats,mesh,input,this);
  dnps = new MultiGroupPrecursor(mats,mesh,input,this);
};
//==============================================================================

//==============================================================================
/// Include a flux source in the linear system
///
/// @param [in] iZ axial location 
/// @param [in] iR radial location
/// @param [in] iEq equation index
/// @param [in] coeff coefficient of flux source
void MultiPhysicsCoupledQD::fluxSource(int iZ,int iR,int iEq,double coeff)
{

};
//==============================================================================

//==============================================================================
/// Solve linear system for multiphysics coupled quasidiffusion system
///
void MultiPhysicsCoupledQD::solveLinearSystem()
{
  cout << "x: " << x << endl;
  Eigen::SparseLU<Eigen::SparseMatrix<double>,\
    Eigen::COLAMDOrdering<int> > solverLU;
  A.makeCompressed();
  solverLU.compute(A);
  x = solverLU.solve(b);
  cout << "x: " << x << endl;
};
//==============================================================================
