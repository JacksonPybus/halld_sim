//
//    File: JEventProcessor_danarest.h
// Created: Mon Jul 1 09:08:37 EDT 2012
// Creator: Richard Jones
//

#ifndef _JEventProcessor_danarest_
#define _JEventProcessor_danarest_

#include <string>
using namespace std;

#include <JANA/JEventProcessor.h>
#include <JANA/JEventLoop.h>

#include <HDDM/DEventWriterREST.h>
#include <TRIGGER/DTrigger.h>

class JEventProcessor_danarest : public jana::JEventProcessor
{
	public:
		jerror_t init(void); ///< Called once at program start.
		jerror_t brun(JEventLoop *loop, int32_t runnumber); ///< Called everytime a new run number is detected.
		jerror_t evnt(JEventLoop *loop, uint64_t eventnumber); ///< Called every event.
		jerror_t erun(void); ///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);	///< Called after last event of last event source has been processed.

 private:
		bool is_mc;
};

#endif // _JEventProcessor_danarest_
