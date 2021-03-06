#ifndef QUASIDIFFUSIONSOLVER_H
#define QUASIDIFFUSIONSOLVER_H

#include "Mesh.h"
#include "Materials.h"
#include "MultiPhysicsCoupledQD.h"
#include "PETScWrapper.h"

class SingleGroupQD;
class GreyGroupQD;

using namespace std; 

//==============================================================================
//! QDSolver class that solves RZ neutron transport

class QDSolver
{
  public:
    // public functions
    QDSolver(Mesh * myMesh,\
        Materials * myMaterials,\
        YAML::Node * myInput);
    void formLinearSystem(SingleGroupQD * SGQD);
    void formSteadyStateLinearSystem(SingleGroupQD * SGQD);
    void formBackCalcSystem(SingleGroupQD * SGQD);
    void formSteadyStateBackCalcSystem(SingleGroupQD * SGQD);

    // functions to map grid indices to global index
    vector<int> getIndices(int iR,int iZ,int energyGroup);

    // functions to calculate geometry parameters
    double calcVolAvgR(double rDown,double rUp);
    vector<double> calcGeoParams(int iR,int iZ);
    
    // functions to get Eddington factors
    double getWestErr(int iZ,int iR, SingleGroupQD * SGQD);
    double getWestErz(int iZ,int iR, SingleGroupQD * SGQD);
    double getEastErr(int iZ,int iR, SingleGroupQD * SGQD);
    double getEastErz(int iZ,int iR, SingleGroupQD * SGQD);
    double getNorthEzz(int iZ,int iR, SingleGroupQD * SGQD);
    double getNorthErz(int iZ,int iR, SingleGroupQD * SGQD);
    double getSouthEzz(int iZ,int iR, SingleGroupQD * SGQD);
    double getSouthErz(int iZ,int iR, SingleGroupQD * SGQD);

    // functions to enforce governing equations
    void assertZerothMoment(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void applyRadialBoundary(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void applyAxialBoundary(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // functions to enforce steady state governing equations
    void assertSteadyStateZerothMoment(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void applySteadyStateRadialBoundary(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void applySteadyStateAxialBoundary(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // functions to enforce coefficients for facial currents
    void southCurrent(double coeff,int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void northCurrent(double coeff,int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void westCurrent(double coeff,int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void eastCurrent(double coeff,int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // functions to enforce coefficients for steady state facial currents
    void steadyStateSouthCurrent(double coeff,int iR,int iZ,int iEq,\
        int energyGroup, SingleGroupQD * SGQD);
    void steadyStateNorthCurrent(double coeff,int iR,int iZ,int iEq,\
        int energyGroup, SingleGroupQD * SGQD);
    void steadyStateWestCurrent(double coeff,int iR,int iZ,int iEq,\
        int energyGroup, SingleGroupQD * SGQD);
    void steadyStateEastCurrent(double coeff,int iR,int iZ,int iEq,\
        int energyGroup, SingleGroupQD * SGQD);

    // functions to enforce coefficients for calculation of facial currents
    void calcSouthCurrent(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void calcNorthCurrent(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void calcWestCurrent(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void calcEastCurrent(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // functions to enforce coefficients for calculation of steady state 
    // facial currents
    void calcSteadyStateSouthCurrent(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void calcSteadyStateNorthCurrent(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void calcSteadyStateWestCurrent(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void calcSteadyStateEastCurrent(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // functions to assert flux BCs
    void assertWFluxBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertEFluxBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertNFluxBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSFluxBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // functions to assert current BCs
    void assertWCurrentBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertECurrentBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertNCurrentBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSCurrentBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // functions to assert steady state current BCs
    void assertSteadyStateWCurrentBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSteadyStateECurrentBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSteadyStateNCurrentBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSteadyStateSCurrentBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // functions to assert Gol'din BCs
    void assertNGoldinBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSGoldinBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertEGoldinBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // functions to assert Gol'din diffusion BCs
    void assertNGoldinP1BC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSGoldinP1BC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertEGoldinP1BC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // functions to assert steady state Gol'din BCs
    void assertSteadyStateNGoldinBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSteadyStateSGoldinBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSteadyStateEGoldinBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // functions to assert Gol'din diffusion BCs
    void assertSteadyStateNGoldinP1BC(int iR,int iZ,int iEq,\
        int energyGroup,SingleGroupQD * SGQD);
    void assertSteadyStateSGoldinP1BC(int iR,int iZ,int iEq,\
        int energyGroup,SingleGroupQD * SGQD);
    void assertSteadyStateEGoldinP1BC(int iR,int iZ,int iEq,\
        int energyGroup,SingleGroupQD * SGQD);

    // wrapper to assert either a flux or current BC
    void assertWBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertEBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertNBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // wrapper to assert either a flux or current BC
    void assertSteadyStateWBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSteadyStateEBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSteadyStateNBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSteadyStateSBC(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    double calcScatterAndFissionCoeff(int iR,int iZ,int toEnergyGroup,\
        int fromEnergyGroup);
    void greyGroupSources(int iR,int iZ,int iEq,int toEnergyGroup,\
        vector<double> geoParams);
    void steadyStateGreyGroupSources(int iR,int iZ,int iEq,int toEnergyGroup,\
        vector<double> geoParams);
    double calcIntegratingFactor(int iR,int iZ,double rEval,\
        SingleGroupQD * SGQD);

    // function to solve linear system
    void solve();
    void solveIterative();
    int solveSuperLU();
    int solveIterativeDiag();
    int solveIterativeILU();
    void backCalculateCurrent();

    // function to parse solution vector
    int getFlux(SingleGroupQD * SGQD);
    Eigen::VectorXd getFluxSolutionVector(SingleGroupQD * SGQD);
    Eigen::VectorXd getCurrentSolutionVector(SingleGroupQD * SGQD);

    // check for input parameters
    void checkOptionalParams();

    // public variables
    Eigen::SparseMatrix<double,Eigen::RowMajor> A,C;
    Eigen::VectorXd x;
    Eigen::VectorXd xPast,currPast;
    Eigen::VectorXd b,d;
    int energyGroups,nR,nZ,nGroupUnknowns,nGroupCurrentUnknowns;
    int nUnknowns,nCurrentUnknowns;
    int preconditioner = 1;
    bool reflectingBCs = false;
    bool goldinBCs = false;
    bool diffusionBCs = false;
    bool useMPQDSources = false;
    MultiPhysicsCoupledQD * mpqd;

    /* PETSc stuff */
    // PETSc variables (the p denotes a petsc variable)
    Vec x_p,b_p,d_p;
    Vec xPast_p,currPast_p;
    Vec xPast_p_seq,currPast_p_seq;
    Mat A_p,C_p;
    KSP ksp;
    PC pc;
   
    /* =========== TRANSIENT AND STEADY-STATE FUNCTIONS =============*/ 
    int solve_p();
    int backCalculateCurrent_p();

    // functions to assert flux BCs
    int assertWFluxBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    int assertEFluxBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    int assertNFluxBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    int assertSFluxBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    /*=================== TRANSIENT FUNCTIONS =======================*/
    
    void formLinearSystem_p(SingleGroupQD * SGQD);
    void formBackCalcSystem_p(SingleGroupQD * SGQD);

    // functions to enforce steady state governing equations
    int assertZerothMoment_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void applyRadialBoundary_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void applyAxialBoundary_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // functions to enforce coefficients for steady state facial currents
    int southCurrent_p(double coeff,int iR,int iZ,int iEq,\
        int energyGroup, SingleGroupQD * SGQD);
    int northCurrent_p(double coeff,int iR,int iZ,int iEq,\
        int energyGroup, SingleGroupQD * SGQD);
    int westCurrent_p(double coeff,int iR,int iZ,int iEq,\
        int energyGroup, SingleGroupQD * SGQD);
    int eastCurrent_p(double coeff,int iR,int iZ,int iEq,\
        int energyGroup, SingleGroupQD * SGQD);

    // functions to assert steady state current BCs
    void assertWCurrentBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertECurrentBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertNCurrentBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSCurrentBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // functions to assert steady state Gol'din BCs
    int assertNGoldinBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    int assertSGoldinBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    int assertEGoldinBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // functions to assert Gol'din diffusion BCs
    int assertNGoldinP1BC_p(int iR,int iZ,int iEq,\
        int energyGroup,SingleGroupQD * SGQD);
    int assertSGoldinP1BC_p(int iR,int iZ,int iEq,\
        int energyGroup,SingleGroupQD * SGQD);
    int assertEGoldinP1BC_p(int iR,int iZ,int iEq,\
        int energyGroup,SingleGroupQD * SGQD);

    // wrapper to assert either a flux or current BC
    void assertWBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertEBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertNBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // functions to enforce coefficients for calculation of steady state 
    // facial currents
    int calcSouthCurrent_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    int calcNorthCurrent_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    int calcWestCurrent_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    int calcEastCurrent_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    int greyGroupSources_p(int iR,int iZ,int iEq,int toEnergyGroup,\
        vector<double> geoParams);
    
    /*=================== STEADY-STATE FUNCTIONS ====================*/

    void formSteadyStateLinearSystem_p(SingleGroupQD * SGQD);
    void formSteadyStateBackCalcSystem_p(SingleGroupQD * SGQD);

    // functions to enforce steady state governing equations
    int assertSteadyStateZerothMoment_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void applySteadyStateRadialBoundary_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void applySteadyStateAxialBoundary_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // functions to enforce coefficients for steady state facial currents
    int steadyStateSouthCurrent_p(double coeff,int iR,int iZ,int iEq,\
        int energyGroup, SingleGroupQD * SGQD);
    int steadyStateNorthCurrent_p(double coeff,int iR,int iZ,int iEq,\
        int energyGroup, SingleGroupQD * SGQD);
    int steadyStateWestCurrent_p(double coeff,int iR,int iZ,int iEq,\
        int energyGroup, SingleGroupQD * SGQD);
    int steadyStateEastCurrent_p(double coeff,int iR,int iZ,int iEq,\
        int energyGroup, SingleGroupQD * SGQD);

    // functions to assert steady state current BCs
    void assertSteadyStateWCurrentBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSteadyStateECurrentBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSteadyStateNCurrentBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSteadyStateSCurrentBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // functions to assert steady state Gol'din BCs
    int assertSteadyStateNGoldinBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    int assertSteadyStateSGoldinBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    int assertSteadyStateEGoldinBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // functions to assert Gol'din diffusion BCs
    int assertSteadyStateNGoldinP1BC_p(int iR,int iZ,int iEq,\
        int energyGroup,SingleGroupQD * SGQD);
    int assertSteadyStateSGoldinP1BC_p(int iR,int iZ,int iEq,\
        int energyGroup,SingleGroupQD * SGQD);
    int assertSteadyStateEGoldinP1BC_p(int iR,int iZ,int iEq,\
        int energyGroup,SingleGroupQD * SGQD);

    // wrapper to assert either a flux or current BC
    void assertSteadyStateWBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSteadyStateEBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSteadyStateNBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    void assertSteadyStateSBC_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    // functions to enforce coefficients for calculation of steady state 
    // facial currents
    int calcSteadyStateSouthCurrent_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    int calcSteadyStateNorthCurrent_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    int calcSteadyStateWestCurrent_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);
    int calcSteadyStateEastCurrent_p(int iR,int iZ,int iEq,int energyGroup,\
        SingleGroupQD * SGQD);

    int steadyStateGreyGroupSources_p(int iR,int iZ,int iEq,int toEnergyGroup,\
        vector<double> geoParams);

  private:
    // private variables
    YAML::Node * input;
    Mesh * mesh;
    Materials * materials;
    // indices for accessing index and geometry parameters vectors
    const int iCF = 0;
    const int iWF = 1, iEF = 2, iNF = 3, iSF = 4;
    const int iWC = 5, iEC = 6, iNC = 7, iSC = 8;
    const int iluPreconditioner = 0, diagPreconditioner = 1;
};

//==============================================================================

#endif
