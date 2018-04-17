#include "NetworkManager.h"

void initSender() {}

void initReceiver() {}

void send(MessageTypes messageid, unsigned char size, unsigned char* message) {
    C1TR01CONbits.TXEN2 = 0x1;
    C1TR01CONbits.TX2PRI = 0x3;

    ecan1MsgBuf[0][0] = messageid << 2;

    ecan1MsgBuf[0][1] = 0;

    ecan1MsgBuf[0][2] = size;

    unsigned char transmitted = 0;
    while(transmitted < 8 & transmitted < size) {
        if(transmitted%2) {
            // set MSB
            ecan1MsgBuf[0][transmitted/2] = message[transmitted] << 8;
        }
        else {
            // set LSB
            ecan1MsgBuf[0][transmitted/2] |= message[transmitted]; // << 0
        }
        transmitted++;
    }

    C1TR01CONbits.TXREQ0 = 0x1;
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
