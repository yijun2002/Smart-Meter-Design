//Compilation command
//turboAVR SM_18042023.c D5.c font.c ili934x.c lcdnew.c
//-----------------------------------------------------

#include <avr/io.h>
#include <util/delay.h>
#include "debug.h"
#include <avr/interrupt.h>
#include <math.h>
#include <string.h>

//Opcode for serial communication
#define BatteryAmount 1
#define WindCapacity 2
#define PVCapacity 3
#define busbarVoltageOp 4
#define busbarCurrentOp 5

//Provided settings
#define MAX_WIND 3.0
#define MAX_PV 1.0
#define MAINS_CAPACITY 2.0
#define CHARGING_CONSTANT 1.0
#define DISCHARGING_CONSTANT 1.0
#define LOAD1_DEMAND 1.2
#define LOAD2_DEMAND 2.0
#define LOAD3_DEMAND 0.8

//Define MACROs
#define BBCURRENT PA0
#define BBVOLTAGE PA1
#define WINDTB PA2
#define SOLARPV PA3
#define L1CALL PA4
#define L2CALL PA5
#define L3CALL PA6
#define CBATT PA7
#define DBATT PD2
#define L1SW PD3
#define L2SW PD4
#define L3SW PD5
#define MAINSREQ PD7

//variable for timer ISR
volatile uint8_t count = 0;
volatile uint8_t interval;

//Global variables and flags
volatile uint8_t flag = 0;

//Previous data - Shed Load = t+1; Doesnt Shed Load  = t remains
int TLOAD1 = 0;
int TLOAD2 = 0;
int TLOAD3 = 0;
int TLOAD2_3 = 0;

//Energy stored in Battery Bank
int Battery = 0;
int chargeBattery = 0;
int dischargeBattery = 0;

//Load
int callLoad1, callLoad2, callLoad3;
int supplyLoad1, supplyLoad2, supplyLoad3;

//Renewable resources (Wind Turbine + Solar Photovoltaic Panel)
float WindTurbine, SolarPV;
double renewablesAvailable;
double energyDemand;

//Local distribution networks (Mains supply)
float MainsCurrent;

//Busbar data
int BusbarVoltage, BusbarCurrent;


//timer1 overflow interrupt, when timer1 overflow, trigger ISR and implement count,interval ++
ISR(TIMER1_OVF_vect){
	count++;
	//when count == 11, 1 minutes has elapsed
	if (count == 11){
		count = 0;
		interval ++;
	}
	if(count > 1 && count < 10){
		flag = 1;
	}
}

void initIO()
{
    //ADC Input Pins
    DDRA &= ~(_BV(BBCURRENT) | _BV(BBVOLTAGE) | _BV(WINDTB) | _BV(SOLARPV));

    //Load Call Input Pins
    DDRA &= ~(_BV(L1CALL) | _BV(L2CALL) | _BV(L3CALL));

    //Load Switches Ouput Pins
    DDRA |= _BV(CBATT);
    DDRD |= _BV(DBATT) | _BV(L1SW) | _BV(L2SW) | _BV(L3SW);

    //Mains Current Request PWM Output Pin
    DDRD |= _BV(MAINSREQ);
}

void initADC()
{
    // First conversion = 25 clock cycles; Normal conversion = 13 cycles
	DDRA &= ~(_BV(PA0));
    ADMUX |= (1 << REFS0);                            // AVCC with external capacitor at AREF pin
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1);            // Division Factor = 64, ADC frequency = 18.75kHz
    ADCSRA |= (1 << ADEN);                             // ADC Enabled
    ADCSRB = 0x00;                                    // ADC Auto Triggering Disabled
}

void initTimers()
{
    // Timer 1 is chosen as a stopwatch to record time 
	// as it has 16 bits thus is able to count to 65535
    TCCR1A = 0X00;      							//set normal mode
    TCNT1=0;
    TCCR1B = (1<<CS10) | (1<<CS12); 				//1024 prescaler
    TIMSK1 = (1<<TOIE1); 							//Timer/Counter1 Overflow interrupt is enabled
    sei();											//enable global interrupt
	
	// Timer 2 is chosen to produce PWM output waveform
    TCCR2A |= (1 << COM2A1);							// Toggle OC2A on Compare Match
    TCCR2A |= (1 << WGM21) | (1 << WGM20);				// Mode 7: Fast PWM, TOP = OCRA
    TCCR2B |= (1 << CS20);								// No prescaling, PWM frequency = 12M/(1*256) = 46875Hz
    OCR2A = 0;
}

void setPWM(int mains_current)
{
    //calculations
    OCR2A = (mains_current*256/2)-1;					
}

void readPin()
{
	if(PINA & _BV(L1CALL)){callLoad1 = 1;}
	else				  {callLoad1 = 0;}
	if(PINA & _BV(L2CALL)){callLoad2 = 1;}
	else				  {callLoad2 = 0;}	
	if(PINA & _BV(L3CALL)){callLoad3 = 1;}
	else				  {callLoad3 = 0;}
}

void writePin()
{
	if(chargeBattery)		{PORTA |= _BV(CBATT);}
	else					{PORTA &= ~_BV(CBATT);}
	if(dischargeBattery)	{PORTD |= _BV(DBATT);}
	else					{PORTD &= ~_BV(DBATT);}
	if(supplyLoad1)			{PORTD |= _BV(L1SW);}
	else					{PORTD &= ~_BV(L1SW);}
	if(supplyLoad2)			{PORTD |= _BV(L2SW);}
	else					{PORTD &= ~_BV(L2SW);}
	if(supplyLoad3)			{PORTD |= _BV(L3SW);}
	else					{PORTD &= ~_BV(L3SW);}
}

uint16_t readADC(uint8_t channel)
{
    ADMUX = (ADMUX & 0xE0) | channel;                // Reset all 4 Mux bits 
    ADCSRA |= (1 << ADSC);                            // Start Conversion
    while (ADCSRA & _BV(ADSC));                        // Wait for End of Conversion
    uint16_t result = 0;
    uint8_t ADCR1 = ADCL;
    uint8_t ADCR2 = ADCH;
    result =  ((ADCR2 << 8) | ADCR1);                // Combined results - By default, result is right adjusted
	return result;                                    // voltage = result/1024 * VCC
}

float getWindTB()
{
	uint16_t sum = 0;
	float average = 0;
	int count = 100;
	char s1[20];
    char s2[20];
	
	while(count){
		sum += readADC(2);
		count--;
	}
	
	average = sum/100;
	// Convert both the integers to string
    sprintf(s1, "%d", WindCapacity); 	//opcode
    sprintf(s2, "%d", (int)average); 			//value
    //send data through serial communication
    printf("%s\n",strcat(s1,s2));
	return ((average*3.3*2*1.1)/(1024*3));
}

float getSolarPV()
{
	float average = 0;
	uint16_t sum = 0;
	int count = 100;
	char s1[20];
    char s2[20];
	
	while(count){
		sum += readADC(3);
		count--;
	}
	
	average = sum/100;
	// Convert both the integers to string
    sprintf(s1, "%d", PVCapacity); 	//opcode
    sprintf(s2, "%d", (int)average); 			//value
    //send data through serial communication
    printf("%s\n",strcat(s1,s2));
	return ((average*3.3*2*1.1)/(1024*3));
}

uint16_t getBusbarCurrentRMS()
{

	int count = 375;
	uint16_t previous_sample= 0;
	uint16_t max_sample = 0;
	char s1[20];
	char s2[20];
	
	while(count){
		max_sample = readADC(0); 												  //read current ADC value
		max_sample = max_sample > previous_sample ? max_sample : previous_sample; //check for current sample > previous sample?
		previous_sample = max_sample;											  //if 1, store new sample value
		count --;
	}

    // Convert both the integers to string
    sprintf(s1, "%d", busbarCurrentOp); 	//opcode
    sprintf(s2, "%d", max_sample); 			//value
    //send data through serial communication
    printf("%s\n",strcat(s1,s2));
    //rmsValue =(((max_sample / 1024) * 3.3 ) * 6.428002451 - 11.2)/sqrt(2);
	//rmsValue = (((max_sample / 1024) * 3.3 + 1.479) *  10 / 3.3) * 100 / sqrt(2) ;

	return max_sample;
}

uint16_t getBusbarVoltageRMS()
{
	
	int count_sample = 375;
	int count_wave = 3;
	uint16_t previous_sample = 0;
	uint16_t max_sample = 0;
    char s1[20];
    char s2[20];


	while(count_wave){
		while(count_sample){
		max_sample = readADC(1); 													//read current ADC value
		max_sample = max_sample > previous_sample ? max_sample : previous_sample;   //check for current sample > previous sample?
		previous_sample = max_sample;												//if 1, store new sample value
		count_sample --;
		}
		count_wave--;
	}

    // Convert both the integers to string
    sprintf(s1, "%d", busbarVoltageOp); 	//opcode
    sprintf(s2, "%d", max_sample);			//value
    //send data through serial communication
    printf("%s\n",strcat(s1,s2));
    //rmsValue = (((max_sample / 1024) * 3.3 * 4.624338624 - 11.2) * 100)/sqrt(2);
	//rmsValue = (((max_sample / 1024) * 3.3 + 0.8615) *  10 / 3.3 )/sqrt(2);

	return max_sample;
}

int getStatus(int variable){
	
	switch (variable){
		case 1:
			if(supplyLoad1)		{return 2;}
			else if(callLoad1)	{return 1;}
			else 				{return 0;}	
			break;
		case 2:
			if(supplyLoad2)		{return 2;}
			else if(callLoad2)	{return 1;}
			else 				{return 0;}		
			break;
		case 3:
			if(supplyLoad3)		{return 2;}
			else if(callLoad3)	{return 1;}
			else 				{return 0;}		
			break;
		case 4:
			if(chargeBattery)			{return 2;}
			else if(dischargeBattery)	{return 1;}
			else						{return 0;}
			break;
		default:
			return 0;
	}

}

void algorithm()
{

    //Resetting all flags
    supplyLoad1 = 0;
    supplyLoad2 = 0;
    supplyLoad3 = 0;
    chargeBattery = 0;
    dischargeBattery = 0;

    //Resetting all output
    MainsCurrent = 0;

    //Renewable resources available
    renewablesAvailable = WindTurbine + SolarPV;

    //Energy demanded by the load
    energyDemand = (callLoad1 * LOAD1_DEMAND) +
                         (callLoad2 * LOAD2_DEMAND) +
                         (callLoad3 * LOAD3_DEMAND);

    //Extra renewable resources
    double renewablesExcess = renewablesAvailable - energyDemand;

    //---------------------------- Decision making ----------------------------//
    //Case A: Excess renewable resources
    if(renewablesExcess >= 0){

        //Supply all load
        supplyLoad1 = 1 * callLoad1;
        supplyLoad2 = 1 * callLoad2;
        supplyLoad3 = 1 * callLoad3;

        //Excess renewables resources >= 1
        if(renewablesExcess >= 1){
            chargeBattery = 1;
            Battery++;
        }

            //Charge Battery?
        else{

            //Pre-peak season
            if(interval < 5){

                if(renewablesExcess >= 0.3){
                    MainsCurrent = CHARGING_CONSTANT - renewablesExcess;
                    chargeBattery = 1;
                    Battery++;
                }
            }

                //Peak season
            else if((interval >= 5) && (interval < 22)){

                //Maintain Battery Bank at 3A
                if(Battery < 3){
                    MainsCurrent = CHARGING_CONSTANT - renewablesExcess;
                    chargeBattery = 1;
                    Battery++;
                }


                else{

                    if(renewablesExcess >= 0.5){
                        MainsCurrent = CHARGING_CONSTANT - renewablesExcess;
                        chargeBattery = 1;
                        Battery++;
                    }
                }
            }

                //Post-peak season
            else{

                //Maintain Battery just enough for the last few intervals left
                if(Battery < 24-interval){
                    if(renewablesExcess >= 0.5){
                        MainsCurrent = CHARGING_CONSTANT - renewablesExcess;
                        chargeBattery = 1;
                        Battery++;
                    }
                }
            }
        }
    }

        //Case B: Renewable resources shortage
    else{

        //Energy shortage
        double renewablesDeficit = energyDemand - renewablesAvailable;

        if(interval < 5){
            if(renewablesDeficit < 1){
                supplyLoad1 = 1 * callLoad1;
                supplyLoad2 = 1 * callLoad2;
                supplyLoad3 = 1 * callLoad3;
				MainsCurrent = renewablesDeficit;
            }
            else{
                if(Battery > 0){
                    supplyLoad1 = 1 * callLoad1;
                    supplyLoad2 = 1 * callLoad2;
                    supplyLoad3 = 1 * callLoad3;
                    MainsCurrent = renewablesDeficit - DISCHARGING_CONSTANT;
                    dischargeBattery = 1;
                    Battery--;
                }
                else{
                    if(renewablesDeficit > 2){
                        supplyLoad1 = 1 * callLoad1;
                        supplyLoad2 = 1 * callLoad2;
                        supplyLoad3 = 0 * callLoad3;
                        MainsCurrent = renewablesDeficit - LOAD3_DEMAND;
                        TLOAD3++;
                    }
                    else{
                        supplyLoad1 = 1 * callLoad1;
                        supplyLoad2 = 1 * callLoad2;
                        supplyLoad3 = 1 * callLoad3;
                        MainsCurrent = renewablesDeficit;
                    }
                }
            }
        }

        else if((interval >= 5) && (interval < 8)){
            if(renewablesDeficit < 1){
                if(Battery > 3){
                    supplyLoad1 = 1 * callLoad1;
                    supplyLoad2 = 1 * callLoad2;
                    supplyLoad3 = 1 * callLoad3;
                    MainsCurrent = renewablesDeficit;
                }
                else{
                    supplyLoad1 = 1 * callLoad1;
                    supplyLoad2 = 1 * callLoad2;
                    supplyLoad3 = 1 * callLoad3;
                    MainsCurrent = renewablesDeficit + CHARGING_CONSTANT;
                    chargeBattery = 1;
                    Battery++;
                }
            }
            else{
                if(Battery > 3){
                    supplyLoad1 = 1 * callLoad1;
                    supplyLoad2 = 1 * callLoad2;
                    supplyLoad3 = 1 * callLoad3;
                    MainsCurrent = renewablesDeficit - DISCHARGING_CONSTANT;
                    dischargeBattery = 1;
                    Battery--;
                }
                else{
                    if(renewablesDeficit > 2){
                        supplyLoad1 = 1 * callLoad1;
                        supplyLoad2 = 1 * callLoad2;
                        supplyLoad3 = 0 * callLoad3;
                        MainsCurrent = renewablesDeficit - LOAD3_DEMAND;
                        TLOAD3++;
                    }
                    else{
                        supplyLoad1 = 1 * callLoad1;
                        supplyLoad2 = 1 * callLoad2;
                        supplyLoad3 = 1 * callLoad3;
                        MainsCurrent = renewablesDeficit;
                    }
                }
            }
        }

        else if((interval >= 8) && (interval <= 22)){
            if(renewablesDeficit < 1){
                if(Battery < 5){				//Battery < 24 - interval
                    supplyLoad1 = 1 * callLoad1;
                    supplyLoad2 = 1 * callLoad2;
                    supplyLoad3 = 1 * callLoad3;
                    MainsCurrent = renewablesDeficit;
                }
                else{
                    supplyLoad1 = 1 * callLoad1;
                    supplyLoad2 = 1 * callLoad2;
                    supplyLoad3 = 1 * callLoad3;
                    dischargeBattery = 1;
                    Battery--;
                }
            }
            else{
                if(Battery > 0){
                    if(renewablesDeficit > 3){
                        if((3*(TLOAD2+1)) > (4*(TLOAD1+1))){
                            supplyLoad1 = 0 * callLoad1;
                            supplyLoad2 = 1 * callLoad2;
                            supplyLoad3 = 1 * callLoad3;
                            MainsCurrent = renewablesDeficit - LOAD1_DEMAND - DISCHARGING_CONSTANT;
                            dischargeBattery = 1;
                            Battery--;
                            TLOAD1++;
                        }
                        else{
                            supplyLoad1 = 1 * callLoad1;
                            supplyLoad2 = 0 * callLoad2;
                            supplyLoad3 = 1 * callLoad3;
                            MainsCurrent = renewablesDeficit - LOAD2_DEMAND - DISCHARGING_CONSTANT;
                            dischargeBattery = 1;
                            Battery--;
                            TLOAD2++;
                        }
                    }
                    else{
                        supplyLoad1 = 1 * callLoad1;
                        supplyLoad2 = 1 * callLoad2;
                        supplyLoad3 = 1 * callLoad3;
                        MainsCurrent = renewablesDeficit - DISCHARGING_CONSTANT;
                        dischargeBattery = 1;
                        Battery--;
                    }
                }
                else{
                    if(renewablesDeficit > 2){
                        if((callLoad1 == 1) && (callLoad2 == 1) && (callLoad3 == 0)){
                            if((3*(TLOAD2+1)) > (4*(TLOAD1+1))){
                                supplyLoad1 = 0 * callLoad1;
                                supplyLoad2 = 1 * callLoad2;
                                supplyLoad3 = 1 * callLoad3;
                                MainsCurrent = renewablesDeficit - LOAD1_DEMAND;
                                TLOAD1++;
                            }
                            else{
                                supplyLoad1 = 1 * callLoad1;
                                supplyLoad2 = 0 * callLoad2;
                                supplyLoad3 = 1 * callLoad3;
                                MainsCurrent = renewablesDeficit - LOAD2_DEMAND;
                                TLOAD2++;
                            }
                        }
                        else if((callLoad1 == 0) && (callLoad2 == 1) && (callLoad3 == 1)){
                            if((1*(TLOAD3+1)) > (3*(TLOAD2+1))){
                                supplyLoad1 = 1 * callLoad1;
                                supplyLoad2 = 0 * callLoad2;
                                supplyLoad3 = 1 * callLoad3;
                                MainsCurrent = renewablesDeficit - LOAD2_DEMAND;
                                TLOAD2++;
                            }
                            else{
                                supplyLoad1 = 1 * callLoad1;
                                supplyLoad2 = 1 * callLoad2;
                                supplyLoad3 = 0 * callLoad3;
                                MainsCurrent = renewablesDeficit - LOAD3_DEMAND;
                                TLOAD3++;
                            }
                        }
                        else{
                            if(((4*(TLOAD1+1))+(1*(TLOAD3+1))) > (3*(TLOAD2+1))){
                                supplyLoad1 = 1 * callLoad1;
                                supplyLoad2 = 0 * callLoad2;
                                supplyLoad3 = 1 * callLoad3;
                                MainsCurrent = renewablesDeficit - LOAD2_DEMAND;
                                TLOAD2++;
                            }
                            else{
                                supplyLoad1 = 0 * callLoad1;
                                supplyLoad2 = 1 * callLoad2;
                                supplyLoad3 = 0 * callLoad3;
                                MainsCurrent = renewablesDeficit - LOAD1_DEMAND - LOAD3_DEMAND;
                                TLOAD1++;
                                TLOAD3++;
                            }
                        }
                    }
                    else{
                        supplyLoad1 = 1 * callLoad1;
                        supplyLoad2 = 1 * callLoad2;
                        supplyLoad3 = 1 * callLoad3;
                        MainsCurrent = renewablesDeficit;
                    }
                }
            }
        }

        else{
            if(renewablesDeficit < 1){
                if(Battery < 24 - interval){
                    supplyLoad1 = 1 * callLoad1;
                    supplyLoad2 = 1 * callLoad2;
                    supplyLoad3 = 1 * callLoad3;
                    MainsCurrent = renewablesDeficit;
                }
                else{
                    supplyLoad1 = 1 * callLoad1;
                    supplyLoad2 = 1 * callLoad2;
                    supplyLoad3 = 1 * callLoad3;
                    dischargeBattery = 1;
                    Battery--;
                }
            }
            else{
                if(Battery > 0){
                    supplyLoad1 = 1 * callLoad1;
                    supplyLoad2 = 1 * callLoad2;
                    supplyLoad3 = 1 * callLoad3;
                    MainsCurrent = renewablesDeficit - DISCHARGING_CONSTANT;
                    dischargeBattery = 1;
                    Battery--;
                }
                else{
                    if(renewablesDeficit > 2){
                        if((3*(TLOAD2+1)) > (1*(TLOAD3+1))){
                            supplyLoad1 = 1 * callLoad1;
                            supplyLoad2 = 1 * callLoad2;
                            supplyLoad3 = 0 * callLoad3;
                            MainsCurrent = renewablesDeficit - LOAD3_DEMAND;
                            TLOAD3++;
                        }
                        else{
                            supplyLoad1 = 1 * callLoad1;
                            supplyLoad2 = 0 * callLoad2;
                            supplyLoad3 = 1 * callLoad3;
                            MainsCurrent = renewablesDeficit - LOAD2_DEMAND;
                            TLOAD2++;
                        }
                    }
                    else{
                        supplyLoad1 = 1 * callLoad1;
                        supplyLoad2 = 1 * callLoad2;
                        supplyLoad3 = 1 * callLoad3;
                        MainsCurrent = renewablesDeficit;
                    }
                }
            }
        }
    }

}

int main(){
	
	//Initialization
	initIO();
	initADC();
	initTimers();
	init_debug_uart0();
	
	//GUI Start up
	GUI_startup();
	GUI_mainscreen();
	
	
	//Reset interval
	interval = 1;
	
	while(1){
		readPin();
		WindTurbine = getWindTB(2);
		SolarPV = getSolarPV(3);
		BusbarVoltage = getBusbarVoltageRMS();
		int rmsVoltage = (((((double)BusbarVoltage / 1024) * 3.3 * 4.661058535 - 10.86) * 100) / sqrt(2));
		BusbarCurrent = getBusbarCurrentRMS();
		int rmsCurrent = ((((double)BusbarCurrent / 1024) * 3.3 ) * 6.428002451 - 10.86)/sqrt(2);
		//Run decision-making block if flag is turned on
		//Skip decision-making algorithm otherwise
		if(flag){
			algorithm();
			flag = 0;
		}
	
	
		char batteryop[20];
		char charge[20];
		sprintf(batteryop, "%d", BatteryAmount);
		sprintf(charge, "%d", Battery);
		printf("%s\n",strcat(batteryop,charge));
			
		//Write outputs
		writePin();
		setPWM(MainsCurrent);
		
		//Update GUI
		int status_L1 = getStatus(1);
		int status_L2 = getStatus(2);
		int status_L3 = getStatus(3);
		int status_Battery = getStatus(4);
		
		GUI_update(interval,rmsVoltage,status_L1,status_L2,status_L3,status_Battery,Battery,renewablesAvailable);
		
	}
	
}