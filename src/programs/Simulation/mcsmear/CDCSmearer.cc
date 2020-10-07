#include "CDCSmearer.h"

//-----------
// cdc_config_t  (constructor)
//-----------
cdc_config_t::cdc_config_t(JEventLoop *loop) 
{
	// default values
	CDC_TDRIFT_SIGMA      = 0.0;
	CDC_TIME_WINDOW       = 0.0;
	CDC_PEDESTAL_SIGMA    = 0.0;
	//	CDC_THRESHOLD_FACTOR  = 0.0;
    CDC_ASCALE = 1.;

    // temporary? this is a ballpark guess from Naomi (sdobbs, 8/28/2017)
    CDC_INTEGRAL_TO_AMPLITUDE = 1. / 28.8;
 		
 	// load data from CCDB
 	jout << "get CDC/cdc_parms parameters from CCDB..." << endl;
    map<string, double> cdcparms;
    if(loop->GetCalib("CDC/cdc_parms", cdcparms)) {
    	jerr << "Problem loading CDC/cdc_parms from CCDB!" << endl;
    } else {
     	CDC_TDRIFT_SIGMA   = cdcparms["CDC_TDRIFT_SIGMA"]; 
 		CDC_TIME_WINDOW    = cdcparms["CDC_TIME_WINDOW"];
 		CDC_PEDESTAL_SIGMA = cdcparms["CDC_PEDESTAL_SIGMA"]; 
		// 		CDC_THRESHOLD_FACTOR = cdcparms["CDC_THRESHOLD_FACTOR"];
	}	// load data from CCDB
 	
    jout << "get CDC/diffusion_parms parameters from CCDB..." << endl;
    map<string, double> diffusionparms;
    if(loop->GetCalib("CDC/diffusion_parms", diffusionparms)) {
      jerr << "Problem loading CDC/diffusion_parms from CCDB!" << endl;
    } else {
      CDC_DIFFUSION_PAR1  = diffusionparms["d1"];
      CDC_DIFFUSION_PAR2  = diffusionparms["d2"]; 
      CDC_DIFFUSION_PAR3  = diffusionparms["d3"]; 
    }
	
 	jout << "get CDC/digi_scales parameters from CCDB..." << endl;
    map<string, double> digi_scales;
    if(loop->GetCalib("CDC/digi_scales", cdcparms)) {
    	jerr << "Problem loading CDC/digi_scales from CCDB!" << endl;
    } else {
     	CDC_ASCALE = cdcparms["CDC_ADC_ASCALE"]; 
	}
	

 // CDC correction for gain drop from progressive gas deterioration in spring 2018
      jout << "get CDC/gain_doca_correction parameters from CCDB..." << endl;
      if(loop->GetCalib("CDC/gain_doca_correction", CDC_GAIN_DOCA_PARS))
		jout << "Error loading CDC/gain_doca_correction !" << endl;




	// LOAD efficiency correction factors

	// first load some geometry information
	vector<unsigned int> Nstraws;
	int32_t runnumber = loop->GetJEvent().GetRunNumber();
    CalcNstraws(loop, runnumber, Nstraws);
    //unsigned int Nrings = Nstraws.size();

	// then load the CCDB table
	vector<double> raw_table;
	if(loop->GetCalib("CDC/wire_mc_efficiency", raw_table)) {
    	jerr << "Problem loading CDC/wire_mc_efficiency from CCDB!" << endl;
    } else {
		// now fill the table
    	wire_efficiencies.resize( Nstraws.size() );

    	int ring = 0;
    	int straw = 0;

    	for (unsigned int channel=0; channel<raw_table.size(); channel++,straw++) {
        	// if we've hit the end of the ring, move on to the next
        	if (straw == (int)Nstraws[ring]) {
            	ring++;
            	straw = 0;
        	}

        	wire_efficiencies[ring].push_back( raw_table[channel] );
    	}
    }


	if(loop->GetCalib("CDC/hit_thresholds", raw_table)) {
    	jerr << "Problem loading CDC/hit_thresholds from CCDB!" << endl;
    } else {
		// now fill the table
    	wire_thresholds.resize( Nstraws.size() );

    	int ring = 0;
    	int straw = 0;

    	for (unsigned int channel=0; channel<raw_table.size(); channel++,straw++) {
        	// if we've hit the end of the ring, move on to the next
        	if (straw == (int)Nstraws[ring]) {
            	ring++;
            	straw = 0;
        	}

        	wire_thresholds[ring].push_back( raw_table[channel] );
    	}
    }

}

//------------------
// CalcNstraws
//------------------
void cdc_config_t::CalcNstraws(jana::JEventLoop *eventLoop, int32_t runnumber, vector<unsigned int> &Nstraws)
{
    DGeometry *dgeom;
    vector<vector<DCDCWire *> >cdcwires;

    // Get pointer to DGeometry object
    DApplication* dapp=dynamic_cast<DApplication*>(eventLoop->GetJApplication());
    dgeom  = dapp->GetDGeometry(runnumber);

    // Get the CDC wire table from the XML
    dgeom->GetCDCWires(cdcwires);

    // Fill array with the number of straws for each layer
    // Also keep track of the total number of straws, i.e., the total number of detector channels
    //maxChannels = 0;
    Nstraws.clear();
    for (unsigned int i=0; i<cdcwires.size(); i++) {
        Nstraws.push_back( cdcwires[i].size() );
        //maxChannels += cdcwires[i].size();
    }

    // clear up all of the wire information
    for (unsigned int i=0; i<cdcwires.size(); i++) {
        for (unsigned int j=0; j<cdcwires[i].size(); j++) {
            delete cdcwires[i][j];
        }
    }    
}


//-----------
// SmearEvent
//-----------
void CDCSmearer::SmearEvent(hddm_s::HDDM *record)
{
   /// Smear the drift times of all CDC hits.
   /// This will add cdcStrawHit objects generated by smearing values in the
   /// cdcStrawTruthHit objects that hdgeant outputs. Any existing cdcStrawHit
   /// objects will be replaced.

   double t_max = config->TRIGGER_LOOKBACK_TIME + cdc_config->CDC_TIME_WINDOW;
   // move to wire-dependent sparsification thresholds compared to an overall factor
   //double threshold = cdc_config->CDC_THRESHOLD_FACTOR * cdc_config->CDC_PEDESTAL_SIGMA; // for sparsification

   // Loop over all cdcStraw tags
   hddm_s::CdcStrawList straws = record->getCdcStraws();
   hddm_s::CdcStrawList::iterator iter;
   for (iter = straws.begin(); iter != straws.end(); ++iter) {
 
      // If the element already contains a cdcStrawHit list then delete it.
      hddm_s::CdcStrawHitList hits = iter->getCdcStrawHits();
      if (hits.size() > 0) {
         static bool warned = false;
         iter->deleteCdcStrawHits();
         if (!warned) {
            warned = true;
            cerr << endl;
            cerr << "WARNING: CDC hits already exist in input file! Overwriting!"
                 << endl << endl;
         }
      }

      // Create new cdcStrawHit from cdcStrawTruthHit information
      hddm_s::CdcStrawTruthHitList thits = iter->getCdcStrawTruthHits();
      hddm_s::CdcStrawTruthHitList::iterator titer;
      for (titer = thits.begin(); titer != thits.end(); ++ titer) {
         // correct simulation efficiencies 
		 if (config->APPLY_EFFICIENCY_CORRECTIONS
		 		&& !gDRandom.DecideToAcceptHit(cdc_config->GetEfficiencyCorrectionFactor(iter->getRing(), iter->getStraw())))
		 	continue;

        double t = titer->getT();
        double q = titer->getQ();
        double d = titer->getD();

        double amplitude = q;   // apply scaling to convert from q to amplitude later


        // Beni effect :-)  modify to mimic spring 2018 gas deteriation

        // CDC/gain_doca_correction contains CDC_GAIN_DOCA_PARS 
        // dmax dcorr goodp0 goodp1 thisp0 thisp1

        // The reconstruction code uses these to scale up the spring 2018 hit amplitude & charge for dE/dx to approximate that w usual conditions
        // 0: dmax Hits outside this DOCA are ignored when calculating dE/dx 
        // 1: dcorr  Hits inside this DOCA are not corrected (not necessary)

        // Here the same linear functions are used to scale the hit amplitude & charge down.

        double dmax = cdc_config->CDC_GAIN_DOCA_PARS[0];  //hits with doca > dmax are not used for dE/dx in recon
        double dmin = cdc_config->CDC_GAIN_DOCA_PARS[1];  //gain is not suppressed for doca < dmin

        bool suppress_gain = 0;
        if (dmin < dmax) suppress_gain = 1;   // default values for good-gas runs have dmin=dmax=1.0cm 

        if (suppress_gain) { 

          double reference;
          double this_run;

          // apply correction for pulse amplitude.  Convert from charge to amplitude later on, after adding pedestal charge smearing 

          if (d > dmin) {
              reference = cdc_config->CDC_GAIN_DOCA_PARS[2] + d*cdc_config->CDC_GAIN_DOCA_PARS[3];
              this_run = cdc_config->CDC_GAIN_DOCA_PARS[4] + d*cdc_config->CDC_GAIN_DOCA_PARS[5];
              amplitude = amplitude * this_run/reference;   
          }  

       
  	  // This is the correction for pulse integral. 

          if (d < dmin) {

             reference    = (cdc_config->CDC_GAIN_DOCA_PARS[2] + cdc_config->CDC_GAIN_DOCA_PARS[3]*dmin) * (dmin - d);
             reference += (cdc_config->CDC_GAIN_DOCA_PARS[2] + 0.5*cdc_config->CDC_GAIN_DOCA_PARS[3]*(dmin+dmax)) * (dmax - dmin);

             this_run    = (cdc_config->CDC_GAIN_DOCA_PARS[4] + cdc_config->CDC_GAIN_DOCA_PARS[5]*dmin) * (dmin - d);
             this_run += (cdc_config->CDC_GAIN_DOCA_PARS[4] + 0.5*cdc_config->CDC_GAIN_DOCA_PARS[5]*(dmin+dmax)) * (dmax - dmin);

          } else { 

             reference = (cdc_config->CDC_GAIN_DOCA_PARS[2] + 0.5*cdc_config->CDC_GAIN_DOCA_PARS[3]*(d+dmax)) * (dmax - d);
             this_run   = (cdc_config->CDC_GAIN_DOCA_PARS[4] + 0.5*cdc_config->CDC_GAIN_DOCA_PARS[5]*(d+dmax)) * (dmax - d);

          }

          q = q * this_run/reference;   
 
        }   // end of gain suppression


        double smearcharge = 0;   // Using the same smearing for both amp and integral for the time being

        if(config->SMEAR_HITS) {
	  // Smear out the CDC drift time using the specified sigma.
     	  double dsq=d*d;
	  double sig_diffusion=cdc_config->CDC_DIFFUSION_PAR1*d
	    +cdc_config->CDC_DIFFUSION_PAR2*dsq
	    +cdc_config->CDC_DIFFUSION_PAR3*dsq*d;
	  double sig_electronics=cdc_config->CDC_TDRIFT_SIGMA*1.0e9;
	  double t_sig=sig_electronics+sig_diffusion;

	  t += gDRandom.SampleGaussian(t_sig);
	  // Pedestal-smeared charge
	  smearcharge =  gDRandom.SampleGaussian(cdc_config->CDC_PEDESTAL_SIGMA);
	}

        q += smearcharge;

        amplitude += smearcharge;   // add on pedestal noise, then convert from charge to amplitude

        // convert charge back into adc units to implement saturation
        double raw_integral = q/cdc_config->CDC_ASCALE;

        // estimate integrated pedestal.  1600 should be IE*8; IE is fa125 integration end sample
        double integrated_pedestal = 100*(1600.0 - t)/8.0;   // pedestal * nsamples in pulse
        if (integrated_pedestal<0) integrated_pedestal = 0;  //should not happen

        // max readout value is 262143, this would include the pedestal
        double pulseintegral_saturation = 262143 - integrated_pedestal;

        if (raw_integral > pulseintegral_saturation) raw_integral = pulseintegral_saturation;

        q = raw_integral*cdc_config->CDC_ASCALE;


        // raw_amplitude mimics the signal in adc units 0-4095. This is used here for comparison with the threshold.
        // amplitude is in charge units. This is included in the hddm file and used for dE_amp/dx

        double raw_amplitude = amplitude * cdc_config->CDC_INTEGRAL_TO_AMPLITUDE / cdc_config->CDC_ASCALE;

        // the fadc signal saturates at 4095; the pedestal is approximately 100
        double saturation = 3995;

        // apply saturation to raw_amplitude
        if (raw_amplitude > saturation) raw_amplitude = saturation;

        // convert this from raw amplitude scale back into amplitude scale 
        saturation = 3995*cdc_config->CDC_ASCALE/cdc_config->CDC_INTEGRAL_TO_AMPLITUDE;

        // apply saturation to amplitude
        if (amplitude > saturation) amplitude = saturation;
       
        // per-wire threshold in ADC units
        double threshold = cdc_config->GetWireThreshold(iter->getRing(), iter->getStraw());
        if (t > config->TRIGGER_LOOKBACK_TIME && t < t_max && raw_amplitude > threshold) {
            hits = iter->addCdcStrawHits();
            hits().setT(t);
            hits().setQ(q);

	    hddm_s::CdcDigihitList digihit = hits().addCdcDigihits();
	    digihit().setPeakAmp(amplitude);

        }

        if (config->DROP_TRUTH_HITS) {
            iter->deleteCdcStrawTruthHits();
        }
      }
   }
}
