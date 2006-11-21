/*
 * hitFTOF - registers hits for forward Time-Of-Flight
 *
 *	This is a part of the hits package for the
 *	HDGeant simulation program for Hall D.
 *
 *	version 1.0 	-Richard Jones July 16, 2001
 *
 * Programmer's Notes:
 * -------------------
 * 1) In applying the attenuation to light propagating down to both ends
 *    of the counters, there has to be some point where the attenuation
 *    factor is 1.  I chose it to be the midplane, so that in the middle
 *    of the counter both ends see the unattenuated dE values.  Closer to
 *    either end, that end has a larger dE value and the opposite end a
 *    lower dE value than the actual deposition.
 * 2) In applying the propagation delay to light propagating down to the
 *    ends of the counters, there has to be some point where the timing
 *    offset is 0.  I chose it to be the midplane, so that for hits in
 *    the middle of the counter the t values measure time-of-flight from
 *    the t=0 of the event.  For hits closer to one end, that end sees
 *    a t value smaller than its true time-of-flight, and the other end
 *    sees a value correspondingly larger.  The average is the true tof.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <hddm_s.h>
#include <geant3.h>
#include <bintree.h>

#define ATTEN_LENGTH	150
#define C_EFFECTIVE	15
#define TWO_HIT_RESOL   25.
#define MAX_HITS        100
#define THRESH_MEV      0.8

binTree_t* forwardTOFTree = 0;
static int counterCount = 0;
static int pointCount = 0;


/* register hits during tracking (from gustep) */

void hitForwardTOF (float xin[4], float xout[4],
                    float pin[5], float pout[5], float dEsum,
                    int track, int stack, int history)
{
   float x[3], t;
   float dx[3], dr;
   float dEdx;
   float xlocal[3];
   float xftof[3];
   float zeroHat[] = {0,0,0};

   x[0] = (xin[0] + xout[0])/2;
   x[1] = (xin[1] + xout[1])/2;
   x[2] = (xin[2] + xout[2])/2;
   t    = (xin[3] + xout[3])/2 * 1e9;
   transformCoord(x,"global",xlocal,"FTOF");
   transformCoord(zeroHat,"local",xftof,"FTOF");
   dx[0] = xin[0] - xout[0];
   dx[1] = xin[1] - xout[1];
   dx[2] = xin[2] - xout[2];
   dr = sqrt(dx[0]*dx[0] + dx[1]*dx[1] + dx[2]*dx[2]);
   if (dr > 1e-3)
   {
      dEdx = dEsum/dr;
   }
   else
   {
      dEdx = 0;
   }

   /* post the hit to the truth tree */

   if (history == 0)
   {
      int mark = (1<<30) + pointCount;
      void** twig = getTwig(&forwardTOFTree, mark);
      if (*twig == 0)
      {
         s_ForwardTOF_t* tof = *twig = make_s_ForwardTOF();
         s_FtofTruthPoints_t* points = make_s_FtofTruthPoints(1);
         tof->ftofTruthPoints = points;
         points->in[0].primary = (stack == 0);
         points->in[0].track = track;
         points->in[0].x = x[0];
         points->in[0].y = x[1];
         points->in[0].z = x[2];
         points->in[0].t = t;
         points->mult = 1;
         pointCount++;
      }
   }

   /* post the hit to the hits tree, mark slab as hit */

   if (dEsum > 0)
   {
      int nhit;
      s_FtofLeftHits_t* leftHits;
      s_FtofRightHits_t* rightHits;
      int row = getrow_();
      int plane = getplane_();
      int column = getcolumn_();
      float dxleft = xlocal[0];
      float dxright = -xlocal[0];
      float tleft  = (column == 2) ? 0 : t + dxleft/C_EFFECTIVE;
      float tright = (column == 1) ? 0 : t + dxright/C_EFFECTIVE;
      float dEleft  = (column == 2) ? 0 : dEsum * exp(-dxleft/ATTEN_LENGTH);
      float dEright = (column == 1) ? 0 : dEsum * exp(-dxright/ATTEN_LENGTH);
      float ycenter = (fabs(xftof[1]) < 1e-4) ? 0 : xftof[1];
      int mark = (plane<<20) + (row<<10) + column;
      void** twig = getTwig(&forwardTOFTree, mark);
      if (*twig == 0)
      {
         s_ForwardTOF_t* tof = *twig = make_s_ForwardTOF();
         s_FtofCounters_t* counters = make_s_FtofCounters(1);
         counters->mult = 1;
         counters->in[0].plane = plane;
         counters->in[0].paddle = row;
         leftHits = HDDM_NULL;
         rightHits = HDDM_NULL;
         if (column == 0 || column == 1)
         {
           counters->in[0].ftofLeftHits = leftHits
                                        = make_s_FtofLeftHits(MAX_HITS);
         }
         if (column == 0 || column == 2)
         {
           counters->in[0].ftofRightHits = rightHits
                                         = make_s_FtofRightHits(MAX_HITS);
         }
         tof->ftofCounters = counters;
         counterCount++;
      }
      else
      {
         s_ForwardTOF_t* tof = *twig;
         leftHits = tof->ftofCounters->in[0].ftofLeftHits;
         rightHits = tof->ftofCounters->in[0].ftofRightHits;
      }

      if (leftHits != HDDM_NULL)
      {
         for (nhit = 0; nhit < leftHits->mult; nhit++)
         {
            if (fabs(leftHits->in[nhit].t - t) < TWO_HIT_RESOL)
            {
               break;
            }
         }
         if (nhit < leftHits->mult)         /* merge with former hit */
         {
            leftHits->in[nhit].t = 
               (leftHits->in[nhit].t * leftHits->in[nhit].dE + tleft * dEleft)
                / (leftHits->in[nhit].dE += dEleft);
         }
         else if (nhit < MAX_HITS)         /* create new hit */
         {
            leftHits->in[nhit].t = tleft;
            leftHits->in[nhit].dE = dEleft;
            leftHits->mult++;
         }
         else
         {
            fprintf(stderr,"HDGeant error in hitForwardTOF: ");
            fprintf(stderr,"max hit count %d exceeded, truncating!\n",MAX_HITS);
         }
      }

      if (rightHits != HDDM_NULL)
      {
         for (nhit = 0; nhit < rightHits->mult; nhit++)
         {
            if (fabs(rightHits->in[nhit].t - t) < TWO_HIT_RESOL)
            {
               break;
            }
         }
         if (nhit < rightHits->mult)         /* merge with former hit */
         {
            rightHits->in[nhit].t = 
             (rightHits->in[nhit].t * rightHits->in[nhit].dE + tright * dEright)
             / (rightHits->in[nhit].dE += dEright);
         }
         else if (nhit < MAX_HITS)         /* create new hit */
         {
            rightHits->in[nhit].t = tright;
            rightHits->in[nhit].dE = dEright;
            rightHits->mult++;
         }
         else
         {
            fprintf(stderr,"HDGeant error in hitForwardTOF: ");
            fprintf(stderr,"max hit count %d exceeded, truncating!\n",MAX_HITS);
         }
      }
   }
}

/* entry point from fortran */

void hitforwardtof_ (float* xin, float* xout,
                     float* pin, float* pout, float* dEsum,
                     int* track, int* stack, int* history)
{
   hitForwardTOF(xin,xout,pin,pout,*dEsum,*track,*stack,*history);
}


/* pick and package the hits for shipping */

s_ForwardTOF_t* pickForwardTOF ()
{
   s_ForwardTOF_t* box;
   s_ForwardTOF_t* item;

   if ((counterCount == 0) && (pointCount == 0))
   {
      return HDDM_NULL;
   }

   box = make_s_ForwardTOF();
   box->ftofCounters = make_s_FtofCounters(counterCount);
   box->ftofTruthPoints = make_s_FtofTruthPoints(pointCount);
   while (item = (s_ForwardTOF_t*) pickTwig(&forwardTOFTree))
   {
      s_FtofCounters_t* counters = item->ftofCounters;
      s_FtofTruthPoints_t* points = item->ftofTruthPoints;

      if (counters != HDDM_NULL)
      {
      /* compress out the hits below threshold */
         s_FtofLeftHits_t* leftHits = counters->in[0].ftofLeftHits;
         s_FtofRightHits_t* rightHits = counters->in[0].ftofRightHits;
         int iok,i;
         int mok=0;
         if (leftHits != HDDM_NULL)
         {
            for (iok=i=0; i < leftHits->mult; i++)
            {
               if (leftHits->in[i].dE >= THRESH_MEV/1e3)
               {
                  if (iok < i)
                  {
                     leftHits->in[iok] = leftHits->in[i];
                  }
                  ++mok;
                  ++iok;
               }
            }
            leftHits->mult = iok;
            if (iok == 0)
            {
               counters->in[0].ftofLeftHits = HDDM_NULL;
               FREE(leftHits);
            }
         }
         if (rightHits != HDDM_NULL) 
         {
            for (iok=i=0; i < rightHits->mult; i++)
            {
               if (rightHits->in[i].dE >= THRESH_MEV/1e3)
               {
                  if (iok < i)
                  {
                     rightHits->in[iok] = rightHits->in[i];
                  }
                  ++mok;
                  ++iok;
               }
            }
            rightHits->mult = iok;
            if (iok == 0)
            {
               counters->in[0].ftofRightHits = HDDM_NULL;
               FREE(rightHits);
            }
         }
         if (mok)
         {
            int m = box->ftofCounters->mult++;
            box->ftofCounters->in[m] = counters->in[0];
         }
         FREE(counters);
      }
      else if (points != HDDM_NULL)
      {
         int m = box->ftofTruthPoints->mult++;
         box->ftofTruthPoints->in[m] = points->in[0];
         FREE(points);
      }
      FREE(item);
   }

   counterCount = pointCount = 0;

   if ((box->ftofCounters != HDDM_NULL) &&
       (box->ftofCounters->mult == 0))
   {
      FREE(box->ftofCounters);
      box->ftofCounters = HDDM_NULL;
   }
   if ((box->ftofTruthPoints != HDDM_NULL) &&
       (box->ftofTruthPoints->mult == 0))
   {
      FREE(box->ftofTruthPoints);
      box->ftofTruthPoints = HDDM_NULL;
   }
   if ((box->ftofCounters->mult == 0) &&
       (box->ftofTruthPoints->mult == 0))
   {
      FREE(box);
      box = HDDM_NULL;
   }
   return box;
}
