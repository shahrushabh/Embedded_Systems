/**
 * util.c: utility functions for the Atmel platform
 * 
 * For an overview of how timer based interrupts work, see
 * page 111 and 133-137 of the Atmel Mega128 User Guide
 *
 * @author Zhao Zhang & Chad Nelson
 * @date 06/26/2012
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "util.h"

// Global used for interrupt driven delay functions
volatile unsigned int timer2_tick;
void timer2_start(char unit);
void timer2_stop();


/// Blocks for a specified number of milliseconds
void wait_ms(unsigned int time_val) {
	//Seting OC value for time requested
	OCR2=250; 				//Clock is 16 MHz. At a prescaler of 64, 250 timer ticks = 1ms.
	timer2_tick=0;
	timer2_start(0);

	//Waiting for time
	while(timer2_tick < time_val);

	timer2_stop();
}


// Start timer2
void timer2_start(char unit) {
	timer2_tick=0;
	if ( unit == 0 ) { 		//unit = 0 is for slow 
        TCCR2=0b00001011;	//WGM:CTC, COM:OC2 disconnected,pre_scaler = 64
        TIMSK|=0b10000000;	//Enabling O.C. Interrupt for Timer2
	}
	if (unit == 1) { 		//unit = 1 is for fast
        TCCR2=0b00001001;	//WGM:CTC, COM:OC2 disconnected,pre_scaler = 1
        TIMSK|=0b10000000;	//Enabling O.C. Interrupt for Timer2
	}
	sei();
}


// Stop timer2
void timer2_stop() {
	TIMSK&=~0b10000000;		//Disabling O.C. Interrupt for Timer2
	TCCR2&=0b01111111;		//Clearing O.C. settings
}


// Interrupt handler (runs every 1 ms)
ISR (TIMER2_COMP_vect) {
	timer2_tick++;
}




/// Initialize PORTC to accept push buttons as input
void init_push_buttons(void) {
	DDRC &= 0xC0;  //Setting PC0-PC5 to input
	PORTC |= 0x3F; //Setting pins' pull up resistors
}

/// Return the position of button being pushed
/**
 * Return the position of button being pushed.
 * @return the position of the button being pushed.  A 1 is the rightmost button.  0 indicates no button being pressed
 */
char read_push_buttons(void) {
	if( (PINC & 0b00100000 ) ==  0b00000000)
	{
		return 6;
	}
	else if ((PINC & 0b00010000 ) ==  0b00000000)
	{
		return 5;
	}
	else if ( (PINC & 0b00001000 ) ==  0b00000000)
	{
		return 4;
	}
	else if ( (PINC & 0b00000100 ) ==  0b00000000)
	{
		return 3;
	}
	else if ( (PINC & 0b00000010 ) ==  0b00000000)
	{
		return 2;
	}
	else if ( (PINC & 0b00000001) ==  0b00000000)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


/// Initialize PORTC for input from the shaft encoder
void shaft_encoder_init(void) {
	DDRC &= 0x3F;	//Setting PC6-PC7 to input
	PORTC |= 0xC0;	//Setting pins' pull-up resistors
}

/// Read the shaft encoder
/**
 * Reads the two switches of the shaft encoder and compares the values
 * to the previous read.  This function should be called very frequently
 * for the best results.
 *
 * @return a value indicating the shaft encoder has moved:
 * 0 = no rotation (switches did not change)
 * 1 = CW rotation
 * -1 = CCW rotation
 */
char read_shaft_encoder(void) {
	// Function variables
	static unsigned char old_value = 0b11000000;
	unsigned char new_value = (PINC & 0b11000000);
	char rotation = 0;					// Keeps track of the rotation.

	if (old_value == 0b11000000) {		// If the knob was in a groove
		if (new_value == 0b01000000)
		rotation = 1;
		if (new_value == 0b10000000)
		rotation = -1;
	}

	old_value = new_value;

	return rotation;
}



/// Initialize PORTE to control the stepper motor
void stepper_init(void) {
	DDRE |= 0xF0;  	//Setting PE4-PE7 to output
	PORTE &= 0x8F;  //Initial postion (0b1000) PE4-PE7
	wait_ms(2);
	PORTE &= 0x0F;  //Clear PE4-PE7
}

/// Turn the Stepper Motor
/**
 * Turn the stepper motor a given number of steps. 
 *
 * @param num_steps A value between 1 and 200 steps (1.8 to 360 degrees)
 * @param direction Indication of direction: 1 for CW and -1 for CCW 
 */
void  move_stepper_motor_by_step(int num_steps, int direction) {

	int steps = 0;								// Counting the number of steps motor has turned.
	unsigned char turner = 0b00010000;			// Represents the initial position for the stepper_motor.
	char masked_porte = (PORTE & 0b00001111);	// Masking PORTE for future use.
	while (steps <= num_steps)
	{
		if (direction == 1) // Detect the direction of rotation.
		{
			for(int i = 0; i < 5; i++)
			{
				PORTE = (masked_porte|turner);
				turner =  (turner << 1);
				if(turner == 0b00000000)
				{
					turner = 0b00010000 ;
				}
			}
			steps++;
		}
		else if(direction == -1) // Detect the direction of rotation.
		{
			for(int i = 0; i < 5; i++)
			{
				PORTE = (masked_porte|turner);
				turner =  (turner >> 1);
				if(turner == 0b00001000)
				{
					turner = 0b10000000;
				}
			}
			steps++;
		}
		wait_ms(2);
	}
}