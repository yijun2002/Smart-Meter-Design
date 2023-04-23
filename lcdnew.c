/*  Author: Steve Gunn
 * Licence: This work is licensed under the Creative Commons Attribution License.
 *           View this license at http://creativecommons.org/about/licenses/
 */
 
#include "ili934x.h"
#include "font.h"
#include "lcd.h"
#define W 5
#define H 7

lcd display = {LCDWIDTH, LCDHEIGHT, North, 0, 0, WHITE, BLACK};

void init_lcd()
{
	/* Disable JTAG in software, so that it does not interfere with Port C  */
	/* It will be re-enabled after a power cycle if the JTAGEN fuse is set. */
	MCUCR |= (1<<JTD);
	MCUCR |= (1<<JTD);
	
	/* Configure ports */
	CTRL_DDR = 0x7F;
	DATA_DDR = 0xFF;
	
	init_display_controller();
}

void set_orientation(orientation o)
{
	display.orient = o;
	write_cmd(MEMORY_ACCESS_CONTROL);
	if (o==North) { 
		display.width = LCDWIDTH;
		display.height = LCDHEIGHT;
		write_data(0x48);
	}
	else if (o==West) {
		display.width = LCDHEIGHT;
		display.height = LCDWIDTH;
		write_data(0xE8);
	}
	else if (o==South) {
		display.width = LCDWIDTH;
		display.height = LCDHEIGHT;
		write_data(0x88);
	}
	else if (o==East) {
		display.width = LCDHEIGHT;
		display.height = LCDWIDTH;
		write_data(0x28);
	}
	write_cmd(COLUMN_ADDRESS_SET);
	write_data16(0);
	write_data16(display.width-1);
	write_cmd(PAGE_ADDRESS_SET);
	write_data16(0);
	write_data16(display.height-1);
}

void fill_rectangle(rectangle r, uint16_t col)
{
	uint16_t x, y;
	write_cmd(COLUMN_ADDRESS_SET);
	write_data16(r.left);
	write_data16(r.right);
	write_cmd(PAGE_ADDRESS_SET);
	write_data16(r.top);
	write_data16(r.bottom);
	write_cmd(MEMORY_WRITE);
	for(x=r.left; x<=r.right; x++)
		for(y=r.top; y<=r.bottom; y++)
			write_data16(col);
}
/*
void fill_rectangle_indexed(rectangle r, uint16_t* col)
{
	uint16_t x, y;
	write_cmd(COLUMN_ADDRESS_SET);
	write_data16(r.left);
	write_data16(r.right);
	write_cmd(PAGE_ADDRESS_SET);
	write_data16(r.top);
	write_data16(r.bottom);
	write_cmd(MEMORY_WRITE);
	for(x=r.left; x<=r.right; x++)
		for(y=r.top; y<=r.bottom; y++)
			write_data16(*col++);
}*/

void clear_screen()
{
	display.x = 0;
	display.y = 0;
	rectangle r = {0, display.width-1, 0, display.height-1};
	fill_rectangle(r, display.background);
}

void display_char(char c,int pos_x,int pos_y)
{
	display.x = pos_x;
	display.y = pos_y;
	uint16_t x, y;
	PGM_P fdata; 
	uint8_t bits, mask; // Changed to uint_16t from 8
	uint16_t sc=display.x, ec=display.x + W - 1, sp=display.y, ep=display.y + H;
	if (c < 32 || c > 126) return;
	fdata = (c - ' ') * W + font5x7; // Changed this line as well
	write_cmd(PAGE_ADDRESS_SET);
	write_data16(sp);
	write_data16(ep);
	for(x=sc; x<=ec; x++) {
		write_cmd(COLUMN_ADDRESS_SET);
		write_data16(x);
		write_data16(x);
		write_cmd(MEMORY_WRITE);
		bits = pgm_read_byte(fdata++);
		for(y=sp, mask=0x01; y<=ep; y++, mask<<=1) // changed mask = 0x01 to this
			write_data16((bits & mask) ? display.foreground : display.background);
	}
	write_cmd(COLUMN_ADDRESS_SET);
	write_data16(x);
	write_data16(x);
	write_cmd(MEMORY_WRITE);
	for(y=sp; y<=ep; y++)
		write_data16(display.background);
}

void display_string(char *str,int pos_x,int pos_y)
{
	uint8_t i;
	for(i=0; str[i]; i++) {
		display_char(str[i],pos_x,pos_y);
		pos_x += (W+3);
		if (pos_x >= display.width) { pos_x=0; pos_y += (H+3); }
	}
}

// If you are changing the size then look at height and width
void drawer(picture* p, uint16_t col)
{
	int height = 20;
	int width = 20;
	uint16_t x, y;
	write_cmd(COLUMN_ADDRESS_SET);
	write_data16(p->left);
	write_data16(p->left + width - 1);
	write_cmd(PAGE_ADDRESS_SET);
	write_data16(p->top);
	write_data16(p->top + height - 1);
	write_cmd(MEMORY_WRITE);
	for(y=0; y<=height-1; y++) {
		for(x=0; x<=width-1; x++) {
			if(p->image_data[(width*y + x)] == 0xff) {
				write_data16(col);
			}
			else {
				write_data16(BLACK);
			}
		}
	}
}