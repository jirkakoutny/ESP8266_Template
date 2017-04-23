// Obsluha LED signalizace
#include <Arduino.h>
#include "led.h"

LED::LED(int pin, int ledon, int ledoff)
{
	_pin = pin;
	_ledon = ledon;
	_ledoff = ledoff;
}

void LED::rtLed(void)
{

	switch (_state)
	{
	case LS_RUN:
	{
		uint8_t instr;

		if (NULL != _signal)
		  instr = *_ptr; // instrukce
		else if (NULL != _psignal)
		  instr = pgm_read_byte(_pptr);
		else
		  instr = LEDS_STOP;
		switch (instr & 0xc0)
		{
		case LEDS_ONFOR:
		  digitalWrite(_pin, _ledon);
		  _timer = 10ul * ((instr & 0x3F) + 1);
		  _state = LS_WAIT;
		break;

		case LEDS_OFFFOR:
		  digitalWrite(_pin, _ledoff);
		  _timer = 10ul * ((instr & 0x3F) + 1);
		  _state = LS_WAIT;
		break;

		case LEDS_STOP:
		  _state = LS_IDLE;
		break;

		case LEDS_RESTART:
		  _ptr = _signal;
		  _pptr = _psignal;
		break;
		}
	}
	break;

	case LS_WAIT:
	  --_timer;
	  if (0 == _timer)
	  {
	  	_state = LS_RUN;
	  	++_ptr;
	  	++_pptr;
	  }
	break;

	default:
	break;
	}
}

void LED::begin(void)
{

  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, _ledoff);
  _state - LS_IDLE;
//  _handler.attach_ms(10, lh, static_cast<void *>(this));
  _handler.attach_ms(10, lh, this);
}

void LED::set(const uint8_t *signal)
{

	noInterrupts();
	_signal = signal;
	_ptr = _signal;
	_psignal = NULL;
	_state = LS_RUN;
	interrupts();
}

//void LED::set(const __FlashStringHelper *signal)
void LED::set(PGM_P signal)
{
	noInterrupts();
//	_psignal = reinterpret_cast<PGM_P>(signal);
	_psignal = signal;
	_pptr = _psignal;
	_signal = NULL;
	_state = LS_RUN;
	interrupts();
}

void LED::start()
{

	noInterrupts();
	_ptr = _signal;
	_pptr = _psignal;
	_state = LS_RUN;
	interrupts();
}

//void LED::lh(void *ptr)
void LED::lh(LED *ptr)
{
//	LED *pled = static_cast<LED *>(ptr);
	LED *pled = ptr;
	pled->rtLed();
}
