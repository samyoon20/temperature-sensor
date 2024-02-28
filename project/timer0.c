#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include "timer0.h"
#include "project.h"

void timer0_init()
{
    // In Task 7, add code to inititialize TIMER1, but don't start it counting
	//CTC mode
	TCCR0A |= (1 << WGM01);

	//enables interrupts
	TIMSK0 |= (1 << OCIE0A);
}
ISR(TIMER0_COMPA_vect)
{
    // In Task 7, add code to change the output bit to the buzzer, and to turn
    // off the timer after enough periods of the signal
	// changes output bit to get it going
	PORTB ^= (1 << PB5);
	// decrement count
	time--;

	// turns off timer after enough periods have happened
	if (time == 0) {
		TCCR0B &= ~((1 << CS02) | (1 << CS01) | (1 << CS00));
	}
}