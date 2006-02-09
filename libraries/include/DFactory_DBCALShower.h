// $Id$
//
//    File: DFactory_DBCALShower.h
// Created: Thu Nov 17 09:56:05 CST 2005
// Creator: gluexuser (on Linux hydra.phys.uregina.ca 2.4.20-8smp i686)
//

#ifndef _DFactory_DBCALShower_
#define _DFactory_DBCALShower_

#include "DFactory.h"
#include "DEventLoop.h"
#include "DBCALShower.h"

class DFactory_DBCALShower:public DFactory<DBCALShower>{
	public:
		DFactory_DBCALShower(){};
		~DFactory_DBCALShower(){};
		const string toString(void);


	private:
		derror_t evnt(DEventLoop *loop, int eventnumber);	///< Invoked via DEventProcessor virtual method
		void ClearEvent(DEventLoop *eventLoop);
                void CellRecon(DEventLoop *eventLoop);
                void CeleToArray(void);            
                void PreCluster(DEventLoop *eventLoop);
                void Connect(int,int);
                void ClusNorm(void);
                void ClusAnalysis();
                void Clus_Break(int nclust);

// The following functions are for track fitting (using layer by layer 
// method)

          void Trakfit(void);
          void Fit_ls();
          void Linefit(int ixyz,int mwt,float &a, float &b,float &siga,
               float &sigb,float &chi2,float &q);
          float Gammq(float a,float x);
          void Gser(float &gamser,float a,float x);
          void Gcf(float &gammcf,float a,float x);
          float Gammln(float xx_gln);


       const static int modulemax_bcal=48;
       const static int layermax_bcal=12;
       const static int colmax_bcal=4;
       const static int cellmax_bcal=modulemax_bcal*layermax_bcal*colmax_bcal;
       const static int clsmax_bcal =modulemax_bcal*layermax_bcal*colmax_bcal;


// the following data member are used bu function CellRecon()

       float  ecel_a[modulemax_bcal][layermax_bcal][colmax_bcal]; 
       float  ecel_b[modulemax_bcal][layermax_bcal][colmax_bcal]; 
       float  tcel_a[modulemax_bcal][layermax_bcal][colmax_bcal];   
       float  tcel_b[modulemax_bcal][layermax_bcal][colmax_bcal];
       float  xx[modulemax_bcal][layermax_bcal][colmax_bcal];   
       float  yy[modulemax_bcal][layermax_bcal][colmax_bcal];

       float  xcel[modulemax_bcal][layermax_bcal][colmax_bcal];  
       float  ycel[modulemax_bcal][layermax_bcal][colmax_bcal];   
       float  zcel[modulemax_bcal][layermax_bcal][colmax_bcal];   
       float  tcel[modulemax_bcal][layermax_bcal][colmax_bcal];   
       float  ecel[modulemax_bcal][layermax_bcal][colmax_bcal];
       float  tcell_anor[modulemax_bcal][layermax_bcal][colmax_bcal];   
       float  tcell_bnor[modulemax_bcal][layermax_bcal][colmax_bcal]; 
       float  ta_offset[modulemax_bcal][layermax_bcal][colmax_bcal];
       float  tb_offset[modulemax_bcal][layermax_bcal][colmax_bcal];   
       float  ta0;
       float  tb0;   
// The above data members are used by function CellRecon()


// The following data are used by function CeleToArray();
       const static float ethr_cell=0.00001;// MIN ENERGY THRESD OF cel in GeV
       int celtot;  //----- TOTAL NUMBER OF CELLS with readout --
       int narr[4][cellmax_bcal+1]; 
       int   nclus[cellmax_bcal+1];  
       int   next[cellmax_bcal+1];
       float e_cel[cellmax_bcal+1];
       float x_cel[cellmax_bcal+1];
       float y_cel[cellmax_bcal+1];
       float z_cel[cellmax_bcal+1];
       float t_cel[cellmax_bcal+1];
       float ta_cel[cellmax_bcal+1];
       float tb_cel[cellmax_bcal+1];    
       float  celdata[4][cellmax_bcal+1];
// The above data are used by function CeleToArray();

// The following data are used by function ClusNorm() and also fitting

       int clstot;  //----- TOTAL NUMBER OF CLUSTERS --

       float  e_cls[clsmax_bcal+1];  //----- TOTAL ENERGY OF CLUSTER -
       float  x_cls[clsmax_bcal+1];  //----- X CENTROID OF CLUSTER -
       float  y_cls[clsmax_bcal+1];  //----- Y CENTROID OF CLUSTER -
       float  z_cls[clsmax_bcal+1];  //----- Z CENTROID OF CLUSTER -
       float  t_cls[clsmax_bcal+1];  //---- <T> OF CLUSTER -
       float  ea_cls[clsmax_bcal+1]; //---- E SUM OF SIDE A OF CLUSTER -
       float  eb_cls[clsmax_bcal+1]; //---- E SUM OF SIDE B OF CLUSTER - 

       float  ta_cls[clsmax_bcal+1]; //--- <T> OF SIDE A OF CLUSTER -
       float  tb_cls[clsmax_bcal+1]; //--- <T> OF SIDE B OF CLUSTER -
       float  tsqr_a[clsmax_bcal+1]; //--- <T^2> OF SIDE A OF CLUSTER - 
       float  tsqr_b[clsmax_bcal+1]; //--- <T^2> OF SIDE B OF CLUSTER -
       float  trms_a[clsmax_bcal+1]; //--- T RMS OF SIDE A OF CLUSTER -
       float  trms_b[clsmax_bcal+1]; //--- T RMS OF SIDE B OF CLUSTER -   

       float  e2_a[clsmax_bcal+1];   //--<E^2> OF SIDE A OF CLUSTER -
       float  e2_b[clsmax_bcal+1];   //--<E^2> OF SIDE B OF CLUSTER -

       int clspoi[clsmax_bcal+1];  //---POINTER TO FIRST CELL OF CLUSTER CHAIN 
       int ncltot[clsmax_bcal+1];    //---- TOTAL NUMBER OF CELLS INCLUSTER -
       int ntopol[clsmax_bcal+1];


//********************for fitting****************************************
       int  nlrtot[clsmax_bcal+1]; //----total layers of the cluster

       const static int  elyr = 1;
       const static int  xlyr = 2; 
       const static int  ylyr = 3; 
       const static int  zlyr = 4; 
       const static int  tlyr = 5; 
       

       float clslyr[6][layermax_bcal+1][clsmax_bcal];
       float clslyr_ix[6][layermax_bcal+1];


       float  apx[4][clsmax_bcal+1];
       float  eapx[4][clsmax_bcal+1];
       float  ctrk[4][clsmax_bcal+1];
       float  ectrk[4][clsmax_bcal+1];

       float ctrk_ix[4];
       float ectrk_ix[4];
       float apx_ix[4];
       float eapx_ix[4];   

       float x[layermax_bcal+1];
       float y[layermax_bcal+1];  
       float z[layermax_bcal+1];
       float e[layermax_bcal+1];
       float rt[layermax_bcal+1];

       float sigx[layermax_bcal+1];  // cm
       float sigy[layermax_bcal+1];  // cm
       float sigz[layermax_bcal+1];  // cm
      
//************************************************************************









// some thresholds
 

//***threshold used in ClusAnalysis()

     const static float r_thr = 40.0;  // CENTROID DISTANCE THRESHOLD
     const static float t_thr = 2.5;   // CENTROID TIME THRESHOLD
     const static float z_thr = 30.0;    // FIBER DISTANCE THRESHOLD
     const static float rt_thr= 40.0; // CENTROID TRANSVERSE DISTANCE THRESHOLD
     const static float ecmin = 0.007;  // MIN ENERGY THRESD OF CLUSTER IN GEV
     const static float rmsmax= 5.0;    // T RMS THRESHOLD

//***the above threshold are used in ClusAnalysis()



// Constants for attenuation 
// attenuation factor =f_att*exp(-x/lmbda1)+(1.0-f_att)*exp(-x/lmbda2)
     const static float f_att= 1.0;
     const static float lmbda1=150.; // Attenuation lenth and other parameters
     const static float lmbda2=700.; // used in formula  attenuation factor
//   mcprod=0 for MC data and mcprod=1 for real data
     const static int  mcprod=0;

// some calibration constants
   const static float C_EFFECTIVE=15.0; //Effective v of light in scintillator
   const static float ECORR=0.13; // ECORR actually could be a function of E


};

#endif // _DFactory_DBCALShower_

