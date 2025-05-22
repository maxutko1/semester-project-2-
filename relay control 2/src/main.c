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
 int time;
 int howLong;
void Reload(int time){
  PORTB = (0<<PB4);
  for(int i = 0;i<time;i++){
  _delay_ms(1000);1
  }
  PORTB = (1<<PB4);
}
void shoot(int howLong){
  PORTD = (0<<PD7);
  for(int x = 0;x<howLong;x++){
    _delay_ms(100);
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
  printf("Please enter how much time you want to reload for\n");
  scanf("%u", &time);
  Reload(time);
  time= 0;
  printf("Please enter how much time you want to shoot for for\n");
  scanf("%u", &howLong);
  shoot(howLong);
  howLong = 0;




}
  
  
  return 0;
}
