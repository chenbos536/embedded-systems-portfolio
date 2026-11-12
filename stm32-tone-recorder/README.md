# STM32 Tone Player and Recorder

Bare-metal C firmware for the NUCLEO-H563ZI. A matrix keypad selects notes and operating modes. GPIO drives the audio output and mode LEDs. A software I2C implementation saves note sequences to external 24LC32A EEPROM for later playback.

## Source and setup

`Src/main.c` contains the firmware and a pin-by-pin wiring reference. Create an empty STM32CubeIDE project for the NUCLEO-H563ZI with the board's startup and linker files, then use this file as the project's application entry point. Build and flash from STM32CubeIDE.

## Hardware verification

1. In play mode, press a note key and check audio output.
2. In record mode, enter a short sequence.
3. Power-cycle the board, select playback mode, and press 9 to check that the sequence persisted.

The source is included here for review. The build and board checks above have not been rerun for this portfolio copy.
