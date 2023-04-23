 #include "lcd.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <util/delay.h> 

 void clear_draw(int position_x, int position_y, int move);
 void left_B(int position_x,int position_y, int move);
 void right_B(int position_x,int position_y, int move);
 void GUI_startup();
 void GUI_mainscreen();
 void GUI_update(int hour,int voltage,int status_Load1,int status_Load2,int status_Load3,int stats_Battery,int charge,double green);
 void D(int position_x, int position_y);
 void five(int position_x, int position_y);
 void grid();
 void Current(int position_x, int position_y);
 void Loads(int position_x, int position_y, int status_Load1, int status_Load2, int status_Load3);
 uint16_t status_Loads(int status_Load);
 void func_battery(int position_x, int position_y);
 void update_battery(int position_x, int position_y, int charge, int status);
 uint16_t status_Battery(int status);
 void picture_draw(int left_, int top_, uint8_t* array, int length, uint16_t col);

 void GUI_startup() {
	init_lcd();
	set_orientation(North);
	int move;
	int pos_x = 0, pos_y = 0;
	left_B(pos_x,pos_y,25);
	right_B(pos_x,pos_y,25);
	_delay_ms(1000);
	for(move=25;move>=0;move-=5){
		clear_draw(pos_x,pos_y,move);
		_delay_ms(10);
		left_B(pos_x,pos_y,move);
		right_B(pos_x,pos_y,move);
		_delay_ms(10);
	}
	_delay_ms(500);
	move = 0;
	left_B(pos_x,pos_y,move);
	right_B(pos_x,pos_y,move);
	clear_screen();
	// Draw D
	pos_x = -25, pos_y = -15;
	D(pos_x,pos_y);
	// Draw 5
	pos_x = 25;
	pos_y = -15;
	five(pos_x,pos_y);
	// Group B
	pos_x = LCDWIDTH/2 - 50;
	pos_y = LCDHEIGHT/2 + 30;
	char string[] = "BY GROUP B";
	int i;
	for(i=0; string[i]!='\0'; i++) {
		display_char(string[i],pos_x,pos_y);
		pos_x += 11;
		_delay_ms(200);
	}
	// Switch Screen
	_delay_ms(1000);
	clear_screen();
 }

 void GUI_mainscreen() {
	// Voltage
	picture_draw(19, 27, &image_data_Voltage, 400, GOLD);
	char string1[] = "VOLTAGE (V)";
	display_string(string1,49,33);
	
	// Draw Current
	Current(19,27+LCDHEIGHT/5);
	char string2[] = "CURRENT (A)";
	display_string(string2,49,27+LCDHEIGHT/5);
	
	// Power Usage
	picture_draw(19, 15+2*LCDHEIGHT/5, &image_data_PowerUsage, 400, TURQ);
	char string3[] = "POWER USAGE";
	display_string(string3,49,15+2*LCDHEIGHT/5);
	char string4[] = "(W/h)";
	display_string(string4,75,27+2*LCDHEIGHT/5);
	
	// Green Energy
	picture_draw(19, 10+3*LCDHEIGHT/5, &image_data_GreenEnergy, 400, GREEN);
	char string5[] = "GREEN ENERGY";
	display_string(string5,49,10+3*LCDHEIGHT/5);
	char string6[] = "(W/h)";
	display_string(string6,75,22+3*LCDHEIGHT/5);
	
	// Loads
	char string7[] = "LOAD STATUS";
	display_string(string7,30,4*LCDHEIGHT/5 - 5);
	char string8[] = "1   2   3";
	display_string(string8,40,35+4*LCDHEIGHT/5);
	
	// Battery
	func_battery(45+LCDWIDTH/2,10+4*LCDHEIGHT/5);
	char string9[] = "BATTERY";
	display_string(string9,45+LCDWIDTH/2,4*LCDHEIGHT/5 - 5);
 }

 void GUI_update(int hour,int voltage,int status_Load1,int status_Load2,int status_Load3,int stats_Battery,int charge,double green) {
 //void GUI_update(int hour,int voltage,int status_Load1,int status_Load2,int status_Load3,int charge,double green) {
	/* FOR LOAD */
	// 0 for no demand, 1 for demand but no supply, 2 for supplied

	/* FOR BATTERY */
	// 0 for idle, 1 for discharge, 2 for charge

	double current;			// Use load status Sum of loads print decimal 1dp
	int power = 0;			// Use load status 240*current
	int power_green = 0;	// Use green 240*green
	//int charge = 0;			// Use battery status
	char hour_status[8];
	char voltage_string[10];
	char current_string[4];
	char power_string[10];
	char green_string[10];
	char battery_string[5];
	// Setting important variables
	current = 1.2 * ((status_Load1 == 2) ? 1 : 0) + 2 * ((status_Load2 == 2) ? 1 : 0) + 0.8 * ((status_Load3 == 2) ? 1 : 0);
	power = (int)((double)voltage * current);
	power_green = (int)((double)voltage * green);

	// Clear display parameters
	rectangle black_d;
	black_d.left = 57+LCDWIDTH/2;
	black_d.right = 85+LCDWIDTH/2;
	black_d.top = 35+4*LCDHEIGHT/5;
	black_d.bottom = 43+4*LCDHEIGHT/5;
	fill_rectangle(black_d, BLACK);
	// Voltage
	black_d.left = 50+LCDWIDTH/2;
	black_d.right = 70+LCDWIDTH/2;
	black_d.top = 31;
	black_d.bottom = 39;
	fill_rectangle(black_d, BLACK);
	// Current
	black_d.left = 50+LCDWIDTH/2;
	black_d.right = 70+LCDWIDTH/2;
	black_d.top = 27+LCDHEIGHT/5;
	black_d.bottom = 35+LCDHEIGHT/5;
	fill_rectangle(black_d, BLACK);
	// Power Consumption
	black_d.left = 50+LCDWIDTH/2;
	black_d.right = 70+LCDWIDTH/2;
	black_d.top = 19+2*LCDHEIGHT/5;
	black_d.bottom = 27+2*LCDHEIGHT/5;
	fill_rectangle(black_d, BLACK);
	// Green Energy
	black_d.left = 50+LCDWIDTH/2;
	black_d.right = 70+LCDWIDTH/2;
	black_d.top = 14+3*LCDHEIGHT/5;
	black_d.bottom = 22+3*LCDHEIGHT/5;
	fill_rectangle(black_d, BLACK);
	
	// Display Load
	Loads(30,10+4*LCDHEIGHT/5,status_Load1,status_Load2,status_Load3);
	
	// Display Battery
	update_battery(45+LCDWIDTH/2,10+4*LCDHEIGHT/5,charge,stats_Battery);
	sprintf(battery_string, "%2d A", charge);
	display_string(battery_string,57+LCDWIDTH/2,35+4*LCDHEIGHT/5);

	// Display other stats
	sprintf(hour_status,"HOUR %d",hour);
	sprintf(voltage_string, "%d",voltage);
	sprintf(current_string,"%d.%d",(int) current,((int) (current*10))%10);	// Strange behaviour for AVR when using float with sprintf
	sprintf(power_string, "%d",power);
	sprintf(green_string, "%d",power_green);
	display_string(hour_status,LCDWIDTH/2 - 17,7);
	display_string(voltage_string,50+LCDWIDTH/2,32);
	display_string(current_string,50+LCDWIDTH/2,28+LCDHEIGHT/5);
	display_string(power_string,50+LCDWIDTH/2,20+2*LCDHEIGHT/5);
	display_string(green_string,50+LCDWIDTH/2,15+3*LCDHEIGHT/5);
 }
 
   void clear_draw(int position_x, int position_y, int move) {
	rectangle black_b;

	// LEFT
	black_b.left = LCDWIDTH/2 - 19 + position_x;
	black_b.right = LCDWIDTH/2 + position_x;
	black_b.top = LCDHEIGHT/2 - 25 + position_y - (move + 5);
	black_b.bottom = LCDHEIGHT/2 - 20 + position_y - (move + 5);
	fill_rectangle(black_b, BLACK);

	// RIGHT
	black_b.left = LCDWIDTH/2 + position_x;
	black_b.right = LCDWIDTH/2 + 19 + position_x;
	black_b.top = LCDHEIGHT/2 + 15 + position_y + (move + 5);
	black_b.bottom = LCDHEIGHT/2 + 20 + position_y + (move + 5);
	fill_rectangle(black_b, BLACK);
  }

  void left_B(int position_x, int position_y, int move) {
	rectangle blue_b;
	rectangle black1_b;
	rectangle black2_b;
	rectangle outline;

	if(move == 0) {
		outline.left = LCDWIDTH/2 - 20 + position_x;
		outline.right = LCDWIDTH/2 + position_x;
		outline.top = LCDHEIGHT/2 - 25 + position_y;
		outline.bottom = LCDHEIGHT/2 + 21 + position_y;
		fill_rectangle(outline, WHITE);
	}
	
	// BLUE
	blue_b.left = LCDWIDTH/2 - 19 + position_x;
	blue_b.right = LCDWIDTH/2 + position_x;
	blue_b.top = LCDHEIGHT/2 - 25 + position_y - move;
	blue_b.bottom = LCDHEIGHT/2 + 20 + position_y - move;
	fill_rectangle(blue_b, TURQ);

	if(move == 0) {
		// OUTLINE
		black1_b.left = LCDWIDTH/2 - 10 + position_x;
		black1_b.right = LCDWIDTH/2 + 10 + position_x;
		black1_b.top = LCDHEIGHT/2 - 15 + position_y;
		black1_b.bottom = LCDHEIGHT/2 -5 + position_y;
		fill_rectangle(black1_b, BLACK);
		
		black2_b.left = LCDWIDTH/2 - 10 + position_x;
		black2_b.right = LCDWIDTH/2 + 10 + position_x;
		black2_b.top = LCDHEIGHT/2 + position_y;
		black2_b.bottom = LCDHEIGHT/2 + 10 + position_y;
		fill_rectangle(black2_b, BLACK);
	}
 }

  void right_B(int position_x, int position_y, int move) {
	rectangle red_b;
	rectangle black1_b;
	rectangle black2_b;
	rectangle outline;
	
	// RED
	red_b.left = LCDWIDTH/2 + position_x;
	red_b.right = LCDWIDTH/2 + 19 + position_x;
	red_b.top = LCDHEIGHT/2 - 25 + position_y + move;
	red_b.bottom = LCDHEIGHT/2 + 20 + position_y + move;
	fill_rectangle(red_b, RED);

	// OUTLINE
	if(move == 0) {

		outline.left = LCDWIDTH/2 - 10 + position_x;
		outline.right = LCDWIDTH/2 + 10 + position_x + 1;
		outline.top = LCDHEIGHT/2 - 15 + position_y - 1;
		outline.bottom = LCDHEIGHT/2 -5 + position_y + 1;
		fill_rectangle(outline, WHITE);
		
		outline.left = LCDWIDTH/2 - 10 + position_x;
		outline.right = LCDWIDTH/2 + 10 + position_x + 1;
		outline.top = LCDHEIGHT/2 + position_y - 1;
		outline.bottom = LCDHEIGHT/2 + 10 + position_y + 1;
		fill_rectangle(outline, WHITE);
		
		black1_b.left = LCDWIDTH/2 - 10 + position_x;
		black1_b.right = LCDWIDTH/2 + 10 + position_x;
		black1_b.top = LCDHEIGHT/2 - 15 + position_y;
		black1_b.bottom = LCDHEIGHT/2 -5 + position_y;
		fill_rectangle(black1_b, BLACK);
		
		black2_b.left = LCDWIDTH/2 - 10 + position_x;
		black2_b.right = LCDWIDTH/2 + 10 + position_x;
		black2_b.top = LCDHEIGHT/2 + position_y;
		black2_b.bottom = LCDHEIGHT/2 + 10 + position_y;
		fill_rectangle(black2_b, BLACK);

		int i;
		for(i=7;i>0;i--){
			black2_b.left = LCDWIDTH/2 + 19 + position_x - i + 2;
			black2_b.right = LCDWIDTH/2 + 19 + position_x + 2;
			black2_b.top = LCDHEIGHT/2 - 25 + position_y + 5 - i  + move;
			black2_b.bottom = LCDHEIGHT/2 - 25 + position_y + 5 - i  + move;
			fill_rectangle(black2_b, BLACK);
		}
		for(i=5;i>0;i--){
			black2_b.left = LCDWIDTH/2 + 19 + position_x - i + 2;
			black2_b.right = LCDWIDTH/2 + 19 + position_x + 2;
			black2_b.top = LCDHEIGHT/2 + 20 + position_y - 4 + i + move;
			black2_b.bottom = LCDHEIGHT/2 + 20 + position_y - 4 + i + move;
			fill_rectangle(black2_b, BLACK);
		}
	}
 }

 void D(int position_x, int position_y) {
	rectangle yellow_d;
	rectangle black_d;
	// Yellow
	yellow_d.left = LCDWIDTH/2 - 19 + position_x;
	yellow_d.right = LCDWIDTH/2 + 20 + position_x;
	yellow_d.top = LCDHEIGHT/2 - 19 + position_y;
	yellow_d.bottom = LCDHEIGHT/2 + 20 + position_y;
	fill_rectangle(yellow_d, TURQ);
	// Black
	black_d.left = LCDWIDTH/2 - 5 + position_x;
	black_d.right = LCDWIDTH/2 + 12 + position_x;
	black_d.top = LCDHEIGHT/2 - 11 + position_y;
	black_d.bottom = LCDHEIGHT/2 + 12 + position_y;
	fill_rectangle(black_d, BLACK);
}

 void five(int position_x, int position_y) {
	rectangle yellow_d;
	rectangle black_d;
	// Yellow
	yellow_d.left = LCDWIDTH/2 - 19 + position_x;
	yellow_d.right = LCDWIDTH/2 + 20 + position_x;
	yellow_d.top = LCDHEIGHT/2 - 19 + position_y;
	yellow_d.bottom = LCDHEIGHT/2 + 20 + position_y;
	fill_rectangle(yellow_d, GOLD);
	// Black
	black_d.left = LCDWIDTH/2 - 19 + position_x;
	black_d.right = LCDWIDTH/2 + 4 + position_x;
	black_d.top = LCDHEIGHT/2 + 3 + position_y;
	black_d.bottom = LCDHEIGHT/2 + 14 + position_y;
	fill_rectangle(black_d, BLACK);
	black_d.left = LCDWIDTH/2 - 3 + position_x;
	black_d.right = LCDWIDTH/2 + 20 + position_x;
	black_d.top = LCDHEIGHT/2 - 13 + position_y;
	black_d.bottom = LCDHEIGHT/2 - 2 + position_y;
	fill_rectangle(black_d, BLACK);
}

 void grid() {
	// Creating grids
	char divider = '|';
	int height = 0;
	while(height < LCDHEIGHT) {
		display_char(divider,27 + LCDWIDTH/2,height);
		height += 16;
	}
	divider = '-';
	int width;
	int i;
	for(i = 1; i<=4; i++) {
		width = 0;
		while(width < LCDWIDTH) {
			display_char(divider,width,i*LCDHEIGHT/5.5);
			width += 11;
		}
	}
 }

 void Current(int position_x, int position_y) {
	rectangle white_bar;
	rectangle white_dashes;
	// BAR
	white_bar.left = position_x;
	white_bar.right = position_x + 16;
	white_bar.top = position_y;
	white_bar.bottom = position_y + 3;
	fill_rectangle(white_bar, RED_MAX);
	// DASH
	int i;
	for(i = 0;i<3;i++) {
		white_dashes.left = position_x + i*6;
		white_dashes.right = position_x + i*6 + 4;
		white_dashes.top = position_y + 6;
		white_dashes.bottom = position_y + 9;
		fill_rectangle(white_dashes, RED_MAX);
	}
 }
 
 uint16_t status_Battery(int status) {
	 switch(status) {
		 case 0:
			return TURQ;	// Idle
		 case 1:
			return RED;		// Discharging
		 case 2:
			return GRASS;	// Charging
		 default:
			return GOLD;	// Not supposed to happen
	 }
 }

 void func_battery(int position_x, int position_y) {
	rectangle casing;
	rectangle tip;
	rectangle data;
	// CASING
	casing.left = position_x;
	casing.right = position_x + 52;
	casing.top = position_y;
	casing.bottom = position_y + 18;
	fill_rectangle(casing, WHITE);
	// TIP
	tip.left = position_x + 52;
	tip.right = position_x + 54;
	tip.top = position_y + 8;
	tip.bottom = position_y + 10;
	fill_rectangle(tip, WHITE);
	// EMPTY
	data.left = position_x + 2;
	data.right = position_x + 50;
	data.top = position_y + 2;
	data.bottom = position_y + 16;
	fill_rectangle(data, BLACK);
 }

 void update_battery(int position_x, int position_y, int charge, int status) {
	rectangle data;
	// DATA
	data.left = position_x + 2;
	data.right = position_x + 50;
	data.top = position_y + 2;
	data.bottom = position_y + 16;
	fill_rectangle(data, BLACK);
	data.right = position_x + 2*charge + 2;
	fill_rectangle(data, status_Battery(status));
 }

 uint16_t status_Loads(int status_Load) {
	 switch(status_Load) {
		 case 0:
			return WHITE;	// No demand
		 case 1:
			return RED;		// Demand but no supply					
		 case 2:
			return GREEN;	// Supplied
		 default:
			return GOLD;	// Not supposed to happen
	 }
 }
 
 void Loads(int position_x, int position_y, int status_Load1, int status_Load2, int status_Load3) {
	rectangle Load1;
	rectangle Load2;
	rectangle Load3;
	
	// Load1
	Load1.left = position_x;
	Load1.right = position_x + 20;
	Load1.top = position_y;
	Load1.bottom = position_y + 20;
	fill_rectangle(Load1, status_Loads(status_Load1));

	// Load2
	Load2.left = position_x + 33;
	Load2.right = position_x + 53;
	Load2.top = position_y;
	Load2.bottom = position_y + 20;
	fill_rectangle(Load2, status_Loads(status_Load2));

	// Load3
	Load3.left = position_x + 66;
	Load3.right = position_x + 86;
	Load3.top = position_y;
	Load3.bottom = position_y + 20;
	fill_rectangle(Load3, status_Loads(status_Load3));
 }

 void picture_draw(int left_, int top_, uint8_t* array, int length, uint16_t col) {
	picture p;
	p.left = left_;
	p.top = top_;
	int i;
	for(i = 0; i<length; i++) {
		p.image_data[i] = *(array+i);
	}
	drawer(&p,col);
 }
 
 /*
 int main() {
	GUI_startup();
	GUI_mainscreen();
 }*/
