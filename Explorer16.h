/**********************************************************************
* © 2008 Microchip Technology Inc.
*
* FileName:        Explorer16.h
* Dependencies:    Header (.h) files if applicable, see below
* Processor:       dsPIC33Fxxxx
* Compiler:        MPLAB® C30 v3.00 or higher
*
* SOFTWARE LICENSE AGREEMENT:
* Microchip Technology Incorporated ("Microchip") retains all ownership and 
* intellectual property rights in the code accompanying this message and in all 
* derivatives hereto.  You may use this code, and any derivatives created by 
* any person or entity by or on your behalf, exclusively with Microchip's
* proprietary products.  Your acceptance and/or use of this code constitutes 
* agreement to the terms and conditions of this notice.
*
* CODE ACCOMPANYING THIS MESSAGE IS SUPPLIED BY MICROCHIP "AS IS".  NO 
* WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED 
* TO, IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A 
* PARTICULAR PURPOSE APPLY TO THIS CODE, ITS INTERACTION WITH MICROCHIP'S 
* PRODUCTS, COMBINATION WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 
*
* YOU ACKNOWLEDGE AND AGREE THAT, IN NO EVENT, SHALL MICROCHIP BE LIABLE, WHETHER 
* IN CONTRACT, WARRANTY, TORT (INCLUDING NEGLIGENCE OR BREACH OF STATUTORY DUTY), 
* STRICT LIABILITY, INDEMNITY, CONTRIBUTION, OR OTHERWISE, FOR ANY INDIRECT, SPECIAL, 
* PUNITIVE, EXEMPLARY, INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, FOR COST OR EXPENSE OF 
* ANY KIND WHATSOEVER RELATED TO THE CODE, HOWSOEVER CAUSED, EVEN IF MICROCHIP HAS BEEN 
* ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT 
* ALLOWABLE BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO 
* THIS CODE, SHALL NOT EXCEED THE PRICE YOU PAID DIRECTLY TO MICROCHIP SPECIFICALLY TO 
* HAVE THIS CODE DEVELOPED.
*
* You agree that you are solely responsible for testing the code and 
* determining its suitability.  Microchip has no obligation to modify, test, 
* certify, or support the code.
***************************************************************************/


#ifndef __EXPLORER16_BOARD_H__
#define __EXPLORER16_BOARD_H__
#include <p33Fxxxx.h>

#define SWITCH_DEBOUNCE	12

/* The LEDS and their ports	*/
#define LED3_TRIS		_TRISA0
#define LED4_TRIS		_TRISA1
#define LED5_TRIS		_TRISA2
#define LED6_TRIS		_TRISA3
#define LED7_TRIS		_TRISA4
#define LED8_TRIS		_TRISA5
#define LED9_TRIS		_TRISA6
#define LED10_TRIS	_TRISA7

#define LED3		_LATA0
#define LED4		_LATA1
#define LED5		_LATA2
#define LED6		_LATA3
#define LED7		_LATA4
#define LED8		_LATA5
#define LED9		_LATA6
#define LED10		_LATA7

/* The Switches and their ports */

#define SWITCH_S3_TRIS	_TRISD6
#define SWITCH_S4_TRIS	_TRISD13
#define SWITCH_S5_TRIS	_TRISA7
#define SWITCH_S6_TRIS	_TRISD7

#define SWITCH_S3	_RD6
#define SWITCH_S4	_RD13
#define SWITCH_S5	_RA7
#define SWITCH_S6	_RD7

#define EXPLORER16_LED_ON 1
#define EXPLORER16_LED_OFF 0

int CheckSwitchS3(void);
int CheckSwitchS6(void);
void Explorer16Init(void);

#endif
