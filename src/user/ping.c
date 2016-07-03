#include <conio.h>
#include <process.h>
#include <sched.h>
#include <sema.h>
#include <string.h>

int main(int argc , char ** argv)
{
  int i,j ;     	/* loop index */
  int start_sem;
  int scr_sem; 		/* id of screen semaphore */
  int time; 		/* current and start time */
  int ping,pong;	/* id of semaphores to sync processes b & c */

  time = Get_Time_Of_Day();
  start_sem = Create_Semaphore ("start" , 1);
  scr_sem = Create_Semaphore ("screen" , 1) ;   /* register for screen use */
  ping = Create_Semaphore ("ping" , 1) ;   
  pong = Create_Semaphore ("pong" , 0) ;  

  P (start_sem) ;
  V (start_sem) ;
  
  for (i=0; i < 50; i++) {
       P(pong);
       for (j=0; j < 35; j++);
     P(scr_sem) ;   
	   Set_Attr(ATTRIB(BLACK, AMBER|BRIGHT));
	   Print("Ping");
	   Set_Attr(ATTRIB(BLACK, GRAY));
     V(scr_sem);
     V(ping);
  }

  time = Get_Time_Of_Day() - time;
  P(scr_sem) ;
  Print ("\nProcess #Ping is done at time: %d\n", time) ;
  V(scr_sem);

  Destroy_Semaphore(pong);
  Destroy_Semaphore(ping);
  Destroy_Semaphore(scr_sem);

  return (0);

}
