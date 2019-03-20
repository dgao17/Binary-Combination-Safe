#ifndef F_CPU
#define F_CPU 1000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define LED_PIN0 (1 << PB0)
#define LED_PIN1 (1 << PB1)
#define LED_PIN2 (1 << PB2)
#define LED_RED 0b101
#define LED_BLUE 0b011

#define KEY_PIN0 (1 << PB3)
#define KEY_PIN1 (1 << PB4)

typedef enum { RELEASED = 0, PRESSED } key_t;
typedef enum { UNLOCKED = 0, LOCKED } safeState; 


volatile uint8_t passcode = 0;
volatile uint8_t unlockPW = 0;
uint8_t counter = 0; 	
uint8_t prev = 1;
uint8_t buttonPressed = 1;

uint8_t history = 0;
key_t keystate = RELEASED; 
safeState currentState = UNLOCKED; 	

ISR(TIMER1_COMPA_vect) {
    // Get a sample of the key pin
    history = history << 1;
    if ((PINB & KEY_PIN0) == 0 | (PINB & KEY_PIN1) == 0 ) // low if button is pressed!
      history = history | 0x1;
    
    // Update the key state based on the latest 8 samples
    if ((history & 0b11111111) == 0b11111111)
      keystate = PRESSED;
    if ((history & 0b00111111) == 0)
      keystate = RELEASED;

    if(keystate != prev) {
	if(!buttonPressed) {		// if button wasn't already pressed 		
	    if(counter < 7) { 
		counter++;		// increment counter & obtain passcode if unlocked
		if(currentState == UNLOCKED) {
			if((PINB & KEY_PIN1) == 0) 
				passcode = (1 | passcode);
			if((PINB & KEY_PIN0) == 0) 
				passcode = (passcode << 1);
		}
		if(currentState == LOCKED) {		// if locked, get unlock passcode
			if((PINB & KEY_PIN1) == 0) 
				unlockPW = (1 | unlockPW);
			if((PINB & KEY_PIN0) == 0) 
				unlockPW = (unlockPW << 1);
		}
			
	    } else {
		counter = 0;
	    }
	    if(currentState == UNLOCKED) {
    	    	DDRB = LED_PIN0|LED_PIN1|LED_PIN2;
	    	PORTB = LED_PIN1;				// flash blue LED for button pressed
	    	buttonPressed = 1;
	    } 
	    if(currentState == LOCKED) {
		// if locked, keep red on & flash blue
		//DDRB = LED_PIN0|LED_PIN1|LED_PIN2;
		_delay_ms(1);
		PORTB = LED_BLUE;
		buttonPressed = 1;
		
	    }
		} else {
			if(currentState == UNLOCKED) {
				DDRB = LED_PIN0|LED_PIN1;	// not pressed & unlocked
				_delay_ms(1);
	  			PORTB ^= LED_PIN0|LED_PIN1;	
				buttonPressed = 0;
			} else if(currentState == LOCKED && keystate == RELEASED) {	// not pressed & locked
				DDRB = LED_PIN1|LED_PIN2;
				PORTB = LED_PIN2;
				buttonPressed = 0;
			}
		}
}
	prev = keystate;
	
}

void wrong() {
	PORTB = 0;
	_delay_ms(300);			// blink yellow 
	DDRB = LED_PIN0|LED_PIN1;
	PORTB = LED_PIN1;
	_delay_ms(300);
	PORTB = 0;
	_delay_ms(300);
	DDRB = LED_PIN0|LED_PIN1;
	PORTB = LED_PIN1;
	_delay_ms(300);
	PORTB = 0;
	_delay_ms(300);
	DDRB = LED_PIN0|LED_PIN1;
	PORTB = LED_PIN1;
	_delay_ms(500);


	DDRB = LED_PIN1|LED_PIN2;	// turn red back on 
	PORTB = LED_PIN2;
}

int main(void) {
	DDRB = LED_PIN0|LED_PIN1;
  	PORTB = LED_PIN1;
	TCCR1 |= (1 << CTC1);					// clear the timer on match
	TCCR1 |= (1 << CS12) | (1 << CS11) | (1 << CS10);	// prescale 4
	OCR1C = 50;						// trigger interrupt every 200 microseconds
	TIMSK |= (1 << OCIE1A);					// enable interrupts  	
	sei();

  	while (1) {
	  if(currentState == UNLOCKED) { 
	 	_delay_ms(1);
	  	PORTB ^= LED_PIN0|LED_PIN1;		// if unlocked, turn on yellow & green
	  }

	  if(currentState == LOCKED) {
		_delay_ms(1);
		DDRB = LED_PIN1|LED_PIN2;		// if locked, turn on red 
		PORTB = LED_RED;
	  }

	  if(counter == 6 && currentState == UNLOCKED) {  // if unlocked & pw is keyed in, then
		DDRB = LED_PIN1|LED_PIN2;		  // turn on red only 
		PORTB = LED_PIN2;		
		currentState = LOCKED;
		counter = 0;
	  }	
	  if(counter == 6 && currentState == LOCKED) {	// if locked & pw is keyed in, then
		if(passcode == unlockPW) {		// check if password matches
			currentState = UNLOCKED;	// yes: turn green and yellow back on
			DDRB = LED_PIN0|LED_PIN1;
  			PORTB = LED_PIN1;
			_delay_ms(1);
	  		PORTB ^= LED_PIN0|LED_PIN1;	// if unlocked, turn on yellow & green
			counter = 0;
			passcode = 0;			// reset passcode
			unlockPW = 0;			// reset unlockpw
		} else {				// no: flash yellow LED only
			wrong();
			counter = 0;
			unlockPW = 0;
		}
	}
      }
  	
}


