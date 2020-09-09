// File: QuasidiffusionSolver.cpp
// Purpose: Solve RZ quasidiffusion equations  
// Date: February 05, 2020

#include "QuasidiffusionSolver.h"
#include "SingleGroupQD.h"
#include "GreyGroupQD.h"

using namespace std; 

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
  int nUnknowns,nCurrentUnknowns;

  // calculate number of unknowns  
  energyGroups = materials->nGroups;
  nR = mesh->rCornerCent.size();
  nZ = mesh->zCornerCent.size();
  nGroupUnknowns = 3*(nZ*nR) + nZ + nR;
  nGroupCurrentUnknowns = 2*(nZ*nR) + nZ + nR;
  nUnknowns = energyGroups*nGroupUnknowns;
  nCurrentUnknowns = energyGroups*nGroupCurrentUnknowns;

  // initialize size of linear system
  A.resize(nUnknowns,nUnknowns);
  A.reserve(3*nUnknowns+nUnknowns/5);
  C.resize(nCurrentUnknowns,nUnknowns);
  C.reserve(4*nCurrentUnknowns);
  x.setZero(nUnknowns);
  xPast.setZero(nUnknowns);
  currPast.setZero(energyGroups*nGroupCurrentUnknowns);
  b.setZero(nUnknowns);
  d.setZero(nCurrentUnknowns);

  checkOptionalParams();
};

//==============================================================================

//==============================================================================
/// Form a portion of the linear system that belongs to SGQD 
/// @param [in] SGQD quasidiffusion energy group to build portion of linear 
///   for
void QDSolver::formLinearSystem(SingleGroupQD * SGQD)	      
{
  int iEq = SGQD->energyGroup*nGroupUnknowns;

  // loop over spatial mesh
  for (int iR = 0; iR < mesh->drsCorner.size(); iR++)
  {
    for (int iZ = 0; iZ < mesh->dzsCorner.size(); iZ++)
    {

      // apply zeroth moment equation
      assertZerothMoment(iR,iZ,iEq,SGQD->energyGroup,SGQD);
      iEq = iEq + 1;


      // south face
      if (iZ == mesh->dzsCorner.size()-1)
      {
        // if on the boundary, assert boundary conditions
        assertSBC(iR,iZ,iEq,SGQD->energyGroup,SGQD);
        iEq = iEq + 1;
      } else
      {
        // otherwise assert first moment balance on south face
        applyAxialBoundary(iR,iZ,iEq,SGQD->energyGroup,SGQD);
        iEq = iEq + 1;
      }

      // east face
      if (iR == mesh->drsCorner.size()-1)
      {
        // if on the boundary, assert boundary conditions
        assertEBC(iR,iZ,iEq,SGQD->energyGroup,SGQD);
        iEq = iEq + 1;
      } else
      {
        // otherwise assert first moment balance on north face
        applyRadialBoundary(iR,iZ,iEq,SGQD->energyGroup,SGQD);
        iEq = iEq + 1;
      }

      // north face
      if (iZ == 0)
      {
        // if on the boundary, assert boundary conditions
        assertNBC(iR,iZ,iEq,SGQD->energyGroup,SGQD);
        iEq = iEq + 1;
      } 

      // west face
      if (iR == 0)
      {
        // if on the boundary, assert boundary conditions
        assertWBC(iR,iZ,iEq,SGQD->energyGroup,SGQD);
        iEq = iEq + 1;
      } 

    }
  }
};

//==============================================================================

//==============================================================================
/// Form a portion of the steady state linear system that belongs to SGQD 
/// @param [in] SGQD quasidiffusion energy group to build portion of linear 
///   for
void QDSolver::formSteadyStateLinearSystem(SingleGroupQD * SGQD)	      
{
  int iEq = SGQD->energyGroup*nGroupUnknowns;

  // loop over spatial mesh
  for (int iR = 0; iR < mesh->drsCorner.size(); iR++)
  {
    for (int iZ = 0; iZ < mesh->dzsCorner.size(); iZ++)
    {

      // apply zeroth moment equation
      assertSteadyStateZerothMoment(iR,iZ,iEq,SGQD->energyGroup,SGQD);
      iEq = iEq + 1;


      // south face
      if (iZ == mesh->dzsCorner.size()-1)
      {
        // if on the boundary, assert boundary conditions
        assertSteadyStateSBC(iR,iZ,iEq,SGQD->energyGroup,SGQD);
        iEq = iEq + 1;
      } else
      {
        // otherwise assert first moment balance on south face
        applySteadyStateAxialBoundary(iR,iZ,iEq,SGQD->energyGroup,SGQD);
        iEq = iEq + 1;
      }

      // east face
      if (iR == mesh->drsCorner.size()-1)
      {
        // if on the boundary, assert boundary conditions
        assertSteadyStateEBC(iR,iZ,iEq,SGQD->energyGroup,SGQD);
        iEq = iEq + 1;
      } else
      {
        // otherwise assert first moment balance on north face
        applySteadyStateRadialBoundary(iR,iZ,iEq,SGQD->energyGroup,SGQD);
        iEq = iEq + 1;
      }

      // north face
      if (iZ == 0)
      {
        // if on the boundary, assert boundary conditions
        assertSteadyStateNBC(iR,iZ,iEq,SGQD->energyGroup,SGQD);
        iEq = iEq + 1;
      } 

      // west face
      if (iR == 0)
      {
        // if on the boundary, assert boundary conditions
        assertSteadyStateWBC(iR,iZ,iEq,SGQD->energyGroup,SGQD);
        iEq = iEq + 1;
      } 

    }
  }
};

//==============================================================================

//==============================================================================
/// Solve linear system for multigroup quasidiffusion system
///
void QDSolver::solve()
{
  
  solveSuperLU();

};
//==============================================================================

//==============================================================================
/// Solve linear system for multigroup quasidiffusion system with an
/// iterative solver
///
void QDSolver::solveIterative()
{

  double duration,totalDuration = 0.0;
  clock_t startTime;
  int n = Eigen::nbThreads();
  int solveOutcome;
  //cout << "number procs: " << n << endl;

  if (preconditioner == iluPreconditioner) 
    solveOutcome = solveIterativeILU();
  else if (preconditioner == diagPreconditioner)
  {
    solveOutcome = solveIterativeDiag();
    if (solveOutcome != Eigen::Success)
    {
      cout << "        " << "BiCGSTAB solve failed! Attempting iterative";
      cout << " solve with ILU preconditioner." << endl;
      solveOutcome = solveIterativeILU();
    }
  }

  if (solveOutcome != Eigen::Success)
  {
    cout << "        " << "Iterative solve failed! ";
    cout << "Using SuperLU direct solve.";  
    solveSuperLU();
  }

};
//==============================================================================

//==============================================================================
/// Solve linear system for multigroup quasidiffusion system with a 
/// direct solve
///
int QDSolver::solveSuperLU()
{

  int success;

  // Declare SuperLU solver
  Eigen::SuperLU<Eigen::SparseMatrix<double>> solverLU;
  A.makeCompressed();
  solverLU.compute(A);
  x = solverLU.solve(b);

  // Return outcome of solve
  return success = solverLU.info();
  
};
//==============================================================================

//==============================================================================
/// Solve linear system for multigroup quasidiffusion system with an
/// iterative solver and incomplete LU preconditioner
///
int QDSolver::solveIterativeILU()
{

  int success;

  // Declare solver with ILUT preconditioner
  Eigen::BiCGSTAB<Eigen::SparseMatrix<double,Eigen::RowMajor>,\
    Eigen::IncompleteLUT<double> > solver;

  // Set preconditioner parameters
  solver.preconditioner().setDroptol(1E-4);
  //solver.preconditioner().setFillfactor(100);
  //solver.preconditioner().setFillfactor(5);

  // Set convergence parameters
  //solver.setTolerance(1E-14);
  //solver.setMaxIterations(20);

  // Solve system
  A.makeCompressed();
  solver.analyzePattern(A);
  solver.factorize(A);
  x = solver.solve(b);

  if (mesh->verbose) 
  {
    cout << "        ";
    cout << "info:     " << solver.info() << endl;
    cout << "        ";
    cout << "#iterations:     " << solver.iterations() << endl;
    cout << "        ";
    cout << "estimated error: " << solver.error() << endl;
    cout << "        ";
    cout << "tolerance: " << solver.tolerance() << endl;
  }

  // Return outcome of solve
  return success = solver.info();
  
};
//==============================================================================

//==============================================================================
/// Solve linear system for multigroup quasidiffusion system with an
/// iterative solver and diagonal preconditioner
///
int QDSolver::solveIterativeDiag()
{

  int success;

  // Declare solver with default diagonal precondition (cheaper to calculate) 
  // but usually requires more iterations to converge
  Eigen::BiCGSTAB<Eigen::SparseMatrix<double,Eigen::RowMajor> > solver;

  // Set convergence parameters
  //solver.setTolerance(1E-14);
  //solver.setMaxIterations(20);

  // Solve system
  A.makeCompressed();
  solver.analyzePattern(A);
  solver.factorize(A);
  x = solver.solve(b);

  if (mesh->verbose) 
  {
    cout << "        ";
    cout << "info:     " << solver.info() << endl;
    cout << "        ";
    cout << "#iterations:     " << solver.iterations() << endl;
    cout << "        ";
    cout << "estimated error: " << solver.error() << endl;
    cout << "        ";
    cout << "tolerance: " << solver.tolerance() << endl;
  }

  // Return outcome of solve
  return success = solver.info();

};
//==============================================================================

//==============================================================================
/// Compute currents from flux values in x
void QDSolver::backCalculateCurrent()
{
  currPast = d + C*x; 
}
//==============================================================================

//==============================================================================
/// Assert the zeroth moment equation for cell (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
// ToDo: eliminate energyGroup input, as it can just be defined from the SGQD
// object. Same with a lot of functions in this class
void QDSolver::assertZerothMoment(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  vector<int> indices;
  vector<double> geoParams = calcGeoParams(iR,iZ);
  double deltaT = mesh->dt;
  double v = materials->neutVel(iZ,iR,energyGroup);
  double sigT = materials->sigT(iZ,iR,energyGroup);
  double groupSourceCoeff;

  // populate entries representing sources from scattering and fission in 
  // this and other energy groups
  if (useMPQDSources)
    greyGroupSources(iR,iZ,iEq,energyGroup,geoParams);
  else
  {
    for (int iGroup = 0; iGroup < materials->nGroups; ++iGroup)
    {
      indices = getIndices(iR,iZ,iGroup);
      groupSourceCoeff = calcScatterAndFissionCoeff(iR,iZ,energyGroup,iGroup);
      A.insert(iEq,indices[iCF]) = -geoParams[iCF] * groupSourceCoeff;
    }
  }

  // populate entries representing streaming and reaction terms
  indices = getIndices(iR,iZ,energyGroup);

  A.coeffRef(iEq,indices[iCF]) += geoParams[iCF] * ((1/(v*deltaT)) + sigT);

  westCurrent(-geoParams[iWF],iR,iZ,iEq,energyGroup,SGQD);

  eastCurrent(geoParams[iEF],iR,iZ,iEq,energyGroup,SGQD);

  northCurrent(-geoParams[iNF],iR,iZ,iEq,energyGroup,SGQD);

  southCurrent(geoParams[iSF],iR,iZ,iEq,energyGroup,SGQD);

  // formulate RHS entry
  b(iEq) = b(iEq) + geoParams[iCF]*\
           ( (xPast(indices[iCF])/(v*deltaT)) + SGQD->q(iZ,iR));
};
//==============================================================================

//==============================================================================
/// Apply radial boundary for cell (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::applyRadialBoundary(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  eastCurrent(1,iR,iZ,iEq,energyGroup,SGQD);
  westCurrent(-1,iR+1,iZ,iEq,energyGroup,SGQD);
}
//==============================================================================

//==============================================================================
/// Apply axial boundary for cell (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::applyAxialBoundary(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  northCurrent(1,iR,iZ+1,iEq,energyGroup,SGQD);
  southCurrent(-1,iR,iZ,iEq,energyGroup,SGQD);
}
//==============================================================================

//==============================================================================
/// Assert steady state zeroth moment equation for cell (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
// ToDo: eliminate energyGroup input, as it can just be defined from the SGQD
// object. Same with a lot of functions in this class
void QDSolver::assertSteadyStateZerothMoment(int iR,int iZ,int iEq,\
    int energyGroup,SingleGroupQD * SGQD)
{
  vector<int> indices;
  vector<double> geoParams = calcGeoParams(iR,iZ);
  double deltaT = mesh->dt;
  double v = materials->neutVel(iZ,iR,energyGroup);
  double sigT = materials->sigT(iZ,iR,energyGroup);
  double groupSourceCoeff;

  // populate entries representing sources from scattering and fission in 
  // this and other energy groups
  if (useMPQDSources)
    steadyStateGreyGroupSources(iR,iZ,iEq,energyGroup,geoParams);
  else
  {
    for (int iGroup = 0; iGroup < materials->nGroups; ++iGroup)
    {
      indices = getIndices(iR,iZ,iGroup);
      groupSourceCoeff = calcScatterAndFissionCoeff(iR,iZ,energyGroup,iGroup);
      A.insert(iEq,indices[iCF]) = -geoParams[iCF] * groupSourceCoeff;
    }
  }

  // populate entries representing streaming and reaction terms
  indices = getIndices(iR,iZ,energyGroup);

  A.coeffRef(iEq,indices[iCF]) += geoParams[iCF] * sigT;

  steadyStateWestCurrent(-geoParams[iWF],iR,iZ,iEq,energyGroup,SGQD);

  steadyStateEastCurrent(geoParams[iEF],iR,iZ,iEq,energyGroup,SGQD);

  steadyStateNorthCurrent(-geoParams[iNF],iR,iZ,iEq,energyGroup,SGQD);

  steadyStateSouthCurrent(geoParams[iSF],iR,iZ,iEq,energyGroup,SGQD);

  // external source entry
  b(iEq) = b(iEq) + geoParams[iCF] * (SGQD->q(iZ,iR));
};
//==============================================================================

//==============================================================================
/// Apply steady state radial boundary for cell (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::applySteadyStateRadialBoundary(int iR,int iZ,int iEq,\
    int energyGroup,SingleGroupQD * SGQD)
{
  steadyStateEastCurrent(1,iR,iZ,iEq,energyGroup,SGQD);
  steadyStateWestCurrent(-1,iR+1,iZ,iEq,energyGroup,SGQD);
}
//==============================================================================

//==============================================================================
/// Apply steady state axial boundary for cell (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::applySteadyStateAxialBoundary(int iR,int iZ,int iEq,\
    int energyGroup,SingleGroupQD * SGQD)
{
  steadyStateNorthCurrent(1,iR,iZ+1,iEq,energyGroup,SGQD);
  steadyStateSouthCurrent(-1,iR,iZ,iEq,energyGroup,SGQD);
}
//==============================================================================

//==============================================================================
/// Enforce coefficients for current on south face
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::southCurrent(double coeff,int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  vector<int> indices;
  vector<double> geoParams = calcGeoParams(iR,iZ);
  double deltaT = mesh->dt;
  double v = materials->neutVel(iZ,iR,energyGroup);
  double sigT = materials->sigT(iZ,iR,energyGroup);
  double rUp,rDown,zUp,zDown,rAvg,zAvg,deltaR,deltaZ;  
  double EzzC,EzzS,ErzW,ErzE;

  // calculate geometric values
  rUp = mesh->rCornerEdge(iR+1); rDown = mesh->rCornerEdge(iR);
  zUp = mesh->zCornerEdge(iZ+1); zDown = mesh->zCornerEdge(iZ);
  rAvg = calcVolAvgR(rDown,rUp); zAvg = (zUp + zDown)/2;
  deltaR = rUp-rDown; deltaZ = zUp-zAvg;

  // get local Eddington factors
  EzzC = SGQD->Ezz(iZ,iR);
  EzzS = SGQD->EzzAxial(iZ+1,iR); 
  ErzW = SGQD->ErzRadial(iZ,iR);
  ErzE = SGQD->ErzRadial(iZ,iR+1);

  // populate entries representing streaming and reaction terms
  indices = getIndices(iR,iZ,energyGroup);

  coeff = coeff/((1/(v*deltaT))+sigT); 

  A.coeffRef(iEq,indices[iSF]) -= coeff*EzzS/deltaZ;

  A.coeffRef(iEq,indices[iCF]) += coeff*EzzC/deltaZ;

  A.coeffRef(iEq,indices[iWF]) += coeff*(rDown*ErzW/(rAvg*deltaR));

  A.coeffRef(iEq,indices[iEF]) -= coeff*rUp*ErzE/(rAvg*deltaR);

  // formulate RHS entry
  b(iEq) = b(iEq) - coeff*(currPast(indices[iSC])/(v*deltaT));
};
//==============================================================================

//==============================================================================
/// Enforce coefficients for current on north face 
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::northCurrent(double coeff,int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  vector<int> indices;
  vector<double> geoParams = calcGeoParams(iR,iZ);
  double deltaT = mesh->dt;
  double v = materials->neutVel(iZ,iR,energyGroup);
  double sigT = materials->sigT(iZ,iR,energyGroup);
  double rUp,rDown,zUp,zDown,rAvg,zAvg,deltaR,deltaZ;  
  double EzzC,EzzN,ErzW,ErzE;

  // calculate geometric values
  rUp = mesh->rCornerEdge(iR+1); rDown = mesh->rCornerEdge(iR);
  zUp = mesh->zCornerEdge(iZ+1); zDown = mesh->zCornerEdge(iZ);
  rAvg = calcVolAvgR(rDown,rUp); zAvg = (zUp + zDown)/2;
  deltaR = rUp-rDown; deltaZ = zAvg-zDown;

  // get local Eddington factors
  EzzC = SGQD->Ezz(iZ,iR);
  EzzN = SGQD->EzzAxial(iZ,iR); 
  ErzW = SGQD->ErzRadial(iZ,iR);
  ErzE = SGQD->ErzRadial(iZ,iR+1);

  // populate entries representing streaming and reaction terms
  indices = getIndices(iR,iZ,energyGroup);

  coeff = coeff/((1/(v*deltaT))+sigT); 

  A.coeffRef(iEq,indices[iNF]) += coeff*EzzN/deltaZ;

  A.coeffRef(iEq,indices[iCF]) -= coeff*EzzC/deltaZ;

  A.coeffRef(iEq,indices[iWF]) += coeff*(rDown*ErzW/(rAvg*deltaR));

  A.coeffRef(iEq,indices[iEF]) -= coeff*rUp*ErzE/(rAvg*deltaR);

  // formulate RHS entry
  b(iEq) = b(iEq) - coeff*(currPast(indices[iNC])/(v*deltaT));
};
//==============================================================================

//==============================================================================
/// Enforce coefficients for current on west face
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::westCurrent(double coeff,int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  vector<int> indices;
  vector<double> geoParams = calcGeoParams(iR,iZ);
  double deltaT = mesh->dt;
  double v = materials->neutVel(iZ,iR,energyGroup);
  double sigT = materials->sigT(iZ,iR,energyGroup);
  double rUp,rDown,zUp,zDown,rAvg,zAvg,deltaR,deltaZ;  
  double hCent,hDown;
  double ErzN,ErzS,ErrC,ErrW;  

  // calculate geometric values
  rUp = mesh->rCornerEdge(iR+1); rDown = mesh->rCornerEdge(iR);
  zUp = mesh->zCornerEdge(iZ+1); zDown = mesh->zCornerEdge(iZ);
  rAvg = calcVolAvgR(rDown,rUp); zAvg = (zUp + zDown)/2;
  deltaR = rAvg-rDown; deltaZ = zUp-zDown;
  hCent = calcIntegratingFactor(iR,iZ,rAvg,SGQD);
  hDown = calcIntegratingFactor(iR,iZ,rDown,SGQD);

  // get local Eddington factors
  ErrC = SGQD->Err(iZ,iR);
  ErrW = SGQD->ErrRadial(iZ,iR); 
  ErzN = SGQD->ErzAxial(iZ,iR);
  ErzS = SGQD->ErzAxial(iZ+1,iR);
  
  // populate entries representing streaming and reaction terms
  indices = getIndices(iR,iZ,energyGroup);

  coeff = coeff/((1/(v*deltaT))+sigT); 

  A.coeffRef(iEq,indices[iSF]) -= coeff*ErzS/deltaZ;

  A.coeffRef(iEq,indices[iNF]) += coeff*ErzN/deltaZ;

  A.coeffRef(iEq,indices[iCF]) -= coeff*hCent*ErrC/(hDown*deltaR);

  A.coeffRef(iEq,indices[iWF]) += coeff*hDown*ErrW/(hDown*deltaR);

  // formulate RHS entry
  b(iEq) = b(iEq) - coeff*(currPast(indices[iWC])/(v*deltaT));
};
//==============================================================================

//==============================================================================
/// Enforce coefficients for current on east face
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::eastCurrent(double coeff,int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  vector<int> indices;
  vector<double> geoParams = calcGeoParams(iR,iZ);
  double deltaT = mesh->dt;
  double v = materials->neutVel(iZ,iR,energyGroup);
  double sigT = materials->sigT(iZ,iR,energyGroup);
  double rUp,rDown,zUp,zDown,rAvg,zAvg,deltaR,deltaZ;  
  double hCent,hUp;
  double ErzN,ErzS,ErrC,ErrE;  

  // calculate geometric values
  rUp = mesh->rCornerEdge(iR+1); rDown = mesh->rCornerEdge(iR);
  zUp = mesh->zCornerEdge(iZ+1); zDown = mesh->zCornerEdge(iZ);
  rAvg = calcVolAvgR(rDown,rUp); zAvg = (zUp + zDown)/2;
  deltaR = rUp-rAvg; deltaZ = zUp-zDown;
  hCent = calcIntegratingFactor(iR,iZ,rAvg,SGQD);
  hUp = calcIntegratingFactor(iR,iZ,rUp,SGQD);

  // get local Eddington factors
  ErrC = SGQD->Err(iZ,iR);
  ErrE = SGQD->ErrRadial(iZ,iR+1); 
  ErzN = SGQD->ErzAxial(iZ,iR);
  ErzS = SGQD->ErzAxial(iZ+1,iR);
  
  // populate entries representing streaming and reaction terms
  indices = getIndices(iR,iZ,energyGroup);

  coeff = coeff/((1/(v*deltaT))+sigT); 

  A.coeffRef(iEq,indices[iSF]) -= coeff*ErzS/deltaZ;

  A.coeffRef(iEq,indices[iNF]) += coeff*ErzN/deltaZ;

  A.coeffRef(iEq,indices[iCF]) += coeff*hCent*ErrC/(hUp*deltaR);

  A.coeffRef(iEq,indices[iEF]) -= coeff*hUp*ErrE/(hUp*deltaR);

  // formulate RHS entry
  b(iEq) = b(iEq) - coeff*(currPast(indices[iEC])/(v*deltaT));
};
//==============================================================================

//==============================================================================
/// Enforce coefficients for steady state current on south face
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::steadyStateSouthCurrent(double coeff,int iR,int iZ,int iEq,\
    int energyGroup, SingleGroupQD * SGQD)
{
  vector<int> indices;
  vector<double> geoParams = calcGeoParams(iR,iZ);
  double deltaT = mesh->dt;
  double v = materials->neutVel(iZ,iR,energyGroup);
  double sigT = materials->zSigT(iZ+1,iR,energyGroup);
  double rUp,rDown,zUp,zDown,rAvg,zAvg,deltaR,deltaZ;  
  double EzzC,EzzS,ErzW,ErzE;

  // calculate geometric values
  rUp = mesh->rCornerEdge(iR+1); rDown = mesh->rCornerEdge(iR);
  zUp = mesh->zCornerEdge(iZ+1); zDown = mesh->zCornerEdge(iZ);
  rAvg = calcVolAvgR(rDown,rUp); zAvg = (zUp + zDown)/2;
  deltaR = rUp-rDown; deltaZ = zUp-zAvg;

  // get local Eddington factors
  EzzC = SGQD->Ezz(iZ,iR);
  EzzS = SGQD->EzzAxial(iZ+1,iR); 
  ErzW = SGQD->ErzRadial(iZ,iR);
  ErzE = SGQD->ErzRadial(iZ,iR+1);

  // populate entries representing streaming and reaction terms
  indices = getIndices(iR,iZ,energyGroup);

  coeff = coeff/(sigT); 

  A.coeffRef(iEq,indices[iSF]) -= coeff*EzzS/deltaZ;

  A.coeffRef(iEq,indices[iCF]) += coeff*EzzC/deltaZ;

  A.coeffRef(iEq,indices[iWF]) += coeff*(rDown*ErzW/(rAvg*deltaR));

  A.coeffRef(iEq,indices[iEF]) -= coeff*rUp*ErzE/(rAvg*deltaR);

};
//==============================================================================

//==============================================================================
/// Enforce coefficients for steady state current on north face 
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::steadyStateNorthCurrent(double coeff,int iR,int iZ,int iEq,\
    int energyGroup, SingleGroupQD * SGQD)
{
  vector<int> indices;
  vector<double> geoParams = calcGeoParams(iR,iZ);
  double deltaT = mesh->dt;
  double v = materials->neutVel(iZ,iR,energyGroup);
  double sigT = materials->zSigT(iZ,iR,energyGroup);
  double rUp,rDown,zUp,zDown,rAvg,zAvg,deltaR,deltaZ;  
  double EzzC,EzzN,ErzW,ErzE;

  // calculate geometric values
  rUp = mesh->rCornerEdge(iR+1); rDown = mesh->rCornerEdge(iR);
  zUp = mesh->zCornerEdge(iZ+1); zDown = mesh->zCornerEdge(iZ);
  rAvg = calcVolAvgR(rDown,rUp); zAvg = (zUp + zDown)/2;
  deltaR = rUp-rDown; deltaZ = zAvg-zDown;

  // get local Eddington factors
  EzzC = SGQD->Ezz(iZ,iR);
  EzzN = SGQD->EzzAxial(iZ,iR); 
  ErzW = SGQD->ErzRadial(iZ,iR);
  ErzE = SGQD->ErzRadial(iZ,iR+1);

  // populate entries representing streaming and reaction terms
  indices = getIndices(iR,iZ,energyGroup);

  coeff = coeff/(sigT); 

  A.coeffRef(iEq,indices[iNF]) += coeff*EzzN/deltaZ;

  A.coeffRef(iEq,indices[iCF]) -= coeff*EzzC/deltaZ;

  A.coeffRef(iEq,indices[iWF]) += coeff*(rDown*ErzW/(rAvg*deltaR));

  A.coeffRef(iEq,indices[iEF]) -= coeff*rUp*ErzE/(rAvg*deltaR);

};
//==============================================================================

//==============================================================================
/// Enforce coefficients for steady state current on west face
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::steadyStateWestCurrent(double coeff,int iR,int iZ,int iEq,
    int energyGroup,SingleGroupQD * SGQD)
{
  vector<int> indices;
  vector<double> geoParams = calcGeoParams(iR,iZ);
  double deltaT = mesh->dt;
  double v = materials->neutVel(iZ,iR,energyGroup);
  double sigT = materials->rSigT(iZ,iR,energyGroup);
  double rUp,rDown,zUp,zDown,rAvg,zAvg,deltaR,deltaZ;  
  double hCent,hDown;
  double ErzN,ErzS,ErrC,ErrW;  

  // calculate geometric values
  rUp = mesh->rCornerEdge(iR+1); rDown = mesh->rCornerEdge(iR);
  zUp = mesh->zCornerEdge(iZ+1); zDown = mesh->zCornerEdge(iZ);
  rAvg = calcVolAvgR(rDown,rUp); zAvg = (zUp + zDown)/2;
  deltaR = rAvg-rDown; deltaZ = zUp-zDown;
  hCent = calcIntegratingFactor(iR,iZ,rAvg,SGQD);
  hDown = calcIntegratingFactor(iR,iZ,rDown,SGQD);

  // get local Eddington factors
  ErrC = SGQD->Err(iZ,iR);
  ErrW = SGQD->ErrRadial(iZ,iR); 
  ErzN = SGQD->ErzAxial(iZ,iR);
  ErzS = SGQD->ErzAxial(iZ+1,iR);
  
  // populate entries representing streaming and reaction terms
  indices = getIndices(iR,iZ,energyGroup);

  coeff = coeff/(sigT); 

  A.coeffRef(iEq,indices[iSF]) -= coeff*ErzS/deltaZ;

  A.coeffRef(iEq,indices[iNF]) += coeff*ErzN/deltaZ;

  A.coeffRef(iEq,indices[iCF]) -= coeff*hCent*ErrC/(hDown*deltaR);

  A.coeffRef(iEq,indices[iWF]) += coeff*hDown*ErrW/(hDown*deltaR);

};
//==============================================================================

//==============================================================================
/// Enforce coefficients for steady state current on east face
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::steadyStateEastCurrent(double coeff,int iR,int iZ,int iEq,\
    int energyGroup, SingleGroupQD * SGQD)
{
  vector<int> indices;
  vector<double> geoParams = calcGeoParams(iR,iZ);
  double deltaT = mesh->dt;
  double v = materials->neutVel(iZ,iR,energyGroup);
  double sigT = materials->rSigT(iZ,iR+1,energyGroup);
  double rUp,rDown,zUp,zDown,rAvg,zAvg,deltaR,deltaZ;  
  double hCent,hUp;
  double ErzN,ErzS,ErrC,ErrE;  

  // calculate geometric values
  rUp = mesh->rCornerEdge(iR+1); rDown = mesh->rCornerEdge(iR);
  zUp = mesh->zCornerEdge(iZ+1); zDown = mesh->zCornerEdge(iZ);
  rAvg = calcVolAvgR(rDown,rUp); zAvg = (zUp + zDown)/2;
  deltaR = rUp-rAvg; deltaZ = zUp-zDown;
  hCent = calcIntegratingFactor(iR,iZ,rAvg,SGQD);
  hUp = calcIntegratingFactor(iR,iZ,rUp,SGQD);

  // get local Eddington factors
  ErrC = SGQD->Err(iZ,iR);
  ErrE = SGQD->ErrRadial(iZ,iR+1); 
  ErzN = SGQD->ErzAxial(iZ,iR);
  ErzS = SGQD->ErzAxial(iZ+1,iR);
  
  // populate entries representing streaming and reaction terms
  indices = getIndices(iR,iZ,energyGroup);

  coeff = coeff/(sigT); 

  A.coeffRef(iEq,indices[iSF]) -= coeff*ErzS/deltaZ;

  A.coeffRef(iEq,indices[iNF]) += coeff*ErzN/deltaZ;

  A.coeffRef(iEq,indices[iCF]) += coeff*hCent*ErrC/(hUp*deltaR);

  A.coeffRef(iEq,indices[iEF]) -= coeff*hUp*ErrE/(hUp*deltaR);

};
//==============================================================================

//==============================================================================
/// Form a portion of the current back calc linear system that belongs to SGQD 
/// @param [in] SGQD quasidiffusion energy group to build portion of linear 
///   for
void QDSolver::formBackCalcSystem(SingleGroupQD * SGQD)	      
{
  int iEq = SGQD->energyGroup*nGroupCurrentUnknowns;

  // loop over spatial mesh
  for (int iR = 0; iR < mesh->drsCorner.size(); iR++)
  {
    for (int iZ = 0; iZ < mesh->dzsCorner.size(); iZ++)
    {

      // south face
      calcSouthCurrent(iR,iZ,iEq,SGQD->energyGroup,SGQD);
      iEq = iEq + 1;

      // east face
      calcEastCurrent(iR,iZ,iEq,SGQD->energyGroup,SGQD);
      iEq = iEq + 1;

      // north face
      if (iZ == 0)
      {
        calcNorthCurrent(iR,iZ,iEq,SGQD->energyGroup,SGQD);
        iEq = iEq + 1;
      } 

      // west face
      if (iR == 0)
      {
        // if on the boundary, assert boundary conditions
        calcWestCurrent(iR,iZ,iEq,SGQD->energyGroup,SGQD);
        iEq = iEq + 1;
      } 

    }
  }
};

//==============================================================================

//==============================================================================
/// Form a portion of the steady state current back calc linear system that 
/// belongs to SGQD 
/// @param [in] SGQD quasidiffusion energy group to build portion of linear 
///   for
void QDSolver::formSteadyStateBackCalcSystem(SingleGroupQD * SGQD)	      
{
  int iEq = SGQD->energyGroup*nGroupCurrentUnknowns;

  // loop over spatial mesh
  for (int iR = 0; iR < mesh->drsCorner.size(); iR++)
  {
    for (int iZ = 0; iZ < mesh->dzsCorner.size(); iZ++)
    {

      // south face
      calcSteadyStateSouthCurrent(iR,iZ,iEq,SGQD->energyGroup,SGQD);
      iEq = iEq + 1;

      // east face
      calcSteadyStateEastCurrent(iR,iZ,iEq,SGQD->energyGroup,SGQD);
      iEq = iEq + 1;

      // north face
      if (iZ == 0)
      {
        calcSteadyStateNorthCurrent(iR,iZ,iEq,SGQD->energyGroup,SGQD);
        iEq = iEq + 1;
      } 

      // west face
      if (iR == 0)
      {
        // if on the boundary, assert boundary conditions
        calcSteadyStateWestCurrent(iR,iZ,iEq,SGQD->energyGroup,SGQD);
        iEq = iEq + 1;
      } 

    }
  }
};

//==============================================================================

//==============================================================================
/// Enforce coefficients to calculate current on south face
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::calcSouthCurrent(int iR,int iZ,int iEq,\
    int energyGroup,SingleGroupQD * SGQD)
{
  vector<int> indices;
  vector<double> geoParams = calcGeoParams(iR,iZ);
  double deltaT = mesh->dt;
  double v = materials->neutVel(iZ,iR,energyGroup);
  double sigT = materials->sigT(iZ,iR,energyGroup);
  double rUp,rDown,zUp,zDown,rAvg,zAvg,deltaR,deltaZ,coeff;  
  double EzzC,EzzS,ErzW,ErzE;

  // calculate geometric values
  rUp = mesh->rCornerEdge(iR+1); rDown = mesh->rCornerEdge(iR);
  zUp = mesh->zCornerEdge(iZ+1); zDown = mesh->zCornerEdge(iZ);
  rAvg = calcVolAvgR(rDown,rUp); zAvg = (zUp + zDown)/2;
  deltaR = rUp-rDown; deltaZ = zUp-zAvg;

  // get local Eddington factors
  EzzC = SGQD->Ezz(iZ,iR);
  EzzS = SGQD->EzzAxial(iZ+1,iR); 
  ErzW = SGQD->ErzRadial(iZ,iR);
  ErzE = SGQD->ErzRadial(iZ,iR+1);
  
  // populate entries representing streaming and reaction terms
  indices = getIndices(iR,iZ,energyGroup);

  coeff = 1/((1/(v*deltaT))+sigT); 

  C.coeffRef(iEq,indices[iSF]) -= coeff*EzzS/deltaZ;

  C.coeffRef(iEq,indices[iCF]) += coeff*EzzC/deltaZ;

  C.coeffRef(iEq,indices[iWF]) += coeff*(rDown*ErzW/(rAvg*deltaR));

  C.coeffRef(iEq,indices[iEF]) -= coeff*rUp*ErzE/(rAvg*deltaR);

  // formulate RHS entry
  d(iEq) = coeff*(currPast(indices[iSC])/(v*deltaT));
};
//==============================================================================

//==============================================================================
/// Enforce coefficients to calculate current on north face 
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::calcNorthCurrent(int iR,int iZ,int iEq,\
    int energyGroup,SingleGroupQD * SGQD)
{
  vector<int> indices;
  vector<double> geoParams = calcGeoParams(iR,iZ);
  double deltaT = mesh->dt;
  double v = materials->neutVel(iZ,iR,energyGroup);
  double sigT = materials->sigT(iZ,iR,energyGroup);
  double rUp,rDown,zUp,zDown,rAvg,zAvg,deltaR,deltaZ,coeff;  
  double EzzC,EzzN,ErzW,ErzE;

  // calculate geometric values
  rUp = mesh->rCornerEdge(iR+1); rDown = mesh->rCornerEdge(iR);
  zUp = mesh->zCornerEdge(iZ+1); zDown = mesh->zCornerEdge(iZ);
  rAvg = calcVolAvgR(rDown,rUp); zAvg = (zUp + zDown)/2;
  deltaR = rUp-rDown; deltaZ = zAvg-zDown;

  // get local Eddington factors
  EzzC = SGQD->Ezz(iZ,iR);
  EzzN = SGQD->EzzAxial(iZ,iR); 
  ErzW = SGQD->ErzRadial(iZ,iR);
  ErzE = SGQD->ErzRadial(iZ,iR+1);

  // populate entries representing streaming and reaction terms
  indices = getIndices(iR,iZ,energyGroup);

  coeff = 1/((1/(v*deltaT))+sigT); 

  C.coeffRef(iEq,indices[iNF]) += coeff*EzzN/deltaZ;

  C.coeffRef(iEq,indices[iCF]) -= coeff*EzzC/deltaZ;

  C.coeffRef(iEq,indices[iWF]) += coeff*(rDown*ErzW/(rAvg*deltaR));

  C.coeffRef(iEq,indices[iEF]) -= coeff*rUp*ErzE/(rAvg*deltaR);

  // formulate RHS entry
  d(iEq) = coeff*(currPast(indices[iNC])/(v*deltaT));
};
//==============================================================================

//==============================================================================
/// Enforce coefficients to calculate current on west face
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::calcWestCurrent(int iR,int iZ,int iEq,\
    int energyGroup,SingleGroupQD * SGQD)
{
  vector<int> indices;
  vector<double> geoParams = calcGeoParams(iR,iZ);
  double deltaT = mesh->dt;
  double v = materials->neutVel(iZ,iR,energyGroup);
  double sigT = materials->sigT(iZ,iR,energyGroup);
  double rUp,rDown,zUp,zDown,rAvg,zAvg,deltaR,deltaZ,coeff;  
  double hCent,hDown;
  double ErzN,ErzS,ErrC,ErrW;  

  // calculate geometric values
  rUp = mesh->rCornerEdge(iR+1); rDown = mesh->rCornerEdge(iR);
  zUp = mesh->zCornerEdge(iZ+1); zDown = mesh->zCornerEdge(iZ);
  rAvg = calcVolAvgR(rDown,rUp); zAvg = (zUp + zDown)/2;
  deltaR = rAvg-rDown; deltaZ = zUp-zDown;
  hCent = calcIntegratingFactor(iR,iZ,rAvg,SGQD);
  hDown = calcIntegratingFactor(iR,iZ,rDown,SGQD);

  // get local Eddington factors
  ErrC = SGQD->Err(iZ,iR);
  ErrW = SGQD->ErrRadial(iZ,iR); 
  ErzN = SGQD->ErzAxial(iZ,iR);
  ErzS = SGQD->ErzAxial(iZ+1,iR);

  // populate entries representing streaming and reaction terms
  indices = getIndices(iR,iZ,energyGroup);

  coeff = 1/((1/(v*deltaT))+sigT); 

  C.coeffRef(iEq,indices[iSF]) -= coeff*ErzS/deltaZ;

  C.coeffRef(iEq,indices[iNF]) += coeff*ErzN/deltaZ;

  C.coeffRef(iEq,indices[iCF]) -= coeff*hCent*ErrC/(hDown*deltaR);

  C.coeffRef(iEq,indices[iWF]) += coeff*hDown*ErrW/(hDown*deltaR);

  // formulate RHS entry
  d(iEq) = coeff*(currPast(indices[iWC])/(v*deltaT));
};
//==============================================================================

//==============================================================================
/// Enforce coefficients to calculate current on east face
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::calcEastCurrent(int iR,int iZ,int iEq,\
    int energyGroup,SingleGroupQD * SGQD)
{
  vector<int> indices;
  vector<double> geoParams = calcGeoParams(iR,iZ);
  double deltaT = mesh->dt;
  double v = materials->neutVel(iZ,iR,energyGroup);
  double sigT = materials->sigT(iZ,iR,energyGroup);
  double rUp,rDown,zUp,zDown,rAvg,zAvg,deltaR,deltaZ,coeff;  
  double hCent,hUp;
  double ErzN,ErzS,ErrC,ErrE;  

  // calculate geometric values
  rUp = mesh->rCornerEdge(iR+1); rDown = mesh->rCornerEdge(iR);
  zUp = mesh->zCornerEdge(iZ+1); zDown = mesh->zCornerEdge(iZ);
  rAvg = calcVolAvgR(rDown,rUp); zAvg = (zUp + zDown)/2;
  deltaR = rUp-rAvg; deltaZ = zUp-zDown;
  hCent = calcIntegratingFactor(iR,iZ,rAvg,SGQD);
  hUp = calcIntegratingFactor(iR,iZ,rUp,SGQD);

  // get local Eddington factors
  ErrC = SGQD->Err(iZ,iR);
  ErrE = SGQD->ErrRadial(iZ,iR+1); 
  ErzN = SGQD->ErzAxial(iZ,iR);
  ErzS = SGQD->ErzAxial(iZ+1,iR);

  // populate entries representing streaming and reaction terms
  indices = getIndices(iR,iZ,energyGroup);

  coeff = 1/((1/(v*deltaT))+sigT); 

  C.coeffRef(iEq,indices[iSF]) -= coeff*ErzS/deltaZ;

  C.coeffRef(iEq,indices[iNF]) += coeff*ErzN/deltaZ;

  C.coeffRef(iEq,indices[iCF]) += coeff*hCent*ErrC/(hUp*deltaR);

  C.coeffRef(iEq,indices[iEF]) -= coeff*hUp*ErrE/(hUp*deltaR);

  // formulate RHS entry
  d(iEq) = coeff*(currPast(indices[iEC])/(v*deltaT));
};
//==============================================================================

//==============================================================================
/// Enforce coefficients to calculate steady state current on south face
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::calcSteadyStateSouthCurrent(int iR,int iZ,int iEq,\
    int energyGroup,SingleGroupQD * SGQD)
{
  vector<int> indices;
  vector<double> geoParams = calcGeoParams(iR,iZ);
  double deltaT = mesh->dt;
  double v = materials->neutVel(iZ,iR,energyGroup);
  double sigT = materials->zSigT(iZ+1,iR,energyGroup);
  double rUp,rDown,zUp,zDown,rAvg,zAvg,deltaR,deltaZ,coeff;  
  double EzzC,EzzS,ErzW,ErzE;

  // calculate geometric values
  rUp = mesh->rCornerEdge(iR+1); rDown = mesh->rCornerEdge(iR);
  zUp = mesh->zCornerEdge(iZ+1); zDown = mesh->zCornerEdge(iZ);
  rAvg = calcVolAvgR(rDown,rUp); zAvg = (zUp + zDown)/2;
  deltaR = rUp-rDown; deltaZ = zUp-zAvg;

  // get local Eddington factors
  EzzC = SGQD->Ezz(iZ,iR);
  EzzS = SGQD->EzzAxial(iZ+1,iR); 
  ErzW = SGQD->ErzRadial(iZ,iR);
  ErzE = SGQD->ErzRadial(iZ,iR+1);
  
  // populate entries representing streaming and reaction terms
  indices = getIndices(iR,iZ,energyGroup);

  coeff = 1/(sigT); 

  C.coeffRef(iEq,indices[iSF]) -= coeff*EzzS/deltaZ;

  C.coeffRef(iEq,indices[iCF]) += coeff*EzzC/deltaZ;

  C.coeffRef(iEq,indices[iWF]) += coeff*(rDown*ErzW/(rAvg*deltaR));

  C.coeffRef(iEq,indices[iEF]) -= coeff*rUp*ErzE/(rAvg*deltaR);

};
//==============================================================================

//==============================================================================
/// Enforce coefficients to calculate steady state current on north face 
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::calcSteadyStateNorthCurrent(int iR,int iZ,int iEq,\
    int energyGroup,SingleGroupQD * SGQD)
{
  vector<int> indices;
  vector<double> geoParams = calcGeoParams(iR,iZ);
  double deltaT = mesh->dt;
  double v = materials->neutVel(iZ,iR,energyGroup);
  double sigT = materials->zSigT(iZ,iR,energyGroup);
  double rUp,rDown,zUp,zDown,rAvg,zAvg,deltaR,deltaZ,coeff;  
  double EzzC,EzzN,ErzW,ErzE;

  // calculate geometric values
  rUp = mesh->rCornerEdge(iR+1); rDown = mesh->rCornerEdge(iR);
  zUp = mesh->zCornerEdge(iZ+1); zDown = mesh->zCornerEdge(iZ);
  rAvg = calcVolAvgR(rDown,rUp); zAvg = (zUp + zDown)/2;
  deltaR = rUp-rDown; deltaZ = zAvg-zDown;

  // get local Eddington factors
  EzzC = SGQD->Ezz(iZ,iR);
  EzzN = SGQD->EzzAxial(iZ,iR); 
  ErzW = SGQD->ErzRadial(iZ,iR);
  ErzE = SGQD->ErzRadial(iZ,iR+1);

  // populate entries representing streaming and reaction terms
  indices = getIndices(iR,iZ,energyGroup);

  coeff = 1/(sigT); 

  C.coeffRef(iEq,indices[iNF]) += coeff*EzzN/deltaZ;

  C.coeffRef(iEq,indices[iCF]) -= coeff*EzzC/deltaZ;

  C.coeffRef(iEq,indices[iWF]) += coeff*(rDown*ErzW/(rAvg*deltaR));

  C.coeffRef(iEq,indices[iEF]) -= coeff*rUp*ErzE/(rAvg*deltaR);

};
//==============================================================================

//==============================================================================
/// Enforce coefficients to calculate steady state current on west face
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::calcSteadyStateWestCurrent(int iR,int iZ,int iEq,\
    int energyGroup,SingleGroupQD * SGQD)
{
  vector<int> indices;
  vector<double> geoParams = calcGeoParams(iR,iZ);
  double deltaT = mesh->dt;
  double v = materials->neutVel(iZ,iR,energyGroup);
  double sigT = materials->rSigT(iZ,iR,energyGroup);
  double rUp,rDown,zUp,zDown,rAvg,zAvg,deltaR,deltaZ,coeff;  
  double hCent,hDown;
  double ErzN,ErzS,ErrC,ErrW;  

  // calculate geometric values
  rUp = mesh->rCornerEdge(iR+1); rDown = mesh->rCornerEdge(iR);
  zUp = mesh->zCornerEdge(iZ+1); zDown = mesh->zCornerEdge(iZ);
  rAvg = calcVolAvgR(rDown,rUp); zAvg = (zUp + zDown)/2;
  deltaR = rAvg-rDown; deltaZ = zUp-zDown;
  hCent = calcIntegratingFactor(iR,iZ,rAvg,SGQD);
  hDown = calcIntegratingFactor(iR,iZ,rDown,SGQD);

  // get local Eddington factors
  ErrC = SGQD->Err(iZ,iR);
  ErrW = SGQD->ErrRadial(iZ,iR); 
  ErzN = SGQD->ErzAxial(iZ,iR);
  ErzS = SGQD->ErzAxial(iZ+1,iR);

  // populate entries representing streaming and reaction terms
  indices = getIndices(iR,iZ,energyGroup);

  coeff = 1/(sigT); 

  C.coeffRef(iEq,indices[iSF]) -= coeff*ErzS/deltaZ;

  C.coeffRef(iEq,indices[iNF]) += coeff*ErzN/deltaZ;

  C.coeffRef(iEq,indices[iCF]) -= coeff*hCent*ErrC/(hDown*deltaR);

  C.coeffRef(iEq,indices[iWF]) += coeff*hDown*ErrW/(hDown*deltaR);

};
//==============================================================================

//==============================================================================
/// Enforce coefficients to calculate steady state current on east face
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert equation for
/// @param [in] SGQD of this energyGroup
void QDSolver::calcSteadyStateEastCurrent(int iR,int iZ,int iEq,\
    int energyGroup,SingleGroupQD * SGQD)
{
  vector<int> indices;
  vector<double> geoParams = calcGeoParams(iR,iZ);
  double deltaT = mesh->dt;
  double v = materials->neutVel(iZ,iR,energyGroup);
  double sigT = materials->rSigT(iZ,iR+1,energyGroup);
  double rUp,rDown,zUp,zDown,rAvg,zAvg,deltaR,deltaZ,coeff;  
  double hCent,hUp;
  double ErzN,ErzS,ErrC,ErrE;  

  // calculate geometric values
  rUp = mesh->rCornerEdge(iR+1); rDown = mesh->rCornerEdge(iR);
  zUp = mesh->zCornerEdge(iZ+1); zDown = mesh->zCornerEdge(iZ);
  rAvg = calcVolAvgR(rDown,rUp); zAvg = (zUp + zDown)/2;
  deltaR = rUp-rAvg; deltaZ = zUp-zDown;
  hCent = calcIntegratingFactor(iR,iZ,rAvg,SGQD);
  hUp = calcIntegratingFactor(iR,iZ,rUp,SGQD);

  // get local Eddington factors
  ErrC = SGQD->Err(iZ,iR);
  ErrE = SGQD->ErrRadial(iZ,iR+1); 
  ErzN = SGQD->ErzAxial(iZ,iR);
  ErzS = SGQD->ErzAxial(iZ+1,iR);

  // populate entries representing streaming and reaction terms
  indices = getIndices(iR,iZ,energyGroup);

  coeff = 1/(sigT); 

  C.coeffRef(iEq,indices[iSF]) -= coeff*ErzS/deltaZ;

  C.coeffRef(iEq,indices[iNF]) += coeff*ErzN/deltaZ;

  C.coeffRef(iEq,indices[iCF]) += coeff*hCent*ErrC/(hUp*deltaR);

  C.coeffRef(iEq,indices[iEF]) -= coeff*hUp*ErrE/(hUp*deltaR);

};
//==============================================================================

//==============================================================================
/// Assert the flux boundary condition on the north face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertNFluxBC(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  vector<int> indices = getIndices(iR,iZ,energyGroup);

  A.insert(iEq,indices[iNF]) = 1.0;
  b(iEq) = SGQD->nFluxBC(iR);
};
//==============================================================================

//==============================================================================
/// Assert the flux boundary condition on the south face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertSFluxBC(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  vector<int> indices = getIndices(iR,iZ,energyGroup);

  A.insert(iEq,indices[iSF]) = 1.0;
  b(iEq) = SGQD->sFluxBC(iR);
};
//==============================================================================

//==============================================================================
/// Assert the flux boundary condition on the west face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertWFluxBC(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  vector<int> indices = getIndices(iR,iZ,energyGroup);

  A.insert(iEq,indices[iWF]) = 1.0;
  b(iEq) = SGQD->wFluxBC(iZ);
};
//==============================================================================

//==============================================================================
/// Assert the flux boundary condition on the east face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertEFluxBC(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  vector<int> indices = getIndices(iR,iZ,energyGroup);

  A.insert(iEq,indices[iEF]) = 1.0;
  b(iEq) = SGQD->eFluxBC(iZ);
};
//==============================================================================

//==============================================================================
/// Assert the current boundary condition on the north face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertNCurrentBC(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  northCurrent(1,iR,iZ,iEq,energyGroup,SGQD);
};
//==============================================================================

//==============================================================================
/// Assert the current boundary condition on the south face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertSCurrentBC(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  southCurrent(1,iR,iZ,iEq,energyGroup,SGQD);
};
//==============================================================================

//==============================================================================
/// Assert the current boundary condition on the west face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertWCurrentBC(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  westCurrent(1,iR,iZ,iEq,energyGroup,SGQD);
};
//==============================================================================

//==============================================================================
/// Assert the current boundary condition on the east face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertECurrentBC(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  eastCurrent(1,iR,iZ,iEq,energyGroup,SGQD);
};
//==============================================================================

//==============================================================================
/// Assert the steady state current boundary condition on the north face at 
/// location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertSteadyStateNCurrentBC(int iR,int iZ,int iEq,\
    int energyGroup, SingleGroupQD * SGQD)
{
  steadyStateNorthCurrent(1,iR,iZ,iEq,energyGroup,SGQD);
};
//==============================================================================

//==============================================================================
/// Assert the steady state current boundary condition on the south face at 
/// location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertSteadyStateSCurrentBC(int iR,int iZ,int iEq,\
    int energyGroup, SingleGroupQD * SGQD)
{
  steadyStateSouthCurrent(1,iR,iZ,iEq,energyGroup,SGQD);
};
//==============================================================================

//==============================================================================
/// Assert the steady state current boundary condition on the west face at 
/// location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertSteadyStateWCurrentBC(int iR,int iZ,int iEq,\
    int energyGroup, SingleGroupQD * SGQD)
{
  steadyStateWestCurrent(1,iR,iZ,iEq,energyGroup,SGQD);
};
//==============================================================================

//==============================================================================
/// Assert the steady state current boundary condition on the east face at 
/// location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertSteadyStateECurrentBC(int iR,int iZ,int iEq,\
    int energyGroup, SingleGroupQD * SGQD)
{
  steadyStateEastCurrent(1,iR,iZ,iEq,energyGroup,SGQD);
};
//==============================================================================

//==============================================================================
/// Assert Gol'din's boundary condition on the north face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertNGoldinBC(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  vector<int> indices = getIndices(iR,iZ,energyGroup);
  double ratio = SGQD->nOutwardCurrToFluxRatioBC(iR);
  double absCurrent = SGQD->nAbsCurrentBC(iR);
  double inwardCurrent = SGQD->nInwardCurrentBC(iR);
  double inwardFlux = SGQD->nInwardFluxBC(iR);

  northCurrent(1.0,iR,iZ,iEq,energyGroup,SGQD);
  A.coeffRef(iEq,indices[iNF]) -= ratio;
  b(iEq) = b(iEq) + (inwardCurrent-ratio*inwardFlux);

  //northCurrent(1.0,iR,iZ,iEq,energyGroup,SGQD);
  //A.coeffRef(iEq,indices[iNF]) -= absCurrent/SGQD->nFluxBC(iR);
  //b(iEq) = b(iEq) + (2*inwardCurrent);

  //northCurrent(0.5,iR,iZ,iEq,energyGroup,SGQD);
  //southCurrent(0.5,iR,iZ,iEq,energyGroup,SGQD);
  //b(iEq) = b(iEq) + SGQD->nCurrentZBC(iR);

  //A.coeffRef(iEq,indices[iNF]) = 1.0;
  //b(iEq) = SGQD->nFluxBC(iR); 
};
//==============================================================================

//==============================================================================
/// Assert Gol'din's boundary condition on the south face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertSGoldinBC(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  vector<int> indices = getIndices(iR,iZ,energyGroup);
  double ratio = SGQD->sOutwardCurrToFluxRatioBC(iR);
  double absCurrent = SGQD->sAbsCurrentBC(iR);
  double inwardCurrent = SGQD->sInwardCurrentBC(iR);
  double inwardFlux = SGQD->sInwardFluxBC(iR);

  southCurrent(1.0,iR,iZ,iEq,energyGroup,SGQD);
  A.coeffRef(iEq,indices[iSF]) -= ratio;
  b(iEq) = b(iEq) + (inwardCurrent-ratio*inwardFlux);

  //southCurrent(1.0,iR,iZ,iEq,energyGroup,SGQD);
  //A.coeffRef(iEq,indices[iSF]) -= absCurrent/SGQD->sFluxBC(iR);
  //b(iEq) = b(iEq) + (2*inwardCurrent);

  //southCurrent(0.5,iR,iZ,iEq,energyGroup,SGQD);
  //northCurrent(0.5,iR,iZ,iEq,energyGroup,SGQD);
  //b(iEq) = b(iEq) + SGQD->sCurrentZBC(iR);

  //A.coeffRef(iEq,indices[iSF]) = 1.0;
  //b(iEq) = SGQD->sFluxBC(iR); 
};
//==============================================================================

//==============================================================================
/// Assert Gol'din's boundary condition on the east face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertEGoldinBC(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  vector<int> indices = getIndices(iR,iZ,energyGroup);
  double ratio = SGQD->eOutwardCurrToFluxRatioBC(iZ);
  double absCurrent = SGQD->eAbsCurrentBC(iZ);
  double inwardCurrent = SGQD->eInwardCurrentBC(iZ);
  double inwardFlux = SGQD->eInwardFluxBC(iZ);

  eastCurrent(1.0,iR,iZ,iEq,energyGroup,SGQD);
  A.coeffRef(iEq,indices[iEF]) -= ratio;
  b(iEq) = b(iEq) + (inwardCurrent-ratio*inwardFlux);

  //eastCurrent(1.0,iR,iZ,iEq,energyGroup,SGQD);
  //A.coeffRef(iEq,indices[iEF]) -= absCurrent/SGQD->eFluxBC(iZ);
  //b(iEq) = b(iEq) + (2*inwardCurrent);

  //eastCurrent(0.5,iR,iZ,iEq,energyGroup,SGQD);
  //westCurrent(0.5,iR,iZ,iEq,energyGroup,SGQD);
  //b(iEq) = b(iEq) + SGQD->eCurrentRBC(iZ);

  //A.coeffRef(iEq,indices[iEF]) = 1.0;
  //b(iEq) = SGQD->eFluxBC(iZ); 
};
//==============================================================================

//==============================================================================
/// Assert Gol'din's steady state boundary condition on the north face at 
/// location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertSteadyStateNGoldinBC(int iR,int iZ,int iEq,\
    int energyGroup,SingleGroupQD * SGQD)
{
  vector<int> indices = getIndices(iR,iZ,energyGroup);
  double ratio = SGQD->nOutwardCurrToFluxRatioBC(iR);
  double absCurrent = SGQD->nAbsCurrentBC(iR);
  double inwardCurrent = SGQD->nInwardCurrentBC(iR);
  double inwardFlux = SGQD->nInwardFluxBC(iR);

  steadyStateNorthCurrent(1.0,iR,iZ,iEq,energyGroup,SGQD);
  A.coeffRef(iEq,indices[iNF]) -= ratio;
  b(iEq) = b(iEq) + (inwardCurrent-ratio*inwardFlux);

  //northCurrent(1.0,iR,iZ,iEq,energyGroup,SGQD);
  //A.coeffRef(iEq,indices[iNF]) -= absCurrent/SGQD->nFluxBC(iR);
  //b(iEq) = b(iEq) + (2*inwardCurrent);

  //northCurrent(0.5,iR,iZ,iEq,energyGroup,SGQD);
  //southCurrent(0.5,iR,iZ,iEq,energyGroup,SGQD);
  //b(iEq) = b(iEq) + SGQD->nCurrentZBC(iR);

  //A.coeffRef(iEq,indices[iNF]) = 1.0;
  //b(iEq) = SGQD->nFluxBC(iR); 
};
//==============================================================================

//==============================================================================
/// Assert Gol'din's steady state boundary condition on the south face at 
/// location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertSteadyStateSGoldinBC(int iR,int iZ,int iEq,\
    int energyGroup, SingleGroupQD * SGQD)
{
  vector<int> indices = getIndices(iR,iZ,energyGroup);
  double ratio = SGQD->sOutwardCurrToFluxRatioBC(iR);
  double absCurrent = SGQD->sAbsCurrentBC(iR);
  double inwardCurrent = SGQD->sInwardCurrentBC(iR);
  double inwardFlux = SGQD->sInwardFluxBC(iR);

  steadyStateSouthCurrent(1.0,iR,iZ,iEq,energyGroup,SGQD);
  A.coeffRef(iEq,indices[iSF]) -= ratio;
  b(iEq) = b(iEq) + (inwardCurrent-ratio*inwardFlux);

  //southCurrent(1.0,iR,iZ,iEq,energyGroup,SGQD);
  //A.coeffRef(iEq,indices[iSF]) -= absCurrent/SGQD->sFluxBC(iR);
  //b(iEq) = b(iEq) + (2*inwardCurrent);

  //southCurrent(0.5,iR,iZ,iEq,energyGroup,SGQD);
  //northCurrent(0.5,iR,iZ,iEq,energyGroup,SGQD);
  //b(iEq) = b(iEq) + SGQD->sCurrentZBC(iR);

  //A.coeffRef(iEq,indices[iSF]) = 1.0;
  //b(iEq) = SGQD->sFluxBC(iR); 
};
//==============================================================================

//==============================================================================
/// Assert Gol'din's steady state boundary condition on the east face at 
/// location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertSteadyStateEGoldinBC(int iR,int iZ,int iEq,\
    int energyGroup, SingleGroupQD * SGQD)
{
  vector<int> indices = getIndices(iR,iZ,energyGroup);
  double ratio = SGQD->eOutwardCurrToFluxRatioBC(iZ);
  double absCurrent = SGQD->eAbsCurrentBC(iZ);
  double inwardCurrent = SGQD->eInwardCurrentBC(iZ);
  double inwardFlux = SGQD->eInwardFluxBC(iZ);

  steadyStateEastCurrent(1.0,iR,iZ,iEq,energyGroup,SGQD);
  A.coeffRef(iEq,indices[iEF]) -= ratio;
  b(iEq) = b(iEq) + (inwardCurrent-ratio*inwardFlux);

  //eastCurrent(1.0,iR,iZ,iEq,energyGroup,SGQD);
  //A.coeffRef(iEq,indices[iEF]) -= absCurrent/SGQD->eFluxBC(iZ);
  //b(iEq) = b(iEq) + (2*inwardCurrent);

  //eastCurrent(0.5,iR,iZ,iEq,energyGroup,SGQD);
  //westCurrent(0.5,iR,iZ,iEq,energyGroup,SGQD);
  //b(iEq) = b(iEq) + SGQD->eCurrentRBC(iZ);

  //A.coeffRef(iEq,indices[iEF]) = 1.0;
  //b(iEq) = SGQD->eFluxBC(iZ); 
};
//==============================================================================

//==============================================================================
/// Assert boundary condition on the north face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertNBC(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  if (reflectingBCs)
    assertNCurrentBC(iR,iZ,iEq,energyGroup,SGQD);
  else if (goldinBCs)
    assertNGoldinBC(iR,iZ,iEq,energyGroup,SGQD);
  else
    assertNFluxBC(iR,iZ,iEq,energyGroup,SGQD);
};
//==============================================================================

//==============================================================================
/// Assert boundary condition on the south face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertSBC(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  if (reflectingBCs)
    assertSCurrentBC(iR,iZ,iEq,energyGroup,SGQD);
  else if (goldinBCs)
    assertSGoldinBC(iR,iZ,iEq,energyGroup,SGQD);
  else
    assertSFluxBC(iR,iZ,iEq,energyGroup,SGQD);
};
//==============================================================================

//==============================================================================
/// Assert boundary condition on the west face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertWBC(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  if (reflectingBCs or goldinBCs)
    assertWCurrentBC(iR,iZ,iEq,energyGroup,SGQD);
  else
    // Can't think of a circumstance where there wouldn't be a reflecting BC at
    //   r = 0 
    //assertWFluxBC(iR,iZ,iEq,energyGroup,SGQD);
    assertWCurrentBC(iR,iZ,iEq,energyGroup,SGQD);
};
//==============================================================================

//==============================================================================
/// Assert boundary condition on the east face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertEBC(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  if (reflectingBCs)
    assertECurrentBC(iR,iZ,iEq,energyGroup,SGQD);
  else if (goldinBCs)
    assertEGoldinBC(iR,iZ,iEq,energyGroup,SGQD);
  else
    assertEFluxBC(iR,iZ,iEq,energyGroup,SGQD);
};
//==============================================================================

//==============================================================================
/// Assert steady state boundary condition on the north face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertSteadyStateNBC(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  if (reflectingBCs)
    assertSteadyStateNCurrentBC(iR,iZ,iEq,energyGroup,SGQD);
  else if (goldinBCs)
    assertSteadyStateNGoldinBC(iR,iZ,iEq,energyGroup,SGQD);
  else
    assertNFluxBC(iR,iZ,iEq,energyGroup,SGQD);
};
//==============================================================================

//==============================================================================
/// Assert steady state boundary condition on the south face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertSteadyStateSBC(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  if (reflectingBCs)
    assertSteadyStateSCurrentBC(iR,iZ,iEq,energyGroup,SGQD);
  else if (goldinBCs)
    assertSteadyStateSGoldinBC(iR,iZ,iEq,energyGroup,SGQD);
  else
    assertSFluxBC(iR,iZ,iEq,energyGroup,SGQD);
};
//==============================================================================

//==============================================================================
/// Assert steady state boundary condition on the west face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertSteadyStateWBC(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  if (reflectingBCs or goldinBCs)
    assertSteadyStateWCurrentBC(iR,iZ,iEq,energyGroup,SGQD);
  else
    // Can't think of a circumstance where there wouldn't be a reflecting BC at
    //   r = 0 
    //assertWFluxBC(iR,iZ,iEq,energyGroup,SGQD);
    assertSteadyStateWCurrentBC(iR,iZ,iEq,energyGroup,SGQD);
};
//==============================================================================

//==============================================================================
/// Assert steady state boundary condition on the east face at location (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] iEq row to place equation in
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [in] SGQD of this energyGroup
void QDSolver::assertSteadyStateEBC(int iR,int iZ,int iEq,int energyGroup,\
    SingleGroupQD * SGQD)
{
  if (reflectingBCs)
    assertSteadyStateECurrentBC(iR,iZ,iEq,energyGroup,SGQD);
  else if (goldinBCs)
    assertSteadyStateEGoldinBC(iR,iZ,iEq,energyGroup,SGQD);
  else
    assertEFluxBC(iR,iZ,iEq,energyGroup,SGQD);
};
//==============================================================================


//==============================================================================
/// Calculate multigroup source coefficient for cell at (iR,iZ)
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] toEnergyGroup energy group of sourcing group
/// @param [in] fromEnergyGroup energy group of source group
double QDSolver::calcScatterAndFissionCoeff(int iR,int iZ,int toEnergyGroup,\
    int fromEnergyGroup)
{

  double localSigF,localNu,localChiP,localSigS,sourceCoefficient;

  localSigF = materials->sigF(iZ,iR,fromEnergyGroup);
  localNu = materials->nu(iZ,iR,fromEnergyGroup);
  localChiP = materials->chiP(iZ,iR,toEnergyGroup);
  localSigS = materials->sigS(iZ,iR,fromEnergyGroup,toEnergyGroup);
  sourceCoefficient = localSigS + localChiP * localNu * localSigF;

  return sourceCoefficient;
};
//==============================================================================

//==============================================================================
/// Impose scattering and fission source from grey group values 
/// 
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] toEnergyGroup energy group of sourcing group
/// @param [in] fromEnergyGroup energy group of source group
void QDSolver::greyGroupSources(int iR,int iZ,int iEq,int toEnergyGroup,\
    vector<double> geoParams)
{

  double localChiP,localSigS,localFissionCoeff,localFlux,localUpscatterCoeff,\
    localChiD,localDNPSource;
  vector<int> indices;

  for (int iFromEnergyGroup = 0; iFromEnergyGroup <= toEnergyGroup;\
      ++iFromEnergyGroup)
  {
    indices = getIndices(iR,iZ,iFromEnergyGroup);
    localSigS = materials->sigS(iZ,iR,iFromEnergyGroup,toEnergyGroup);
    A.insert(iEq,indices[iCF]) = -geoParams[iCF] * localSigS;
  }

  localChiD = materials->chiD(iZ,iR,toEnergyGroup);
  localDNPSource = mpqd->mgdnp->dnpSource(iZ,iR); 
  localChiP = materials->chiP(iZ,iR,toEnergyGroup);
  localUpscatterCoeff = materials->oneGroupXS->upscatterCoeff(iZ,iR,\
      toEnergyGroup);
  localFissionCoeff = materials->oneGroupXS->qdFluxCoeff(iZ,iR);
  localFlux = mpqd->ggqd->sFlux(iZ,iR); 

  b(iEq) += geoParams[iCF]*((localUpscatterCoeff\
      + localChiP*localFissionCoeff)*localFlux + localChiD*localDNPSource);
};
//==============================================================================

//==============================================================================
/// Impose steady state scattering and fission source from grey group values 
/// 
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] toEnergyGroup energy group of sourcing group
/// @param [in] fromEnergyGroup energy group of source group
void QDSolver::steadyStateGreyGroupSources(int iR,int iZ,int iEq,\
    int toEnergyGroup,vector<double> geoParams)
{

  double localChiP,localSigS,localFissionCoeff,localFlux,localUpscatterCoeff,\
    localChiD,localDNPSource,keff;
  vector<int> indices;

  for (int iFromEnergyGroup = 0; iFromEnergyGroup <= toEnergyGroup;\
      ++iFromEnergyGroup)
  {
    indices = getIndices(iR,iZ,iFromEnergyGroup);
    localSigS = materials->sigS(iZ,iR,iFromEnergyGroup,toEnergyGroup);
    A.insert(iEq,indices[iCF]) = -geoParams[iCF] * localSigS;
  }

  localChiD = materials->chiD(iZ,iR,toEnergyGroup);
  localDNPSource = mpqd->mgdnp->dnpSource(iZ,iR); 
  localChiP = materials->chiP(iZ,iR,toEnergyGroup);
  localUpscatterCoeff = materials->oneGroupXS->upscatterCoeff(iZ,iR,\
      toEnergyGroup);
  localFissionCoeff = materials->oneGroupXS->qdFluxCoeff(iZ,iR);
  localFlux = mpqd->ggqd->sFlux(iZ,iR); 
  keff = materials->oneGroupXS->keff;

  b(iEq) += geoParams[iCF]*((localUpscatterCoeff\
      + localChiP*localFissionCoeff/keff)*localFlux + localChiD*localDNPSource);
};
//==============================================================================

//==============================================================================
/// Form a portion of the linear system that belongs to SGQD 
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] rEval radial location to evaluate integrating factor at
/// @param [in] SGQD of this energyGroup
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
  rUp = mesh->rCornerEdge(iR+1); rDown = mesh->rCornerEdge(iR);
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

  return hEval;

};

//==============================================================================

//==============================================================================
/// Return global index of south face current at indices iR and iZ 
/// @param [in] iR radial index of cell
/// @param [in] iZ axial index of cell
/// @param [in] energyGroup energy group to assert boundary condition for
/// @param [out] index global index for south face current in cell at (iR,iZ)
vector<int> QDSolver::getIndices(int iR,int iZ,int energyGroup)
{

  vector<int> indices,oneGroupIndices;

  // Set flux and current offsets according to energy group
  int offsetFlux = energyGroup*nGroupUnknowns;
  int offsetCurr = energyGroup*nGroupCurrentUnknowns;

  // Get indices for a single energy group 
  oneGroupIndices = mesh->getQDCellIndices(iR,iZ);

  // Offset by specified energy group
  indices.push_back(oneGroupIndices[iCF] + offsetFlux);
  indices.push_back(oneGroupIndices[iWF] + offsetFlux);
  indices.push_back(oneGroupIndices[iEF] + offsetFlux);
  indices.push_back(oneGroupIndices[iNF] + offsetFlux);
  indices.push_back(oneGroupIndices[iSF] + offsetFlux);
  indices.push_back(oneGroupIndices[iWC] + offsetCurr);
  indices.push_back(oneGroupIndices[iEC] + offsetCurr);
  indices.push_back(oneGroupIndices[iNC] + offsetCurr);
  indices.push_back(oneGroupIndices[iSC] + offsetCurr);

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
  rDown = mesh->rCornerEdge(iR); rUp = mesh->rCornerEdge(iR+1);
  zDown = mesh->zCornerEdge(iZ); zUp = mesh->zCornerEdge(iZ+1);

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
  double volAvgR = (2.0/3.0)*(pow(rUp,3) - pow(rDown,3))/(pow(rUp,2)-pow(rDown,2));

  return volAvgR;
};
//==============================================================================

//==============================================================================
/// Get Err on west interface 
///
/// @param [in] iZ axial coordinate 
/// @param [in] iR radial coordinate 
/// @param [out] Eddington value to use at interface 
double QDSolver::getWestErr(int iZ,int iR,SingleGroupQD * SGQD)
{

  double volLeft,volRight,totalVol;

  // Return the harmonic average of the Eddingtons on either side of the west
  // interface, unless on a boundary. In that case, simply return the cell
  // center Eddington.
  if (iR == 0)
    return SGQD->Err(iZ,iR);
  else
  { 
    volLeft = calcGeoParams(iR-1,iZ)[iCF];
    volRight = calcGeoParams(iR,iZ)[iCF];
    totalVol = volLeft + volRight;
    return (totalVol)/(volLeft/SGQD->Err(iZ,iR-1) + volRight/SGQD->Err(iZ,iR));
  }

};
//==============================================================================

//==============================================================================
/// Get Erz on west interface 
///
/// @param [in] iZ axial coordinate 
/// @param [in] iR radial coordinate 
/// @param [out] Eddington value to use at interface 
double QDSolver::getWestErz(int iZ,int iR,SingleGroupQD * SGQD)
{

  double volLeft,volRight,totalVol;

  // Return the harmonic average of the Eddingtons on either side of the west
  // interface, unless on a boundary. In that case, simply return the cell
  // center Eddington.
  if (iR == 0)
    return SGQD->Erz(iZ,iR);
  else
  { 
    volLeft = calcGeoParams(iR-1,iZ)[iCF];
    volRight = calcGeoParams(iR,iZ)[iCF];
    totalVol = volLeft + volRight;
    return (totalVol)/(volLeft/SGQD->Erz(iZ,iR-1) + volRight/SGQD->Erz(iZ,iR));
  }

};
//==============================================================================

//==============================================================================
/// Get Err on east interface 
///
/// @param [in] iZ axial coordinate 
/// @param [in] iR radial coordinate 
/// @param [out] Eddington value to use at interface 
double QDSolver::getEastErr(int iZ,int iR,SingleGroupQD * SGQD)
{

  double volLeft,volRight,totalVol;

  // Return the harmonic average of the Eddingtons on either side of the east
  // interface, unless on a boundary. In that case, simply return the cell
  // center Eddington.
  if (iR == mesh->nR - 1)
    return SGQD->Err(iZ,iR);
  else
  { 
    volLeft = calcGeoParams(iR,iZ)[iCF];
    volRight = calcGeoParams(iR+1,iZ)[iCF];
    totalVol = volLeft + volRight;
    return (totalVol)/(volLeft/SGQD->Err(iZ,iR) + volRight/SGQD->Err(iZ,iR+1));
  }

};
//==============================================================================

//==============================================================================
/// Get Erz on east interface 
///
/// @param [in] iZ axial coordinate 
/// @param [in] iR radial coordinate 
/// @param [out] Eddington value to use at interface 
double QDSolver::getEastErz(int iZ,int iR,SingleGroupQD * SGQD)
{

  double volLeft,volRight,totalVol;

  // Return the harmonic average of the Eddingtons on either side of the east
  // interface, unless on a boundary. In that case, simply return the cell
  // center Eddington.
  if (iR == mesh->nR - 1)
    return SGQD->Erz(iZ,iR);
  else
  { 
    volLeft = calcGeoParams(iR,iZ)[iCF];
    volRight = calcGeoParams(iR+1,iZ)[iCF];
    totalVol = volLeft + volRight;
    return (totalVol)/(volLeft/SGQD->Erz(iZ,iR) + volRight/SGQD->Erz(iZ,iR+1));
  }

};
//==============================================================================

//==============================================================================
/// Get Ezz on north interface 
///
/// @param [in] iZ axial coordinate 
/// @param [in] iR radial coordinate 
/// @param [out] Eddington value to use at interface 
double QDSolver::getNorthEzz(int iZ,int iR,SingleGroupQD * SGQD)
{

  double volDown,volUp,totalVol;

  // Return the harmonic average of the Eddingtons on either side of the north
  // interface, unless on a boundary. In that case, simply return the cell
  // center Eddington.
  if (iZ == 0)
    return SGQD->Ezz(iZ,iR);
  else
  { 
    volDown = calcGeoParams(iR,iZ-1)[iCF];
    volUp = calcGeoParams(iR,iZ)[iCF];
    totalVol = volUp + volDown;
    return (totalVol)/(volDown/SGQD->Ezz(iZ-1,iR) + volUp/SGQD->Ezz(iZ,iR));
  }

};
//==============================================================================

//==============================================================================
/// Get Erz on north interface 
///
/// @param [in] iZ axial coordinate 
/// @param [in] iR radial coordinate 
/// @param [out] Eddington value to use at interface 
double QDSolver::getNorthErz(int iZ,int iR,SingleGroupQD * SGQD)
{

  double volDown,volUp,totalVol;

  // Return the harmonic average of the Eddingtons on either side of the north
  // interface, unless on a boundary. In that case, simply return the cell
  // center Eddington.
  if (iZ == 0)
    return SGQD->Erz(iZ,iR);
  else
  { 
    volDown = calcGeoParams(iR,iZ-1)[iCF];
    volUp = calcGeoParams(iR,iZ)[iCF];
    totalVol = volUp + volDown;
    return (totalVol)/(volDown/SGQD->Erz(iZ-1,iR) + volUp/SGQD->Erz(iZ,iR));
  }

};
//==============================================================================

//==============================================================================
/// Get Ezz on south interface 
///
/// @param [in] iZ axial coordinate 
/// @param [in] iR radial coordinate 
/// @param [out] Eddington value to use at interface 
double QDSolver::getSouthEzz(int iZ,int iR,SingleGroupQD * SGQD)
{

  double volDown,volUp,totalVol;

  // Return the harmonic average of the Eddingtons on either side of the south
  // interface, unless on a boundary. In that case, simply return the cell
  // center Eddington.
  if (iZ == mesh->nZ-1)
    return SGQD->Ezz(iZ,iR);
  else
  { 
    volDown = calcGeoParams(iR,iZ)[iCF];
    volUp = calcGeoParams(iR,iZ+1)[iCF];
    totalVol = volUp + volDown;
    return (totalVol)/(volDown/SGQD->Ezz(iZ,iR) + volUp/SGQD->Ezz(iZ+1,iR));
  }

};
//==============================================================================

//==============================================================================
/// Get Erz on south interface 
///
/// @param [in] iZ axial coordinate 
/// @param [in] iR radial coordinate 
/// @param [out] Eddington value to use at interface 
double QDSolver::getSouthErz(int iZ,int iR,SingleGroupQD * SGQD)
{

  double volDown,volUp,totalVol;

  // Return the harmonic average of the Eddingtons on either side of the south
  // interface, unless on a boundary. In that case, simply return the cell
  // center Eddington.
  if (iZ == mesh->nZ-1)
    return SGQD->Erz(iZ,iR);
  else
  { 
    volDown = calcGeoParams(iR,iZ)[iCF];
    volUp = calcGeoParams(iR,iZ+1)[iCF];
    totalVol = volUp + volDown;
    return (totalVol)/(volDown/SGQD->Erz(iZ,iR) + volUp/SGQD->Erz(iZ+1,iR));
  }

};
//==============================================================================

//==============================================================================
/// Extract cell average values from solution vector and store
/// @param [in] SGQD single group quasidiffusion object to get flux for
void QDSolver::getFlux(SingleGroupQD * SGQD)
{
  vector<int> indices;

  // Store past current values. These are used in the group collapse calc.
  SGQD->sFluxPrev = SGQD->sFlux;
  SGQD->sFluxRPrev = SGQD->sFluxR;
  SGQD->sFluxZPrev = SGQD->sFluxZ;
  SGQD->currentRPrev = SGQD->currentR;
  SGQD->currentZPrev = SGQD->currentZ;

  // loop over spatial mesh
  for (int iR = 0; iR < mesh->drsCorner.size(); iR++)
  {
    for (int iZ = 0; iZ < mesh->dzsCorner.size(); iZ++)
    {
      indices = getIndices(iR,iZ,SGQD->energyGroup);

      // Get cell average flux
      SGQD->sFlux(iZ,iR) = x(indices[iCF]);

      // Get cell interface fluxes
      SGQD->sFluxR(iZ,iR) = x(indices[iWF]);
      SGQD->sFluxR(iZ,iR+1) = x(indices[iEF]);
      SGQD->sFluxZ(iZ,iR) = x(indices[iNF]);
      SGQD->sFluxZ(iZ+1,iR) = x(indices[iSF]);

      // Get cell currents 
      SGQD->currentR(iZ,iR) = currPast(indices[iWC]);
      SGQD->currentR(iZ,iR+1) = currPast(indices[iEC]);
      SGQD->currentZ(iZ,iR) = currPast(indices[iNC]);
      SGQD->currentZ(iZ+1,iR) = currPast(indices[iSC]);

    }
  } 
};
//==============================================================================

//==============================================================================
/// Extract flux values from SGQD object, store to solution vector, and return 
/// @param [in] SGQD single group quasidiffusion object to get flux for
/// @param [out] solVector flux values mapped to the 1D vector
Eigen::VectorXd QDSolver::getFluxSolutionVector(SingleGroupQD * SGQD)
{
  Eigen::VectorXd solVector(energyGroups*nGroupUnknowns);
  solVector.setZero();
  vector<int> indices;

  // loop over spatial mesh
  for (int iR = 0; iR < mesh->drsCorner.size(); iR++)
  {
    for (int iZ = 0; iZ < mesh->dzsCorner.size(); iZ++)
    {
      indices = getIndices(iR,iZ,SGQD->energyGroup);
      solVector(indices[iCF]) = SGQD->sFlux(iZ,iR);

      solVector(indices[iWF]) = SGQD->sFluxR(iZ,iR);
      solVector(indices[iEF]) = SGQD->sFluxR(iZ,iR+1);
      solVector(indices[iNF]) = SGQD->sFluxZ(iZ,iR);
      solVector(indices[iSF]) = SGQD->sFluxZ(iZ+1,iR);

    }
  }  

  return solVector;
};
//==============================================================================

//==============================================================================
/// Extract current values from SGQD object, store to solution vector, and 
/// return 
/// @param [in] SGQD single group quasidiffusion object to get current for
/// @param [out] solVector current values mapped to the 1D vector
Eigen::VectorXd QDSolver::getCurrentSolutionVector(SingleGroupQD * SGQD)
{
  Eigen::VectorXd solVector(energyGroups*nGroupCurrentUnknowns);
  solVector.setZero();
  vector<int> indices;

  // loop over spatial mesh
  for (int iR = 0; iR < mesh->drsCorner.size(); iR++)
  {
    for (int iZ = 0; iZ < mesh->dzsCorner.size(); iZ++)
    {
      indices = getIndices(iR,iZ,SGQD->energyGroup);

      solVector(indices[iWC]) = SGQD->currentR(iZ,iR);
      solVector(indices[iEC]) = SGQD->currentR(iZ,iR+1);
      solVector(indices[iNC]) = SGQD->currentZ(iZ,iR);
      solVector(indices[iSC]) = SGQD->currentZ(iZ+1,iR);
    }
  }  

  return solVector;
};
//==============================================================================

//==============================================================================
/// Check for optional inputs of relevance to this object
void QDSolver::checkOptionalParams()
{
  string boundaryType,precondInput;

  // check for optional parameters specified in input file

  if ((*input)["parameters"]["solve type"])
  {

    boundaryType=(*input)["parameters"]["solve type"].as<string>();
    if (boundaryType == "TQD") goldinBCs = true;

  }

  if ((*input)["parameters"]["mgqd-bcs"])
  {

    boundaryType=(*input)["parameters"]["mgqd-bcs"].as<string>();

    if (boundaryType == "reflective" or boundaryType == "REFLECTIVE"\
        or boundaryType == "Reflective")
      reflectingBCs = true;
    else if (boundaryType == "goldin" or boundaryType == "GOLDIN" \
        or boundaryType == "Goldin")
      goldinBCs = true;

  }

  if ((*input)["parameters"]["preconditionerMGLOQD"])
  {
    precondInput=(*input)["parameters"]["preconditionerMGLOQD"].as<string>();

    if (precondInput == "ilu")
      preconditioner = iluPreconditioner;
    else if (precondInput == "diagonal" or precondInput == "diag")
      preconditioner = diagPreconditioner;
      
  }

}
//==============================================================================
