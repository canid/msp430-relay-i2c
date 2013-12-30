//******************************************************************************
//   MSP430G2x31 - I2C Relay Controller, Master
//
//   Description: This is the test firmware for the I2C master controller. It
// currently advances, by action of s2 on the launchpad (falling edge of P1.3),
// the control word from 0x00 through 0x1111 were each bit is the relay to be
// (0) opened or (1) closed.
//
//******************************************************************************

#include <msp430g2231.h>

char MST_Data = 0;						// Variable for transmitted data
char SLV_Addr = 0x90;					// Address is 0x48 << 1 bit + 0 for Write
int I2C_State = 0;						// State variable

void main(void) {
	volatile unsigned int i;             // Use volatile to prevent removal

	WDTCTL = WDTPW + WDTHOLD;            // Stop watchdog
	if (CALBC1_1MHZ ==0xFF || CALDCO_1MHZ == 0xFF) {
		while(1);                          // If calibration constants erased do not load, trap CPU!!
	}
	BCSCTL1 = CALBC1_1MHZ;               // Set DCO
	DCOCTL = CALDCO_1MHZ;

	P1OUT = 0xC0;                        // P1.6 & P1.7 Pullups, others to 0
	P1OUT |= BIT3;
	P1REN |= 0xC0;                       // P1.3, P1.6 & P1.7 Pullups
	P1REN |= BIT3;
	P1DIR = 0xFF;
	P1DIR &= ~BIT3;                   // Unused pins as outputs. s2 as input.
	P1IE |= BIT3; 	   				// Enable P1 interrupt for P1.3
	P2OUT = 0;
	P2DIR = 0xFF;

	USICTL0 = USIPE6+USIPE7+USIMST+USISWRST; // Port & USI mode setup
	USICTL1 = USII2C+USIIE;              // Enable I2C mode & USI interrupt
	USICKCTL = USIDIV_3+USISSEL_2+USICKPL; // Setup USI clocks: SCL = SMCLK/8 (~125kHz)
	USICNT |= USIIFGCC;                  // Disable automatic clear control
	USICTL0 &= ~USISWRST;                // Enable USI
	USICTL1 &= ~USIIFG;                  // Clear pending flag
	P1IFG &= ~BIT3;						// Clear p1.3 interrupt flag.
	_EINT();

	while(1) {
		USICTL1 |= USIIFG;                 // Set flag and start communication
		LPM0;                              // CPU off, await USI interrupt
		_NOP();                            // Used for IAR
		for (i = 0; i < 5000; i++);        // Dummy delay between communication cycles
	}
}

/******************************************************
// Port1 isr
******************************************************/

#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void) {
	USICTL1 &= ~USIIE;			// Disable I2C interrupt until we finish.

	P1IE &= ~BIT3;				// Clear own interrupt until we finish

	if(MST_Data < 15) {
		MST_Data++;            // Increment Master data
	} else {
		MST_Data = 0;
	}
	P1OUT &= ~BIT0;
	__delay_cycles(1000000);
	P1OUT |= BIT0;

	P1IE |= BIT3;				// Re-enable own interrupt and clear interrupt flag.
	P1IFG &= ~BIT3;

	USICTL1 |= USIIE;			// Re-enable I2C interrupt and clear pending flag.
	USICTL1 &= ~USIIFG;
}

/******************************************************
// USI isr
******************************************************/
#pragma vector = USI_VECTOR
__interrupt void USI_TXRX (void) {
	switch(I2C_State) {
		case 0: // Generate Start Condition & send address to slave
              //P1OUT |= 0x01;           // LED on: sequence start
			USISRL = 0x00;           // Generate Start Condition...
			USICTL0 |= USIGE+USIOE;
			USICTL0 &= ~USIGE;
			USISRL = SLV_Addr;       // ... and transmit address, R/W = 0
			USICNT = (USICNT & 0xE0) + 0x08; // Bit counter = 8, TX Address
			I2C_State = 2;           // Go to next state: receive address (N)Ack
			break;

		case 2: // Receive Address Ack/Nack bit
			USICTL0 &= ~USIOE;       // SDA = input
			USICNT |= 0x01;          // Bit counter = 1, receive (N)Ack bit
			I2C_State = 4;           // Go to next state: check (N)Ack
			break;

		case 4: // Process Address Ack/Nack & handle data TX
			USICTL0 |= USIOE;        // SDA = output
			if (USISRL & 0x01) {       // If Nack received, send stop
				USISRL = 0x00;
				USICNT |=  0x01;       // Bit counter = 1, SCL high, SDA low
				I2C_State = 10;        // Go to next state: generate Stop
				//P1OUT |= 0x01;         // Turn on LED: error
			} else { // Ack received, TX data to slave...
				USISRL = MST_Data;     // Load data byte
				USICNT |=  0x08;       // Bit counter = 8, start TX
				I2C_State = 6;         // Go to next state: receive data (N)Ack
				//P1OUT &= ~0x01;        // Turn off LED
			} break;

		case 6: // Receive Data Ack/Nack bit
			USICTL0 &= ~USIOE;       // SDA = input
			USICNT |= 0x01;          // Bit counter = 1, receive (N)Ack bit
			I2C_State = 8;           // Go to next state: check (N)Ack
			break;

		case 8: // Process Data Ack/Nack & send Stop
			USICTL0 |= USIOE;
			if (USISRL & 0x01) {       // If Nack received...
                //P1OUT |= 0x01;         // Turn on LED: error
			} else {                     // Ack received
				//P1OUT &= ~0x01;        // Turn off LED
			}
			// Send stop...
			USISRL = 0x00;
			USICNT |=  0x01;         // Bit counter = 1, SCL high, SDA low
			I2C_State = 10;          // Go to next state: generate Stop
			break;

		case 10:// Generate Stop Condition
			USISRL = 0x0FF;          // USISRL = 1 to release SDA
			USICTL0 |= USIGE;        // Transparent latch enabled
			USICTL0 &= ~(USIGE+USIOE);// Latch/SDA output disabled
			I2C_State = 0;           // Reset state machine for next transmission
			LPM0_EXIT;               // Exit active for next transfer
			break;
	}
	USICTL1 &= ~USIIFG;                  // Clear pending flag
}

