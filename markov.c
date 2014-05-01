#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#define SIZE 8
#define NCHAINS 1000
#define BITS 8
#define TRNS (1<<BITS)

typedef struct chain {
  int Ls[BITS]; // locations of sampled bytes
  uint16_t Ss[TRNS][TRNS]; // statistics
  uint16_t Ts[TRNS]; // transitions
} chain;

chain Cs[NCHAINS];
uint8_t Is[SIZE]; // input state
uint8_t Os[SIZE]; // output state
int Vs[SIZE][2]; // vote space

int cmp(const void *a, const void *b) {
  return (int)*(uint8_t*)a - (int)*(uint8_t*)b;
}

void exampleSort() {
  memcpy(Os, Is, SIZE);
  heapsort(Os, SIZE, 1, cmp);
}

int rnd(int N) {
  return round((double)rand()/RAND_MAX*(N-1));
}

void randomizeInput() {
  int I;
  for (I = 0; I < SIZE; I++) Is[I] = (uint8_t)rnd(256);
}

uint8_t gatherBits(uint8_t *Bs, chain *C) {
  int I;
  uint8_t R = 0;
  int Threshold = (int)round((double)(C-Cs)/(NCHAINS-1)*255);
  for (I = 0; I < BITS; I++) {
    int B = Threshold<Bs[C->Ls[I]] ? 1 : 0;
    R |= B << I;
  }
  return R;
}

void chainInit(chain *C) {
  int I;
  for (I = 0; I < BITS; I++) C->Ls[I] = rnd(SIZE);
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
    C->Ts[I] = BestTransition;
  }
}

void trainChains(int Examples) {
  int I;
  while (Examples-- > 0) {
    switch (rnd(4)) { //degenerate cases
    case 0: memset(Is, 0, SIZE); break;
    case 1: memset(Is, 0xFF, SIZE); break;
    default: randomizeInput();
    }
    exampleSort();
    for (I = 0; I < NCHAINS; I++) chainTrain(Cs+I);
  }
  for (I = 0; I < NCHAINS; I++) chainFinishTraining(Cs+I);
}

void chainVote(chain *C) {
  int I;
  uint8_t In = gatherBits(Is, C);
  uint8_t Out = C->Ts[In];
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
    Os[I] = (uint8_t)round((double)Vs[I][0]/Vs[I][1]*255);
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
  trainChains(1000);

  randomizeInput();
  //memset(Is, 0, SIZE);
  //memset(Is, 255, SIZE);
  printArray("Sorting:", Is);

  markovSort();
  printArray("Guess:  ", Os);

  exampleSort();
  printArray("Answer: ", Os);
  return 0;
}

