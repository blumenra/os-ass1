#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

void printAverages(char*, int, int, int, int);
int printToScreen(void);
int simpleCalc(void);
void sanityTest(char* const type, int loop_size, int (*func)());

int printToScreen(){

  printf(1, "Printing to screen..\n");
  return 0;
}

int simpleCalc(){

  return 1+2*9;
}

void sanityTest(char* const type, int loop_size, int (*func)()){
    
  printf(1, "Starting sanityTest..\n");


  int wtime = 0, rtime = 0, iotime = 0;
  int wtimeAcc = 0, rtimeAcc = 0, iotimeAcc = 0;

  // CANNOT MAKE TOO MANY PROCESSES IN A STATIC ARRAY. NEED TO BE HANDLED!!
  int procPids[loop_size];
  
  //Create lot's of sub procs
  int currPid;
  int i;
  for(i=0; i < loop_size; i++){

    currPid = fork();

    if(currPid == 0){

      // I'M THE CHILD HERE..

      (*func)();

      exit(); // kill child
    }
    
    procPids[i] = currPid;
  }

  // I'M THE FATHER HERE..

  int k;
  for(k=0; k < loop_size; k++){

    printf(1, "procPids[%d]: %d\n", k, procPids[k]);
    wait2(procPids[k], &wtime, &rtime, &iotime);
    wtimeAcc += wtime;
    rtimeAcc += rtime;
    iotimeAcc += iotime;
  }

  printAverages(type, loop_size, wtimeAcc, rtimeAcc, iotimeAcc);
  // printAverages(type, loop_size, 100, 200, 300);
}

void printAverages(char* type, int loop_size, int wtimeAcc, int rtimeAcc, int iotimeAcc){

  printf(1, "%s\n", type);
  printf(1, "   Average wtime: %d\n", wtimeAcc/loop_size);
  printf(1, "   Average rtime: %d\n", rtimeAcc/loop_size);
  printf(1, "   Average iotime: %d\n", iotimeAcc/loop_size);
}

int
main(int argc, char *argv[])
{
  
  // CANNOT MAKE TOO MANY PROCESSES IN A STATIC ARRAY. NEED TO BE HANDLED!!

  // sanityTest("Simple calc with Medium loop size", 10000000, simpleCalc);         // simple calculation within a medium sized loop
  // sanityTest("Simple calc with Very Large loop size", 1000000000, simpleCalc);   // simple calculation within a very large loop
  sanityTest("Print to Screen with Medium loop size", 10, printToScreen);      // printing to screen within a medium sized loop
  // sanityTest("Print to Screen with Very Large loop size", 10000, printToScreen); // printing to screen within a very large loop

  exit();
}
