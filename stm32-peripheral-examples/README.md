# STM32 Peripheral Examples

Three standalone bare-metal C examples for the NUCLEO-H563ZI.

| Example | What it demonstrates |
| --- | --- |
| [Keypad and LCD calculator](keypad-lcd-calculator/main.c) | Scans a matrix keypad and drives a 16×2 LCD in 4-bit mode. Enter two numbers separated by `*`, press `*` again to multiply, and use `#` to clear. |
| [ADC-controlled PWM LED](adc-pwm-led/main.c) | Reads a potentiometer on the ADC and uses TIM3 PWM to drive a breathing LED. |
| [Timer interrupt and keypad](timer-interrupt-keypad/main.c) | Configures TIM2 and the NVIC for a periodic LED update while the foreground code scans keypad input. |

Each example is a separate firmware image. Use its `main.c` as the application entry point in an empty STM32CubeIDE project for the NUCLEO-H563ZI. The source comments describe the pin assignments and behavior. Start with the board's standard startup and linker files.

All three sources passed an ARM GCC syntax check for Cortex-M33. Linking, flashing, and board behavior were outside that check.
