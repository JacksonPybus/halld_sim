#if !defined(TWOPIANGLESRADIATIVE)
#define TWOPIANGLESRADIATIVE

#include "IUAmpTools/Amplitude.h"
#include "IUAmpTools/UserAmplitude.h"
#include "IUAmpTools/AmpParameter.h"
#include "GPUManager/GPUCustomTypes.h"

#include "TH1D.h"
#include "TFile.h"
#include <string>
#include <complex>
#include <vector>

#ifdef GPU_ACCELERATION
void
GPUTwoPiAnglesRadiative_exec( dim3 dimGrid, dim3 dimBlock, GPU_AMP_PROTO,
                     int j, int m, GDouble bigTheta, GDouble refFact );

#endif // GPU_ACCELERATION

using std::complex;
using namespace std;

class Kinematics;

class TwoPiAnglesRadiative : public UserAmplitude< TwoPiAnglesRadiative >
{
    
public:
	
	TwoPiAnglesRadiative() : UserAmplitude< TwoPiAnglesRadiative >() { };
	TwoPiAnglesRadiative( const vector< string >& args );
	
	string name() const { return "TwoPiAnglesRadiative"; }
    
	complex< GDouble > calcAmplitude( GDouble** pKin ) const;
	
#ifdef GPU_ACCELERATION
  
	//void launchGPUKernel( dim3 dimGrid, dim3 dimBlock, GPU_AMP_PROTO ) const;
  
	bool isGPUEnabled() const { return true; }
  
#endif // GPU_ACCELERATION
  
private:

  AmpParameter rho000;
  AmpParameter rho100;
  AmpParameter rho1m10;
	
  AmpParameter rho111;
  AmpParameter rho001;
  AmpParameter rho101;
  AmpParameter rho1m11;

  AmpParameter rho102;
  AmpParameter rho1m12;

  AmpParameter polAngle;

  double polFraction;

  TH1D *totalFlux_vs_E;
  TH1D *polFlux_vs_E;
  TH1D *polFrac_vs_E;

};

#endif
