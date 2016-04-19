// $Id$
//
//    File: DParsedEvent.h
// Created: Mon Mar 28 11:07:41 EDT 2016
// Creator: davidl (on Darwin harriet.jlab.org 13.4.0 i386)
//

#ifndef _DParsedEvent_
#define _DParsedEvent_

#include <string>
#include <map>
using std::string;
using std::map;

#include <JANA/jerror.h>
#include <JANA/JObject.h>
#include <JANA/JEventLoop.h>
using namespace jana;

#include "daq_param_type.h"
#include "DModuleType.h"


#include "Df250Config.h"
#include "Df250PulseIntegral.h"
#include "Df250StreamingRawData.h"
#include "Df250WindowSum.h"
#include "Df250PulseRawData.h"
#include "Df250TriggerTime.h"
#include "Df250PulseTime.h"
#include "Df250PulsePedestal.h"
#include "Df250WindowRawData.h"
#include "Df125Config.h"
#include "Df125TriggerTime.h"
#include "Df125PulseIntegral.h"
#include "Df125PulseTime.h"
#include "Df125PulsePedestal.h"
#include "Df125PulseRawData.h"
#include "Df125WindowRawData.h"
#include "Df125CDCPulse.h"
#include "Df125FDCPulse.h"
#include "DF1TDCConfig.h"
#include "DF1TDCHit.h"
#include "DF1TDCTriggerTime.h"
#include "DCAEN1290TDCConfig.h"
#include "DCAEN1290TDCHit.h"
#include "DCODAEventInfo.h"
#include "DCODAROCInfo.h"
#include "DTSscalers.h"
#include "DEPICSvalue.h"
#include "DEventTag.h"
#include "Df250BORConfig.h"
#include "Df125BORConfig.h"
#include "DF1TDCBORConfig.h"
#include "DCAEN1290TDCBORConfig.h"

// Here is some C++ macro script-fu. For each type of class the DParsedEvent
// can hold, we want to have a vector of pointers to that type of object. 
// There's also a number of other things we need to do for each of these types
// but don't want to have to write the entire list out multiple times. The
// "MyTypes" macro below defines all of the types and then is used multiple
// times in the DParsedEvent class. It would be nice if we could also generate
// the above #includes using this trick but alas, the C++ language
// prohibits using #includes in macros so it's not possible.
#define MyTypes(X) \
		X(Df250Config) \
		X(Df250PulseIntegral) \
		X(Df250StreamingRawData) \
		X(Df250WindowSum) \
		X(Df250PulseRawData) \
		X(Df250TriggerTime) \
		X(Df250PulseTime) \
		X(Df250PulsePedestal) \
		X(Df250WindowRawData) \
		X(Df125Config) \
		X(Df125TriggerTime) \
		X(Df125PulseIntegral) \
		X(Df125PulseTime) \
		X(Df125PulsePedestal) \
		X(Df125PulseRawData) \
		X(Df125WindowRawData) \
		X(Df125CDCPulse) \
		X(Df125FDCPulse) \
		X(DF1TDCConfig) \
		X(DF1TDCHit) \
		X(DF1TDCTriggerTime) \
		X(DCAEN1290TDCConfig) \
		X(DCAEN1290TDCHit) \
		X(DCODAEventInfo) \
		X(DCODAROCInfo) \
		X(DTSscalers) \
		X(DEPICSvalue) \
		X(DEventTag) \
		X(Df250BORConfig) \
		X(Df125BORConfig) \
		X(DF1TDCBORConfig) \
		X(DCAEN1290TDCBORConfig)


class DParsedEvent{
	public:		
		
		atomic<bool> in_use;
		bool copied_to_factories;
		
		uint64_t istreamorder;
		uint64_t run_number;
		uint64_t event_number;
		bool     sync_flag;
		
		// For each type defined in "MyTypes" above, define a vector of
		// pointers to it with a name made by prepending a "v" to the classname
		// The following expands to things like e.g.
		//
		//       vector<Df250Config*> vDf250Config;
		//
		#define makevector(A) vector<A*>  v##A;
		MyTypes(makevector)
		
		// DParsedEvent objects are recycled to save malloc/delete cycles. Do the
		// same for the objects they provide by creating a pool vector for each
		// object type. No need for locks here since this will only ever be accessed
		// by the same worker thread.
		#define makepoolvector(A) vector<A*>  v##A##_pool;
		MyTypes(makepoolvector)

		// Method to return all objects in vectors to their respective pools and 
		// clear the vectors to set up for processing the next event.
		// This is called from DEVIOWorkerThread::MakeEvents
		#define returntopool(A) if(!v##A.empty()){ v##A##_pool.insert(v##A##_pool.end(), v##A.begin(), v##A.end()); v##A.clear(); }
		void Clear(void){ MyTypes(returntopool) }

		// Method to delete all objects in all vectors and all pools. This should
		// usually only be called from the DParsedEvent destructor
		#define deletevector(A) for(auto p : v##A       ) delete p;
		#define deletepool(A)   for(auto p : v##A##_pool) delete p;
		#define clearvectors(A) v##A.clear(); v##A##_pool.clear();
		void Delete(void){
			MyTypes(deletevector)
			MyTypes(deletepool)
			MyTypes(clearvectors)
		}
		
		// Define a class that has pointers to factories for each data type.
		// One of these is instantiated for each JEventLoop encountered.
		// See comments below for CopyToFactories for details.
		#define makefactoryptr(A) JFactory<A> *fac_##A;
		#define copyfactoryptr(A) fac_##A = (JFactory<A>*)loop->GetFactory(#A);
		class DFactoryPointers{
			public:
				JEventLoop *loop;
				MyTypes(makefactoryptr)

				DFactoryPointers():loop(NULL){}
				~DFactoryPointers(){}
				
				void Init(JEventLoop *loop){
					this->loop = loop;
					MyTypes(copyfactoryptr)
				}
		};
		
		// Copy objects to factories. For efficiency, we keep an object that
		// holds the relevant factory pointers for each JEventLoop we encounter.
		// This avoids having to look up the factory pointer for each data type
		// for every event. Note that only one processing thread at a time will
		// ever call this method for this DParsedEvent object so we don't need
		// to lock access to the factory_pointers map.
		#define copytofactory(A) facptrs.fac_##A->CopyTo(v##A);
		#define setevntcalled(A) facptrs.fac_##A->Set_evnt_called();
		#define keepownership(A) facptrs.fac_##A->SetFactoryFlag(JFactory_base::NOT_OBJECT_OWNER);
		void CopyToFactories(JEventLoop *loop){
			// Get DFactoryPointers for this JEventLoop, creating new one if necessary
			DFactoryPointers &facptrs = factory_pointers[loop];
			if(facptrs.loop == NULL) facptrs.Init(loop);

			// Copy all data vectors to appropriate factories
			MyTypes(copytofactory)
			MyTypes(setevntcalled)
			MyTypes(keepownership)
			copied_to_factories=true;
		}
		
		// Method to check class name against each classname in MyTypes returning
		// true if found and false if not.
		#define checkclassname(A) if(classname==#A) return true;
		bool IsParsedDataType(string &classname){
			MyTypes(checkclassname)
			return false;
		}
		
		// The following is pretty complicated to understand. What it does is
		// create a templated method for each data type that is used to
		// replace the use of "new" to allocate an object of that type.
		// What makes it complicated is the use of C++11 variadic functions
		// to allow passing in (and through) variable length argument lists.
		// This is needed since each type's constructor takes a different
		// set of arguments and we don't want to have to encode all of that
		// here.
		//
		// For each data type, a method called NEW_XXX is defined that looks
		// to see if an object already exists in the corresponding pool vector.
		// If so, it returns a pointer to it after doing an in-place constructor
		// call with the given arguments. If no objects are available in the
		// pool, then a new one is created in the normal way with the given
		// arguments.
		//
		// This will also automatically add the created/recycled object to
		// the appropriate vXXX vector as part of the current event. It
		// returns of pointer to the object so that it can be accessed by the
		// caller of needed.
		//
		// This will provide a method that looks something like this:
		//
		//  Df250TriggerTime* NEW_Df250TriggerTime(...);
		//
		// where the "..." represents whatever arguments are passed into the
		// Df250TriggerTime constructor.
		//
		#define makeallocator(A) template<typename... Args> \
		A* NEW_##A(Args&&... args){ \
			A* t = NULL; \
			if(v##A##_pool.empty()){ \
				t = new A(std::forward<Args>(args)...); \
			}else{ \
				t = v##A##_pool.back(); \
				v##A##_pool.pop_back(); \
				t->ClearAssociatedObjects(); \
				new(t) A(std::forward<Args>(args)...); \
			} \
			v##A.push_back(t); \
			return t; \
		}
		MyTypes(makeallocator);

		// Constructor and destructor
		DParsedEvent(uint64_t istreamorder):in_use(false),istreamorder(istreamorder){}
		#define printcounts(A) if(!v##A.empty()) cout << v##A.size() << " : " << #A << endl;
		#define printpoolcounts(A) if(!v##A##_pool.empty()) cout << v##A##_pool.size() << " : " << #A << "_pool" << endl;
		virtual ~DParsedEvent(){
//			cout << "----- DParsedEvent (" << this << ") -------" << endl;
//			MyTypes(printcounts);
//			MyTypes(printpoolcounts);
			Delete();
		}

	protected:
		map<JEventLoop*, DFactoryPointers> factory_pointers;


};

#undef MyTypes

#endif // _DParsedEvent_

