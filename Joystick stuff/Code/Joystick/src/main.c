#include <stdio.h>
#include <avr/io.h> // + delay, + usart, + io
#include <util/delay.h>
#include "usart.h"
#include "i2cmaster.h"
#include <util/delay.h>

//functions declerations
uint16_t adc_read(uint8_t adc_channel);
uint16_t Joystick_ADC_Values();

//global variables
uint16_t adc_result_x;
uint16_t adc_result_y;

int main(){
  uart_init();
  io_redirect();
  
  while(1){
   turret_state();
  }
}

uint16_t adc_read(uint8_t adc_channel){
  ADMUX &= 0xF0;                         // clear registers 
  ADMUX |= adc_channel;                  // sets the MUX (4 last bits of ADMUX) and sets which ADC we'll use (depending on input)
  ADCSRA |= (1<<ADSC);                   // starts the conversion 
  while( (ADCSRA & (1<<ADSC)) );         // waits for conversion to complete
  return ADC;                            // returns the results
}

uint16_t Joystick_ADC_Values(){ 
  ADMUX = (1<<REFS0);                                       // These bits select the voltage reference for the ADC (AV_CC with external capacitor at AREF pin)
  ADCSRA = (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN);      // Enables ADC and sets prescaler to 128
  adc_result_x = adc_read(0);                               // Reads ADC PIN 0, which is our x axis
  adc_result_y = adc_read(1);                               // Reads ADC PIN 1, which is our y axis
}

void turret_state(void){                                    // Turret state depending on joystick input
  Joystick_ADC_Values();
    if(adc_result_x>600){                                   // check x-axis adc values
      printf("turret is turning clockwise \n");
      //do something ..
    }
    if(adc_result_x<400){                                   //check x-axis adc values 
      printf("turret is turning counter-clockwise \n");
      // do something ..
    }
    if(adc_result_y<400){                                   //check y-axis adc values 
      printf("turret is aiming up \n");
      //do something ..
    }
    if(adc_result_y>600){                                   //check y-axis adc values 
      printf("turret is aiming down \n");
      //do something ..
    }

}