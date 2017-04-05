
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

typedef struct word{
  char first;
  char second;
  char third;
  char rest[30];
  struct word *firstlevelsister;
  struct word *secondlevelsister;
  struct word *thirdlevelsister;
  struct word *fourthlevelsister;
  int index;
} Word;

typedef struct wordcount{
  int index;
  int count;
  struct wordcount *next;
} Wordcount;

int getindex(char w[30], Word* first);
Word* createword(char w[30]);
int getword(FILE* fp, Word *top);
void printlist(Word *top);
void printwordcountlist(Wordcount w[30]);

int nwords = 0;
int test = 0;
int maxvocab;
FILE *dictout;

int main(int argc, char** argv)
{
  FILE *flist;
  FILE *in;
  FILE *out;
  char *infile;
  char *outfile;
  char *dictoutfile;
  int indices[10];
  int i, j, k, ind, n=2;
  Word *top = (Word *)malloc(sizeof(Word));
  int dim = 0;
  char filename[200];
  int items=0, vocabsize=0;
  Wordcount *topword = (Wordcount *)malloc(sizeof(Wordcount));
  Wordcount *currentword;
  int wordcountsinexistence=1;

  topword->index=-1;
  topword->count=0;
  topword->next=NULL;

  if(argc==4){
    infile = argv[1];
    outfile = argv[2];
    dictoutfile = argv[3];
    flist = fopen(infile, "r");
    out = fopen(outfile, "w");
    dictout = fopen(dictoutfile, "w");

    if(flist!=NULL && out!=NULL && dictout!=NULL){
      fprintf(out, "                                                          \n");
    
      k=fscanf(flist, "%200s\n", filename);
      while(k!=EOF){
	printf("%d--%s\n", items, filename);
	in = fopen(filename, "r");
	if(in!=NULL){
	  items++;
	  ind=getword(in, top);
	  while(ind!=-1){
	    if(ind>=dim){
	      dim=ind+1;
	    }
	    currentword = topword;
	    while(currentword->index!=ind){
	      if(currentword->next==NULL){
		Wordcount *newword = (Wordcount *)malloc(sizeof(Wordcount));
		wordcountsinexistence++;
		newword->index = ind;
		newword->count=0;
		newword->next=NULL;
		currentword->next = newword;
		vocabsize++;
	      }
	      currentword = currentword->next;
	    }
	    currentword->count++;
	    ind=getword(in, top);
	  }

	  fclose(in);
	  currentword = topword->next;
	  Wordcount *tmp;
	  fprintf(out, "%d;", vocabsize);
	  while(currentword!=NULL){
	    fprintf(out, "%d,%d;", currentword->index, currentword->count);
	    tmp = currentword->next;
	    free(currentword);
	    wordcountsinexistence--;
	    currentword=tmp;
	  }
	  topword->next=NULL;
	  if(wordcountsinexistence!=1){
	    printf("There are oddly %d word counts in existence.\n", wordcountsinexistence);
	  }
	  if(vocabsize>maxvocab){
	    maxvocab=vocabsize;
	  }
	  vocabsize=0;
	  fprintf(out, "\n");
	} else {
	  printf("Failed to open file; %s\n", filename);
	}
	k=fscanf(flist, "%200s\n", filename);
      }

      fseek(out, 0, SEEK_SET);
      fprintf(out, "%d,%d;", dim, items);
      fclose(out);
      fclose(flist);
    } else {
      printf("File does not exist.\n");
    }
  }else{
    printf("Usage: process_wordbag_corpus <filelist> <outfile> <dictionary outfile>\n");
  }

  //printlist(top);
}

void printwordcountlist(Wordcount w[30]){
  Wordcount *ptr=w;
  while(ptr!=NULL){
    printf("%d\n", ptr->index);
    ptr=ptr->next;
  }
}

int getword(FILE* fp, Word *top){
  char w[30];
  int k, i, ind;

  i=fscanf(fp, "%29s", w);
  w[29]='\0';

  if(i==EOF){
    return -1;
  } else {
    test=0;
    ind = getindex(w, top);
    //printf("Searched through %d to find %s\n", test, w);
    //printf("Word is %s, index is %d\n", w, ind);
    return ind;
  }
}

int getindex(char w[30], Word* top){
  Word *thisword = top;

  //printf("Word is %s, length is %d\n", w, strlen(w));
  
  while(thisword->firstlevelsister!=NULL && w[0]!=thisword->first){
    test++;
    thisword=thisword->firstlevelsister;
  }
  if(thisword->firstlevelsister==NULL && w[0]!=thisword->first){
    Word *newword = createword(w);
    thisword->firstlevelsister=newword;
    return newword->index;
  } else {
    while(thisword->secondlevelsister!=NULL && w[1]!=thisword->second){
      test++;
      thisword=thisword->secondlevelsister;
    }
    if(thisword->secondlevelsister==NULL && w[1]!=thisword->second){
      Word *newword = createword(w);
      thisword->secondlevelsister=newword;
      return newword->index;
    } else {
      while(thisword->thirdlevelsister!=NULL && w[strlen(w)-1]!=thisword->third){
	test++;
	thisword=thisword->thirdlevelsister;
      }
      if(thisword->thirdlevelsister==NULL && w[strlen(w)-1]!=thisword->third){
	Word *newword = createword(w);
	thisword->thirdlevelsister=newword;
	return newword->index;
      } else {
	while(thisword->fourthlevelsister!=NULL && strcmp(w,thisword->rest)!=0){
	  test++;
	  thisword=thisword->fourthlevelsister;
	}
	if(thisword->fourthlevelsister==NULL && strcmp(w,thisword->rest)!=0){
	  Word *newword = createword(w);
	  thisword->fourthlevelsister=newword;
	  return newword->index;
	} else {
	  return thisword->index;
	}
      }
    }
  }
}

Word* createword(char w[30]){
  int i;
  Word *newword = (Word *)malloc(sizeof(Word));
  
  newword->first=w[0];
  newword->second=w[1];
  newword->third=w[strlen(w)-1];
  //w+=3;
  for(i=0;i<29;i++){
    newword->rest[i]=w[i];
  }
  newword->rest[29]='\0';
  //strcpy(newword->rest,w);
  newword->index=nwords;
  newword->firstlevelsister=NULL;
  newword->secondlevelsister=NULL;
  newword->thirdlevelsister=NULL;
  newword->fourthlevelsister=NULL;
  fprintf(dictout, "%d\t>>%s<<\n", nwords, w);
  nwords++;
  return newword;
}
