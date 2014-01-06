//******************************************************************************
//   MSP430G2x31 - I2C Automation Controller/Sensor, Slave Node
//
//   Description: This is the firmware for the i2c slave controller.
//
//******************************************************************************

#include <msp430.h>

// peripheral selections
// Define if slave needs ADC10
#define ADC								

// Select peripheral type for each pin:
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

// Address is 0x48<<1 for R/W
char ownAddr = 0x90;         

// State variable
int i2cState = 0;           

unsigned char RXData=0;

void main(void)
{
  // Stop watchdog
  WDTCTL = WDTPW + WDTHOLD;            
  if (CALBC1_1MHZ ==0xFF || CALDCO_1MHZ == 0xFF)
  {
    
    // If calibration constants erased
    while(1);          
    
    // do not load, trap CPU!!
                                       
  }

  // Peripherals setup
  // port setup stuff
  #if P1_0 == RELAY
  
    // set pin to output.
    P1DIR |= BIT0;						
  #elif P1_0 == THERM
  
    // set pin to input.
    P1DIR &= ~BIT0;			
    
    // enable adc chan. 0
    ADC10CTL1 |= INCH0;					
  #elif P1_0 == CT
  
    // set pin to input.
    P1DIR &= ~BIT0;				
    
    // enable adc chan
    ADC10CTL1 |= INCH0;					
  #elif P1_0 == ACLK

  #elif P1_0 == TA0CLK

  #endif

  #if P1_1 == RELAY
  
    // set pin to output.
    P1DIR |= BIT1;						
  #elif P1_1 == THERM
  
    // set pin to input.
    P1DIR &= ~BIT1;			
    
    // enable adc chan.
    ADC10CTL1 |= INCH1;					
  #elif P1_1 == CT
  
    // set pin to input.
    P1DIR &= ~BIT1;			
    
    // enable adc chan.
    ADC10CTL1 |= INCH1;					
  #elif P1_1 == TA0

  #endif

  #if P1_2 == RELAY
  
    // set pin to output.
  	P1DIR |= BIT2;						
  #elif P1_2 == THERM
  
    // set pin to input.
    P1DIR &= ~BIT2;				
    
    // enable adc chan.
    ADC10CTL1 |= INCH2;					
  #elif P1_2 == CT
  
    // set pin to input.
    P1DIR &= ~BIT2;					
    
    // enable adc chan.
    ADC10CTL1 |= INCH2;					
  #elif P1_2 == TA0

  #endif

  #if P1_3 == RELAY
  
    // set pin to output.
  	P1DIR |= BIT3;						
  #elif P1_3 == NTC
  
    // set pin to input.
    P1DIR &= ~BIT3;				
    
    // enable adc chan.
    ADC10CTL1 |= INCH3;					
  #elif P1_3 == CT
  
    // set pin to input.
    P1DIR &= ~BIT3;				
    
    // enable adc chan.
    ADC10CTL1 |= INCH3;					
  #elif P1_3 == ADC10CLK

  #elif P1_3 == VREFn

  #endif

  #if P1_4 == RELAY
  
    // set pin to output.
  	P1DIR |= BIT4;						
  #elif P1_4 == THERM
  
    // set pin to input.
    P1DIR &= ~BIT4;					
    
    // enable adc chan.
    ADC10CTL1 |= INCH4;					
  #elif P1_4 == CT
  
    // set pin to input.
    P1DIR &= ~BIT4;						
    
    // enable adc chan.
    ADC10CTL1 |= INCH4;					
  #elif P1_4 == SMCLK

  #elif P1_4 == VREFp

  #endif

  #if P1_5 == RELAY
  
    // set pin to output.
  	P1DIR |= BIT5;						
  #elif P1_5 == THERM
  
    // set pin to input.
  	P1DIR &= ~BIT5;				
  	
  	// enable adc chan.
  	ADC10CTL1 |= INCH5;					
  #elif P1_5 == CT
  
    // set pin to input.
  	P1DIR &= ~BIT5;				
  	
  	// enable adc chan.
  	ADC10CTL1 |= INCH5;					
  #elif P1_5 == SCLK

  #endif


  // Set DCO
  BCSCTL1 = CALBC1_1MHZ;               
  DCOCTL = CALDCO_1MHZ;

  // P1.6 & P1.7 Pullups
  P1OUT = 0xC0;       
  
  // P1.6 & P1.7 Pullups
  P1REN |= 0xC0;                       
  P2OUT = 0;
  P2DIR = 0xFF;

  // Port & USI mode setup
  USICTL0 = USIPE6+USIPE7+USISWRST;    
  
  // Enable I2C mode & USI interrupts
  USICTL1 = USII2C+USIIE+USISTTIE;     
  
  // Setup clock polarity
  USICKCTL = USICKPL;         
  
  // Disable automatic clear control
  USICNT |= USIIFGCC;      
  
  // Enable USI
  USICTL0 &= ~USISWRST;          
  
  // Clear pending flag
  USICTL1 &= ~USIIFG;                  
  _EINT();

  #ifdef ADC
    ADC10CTL0 |= ENC + ADC10SC;
  #endif

  while(1)
  {
    // CPU off, await USI interrupt
    LPM0;                          
    
    // Used for IAR
    _NOP();                            
  }
}

//******************************************************************************
// USI interrupt service routine
//******************************************************************************
#pragma vector = USI_VECTOR
__interrupt void USI_TXRX (void)
{
  
  // Start entry?
  if (USICTL1 & USISTTIFG)             
  {
    
    // LED on: sequence start
    P1OUT |= 0x01;           
    
    // Enter 1st state on start
    i2cState = 2;                     
  }

  switch(i2cState)
    {
      
      // Idle, should not get here
      case 0: 
              break;

      // RX Address
      case 2: 
      
              // Bit counter = 8, RX address
              USICNT = (USICNT & 0xE0) + 0x08; 
              
              // Clear start flag
              USICTL1 &= ~USISTTIFG;   
              
              // Go to next state: check address
              i2cState = 4;           
              break;

      // Process Address and send (N)Ack
      case 4: 
      
              // If read...
              if (USISRL & 0x01)    
              
                // Save R/W bit
                ownAddr++;       
                
              // SDA = output
              USICTL0 |= USIOE;      
              
              // Address match?
              if (USISRL == ownAddr)  
              {
                
                // Send Ack
                USISRL = 0x00;    
                
                // LED off
                P1OUT &= ~0x01;      
                
                // Go to next state: RX data
                i2cState = 8;         
              }
              else
              {
                
                // Send NAck
                USISRL = 0xFF;    
                
                // LED on: error
                P1OUT |= 0x01;     
                
                // Go to next state: prep for next Start
                i2cState = 6;         
              }
              
              // Bit counter = 1, send (N)Ack bit
              USICNT |= 0x01;          
              break;

      // Prep for Start condition
      case 6: 
      
              // SDA = input
              USICTL0 &= ~USIOE;     
              
              // Reset slave address
              ownAddr = 0x90;   
              
              // Reset state machine
              i2cState = 0;           
              break;

      // Receive data byte
      case 8: 
      
              // SDA = input
              USICTL0 &= ~USIOE;     
              
              // Bit counter = 8, RX data
              USICNT |=  0x08;       
              
              // Go to next state: Test data and (N)Ack
              i2cState = 10;          
              break;

      // Check Data & TX (N)Ack
      case 10:
      
              // Get RX data
              RXData = USISRL;                 

              // SDA = output
              USICTL0 |= USIOE;        
              P1OUT = 0x0;
              if(RXData & BIT0) {
                  P1OUT |= BIT1;
              } if(RXData & BIT1) {
                  P1OUT |= BIT2;
              } if(RXData & BIT2) {
                  P1OUT |= BIT3;
              } if(RXData & BIT3) {
                  P1OUT |= BIT4;
              } 
              
              // Send Ack
              USISRL = 0x00;     
              
              // LED off
              P1OUT &= ~0x01;        
/*              {
                // Send NAck
                USISRL = 0xFF;    
                
                // LED on: error
                P1OUT |= 0x01;         
              }
*/
              
              // Bit counter = 1, send (N)Ack bit
              USICNT |= 0x01;     
              
              // Go to next state: prep for next Start
              i2cState = 6;           
              break;
    }

  USICTL1 &= ~USIIFG;                  // Clear pending flags
}

