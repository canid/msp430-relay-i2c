//******************************************************************************
//   MSP430G2x31 - I2C Automation Controller/Sensor, Slave Node
//
//   Description: This is the firmware for the i2c slave controller.
//
//******************************************************************************

#include <msp430.h>

// peripheral selections
#define ADC								// Define if slave needs ADC10

//   Select peripheral type for each pin:
// RELAY for relay or other digital output such as MOSFET or other TTL control,
// DIGITAL for digtal input such as TTL sensor, switch, etc.
// CT for current transformer,
// NTC for negative temp. coefficient thermistor,
// PTC for positive temp. coefficient thermistor,
#define P1_0 RELAY
#define P1_1 RELAY
#define P1_2 RELAY
#define P1_3 RELAY
#define P1_4 CT
#define P1_5 NTC

char ownAddr = 0x90;                  // Address is 0x48<<1 for R/W
int i2cState = 0;                     // State variable
unsigned char RXData=0;

void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;            // Stop watchdog
  if (CALBC1_1MHZ ==0xFF || CALDCO_1MHZ == 0xFF)
  {
    while(1);                          // If calibration constants erased
                                       // do not load, trap CPU!!
  }

  // Peripherals setup
  // port setup stuff
  #if P1_0 == RELAY
    P1DIR |= BIT0;						// set pin to output.
  #elif P1_0 == THERM
    P1DIR &= ~BIT0;						// set pin to input.
    ADC10CTL1 |= INCH0;					// enable adc chan. 0
  #elif P1_0 == CT
    P1DIR &= ~BIT0;						// set pin to input.
    ADC10CTL1 |= INCH0;					// enable adc chan
  #elif P1_0 == ACLK

  #elif P1_0 == TA0CLK

  #endif

  #if P1_1 == RELAY
    P1DIR |= BIT1;						// set pin to output.
  #elif P1_1 == THERM
    P1DIR &= ~BIT1;						// set pin to input.
    ADC10CTL1 |= INCH1;					// enable adc chan.
  #elif P1_1 == CT
    P1DIR &= ~BIT1;						// set pin to input.
    ADC10CTL1 |= INCH1;					// enable adc chan.
  #elif P1_1 == TA0

  #endif

  #if P1_2 == RELAY
  	P1DIR |= BIT2;						// set pin to output.
  #elif P1_2 == THERM
    P1DIR &= ~BIT2;						// set pin to input.
    ADC10CTL1 |= INCH2;					// enable adc chan.
  #elif P1_2 == CT
    P1DIR &= ~BIT2;						// set pin to input.
    ADC10CTL1 |= INCH2;					// enable adc chan.
  #elif P1_2 == TA0

  #endif

  #if P1_3 == RELAY
  	P1DIR |= BIT3;						// set pin to output.
  #elif P1_3 == NTC
    P1DIR &= ~BIT3;						// set pin to input.
    ADC10CTL1 |= INCH3;					// enable adc chan.
  #elif P1_3 == CT
    P1DIR &= ~BIT3;						// set pin to input.
    ADC10CTL1 |= INCH3;					// enable adc chan.
  #elif P1_3 == ADC10CLK

  #elif P1_3 == VREFn

  #endif

  #if P1_4 == RELAY
  	P1DIR |= BIT4;						// set pin to output.
  #elif P1_4 == THERM
    P1DIR &= ~BIT4;						// set pin to input.
    ADC10CTL1 |= INCH4;					// enable adc chan.
  #elif P1_4 == CT
    P1DIR &= ~BIT4;						// set pin to input.
    ADC10CTL1 |= INCH4;					// enable adc chan.
  #elif P1_4 == SMCLK

  #elif P1_4 == VREFp

  #endif

  #if P1_5 == RELAY
  	P1DIR |= BIT5;						// set pin to output.
  #elif P1_5 == THERM
  	P1DIR &= ~BIT5;						// set pin to input.
  	ADC10CTL1 |= INCH5;					// enable adc chan.
  #elif P1_5 == CT
  	P1DIR &= ~BIT5;						// set pin to input.
  	ADC10CTL1 |= INCH5;					// enable adc chan.
  #elif P1_5 == SCLK

  #endif


  BCSCTL1 = CALBC1_1MHZ;               // Set DCO
  DCOCTL = CALDCO_1MHZ;

  P1OUT = 0xC0;                        // P1.6 & P1.7 Pullups
  P1REN |= 0xC0;                       // P1.6 & P1.7 Pullups
  P2OUT = 0;
  P2DIR = 0xFF;

  USICTL0 = USIPE6+USIPE7+USISWRST;    // Port & USI mode setup
  USICTL1 = USII2C+USIIE+USISTTIE;     // Enable I2C mode & USI interrupts
  USICKCTL = USICKPL;                  // Setup clock polarity
  USICNT |= USIIFGCC;                  // Disable automatic clear control
  USICTL0 &= ~USISWRST;                // Enable USI
  USICTL1 &= ~USIIFG;                  // Clear pending flag
  _EINT();

  #ifdef ADC
    ADC10CTL0 |= ENC + ADC10SC;
  #endif

  while(1)
  {
    LPM0;                              // CPU off, await USI interrupt
    _NOP();                            // Used for IAR
  }
}

//******************************************************************************
// USI interrupt service routine
//******************************************************************************
#pragma vector = USI_VECTOR
__interrupt void USI_TXRX (void)
{
  if (USICTL1 & USISTTIFG)             // Start entry?
  {
    P1OUT |= 0x01;                     // LED on: sequence start
    i2cState = 2;                     // Enter 1st state on start
  }

  switch(i2cState)
    {
      case 0: // Idle, should not get here
              break;

      case 2: // RX Address
              USICNT = (USICNT & 0xE0) + 0x08; // Bit counter = 8, RX address
              USICTL1 &= ~USISTTIFG;   // Clear start flag
              i2cState = 4;           // Go to next state: check address
              break;

      case 4: // Process Address and send (N)Ack
              if (USISRL & 0x01)       // If read...
                ownAddr++;            // Save R/W bit
              USICTL0 |= USIOE;        // SDA = output
              if (USISRL == ownAddr)  // Address match?
              {
                USISRL = 0x00;         // Send Ack
                P1OUT &= ~0x01;        // LED off
                i2cState = 8;         // Go to next state: RX data
              }
              else
              {
                USISRL = 0xFF;         // Send NAck
                P1OUT |= 0x01;         // LED on: error
                i2cState = 6;         // Go to next state: prep for next Start
              }
              USICNT |= 0x01;          // Bit counter = 1, send (N)Ack bit
              break;

      case 6: // Prep for Start condition
              USICTL0 &= ~USIOE;       // SDA = input
              ownAddr = 0x90;         // Reset slave address
              i2cState = 0;           // Reset state machine
              break;

      case 8: // Receive data byte
              USICTL0 &= ~USIOE;       // SDA = input
              USICNT |=  0x08;         // Bit counter = 8, RX data
              i2cState = 10;          // Go to next state: Test data and (N)Ack
              break;

      case 10:// Check Data & TX (N)Ack
              RXData = USISRL;                 // Get RX data

              USICTL0 |= USIOE;        // SDA = output
              P1OUT = 0x0;
              if(RXData & BIT0) {
                  P1OUT |= BIT1;
              } if(RXData & BIT1) {
                  P1OUT |= BIT2;
              } if(RXData & BIT2) {
                  P1OUT |= BIT3;
              } if(RXData & BIT3) {
                  P1OUT |= BIT4;
              } USISRL = 0x00;         // Send Ack
              P1OUT &= ~0x01;        // LED off
/*              {
                USISRL = 0xFF;         // Send NAck
                P1OUT |= 0x01;         // LED on: error
              }
*/
              USICNT |= 0x01;          // Bit counter = 1, send (N)Ack bit
              i2cState = 6;           // Go to next state: prep for next Start
              break;
    }

  USICTL1 &= ~USIIFG;                  // Clear pending flags
}

