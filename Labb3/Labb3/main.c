/*
 * Labb3.c
 *
 * Created: 2021-02-15 13:10:07
 * Author : hjall
 */ 

#include <avr/io.h>
#include <avr/portpins.h>
#include <stdint.h>
#include "tinythreads.h"

int characters[13] =
{
	0x1551,		// 0
	0x0118,		// 1
	0x1e11,		// 2
	0x1b11,		// 3
	0x0b50,		// 4
	0x1b41,		// 5
	0x1f41,		// 6
	0x4009,		// 7
	0x1f51,		// 8
	0x1b51,		// 9
	0x0f50,		// H
	0x1641,		// E
	0x1510		// J
};

void LCD_init(void){
	LCDCRA |= 0x80; // LCD enable
	LCDCRB = 0xb7; // 1/3 bias och 1/4 duty, asynk-klockan används och 25 segment används
	LCDCCR |= 15; // sätter kontrastkontrollen till 3,35 V
	LCDFRR = 7;	// sätter prescalern och ger framerate 32 Hz
}

void writeChar(char ch, int pos){
	if((pos>5) | (pos<0)){
		return;
	}
	int shift;
	char mask_reg;
	char currbyte = 0x00;
	int character = 0;
	volatile unsigned char *ptr;
	ptr = &LCDDR0;
	
	if((int)ch > -1 && (int)ch < 10){
		character = characters[(int)ch];
	}
	
	
	if(pos & 0x01){ // om pos är udda finns det en etta på slutet, då ska den skrivas på de fyra bitarna till vänster på registren
		mask_reg = 0x0f;
		shift = 4; // bitarna som ska skrivas behöver även flyttas för att skrivas på vänster sida
		}else{
		mask_reg = 0xf0;
		shift = 0;
	}
	ptr = ptr + (pos>>1); // pekaren hamnar på lcddr0, 1 eller 2 beroende på pos
	
	for(int i = 0; i < 4; i++){
		currbyte = (character & 0x0f); // vi tar de 4 bittarna längst till höger
		currbyte = currbyte << shift; // bittarna shiftas antingen 0 eller 4 steg åt vänster
		*ptr = ((*ptr & mask_reg)|currbyte); // registret maskas och fylls sedan in med de bittarna vi hämtat
		character = (character>>4); // vi tar bort de 4 bittarna till höger
		ptr += 5; // pekaren går fem register fram för nästa 4 bittar
	}
}

void writeLong(long i){
	long y = i;
	char ch;
	int cntr = 5;
	while(y>0){ // så länge talet är störrer än noll eller om vi inte skrivit 6 siffror ska en siffra skrivas
		if(cntr<0){
			return;
		}
		ch = y%10; // modulo 10 för att få ut siffran till höger
		writeChar(ch, cntr); // skriv siffran till höger
		y = y/10; // vi tar bort siffran längst till höger
		cntr--; // vi börjar till höger och går sedan åt vänster därifrån
	}
	
}

int is_prime(long i){
	for(int x = 2; x < i; x++){
		if(i%x == 0){
			return 0;
		}
	}
	return 1;
}

void primes(long i){
	for(int x = 2; x < i; x++){
		if(is_prime(x)){
			writeLong(x);
		}
	}
}

void blink(int something){
	int light = 0; // light bestämmer om lampan är av eller på
	unsigned short time = 3906; // 8000000/1024 = 7813, för en sekund, alltså 3906 för en blinkning
	// short är 2 byte, precis som timern, alltså kommer den att börja om på noll lika fort som timerregistret
	//TCNT1 = 0x0000;
	
	while(1){
		
		if(timekeeper >= time){
			if(light){
				LCDDR0 = LCDDR0 & 0x99; // om den är på slår vi av den
				}else{
				LCDDR0 = LCDDR0 | 0x60; // annars slår vi på den
			}
			light = ~light; // vi ändrar light för att indikera att lampan är av/på
			time += 3906;
		}
		
	}
	
}

void button(int something)
{
	int buttonpress = 0;
	LCDDR0 |= 0x06;
	while(1)
	{
		if (!(PINB&0x80) && buttonpress == 0) // PINB7 = 0, när den är intryckt
		{									  // vi byter läge endast då knappen är nedtryckt och den nyss inte var det
			buttonpress = 1;
			if ((LCDDR0&0x06)) // vi ser om det ena läget är på
			{
				LCDDR0 = LCDDR0 & 0xf9; // slår av
				LCDDR1 = LCDDR1 | 0x60; // slår på
			}
			else // annars är det ju det andra läget
			{
				LCDDR0 = LCDDR0 | 0x06; // slår på
				LCDDR1 = LCDDR1 & 0x9f; // slår av
			}
		}
		if((PINB&0x80) && buttonpress == 1) // om den inte är nedtryckt blir buttonpress noll
		{
			buttonpress = 0;
		}

		
		
	}
}

int main(void)
{
	/* Replace with your application code */
	CLKPR = 0x80;
	CLKPR = 0x00;
	
	LCD_init();
	spawn(button, 1);
	spawn(blink, 0);
	primes(30000);
	
	while (1)
	{
	}
}
