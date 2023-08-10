#pragma once
#include <iostream>
#include <winusb.h>
extern "C"{
	#include <hidsdi.h>
}
#include <setupapi.h>
#include "util.h"

#define SILENT		//Packete werden nicht in den output stream geschrieben
//#define SHOW_ERRORS	//Überschreibt SILENT nur für Fehlernachrichten

//ret: Handle für das Gerät
ErrCode openDevice(HANDLE& handle, const char* devicePath = "\\\\.\\COM3", DWORD baudrate = 9600){
	HANDLE hDevice = CreateFile(devicePath, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	if(hDevice == INVALID_HANDLE_VALUE){
		std::cerr << GetLastError() << std::endl;
		return INVALID_USB_HANDLE;
	}
    DCB dcbSerialParams = {};
    COMMTIMEOUTS timeouts = {};
	//Konfiguriere die Schnittstelle (Baudrate, Datenbits, Parität, Stoppbits)
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	if(!GetCommState(hDevice, &dcbSerialParams)){
		std::cerr << GetLastError() << std::endl;
		CloseHandle(hDevice);
		return COMMSTATE_ERROR;
	}
	dcbSerialParams.BaudRate = baudrate;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;
	if(!SetCommState(hDevice, &dcbSerialParams)){
		std::cerr << GetLastError() << std::endl;
		CloseHandle(hDevice);
		return COMMSTATE_ERROR;
	}
	//Konfiguriere die Timeouts für den Lesevorgang
	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;
	if(!SetCommTimeouts(hDevice, &timeouts)){
		std::cerr << GetLastError() << std::endl;
		CloseHandle(hDevice);
		return TIMEOUT_SET_ERROR;
	}
	handle = hDevice;
	return SUCCESS;
}

//ret: Anzahl der gesendeten Bytes, -1 bei Fehler
int sendData(HANDLE hDevice, BYTE* data, DWORD length){
	DWORD bytesSent;
	if(!WriteFile(hDevice, data, length, &bytesSent, 0)){
		std::cout << "Fehler beim Senden der Daten! " << GetLastError() << std::endl;
		return -1;
	}
	return bytesSent;
}

//Liest die Daten im Empfangspuffer, diese können noch unvollständig sein
//ret: Anzahl der gelesenen Bytes
int readData(HANDLE hDevice, BYTE* data, DWORD length){
	DWORD bytesRead;
	if(!ReadFile(hDevice, data, length, &bytesRead, 0)){
		std::cout << "Fehler beim Lesen der Daten! " << GetLastError() << std::endl;
		return -1;
	}
	return bytesRead;
}

//Liest die Daten im Empfangspuffer eins nach dem anderen, liest solange bis ein CRLF am ende der Daten steht
//Gibt -1 zurück, falls keien Daten im Empfangspuffer sind, oder 50Bytes überschritten wurden, oder API Fehler
int readPacket(HANDLE hDevice, BYTE* data, DWORD length){
	DWORD bytesRead;
	DWORD currentRead = 0;
	BYTE byte;
	if(!ReadFile(hDevice, &byte, 1, &bytesRead, 0)){
		std::cout << "Fehler beim Lesen der Daten! " << GetLastError() << std::endl;
		return -1;
	}
	if(bytesRead <= 0) return -1;
	data[currentRead++] = byte;
	while(1){
		if(!ReadFile(hDevice, &byte, 1, &bytesRead, 0)){
			std::cout << "Fehler beim Lesen der Daten! " << GetLastError() << std::endl;
			return -1;
		}
		if(bytesRead > 0) data[currentRead++] = byte;
		if(currentRead > 50) return -1;
		if(currentRead > 1 && data[currentRead-2] == 0x0D && data[currentRead-1] == 0x0A){
			return currentRead;
		}
	}
}

//Sendet die Daten mit allen nötigen zusätzlichen Zeichen (max. 48 Bytes Nutzdaten)
//-1 falls Daten mehr wie 48 Byte enthalten oder API Fehler
int sendPacket(HANDLE hDevice, BYTE* data, DWORD length){
	if(length > 48) return -1;
	BYTE packet[50];
	for(DWORD i=0; i < length; ++i){
		packet[i] = data[i];
	} packet[length] = 0x0D; packet[length+1] = 0x0A;
	length += 2;
	DWORD bytesSent;
	if(!WriteFile(hDevice, packet, length, &bytesSent, 0)){
		std::cout << "Fehler beim Senden der Daten! " << GetLastError() << std::endl;
		return -1;
	}
	return bytesSent;
}

void printPacket(BYTE* buffer, int length){
	SYSTEMTIME lpSystemTime;
	GetSystemTime(&lpSystemTime);
	std::cout << "Packet " << lpSystemTime.wHour << ':' << lpSystemTime.wMinute << ':' << lpSystemTime.wSecond << '.' << lpSystemTime.wMilliseconds << std::endl;
	for(int i=0; i < length; ++i){
		std::cout << buffer[i];
	}
	std::cout << std::endl;
}

int initCommunication(HANDLE hDevice, BYTE* sendBuffer, BYTE* receiveBuffer){
	int length;
	strcpy((char*)sendBuffer, "USB");
	sendPacket(hDevice, sendBuffer, sizeof("USB")-1);
	while((length = readPacket(hDevice, receiveBuffer, 128)) < 1);
#ifndef SILENT
	printPacket(receiveBuffer, length);
#endif
	strcpy((char*)sendBuffer, "IV?");
	sendPacket(hDevice, sendBuffer, sizeof("IV?")-1);
	while((length = readPacket(hDevice, receiveBuffer, 128)) < 1);
#ifndef SILENT
	printPacket(receiveBuffer, length);
#endif
	return 0;
}

/* Der Waterrower sollte nicht mit Packeten überlastet werden, daher wird empfohlen nur ca. alle 25ms ein Packet zu senden
   Diese Funktionen senden Requests für e.g. Speicherdaten,... indem die Packete in eine Warteschlange eingereiht werden
   und alle 25ms das Älteste gesendet wird
*/

struct Request{
	BYTE data[50];	//Daten
	BYTE length;	//Länge der Nachricht
};
static SYSTEMTIME last_request_tp = {};

#define REQUEST_QUEUE_SIZE 50
struct RequestQueue{
	Request requests[REQUEST_QUEUE_SIZE];
	WORD request_ptr1 = 0;
	WORD request_ptr2 = 0;
};

//TODO für alle machen und unten einfügen
#define DISTANCECODE "054"
#define DISTANCELOCATION "IRT054"
#define SECONDSCODE "1E1"
#define SECONDSLOCATION "IRS1E1"
#define MINUTESCODE "1E2"
#define MINUTESLOCATION "IRS1E2"
#define HOURSCODE "1E3"
#define HOURSLOCATION "IRS1E3"

//Fügt eine request in die Warteschlange ein, id gibt die request an
ErrCode addRequest(RequestQueue& queue, DWORD id){
	if((queue.request_ptr1+1)%(REQUEST_QUEUE_SIZE) == queue.request_ptr2) return QUEUE_FULL;	//Warteschlange voll
	switch(id){
	case 0:{	//Distanzabfrage
		strcpy((char*)queue.requests[queue.request_ptr1].data, "IRT054");
		queue.requests[queue.request_ptr1].length = sizeof("IRT054")-1;
		queue.request_ptr1 = (queue.request_ptr1+1)%(REQUEST_QUEUE_SIZE);
		return SUCCESS;
	}
	case 1:{	//Sekundenabfrage
		strcpy((char*)queue.requests[queue.request_ptr1].data, "IRS1E1");
		queue.requests[queue.request_ptr1].length = sizeof("IRS1e1")-1;
		queue.request_ptr1 = (queue.request_ptr1+1)%(REQUEST_QUEUE_SIZE);
		return SUCCESS;
	}
	case 2:{	//Minutenabfrage
		strcpy((char*)queue.requests[queue.request_ptr1].data, "IRS1E2");
		queue.requests[queue.request_ptr1].length = sizeof("IRS1e2")-1;
		queue.request_ptr1 = (queue.request_ptr1+1)%(REQUEST_QUEUE_SIZE);
		return SUCCESS;
	}
	case 3:{	//Stundenabfrage
		strcpy((char*)queue.requests[queue.request_ptr1].data, "IRS1E3");
		queue.requests[queue.request_ptr1].length = sizeof("IRS1e3")-1;
		queue.request_ptr1 = (queue.request_ptr1+1)%(REQUEST_QUEUE_SIZE);
		return SUCCESS;
	}
	case 4:{	//Meter pro Sekunde total Abfrage
		strcpy((char*)queue.requests[queue.request_ptr1].data, "IRD148");
		queue.requests[queue.request_ptr1].length = sizeof("IRD148")-1;
		queue.request_ptr1 = (queue.request_ptr1+1)%(REQUEST_QUEUE_SIZE);
		return SUCCESS;
	}
	case 5:{	//Meter pro Sekunde durschnitts Abfrage
		strcpy((char*)queue.requests[queue.request_ptr1].data, "IRD14A");
		queue.requests[queue.request_ptr1].length = sizeof("IRD14A")-1;
		queue.request_ptr1 = (queue.request_ptr1+1)%(REQUEST_QUEUE_SIZE);
		return SUCCESS;
	}
	case 6:{	//Strokes Zähler
		strcpy((char*)queue.requests[queue.request_ptr1].data, "IRD140");
		queue.requests[queue.request_ptr1].length = sizeof("IRD140")-1;
		queue.request_ptr1 = (queue.request_ptr1+1)%(REQUEST_QUEUE_SIZE);
		return SUCCESS;
	}
	case 7:{	//Durchschnittszeit für einen gesamten Stroke
		strcpy((char*)queue.requests[queue.request_ptr1].data, "IRS142");
		queue.requests[queue.request_ptr1].length = sizeof("IRS142")-1;
		queue.request_ptr1 = (queue.request_ptr1+1)%(REQUEST_QUEUE_SIZE);
		return SUCCESS;
	}
	case 8:{	//Durschnittzeit für einen Zug (von Beschleunigung bis Entschleunigung)
		strcpy((char*)queue.requests[queue.request_ptr1].data, "IRS143");
		queue.requests[queue.request_ptr1].length = sizeof("IRS143")-1;
		queue.request_ptr1 = (queue.request_ptr1+1)%(REQUEST_QUEUE_SIZE);
		return SUCCESS;
	}
	case 9:{	//Wasservolumen
		strcpy((char*)queue.requests[queue.request_ptr1].data, "IRS0A9");
		queue.requests[queue.request_ptr1].length = sizeof("IRS0A9")-1;
		queue.request_ptr1 = (queue.request_ptr1+1)%(REQUEST_QUEUE_SIZE);
		return SUCCESS;
	}
	case 10:{	//Gesamte Zeit auf einmal (Sekuden + Minuten + Stunden)
		strcpy((char*)queue.requests[queue.request_ptr1].data, "IRT1E1");
		queue.requests[queue.request_ptr1].length = sizeof("IRT1E1")-1;
		queue.request_ptr1 = (queue.request_ptr1+1)%(REQUEST_QUEUE_SIZE);
		return SUCCESS;
	}
	}
	return REQUEST_NOT_FOUND;
}

//Diese Funktion sendet die erste request aus der Warteschlange zum waterrower sobald 25ms seit der Letzten verstrichen sind
void transmitRequests(RequestQueue& queue, HANDLE hDevice){
	if(queue.request_ptr1 == queue.request_ptr2) return;	//Keine Daten in der Warteschlange
	SYSTEMTIME systemTime;
	GetSystemTime(&systemTime);
	DWORD newmilli = systemTime.wMilliseconds;
	newmilli += systemTime.wSecond*1000;
	newmilli += systemTime.wMinute*60000;
	newmilli += systemTime.wHour*3600000;
	DWORD currentmilli = last_request_tp.wMilliseconds;
	currentmilli += last_request_tp.wSecond*1000;
	currentmilli += last_request_tp.wMinute*60000;
	currentmilli += last_request_tp.wHour*3600000;
	if(newmilli - currentmilli >= 25){
		last_request_tp = systemTime; //TODO Kopiert das ganze struct, unnötig
		sendPacket(hDevice, queue.requests[queue.request_ptr2].data, queue.requests[queue.request_ptr2].length);
		queue.request_ptr2 = (queue.request_ptr2+1)%(REQUEST_QUEUE_SIZE);
	}
}

struct Position{
	WORD second;	//Zeit seit Beginn der Messung
	WORD distance;	//Distanz
};

struct Timepoint{
	BYTE sec;
	BYTE min;
	BYTE hrs;
};

struct Distance{
	BYTE lower;
	WORD upper;
};

struct RowingData{
	Distance dist = {0};	//Distanz zurückgelegt, Byte 0 ist die .x Distanz, Byte 1-2 sind die Distanz in Meter
	Timepoint time = {0};	//Ruderzeit
	WORD last_sec = 0;		//Letzter Zeitpunkt in Sekunden
	WORD last_dist = 0;		//Letzte Distanz
	WORD ms_total = 0;		//Aktuelle Geschwindigkeit
	WORD ms_avg = 0;		//Durschnittliche Geschwindigkeit

	WORD strokes = 0;		//Anzahl der Strokes
	BYTE stroke_avg = 0;	//Durschnittliche Zeit für einen Stroke
	BYTE stroke_pull = 0;	//Durschnittliche Zeit für einen Zug (Von Beschleunigung bis Entschleunigung)
	BYTE volume = 0;		//Volumen an Wasser im Wassertank
}; static RowingData rowingData;

void initRowingData(RowingData& data){
	data = {0, 0, 0, 0, 0, 0, 0};
}

//3 Bytes zu int
constexpr inline int codeToInt3(const char* code){
	return (code[0]|code[1]<<8|code[2]<<16);
}
//2 Bytes zu int
constexpr inline int codeToInt2(const char* code){
	return (code[0]|code[1]<<8);
}

ErrCode checkCode(BYTE* receiveBuffer, int length){
	BYTE code[3]; code[0] = receiveBuffer[0]; code[1] = receiveBuffer[1]; code[2] = receiveBuffer[2];
	switch(codeToInt3((char*)code)){

	case codeToInt3("ERR"):{	//Fehlermeldung
#if defined(SHOW_ERRORS)
		printPacket(receiveBuffer, length);
#elif !defined(SILENT)
		printPacket(receiveBuffer, length);
#endif
		break;
	}

	case codeToInt3("IDT"):{	//Dreifacher Speicherbereich
		BYTE value[7]; value[6] = '\0';
		for(int i=0; i < 6; ++i){
			value[i] = receiveBuffer[i+6];
		}
		BYTE location[3];
		for(int i=0; i < 3; ++i){
			location[i] = receiveBuffer[i+3];
		}
		switch(codeToInt3((char*)location)){
		case codeToInt3("054"):{	//Das hier ist speziell
			rowingData.dist.lower = strtol((char*)value+2, 0, 16);
			value[4] = '\0';
			rowingData.dist.upper = strtol((char*)value, 0, 16);
			return SUCCESS;
		}
		case codeToInt3("1E1"):{	//Das hier ist auch speziell
			rowingData.time.sec = strtol((char*)value+4, 0, 10);
			value[4] = '\0';
			rowingData.time.min = strtol((char*)value+2, 0, 10);
			value[2] = '\0';
			rowingData.time.hrs = strtol((char*)value, 0, 10);
			return SUCCESS;
		}
		}
#ifndef SILENT
		printPacket(receiveBuffer, length);
#endif
		break;
	}

	case codeToInt3("IDD"):{	//Doppelter Speicherbereich
		BYTE value[5]; value[4] = '\0';
		for(int i=0; i < 4; ++i){
			value[i] = receiveBuffer[i+6];
		}
		BYTE location[3];
		for(int i=0; i < 3; ++i){
			location[i] = receiveBuffer[i+3];
		}
		switch(codeToInt3((char*)location)){
		case codeToInt3("148"):{
			rowingData.ms_total = strtol((char*)value, 0, 16);
			return SUCCESS;
		}
		case codeToInt3("14A"):{
			rowingData.ms_avg = strtol((char*)value, 0, 16);
			return SUCCESS;
		}
		case codeToInt3("140"):{
			rowingData.strokes = strtol((char*)value, 0, 16);
			return SUCCESS;
		}
		}
#ifndef SILENT
		printPacket(receiveBuffer, length);
#endif
		break;
	}

	case codeToInt3("IDS"):{	//Einzelner Speicherbereich
		BYTE value[3]; value[2] = '\0';
		for(int i=0; i < 2; ++i){
			value[i] = receiveBuffer[i+6];
		}
		BYTE location[3];
		for(int i=0; i < 3; ++i){
			location[i] = receiveBuffer[i+3];
		}
		switch(codeToInt3((char*)location)){
		case codeToInt3("1E1"):{
			rowingData.time.sec = strtol((char*)value, 0, 10);
			return SUCCESS;
		}
		case codeToInt3("1E2"):{
			rowingData.time.min = strtol((char*)value, 0, 10);
			return SUCCESS;
		}
		case codeToInt3("1E3"):{
			rowingData.time.hrs = strtol((char*)value, 0, 10);
			return SUCCESS;
		}
		case codeToInt3("142"):{
			rowingData.stroke_avg = strtol((char*)value, 0, 16);
			return SUCCESS;
		}
		case codeToInt3("143"):{
			rowingData.stroke_pull = strtol((char*)value, 0, 16);
			return SUCCESS;
		}
		case codeToInt3("0A9"):{
			rowingData.volume = strtol((char*)value, 0, 16);
			return SUCCESS;
		}
		}
#ifndef SILENT
		printPacket(receiveBuffer, length);
#endif
		break;
	}

	}

	switch(codeToInt2((char*)code)){
	case(codeToInt2("SS")):{
#ifndef SILENT
		printPacket(receiveBuffer, length);
#endif
		break;
	}
	case(codeToInt2("SE")):{
#ifndef SILENT
		printPacket(receiveBuffer, length);
#endif
		break;
	}
	}
	return CODE_NOT_FOUND;
}
