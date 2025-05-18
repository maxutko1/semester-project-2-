
 #include <avr/io.h>
 #include <stdio.h>
 #include <util/delay.h>
 #include "usart.h"
 
// #define F_CPU 16000000UL
#define BAUD 9600
#define MOTOR_IN3_PIN  PB0    // L298N IN3
#define MOTOR_IN4_PIN  PB1    // L298N IN4
#define MOTOR_SPEED 255  // PWM duty cycle (0-255) for motor speed
 
 // Global variables
 uint16_t xValue, yValue;
 
 // Function declarations
 void adc_init();
 uint16_t adc_read(uint8_t);
 void read_joystick();
 void clockwise(void);
 void pwm_init(void);
void pwm_set(uint8_t duty);
void stop(void);
void counterclockwise(void);
 
 int main(void) {
     uart_init();
     io_redirect();
     adc_init();
     pwm_init();
     
     DDRD &= ~(1 << PD2); // Setting button on PD2 as input
     PORTD |= (1 << PD2); // Enable pull-up
 
     while (1) {
         read_joystick();
        // _delay_ms(200);
         if (xValue>525 && yValue<530 && yValue>480){
            counterclockwise();
            pwm_set(MOTOR_SPEED);
         }
         else if (xValue<500 && yValue<530 && yValue>480){
            clockwise();
            pwm_set(MOTOR_SPEED);
        }
        else {
            stop();
        }
        if (PIND & (1 << PD2)){
            printf("SHOTS FIRED\n");
        }
 }
}
 
 // ADC Initialization
 void adc_init() {
     ADMUX = (1 << REFS0); // AVcc reference
     ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Enable ADC, prescaler 128
 }
 
 // Read ADC value
 uint16_t adc_read(uint8_t channel) {
     ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
     ADCSRA |= (1 << ADSC);
     while (ADCSRA & (1 << ADSC));
     return ADC;
 }
 
 // Read and update joystick values
 void read_joystick() {
     xValue = adc_read(1);
     yValue = adc_read(2);
     //printf("X: %d, Y: %d, Button: %s\n", xValue, yValue, (PIND & (1 << PD2)) ? "Pressed" : "Released");
 }
 
 void stop(void){
    //the motor is stopped here 
    pwm_set(0);
   // printf("the motor is stpopped\n");
   // _delay_ms(5000);
}

void pwm_init() {
    DDRD |= (1 << PD3);
    TCCR2A |= (1 << WGM20) | (1 << WGM21); 
    TCCR2B |= (1 << CS22);             
    TCCR2A |= (1 << COM2B1);
    OCR2B = 0;
}

void pwm_set(uint8_t duty) {
    //printf("pwm is set %u\n", duty);
    OCR2B = duty; 
}

void clockwise(void){
    PORTB |= (1 << MOTOR_IN3_PIN);
    PORTB &= ~(1 << MOTOR_IN4_PIN);
}

void counterclockwise(void){
    PORTB &= ~(1 << MOTOR_IN3_PIN);
    PORTB |= (1 << MOTOR_IN4_PIN);
}