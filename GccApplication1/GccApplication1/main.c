/* 
 * AUTOMATED COMPOST MAKING MACHINE
 * GccApplication1.c
 * 
 * Author : Group 7
 */ 

#define F_CPU 4000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>


//////////LCD
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LCD_Dir  DDRD			/* Define LCD data port direction */
#define LCD_Port PORTD			/* Define LCD data port */
#define RS PD0				/* Define Register Select pin */
#define EN PD1 				/* Define Enable signal pin */

void LCD_Command( unsigned char cmnd )
{
	LCD_Port = (LCD_Port & 0x0F) | (cmnd & 0xF0); /* sending upper nibble */
	LCD_Port &= ~ (1<<RS);		/* RS=0, command reg. */
	LCD_Port |= (1<<EN);		/* Enable pulse */
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);

	_delay_us(200);

	LCD_Port = (LCD_Port & 0x0F) | (cmnd << 4);  /* sending lower nibble */
	LCD_Port |= (1<<EN);
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);
	_delay_ms(2);
}

void LCD_Char( unsigned char data )
{
	LCD_Port = (LCD_Port & 0x0F) | (data & 0xF0); /* sending upper nibble */
	LCD_Port |= (1<<RS);		/* RS=1, data reg. */
	LCD_Port|= (1<<EN);
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);

	_delay_us(200);

	LCD_Port = (LCD_Port & 0x0F) | (data << 4); /* sending lower nibble */
	LCD_Port |= (1<<EN);
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);
	_delay_ms(2);
}

void LCD_Init (void)			/* LCD Initialize function */
{
	LCD_Dir = 0xCF;			/* Make LCD port direction as o/p */
	_delay_ms(20);			/* LCD Power ON delay always >15ms */
	
	LCD_Command(0x02);		/* send for 4 bit initialization of LCD  */
	LCD_Command(0x28);              /* 2 line, 5*7 matrix in 4-bit mode */
	LCD_Command(0x0c);              /* Display on cursor off*/
	LCD_Command(0x06);              /* Increment cursor (shift cursor to right)*/
	LCD_Command(0x01);              /* Clear display screen*/
	_delay_ms(2);
}

void LCD_String_xy (char row, char pos, char *str)	/* Send string to LCD with xy position */
{
	if (row == 0 && pos<16)
	LCD_Command((pos & 0x0F)|0x80);	/* Command of first row and required position<16 */
	else if (row == 1 && pos<16)
	LCD_Command((pos & 0x0F)|0xC0);	/* Command of second row and required position<16 */
	
	int i;
	for(i=0;str[i]!=0;i++)		/* Send each char of string till the NULL */
	{
		LCD_Char (str[i]);
	}
}

void LCD_Clear()
{
	LCD_Command (0x01);		/* Clear display */
	_delay_ms(2);
	LCD_Command (0x80);		/* Cursor at home position */
}


////////////keypad
void keyINT_Enable()
{
	_delay_ms(300);
	DDRC = 0x01;		//keypad interrupt enable
	PORTC = 0xFE;
}


#include <math.h>

int keycheck()
{	
	DDRC=0b11110000;
	
	while(1)
	{
		PORTC=0b1101111;
		_delay_ms(10);

		switch(PINC)
		{
			case 0b1101110:return 1;break;
			case 0b1101101:return 4;break;
			case 0b1101011:return 7;break;
			case 0b1100111:return 19;break;
		}

		PORTC=0b1011111;
		_delay_ms(10);

		switch(PINC)
		{
			case 0b1011110:return 2;break;
			case 0b1011101:return 5;break;
			case 0b1011011:return 8;break;
			case 0b1010111:return 0;break;
		}
		
		PORTC=0b0111111;
		_delay_ms(10);

		switch(PINC)
		{
			case 0b0111110:return 3;break;
			case 0b0111101:return 6;break;
			case 0b0111011:return 9;break;
			case 0b0110111:return 99;break;
		}
	}
}


int i,keyVal,j;
int key[3];

int keyValue()
{
	i=0,keyVal=0,j=0;
		
	LCD_Command(0xCC);
	while(1)
	{
		key[i]=keycheck();
		if (key[i]==99) break;
		LCD_Char(key[i]+48);
		i++;
		_delay_ms(400);
	}
	keyINT_Enable();
	
	while(1)
	{
		i--;
		keyVal+=key[i]*pow(10,j);
		if (i==0) return keyVal;
		j++;
	}	
}




////////mixer
int intvl,duratn;
int mixTmrCount;

void mixConfig()
{
	LCD_Clear();
	LCD_String_xy(0,0,">>Mixer Config");
	_delay_ms(750);
	
	LCD_Clear();
	LCD_String_xy(0,0,"Enter Interval");
	LCD_String_xy(1,0,"in hours:");
	intvl = keyValue();					//real calculation-> intvl = keyValue()*60*60 / program execution time
	_delay_ms(100);
	
	LCD_Clear();
	LCD_String_xy(0,0,"Enter Duration");
	LCD_String_xy(1,0,"in minutes:");
	duratn = keyValue();				//real calculation-> duratn = keyValue()*60 / program execution time
	
	LCD_Clear();
	LCD_String_xy(0,0,"Setting Done");
	_delay_ms(500);
	
	mixTmrCount=0;
}

void mixToggle(int x)
{
	if (x==1)
	{
		PORTB |= (1<<3);
	}
	
	else if (x==0)
	{
		PORTB &= ~(1<<3);
	}
}


char mixState = 'i';

void mixAndTmrCheck()
{
	if (mixTmrCount>intvl && mixState=='i')		//interval met
	{
		mixToggle(1);
		mixTmrCount=0;
		mixState='d';
	}

	else if (mixTmrCount>duratn && mixState=='d')		//duration met
	{
		mixToggle(0);
		mixTmrCount=0;
		mixState='i';
	}
	
	mixTmrCount++;		//1 count = program execution time
}



/////////shredder and LED indicator
void shrdr(int x)
{
	if(x==1)
	{
		PORTB&= ~(1<<2);
		PORTB|= (1<<1);
	}
	else if(x==0)
	{
		PORTB|= (1<<2);
	    PORTB&= ~(1<<1);
	}
}



////////////ADC
void ADC_Init(){
	DDRA &= ~(7<<0);	        /* Make ADC port as input */
	ADCSRA = 0x85;          /* Enable ADC, with freq/128  */
}

int ADC_Read(char channel)
{
	ADMUX = 0x40 | (channel & 0x07);   /* set input channel to read */
	ADCSRA |= (1<<ADSC);               /* Start ADC conversion */
	while (!(ADCSRA & (1<<ADIF)));     /* Wait until end of conversion by polling ADC interrupt flag */
	ADCSRA |= (1<<ADIF);               /* Clear interrupt flag */
	_delay_ms(1);                      /* Wait a little bit */
	return ADCW;                       /* Return ADC word */
}



///////////Temperature
void Temp()
{
	int temp;
	char tempStr[2];
	
	LCD_Clear();
	LCD_String_xy(0,0,"Checking Temp..");
	_delay_ms(1000);
	temp=(ADC_Read(0)*4.88/10);
	LCD_Clear();
	
	if (temp<=25)
	{
		LCD_String_xy(0,0,"Temperature LOW");
		LCD_String_xy(1,0,"Heating...");
		mixToggle(1);
		PORTB|=(3<<6);		//Fan and heat coil on
	}
	else if(temp>=35)
	{
		LCD_String_xy(0,0,"Temperature HIGH");
		LCD_String_xy(1,0,"Cooling...");
		mixToggle(1);
		PORTB&=~(1<<6);		//heat off
		PORTB|=(1<<7);		//Fan on
	}
	else
	{
		LCD_String_xy(0,0,"Temperature OK");
		sprintf(tempStr,"%d C",temp);
		LCD_String_xy(1,0,tempStr);
		mixToggle(0);
		PORTB&=~(3<<6);		//fan and heat off
	}
	_delay_ms(2000);
}


//Solenoid Valve and indicator
void valve(int x)
{
	if (x==1)
	{
		PORTB |= (1<<4);
		PORTA |= (1<<5);
	}
	
	else if (x==0)
	{
		PORTB&= ~(1<<4);	
		PORTA&= ~(1<<5);	
	}
}

//////////Soil Moisture
void Soil()
{
	int soil;
	char soilStr[2];
	
	LCD_Clear();
	LCD_String_xy(0,0,"Checking");
	LCD_String_xy(1,0,"Soil Moisture...");
	_delay_ms(1000);
	soil=(ADC_Read(1)*100.00/1023.00);
	LCD_Clear();
	
	if (soil<=40)
	{
		LCD_String_xy(0,0,"Moisture LOW");
		LCD_String_xy(1,0,"Watering...");
		
		valve(1);		//solenoid valve open
		mixToggle(1);
		_delay_ms(5000);
		valve(0);	//solenoid valve close
		mixToggle(0);
	}
	else
	{
		LCD_String_xy(0,0,"Moisture OK");
		sprintf(soilStr,"%d %%",soil);
		LCD_String_xy(1,0,soilStr);
		_delay_ms(2000);
	}	
}


///////////pH
void pH()
{
	float ph;
	char phStr[5];
	
	LCD_Clear();
	LCD_String_xy(0,0,"Checking pH...");
	_delay_ms(1000);
	ph=((ADC_Read(2)*5.00*2.80)/1023.00);
	
	dtostrf(ph,3,1,phStr);
	LCD_Clear();
	LCD_String_xy(0,0,"pH:");
	LCD_String_xy(1,0,phStr);
	_delay_ms(2000);
}


///////////Ultrasonic
#define US_PORT PORTA           // we have connected the Ultrasonic sensor on port C. to use the ultrasonic we need two pins of the ultrasonic to be connected on port C
#define	US_PIN	PINA            // we need to initialize the pin resistor when we want to take input.
#define US_DDR 	DDRA            // we need data-direction-resistor (DDR) to set the direction of data flow. input or output. we will define it later, now we're just naming it.

#define US_ECHO_POS	PA6         // the echo pin of the ultrasonic sound sensor is connected to port C pin 1
#define US_TRIG_POS	PA7         // the trigger pin of ultrasonic sound sensor is connected to port C pin 0

#define US_ERROR		-1      // we're defining two more variables two know if the ultrasonic sensor is working or not
#define	US_NO_OBSTACLE	-2


void US_Init()
{
	US_DDR|=(1<<US_TRIG_POS);		// we're setting the trigger pin as output as it will generate ultrasonic sound wave
}

void US_Trigger()
{   // this function will generate ultrasonic sound wave for 15 microseconds
	//Send a 10uS pulse on trigger line
	
	US_PORT|=(1<<US_TRIG_POS);	//high
	_delay_us(15);				//wait 15uS
	US_PORT&=~(1<<US_TRIG_POS);	//low
}

uint16_t GetPulseWidth()
{
	// this function will be used to measure the pulse duration. When the ultra sound echo back after hitting an object
	// the microcontroller will read the pulse using the echo pin of the ultrasonic sensor connected to it.
	
	uint32_t i,result;

	// Section - 1: the following lines of code before the section - 2 is checking if the ultrasonic is working or not
	// it check the echo pin for a certain amount of time. If there is no signal it means the sensor is not working or not connect properly
	for(i=0;i<600000;i++)
	{
		if(!(US_PIN & (1<<US_ECHO_POS)))
		continue;	//Line is still low, so wait
		else
		break;		//High edge detected, so break.
	}

	if(i==600000)
	return US_ERROR;	//Indicates time out
	
	//High Edge Found
	
	// Section -2 : This section is all about preparing the timer for counting the time of the pulse. Timers in microcontrllers is used for timimg operation
	//Setup Timer1
	TCCR1A=0X00;
	TCCR1B=(1<<CS11);	// This line sets the resolution of the timer. Maximum of how much value it should count.
	TCNT1=0x00;			// This line start the counter to start counting time

	// Section -3 : This section checks weather the there is any object or not
	for(i=0;i<600000;i++)                // the 600000 value is used randomly to denote a very small amount of time, almost 40 miliseconds
	{
		if(US_PIN & (1<<US_ECHO_POS))
		{
			if(TCNT1 > 60000) break; else continue;   // if the TCNT1 value gets higher than 60000 it means there is not object in the range of the sensor
		}
		else
		break;
	}

	if(i==600000)
	return US_NO_OBSTACLE;	//Indicates time out

	//Falling edge found

	result=TCNT1;          // microcontroller stores the the value of the counted pulse time in the TCNT1 register.
	TCCR1B=0x00;		//Stop Timer

	if(result > 60000)
	return US_NO_OBSTACLE;	//No obstacle
	else
	return (result>>1);
}




/////////////buzzer and LED indicator
void buzz(int x)
{
	if(x==1)
	{
		PORTB|=((1<<5) | (1<<0));
	}
	else if(x==0)
	{
		PORTB&=~((1<<5) | (1<<0)) ;
	}
}


////////water level
uint16_t USduration;
int wtrLvl=0;
char wtrLvlStr[3];

void waterLevel()
{
	uint16_t USduration;
	int wtrLvl=0;
	char wtrLvlStr[3];
	
	LCD_Clear();
	LCD_String_xy(0,0,"Checking");
	LCD_String_xy(1,1,"Water Level...");
	_delay_ms(1000);
	
	while(1)
	{
		US_Trigger();               // calling the ultrasonic sound wave generator function
		USduration=GetPulseWidth();             // getting the duration of the ultrasound took to echo back
		
		if(USduration==US_ERROR)       // if microcontroller doesn't get any pulse then it will set the US_ERROR variable to -1
		{
			LCD_Clear();
			LCD_String_xy(0,0,"Error: Check");
			LCD_String_xy(1,0,"Water Lvl Sensor");
			wtrLvl=US_ERROR;
			_delay_ms(2000);
		}
		else
		{
			wtrLvl=100-(USduration*0.034*4/2.0);	// This will give the distance in centimeters
			//*4 for 4MHz--------------^
			break;
		}
	}
	
	
	if (0<wtrLvl && wtrLvl<=15)
	{
		LCD_Clear();
		LCD_String_xy(0,0,"Water Level LOW");
		buzz(1);							//buzz and LED on
		_delay_ms(2000);
	}
	else if (wtrLvl>15)
	{
		LCD_Clear();
		LCD_String_xy(0,0,"Water Level:");
		sprintf(wtrLvlStr,"%d cm",wtrLvl);
		LCD_String_xy(1,0,wtrLvlStr);
		buzz(0);							//buzz and LED off
		_delay_ms(2000);
	}
}



////////////menu
void menu()
{
	LCD_Clear();
	LCD_String_xy(0,1,"==== MENU ====");
	LCD_String_xy(1,0,"select option");
	_delay_ms(1000);
	LCD_Clear();
	LCD_String_xy(0,0,"1-> Mixer Config");
	LCD_String_xy(1,0,"2-> Check pH");
	
	switch (keycheck())
	{
		case 1:
			mixConfig();
			break;
		case 2:
			pH();
			break;
	}
	
	LCD_Clear();
	keyINT_Enable();
}



//////////Interrupt
void INT_Init()
{
	DDRD&=~(3<<2);
	PORTD&=~(1<<2);
	PORTD |= (1<<3);
	keyINT_Enable();  
	
	GICR |=(1<<INT0 | 1<<INT1); //Set Bit6 and Bit7 of GICR to unmask INT0 and INT1 interrupt
	MCUCR |=(3<<ISC00 | 2<<ISC10); //Configuring MCUCR for Rising Edge interrupt for INT0 and Falling Edge interrupt INT1
	sei(); //Enable Global Interrupts
}


ISR(INT0_vect)
{
	shrdr(0);
	_delay_ms(3000);
	shrdr(1);
}


ISR(INT1_vect)
{
	menu();
}



////////////components
void COM_Init()
{
	DDRA &= ~(1<<5);
	DDRB = 0xFF;		//output components direction
	shrdr(1);
}



/////////////main
int main(void)
{
	LCD_Init();			/*Initialization of LCD*/
	INT_Init();
	COM_Init();
	ADC_Init();
	US_Init();		//Set io port direction of sensor
	
	/*LCD_String_xy(0,0,">>> WELCOME <<<");
	_delay_ms(2000);
	LCD_Clear();
	LCD_String_xy(0,0,"AutomatedCompost");
	LCD_String_xy(1,1,"Making Machine");
	_delay_ms(3000);
	
	waterLevel();*/
	
	mixToggle(1);		//initial mix
	_delay_ms(5000);
	mixToggle(0);
	
	//mixConfig();	
	
	while(1)
	{
		Temp();
		
		if (mixTmrCount%3==0)			//real time for 15mins=90
		{
			Soil();
		}
		
		waterLevel();
		
		mixAndTmrCheck();				//min program execution time=10s
	}
}

