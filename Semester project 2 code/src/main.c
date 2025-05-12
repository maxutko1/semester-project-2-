/*
 * TutorialProject1.c
 *
 * Created: 24-08-2020 15:19:54
 * Author : duggen
 */ 
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


int fire;
int row, seat;
double x_axis, y_axis;

// Function prototype
void select_mode_coordinates(void);
void read_operation_choice(void);
void fire_button(void);
void power_off(void);
void select_mode_manual(void);
void read_power_button(void);
void calculations_for_x(void);
void calculations_for_y(void);

int main(void)
{
    row = 0, seat = 0;
    fire = 0;

    uart_init(); // Initialize communication with PC
    io_redirect(); // Redirect printf to UART for debugging

    printf("page page0%c%c%c", 255, 255, 255);
    

    read_operation_choice();

    while (1)
    {
     
        fire_button();
        //read_power_button();
        
    }
}

// Function to handle touch events and data retrieval
void select_mode_coordinates(void)
{
    unsigned char readBuffer[100]; // Buffer to hold incoming data
    unsigned int typeExpected = 0;

    
        for (int i = 0; i < 7; i++) {
            scanf("%c", &readBuffer[i]);
            if (readBuffer[i] == 0x65) {  // Start byte indicates touch event
                typeExpected = BUTTON_STRING;
                readBuffer[0] = 0x65;  // Keep the format consistent
                break;
            }
        }

        if (typeExpected == BUTTON_STRING) {
            // Read the full response for b1.val on page 1
            for (int i = 1; i < 7; i++) {
                scanf("%c", &readBuffer[i]);
            }

            // Validate the response (start byte 0x71 and end markers 0xFF 0xFF 0xFF)
            if (readBuffer[0] == 0x65 && readBuffer[4] == 0xFF && readBuffer[5] == 0xFF && readBuffer[6] == 0xFF)
            {
                if (readBuffer[1] == 0x03 && readBuffer[2] == 0x07) { // Save button pressed on page 3
                    // Fetch page3.n0.val
                    printf("get page3.n0.val%c%c%c", 0xFF, 0xFF, 0xFF);
                    for (int i = 0; i < 8; i++) {
                        scanf("%c", &readBuffer[i]);
                        if (readBuffer[i] == 0x71) {
                            typeExpected = NUMBER_STRING;
                            readBuffer[0] = 0x71;
                            break;
                        }
                    }

                    if (typeExpected == NUMBER_STRING) {
                        for (int i = 1; i < 8; i++) {
                            scanf("%c", &readBuffer[i]);
                        }
                        if (readBuffer[0] == 0x71 && readBuffer[5] == 0xFF && readBuffer[6] == 0xFF && readBuffer[7] == 0xFF) {
                            row = readBuffer[1] | (readBuffer[2] << 8) | (readBuffer[3] << 16) | (readBuffer[4] << 24);
                        }
                    }

                    // Fetch page3.n1.val
                    printf("get page3.n1.val%c%c%c", 0xFF, 0xFF, 0xFF);
                    for (int i = 0; i < 8; i++) {
                        scanf("%c", &readBuffer[i]);
                        if (readBuffer[i] == 0x71) {
                            typeExpected = NUMBER_STRING;
                            readBuffer[0] = 0x71;
                            break;
                        }
                    }

                    if (typeExpected == NUMBER_STRING) {
                        for (int i = 1; i < 8; i++) {
                            scanf("%c", &readBuffer[i]);
                        }
                        if (readBuffer[0] == 0x71 && readBuffer[5] == 0xFF && readBuffer[6] == 0xFF && readBuffer[7] == 0xFF) {
                            seat = readBuffer[1] | (readBuffer[2] << 8) | (readBuffer[3] << 16) | (readBuffer[4] << 24);

                            calculations_for_x();
                            calculations_for_y();
                        }
                    }
                }
            }
        }
}

void read_operation_choice(void){
    unsigned char readBuffer[100]; // Buffer to hold incoming data
    unsigned int typeExpected = 0;

    
        for (int i = 0; i < 7; i++) {
            scanf("%c", &readBuffer[i]);
            if (readBuffer[i] == 0x65) {  // Start byte indicates touch event
                typeExpected = BUTTON_STRING;
                readBuffer[0] = 0x65;  // Keep the format consistent
                break;
            }
        }

        if (typeExpected == BUTTON_STRING) {
            // Read the full response for b1.val on page 0
            for (int i = 1; i < 7; i++) {
                scanf("%c", &readBuffer[i]);
            }

            // Validate the response (start byte 0x71 and end markers 0xFF 0xFF 0xFF)
            if (readBuffer[0] == 0x65 && readBuffer[4] == 0xFF && readBuffer[5] == 0xFF && readBuffer[6] == 0xFF)
            {
                if (readBuffer[1] == 0x00 && readBuffer[2] == 0x02) { // Save button pressed on page 0
                    select_mode_manual();
                }
                if (readBuffer[1] == 0x00 && readBuffer[2] == 0x03) {
                    select_mode_coordinates();
                }
            }
        }
}

void fire_button(void){
    unsigned char readBuffer[100]; // Buffer to hold incoming data
    unsigned int typeExpected = 0;


        for (int i = 0; i < 7; i++) {
            scanf("%c", &readBuffer[i]);
            if (readBuffer[i] == 0x65) {  // Start byte indicates touch event
                typeExpected = BUTTON_STRING;
                readBuffer[0] = 0x65;  // Keep the format consistent
                break;
            }
        }

        if (typeExpected == BUTTON_STRING) {
            // Read the full response for b1.val on page 4
            for (int i = 1; i < 7; i++) {
                scanf("%c", &readBuffer[i]);
            }

            // Validate the response (start byte 0x71 and end markers 0xFF 0xFF 0xFF)
            if (readBuffer[0] == 0x65 && readBuffer[4] == 0xFF && readBuffer[5] == 0xFF && readBuffer[6] == 0xFF)
            {
                if (readBuffer[1] == 0x04 && readBuffer[2] == 0x03) { // Save button pressed on page 1
                    fire = 1;
                    printf("page page0%c%c%c", 255, 255, 255);
                    //printf("fire is %d\n", fire);
                    _delay_ms(1000);
                    fire = 0;
                    //printf("fire is %d\n", fire);
                    read_operation_choice();
                }
            }
        }
}


void select_mode_manual(void){

}



void read_power_button(void){
    unsigned char readBuffer[100]; // Buffer to hold incoming data
    unsigned int typeExpected = 0;

        for (int i = 0; i < 7; i++) {
            scanf("%c", &readBuffer[i]);
            if (readBuffer[i] == 0x65) {  // Start byte indicates touch event
                typeExpected = BUTTON_STRING;
                readBuffer[0] = 0x65;  // Keep the format consistent
                break;
            }
        }

        if (typeExpected == BUTTON_STRING) {
            // Read the full response for b1.val on page 4
            for (int i = 1; i < 7; i++) {
                scanf("%c", &readBuffer[i]);
            }

            // Validate the response (start byte 0x71 and end markers 0xFF 0xFF 0xFF)
            if (readBuffer[0] == 0x65 && readBuffer[4] == 0xFF && readBuffer[5] == 0xFF && readBuffer[6] == 0xFF)
            {
                if (readBuffer[1] == 0x04 && readBuffer[2] == 0x07) { // Save button pressed on page 1
                    printf("sleep 0%c%c%c", 255, 255, 255);
                }
            }
        }
}

void calculations_for_y(void){
    double term1 = 0.65*seat-3.225;
    double term2 = 1.65*row-0.65;
    double term3 = pow(term1, 2)+pow(term2, 2);
    double term4 = 0.02049368066*sqrt(term3);

    y_axis = asin(term4)/2;
    printf("y-axis is %f\n", y_axis);
}

void calculations_for_x(void){
    double term1 = 0.65*seat-3.225;
    double term2 = 1.65*row-0.65;
    double term3 = term1/term2;

    x_axis = 180*atan(term3)/3.14;
    printf("x-axis is %f\n", x_axis);
}