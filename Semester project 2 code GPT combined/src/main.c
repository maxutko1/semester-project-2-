/*
 * TutorialProject1.c
 *
 * Created: 24-08-2020 15:19:54
 * Author : duggen
 */ 
#include <avr/wdt.h>
#include <avr/io.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <util/delay.h> // Delay functions
#include <math.h>
#include "usart.h"

#define F_CPU 16000000UL // Needs to be defined for the delay functions to work.
#define BAUD 9600
#define NUMBER_STRING 1001
#define BUTTON_STRING 1002
#define TIMER_STOP 1003

// Motor A (Angle Control) Definitions
#define MOTOR_IN1 PB0
#define MOTOR_IN2 PB1
#define MOTOR_A_SPEED 200

// Motor B (Position Control) Definitions
#define MOTOR_IN3 PB2
#define MOTOR_IN4 PB3
#define MOTOR_B_SPEED 200

// Angle control globals (Motor A)
uint16_t neededAngleA;
uint16_t angleA;
int angleTolerance = 5; // in degrees

// Position control globals (Motor B)
uint16_t current_postion_X = 0;
uint16_t neededX;
uint16_t difference;
unsigned int address = 0;

// Function Prototypes
// --- Angle Motor (Motor A)
void adc_init(void);
uint16_t read_adc(void);
uint16_t read_angleA(void);
void pwmA_init(void);
void pwmA_set(uint8_t duty);
void motorA_ccw(void);
void motorA_cw(void);
void motorA_stop(void);
void motorA_run(void);

// --- Position Motor (Motor B)
void pwmB_init(void);
void pwmB_set(uint8_t duty);
void stopX(void);
void directionX(int dir);
void difference_function(void);
void counter(void);
void update_position(void);
void movement(void);


int fire;
int row, seat;
double x_axis, y_axis;
int reset;
int current_page = 1;

// Function prototype
void select_mode_coordinates(void);
void handle_page1_buttons(void);
void handle_page4_buttons(void);
//void fire_button(void);
//void power_off(void);
void select_mode_manual(void);
void read_power_button(void);
void calculations_for_x(void);
void calculations_for_y(void);
void flush_uart(void);
void soft_reset(void);
void reload(void);
void reset_turret(void);

int main(void)
{
    row = 0, seat = 0;
    fire = 0;

    uart_init(); // Initialize communication with PC
    io_redirect(); // Redirect printf to UART for debugging
    adc_init();
    pwmA_init();
    pwmB_init();

    DDRB |= (1 << MOTOR_IN1) | (1 << MOTOR_IN2) | (1 << MOTOR_IN3) | (1 << MOTOR_IN4); // Set motor pins as output

    printf("page page0%c%c%c", 255, 255, 255);
    _delay_ms(300);
    flush_uart();

    //handle_page1_buttons();

    while (1)
    {
        if(current_page == 1){
            handle_page1_buttons();
        } else if(current_page == 4){
            handle_page4_buttons();
        }
        //fire_button();
        //read_power_button();
        //handle_page4_buttons();
        
        if(reset == 1){
            reset = 0;
            fire = 0;
            printf("firing stopped");
            soft_reset();
        }
        
    }
}

// Function to handle touch events and data retrieval
void select_mode_coordinates(void)
{
    unsigned char readBuffer[8];
    int typeExpected = 0;

    // Wait for button event
    while (uart_data_available() < 7);

    for (int i = 0; i < 7;) {
        if (uart_data_available()) {
            readBuffer[i++] = uart_getchar_raw();
        }
    }

    if (readBuffer[0] == 0x65 &&
        readBuffer[4] == 0xFF &&
        readBuffer[5] == 0xFF &&
        readBuffer[6] == 0xFF &&
        readBuffer[1] == 0x03 && readBuffer[2] == 0x07) {

        // Read row
        printf("get page3.n0.val%c%c%c", 255, 255, 255);
        while (uart_data_available() < 8);
        for (int i = 0; i < 8;) {
            if (uart_data_available()) {
                readBuffer[i++] = uart_getchar_raw();
            }
        }
        if (readBuffer[0] == 0x71) {
            row = readBuffer[1] | (readBuffer[2] << 8) | (readBuffer[3] << 16) | (readBuffer[4] << 24);
        }

        // Read seat
        printf("get page3.n1.val%c%c%c", 255, 255, 255);
        while (uart_data_available() < 8);
        for (int i = 0; i < 8;) {
            if (uart_data_available()) {
                readBuffer[i++] = uart_getchar_raw();
            }
        }
        if (readBuffer[0] == 0x71) {
            seat = readBuffer[1] | (readBuffer[2] << 8) | (readBuffer[3] << 16) | (readBuffer[4] << 24);

            calculations_for_x();
            calculations_for_y();
            current_page = 4;
        }
    }
}

void handle_page1_buttons(void){
    unsigned char readBuffer[7];

    // Wait for full event
    while (uart_data_available() < 7);

    for (int i = 0; i < 7; ) {
        if (uart_data_available()) {
            readBuffer[i++] = uart_getchar_raw();
        }
    }

    if (readBuffer[0] == 0x65 &&
        readBuffer[4] == 0xFF &&
        readBuffer[5] == 0xFF &&
        readBuffer[6] == 0xFF) {

        if (readBuffer[1] == 0x00 && readBuffer[2] == 0x02) {
            select_mode_manual();
        } 
        else if (readBuffer[1] == 0x00 && readBuffer[2] == 0x03) {
            select_mode_coordinates();
        }
        else if (readBuffer[1] == 0x00 && readBuffer[2] == 0x04) {
            reload();
        }
        else if (readBuffer[1] == 0x00 && readBuffer[2] == 0x05) {
            printf("done reloading button pressed");
            reset_turret();
        }
    }
}

void handle_page4_buttons(void) {
    unsigned char buffer[7];
    static int index = 0;

    while (uart_data_available()) {
        buffer[index] = uart_getchar_raw();

        if (index == 0 && buffer[0] != 0x65) {
            continue;  // wait for valid start
        }

        index++;

        if (index == 7) {
            printf("Touch Event (Page 4): %02X %02X %02X %02X %02X %02X %02X\n",
                   buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);

            if (buffer[0] == 0x65 &&
                buffer[4] == 0xFF &&
                buffer[5] == 0xFF &&
                buffer[6] == 0xFF &&
                buffer[1] == 0x04) {  // Page 4

                switch (buffer[2]) {
                    case 0x03:  // Fire button
                        fire = 1;
                        //printf("page page0%c%c%c", 255, 255, 255);
                        _delay_ms(300);
                        flush_uart();
                        reset = 1;
                        break;

                    case 0x07:  // Power button
                        printf("sleep=1%c%c%c", 255, 255, 255);
                        break;

                    default:
                        break;
                }
            }

            index = 0;
            break;
        }
    }
}


void select_mode_manual(void){

}


void calculations_for_y(void){
    double term1 = 0.65*seat-3.225;
    double term2 = 1.65*row-0.65;
    double term3 = pow(term1, 2)+pow(term2, 2);
    double term4 = 0.02049368066*sqrt(term3);

    y_axis = asin(term4)/2;
    //printf("y-axis is %f\n", y_axis);

    neededAngleA = y_axis + 150;

    run_motor_control_sequence();
}

void calculations_for_x(void){
    double term1 = 0.65*seat-3.225;
    double term2 = 1.65*row-0.65;
    double term3 = term1/term2;

    x_axis = 180*atan(term3)/3.141592654;
    //printf("x-axis is %f\n", x_axis);

    if (x_axis < 0){
        neededX = 360 + x_axis;
        //printf("neededX is %u\n", neededX);
    } else {
        neededX = x_axis;
        //printf("neededX is %u\n", neededX);
    }
}

void flush_uart(void) {
    while (uart_data_available()) {
        uart_getchar_raw();
    }
}

void soft_reset(void) {
    wdt_enable(WDTO_15MS);  // Set watchdog timer to 15ms
    while (1);              // Wait for the watchdog to trigger reset
}

void adc_init() {
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t read_adc() {
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

uint16_t read_angleA() {
    return (read_adc() * 300UL) / 1023;
}

void pwmA_init() {
    DDRD |= (1 << PD6); // OC0A
    TCCR0A = (1 << COM0A1) | (1 << WGM00) | (1 << WGM01);
    TCCR0B = (1 << CS01) | (1 << CS00);
    OCR0A = 0;
}

void pwmA_set(uint8_t duty) {
    OCR0A = duty;
}

void motorA_ccw() {
    PORTB &= ~(1 << MOTOR_IN1);
    PORTB |= (1 << MOTOR_IN2);
}

void motorA_cw() {
    PORTB |= (1 << MOTOR_IN1);
    PORTB &= ~(1 << MOTOR_IN2);
}

void motorA_stop() {
    pwmA_set(0);
}

void motorA_run() {
    while ((angleA = read_angleA()) < neededAngleA - angleTolerance || angleA > neededAngleA + angleTolerance) {
        if (angleA < neededAngleA - angleTolerance) {
            motorA_ccw();
            pwmA_set(MOTOR_A_SPEED);
        } else if (angleA > neededAngleA + angleTolerance) {
            motorA_cw();
            pwmA_set(MOTOR_A_SPEED);
        }
        printf("Adjusting angle... Current: %u degrees\n", angleA);
    }
    motorA_stop();
    printf("Angle within tolerance: %u degrees\n", angleA);
}

// ---------------- POSITION CONTROL (Motor B) ----------------

void pwmB_init() {
    DDRD |= (1 << PD3); // OC2B
    TCCR2A |= (1 << WGM20) | (1 << WGM21);
    TCCR2B |= (1 << CS22);
    TCCR2A |= (1 << COM2B1);
    OCR2B = 0;
}

void pwmB_set(uint8_t duty) {
    OCR2B = duty;
}

void stopX(void) {
    pwmB_set(0);
    printf("Position motor stopped\n");
}

void directionX(int dir) {
    if (dir == 0) {
        PORTB &= ~(1 << MOTOR_IN3);
        PORTB |= (1 << MOTOR_IN4);
        printf("Motor B direction: CCW\n");
    } else {
        PORTB |= (1 << MOTOR_IN3);
        PORTB &= ~(1 << MOTOR_IN4);
        printf("Motor B direction: CW\n");
    }
}

void counter(void) {
    DDRD &= ~(1 << DDD4);
    PORTD |= (1 << PORTD4);
    TCCR1B |= (1 << CS12) | (1 << CS11) | (1 << CS10); // Max prescaler
}

void difference_function(void) {
    difference = (neededX > current_postion_X) ? (neededX - current_postion_X) : (current_postion_X - neededX);
}

void update_position(void) {
    if (neededX < current_postion_X)
        current_postion_X = (current_postion_X - difference) % 360;
    else
        current_postion_X = (current_postion_X + difference) % 360;

    if (current_postion_X < 0)
        current_postion_X += 360;

    printf("Updated position: %u\n", current_postion_X);
    eeprom_write_word((uint16_t *)address, current_postion_X);
}

void movement(void) {
    uint16_t target = TCNT1 + difference;
    if (target >= 360) target %= 360;

    while (TCNT1 < target) {
        pwmB_set(MOTOR_B_SPEED);
        printf("Moving... Encoder: %u\n", TCNT1);
    }

    stopX();
    printf("Reached target position\n");
}

void run_motor_control_sequence() {

    // EEPROM: Read saved position
    current_postion_X = eeprom_read_word((uint16_t *)address);
    printf("Current EEPROM position: %u\n", current_postion_X);

    // --- Angle Control (Motor A)
    printf("Needed angle for A: %u\n", neededAngleA);
    motorA_run();

    // --- Position Control (Motor B)
    counter(); // Start encoder counter

    printf("Needed angle for x: %u\n", neededX);
    //scanf("%u", &neededX);

    difference_function();
    printf("Difference to move: %u degrees\n", difference);

    if (current_postion_X > neededX)
        directionX(0); // CCW
    else
        directionX(1); // CW

    movement();
    update_position();

    printf("Motor control sequence completed.\n");
}

void reload(void){
    neededAngleA = 240;

    // --- Angle Control (Motor A)
    printf("Needed angle for A: %u\n", neededAngleA);
    motorA_run();

    printf("turret at reload position");

    reset = 1;
}

void reset_turret(void){
    neededAngleA = 150;

    motorA_run();

    neededX = 0;

    counter();

    difference_function();
    if (current_postion_X > neededX)
        directionX(0); // CCW
    else
        directionX(1); // CW

    movement();
    update_position();

    reset = 1;
}