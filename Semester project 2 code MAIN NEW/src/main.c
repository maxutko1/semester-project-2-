#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include <avr/eeprom.h>
#include <stdint.h>
#include "usart.h"
#include "i2cmaster.h"
#include <math.h>
#include <avr/wdt.h>

// Nextion
#define F_CPU 16000000UL
#define BAUD 9600
#define NUMBER_STRING 1001
#define BUTTON_STRING 1002
#define TIMER_STOP 1003

// Motor A (Angle Control)
#define MOTOR_IN1 PB0
#define MOTOR_IN2 PB1
#define MOTOR_A_SPEED 50

// Motor B (Position Control)
#define MOTOR_IN3 PB2
#define MOTOR_IN4 PB3
#define MOTOR_B_SPEED 200

// Shooting Pins
#define SHOOT_RELAY PD7
#define SHOOT_BUTTON PD2

// Globals
uint16_t neededAngleA;
uint16_t angleA;
int angleTolerance = 1;
uint16_t current_position_X = 0;
uint16_t neededX;
uint16_t difference;
unsigned int address = 0;
uint16_t adc_result_x;
uint16_t adc_result_y;
bool manual_flag = false;
int time;
int howLong;
double x_axis, y_axis;
int row, seat;

// ----------- Function Declarations -----------
void adc_init(void);
uint16_t read_adc(uint8_t adc_channel);
uint16_t read_angleA(void);
void pwmA_init(void);
void pwmA_set(uint8_t duty);
void motorA_ccw(void);
void motorA_cw(void);
void motorA_stop(void);
void motorA_run(void);

void pwmB_init(void);
void pwmB_set(uint8_t duty);
void stopX(void);
void directionX(int dir);
void difference_function(void);
void counter(void);
void update_position(void);
void movement(void);
void turret_homing(void);

void Joystick_ADC_Values();
void turret_state(void);
void manual_motor_control(int x_dir, int x_strength, int y_dir, int y_strength);
void shoot(int howLong);
void Reload(int time);

void handle_page1_buttons(void);
void select_mode_coordinates(void);
void calculations_for_x(void);
void calculations_for_y(void);

void flush_uart();
void soft_reset();

// ---------------- MAIN ----------------

int main(void) {
    uart_init();
    io_redirect();
    adc_init();
    pwmA_init();
    pwmB_init();

    DDRB |= (1 << MOTOR_IN1) | (1 << MOTOR_IN2) | (1 << MOTOR_IN3) | (1 << MOTOR_IN4);
    DDRD |= (1 << SHOOT_RELAY);   // PD7 output (shooting relay)
    DDRD &= ~(1 << SHOOT_BUTTON); // PD2 input (shoot button)
    PORTD |= (1 << SHOOT_BUTTON); // Enable pull-up on button
    PORTD |= (1 << SHOOT_RELAY); // Enable pull-up on button

    printf("page page0%c%c%c", 255, 255, 255);
    _delay_ms(300);
    flush_uart();

    current_position_X = eeprom_read_word((uint16_t *)address);
    printf("Current EEPROM position: %u\n", current_position_X);

    counter();

    //printf("Enter mode (0 for auto, 1 for manual): ");
    //scanf("%hhu", &manual_flag);

    if (!manual_flag) {
        //printf("Enter needed angle (0-240): ");
        //scanf("%u", &neededAngleA);
        if (neededAngleA > 240) neededAngleA = 240;
        motorA_run();

        //printf("Enter needed X position (0-359): ");
        //scanf("%u", &neededX);
        difference_function();

        if (current_position_X > neededX)
            directionX(0);
        else
            directionX(1);

        movement();
        update_position();
    } else {
        while (1) {
            turret_state();
            if (!(PIND & (1 << SHOOT_BUTTON))) {
                printf("Button pressed, shooting for 0.3 seconds\n");
                shoot(3);
                _delay_ms(500);  // debounce
            }
        }
    }

    return 0;
}

// --------- Nextion Display reading buttons ----------

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
            manual_flag = 1;
        } 
        else if (readBuffer[1] == 0x00 && readBuffer[2] == 0x03) {
            select_mode_coordinates();
        }
        else if (readBuffer[1] == 0x00 && readBuffer[2] == 0x04) {
            Reload(howLong);
        }
        else if (readBuffer[1] == 0x00 && readBuffer[2] == 0x05) {
            turret_homing();
        }
    }
}

void handle_page4_buttons(void) {
    flush_uart();
    unsigned char readBuffer[7];

    while (uart_data_available() < 7);
    
    for (int i = 0; i < 7; ) {
        if (uart_data_available()) {
            readBuffer[i++] = uart_getchar_raw();
        }
    }
            
    if (readBuffer[0] == 0x65 &&
        readBuffer[4] == 0xFF &&
        readBuffer[5] == 0xFF &&
        readBuffer[6] == 0xFF) {  // Page 4

        if (readBuffer[1] == 0x04 && readBuffer[2] == 0x03) {
            shoot(time);
        } else if (readBuffer[1] == 0x04 && readBuffer[2] == 0x07) {
            printf("sleep=1%c%c%c", 255, 255, 255);
        } else if (readBuffer[1] == 0x04 && readBuffer[2] == 0x06) {
            soft_reset();
        }
            
    }

}

// --------- Coordinate system ---------

void select_mode_coordinates(void)
{
    unsigned char readBuffer[8];

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
            manual_flag = 0;
            handle_page4_buttons();
        }
    }
}

void calculations_for_y(void){
    double term1 = 0.65*seat-3.225;
    double term2 = 1.65*row-0.65;
    double term3 = pow(term1, 2)+pow(term2, 2);
    double term4 = 0.02049368066*sqrt(term3);

    y_axis = asin(term4)/2;
    //printf("y-axis is %f\n", y_axis);

    neededAngleA = y_axis + 150;
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

// --------- Homing function ---------

void turret_homing(void) {
    // Move Motor A to 150 degrees
    neededAngleA = 150;
    motorA_run();

    // Move Motor B to 0 degrees position
    neededX = 0;
    difference_function();

    if (current_position_X > neededX)
        directionX(0);  // CCW
    else
        directionX(1);  // CW

    movement();

    // Update current position in EEPROM
    current_position_X = 0;
    eeprom_write_word((uint16_t *)address, current_position_X);

    printf("Homing complete: Motor A at 150, Motor B at 0\n");

    soft_reset();
}


// --------- Shooting System ---------

void Reload(int time) {
    // Move Motor A to 240 degrees first
    neededAngleA = 240;
    motorA_run();  // This will run the motor until angle is ~240 degrees

    // Now energize the reload relay
    PORTB &= ~(1 << PB4);  // Turn relay ON (active low)
    for (int i = 0; i < time; i++) {
        _delay_ms(1000);   // Wait 1 second per loop
    }
    PORTB |= (1 << PB4);   // Turn relay OFF

    flush_uart();
    soft_reset();
}


void shoot(int howLong) {
    PORTD &= ~(1 << SHOOT_RELAY);  // Relay ON
    for (int i = 0; i < howLong; i++) {
        _delay_ms(100);
    }
    PORTD |= (1 << SHOOT_RELAY);   // Relay OFF

    flush_uart();
    soft_reset();
}

// ---------- ADC & Angle Control -----------

void adc_init() {
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t read_adc(uint8_t adc_channel) {
    ADMUX = (ADMUX & 0xF0) | adc_channel;
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

uint16_t read_angleA() {
    return (read_adc(0) * 300UL) / 1023;
}

void pwmA_init() {
    DDRD |= (1 << PD6);
    TCCR0A = (1 << COM0A1) | (1 << WGM00) | (1 << WGM01);
    TCCR0B = (1 << CS01) | (1 << CS00);
    OCR0A = 0;
}

void pwmA_set(uint8_t duty) {
    OCR0A = duty;
}

void motorA_ccw() {
    printf("Motor A CCW\n");
      PORTB &= ~(1 << MOTOR_IN1);
    PORTB |= (1 << MOTOR_IN2);
}

void motorA_cw() {
    printf("Motor A CW\n");
     PORTB |= (1 << MOTOR_IN1);
    PORTB &= ~(1 << MOTOR_IN2);
}

void motorA_stop() {
   // printf("Motor A STOP\n");
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
        printf("Adjusting angle... Current: %u\n", angleA);
    }
    motorA_stop();
    printf("Angle within tolerance: %u\n", angleA);
}

// ----------- Position Control ---------------

void pwmB_init() {
    DDRD |= (1 << PD3);
    TCCR2A |= (1 << WGM20) | (1 << WGM21);
    TCCR2B |= (1 << CS22);
    TCCR2A |= (1 << COM2B1);
    OCR2B = 0;
}

void pwmB_set(uint8_t duty) {
    OCR2B = duty;
}

void stopX(void) {
    //printf("Motor B STOP\n");
    pwmB_set(0);
}

void directionX(int dir) {
    if (dir == 0){
        PORTB &= ~(1 << MOTOR_IN3);
        PORTB |= (1 << MOTOR_IN4);
        printf("Motor B direction: CCW\n");}
    else{
        PORTB |= (1 << MOTOR_IN3);
        PORTB &= ~(1 << MOTOR_IN4);
        printf("Motor B direction: CW\n");}
}

void counter(void) {
    DDRD &= ~(1 << DDD4);
    PORTD |= (1 << PORTD4);
    TCCR1B |= (1 << CS12) | (1 << CS11) | (1 << CS10);
}

void difference_function(void) {
    difference = (neededX > current_position_X) ? (neededX - current_position_X) : (current_position_X - neededX);
}

void update_position(void) {
    if (neededX < current_position_X)
        current_position_X = (current_position_X - difference) % 360;
    else
        current_position_X = (current_position_X + difference) % 360;

    if ((int)current_position_X < 0)
        current_position_X += 360;

    printf("Updated position: %u\n", current_position_X);
    eeprom_write_word((uint16_t *)address, current_position_X);
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

// ---------------- MANUAL MODE ----------------

uint16_t last_encoder = 0;  // Store the last encoder value

void Joystick_ADC_Values() {
    adc_result_x = read_adc(1);
    adc_result_y = read_adc(2);
  //  printf("Joystick X: %u, Y: %u\n", adc_result_x, adc_result_y);
}

void turret_state(void) {
    Joystick_ADC_Values();

    int x_direction = 0, x_strength = 0;
    if (adc_result_x > 600) {
        x_direction = 1;
        x_strength = adc_result_x - 600;
    } else if (adc_result_x < 400) {
        x_direction = -1;
        x_strength = 400 - adc_result_x;
    }

    int y_direction = 0, y_strength = 0;
    if (adc_result_y > 600) {
        y_direction = 1;
        y_strength = adc_result_y - 600;
    } else if (adc_result_y < 400) {
        y_direction = -1;
        y_strength = 400 - adc_result_y;
    }

    if (x_strength == 0 && y_strength == 0) {
        stopX();
        motorA_stop();
        return;
    }

    manual_motor_control(x_direction, x_strength, y_direction, y_strength);
    update_position();

    while(1){
        handle_page4_buttons();
    }
}

void manual_motor_control(int x_dir, int x_strength, int y_dir, int y_strength) {
    uint16_t current_encoder = TCNT1;
    int16_t delta;

    if (x_strength > y_strength) {
        if (x_dir == 1) {
            directionX(1);
            pwmB_set(MOTOR_B_SPEED);
            delta = (int16_t)(current_encoder - last_encoder);
            current_position_X = (current_position_X + delta) % 360;
            printf("Manual CW. ΔEncoder: %d, New Pos: %u\n", delta, current_position_X);
        } else if (x_dir == -1) {
            directionX(0);
            pwmB_set(MOTOR_B_SPEED);
            delta = (int16_t)(last_encoder - current_encoder);
            current_position_X = (current_position_X + 360 - delta) % 360;
            printf("Manual CCW. ΔEncoder: %d, New Pos: %u\n", delta, current_position_X);
        }
        last_encoder = current_encoder;

    } else {
        if (y_dir == 1) {
            motorA_cw();
            pwmA_set(MOTOR_A_SPEED);
            printf("Manual A CW. Potentiometer: %u\n", read_angleA());
        } else if (y_dir == -1) {
            if (read_angleA() <= 240) {
                motorA_ccw();
                pwmA_set(MOTOR_A_SPEED);
                printf("Manual A CCW. Potentiometer: %u\n", read_angleA());
            } else {
                printf("Angle limit reached\n");
            }
        }
    }

    eeprom_write_word((uint16_t *)address, current_position_X);
}

// ---------- Flsuh the bytes -----------

void flush_uart(void) {
    while (uart_data_available()) {
        uart_getchar_raw();
    }
}

// ---------- Soft reset -----------

void soft_reset(void) {
    wdt_enable(WDTO_15MS);  // Set watchdog timer to 15ms
    while (1);              // Wait for the watchdog to trigger reset
}
