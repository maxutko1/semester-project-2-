/*
 * HelloWorld.c
 *
 * Created: 11/9/2023 10:43:27 AM
 * Author : Alin
 */ 



#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>

#include "usart.h"

int main(void) {  

  int input;  

  uart_init(); // open the communication to the microcontroller
  io_redirect(); // redirect input and output to the communication

    
  while(1) {
		
	  printf("Type in a number \n");
    scanf("%d", &input);
    printf("The number you typed is %d is %x in hexadecimal \n", input, input);
	  _delay_ms(1000)	;

  }
  
  return 0;
}
