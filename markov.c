#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#define SIZE 4
#define NCHAINS (1000*SIZE)
#define BITS 6
#define TRNS (1<<BITS)
#define SMAX 255

typedef struct chain {
  int Ls[BITS]; // locations of sampled values
  int Rs[BITS]; // responsiveness (amplitude cutoff)
  int Ss[TRNS][TRNS]; // statistics
  int Ts[TRNS]; // transitions
} chain;

chain Cs[NCHAINS];
uint8_t Is[SIZE]; // input state
uint8_t Os[SIZE]; // output state
int Vs[SIZE][2]; // vote space
// NOTE: every Vs cell gets around NCHAINS*BITS/SIZE votes.

int cmp(const void *a, const void *b) {
  return (int)*(uint8_t*)a - (int)*(uint8_t*)b;
}

void makeExample() {
  int I = 0;
  memcpy(Os, Is, SIZE);
  heapsort(Os, SIZE, 1, cmp);
  //for (I = 0; I < SIZE; I++) Os[I] /= 2;
  //Os[0] = Os[0] + Os[1];
  //Os[1] = 0;
}

void generateInput() {
  int I;
  for (I = 0; I < SIZE; I++) Is[I] = (uint8_t)rnd(SMAX+1);
  //Is[0] = rnd(SMAX/2);
  //Is[1] = rnd(SMAX/2);
}

int rnd(int N) {
  return round((double)rand()/RAND_MAX*(N-1));
}

uint8_t gatherBits(uint8_t *Bs, chain *C) {
  int I;
  uint8_t R = 0;
  for (I = 0; I < BITS; I++) if (C->Rs[I]<Bs[C->Ls[I]]) R |= 1<<I;
  return R;
}

void chainInit(chain *C) {
  int I;
  for (I = 0; I < BITS; I++) {
    C->Ls[I] = rnd(SIZE);
    C->Rs[I] = rnd(SMAX-1);
  }
}

void initChains() {
  int I;
  for (I = 0; I < NCHAINS; I++) chainInit(Cs+I);
}

void chainTrain(chain *C) {
  int In = gatherBits(Is, C);
  int Out = gatherBits(Os, C);
  C->Ss[In][Out]++;
}

// FIXME: if all futures are equally probable,
//        then just eliminate this transition
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
    if (!BestCount) BestTransition = -1;
    C->Ts[I] = BestTransition;
  }
}

void trainChains(int Examples) {
  int I;
  while (Examples-- > 0) {
    switch (rnd(8)) { //degenerate cases
    case 0: memset(Is, 0, SIZE); break;
    case 1: memset(Is, 0xFF, SIZE); break;
    default: generateInput();
    }
    makeExample();
    for (I = 0; I < NCHAINS; I++) chainTrain(Cs+I);
  }
  for (I = 0; I < NCHAINS; I++) chainFinishTraining(Cs+I);
}

void chainVote(chain *C) {
  int I;
  int In = gatherBits(Is, C);
  int Out = C->Ts[In];
  if (Out == -1) return;
  for (I = 0; I < BITS; I++) {
    Vs[C->Ls[I]][0] += (Out&(1<<I)) ? 1 : 0;
    Vs[C->Ls[I]][1]++;
  }
}

void markovSort() {
  int I;
  memset(Vs, 0, SIZE*2*sizeof(int));
  memset(Os, 0, SIZE);
  for (I = 0; I < NCHAINS; I++) chainVote(Cs+I);
  for (I = 0; I < SIZE; I++) {
    //printf("%d: %d/%d\n", I, Vs[I][0], Vs[I][1]);
    Os[I] = (uint8_t)round((double)Vs[I][0]/Vs[I][1]*SMAX);
  }
}

void printArray(char *Info, uint8_t *Xs) {
  printf("%s", Info);
  int I;
  for (I = 0; I < SIZE; I++) {
    if (I != 0) printf(",");
    printf("%03d", Xs[I]);
  }
  printf("\n");
}

int main() {
  srand((unsigned)time(0));

  printf("Training...\n");
  initChains();
  trainChains(NCHAINS/2);

  generateInput();
  //memset(Is, 0, SIZE);
  //memset(Is, SMAX, SIZE);
  printArray("Sorting:", Is);

  markovSort();
  printArray("Guess:  ", Os);

  makeExample();
  printArray("Answer: ", Os);
  return 0;
}

