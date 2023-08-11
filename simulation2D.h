#pragma once
#include "window.h"

/*	Idee: Man hat eine Kamera fest auf dem eigenen Boot und andere Boote die davor oder dahinter sein können,
 *	jedes Boot hat eine Distanz gespeichert und so kann man berechnen ob diese davor oder dahinter sind,
 *	es wird angezeigt auf welchem Platz man ist und wie weit die anderen vor oder hinter einem sind.
 *	Die Boote sind so schnell wie die Intensität die man will +-(1-2%) das immer durchwechselt
 *	TODO überlegen wie man Animationen und de- und acceleration darstellen kann
 *	TODO ein/ein paar Boot/e könnten vllt immer schneller sein wie der Spieler damit dieser sich anstrengt,... mal sehen
 */

#define BOATCOUNT_MAX 6
//Spieler ist immer Boot 0
struct VirtualRowing2D{
	BYTE boat_count = 0;					//Anzahl der Boote
	Image boat_images[BOATCOUNT_MAX];		//Bilder der Boote
	float distances[BOATCOUNT_MAX] = {};	//Distanz der einzelnen Boote
	WORD speeds[BOATCOUNT_MAX];				//Geschwindigkeit der einzelnen Boote
	WORD intensity = 360;					//Gewünschte Intensität die die Boote fahren sollen (in m/s)
	SYSTEMTIME last_time;					//Letzter Zeitpunkt an dem die Simulation geupdated wurde
	WORD millis = 0;						//Zählt die Millisekunden
};

//Die Anzahl der Image Pfade muss gleich dem boat_count sein!
void createVirtualRowing2D(VirtualRowing2D*& sim, const char** images_paths, BYTE boat_count=1){
	if(sim) return;
	sim = new VirtualRowing2D;
	for(BYTE i=0; i < boat_count; ++i){
		if(ErrCheck(loadImage(images_paths[i], sim->boat_images[sim->boat_count])) != SUCCESS) continue;
		sim->distances[sim->boat_count] = 0;
		sim->boat_count++;
	}
}

void destroyVirtualRowing2D(VirtualRowing2D*& sim){
	for(BYTE i=0; i < sim->boat_count; ++i){
		destroyImage(sim->boat_images[i]);
	}
	delete sim;
	sim = nullptr;
}

void initVirtualRowing2D(VirtualRowing2D& sim, WORD intensity){
	for(BYTE i=0; i < sim.boat_count; ++i){
		sim.distances[i] = 0;
	}
	sim.intensity = intensity;
	for(BYTE i=0; i < sim.boat_count; ++i){
		float random = 1+((rand()%16001)/100000.f-0.08);
		sim.speeds[i] = sim.intensity*random;
	}
	GetSystemTime(&sim.last_time);
	srand(sim.last_time.wMilliseconds);
}

//TODO Boote starten direkt mit der intensity, sollten am Start aber langsam beschleunigen, Messdaten notwendig
void updateVirtualRowing2D(VirtualRowing2D& sim, HWND window, Font& font, WORD avg_ms){
	if(sim.boat_count < 1) return;
	SYSTEMTIME tp;
	GetSystemTime(&tp);
	DWORD millis = tp.wMilliseconds-sim.last_time.wMilliseconds;
	millis += (tp.wSecond-sim.last_time.wSecond)*1000;
	millis += (tp.wMinute-sim.last_time.wMinute)*60000;
	millis += (tp.wHour-sim.last_time.wHour)*3600000;
	sim.millis += millis;
	float dt = millis/1000.f;
	sim.last_time = tp;	//TODO Kopiert das ganze struct, unnötig
	//Update die Positionen der anderen Boote
	for(BYTE i=1; i < sim.boat_count; ++i){
		sim.distances[i] += sim.speeds[i]*0.01*dt;
	}
	sim.distances[0] += avg_ms*0.01*dt;	//Update die Position des eigenen Bootes
	//Update die Geschwindigkeiten
	if(sim.millis >= 20000){	//Alle 20 Sekunden
		for(BYTE i=0; i < sim.boat_count; ++i){
			float random = 1+((rand()%16001)/100000.f-0.08);
			sim.speeds[i] = sim.intensity*random;
		}
	}
	//Zeichne die Boote
	WORD window_idx;
	getWindow(window, window_idx);
	WORD width = app.info[window_idx].window_width/app.info[window_idx].pixel_size;
	WORD height = app.info[window_idx].window_height/app.info[window_idx].pixel_size;
	WORD boat_positions = height/6;
	WORD boat_size = boat_positions*0.8;
	WORD boat_spacing = boat_positions*0.1;
	//Zeichne das eigene Boot
	float boat_y = (float)boat_size/sim.boat_images[0].height;
	drawLine(window_idx, 0, 0, width-1, 0, RGBA(255, 255, 255));
	copyImageToWindow(window_idx, sim.boat_images[0], width/2-sim.boat_images[0].width*boat_y/2, boat_spacing, width/2+sim.boat_images[0].width*boat_y/2, boat_spacing+boat_size);
	for(BYTE i=1; i < sim.boat_count; ++i){
		WORD pos = boat_positions*i;
		boat_y = (float)boat_size/sim.boat_images[i].height;
		drawLine(window_idx, 0, pos, width-1, pos, RGBA(255, 255, 255));
		float x_offset = (sim.distances[i]-sim.distances[0])*20.;
		copyImageToWindowSave(window_idx, sim.boat_images[i], width/2-sim.boat_images[i].width*boat_y/2+x_offset, pos+boat_spacing, width/2+sim.boat_images[i].width*boat_y/2+x_offset, pos+boat_spacing+boat_size);
		if(x_offset <= 0){
			WORD tmp = font.font_size;
			font.font_size = boat_spacing*4;
			std::string dist = floatToString(x_offset/20.) + 'm';
			WORD offset = 0;
			for(size_t i=0; i < dist.size(); ++i){
				offset += drawFontChar(window_idx, font, dist[i], 10+offset, pos);
			}
			font.font_size = tmp;
		}
		else if(x_offset > 0){
			WORD tmp = font.font_size;
			font.font_size = boat_spacing*4;
			std::string dist = floatToString(x_offset/20.) + 'm';
			WORD init_offset = getStringFontSize(font, dist);
			WORD offset = 0;
			for(size_t i=0; i < dist.size(); ++i){
				offset += drawFontChar(window_idx, font, dist[i], width-init_offset-10+offset, pos);
			}
			font.font_size = tmp;
		}
	}
}
