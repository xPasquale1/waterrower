#pragma once

#include "util.h"
#include "window.h"
#include "fonts/5x5font.h"

//Geht nur von 0-9!!!
//TODO alle Funktionen hier sind nur auf die 5x5 font abgestimmt, sollte man verallgemeinern
inline void draw_number(uint x, uint y, uint size, uchar number, uint color){
	short idx = (number+48)*5;
	for(uint i=0; i < 5; ++i){
		for(uint j=0; j < 5; ++j){
			if((font5x5[idx]<<j)&0b10000) draw_rectangle(x+j*size, y+i*size, size, size, color);
		}
		++idx;
	}
}

static int _rec_offset = 0;
void _number_recurse(uint x, uint y, uint size, int num, uint color, int iter){
	if(num > 0){
		++_rec_offset;
		_number_recurse(x, y, size, num/10, color, iter+1);
		draw_number(x+(_rec_offset-iter-1)*5*size, y, size, num%10, color);
	}
}

void draw_character(uint x, uint y, uint size, uchar character, uint color){
	short idx = character*5;
	for(uint i=0; i < 5; ++i){
		for(uint j=0; j < 5; ++j){
			if((font5x5[idx]<<j)&0b10000) draw_rectangle(x+j*size, y+i*size, size, size, color);
		}
		++idx;
	}
}

void draw_int(uint x, uint y, uint size, int num, uint color){
	if(num == 0){
		draw_number(x, y, size, 0, color);
		return;
	}
	_rec_offset = 0;
	if(num < 0){
		_rec_offset = 1;
		num *= -1;
		draw_character(x, y, size, '-', color);
	}
	_number_recurse(x, y, size, num, color, 0);
}
