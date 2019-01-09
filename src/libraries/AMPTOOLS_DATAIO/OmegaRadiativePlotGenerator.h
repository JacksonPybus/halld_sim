#if !(defined OMEGARADIATIVEPLOTGENERATOR)
#define OMEGARADIATIVEPLOTGENERATOR

#include <vector>
#include <string>

#include "IUAmpTools/PlotGenerator.h"

using namespace std;

class FitResults;
class Kinematics;

class OmegaRadiativePlotGenerator : public PlotGenerator
{
    
public:
  
  // create an index for different histograms
  enum { kOmegaMass = 0, kCosThetaPi0, kCosThetaGamma, kPhiPi0, kPhiGamma, kCosTheta, kPhi, kphi, kPsi, kt, kNumHists};

  OmegaRadiativePlotGenerator( const FitResults& results );
  OmegaRadiativePlotGenerator( );
    
  void projectEvent( Kinematics* kin );
 
private:
  
  void createHistograms( );
 
};

#endif
