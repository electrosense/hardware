#include <stdint.h>

#include <avr/io.h> 
#include <util/delay.h> 

#define DATA	(PIN0)
#define SEL 	(PIN1)
#define CLK	(PIN2)

#define DELAY_LEN 10
void writeTuner(uint8_t address, uint32_t data){
	
	//ADR[0-3] R/W DAT[0-19]
	data<<=5;
	data|=(1<<4);
	data|=address & (0xF);
	PORTA &=~ _BV(SEL);
	uint8_t i;
	for(i=0; i<25; i++){
		PORTA &=~ _BV(CLK);
		if(data & 1){
			PORTA |= _BV(DATA);
		}else{
			PORTA &=~ _BV(DATA);
		}
		data>>=1;
		_delay_us(DELAY_LEN);
		PORTA |= _BV(CLK);
		_delay_us(DELAY_LEN);
	}
	PORTA &=~ _BV(CLK);
	_delay_us(DELAY_LEN);
	PORTA |= _BV(SEL);
	_delay_us(DELAY_LEN);
}

void tuneToFrequency(uint16_t freq){
	writeTuner(0, 8);

	freq/=2;

	uint32_t n = freq/32;
	uint16_t a = freq-n*32;
	
	writeTuner(1, (n<<7)|a);
}

int main(){

	DDRB=_BV(PIN0)|_BV(PIN1);
	DDRA = _BV(DATA) | _BV(SEL) | _BV(CLK);
	PORTA |= _BV(SEL);
	PORTA &=~ _BV(CLK);

	_delay_ms(100);
	tuneToFrequency(4900);

	while(1){
		_delay_ms(500);
		PORTB=_BV(PIN1);
		_delay_ms(500);
		PORTB=_BV(PIN0);
	}


	return 0;
}
