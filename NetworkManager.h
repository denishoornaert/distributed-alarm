#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

typedef enum MessageTypes {
    heartbeat = 0,
    intrusion = 1,
    disarming = 2
    arming = 4,
    alarmStarted = 8,
    newPassword = 24
};

void initSender();

void initReceiver();

void send(MessageTypes, unsigned char, unsigned char*);

void recv();

#endif /* NETWORKMANAGER_H */
