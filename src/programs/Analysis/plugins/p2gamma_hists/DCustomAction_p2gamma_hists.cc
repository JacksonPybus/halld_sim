// $Id$
//
//    File: DCustomAction_p2gamma_hists.cc
// Created: Wed Jan 21 16:53:41 EST 2015
// Creator: jrsteven (on Linux ifarm1401 2.6.32-431.el6.x86_64 x86_64)
//

#include "DCustomAction_p2gamma_hists.h"

void DCustomAction_p2gamma_hists::Initialize(JEventLoop* locEventLoop)
{
	japp->RootWriteLock(); //ACQUIRE ROOT LOCK!!
	{
		//Required: Create a folder in the ROOT output file that will contain all of the output ROOT objects (if any) for this action.
			//If another thread has already created the folder, it just changes to it. 
		CreateAndChangeTo_ActionDirectory();

		dEgamma = GetOrCreate_Histogram<TH1I>("Egamma", "TAGGER photon energy; E_{#gamma}", 400, 0., 12.);
		
		dMM2_M2g = GetOrCreate_Histogram<TH2I>("MM2_M2g", "MM^{2} off p #gamma#gamma vs M_{#gamma#gamma}; M_{#gamma#gamma}; MM^{2}", 500, 0.0, 1.0, 200, -1., 1.);
		dProton_dEdx_P = GetOrCreate_Histogram<TH2I>("Proton_dEdx_P","dE/dx vs p; p; dE/dx",200,0,2,500,0,5);
		dProton_P_Theta = GetOrCreate_Histogram<TH2I>("Proton_P_Theta","p vs #theta; #theta; p (GeV/c)",180,0,180,120,0,12);
		dProtonPhi_Egamma = GetOrCreate_Histogram<TH2I>("ProtonPhi_Egamma","#phi vs E_{#gamma}; E_{#gamma}; proton #phi",240,0,6,360,-180,180);
		dProtonPhi_Theta = GetOrCreate_Histogram<TH2I>("ProtonPhi_Theta","#phi vs #theta_{CM}; #pi^{0} #theta_{CM}; proton #phi",180,0,180,360,-180,180);
		dProtonPhi_t = GetOrCreate_Histogram<TH2I>("ProtonPhi_t","#psi vs |t|; |t|; proton #phi",1000,0.,5.,360,-180,180);

		dPi0Phi_Egamma = GetOrCreate_Histogram<TH2I>("Pi0Phi_Egamma","#phi vs E_{#gamma}; E_{#gamma}; #pi^{0} #phi",240,0,6,360,-180,180);
		dPi0Phi_Theta = GetOrCreate_Histogram<TH2I>("Pi0Phi_Theta","#phi vs #theta_{CM}; #pi^{0} #theta_{CM}; #pi^{0} #phi",180,0,180,360,-180,180);
		dPi0EgammaCorr = GetOrCreate_Histogram<TH2I>("Pi0EgammaCorr","E_{#gamma 1} vs E_{#gamma 2}; E_{#gamma 2}; E_{#gamma 1}",100,0.,5., 100.,0.,5.);

		dDeltaE_M2g = GetOrCreate_Histogram<TH2I>("dDeltaE_M2g", "#Delta E vs M_{#gamma#gamma}; M_{#gamma#gamma}; #Delta E (tagger - p#gamma#gamma)", 500, 0.0, 1.0, 200, -5., 5.);
		dPhi2g_PhiP = GetOrCreate_Histogram<TH2I>("Phi2g_PhiP", "#phi_{#gamma#gamma} vs. #phi_{p}; #phi_{p}; #phi_{#gamma#gamma}", 360, -180.0, 180.0, 360, -180., 180.);
		dDeltaPhi_M2g = GetOrCreate_Histogram<TH2I>("DeltaPhi_M2g", "#Delta#phi vs M_{#gamma#gamma}; M_{#gamma#gamma}; #Delta#phi", 500, 0.0, 1.0, 360, 0.0, 360.0);

		dEgamma_M2g_ProtonTag = GetOrCreate_Histogram<TH2I>("dEgamma_M2g_ProtonTag", "E_{#gamma} vs M_{#gamma#gamma}; M_{#gamma#gamma}; E_{#gamma}", 500, 0.0, 1.0, 240, 0., 6.);

		dMM2_M2g_CoplanarTag = GetOrCreate_Histogram<TH2I>("MM2_M2g_CoplanarTag", "MM^{2} off p #gamma#gamma vs M_{#gamma#gamma}: Coplanar Tag; M_{#gamma#gamma}; MM^{2}", 500, 0.0, 1.0, 200, -1., 1.);
                dDeltaE_M2g_CoplanarTag = GetOrCreate_Histogram<TH2I>("dDeltaE_M2g_CoplanarTag", "#Delta E vs M_{#gamma#gamma}: Coplanar Tag; M_{#gamma#gamma}; #Delta E (tagger - p#gamma#gamma)", 500, 0.0, 1.0, 200, -5., 5.);
		dMM2_DeltaE_CoplanarTag = GetOrCreate_Histogram<TH2I>("dMM2_DeltaE_CoplanarTag", "MM^{2} vs #Delta E: Coplanar Tag; #Delta E (tagger - p#gamma#gamma); MM^{2}", 200, -5., 5., 200, -1., 1.);

		dMM2_M2g_ProtonTag = GetOrCreate_Histogram<TH2I>("MM2_M2g_ProtonTag", "MM^{2} off p #gamma#gamma vs M_{#gamma#gamma}: Proton Tag; M_{#gamma#gamma}; MM^{2}", 500, 0.0, 1.0, 200, -1., 1.);
		dDeltaE_M2g_ProtonTag = GetOrCreate_Histogram<TH2I>("dDeltaE_M2g_ProtonTag", "#Delta E vs M_{#gamma#gamma}: Proton Tag; M_{#gamma#gamma}; #Delta E (tagger - p#gamma#gamma)", 500, 0.0, 1.0, 200, -5., 5.);
		dMM2_DeltaE_ProtonTag = GetOrCreate_Histogram<TH2I>("dMM2_DeltaE_ProtonTag", "MM^{2} vs #Delta E: Proton Tag; #Delta E (tagger - p#gamma#gamma); MM^{2}", 200, -5., 5., 200, -1., 1.);
		
	}
	japp->RootUnLock(); //RELEASE ROOT LOCK!!
}

bool DCustomAction_p2gamma_hists::Perform_Action(JEventLoop* locEventLoop, const DParticleCombo* locParticleCombo)
{
	const DParticleComboStep* locParticleComboStep = locParticleCombo->Get_ParticleComboStep(0);

        // get beam photon energy and final state particles
        const DKinematicData* locBeamPhoton = NULL;
        deque<const DKinematicData*> locParticles;
        if(!Get_UseKinFitResultsFlag()) { //measured
		locBeamPhoton = locParticleComboStep->Get_InitialParticle_Measured();
                locParticleComboStep->Get_FinalParticles_Measured(locParticles);
	}
	else {
		locBeamPhoton = locParticleComboStep->Get_InitialParticle();
		locParticleComboStep->Get_FinalParticles(locParticles);
	}
        double locBeamPhotonEnergy = locBeamPhoton->energy();
	
	// cut on tagger energy
	if(locBeamPhotonEnergy < 1.5) 
		return false;
	
	// calculate missing mass
	DLorentzVector locMissingP4; 
	DLorentzVector locProtonP4Init(0,0,0,0.938);
	locMissingP4 += locProtonP4Init;
	locMissingP4 += locBeamPhoton->lorentzMomentum();
	DLorentzVector locSumInitP4 = locMissingP4;

	// reconstructed proton variables
	const DChargedTrack* locChargedTrack = static_cast<const DChargedTrack*>(locParticleComboStep->Get_FinalParticle_SourceObject(2));
	const DChargedTrackHypothesis* locChargedTrackHypothesis = locChargedTrack->Get_Hypothesis(Proton);
	double dEdx = locChargedTrackHypothesis->dEdx()*1e6;
	DLorentzVector locProtonP4 = locChargedTrackHypothesis->lorentzMomentum();	
	
	// calculate missing mass
	DLorentzVector loc2g_P4;
	for(size_t loc_i = 0; loc_i < 3; ++loc_i) {
		locMissingP4 -= locParticles[loc_i]->lorentzMomentum();
		
		if(locParticles[loc_i]->PID() == Gamma) 
			loc2g_P4 += locParticles[loc_i]->lorentzMomentum();
	}

	// production kinematics
	TVector3 locBoostVector = -1.*locSumInitP4.BoostVector();
	TLorentzVector locGamma_P4CMFrame = locBeamPhoton->lorentzMomentum();
	TLorentzVector locPi0_P4CMFrame = loc2g_P4;
	locGamma_P4CMFrame.Boost(locBoostVector);
	locPi0_P4CMFrame.Boost(locBoostVector);
	double locThetaCM = locGamma_P4CMFrame.Vect().Angle(locPi0_P4CMFrame.Vect());
	TLorentzVector locDelta = (locProtonP4 - locProtonP4Init);
	double t = locDelta.M2();

	double dEdxCut = 2.2;
	
	japp->RootWriteLock(); //ACQUIRE ROOT LOCK!!
	{
		// Fill histograms here
		dEgamma->Fill(locBeamPhotonEnergy);

		dMM2_M2g->Fill(loc2g_P4.M(), locMissingP4.M2());
		dDeltaE_M2g->Fill(loc2g_P4.M(),locMissingP4.E());

		double locDeltaPhi = (locProtonP4.Phi() - loc2g_P4.Phi())*180./TMath::Pi();
		if(locDeltaPhi > 360.) locDeltaPhi -= 360.;
		if(locDeltaPhi < 0.) locDeltaPhi += 360.;
		dPhi2g_PhiP->Fill(locProtonP4.Phi()*180./TMath::Pi(), loc2g_P4.Phi()*180./TMath::Pi());
		dDeltaPhi_M2g->Fill(loc2g_P4.M(), locDeltaPhi);

		// require proton and pi0 are back-to-back
		if(locDeltaPhi < 175. || locDeltaPhi > 185.) {
			japp->RootUnLock();	
			return false;
		}
		dMM2_M2g_CoplanarTag->Fill(loc2g_P4.M(), locMissingP4.M2());
		dDeltaE_M2g_CoplanarTag->Fill(loc2g_P4.M(), locMissingP4.E());
		if(loc2g_P4.M() > 0.10 && loc2g_P4.M() < 0.16)
			dMM2_DeltaE_CoplanarTag->Fill(locMissingP4.E(), locMissingP4.M2());

		// tag proton with dE/dx
		if(dEdx > dEdxCut) {
			dMM2_M2g_ProtonTag->Fill(loc2g_P4.M(), locMissingP4.M2());
			dDeltaE_M2g_ProtonTag->Fill(loc2g_P4.M(), locMissingP4.E());
			if(loc2g_P4.M() > 0.10 && loc2g_P4.M() < 0.16)
				dMM2_DeltaE_ProtonTag->Fill(locMissingP4.E(), locMissingP4.M2());

			// di-photon invariant mass for exclusive g+p -> p + 2g
			if(fabs(locMissingP4.M2()) < 0.05 && fabs(locMissingP4.E()) < 0.5) {
				dEgamma_M2g_ProtonTag->Fill(loc2g_P4.M(),locBeamPhotonEnergy);
			}
		}

		// for pi0 candidates require recoil proton
		if(loc2g_P4.M() > 0.10 && loc2g_P4.M() < 0.16 && fabs(locMissingP4.M2()) < 0.05 && fabs(locMissingP4.E()) < 0.5) {
			dProton_dEdx_P->Fill(locProtonP4.Vect().Mag(), dEdx);
			dProton_P_Theta->Fill(locProtonP4.Vect().Theta()*180/TMath::Pi(), locProtonP4.Vect().Mag());			
			dProtonPhi_Egamma->Fill(locBeamPhotonEnergy, locProtonP4.Phi()*180/TMath::Pi());
			dPi0Phi_Egamma->Fill(locBeamPhotonEnergy, loc2g_P4.Phi()*180/TMath::Pi());

			if(locBeamPhotonEnergy > 2.5 && locBeamPhotonEnergy < 3.0){ // 2931 is 2.9 to 3.35 GeV
				dProtonPhi_Theta->Fill(locThetaCM*180/TMath::Pi(), locProtonP4.Phi()*180/TMath::Pi());
				dProtonPhi_t->Fill(fabs(t), locProtonP4.Phi()*180/TMath::Pi());
				dPi0Phi_Theta->Fill(locThetaCM*180/TMath::Pi(), loc2g_P4.Phi()*180/TMath::Pi());
				dPi0EgammaCorr->Fill(locParticles[0]->energy(),locParticles[1]->energy());
			}
			
			japp->RootUnLock();
			return true;
		}
	}
	japp->RootUnLock(); //RELEASE ROOT LOCK!!

	return false; //return false if you want to use this action to apply a cut (and it fails the cut!)
}