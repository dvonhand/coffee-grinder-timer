#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#define DEBOUNCE_TICKS (F_CPU / 1024 / 100)
#define TICKS_PER_SECOND (F_CPU / 1024)
#define HALF_CARAFE_GRIND_SECONDS 18
#define FULL_CARAFE_GRIND_SECONDS 36
#define SINGLE_CUP_GRIND_SECONDS 3

volatile unsigned int counter = 0;
volatile char counterStep = 0;
volatile unsigned char digit = 0;

static inline void clearDisplay(void);
static inline void disableButtonTimer(void);
static inline void disableDebounceTimer(void);
static inline void disableDisplay(void);
static inline void enableButtonTimer(void);
static inline void enableDebounceTimer(void);
static inline void enableDisplay(void);
static inline void grinderOff(void);
static inline void grinderOn(void);
void main(void);
static inline void setupButtonDebounceTimer(void);
static inline void setupButtonTimer(void);
static inline void setupButtons(void);
static inline void setupDisplay(void);
static inline void setupGrinder(void);

void clearDisplay(void) {
	PORTB &= ~(_BV(PORTB2) | _BV(PORTB1) | _BV(PORTB0));
	PORTC &= ~(_BV(PORTC2) | _BV(PORTC1) | _BV(PORTC0));
	PORTD &= ~(_BV(PORTD7) | _BV(PORTD6) | _BV(PORTD5) | _BV(PORTD4) | _BV(PORTD3) | _BV(PORTD2));
}

void disableDebounceTimer(void) {
	TCCR2B &= ~(_BV(CS22) | _BV(CS20));
	TCNT2 = 0;
	GTCCR |= _BV(PSRASY);
	power_timer2_disable();
}

void disableDisplay(void) {
	TCCR0B &= ~(_BV(CS02) | _BV(CS00));
	TCNT0 = 0;
	power_timer0_disable();
	clearDisplay();
}

void disableButtonTimer(void) {
	TCCR1B &= ~(_BV(CS12) | _BV(CS10));
	digit = 0;
	TCNT1H = 0;
	TCNT1L = 0;
	GTCCR |= _BV(PSRSYNC);
	power_timer1_disable();
}

void enableButtonTimer(void) {
	power_timer1_enable();
	TCNT1H = 0;
	TCNT1L = 0;
	GTCCR |= _BV(PSRSYNC);
	TCCR1B |= _BV(CS12) | _BV(CS10);
}

void enableDebounceTimer(void) {
	power_timer2_enable();
	GTCCR |= _BV(PSRASY);
	TCNT2 = 0;
	TCCR2B |= _BV(CS22) | _BV(CS20);
}

void enableDisplay(void) {
	power_timer0_enable();
	TCNT0 = 0;
	digit = 0;
	TCCR0B |= _BV(CS02) | _BV(CS10);
}

void grinderOff(void) {
	PORTB &= ~_BV(PORTB3);
	disableButtonTimer();
	disableDisplay();
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

void grinderOn(void) {
	PORTB |= _BV(PORTB3);
	if (!(TCCR1B & _BV(CS12))) {
		enableButtonTimer();
		enableDisplay();
	}
}

void main (void) {
	wdt_disable();
	setupButtonDebounceTimer();
	setupButtonTimer();
	setupButtons();
	setupDisplay();
	setupGrinder();
	power_all_disable();
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	wdt_enable(WDTO_15MS);
	sei();

	while(1) {
		wdt_reset();
		sleep_enable();
		sleep_bod_disable();
		wdt_disable();
		sei();
		sleep_cpu();
		wdt_enable(WDTO_15MS);
		sleep_disable();
	}
}

void setupButtonDebounceTimer(void) {
	ASSR &= ~_BV(AS2);
	TCCR2A = _BV(WGM21);
	TCCR2B = 0;
	TCNT2 = 0;
	OCR2A = DEBOUNCE_TICKS;
	GTCCR |= _BV(PSRASY);
	TIMSK2 |= _BV(OCIE2A);
}

void setupButtonTimer(void) {
	TCCR1A = 0;
	TCCR1B = _BV(WGM12);
	OCR1AH = (TICKS_PER_SECOND / 10) >> 8;
	OCR1AL = (TICKS_PER_SECOND / 10) && 0xFF;
	TCNT1H = 0;
	TCNT1L = 0;
	TIMSK1 |= _BV(OCIE1A);
	GTCCR |= _BV(PSRSYNC);
}

void setupDisplay(void) {
	DDRB |= _BV(DDB0) | _BV(DDB1) | _BV(DDB2);
	DDRC |= _BV(DDC0) | _BV(DDC1) | _BV(DDC2);
	DDRD |= _BV(DDD2) | _BV(DDD3) | _BV(DDD4) | _BV(DDD5) | _BV(DDD6) | _BV(DDD7);
	clearDisplay();

	TCCR0A = _BV(WGM01);
	TCCR0B = 0;
	OCR0A = 0x0F;
	TCNT0 = 0;
	TIMSK0 |= _BV(OCIE0A);
}

void setupButtons(void) {
	DDRB &= ~_BV(DDB4);
	PORTB |= _BV(PORTB4);

	DDRC &= ~(_BV(DDC5) | _BV(DDC4) | _BV(DDC3));
	PORTC |= _BV(PORTC5) | _BV(PORTC4) | _BV(PORTC3);

	PCMSK0 = _BV(PCINT4);
	PCMSK1 = _BV(PCINT13) | _BV(PCINT12) | _BV(PCINT11);
	PCICR |= _BV(PCIE1) | _BV(PCIE0);
}

void setupGrinder(void) {
	DDRB |= _BV(DDB3) | _BV(DDB5);
	PORTB &= ~(_BV(PORTB3) | _BV(PORTB5));
}

ISR(TIMER0_COMPA_vect) {
	unsigned char segment = 0;
	static unsigned int residual;

	wdt_enable(WDTO_15MS);
	wdt_reset();

	PORTB |= _BV(PORTB1) | _BV(PORTB0);
	PORTD |= _BV(PORTD2) | _BV(PORTD3) | _BV(PORTD4) | _BV(PORTD5) | _BV(PORTD6) | _BV(PORTD7);
	PORTB &= ~_BV(PORTB2);
	PORTC &= ~(_BV(PORTC2) | _BV(PORTC1) | _BV(PORTC0));

	if (digit == 0) {
		digit += 1;
		PORTC |= _BV(PORTC0);
		residual = counter;
		while (residual >= 1000) {
			residual -= 1000;
			segment += 1;
		}
	} else if (digit == 1) {
		digit += 1;
		PORTC |= _BV(PORTC1);
		while (residual >= 100) {
			residual -= 100;
			segment += 1;
		}
	} else if (digit == 2) {
		digit += 1;
		PORTC |= _BV(PORTC2);
		while (residual >= 10) {
			residual -= 10;
			segment += 1;
		}
		PORTD &= ~_BV(PORTD7);
	} else if (digit == 3) {
		digit = 0;
		PORTB |= _BV(PORTB2);
		segment = residual;
	}

	if (segment == 0) {
		PORTB &= ~_BV(PORTB0);
		PORTD &= ~(_BV(PORTD4) | _BV(PORTD2) | _BV(PORTD3) | _BV(PORTD5) | _BV(PORTD6));
	} else if (segment == 1) {
		PORTB &= ~_BV(PORTB0);
		PORTD &= ~_BV(PORTD4);
	} else if (segment == 2) {
		PORTB &= ~_BV(PORTB1);
		PORTD &= ~(_BV(PORTD2) | _BV(PORTD4) | _BV(PORTD5) | _BV(PORTD6));
	} else if (segment == 3) {
		PORTB &= ~(_BV(PORTB1) | _BV(PORTB0));
		PORTD &= ~(_BV(PORTD2) | _BV(PORTD4) | _BV(PORTD6));
	} else if (segment == 4) {
		PORTB &= ~(_BV(PORTB1) | _BV(PORTB0));
		PORTD &= ~(_BV(PORTD3) | _BV(PORTD4));
	} else if (segment == 5) {
		PORTB &= ~(_BV(PORTB1) | _BV(PORTB0));
		PORTD &= ~(_BV(PORTD2) | _BV(PORTD3) | _BV(PORTD6));
	} else if (segment == 6) {
		PORTB &= ~(_BV(PORTB1) | _BV(PORTB0));
		PORTD &= ~(_BV(PORTD2) | _BV(PORTD3) | _BV(PORTD5) | _BV(PORTD6));
	} else if (segment == 7) {
		PORTB &= ~_BV(PORTB0);
		PORTD &= ~(_BV(PORTD4) | _BV(PORTD2));
	} else if (segment == 8) {
		PORTB &= ~(_BV(PORTB1) | _BV(PORTB0));
		PORTD &= ~(_BV(PORTD4) | _BV(PORTD2) | _BV(PORTD3) | _BV(PORTD5) | _BV(PORTD6));
	} else if (segment == 9) {
		PORTB &= ~(_BV(PORTB1) | _BV(PORTB0));
		PORTD &= ~(_BV(PORTD4) | _BV(PORTD2) | _BV(PORTD3) | _BV(PORTD6));
	}
}

ISR(TIMER1_COMPA_vect) {
	wdt_enable(WDTO_15MS);
	wdt_reset();

	if (counter > 0 && counterStep < 0) {
		counter -= 1;
	} else if (counterStep > 0) {
		counter += 1;
	}

	if (counter == 0 && counterStep < 0) {
		grinderOff();
	}
}

ISR(TIMER2_COMPA_vect) {
	wdt_enable(WDTO_15MS);
	wdt_reset();

	disableDebounceTimer();

	unsigned char pins = PINC;
	if ((pins ^ _BV(PINC3)) & _BV(PINC3)) {
		counter = 0;
		counterStep = 1;
	} else if ((pins ^ _BV(PINC5)) & _BV(PINC5)) {
		counter += FULL_CARAFE_GRIND_SECONDS * 10;
		counterStep = -1;
	} else if ((pins ^ _BV(PINC4)) & _BV(PINC4)) {
		counter += HALF_CARAFE_GRIND_SECONDS * 10;
		counterStep = -1;
	} else if ((PINB ^ _BV(PINB4)) & _BV(PINB4)) {
		counter += SINGLE_CUP_GRIND_SECONDS * 10;
		counterStep = -1;
	} else if (counterStep == 1) {
		counter = 0;
		counterStep = 0;
	}

	if (counterStep != 0) {
		grinderOn();
	} else {
		grinderOff();
	}
}

ISR(PCINT1_vect) {
	wdt_enable(WDTO_15MS);
	wdt_reset();

	set_sleep_mode(SLEEP_MODE_IDLE);
	enableDebounceTimer();
}

ISR(PCINT0_vect) {
	wdt_enable(WDTO_15MS);
	wdt_reset();

	set_sleep_mode(SLEEP_MODE_IDLE);
	enableDebounceTimer();
}
