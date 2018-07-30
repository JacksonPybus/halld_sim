//
//	 File: JEventProcessor_randomtrigger_skim.h
// Created: Wed Nov 9 15:08:37 EDT 2014
// Creator: Paul Mattione
//

#ifndef _JEventProcessor_randomtrigger_skim_
#define _JEventProcessor_randomtrigger_skim_

#include <string>
#include <vector>

#include <JANA/JEventProcessor.h>
#include <JANA/JEventLoop.h>
#include <JANA/JApplication.h>
#include <JANA/JEventSource.h>
#include <JANA/JEvent.h>

#include "evio_writer/DEventWriterEVIO.h"

#include "PID/DChargedTrack.h"
#include "DAQ/DEPICSvalue.h"

#include "DAQ/DBeamCurrent.h"
#include "DAQ/DBeamCurrent_factory.h"

using namespace std;
using namespace jana;

class JEventProcessor_randomtrigger_skim : public jana::JEventProcessor
{
  public:
    
    jerror_t init(void); ///< Called once at program start.
    jerror_t brun(JEventLoop *loop, int32_t runnumber); ///< Called everytime a new run number is detected.
    jerror_t evnt(JEventLoop *loop, uint64_t eventnumber); ///< Called every event.
    jerror_t erun(void); ///< Called everytime run number changes, provided brun has been called.
    jerror_t fini(void);	///< Called after last event of last event source has been processed.

  private:
    DBeamCurrent_factory *dBeamCurrentFactory;
  
};

#endif // _JEventProcessor_randomtrigger_skim_

