#if !(defined TWOPIPLOTGENERATOR)
#define TWOPIPLOTGENERATOR

#include <vector>
#include <string>

#include "IUAmpTools/PlotGenerator.h"

using namespace std;

class FitResults;
class Kinematics;

class TwoPiPlotGenerator : public PlotGenerator
{
    
public:
  
  // create an index for different histograms
  enum { k2PiMass = 0, kPiPCosTheta, kPhiPiPlus, kPhiPiMinus, kPhi, kphi, kPsi, kt, kNumHists};
  
  TwoPiPlotGenerator( const FitResults& results );
  TwoPiPlotGenerator( );

  void projectEvent( Kinematics* kin );
  
private:
        
  void createHistograms();
  
};

#endif
