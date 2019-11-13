// File: SingleGroupTransport.cpp
// Purpose: define a single group transport object
// Date: October 28, 2019

#include <iostream>
#include <vector>
#include <iomanip>
#include <armadillo>
#include "Mesh.h"
#include "Material.h"
#include "Materials.h"
#include "MultiGroupTransport.h"
#include "SingleGroupTransport.h"
#include "StartingAngle.h"
#include "SimpleCornerBalance.h"
#include "../TPLs/yaml-cpp/include/yaml-cpp/yaml.h"
#include "../TPLs/eigen-git-mirror/Eigen/Eigen"

using namespace std; 
using namespace arma;

//==============================================================================
//! SingleGroupTransport class object constructor

SingleGroupTransport::SingleGroupTransport(int myEnergyGroup,\
  MultiGroupTransport * myMGT,\
  Materials * myMaterials,\
  Mesh * myMesh,\
  YAML::Node * myInput)
{
  energyGroup = myEnergyGroup;
  MGT = myMGT;
  mats = myMaterials;
  mesh = myMesh;
  input = myInput;

  aFlux.set_size(mesh->zCent.size(),mesh->rCent.size(),mesh->nAngles);
  aFlux.zeros();
  aHalfFlux.set_size(mesh->zCent.size(),mesh->rCent.size(),\
    mesh->quadrature.size());
  aHalfFlux.zeros();
  sFlux.setOnes(mesh->zCent.size(),mesh->rCent.size());
  sFluxPrev.setOnes(mesh->zCent.size(),mesh->rCent.size());
  sFluxPrev = sFluxPrev;
  alpha.setZero(mesh->zCent.size(),mesh->rCent.size());
  q.setZero(mesh->zCent.size(),mesh->rCent.size());
  scatterSource.setZero(mesh->zCent.size(),mesh->rCent.size());
  fissionSource.setZero(mesh->zCent.size(),mesh->rCent.size());
  cout << "Created transport energy group " << energyGroup << endl;
};

//==============================================================================

//==============================================================================
//! solveStartAngle call starting angle solver

void SingleGroupTransport::solveStartAngle()
{
  aHalfFlux.zeros();
  MGT->startAngleSolve->calcStartingAngle(&aHalfFlux,&q,&alpha,energyGroup);
};

//==============================================================================

//==============================================================================
//! solveSCB call starting angle solver

void SingleGroupTransport::solveSCB()
{
  aFlux.zeros();  
  MGT->SCBSolve->solve(&aFlux,&aHalfFlux,&q,&alpha,energyGroup);
};

//==============================================================================

//==============================================================================
//! calcSource calculate the source in an energy group

// Assuming the fluxes currently contained in each SGT object
double SingleGroupTransport::calcSource(string calcType)
{ 
  double residual; 
  Eigen::MatrixXd q_old = q;

  if (calcType=="s" or calcType=="S"){
    calcScatterSource();
    q = scatterSource + fissionSource;
  } else if (calcType == "fs" or calcType == "FS"){
    calcScatterSource();
    calcFissionSource();
    q = scatterSource+fissionSource;
  }
  residual =  (q_old-q).norm();
  return residual;
};

//==============================================================================

//==============================================================================
//! calcFissionSource calculate the fission source in an energy group

// Assuming the fluxes currently contained in each SGT object
double SingleGroupTransport::calcFissionSource()
{  
  
  double sourceNorm;
  Eigen::MatrixXd fissionSource_old = fissionSource;
  Eigen::MatrixXd fissionSourceDiff;
  fissionSource.setZero(mesh->zCent.size(),mesh->rCent.size());
  
  for (int iZ = 0; iZ < mesh->zCent.size(); ++iZ){

    for (int iR = 0; iR < mesh->rCent.size(); ++iR){

      for (int iGroup = 0; iGroup < MGT->SGTs.size(); ++iGroup){

        fissionSource(iZ,iR) = fissionSource(iZ,iR) + (1.0/mesh->totalWeight)*(
        mats->chiP(iZ,iR,energyGroup)*mats->nu(iZ,iR)*mats->sigF(iZ,iR,iGroup)\
        *MGT->SGTs[iGroup]->sFlux(iZ,iR));
        // need to account for precursors, too.
      } // iGroup
    } // iR
  } // iZ
   
  //sourceNorm = ((fissionSource_old-fissionSource)\
  //  .cwiseQuotient(fissionSource)).norm();
  cout << "Old fission source: " << endl;
  cout << fissionSource_old << endl;
  cout << endl;
  cout << "New fission source: " << endl;
  cout << fissionSource << endl;
  cout << endl;

  fissionSourceDiff = (fissionSource_old-fissionSource);

  for (int iRow = 0; iRow < fissionSourceDiff.rows(); ++iRow){
    for (int iCol = 0; iCol < fissionSourceDiff.cols(); ++iCol){
      if (fissionSource(iRow,iCol)!=0){
        fissionSourceDiff(iRow,iCol) = fissionSourceDiff(iRow,iCol)\
          /fissionSource(iRow,iCol);
      } else {
        fissionSourceDiff(iRow,iCol) = 0;
      }
    }
  }

  sourceNorm=fissionSourceDiff.norm();
  
  return sourceNorm;

};

//==============================================================================

//==============================================================================
//! calcScatteringSource calculate the fission source in an energy group

// Assuming the fluxes currently contained in each SGT object
double SingleGroupTransport::calcScatterSource()
{  
  
  double sourceNorm;
  Eigen::MatrixXd scatterSource_old = scatterSource;
  scatterSource.setZero(mesh->zCent.size(),mesh->rCent.size());
  
  for (int iZ = 0; iZ < mesh->zCent.size(); ++iZ){

    for (int iR = 0; iR < mesh->rCent.size(); ++iR){

      for (int iGroup = 0; iGroup < MGT->SGTs.size(); ++iGroup){

        scatterSource(iZ,iR) = scatterSource(iZ,iR) + (1.0/mesh->totalWeight)*(
        mats->sigS(iZ,iR,iGroup,energyGroup)*MGT->SGTs[iGroup]->sFlux(iZ,iR));

        // need to account for precursors, too.
      } // iGroup
    } // iR
  } // iZ
   
  sourceNorm = ((scatterSource_old-scatterSource)\
    .cwiseQuotient(scatterSource)).norm();
  return sourceNorm;
};

//==============================================================================


//==============================================================================
//! calcFlux calculate the flux in this energy group

// Assuming the fluxes currently contained in each SGT object
double SingleGroupTransport::calcFlux()
{
  
  double weight,residual; 
  int weightIdx=3,angIdx;
  Eigen::MatrixXd sFlux_old = sFlux;

  sFlux.setZero(mesh->zCent.size(),mesh->rCent.size());

  for (int iQ = 0; iQ < mesh->quadrature.size(); ++iQ){
    for (int iP = 0; iP < mesh->quadrature[iQ].nOrd; ++iP){

      weight = mesh->quadrature[iQ].quad[iP][weightIdx];
      angIdx = mesh->quadrature[iQ].ordIdx[iP];

      for (int iZ = 0; iZ < mesh->zCent.size(); ++iZ){
        for (int iR = 0; iR < mesh->rCent.size(); ++iR){
          
          sFlux(iZ,iR) = sFlux(iZ,iR)\
          +weight*aFlux(iZ,iR,angIdx);
        
        } // iR
      } // iZ
    } // iP
  } // iQ
  
  residual = ((sFlux_old-sFlux).cwiseQuotient(sFlux)).norm();
  return residual;

};

//==============================================================================

//==============================================================================
//! calcFissionSource calculate the fission source in an energy group

// Assuming the fluxes currently contained in each SGT object
double SingleGroupTransport::calcAlpha()
{  
  
  double residual,deltaT = 0.0001;
  Eigen::MatrixXd alpha_old = alpha;
  alpha.setZero(mesh->zCent.size(),mesh->rCent.size());

  for (int iZ = 0; iZ < mesh->zCent.size(); ++iZ){
    for (int iR = 0; iR < mesh->rCent.size(); ++iR){
      alpha(iZ,iR) = (1.0/deltaT)*log(sFlux(iZ,iR)/sFluxPrev(iZ,iR));
    } // iR
  } // iZ
   
  cout << "Old alpha: " << endl;
  cout << alpha_old << endl;
  cout << endl;
  cout << "New alpha: " << endl;
  cout << alpha << endl;
  cout << endl;

  residual = ((alpha_old-alpha).cwiseQuotient(alpha)).norm();

  return residual;

};

//==============================================================================


//==============================================================================
//! writeFlux write the flux in this energy group to a CVS

void SingleGroupTransport::writeFlux()
{

  ofstream fluxFile;
  string fileName;
  
  // parse file name 
  fileName = "scalar-flux-group-" + to_string(energyGroup) + ".csv";
  fluxFile.open(fileName);

  // write flux values to .csv
  for (int iZ = 0; iZ < sFlux.rows(); ++iZ) {
    fluxFile << sFlux(iZ,0);
    for (int iR = 1; iR < sFlux.cols(); ++iR) {
      fluxFile <<","<< sFlux(iZ,iR);
    }
    fluxFile << endl;
  }
  fluxFile.close();

  // if this is the first energy group, write mesh too
  if (energyGroup==0){
    // write radial mesh to .csv 
    fileName = "r-mesh.csv"; 
    fluxFile.open(fileName); 
    fluxFile << mesh->rCent(0);
    for (int iR = 1; iR < mesh->rCent.size(); ++iR) {
      fluxFile << "," << mesh->rCent(iR);
    }
    fluxFile << endl;
    fluxFile.close();
  
    // write axial mesh to .csv
    fileName = "z-mesh.csv"; 
    fluxFile.open(fileName); 
    fluxFile << mesh->zCent(0);
    for (int iZ = 1; iZ < mesh->zCent.size(); ++iZ) {
      fluxFile << "," << mesh->zCent(iZ);
    }
    fluxFile << endl;
    fluxFile.close();
  }
  
};

//==============================================================================




