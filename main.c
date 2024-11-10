#include "tm4c123gh6pm.h"
#include <stdint.h>

#define NUM_TAPS 31
#define ADC_SAMPLE_RATE 10000 // 10 kHz sampling rate

float fir_coefficients[NUM_TAPS] = {
    -0.0011, -0.0023, -0.0035, -0.0045, -0.0051, -0.0052, -0.0045, -0.0029,
    0.0000, 0.0042, 0.0095, 0.0154, 0.0213, 0.0265, 0.0305, 0.0325,
    0.0325, 0.0305, 0.0265, 0.0213, 0.0154, 0.0095, 0.0042, 0.0000,
    -0.0029, -0.0045, -0.0052, -0.0051, -0.0045, -0.0035, -0.0023
};

float input_buffer[NUM_TAPS] = {0};
uint32_t adc_value;
float filtered_output;

// Function prototypes
void ADC_Init(void);
void Timer0A_Init(void);
void PWM_Init(void);
void ADC0Seq3_Handler(void);
void Timer0A_Handler(void);

void ADC_Init(void) {
    SYSCTL_RCGCADC_R |= 1; // Enable ADC0
    SYSCTL_RCGCGPIO_R |= (1 << 4); // Enable clock for Port E
    GPIO_PORTE_AFSEL_R |= (1 << 3); // Enable alternate function on PE3
    GPIO_PORTE_DEN_R &= ~(1 << 3); // Disable digital function on PE3
    GPIO_PORTE_AMSEL_R |= (1 << 3); // Enable analog function on PE3

    ADC0_ACTSS_R &= ~8; // Disable sample sequencer 3
    ADC0_EMUX_R = (ADC0_EMUX_R & 0xFFFF0FFF) | (0x5 << 12); // Timer trigger
    ADC0_SSMUX3_R = 0; // AIN0 (PE3)
    ADC0_SSCTL3_R = 0x6; // End of sequence, interrupt enabled
    ADC0_IM_R |= 8; // Enable interrupt
    ADC0_ACTSS_R |= 8; // Enable sample sequencer 3
    NVIC_EN0_R |= (1 << 17); // Enable ADC0SS3 interrupt
}

void Timer0A_Init(void) {
    SYSCTL_RCGCTIMER_R |= 1; // Enable Timer 0
    TIMER0_CTL_R = 0; // Disable Timer 0A
    TIMER0_CFG_R = 0; // 32-bit mode
    TIMER0_TAMR_R = 0x02; // Periodic mode
    TIMER0_TAILR_R = 1600000 - 1; // 10 kHz sampling rate
    TIMER0_IMR_R |= 0x01; // Enable timeout interrupt
    TIMER0_CTL_R |= 0x01; // Enable Timer 0A
    NVIC_EN0_R |= (1 << 19); // Enable Timer 0A interrupt
}

void PWM_Init(void) {
    SYSCTL_RCGCPWM_R |= 2; // Enable PWM1
    SYSCTL_RCGCGPIO_R |= (1 << 5); // Enable clock for Port F
    GPIO_PORTF_AFSEL_R |= (1 << 2); // Enable alternate function on PF2
    GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R & 0xFFFFF0FF) | 0x00000500;
    GPIO_PORTF_DEN_R |= (1 << 2);

    PWM1_3_CTL_R = 0;
    PWM1_3_GENA_R = 0xC8;
    PWM1_3_LOAD_R = 16000 - 1; // 10 kHz frequency
    PWM1_3_CMPA_R = 8000; // 50% duty cycle
    PWM1_3_CTL_R |= 1;
    PWM1_ENABLE_R |= (1 << 6);
}

void ADC0Seq3_Handler(void) {
    ADC0_ISC_R = 8; // Clear interrupt
    adc_value = ADC0_SSFIFO3_R; // Read ADC value
}

void Timer0A_Handler(void) {
    TIMER0_ICR_R = 0x01; // Clear interrupt

    // Declare the loop variable
    int i;

    // Shift buffer and add new sample
    for (i = NUM_TAPS - 1; i > 0; i--) {
        input_buffer[i] = input_buffer[i - 1];
    }
    input_buffer[0] = (float)adc_value;

    // FIR Filter Calculation
    filtered_output = 0;
    for (i = 0; i < NUM_TAPS; i++) {
        filtered_output += fir_coefficients[i] * input_buffer[i];
    }

    // Convert filtered output to PWM duty cycle (emulated DAC)
    PWM1_3_CMPA_R = (uint16_t)((filtered_output / 4096.0) * 16000);
}

int main(void) {
    ADC_Init();
    Timer0A_Init();
    PWM_Init();
    while (1) {
        // Main loop does nothing, processing is in ISR
    }
}
