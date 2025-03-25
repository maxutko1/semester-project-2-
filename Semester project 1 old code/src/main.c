#define F_CPU 16000000UL // Needs to be defined for delay functions to work

#include <avr/io.h>        // Used for pins input/output
#include <util/delay.h>    // For delay functions
#include <stdio.h>         // Used for printf function
#include "usart.h"         // For UART output to PC, debugging purposes
#include <avr/interrupt.h> // For interrupt handling

// for display things
#define BAUD 9600 
#define NUMBER_STRING 1001
#define BUTTON_STRING 1002
#define TIMER_STOP 1003

// Declare global variables
volatile float time = 0; // Declare volatile for interrupt updates
int need_speed;
int need_speed2;
int pwm_perc;
int timervar;
int hole_spinning_time = 0;
long int time_passed = 0;
int desired_distance = 0;
int desired_distance_2= 0;
int desired_time = 0;
int desired_time_2 =0;
int printstatement = 0;
int stage = 1; // 1: use need_speed, 2: use need_speed2
uint32_t num1 = 0, num2 = 0, num3 = 0, num4 = 0;

    
// Declare function prototypes
void setup_timer2_interrupt_8ms(void);
int calculate_speed(void);
void timers(void);
int calculate_hole_spinning_time(int timervar);
void adjust_speed(int mm_s);
void stop_timer0_if_time_reached(int desired_time);
void stop_program(void);
void adc_init(void);
unsigned int read_adc(void);
float read_volts(void);

// Timer2 interrupt service routine
ISR(TIMER2_COMPA_vect) {
    time += 0.008;// this counts up everytime that the timer2 has counted to the maximum "loft" created by the ocr2a being set to 
    //124, and at a prescaler of 1024, and 16mhz processer, it will accure at 0,008s or 8ms. we can therefore make a counter that keeps
    //time by just keep counting this up. Time is also a volitaile, meaning it can be changed at all times

}

int main(void) {
    // Initialize system
    uart_init();       // Initialize communication with PC
    io_redirect();
    
    handle_touch_events(&num1, &num2, &num3, &num4); //getting all the values from the display
    desired_time = num1;//putting all these numbers into our desired values, this is just to make it easier on ourselves when we are 
    desired_distance = num2*100;//handeling the rest of the progrma. 
    desired_time_2 = num3;
    desired_distance_2 = num4*100; // this is times by 100 to get it to be mm, instead of decimeters. 
    //printf("desired time %d",desired_time); these are commented out to make sure they dont interrupt our nextion display
    //printf("desired distance, %d",desired_distance);
    //printf("desired time2 %d",desired_time_2);
    //printf("desired distance2 %d",desired_distance_2);
    calculate_speed(); //calls the calculate speed distance. 
    setup_timer2_interrupt_8ms(); //setus up the interrup so that we can use that to keep time. 
    timers(); //sets up the rest of the timers. 
    adc_init(); // Initialize ADC for voltage reading
        
    
    while (1) {
        while (!(TIFR1 & (1 << ICF1))) {// this blocking loop will do nothing until it sees our octocoupler whell rotating and giving 
            //us feedback on the timings and the calculated speed at which we are going. 
        }
        TCNT1 = 0; //resets the flagæ 
        timervar = ICR1;       // Store captured time
        ICR1 = 0;              // Reset capture register
        TIFR1 |= (1 << ICF1);  // Reset the interrupt flag

        hole_spinning_time = calculate_hole_spinning_time(timervar); //puts the "calculated speed" from the octocoupler into a local 
        //variable, making it easier for us to use.  

        //printf("The time between holes: %d ns\n", hole_spinning_time);
        int mm_s = 26250 / hole_spinning_time; // Calculate speed in mm/s. 26,250 is in mm, but because "hole spining time" is in 
        // ms, we have to times this by 1000. this is easier just to on the left side, because then we dont have to have mms as a float. 
        //printf("Speed in mm/s: %d\n", mm_s);

        adjust_speed(mm_s); // Call the function to adjust speed // calls the function to adjust the speed, if mms is not hitting the 
        //desired speeds, it can adjust it slightly at a time to make sure that we actually hit what we want to hit. 

        // Stop Timer0 if the desired time is reached
        stop_timer0_if_time_reached(desired_time);//this is actually a dumb name, because it wont stop timer0, but rather
        //it will end stage 1 of our (desired time and desired distance) and now switch the statements in our "adjust speed"
        //this is done so we can hit the targets of the mms, on each "stage" because they are ofc not equal. 

        //printf("Time elapsed: %.3f seconds\n", time);
        if (time >= desired_time + desired_time_2) {
            stop_program(); // this one actually stops the program, sorry for the confusion. 
        }

        // Read voltage from ADC
        float voltage = (read_volts())*(3.18);//this just takes the read voltage and times it by a constant determent by our ratio
        //of the voltage devider. this constant is a guesstimate, just like the voltage devider is. 
        //printf("Voltage: %.2f V\n", voltage);
        printstatement++;

        if(printstatement >= 32){
        unsigned int mili_voltage = voltage*100; //this converts it to a milivoltage, which is easier to display on the nextion display
        printf("page3.n0.val=%d%c%c%c", mili_voltage, 255, 255, 255);//(because milivoltage is an int and not a float.)
        printstatement = 0; 
        }
    }

    return 0;
}

int calculate_speed(void) {
    int max_mms = 90;

    need_speed = desired_distance / desired_time; // our need for speed is determend by the distance in mm / the time in seconds. 
    //this just gives us a nice number to use in our adjust speed, function.
    need_speed2 = desired_distance_2 / desired_time_2;

    pwm_perc = ((need_speed * 100) / max_mms) * 2.55;//this is a function we made to determend the pwm signal that we need. 
    //its doing so by taking the need for speed*100 / and then divding this by the maximum speed. this will give us the procentage 
    //of the pwm that we need to hit. afterwards we times it by 2.55, making it into the timer0's maximum 
    //OCR0A value. meaning that we can calculate a start value for the pwm signal, which we then can adjust afterwords. 
    OCR0A = pwm_perc;
    printf("page3.t4.txt=\"%s\"%c%c%c", "Accelerating", 255, 255, 255);
    return pwm_perc;//actually not needed, but i dont dare ruin the program by removing it. 
    
}

int calculate_hole_spinning_time(int timervar) {// just a function to calculate the time between each hole. this is taking the 
// prescaler and the mhz that the processor is running at. 
//therefore we can cacluate the time for it. 
    return timervar * (1024.0f / 16000.0f);
}

void adjust_speed(int mm_s) {
    int target_speed = (stage == 1) ? need_speed : need_speed2;//switching between each states of the need for speeds 
    //this could have been done by easy to use if statements instead. but this looks cleaner, and works everytime. 

    if (target_speed < mm_s) {
        OCR0A -= 3;
        //if (OCR0A <= 5) {
          //  OCR0A = 11; // Minimum value for OCR0A we have commented this out because it doesent matter if it runs below 10 ish
          // pwm, because then it dosent have enough energy to move, and therefore it cannot run the main code
          // because that requries the encoder wheel to move before the main code runs.
        //}
    } else {
        OCR0A += 5;
        if (OCR0A >= 250) {
            OCR0A = 244; // Maximum value for OCR0A // because if we go over 255, the code breaks. this is basically just a bug that 
            //we fixed. this is the way we did it. is it the correct way? maybe not. but this works. 
        }
    }
}

void timers(void) {
    DDRB = 0x00; // PINB0 as input for ICP1
    PORTB = 0x01; // Enable pull-up on PINB0

    TCCR1A = 0x00; // Normal mode
    TCCR1B = (1 << ICNC1) | (1 << ICES1) | (1 << CS12) | (1 << CS10); // all the bits needed for timer 1 to work, and with prescalers
    // + having debounce on the input. 

    DDRD |= 0x60; // Configure PWM pins

    TCCR0A |= 0xA3; // Fast PWM
    TCCR0B |= 0x05; // Prescaler 1024
    OCR0A = 128;
    TCNT0 = 0;
}

void setup_timer2_interrupt_8ms(void) {
    TCCR2A = (1 << WGM21);                  // CTC mode
    TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20); // Prescaler 1024
    OCR2A = 124;                            // Compare match value aka when we hit this, it interups our timer. 
    //when it hits 124 we know that
    // 8 ms has pased, and therefore we can keep time this way. not the most effectient way to keep time.
    //because if you used timer 1, you dont interrupt the program asmuch, therefore keeping time more precis. 
    TIMSK2 = (1 << OCIE2A);                 // Enable Timer2 interrupt
    sei();                                  // Enable global interrupts
}

void stop_timer0_if_time_reached(int desired_time) { // actually the wrong name for it. but i dont dare to switch anything around anymore 
//this works, so im not gonna fuck anything over by switching names. this is just and old function that i reused for the 
//switching of the stages. 
    if (time >= desired_time) {
        if (stage == 1) {
            stage = 2; // Switch to the second speed target
            //printf("Switching to need_speed2\n");
            // i guess this is pretty self explaining, i dont really need to explain this. 
        }
    }
}

void stop_program(void) {
    // Stop Timer0 and reset OCR0A
   
    OCR0A = 1;//setting the timer0 to 1, sometimes setting it to 0 gave me bugs sometimes :/ 
     TCCR0B &= ~((1 << CS02) | (1 << CS01) | (1 << CS00));// stopping the timers for the the timer0. 
    //printf("Program stopped. OCR0A set to 0\n");
    printf("page3.t4.txt=\"%s\"%c%c%c", "Breaking", 255, 255, 255); // displaying that we are now braking. 

     
}

void adc_init(void) { // all alins code that we got from his read voltage library. cant really explain most of this. 
//its basically the same as "read analog voltage" like in the arduino IDE. 
    ADMUX = (1 << REFS0); // Set reference voltage to AVCC
    ADCSRA = (1 << ADEN)  // Enable ADC
           | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler of 128
}

unsigned int read_adc(void) {
    ADCSRA |= (1 << ADSC); // Start conversion
    while (ADCSRA & (1 << ADSC)); // Wait for conversion to complete

    return ADCW; // Return the 10-bit ADC result
}

float read_volts(void) {
    unsigned int adc_value = read_adc();
    return (float)adc_value * (5.0 / 1023.0); // Convert ADC value to voltage
}

// Function to handle touch events and data retrieval
void handle_touch_events(uint32_t* num1, uint32_t* num2, uint32_t* num3, uint32_t* num4)
{
    unsigned char readBuffer[100]; // Buffer to hold incoming data
    uint32_t result1 = 0, result2 = 0;
    unsigned int typeExpected = 0;
    //int flag = 0;

    printf("page 0%c%c%c", 255, 255, 255); // Initialize at 9600 baud on Page 1
    
//if(flag != 1){

    {
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
                if (readBuffer[1] == 0x02 && readBuffer[2] == 0x08) { // Save button pressed on page 1
                    // Fetch page1.n0.val
                    printf("get page1.n0.val%c%c%c", 0xFF, 0xFF, 0xFF);
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
                            *num1 = readBuffer[1] | (readBuffer[2] << 8) | (readBuffer[3] << 16) | (readBuffer[4] << 24);
                        }
                    }

                    // Fetch page1.n1.val
                    printf("get page1.n1.val%c%c%c", 0xFF, 0xFF, 0xFF);
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
                            *num2 = readBuffer[1] | (readBuffer[2] << 8) | (readBuffer[3] << 16) | (readBuffer[4] << 24);
                        }
                    }

                    // Fetch page2.n0.val
                    printf("get page2.n0.val%c%c%c", 0xFF, 0xFF, 0xFF);
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
                            *num3 = readBuffer[1] | (readBuffer[2] << 8) | (readBuffer[3] << 16) | (readBuffer[4] << 24);
                        }
                    }

                    // Fetch page2.n1.val
                    printf("get page2.n1.val%c%c%c", 0xFF, 0xFF, 0xFF);
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
                            *num4 = readBuffer[1] | (readBuffer[2] << 8) | (readBuffer[3] << 16) | (readBuffer[4] << 24);
                        }
                    }
                }
            }
        }

    
    }

}
