// File: MultiGroupQD.cpp
// Purpose: define a class that manipulates each single group quasidiffusion object
// Date: February 05, 2020

#include "MultiGroupQD.h"
#include "SingleGroupQD.h"

using namespace std;

//==============================================================================
/// MultiGroupQD class object constructor
///
/// @param [in] myMaterials Materials object for the simulation
/// @param [in] myMesh Mesh object for the simulation
/// @param [in] myInput YAML input object for the simulation
MultiGroupQD::MultiGroupQD(Materials * myMaterials,\
    Mesh * myMesh,\
    YAML::Node * myInput)
{
  // Assign pointers for materials, mesh, and input objects
  materials = myMaterials;
  mesh = myMesh;
  input = myInput;

  // Initialize pointers to each SGQD group
  for (int iGroups = 0; iGroups < materials->nGroups; ++iGroups){
    shared_ptr<SingleGroupQD> newSGQD (new SingleGroupQD(iGroups,\
          this,materials,mesh,input));
    SGQDs.push_back(std::move(newSGQD));
  }

  QDSolve = std::make_shared<QDSolver>(mesh,materials,input);

  // Initialize variables
  setInitialCondition();

};
//==============================================================================

//==============================================================================
/// Loops over energy groups and builds the linear system to solve the 
/// multigroup quasidiffusion equations
void MultiGroupQD::buildLinearSystem()
{
  // Get non-zero elements to optimize building linear system
  int nonZeros = QDSolve->A.nonZeros();

  QDSolve->A.setZero();
  QDSolve->A.reserve(nonZeros);
  QDSolve->b.setZero();
  for (int iGroup = 0; iGroup < SGQDs.size(); iGroup++)
  {
    SGQDs[iGroup]->formContributionToLinearSystem();
  }
}
//==============================================================================

//==============================================================================
/// Loops over energy groups and builds the linear system to solve the 
/// multigroup quasidiffusion equations
void MultiGroupQD::buildSteadyStateLinearSystem()
{
  // Get non-zero elements to optimize building linear system
  int nonZeros = QDSolve->A.nonZeros();

  QDSolve->A.setZero();
  QDSolve->A.reserve(nonZeros);
  QDSolve->b.setZero();
  for (int iGroup = 0; iGroup < SGQDs.size(); iGroup++)
  {
    SGQDs[iGroup]->formSteadyStateContributionToLinearSystem();
  }
}
//==============================================================================

//==============================================================================
/// Solves the linear system formed by the muligroup quasidiffusion equations
void MultiGroupQD::solveLinearSystem()
{
  QDSolve->solve();
}
//==============================================================================


//==============================================================================
/// Solves the linear system formed by the muligroup quasidiffusion equations
/// using a parallelized method
void MultiGroupQD::solveLinearSystemIterative()
{
  QDSolve->solveIterative();
}
//==============================================================================

//==============================================================================
/// Loops over energy groups and builds linear system to calculate the net 
/// neutron currents from the flux values currrently held in x, the solution
/// vector
void MultiGroupQD::buildBackCalcSystem()
{
  QDSolve->C.setZero();
  QDSolve->d.setZero();
  for (int iGroup = 0; iGroup < SGQDs.size(); iGroup++)
  {
    SGQDs[iGroup]->formContributionToBackCalcSystem();
  }
}
//==============================================================================

//==============================================================================
/// Loops over energy groups and builds linear system to calculate the net 
/// neutron currents from the flux values currrently held in x, the solution
/// vector
void MultiGroupQD::buildSteadyStateBackCalcSystem()
{
  QDSolve->C.setZero();
  QDSolve->d.setZero();
  for (int iGroup = 0; iGroup < SGQDs.size(); iGroup++)
  {
    SGQDs[iGroup]->formSteadyStateContributionToBackCalcSystem();
  }
}
//==============================================================================


//==============================================================================
/// Solve the linear system which calculates net currents from the current flux
/// values
void MultiGroupQD::backCalculateCurrent()
{

  QDSolve->backCalculateCurrent();

}
//==============================================================================


//==============================================================================
/// Set the initial previous solution vectors to the values currently held in 
/// the flux and current matrices
void MultiGroupQD::setInitialCondition()
{
  // Initialize vectors
  Eigen::VectorXd initialFluxCondition(QDSolve->energyGroups*\
      QDSolve->nGroupUnknowns);
  Eigen::VectorXd initialCurrentCondition(QDSolve->energyGroups*\
      QDSolve->nGroupCurrentUnknowns);
  initialFluxCondition.setZero();   
  initialCurrentCondition.setZero(); 

  // Populate initial vectors with values from each energy group  
  for (int iGroup = 0; iGroup < SGQDs.size(); iGroup++)
  {
    initialFluxCondition = initialFluxCondition\
                           + SGQDs[iGroup]->getFluxSolutionVector();
    initialCurrentCondition = initialCurrentCondition\
                              + SGQDs[iGroup]->getCurrentSolutionVector();
  }

  // Set initial conditions in QDSolver object
  if (mesh->petsc)
  {
    eigenVecToPETScVec(&initialFluxCondition,&(QDSolve->x_p));
    eigenVecToPETScVec(&initialCurrentCondition,&(QDSolve->currPast_p));
  }
  else
  {
    QDSolve->xPast = initialFluxCondition;
    QDSolve->currPast = initialCurrentCondition;
  }
}
//==============================================================================

//==============================================================================
/// Solve a transient problem without any transport coupling using diffusion
/// values for the Eddington factors
void MultiGroupQD::solveMGQDOnly()
{
  setInitialCondition();
  for (int iTime = 0; iTime < mesh->dts.size(); iTime++)
  {
    buildLinearSystem();
    cout << "time: " <<mesh->ts[iTime+1] << endl;
    solveLinearSystem();
    QDSolve->xPast = QDSolve->x;
    buildBackCalcSystem();
    backCalculateCurrent();
    getFluxes();
  }
  writeFluxes();
}
//==============================================================================

//==============================================================================
/// Extracts fluxes and currents from solution vector into 2D matrices 
void MultiGroupQD::getFluxes()
{
  for (int iGroup = 0; iGroup < SGQDs.size(); iGroup++)
  {
    SGQDs[iGroup]->getFlux();
  }
}
//==============================================================================

//==============================================================================
/// Solve a transient problem without any transport coupling using diffusion
/// values for the Eddington factors
void MultiGroupQD::updateVarsAfterConvergence()
{
  QDSolve->xPast = QDSolve->x;
  buildBackCalcSystem();
  backCalculateCurrent();
  getFluxes();
}
//==============================================================================

//==============================================================================
/// Solve a transient problem without any transport coupling using diffusion
/// values for the Eddington factors
void MultiGroupQD::updateSteadyStateVarsAfterConvergence()
{
  QDSolve->xPast = QDSolve->x;
  buildSteadyStateBackCalcSystem();
  backCalculateCurrent();
  getFluxes();
}
//==============================================================================

//==============================================================================
/// Loops over energy groups and builds the linear system to solve the 
/// multigroup quasidiffusion equations
int MultiGroupQD::buildSteadyStateLinearSystem_p()
{
  
  PetscErrorCode ierr;
  
  for (int iGroup = 0; iGroup < SGQDs.size(); iGroup++)
  {
    SGQDs[iGroup]->formSteadyStateContributionToLinearSystem_p();
  }

  /* Finalize assembly for A_p and b_p */
  ierr = MatAssemblyBegin(QDSolve->A_p,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(QDSolve->A_p,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);  
  ierr = VecAssemblyBegin(QDSolve->b_p);CHKERRQ(ierr);
  ierr = VecAssemblyEnd(QDSolve->b_p);CHKERRQ(ierr);

}
//==============================================================================

//==============================================================================
/// Solves the linear system formed by the muligroup quasidiffusion equations
void MultiGroupQD::solveLinearSystem_p()
{
  QDSolve->solve_p();
}
//==============================================================================

//==============================================================================
/// Loops over energy groups and builds linear system to calculate the net 
/// neutron currents from the flux values currrently held in x, the solution
/// vector
int MultiGroupQD::buildSteadyStateBackCalcSystem_p()
{
  PetscErrorCode ierr;
  
  ierr = VecSet(QDSolve->d_p,0.0);CHKERRQ(ierr);

  for (int iGroup = 0; iGroup < SGQDs.size(); iGroup++)
  {
    SGQDs[iGroup]->formSteadyStateContributionToBackCalcSystem_p();
  }

  /* Finalize assembly for C_p and d_p */
  ierr = MatAssemblyBegin(QDSolve->C_p,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(QDSolve->C_p,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);  
  ierr = VecAssemblyBegin(QDSolve->d_p);CHKERRQ(ierr);
  ierr = VecAssemblyEnd(QDSolve->d_p);CHKERRQ(ierr);

}
//==============================================================================

//==============================================================================
/// Solve the linear system which calculates net currents from the current flux
/// values
void MultiGroupQD::backCalculateCurrent_p()
{

  QDSolve->backCalculateCurrent_p();

}
//==============================================================================

//==============================================================================
/// Assigning pointer to object containing grey group sources 
void MultiGroupQD::assignMultiPhysicsCoupledQDPointer\
       (MultiPhysicsCoupledQD * myMPQD)
{
  QDSolve->mpqd = myMPQD;
  QDSolve->useMPQDSources = true;
}
//==============================================================================

//==============================================================================
/// Wrapper over SGQDs to write flux present in each
void MultiGroupQD::writeVars()
{

  string name; 

  for (int iGroup = 0; iGroup < materials->nGroups; ++iGroup){
    
    // Fluxes 
    name =  "Flux_Group_" + to_string(iGroup);
    mesh->output->write(outputDir,name,SGQDs[iGroup]->sFlux);

    // Face fluxes 
    name =  "Flux_Radial_Group_" + to_string(iGroup);
    mesh->output->write(outputDir,name,SGQDs[iGroup]->sFluxR);
    name =  "Flux_Axial_Group_" + to_string(iGroup);
    mesh->output->write(outputDir,name,SGQDs[iGroup]->sFluxZ);

    // Currents 
    name =  "Current_Radial_Group_" + to_string(iGroup);
    mesh->output->write(outputDir,name,SGQDs[iGroup]->currentR);
    name =  "Current_Axial_Group_" + to_string(iGroup);
    mesh->output->write(outputDir,name,SGQDs[iGroup]->currentZ);

    // Eddington factors
    name =  "Err_Group_" + to_string(iGroup);
    mesh->output->write(outputDir,name,SGQDs[iGroup]->Err);
    name =  "Ezz_Group_" + to_string(iGroup);
    mesh->output->write(outputDir,name,SGQDs[iGroup]->Ezz);
    name =  "Erz_Group_" + to_string(iGroup);
    mesh->output->write(outputDir,name,SGQDs[iGroup]->Erz);
    name =  "Err_Axial_Group_" + to_string(iGroup);
    mesh->output->write(outputDir,name,SGQDs[iGroup]->ErrAxial);
    name =  "Ezz_Axial_Group_" + to_string(iGroup);
    mesh->output->write(outputDir,name,SGQDs[iGroup]->EzzAxial);
    name =  "Erz_Axial_Group_" + to_string(iGroup);
    mesh->output->write(outputDir,name,SGQDs[iGroup]->ErzAxial);
    name =  "Err_Radial_Group_" + to_string(iGroup);
    mesh->output->write(outputDir,name,SGQDs[iGroup]->ErrRadial);
    name =  "Ezz_Radial_Group_" + to_string(iGroup);
    mesh->output->write(outputDir,name,SGQDs[iGroup]->EzzRadial);
    name =  "Erz_Radial_Group_" + to_string(iGroup);
    mesh->output->write(outputDir,name,SGQDs[iGroup]->ErzRadial);
    name =  "G_Group_" + to_string(iGroup);
    mesh->output->write(outputDir,name,SGQDs[iGroup]->G);
    name =  "G_Radial_Group_" + to_string(iGroup);
    mesh->output->write(outputDir,name,SGQDs[iGroup]->GRadial);
  }

}
//==============================================================================

//==============================================================================
/// Print fluxes and currents 
/// 
void MultiGroupQD::printVars()
{

  for (int iGroup = 0; iGroup < materials->nGroups; ++iGroup)
  {
    cout << "Flux, Group " << SGQDs[iGroup]->energyGroup << ":" << endl; 
    cout << SGQDs[iGroup]->sFlux << endl;
    cout << endl;
    cout << "Axial Flux, Group " << SGQDs[iGroup]->energyGroup << ":" << endl; 
    cout << SGQDs[iGroup]->sFluxZ << endl;
    cout << endl;
    cout << "Radial Flux, Group " << SGQDs[iGroup]->energyGroup << ":" << endl; 
    cout << SGQDs[iGroup]->sFluxR << endl;
    cout << endl;
    cout << "Axial Current, Group " << SGQDs[iGroup]->energyGroup\
      << ":" << endl; 
    cout << SGQDs[iGroup]->currentZ << endl;
    cout << endl;
    cout << "Radial Current, Group " << SGQDs[iGroup]->energyGroup\
      << ":" << endl; 
    cout << SGQDs[iGroup]->currentR << endl;
    cout << endl;
  }

};
//==============================================================================

//==============================================================================
/// Print Eddingtons
/// 
void MultiGroupQD::printEddingtons()
{

  for (int iGroup = 0; iGroup < materials->nGroups; ++iGroup)
  {
    cout << "Group " << iGroup << ":" << endl;
    cout << endl;
    SGQDs[iGroup]->printEddingtons();
  }

};
//==============================================================================


//==============================================================================
/// Wrapper over SGQDs to write flux present in each
void MultiGroupQD::writeFluxes()
{
  for (int iGroup = 0; iGroup < materials->nGroups; ++iGroup){
    SGQDs[iGroup]->writeFlux();
  }
}
//==============================================================================

