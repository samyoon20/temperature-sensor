#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include "timer1.h"
#include "project.h"

void timer1_init() {
	TCCR1B |= (1 << WGM12);
	TIMSK1 |= (1 << OCIE1A);
	OCR1A = 62500;
}

ISR(TIMER1_COMPA_vect) {

	// turns off timer after enough periods have happened
	TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));
	
	servo = 0;
}