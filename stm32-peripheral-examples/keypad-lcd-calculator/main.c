#include <stdint.h>

/*
 * Simple keypad + LCD calculator for NUCLEO-H563ZI.
 *
 * Demo summary:
 *   This program uses a keypad and a 16x2 LCD to make a simple multiplication
 *   calculator. The calculator only accepts digits, '*', and '#'.
 *
 * User flow:
 *   1. Type the first number A.
 *   2. Press '*' to move to the second number B.
 *   3. Type the second number B.
 *   4. Press '*' again to display A * B on the bottom LCD row.
 *   5. Press '#' at any time to clear everything and restart.
 *
 * LCD wiring:
 *   LCD pin 1  GND  -> breadboard GND
 *   LCD pin 2  VDD  -> 5V
 *   LCD pin 3  VO   -> potentiometer middle pin
 *   LCD pin 4  RS   -> D56 / PE2
 *   LCD pin 5  R/W  -> GND
 *   LCD pin 6  E    -> D57 / PE4
 *   LCD pin 7  DB0  -> not connected
 *   LCD pin 8  DB1  -> not connected
 *   LCD pin 9  DB2  -> not connected
 *   LCD pin 10 DB3  -> not connected
 *   LCD pin 11 DB4  -> D58 / PE5
 *   LCD pin 12 DB5  -> D59 / PE6
 *   LCD pin 13 DB6  -> D60 / PE3
 *   LCD pin 14 DB7  -> D61 / PF8
 *
 * Keypad wiring, reversed version:
 *   keypad wire 1 -> D0 / PB7
 *   keypad wire 2 -> D1 / PB6
 *   keypad wire 3 -> D2 / PG14
 *   keypad wire 4 -> D3 / PE13
 *   keypad wire 5 -> D4 / PE14
 *   keypad wire 6 -> D5 / PE11
 *   keypad wire 7 -> D6 / PE9
 *   keypad wire 8 -> D7 / PG12
 */

/* RCC clock enable register */
#define RCC_AHB2ENR     (*(volatile uint32_t*)0x44020C8C)

/* GPIOB registers */
#define GPIOB_MODER     (*(volatile uint32_t*)0x42020400)
#define GPIOB_OTYPER    (*(volatile uint32_t*)0x42020404)
#define GPIOB_OSPEEDR   (*(volatile uint32_t*)0x42020408)
#define GPIOB_PUPDR     (*(volatile uint32_t*)0x4202040C)
#define GPIOB_BSRR      (*(volatile uint32_t*)0x42020418)

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
#define GPIOE_EN        4
#define GPIOF_EN        5
#define GPIOG_EN        6

/* LCD pins */
#define LCD_RS_PIN      2   /* PE2  / D56 */
#define LCD_E_PIN       4   /* PE4  / D57 */
#define LCD_D4_PIN      5   /* PE5  / D58 */
#define LCD_D5_PIN      6   /* PE6  / D59 */
#define LCD_D6_PIN      3   /* PE3  / D60 */
#define LCD_D7_PIN      8   /* PF8  / D61 */

/*
 * Reversed keypad wiring:
 *
 * Rows are connected to D0-D3.
 * Columns are connected to D4-D7.
 *
 * During scanning, one row is pulled LOW at a time.
 * If a button in that row is pressed, the matching column also reads LOW.
 */
#define ROW0_PIN        7   /* PB7  / D0 */
#define ROW1_PIN        6   /* PB6  / D1 */
#define ROW2_PIN        14  /* PG14 / D2 */
#define ROW3_PIN        13  /* PE13 / D3 */

#define COL0_PIN        14  /* PE14 / D4 */
#define COL1_PIN        11  /* PE11 / D5 */
#define COL2_PIN        9   /* PE9  / D6 */
#define COL3_PIN        12  /* PG12 / D7 */

/*
 * Calculator state machine.
 *
 * INITIAL_STATE:
 *   Clear everything and reset A/B.
 *
 * START_STATE:
 *   Prepare for a new calculation.
 *
 * A_STATE:
 *   User is typing number A.
 *
 * B_STATE:
 *   User is typing number B.
 *
 * DISPLAY_STATE:
 *   Compute and display A * B.
 */
typedef enum
{
    INITIAL_STATE,
    START_STATE,
    A_STATE,
    B_STATE,
    DISPLAY_STATE
} CalculatorState;

static CalculatorState state = INITIAL_STATE;

/*
 * A and B are the two numbers being multiplied.
 * digitsA and digitsB enforce the assignment's 8-digit limit.
 */
static uint32_t A = 0;
static uint32_t B = 0;
static uint8_t digitsA = 0;
static uint8_t digitsB = 0;

/* ---------- Delay functions ---------- */

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

/* ---------- GPIO helper functions ---------- */

static void gpiob_output(uint32_t pin)
{
    GPIOB_MODER &= ~(3U << (pin * 2));
    GPIOB_MODER |=  (1U << (pin * 2));
    GPIOB_OTYPER &= ~(1U << pin);
    GPIOB_OSPEEDR &= ~(3U << (pin * 2));
    GPIOB_OSPEEDR |=  (2U << (pin * 2));
    GPIOB_PUPDR &= ~(3U << (pin * 2));
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

static uint32_t gpioe_read(uint32_t pin)
{
    return (GPIOE_IDR >> pin) & 1U;
}

static uint32_t gpiog_read(uint32_t pin)
{
    return (GPIOG_IDR >> pin) & 1U;
}

/* ---------- LCD functions ---------- */

static void lcd_rs_write(uint32_t value)
{
    gpioe_write(LCD_RS_PIN, value);
}

static void lcd_e_write(uint32_t value)
{
    gpioe_write(LCD_E_PIN, value);
}

static void lcd_d4_write(uint32_t value)
{
    gpioe_write(LCD_D4_PIN, value);
}

static void lcd_d5_write(uint32_t value)
{
    gpioe_write(LCD_D5_PIN, value);
}

static void lcd_d6_write(uint32_t value)
{
    gpioe_write(LCD_D6_PIN, value);
}

static void lcd_d7_write(uint32_t value)
{
    gpiof_write(LCD_D7_PIN, value);
}

/*
 * Sends 4 bits to LCD DB4-DB7.
 * Since the LCD uses 4-bit mode, each byte is split into two nibbles.
 */
static void lcd_set_nibble(uint8_t nibble)
{
    lcd_d4_write((nibble >> 0) & 1U);
    lcd_d5_write((nibble >> 1) & 1U);
    lcd_d6_write((nibble >> 2) & 1U);
    lcd_d7_write((nibble >> 3) & 1U);
}

/*
 * Enable pulse:
 * The LCD reads RS and DB4-DB7 when E is pulsed.
 */
static void lcd_pulse_enable(void)
{
    lcd_e_write(1U);
    delay_us(20);
    lcd_e_write(0U);
    delay_us(200);
}

static void lcd_write_nibble(uint8_t nibble)
{
    lcd_set_nibble(nibble);
    lcd_pulse_enable();
}

/*
 * rs = 0: command mode.
 * rs = 1: character data mode.
 */
static void lcd_write_byte(uint8_t value, uint8_t rs)
{
    lcd_rs_write(rs);
    lcd_write_nibble((value >> 4) & 0x0F);
    lcd_write_nibble(value & 0x0F);
}

static void lcd_command(uint8_t command)
{
    lcd_write_byte(command, 0U);

    if (command == 0x01U || command == 0x02U)
    {
        delay_ms(3);
    }
    else
    {
        delay_ms(1);
    }
}

static void lcd_data(uint8_t data)
{
    lcd_write_byte(data, 1U);
    delay_ms(1);
}

static void lcd_print(const char *text)
{
    while (*text != '\0')
    {
        lcd_data((uint8_t)*text);
        text++;
    }
}

/*
 * LCD row addresses:
 *   row 0 starts at 0x00
 *   row 1 starts at 0x40
 */
static void lcd_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t address;

    if (row == 0)
    {
        address = 0x00 + col;
    }
    else
    {
        address = 0x40 + col;
    }

    lcd_command(0x80 | address);
}

static void lcd_clear(void)
{
    lcd_command(0x01);
    delay_ms(3);
}

/*
 * There is no direct "clear one row" LCD command.
 * So this function overwrites one row with spaces.
 */
static void lcd_clear_row(uint8_t row)
{
    lcd_set_cursor(row, 0);

    for (uint8_t i = 0; i < 16; i++)
    {
        lcd_data(' ');
    }

    lcd_set_cursor(row, 0);
}

/*
 * Prints a 64-bit unsigned number.
 * This is used because A and B can each be up to 8 digits,
 * so A * B may be larger than a 32-bit integer.
 */
static void lcd_print_uint64(uint64_t value)
{
    char buffer[21];
    uint8_t index = 0;

    if (value == 0)
    {
        lcd_data('0');
        return;
    }

    while (value > 0 && index < sizeof(buffer))
    {
        buffer[index] = (char)('0' + (value % 10));
        value = value / 10;
        index++;
    }

    while (index > 0)
    {
        index--;
        lcd_data((uint8_t)buffer[index]);
    }
}

static void lcd_gpio_init(void)
{
    RCC_AHB2ENR |= (1U << GPIOE_EN);
    RCC_AHB2ENR |= (1U << GPIOF_EN);

    delay_ms(1);

    gpioe_output(LCD_RS_PIN);
    gpioe_output(LCD_E_PIN);
    gpioe_output(LCD_D4_PIN);
    gpioe_output(LCD_D5_PIN);
    gpioe_output(LCD_D6_PIN);
    gpiof_output(LCD_D7_PIN);

    lcd_rs_write(0U);
    lcd_e_write(0U);
    lcd_d4_write(0U);
    lcd_d5_write(0U);
    lcd_d6_write(0U);
    lcd_d7_write(0U);
}

/*
 * Standard HD44780 LCD initialization sequence for 4-bit mode.
 */
static void lcd_init(void)
{
    delay_ms(50);

    lcd_rs_write(0U);
    lcd_e_write(0U);

    lcd_write_nibble(0x03);
    delay_ms(5);

    lcd_write_nibble(0x03);
    delay_ms(5);

    lcd_write_nibble(0x03);
    delay_ms(5);

    lcd_write_nibble(0x02);
    delay_ms(5);

    lcd_command(0x28);  /* 4-bit mode, 2-line display */
    lcd_command(0x0C);  /* display on, cursor off */
    lcd_command(0x06);  /* cursor moves right after each character */
    lcd_clear();
}

/* ---------- Keypad functions ---------- */

/*
 * Set all keypad rows high.
 * A pressed key is detected only when its row is pulled low.
 */
static void keypad_all_rows_high(void)
{
    gpiob_write(ROW0_PIN, 1U);
    gpiob_write(ROW1_PIN, 1U);
    gpiog_write(ROW2_PIN, 1U);
    gpioe_write(ROW3_PIN, 1U);
}

/*
 * Pull exactly one row low.
 * Then the program checks which column becomes low.
 */
static void keypad_set_row_low(uint8_t row)
{
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

/*
 * Read one keypad column.
 * Columns are input pull-up:
 *   1 = not pressed
 *   0 = pressed
 */
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

    /*
     * Rows are outputs.
     * Columns are inputs with internal pull-up.
     */
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

/*
 * Scan the keypad once.
 *
 * The program checks one row at a time:
 *   1. Pull row low.
 *   2. Read each column.
 *   3. If a column reads 0, that key is pressed.
 *
 * A, B, C, and D are still detected here, but the calculator later ignores them.
 */
static char keypad_scan_once(void)
{
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
                keypad_all_rows_high();
                return keys[row][col];
            }
        }
    }

    keypad_all_rows_high();
    return '\0';
}

/*
 * Debounced key read.
 *
 * Without this, one physical press might be counted many times.
 * This function waits until the key is released before returning.
 */
static char keypad_get_key(void)
{
    char key1 = keypad_scan_once();

    if (key1 == '\0')
    {
        return '\0';
    }

    delay_ms(30);

    char key2 = keypad_scan_once();

    if (key1 != key2)
    {
        return '\0';
    }

    while (keypad_scan_once() != '\0')
    {
        delay_ms(10);
    }

    delay_ms(30);

    return key1;
}

/* ---------- Calculator functions ---------- */

static uint8_t is_digit(char key)
{
    return key >= '0' && key <= '9';
}

/*
 * Initial State:
 *   - Happens at power-up or when '#' is pressed.
 *   - Clears both LCD rows.
 *   - Resets A and B.
 *   - Then moves to Start State.
 */
static void enter_initial_state(void)
{
    A = 0;
    B = 0;
    digitsA = 0;
    digitsB = 0;

    lcd_clear();

    state = START_STATE;
}

/*
 * Start State:
 *   - Prepares a new calculation.
 *   - Resets A and B.
 *   - Clears the top row.
 *   - Does NOT clear the bottom row, so the old answer can remain visible.
 *   - Then moves to A State.
 */
static void enter_start_state(void)
{
    A = 0;
    B = 0;
    digitsA = 0;
    digitsB = 0;

    lcd_clear_row(0);
    lcd_set_cursor(0, 0);

    state = A_STATE;
}

/*
 * B State setup:
 *   - Clears the top row after A is finished.
 *   - Then B is typed on the top row.
 */
static void enter_b_state(void)
{
    lcd_clear_row(0);
    lcd_set_cursor(0, 0);

    state = B_STATE;
}

/*
 * Display State:
 *   - Computes A * B.
 *   - Displays the result on the bottom row.
 *   - Then goes back to Start State for the next calculation.
 */
static void enter_display_state(void)
{
    uint64_t product = (uint64_t)A * (uint64_t)B;

    lcd_clear_row(1);
    lcd_set_cursor(1, 0);
    lcd_print_uint64(product);

    state = START_STATE;
}

/*
 * Main input logic:
 *
 * '#' always resets immediately.
 *
 * In A_STATE:
 *   - digit: append to A
 *   - '*': switch to B_STATE
 *   - 8 digits: automatically switch to B_STATE
 *
 * In B_STATE:
 *   - digit: append to B
 *   - '*': switch to DISPLAY_STATE
 *   - 8 digits: automatically switch to DISPLAY_STATE
 *
 * A/B/C/D are ignored because the assignment says only digits, '*', and '#'
 * should matter.
 */
static void handle_key(char key)
{
    if (key == '\0')
    {
        return;
    }

    if (key == '#')
    {
        enter_initial_state();
        return;
    }

    if (state == A_STATE)
    {
        if (is_digit(key))
        {
            A = A * 10U + (uint32_t)(key - '0');
            digitsA++;

            lcd_data((uint8_t)key);

            if (digitsA >= 8U)
            {
                enter_b_state();
            }
        }
        else if (key == '*')
        {
            enter_b_state();
        }
        else
        {
            /* Ignore A, B, C, D. */
        }
    }
    else if (state == B_STATE)
    {
        if (is_digit(key))
        {
            B = B * 10U + (uint32_t)(key - '0');
            digitsB++;

            lcd_data((uint8_t)key);

            if (digitsB >= 8U)
            {
                state = DISPLAY_STATE;
            }
        }
        else if (key == '*')
        {
            state = DISPLAY_STATE;
        }
        else
        {
            /* Ignore A, B, C, D. */
        }
    }
}

/*
 * Main program:
 *   1. Initialize LCD GPIO.
 *   2. Initialize keypad GPIO.
 *   3. Initialize LCD controller.
 *   4. Run the calculator state machine forever.
 */
int main(void)
{
    lcd_gpio_init();
    keypad_gpio_init();
    lcd_init();

    state = INITIAL_STATE;

    while (1)
    {
        if (state == INITIAL_STATE)
        {
            enter_initial_state();
        }
        else if (state == START_STATE)
        {
            enter_start_state();
        }
        else if (state == DISPLAY_STATE)
        {
            enter_display_state();
        }
        else
        {
            char key = keypad_get_key();
            handle_key(key);
        }
    }
}
