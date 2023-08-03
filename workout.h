#pragma once
#include <windows.h>
#include "util.h"

struct Workout{
	BYTE id = 0;			//0 -> Zeitworkout
	SYSTEMTIME last_time;	//Start-Zeitpunkt
	long duration = 0;		//in Sekunden
	WORD distance = 0;		//Ruderweite
};

void createWorkout(Workout*& workout){
	workout = new Workout;
}

void destroyWorkout(Workout*& workout){
	delete workout;
	workout = nullptr;
}

void initWorkout(Workout& workout){
	GetSystemTime(&workout.last_time);
}

//True falls workout zu Ende ist, false sonst
//Distanz wird aufaddiert
bool updateWorkout(Workout& workout, WORD distance){
	SYSTEMTIME new_time;
	GetSystemTime(&new_time);
	switch(workout.id){
	case 0:
		long seconds = new_time.wSecond-workout.last_time.wSecond+(new_time.wMinute-workout.last_time.wMinute)*60+(new_time.wHour-workout.last_time.wHour)*3600;
		workout.last_time = new_time;
		workout.duration -= seconds;
		workout.distance += distance;
		if(workout.duration < 0) return true;
		break;
	}
	return false;
}
