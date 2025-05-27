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
//#include <lcd.h>
#include <avr/eeprom.h>
 int time; // global variable
 int howLong;//globale variable
void Reload(int time){ // function, takes the amount of time you want to reload for. 1 sek is ish 8 psi. 
  PORTB = (0<<PB4);//turn the relay on. the relay works by putting the input pin to ground
  for(int i = 0;i<time;i++){// a for loop to control the amount of time it should reload for. 
  _delay_ms(1000); // delay
  }
  PORTB = (1<<PB4);//turns the relay off
}
void shoot(int howLong){ // again same thing, but this releases pressure exponentially. therefore, 8 psi ish pr 0.1 sec of shooting
  PORTD = (0<<PD7);//same thing, diffrent pin 
  for(int x = 0;x<howLong;x++){//same thing
    _delay_ms(100);//same thing
}
PORTD = (1<<PD7);
}
int main(void) {  
  uart_init();
    io_redirect();
    DDRD = 0xff;
    DDRB = 0xff;
    PORTD = 0xff;
    PORTB = 0xff;
while(1){
  printf("Please enter how much time you want to reload for\n");//this is currently how we get how much to reload for. 
  scanf("%u", &time);//type in a how much time u want
  Reload(time);//reload for the amount of time. 
  time= 0;//resets the time. this could be moved to the function. 
  printf("Please enter how much time you want to shoot for for\n");
  scanf("%u", &howLong);//same thing
  shoot(howLong);
  howLong = 0;




}
  
  
  return 0;
}
