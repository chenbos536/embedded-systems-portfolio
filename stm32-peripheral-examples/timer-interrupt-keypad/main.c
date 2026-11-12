#include <stdint.h>

/*
 * Hardware connections:
 *
 * LED1:
 *   PB0 -> LED long leg
 *   LED short leg -> 330 ohm resistor -> GND
 *
 * LED2:
 *   PB1 -> LED long leg
 *   LED short leg -> 330 ohm resistor -> GND
 *
 * Pushbutton:
 *   3.3V -> button -> PA0
 *   PA0 uses internal pull-down
 */

/* RCC registers */
#define RCC_AHB2ENR     (*(volatile uint32_t*)0x44020C8C)
#define RCC_APB1LENR    (*(volatile uint32_t*)0x44020C9C)

/* GPIOA registers */
#define GPIOA_MODER     (*(volatile uint32_t*)0x42020000)
#define GPIOA_PUPDR     (*(volatile uint32_t*)0x4202000C)
#define GPIOA_IDR       (*(volatile uint32_t*)0x42020010)

/* GPIOB registers */
#define GPIOB_MODER     (*(volatile uint32_t*)0x42020400)
#define GPIOB_OTYPER    (*(volatile uint32_t*)0x42020404)
#define GPIOB_PUPDR     (*(volatile uint32_t*)0x4202040C)
#define GPIOB_ODR       (*(volatile uint32_t*)0x42020414)
#define GPIOB_BSRR      (*(volatile uint32_t*)0x42020418)

/* TIM2 registers */
#define TIM2_CR1        (*(volatile uint32_t*)0x40000000)
#define TIM2_DIER       (*(volatile uint32_t*)0x4000000C)
#define TIM2_SR         (*(volatile uint32_t*)0x40000010)
#define TIM2_EGR        (*(volatile uint32_t*)0x40000014)
#define TIM2_CNT        (*(volatile uint32_t*)0x40000024)
#define TIM2_PSC        (*(volatile uint32_t*)0x40000028)
#define TIM2_ARR        (*(volatile uint32_t*)0x4000002C)

/* NVIC register */
#define NVIC_ISER1      (*(volatile uint32_t*)0xE000E104)

/* Bit positions */
#define GPIOA_EN        0
#define GPIOB_EN        1
#define TIM2_EN         0

#define LED_BLINK_PIN   0   /* PB0 */
#define LED_BUTTON_PIN  1   /* PB1 */
#define BUTTON_PIN      0   /* PA0 */

/*
 * TIM2 interrupt number for STM32H563 is in NVIC_ISER1.
 * TIM2 IRQ number = 45, so inside ISER1 the bit is 45 - 32 = 13.
 */
#define TIM2_IRQ_BIT_IN_ISER1  13

static void gpio_init(void)
{
	/*
	 * This function sets up the pins.
	 *
	 * PB0 and PB1 are outputs because they control the two LEDs.
	 * PA0 is an input because it reads the pushbutton.
	 *
	 * Enable GPIOA and GPIOB clocks */
    RCC_AHB2ENR |= (1 << GPIOA_EN);
    RCC_AHB2ENR |= (1 << GPIOB_EN);

    /*
     * PB0 and PB1 output mode.
     * GPIO mode bits:
     * 00 = input
     * 01 = output
     * 10 = alternate function
     * 11 = analog
     */

    /* Clear mode bits for PB0 and PB1 */
    GPIOB_MODER &= ~(3U << (LED_BLINK_PIN * 2));
    GPIOB_MODER &= ~(3U << (LED_BUTTON_PIN * 2));

    /* Set PB0 and PB1 to output mode */
    GPIOB_MODER |=  (1U << (LED_BLINK_PIN * 2));
    GPIOB_MODER |=  (1U << (LED_BUTTON_PIN * 2));

    /* Push-pull output for PB0 and PB1 */
    GPIOB_OTYPER &= ~(1U << LED_BLINK_PIN);
    GPIOB_OTYPER &= ~(1U << LED_BUTTON_PIN);

    /* No pull-up / pull-down for LED pins */
    GPIOB_PUPDR &= ~(3U << (LED_BLINK_PIN * 2));
    GPIOB_PUPDR &= ~(3U << (LED_BUTTON_PIN * 2));

    /*
     * PA0 input mode for pushbutton.
     * Button wiring:
     * 3.3V -> button -> PA0
     * So PA0 should use internal pull-down.
     */

    /* PA0 input mode */
    GPIOA_MODER &= ~(3U << (BUTTON_PIN * 2));

    /* PA0 pull-down: PUPDR = 10 */
    GPIOA_PUPDR &= ~(3U << (BUTTON_PIN * 2));
    GPIOA_PUPDR |=  (2U << (BUTTON_PIN * 2));

    /* Start with both LEDs off */
    GPIOB_BSRR = (1U << (LED_BLINK_PIN + 16));
    GPIOB_BSRR = (1U << (LED_BUTTON_PIN + 16));
}

static void tim2_init(void)
{
	/*
	 * tim2_init()
	 *
	 * This function sets up TIM2.
	 *
	 * TIM2 is used to make LED1 blink without using delay.
	 * The timer counts in the background.
	 * When one second passes, TIM2 creates an interrupt.
	 *
	 */
    /* Enable TIM2 clock.*/
    RCC_APB1LENR |= (1 << TIM2_EN);

    /* Disable TIM2 while configuring */
    TIM2_CR1 &= ~1U;

    /*
     * TIM2 clock is 32 MHz:
     *
     * Prescaler = 32000 - 1
     * Timer count frequency = 64,000,000 / 64,000 = 1000 Hz
     *
     * ARR = 1000 - 1
     * Update event frequency = 1000 / 1000 = 1 Hz
     *
     * So TIM2 interrupt happens once per second.
     */
    TIM2_PSC = 32000 - 1;
    TIM2_ARR = 1000 - 1;
    TIM2_CNT = 0;

    /* Enable auto-reload preload, optional but clean */
    TIM2_CR1 |= (1U << 7);

    /* Enable update interrupt in TIM2 */
    TIM2_DIER |= 1U;

    /*
     * Generate an update event so PSC/ARR values are loaded.
     * Then clear UIF because EGR can set the update flag.
     */
    TIM2_EGR |= 1U;
    TIM2_SR &= ~1U;

    /*
     * Enable TIM2 interrupt in NVIC.
     * TIM2 IRQ number 45 -> ISER1 bit 13.
     */
    NVIC_ISER1 |= (1U << TIM2_IRQ_BIT_IN_ISER1);

    /* Enable TIM2 counter */
    TIM2_CR1 |= 1U;
}

 /*
 * The main loop only controls LED2.
 */
int main(void)
{
    gpio_init();
    tim2_init();

    while (1)
    {
        /*
         * Read pushbutton on PA0.
         *
         * Not pressed: PA0 = 0 because of internal pull-down
         * Pressed:     PA0 = 1 because button connects PA0 to 3.3V
         */
        if (GPIOA_IDR & (1U << BUTTON_PIN))
        {
            /* Turn on PB1 LED */
            GPIOB_BSRR = (1U << LED_BUTTON_PIN);
        }
        else
        {
            /* Turn off PB1 LED */
            GPIOB_BSRR = (1U << (LED_BUTTON_PIN + 16));
        }
    }
}

/*
 * This function runs automatically when TIM2 interrupts.
 * It is not called by main().
 *
 * Every one second, TIM2 sets the UIF flag.
 * Inside this handler:
 *   1. Check the UIF flag
 *   2. Clear the UIF flag
 *   3. Toggle PB0
 *
 */
void TIM2_IRQHandler(void)
{
    /*
     * UIF is bit 0 in TIM2_SR.
     * Hardware sets UIF when update event happens.
     * Software must clear it inside the ISR.
     */
    if (TIM2_SR & 1U)
    {
        TIM2_SR &= ~1U;

        /* Toggle PB0 LED once per timer interrupt */
        GPIOB_ODR ^= (1U << LED_BLINK_PIN);
    }
}
