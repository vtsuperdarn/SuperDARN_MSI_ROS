/* RadarGetCodeNum
   ===============
   Author: R.J.Barnes */


#include <stdio.h>
#include <stdlib.h>

#include "rtypes.h"
#include "radar.h"

struct RadarNetwork *network; 

int main(int argc,char *argv[]) {
  char *envstr;
  FILE *fp;
  int codenum=0;
  char *code=NULL;
  int st,c;

  envstr=getenv("SD_RADAR");
  if (envstr==NULL) {
    fprintf(stderr,"Environment variable 'SD_RADAR' must be defined.\n");
    exit(-1);
  }

  fp=fopen(envstr,"r");

  if (fp==NULL) {
    fprintf(stderr,"Could not locate radar information file.\n");
    exit(-1);
  }

  network=RadarLoad(fp);
  fclose(fp); 
  if (network==NULL) {
    fprintf(stderr,"Failed to read radar information.\n");
    exit(-1);
  }
  
  st=atoi(argv[argc-1]);

  codenum=RadarGetCodeNum(network,st);

  fprintf(stdout,"RadarGetCodeNum\n"); 
  for (c=0;c<codenum;c++) {
    code=RadarGetCode(network,st,c);
    fprintf(stdout,"%s\n",code);
  }

  return 0;
}
