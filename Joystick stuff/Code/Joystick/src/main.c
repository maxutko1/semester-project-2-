#include <stdio.h>
#include <avr/io.h> // + delay, + usart, + io
#include <util/delay.h>
#include "usart.h"
#include "i2cmaster.h"
#include <util/delay.h>

#define ADC_PIN //define pin 0 ADC channel

uint16_t adc_read(uint8_t adc_channel);

int main(){
  uart_init();
  io_redirect();
  
  uint16_t adc_result_x;
  uint16_t adc_result_y;
  ADMUX = (1<<REFS0);
  ADCSRA = (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN);
  while(1){
   for(int i=0; i<1; i++){
      adc_result_x = adc_read(ADC_PIN + i);
      printf("%d X axis;    ", adc_result_x);
   }
   for (int i=1; i<2; i++){
    adc_result_y = adc_read(ADC_PIN + i);
    printf("%d Y axis \n", adc_result_y);
   }
  }
}

uint16_t adc_read(uint8_t adc_channel){
  ADMUX &= 0xF0;
  ADMUX |= adc_channel;
  ADCSRA |= (1<<ADSC);
  while( (ADCSRA & (1<<ADSC)) );
  return ADC;
}