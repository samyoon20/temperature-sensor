// Temperature Sensor

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include "lcd.h"
#include "ds18b20.h"

#include "project.h"
#include "timer1.h"
#include "timer0.h"

void play_note(uint16_t);
void variable_delay_us(int delay);

uint8_t a = 0;
uint8_t b = 0;

uint8_t new_state = 0;
uint8_t old_state = 0;
volatile uint16_t time = 0;
volatile uint8_t changed = 1;
unsigned char highTemp = 50;
unsigned char lowTemp = 50;

//keeps track of the high and low counts for the potentiometer separately
volatile int16_t highCount = 0;
volatile int16_t lowCount = 0;

//keeps track of which potentiometer measurement is being changed
// 0 is low and 1 is high
volatile uint16_t highOrLow = 1;

// to check if buzzer should be played or not
volatile uint16_t buzz = 0;

//servo modes
// 0 is default, 1 is low temperature, 2 is high temperature
volatile uint16_t servo = 0;


// unsigned char = low, high, turn;
int main(void) {

    // Initialize DDR and PORT registers and LCD
	// enable the inputs

	// specifically the buttons
	PORTD |= ((1 << 2) | (1 << 3));
	// specifically the led's
	PORTC |= ((1 << 3) | (1 << 4) | (1 << 5));
	DDRC |= ((1 << 3) | (1 << 4) | (1 << 5));
	// specifically the rotary encoder
	PORTC |= ((1 << PC1) | (1 << PC2));
	DDRC &= ~((1 << 1) | (1 << 2));
	PCICR |= (1 << PCIE1);
	PCMSK1 |= ((1 << PCINT9) | (1 << PCINT10));


	//specifically buzzer
	DDRB |= (1 << PB5);

	//specifically servo motor
	DDRB |= (1 << PB3);

	lcd_init();
	// declare the timer
	timer0_init();
	timer1_init();
	timer2_init();

	// allows the global interrupts	
	sei();

    // Write a spash screen to the LCD
	// and to clear the screen
	lcd_writecommand(0x01);
	// to center the words on the screen
	lcd_moveto(0, 2);
	// for centering on second row
	lcd_moveto(1, 2);
	lcd_stringout("Samuel Yoon");

	// delay
	_delay_ms(1000);
	lcd_writecommand(0x01);

	lcd_moveto(0, 0);
	lcd_stringout("Temp:");
	lcd_moveto(1, 0);
	lcd_stringout("Low=");
	lcd_moveto(1, 8);
	lcd_stringout("High=");
	_delay_ms(1000);

	
	// display counts
	char buffer[10];

	// Read the A and B inputs to determine the intial state.
    // In the state number, B is the MSB and A is the LSB.
    // Warning: Do NOT read A and B separately.  You should read BOTH inputs
    // at the same time, then determine the A and B values from that value.    
	int x = PINC;
	
	if (x & (1 << 1)) {
        a = 1;
    }
    else {
        a = 0;
    }

    if (x & (1 << 2)) {
        b = 1;
    }
    else {
        b = 0;
    }
    
    // b = 0 and a = 0
    if (!b && !a)
    old_state = 0;
    // b = 0 and a = 1
    else if (!b && a)
    old_state = 1;
    // b = 1 and a = 0  
    else if (b && !a)
    old_state = 2;
    // b = 1 and a = 1
    else
    old_state = 3;

    new_state = old_state;

    int setTemp = -1;

    unsigned char t[2];

    if (ds_init() == 0) {    // Initialize the DS18B20
        // Sensor not responding
    }

    ds_convert();    // Start first temperature conversion
	//enables temperature sensor

	a = ((x & (1 << PC1)) >> PC1);
	b = ((x & (1 << PC2)) >> PC2);
    if (!b && !a)
	old_state = 0;
    else if (!b && a)
	old_state = 1;
    else if (b && !a)
	old_state = 2;
    else
	old_state = 3;

    new_state = old_state;

	// read from eeprom to show high/low temps
	lowTemp = eeprom_read_byte((void *) 100);
	highTemp = eeprom_read_byte((void *) 101);

	//verification
	if (lowTemp < 50 | lowTemp > 90) {
		lowTemp = 50;
	}
	if (highTemp < 50 | highTemp > 90) {
		highTemp = 50;
	}

	highCount = highTemp - 50;
	lowCount = lowTemp - 50;

	// to display the initial temperatures
	char initialBuffer[10];
	//high temp
	lcd_moveto(1, 13);
	snprintf(initialBuffer, 10, "%3d", highTemp);
	lcd_stringout(initialBuffer);

	//low temp
	lcd_moveto(1, 4);
	snprintf(initialBuffer, 10, "%3d", lowTemp);
	lcd_stringout(initialBuffer);

	
	
	int highSetting = 90;
	int lowSetting = 50;
	uint16_t alert = 0;

    while (1) {         
		int number, decimal, high, low;
		// Loop forever
		if (ds_temp(t)) {
			lcd_moveto(0, 6);
			uint32_t toPrint = (t[1] << 8) + t[0];

			toPrint = (((toPrint * 9) / 5) + 32 * 16);
			number =  toPrint / 16;
			decimal = (((toPrint % 16) * 10) / 16);
			snprintf(buffer, 16, "%d.%d", number, decimal);

			lcd_stringout(buffer);

			ds_convert();
		}
		_delay_ms(100);

		if ((PIND & (1 << 2)) == 0) {
			lcd_moveto(1, 3);
			lcd_stringout("=");

			lcd_moveto(1, 12);
			lcd_stringout("*");

			servo = 1;
			TCCR1B |= ((1 << CS12) | (1 << CS10));

			setTemp = 0;
			highOrLow = 0;
		}
		else if ((PIND & (1 << 3)) == 0) {
			lcd_moveto(1, 3);
			lcd_stringout("*");

			lcd_moveto(1, 12);
			lcd_stringout("=");

			servo = 2;
			TCCR1B |= ((1 << CS12) | (1 << CS10));


			setTemp = 1;
			highOrLow = 1;
		}
		_delay_ms(100);

        if (changed) { // Did state change?
            changed = 0;        // Reset changed flag

            // Output count to LCD
            if (setTemp) {
				highTemp = 50 + highCount;
                lcd_moveto(1, 13);
				snprintf(buffer, 10, "%3d", highTemp);
                lcd_stringout(buffer);
				unsigned char h = highTemp;
				eeprom_update_byte((void *) 101, h);
            }
            else {
				lowTemp = 50 + lowCount;
                lcd_moveto(1, 4);
				snprintf(buffer, 10, "%3d", lowTemp);
                lcd_stringout(buffer);
				unsigned char l = lowTemp;
				eeprom_update_byte((void *) 100, l);
            }
        }

		if (number < lowTemp) {
            // turn heater on
            lcd_moveto(0, 11);
            lcd_stringout("HEAT");
            // LED RED
            PORTC |= ((1 << 3) | (1 << 4) | (1 << 5));
            PORTC &= ~(1 << 3);
        }
        else if (number > highTemp) {
            // turn A/C on
            lcd_moveto(0,11);
            lcd_stringout("A/C");
            // LED BLUE
            PORTC |= ((1 << 3) | (1 << 4) | (1 << 5));
            PORTC &= ~(1 << 5);
        }
        else {
            lcd_moveto(0, 11);
            lcd_stringout("    ");
            // LED GREEN
            PORTC |= ((1 << 3) | (1 << 4) | (1 << 5));
            PORTC &= ~(1 << 4);
        }
		// highsetting and low setting, will it be integer val
		// or will the value be ever-changing?
		// more than 3 values so just add 3, or less than 3 values
		// to sound the alert it is
		
		//manages if buzzer should be played
		if (buzz == 0) {
			if (number > lowTemp - 3 & number < highTemp + 3) {
				buzz = 1;
			}
		}
		if (buzz == 1) {
			if (number < lowTemp - 3 | number > highTemp + 3) {
				buzz = 2;
			}
		}
		if (buzz == 2) {
			play_note(400);
			buzz = 0;
		}

		//deals with servo motor
		//min at 12 max at 36
		//makes servo point to temperature

		if (servo == 0) {
			OCR2A = ((-24 * number/60) + 52);
		}
		else if (servo == 1) {
			OCR2A = ((-24 * lowTemp/60)+ 52);
		}
		else {
			OCR2A = ((-24 * highTemp/60) + 52);
		}

    }
}
	
/*
    variable_delay_us - Delay a variable number of microseconds
*/
void variable_delay_us(int delay)
{
    int i = (delay + 5) / 10;

    while (i--)
        _delay_us(10);
}

ISR(PCINT1_vect)
{
    // In Task 6, add code to read the encoder inputs and determine the new
    // count value

	old_state = new_state;

	// reads the current a and b inputs
	int x = PINC;
	a = ((x & (1 << PC1)) >> PC1);
	b = ((x & (1 << PC2)) >> PC2);

	if (old_state == 0) {
		// Handle A and B inputs for state 0
		if (a == 1) {
			new_state = 1;
			if (highOrLow == 0) {
				lowCount--;
			}
			else {
				highCount--;
			}
		}
		else if (b == 1) {
			new_state = 3;
			if (highOrLow == 0) {
				lowCount++;
			}
			else {
				highCount++;
			}
		}
	}
	else if (old_state == 1) {
		// Handle A and B inputs for state 1
		if (b == 1) {
			new_state = 2;
			if (highOrLow == 0) {
				lowCount--;
			}
			else {
				highCount--;
			}
		}
		else if(a == 0) {
			new_state = 0;
			if (highOrLow == 0) {
				lowCount++;
			}
			else {
				highCount++;
			}
		}
	}
	else if (old_state == 2) {
		// Handle a and b inputs for state 2
		if (a == 0) {
			new_state = 3;
			if (highOrLow == 0) {
				lowCount--;
			}
			else {
				highCount--;
			}
		}
		else if (b == 0) {
			new_state = 1;
			if (highOrLow == 0) {
				lowCount++;
			}
			else {
				highCount++;
			}

		}
	}
	else {
		// Handle A and B inputs for state 3
		if (b == 0) {
			new_state = 0;
			if (highOrLow == 0) {
				lowCount--;
			}
			else {
				highCount--;
			}
		}
		else if (a == 1) {
			new_state = 2;
			if (highOrLow == 0) {
				lowCount++;
			}
			else {
				highCount++;
			}
		}
	}

	if (highCount < 0) {
		highCount = 0;
	}
	if (highCount > 40) {
		highCount = 40;
	}

	if (lowCount < 0) {
		lowCount = 0;
	}
	if (lowCount > 40) {
		lowCount = 40;
	}

	if (new_state != old_state) {
		changed = 1;
		old_state = new_state;
	}
}

/*
  Play a tone at the frequency specified for one second
*/
void play_note(uint16_t freq) 
{	
	// sets how many times the timer at ISR happens
	time = 2 * freq;

    // set the count variable for timer
	TCCR0B |= (1 << CS02);
	OCR0A = 16000000 / (2 * freq);

}

void timer2_init() {
	TCCR2A |= (0b11 << WGM20); //modulus 256
	TCCR2A |= (0b10 << COM2A0);
	//OCR2A = 128;
	TCCR2B |= (0b111 << CS20); //prescaler = 1024 for 16ms period
}