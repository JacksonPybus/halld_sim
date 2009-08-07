// $Id$
//
//    File: DTrackHitSelectorALT1.cc
// Created: Fri Feb  6 08:22:58 EST 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.6.0 i386)
//

#include <TRACKING/DReferenceTrajectory.h>

#include "DTrackHitSelectorALT1.h"

#ifndef ansi_escape
#define ansi_escape			((char)0x1b)
#define ansi_bold 			ansi_escape<<"[1m"
#define ansi_normal			ansi_escape<<"[0m"
#define ansi_red				ansi_escape<<"[31m"
#define ansi_green			ansi_escape<<"[32m"
#define ansi_blue				ansi_escape<<"[34m"
#endif // ansi_escape

//---------------------------------
// DTrackHitSelectorALT1    (Constructor)
//---------------------------------
DTrackHitSelectorALT1::DTrackHitSelectorALT1(jana::JEventLoop *loop):DTrackHitSelector(loop)
{
	HS_DEBUG_LEVEL = 0;
	gPARMS->SetDefaultParameter("TRKFIT:HS_DEBUG_LEVEL", HS_DEBUG_LEVEL);

}

//---------------------------------
// ~DTrackHitSelectorALT1    (Destructor)
//---------------------------------
DTrackHitSelectorALT1::~DTrackHitSelectorALT1()
{

}

//---------------------------------
// GetCDCHits
//---------------------------------
void DTrackHitSelectorALT1::GetCDCHits(fit_type_t fit_type, DReferenceTrajectory *rt, const vector<const DCDCTrackHit*> &cdchits_in, vector<const DCDCTrackHit*> &cdchits_out) const
{
	/// Determine the probability that for each CDC hit that it came from the track with the given trajectory.
	///
	/// This will calculate a probability for each CDC hit that
	/// it came from the track represented by the given
	/// DReference trajectory. The probability is based on
	/// the residual between the distance of closest approach
	/// of the trajectory to the wire and the drift time for
	/// time-based tracks and the distance to the wire for
	/// wire-based tracks.

	// Calculate beta of particle.
	double beta = 1.0/sqrt(1.0+pow(rt->GetMass(),2.0)/rt->swim_steps[0].mom.Mag2());
	
	// The error on the residual. This will be different based on the
	// quality of the track and whether MULS is on or not etc.
	// In principle, this could also depend on the momentum parameters
	// of the track.
	double sigma;
	switch(fit_type){
		case kTimeBased:
			sigma = 0.8/sqrt(12.0);
			break;
		case kWireBased:
			sigma = 2.0*0.8/sqrt(12.0);
			break;
		case kHelical:
		default:
			sigma = 10.0*0.8/sqrt(12.0);
	}

	// Low-momentum tracks are more poorly defined than high-momentum tracks.
	// We account for that here by increasing the error as a function of momentum
	double g = 0.350/sqrt(log(2.0)); // total guess
	sigma *= 1.0 + exp(-pow(rt->swim_steps[0].mom.Mag()/g,2.0));
	
	// Minimum probability of hit belonging to wire and still be accepted
	double MIN_HIT_PROB = 0.05;

	vector<const DCDCTrackHit*>::const_iterator iter;
	for(iter=cdchits_in.begin(); iter!=cdchits_in.end(); iter++){
		const DCDCTrackHit *hit = *iter;
		
		// Find the DOCA to this wire
		double s;
		double doca = rt->DistToRT(hit->wire, &s);

		// Get "measured" distance to wire. For time-based tracks
		// this is calculated from the drift time. For all other
		// tracks, this is assumed to be half a cell size
		double dist;
		if(fit_type == kTimeBased){
			// Distance using drift time
			// NOTE: Right now we assume pions for the TOF
			// and a constant drift velocity of 55um/ns
			double tof = s/(beta*3E10*1E-9);
			dist = (hit->tdrift - tof)*55E-4;
		}else{
			dist = 0.8/2.0; // half cell-size
		}
		
		// For time-based and wire-based tracks, the fit was
		// weighted for multiple scattering by material times 
		// angle giving preference to the begining of the 
		// track. Take this into account here by enhancing the
		// error for hits further from the vertex
		double sigma_total = sigma;
		if(fit_type == kTimeBased || fit_type == kWireBased){
			sigma_total *= 1.0 + s/50.0; // double error at 50cm out (guess for now)
		}
		
		// Residual
		double resi = dist - doca;
		double chisq = pow(resi/sigma_total, 2.0);

		// Use chi-sq probaility function with Ndof=1 to calculate probability
		double probability = TMath::Prob(chisq, 1);
		if(probability>=MIN_HIT_PROB)cdchits_out.push_back(hit);

		if(HS_DEBUG_LEVEL>10){
			_DBG_;
			if(probability>=MIN_HIT_PROB)cerr<<ansi_bold<<ansi_green;
			cerr<<"s="<<s<<" doca="<<doca<<" dist="<<dist<<" resi="<<resi<<" sigma="<<sigma_total<<" prob="<<probability<<endl;
			cerr<<ansi_normal;
		}
	}
}

//---------------------------------
// GetFDCHits
//---------------------------------
void DTrackHitSelectorALT1::GetFDCHits(fit_type_t fit_type, DReferenceTrajectory *rt, const vector<const DFDCPseudo*> &fdchits_in, vector<const DFDCPseudo*> &fdchits_out) const
{
	/// Determine the probability that for each FDC hit that it came from the track with the given trajectory.
	///
	/// This will calculate a probability for each FDC hit that
	/// it came from the track represented by the given
	/// DReference trajectory. The probability is based on
	/// the residual between the distance of closest approach
	/// of the trajectory to the wire and the drift time
	/// and the distance along the wire.

	// Calculate beta of particle assuming its a pion for now. If the
	// particles is really a proton or an electron, the residual
	// calculated below will only be off by a little.
	double beta = 1.0/sqrt(1.0+pow(rt->GetMass(),2.0)/rt->swim_steps[0].mom.Mag2());
	
	// The error on the residual. This will be different based on the
	// quality of the track and whether MULS is on or not etc.
	// In principle, this could also depend on the momentum parameters
	// of the track.
	double sigma_anode = 0.5/sqrt(12.0);
	double sigma_cathode = 0.5/sqrt(12.0);
	switch(fit_type){
		case kTimeBased:
			sigma_anode = 0.5/sqrt(12.0);
			sigma_cathode = 0.5/sqrt(12.0);
			break;
		case kWireBased:
			sigma_anode = 2.0*0.5/sqrt(12.0);
			sigma_cathode = 2.0*0.5/sqrt(12.0);
			break;
		case kHelical:
		default:
			sigma_anode = 10.0*0.5/sqrt(12.0);
			sigma_cathode = 10.0*0.5/sqrt(12.0);
	}
	
	// Scale the errors up by 1/p for p<1GeV/c. This accounts for the poorer knowledge
	// of the track parameters contained in the candidate for low momentum tracks
	double ptot = rt->swim_steps[0].mom.Mag();
	if(ptot<1.0){
		sigma_anode /= ptot;
		sigma_cathode /= ptot;
	}
	
	// Higher mass particles (protons) lose energy quicker and therefore have even
	// less helical shapes for candidates and larger errors due to eloss straggling.
	// These next 2 lines are here mainly to improve the situation for protons so
	// we take a stab at scaling up the errors by the number of pion masses since
	// this worked reasonably well for pions before.
	sigma_anode *= rt->GetMass()/0.13957;
	sigma_cathode *= rt->GetMass()/0.13957;

	// Minimum probability of hit belonging to wire and still be accepted
	double MIN_HIT_PROB = 0.01;

	vector<const DFDCPseudo*>::const_iterator iter;
	for(iter=fdchits_in.begin(); iter!=fdchits_in.end(); iter++){
		const DFDCPseudo *hit = *iter;
		
		// Find the DOCA to this wire
		double s;
		double doca = rt->DistToRT(hit->wire, &s);


		// Get "measured" distance to wire. For time-based tracks
		// this is calculated from the drift time. For all other
		// tracks, this is assumed to be half a cell size
		double dist;
		if(kTimeBased){
			// Distance using drift time
			// NOTE: Right now we assume pions for the TOF
			// and a constant drift velocity of 55um/ns
			double tof = s/(beta*3E10*1E-9);
			dist = (hit->time - tof)*55E-4;
		}else{
			dist = 0.5/2.0; // half cell-size
		}
		
		// Anode Residual
		double resi = dist - doca;		

		// Cathode Residual
		double u=rt->GetLastDistAlongWire();
		double resic = u - hit->s;

		// Probability of this hit being on the track
		double chisq = pow(resi/sigma_anode, 2.0) + pow(resic/sigma_cathode, 2.0);
		double probability = TMath::Prob(chisq, 2);
		if(probability>=MIN_HIT_PROB)fdchits_out.push_back(hit);

		if(HS_DEBUG_LEVEL>10){
			_DBG_;
			if(probability>=MIN_HIT_PROB)cerr<<ansi_bold<<ansi_blue;
			cerr<<"s="<<s<<" doca="<<doca<<" dist="<<dist<<" resi="<<resi<<" resic="<<resic<<" chisq="<<chisq<<" prob="<<probability<<endl;
			cerr<<ansi_normal;
		}
	}
}
