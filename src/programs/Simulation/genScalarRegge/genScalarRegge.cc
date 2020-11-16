// Main program for generating scalar events. 
#include "HDDM/hddm_s.h"
#include "particleType.h"

#include <TMath.h>
#include <TRandom3.h>
#include <TGenPhaseSpace.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TGraph.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
using namespace std;

#include "UTILITIES/BeamProperties.h"

// Photon beam energy for cross section plots
double EgammaPlot=8.5; 

// Masses
const double m_p=0.93827; // GeV
const double m_p_sq=m_p*m_p;
// Width
double width=0.;
// Coupling constants 
const double g0_sq=110.5; // GeV^-2
const double g_omega_V=15.;
const double gsq_omega_V=g_omega_V*g_omega_V;
const double g_rho_V=3.4;
const double g_rho_T=11.0; // GeV^-1
const double gsq_rho_T=g_rho_T*g_rho_T;
const double g_rho_V_and_T=g_rho_V+2.*m_p*g_rho_T;
const double gsq_rho_V_and_T=g_rho_V_and_T*g_rho_V_and_T;
// Regge cut parameters
double regge_cuts[5]; // dc c_P_omega c_f2_omega c_P_rho c_f2_rho 

int Nevents=10000;
int runNo=30300;
bool debug=false;

// Diagnostic histograms
TH1D *thrown_t;
TH1D *thrown_mass;
TH1D *thrown_dalitzZ;
TH1D *thrown_Egamma;
TH2D *thrown_dalitzXY;  
TH2D *thrown_theta_vs_p;
TH2D *thrown_mass_vs_E;
TH1D *cobrems_vs_E;

char input_file_name[50]="scalar.in";
char output_file_name[50]="scalar_gen.hddm";

void Usage(void){
  printf("genScalarRegge: generator for eta production based on Regge trajectory formalism.\n");
  printf(" Usage:  genScalarRegge <options>\n");
  printf("   Options:  -N<number of events> (number of events to generate)\n");
  printf("             -O<output.hddm>   (default: scalar_gen.hddm)\n");
  printf("             -I<input.in>      (default: scalar.in)\n");
  printf("             -R<run number>    (default: 30300)\n");
  printf("             -h                (Print this message and exit.)\n");
  printf("Photon beam energy range, Regge cut parameters, and decay products are\n");
  printf("specified in the <input.in> file.\n");

  exit(0);
}

//-----------
// ParseCommandLineArguments
//-----------
void ParseCommandLineArguments(int narg, char* argv[])
{
  int seed=0;
  if (narg==1){
    Usage();
  }
  for(int i=1; i<narg; i++){
    char *ptr = argv[i];
    
    if(ptr[0] == '-'){
      switch(ptr[1]){
      case 'h': Usage(); break;
      case 'I':
	sscanf(&ptr[2],"%s",input_file_name);
	break;
      case 'O':
	sscanf(&ptr[2],"%s",output_file_name);
	break;
      case 'N':
	sscanf(&ptr[2],"%d",&Nevents);
	break;
      case 'R':
	sscanf(&ptr[2],"%d",&runNo);
	break;
      case 'S':
	sscanf(&ptr[2],"%d",&seed);
	break;
      case 'E':
	char stmp[80];
	sscanf(&ptr[2],"%s",stmp);
	EgammaPlot=atof(stmp);
	break;
      case 'd':
	debug=true;
	break;
      default:
	break;
      }
    }
  }
}

// Non-resonant pi-pi or pi-eta background following Donnachie and Kalashnikova,
// arXiv:0806.3698v1
double BackgroundCrossSection(TLorentzVector &q /* beam */,
			      vector<Particle_t>&particle_types,
			      vector<TLorentzVector>&particles){
  
  int two_particles=particle_types[0]+particle_types[1];
  TLorentzVector p1(0,0,0.,ParticleMass(Proton));
  TLorentzVector p2=particles[2];
  TLorentzVector p=p1-p2;
  double t=p.M2();
  double s=(q+p1).M2();
  TLorentzVector v1=particles[0]-q;
  TLorentzVector v2=particles[1]-q;

  double p1_dot_p2=p1.Dot(p2);
  double q_dot_p=q.Dot(p);
  double q_dot_v1=q.Dot(v1);
  double p_dot_v1=p.Dot(v1);
  double q_dot_v2=q.Dot(v2);
  double p_dot_v2=p.Dot(v2);
  double v1sq=v1.M2();
  double v2sq=v2.M2();
  double v1sq_minus_v2sq=v1sq-v2sq;
  double v1_dot_v2=v1.Dot(v2);
  double psq=p.M2();
  double b1=q_dot_p*v1sq-q_dot_v1*p_dot_v1;
  double b2=q_dot_p*v2sq-q_dot_v2*p_dot_v2;
  TLorentzVector c1=p_dot_v1*q-q_dot_p*v1;
  TLorentzVector c2=p_dot_v2*q-q_dot_p*v2;
  TLorentzVector d1=q_dot_v1*v1-v1sq*q;
  TLorentzVector d2=q_dot_v2*v2-v2sq*q;
  TLorentzVector N1=b1*p1+p1.Dot(c1)*v1+p1.Dot(d1)*p;
  TLorentzVector N2=b2*p1+p1.Dot(c2)*v1+p1.Dot(d2)*p;
  double N1_N1=N1.Dot(N1);
  double N2_N2=N2.Dot(N2);
  double N1_N2=N1.Dot(N2);
  double M1_M2=p_dot_v1*p_dot_v1*q_dot_v2*q_dot_v2 
    - 2.*v1_dot_v2*q_dot_p*(p_dot_v1*q_dot_v2+p_dot_v2*q_dot_v1)
    + v1_dot_v2*v1_dot_v2*q_dot_p*q_dot_p + p_dot_v2*p_dot_v2*q_dot_v1*q_dot_v1
    + v2sq*p_dot_v1*q_dot_v1*q_dot_p + v1sq*p_dot_v2*q_dot_v2*q_dot_p
    + psq*v1_dot_v2*q_dot_v1*q_dot_v2 - psq*v2sq*q_dot_v1*q_dot_v1
    - psq*v1sq*q_dot_v2*q_dot_v2;
  double M1_M1=2.*p_dot_v1*p_dot_v1*q_dot_v1*q_dot_v1
    - 2.*v1sq*p_dot_v1*q_dot_v1*q_dot_p + v1sq*v1sq*q_dot_p*q_dot_p
    - psq*v1sq*q_dot_v1*q_dot_v1; 
  double M2_M2=2.*p_dot_v2*p_dot_v2*q_dot_v2*q_dot_v2
    - 2.*v2sq*p_dot_v2*q_dot_v2*q_dot_p + v2sq*v2sq*q_dot_p*q_dot_p
    - psq*v2sq*q_dot_v2*q_dot_v2;
  
  // Rho propagators for top exchange
  double m_rho=0.7685;
  double Gamma_rho=0.1462;
  double m_rhosq_minus_v1sq=m_rho*m_rho-v1sq;
  double m_Gamma_rho_sq=m_rho*m_rho*Gamma_rho*Gamma_rho;
  double Pi_rho_1_sq=1./(m_rhosq_minus_v1sq*m_rhosq_minus_v1sq+m_Gamma_rho_sq);
  double m_rhosq_minus_v2sq=m_rho*m_rho-v2sq;
  double Pi_rho_2_sq=1./(m_rhosq_minus_v2sq*m_rhosq_minus_v2sq+m_Gamma_rho_sq);
  double Re_rho_1_rho_2=m_rhosq_minus_v1sq*m_rhosq_minus_v2sq + m_Gamma_rho_sq;
  
  double s0=1.;
  // Regge trajectory for rho
  double a_rho=0.55+0.8*t;
  double a_rho_prime=0.8;
  double cos_rho=cos(M_PI*a_rho);
  double sin_rho=sin(M_PI*a_rho);
  double regge_rho=pow(s/s0,a_rho-1.)*M_PI*a_rho_prime/(sin_rho*TMath::Gamma(a_rho)); // excluding phase factor
  // Assume rotating phase:  regge_rho=f_rho(s,t) exp(-i pi a_rho(t))
  double regge_rho_sq=regge_rho*regge_rho;

  // omega propagators for top exchange
  double m_omega=0.78265;
  double Gamma_omega=0.00849;
  double m_omegasq_minus_v1sq=m_omega*m_omega-v1sq;
  double m_Gamma_omega_sq=m_omega*m_omega*Gamma_omega*Gamma_omega;
  double Pi_omega_1_sq=1./(m_omegasq_minus_v1sq*m_omegasq_minus_v1sq
			   + m_Gamma_omega_sq);
  double m_omegasq_minus_v2sq=m_omega*m_omega-v2sq;
  double Pi_omega_2_sq=1./(m_omegasq_minus_v2sq*m_omegasq_minus_v2sq
			   + m_Gamma_omega_sq);
  double Re_omega_1_omega_2=m_omegasq_minus_v1sq*m_omegasq_minus_v2sq 
    + m_Gamma_omega_sq;

  // Regge trajectory for omega
  double a_omega=0.44+0.9*t;
  double a_omega_prime=0.9; 
  double cos_omega=cos(M_PI*a_omega);
  double sin_omega=sin(M_PI*a_omega);
  double regge_omega=pow(s/s0,a_omega-1.)*M_PI*a_omega_prime/(sin_omega*TMath::Gamma(a_omega)); // excluding phase factor
  // Assume rotating phase:  regge_omega=f_omega(s,t) exp(-i pi a_omega(t))
  double regge_omega_sq=regge_omega*regge_omega;

  // rho-omega interference 
  double regge_rho_omega_sum
    =2.*regge_rho*regge_omega*cos(M_PI*(a_rho-a_omega));

  // i (Domega Drho* - Domega* Drho)
  double regge_rho_omega_diff  // check sign!
    =2.*regge_rho*regge_omega*sin(M_PI*(a_omega-a_rho));
  
  double Re_rho_1_omega_1=m_rhosq_minus_v1sq*m_omegasq_minus_v1sq
    + m_rho*m_omega*Gamma_rho*Gamma_omega;
  double Im_rho_1_omega_1=m_omega*Gamma_omega*m_rhosq_minus_v1sq
    - m_rho*Gamma_rho*m_omegasq_minus_v1sq;
  double Re_rho_2_omega_2=m_rhosq_minus_v2sq*m_omegasq_minus_v2sq
    + m_rho*m_omega*Gamma_rho*Gamma_omega;
  double Im_rho_2_omega_2=m_omega*Gamma_omega*m_rhosq_minus_v2sq
    - m_rho*Gamma_rho*m_omegasq_minus_v2sq;
  double Re_rho_2_omega_1=m_rhosq_minus_v2sq*m_omegasq_minus_v1sq
    + m_rho*m_omega*Gamma_rho*Gamma_omega;
  double Im_rho_2_omega_1=m_omega*Gamma_omega*m_rhosq_minus_v2sq
    - m_rho*Gamma_rho*m_omegasq_minus_v1sq;
  double Re_rho_1_omega_2=m_rhosq_minus_v1sq*m_omegasq_minus_v2sq
    + m_rho*m_omega*Gamma_rho*Gamma_omega;
  double Im_rho_1_omega_2=m_omega*Gamma_omega*m_rhosq_minus_v1sq
    - m_rho*Gamma_rho*m_omegasq_minus_v2sq;

  double rho_1_omega_1_interference
    = Pi_omega_1_sq*Pi_rho_1_sq*(Re_rho_1_omega_1*regge_rho_omega_sum 
				 + Im_rho_1_omega_1*regge_rho_omega_diff);
  double rho_2_omega_2_interference
    = Pi_omega_2_sq*Pi_rho_2_sq*(Re_rho_2_omega_2*regge_rho_omega_sum 
				 + Im_rho_2_omega_2*regge_rho_omega_diff);
  double rho_12_omega_21_interference
    =regge_rho_omega_sum*(Pi_rho_1_sq*Pi_omega_2_sq*Re_rho_1_omega_2
			  + Pi_rho_2_sq*Pi_omega_1_sq*Re_rho_2_omega_1
			  )
    + regge_rho_omega_diff*(Pi_rho_1_sq*Pi_omega_2_sq*Im_rho_1_omega_2
			    + Pi_rho_2_sq*Pi_omega_1_sq*Im_rho_2_omega_1
			    );
  double rho_2_omega_1_interference
    =Pi_omega_1_sq*Pi_rho_2_sq*(Re_rho_2_omega_1*regge_rho_omega_sum
				+ Im_rho_2_omega_1*regge_rho_omega_diff);
  double rho_1_omega_2_interference
    =Pi_omega_2_sq*Pi_rho_1_sq*(Re_rho_1_omega_2*regge_rho_omega_sum
				+ Im_rho_1_omega_2*regge_rho_omega_diff);
    
  // terms involving complex conjugates of Regge propagators and rho/omega propagtors
  double a1_a1=0.,b1_b1=0.;
  double a2_a2=0.,b2_b2=0.;
  double a1_a2=0.,b1_b2=0.;
  double b1_a1=0,b2_a2=0.,b1_a2=0.,b2_a1=0.;
  double Csq=1.,zetasq=1./2.;
  // Compute square of amplitude    
  if (two_particles==(7+7)){ // pi0 pi0
    a1_a1=gsq_rho_V_and_T*regge_rho_sq*Pi_omega_1_sq
      + (1./3.)*g_omega_V*g_rho_V_and_T*rho_1_omega_1_interference
      + (1./9.)*gsq_omega_V*regge_omega_sq*Pi_rho_1_sq;
    a2_a2=gsq_rho_V_and_T*regge_rho_sq*Pi_omega_2_sq
      + (1./3.)*g_omega_V*g_rho_V_and_T*rho_2_omega_2_interference
      + (1./9.)*gsq_omega_V*regge_omega_sq*Pi_rho_2_sq;
    a1_a2=2.*gsq_rho_V_and_T*regge_rho_sq*Pi_omega_1_sq*Pi_omega_2_sq
      * Re_omega_1_omega_2
      + (1./3.)*g_omega_V*g_rho_V_and_T*rho_12_omega_21_interference
      + (2./9.)*gsq_omega_V*regge_omega_sq*Pi_rho_1_sq*Pi_rho_2_sq
      *Re_rho_1_rho_2;
    
    b1_b1=4.*gsq_rho_T*regge_rho_sq*Pi_omega_1_sq; 
    b2_b2=4.*gsq_rho_T*regge_rho_sq*Pi_omega_2_sq;
    b1_b2=8.*gsq_rho_T*regge_rho_sq*Pi_omega_1_sq*Pi_omega_2_sq
      *Re_omega_1_omega_2;
    
    b1_a1=-2.*g_rho_T*(2.*g_rho_V_and_T*regge_rho_sq*Pi_omega_1_sq
		       +(1./3.)*g_omega_V*rho_1_omega_1_interference);
    b2_a2=-2.*g_rho_T*(2.*g_rho_V_and_T*regge_rho_sq*Pi_omega_2_sq
		       +(1./3.)*g_omega_V*rho_2_omega_2_interference);
    b1_a2=-2.*g_rho_T*(2.*g_rho_V_and_T*regge_rho_sq*Pi_omega_1_sq*Pi_omega_2_sq
		       * Re_omega_1_omega_2
		       + (1./3.)*g_omega_V*rho_2_omega_1_interference
		       );
    b2_a1=-2.*g_rho_T*(2.*g_rho_V_and_T*regge_rho_sq*Pi_omega_1_sq*Pi_omega_2_sq
		       * Re_omega_1_omega_2
		       + (1./3.)*g_omega_V*rho_1_omega_2_interference
		       );
  }
  else if (two_particles==(7+17)){ // pi0 eta
    Csq=2./3.;
    zetasq=1.;
    
    a1_a1=(1./9.)*gsq_rho_V_and_T*regge_rho_sq*Pi_rho_1_sq
      + (1./3.)*g_omega_V*g_rho_V_and_T*rho_1_omega_1_interference
      + gsq_omega_V*regge_omega_sq*Pi_omega_1_sq;
    a2_a2=(1./9.)*gsq_rho_V_and_T*regge_rho_sq*Pi_rho_2_sq
      + (1./3.)*g_omega_V*g_rho_V_and_T*rho_2_omega_2_interference
      + gsq_omega_V*regge_omega_sq*Pi_omega_2_sq;
    a1_a2=(2./9.)*gsq_rho_V_and_T*regge_rho_sq*Pi_rho_1_sq*Pi_rho_2_sq
      *Re_rho_1_rho_2
      + (1./3.)*g_omega_V*g_rho_V_and_T*rho_12_omega_21_interference
      + 2.*gsq_omega_V*Pi_omega_1_sq*Pi_omega_2_sq*regge_omega_sq
      *Re_omega_1_omega_2;

    b1_b1=(4./9.)*gsq_rho_T*regge_rho_sq*Pi_rho_1_sq; 
    b2_b2=(4./9.)*gsq_rho_T*regge_rho_sq*Pi_rho_2_sq;
    b1_b2=(8./9.)*gsq_rho_T*regge_rho_sq*Pi_rho_1_sq*Pi_rho_2_sq*Re_rho_1_rho_2;

    b1_a1=-2.*g_rho_T*((2./9.)*g_rho_V_and_T*regge_rho_sq*Pi_omega_1_sq
		       +(1./3.)*g_omega_V*rho_1_omega_1_interference);
    b2_a2=-2.*g_rho_T*((2./9.)*g_rho_V_and_T*regge_rho_sq*Pi_omega_2_sq
		       +(1./3.)*g_omega_V*rho_2_omega_2_interference);
    b1_a2=-2.*g_rho_T*((2./9.)*g_rho_V_and_T*regge_rho_sq*Pi_rho_1_sq
		       *Pi_rho_2_sq*Re_rho_1_rho_2
		       + (1./3.)*g_omega_V*rho_2_omega_1_interference
		       );
    b2_a1=-2.*g_rho_T*((2./9.)*g_rho_V_and_T*regge_rho_sq*Pi_omega_1_sq
		       *Pi_omega_2_sq* Re_omega_1_omega_2
		       + (1./3.)*g_omega_V*rho_1_omega_2_interference
		       );
  }

    /*
    b1_b1=0.;
    b2_b2=0.;
    b1_b2=0.;
    
    b1_a1=0.;
    b2_a2=0.;
    b1_a2=0.;
    b2_a1=0.;
    /*
    a2_a2=0.;
    a1_a1=0.;
    a1_a2=0.;
    */  

  double T=Csq*((m_p_sq-p1_dot_p2)*(a1_a1*M1_M1 + a1_a2*M1_M2 + a2_a2*M2_M2)
		+ 2.*(a1_a1*N1_N1 + a1_a2*N1_N2 + a2_a2*N2_N2)
		+ 2.*m_p*(b1_a1*N1_N1 + (b1_a2+b2_a1)*N1_N2 + b2_a2*N2_N2)
		+ (m_p_sq+p1_dot_p2)*(b1_b1*N1_N1 + b1_b2*N1_N2 + b2_b2*N2_N2));
    
  T*=zetasq*g0_sq;

  return T;
}

double InterferenceCrossSection(TLorentzVector &q /* beam */,
				vector<Particle_t>&particle_types,
				vector<TLorentzVector>&particles,
				double gR,double ReB, double ImB,
				double gsq_rho_S,double gsq_omega_S,
				double phase){ 
  int two_particles=particle_types[0]+particle_types[1];
  
  // Four vectors
  TLorentzVector p1(0,0,0.,ParticleMass(Proton));
  TLorentzVector p2=particles[2];
  TLorentzVector p=p1-p2;
  TLorentzVector v1=particles[0]-q;
  TLorentzVector v2=particles[1]-q;

  // Mandelstam variables
  double s=(q+p1).M2();
  double t=(p1-p2).M2();

  // dot products 
  double p1_dot_p2=p1.Dot(p2);
  double q_dot_p=q.Dot(p);
  double q_dot_v1=q.Dot(v1);
  double p_dot_v1=p.Dot(v1);
  double q_dot_v2=q.Dot(v2);
  double p_dot_v2=p.Dot(v2);
  double v1sq=v1.M2();
  double v2sq=v2.M2();
  double psq=p.M2();
  double b1=q_dot_p*v1sq-q_dot_v1*p_dot_v1;
  double b2=q_dot_p*v2sq-q_dot_v2*p_dot_v2;
  TLorentzVector c1=p_dot_v1*q-q_dot_p*v1;
  TLorentzVector c2=p_dot_v2*q-q_dot_p*v2;
  TLorentzVector d1=q_dot_v1*v1-v1sq*q;
  TLorentzVector d2=q_dot_v2*v2-v2sq*q;
  TLorentzVector N1=b1*p1+p1.Dot(c1)*v1+p1.Dot(d1)*p;
  TLorentzVector N2=b2*p1+p1.Dot(c2)*v1+p1.Dot(d2)*p;
  TLorentzVector N=(q_dot_p)*p1-q.Dot(p1)*p;
  double N_N1=N.Dot(N1);
  double N_N2=N.Dot(N2);
  double M_M1=3*b1*q_dot_p + v1.Dot(c1)*q_dot_p - p_dot_v1*q.Dot(c1)
    + p.Dot(d1)*q_dot_p - psq*q.Dot(d1);  
  double M_M2=3*b2*q_dot_p + v2.Dot(c2)*q_dot_p - p_dot_v2*q.Dot(c2)
    + p.Dot(d2)*q_dot_p - psq*q.Dot(d2);

  // Coupling constants 
  double g_rho_S=sqrt(gsq_rho_S);
  double g_omega_S=sqrt(gsq_omega_S);

  // Rho propagator for top exchange for double-exchange diagrams
  double m_rho=0.7685;
  double Gamma_rho=0.1462;
  double m_rhosq_minus_v1sq=m_rho*m_rho-v1sq;
  double Pi_rho_1_sq=1./(m_rhosq_minus_v1sq*m_rhosq_minus_v1sq
			 +m_rho*m_rho*Gamma_rho*Gamma_rho);
  double m_rhosq_minus_v2sq=m_rho*m_rho-v2sq;
  double Pi_rho_2_sq=1./(m_rhosq_minus_v2sq*m_rhosq_minus_v2sq
			 +m_rho*m_rho*Gamma_rho*Gamma_rho);
  
  double s0=1.;
  // Regge trajectory for rho
  double a_rho=0.55+0.8*t;
  double a_rho_prime=0.8;
  double cos_rho=cos(M_PI*a_rho);
  double one_minus_cos_rho=1.-cos_rho;
  double sin_rho=sin(M_PI*a_rho);
  double regge_rho=pow(s/s0,a_rho-1.)*M_PI*a_rho_prime/(sin_rho*TMath::Gamma(a_rho)); // excluding phase factor 

  // Regge cuts for rho
  double dc=regge_cuts[0];
  double a_rho_P=0.64+0.16*t; // Pomeron
  double a_rho_f2=0.222+0.404*t;
  double regge_rho_P_cut=exp(dc*t)*pow(s/s0,a_rho_P-1.);
  double regge_rho_f2_cut=exp(dc*t)*pow(s/s0,a_rho_f2-1.);

  // omega propagator for top exchange
  double m_omega=0.78265;
  double Gamma_omega=0.00849;
  double m_omegasq_minus_v1sq=m_omega*m_omega-v1sq;
  double Pi_omega_1_sq=1./(m_omegasq_minus_v1sq*m_omegasq_minus_v1sq
			   +m_omega*m_omega*Gamma_omega*Gamma_omega);
  double m_omegasq_minus_v2sq=m_omega*m_omega-v2sq;
  double Pi_omega_2_sq=1./(m_omegasq_minus_v2sq*m_omegasq_minus_v2sq
			   +m_omega*m_omega*Gamma_omega*Gamma_omega);

  // Regge trajectory for omega
  double a_omega=0.44+0.9*t;
  double a_omega_prime=0.9; 
  double cos_omega=cos(M_PI*a_omega);
  double one_minus_cos_omega=1.-cos_omega;
  double sin_omega=sin(M_PI*a_omega);
  double regge_omega=pow(s/s0,a_omega-1.)*M_PI*a_omega_prime/(sin_omega*TMath::Gamma(a_omega)); // excluding phase factor

  // Regge cuts for omega
  double a_omega_P=0.52+0.196*t; // Pomeron
  double a_omega_f2=0.112+0.428*t; 
  double regge_omega_P_cut=exp(dc*t)*pow(s/s0,a_omega_P-1.);
  double regge_omega_f2_cut=exp(dc*t)*pow(s/s0,a_omega_f2-1.);

  //Phase differences
  double a_rho_rho_P=a_rho-0.5*a_rho_P;
  double a_rho_rho_f2=a_rho-0.5*a_rho_f2;
  double a_omega_omega_P=a_omega-0.5*a_omega_P;
  double a_omega_omega_f2=a_omega-0.5*a_omega_f2;
  double a_omega_rho_P=a_omega-0.5*a_rho_P;
  double a_omega_rho_f2=a_omega-0.5*a_rho_f2;
  double a_rho_omega_P=a_rho-0.5*a_omega_P;
  double a_rho_omega_f2=a_rho-0.5*a_omega_f2;
  double a_rho_omega=a_rho-a_omega;
  
  // Donnachie and Kalashnikova (2015):  assume Regge cut parameters for rho 
  // and omega are the same, but have different values for natural and unnatural
  // parity exchanges.
  // Natural parity:
  double C_n_P_cut=regge_cuts[1];
  double C_n_f2_cut=regge_cuts[2];
  double Csq_n_P_cut=C_n_P_cut*C_n_P_cut;
  double Csq_n_f2_cut=C_n_f2_cut*C_n_f2_cut;
  double C_n_f2_C_n_P_cut=C_n_f2_cut*C_n_P_cut;
  // Unnatural parity
  double C_u_P_cut=regge_cuts[3];
  double C_u_f2_cut=regge_cuts[4];
  double Csq_u_P_cut=C_u_P_cut*C_u_P_cut;
  double Csq_u_f2_cut=C_u_f2_cut*C_u_f2_cut;
  double C_u_f2_C_u_P_cut=C_u_f2_cut*C_u_P_cut;

  // terms for interference between resonance shape and propagator in top
  // part of background diagrams
  double Sum_B_and_omega_1=m_omegasq_minus_v1sq*ReB+m_omega*Gamma_omega*ImB;
  double Diff_B_and_omega_1=m_omega*Gamma_omega*ReB-m_omegasq_minus_v1sq*ImB;
  double Sum_B_and_omega_2=m_omegasq_minus_v2sq*ReB+m_omega*Gamma_omega*ImB;
  double Diff_B_and_omega_2=m_omega*Gamma_omega*ReB-m_omegasq_minus_v2sq*ImB;
  double Sum_B_and_rho_1=m_rhosq_minus_v1sq*ReB+m_rho*Gamma_rho*ImB;
  double Diff_B_and_rho_1=m_rho*Gamma_rho*ReB-m_rhosq_minus_v1sq*ImB;
  double Sum_B_and_rho_2=m_rhosq_minus_v2sq*ReB+m_rho*Gamma_rho*ImB;
  double Diff_B_and_rho_2=m_rho*Gamma_rho*ReB-m_rhosq_minus_v2sq*ImB;

  // Squared Regge-ized propagators, including cut terms
  double regge_rho_sq_sum=regge_rho*regge_rho*(1.-cos_rho)
    + 2.*C_n_P_cut*regge_rho*regge_rho_P_cut*cos(M_PI*a_rho_rho_P)
    + 2.*C_n_f2_cut*regge_rho*regge_rho_f2_cut*cos(M_PI*a_rho_rho_f2);
  double regge_omega_sq_sum=regge_omega*regge_omega*(1.-cos_omega)
    + 2.*C_n_P_cut*regge_omega*regge_omega_P_cut*cos(M_PI*a_omega_omega_P)
    + 2.*C_n_f2_cut*regge_omega*regge_omega_f2_cut*cos(M_PI*a_omega_omega_f2);
  // i (Drho Drho_back* - Drho* Drho_back)
  double regge_rho_sq_diff=regge_rho*regge_rho*sin(M_PI*a_rho)
    - 2.*C_n_P_cut*regge_rho*regge_rho_P_cut*sin(M_PI*a_rho_rho_P)
    - 2.*C_n_f2_cut*regge_rho*regge_rho_f2_cut*sin(M_PI*a_rho_rho_f2);
  // i (Domega Domega_back* - Domega* Domega_back)
  double regge_omega_sq_diff=regge_omega*regge_omega*sin(M_PI*a_omega)
    - 2.*C_n_P_cut*regge_omega*regge_omega_P_cut*sin(M_PI*a_omega_omega_P)
    - 2.*C_n_f2_cut*regge_omega*regge_omega_f2_cut*sin(M_PI*a_omega_omega_f2);
  // Drho Domega_back* + Drho* Domega_back
  double regge_rho_omega_back_sum
    =regge_rho*regge_omega*(cos(M_PI*a_rho_omega)-cos(M_PI*a_omega))
    + 2.*C_n_P_cut*regge_omega*regge_rho_P_cut*cos(M_PI*a_omega_rho_P)
    + 2.*C_n_f2_cut*regge_omega*regge_rho_f2_cut*cos(M_PI*a_omega_rho_f2);
   // i (Drho Domega_back* - Drho* Domega_back)
  double regge_rho_omega_back_diff
    =regge_rho*regge_omega*(sin(M_PI*a_rho_omega)+sin(M_PI*a_omega))
    - 2.*C_n_P_cut*regge_omega*regge_rho_P_cut*sin(M_PI*a_omega_rho_P)
    - 2.*C_n_f2_cut*regge_omega*regge_rho_f2_cut*sin(M_PI*a_omega_rho_f2);
  // Domega Drho_back* + Domega* Drho_back
  double regge_omega_rho_back_sum
    =regge_rho*regge_omega*(cos(M_PI*a_rho_omega)-cos(M_PI*a_omega))
    + 2.*C_n_P_cut*regge_rho*regge_omega_P_cut*cos(M_PI*a_rho_omega_P)
    + 2.*C_n_f2_cut*regge_rho*regge_omega_f2_cut*cos(M_PI*a_rho_omega_f2);
   // i (Domega Drho_back* - Domega* Drho_back)
  double regge_omega_rho_back_diff
    =regge_rho*regge_omega*(sin(M_PI*a_rho_omega)+sin(M_PI*a_omega))
    - 2.*C_n_P_cut*regge_rho*regge_omega_P_cut*sin(M_PI*a_rho_omega_P)
    - 2.*C_n_f2_cut*regge_rho*regge_omega_f2_cut*sin(M_PI*a_rho_omega_f2);
				      
  // Interference terms
  double cosphi=cos(phase);
  double sinphi=sin(phase);
  double ReBWfac_omega_1=Sum_B_and_omega_1*cosphi+Diff_B_and_omega_1*sinphi;
  double ImBWfac_omega_1=Sum_B_and_omega_1*sinphi-Diff_B_and_omega_1*cosphi;
  double ReBWfac_omega_2=Sum_B_and_omega_2*cosphi+Diff_B_and_omega_2*sinphi;
  double ImBWfac_omega_2=Sum_B_and_omega_2*sinphi-Diff_B_and_omega_2*cosphi;
  double ReBWfac_rho_1=Sum_B_and_rho_1*cosphi+Diff_B_and_rho_1*sinphi;
  double ImBWfac_rho_1=Sum_B_and_rho_1*sinphi-Diff_B_and_rho_1*cosphi;
  double ReBWfac_rho_2=Sum_B_and_rho_2*cosphi+Diff_B_and_rho_2*sinphi;
  double ImBWfac_rho_2=Sum_B_and_rho_2*sinphi-Diff_B_and_rho_2*cosphi;
  
  double a1_aS=0.,a2_aS=0.,b1_aS=0.,b2_aS=0.,a1_bS=0.,a2_bS=0.;
  double b1_bS=0.,b2_bS=0.;

  // Compute imaginary and real contributions    
  double zeta=1., C=1;
  if (two_particles==(7+7)){ // pi0 pi0
    zeta=1./sqrt(2.);

    a1_aS=Pi_omega_1_sq*(ReBWfac_omega_1*(g_rho_S*gsq_rho_V_and_T
					  *regge_rho_sq_sum
					  + g_omega_S*g_omega_V*g_rho_V_and_T
					  *regge_rho_omega_back_sum)
			 + ImBWfac_omega_1*(g_rho_S*gsq_rho_V_and_T
					    *regge_rho_sq_diff
					    + g_omega_S*g_omega_V*g_rho_V_and_T
					    *regge_rho_omega_back_diff
					    )
			 )
      + (1./3.)*Pi_rho_1_sq*(ReBWfac_rho_1*(g_omega_S*gsq_omega_V
					    *regge_omega_sq_sum
					    + g_rho_S*g_omega_V*g_rho_V_and_T
					    *regge_omega_rho_back_sum)
			     + ImBWfac_rho_1*(g_omega_S*gsq_omega_V
					      *regge_omega_sq_diff
					      + g_rho_S*g_omega_V*g_rho_V_and_T
					      *regge_omega_rho_back_diff
					      )
			     );
    a2_aS=Pi_omega_2_sq*(ReBWfac_omega_2*(g_rho_S*gsq_rho_V_and_T
					  *regge_rho_sq_sum
					  + g_omega_S*g_omega_V*g_rho_V_and_T
					  *regge_rho_omega_back_sum)
			 + ImBWfac_omega_2*(g_rho_S*gsq_rho_V_and_T
					    *regge_rho_sq_diff
					    + g_omega_S*g_omega_V*g_rho_V_and_T
					    *regge_rho_omega_back_diff
					    )
			 )
      + (1./3.)*Pi_rho_2_sq*(ReBWfac_rho_2*(g_omega_S*gsq_omega_V
					    *regge_omega_sq_sum
					    + g_rho_S*g_omega_V*g_rho_V_and_T
					    *regge_omega_rho_back_sum)
			     + ImBWfac_rho_2*(g_omega_S*gsq_omega_V
					      *regge_omega_sq_diff
					      + g_rho_S*g_omega_V*g_rho_V_and_T
					      *regge_omega_rho_back_diff
					      )
			     );
    b1_aS=-2.*g_rho_T*Pi_omega_1_sq*(ReBWfac_omega_1*(g_rho_S*g_rho_V_and_T
						      *regge_rho_sq_sum
						      +g_omega_S*g_omega_V
						      *regge_rho_omega_back_sum
						      )
				     + ImBWfac_omega_1*(g_rho_S*g_rho_V_and_T
							*regge_rho_sq_diff
							+ g_omega_S*g_omega_V
							*regge_rho_omega_back_diff							
							)
				     );
    b2_aS=-2.*g_rho_T*Pi_omega_2_sq*(ReBWfac_omega_2*(g_rho_S*g_rho_V_and_T
						      *regge_rho_sq_sum
						      +g_omega_S*g_omega_V
						      *regge_rho_omega_back_sum
						      )
				     + ImBWfac_omega_2*(g_rho_S*g_rho_V_and_T
							*regge_rho_sq_diff
							+ g_omega_S*g_omega_V
							*regge_rho_omega_back_diff	
							)
				     );
    a1_bS=-2.*g_rho_S*g_rho_T*(Pi_omega_1_sq*g_rho_V_and_T
			       *(ReBWfac_omega_1*regge_rho_sq_sum
				 + ImBWfac_omega_1*regge_rho_omega_back_diff)
			       + (1./3.)*Pi_rho_1_sq*g_omega_V
			       *(ReBWfac_rho_1*regge_omega_rho_back_sum
				 + ImBWfac_rho_1*regge_omega_rho_back_diff)
			       );
    a2_bS=-2.*g_rho_S*g_rho_T*(Pi_omega_2_sq*g_rho_V_and_T
			       *(ReBWfac_omega_2*regge_rho_sq_sum
				 + ImBWfac_omega_2*regge_rho_omega_back_diff)
			       + (1./3.)*Pi_rho_2_sq*g_omega_V
			       *(ReBWfac_rho_2*regge_omega_rho_back_sum
				 + ImBWfac_rho_2*regge_omega_rho_back_diff)
			       );
    b1_bS=4.*g_rho_S*gsq_rho_T*(ReBWfac_omega_1*regge_rho_sq_sum
				+ ImBWfac_omega_1*regge_rho_omega_back_diff);
    b2_bS=4.*g_rho_S*gsq_rho_T*(ReBWfac_omega_2*regge_rho_sq_sum
				+ ImBWfac_omega_2*regge_rho_omega_back_diff);
     
  }
  if (two_particles==(7+17)){ // pi0 eta
    C=sqrt(2./3.);

    a1_aS=(1./3.)*Pi_rho_1_sq*(g_rho_S*gsq_rho_V_and_T
			       *(ReBWfac_rho_1*regge_rho_sq_sum 
				 + ImBWfac_rho_1*regge_rho_sq_diff)
			       + g_omega_S*g_omega_V*g_rho_V_and_T
			       *(ReBWfac_omega_1*regge_omega_rho_back_sum 
				 + ImBWfac_omega_1*regge_omega_rho_back_diff)
			       )
      + Pi_omega_1_sq*(g_rho_S*g_omega_V*g_rho_V_and_T
		       *(ReBWfac_omega_1*regge_rho_omega_back_sum
			 + ImBWfac_omega_1*regge_rho_omega_back_diff)
		       + g_omega_S*gsq_omega_V
		       *(ReBWfac_omega_1*regge_omega_sq_sum 
			 + ImBWfac_omega_1*regge_omega_sq_diff)
		       );
     a2_aS=(1./3.)*Pi_rho_2_sq*(g_rho_S*gsq_rho_V_and_T
			       *(ReBWfac_rho_2*regge_rho_sq_sum 
				 + ImBWfac_rho_2*regge_rho_sq_diff)
			       + g_omega_S*g_omega_V*g_rho_V_and_T
			       *(ReBWfac_omega_2*regge_omega_rho_back_sum 
				 + ImBWfac_omega_2*regge_omega_rho_back_diff)
			       )
      + Pi_omega_2_sq*(g_rho_S*g_omega_V*g_rho_V_and_T
		       *(ReBWfac_omega_2*regge_rho_omega_back_sum
			 + ImBWfac_omega_2*regge_rho_omega_back_diff)
		       + g_omega_S*gsq_omega_V
		       *(ReBWfac_omega_2*regge_omega_sq_sum 
			 + ImBWfac_omega_2*regge_omega_sq_diff)
		       );
     b1_aS=-2.*g_rho_T*Pi_rho_1_sq*(g_rho_S*g_rho_V_and_T
				    *(ReBWfac_rho_1*regge_rho_sq_sum 
				      + ImBWfac_rho_1*regge_rho_sq_diff)
				    + g_omega_S*g_omega_V
				    *(ReBWfac_rho_1*regge_omega_rho_back_sum
				      + ImBWfac_rho_1*regge_omega_rho_back_diff)
				    );
     b2_aS=-2.*g_rho_T*Pi_rho_2_sq*(g_rho_S*g_rho_V_and_T
				    *(ReBWfac_rho_2*regge_rho_sq_sum 
				      + ImBWfac_rho_2*regge_rho_sq_diff)
				    + g_omega_S*g_omega_V
				    *(ReBWfac_rho_2*regge_omega_rho_back_sum
				      + ImBWfac_rho_2*regge_omega_rho_back_diff)
				    );
     a1_bS=-2.*g_rho_S*g_rho_T*((1./3.)*g_rho_V_and_T*Pi_rho_1_sq
				*(ReBWfac_rho_1*regge_rho_sq_sum
				  + ImBWfac_rho_1*regge_rho_sq_diff)
				+ g_omega_V*Pi_omega_1_sq
				*(ReBWfac_omega_1*regge_rho_omega_back_sum
				  + ImBWfac_omega_1*regge_rho_omega_back_diff)
				);
     a2_bS=-2.*g_rho_S*g_rho_T*((1./3.)*g_rho_V_and_T*Pi_rho_2_sq
				*(ReBWfac_rho_2*regge_rho_sq_sum
				  + ImBWfac_rho_2*regge_rho_sq_diff)
				+ g_omega_V*Pi_omega_2_sq
				*(ReBWfac_omega_2*regge_rho_omega_back_sum
				  + ImBWfac_omega_2*regge_rho_omega_back_diff)
				);
     b1_bS=(4./3.)*g_rho_S*gsq_rho_T*Pi_rho_1_sq
       *(ReBWfac_rho_1*regge_rho_sq_sum + ImBWfac_rho_1*regge_rho_sq_diff);
     b2_bS=(4./3.)*g_rho_S*gsq_rho_T*Pi_rho_2_sq
       *(ReBWfac_rho_2*regge_rho_sq_sum + ImBWfac_rho_2*regge_rho_sq_diff);    
  }
 
  double T=zeta*sqrt(g0_sq)*C*gR/(ReB*ReB+ImB*ImB)
    *((m_p_sq-p1_dot_p2)*(a1_aS*M_M1+a2_aS*M_M2)
      + 2.*a1_aS*N_N1+2.*a2_aS*N_N2
      + 2.*m_p*((b1_aS+a1_bS)*N_N1+(b2_aS+a2_bS)*N_N2)
      + (m_p_sq+p1_dot_p2)*(b1_bS*N_N1+b2_bS*N_N2)
      );
  
  return T;
}


// Get parameters for Breit-Wigner distribution for a0(980)/f0(980) resonance shape
void GetResonanceParameters(double m1,double m2, double M_sq,double M_sq_R,
			    double &ReB,double &ImB){
  double m1_plus_m2=m1+m2;
  double m1_minus_m2=m1-m2;  
  bool got_pipi=(fabs(m1_minus_m2)>0.01)?false:true;

  // "No structure" model for a0(980)/f0(980)
  // masses
  double M_S=sqrt(M_sq);
  double MK0=ParticleMass(KShort);
  double MKPlus=ParticleMass(KPlus);

  // coupling constants
  double gK=2.22; 
  double g_m1m2=2.16;
  if (got_pipi){
    gK=0.556;
    g_m1m2=1.6;
  }
  double gKsq=gK*gK;    
  double g_m1m2_sq=g_m1m2*g_m1m2;
    
  // kinematic factors
  double rhoK0sq=1.-4.*MK0*MK0/M_sq;
  double rhoKPlussq=1.-4.*MKPlus*MKPlus/M_sq;
  double rho_m1m2
   =sqrt((1.-m1_plus_m2*m1_plus_m2/M_sq)*(1-m1_minus_m2*m1_minus_m2/M_sq));

      // Real and imaginary parts of BW amplitude
  ReB=M_sq_R-M_sq;
  if (M_S<2.*MKPlus){
    ReB+=gKsq/(32.*M_PI)*sqrt(-rhoKPlussq);
  }
  if (M_S<2.*MK0){
    ReB+=gKsq/(32.*M_PI)*sqrt(-rhoK0sq);
  }
  ImB=g_m1m2_sq/(16*M_PI)*rho_m1m2;
  if (M_S>2.*MKPlus){
    ImB+=gKsq/(32.*M_PI)*sqrt(rhoKPlussq);
  }
  if (M_S>2.*MK0){
    ImB+=gKsq/(32.*M_PI)*sqrt(rhoK0sq);
  }
}


// Cross section dsigma/(dt/dM/dOmega) from Donnachie and Kalashnikova
double CrossSection(double m1,double m2, double ms_sq, double s, double t,
		    double gR,double ReB,double ImB,double gsq_rho_S,
		    double gsq_omega_S, bool interfere,
		    double gR2,double ReB2,double ImB2,double gsq_rho_S2,
		    double gsq_omega_S2,double phase){
  // Kinematic factors
  double mp_sq_minus_s=m_p_sq-s;
  double mp_sq_plus_s=m_p_sq+s;
  double mp_sq_minus_s_sq=mp_sq_minus_s*mp_sq_minus_s;
  double temp1=ms_sq*mp_sq_plus_s-mp_sq_minus_s_sq;
  double temp2=mp_sq_minus_s*sqrt(mp_sq_minus_s_sq-2.*ms_sq*mp_sq_plus_s
				  +ms_sq*ms_sq);
  double t1=(temp1+temp2)/(2.*s);
  double t2=(temp1-temp2)/(2.*s);
  double t_minus_ms_sq=t-ms_sq;
  double kin1=s*(t-t1)*(t-t2);
  double kin_aS_aS=0.5*kin1+0.25*t*t_minus_ms_sq*t_minus_ms_sq;
  double kin_aS_bS=0.5*m_p*kin1;
  double kin_bS_bS=0.125*(4.*m_p_sq-t)*kin1;
  double kin_b1=-0.5*t*kin1;

  // Coupling constants 
  double g_rho_S=sqrt(gsq_rho_S);
  double g_omega_S=sqrt(gsq_omega_S);

  // s scale for regge trajectories
  double s0=1.;

  // Regge trajectory for omega
  double a_omega=0.44+0.9*t;
  double a_omega_prime=0.9;
  double cos_omega=cos(M_PI*a_omega);
  double regge_omega=pow(s/s0,a_omega-1.)*M_PI*a_omega_prime/(sin(M_PI*a_omega)*TMath::Gamma(a_omega)); // excluding phase factor

  // Regge cuts for omega
  double a_omega_P=0.52+0.196*t; // Pomeron
  double a_omega_f2=0.112+0.428*t;
  double dc=regge_cuts[0];
  double regge_omega_P_cut=exp(dc*t)*pow(s/s0,a_omega_P-1.);
  double regge_omega_f2_cut=exp(dc*t)*pow(s/s0,a_omega_f2-1.);

  // Donnachie and Kalashnikova (2015):  assume Regge cut parameters for rho 
  // and omega are the same, but have different values for natural and unnatural
  // parity exchanges.
  // Natural parity:
  double C_n_P_cut=regge_cuts[1];
  double C_n_f2_cut=regge_cuts[2];
  double Csq_n_P_cut=C_n_P_cut*C_n_P_cut;
  double Csq_n_f2_cut=C_n_f2_cut*C_n_f2_cut;
  double C_n_f2_C_n_P_cut=C_n_f2_cut*C_n_P_cut;
  // Unnatural parity
  double C_u_P_cut=regge_cuts[3];
  double C_u_f2_cut=regge_cuts[4];
  double Csq_u_P_cut=C_u_P_cut*C_u_P_cut;
  double Csq_u_f2_cut=C_u_f2_cut*C_u_f2_cut;
  double C_u_f2_C_u_P_cut=C_u_f2_cut*C_u_P_cut;

  // Regge trajectory for rho
  double a_rho=0.55+0.8*t;
  double a_rho_prime=0.8;
  double cos_rho=cos(M_PI*a_rho);
  double regge_rho=pow(s/s0,a_rho-1.)*M_PI*a_rho_prime/(sin(M_PI*a_rho)*TMath::Gamma(a_rho)); // excluding phase factor

  // Regge cuts for rho
  double a_rho_P=0.64+0.16*t; // Pomeron
  double a_rho_f2=0.222+0.404*t;
  double regge_rho_P_cut=exp(dc*t)*pow(s/s0,a_rho_P-1.);
  double regge_rho_f2_cut=exp(dc*t)*pow(s/s0,a_rho_f2-1.);
 
  // rho-omega interference
  double cos_rho_omega_sum=0.5*(cos(M_PI*(a_rho-a_omega))-cos(M_PI*a_rho)
				-cos(M_PI*a_omega)+1.); 
  // Regge propagator products
  double regge_omega_omega_P_cut=regge_omega*regge_omega_P_cut
    *(cos(M_PI*(a_omega-0.5*a_omega_P))-cos(M_PI_2*a_omega_P)) ;
  double regge_omega_omega_f2_cut=regge_omega*regge_omega_f2_cut
    *(cos(M_PI*(a_omega-0.5*a_omega_f2))-cos(M_PI_2*a_omega_f2));
  double regge_rho_omega_P_cut=regge_rho*regge_omega_P_cut
    *(cos(M_PI*(a_rho-0.5*a_omega_P))-cos(M_PI_2*a_omega_P)); 
  double regge_rho_omega_f2_cut=regge_rho*regge_omega_f2_cut
    *(cos(M_PI*(a_rho-0.5*a_omega_f2))-cos(M_PI_2*a_omega_f2));
  double regge_omega_f2_omega_P_cuts
    =regge_omega_f2_cut*regge_omega_P_cut*cos(M_PI_2*(a_omega_f2-a_omega_P));
  double regge_omega_P_cut_sq=regge_omega_P_cut*regge_omega_P_cut;
  double regge_omega_f2_cut_sq=regge_omega_f2_cut*regge_omega_f2_cut;
  double regge_rho_P_cut_sq=regge_rho_P_cut*regge_rho_P_cut;
  double regge_rho_f2_cut_sq=regge_rho_f2_cut*regge_rho_f2_cut;
  double regge_rho_rho_P_cut=regge_rho*regge_rho_P_cut
    *(cos(M_PI*(a_rho-0.5*a_rho_P))-cos(M_PI_2*a_rho_P));
  double regge_rho_rho_f2_cut=regge_rho*regge_rho_f2_cut
    *(cos(M_PI*(a_rho-0.5*a_rho_f2))-cos(M_PI_2*a_rho_f2));
  double regge_omega_rho_P_cut=regge_omega*regge_rho_P_cut
    *(cos(M_PI*(a_omega-0.5*a_rho_P))-cos(M_PI_2*a_rho_P));
  double regge_omega_rho_f2_cut=regge_omega*regge_rho_f2_cut
    *(cos(M_PI*(a_omega-0.5*a_rho_f2))-cos(M_PI_2*a_rho_f2));
  double regge_rho_P_omega_P_cuts=regge_rho_P_cut*regge_omega_P_cut
    *cos(M_PI_2*(a_rho_P-a_omega_P));
  double regge_rho_P_omega_f2_cuts=regge_rho_P_cut*regge_omega_f2_cut
    *cos(M_PI_2*(a_rho_P-a_omega_f2));
  double regge_rho_f2_omega_P_cuts=regge_rho_f2_cut*regge_omega_P_cut
    *cos(M_PI_2*(a_rho_f2-a_omega_P));
  double regge_rho_f2_omega_f2_cuts=regge_rho_f2_cut*regge_omega_f2_cut
    *cos(M_PI_2*(a_rho_f2-a_omega_f2));
  double regge_rho_f2_rho_P_cuts=regge_rho_f2_cut*regge_rho_P_cut
    *cos(M_PI_2*(a_rho_P-a_rho_f2));

  // Squared Regge-ized propagators, including cut terms
  double regge_rho_sq_sum=regge_rho*regge_rho*0.5*(1.-cos_rho)
    + C_n_P_cut*regge_rho_rho_P_cut
    + C_n_f2_cut*regge_rho_rho_f2_cut
    + Csq_n_P_cut*regge_rho_P_cut_sq
    + Csq_n_f2_cut*regge_rho_f2_cut_sq
    + 2.*C_n_f2_C_n_P_cut*regge_rho_f2_rho_P_cuts;
  double regge_omega_sq_sum=regge_omega*regge_omega*0.5*(1.-cos_omega)
    + C_n_P_cut*regge_omega_omega_P_cut
    + C_n_f2_cut*regge_omega_omega_f2_cut
    + Csq_n_P_cut*regge_omega_P_cut_sq
    + Csq_n_f2_cut*regge_omega_f2_cut_sq
    + 2.*C_n_f2_C_n_P_cut*regge_omega_f2_omega_P_cuts;
  double regge_omega_rho_sum=regge_omega*regge_rho*cos_rho_omega_sum
    + C_n_P_cut*(regge_rho_omega_P_cut+regge_omega_rho_P_cut)
    + C_n_f2_cut*(regge_rho_omega_f2_cut+regge_omega_rho_f2_cut)
    + C_n_f2_C_n_P_cut*(regge_rho_f2_omega_P_cuts+regge_rho_P_omega_f2_cuts)
    + Csq_n_P_cut*regge_rho_P_omega_P_cuts
    + Csq_n_f2_cut*regge_rho_f2_omega_f2_cuts;
  double regge_rho_sq_u_sum=Csq_u_P_cut*regge_rho_P_cut_sq
    + Csq_u_f2_cut*regge_rho_f2_cut_sq
    + 2.*C_u_f2_C_u_P_cut*regge_rho_f2_rho_P_cuts;
  double regge_omega_sq_u_sum=Csq_u_P_cut*regge_omega_P_cut_sq
    + Csq_u_f2_cut*regge_omega_f2_cut_sq
    + 2.*C_u_f2_C_u_P_cut*regge_omega_f2_omega_P_cuts;
  double regge_omega_rho_u_sum
    = C_u_f2_C_u_P_cut*(regge_rho_f2_omega_P_cuts+regge_rho_P_omega_f2_cuts)
    + Csq_u_P_cut*regge_rho_P_omega_P_cuts
    + Csq_u_f2_cut*regge_rho_f2_omega_f2_cuts;
  
  // Compute the square of the amplitude, including interference
  double T=0.,aS_aS=0.,aS_bS=0.,bS_bS=0.,b1S_b1S=0.;
  double Pi_R_sq=0.; // Breit-Wigner scale factor
  if (interfere==false){
    Pi_R_sq=gR*gR/(ReB*ReB+ImB*ImB);
   
    aS_aS=gsq_rho_S*gsq_rho_V_and_T*regge_rho_sq_sum
      + gsq_omega_S*gsq_omega_V*regge_omega_sq_sum
      + g_rho_S*g_omega_S*g_omega_V*g_rho_V_and_T*regge_omega_rho_sum;
   
    bS_bS=4.*gsq_rho_S*g_rho_T*g_rho_T*regge_rho_sq_sum;

    aS_bS=-2.*g_rho_S*g_rho_T*(2.*g_rho_S*g_rho_V_and_T*regge_rho_sq_sum
			       +g_omega_S*g_omega_V*regge_omega_rho_sum);
    
    // Cut contribution from b1 exchange.  We ignore the pole term.
    b1S_b1S=gsq_rho_S*gsq_rho_V_and_T*regge_rho_sq_u_sum
      + gsq_omega_S*gsq_omega_V*regge_omega_sq_u_sum
      + g_rho_S*g_omega_S*g_omega_V*g_rho_V_and_T*regge_omega_rho_u_sum;
  }
  else{
    Pi_R_sq=gR/(ReB*ReB+ImB*ImB) * gR2/(ReB2*ReB2+ImB2*ImB2);
 
    double g1rho=sqrt(gsq_rho_S);
    double g1omega=sqrt(gsq_omega_S);
    double g2rho=sqrt(gsq_rho_S2);
    double g2omega=sqrt(gsq_omega_S2);

    double sin_rho_omega_sum=-0.5*(sin(M_PI*a_rho)-sin(M_PI*a_omega)
				   +sin(M_PI*(a_omega-a_rho)));
    double sin_omega_omega_P_sum=sin(M_PI*(a_omega-0.5*a_omega_P))
      +sin(M_PI_2*a_omega_P);
    double sin_omega_omega_f2_sum=sin(M_PI*(a_omega-0.5*a_omega_f2))
      +sin(M_PI_2*a_omega_f2);
    double sin_rho_omega_P_sum=sin(M_PI*(a_rho-0.5*a_omega_P))
      +sin(M_PI_2*a_omega_P);
    double sin_rho_omega_f2_sum=sin(M_PI*(a_rho-0.5*a_omega_f2))
      +sin(M_PI_2*a_omega_f2);
    double sin_rho_rho_P_sum=sin(M_PI*(a_rho-0.5*a_rho_P))
      +sin(M_PI_2*a_rho_P);
    double sin_rho_rho_f2_sum=sin(M_PI*(a_rho-0.5*a_rho_f2))
      +sin(M_PI_2*a_rho_f2);
    double sin_omega_rho_P_sum=sin(M_PI*(a_omega-0.5*a_rho_P))
      +sin(M_PI_2*a_rho_P);
    double sin_omega_rho_f2_sum=sin(M_PI*(a_omega-0.5*a_rho_f2))
      +sin(M_PI_2*a_rho_f2);
    double regge_omega_rho_diff=regge_omega*regge_rho*sin_rho_omega_sum
      + C_n_P_cut*(regge_rho*regge_rho_P_cut*sin_rho_rho_P_sum
		   + regge_omega*regge_rho_P_cut*sin_omega_rho_P_sum)
      + C_n_f2_cut*(regge_rho*regge_rho_f2_cut*sin_rho_rho_f2_sum
		   + regge_omega*regge_rho_f2_cut*sin_omega_rho_f2_sum)
      + C_n_f2_C_n_P_cut*(2.*regge_rho_f2_cut*regge_omega_P_cut
			  *sin(M_PI_2*(a_rho_f2-a_omega_P))
			  + 2.*regge_rho_P_cut*regge_omega_f2_cut
			  *sin(M_PI_2*(a_rho_P-a_omega_f2)))
      + Csq_n_P_cut*regge_rho_P_cut*regge_rho_P_cut
      *2.*sin(M_PI_2*(a_rho_P-a_omega_P))
      + Csq_n_f2_cut*regge_rho_f2_cut*regge_rho_f2_cut
      *2.*sin(M_PI_2*(a_rho_f2-a_omega_f2));
     double regge_omega_rho_u_diff
      = C_n_f2_C_n_P_cut*(2.*regge_rho_f2_cut*regge_omega_P_cut
			  *sin(M_PI_2*(a_rho_f2-a_omega_P))
			  + 2.*regge_rho_P_cut*regge_omega_f2_cut
			  *sin(M_PI_2*(a_rho_P-a_omega_f2)))
       + Csq_n_P_cut*regge_rho_P_cut*regge_rho_P_cut
       *2.*sin(M_PI_2*(a_rho_P-a_omega_P))
       + Csq_n_f2_cut*regge_rho_f2_cut*regge_rho_f2_cut
       *2.*sin(M_PI_2*(a_rho_f2-a_omega_f2));
     
    double cosphi=cos(phase);
    double sinphi=sin(phase);
    double ReBWfac=(ReB*ReB2+ImB*ImB2)*cosphi-(ImB*ReB2-ImB2*ReB)*sinphi;
    double ImBWfac=(ReB*ReB2+ImB*ImB2)*sinphi+(ImB*ReB2-ImB2*ReB)*cosphi;
    
    aS_aS=ReBWfac*(2.*g1rho*g2rho*gsq_rho_V_and_T*regge_rho_sq_sum
		   + 2.*g1omega*g2omega*gsq_omega_V*regge_omega_sq_sum
		   + (g1rho*g2omega+g2rho*g1omega)
		   *g_omega_V*g_rho_V_and_T*regge_omega_rho_sum)
      - ImBWfac*g_omega_V*g_rho_V_and_T*(g1rho*g2omega-g1omega*g2rho)
      *regge_omega_rho_diff;
    
    aS_bS=-2.*g_rho_T*(ReBWfac*(4.*g1rho*g2rho*g_rho_V_and_T*regge_rho_sq_sum
				+(g1rho*g2omega+g2rho*g1omega)
				*g_omega_V*regge_omega_rho_sum)
		       + ImBWfac*(g1rho*g2omega+g2rho*g1omega)
		       *g_omega_V*regge_omega_rho_diff);

    bS_bS=8*g1rho*g2rho*g_rho_T*g_rho_T*ReBWfac*regge_rho_sq_sum;


    b1S_b1S=ReBWfac*(2.*g1rho*g2rho*gsq_rho_V_and_T*regge_rho_sq_u_sum
		     + 2.*g1omega*g2omega*gsq_omega_V*regge_omega_sq_u_sum
		     + (g1rho*g2omega+g2rho*g1omega)
		     *g_omega_V*g_rho_V_and_T*regge_omega_rho_u_sum)
      - ImBWfac*g_omega_V*g_rho_V_and_T*(g1rho*g2omega-g1omega*g2rho)
      *regge_omega_rho_u_diff;

  }
  //  printf("kin %f %f %f \n",kin_aS_aS,kin_aS_bS,kin_bS_bS);
  /*
  aS_aS=0;
  bS_bS=0;
  aS_bS*=-1.;
  */
  //aS_bS=0.;

  
  T=Pi_R_sq*(aS_aS*kin_aS_aS+aS_bS*kin_aS_bS+bS_bS*kin_bS_bS+b1S_b1S*kin_b1);

  return T;
}

double TensorCrossSection(TLorentzVector &q /* beam */,
			  vector<Particle_t>&particle_types,
			  vector<TLorentzVector>&particles,
			  double gR,double ReB, double ImB){
  int two_particles=particle_types[0]+particle_types[1];
  
  // Four vectors
  TLorentzVector p1(0,0,0.,ParticleMass(Proton));
  TLorentzVector p2=particles[2];
  TLorentzVector dp=p2-p1;
  
  // Mandelstam variables
  double s=(q+p1).M2();
  double t=(p1-p2).M2();

  // dot products 
  double p1_dot_dp=p1.Dot(dp);
  double p2_dot_dp=p2.Dot(dp);
  double p1_dot_p2=p1.Dot(p2);

  // momentum transfer compenents
  double dpx=dp.X();
  double dpy=dp.Y();
  double dpx2_plus_dpy2=dpx*dpx+dpy*dpy;

  // other constants
  double m_rho=0.775; // GeV
  double m_rho_sq=m_rho*m_rho;
 
  // Coupling constants 
  double f=1.;  // scale factor to account for normalization of regge factor 
  // to data?? 
  double gT_sq=f*(2./3.)*150.; // GeV^2
  if (two_particles==(7+17)){
    f=1.; // guess
    gT_sq=f*183.675;  // GeV^2
  }

  // s scale for regge trajectories
  double s0=1.;
 
  // Regge trajectory for rho
  double a_rho=0.55+0.8*t;
  double a_rho_prime=0.8;
  double regge_rho=pow(s/s0,a_rho-1.)*M_PI*a_rho_prime/(sin(M_PI*a_rho)*TMath::Gamma(a_rho)); // excluding phase factor
  double regge_rho_sq=regge_rho*regge_rho;

  // Regge trajectory for odderon
  double a_odderon=1.+0.25*t;
  double a_odderon_prime=0.25;
  double regge_odderon=pow(s/s0,a_odderon-1.)*M_PI*a_odderon_prime
    /(sin(M_PI*a_odderon)*TMath::Gamma(a_odderon)); // excluding phase factor
  double regge_odderon_sq=regge_odderon*regge_odderon; 
 
  // coupling constant for tensor interaction at rhoNN vertex
  double Kappa_rho=6.1;

  // Amplitude
  double one_minus_dpx_over_m_rho=1.-dpx/m_rho;
  double one_minus_dpy_over_m_rho=1.-dpy/m_rho;
  double common_fac=(38./9.)*(M_PI/2.)*(1./137.);
  double vector_coupling
    =2.*dpx2_plus_dpy2/m_rho_sq*p1_dot_dp*(p2_dot_dp/m_rho_sq-1.)
    +(m_p_sq-p1_dot_p2)*(one_minus_dpx_over_m_rho*one_minus_dpx_over_m_rho
			 +one_minus_dpy_over_m_rho*one_minus_dpy_over_m_rho);
  double tensor_coupling
    =(2.*t-dpx2_plus_dpy2)*(-Kappa_rho
			    +0.25*Kappa_rho*Kappa_rho*(1.+p1_dot_p2/m_p_sq));
  double amp_sum=gT_sq*(vector_coupling+tensor_coupling)*regge_rho_sq;
  double gT_odderon=0.;//5.*f; // need better guess
  if (two_particles==(7+17)){
    //gT_odderon=5.; // need better guess
  }
  amp_sum+=gT_odderon*gT_odderon*vector_coupling*regge_odderon_sq;
  amp_sum-=2.*cos(M_PI*(a_odderon-a_rho))*sqrt(gT_sq)*gT_odderon
    *regge_rho*regge_odderon*(vector_coupling-(2.*t-dpx2_plus_dpy2)*Kappa_rho);
  double decay_weight=1.; // take care of angular distribution of decay
  double T=common_fac*amp_sum*decay_weight*gR*gR/(ReB*ReB+ImB*ImB);
  
  return T;
}

double TensorBackgroundInterference(TLorentzVector &q /* beam */,
				    vector<Particle_t>&particle_types,
				    vector<TLorentzVector>&particles,
				    double gR,double ReB, double ImB,
				    double phase){
  int two_particles=particle_types[0]+particle_types[1];
  
  // Four vectors
  TLorentzVector p1(0,0,0.,ParticleMass(Proton));
  TLorentzVector p2=particles[2];
  TLorentzVector dp=p2-p1;
  TLorentzVector v1=particles[0]-q;
  TLorentzVector v2=particles[1]-q;  
  double q_dot_dp=q.Dot(dp);
  double q_dot_v1=q.Dot(v1);
  double dp_dot_v1=dp.Dot(v1);
  double q_dot_v2=q.Dot(v2);
  double dp_dot_v2=dp.Dot(v2);
  double v1sq=v1.M2();
  double v2sq=v2.M2();
  double b1=-q_dot_dp*v1sq+q_dot_v1*dp_dot_v1;
  double b2=-q_dot_dp*v2sq+q_dot_v2*dp_dot_v2;
  TLorentzVector c1=-dp_dot_v1*q+q_dot_dp*v1;
  TLorentzVector c2=-dp_dot_v2*q-q_dot_dp*v2;
  TLorentzVector d1=q_dot_v1*v1-v1sq*q;
  TLorentzVector d2=q_dot_v2*v2-v2sq*q;
  double d1x_plus_d1y=d1.Vect().x()+d1.Vect().y(); 
  double d2x_plus_d2y=d2.Vect().x()+d2.Vect().y(); 
  double c1x_plus_c1y=c1.Vect().x()+c1.Vect().y(); 
  double c2x_plus_c2y=c2.Vect().x()+c2.Vect().y();
  
  // Mandelstam variables
  double s=(q+p1).M2();
  double t=(p1-p2).M2();

  // dot products 
  double p1_dot_dp=p1.Dot(dp);
  double p2_dot_dp=p2.Dot(dp);
  double p1_dot_p2=p1.Dot(p2);
  double c1_dot_p1=c1.Dot(p1);
  double c2_dot_p1=c2.Dot(p1);
  double c1_dot_p2=c1.Dot(p2);
  double c2_dot_p2=c2.Dot(p2); 
  double d1_dot_p1=d1.Dot(p1);
  double d2_dot_p1=d2.Dot(p1);
  double d1_dot_dp=d1.Dot(dp);
  double d2_dot_dp=d2.Dot(dp);
  double c1_dot_dp=c1.Dot(dp);
  double c2_dot_dp=c2.Dot(dp);

  // momentum transfer compenents
  double dpx=dp.X();
  double dpy=dp.Y();
  double dpx_plus_dpy=dpx+dpy;
  // other momentum components
  double v1x_plus_v1y=v1.Vect().x()+v1.Vect().y();
  double v2x_plus_v2y=v2.Vect().x()+v2.Vect().y();
  
  // Coupling constants 
  double f=1.;  // scale factor to account for normalization of regge factor 
  // to data?? 
  double gT_sq=f*(2./3.)*150.; // GeV^2
  if (two_particles==(7+17)){
    f=1.; // guess
    gT_sq=f*183.675;  // GeV^2
  } 

  // Rho propagator for top exchange for double-exchange diagrams
  double m_rho=0.7685;
  double m_rho_sq=m_rho*m_rho;
  double Gamma_rho=0.1462;
  double m_rhosq_minus_v1sq=m_rho*m_rho-v1sq;
  double Pi_rho_1_sq=1./(m_rhosq_minus_v1sq*m_rhosq_minus_v1sq
			 +m_rho*m_rho*Gamma_rho*Gamma_rho);
  double m_rhosq_minus_v2sq=m_rho*m_rho-v2sq;
  double Pi_rho_2_sq=1./(m_rhosq_minus_v2sq*m_rhosq_minus_v2sq
			 +m_rho*m_rho*Gamma_rho*Gamma_rho);
  
  // s scale for regge trajectories
  double s0=1.;
 
  // Regge trajectory for rho
  double a_rho=0.55+0.8*t;
  double a_rho_prime=0.8;
  double regge_rho=pow(s/s0,a_rho-1.)*M_PI*a_rho_prime/(sin(M_PI*a_rho)*TMath::Gamma(a_rho)); // excluding phase factor
  double regge_rho_sq=regge_rho*regge_rho;

  // Regge trajectory for odderon
  /*
  double a_odderon=1.+0.25*t;
  double a_odderon_prime=0.25;
  double regge_odderon=pow(s/s0,a_odderon-1.)*M_PI*a_odderon_prime
    /(sin(M_PI*a_odderon)*TMath::Gamma(a_odderon)); // excluding phase factor
  double regge_odderon_sq=regge_odderon*regge_odderon; 
  */

    // omega propagator for top exchange
  double m_omega=0.78265;
  double Gamma_omega=0.00849;
  double m_omegasq_minus_v1sq=m_omega*m_omega-v1sq;
  double Pi_omega_1_sq=1./(m_omegasq_minus_v1sq*m_omegasq_minus_v1sq
			   +m_omega*m_omega*Gamma_omega*Gamma_omega);
  double m_omegasq_minus_v2sq=m_omega*m_omega-v2sq;
  double Pi_omega_2_sq=1./(m_omegasq_minus_v2sq*m_omegasq_minus_v2sq
			   +m_omega*m_omega*Gamma_omega*Gamma_omega);

  // Regge trajectory for omega
  double a_omega=0.44+0.9*t;
  double a_omega_prime=0.9; 
  //  double cos_omega=cos(M_PI*a_omega);
  double sin_omega=sin(M_PI*a_omega);
  double regge_omega=pow(s/s0,a_omega-1.)*M_PI*a_omega_prime/(sin_omega*TMath::Gamma(a_omega)); // excluding phase factor
  // interference between rho and omega;
  double cos_rho_omega=cos(M_PI*(a_rho-a_omega));
  double sin_rho_omega=sin(M_PI*(a_rho-a_omega));

  // coupling constant for tensor interaction at rhoNN vertex
  double Kappa_rho=6.1;
  
  /*
  double gT_odderon=0.;//5.*f; // need better guess
  if (two_particles==(7+17)){
    //gT_odderon=5.; // need better guess
  }
  */

  // terms for interference between resonance shape and propagator in top
  // part of background diagrams
  double Re_B_and_omega_1=m_omegasq_minus_v1sq*ReB+m_omega*Gamma_omega*ImB;
  double Im_B_and_omega_1=m_omega*Gamma_omega*ReB-m_omegasq_minus_v1sq*ImB;
  double Re_B_and_omega_2=m_omegasq_minus_v2sq*ReB+m_omega*Gamma_omega*ImB;
  double Im_B_and_omega_2=m_omega*Gamma_omega*ReB-m_omegasq_minus_v2sq*ImB;
  double Re_B_and_rho_1=m_rhosq_minus_v1sq*ReB+m_rho*Gamma_rho*ImB;
  double Im_B_and_rho_1=m_rho*Gamma_rho*ReB-m_rhosq_minus_v1sq*ImB;
  double Re_B_and_rho_2=m_rhosq_minus_v2sq*ReB+m_rho*Gamma_rho*ImB;
  double Im_B_and_rho_2=m_rho*Gamma_rho*ReB-m_rhosq_minus_v2sq*ImB;
  

  double ReT=0.,ImT=0.;
  // Kinematic factors
  double temp1_1=(v1x_plus_v1y*c1_dot_p1-d1_dot_p1*dpx_plus_dpy)*dpx_plus_dpy;
  double temp2_1=(v2x_plus_v2y*c2_dot_p1-d2_dot_p1*dpx_plus_dpy)*dpx_plus_dpy;
  double temp1_2=(v1x_plus_v1y*c1_dot_p2+(b1-d1_dot_p1)*dpx_plus_dpy)*dpx_plus_dpy;
  double temp2_2=(v2x_plus_v2y*c2_dot_p2+(b2-d2_dot_p1)*dpx_plus_dpy)*dpx_plus_dpy;
  double temp3_1=((b1-d1_dot_dp)*dpx_plus_dpy+v1x_plus_v1y*c1.Dot(dp))*dpx_plus_dpy; 
  double temp3_2=((b2-d2_dot_dp)*dpx_plus_dpy+v2x_plus_v2y*c2.Dot(dp))*dpx_plus_dpy;
  
  double kin1_1=temp1_1*(p2_dot_dp/m_rho_sq-1.)+temp2_1*p1_dot_dp/m_rho_sq
    +(m_p_sq-p1_dot_p2)*(temp3_1/m_rho_sq-2.*b1-v1x_plus_v1y*c1x_plus_c1y
			 +dpx_plus_dpy*d1x_plus_d1y);
  double kin2_1=m_p*temp1_1*((p1_dot_dp+p2_dot_dp)/m_rho_sq-1.);
  double kin3_1=temp1_1*p1_dot_dp;
  double kin4_1=temp3_1*(t/m_rho_sq-1.)
    +t*((b1-d1_dot_dp)*dpx_plus_dpy*dpx_plus_dpy/m_rho_sq-2.*b1
	+dpx_plus_dpy*d1x_plus_d1y
	+v1x_plus_v1y*(dpx_plus_dpy*c1_dot_dp/m_rho_sq-c1x_plus_c1y)); 
  double kin1_2=temp1_2*(p2_dot_dp/m_rho_sq-1.)+temp2_2*p1_dot_dp/m_rho_sq
    +(m_p_sq-p1_dot_p2)*(temp3_2/m_rho_sq-2.*b2-v2x_plus_v2y*c2x_plus_c2y
			 +dpx_plus_dpy*d2x_plus_d2y);
  double kin2_2=m_p*temp1_2*((p1_dot_dp+p2_dot_dp)/m_rho_sq-1.);
  double kin3_2=temp1_2*p1_dot_dp/m_p;
  double kin4_2=temp3_2*(t/m_rho_sq-1.)
    +t*((b2-d2_dot_dp)*dpx_plus_dpy*dpx_plus_dpy/m_rho_sq-2.*b2
	+dpx_plus_dpy*d2x_plus_d2y
	+v2x_plus_v2y*(dpx_plus_dpy*c2_dot_dp/m_rho_sq-c2x_plus_c2y));

  // Compute imaginary and real contributions    
  double zeta=1., C=1;
  if (two_particles==(7+7)){ // pi0 pi0
    zeta=1./sqrt(2.);	     

    ReT=(kin1_1-0.25*Kappa_rho*kin4_1)
      *(2.*g_rho_V_and_T*regge_rho_sq*Pi_omega_1_sq*Re_B_and_omega_1
	+(2./3.)*g_omega_V*regge_rho*regge_omega*Pi_rho_1_sq
	*(Re_B_and_rho_1*cos_rho_omega-Im_B_and_rho_1*sin_rho_omega)
	)
      +(kin1_2-0.25*Kappa_rho*kin4_2)
      *(2.*g_rho_V_and_T*regge_rho_sq*Pi_omega_2_sq*Re_B_and_omega_2
	+(2./3.)*g_omega_V*regge_rho*regge_omega*Pi_rho_2_sq
	*(Re_B_and_rho_2*cos_rho_omega-Im_B_and_rho_2*sin_rho_omega)
	)
      -4.*(kin2_1-Kappa_rho*kin3_1)*regge_rho_sq*g_rho_T*Pi_omega_1_sq
      *Re_B_and_omega_1 
      -4.*(kin2_2-Kappa_rho*kin3_2)*regge_rho_sq*g_rho_T*Pi_omega_2_sq
      *Re_B_and_omega_2;
    ImT=(kin1_1-0.25*Kappa_rho*kin4_1)
      *(2.*g_rho_V_and_T*regge_rho_sq*Pi_omega_1_sq*Im_B_and_omega_1
	+(2./3.)*g_omega_V*regge_rho*regge_omega*Pi_rho_1_sq
	*(Re_B_and_rho_1*sin_rho_omega+Im_B_and_rho_1*cos_rho_omega)
	)
      +(kin1_2-0.25*Kappa_rho*kin4_2)
      *(2.*g_rho_V_and_T*regge_rho_sq*Pi_omega_2_sq*Im_B_and_omega_2
	+(2./3.)*g_omega_V*regge_rho*regge_omega*Pi_rho_2_sq
	*(Re_B_and_rho_2*sin_rho_omega+Im_B_and_rho_2*cos_rho_omega)
	)
      -4.*(kin2_1-Kappa_rho*kin3_1)*regge_rho_sq*g_rho_T*Pi_omega_1_sq
      *Im_B_and_omega_1 
      -4.*(kin2_2-Kappa_rho*kin3_2)*regge_rho_sq*g_rho_T*Pi_omega_2_sq
      *Im_B_and_omega_2;
  }
  if (two_particles==(7+17)){ // pi0 eta
    C=sqrt(2./3.);

    ReT=(kin1_1-0.25*Kappa_rho*kin4_1)
      *((2./3.)*g_rho_V_and_T*regge_rho_sq*Pi_rho_1_sq*Re_B_and_rho_1
	+2.*g_omega_V*regge_rho*regge_omega*Pi_omega_1_sq
	*(Re_B_and_omega_1*cos_rho_omega-Im_B_and_omega_1*sin_rho_omega)
	)
      +(kin1_2-0.25*Kappa_rho*kin4_2)
      *((2./3.)*g_rho_V_and_T*regge_rho_sq*Pi_omega_2_sq*Re_B_and_omega_2
	+2.*g_omega_V*regge_rho*regge_omega*Pi_rho_2_sq
	*(Re_B_and_rho_2*cos_rho_omega-Im_B_and_rho_2*sin_rho_omega)
	)
      -4./3.*(kin2_1-Kappa_rho*kin3_1)*regge_rho_sq*g_rho_T*Pi_rho_1_sq
      *Re_B_and_rho_1 
      -4./3.*(kin2_2-Kappa_rho*kin3_2)*regge_rho_sq*g_rho_T*Pi_omega_2_sq
      *Re_B_and_omega_2;
    ImT=(kin1_1-0.25*Kappa_rho*kin4_1)
      *(2./3.*g_rho_V_and_T*regge_rho_sq*Pi_rho_1_sq*Im_B_and_rho_1
	+(2.)*g_omega_V*regge_rho*regge_omega*Pi_omega_1_sq
	*(Re_B_and_omega_1*sin_rho_omega+Im_B_and_omega_1*cos_rho_omega)
	)
      +(kin1_2-0.25*Kappa_rho*kin4_2)
      *(2./3.*g_rho_V_and_T*regge_rho_sq*Pi_omega_2_sq*Im_B_and_omega_2
	+(2.)*g_omega_V*regge_rho*regge_omega*Pi_rho_2_sq
	*(Re_B_and_rho_2*sin_rho_omega+Im_B_and_rho_2*cos_rho_omega)
	)
      -4./3.*(kin2_1-Kappa_rho*kin3_1)*regge_rho_sq*g_rho_T*Pi_rho_1_sq
      *Im_B_and_rho_1 
      -4./3.*(kin2_2-Kappa_rho*kin3_2)*regge_rho_sq*g_rho_T*Pi_omega_2_sq
      *Im_B_and_omega_2;
  }
  double common_fac=zeta*C*sqrt(38./9.*M_PI/2./137.*gT_sq*g0_sq)*gR/(ReB*ReB+ImB*ImB);
  

  double T=common_fac*(ReT*cos(phase)+ImT*sin(phase));

  return T;
}

double TensorScalarInterference(TLorentzVector &q /* beam */,
				vector<Particle_t>&particle_types,
				vector<TLorentzVector>&particles,
				double gR,double ReB, double ImB,
				double gR_S,double ReB_S,double ImB_S,
				double g_omega_S,double g_rho_S,
				double phase){
  int two_particles=particle_types[0]+particle_types[1];
  double m_rho=0.7685;
  double m_rho_sq=m_rho*m_rho;

  // Four vectors
  TLorentzVector p1(0,0,0.,ParticleMass(Proton));
  TLorentzVector p2=particles[2];
  TLorentzVector dp=p2-p1;
  
  // Mandelstam variables
  double s=(q+p1).M2();
  double t=(p1-p2).M2();
  double q_dot_dp=q.Dot(dp);
  double q_dot_p1=q.Dot(p1);

  // dot products 
  double p1_dot_dp=p1.Dot(dp);
  double p2_dot_dp=p2.Dot(dp);
  double p1_dot_p2=p1.Dot(p2);
  

  // momentum transfer compenents
  double dpx=dp.X();
  double dpy=dp.Y();
  double dpx_plus_dpy=dpx+dpy;

  // Coupling constants 
  double f=1.;  // scale factor to account for normalization of regge factor 
  // to data?? 
  double gT_sq=f*(2./3.)*150.; // GeV^2
  if (two_particles==(7+17)){
    f=1.; // guess
    gT_sq=f*183.675;  // GeV^2
  } 
 
  // s scale for regge trajectories
  double s0=1.;
 
  // Regge trajectory for rho
  double a_rho=0.55+0.8*t;
  double a_rho_prime=0.8;
  double cos_rho=cos(M_PI*a_rho);
  double sin_rho=sin(M_PI*a_rho);
  double regge_rho=pow(s/s0,a_rho-1.)*M_PI*a_rho_prime/(sin(M_PI*a_rho)*TMath::Gamma(a_rho)); // excluding phase factor
  double regge_rho_sq=regge_rho*regge_rho;

  /*
  // Regge trajectory for odderon
  double a_odderon=1.+0.25*t;
  double a_odderon_prime=0.25;
  double regge_odderon=pow(s/s0,a_odderon-1.)*M_PI*a_odderon_prime
    /(sin(M_PI*a_odderon)*TMath::Gamma(a_odderon)); // excluding phase factor
  double regge_odderon_sq=regge_odderon*regge_odderon; 
  */

  // Regge trajectory for omega
  double a_omega=0.44+0.9*t;
  double a_omega_prime=0.9; 
  //double cos_omega=cos(M_PI*a_omega);
  double sin_omega=sin(M_PI*a_omega);
  double regge_omega=pow(s/s0,a_omega-1.)*M_PI*a_omega_prime/(sin_omega*TMath::Gamma(a_omega)); // excluding phase factor
  //double regge_omega_sq=regge_omega*regge_omega;
  // interference between rho and omega;
  double cos_rho_omega=cos(M_PI*(a_rho-a_omega));
  double sin_rho_omega=sin(M_PI*(a_rho-a_omega));

   // Regge cuts for rho
  double dc=regge_cuts[0];
  double a_rho_P=0.64+0.16*t; // Pomeron
  double a_rho_f2=0.222+0.404*t;
  double regge_rho_P_cut=exp(dc*t)*pow(s/s0,a_rho_P-1.);
  double regge_rho_f2_cut=exp(dc*t)*pow(s/s0,a_rho_f2-1.);
  double C_rho_P_cut=regge_cuts[3];
  double C_rho_f2_cut=regge_cuts[4];

  // Regge cuts for omega
  double a_omega_P=0.52+0.196*t; // Pomeron
  double a_omega_f2=0.112+0.428*t; 
  double regge_omega_P_cut=exp(dc*t)*pow(s/s0,a_omega_P-1.);
  double regge_omega_f2_cut=exp(dc*t)*pow(s/s0,a_omega_f2-1.);
  double C_omega_P_cut=regge_cuts[1];
  double C_omega_f2_cut=regge_cuts[2];
  
  // cut interference factors
  double cos_rho_rho_P=cos(M_PI*(a_rho-0.5*a_rho_P)); 
  double cos_rho_rho_f2=cos(M_PI*(a_rho-0.5*a_rho_f2));
  double sin_rho_rho_P=sin(M_PI*(a_rho-0.5*a_rho_P));
  double sin_rho_rho_f2=sin(M_PI*(a_rho-0.5*a_rho_f2));
  double cos_rho_omega_P=cos(M_PI*(a_rho-0.5*a_omega_P)); 
  double cos_rho_omega_f2=cos(M_PI*(a_rho-0.5*a_omega_f2));
  double sin_rho_omega_P=sin(M_PI*(a_rho-0.5*a_omega_P));
  double sin_rho_omega_f2=sin(M_PI*(a_rho-0.5*a_omega_f2));
 
  // coupling constant for tensor interaction at rhoNN vertex
  double Kappa_rho=6.1;

  /*
  double gT_odderon=0.;//5.*f; // need better guess
  if (two_particles==(7+17)){
    //gT_odderon=5.; // need better guess
  }
  */

  // terms for interference between resonances
  double Re_B_S_and_T=ReB*ReB_S+ImB*ImB_S;
  double Im_B_S_and_T=ReB*ImB_S-ReB_S*ImB;

  double ReT=0.,ImT=0.;
  // Kinematic factors
  double kin_aS=(4./3.)*(m_p_sq-p1_dot_p2+0.5*Kappa_rho*t)*q_dot_dp
    +(5./3.)*dpx_plus_dpy*dpx_plus_dpy
    *q_dot_p1*((p1_dot_dp+p2_dot_dp)/m_rho_sq-1.);
  double kin_bS=(5./3.)*dpx_plus_dpy*dpx_plus_dpy*q_dot_p1
    *(m_p*((p1_dot_dp+p2_dot_dp)/m_rho_sq-1.)
      -Kappa_rho/(2.*m_p)*p1_dot_dp*(2.*p2_dot_dp/m_rho_sq-1.));
  double common_fac=gR*gR_S*sqrt(38./9.*gT_sq*M_PI/2./137.)
    /(ReB*ReB+ImB*ImB)/(ReB_S*ReB_S+ImB_S*ImB_S);
 
  ReT=kin_aS*(g_rho_V_and_T*g_rho_S*regge_rho_sq
	      *(Re_B_S_and_T*(1.-cos_rho)+Im_B_S_and_T*sin_rho)
	      +2.*g_rho_V*g_rho_S*regge_rho
	      *(C_rho_P_cut*regge_rho_P_cut*(Re_B_S_and_T*cos_rho_rho_P
					     -Im_B_S_and_T*sin_rho_rho_P)
		+C_rho_f2_cut*regge_rho_f2_cut*(Re_B_S_and_T*cos_rho_rho_f2
					     -Im_B_S_and_T*sin_rho_rho_f2)
		)
	      +g_omega_V*g_omega_S*regge_rho
	      *(regge_omega*(Re_B_S_and_T*(cos_rho_omega-cos_rho)
			     -Im_B_S_and_T*(sin_rho_omega+sin_rho))
		+2.*C_omega_P_cut*regge_omega_P_cut
		*(Re_B_S_and_T*cos_rho_omega_P-Im_B_S_and_T*sin_rho_omega_P)
		+2.*C_omega_f2_cut*regge_omega_f2_cut
		*(Re_B_S_and_T*cos_rho_omega_f2-Im_B_S_and_T*sin_rho_omega_f2)
		)
	      )
    +kin_bS*(-2.*g_rho_S*g_rho_T*regge_rho_sq)
    *(Re_B_S_and_T*(1.-cos_rho)+Im_B_S_and_T*sin_rho);
  
  ImT=kin_aS*(g_rho_V_and_T*g_rho_S*regge_rho_sq
	      *(Re_B_S_and_T*sin_rho-Im_B_S_and_T*(1.-cos_rho))
	      +2.*g_rho_V*g_rho_S*regge_rho
	      *(C_rho_P_cut*regge_rho_P_cut*(Re_B_S_and_T*sin_rho_rho_P
					     +Im_B_S_and_T*cos_rho_rho_P)
		+C_rho_f2_cut*regge_rho_f2_cut*(Re_B_S_and_T*sin_rho_rho_f2
					     +Im_B_S_and_T*cos_rho_rho_f2)
		)
	      +g_omega_V*g_omega_S*regge_rho
	      *(regge_omega*(Re_B_S_and_T*(sin_rho_omega+sin_rho)
			     +Im_B_S_and_T*(cos_rho_omega-cos_rho))
		+2.*C_omega_P_cut*regge_omega_P_cut
		*(Re_B_S_and_T*sin_rho_omega_P+Im_B_S_and_T*cos_rho_omega_P)
		+2.*C_omega_f2_cut*regge_omega_f2_cut
		*(Re_B_S_and_T*sin_rho_omega_f2+Im_B_S_and_T*cos_rho_omega_f2)
		)
	      )
    +kin_bS*(-2.*g_rho_S*g_rho_T*regge_rho_sq)
    *(Re_B_S_and_T*sin_rho-Im_B_S_and_T*(1.-cos_rho));

  // Sqaure of amplitudes
  double T=common_fac*(ReT*cos(phase)+ImT*sin(phase));

  return T;
}


// Put particle data into hddm format and output to file
void WriteEvent(unsigned int eventNumber,TLorentzVector &beam, float vert[3],
		vector<Particle_t>&particle_types,
		vector<TLorentzVector>&particle_vectors, s_iostream_t *file){  
   s_PhysicsEvents_t* pes;
   s_Reactions_t* rs;
   s_Target_t* ta;
   s_Beam_t* be;
   s_Vertices_t* vs;
   s_Origin_t* origin;
   s_Products_t* ps;
   s_HDDM_t *thisOutputEvent = make_s_HDDM();
   thisOutputEvent->physicsEvents = pes = make_s_PhysicsEvents(1);
   pes->mult = 1;
   pes->in[0].runNo = runNo;
   pes->in[0].eventNo = eventNumber;
   pes->in[0].reactions = rs = make_s_Reactions(1);
   rs->mult = 1;
   // Beam 
   rs->in[0].beam = be = make_s_Beam();
   be->type = Gamma;
   be->properties = make_s_Properties();
   be->properties->charge = ParticleCharge(be->type);
   be->properties->mass = ParticleMass(be->type);
   be->momentum = make_s_Momentum();
   be->momentum->px = 0.;
   be->momentum->py = 0.;
   be->momentum->pz = beam.Pz();
   be->momentum->E  = beam.E();
   // Target
   rs->in[0].target = ta = make_s_Target();
   ta->type = Proton;
   ta->properties = make_s_Properties();
   ta->properties->charge = ParticleCharge(ta->type);
   ta->properties->mass = ParticleMass(ta->type);
   ta->momentum = make_s_Momentum();
   ta->momentum->px = 0.;
   ta->momentum->py = 0.;
   ta->momentum->pz = 0.;
   ta->momentum->E  = ParticleMass(ta->type);
   // Primary vertex 
   rs->in[0].vertices = vs = make_s_Vertices(1);
   vs->mult = 1;
   vs->in[0].origin = origin = make_s_Origin();
   vs->in[0].products = ps = make_s_Products(particle_vectors.size());
   ps->mult = 0;
   origin->t = 0.0;
   origin->vx = vert[0];
   origin->vy = vert[1];
   origin->vz = vert[2];
   // Final state particles
   for (unsigned int i=0;i<particle_vectors.size();i++,ps->mult++){
     Particle_t my_particle=particle_types[i];
     ps->in[ps->mult].type = my_particle;
     ps->in[ps->mult].pdgtype = PDGtype(my_particle);
     ps->in[ps->mult].id = i+1; /* unique value for this particle within the event */
     ps->in[ps->mult].parentid = 0;  /* All internally generated particles have no parent */
     ps->in[ps->mult].mech = 0; // ???     
     ps->in[ps->mult].momentum = make_s_Momentum();
     ps->in[ps->mult].momentum->px = particle_vectors[i].Px();
     ps->in[ps->mult].momentum->py = particle_vectors[i].Py();
     ps->in[ps->mult].momentum->pz = particle_vectors[i].Pz();
     ps->in[ps->mult].momentum->E  = particle_vectors[i].E();
   }
   flush_s_HDDM(thisOutputEvent,file);
}

// Create some diagnostic histograms
void CreateHistograms(string beamConfigFile){

  thrown_t=new TH1D("thrown_t","Thrown t distribution",1000,0.,5);
  thrown_t->SetXTitle("-t [GeV^{2}]");
  thrown_dalitzZ=new TH1D("thrown_dalitzZ","thrown dalitz Z",110,-0.05,1.05);
  thrown_Egamma=new TH1D("thrown_Egamma","Thrown E_{#gamma} distribution",
			       1000,0,12.);
  thrown_Egamma->SetTitle("E_{#gamma} [GeV]"); 
  thrown_mass=new TH1D("thrown_mass","Thrown mass distribution",
		       1000,0,4.);
  thrown_mass->SetXTitle("mass [GeV]");
  thrown_dalitzXY=new TH2D("thrown_dalitzXY","Dalitz distribution Y vs X",100,-1.,1.,100,-1.,1);
  thrown_mass_vs_E=new TH2D("thrown_mass_vs_E","M(4#gamma) vs Ebeam",
			    48,2.8,12.4,450,0,4.5);
  thrown_mass_vs_E->SetYTitle("M(4#gamma) [GeV]");
  thrown_mass_vs_E->SetXTitle("E(beam) [GeV]");
  
  thrown_theta_vs_p=new TH2D("thrown_theta_vs_p","Proton #theta_{LAB} vs. p",
			       200,0,2.,180,0.,90.);
  thrown_theta_vs_p->SetXTitle("p [GeV/c]");
  thrown_theta_vs_p->SetYTitle("#theta [degrees]");
  
  BeamProperties beamProp(beamConfigFile);
  cobrems_vs_E = (TH1D*)beamProp.GetFlux();
}

double GetCrossSection(double s,double t,double M_sq,TLorentzVector &beam,
		       vector<Particle_t>&particle_types,
		       vector<TLorentzVector>&particle_vectors,
		       double phase[],int *generate
		       ){
  // Decay particles
  double m1=ParticleMass(particle_types[0]);
  double m2=ParticleMass(particle_types[1]);
  double m1_plus_m2=m1+m2;
  double m1_minus_m2=m1-m2;
  bool got_pipi=(fabs(m1-m2)>0.01)?false:true;
  double m1sq=m1*m1;
  double m2sq=m2*m2;
  double m1sq_plus_m2sq=m1sq+m2sq;

  // Coupling constants for f0(500)
  double gsq_rho_f500_gamma=0.25;
  double gsq_omega_f500_gamma=(1./9)*gsq_rho_f500_gamma;
  // Coupling constants: Donnachie and Kalashnikova (2008) scenario IV
  double gsq_rho_S_gamma=0.02537;
  double gsq_omega_S_gamma=0.2283;
  double gsq_rho_f1370_gamma=0.094;
  double gsq_omega_f1370_gamma=(1./9.)*gsq_rho_f1370_gamma;
  double gsq_rho_a1450_gamma=0.0054;
  double gsq_omega_a1450_gamma=9.*gsq_rho_a1450_gamma;
  
  //Resonance parameters 
  double ReB=0.,ImB=0,gR=0.;
  double gR_T=0., ImB_T=0., ReB_T=0.;
  double gRf500=0.,ImBf500=0.,ReBf500=0.; 
  double gRf1370=0.,ImBf1370=0.,ReBf1370=0.;  
  double gRa1450=0.,ImBa1450=0.,ReBa1450=0.;
  
  // Intialize cross section (will fill in kinematic factors later)
  double T=0.;
  
  // f0(600)
  if (got_pipi && generate[0]){
    double m_Sigma=0.55;// PDG (2020), upper end of range
    double M_sq_R=m_Sigma*m_Sigma; 
    width=0.7; // PDG (2020)
    ReBf500=M_sq_R-M_sq;
    double MRsq_minus_m1sq_m2sq=M_sq_R-m1sq_plus_m2sq;
    double temp=4.*m1sq*m2sq;
    double qR=sqrt((MRsq_minus_m1sq_m2sq*MRsq_minus_m1sq_m2sq-temp)
		   /(4.*M_sq_R));
    double partial_width=width/3.;
    gRf500=sqrt(8.*M_PI*M_sq_R*partial_width/qR);
    ImBf500=width*sqrt(M_sq);
    
    T+=CrossSection(m1,m2,M_sq,s,t,gRf500,ReBf500,ImBf500,
		    gsq_rho_f500_gamma,gsq_omega_f500_gamma,false,
		    0.,0.,0.,0.,0.,0.);
    
  }
  // f0(1370)
  if (got_pipi && generate[2]){
    double m_f1370=1.3; // Bugg
    double M_sq_R=m_f1370*m_f1370; 
    width=0.23;  // from Bugg
    ReBf1370=M_sq_R-M_sq;
    double MRsq_minus_m1sq_m2sq=M_sq_R-m1sq_plus_m2sq;
    double temp=4.*m1sq*m2sq;
    double qR=sqrt((MRsq_minus_m1sq_m2sq*MRsq_minus_m1sq_m2sq-temp)
		   /(4.*M_sq_R));
    // partial width from Bugg(2007):arxiv.org/pdf/0706.1341.pdf, table 2
    double partial_width=0.325/3.;
    gRf1370=sqrt(8.*M_PI*M_sq_R*partial_width/qR);
    ImBf1370=width*sqrt(M_sq);
    
    T+=CrossSection(m1,m2,M_sq,s,t,gRf1370,ReBf1370,ImBf1370,
		    gsq_rho_f1370_gamma,gsq_omega_f1370_gamma,false,
		    0.,0.,0.,0.,0.,0.);
      
    // Interference between f0(500) and f0(1370)
    if (generate[0]){
      T+=CrossSection(m1,m2,M_sq,s,t,gRf1370,ReBf1370,ImBf1370,
		      gsq_rho_f1370_gamma,
		      gsq_omega_f1370_gamma,true,gRf500,ReBf500,ImBf500,
		      gsq_rho_f500_gamma,gsq_omega_f500_gamma,
		      phase[0]-phase[2]);
    }	
  }

  // a0(1450)
  if (got_pipi==false && generate[2]){
    double m_a1450=1.448;// Bugg:arXiv:0808.2706v2,Table 2
    double M_sq_R=m_a1450*m_a1450; 
    width=0.192; // Bugg:arXiv:0808.2706v2,Table 2
    ReBa1450=M_sq_R-M_sq;
    double MRsq_minus_m1sq_m2sq=M_sq_R-m1sq_plus_m2sq;
    double temp=4.*m1sq*m2sq;
    double qR=sqrt((MRsq_minus_m1sq_m2sq*MRsq_minus_m1sq_m2sq-temp)
		   /(4.*M_sq_R));	
    double partial_width=0.0237;// Bugg:arXiv:0808.2706v2,Table 2
    gRa1450=sqrt(8.*M_PI*M_sq_R*partial_width/qR);
    ImBa1450=width*sqrt(M_sq);
    
    T+=CrossSection(m1,m2,M_sq,s,t,gRa1450,ReBa1450,ImBa1450,
		    gsq_rho_a1450_gamma,gsq_omega_a1450_gamma,false,
		    0.,0.,0.,0.,0.,0.);
    
  }
  // f0(980)/a0(980)
  if (generate[1]){
    double my_msq_R=0.9783*0.9783;
    if (got_pipi){ // f0(980)	
      double MRsq_minus_m1sq_m2sq=my_msq_R-m1sq_plus_m2sq;	
      double temp=4.*m1sq*m2sq;
      double qR=sqrt((MRsq_minus_m1sq_m2sq*MRsq_minus_m1sq_m2sq-temp)
		     /(4.*my_msq_R));
      double partial_width=0.05; //?? // guess from note in pdg
      gR=sqrt(8.*M_PI*my_msq_R*partial_width/qR);
      // gsq_rho_S_gamma=0.15; // GeV^-2
      //gsq_omega_S_gamma=(1./9.)*gsq_rho_S_gamma;
      //gsq_omega_S_gamma=0.0054; // 3 keV
      //gsq_rho_S_gamma=9.*gsq_omega_S_gamma;
      // Molecular KK model (need reference)
      gsq_rho_S_gamma=0.044;
      gsq_omega_S_gamma=(1./9.)*gsq_rho_S_gamma;
    }
    else{ // a0(980)  
      my_msq_R=0.9825*0.9825;	
      double MRsq_minus_m1sq_m2sq=my_msq_R-m1sq_plus_m2sq;	
      double temp=4.*m1sq*m2sq;
      double qR=sqrt((MRsq_minus_m1sq_m2sq*MRsq_minus_m1sq_m2sq-temp)
			 /(4.*my_msq_R));
      double partial_width=0.06; //?? guess from note in pdg
      gR=sqrt(8.*M_PI*my_msq_R*partial_width/qR);
      gsq_rho_S_gamma=0.02537;
      gsq_omega_S_gamma=9.*gsq_rho_S_gamma;
    } 
    GetResonanceParameters(m1,m2,M_sq,my_msq_R,ReB,ImB);
    T+=CrossSection(m1,m2,M_sq,s,t,gR,ReB,ImB,gsq_rho_S_gamma,
		    gsq_omega_S_gamma,false,
		    0.,0.,0.,0.,0.,0.);
    // Interference between f0(1370) and f0(980)
    if (got_pipi && generate[2]){
      T+=CrossSection(m1,m2,M_sq,s,t,gR,ReB,ImB,gsq_rho_S_gamma,
		      gsq_omega_S_gamma,true,gRf1370,ReBf1370,ImBf1370,
		      gsq_rho_f1370_gamma,gsq_omega_f1370_gamma,
		      phase[1]-phase[2]);
    }	
    // Interference between f0(500) and f0(980)
    if (got_pipi && generate[0]){      
      T+=CrossSection(m1,m2,M_sq,s,t,gR,ReB,ImB,gsq_rho_S_gamma,
		      gsq_omega_S_gamma,true,gRf500,ReBf500,ImBf500,
		      gsq_rho_f500_gamma,gsq_omega_f500_gamma,
		      phase[0]-phase[1]);
      
    }	
    // Interference between a0(1450) and a0(980)
    if (got_pipi==false && generate[2]){
      T+=CrossSection(m1,m2,M_sq,s,t,gR,ReB,ImB,gsq_rho_S_gamma,
		      gsq_omega_S_gamma,true,gRa1450,ReBa1450,ImBa1450,
		      gsq_rho_a1450_gamma,gsq_omega_a1450_gamma,
		      phase[1]-phase[2]);
    }
  }
  if (generate[4]){ // non-resonant background
    T+=BackgroundCrossSection(beam,particle_types,particle_vectors);

    if (generate[1]){ // interference with resonant signal
      T+=InterferenceCrossSection(beam,particle_types,particle_vectors,
				  gR,ReB,ImB,gsq_rho_S_gamma,
				  gsq_omega_S_gamma,phase[1]);
    }
    if (got_pipi){
      if (generate[0]){ // interference with f0(500)
	T+=InterferenceCrossSection(beam,particle_types,particle_vectors,
				    gRf500,ReBf500,ImBf500,gsq_rho_f500_gamma,
				    gsq_omega_f500_gamma,phase[0]);
      }  
      if (generate[2]){ // interference with f0(1370)
	T+=InterferenceCrossSection(beam,particle_types,particle_vectors,
				    gRf1370,ReBf1370,ImBf1370,
				    gsq_rho_f1370_gamma,
				    gsq_omega_f1370_gamma,phase[2]);
      }
    }
    else{ 
      if (generate[2]){ // interference with a0(1450)
	T+=InterferenceCrossSection(beam,particle_types,particle_vectors,
				    gRa1450,ReBa1450,ImBa1450,
				    gsq_rho_a1450_gamma,
				    gsq_omega_a1450_gamma,phase[2]);
      }
      
    }
  }
  if (generate[3]){ // Tensor background
    double m_T=1.275;	
    double Gamma_T=0.185;
    if (!got_pipi){
      Gamma_T=0.107;
      m_T=1.3183;
    }
    double M_sq_R_T=m_T*m_T; 
    ReB_T=M_sq_R_T-M_sq;
    double Msq_minus_m1sq_m2sq=M_sq-m1sq_plus_m2sq;
    double MRsq_minus_m1sq_m2sq=M_sq_R_T-m1sq_plus_m2sq;
    double temp=4.*m1sq*m2sq;
    double M=sqrt(M_sq);
    double q_over_qR
      =m_T/M*sqrt((Msq_minus_m1sq_m2sq*Msq_minus_m1sq_m2sq-temp)
		  /(MRsq_minus_m1sq_m2sq*MRsq_minus_m1sq_m2sq-temp));
    double q_over_qR_5=pow(q_over_qR,5);
    if (got_pipi){ // f2(1270)  
      double partial_width=0.85*(1./3.)*M_sq_R_T*q_over_qR_5/M_sq;
      gR_T=sqrt(8.*M_PI*M_sq_R_T*partial_width*q_over_qR);
      ImB_T=M*Gamma_T*(0.85*q_over_qR_5*M_sq_R_T/M_sq+0.15);
    }
    else { // a2(1320)
      double partial_width=0.155*M_sq_R_T*q_over_qR_5/M_sq;
      gR_T=sqrt(8.*M_PI*M_sq_R_T*partial_width*q_over_qR);
      ImB_T=M*Gamma_T*(0.145*q_over_qR_5*M_sq_R_T/M_sq+0.855);
    }
    T+=TensorCrossSection(beam,particle_types,particle_vectors,gR_T,ReB_T,
			  ImB_T);
    if (generate[1]){ // interference with a0(980)/f0(980)
      T+=TensorScalarInterference(beam,particle_types,particle_vectors,
				  gR_T,ReB_T,ImB_T,gR,ReB,ImB,
				  sqrt(gsq_omega_S_gamma),
				  sqrt(gsq_rho_S_gamma),phase[1]-phase[3]);
    }
    // interference with f0(600)
    if (got_pipi && generate[0]){
      T+=TensorScalarInterference(beam,particle_types,particle_vectors,
				  gR_T,ReB_T,ImB_T,gRf500,ReBf500,ImBf500,
				  sqrt(gsq_omega_f500_gamma),
				  sqrt(gsq_rho_f500_gamma),phase[0]-phase[3]);
    }
    // interference with f0(1370)
    if (got_pipi && generate[2]){
      T+=TensorScalarInterference(beam,particle_types,particle_vectors,
				  gR_T,ReB_T,ImB_T,gRf1370,ReBf1370,ImBf1370,
				  sqrt(gsq_omega_f1370_gamma),
				  sqrt(gsq_rho_f1370_gamma),phase[2]-phase[3]);
    }	
    // interference with a0(1450)
    if (got_pipi==false && generate[2]){
      T+=TensorScalarInterference(beam,particle_types,particle_vectors,
				  gR_T,ReB_T,ImB_T,gRa1450,ReBa1450,ImBa1450,
				  sqrt(gsq_omega_a1450_gamma),
				  sqrt(gsq_rho_a1450_gamma),phase[2]-phase[3]);
    }
    
    if (generate[4]){ //interference with background wave
      T+=TensorBackgroundInterference(beam,particle_types,particle_vectors,
				      gR_T,ReB_T,ImB_T,phase[3]);
    } 
    
  }

  // Kinematic factors
  double mp_sq_minus_s=m_p_sq-s;
  double k=sqrt((M_sq-m1_plus_m2*m1_plus_m2)*(M_sq-m1_minus_m2*m1_minus_m2))
    /(2.*sqrt(M_sq));
  double hbarc_sq=389.; // Convert to micro-barns
  double xsec=-hbarc_sq*k*T/(256.*M_PI*M_PI*M_PI*M_PI*mp_sq_minus_s
			     *mp_sq_minus_s);
  return xsec;
}
  
void GetDecayVectors(double m1, double m2, double p_rest, double theta_rest, 
		     double phi_rest,
		     const TVector3 &v_S,
		     vector<TLorentzVector>&particle_vectors){
    double pt_rest=p_rest*sin(theta_rest);
    particle_vectors[0].SetXYZT(pt_rest*cos(phi_rest),pt_rest*sin(phi_rest),
				p_rest*cos(theta_rest),
				sqrt(p_rest*p_rest+m1*m1));
    particle_vectors[1].SetVect(-particle_vectors[0].Vect());
    particle_vectors[1].SetT(sqrt(p_rest*p_rest+m2*m2));
    particle_vectors[0].Boost(v_S);
    particle_vectors[1].Boost(v_S);
}

// Create a graph of the cross section dsigma/dt as a function of -t
void GraphCrossSection(vector<Particle_t>&particle_types,double phase[],
		       int *generate){
  double xsec_max=0.;
  
  TLorentzVector beam(0,0,EgammaPlot,EgammaPlot);
  TLorentzVector target(0,0,0,m_p);
  vector<TLorentzVector>particle_vectors(3);

   // Velocity of the cm frame with respect to the lab frame
  TVector3 v_cm=(1./(EgammaPlot+m_p))*beam.Vect();

  // CM energy
  double s=m_p*(m_p+2.*EgammaPlot);
  double Ecm=sqrt(s);
  
  // Parameters for integration over line shape
  double m1=ParticleMass(particle_types[0]);
  double m2=ParticleMass(particle_types[1]);
  double m1_plus_m2=m1+m2;
  double m1_minus_m2=m1-m2;
  double m1_plus_m2_sq=m1_plus_m2*m1_plus_m2;
  double m1_minus_m2_sq=m1_minus_m2*m1_minus_m2;
  double m_max=m_p*(sqrt(1.+2.*EgammaPlot/m_p)-1.);
  double dmrange=m_max-m1_plus_m2;
  double dm=dmrange/1000.;

  // Momenta of incoming photon and outgoing S and proton in cm frame
  double p_gamma=(s-m_p_sq)/(2.*Ecm);
  double M_sq=0.980*0.980; // f0(980)/a0(980)
  double E_S=(s+M_sq-m_p_sq)/(2.*Ecm);
  double p_S=sqrt(E_S*E_S-M_sq);
  
  // Momentum transfer t
  double p_diff=p_gamma-p_S;
  double t0=M_sq*M_sq/(4.*s)-p_diff*p_diff;
  
  // differential cross section
  double sum=0.;
  double t_old=t0;
  double t_array[1000];
  double xsec_array[1000];
  for (unsigned int k=0;k<1000;k++){
    double theta_cm=M_PI*double(k)/1000.;
    double sin_theta_over_2=sin(0.5*theta_cm);
    double t=t0-4.*p_gamma*p_S*sin_theta_over_2*sin_theta_over_2;

    // Four-momentum of the S in the CM frame
    double phi_cm=0.;
    double pt=p_S*sin(theta_cm);
    TLorentzVector S4(pt*cos(phi_cm),pt*sin(phi_cm),p_S*cos(theta_cm),
		      sqrt(p_S*p_S+M_sq));
    //S4.Print();
    
    //Boost the S 4-momentum into the lab
    S4.Boost(v_cm);
    TVector3 v_S=(1./S4.E())*S4.Vect();
    
    // Compute the 4-momentum for the recoil proton
    particle_vectors[2]=beam+target-S4;
 
    // Decay particles
    double p_rest=sqrt((M_sq-m1_plus_m2_sq)*(M_sq-m1_minus_m2_sq))/(2.*M_sq);
    double theta_rest=0.;
    double phi_rest=0.; 
    GetDecayVectors(m1,m2,p_rest,theta_rest,phi_rest,v_S,particle_vectors);

    double xsec=GetCrossSection(s,t,M_sq,beam,particle_types,particle_vectors,
				phase,generate);
    t_array[k]=-t;
    xsec_array[k]=4*M_PI*1000*xsec;
  }  
  
  double m_array[1000];
  double xsec_array2[1000];  
  for (unsigned int j=0;j<1000;j++){
    double mass=m1_plus_m2+dm*double(j);
    m_array[j]=mass;
    M_sq=mass*mass;
    E_S=(s+M_sq-m_p_sq)/(2.*Ecm);
    p_S=sqrt(E_S*E_S-M_sq);

    // Momentum transfer t
    double p_diff=p_gamma-p_S;
    double t0=M_sq*M_sq/(4.*s)-p_diff*p_diff;
    t_old=t0;
    
    double xsec=0.;
    for (unsigned int k=0;k<1000;k++){
      double theta_cm=M_PI*double(k)/1000.;
      double sin_theta_over_2=sin(0.5*theta_cm);
      double t=t0-4.*p_gamma*p_S*sin_theta_over_2*sin_theta_over_2;

      // Four-momentum of the S in the CM frame
      double phi_cm=0.;
      double pt=p_S*sin(theta_cm);
      TLorentzVector S4(pt*cos(phi_cm),pt*sin(phi_cm),p_S*cos(theta_cm),
			sqrt(p_S*p_S+M_sq));
      // S4.Print();
      
      //Boost the S 4-momentum into the lab
      S4.Boost(v_cm);
      TVector3 v_S=(1./S4.E())*S4.Vect();

      // Compute the 4-momentum for the recoil proton
      particle_vectors[2]=beam+target-S4;

      // Decay particles
      double p_rest=sqrt((M_sq-m1_plus_m2_sq)*(M_sq-m1_minus_m2_sq))/(2.*M_sq);
      double theta_rest=0.;
      double phi_rest=0.;
      GetDecayVectors(m1,m2,p_rest,theta_rest,phi_rest,v_S,particle_vectors);
     
      double dsigma_dtdMdOmega=GetCrossSection(s,t,M_sq,beam,particle_types,
					       particle_vectors,phase,generate);
      if (dsigma_dtdMdOmega>xsec_max) xsec_max=dsigma_dtdMdOmega;
      xsec+=(t_old-t)*dsigma_dtdMdOmega;
      t_old=t;
    }
    sum+=4*M_PI*1000*xsec*dm;
    xsec_array2[j]=4*M_PI*1000*xsec;
  }
  cout << "xec mx " << xsec_max << endl;

  TGraph *Gxsec=new TGraph(1000,t_array,xsec_array);
  Gxsec->GetXaxis()->SetTitle("-t [GeV^{2}]");
  Gxsec->GetYaxis()->SetTitle("d#sigma/dt [nb/GeV^{2}]");
  Gxsec->Write("Cross section d#sigma/dt");
  TGraph *Gxsec2=new TGraph(1000,m_array,xsec_array2);
  Gxsec2->GetXaxis()->SetTitle("M [GeV]");
  Gxsec2->GetYaxis()->SetTitle("d#sigma/dM [nb/GeV]");
  Gxsec2->Write("Cross section d#sigma/dM"); 
 
  cout << "Total cross section at " << EgammaPlot << " GeV = "<< sum 
       << " nano-barns"<<endl;
}


//-----------
// main
//-----------
int main(int narg, char *argv[])
{  
  ParseCommandLineArguments(narg, argv);

  // open ROOT file
  string rootfilename="scalar_gen.root";
  TFile *rootfile=new TFile(rootfilename.c_str(),"RECREATE",
			    "Produced by genScalarRegge");

  // Initialize random number generator
  TRandom3 *myrand=new TRandom3(0);// If seed is 0, the seed is automatically computed via a TUUID object, according to TRandom3 documentation

  // Fixed target
  TLorentzVector target(0.,0.,0.,m_p);
 
  //----------------------------------------------------------------------------
  // Get production (Egamma range) and decay parameters from input file
  //----------------------------------------------------------------------------

  // Start reading the input file 
  ifstream infile(input_file_name);
  if (!infile.is_open()){
    cerr << "Input file missing! Exiting..." <<endl;
    exit(-1);
  } 

  // Get beam properties configuration file
  string comment_line;
  getline(infile,comment_line);
  string beamConfigFile;
  infile >> beamConfigFile;
  infile.ignore(); // ignore the '\n' at the end of this line

  cout << "Photon beam configuration file " << beamConfigFile.data() << endl;
  
  // Set number of decay particles
  int num_decay_particles=2;
  // Set up vectors of particle ids and 4-vectors
  int last_index=num_decay_particles;
  int num_final_state_particles=num_decay_particles+1;
  vector<TLorentzVector>particle_vectors(num_final_state_particles);
  vector<Particle_t>particle_types(num_final_state_particles);
  double *decay_masses =new double[num_decay_particles];
  particle_types[last_index]=Proton;

  // GEANT ids of decay particles
  getline(infile,comment_line);
  cout << "Particle id's of decay particles =";
  for (int k=0;k<num_decay_particles;k++){
    int ipart;
    infile >> ipart;
    cout << " " << ipart; 
    particle_types[k]=(Particle_t)ipart;
    decay_masses[k]=ParticleMass(particle_types[k]);
  }
  infile.ignore(); // ignore the '\n' at the end of this line
  cout << endl; 

  // Parameters for regge cuts
  getline(infile,comment_line);
  cout << "Regge cut parameters =";
  for (int k=0;k<5;k++){
    infile >> regge_cuts[k];
    cout << " " << regge_cuts[k]; 
  }
  infile.ignore(); // ignore the '\n' at the end of this line
  cout << endl;

  // processes to simulate
  int num_processes=5;
  getline(infile,comment_line);
  int *generate=new int[num_processes];
  for (int k=0;k<num_processes;k++){
    infile >> generate[k];
  } 
  infile.ignore(); // ignore the '\n' at the end of this line

  // phases for interference
  getline(infile,comment_line);
  double phase[10];
  cout << " Interference phases =";
  for (int k=0;k<10;k++){
    infile >> phase[k];
    cout << " " << phase[k]; 
  }
  infile.ignore(); // ignore the '\n' at the end of this line
  cout << endl;
  infile.close();
 
  // Make a TGraph of the cross section at a fixed beam energy
  GraphCrossSection(particle_types,phase,generate);

  if (Nevents>0){
    // open HDDM file
    s_iostream_t *file = init_s_HDDM(output_file_name);

    // Create some diagonistic histograms
    CreateHistograms(beamConfigFile);

    // Set up some variables for cross section calculation
    // masses of decay particles
    double m1=decay_masses[0];
    double m2=decay_masses[1];
    bool got_pipi=(fabs(m1-m2)>0.01)?false:true;
    
    double mymax=0.;
    //--------------------------------------------------------------------------
    // Event generation loop
    //--------------------------------------------------------------------------
    for (int i=1;i<=Nevents;i++){
      // photon beam
      double Egamma=0.;
      TLorentzVector beam;
      
      // Maximum value for cross section 
      double xsec_max=(got_pipi)?10.0:3.5;
      double xsec=0.,xsec_test=0.;

      // Polar angle in center of mass frame
      double theta_cm=0.;
      
      // Scalar momentum in cm
      double p_S=0.;
      
      // Mass squared of resonance
      double M_sq=0.;
      
      // Transfer 4-momentum;
      double t=0.;
      
      // vertex position at target
      float vert[4]={0.,0.,0.,0.};
      
      // use the rejection method to produce S's based on the cross section
      do{
	// First generate a beam photon using bremsstrahlung spectrum
	Egamma = cobrems_vs_E->GetRandom();
	
	// CM energy
	double s=m_p*(m_p+2.*Egamma);
	double Ecm=sqrt(s);

	// Momenta of incoming photon and outgoing S and proton in cm frame
	double p_gamma=(s-m_p_sq)/(2.*Ecm);
	
	// Mass of two-meson system     
	double m1_plus_m2=m1+m2;
	double m_max=m_p*(sqrt(1.+2.*Egamma/m_p)-1.);
	double M=m1_plus_m2+myrand->Uniform(m_max-m1_plus_m2);
	M_sq=M*M;

	// Momentum and energy of two-meson system
	double E_S=(s+M_sq-m_p_sq)/(2.*Ecm);
	p_S=sqrt(E_S*E_S-M_sq);
	
	// Momentum transfer t
	double p_diff=p_gamma-p_S;
	double t0=M_sq*M_sq/(4.*s)-p_diff*p_diff;
	double sin_theta_over_2=0.;
	t=t0;

	// Generate cos(theta) with a uniform distribution and compute the 
	// cross section at this value
	double cos_theta_cm=-1.0+myrand->Uniform(2.);
	theta_cm=acos(cos_theta_cm);
	// compute t
	sin_theta_over_2=sin(0.5*theta_cm);
	t=t0-4.*p_gamma*p_S*sin_theta_over_2*sin_theta_over_2;
	
	// Generate phi using uniform distribution
	double phi_cm=myrand->Uniform(2.*M_PI);
	
	// beam 4-vector (ignoring px and py, which are extremely small)
	beam.SetXYZT(0.,0.,Egamma,Egamma);
	
	// Velocity of the cm frame with respect to the lab frame
	TVector3 v_cm=(1./(Egamma+m_p))*beam.Vect();
	// Four-momentum of the S in the CM frame
	double pt=p_S*sin(theta_cm);
	TLorentzVector S4(pt*cos(phi_cm),pt*sin(phi_cm),p_S*cos(theta_cm),
			  sqrt(p_S*p_S+M_sq));
	// S4.Print();
	
	//Boost the S 4-momentum into the lab
	S4.Boost(v_cm);
	// S4.Print();
        
	// Compute the 4-momentum for the recoil proton
	TLorentzVector proton4=beam+target-S4;
      
	// Generate decay of S according to phase space
	TGenPhaseSpace phase_space;
	phase_space.SetDecay(S4,num_decay_particles,decay_masses);
	double weight=0.,rand_weight=1.;
	do{
	  weight=phase_space.Generate();
	  rand_weight=myrand->Uniform(1.);
	}
	while (rand_weight>weight);
	
	// Gather the particles in the reaction and write out event in hddm 
	// format
	particle_vectors[last_index]=proton4;
	for (int j=0;j<num_decay_particles;j++){
	  particle_vectors[j]=*phase_space.GetDecay(j);
	}
	
	// Cross section
	xsec=GetCrossSection(s,t,M_sq,beam,particle_types,particle_vectors,
			     phase,generate);
	
	if (xsec>mymax ) mymax=xsec;
	
	// Generate a test value for the cross section
	xsec_test=myrand->Uniform(xsec_max);
      }
      while (xsec_test>xsec);

      // Other diagnostic histograms
      thrown_t->Fill(-t);
      thrown_Egamma->Fill(Egamma);
      thrown_theta_vs_p->Fill(particle_vectors[last_index].P(),
			      180./M_PI*particle_vectors[last_index].Theta());
      thrown_mass->Fill((particle_vectors[0]+particle_vectors[1]).M());  
      thrown_mass_vs_E->Fill(Egamma,(particle_vectors[0]+particle_vectors[1]).M());
   
      WriteEvent(i,beam,vert,particle_types,particle_vectors,file);
    
      if ((i%(Nevents/10))==0) 
	cout << 100.*double(i)/double(Nevents) << "\% done" << endl;
    }
   
    // Close HDDM file
    close_s_HDDM(file);
    cout<<endl<<"Closed HDDM file"<<endl;
    cout<<" "<<Nevents<<" event written to "<<output_file_name<<endl;
    
    cout << mymax << endl;
  }
   
  // Write histograms and close root file
  rootfile->Write();
  rootfile->Close();
      
  return 0;
}


