/*
quick start guide:

LCD Commands:
1. LCD_set_cursor(x,y)  sets the cursor to a coordinate on the lcd screen grid.
2. printf("xxx")  prints a string to the screen from the point your cursor is on.
3. LCD_clear()  clears the lcd screen

Button reading:
if (PINC == 0b0011xxxx) | reads for each pin where the "x's" correspond to a button being pressed (note: the buttons are normally 1 and becom 0 when pressed)

LED control:
PORTD = 0bxxxx0000 | the "x's" represent each LED such that if the x is replaced with a 1 the corresponding LED will be on and vice versa for 0

Temperature reading:
get_temperature(); | reads the value of the temperaturethrough the sensor. can be used to insert values into other variables by for of: variable = get_temperature();
*/


#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include "usart.h"
#include <lm75.h>
#include <stdbool.h>
#include <lcd.h>
#include <avr/eeprom.h>
void delay_ms(unsigned int milliseconds){
  TCCR0A |= (1 << WGM01);
  OCR0A = 249;
  TCCR0B |= (1 << CS01) | (1 << CS00); 
  for(int i=0;i<milliseconds;i++){
  
  while ( (TIFR0 & (1 << OCF0A) ) == 0)
  {
    
  }
  TIFR0 = (1 << OCF0A);
}

}
void delay_hs(unsigned int hunseconds){//just a function for the hunseconds
  for(int x = 0;x<hunseconds;x++){
  delay_ms(100);//literally just calls my function
  }

}
void counter(){
  DDRD &= ~(1 << DDD4);//sets the pin 4 to be input
  PORTD |= (1 << PORTD4);//do it on pb 4
  TCCR0B |= (1 << CS02) | (1 << CS01) | (1 << CS00);// setting up counter on rising edge
  
}
int main(void) {  

    i2c_init();
    uart_init();
    io_redirect();
    lm75_init();
  DDRC = 0xf0;//sets the button pins as input
  PORTC = 0x3f;//enable input pull for the pins
  DDRD = 0xff;//sets the led pins as output
  PORTD = 0x00;// sets the led pins to be off
  DDRB = 0xff;
  counter();
  while (1){
    PORTB = (1<<PIN5);
 // delay_hs(2);
  PORTB = (0<<PIN5);
 // delay_hs(2);
 _delay_ms(1);
  printf("%d\n",TCNT0);
  }

  return 0; 
}
