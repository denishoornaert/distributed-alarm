/**********************************************************************
* © 2008 Microchip Technology Inc.
*
* FileName:        Explorer16.c
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
************************************************************************/

#include "..\h\Explorer16.h"

int debounceS3;						/* Debounce counter for switch S1	*/
int debounceS6;						/* Debounce counter for switch S2	*/


void Explorer16Init(void)
{
	/* Intialize the board LED and swicth ports	
	 * and turn all LEDS off 	*/
	 
	LED3_TRIS = 0;
	LED4_TRIS = 0;
	LED5_TRIS = 0;
	LED6_TRIS = 0;
	LED7_TRIS = 0;
	LED8_TRIS = 0;
	LED9_TRIS = 0;
	LED10_TRIS = 0;

	SWITCH_S3_TRIS = 1;
	SWITCH_S4_TRIS = 1;
	SWITCH_S5_TRIS = 1;
	SWITCH_S6_TRIS = 1;

	LED3 = EXPLORER16_LED_OFF; 	
	LED4 =  EXPLORER16_LED_OFF;	
	LED5 =  EXPLORER16_LED_OFF;	
	LED6 = EXPLORER16_LED_OFF;	
	LED7 =  EXPLORER16_LED_OFF;	
	LED8 = EXPLORER16_LED_OFF;	
	LED9 =  EXPLORER16_LED_OFF;	
	LED10 = EXPLORER16_LED_OFF;	

}

int CheckSwitchS3(void)
{
	/* This function returns a 1 if a valid key press was detected on SW1 */
	int isActive = 0;
	if(SWITCH_S3 == 0){
		
		debounceS3++;
		if (debounceS3 == SWITCH_DEBOUNCE){
			
			/* This means that the Switch S3 was pressed long enough
			 * for a valid key press	*/
			 
			debounceS3  = 0;
			isActive = 1;
		}
	}
	else	{
		debounceS3 = 0;
		isActive = 0;
	}
	
	return(isActive);
}	


int CheckSwitchS6(void)
{
	/* This function returns a 1 if a valid key press was detected on SW2 */
	int isActive = 0;
	if(SWITCH_S6 == 0){
		/* If SW2 press was valid then toggle the record function	*/
		debounceS6++;
		if (debounceS6 == SWITCH_DEBOUNCE){
			
			/* This means that the Switch S6 was pressed long enough
			 * for a valid key press	*/
			debounceS6  = 0;
			isActive = 1;
		}
	}
	else	{
		debounceS6 = 0;
		isActive = 0;
	}
	return(isActive);
}

