#include <stdint.h>

/*
 * A5 Final Simple Version:
 * PWM breathing LED controlled by potentiometer.
 *
 * LED:
 * PB0 / D33 -> LED long leg
 * LED short leg -> 330 ohm resistor -> GND
 *
 * Potentiometer:
 * One outer pin   -> 3.3V
 * Middle pin      -> CN9 A0 / PA6
 * Other outer pin -> GND
 *
 * Behavior:
 * LED gradually gets brighter, then gradually gets darker.
 * Potentiometer controls the breathing speed.
 */

/* RCC registers */
#define RCC_AHB2ENR      (*(volatile uint32_t*)0x44020C8C)
#define RCC_APB1LENR     (*(volatile uint32_t*)0x44020C9C)

/* GPIOA: PA6 = CN9 A0 ADC input */
#define GPIOA_MODER      (*(volatile uint32_t*)0x42020000)
#define GPIOA_PUPDR      (*(volatile uint32_t*)0x4202000C)

/* GPIOB: PB0 PWM LED output */
#define GPIOB_MODER      (*(volatile uint32_t*)0x42020400)
#define GPIOB_OTYPER     (*(volatile uint32_t*)0x42020404)
#define GPIOB_OSPEEDR    (*(volatile uint32_t*)0x42020408)
#define GPIOB_PUPDR      (*(volatile uint32_t*)0x4202040C)
#define GPIOB_AFRL       (*(volatile uint32_t*)0x42020420)

/* TIM3 registers */
#define TIM3_CR1         (*(volatile uint32_t*)0x40000400)
#define TIM3_EGR         (*(volatile uint32_t*)0x40000414)
#define TIM3_CCMR2       (*(volatile uint32_t*)0x4000041C)
#define TIM3_CCER        (*(volatile uint32_t*)0x40000420)
#define TIM3_PSC         (*(volatile uint32_t*)0x40000428)
#define TIM3_ARR         (*(volatile uint32_t*)0x4000042C)
#define TIM3_CCR3        (*(volatile uint32_t*)0x4000043C)

/* ADC1 registers */
#define ADC1_ISR         (*(volatile uint32_t*)0x42028000)
#define ADC1_CR          (*(volatile uint32_t*)0x42028008)
#define ADC1_CFGR        (*(volatile uint32_t*)0x4202800C)
#define ADC1_SMPR1       (*(volatile uint32_t*)0x42028014)
#define ADC1_PCSEL       (*(volatile uint32_t*)0x4202801C)
#define ADC1_SQR1        (*(volatile uint32_t*)0x42028030)
#define ADC1_DR          (*(volatile uint32_t*)0x42028040)
#define ADC1_DIFSEL      (*(volatile uint32_t*)0x420280B0)

/* ADC common register */
#define ADC12_CCR        (*(volatile uint32_t*)0x42028308)

/* Clock enable bits */
#define GPIOAEN          (1U << 0)
#define GPIOBEN          (1U << 1)
#define ADC12EN          (1U << 10)   /* ADC1/ADC2 clock enable */
#define TIM3EN           (1U << 1)
#define PWREN            (1U << 28)

/* TIM3 bits */
#define TIM_CR1_CEN      (1U << 0)
#define TIM_CR1_ARPE     (1U << 7)
#define TIM_EGR_UG       (1U << 0)
#define TIM_CCER_CC3E    (1U << 8)

/* ADC bits */
#define ADC_ISR_ADRDY    (1U << 0)
#define ADC_ISR_EOC      (1U << 2)

#define ADC_CR_ADEN      (1U << 0)
#define ADC_CR_ADDIS     (1U << 1)
#define ADC_CR_ADSTART   (1U << 2)
#define ADC_CR_ADVREGEN  (1U << 28)
#define ADC_CR_DEEPPWD   (1U << 29)
#define ADC_CR_ADCALDIF  (1U << 30)
#define ADC_CR_ADCAL     (1U << 31)

/* Project constants */
#define PWM_MAX          999U

/*
 * CN9 A0 = PA6.
 * PA6 uses ADC1 channel 3.
 */
#define POT_GPIO_PIN     6U
#define POT_CHANNEL      3U

/*
 * Linear speed range.
 * Smaller MIN_DELAY_COUNT = faster maximum speed.
 * Larger MAX_DELAY_COUNT = slower minimum speed.
 */
#define MIN_DELAY_COUNT  300U
#define MAX_DELAY_COUNT  12000U

#define TIMEOUT_LIMIT    5000000U

/*
 * If clockwise makes breathing slower instead of faster,
 * change this from 0 to 1.
 */
#define REVERSE_POT_DIRECTION 0

/* Debug variables */
volatile uint32_t debug_adc_ok = 0;
volatile uint32_t debug_adc_value = 0;
volatile uint32_t debug_delay_count = 0;
volatile uint32_t debug_duty = 0;

void delay_loop(uint32_t count)
{
    for (volatile uint32_t i = 0; i < count; i++) {
    }
}

void delay_short(void)
{
    for (volatile uint32_t i = 0; i < 300000U; i++) {
    }
}

int wait_until_set(volatile uint32_t *reg, uint32_t mask)
{
    uint32_t timeout = TIMEOUT_LIMIT;

    while (((*reg) & mask) == 0U) {
        timeout--;

        if (timeout == 0U) {
            return 0;
        }
    }

    return 1;
}

int wait_until_clear(volatile uint32_t *reg, uint32_t mask)
{
    uint32_t timeout = TIMEOUT_LIMIT;

    while (((*reg) & mask) != 0U) {
        timeout--;

        if (timeout == 0U) {
            return 0;
        }
    }

    return 1;
}

void init_pwm_pb0_tim3_ch3(void)
{
    /*
     * Enable GPIOB clock and TIM3 clock.
     */
    RCC_AHB2ENR  |= GPIOBEN;
    RCC_APB1LENR |= TIM3EN;

    /*
     * PB0 alternate function mode.
     * MODER0 = 10.
     */
    GPIOB_MODER &= ~(3U << 0);
    GPIOB_MODER |=  (2U << 0);

    /*
     * PB0 push-pull, high speed, no pull-up/pull-down.
     */
    GPIOB_OTYPER &= ~(1U << 0);

    GPIOB_OSPEEDR &= ~(3U << 0);
    GPIOB_OSPEEDR |=  (3U << 0);

    GPIOB_PUPDR &= ~(3U << 0);

    /*
     * PB0 AF2 = TIM3_CH3.
     */
    GPIOB_AFRL &= ~(0xFU << 0);
    GPIOB_AFRL |=  (2U << 0);

    /*
     * TIM3 PWM setup.
     * ARR is the PWM max count.
     * CCR3 is the duty cycle.
     */
    TIM3_CR1 = 0;

    TIM3_PSC = 63U;
    TIM3_ARR = PWM_MAX;
    TIM3_CCR3 = 0;

    /*
     * TIM3 channel 3 PWM mode 1.
     * OC3M bits [6:4] = 110.
     * OC3PE bit [3] = 1.
     */
    TIM3_CCMR2 &= ~(3U << 0);
    TIM3_CCMR2 &= ~(7U << 4);
    TIM3_CCMR2 |=  (6U << 4);
    TIM3_CCMR2 |=  (1U << 3);

    /*
     * Enable channel 3 output.
     */
    TIM3_CCER |= TIM_CCER_CC3E;

    /*
     * Enable auto-reload preload.
     */
    TIM3_CR1 |= TIM_CR1_ARPE;

    /*
     * Force update event.
     */
    TIM3_EGR = TIM_EGR_UG;

    /*
     * Start timer.
     */
    TIM3_CR1 |= TIM_CR1_CEN;
}

int init_adc_a0_pa6(void)
{
    /*
     * Enable PWR, GPIOA, and ADC clocks.
     */
    RCC_APB1LENR |= PWREN;
    RCC_AHB2ENR  |= GPIOAEN;
    RCC_AHB2ENR  |= ADC12EN;

    delay_short();

    /*
     * PA6 analog mode.
     * MODER6 = 11.
     */
    GPIOA_MODER |= (3U << (POT_GPIO_PIN * 2U));

    /*
     * No pull-up, no pull-down on PA6.
     */
    GPIOA_PUPDR &= ~(3U << (POT_GPIO_PIN * 2U));

    /*
     * ADC clock mode.
     * CKMODE bits [17:16] = 11.
     */
    ADC12_CCR &= ~(3U << 16);
    ADC12_CCR |=  (3U << 16);

    /*
     * Exit deep-power-down mode.
     */
    ADC1_CR &= ~ADC_CR_DEEPPWD;

    /*
     * Enable ADC voltage regulator.
     */
    ADC1_CR |= ADC_CR_ADVREGEN;

    delay_short();

    /*
     * Make sure ADC is disabled before calibration.
     */
    if ((ADC1_CR & ADC_CR_ADEN) != 0U) {
        ADC1_CR |= ADC_CR_ADDIS;

        if (!wait_until_clear(&ADC1_CR, ADC_CR_ADEN)) {
            return 0;
        }
    }

    /*
     * Single-ended calibration.
     */
    ADC1_CR &= ~ADC_CR_ADCALDIF;

    /*
     * Start ADC calibration.
     */
    ADC1_CR |= ADC_CR_ADCAL;

    if (!wait_until_clear(&ADC1_CR, ADC_CR_ADCAL)) {
        return 0;
    }

    /*
     * 12-bit resolution, right alignment, single conversion.
     */
    ADC1_CFGR = 0;

    /*
     * Enable ADC channel 3 path.
     */
    ADC1_PCSEL |= (1U << POT_CHANNEL);

    /*
     * Single-ended input.
     */
    ADC1_DIFSEL &= ~(1U << POT_CHANNEL);

    /*
     * Long sampling time for 10k potentiometer.
     */
    ADC1_SMPR1 &= ~(7U << (POT_CHANNEL * 3U));
    ADC1_SMPR1 |=  (7U << (POT_CHANNEL * 3U));

    /*
     * Regular sequence:
     * L = 0 means one conversion.
     * SQ1 bits [10:6] = channel number.
     */
    ADC1_SQR1 = (POT_CHANNEL << 6);

    /*
     * Clear ADC ready flag.
     */
    ADC1_ISR = ADC_ISR_ADRDY;

    /*
     * Enable ADC.
     */
    ADC1_CR |= ADC_CR_ADEN;

    /*
     * Wait until ADC is ready.
     */
    if (!wait_until_set(&ADC1_ISR, ADC_ISR_ADRDY)) {
        return 0;
    }

    return 1;
}

uint32_t read_adc_a0_pa6(int adc_ok)
{
    if (!adc_ok) {
        /*
         * If ADC fails, use middle value.
         */
        return 2048U;
    }

    /*
     * Clear end-of-conversion flag.
     */
    ADC1_ISR = ADC_ISR_EOC;

    /*
     * Start ADC conversion.
     */
    ADC1_CR |= ADC_CR_ADSTART;

    /*
     * Wait until conversion finishes.
     */
    if (!wait_until_set(&ADC1_ISR, ADC_ISR_EOC)) {
        return 2048U;
    }

    /*
     * Return 12-bit ADC value.
     */
    return ADC1_DR & 0xFFFU;
}

uint32_t adc_to_delay_count(uint32_t adc_value)
{
#if REVERSE_POT_DIRECTION
    adc_value = 4095U - adc_value;
#endif

    /*
     * Linear mapping:
     *
     * adc = 0:
     * delay is large, breathing is slow.
     *
     * adc = 4095:
     * delay is small, breathing is fast.
     */
    uint32_t delay_count =
        MAX_DELAY_COUNT -
        ((adc_value * (MAX_DELAY_COUNT - MIN_DELAY_COUNT)) / 4095U);

    return delay_count;
}
/*Before the loop, I initialize PWM on PB0 using TIM3 channel 3, and I initialize ADC on PA6 to read the potentiometer.*/
int main(void)
{
    int32_t duty = 0;
    int32_t direction = 1;

    init_pwm_pb0_tim3_ch3();

    int adc_ok = init_adc_a0_pa6();
    debug_adc_ok = (uint32_t)adc_ok;

    while (1) {
        /*
         * Read potentiometer value.
         */
        uint32_t adc_value = read_adc_a0_pa6(adc_ok);

        /*
         * Convert ADC value to delay using linear mapping.
         */
        uint32_t delay_count = adc_to_delay_count(adc_value);

        /*
         * Gradually increase or decrease PWM duty cycle.
         */
        duty += direction;

        if (duty >= (int32_t)PWM_MAX) {
            duty = PWM_MAX;
            direction = -1;
        } else if (duty <= 0) {
            duty = 0;
            direction = 1;
        }

        /*
         * Update PWM duty cycle.
         * Higher duty cycle means brighter LED.
         */
        TIM3_CCR3 = (uint32_t)duty;

        /*
         * Debug values.
         */
        debug_adc_value = adc_value;
        debug_delay_count = delay_count;
        debug_duty = (uint32_t)duty;

        /*
         * Smaller delay means faster brightness oscillation.
         */
        delay_loop(delay_count);
    }
}
