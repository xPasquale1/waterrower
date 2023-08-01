#include "window.h"

struct DataPoint{
	WORD x;
	WORD y;
};

ErrCode drawGraph(HWND window, uint x, uint y, uint size_x, uint size_y, DataPoint* data, WORD data_count){
	for(WORD i=0; i < app.window_count; ++i){
		if(app.windows[i] == window){
			uint inc = size_y/5;
			drawRectangle(i, x, y, size_x, size_y, RGBA(40, 40, 40));
			drawLine(i, x, y, x+size_x, y, RGBA(70, 70, 70));
			drawLine(i, x, y, x, y+size_y, RGBA(70, 70, 70));
			drawLine(i, x, y+size_y, x+size_x, y+size_y, RGBA(70, 70, 70));
			drawLine(i, x+size_x, y, x+size_x, y+size_y, RGBA(70, 70, 70));
			for(WORD j=1; j < 5; ++j){
				drawLine(i, x, y+inc*j, x+size_x, y+inc*j, RGBA(70, 70, 70));
			}
			inc = size_x/(data_count+1);
			WORD min_y = data[0].y; WORD max_y = data[0].y;
			for(WORD j=0; j < data_count; ++j){
				if(data[j].y < min_y) min_y = data[j].y;
				if(data[j].y > max_y) max_y = data[j].y;
			}
			WORD min_x = data[0].x; WORD max_x = data[0].x;
			for(WORD j=0; j < data_count; ++j){
				if(data[j].x < min_x) min_x = data[j].x;
				if(data[j].x > max_x) max_x = data[j].x;
			}
			WORD last_x; WORD last_y;
			for(WORD j=1; j < data_count+1; ++j){
				float pos_y = ((float)data[j-1].y-min_y)/(max_y-min_y);
				float pos_x = ((float)data[j-1].x-min_x)/(max_x-min_x);
				drawRectangle(i, x+5+pos_x*(size_x-10)-1, y+size_y-5-pos_y*(size_y-10)-1, 3, 3, RGBA(90, 90, 90));
				if(j > 1) drawLine(i, x+5+pos_x*(size_x-10), y+size_y-5-pos_y*(size_y-10), last_x, last_y, RGBA(120, 0, 0));
				last_x = x+5+pos_x*(size_x-10); last_y = y+size_y-5-pos_y*(size_y-10);
			}

			return SUCCESS;
		}
	}
	return WINDOW_NOT_FOUND;
}
