/*
#include "CanDspic.h"
#include <includes.h>	// uC/OSII includes

extern lcdBox;
extern CanMutex;
extern HBCounter[10];

#define OFFSET 0x200

typedef enum MessageTypes {
    heartbeat = OFFSET+0,
    intrusion = OFFSET+1,
    disarming = OFFSET+2,
    arming = OFFSET+4,
    alarmStarted = OFFSET+8,
    newPassword = OFFSET+24,
	test = 0x180
} MessageTypes;

unsigned char index;

void actOnRecv(unsigned char offset) {
	INT8U err;
	switch(receiveBuffers[offset].SID) {
		case(heartbeat):
			//detect from which node 0-9 excluding ours
			OSMutexPend(CanMutex, 0, &err);
			index = receiveBuffers[offset].DATA[0];
			HBCounter[index-1] = 230;	//Reset Counter with ID, the '-1' is due to a unexpected offset
			LATAbits.LATA3 = !LATAbits.LATA3;
			err = OSMutexPost(CanMutex);
			break;
		case(intrusion):
			LATAbits.LATA2 = !LATAbits.LATA2;
			break;
/*		case(disarming):
			OSMboxPost(lcdBox, "intrusion");
			break;
		case(arming):
			OSMboxPost(lcdBox, "intrusion");
			break;
		case(alarmStarted):
			OSMboxPost(lcdBox, "intrusion");
			break;
		case(newPassword):
			OSMboxPost(lcdBox, "intrusion");
			break;
	}
}

void __attribute__((interrupt, no_auto_psv))_C1Interrupt(void)  
{    
	if (CAN_RX_BUFFER_IF){
		// HeartBeat
		if(CAN_RX_BUFFER_0){
			//
			actOnRecv(0);
			CAN_RX_BUFFER_0 = 0;
		}
		// intrusion
		if(CAN_RX_BUFFER_1){
			actOnRecv(1);
			CAN_RX_BUFFER_1 = 0;
		}
		// disarming
		if(CAN_RX_BUFFER_2){
			//LATAbits.LATA0 = !LATAbits.LATA0;
			actOnRecv(2);
			CAN_RX_BUFFER_2 = 0;
		}
		// arming
		if(CAN_RX_BUFFER_3){
			//LATAbits.LATA0 = !LATAbits.LATA0;
			actOnRecv(3);
			CAN_RX_BUFFER_3 = 0;
		}
		// alarm started
		if(CAN_RX_BUFFER_4){
			//LATAbits.LATA0 = !LATAbits.LATA0;
			actOnRecv(4);
			CAN_RX_BUFFER_4 = 0;
		}
		// new password
		if(CAN_RX_BUFFER_5){
			//LATAbits.LATA0 = !LATAbits.LATA0;
			actOnRecv(5);
			CAN_RX_BUFFER_5 = 0;
		}
		CAN_RX_BUFFER_IF = 0;
	}
	CAN_INTERRUPT_FLAG = 0;
}
*/