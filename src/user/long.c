#include <conio.h>
#include "libuser.h"
#include "process.h"

int main(int argc, char **argv)
{
  int i, j ;     	/* loop index */
  int start_sem;
  int scr_sem;		/* id of screen semaphore */
  int now, start, elapsed; 		

  start = Get_Time_Of_Day();
  start_sem = Create_Semaphore ("start" , 1);
  scr_sem = Create_Semaphore ("screen" , 1) ;   /* register for screen use */

  P (start_sem) ;
  V (start_sem) ;
  
  for (i=0; i < 200; i++) {
      for (j=0 ; j < 200000; j++) ;
	  //P (scr_sem) ;
    Set_Attr(ATTRIB(BLACK, MAGENTA|BRIGHT));
    Print("Long");
	  Set_Attr(ATTRIB(BLACK, GRAY));
    //V(scr_sem);
    now = Get_Time_Of_Day();
  }
  elapsed = Get_Time_Of_Day() - start;
  P (scr_sem) ;
  Print("\nProcess Long is done at time: %d\n", elapsed) ;
  V(scr_sem);


  return 0;
}

