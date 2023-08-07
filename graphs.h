#pragma once
#include <fstream>
#include "window.h"
#include "util.h"

struct Statistic{
	WORD distance = 0;	//Tägliche Distanz in m
	SYSTEMTIME time;	//Zeitpunkt (nur Tag, Monat und Jahr werden gespeichert)
};

struct DataPoint{
	WORD x;
	WORD y;
};

//TODO Tag und Monat brauchen theoretisch nur jeweils ein Byte
ErrCode saveStatistic(Statistic& statistics, bool override = false){
	std::fstream file;
	file.open("data/stats.txt", std::ios::in | std::ios::out | std::ios::binary);
	if(!file.is_open()){
		file.close();
		return FILE_NOT_FOUND;
	}
	file.seekp(0, std::ios::end);
	DWORD size = file.tellp();
	if(override && size > 7) file.seekp(size-8, std::ios::beg);
	char* distance = (char*)(&statistics.distance);
	file.write(distance, 2);
	char* time = (char*)(&statistics.time.wDay);
	file.write(time, 2);
	time = (char*)(&statistics.time.wMonth);
	file.write(time, 2);
	time = (char*)(&statistics.time.wYear);
	file.write(time, 2);
	file.close();
	return SUCCESS;
}

ErrCode readStatistics(Statistic* data, WORD data_count){
	std::fstream file;
	file.open("data/stats.txt", std::ios::in | std::ios::binary);
	if(!file.is_open()) return FILE_NOT_FOUND;
	file.seekg(0, std::ios::end);
	DWORD size = file.tellg();
	DWORD offset = 8;
	for(WORD i=0; i < data_count; ++i, offset+=8){
		if(offset > size){
			file.close();
			return SUCCESS;
		}
		file.seekg(size-offset, std::ios::beg);
		file.read((char*)(&data[i].distance), 2);
		file.read((char*)(&data[i].time.wDay), 2);
		file.read((char*)(&data[i].time.wMonth), 2);
		file.read((char*)(&data[i].time.wYear), 2);
	}
	file.close();
	return SUCCESS;
}

//Zeichnet die DataPoints basierend auf den x Daten (immer inkrementell um 1)
ErrCode drawGraph(HWND window, Font& font, uint x, uint y, uint size_x, uint size_y, DataPoint* data, WORD data_count){
	for(WORD i=0; i < app.window_count; ++i){
		if(app.windows[i] == window){
			WORD inc_y = size_y/5;
			drawRectangle(i, x, y, size_x, size_y, RGBA(40, 40, 40));
			drawLine(i, x, y, x+size_x, y, RGBA(70, 70, 70));
			drawLine(i, x, y, x, y+size_y, RGBA(70, 70, 70));
			drawLine(i, x, y+size_y, x+size_x, y+size_y, RGBA(70, 70, 70));
			drawLine(i, x+size_x, y, x+size_x, y+size_y, RGBA(70, 70, 70));
			for(WORD j=1; j < 5; ++j){
				drawLine(i, x, y+inc_y*j, x+size_x, y+inc_y*j, RGBA(70, 70, 70));
			}
			//min, max x
			WORD min_x = data[0].x; WORD max_x = data[0].x;
			for(WORD j=0; j < data_count; ++j){
				if(data[j].x < min_x) min_x = data[j].x;
				if(data[j].x > max_x) max_x = data[j].x;
			}
			//X labels
			WORD inc_x = (size_x-10)/(max_x-min_x);
			WORD tmp = font.font_size;
			font.font_size = size_x*0.048;
			for(WORD j=0; j < (max_x-min_x)+1; ++j){
				std::string txt = std::to_string(min_x+j);
				for(size_t k=0; k < txt.size(); ++k){
					drawFontChar(i, font, txt[k], x+5-font.font_size/4+inc_x*j, y+size_y);
				}
			}
			font.font_size = tmp;
			//min, max y
			WORD min_y = data[0].y; WORD max_y = data[0].y;
			for(WORD j=0; j < data_count; ++j){
				if(data[j].y < min_y) min_y = data[j].y;
				if(data[j].y > max_y) max_y = data[j].y;
			}
			//Y labels (10 Daten)
			inc_y = (size_y-10)/9;
			float inc_y_data = ((float)(max_y-min_y))/9.f;
			tmp = font.font_size;
			font.font_size = size_x*0.048;
			for(WORD j=0; j < 10; ++j){
				std::string txt = std::to_string((WORD)(min_y+j*inc_y_data));
				WORD offset=0;
				WORD total_offset = getStringFontSize(font, txt);
				for(size_t k=0; k < txt.size(); ++k){
					offset += drawFontChar(i, font, txt[k], x+offset-total_offset, y+size_y-5-font.font_size/2-inc_y*j);
				}
			}
			font.font_size = tmp;
			//DataPoints zeichnen und mit Linien verbinden
			WORD last_x; WORD last_y;
			for(WORD j=1; j < data_count+1; ++j){
				float pos_y = ((float)data[j-1].y-min_y)/(max_y-min_y+1);
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
