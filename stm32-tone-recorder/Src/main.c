#include <stdint.h>

/*
 * Tone Player / Recorder - Final STM32 Version
 * NUCLEO-H563ZI
 *
 * Simple demo explanation:
 * This project is a small tone keyboard with three modes.
 * In Play Mode, keys 1-8 directly play different notes.
 * In Record Mode, keys 1-8 still play notes, but the notes are also saved into EEPROM.
 * In Playback Mode, key 9 reads the saved notes from EEPROM and plays them back.
 *
 * The main idea is:
 *   keypad input  ->  mode decision  ->  speaker output / EEPROM save / EEPROM playback
 *
 * LEDs are used only as a visual indicator of the current mode.
 *
 * Features:
 *   1. Keypad buttons 1 through 8 play different tones.
 *   2. Tone plays while the button is held.
 *   3. Three modes: Play, Record, Playback.
 *   4. Three LEDs show the current mode.
 *   5. In Record Mode, notes are saved to external 24LC32A EEPROM.
 *   6. In Playback Mode, pressing 9 plays the saved sequence from EEPROM.
 *   7. Because the sequence is stored in EEPROM, it persists after power-off.
 *
 * Keypad wiring:
 *   keypad wire 1 -> D0 / PB7   Row 1
 *   keypad wire 2 -> D1 / PB6   Row 2
 *   keypad wire 3 -> D2 / PG14  Row 3
 *   keypad wire 4 -> D3 / PE13  Row 4
 *   keypad wire 5 -> D4 / PE14  Col 1
 *   keypad wire 6 -> D5 / PE11  Col 2
 *   keypad wire 7 -> D6 / PE9   Col 3
 *   keypad wire 8 -> D7 / PG12  Col 4
 *
 * Speaker wiring:
 *   D8 / PF3 -> 330 ohm resistor -> 8 ohm speaker -> GND
 *
 * LED wiring:
 *   Play LED     D9  / PD15 -> 330 ohm resistor -> LED -> GND
 *   Record LED   D10 / PD14 -> 330 ohm resistor -> LED -> GND
 *   Playback LED D11 / PB5  -> 330 ohm resistor -> LED -> GND
 *
 * EEPROM wiring:
 *   24LC32A pin 1 A0  -> GND
 *   24LC32A pin 2 A1  -> GND
 *   24LC32A pin 3 A2  -> GND
 *   24LC32A pin 4 VSS -> GND
 *   24LC32A pin 5 SDA -> D14 / PB9
 *   24LC32A pin 6 SCL -> D15 / PB8
 *   24LC32A pin 7 WP  -> GND
 *   24LC32A pin 8 VCC -> 3.3V
 */

/* RCC clock enable register */
#define RCC_AHB2ENR     (*(volatile uint32_t*)0x44020C8C)

/* GPIOB registers */
#define GPIOB_MODER     (*(volatile uint32_t*)0x42020400)
#define GPIOB_OTYPER    (*(volatile uint32_t*)0x42020404)
#define GPIOB_OSPEEDR   (*(volatile uint32_t*)0x42020408)
#define GPIOB_PUPDR     (*(volatile uint32_t*)0x4202040C)
#define GPIOB_IDR       (*(volatile uint32_t*)0x42020410)
#define GPIOB_BSRR      (*(volatile uint32_t*)0x42020418)

/* GPIOD registers */
#define GPIOD_MODER     (*(volatile uint32_t*)0x42020C00)
#define GPIOD_OTYPER    (*(volatile uint32_t*)0x42020C04)
#define GPIOD_OSPEEDR   (*(volatile uint32_t*)0x42020C08)
#define GPIOD_PUPDR     (*(volatile uint32_t*)0x42020C0C)
#define GPIOD_BSRR      (*(volatile uint32_t*)0x42020C18)

/* GPIOE registers */
#define GPIOE_MODER     (*(volatile uint32_t*)0x42021000)
#define GPIOE_OTYPER    (*(volatile uint32_t*)0x42021004)
#define GPIOE_OSPEEDR   (*(volatile uint32_t*)0x42021008)
#define GPIOE_PUPDR     (*(volatile uint32_t*)0x4202100C)
#define GPIOE_IDR       (*(volatile uint32_t*)0x42021010)
#define GPIOE_BSRR      (*(volatile uint32_t*)0x42021018)

/* GPIOF registers */
#define GPIOF_MODER     (*(volatile uint32_t*)0x42021400)
#define GPIOF_OTYPER    (*(volatile uint32_t*)0x42021404)
#define GPIOF_OSPEEDR   (*(volatile uint32_t*)0x42021408)
#define GPIOF_PUPDR     (*(volatile uint32_t*)0x4202140C)
#define GPIOF_BSRR      (*(volatile uint32_t*)0x42021418)

/* GPIOG registers */
#define GPIOG_MODER     (*(volatile uint32_t*)0x42021800)
#define GPIOG_OTYPER    (*(volatile uint32_t*)0x42021804)
#define GPIOG_OSPEEDR   (*(volatile uint32_t*)0x42021808)
#define GPIOG_PUPDR     (*(volatile uint32_t*)0x4202180C)
#define GPIOG_IDR       (*(volatile uint32_t*)0x42021810)
#define GPIOG_BSRR      (*(volatile uint32_t*)0x42021818)

/* GPIO clock enable bits */
#define GPIOB_EN        1
#define GPIOD_EN        3
#define GPIOE_EN        4
#define GPIOF_EN        5
#define GPIOG_EN        6

/* Keypad pins */
#define ROW0_PIN        7   /* PB7  / D0 */
#define ROW1_PIN        6   /* PB6  / D1 */
#define ROW2_PIN        14  /* PG14 / D2 */
#define ROW3_PIN        13  /* PE13 / D3 */

#define COL0_PIN        14  /* PE14 / D4 */
#define COL1_PIN        11  /* PE11 / D5 */
#define COL2_PIN        9   /* PE9  / D6 */
#define COL3_PIN        12  /* PG12 / D7 */

/* Speaker pin */
#define SPEAKER_PIN     3   /* PF3 / D8 */

/* LED pins */
#define PLAY_LED_PIN        15  /* PD15 / D9 */
#define RECORD_LED_PIN      14  /* PD14 / D10 */
#define PLAYBACK_LED_PIN    5   /* PB5  / D11 */

/* EEPROM I2C pins */
#define I2C_SCL_PIN     8   /* PB8 / D15 */
#define I2C_SDA_PIN     9   /* PB9 / D14 */

#define EEPROM_I2C_ADDR 0x50U
#define MAX_SEQUENCE_LENGTH 64U

/*
 * These are the three states of the whole system.
 * I use an enum so the code is easier to read than using numbers like 0, 1, 2.
 */
typedef enum
{
    MODE_PLAY,
    MODE_RECORD,
    MODE_PLAYBACK
} SystemMode;

/*
 * current_mode tells the program what the keypad should do right now.
 * record_count keeps track of how many notes have been saved in EEPROM.
 */
static SystemMode current_mode = MODE_PLAY;
static uint8_t record_count = 0;

/* ---------- Delay functions ----------
 *
 * These are simple busy-wait delays.
 * I use them for keypad debounce, tone timing, I2C timing, and LED blink signals.
 * This part is low-level setup, so I would not spend too much demo time here.
 */

static void delay(volatile uint32_t count)
{
    while (count > 0)
    {
        count--;
    }
}

static void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++)
    {
        delay(8000);
    }
}

static void delay_us(uint32_t us)
{
    for (uint32_t i = 0; i < us; i++)
    {
        delay(8);
    }
}

/* ---------- GPIO helper functions ----------
 *
 * These functions only make the register code cleaner.
 * Instead of writing GPIO registers everywhere, I call helper functions like gpiob_output()
 * or gpiob_write().
 * For the demo, I can quickly say these functions configure pins as input/output and read/write pins.
 */

static void gpiob_output(uint32_t pin)
{
    GPIOB_MODER &= ~(3U << (pin * 2));
    GPIOB_MODER |=  (1U << (pin * 2));
    GPIOB_OTYPER &= ~(1U << pin);
    GPIOB_OSPEEDR &= ~(3U << (pin * 2));
    GPIOB_OSPEEDR |=  (2U << (pin * 2));
    GPIOB_PUPDR &= ~(3U << (pin * 2));
}

static void gpiod_output(uint32_t pin)
{
    GPIOD_MODER &= ~(3U << (pin * 2));
    GPIOD_MODER |=  (1U << (pin * 2));
    GPIOD_OTYPER &= ~(1U << pin);
    GPIOD_OSPEEDR &= ~(3U << (pin * 2));
    GPIOD_OSPEEDR |=  (2U << (pin * 2));
    GPIOD_PUPDR &= ~(3U << (pin * 2));
}

static void gpioe_output(uint32_t pin)
{
    GPIOE_MODER &= ~(3U << (pin * 2));
    GPIOE_MODER |=  (1U << (pin * 2));
    GPIOE_OTYPER &= ~(1U << pin);
    GPIOE_OSPEEDR &= ~(3U << (pin * 2));
    GPIOE_OSPEEDR |=  (2U << (pin * 2));
    GPIOE_PUPDR &= ~(3U << (pin * 2));
}

static void gpiof_output(uint32_t pin)
{
    GPIOF_MODER &= ~(3U << (pin * 2));
    GPIOF_MODER |=  (1U << (pin * 2));
    GPIOF_OTYPER &= ~(1U << pin);
    GPIOF_OSPEEDR &= ~(3U << (pin * 2));
    GPIOF_OSPEEDR |=  (2U << (pin * 2));
    GPIOF_PUPDR &= ~(3U << (pin * 2));
}

static void gpiog_output(uint32_t pin)
{
    GPIOG_MODER &= ~(3U << (pin * 2));
    GPIOG_MODER |=  (1U << (pin * 2));
    GPIOG_OTYPER &= ~(1U << pin);
    GPIOG_OSPEEDR &= ~(3U << (pin * 2));
    GPIOG_OSPEEDR |=  (2U << (pin * 2));
    GPIOG_PUPDR &= ~(3U << (pin * 2));
}

static void gpiob_input_pullup(uint32_t pin)
{
    GPIOB_MODER &= ~(3U << (pin * 2));
    GPIOB_PUPDR &= ~(3U << (pin * 2));
    GPIOB_PUPDR |=  (1U << (pin * 2));
}

static void gpioe_input_pullup(uint32_t pin)
{
    GPIOE_MODER &= ~(3U << (pin * 2));
    GPIOE_PUPDR &= ~(3U << (pin * 2));
    GPIOE_PUPDR |=  (1U << (pin * 2));
}

static void gpiog_input_pullup(uint32_t pin)
{
    GPIOG_MODER &= ~(3U << (pin * 2));
    GPIOG_PUPDR &= ~(3U << (pin * 2));
    GPIOG_PUPDR |=  (1U << (pin * 2));
}

static void gpiob_write(uint32_t pin, uint32_t value)
{
    if (value)
    {
        GPIOB_BSRR = (1U << pin);
    }
    else
    {
        GPIOB_BSRR = (1U << (pin + 16));
    }
}

static void gpiod_write(uint32_t pin, uint32_t value)
{
    if (value)
    {
        GPIOD_BSRR = (1U << pin);
    }
    else
    {
        GPIOD_BSRR = (1U << (pin + 16));
    }
}

static void gpioe_write(uint32_t pin, uint32_t value)
{
    if (value)
    {
        GPIOE_BSRR = (1U << pin);
    }
    else
    {
        GPIOE_BSRR = (1U << (pin + 16));
    }
}

static void gpiof_write(uint32_t pin, uint32_t value)
{
    if (value)
    {
        GPIOF_BSRR = (1U << pin);
    }
    else
    {
        GPIOF_BSRR = (1U << (pin + 16));
    }
}

static void gpiog_write(uint32_t pin, uint32_t value)
{
    if (value)
    {
        GPIOG_BSRR = (1U << pin);
    }
    else
    {
        GPIOG_BSRR = (1U << (pin + 16));
    }
}

static uint32_t gpiob_read(uint32_t pin)
{
    return (GPIOB_IDR >> pin) & 1U;
}

static uint32_t gpioe_read(uint32_t pin)
{
    return (GPIOE_IDR >> pin) & 1U;
}

static uint32_t gpiog_read(uint32_t pin)
{
    return (GPIOG_IDR >> pin) & 1U;
}

/* ---------- Keypad functions ----------
 *
 * This is one of the most important parts.
 * The keypad is a 4x4 matrix, so I scan it row by row.
 * I pull one row low, then check which column becomes low.
 * The row and column together tell me which key is being pressed.
 */

static void keypad_all_rows_high(void)
{
    /*
     * Default state: all rows are high.
     * During scanning, I only pull one row low at a time.
     */
    gpiob_write(ROW0_PIN, 1U);
    gpiob_write(ROW1_PIN, 1U);
    gpiog_write(ROW2_PIN, 1U);
    gpioe_write(ROW3_PIN, 1U);
}

static void keypad_set_row_low(uint8_t row)
{
    /*
     * This selects one row for scanning.
     * Example: if row 0 is low and column 1 reads low,
     * then the pressed key is row 0, column 1, which is '2'.
     */
    keypad_all_rows_high();

    if (row == 0)
    {
        gpiob_write(ROW0_PIN, 0U);
    }
    else if (row == 1)
    {
        gpiob_write(ROW1_PIN, 0U);
    }
    else if (row == 2)
    {
        gpiog_write(ROW2_PIN, 0U);
    }
    else
    {
        gpioe_write(ROW3_PIN, 0U);
    }
}

static uint32_t keypad_read_col(uint8_t col)
{
    if (col == 0)
    {
        return gpioe_read(COL0_PIN);
    }
    else if (col == 1)
    {
        return gpioe_read(COL1_PIN);
    }
    else if (col == 2)
    {
        return gpioe_read(COL2_PIN);
    }
    else
    {
        return gpiog_read(COL3_PIN);
    }
}

static void keypad_gpio_init(void)
{
    RCC_AHB2ENR |= (1U << GPIOB_EN);
    RCC_AHB2ENR |= (1U << GPIOE_EN);
    RCC_AHB2ENR |= (1U << GPIOG_EN);

    delay_ms(1);

    gpiob_output(ROW0_PIN);
    gpiob_output(ROW1_PIN);
    gpiog_output(ROW2_PIN);
    gpioe_output(ROW3_PIN);

    gpioe_input_pullup(COL0_PIN);
    gpioe_input_pullup(COL1_PIN);
    gpioe_input_pullup(COL2_PIN);
    gpiog_input_pullup(COL3_PIN);

    keypad_all_rows_high();
}

static char keypad_scan_once(void)
{
    /*
     * This function checks the keypad one time.
     * If a key is pressed, it returns that character.
     * If no key is pressed, it returns '\0'.
     */
    static const char keys[4][4] =
    {
        { '1', '2', '3', 'A' },
        { '4', '5', '6', 'B' },
        { '7', '8', '9', 'C' },
        { '*', '0', '#', 'D' }
    };

    for (uint8_t row = 0; row < 4; row++)
    {
        keypad_set_row_low(row);
        delay_ms(1);

        for (uint8_t col = 0; col < 4; col++)
        {
            if (keypad_read_col(col) == 0U)
            {
                /*
                 * Because the columns use pull-up resistors,
                 * a pressed key reads as 0, not 1.
                 */
                keypad_all_rows_high();
                return keys[row][col];
            }
        }
    }

    keypad_all_rows_high();
    return '\0';
}

static void wait_until_key_released(void)
{
    while (keypad_scan_once() != '\0')
    {
        delay_ms(10);
    }

    delay_ms(30);
}

/* ---------- Speaker / tone functions ----------
 *
 * The speaker is controlled by a square wave.
 * I turn the speaker pin high and low repeatedly.
 * A shorter delay means a higher frequency, so it sounds like a higher note.
 */

static void speaker_gpio_init(void)
{
    RCC_AHB2ENR |= (1U << GPIOF_EN);
    delay_ms(1);

    gpiof_output(SPEAKER_PIN);
    gpiof_write(SPEAKER_PIN, 0U);
}

static void speaker_write(uint32_t value)
{
    gpiof_write(SPEAKER_PIN, value);
}

static uint8_t is_tone_key(char key)
{
    /*
     * Only keys 1 through 8 are real music notes in this project.
     */
    return key >= '1' && key <= '8';
}

static uint32_t tone_half_period_us(char key)
{
    /*
     * Each key maps to a different half-period.
     * The half-period controls the pitch of the tone.
     * Smaller half-period = faster square wave = higher pitch.
     */
    if (key == '1')
    {
        return 1908;
    }
    else if (key == '2')
    {
        return 1701;
    }
    else if (key == '3')
    {
        return 1515;
    }
    else if (key == '4')
    {
        return 1433;
    }
    else if (key == '5')
    {
        return 1276;
    }
    else if (key == '6')
    {
        return 1136;
    }
    else if (key == '7')
    {
        return 1012;
    }
    else if (key == '8')
    {
        return 956;
    }

    return 0;
}

static void play_tone_cycles(char key, uint32_t cycles)
{
    /*
     * This is the basic sound generator.
     * One cycle means high for half a period, then low for half a period.
     * Repeating many cycles creates an audible tone.
     */
    uint32_t half_period = tone_half_period_us(key);

    if (half_period == 0U)
    {
        speaker_write(0U);
        return;
    }

    for (uint32_t i = 0; i < cycles; i++)
    {
        speaker_write(1U);
        delay_us(half_period);

        speaker_write(0U);
        delay_us(half_period);
    }
}

static void play_tone_for_ms(char key, uint32_t ms)
{
    uint32_t half_period = tone_half_period_us(key);

    if (half_period == 0U)
    {
        speaker_write(0U);
        return;
    }

    uint32_t cycles = (ms * 1000U) / (2U * half_period);

    if (cycles == 0U)
    {
        cycles = 1U;
    }

    play_tone_cycles(key, cycles);
    speaker_write(0U);
}

static void play_tone_while_pressed(char key)
{
    /*
     * This makes the note play only while the user is holding the key.
     * When the key is released, the loop stops and the speaker turns off.
     */
    while (keypad_scan_once() == key)
    {
        play_tone_cycles(key, 20U);
    }

    speaker_write(0U);
}

/* ---------- Mode LED functions ----------
 *
 * These functions are mainly for user feedback.
 * One LED turns on for Play Mode, one for Record Mode, and one for Playback Mode.
 */

static void led_gpio_init(void)
{
    RCC_AHB2ENR |= (1U << GPIOB_EN);
    RCC_AHB2ENR |= (1U << GPIOD_EN);

    delay_ms(1);

    gpiod_output(PLAY_LED_PIN);
    gpiod_output(RECORD_LED_PIN);
    gpiob_output(PLAYBACK_LED_PIN);

    gpiod_write(PLAY_LED_PIN, 0U);
    gpiod_write(RECORD_LED_PIN, 0U);
    gpiob_write(PLAYBACK_LED_PIN, 0U);
}

static void all_leds_off(void)
{
    gpiod_write(PLAY_LED_PIN, 0U);
    gpiod_write(RECORD_LED_PIN, 0U);
    gpiob_write(PLAYBACK_LED_PIN, 0U);
}

static void all_leds_on(void)
{
    gpiod_write(PLAY_LED_PIN, 1U);
    gpiod_write(RECORD_LED_PIN, 1U);
    gpiob_write(PLAYBACK_LED_PIN, 1U);
}

static void update_mode_leds(void)
{
    /*
     * This function makes the LEDs match the current system mode.
     */
    if (current_mode == MODE_PLAY)
    {
        gpiod_write(PLAY_LED_PIN, 1U);
        gpiod_write(RECORD_LED_PIN, 0U);
        gpiob_write(PLAYBACK_LED_PIN, 0U);
    }
    else if (current_mode == MODE_RECORD)
    {
        gpiod_write(PLAY_LED_PIN, 0U);
        gpiod_write(RECORD_LED_PIN, 1U);
        gpiob_write(PLAYBACK_LED_PIN, 0U);
    }
    else
    {
        gpiod_write(PLAY_LED_PIN, 0U);
        gpiod_write(RECORD_LED_PIN, 0U);
        gpiob_write(PLAYBACK_LED_PIN, 1U);
    }
}

static void set_mode(SystemMode new_mode)
{
    /*
     * Changing mode is only two steps:
     * 1. update the software state
     * 2. update the LEDs so the user can see the state
     */
    current_mode = new_mode;
    update_mode_leds();
}

static void error_signal(void)
{
    for (uint8_t i = 0; i < 6; i++)
    {
        all_leds_on();
        delay_ms(100);
        all_leds_off();
        delay_ms(100);
    }

    update_mode_leds();
}

static void empty_sequence_signal(void)
{
    for (uint8_t i = 0; i < 3; i++)
    {
        gpiob_write(PLAYBACK_LED_PIN, 0U);
        delay_ms(120);
        gpiob_write(PLAYBACK_LED_PIN, 1U);
        delay_ms(120);
    }

    update_mode_leds();
}

/* ---------- Bit-banged I2C functions ----------
 *
 * This is software I2C.
 * Instead of using the hardware I2C peripheral, I manually control SDA and SCL with GPIO.
 * For the demo, the main point is:
 *   SDA carries the data, SCL is the clock.
 * The EEPROM communicates through this I2C protocol.
 */

static void i2c_delay(void)
{
    delay_us(50);
}

static void sda_release_high(void)
{
    gpiob_input_pullup(I2C_SDA_PIN);
}

static void scl_release_high(void)
{
    gpiob_input_pullup(I2C_SCL_PIN);
}

static void sda_drive_low(void)
{
    gpiob_write(I2C_SDA_PIN, 0U);
    gpiob_output(I2C_SDA_PIN);
}

static void scl_drive_low(void)
{
    gpiob_write(I2C_SCL_PIN, 0U);
    gpiob_output(I2C_SCL_PIN);
}

static uint32_t sda_read(void)
{
    return gpiob_read(I2C_SDA_PIN);
}

static void i2c_gpio_init(void)
{
    RCC_AHB2ENR |= (1U << GPIOB_EN);
    delay_ms(1);

    sda_release_high();
    scl_release_high();
    i2c_delay();
}

static void i2c_start(void)
{
    /*
     * I2C start condition:
     * SDA goes low while SCL is high.
     */
    sda_release_high();
    scl_release_high();
    i2c_delay();

    sda_drive_low();
    i2c_delay();

    scl_drive_low();
    i2c_delay();
}

static void i2c_stop(void)
{
    /*
     * I2C stop condition:
     * SDA goes high while SCL is high.
     */
    sda_drive_low();
    i2c_delay();

    scl_release_high();
    i2c_delay();

    sda_release_high();
    i2c_delay();
}

static void i2c_write_bit(uint8_t bit_value)
{
    if (bit_value)
    {
        sda_release_high();
    }
    else
    {
        sda_drive_low();
    }

    i2c_delay();

    scl_release_high();
    i2c_delay();

    scl_drive_low();
    i2c_delay();
}

static uint8_t i2c_read_bit(void)
{
    uint8_t bit_value;

    sda_release_high();
    i2c_delay();

    scl_release_high();
    i2c_delay();

    bit_value = (uint8_t)sda_read();

    scl_drive_low();
    i2c_delay();

    return bit_value;
}

static uint8_t i2c_write_byte(uint8_t value)
{
    /*
     * I send one byte from the most significant bit to the least significant bit.
     * After sending 8 bits, the EEPROM sends ACK.
     * ACK is 0, so this function returns true when the read bit is 0.
     */
    for (int8_t bit = 7; bit >= 0; bit--)
    {
        i2c_write_bit((value >> bit) & 1U);
    }

    return i2c_read_bit() == 0U;
}

static uint8_t i2c_read_byte(uint8_t send_ack)
{
    uint8_t value = 0;

    for (uint8_t bit = 0; bit < 8; bit++)
    {
        value <<= 1;
        value |= i2c_read_bit();
    }

    if (send_ack)
    {
        i2c_write_bit(0U);
    }
    else
    {
        i2c_write_bit(1U);
    }

    return value;
}

/* ---------- EEPROM functions ----------
 *
 * Demo note:
 * EEPROM is the memory chip.
 * I use address 0 to store the number of recorded notes.
 * I use address 1, 2, 3, ... to store the actual note keys.
 * Example:
 *   address 0 = 3
 *   address 1 = '1'
 *   address 2 = '5'
 *   address 3 = '8'
 */

static uint8_t eeprom_write_byte(uint16_t address, uint8_t data)
{
    uint8_t ok = 1U;

    i2c_start();

    ok &= i2c_write_byte((uint8_t)((EEPROM_I2C_ADDR << 1) | 0U));
    ok &= i2c_write_byte((uint8_t)((address >> 8) & 0xFFU));
    ok &= i2c_write_byte((uint8_t)(address & 0xFFU));
    ok &= i2c_write_byte(data);

    i2c_stop();

    delay_ms(10);

    return ok;
}

static uint8_t eeprom_read_byte(uint16_t address, uint8_t *data)
{
    uint8_t ok = 1U;

    i2c_start();

    ok &= i2c_write_byte((uint8_t)((EEPROM_I2C_ADDR << 1) | 0U));
    ok &= i2c_write_byte((uint8_t)((address >> 8) & 0xFFU));
    ok &= i2c_write_byte((uint8_t)(address & 0xFFU));

    i2c_start();

    ok &= i2c_write_byte((uint8_t)((EEPROM_I2C_ADDR << 1) | 1U));
    *data = i2c_read_byte(0U);

    i2c_stop();

    return ok;
}

static uint8_t clear_recording(void)
{
    /*
     * When I enter Record Mode, I clear the old recording.
     * I reset the local count and also write 0 to EEPROM address 0.
     */
    record_count = 0;
    return eeprom_write_byte(0, 0);
}

static uint8_t record_note_to_eeprom(char key)
{
    /*
     * This saves one note into EEPROM.
     * The note itself is stored at address record_count + 1.
     * Then address 0 is updated with the new total length.
     */
    if (record_count >= MAX_SEQUENCE_LENGTH)
    {
        return 0U;
    }

    uint16_t note_address = (uint16_t)record_count + 1U;

    if (!eeprom_write_byte(note_address, (uint8_t)key))
    {
        return 0U;
    }

    record_count++;

    if (!eeprom_write_byte(0, record_count))
    {
        return 0U;
    }

    return 1U;
}

static void playback_sequence(void)
{
    /*
     * Playback logic:
     * 1. Read the saved length from EEPROM address 0.
     * 2. Read each saved note from address 1 onward.
     * 3. Play each note for 500 ms.
     */
    uint8_t length = 0;

    if (!eeprom_read_byte(0, &length))
    {
        error_signal();
        return;
    }

    if (length == 0U || length > MAX_SEQUENCE_LENGTH)
    {
        empty_sequence_signal();
        return;
    }

    for (uint8_t i = 0; i < length; i++)
    {
        uint8_t note = 0;

        if (!eeprom_read_byte((uint16_t)i + 1U, &note))
        {
            error_signal();
            return;
        }

        if (note >= '1' && note <= '8')
        {
            play_tone_for_ms((char)note, 500);
            delay_ms(100);
        }
    }

    speaker_write(0U);
}

/* ---------- Input handling ----------
 *
 * Demo note:
 * This is the main control logic of the program.
 * handle_key() decides what to do based on:
 *   1. which key was pressed
 *   2. which mode the system is currently in
 */

static void handle_mode_key(char key)
{
    /*
     * Special keys control the mode:
     * '*' means Play Mode.
     * '0' means Record Mode and starts a new recording.
     * '#' means Playback Mode.
     */
    if (key == '*')
    {
        set_mode(MODE_PLAY);
        wait_until_key_released();
    }
    else if (key == '0')
    {
        set_mode(MODE_RECORD);

        if (!clear_recording())
        {
            error_signal();
        }

        wait_until_key_released();
    }
    else if (key == '#')
    {
        set_mode(MODE_PLAYBACK);
        wait_until_key_released();
    }
}

static void handle_play_mode_key(char key)
{
    /*
     * In Play Mode, the system only plays the note.
     * It does not save anything.
     */
    if (is_tone_key(key))
    {
        delay_ms(20);

        if (keypad_scan_once() == key)
        {
            play_tone_while_pressed(key);
        }
    }
}

static void handle_record_mode_key(char key)
{
    /*
     * In Record Mode, the system plays the note first.
     * After the key is released, it saves that note into EEPROM.
     */
    if (is_tone_key(key))
    {
        delay_ms(20);

        if (keypad_scan_once() == key)
        {
            play_tone_while_pressed(key);

            if (!record_note_to_eeprom(key))
            {
                error_signal();
            }
        }
    }
}

static void handle_playback_mode_key(char key)
{
    /*
     * In Playback Mode, key 9 triggers playback.
     * Other keys do nothing in this mode.
     */
    if (key == '9')
    {
        delay_ms(20);

        if (keypad_scan_once() == key)
        {
            wait_until_key_released();
            playback_sequence();
        }
    }
}

static void handle_key(char key)
{
    /*
     * This function is the center of the program.
     * It receives one scanned key and decides what action should happen.
     */
    if (key == '\0')
    {
        /* No key pressed: make sure the speaker is off. */
        speaker_write(0U);
        return;
    }

    if (key == '*' || key == '0' || key == '#')
    {
        /*
         * These are mode keys, so I handle them before normal note keys.
         * The small delay is for debounce, so one press is not read multiple times.
         */
        delay_ms(20);

        if (keypad_scan_once() == key)
        {
            handle_mode_key(key);
        }

        return;
    }

    if (current_mode == MODE_PLAY)
    {
        handle_play_mode_key(key);
    }
    else if (current_mode == MODE_RECORD)
    {
        handle_record_mode_key(key);
    }
    else
    {
        handle_playback_mode_key(key);
    }
}

/* ---------- Main program ----------
 *
 * Demo note:
 * The main function is intentionally simple.
 * First, initialize the hardware modules.
 * Then, start in Play Mode.
 * Finally, repeatedly scan the keypad and pass the key into handle_key().
 */

int main(void)
{
    /*
     * Setup part:
     * I initialize the keypad, speaker, LEDs, and EEPROM communication.
     * In the demo, I can mention this quickly and focus more on the loop below.
     */
    keypad_gpio_init();
    speaker_gpio_init();
    led_gpio_init();
    i2c_gpio_init();

    /*
     * The system starts in Play Mode by default.
     * The Play LED turns on here.
     */
    set_mode(MODE_PLAY);

    while (1)
    {
        /*
         * Main loop:
         * 1. Scan the keypad once.
         * 2. Send the result to handle_key().
         * 3. handle_key() decides play, record, playback, or mode change.
         */
        char key = keypad_scan_once();
        handle_key(key);

        delay_ms(5);
    }
}
