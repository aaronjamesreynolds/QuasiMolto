// File: HeatTransfer.cpp     
// Purpose: Contains information and builds linear system for heat transfer
// Date: April 9, 2020

#include "HeatTransfer.h"
#include "MultiPhysicsCoupledQD.h"

using namespace std;

//==============================================================================
/// HeatTransfer class object constructor
///
/// @param [in] myMaterials materials object for the simulation
/// @param [in] myMesh mesh object for the simulation
/// @param [in] myInput input object for the simulation
/// @param [in] myQD multiphysics coupled QD object for the simulation
HeatTransfer::HeatTransfer(Materials * myMaterials,\
  Mesh * myMesh,\
  YAML::Node * myInput,\
  MultiPhysicsCoupledQD * myMPQD)
{
  // Assign inputs to their member variables
  mats = myMaterials;
  mesh = myMesh;
  input = myInput;
  mpqd = myMPQD;

  // Check for optional inputs 
  if ((*input)["parameters"]["wallTemp"]){
    wallT=(*input)["parameters"]["wallTemp"].as<double>();
  } 
  if ((*input)["parameters"]["inletTemp"]){
    inletT=(*input)["parameters"]["inletTemp"].as<double>();
  } 
  if ((*input)["parameters"]["flux limiter"]){
    fluxLimiter=(*input)["parameters"]["flux limiter"].as<string>();
  } 

  // Set number of unknowns
  nUnknowns = mesh->nR*mesh->nZ;

  // Initialize size of matrices and vectors
  temp.setConstant(mesh->nZ,mesh->nR,inletT);
  flux.setZero(mesh->nZ+1,mesh->nR);
  dirac.setZero(mesh->nZ+1,mesh->nR);
  inletTemp.setConstant(2,mesh->nR,inletT);
  outletTemp.setZero(mesh->nR);
  inletDensity.setZero(mesh->nR);
  inletVelocity.setZero(mesh->nR);
  inletcP.setZero(mesh->nR);
  

  cout << "Initialized HeatTransfer object." << endl;
  cout << "wallTemp: " << wallT << endl;
  cout << "inletTemp: " << inletT << endl;
  cout << getIndex(4,4) << endl; 
};
//==============================================================================

//==============================================================================
/// Build linear system to solve heat equation
///
void HeatTransfer::buildLinearSystem()
{
  
  int myIndex,sIndex,nIndex,wIndex,eIndex,iEq = indexOffset;
  int nR = temp.cols()-1;
  int nZ = temp.rows()-1;
  double harmonicAvg,coeff;
  vector<double> gParams;
  
  for (int iR = 0; iR < temp.cols(); iR++)
  {
    for (int iZ = 0; iZ < temp.rows(); iZ++)
    {
      myIndex = getIndex(iZ,iR);     
      sIndex = getIndex(iZ+1,iR);     
      nIndex = getIndex(iZ-1,iR);     
      wIndex = getIndex(iZ,iR-1);     
      eIndex = getIndex(iZ,iR+1);     

      gParams = mesh->getGeoParams(iR,iZ);
      
      mpqd->A.coeffRef(iEq,myIndex) = mats->density(iZ,iR)*mats->cP(iZ,iR);
      
      // East face
      if (iR == nR)
      {
        coeff = (-gParams[iEF]*mats->k(iZ,iR)\
        /mesh->drsCorner(iR))/gParams[iVol];
        mpqd->A.coeffRef(iEq,myIndex) -= mesh->dt*coeff;
        mpqd->b(iEq) -= mesh->dt*coeff*wallT; 
      } else
      {
        harmonicAvg = pow(mesh->drsCorner(iR)/mats->k(iZ,iR)\
          + mesh->drsCorner(iR+1)/mats->k(iZ,iR+1),-1.0);
        coeff = -2.0*gParams[iEF]*harmonicAvg/gParams[iVol];
        mpqd->A.coeffRef(iEq,eIndex) = mesh->dt*coeff;
        mpqd->A.coeffRef(iEq,myIndex) -= mesh->dt*coeff;
      }

      // West face
      if (iR != 0)
      {
        harmonicAvg = pow(mesh->drsCorner(iR-1)/mats->k(iZ,iR-1)\
          + mesh->drsCorner(iR)/mats->k(iZ,iR),-1.0);
        coeff = 2.0*gParams[iWF]*harmonicAvg/gParams[iVol];
        mpqd->A.coeffRef(iEq,wIndex) = -mesh->dt*coeff;
        mpqd->A.coeffRef(iEq,myIndex) += mesh->dt*coeff;
      } 

      // North face
      if (iZ == 0 and mats->posVelocity)
      {
        coeff = (gParams[iNF]*mats->k(iZ,iR)\
        /mesh->dzsCorner(iZ))/gParams[iVol];
        mpqd->A.coeffRef(iEq,myIndex) += mesh->dt*coeff;
        mpqd->b(iEq) += mesh->dt*coeff*inletTemp(1,iR);             
      } else if (iZ != 0)
      {
        harmonicAvg = pow(mesh->dzsCorner(iZ-1)/mats->k(iZ-1,iR)\
          + mesh->dzsCorner(iZ)/mats->k(iZ,iR),-1.0);
        coeff = 2.0*gParams[iNF]*harmonicAvg/gParams[iVol];
        mpqd->A.coeffRef(iEq,nIndex) = -mesh->dt*coeff;
        mpqd->A.coeffRef(iEq,myIndex) += mesh->dt*coeff;
      }

      // South face
      if (iZ == nZ and !(mats->posVelocity))
      {
        coeff = -(gParams[iSF]*mats->k(iZ,iR)\
        /mesh->dzsCorner(iZ))/gParams[iVol];
        mpqd->A.coeffRef(iEq,myIndex) -= mesh->dt*coeff;
        mpqd->b(iEq) -= mesh->dt*coeff*inletTemp(0,iR);             
      } else if (iZ != nZ)
      {
        harmonicAvg = pow(mesh->dzsCorner(iZ+1)/mats->k(iZ+1,iR)\
          + mesh->dzsCorner(iZ)/mats->k(iZ,iR),-1.0);
        coeff = -2.0*gParams[iSF]*harmonicAvg/gParams[iVol];
        mpqd->A.coeffRef(iEq,sIndex) = mesh->dt*coeff;
        mpqd->A.coeffRef(iEq,myIndex) -= mesh->dt*coeff;
      }

      // Time term
      mpqd->b(iEq) += mats->density(iZ,iR)*mats->cP(iZ,iR)*temp(iZ,iR); 

      // Flux source term 
      coeff = mesh->dt*mats->omega(iZ,iR)*mats->oneGroupXS->sigF(iZ,iR);
      mpqd->fluxSource(iZ,iR,iEq,coeff);

      // Advection term
      mpqd->b(iEq) += (mesh->dt/mesh->dzsCorner(iZ))*(flux(iZ,iR)-flux(iZ+1,iR));

      // Iterate equation count
      iEq = iEq + 1;
  
    }
  }
   
};
//==============================================================================


//==============================================================================
/// Calculate energy diracs
///
void HeatTransfer::calcDiracs()
{
  double TupwindInterface,Tinterface,theta,phi;
  int lastDiracIndex = dirac.rows()-1;

  if (mats->posVelocity) 
  {
    for (int iR = 0; iR < dirac.cols(); iR++)
    {
      // Handle iZ=0 case
      TupwindInterface = inletTemp(1,iR) - inletTemp(0,iR);
      Tinterface = temp(0,iR) - inletTemp(1,iR);
      theta = calcTheta(TupwindInterface,Tinterface);
      phi = calcPhi(theta,fluxLimiter); 
      dirac(0,iR) = phi*Tinterface;

      // Handle iZ=1 case
      TupwindInterface = temp(0,iR) - inletTemp(1,iR);
      Tinterface = temp(1,iR) - temp(0,iR);
      theta = calcTheta(TupwindInterface,Tinterface);
      phi = calcPhi(theta,fluxLimiter); 
      dirac(1,iR) = phi*Tinterface; 
      
      // Handle all other cases
      for (int iZ = 2; iZ < dirac.rows()-1; iZ++)
      {

        TupwindInterface = temp(iZ-1,iR) - temp(iZ-2,iR);
        Tinterface = temp(iZ,iR) - temp(iZ-1,iR);
        theta = calcTheta(TupwindInterface,Tinterface);
        phi = calcPhi(theta,fluxLimiter); 
        dirac(iZ,iR) = phi*Tinterface; 
        
      }

      // Handle iZ = nZ case
      TupwindInterface = temp(lastDiracIndex-1,iR) - temp(lastDiracIndex-2,iR);
      Tinterface = outletTemp(iR) - temp(lastDiracIndex-1,iR);
      theta = calcTheta(TupwindInterface,Tinterface);
      phi = calcPhi(theta,fluxLimiter); 
      dirac(lastDiracIndex,iR) = phi*Tinterface; 

    }
  } else 
  {
    for (int iR = 0; iR < dirac.cols(); iR++)
    {
      // Handle iZ=0 case
      TupwindInterface = temp(1,iR) - temp(0,iR);
      Tinterface = temp(0,iR) - outletTemp(iR);
      theta = calcTheta(TupwindInterface,Tinterface);
      phi = calcPhi(theta,fluxLimiter); 
      dirac(0,iR) = phi*Tinterface; 
      
      // Handle all other cases
      for (int iZ = 1; iZ < dirac.rows()-2; iZ++)
      {

        TupwindInterface = temp(iZ+1,iR) - temp(iZ,iR);
        Tinterface = temp(iZ,iR) - temp(iZ-1,iR);
        theta = calcTheta(TupwindInterface,Tinterface);
        phi = calcPhi(theta,fluxLimiter); 
        dirac(iZ,iR) = phi*Tinterface; 
        
      }

      // Handle iZ = nZ-1 case
      TupwindInterface = inletTemp(0,iR) - temp(lastDiracIndex-1,iR);
      Tinterface = temp(lastDiracIndex-1,iR) - temp(lastDiracIndex-2,iR);
      theta = calcTheta(TupwindInterface,Tinterface);
      phi = calcPhi(theta,fluxLimiter); 
      dirac(lastDiracIndex-1,iR) = phi*Tinterface; 

      // Handle iZ = nZ case
      TupwindInterface = inletTemp(1,iR) - inletTemp(0,iR);
      Tinterface = inletTemp(0,iR) - temp(lastDiracIndex-1,iR);
      theta = calcTheta(TupwindInterface,Tinterface);
      phi = calcPhi(theta,fluxLimiter); 
      dirac(lastDiracIndex,iR) = phi*Tinterface; 
    }

  }
  
  cout << "calculated diracs" << endl;
  cout << dirac << endl;
  
};
//==============================================================================

//==============================================================================
/// Calculate energy flux 
///
void HeatTransfer::calcFluxes()
{

  double tdc; // shorthand for temp*density*specific heat 
  int lastFluxIndex = flux.rows()-1;
 
  if (mats->posVelocity) {

    for (int iR = 0; iR < flux.cols(); iR++)
    {
      // Handle iZ = 0 case
      tdc = inletVelocity(iR)*inletDensity(iR)*inletcP(iR);
      flux(0,iR) = tdc*inletTemp(1,iR)\
        + 0.5*abs(tdc)*(1-abs(tdc*mesh->dt/mesh->dzsCorner(0)))\
        *dirac(0,iR);

      // Handle all other cases
      for (int iZ = 1; iZ < flux.rows(); iZ++)
      {
        tdc = mats->flowVelocity(iZ-1,iR)*mats->density(iZ-1,iR)\
          *mats->cP(iZ-1,iR);
        flux(iZ,iR) = tdc*temp(iZ-1,iR)\
          + 0.5*abs(tdc)*(1-abs(tdc*mesh->dt/mesh->dzsCorner(iZ-1)))\
          *dirac(iZ,iR);
      }

    }
    
  } else
  {

    for (int iR = 0; iR < flux.cols(); iR++)
    {

      // Handle all other cases
      for (int iZ = 0; iZ < flux.rows()-1; iZ++)
      {
        tdc = mats->flowVelocity(iZ,iR)*mats->density(iZ,iR)\
          *mats->cP(iZ,iR);
        flux(iZ,iR) = tdc*temp(iZ,iR)\
          + 0.5*abs(tdc)*(1-abs(tdc*mesh->dt/mesh->dzsCorner(iZ)))*dirac(iZ,iR);
      }
      
      // Handle iZ = nZ case
      tdc = inletVelocity(iR)*inletDensity(iR)*inletcP(iR);
      flux(lastFluxIndex,iR) = tdc*inletTemp(0,iR)\
        + 0.5*abs(tdc)*(1-abs(tdc*mesh->dt/mesh->dzsCorner(lastFluxIndex-1)))\
        *dirac(lastFluxIndex,iR);

    }
  }
  
  cout << "calculated fluxes" << endl;
  cout << flux << endl;

};
//==============================================================================

//==============================================================================
/// Calculate theta for use in flux limiting framework 
///
/// @param [in] TupwindInterface change in temp at upwind interface
/// @param [in] Tinterface change in temp at interface
double HeatTransfer::calcTheta(double TupwindInterface, double Tinterface)
{
 
  double theta;
  if (abs(Tinterface) < 1E-10)
  {
    theta = 1;
  } else 
  {
    theta = TupwindInterface/Tinterface;
  }

  return theta;
  
};
//==============================================================================


//==============================================================================
/// Calculate phi for use in flux limiting framework 
///
/// @param [in] theta ratio of change in temperature in upwind and current cell
/// @param [in] fluxLimiter string indicating how to calculate phi
/// @param [out] phi flux limiting parameter
double HeatTransfer::calcPhi(double theta, string fluxLimiter)
{
 
  double phi; 
  Eigen::Vector2d fluxLimiterArg1,fluxLimiterArg2;
  Eigen::Vector3d fluxLimiterArg3;

  if (fluxLimiter == "superbee")
  {

    fluxLimiterArg1 << 1,2*theta; 
    fluxLimiterArg2 << 2,theta; 
    fluxLimiterArg3 << 0,\
      fluxLimiterArg1.minCoeff(),\
      fluxLimiterArg2.minCoeff();
    phi = fluxLimiterArg3.maxCoeff();

  } else if (fluxLimiter == "upwind") 
  {
    phi = 0.0;
  } else if (fluxLimiter == "lax-wendroff")
  {
    phi = 1.0;
  } else if (fluxLimiter == "beam-warming")
  {
    phi = theta;
  }

  return phi;
  
};
//==============================================================================

//==============================================================================
/// Assign boundary indices depending on direction of flow velocity
///
void HeatTransfer::assignBoundaryIndices()
{
  if (mats->posVelocity)
  { 
    coreInletIndex = 0;
    coreOutletIndex = mesh->nZ-1;
  } else 
  {
    coreInletIndex = mesh->nZ-1;
    coreOutletIndex = 0;
  }
  
};
//==============================================================================

//==============================================================================
/// Update temperatures and physical parameters at inlet and outlet     
///
void HeatTransfer::updateBoundaryConditions()
{

  // Update variables that can set with array splicing
  inletTemp.setConstant(inletTemp.rows(),inletTemp.cols(),inletT);
  outletTemp = temp.row(coreOutletIndex);   
  inletVelocity = mats->flowVelocity.row(coreInletIndex);
  
  // Update variables that require looping to access   
  for (int iR = 0; iR < mesh->nR; iR++)
  {
    cout << iR << endl;
    inletDensity(iR) = mats->density(coreInletIndex,iR);
    inletcP(iR) = mats->cP(coreInletIndex,iR);
  }

  cout << "inletTemp" << endl;
  cout << inletTemp << endl;
  cout << "temp" << endl;
  cout << temp << endl; 
};
//==============================================================================

//==============================================================================
/// Map 2D coordinates to index of temperature in the 1D solution vector
///
/// @param [in] iZ axial location
/// @param [in] iR radial location
/// @param [out] index the index for temperature in the 1D solution vector
int HeatTransfer::getIndex(int iZ, int iR)
{

  int index,nR = mesh->drsCorner.size();

  index = indexOffset + iR + nR*iZ;

  return index;
  
};
//==============================================================================

//==============================================================================
/// Map 2D coordinates to index of temperature in the 1D solution vector
///
/// @param [in] iZ axial location
/// @param [in] iR radial location
/// @param [out] index the index for temperature in the 1D solution vector
void HeatTransfer::getTemp()
{

  for (int iR = 0; iR < mesh-> drsCorner.size(); iR++)
  {
    for (int iZ = 0; iZ < mesh-> dzsCorner.size(); iZ++)
    { 
    
      temp(iZ,iR) = mpqd->x(getIndex(iZ,iR));   
    
    }
  }
  
};
//==============================================================================
