#include <conio.h>
#include <libuser.h>

int gv_0 = 0xdeadbeef;
int gv_1 = 0;

int fibo(int num) ;

int main(void){
        int num;
        int i;

        Get_Key();
        Print("\n Enter the fibonacci number : " );
        num = Get_Key()-'0';

 		Print("\n ");
        for(i = 0 ; i < num ; i++ ){
                Print("%d " , fibo(i));
        }
        Print("\n\n");

        return 0;
}

int fibo(int num){
        if(num == 0) return 0;
        else if(num == 1) return 1;
        else return fibo(num-1) + fibo(num-2);
}