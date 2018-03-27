/**********************************************************************
* © 2008 Microchip Technology Inc.
*
* FileName:        main.c
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

#include <p33FJ256GP710.h>
#include "..\h\WM8510CodecDrv.h"
#include "Explorer16.h"
#include "speex.h"
#include "PgmMemory.h"

_FGS(GWRP_OFF & GCP_OFF);
_FOSCSEL(FNOSC_FRC);
_FOSC(FCKSM_CSECMD & OSCIOFNC_ON & POSCMD_XT);
_FWDT(FWDTEN_OFF);


/* Allocate state memory, scratch memory and buffers
 * for Speex decoder */
 
int				speexDecoderState	[WB_SPEEX_DECODER_STATE_SIZE];
unsigned char 	scratchY			[SPEEX_DECODER_Y_SCRATCH_SIZE] _YBSS(2);
unsigned char 	scratchX			[SPEEX_DECODER_X_SCRATCH_SIZE] _XBSS(2);
char 			encodedSamples		[WB_SPEEX_ENCODED_FRAME_SIZE_12K8];
int 			decodedSamples		[WB_SPEEX_DECODER_OUTPUT_SIZE];
int 			decoderRetVal;

int				buf[2][WB_SPEEX_DECODER_OUTPUT_SIZE];
unsigned int	a, not_a, b;

/* Descriptor and handle to the program memory */
ProgramMemory samplesInPGMemory;
ProgramMemory * samples;
unsigned int bytesRead;

/* This is the encoded segment in program memory
 * declared here as a function.  Note that the file was generated
 * using the Speex PC Encoding Utility */

extern void SpeechSegment(void);
#define SPEECH_SEGMENT_SIZE_BYTES (7744)

void ecrit_dac_signal(int sig);

int main(void)
{
	unsigned int i;

	/* Configure Oscillator to operate the device at 40MHz.
	 * Fosc= Fin*M/(N1*N2), Fcy=Fosc/2
	 * Fosc= 8.0M*40/(2*2)=80Mhz for 8.0M input clock */
	 
	PLLFBD=38;				/* M=40	*/
	CLKDIVbits.PLLPOST=0;		/* N1=2	*/
	CLKDIVbits.PLLPRE=0;		/* N2=2	*/
	OSCTUN=0;			
	
	__builtin_write_OSCCONH(0x03);		/*	Initiate Clock Switch to PRI with PLL*/
	__builtin_write_OSCCONL(0x01);
	while (OSCCONbits.COSC != 0b011);	/*	Wait for Clock switch to occur	*/
	while(!OSCCONbits.LOCK);	


	/* Intialize the speex decoder and obtain a handle to the encoded
	 * speech segment in program memory */
	 
	Speex16KHzDecoderInit(speexDecoderState,scratchX,scratchY);
	samples = PGMemoryOpen(&samplesInPGMemory,(long)SpeechSegment);

	/* Intialize the board and the drivers, start the codec 
	 * communication and configure the codec for 8KHz
	 * sampling operation.	*/
	 
	Explorer16Init();
	// Initialisation du DAC
	TRISBbits.TRISB2=0; //SPI
	TRISFbits.TRISF6=0;
	TRISFbits.TRISF8=0;
	a = 0;
	not_a = 1;
	for (i=0; i<WB_SPEEX_DECODER_OUTPUT_SIZE; i++)
	{
		buf[a][i] = 0;
	}
	b = 0;
	PR2 = 2498;
	TMR2 = 0;
	IFS0bits.T2IF = 0;
	IEC0bits.T2IE = 1;
	T2CONbits.TON = 1;		// ne sert que la 1ere fois
	
	while(1)
	{	
		LED3 = EXPLORER16_LED_ON;
		
		bytesRead += PGMemoryRead(samples,encodedSamples,WB_SPEEX_ENCODED_FRAME_SIZE_12K8);
		
		if(bytesRead < SPEECH_SEGMENT_SIZE_BYTES)
		{
			/* Decode the encoded frame and output to the 
			 * codec. Wait till the codec is available for a new  frame	
			 * and then write to the codec */
			 
			decoderRetVal = Speex16KHzDecode(speexDecoderState, encodedSamples, decodedSamples);
		
			if(decoderRetVal == SPEEX_INVALID_MODE_ID)
			{
				/* If there is mode id error, then halt */
				LED4 = EXPLORER16_LED_ON;
				LED3 = EXPLORER16_LED_OFF;
				while(1);
			}	
				
			// Ecriture dans le DAC
			for (i=0; i<WB_SPEEX_DECODER_OUTPUT_SIZE; i++)
			{
				buf[not_a][i] = 2048 + decodedSamples[i]/8;
				if ( (buf[not_a][i] >= 4096) || ( buf[not_a][i] < 0 ) )
					LED4 = EXPLORER16_LED_ON;
			}
			while ( not_a != a );	// on attend que le DAC ait vidé l'autre buffer
			if ( a == 1 )
				not_a = 0;
			else
				not_a = 1;
		}
		else
		{
			/* Reset the byte counter and rewind the program memory
			 * pointer to start of the speech segment */
			
			bytesRead = 0;
			samples = PGMemoryOpen(&samplesInPGMemory,(long)SpeechSegment);
		}
	}
		

	
}


void __attribute__((interrupt, no_auto_psv))_T2Interrupt(void)  
{
	int j = 0;
	int mot1;
	
	mot1 = 0x3000 | (buf[a][b] & 0x0FFF);
	PORTBbits.RB2 = 0; // lach enable low -> pas de données dans le dac
	for(j=0; j<16; j++) //ecriture d'un bit
	{
		LATFbits.LATF6 = 0;
		if ( mot1 & 0x8000 )
			LATFbits.LATF8 = 1;
        else
			LATFbits.LATF8 = 0;
		mot1 = mot1<<1;
		LATFbits.LATF6 = 1;//coup d'horloge sur la patte Clk du DAC
	}
	PORTBbits.RB2 = 1; // latch enable 1, on prend en compte le dac
	b++;
	if ( b >= WB_SPEEX_DECODER_OUTPUT_SIZE )
	{
		b = 0;
		if ( a == 1 )
		{
			a = 0;
		}	
		else
		{
			a = 1;
		}	
	}
	IFS0bits.T2IF = 0;
}


void ecrit_dac_signal(int sig)
{ 
	int j=0;
    int mot1=0x3000|sig;
	PORTBbits.RB2 = 0; // lach enable low -> pas de données dans le dac
	for(j=0;j<16;j++) //ecriture d'un bit
	{
		LATFbits.LATF6 = 0;
//		for(i=0;i<3;i++);
		if(mot1&0x8000)
			LATFbits.LATF8=1;
        else
			LATFbits.LATF8=0;
		mot1=mot1<<1;
		LATFbits.LATF6 = 1;//coup d'horloge sur la patte Clk du DAC
		
	}
	
	PORTBbits.RB2 = 1; // latch enable 1, on prend en compte le dac


}
