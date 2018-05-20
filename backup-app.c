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
// Inpits have beeen prioritised over the out puts and the background tasks are left in the middle.
#define  APP_TASK_START_PRIO                    2                       /* Lower numbers are of higher priority                     */
#define  Timer_Task_PRIO						13
#define  Keyboard_Task_PRIO						11						// should the breaking be the highest prio ???????????
#define  Password_Management_Task_PRIO			14
#define  Button_handler_Task_PRIO				12
#define  APP_TASK_LCD_PRIO                      16
#define  HeartBeat_TASK_PRIO                    15

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
#define  HeartBeat_Task_PERIOD					5000

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
OS_STK  HeartBeatTaskStk[HeartBeat_Task_STK_SIZE];

// Programmer defined variables
unsigned char flagSystemUnlocked = 0;
unsigned char flagTimerActivated = 0;
unsigned char flagPasswordChange = 0;
char systemProvidedCode[4] = {'B', '1', '6', '9'};

#define PWDSIZE 4
#define STARCHAR 42
#define NODE_ID 5

char pmsg[PWDSIZE+1] = "    "; // the '+1' is due to the eos character
OS_EVENT* myBox;

char lcdpmsg[PWDSIZE+1] = "    "; // the '+1' is due to the eos character
OS_EVENT* lcdBox;

//////////////////////////////////////////////////////////////////////////////
//							FUNCTION PROTOTYPES								//
//////////////////////////////////////////////////////////////////////////////
static  void  AppStartTask(void *p_arg);
static  void  PasswordManagementTask(void *p_arg);
static  void  TimerTask(void *p_arg);
static  void  KeyboardTask(void *p_arg);
static  void  ButtonHandlerTask(void *p_arg);
static  void  AppLCDTask(void *p_arg);
static  void  HeartBeatTask(void *p_arg);

typedef enum MessageTypes {
    heartbeat = 0,
    intrusion = 1,
    disarming = 2,
    arming = 4,
    alarmStarted = 8,
    newPassword = 24,
	test = 0x180
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

	//myBox = OSMboxCreate((void*)0);
	//lcdBox = OSMboxCreate((void*)0);
	TRISDbits.TRISD12 = 1;	// set pin to input. INTRUSION
	TRISDbits.TRISD13 = 1;	// set pin to input. CHANGING PASSWORD
	TRISAbits.TRISA0 = 0;	// set pin to output. BUZZER
	TRISAbits.TRISA1 = 0;	// set pin to output. SYSTEM STATUS (UN)LOCK
	TRISAbits.TRISA2 = 0;	// set pin to output. TIMER STATUS (DES)ACTIVATED
	TRISAbits.TRISA6 = 0;	// set pin to output.
	TRISAbits.TRISA7 = 0;	// set pin to output.
	LATAbits.LATA7 = 1;

	CanInitialisation(CAN_OP_MODE_NORMAL, CAN_BAUDRATE_500k);
	CanLoadFilter(0, 0x099);
	CanLoadMask(0, 0x7FF);
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

	OSTaskCreateExt(HeartBeatTask,
					(void *)0,
					(OS_STK *)&HeartBeatTaskStk[0],
					HeartBeat_TASK_PRIO,
					HeartBeat_TASK_PRIO,
					(OS_STK *)&HeartBeatTaskStk[APP_TASK_STK_SIZE-1],
					HeartBeat_Task_STK_SIZE,
					(void *)0,
					OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
	// defines the App Name (for debug purpose)
	OSTaskNameSet(HeartBeat_TASK_PRIO, (CPU_INT08U *)"HB Task", &err);

	/*OSTaskCreateExt(AppLCDTask,
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
	*/

	while(1) {
		OSTaskSuspend(OS_PRIO_SELF);
		OSTimeDly(500);	// waits 500ms
   	}
}

static void HeartBeatTask(void *p_arg) {
	(void)p_arg;
	//unsigned char toggle = 0;
	LATAbits.LATA6 = 1;
	while(1) {
		//toggle = !toggle;
		LATAbits.LATA7 = !LATAbits.LATA7; //toggle;
		//unsigned char tmp[1] = {NODE_ID};
		send(test, 4, "test");
		OSTimeDly(HeartBeat_Task_PERIOD);
	}
}

/*static  void  AppLCDTask (void *p_arg) {
	INT8U err;
	INT8U* key;

   (void)p_arg;			// to avoid a warning message

	DispInit(2, 16);	// Initialize uC/LCD for a 2 row by 16 column display
	DispClrScr();		// Clear the screen
	INT8U row = 0;			// initialise the cursor position
	INT8U line = 0;

	while(1) {
		TASK_ENABLE5 = 1;
		LATAbits.LATA6 = 1;
		key = OSMboxPend(lcdBox, 0, &err);
		DispClrScr();
		DispStr(line, row, key);
		TASK_ENABLE5 = 0;
		LATAbits.LATA6 = 0;
	}
}*/
