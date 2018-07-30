//
// JEventProcessor_danarest.cc
//
// JANA event processor plugin writes out rest events to a file
//
// Richard Jones, 1-July-2012

#include "JEventProcessor_danarest.h"
#include <TRACKING/DMCThrown.h>

// Make us a plugin
// for initializing plugins
extern "C" {
   void InitPlugin(JApplication *app) {
      InitJANAPlugin(app);
      app->AddProcessor(new JEventProcessor_danarest(), true);
   }
} // "extern C"

//-------------------------------
// init
//-------------------------------
jerror_t JEventProcessor_danarest::init(void)
{
  return NOERROR;
}

//-------------------------------
// brun
//-------------------------------
jerror_t JEventProcessor_danarest::brun(JEventLoop *locEventLoop, int32_t runnumber)
{
  
   return NOERROR;
}

//-------------------------------
// evnt
//-------------------------------
jerror_t JEventProcessor_danarest::evnt(JEventLoop *locEventLoop, uint64_t eventnumber)
{
  // Check if we have thrown events (therefore MC)
  vector<const DMCThrown *>throwns;
  locEventLoop->Get(throwns);

	//CHECK TRIGGER TYPE
	const DTrigger* locTrigger = NULL;
	locEventLoop->GetSingle(locTrigger);
	if(throwns.size()==0 && (!locTrigger->Get_IsPhysicsEvent()))
		return NOERROR;

	// Write this event to the rest output stream.
	vector<const DEventWriterREST*> locEventWriterRESTVector;
	locEventLoop->Get(locEventWriterRESTVector);
	locEventWriterRESTVector[0]->Write_RESTEvent(locEventLoop, "");

   return NOERROR;
}

//-------------------------------
// erun
//-------------------------------
jerror_t JEventProcessor_danarest::erun(void)
{
   return NOERROR;
}

//-------------------------------
// fini
//-------------------------------
jerror_t JEventProcessor_danarest::fini(void)
{
   return NOERROR;
}
