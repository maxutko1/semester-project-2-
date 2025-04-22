#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include "usart.h"

// Motor pin definitions for L298N
#define MOTOR_IN1 PB0  // Motor driver input 1
#define MOTOR_IN2 PB1  // Motor driver input 2
#define MOTOR_SPEED 200  // PWM duty cycle (0-255) for motor speed

// Global variables
uint16_t needed;
uint16_t adc;
uint16_t voltage;
int tolerance = 100;
int flag = 0;

// Function prototypes
void adc_init();
uint16_t read_adc();
uint16_t read_volts();
void pwm_init();
void pwm_set(uint8_t duty);
void ccw();
void cw();
void motor_stop();
void motorrun();

int main(void) {
    uart_init();   // Initialize UART
    io_redirect(); // Redirect input/output to UART
    adc_init();    // Initialize ADC
    pwm_init();    // Initialize PWM

    // Set motor control pins as outputs (for direction)
    DDRB |= (1 << MOTOR_IN1) | (1 << MOTOR_IN2);

    printf("Please, type in the needed voltage in millivolts:\n");
    scanf("%u", &needed);

    while (1) {
        adc = read_adc();
        voltage = read_volts();

        // If voltage is too low, run motor counterclockwise
        if (voltage < needed - tolerance) {
            ccw();
        }
        // If voltage is too high, run motor clockwise
        if (voltage > needed + tolerance) {
            cw();
        }
        // Run the motor until the voltage is within tolerance
        motorrun();

        printf("ADC: %u | Voltage: %u mV\n", adc, voltage);
        _delay_ms(1500); // Adjust the delay as needed
    }

    return 0;
}

// ADC FUNCTIONS
void adc_init() {
    ADMUX = (1 << REFS0); // Use AVcc as reference; select ADC0 (PC0)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Enable ADC, prescaler 128
}

uint16_t read_adc() {
    ADCSRA |= (1 << ADSC);           // Start conversion
    while (ADCSRA & (1 << ADSC));    // Wait for conversion to finish
    return ADC;                      // Return the 10-bit ADC value
}

uint16_t read_volts() {
    // Convert ADC value (0-1023) to millivolts (assuming 5V reference)
    return (read_adc() * 5000UL) / 1023;
}

// PWM FUNCTIONS
void pwm_init() {
    // Set PD6 (OC0A) as output for PWM
    DDRD |= (1 << PD6);
    // Timer0: Fast PWM mode, non-inverting on OC0A, prescaler = 64
    TCCR0A = (1 << COM0A1) | (1 << WGM00) | (1 << WGM01);
    TCCR0B = (1 << CS01) | (1 << CS00);
    OCR0A = 0;  // Initially no PWM output
}

void pwm_set(uint8_t duty) {
    OCR0A = duty; // Set PWM duty cycle (0-255)
}

// MOTOR CONTROL FUNCTIONS
void ccw(void) {
    // For counterclockwise rotation, set MOTOR_IN1 low and MOTOR_IN2 high
    PORTB &= ~(1 << MOTOR_IN1);
    PORTB |= (1 << MOTOR_IN2);
    pwm_set(MOTOR_SPEED); // Apply PWM to drive the motor
}

void cw(void) {
    // For clockwise rotation, set MOTOR_IN1 high and MOTOR_IN2 low
    PORTB |= (1 << MOTOR_IN1);
    PORTB &= ~(1 << MOTOR_IN2);
    pwm_set(MOTOR_SPEED); // Apply PWM to drive the motor
}

void motor_stop(void) {
    // Stop the motor by disabling PWM and clearing direction pins
    pwm_set(0);
    PORTB &= ~((1 << MOTOR_IN1) | (1 << MOTOR_IN2));
}

void motorrun(void) {
    // Continue updating voltage until it's within the tolerance window
    while ((voltage < needed - tolerance) || (voltage > needed + tolerance)) {
        voltage = read_volts(); // Update the voltage value
        // Adjust direction based on updated voltage
        if (voltage < needed - tolerance) {
            ccw();
        } else if (voltage > needed + tolerance) {
            cw();
        }
        printf("Moving to position, current voltage: %u mV\n", voltage);
        _delay_ms(1000); // Delay to simulate motor movement response
    }
    // Once within tolerance, stop the motor
    motor_stop();
    printf("We are at the right position\n");
    flag = 1;
    _delay_ms(1000);
}      