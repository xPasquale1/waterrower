#include <iostream>
#include <winusb.h>
extern "C"{
	#include <hidsdi.h>
}
#include <setupapi.h>

//ret: Handle für das Gerät
HANDLE open_device(const char* devicePath = "\\\\.\\COM3", DWORD baudrate = 9600){
	HANDLE hDevice = CreateFile(devicePath, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	if(hDevice == INVALID_HANDLE_VALUE){
		std::cout << "Fehler beim Öffnen des Gerätehandles! " << GetLastError() << std::endl;
		exit(-1);
	}
    DCB dcbSerialParams = {};
    COMMTIMEOUTS timeouts = {};
	//Konfiguriere die Schnittstelle (Baudrate, Datenbits, Parität, Stoppbits)
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	if(!GetCommState(hDevice, &dcbSerialParams)){
		std::cout << "Fehler beim Abrufen der Schnittstelle-Einstellungen! " << GetLastError() << std::endl;
		CloseHandle(hDevice);
		exit(-1);
	}
	dcbSerialParams.BaudRate = baudrate;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;
	if(!SetCommState(hDevice, &dcbSerialParams)){
		std::cout << "Fehler beim Konfigurieren der Schnittstelle-Einstellungen! " << GetLastError() << std::endl;
		CloseHandle(hDevice);
		exit(-1);
	}
	//Konfiguriere die Timeouts für den Lesevorgang
	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;
	if(!SetCommTimeouts(hDevice, &timeouts)){
		std::cout << "Fehler beim Konfigurieren der Timeouts! " << GetLastError() << std::endl;
		CloseHandle(hDevice);
		exit(-1);
	}
	return hDevice;
}

//ret: Anzahl der gesendeten Bytes
int sendData(HANDLE hDevice, BYTE* data, DWORD length){
	DWORD bytesSent;
	if(!WriteFile(hDevice, data, length, &bytesSent, 0)){
		std::cout << "Fehler beim Senden der Daten! " << GetLastError() << std::endl;
		CloseHandle(hDevice);
		exit(-1);
	}
	return bytesSent;
}

//Liest die Daten im Empfangspuffer, diese können noch unvollständig sein
//ret: Anzahl der gesendeten Bytes
int readData(HANDLE hDevice, BYTE* data, DWORD length){
	DWORD bytesRead;
	if(!ReadFile(hDevice, data, length, &bytesRead, 0)){
		std::cout << "Fehler beim Lesen der Daten! " << GetLastError() << std::endl;
		CloseHandle(hDevice);
		exit(-1);
	}
	return bytesRead;
}

//Liest die Daten im Empfangspuffer eins nach dem anderen, liest solange bis ein CRLF am ende der Daten steht
//Gibt -1 zurück, falls keien Daten im Empfangspuffer sind, oder 50Bytes überschritten wurden
int readPacket(HANDLE hDevice, BYTE* data, DWORD length){
	DWORD bytesRead;
	DWORD currentRead = 0;
	BYTE byte;
	if(!ReadFile(hDevice, &byte, 1, &bytesRead, 0)){
		std::cout << "Fehler beim Lesen der Daten! " << GetLastError() << std::endl;
		CloseHandle(hDevice);
		exit(-1);
	}
	if(bytesRead <= 0) return -1;
	data[currentRead++] = byte;
	while(1){
		if(!ReadFile(hDevice, &byte, 1, &bytesRead, 0)){
			std::cout << "Fehler beim Lesen der Daten! " << GetLastError() << std::endl;
			CloseHandle(hDevice);
			exit(-1);
		}
		if(bytesRead > 0) data[currentRead++] = byte;
		if(currentRead > 50) return -1;
		if(currentRead > 1 && data[currentRead-2] == 0x0D && data[currentRead-1] == 0x0A){
			return currentRead;
		}
	}
}

//Sendet die Daten mit allen nötigen zusätzlichen Zeichen (max. 48 Bytes Nutzdaten)
//-1 falls Daten mehr wie 48 Byte enthalten
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
		CloseHandle(hDevice);
		exit(-1);
	}
	return bytesSent;
}

void print_packet(BYTE* buffer, int length){
	SYSTEMTIME lpSystemTime;
	GetSystemTime(&lpSystemTime);
	std::cout << "Packet " << lpSystemTime.wHour << ':' << lpSystemTime.wMinute << ':' << lpSystemTime.wSecond << '.' << lpSystemTime.wMilliseconds << std::endl;
	for(int i=0; i < length; ++i){
		std::cout << buffer[i];
	}
	std::cout << std::endl;
}

int init_communication(HANDLE hDevice, BYTE* sendBuffer, BYTE* receiveBuffer){
	int length;
	strcpy((char*)sendBuffer, "USB");
	sendPacket(hDevice, sendBuffer, sizeof("USB")-1);
	while((length = readPacket(hDevice, receiveBuffer, 128)) < 1);
	print_packet(receiveBuffer, length);
	strcpy((char*)sendBuffer, "IV?");
	sendPacket(hDevice, sendBuffer, sizeof("IV?")-1);
	while((length = readPacket(hDevice, receiveBuffer, 128)) < 1);
	print_packet(receiveBuffer, length);
	return 0;
}

/* Der Waterrower sollte nicht mit Packeten überlastet werden, daher wird empfohlen nur ca. alle 25ms ein Packet zu senden
   Diese Funktionen senden requests für e.g. Speicherdaten,... indem die Packete in eine Warteschlange eingereiht werden
   und alle 25ms das erste gesendet wird
*/

struct Request{
	BYTE data[50];	//Daten
	BYTE length;	//Länge der Nachricht
};
static SYSTEMTIME last_request_tp = {};
#define REQUEST_QUEUE_SIZE 40
static Request requests[REQUEST_QUEUE_SIZE];
static int request_ptr1 = 0;
static int request_ptr2 = 0;

//Fügt eine request in die Warteschlange ein, id gibt die request an
int addRequest(DWORD id){
	if((request_ptr1+1)%(REQUEST_QUEUE_SIZE) == request_ptr2) return -1;	//Warteschlange voll
	switch(id){
	case 0:{	//Distanzabfrage
		strcpy((char*)requests[request_ptr1].data, "IRT057");
		requests[request_ptr1].length = sizeof("IRT057")-1;
		break;
	}
	case 1:{	//Sekundenabfrage
		strcpy((char*)requests[request_ptr1].data, "IRS1E1");
		requests[request_ptr1].length = sizeof("IRS1e1")-1;
		break;
	}
	case 2:{	//Minutenabfrage
		strcpy((char*)requests[request_ptr1].data, "IRS1E2");
		requests[request_ptr1].length = sizeof("IRS1e2")-1;
		break;
	}
	case 3:{	//Stundenabfrage
		strcpy((char*)requests[request_ptr1].data, "IRS1E3");
		requests[request_ptr1].length = sizeof("IRS1e3")-1;
		break;
	}
	}
	request_ptr1 = (request_ptr1+1)%(REQUEST_QUEUE_SIZE);
	return 0;
}

//Diese Funktion sendet die erste request aus der Warteschlange zum waterrower sobald 25ms seit der Letzten verstrichen sind
int transmitRequests(HANDLE hDevice){
	if(request_ptr1 == request_ptr2) return 1;	//Keine Daten in der Warteschlange
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
	if(newmilli - currentmilli > 25){
		last_request_tp = systemTime;
		sendPacket(hDevice, requests[request_ptr2].data, requests[request_ptr2].length);
		request_ptr2 = (request_ptr2+1)%(REQUEST_QUEUE_SIZE);
	}
	return 0;
}

struct RowingData{
	WORD dist = 0;
	WORD ms_total = 0;
	WORD ms_avg = 0;
	BYTE sec = 0;
	BYTE min = 0;
	BYTE hrs = 0;
}; static RowingData rowingData;

constexpr inline int codeToInt(const char* code){
	return (code[0]|code[1]<<8|code[2]<<16);
}

int checkCode(BYTE* receiveBuffer, int length){
	BYTE code[3]; code[0] = receiveBuffer[0]; code[1] = receiveBuffer[1]; code[2] = receiveBuffer[2];
	switch(codeToInt((char*)code)){
	case codeToInt("ERR"):{
		print_packet(receiveBuffer, length);
		break;
	}
	case codeToInt("IDT"):{
		BYTE value[7]; value[6] = '\0';
		for(int i=0; i < 6; ++i){
			value[i] = receiveBuffer[i+6];
		}
		BYTE location[3];	//TODO naja das könnte ja eigentlich alles sein, daher noch den Speicherbereich checken
		for(int i=0; i < 3; ++i){
			location[i] = receiveBuffer[i+3];
		}
		rowingData.dist = strtol((char*)value, 0, 16);
		print_packet(receiveBuffer, length);
		break;
	}
	case codeToInt("IDS"):{
		BYTE value[3]; value[2] = '\0';
		for(int i=0; i < 2; ++i){
			value[i] = receiveBuffer[i+6];
		}
		BYTE location[3];
		for(int i=0; i < 3; ++i){
			location[i] = receiveBuffer[i+3];
		}
		switch(codeToInt((char*)location)){
		case codeToInt("1E1"):{
			rowingData.sec = strtol((char*)value, 0, 10);
			break;
		}
		case codeToInt("1E2"):{
			rowingData.min = strtol((char*)value, 0, 10);
			break;
		}
		case codeToInt("1E3"):{
			rowingData.hrs = strtol((char*)value, 0, 10);
			break;
		}
		}
		print_packet(receiveBuffer, length);
		break;
	}
	}
	return 0;
}
