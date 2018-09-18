/*
 *  GammaPToNPartP.h
 *   by Igor Senderovich
 *  structure based on GammaToXYZP
 *  written by Matthew Shepherd 
 */

#include "GammaPToNPartP.h"
#include "particleType.h"
#include "AMPTOOLS_MCGEN/DalitzDecayFactory.h"
#include "TGenPhaseSpace.h"
#include "NBodyPhaseSpaceFactory.h"
#include "TLorentzVector.h"
#include "IUAmpTools/Kinematics.h"

#include "AMPTOOLS_MCGEN/BeamProperties.h"

GammaPToNPartP::GammaPToNPartP( float lowMass, float highMass, 
				vector<double> &ChildMass,
				ProductionMechanism::Type type, float slope, double lowT, double highT, int seed,
				TString beamConfigFile ) : 
  m_prodMech( ProductionMechanism::kProton, type, slope, seed ),
  m_target( 0, 0, 0, ParticleMass(Proton) ),
  m_ChildMass(ChildMass)
{
  m_Npart = ChildMass.size();
  assert(m_Npart>0);

  m_prodMech.setMassRange( lowMass, highMass );
  m_prodMech.setTRange( lowT, highT );

  // get beam properties from configuration file
  BeamProperties beamProp(beamConfigFile);
  cobrem_vs_E = (TH1D*)beamProp.GetFlux();
  cobrem_vs_E->GetName();

}

/**
 * The function generates a N particle final
 * state event consistent with N-body phase space.
 * (No intermediate resonances are used for important sampling.)
 */
Kinematics* 
GammaPToNPartP::generate(){

  double beamE = cobrem_vs_E->GetRandom();
  m_beam.SetPxPyPzE(0,0,beamE,beamE);

  TLorentzVector resonance;
  do{
    resonance=m_prodMech.produceResonance( m_beam );
  }while(!(resonance.E() < m_beam.E()));


  //TLorentzVector tresonance(resonance.px(),resonance.py(),
  //		    resonance.pz(),resonance.e());
  double genWeight = m_prodMech.getLastGeneratedWeight();
  
  vector< TLorentzVector > allPart;
  allPart.push_back( m_beam );
  allPart.push_back( m_beam + m_target - resonance );
  
  // X decay phase space 
  /*TGenPhaseSpace Xdecay;
  Xdecay.SetDecay(tresonance, m_Npart, m_ChildMass);
  genWeight *= Xdecay.Generate();
  */

  NBodyPhaseSpaceFactory psFactory(resonance.M(),m_ChildMass);
  vector< TLorentzVector > children = psFactory.generateDecay(false);
  genWeight *= psFactory.getLastGeneratedWeight();

  TVector3 b3(resonance.BoostVector());   // boost vector from parent
  for (unsigned int n=0; n<children.size(); ++n ){
    children[n].Boost(b3);
    allPart.push_back(children[n]);
  }

  /*
  for(unsigned int i = 0 ; i<m_Npart ; ++i){
    TLorentzVector *tPart = Xdecay.GetDecay(i);
    TLorentzVector Part(tPart->Px(),tPart->Py(),tPart->Pz(),tPart->Energy());
    allPart.push_back(Part);
    }*/
  
  return new Kinematics( allPart, genWeight );
}

void
GammaPToNPartP::addResonance( float mass, float width, float bf ){
  
  m_prodMech.addResonance( mass, width, bf );
}

