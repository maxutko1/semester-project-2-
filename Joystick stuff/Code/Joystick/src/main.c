#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include "usart.h"
#include <stdbool.h>
#include <avr/eeprom.h>
#include <stdint.h>

//Definitions
#define MOTOR_IN3_PIN  PB2    // L298N IN3
#define MOTOR_IN4_PIN  PB3    // L298N IN4
#define MOTOR_SPEED 200  // PWM duty cycle (0-255) for motor speed

// Global variables:
uint16_t current_postion_X = 0;
unsigned int address = 0;
uint16_t neededX;
uint16_t difference;

// Function prototypes:
void difference_function(void);
void counter(void);
void update_position(void);
void movement(void);
void stopX(void);
void directionX(int);
void pwm_init(void);
void pwm_set(uint8_t duty);

int main(void) {  
    uart_init();
    io_redirect();
    pwm_init();

    current_postion_X = eeprom_read_byte((uint16_t *)address);   //using eeprom memory to read the last position
    printf(" %u\n", current_postion_X);
    //encoder counter.
    counter();

    printf("type it in \n");
    scanf("%u", &neededX);

    difference_function();
    printf("difference is %u\n", difference);
    
    if (current_postion_X > neededX) {
        directionX(0); 
    }
    else if (current_postion_X < neededX) {
        directionX(1);  
    }

    movement(); 

    update_position();

    while (1) {

       /* _delay_ms(1);
        printf("Current position: %u\n", TCNT1); */
    }
    
    return 0; 
}

void counter(void) {
    DDRD &= ~(1 << DDD4);   // Set pin 4 as input
    PORTD |= (1 << PORTD4);  // Enable pull-up on PD4
    TCCR1B |= (1 << CS12) | (1 << CS11) | (1 << CS10); // Set Timer1 prescaler
}

void difference_function(void) {
    // calculating the difference between neededX and current_postion_X.
    if (neededX > current_postion_X) {
        difference = neededX - current_postion_X;
    } else {
        difference = current_postion_X - neededX;
    }
}

void update_position(void) {
    if (neededX < current_postion_X){
        current_postion_X = (current_postion_X - difference) % 360;
    }
    else {
    current_postion_X = (current_postion_X + difference) % 360;
    }
    if(current_postion_X < 0) {
        current_postion_X += 360;
    }
    printf("hey ho %u\n", current_postion_X);
    eeprom_write_word((uint16_t *)address, current_postion_X);
}

void movement(void){
    uint16_t target = TCNT1 + difference;

    if(target >= 360) {
        target = target % 360;
    }

    while (TCNT1 < target) {
        pwm_set(MOTOR_SPEED);
        counter();
        printf("Moving... Current counter: %u\n", TCNT1);
    }
    printf("Reached target position\n");
    stopX();
}

void directionX(int dir){
    if (dir==0){
        //here we set the motor to go counterclockwise
        PORTB &= ~(1 << MOTOR_IN3_PIN);
        PORTB |= (1 << MOTOR_IN4_PIN);
        printf("motor is going ccw\n");
    }
    if (dir==1){
        //set the motor to go clockwise 
        PORTB |= (1 << MOTOR_IN3_PIN);
        PORTB &= ~(1 << MOTOR_IN4_PIN);
        printf("clockwise\n");
    }
}

void stopX(void){
    //the motor is stopped here 
    pwm_set(0);
    printf("the motor is stpopped\n");
    _delay_ms(5000);
}

void pwm_init() {
    // Set Pin D3 (PD3/OC2B) as output
    DDRD |= (1 << PD3);

    // Set Fast PWM mode: WGM22:0 = 0b011
    TCCR2A |= (1 << WGM20) | (1 << WGM21); // WGM21:0 = 11
    TCCR2B |= (1 << CS22);                // Prescaler = 64 (CS22 = 1)

    // Non-inverting mode for OC2B (Pin D3)
    TCCR2A |= (1 << COM2B1);

    // Set duty cycle (0-255)
    OCR2B = 0;
}

void pwm_set(uint8_t duty) {
    printf("pwm is set %u\n", duty);
    OCR2B = duty; // Set PWM duty cycle (0-255)
}




