/* stepcamp.c
   ============
   Author: R.J.Barnes, J.Spaleta, & K.T. Sterne
*/

/*
 LICENSE AND DISCLAIMER

 Copyright (c) 2012 The Johns Hopkins University/Applied Physics Laboratory

 This file is part of the Radar Software Toolkit (RST).

 RST is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 any later version.

 RST is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with RST.  If not, see <http://www.gnu.org/licenses/>.



*/


#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <zlib.h>
#include "rtypes.h"
#include "option.h"
#include "rtime.h"
#include "dmap.h"
#include "limit.h"
#include "radar.h"
#include "rprm.h"
#include "iq.h"
#include "rawdata.h"
#include "fitblk.h"
#include "fitdata.h"
#include "fitacf.h"


#include "errlog.h"
#include "freq.h"
#include "tcpipmsg.h"

#include "rmsg.h"
#include "rmsgsnd.h"

#include "radarshell.h"

#include "build.h"
#include "global.h"
#include "reopen.h"
#include "setup.h"
#include "sync.h"

#include "site.h"
#include "sitebuild.h"
#include "siteglobal.h"
#include "rosmsg.h"
#include "tsg.h"

char *ststr=NULL;
char *dfststr="tst";

void *tmpbuf;
size_t tmpsze;

char progid[80]={"stepcamp"};
char progname[256];

int arg=0;
struct OptionData opt;

char *roshost=NULL;
char *droshost={"127.0.0.1"};

int baseport=44100;

struct TCPIPMsgHost errlog={"127.0.0.1",44100,-1};

struct TCPIPMsgHost shell={"127.0.0.1",44101,-1};

struct TCPIPMsgHost freqcoord={"127.0.0.1",44110,-1};


int tnum=4;
struct TCPIPMsgHost task[4]={
  {"127.0.0.1",1,-1}, /* iqwrite */
  {"127.0.0.1",2,-1}, /* rawacfwrite */
  {"127.0.0.1",3,-1}, /* fitacfwrite */
  {"127.0.0.1",4,-1}  /* rtserver */
};

int main(int argc,char *argv[]) {

  int ptab[8] = {0,14,22,24,27,31,42,43};

  int lags[LAG_SIZE][2] = {
    { 0, 0},		/*  0 */
    {42,43},		/*  1 */
    {22,24},		/*  2 */
    {24,27},		/*  3 */
    {27,31},		/*  4 */
    {22,27},		/*  5 */

    {24,31},		/*  7 */
    {14,22},		/*  8 */
    {22,31},		/*  9 */
    {14,24},		/* 10 */
    {31,42},		/* 11 */
    {31,43},		/* 12 */
    {14,27},		/* 13 */
    { 0,14},		/* 14 */
    {27,42},		/* 15 */
    {27,43},		/* 16 */
    {14,31},		/* 17 */
    {24,42},		/* 18 */
    {24,43},		/* 19 */
    {22,42},		/* 20 */
    {22,43},		/* 21 */
    { 0,22},		/* 22 */

    { 0,24},		/* 24 */

    {43,43}};		/* alternate lag-0  */

    char logtxt[1024];

  int exitpoll=0;
  int scannowait=0;

  int scnsc=60;
  int scnus=0;
  int skip;
  int cnt=0;

  unsigned char discretion=0;

  int status=0,n;

  int beams=0;
  int total_scan_usecs=0;
  int total_integration_usecs=0;
  int fixfrq=-1;
  /* Flag to override auto-calc of integration time */
  int setintt=0;

  /* Flag and variables to better sync beam soundings */
  int bm_sync=0;
  int bmsc=6;
  int bmus=0;

  int camp_count=0;  /* Counting variable for camp count */
  int camp_num=10;   /* Number of times to camp */

  int sock;
  int freqport=0,arg=0;
  socklen_t clength;
  struct sockaddr_in server;
  struct sockaddr_in client;
  int msgsock=0;

  printf("Size of int %d\n",(int)sizeof(int));
  printf("Size of long %d\n",(int)sizeof(long));
  printf("Size of long long %d\n",(int)sizeof(long long));
  printf("Size of struct TRTimes %d\n",(int)sizeof(struct TRTimes));
  printf("Size of struct SeqPRM %d\n",(int)sizeof(struct SeqPRM));
  printf("Size of struct RosData %d\n",(int)sizeof(struct RosData));
  printf("Size of struct DataPRM %d\n",(int)sizeof(struct DataPRM));
  printf("Size of Struct ControlPRM  %d\n",(int)sizeof(struct ControlPRM));
  printf("Size of Struct RadarPRM  %d\n",(int)sizeof(struct RadarPRM));
  printf("Size of Struct ROSMsg  %d\n",(int)sizeof(struct ROSMsg));
  printf("Size of Struct CLRFreq  %d\n",(int)sizeof(struct CLRFreqPRM));
  printf("Size of Struct TSGprm  %d\n",(int)sizeof(struct TSGprm));
  printf("Size of Struct SiteSettings  %d\n",(int)sizeof(struct SiteSettings));

  cp=1241;
  intsc=6; /* Set default integration time for stepcamp (slow) */
  intus=0; /* Integration time is dynamically calculated below based on the numbuer of beams. */
  mppul=8;
  mplgs=23;
  mpinc=1500;
  dmpinc=1500;
  nrang=100;
  rsep=45;
  txpl=300;

  /* ========= PROCESS COMMAND LINE ARGUMENTS ============= */

  OptionAdd(&opt,"di",'x',&discretion);

  OptionAdd(&opt,"frang",'i',&frang);
  OptionAdd(&opt,"rsep",'i',&rsep);
  OptionAdd(&opt,"nrang",'i',&nrang);

  OptionAdd( &opt, "dt", 'i', &day);
  OptionAdd( &opt, "nt", 'i', &night);
  OptionAdd( &opt, "df", 'i', &dfrq);
  OptionAdd( &opt, "nf", 'i', &nfrq);
  OptionAdd( &opt, "fixfrq", 'i', &fixfrq);
  OptionAdd( &opt, "xcf", 'i', &xcnt);

  OptionAdd(&opt,"ep",'i',&errlog.port);
  OptionAdd(&opt,"sp",'i',&shell.port);

  OptionAdd(&opt,"bp",'i',&baseport);

  OptionAdd(&opt,"ros",'t',&roshost);

  OptionAdd(&opt,"stid",'t',&ststr);

  OptionAdd( &opt, "nowait", 'x', &scannowait);
  OptionAdd(&opt,"sb",'i',&sbm);
  OptionAdd(&opt,"eb",'i',&ebm);
  OptionAdd(&opt,"c",'i',&cnum);  /* Unsure of where this comes in as not used elsewhere */

  OptionAdd(&opt,"intsc",'i',&intsc);
  OptionAdd(&opt,"intus",'i',&intus);
  OptionAdd(&opt,"setintt",'x',&setintt);

  OptionAdd(&opt,"bm_sync",'x',&bm_sync);
  OptionAdd(&opt,"bmsc",'i',&bmsc);
  OptionAdd(&opt,"bmus",'i',&bmus);

  OptionAdd(&opt,"freqport",'i',&freqport);

  OptionAdd(&opt,"camp_num",'i',&camp_num);

  arg=OptionProcess(1,argc,argv,&opt,NULL);

  if (ststr==NULL) ststr=dfststr;

  if (roshost==NULL) roshost=getenv("ROSHOST");
  if (roshost==NULL) roshost=droshost;

  if ((errlog.sock=TCPIPMsgOpen(errlog.host,errlog.port))==-1) {
    fprintf(stderr,"Error connecting to error log.\n");
  }

  if ((shell.sock=TCPIPMsgOpen(shell.host,shell.port))==-1) {
    fprintf(stderr,"Error connecting to shell.\n");
  }

  if ((freqcoord.sock=TCPIPMsgOpen(freqcoord.host,freqport))==-1 && freqport!=0) {
    fprintf(stderr,"Error connecting to frequency coord.\n");
  }

  for (n=0;n<tnum;n++) task[n].port+=baseport;

  OpsStart(ststr);

  status=SiteBuild(ststr,NULL); /* second argument is version string */

  if (status==-1) {
    fprintf(stderr,"Could not identify station.\n");
    exit(1);
  }

  SiteStart(roshost);
  arg=OptionProcess(1,argc,argv,&opt,NULL);

  printf("Station ID: %s  %d\n",ststr,stid);

  /* Only do this if wanting to broadcast frequency */
/*  if (freqport !=0) { */
    /* Setting up freq coord socket */
/*    sock=socket(AF_INET,SOCK_STREAM,0);
    if (sock<0) {
        fprintf(stderr,"opening stream socket\n");
        exit(1);
    }

    server.sin_family=AF_INET;
    server.sin_addr.s_addr=INADDR_ANY;
    if (freqport !=0) server.sin_port=htons(freqport);
    else server.sin_port=0;

    if (bind(sock,(struct sockaddr *) &server,sizeof(server))){
        fprintf(stderr,"binding stream socket\n");
        exit(1);
    }

    listen(sock,5);

    fprintf(stderr,"Accepting a new connection....\n");
    clength=sizeof(client);
    msgsock=accept(sock,(struct sockaddr *) &client, &clength);
    */
    /* Use ROS functions? */
/*    msgsock=TCPIPMsgOpen(freqcoord,freqport);
    if (msgsock==-1) {
        fprintf(stderr,"Error attaching to 127.0.0.1:%d",freqport);
    } else {
        fprintf(stderr,"Attached to 127.0.0.1:%d",freqport);
    }



  }*/


  strncpy(combf,progid,80);

  OpsSetupCommand(argc,argv);
  OpsSetupShell();

  RadarShellParse(&rstable,"sbm l ebm l dfrq l nfrq l dfrang l nfrang l dmpinc l nmpinc l frqrng l xcnt l intsc l intus l",
                  &sbm,&ebm,
                  &dfrq,&nfrq,
                  &dfrang,&nfrang,
                  &dmpinc,&nmpinc,
                  &frqrng,&xcnt,&intsc,&intus);


  status=SiteSetupRadar();

  printf("Initial Setup Complete: Station ID: %s  %d\n",ststr,stid);
  if (status !=0) {
    ErrLog(errlog.sock,progname,"Error locating hardware.");
    exit (1);
  }

  beams=abs(ebm-sbm)+1;

  if (discretion) cp= -cp;

  txpl=(rsep*20)/3;

  sprintf(progname,"stepcamp");

  OpsLogStart(errlog.sock,progname,argc,argv);

  OpsSetupTask(tnum,task,errlog.sock,progname);

  for (n=0;n<tnum;n++) {
    RMsgSndReset(task[n].sock);
    RMsgSndOpen(task[n].sock,strlen( (char *) command),command);
  }

  printf("Preparing OpsFitACFStart Station ID: %s  %d\n",ststr,stid);
  OpsFitACFStart();

  printf("Preparing SiteTimeSeq Station ID: %s  %d\n",ststr,stid);
  tsgid=SiteTimeSeq(ptab);

  /* Using the OpsFindSkip to figure out what camp_count
     we should be using on initial startup */
  skip=OpsFindSkip(scnsc,scnus);
  camp_count=skip+1; /* To make sure we don't go over a scan boundary */

  /* Figure out where in our beam looping we are
     Pulls largely from OpsFindSkip, but can't use
     that due to non-sequential beam sounding */
  TimeReadClock(&yr,&mo,&dy,&hr,&mt,&sc,&us);
  if (backward){
      bmnum=sbm-(mt%beams);
  } else {
      bmnum=sbm+(mt%beams);
  }

  printf("Entering Scan loop Station ID: %s  %d\n",ststr,stid);
  do {
    printf("Entering Site Start Scan Station ID: %s  %d\n",ststr,stid);
    if (SiteStartScan() !=0) continue;

    if (OpsReOpen(2,0,0) !=0) {
      ErrLog(errlog.sock,progname,"Opening new files.");
      for (n=0;n<tnum;n++) {
        RMsgSndClose(task[n].sock);
        RMsgSndOpen(task[n].sock,strlen( (char *) command),command);
      }
    }

    scan=1;

    ErrLog(errlog.sock,progname,"Starting scan.");

    if (xcnt>0) {
      cnt++;
      if (cnt==xcnt) {
        xcf=1;
        cnt=0;
      } else xcf=0;
    } else xcf=0;

    do {

      TimeReadClock(&yr,&mo,&dy,&hr,&mt,&sc,&us);

      if (OpsDayNight()==1) {
        stfrq=dfrq;
        mpinc=dmpinc;
        frang=dfrang;
      } else {
        stfrq=nfrq;
        mpinc=nmpinc;
        frang=nfrang;
      }
      if(fixfrq>0) {
        stfrq=fixfrq;
        tfreq=fixfrq;
        noise=0;
      }
      sprintf(logtxt,"Integrating beam:%d intt:%ds.%dus (%d:%d:%d:%d)",bmnum,
                      intsc,intus,hr,mt,sc,us);
      ErrLog(errlog.sock,progname,logtxt);

      ErrLog(errlog.sock,progname,"Starting Integration.");

      printf("Entering Site Start Intt Station ID: %s  %d\n",ststr,stid);
      SiteStartIntt(intsc,intus);

      ErrLog(errlog.sock,progname,"Doing clear frequency search.");

      sprintf(logtxt, "FRQ: %d %d", stfrq, frqrng);
      ErrLog(errlog.sock,progname, logtxt);

      if(fixfrq<0) {
        tfreq=SiteFCLR(stfrq,stfrq+frqrng);
      }
      sprintf(logtxt,"Transmitting on: %d (Noise=%g)",tfreq,noise);
      ErrLog(errlog.sock,progname,logtxt);
      /* Place to actually send out transmit frequency */
      if (freqcoord.sock!=0) {
          TCPIPMsgSend(freqcoord.sock,&tfreq,sizeof(tfreq));
      }

      nave=SiteIntegrate(lags);
      if (nave<0) {
        sprintf(logtxt,"Integration error:%d",nave);
        ErrLog(errlog.sock,progname,logtxt);
        continue;
      }
      sprintf(logtxt,"Number of sequences: %d",nave);
      ErrLog(errlog.sock,progname,logtxt);

      OpsBuildPrm(prm,ptab,lags);

      OpsBuildIQ(iq,&badtr);

      OpsBuildRaw(raw);

      FitACF(prm,raw,fblk,fit);

      msg.num=0;
      msg.tsize=0;

      tmpbuf=RadarParmFlatten(prm,&tmpsze);
      RMsgSndAdd(&msg,tmpsze,tmpbuf,
		PRM_TYPE,0);

      tmpbuf=IQFlatten(iq,prm->nave,&tmpsze);
      RMsgSndAdd(&msg,tmpsze,tmpbuf,IQ_TYPE,0);

      RMsgSndAdd(&msg,sizeof(unsigned int)*2*iq->tbadtr,
                 (unsigned char *) badtr,BADTR_TYPE,0);

      RMsgSndAdd(&msg,strlen(sharedmemory)+1,(unsigned char *) sharedmemory,
		 IQS_TYPE,0);

      tmpbuf=RawFlatten(raw,prm->nrang,prm->mplgs,&tmpsze);
      RMsgSndAdd(&msg,tmpsze,tmpbuf,RAW_TYPE,0);

      tmpbuf=FitFlatten(fit,prm->nrang,&tmpsze);
      RMsgSndAdd(&msg,tmpsze,tmpbuf,FIT_TYPE,0);


      RMsgSndAdd(&msg,strlen(progname)+1,(unsigned char *) progname,
		NME_TYPE,0);



      for (n=0;n<tnum;n++) RMsgSndSend(task[n].sock,&msg);

      for (n=0;n<msg.num;n++) {
        if (msg.data[n].type==PRM_TYPE) free(msg.ptr[n]);
        if (msg.data[n].type==IQ_TYPE) free(msg.ptr[n]);
        if (msg.data[n].type==RAW_TYPE) free(msg.ptr[n]);
        if (msg.data[n].type==FIT_TYPE) free(msg.ptr[n]);
      }

      RadarShell(shell.sock,&rstable);

      if (exitpoll !=0) break;
      scan=0;
      camp_count++;
      if (camp_count==camp_num) break;

      if (bm_sync==1){
        ErrLog(errlog.sock,progname,"Syncing to beam timing");
        SiteEndScan(bmsc,bmus);
      }

    } while (1);

    /* Reset camping counter */
    camp_count=0;

    if (backward) bmnum--;
    else bmnum++;
    if (bmnum<ebm) bmnum=sbm;

    ErrLog(errlog.sock,progname,"Waiting for scan boundary.");
    if ((exitpoll==0) && (scannowait==0)) SiteEndScan(scnsc,scnus);
  } while (exitpoll==0);


  for (n=0;n<tnum;n++) RMsgSndClose(task[n].sock);
  if (msgsock!=0) {
    close(msgsock);
  }

  ErrLog(errlog.sock,progname,"Ending program.");


  SiteExit(0);

  return 0;
}

