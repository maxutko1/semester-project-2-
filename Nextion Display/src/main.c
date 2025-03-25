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
#include "i2cmaster.h"
#include "lcd.h"

unsigned char x, y, z1, z2;
unsigned char q0, q1, q2, q0_next, q1_next, q2_next;

char current_state;

void read_xy_values(void);
void show_output(void);
void state_transition(void);
void get_current_state(void);

void main() 
{  
  current_state = 'A';
  q0 = 0;
  q1 = 0;
  q2 = 0;
  q0_next = 0;
  q1_next = 0;
  q2_next = 0;
  x = 0;
  y = 0;  

  DDRC = 0xF0;
  PORTC = 0x3F;
  DDRB |= (1 << DDB5);

  i2c_init();
  LCD_init();
  uart_init(); // open the communication to the microcontroller
  //io_redirect(); // redirect input and output to the communication

    
  while(1) {
		
	  read_xy_values();
    get_current_state();
    state_transition();
    show_output();

    q0_next = ((x && !q1) + (q0 && !x) + q2_next);
    q1_next = ((!q2 && q0 && x) + (q1 && !x) + (q2 && q1));
    q2_next = ((q2 && !q0) + (!q0 && !x && y));
    q0 = q0_next;
    q1 = q1_next;
    q2 = q2_next;
    if (q0_next >> 1){
      q0 = 1;
    }
    if(q1_next >> 1){
      q1 = 1;
    }
    if(q2_next >> 1){
      q2 = 1;
    }

    _delay_ms(1000);
  }
}

void read_xy_values(void){
  if(!(PINC & (1 << PC2))){
    x = 1;
  }
  else{
    x = 0;
  }
  if(!(PINC & (1 << PC3))){
    y = 1;
  }
  else{
    y = 0;
  }

}

void show_output(void){
  LCD_set_cursor(4,1);
  printf("x = %d  y = %d", x, y);
  LCD_set_cursor(0,2);
  printf("Q0 = %d Q1 = %d Q2 = %d", q0, q1, q2);
  LCD_set_cursor(2,3);
  printf("State: %c", current_state);
}

void state_transition(void){
  PORTB |= (1 << PORTB5);
  _delay_ms(100);
  PORTB = 0b00000000;
}

void get_current_state(void){
  if(q0 == 0 && q1 == 0 && q2 == 0){
    current_state ='A';
  }
  if(q0 == 1 && q1 == 0 && q2 == 0){
    current_state ='B';
  }
  if(q0 == 0 && q1 == 1 && q2 == 0){
    current_state ='C';
  }
  if(q0 == 1 && q1 == 1 && q2 == 0){
    current_state ='D';
  }
  if(q0 == 0 && q1 == 0 && q2 == 1){
    current_state ='E';
  }
  if(q0 == 1 && q1 == 0 && q2 == 1){
    current_state ='F';
  }
  if(q0 == 0 && q1 == 1 && q2 == 1){
    current_state ='G';
  }
  if(q0 == 1 && q1 == 1 && q2 == 1){
    current_state ='H';
  }
}