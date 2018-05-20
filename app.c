//////////////////////////////////////////////////////////////////////////////
//																			//
//					uC/OSII example for ELEC-H-410 labs						//
//																			//
//	This application creates 3 tasks :										//
//		- AppStartTask :													//
//			it creates the other tasks and flashes LED1 at 1Hz				//
//		- AppKeyboardTask :													//
//			it scans the keyboard ; when a key is pressed, the task writes	//
//			the key value on LED8-5	and sends the corresponding ASCII		//
//			character to AppLCDTask (using a mailbox)						//
//		- AppLCDTask : 														//
//			it displays the characters send by AppKeyboardTask on the LCD	//
//																			//
//	As AppStartTask and AppKeyboardTask shares the LEDs, we must use a		//
//	semaphore.																//
//																			//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
//									INCLUDES								//
//////////////////////////////////////////////////////////////////////////////
#include <includes.h>	// uC/OSII includes
#include "Keyboard.h"	// Keyboard functions library
#include "elec-h-410.h"
#include "CanDspic.h"
#include <string.h> // useful ??

/*
*********************************************************************************************************
*                                       TASK PRIORITIES
*********************************************************************************************************
*/
// Inputs have beeen prioritised over the outputs and the background tasks are left in the middle.
#define  APP_TASK_START_PRIO                    2                       /* Lower numbers are of higher priority                     */
#define  Mutex_PRIO								8						//Priority of the mutex (same for all)
#define  Timer_Task_PRIO						13	//WE DON'T USE IT ANYMORE????????
#define  Keyboard_Task_PRIO						11						//Priority for the keyboard task	// should the breaking be the highest prio ???????????
#define  Password_Management_Task_PRIO			14						//Priority for the password manager task
#define  Button_handler_Task_PRIO				12						//Priority for the INTRUSION task
#define  APP_TASK_LCD_PRIO                      16						//Priority for the LCD MANAGER task (Lowest)
#define  HeartBeat_TASK_PRIO                    15	//WE DON'T USE IT ANYMORE??????

/*
*********************************************************************************************************
*                                       TASK PERIODS
*********************************************************************************************************
*/
#define	 APP_TASK_1_PERIOD						10
#define	 Timer_Task_PERIOD						100 // useless ??
#define	 Keyboard_Task_PERIOD					100
#define  Password_Management_Task_PERIOD		100
#define  Button_handler_Task_PERIOD				100
#define  HeartBeat_Task_PERIOD					5000 // useless ??

/*
*********************************************************************************************************
*                                       TASK STACK SIZES
*
* Notes :   1) Warning, setting a stack size too small may result in the OS crashing. It the OS crashes
*              within a deep nested function call, the stack size may be to blame. The current maximum
*              stack usage for each task may be checked by using uC/OS-View or the stack checking
*              features of uC/OS-II.
*********************************************************************************************************
*/
#define  APP_TASK_START_STK_SIZE						128
#define  APP_TASK_STK_SIZE								128
#define  APP_TASK_LCD_STK_SIZE							256
#define  HeartBeat_Task_STK_SIZE						128


//////////////////////////////////////////////////////////////////////////////
//									CONSTANTES								//
//////////////////////////////////////////////////////////////////////////////
const INT8U hex2ASCII[16] = {	'0', '1', '2', '3', '4', '5', '6', '7',
								'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

//////////////////////////////////////////////////////////////////////////////
//									VARIABLES								//
//////////////////////////////////////////////////////////////////////////////
// Tasks stack
OS_STK  AppTaskStartStk[APP_TASK_START_STK_SIZE];
OS_STK  AppTask1Stk[APP_TASK_STK_SIZE];
OS_STK  TimerTaskStk[APP_TASK_STK_SIZE];
OS_STK  KeyboardTaskStk[APP_TASK_STK_SIZE];
OS_STK  PasswordManagementTaskStk[APP_TASK_STK_SIZE];
OS_STK  ButtonHandlerTaskStk[APP_TASK_STK_SIZE];
OS_STK  AppLCDTaskStk[APP_TASK_LCD_STK_SIZE];

#define PWDSIZE 4
#define STARCHAR 42
#define NODE_ID 10		// Starting condition only
#define OFFSET 0x200	// Offset to identify our node in the CAN
#define NUMBER_NODES 10 // Upper bound (maximum 10 nodes 0-9)

// Programmer defined variables
unsigned char nodeId[1] = {NODE_ID}; //Read only variable
unsigned char flagSystemUnlocked = 0;
unsigned char flagTimerActivated = 0;
unsigned char flagPasswordChange = 0;
char nodeIdentity[PWDSIZE+1] = {NODE_ID, 'B', '1', '6', '9'};
char* systemProvidedCode = &nodeIdentity[1];

char pmsg[PWDSIZE+1] = "    "; // the '+1' is due to the eos character
OS_EVENT* myBox;

char lcdpmsg[PWDSIZE+1] = "    "; // the '+1' is due to the eos character
OS_EVENT* lcdBox;

OS_EVENT *heartBeatMutex;
OS_EVENT *alarmStartedMutex;
OS_EVENT *flagPasswordChangeMutex;
OS_EVENT *flagSystemUnlockedMutex;
OS_EVENT *flagTimerActivatedMutex;
OS_EVENT *systemProvidedCodeMutex;

OS_TMR* heartBeatTimer;
OS_TMR* timerTimer;
OS_TMR* HBCheckerTimer;

unsigned char IDcounter = 0; //Read only
unsigned char HBflags[10]   = {0, 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0};
unsigned char HBCounter[10] = {0, 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0};

//////////////////////////////////////////////////////////////////////////////
//							FUNCTION PROTOTYPES								//
//////////////////////////////////////////////////////////////////////////////
static  void  AppStartTask(void *p_arg);
static  void  PasswordManagementTask(void *p_arg);
static  void  KeyboardTask(void *p_arg);
static  void  ButtonHandlerTask(void *p_arg);
static  void  AppLCDTask(void *p_arg);
static  void  HeartBeatTask(void *p_arg);
static  void  TimerFunc(void *p_arg); 
static  void  HeartBeatFunc(void *p_arg);
static  void  CheckerTimerFunc(void *p_arg);


typedef enum MessageTypes {
    heartbeat = OFFSET+0,
    intrusion = OFFSET+1,
    disarming = OFFSET+2,
    arming = OFFSET+4,
    alarmStarted = OFFSET+8,
    newPassword = OFFSET+24,
	test = 0x180 //USEFULL?????????????
} MessageTypes;

void send(MessageTypes messageid, unsigned char size, unsigned char* message) {
    transmitBuffer.SID = messageid;
	transmitBuffer.DLC = size;
	unsigned char i;
	for(i=0; i<size & i<8; i++) {
		transmitBuffer.DATA[i] = message[i];
	}
	CanSendMessage();
}

//////////////////////////////////////////////////////////////////////////////
//							MAIN FUNCTION									//
//////////////////////////////////////////////////////////////////////////////
CPU_INT16S  main (void) {
	CPU_INT08U  err;

	BSP_IntDisAll();	// Disable all interrupts until we are ready to accept them

	OSInit();			// Initialize "uC/OS-II, The Real-Time Kernel"

	init_elec_h_410();

	myBox = OSMboxCreate((void*)0);
	lcdBox = OSMboxCreate((void*)0);

	heartBeatMutex    = OSMutexCreate(8, &err);
	alarmStartedMutex = OSMutexCreate(9, &err);
	flagPasswordChangeMutex = OSMutexCreate(7, &err);
	flagSystemUnlockedMutex = OSMutexCreate(6, &err);
	flagTimerActivatedMutex = OSMutexCreate(5, &err);
	systemProvidedCodeMutex = OSMutexCreate(4, &err);

	TRISDbits.TRISD12 = 1;	// set pin to input. INTRUSION
	TRISDbits.TRISD13 = 1;	// set pin to input. CHANGING PASSWORD
	TRISAbits.TRISA0 = 0;	// set pin to output. BUZZER
	TRISAbits.TRISA1 = 0;	// set pin to output. SYSTEM STATUS (UN)LOCK
	TRISAbits.TRISA2 = 0;	// set pin to output. TIMER STATUS (DES)ACTIVATED
	TRISAbits.TRISA3 = 0;	// set pin to output.
	TRISAbits.TRISA5 = 0;	// set pin to output.
	TRISAbits.TRISA6 = 0;	// set pin to output.
	TRISAbits.TRISA7 = 0;	// set pin to output.
	LATAbits.LATA7 = 1;

	CanInitialisation(CAN_OP_MODE_NORMAL, CAN_BAUDRATE_500k);

	// enumeration of all the filters that will be accepted by the network.
	CanLoadFilter(0, heartbeat);
	CanLoadFilter(1, intrusion);
	CanLoadFilter(2, disarming);
	CanLoadFilter(3, arming);
	CanLoadFilter(4, alarmStarted);
	CanLoadFilter(5, newPassword);

	CanAssociateMaskFilter(0, 0);
	ACTIVATE_CAN_INTERRUPTS = 1;

	OSTaskCreateExt(
			AppStartTask,		// creates AppStartTask
			(void *)0,
			(OS_STK *)&AppTaskStartStk[0],
			APP_TASK_START_PRIO,
			APP_TASK_START_PRIO,
			(OS_STK *)&AppTaskStartStk[APP_TASK_START_STK_SIZE-1],
			APP_TASK_START_STK_SIZE,
			(void *)0,
			OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);

	// defines the App Name (for debug purpose)
    OSTaskNameSet(APP_TASK_START_PRIO, (CPU_INT08U *)"Start Task", &err);

	OSStart();		// Start multitasking (i.e. give control to uC/OS-II)

	return (-1);	// Return an error - This line of code is unreachable
}



//////////////////////////////////////////////////////////////////////////////
//								AppStartTask								//
// Arguments :	p_arg passed to 'AppStartTask()' by 'OSTaskCreate()'.		//
//				Not used in this task										//
//	Notes :																	//
//		1)	The first line of code is used to prevent a compiler warning	//
//			because 'p_arg' is not used.									//
//		2)	Interrupts are enabled once the task start because the I-bit	//
//			of the CCR register was set to 0 by 'OSTaskCreate()'.			//
//////////////////////////////////////////////////////////////////////////////
static  void  AppStartTask (void *p_arg) {
	INT8U	err;
	INT16U	i,j;


   (void)p_arg;	// to avoid a warning message

    BSP_Init();		// Initialize BSP (Board Support Package) functions

	#if OS_TASK_STAT_EN > 0
    	OSStatInit();	// Determine CPU capacity
	#endif

    LED_Off(0);		// Turn OFF all the LEDs

	OSTaskCreateExt(PasswordManagementTask,
					(void *)0,
					(OS_STK *)&PasswordManagementTaskStk[0],
					Password_Management_Task_PRIO,
					Password_Management_Task_PRIO,
					(OS_STK *)&PasswordManagementTaskStk[APP_TASK_STK_SIZE-1],
					APP_TASK_STK_SIZE,
					(void *)0,
					OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
	// defines the App Name (for debug purpose)
    OSTaskNameSet(Password_Management_Task_PRIO, (CPU_INT08U *)"Password Management Task", &err);


	OSTaskCreateExt(KeyboardTask,
					(void *)0,
					(OS_STK *)&KeyboardTaskStk[0],
					Keyboard_Task_PRIO,
					Keyboard_Task_PRIO,
					(OS_STK *)&KeyboardTaskStk[APP_TASK_STK_SIZE-1],
					APP_TASK_STK_SIZE,
					(void *)0,
					OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
	// defines the App Name (for debug purpose)
	OSTaskNameSet(Keyboard_Task_PRIO, (CPU_INT08U *)"Keyboard Task", &err);


	OSTaskCreateExt(ButtonHandlerTask,
					(void *)0,
					(OS_STK *)&ButtonHandlerTaskStk[0],
					Button_handler_Task_PRIO,
					Button_handler_Task_PRIO,
					(OS_STK *)&ButtonHandlerTaskStk[APP_TASK_STK_SIZE-1],
					APP_TASK_STK_SIZE,
					(void *)0,
					OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
	// defines the App Name (for debug purpose)
	OSTaskNameSet(Button_handler_Task_PRIO, (CPU_INT08U *)"Button handler Task", &err);


	heartBeatTimer = OSTmrCreate(0, 5000, OS_TMR_OPT_PERIODIC, HeartBeatFunc, (void*)0, "Heart beat timer", &err);
	OSTmrStart(heartBeatTimer, &err);

	timerTimer = OSTmrCreate(0, 30000, OS_TMR_OPT_ONE_SHOT, TimerFunc, (void*)0, "intrusion timer", &err);

	HBCheckerTimer = OSTmrCreate(0, 100, OS_TMR_OPT_PERIODIC, CheckerTimerFunc, (void*)0, "Heart beat check", &err); //0.1 seconds timer
	OSTmrStart(HBCheckerTimer, &err);
	

	OSTaskCreateExt(AppLCDTask,
					(void *)0,
					(OS_STK *)&AppLCDTaskStk[0],
					APP_TASK_LCD_PRIO,
					APP_TASK_LCD_PRIO,
					(OS_STK *)&AppLCDTaskStk[APP_TASK_LCD_STK_SIZE-1],
					APP_TASK_LCD_STK_SIZE,
					(void *)0,
					OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
	// defines the App Name (for debug purpose)
    OSTaskNameSet(APP_TASK_LCD_PRIO, (CPU_INT08U *)"LCD Task", &err);

	while(1) {
		OSTaskSuspend(OS_PRIO_SELF);
		OSTimeDly(500);	// waits 500ms
   	}
}

// Common functions

unsigned char strEqual(char* word1, char* word2) {
	return (word1[0] == word2[0] & word1[1] == word2[1] & word1[2] == word2[2] & word1[3] == word2[3]);
}

void stringCopy(char* stringDest, char* stringSrc) {
	unsigned char i;
	for(i=0; i<PWDSIZE; i++) {
		stringDest[i] = stringSrc[i];
	}
}

// ----------------------------------------------
// Critical section functions definition

void setTheAlarm(unsigned char mode) {
	// mode is either on=1, off=0
	INT8U err;
	OSMutexPend(alarmStartedMutex, 0, &err);
	LATAbits.LATA0 = mode;
	OSMutexPost(alarmStartedMutex);
}

void flagPasswordChangeSet(unsigned char newValue) {
	INT8U err;
	OSMutexPend(flagPasswordChangeMutex, 0, &err);
	flagPasswordChange = newValue;
	OSMutexPost(flagPasswordChangeMutex);
}

unsigned char flagPasswordChangeGet() {
	INT8U err;
	OSMutexPend(flagPasswordChangeMutex, 0, &err);
	unsigned char res = flagPasswordChange;
	OSMutexPost(flagPasswordChangeMutex);
	return res;
}

void flagSystemUnlockedSet(unsigned char newValue) {
	INT8U err;
	OSMutexPend(flagSystemUnlockedMutex, 0, &err);
	flagSystemUnlocked = newValue;
	OSMutexPost(flagSystemUnlockedMutex);
}

unsigned char flagSystemUnlockedGet() {
	INT8U err;
	OSMutexPend(flagSystemUnlockedMutex, 0, &err);
	unsigned char res = flagSystemUnlocked;
	OSMutexPost(flagSystemUnlockedMutex);
	return res;
}

void flagTimerActivatedSet(unsigned char newValue) {
	INT8U err;
	OSMutexPend(flagTimerActivatedMutex, 0, &err);
	flagTimerActivated = newValue;
	OSMutexPost(flagTimerActivatedMutex);
}

unsigned char flagTimerActivatedGet() {
	INT8U err;
	OSMutexPend(flagTimerActivatedMutex, 0, &err);
	unsigned char res = flagTimerActivated;
	OSMutexPost(flagTimerActivatedMutex);
	return res;
}

void systemProvidedCodeSet(unsigned char *newValue) {
	INT8U err;
	OSMutexPend(systemProvidedCodeMutex, 0, &err);
	stringCopy(systemProvidedCode, newValue);
	OSMutexPost(systemProvidedCodeMutex);
}

unsigned char* systemProvidedCodeGet() {
	INT8U err;
	OSMutexPend(systemProvidedCodeMutex, 0, &err);
	unsigned char* res = systemProvidedCode;
	OSMutexPost(systemProvidedCodeMutex);
	return res;
}

// -------------------------------------

void LockedSystemActOnCorrectPassword(unsigned char doSend) {
	INT8U err;
	int i;
    // Switch the system to the 'unlocked system' state
    flagSystemUnlockedSet(1);
	OSMboxPost(lcdBox, "Unlocked");
    // System unlocked
    LATAbits.LATA1 = 1;
    // Buzzer desactivated !
    setTheAlarm(0);
    // timer desactivated !
	OSTmrStop(timerTimer, OS_TMR_OPT_NONE, (void*)0, &err);
	if(doSend) {
		send(disarming, 1, nodeId);
	}
	LATAbits.LATA2 = 0;
	flagTimerActivatedSet(0);
	TASK_ENABLE2 = 0;
	OSMutexPend(heartBeatMutex, 0, &err);
	//Reset Heart Beat Counters
	for(i=0;i<NUMBER_NODES;i++){
		HBCounter[i]=0;	
	}
	OSMutexPost(heartBeatMutex);
}

void UnlockedSystemActOnCorrectPassword(unsigned char doSend) {
	INT8U err;
    // Switch the to the 'locked system' state
    flagSystemUnlockedSet(0);
	OSMboxPost(lcdBox, "Locked");
    // System locked
	if(doSend) {
		send(arming, 1, nodeId);
	}
    LATAbits.LATA1 = 0;
}

void checkPasswordValidity(char* userProvidedCode, INT8U* err) {
	userProvidedCode = OSMboxPend(myBox, 50, err); // TODO should be blocking ??
	if(*err == OS_ERR_NONE) {
       if(strEqual(userProvidedCode, systemProvidedCodeGet())) {
           if(!flagSystemUnlockedGet()) {
               LockedSystemActOnCorrectPassword(1);
           }
           else {
               UnlockedSystemActOnCorrectPassword(1);
           }
       }
		else {
			OSMboxPost(lcdBox, "Wrong pwd");
		}
	}
}

void changePasswordProcedure(char* userProvidedCode, char* userProvidedCodeConfirmation, INT8U* err) {

	char aux1[4];
	char aux2[4];
	OSMboxPost(lcdBox, "Enter old pwd");
	userProvidedCode = OSMboxPend(myBox, 0, err); // blocking instruction
	if(*err == OS_ERR_NONE) {
		if(strEqual(userProvidedCode, systemProvidedCodeGet())) {
			OSMboxPost(lcdBox, "Enter new pwd");
			userProvidedCode = OSMboxPend(myBox, 0, err); // blocking instruction
			stringCopy(aux1, userProvidedCode);
			OSMboxPost(lcdBox, "Confirm new pwd");
			userProvidedCodeConfirmation = OSMboxPend(myBox, 0, err); // blocking instruction
			stringCopy(aux2, userProvidedCodeConfirmation);
			if(strEqual(aux1, aux2)) {			
				// put the system password to userProvidedCode
				systemProvidedCodeSet(aux1);
				send(newPassword, 5, nodeIdentity);
				OSMboxPost(lcdBox, "New pwd set");
				flagPasswordChangeSet(0);
			}
			else {
				// password change fail
				OSMboxPost(lcdBox, "FAIL - unlock");
				flagPasswordChangeSet(0);
				// set the system in unlock mode
			}
		}
		else {
			OSMboxPost(lcdBox, "Wrong pwd");
			// set the system in unlock mode
		}
	}
}

static  void PasswordManagementTask (void *p_arg) {
	(void)p_arg;			// to avoid a warning message
	LATAbits.LATA5 = 1;
	INT8U err;
	char* userProvidedCode;
	char* userProvidedCodeConfirmation;
    while(1) {
		//TASK_ENABLE1 = 1;
		if(!flagPasswordChangeGet()) {
			checkPasswordValidity(userProvidedCode, &err);
		}
		else { 
			changePasswordProcedure(userProvidedCode, userProvidedCodeConfirmation, &err);
		}
		//TASK_ENABLE1 = 0;
		OSTimeDly(Password_Management_Task_PERIOD); // useful ?
    }
}

static void TimerFunc(void *p_arg) {
	// Look why the timer has stopped. In case of time out, the system unlock flag is set to False, thus we activate the buzzer.
	// Otherwise, we simply set the timer flag to false.
	if(!flagSystemUnlockedGet()) {
        // Buzzer activated !
        setTheAlarm(1);
		send(alarmStarted, 1, nodeId);
    }
	else{
		flagTimerActivatedSet(0);
	}
	LATAbits.LATA2 = 0;
}


static void ButtonHandlerTask(void *p_arg) {
	INT8U err;
	(void)p_arg;
	while(1) {
		TASK_ENABLE3 = 1;
		if(!PORTDbits.RD12 & !flagTimerActivatedGet() & !flagSystemUnlockedGet()) { // the button is ignored when the timer has already been activated
			LATAbits.LATA2 = 1;
			OSTmrStart(timerTimer, &err);
			flagTimerActivatedSet(1);
			send(intrusion, 1, nodeId);
		}
		if(!PORTDbits.RD13 & flagSystemUnlockedGet()) { // TODO check if timer flag is required
			flagPasswordChangeSet(1);
		}
		TASK_ENABLE3 = 0;
		OSTimeDly(Button_handler_Task_PERIOD);
	}
}

void resetPassword(char* code) {
	unsigned char i;
	for(i=0; i<PWDSIZE; i++) {
		code[i] = STARCHAR;
	}
}

static  void  KeyboardTask (void *p_arg) {
	(void)p_arg;			// to avoid a warning message
	KeyboardInit();

	INT8U key1 = 0;
	INT8U key2 = 0;
	INT8U key3;
	char code[PWDSIZE] = {STARCHAR, STARCHAR, STARCHAR, STARCHAR};
    unsigned char i;

    while(1) {
		TASK_ENABLE4 = 1;
		i = 0;
        while(i<PWDSIZE) {
			key3 = key2;
			key2 = key1;
			key1 = KeyboardScan();
			if ((key1 == key2) && (key1 != key3) && (key1 != 255)) {
				code[i] = hex2ASCII[key1];
				stringCopy(lcdpmsg, code);
				OSMboxPost(lcdBox, lcdpmsg);
				i++;
			}
			TASK_ENABLE4 = 0;
			OSTimeDly(Keyboard_Task_PERIOD);
			TASK_ENABLE4 = 1;
        }
		stringCopy(pmsg, code);
		stringCopy(lcdpmsg, code);
        OSMboxPost(myBox, pmsg);
		OSMboxPost(lcdBox, lcdpmsg);
		resetPassword(code);
		TASK_ENABLE4 = 0;
		OSTimeDly(Keyboard_Task_PERIOD);
	}
}

static void HeartBeatFunc(void *p_arg) {
	(void)p_arg;
	LATAbits.LATA7 = !LATAbits.LATA7; //toggle;
	send(heartbeat, 1, nodeId);
}


static  void  AppLCDTask (void *p_arg) {
	INT8U err;
	INT8U* key;

   (void)p_arg;			// to avoid a warning message

	DispInit(2, 16);	// Initialize uC/LCD for a 2 row by 16 column display
	DispClrScr();		// Clear the screen
	INT8U row = 0;			// initialise the cursor position
	INT8U line = 0;

	while(1) {
		TASK_ENABLE5 = 1;
		key = OSMboxPend(lcdBox, 0, &err);
		DispClrScr();
		DispStr(line, row, key);
		TASK_ENABLE5 = 0;
	}
}

static void CheckerTimerFunc(void *p_arg){
	(void)p_arg;
	int i;
	INT8U err;
	TASK_ENABLE1 = 1;
	//Increment all the counters except ours
	if(!flagSystemUnlockedGet()){
		OSMutexPend(heartBeatMutex, 0, &err);
		for(i = 0;i < NUMBER_NODES; i++){
			if(HBflags[i]) {
				if (HBCounter[i]>=50){
					// Buzzer activated !
		   			setTheAlarm(1);
					// TODO set flag to zero in order to free the flag
				}
				else{
					HBCounter[i]++;
				}
			}
		}
		HBCounter[nodeId[0]]=0;
		err = OSMutexPost(heartBeatMutex);
		// Determine the node id
		if(IDcounter = 50) {
			// TODO what to do when every counters are at 1 (no free flag)
			unsigned char i = 0;
			unsigned char found = 0; // used as a boolean
			while(i < NUMBER_NODES && !found) {
				if(!HBflags[i]) { // HBflags[i] == 0
					nodeId[0] = i;
					nodeIdentity[0] = i;
					found = 1;
					IDcounter = 0;
					HBflags[i] = 1;
				}
				i++;
			}
		}
		else {
			IDcounter++;
		}
	}
	TASK_ENABLE1 = 0;
}

void actOnRecv(unsigned char offset) {
	INT8U err;
	unsigned char i;
	switch(receiveBuffers[offset].SID) {
		case(heartbeat):
			//detect from which node 0-9 excluding ours
			OSMutexPend(heartBeatMutex, 0, &err);
			// TODO check the index upper bound (not bigger than 9)
			unsigned char index = receiveBuffers[offset].DATA[0];
			if(index < 10) {
				HBflags[index] = 1;		// 'Activate' a flag
				HBCounter[index] = 0;	// Reset Counter with ID
				LATAbits.LATA3 = !LATAbits.LATA3;
			}
			OSMutexPost(heartBeatMutex);
			break;
		case(intrusion):
			// No need to care about this message
			OSMboxPost(lcdBox, "Intrusion!");
			break;
		case(disarming):
			LockedSystemActOnCorrectPassword(0);
			break;
		case(arming):
			UnlockedSystemActOnCorrectPassword(0);
			break;
		case(alarmStarted):
			// Assumption : all nodes are locked
			setTheAlarm(1);
			break;
		case(newPassword):
			// THere will be a mutex so no worry !
			// With the mutex, we will exclude the other task that modifies 'systemProvidedCode'
			// Therefore race conditions will be avoided
			OSMboxPost(lcdBox, "New pwd set");
			systemProvidedCodeSet(&receiveBuffers[offset].DATA[1]);
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
			actOnRecv(2);
			CAN_RX_BUFFER_2 = 0;
		}
		// arming
		if(CAN_RX_BUFFER_3){
			actOnRecv(3);
			CAN_RX_BUFFER_3 = 0;
		}
		// alarm started
		if(CAN_RX_BUFFER_4){
			actOnRecv(4);
			CAN_RX_BUFFER_4 = 0;
		}
		// new password
		if(CAN_RX_BUFFER_5){
			actOnRecv(5);
			CAN_RX_BUFFER_5 = 0;
		}
		CAN_RX_BUFFER_IF = 0;
	}
	CAN_INTERRUPT_FLAG = 0;
}