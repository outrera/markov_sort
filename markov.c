#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define IN_SIZE 2
#define OUT_SIZE 1
#define NCHAINS (IN_SIZE*255)
#define BITS 6
#define TRNS (1<<BITS)
#define IN_SMAX 127   /* maximum value of a byte in Is */
#define OUT_SMAX 255  /* maximum value of a byte in Os */
#define REFRAIN -1


typedef struct chain {
  int ILs[BITS]; // locations of input sampled values
  int OLs[BITS]; // locations of input sampled values
  int IRs[BITS]; // input responsiveness (amplitude cutoff)
  int ORs[BITS]; // output responsiveness (amplitude cutoff)
  int Ss[TRNS][TRNS]; // statistics
  int Ts[TRNS]; // transitions
} chain;

chain Cs[NCHAINS];
uint8_t Is[IN_SIZE]; // input state
uint8_t Os[OUT_SIZE]; // output state
int Vs[OUT_SIZE][2]; // vote space
// NOTE: every Vs cell gets around NCHAINS*BITS/OUT_SIZE votes.

int cmp(const void *a, const void *b) {
  return (int)*(uint8_t*)a - (int)*(uint8_t*)b;
}

void makeExample() {
  int I = 0;
  memcpy(Os, Is, MIN(IN_SIZE, OUT_SIZE));
  heapsort(Os, OUT_SIZE, 1, cmp);
  //for (I = 0; I < OUT_SIZE; I++) Os[I] /= 2;
  Os[0] = Is[0] + Is[1];
}

void generateInput() {
  int I;
  for (I = 0; I < IN_SIZE; I++) Is[I] = (uint8_t)rnd(IN_SMAX+1);
}

int rnd(int N) {
  return round((double)rand()/RAND_MAX*(N-1));
}

uint8_t gatherBits(uint8_t *Bs, int *Ls, int *Rs) {
  int I;
  uint8_t R = 0;
  for (I = 0; I < BITS; I++) if (Rs[I]<Bs[Ls[I]]) R |= 1<<I;
  return R;
}

void chainInit(chain *C) {
  int I;
  for (I = 0; I < BITS; I++) {
    C->ILs[I] = rnd(IN_SIZE);
    C->OLs[I] = rnd(OUT_SIZE);
    C->IRs[I] = rnd(IN_SMAX-1);
    C->ORs[I] = rnd(OUT_SMAX-1);
    //C->OLs[I] = C->ILs[I];
    //C->ORs[I] = C->IRs[I];
  }
}

void initChains() {
  int I;
  for (I = 0; I < NCHAINS; I++) chainInit(Cs+I);
}

void chainTrain(chain *C) {
  int In = gatherBits(Is, C->ILs, C->IRs);
  int Out = gatherBits(Os, C->OLs, C->ORs);
  C->Ss[In][Out]++;
}

// FIXME: if all futures are equally probable, then pick random one
void chainFinishTraining(chain *C) {
  int I, J;
  for (I = 0; I < TRNS; I++) {
    int BestCount = 0;
    int BestTransition = 0;
    for (J = 0; J < TRNS; J++) {
      if (C->Ss[I][J] > BestCount) {
        BestCount = C->Ss[I][J];
        BestTransition = J;
      }
    }
    if (!BestCount) BestTransition = REFRAIN;
    C->Ts[I] = BestTransition;
  }
}

void trainChains(int Examples) {
  int I;
  while (Examples-- > 0) {
    switch (rnd(8)) { //degenerate cases
    case 0: memset(Is, 0, IN_SIZE); break;
    case 1: memset(Is, 0xFF, IN_SIZE); break;
    default: generateInput();
    }
    makeExample();
    for (I = 0; I < NCHAINS; I++) chainTrain(Cs+I);
  }
  for (I = 0; I < NCHAINS; I++) chainFinishTraining(Cs+I);
}

void chainVote(chain *C) {
  int I;
  int In = gatherBits(Is, C->ILs, C->IRs);
  int Out = C->Ts[In];
  if (Out == REFRAIN) return;
  for (I = 0; I < BITS; I++) {
    Vs[C->OLs[I]][0] += (Out&(1<<I)) ? 1 : 0;
    Vs[C->OLs[I]][1]++;
  }
}

void markovSort() {
  int I;
  memset(Vs, 0, OUT_SIZE*2*sizeof(int));
  memset(Os, 0, OUT_SIZE);
  for (I = 0; I < NCHAINS; I++) chainVote(Cs+I);
  for (I = 0; I < OUT_SIZE; I++) {
    //printf("%d: %d/%d\n", I, Vs[I][0], Vs[I][1]);
    Os[I] = (uint8_t)round((double)Vs[I][0]/Vs[I][1]*OUT_SMAX);
  }
}

void printArray(int N, char *Info, uint8_t *Xs) {
  printf("%s", Info);
  int I;
  for (I = 0; I < N; I++) {
    if (I != 0) printf(",");
    printf("%03d", Xs[I]);
  }
  printf("\n");
}

int main() {
  int I = 0;
  srand((unsigned)time(0));

  printf("Training...\n");
  initChains();
  trainChains(10000);

  for (I = 0; I < 40; I++) {
    generateInput();
    //memset(Is, 0, IN_SIZE);
    //memset(Is, SMAX, IN_SIZE);
    printArray(IN_SIZE , "Input: ", Is);

    markovSort();
    printArray(OUT_SIZE, "Guess: ", Os);

    makeExample();
    printArray(OUT_SIZE, "Answer:", Os);
  }
  return 0;
}

