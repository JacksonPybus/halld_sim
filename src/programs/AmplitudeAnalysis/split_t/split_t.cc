#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <utility>
#include <iostream>

#include "AMPTOOLS_DATAIO/ROOTDataReader.h"
#include "AMPTOOLS_DATAIO/ROOTDataWriter.h"

#include "TLorentzVector.h"

#include "TH1F.h"
using namespace std;

#define DEFTREENAME "kin"

void Usage()
{
  cout << "Usage:\n  split_t <infile> <outputBase> <lowT> <highT> <nBins> [maxEvents]\n";
  cout << "  split_mass <infile> <outputBase> <lowT> <highT> <nBins> -T [tree name]\n\n";
  cout << "   overwrites the default ROOT tree name (\"kin\") in output and/or input files\n";
  cout << "   To specify input and output names delimit with \':\' ex. -T inKin:outKin\n";
  cout << "   Use -t to update existing files with new tree, instead of overwritting.\n";
  exit(1);
}


pair <string,string> GetTreeNames(char* treeArg)
{
  pair <string,string> treeNames(DEFTREENAME,"");
  string treeArgStr(treeArg);
  size_t delimPos=treeArgStr.find(':',1);

  if (delimPos != string::npos){
    treeNames.first=treeArgStr.substr(0,delimPos);
    treeNames.second=treeArgStr.substr(delimPos+1);
  }else
    treeNames.second=treeArgStr;

  return treeNames;
}


int main( int argc, char* argv[] ){
  
  unsigned int maxEvents = 4294967000; //close to 4byte int range
  
  //string treeName( "kin" );
  pair <string,string> treeNames(DEFTREENAME,DEFTREENAME);

  bool recreate=true;

  if( argc < 6 ) Usage();
  
  string outBase( argv[2] );
  
  double lowT = atof( argv[3] );
  double highT = atof( argv[4] );
  int numBins = atoi( argv[5] );
  
  // A somewhat convoluted way to allow tree name specification
  // via "-t [name]" in the arg. list after the standard args
  if( argc > 6 ) {
    for(int i=6; i<=7 && i<argc ; ++i){
      string arg=argv[i];
      if (arg == "-t"){
	if ((i+1 == argc) || (argv[i+1][0] == '-')) Usage();
	else{
	  treeNames = GetTreeNames(argv[++i]);
	  recreate=false;
	}
      }else if (arg == "-T"){
	if ((i+1 == argc) || (argv[i+1][0] == '-')) Usage();
	else{
	  treeNames = GetTreeNames(argv[++i]);
	  recreate=true;
	}
      }else
	if(i==6) maxEvents = atoi( arg.c_str() );
	else Usage();
    }
  }

  
  vector< string > dataReaderArgs;
  dataReaderArgs.push_back( argv[1] );
  dataReaderArgs.push_back( treeNames.first );
  
  // open reader
  ROOTDataReader in( dataReaderArgs );
  
  enum { kMaxBins = 1000 };
  assert( numBins < kMaxBins );
  
  double step = ( highT - lowT ) / numBins;
  
  ROOTDataWriter* outFile[kMaxBins];
  
  for( int i = 0; i < numBins; ++i ){
    
    ostringstream outName;
    outName << outBase << "_" << i << ".root";
    outFile[i] = new ROOTDataWriter( outName.str(),
				     treeNames.second.c_str(), 
				     recreate, in.hasWeight());
  }
  
  unsigned int eventCount = 0;
  
  Kinematics* event;
  while( ( event = in.getEvent() ) != NULL && eventCount++ < maxEvents ){
    
    vector< TLorentzVector > fs = event->particleList();
    
    // the second entry in this list is the recoil
    TLorentzVector Target(0,0,0,0.938272046);
    TLorentzVector Recoil(fs[1]);

    double t = -1 * (Recoil - Target).M2();
    
    int bin = static_cast< int >( floor( ( t - lowT ) / step ) );
    if( ( bin < numBins ) && ( bin >= 0 ) ){
      
      outFile[bin]->writeEvent( *event );
      delete event;
    }
  }
  
  for( int i = 0; i < numBins; ++i ){
    
    delete outFile[i];
  }
  
  return 0;
}
