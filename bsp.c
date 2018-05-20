/*
*********************************************************************************************************
*                                             Microchip dsPIC33FJ
*                                            Board Support Package
*
*                                                   Micrium
*                                    (c) Copyright 2005, Micrium, Weston, FL
*                                              All Rights Reserved
*
*
* File : BSP.C
* By   : Eric Shufro
*********************************************************************************************************
*/

#include <includes.h>

/*
*********************************************************************************************************
*                                       MPLAB CONFIGURATION MACROS
*********************************************************************************************************
*/

    __CONFIG(FWDT, WDTDIS);                                             /* Disable the watchdog timer                               */
    __CONFIG(FOSCSEL, OSCPLL);                                          /* Enable the primary XT / HS oscillator with PLL           */

/*
*********************************************************************************************************
*                                              CONSTANTS
*********************************************************************************************************
*/

#define  LED6   0x40                                                    /* Port A pin 6                                             */
#define  LED7   0x80                                                    /* Port A pin 7                                             */

/*
*********************************************************************************************************
*                                              VARIABLES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                              PROTOTYPES
*********************************************************************************************************
*/

static  void  BSP_PLL_Init(void);
static  void  Tmr_TickInit(void);

/*
*********************************************************************************************************
*                                         BSP INITIALIZATION
*
* Description : This function should be called by your application code before you make use of any of the
*               functions found in this module.
*
* Arguments   : none
*********************************************************************************************************
*/

void  BSP_Init (void)
{
    RCON    &= ~SWDTEN;                                                 /* Ensure Watchdog disabled via IDE CONFIG bits and SW.     */

    BSP_PLL_Init();                                                     /* Initialize the PLL                                       */
    LED_Init();                                                         /* Initialize the I/Os for the LED controls                 */
    Tmr_TickInit();                                                     /* Initialize the uC/OS-II tick interrupt                   */
}

/*
*********************************************************************************************************
*                                      BSP_PLL_Init()
*
* Description : This function configures and enables the PLL with the external oscillator
*               selected as the input clock to the PLL.
*
* Notes       : 1) The PLL output frequency is calculated by FIN * (M / (N1 * N2)).
*               2) FIN is the PLL input clock frequency, defined in bsp.h as
*                  CPU_PRIMARY_OSC_FR. This is the same as the external primary
*                  oscillator on the Explorer 16 Evaluation Board.
*               3) M is the desired PLL multiplier
*               4) N1 is the divider for FIN before FIN enters the PLL block (Pre-Divider)
*               5) N2 is the PLL output divider (Post-Divider)
*
* Summary     :    The PLL is configured as (8MHZ) * (40 / (2 * 2)) = 80MHZ
*                  The processor clock is (1/2) of the PLL output.
*                  Performance = 40 MIPS.
*********************************************************************************************************
*/

static  void  BSP_PLL_Init (void)
{
    PLLFBD   =     38;                                                  /* Set the Multiplier (M) to 40 (2 added automatically) 	*/
    CLKDIV   =      0;                                                  /* Clear the PLL Pre Divider bits, N1 = N2 = 2              */
}

/*
*********************************************************************************************************
*                                      BSP_CPU_ClkFrq()

* Description : This function determines the CPU clock frequency (Fcy)
* Returns     : The CPU frequency in (HZ)
*********************************************************************************************************
*/

CPU_INT32U  BSP_CPU_ClkFrq (void)
{
    CPU_INT08U  Clk_Selected;
    CPU_INT08U  PLL_n1;
    CPU_INT08U  PLL_n2;
    CPU_INT16U  PLL_m;
    CPU_INT16U  FRC_Div;
    CPU_INT32U  CPU_Clk_Frq;


    PLL_m         =  (PLLFBD & PLLDIV_MASK) + 2;                        /* Get the Multiplier value                                 */
    PLL_n1        =  (CLKDIV & PLLPRE_MASK) + 2;                        /* Computer the Pre Divider value                           */
    PLL_n2        = ((CLKDIV & PLLPOST_MASK) >> 6);                     /* Get the Post Divider register value                      */
    PLL_n2        = ((PLL_n2 * 2) + 2);                                 /* Compute the Post Divider value                           */

    FRC_Div       = ((CLKDIV & FRCDIV_MASK) >> 8);                      /* Get the FRC Oscillator Divider register value            */
    FRC_Div       = ((1 << FRC_Div) * 2);                               /* Compute the FRC Divider value                            */

    Clk_Selected  =  (OSCCON & COSC_MASK) >> 12;                        /* Determine which clock source is currently selected       */

    switch (Clk_Selected) {
        case 0:                                                         /* Fast Oscillator (FRC) Selected                           */
        case 7:
             CPU_Clk_Frq   =   CPU_FRC_OSC_FRQ;                         /* Return the frequency of the internal fast oscillator     */
             break;

        case 1:                                                         /* Fast Oscillator (FRC) with PLL Selected                  */
             CPU_Clk_Frq   = ((CPU_FRC_OSC_FRQ  * PLL_m) /              /* Compute the PLL output frequency using the FRC as FIN    */
                              (FRC_Div * PLL_n1 * PLL_n2));
             break;

        case 2:                                                         /* Primary External Oscillator Selected                     */
             CPU_Clk_Frq   =   CPU_PRIMARY_OSC_FRQ;                     /* Return the frequency of the primary external oscillator  */
             break;

        case 3:                                                         /* Primary External Oscillator with PLL Selected            */
             CPU_Clk_Frq   = ((CPU_PRIMARY_OSC_FRQ * PLL_m) /           /* Compute the PLL output frq using the PRI EXT OSC as FIN  */
                              (PLL_n1 * PLL_n2));
             break;

        case 4:                                                         /* Secondary External Low Power Oscillator (SOSC) Selected  */
        case 5:                                                         /* Internal Low Power RC Oscillator Selected                */
             CPU_Clk_Frq   =   CPU_SECONDARY_OSC_FRQ;                   /* Return the frq of the external secondary Low Power OSC   */
             break;

        case 6:
             CPU_Clk_Frq = 0;                                           /* Return 0 for the Reserved clock setting                  */
             break;

        default:
             CPU_Clk_Frq = 0;                                           /* Return 0 if the clock source cannot be determined        */
             break;
    }

    CPU_Clk_Frq   /=  2;                                                /* Divide the final frq by 2, get the actual CPU Frq (Fcy)  */

    return (CPU_Clk_Frq);                                               /* Return the operating frequency                           */
}

/*
*********************************************************************************************************
*                                     DISABLE ALL INTERRUPTS
*
* Description : This function disables all interrupts from the interrupt controller.
*
* Arguments   : none
*********************************************************************************************************
*/

void  BSP_IntDisAll (void)
{
}

/*
*********************************************************************************************************
*                                         LED I/O INITIALIZATION
*
* Description : This function initializes the I/O Pins used by the onboard LEDs
*
* Arguments   : none
*
* Notes       : 1) Jumper JP2 on the Explorer 16 board must be connected to enable the onboard LEDs
*               2) JTAG must be DISABLED in order to utilize all of PORTA I/O Lines for LEDs
*********************************************************************************************************
*/

void  LED_Init (void)
{
    TRISA     =  0;                                                     /* Set all PORTA pins to output                             */
    AD1PCFGH |= (LED6 | LED7);                                          /* Set PA6 & PA7 (R9, R10) to from analog to digital I/O    */

    LED_Off(0);                                                         /* Shut off all LEDs                                        */
}

/*
*********************************************************************************************************
*                                             LED ON
*
* Description : This function is used to control any or all the LEDs on the board.
*
* Arguments   : led    is the number of the LED to control
*                      0    indicates that you want ALL the LEDs to be ON
*                      1    turns ON LED1
*                      2    turns ON LED2
*                      ...
*                      8    turns ON LED8
*
* Notes       : 1) Jumper JP2 on the Explorer 16 board must be connected to enable the onboard LEDs
*********************************************************************************************************
*/

void  LED_On (CPU_INT08U led)
{
    if (led == 0) {
        PORTA |=  0xFF;                                                 /* Turn on all of the LEDs if a 0 is passed in              */
		return;
    }

    if ((led >= 1) && (led <= 8)) {
		led--;                                                          /* Convert the LED number to a pin number by subtracting 1	*/
        PORTA |= (1 << led);                                            /* Turn on the chosen LED                                   */
    }
}

/*
*********************************************************************************************************
*                                             LED OFF
*
* Description : This function is used to control any or all the LEDs on the board.
*
* Arguments   : led    is the number of the LED to turn OFF
*                      0    indicates that you want ALL the LEDs to be OFF
*                      1    turns OFF LED1
*                      2    turns OFF LED2
*                      .
*                      8    turns OFF LED8
*
* Notes       : 1) Jumper JP2 on the Explorer 16 board must be connected to enable the onboard LEDs
*********************************************************************************************************
*/

void  LED_Off (CPU_INT08U led)
{
    if (led == 0) {
        PORTA &= ~0xFF;                                                 /* Turn on all of the LEDs if a 0 is passed in              */
		return;
    }

    if ((led >= 1) && (led <= 8)) {
		led--;                                                          /* Convert the LED number to a pin number by subtracting 1	*/
        PORTA &= ~(1 << led);                                           /* Turn on the chosen LED                                   */
    }
}

/*
*********************************************************************************************************
*                                             LED TOGGLE
*
* Description : This function is used to toggle any or all the LEDs on the board.
*
* Arguments   : led    is the number of the LED to control
*                      0    indicates that you want to toggle ALL the LEDs
*                      1    toggles LED1
*                      2    toggles LED2
*                      .
*                      8    toggles LED8
*
* Notes       : 1) Jumper JP2 on the Explorer 16 board must be connected to enable the onboard LEDs
*********************************************************************************************************
*/

void  LED_Toggle (CPU_INT08U led)
{
    if (led == 0) {
        PORTA ^=  0xFF;                                                 /* Turn on all of the LEDs if a 0 is passed in              */
		return;
    }

    if ((led >= 1) && (led <= 8)) {
		led--;                                                          /* Convert the LED number to a pin number by subtracting 1	*/
        PORTA ^= (1 << led);                                            /* Turn on the chosen LED                                   */
    }
}

/*
*********************************************************************************************************
*                                   OSProbe_TmrInit()
*
* Description : This function is called to by uC/Probe Plug-In for uC/OS-II to initialize the
*               free running timer that is used to make time measurements.
*
* Arguments   : none
*
* Returns     : none
*
* Note(s)     : 1) This timer is shared with the uC/OS-II time tick and is initialized
*                  from Tmr_TickInit().
*********************************************************************************************************
*/

#if (uC_PROBE_OS_PLUGIN > 0) && (OS_PROBE_HOOKS_EN == 1)
void  OSProbe_TmrInit (void)
{
#if OS_PROBE_TIMER_SEL == 3
    T3CON  =   0;                                                       /* Use Internal Osc (Fosc / 4), 16 bit mode, prescaler = 1  */
    TMR3   =   0;                                                       /* Start counting from 0 and clear the prescaler count      */
    PR3    =   0xFFFF;                                                  /* Set the period register to its maximum value             */
    T3CON |=   TON;                                                     /* Start the timer                                          */
#endif

#if OS_PROBE_TIMER_SEL == 5
    T5CON  =   0;                                                       /* Use Internal Osc (Fosc / 4), 16 bit mode, prescaler = 1  */
    TMR5   =   0;                                                       /* Start counting from 0 and clear the prescaler count      */
    PR5    =   0xFFFF;                                                  /* Set the period register to its maximum value             */
    T5CON |=   TON;                                                     /* Start the timer                                          */
#endif
}
#endif

/*
*********************************************************************************************************
*                                   OSProbe_TmrRd()
*
* Description : This function is called to read the current counts of a 16 bit free running timer.
*
* Arguments   : none
*
* Returns     ; The 16 bit count (in a 32 bit variable) of the timer assuming the timer is an UP counter.
*********************************************************************************************************
*/

#if (uC_PROBE_OS_PLUGIN > 0) && (OS_PROBE_HOOKS_EN == 1)
CPU_INT32U  OSProbe_TmrRd (void)
{
#if OS_PROBE_TIMER_SEL == 3
    return ((CPU_INT32U)TMR3);                                          /* Return the value of timer 3 if selected                  */
#endif

#if OS_PROBE_TIMER_SEL == 5
    return ((CPU_INT32U)TMR5);                                          /* Return the value of timer 5 if selected                  */
#endif
}
#endif

/*
*********************************************************************************************************
*                                       TICKER INITIALIZATION
*
* Description : This function is called to initialize uC/OS-II's tick source (typically a timer generating
*               interrupts every 1 to 100 mS).
*
* Arguments   : none
*
* Note(s)     : 1) The timer operates at a frequency of Fosc / 4
*               2) The timer resets to 0 after period register match interrupt is generated
*********************************************************************************************************
*/

static  void  Tmr_TickInit (void)
{
    CPU_INT32U  tmr_frq;
    CPU_INT16U  cnts;


    tmr_frq   =   BSP_CPU_ClkFrq();                                     /* Get the CPU Clock Frequency (Hz) (Fcy)                   */
    cnts      =   (tmr_frq / OS_TICKS_PER_SEC) - 1;                     /* Calaculate the number of timer ticks between interrupts  */

#if BSP_OS_TMR_SEL == 2
    T2CON     =   0;                                                    /* Use Internal Osc (Fcy), 16 bit mode, prescaler = 1  		*/
    TMR2      =   0;                                                    /* Start counting from 0 and clear the prescaler count      */
    PR2       =   cnts;                                                 /* Set the period register                                  */
    IPC1     &=  ~T2IP_MASK;                                            /* Clear all timer 2 interrupt priority bits                */
    IPC1     |=  (TIMER_INT_PRIO << 12);                                /* Set timer 2 to operate with an interrupt priority of 4   */
    IFS0     &=  ~T2IF;                                                 /* Clear the interrupt for timer 2                          */
    IEC0     |=   T2IE;                                                 /* Enable interrupts for timer 2                            */
    T2CON    |=   TON;                                                  /* Start the timer                                          */
#endif

#if BSP_OS_TMR_SEL == 4
    T4CON     =   0;                                                    /* Use Internal Osc (Fcy), 16 bit mode, prescaler = 1  		*/
    TMR4      =   0;                                                    /* Start counting from 0 and clear the prescaler count      */
    PR4       =   cnts;                                                 /* Set the period register                                  */
    IPC6     &=  ~T4IP_MASK;                                            /* Clear all timer 4 interrupt priority bits                */
    IPC6     |=  (TIMER_INT_PRIO << 12);                                /* Set timer 4 to operate with an interrupt priority of 4   */
    IFS1     &=  ~T4IF;                                                 /* Clear the interrupt for timer 4                          */
    IEC1     |=   T4IE;                                                 /* Enable interrupts for timer 4                            */
    T4CON    |=   TON;                                                  /* Start the timer                                          */
#endif
}

/*
*********************************************************************************************************
*                                     OS TICK INTERRUPT SERVICE ROUTINE
*
* Description : This function handles the timer interrupt that is used to generate TICKs for uC/OS-II.
*********************************************************************************************************
*/

void OS_Tick_ISR_Handler (void)
{
#if  BSP_OS_TMR_SEL == 2
    IFS0 &= ~T2IF;
#endif

#if  BSP_OS_TMR_SEL == 4
    IFS1 &= ~T41F;
#endif

    OSTimeTick();
}

