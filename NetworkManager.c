#include "NetworkManager.h"

void initSender() {
	CanInitialisation(CAN_OP_MODE_NORMAL, CAN_BAUDRATE_500k);
	CanLoadFilter(0, 0x099);
	CanLoadMask(0, 0x7FF);
	CanAssociateMaskFilter(0, 0);
	ACTIVATE_CAN_INTERRUPTS = 1;
}

void initReceiver() {} // TODO useful ??

void send(MessageTypes messageid, unsigned char size, unsigned char* message) {
    transmitBuffer.SID = messageid;
	transmitBuffer.DLC = size;
	unsigned char i;
	for(i=0; i<size & i<8; i++) {
		transmitBuffer.DATA[i] = message[i];
	}
	CanSendMessage();
}

void recv() {
    switch(C1RXF0SIDbits.SID) {
        case(heartbeat):
            // fill
            break;
        case(intrusion):
            // fill
            break;
        case(disarming):
            // fill
            break;
        case(arming):
            // fill
            break;
        case(alarmStarted):
            // fill
            break;
        case(newPassword):
            // fill
            break;
        default:
            // do nothing on purpose
        break;
    }
}
