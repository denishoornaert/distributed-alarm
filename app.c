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
#include <string.h>

/*
*********************************************************************************************************
*                                       TASK PRIORITIES
*********************************************************************************************************
*/
#define  APP_TASK_START_PRIO                    0                       /* Lower numbers are of higher priority                     */
#define  Timer_Task_PRIO						3
#define  Keyboard_Task_PRIO						1
#define  Password_Management_Task_PRIO			4
#define  Button_handler_Task_PRIO				2
#define  APP_TASK_LCD_PRIO                      5

/*
*********************************************************************************************************
*                                       TASK PERIODS
*********************************************************************************************************
*/
#define	 APP_TASK_1_PERIOD						10
#define	 Timer_Task_PERIOD						100 // useless ??
#define	 Keyboard_Task_PERIOD					50  // set to 100
#define  Password_Management_Task_PERIOD		100
#define  Button_handler_Task_PERIOD				100

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

// Programmer defined variables
unsigned char flagSystemUnlocked = 0;
unsigned char flagTimerActivated = 0;
char systemProvidedCode[4] = {'B', '1', '6', '9'};

char pmsg[4];
OS_EVENT* myBox;

char lcdPmsg[4];
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
	TRISDbits.TRISD12 = 1;	// set pin to input
	TRISAbits.TRISA0 = 0;	// set pin to output.
	TRISAbits.TRISA1 = 0;	// set pin to output.
	TRISAbits.TRISA2 = 0;	// set pin to output.
	TRISAbits.TRISA7 = 0;	// set pin to output.

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

	OSTaskCreateExt(TimerTask,
					(void *)0,
					(OS_STK *)&TimerTaskStk[0],
					Timer_Task_PRIO,
					Timer_Task_PRIO,
					(OS_STK *)&TimerTaskStk[APP_TASK_STK_SIZE-1],
					APP_TASK_STK_SIZE,
					(void *)0,
					OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
	// defines the App Name (for debug purpose)
	OSTaskNameSet(Timer_Task_PRIO, (CPU_INT08U *)"Timer Task", &err);
	OSTaskSuspend(Timer_Task_PRIO);

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
		OSTaskSuspend(APP_TASK_START_PRIO);
		OSTimeDly(500);	// waits 500ms
   	}
}

void LockedSystemActOnCorrectPassword() {
    // Switch the to the 'unlocked system' state
    flagSystemUnlocked = 1;
	OSMboxPost(lcdBox, "Unlocked");
    // System unlocked
    LATAbits.LATA1 = 1;
    // Buzzer desactivated !
    LATAbits.LATA0 = 0;
    // timer desactivated !
    OSTaskSuspend(Timer_Task_PRIO);
	TASK_ENABLE2 = 0;
}

void UnlockedSystemActOnCorrectPassword() {
    // Switch the to the 'locked system' state
    flagSystemUnlocked = 0;
	OSMboxPost(lcdBox, "Locked");
    // System unlocked
    LATAbits.LATA1 = 0;
}

unsigned char strEqual(char* word1, char* word2) {
	return (word1[0] == word2[0] & word1[1] == word2[1] & word1[2] == word2[2] & word1[3] == word2[3]);
}

static  void PasswordManagementTask (void *p_arg) {
	(void)p_arg;			// to avoid a warning message

	INT8U err;
	char* userProvidedCode;
    while(1) {
		TASK_ENABLE1 = 1;
        userProvidedCode = OSMboxPend(myBox, 50, &err); // blocking instruction
		if(err == OS_ERR_NONE) {
	        if(strEqual(userProvidedCode, systemProvidedCode)) {
	            if(!flagSystemUnlocked) {
	                LockedSystemActOnCorrectPassword();
	            }
	            else {
	                UnlockedSystemActOnCorrectPassword();
	            }
	        }
			else {
				OSMboxPost(lcdBox, "Wrong pwd");
			}
		}
		TASK_ENABLE1 = 0;
		OSTimeDly(Password_Management_Task_PERIOD); // useful ?
    }
}

static  void  TimerTask (void *p_arg) {
	(void)p_arg;			// to avoid a warning message

	char unsigned counter = 60;
	TASK_ENABLE2 = 1;
    LATAbits.LATA2 = 1;
    while(counter > 0 & !flagSystemUnlocked) {
		TASK_ENABLE2 = 0;
        OSTimeDly(500);
		TASK_ENABLE2 = 1;
        counter--;
    }
    if(!flagSystemUnlocked) {
        // Buzzer activated !
        LATAbits.LATA0 = 1;
    }
    TASK_ENABLE2 = 0;
	OSTaskSuspend(OS_PRIO_SELF);
}

static void ButtonHandlerTask(void *p_arg) {
	(void)p_arg;
	while(1) {
		TASK_ENABLE3 = 1;
		if(!PORTDbits.RD12) {
			OSTaskResume(Timer_Task_PRIO);
		}
		TASK_ENABLE3 = 0;
		OSTimeDly(Button_handler_Task_PERIOD);
	}
}

INT8U getch() {
	INT8U key1 = 0;
	INT8U key2 = 0;
	INT8U key3;

	while(1) {
		key3 = key2;
		key2 = key1;
		key1 = KeyboardScan();
		if ((key1 == key2) && (key1 != key3) && (key1 != 255)) {
			return hex2ASCII[key1];
		}
		OSTimeDly(Keyboard_Task_PERIOD); // TODO to reduce or useful ?
	}
}

static  void  KeyboardTask (void *p_arg) {
	(void)p_arg;			// to avoid a warning message
	/*
	KeyboardInit();

	char code[2][4] = {"****", "****"};
    unsigned char i;
    while(1) {
		TASK_ENABLE4 = 1;
        for(i=0; i<4; i++) {
			code[0][i] = getch();
			code[1][i] = code[0][i];
			OSMboxPost(lcdBox, code[1]);
        }
        OSMboxPost(myBox, code[0]);
		OSMboxPost(lcdBox, code[1]);
		TASK_ENABLE4 = 0;
		OSTimeDly(Keyboard_Task_PERIOD);
    }*/
	KeyboardInit();

	INT8U key1 = 0;
	INT8U key2 = 0;
	INT8U key3;
	char code[2][4] = {"****", "****"};
    unsigned char i;

    while(1) {
		TASK_ENABLE4 = 1;
		i = 0;
        while(i<4) {
			key3 = key2;
			key2 = key1;
			key1 = KeyboardScan();
			if ((key1 == key2) && (key1 != key3) && (key1 != 255)) {
				code[0][i] = hex2ASCII[key1];
				code[1][i] = hex2ASCII[key1];
				OSMboxPost(lcdBox, code[1]);
				i++;
			}
			// OSTimeDly(Keyboard_Task_PERIOD);
        }
        OSMboxPost(myBox, code[0]);
		OSMboxPost(lcdBox, code[1]);
		TASK_ENABLE4 = 0;
		OSTimeDly(Keyboard_Task_PERIOD);
}
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