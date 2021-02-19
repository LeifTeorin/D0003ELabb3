/*
 * Labb 3 fast 2.c
 *
 * Created: 2021-02-15 14:01:08
 * Author : hjall
 * Author : crill
 */ 

#include "tinythreads.h"
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

int timekeeper = 1;
int buttonpress = 0;
int buttoncount = 0;
int lastvalue = 0;
int light = 0;

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
	LCDCRB = 0xb7; // 1/3 bias och 1/4 duty, asynk-klockan anv�nds och 25 segment anv�nds
	LCDCCR |= 15; // s�tter kontrastkontrollen till 3,35 V
	LCDFRR = 7;	// s�tter prescalern och ger framerate 32 Hz
}

void writeChar(char ch, int pos){
	if((pos>5) | (pos<0)){
		return;
	}
	int shift;
	char mask_reg;
	char currbyte = 0x00;
	int character = 0;
	char *ptr;
	ptr  = &LCDDR0; // pekaren b�rjar p� lcddr0
	
	if((int)ch > -1 && (int)ch < 10){
		character = characters[(int)ch];
	}
	
	
	if(pos & 0x01){ // om pos �r udda finns det en etta p� slutet, d� ska den skrivas p� de fyra bitarna till v�nster p� registren
		mask_reg = 0x0f;
		shift = 4; // bitarna som ska skrivas beh�ver �ven flyttas f�r att skrivas p� v�nster sida
		}else{
		mask_reg = 0xf0;
		shift = 0;
	}
	ptr = ptr + (pos>>1); // pekaren hamnar p� lcddr0, 1 eller 2 beroende p� pos
	
	for(int i = 0; i < 4; i++){
		currbyte = (character & 0x0f); // vi tar de 4 bittarna l�ngst till h�ger
		currbyte = currbyte << shift; // bittarna shiftas antingen 0 eller 4 steg �t v�nster
		*ptr = ((*ptr & mask_reg)|currbyte); // registret maskas och fylls sedan in med de bittarna vi h�mtat
		character = (character>>4); // vi tar bort de 4 bittarna till h�ger
		ptr += 5; // pekaren g�r fem register fram f�r n�sta 4 bittar
	}
}

void writeLong(long i){
	long y = i;
	char ch;
	int cntr = 5;
	while(y>0){ // s� l�nge talet �r st�rrer �n noll eller om vi inte skrivit 6 siffror ska en siffra skrivas
		if(cntr<0){
			return;
		}
		ch = y%10; // modulo 10 f�r att f� ut siffran till h�ger
		writeChar(ch, cntr); // skriv siffran till h�ger
		y = y/10; // vi tar bort siffran l�ngst till h�ger
		cntr--; // vi b�rjar till h�ger och g�r sedan �t v�nster d�rifr�n
	}
	
}

void printAt(long num, int pos) {
	int pp = pos;
	writeChar( ((num % 100) / 10), pp);
	pp++;
	writeChar( (num % 10), pp);
}

int is_prime(long i){
	for(int x = 2; x < i; x++){
		if(i%x == 0){
			return 0;
		}
	}
	return 1;
}

void primes(int i){
	for(int x = 2; x < i; x++){
		if(is_prime(x)){
			printAt(x, 0);
		}
	}
}

void blink(void){
	if(light){
		printAt(88, 2);
	}else{
		printAt(77, 2);
	}
	light = ~light;
}

void button(void)
{
	if (lastvalue) // vi ser om det ena l�get �r p�
	{
		printAt(buttoncount, 4);
	}
	else // annars �r det ju det andra l�get
	{
		printAt(buttoncount, 4);
	}
	lastvalue = ~lastvalue;
	buttoncount +=1;
	
}

int main(void)
{
	/* Replace with your application code */
	CLKPR = 0x80;
	CLKPR = 0x00;
	LCD_init();
	PORTB = (1<<PB7);
	EIMSK = 0x80;
	PCMSK1 = 0x80;
	TCCR1A = 0xC0;
	TCCR1B = 0x18;
	

	TCCR1A = (1 << COM1A0) | (1 << COM1A1);
	TCCR1B = (1 << WGM12) | (1 << CS10) |(1 << CS12);
	OCR1A = 3906;
	TCNT1 = 0x0;
	
	TIMSK1 = (1 << OCIE1A);
	
	spawn(button, 3);
	spawn(blink, 2);
	primes(30000);
	while (1)
	{
	}
}


ISR(PCINT1_vect)
{
	if ((PINB >> 7) == 0)
	{
		spawn(button, 69);
	}
}

ISR(TIMER1_COMPA_vect)
{
	spawn(blink, 420);
}