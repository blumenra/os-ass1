#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

void printAverages(char*, int, int, int, int);
int printToScreen(void);
int simpleCalc(void);
void sanityTest(char* const type, int num_of_procs, int loop_size, int (*func)());

int printToScreen(){

  printf(1, "Printing to screen..\n");
  return 0;
}

int simpleCalc(){

  return 1+2*9;
}

void sanityTest(char* const type, int num_of_procs, int loop_size, int (*func)()){
    
  printf(1, "Starting sanityTest..\n");


  int wtime = 0, rtime = 0, iotime = 0;
  int wtimeAcc = 0, rtimeAcc = 0, iotimeAcc = 0;

  int procPids[num_of_procs];
  
  //Create lot's of sub procs
  int currPid;
  int i;
  for(i=0; i < num_of_procs; i++){

    currPid = fork();

    if(currPid == 0){

      // I'M THE CHILD HERE..
      
      int j;
      for(j=0; j < loop_size; j++){

        (*func)();
        
        // if(1+2*9);
        //   continue;

        // printf(1, "Hi\n");

      }

      exit(); // kill child
    }
    
    procPids[i] = currPid;
  }

  // I'M THE FATHER HERE..

  int k;
  for(k=0; k < num_of_procs; k++){

    // printf(1, "procPids[%d]: %d\n", k, procPids[k]);
    // printf(1, "::::::rtime before wait2: %d\n", rtime);

    wait2(procPids[k], &wtime, &rtime, &iotime);

    // printf(1, "::::::rtime after wait2: %d\n", rtime);
    wtimeAcc += wtime;
    rtimeAcc += rtime;
    iotimeAcc += iotime;

    wtime = 0;
    rtime = 0;
    iotime = 0;
  }

  printf(1, "Final wtimeAcc: %d\n", wtimeAcc);
  printf(1, "Final rtimeAcc: %d\n", rtimeAcc);
  printf(1, "Final iotimeAcc: %d\n", iotimeAcc);

  printAverages(type, num_of_procs, wtimeAcc, rtimeAcc, iotimeAcc);
}

void printAverages(char* type, int num_of_procs, int wtimeAcc, int rtimeAcc, int iotimeAcc){

  printf(1, "%s\n", type);
  printf(1, "   Average wtime: %d\n", wtimeAcc/num_of_procs);
  printf(1, "   Average rtime: %d\n", rtimeAcc/num_of_procs);
  printf(1, "   Average iotime: %d\n", iotimeAcc/num_of_procs);
}

int
main(int argc, char *argv[]){
  
  int num_of_procs = 10;

  // sanityTest("Simple calc with Medium loop size", num_of_procs, 10000000, simpleCalc);         // simple calculation within a medium sized loop1
  // sanityTest("Simple calc with Very Large loop size", num_of_procs, 1000000000, simpleCalc);   // simple calculation within a very large loop
  sanityTest("Print to Screen with Medium loop size", num_of_procs, 1000, printToScreen);      // printing to screen within a medium sized loop
  // sanityTest("Print to Screen with Very Large loop size", num_of_procs, 10000, printToScreen); // printing to screen within a very large loop

  exit();
}
