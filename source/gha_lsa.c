
/*
   Input vectors are sparse vectors, each line is a bag of words. Index/count pairs.
   First line is vocab size and number of docs.
   Training one eigenvector at a time. It settles, we freeze it and move on.
   Command-line-specified distance-based convergence criterion.
   Command-line option to read in the entire corpus and train from memory.
   Loops when it reaches the end. MAXITEMS specifies the maximum number of document
   vectors.

   Example corpus:
   3,2;
   0,1;1,1;2,1;
   1,3;2,1;

   Example usage: gha_lsa corpus.txt 100 0.0001 true vectors.txt 50
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

typedef struct wordbag{
  int* indices;
  float* counts;
  int vocabsize;
} Wordbag;

void initialise();
void orthogonalise();
double dot(double *fv, double *sv, int firstoffset, int secondoffset, int steps);
void printmatrix(double* m, int w, int l);
void printrow(double *m, int w, int rnum);
void readcorpus(FILE *fp);
void printcorpus();
Wordbag createwordbag(char *doc);
void trainfromarray(double ccrit, int items);
void trainfromfile(FILE *fp, double ccrit);
void step(Wordbag *w);
double checkconvergence(double criterion);
void printvectors(FILE *ofp);
Wordbag* scanitem(FILE *fp);
float compare();

double* evs;
double* vals;
double* storedpreviousvector;
double storedpreviousvectormagnitude=0;
int ntr=0, ttr=0;
int nsvecs = 0;
int dim, nrefvecs=0;
int currentv = 0;
double* H;
double mag;
int MAXITEMS=10000;
int PR;
Wordbag** corpus;
int items;
float* reference=NULL;

int main(int argc, char** argv)
{
  FILE *fp, *ofp, *rafp=NULL;
  char *filename, *outfile, *compfilename;
  char *pEnd;
  double ccrit = 0;
  int cachecorpus = 0, i, k, j;

  if(argc==7 || argc==8){
    filename = argv[1];
    fp = fopen(filename, "r");
    nsvecs = strtod(argv[2], &pEnd);
    ccrit = strtod(argv[3], &pEnd);
    if(strcmp(argv[4], "true")==0){
      cachecorpus=1;
    } else {
      cachecorpus=0;
    }
    outfile = argv[5];
    PR = strtod(argv[6], &pEnd);

    int ret=fscanf(fp, "%d,%d;\n", &dim, &items);

    if(argc==8){
      compfilename=argv[7];
      rafp=fopen(compfilename, "r");
      printf("Reading in the comparison vectors.\n");
      int nvecs, nitems;
      k=fscanf(rafp, "%d %d", &nrefvecs, &nitems);
      if(nsvecs<nrefvecs){
	nrefvecs=nsvecs;
      }
      if(nitems==dim){
	reference = (float *)malloc(nrefvecs*nitems*sizeof(float));
	for(i=0;i<nrefvecs;i++){
	  for(j=0;j<dim;j++){
	    k=fscanf(rafp, "%efg", &reference[i*dim+j]);
	  }
	  k=fscanf(rafp, "\n");
	}
      } else {
	printf("Dimensionality mismatch. Rejecting the reference set.\n");
      }
      fclose(rafp);
    }

    printf("Start: %d\n", clock());

    if(ret!=-1){
      if(items<=MAXITEMS && cachecorpus==1){
	initialise();
	readcorpus(fp);
	//printcorpus();
	trainfromarray(ccrit, items);
      } else if(cachecorpus==1 && items>MAXITEMS){
	printf("Warning! Too many items to cache. Training from file instead.");
	initialise(0);
	trainfromfile(fp, ccrit);
      } else {
	initialise(0);
	trainfromfile(fp, ccrit);
      }
    }
    printf("Finish: %d\n", clock());
    ofp = fopen(outfile, "w");
    printvectors(ofp);
    fclose(fp);
    fclose(ofp);
  }else{
    printf("Usage: gha_lsa <infile> <nvecs> <conv> <cache corpus, true/false> <outfile> <PR> <optional comparison set>\n");
  }
}

void printvectors(FILE *ofp){
  int i, j;

  for(i=0;i<nsvecs;i++){
    for(j=0;j<dim;j++){
      fprintf(ofp, "%f;", evs[i*dim+j]);
    }
    fprintf(ofp, "\n");
  }
}

void readcorpus(FILE *fp){
  int i;
  corpus = (Wordbag**)malloc(items*sizeof(Wordbag**));

  for(i=0;i<items;i++){
    Wordbag *w = scanitem(fp);
    corpus[i]=w;
  }
}

Wordbag* scanitem(FILE *fp){
  int i, ind, vocab, ret;
  float count;
  Wordbag* w;

  ret=fscanf(fp, "%d;", &vocab);
  int* indexarray = (int*)malloc(sizeof(int)*vocab);
  float* countarray = (float*)malloc(sizeof(float)*vocab);
  for(i=0;i<vocab;i++){
    ret=fscanf(fp, "%d,%f;", &ind, &count);
    indexarray[i]=ind;
    countarray[i]=count;
    //printf("%f\n", count);
  }
  if(ret!=-1){
    w = (Wordbag*)malloc(sizeof(Wordbag));
    w->indices=indexarray;
    w->counts=countarray;
    w->vocabsize=vocab;
    fscanf(fp, "\n");
  } else {
    w=NULL;
  }
  return w;
}

void printcorpus(){
  int i, j;
  for(i=0;i<items;i++){
    Wordbag* w = corpus[i];
    for(j=0;j<w->vocabsize;j++){
      printf("%d %d--", w->indices[j], w->counts[j]);
    }
    printf("\n");
  }
}

void trainfromarray(double ccrit, int items){
  int linecounter=0, printcounter=0;

  while(linecounter<items){
    step(corpus[linecounter]);
    printcounter++;
    if(printcounter>=PR){
      printcounter=0;
      double dist = checkconvergence(ccrit);
      if(dist<ccrit){
	orthogonalise();
	currentv++;
	int i;
	for(i=0;i<currentv;i++){
	  H[i]=dot(evs, evs, i*dim, currentv*dim, dim);
	}
	mag=1;
	ntr=0;
      }
    }
    if(currentv>=nsvecs){
      linecounter=items+2;
    } else {
      linecounter++;
      if(linecounter>=items){
	linecounter=0;
      }
    }
  }
}

void trainfromfile(FILE *fp, double ccrit){
  int linecounter=0, printcounter=0, dummy;
  Wordbag* w=scanitem(fp);

  while(w!=NULL){
    step(w);
    printcounter++;
    if(printcounter>=PR){
      printcounter=0;
      double dist = checkconvergence(ccrit);
      if(dist<ccrit){
	orthogonalise();
	currentv++;
	int i;
	for(i=0;i<currentv;i++){
	  H[i]=dot(evs, evs, i*dim, currentv*dim, dim);
	}
	mag=1;
	ntr=0;
      }
    }
    if(currentv>=nsvecs){
      w=NULL;
    } else {
      free(w);
      w=scanitem(fp);
      linecounter++;
      if(w==NULL){
	fseek(fp,0,SEEK_SET);
	fscanf(fp, "%d,%d,%d;\n", &dummy, &dummy, &dummy);
	w=scanitem(fp);
	linecounter=1;
      }
    }
  }
}

void initialise(){
  int i, j;
  double init = sqrt(1.0/dim);
  mag=1;
  ntr=0;

  vals = (double*)malloc(nsvecs*sizeof(double));
  H = (double*)malloc(nsvecs*sizeof(double));
  evs = (double*)malloc(dim*nsvecs*sizeof(double));
  storedpreviousvector = (double*)malloc(dim*sizeof(double));

  for(i=0;i<nsvecs;i++){
    for(j=0;j<dim;j++){
      evs[i*dim+j]=init;
    }
  }
}

/*
  "step" is the core of it. Implements one training iteration of GHA.
  This is a sparse implementation, so it isn't going to look like
  normal GHA.
*/

void step(Wordbag *w){
  int i, j;

  double h[nsvecs];
  for(i=0;i<currentv;i++){
    h[i]=0;
  }
  for(i=0;i<currentv;i++){
    for(j=0;j<w->vocabsize;j++){
      int index = w->indices[j];
      h[i]+=w->counts[j]*evs[i*dim+index];
    }
  }

  double gv=0;
  for(i=0;i<w->vocabsize;i++){
    int index = w->indices[i];
    gv += w->counts[i]*evs[currentv*dim+index];
  }
  double n = gv-dot(H, h, 0, 0, currentv);
  double Hmagsq = 0;
  for(i=0;i<currentv;i++){
    Hmagsq+=H[i]*H[i];
  }
  
  double a = n/(sqrt((mag*mag)-Hmagsq));
  
  for(i=0;i<w->vocabsize;i++){
    int index = w->indices[i];
    mag=sqrt((mag*mag)-(evs[currentv*dim+index]*evs[currentv*dim+index])
	     +((evs[currentv*dim+index]+(a*w->counts[i]))*(evs[currentv*dim+index]+(a*w->counts[i]))));
    evs[currentv*dim+index]+=(a*w->counts[i]);
  }
  
  for(i=0;i<currentv;i++){
    H[i]+=h[i]*a;
  }

  ntr++;
  ttr++;
}

/*
  Sparse implementation involves training pseudo-eigenvectors that aren't
  orthogonal to previous eigenvectors. Once they've settled, we have to
  orthogonalise to get the actual eigenvector.
*/

void orthogonalise(){
  int i, j;
  
  //Orthogonalise current sv
  double* north = (double *)malloc(dim*sizeof(double));
  for(i=0;i<dim;i++){
    north[i]=0;
  }
  for(i=0;i<currentv;i++){
    for(j=0;j<dim;j++){
      north[j]+=H[i]*evs[i*dim+j];
    }
  }
  if(currentv>0){
    for(i=0;i<dim;i++){
      evs[currentv*dim+i]=evs[currentv*dim+i]-north[i];
    }
  }
  
  //Calculate value, store
  mag=0;
  for(i=0;i<dim;i++){
    mag+=evs[currentv*dim+i]*evs[currentv*dim+i];
  }
  mag=sqrt(mag);
  vals[currentv]=mag/ntr;
  printf("Vector at %d converged at %d. Length %f, value %f, ntr %d\n", currentv, clock(), mag, mag/ntr, ntr);
  
  //Normalise sv
  for(i=0;i<dim;i++){
    evs[currentv*dim+i]=evs[currentv*dim+i]/mag;
  }
  
  storedpreviousvectormagnitude=0;
}

double checkconvergence(double criterion){

  //This is designed to be more efficient given usage than simply measuring the distance
  //between the ends of two vectors. The method is given the convergence criterion and if
  //the criterion is exceeded before all dimensions have been considered it simply
  //returns a ball park figure for the user's information rather than bothering to
  //calculate it accurately.
  
  int i=0;
  double t=0, u, csq=criterion*criterion;
  
  if(storedpreviousvectormagnitude!=0){
    while(i<dim && t<csq){
      u=(storedpreviousvector[i]/storedpreviousvectormagnitude)-(evs[currentv*dim+i]/mag);
      t+=u*u;
      i++;
    }
    if(i<dim && t>=csq){
      t=t*(dim/i);
    }
  } else {
    t=criterion+1;
  }
  storedpreviousvectormagnitude=mag;
  for(i=0;i<dim;i++){
    storedpreviousvector[i]=evs[currentv*dim+i];
  }

  //Comparing to reference slows it down, but it's only optional
  if(reference!=NULL && currentv<nrefvecs){
    float dot=compare();
    printf("cc: %f dp: %e tr: %d\n", sqrt(t), dot, ttr);
  } else {
    printf("Convergence criterion approximately %f\r", sqrt(t));
  }

  return sqrt(t);
}

float compare(){
  int i, j;
  float mag=0;

  //Create an orthogonal version of the current vector
  float* north = (float *)malloc(dim*sizeof(float));
  for(i=0;i<dim;i++){
    north[i]=0;
  }

  //We don't want to affect the actual vectorset just to plot convergence,
  //so make this array to store the orthogonalised vector in.
  float* orth = (float *)malloc(dim*sizeof(float));
  for(i=0;i<dim;i++){
    orth[i]=0;
  }

  for(i=0;i<currentv;i++){
    for(j=0;j<dim;j++){
      north[j]+=H[i]*evs[i*dim+j];
    }
  }
  if(currentv>0){
    for(i=0;i<dim;i++){
      orth[i]=evs[currentv*dim+i]-north[i];
    }
  } else {
    for(i=0;i<dim;i++){
      orth[i]=evs[currentv*dim+i];
    }
  }
  
  //Calculate the magnitude of the orthogonal vector
  for(i=0;i<dim;i++){
    mag+=orth[i]*orth[i];
  }
  mag=sqrt(mag);

  //Normalise and dot with reference simultaneously
  float dp = 0;
  for(i=0;i<dim;i++){
    dp+=(orth[i]/mag)*reference[currentv*dim+i];
  }
  
  return sqrt(dp*dp);
}

double dot(double *fv, double *sv, int firstoffset, int secondoffset, int steps){
  int i;
  double t=0;
  for(i=0;i<steps;i++){
    t+=(fv[firstoffset+i]*sv[secondoffset+i]);
  }
  return t;
}

void printrow(double *m, int w, int rnum){
  int i;
  for(i=0;i<w;i++){
    printf("%f ", m[rnum*w+i]);
  }
}

void printmatrix(double* m, int w, int l){
  int i, j;
  for(i=0;i<l;i++){
    for(j=0;j<w;j++){
      printf("%f ", m[i*w+j]);
    }
    printf("\n");
  }
  printf("\n");
}
