#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

#define CALC 1
#define IO 2

void printAverages(int, int, int, int, int);
int IOfunc(void);
int simpleCalc(void);
void sanityTest(int type, int num_of_procs, int loop_size, int (*func)());

int IOfunc(){

  sleep(1);
  return 0;
}

int simpleCalc(){

  return 1+2*9;
}

void sanityTest(int type, int num_of_procs, int loop_size, int (*func)()){
    
  printf(1, "Starting sanityTest..\n");


  int wtime = 0, rtime = 0, iotime = 0;
  int wtimeAcc = 0, rtimeAcc = 0, iotimeAcc = 0;

  int procPids[num_of_procs];
  
  //Create lot's of sub procs
  int currPid;
  int i;
  for(i=0; i < num_of_procs; i++){

    currPid = fork();

    if(currPid == 0){ // I'M THE CHILD HERE..

      set_priority((i%3)+1);

      int j;
      for(j=0; j < loop_size; j++){

        // (*func)();
        int calcAcc = 0;

        if((type == 1) || (type == 2))
          calcAcc++;
        else
          // sleep(1);
      }

      exit(); // kill child
    }
    
    procPids[i] = currPid;
  }

  // I'M THE FATHER HERE..

  int k;
  for(k=0; k < num_of_procs; k++){

    wait2(procPids[k], &wtime, &rtime, &iotime);

    wtimeAcc += wtime;
    rtimeAcc += rtime;
    iotimeAcc += iotime;

    wtime = 0;
    rtime = 0;
    iotime = 0;
  }

  printAverages(type, num_of_procs, wtimeAcc, rtimeAcc, iotimeAcc);
}

void printAverages(int type, int num_of_procs, int wtimeAcc, int rtimeAcc, int iotimeAcc){

  printf(1, "%d: wtime - %d, rtime - %d, iotime - %d\n",
          type,
          wtimeAcc/num_of_procs,
          rtimeAcc/num_of_procs,
          iotimeAcc/num_of_procs);
}

int
main(int argc, char *argv[]){
  
  int num_of_procs = 10;

  sanityTest(1, num_of_procs, 10000000, simpleCalc);         // simple calculation within a medium sized loop1
  sanityTest(2, num_of_procs, 1000000000, simpleCalc);   // simple calculation within a very large loop
  sanityTest(3, num_of_procs, 1000, IOfunc);      // printing to screen within a medium sized loop
  sanityTest(4, num_of_procs, 10000, IOfunc); // printing to screen within a very large loop

  exit();
}
