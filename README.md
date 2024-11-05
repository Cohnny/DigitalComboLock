# Arduino Lock System with EEPROM, Servo Motor, and LCD Display

This project is an Arduino-based digital lock system that uses a keypad, servo motor, EEPROM memory, and an LCD display. It allows users to lock and unlock a secure system using a 4-digit code. Users can also update the code, which is saved in EEPROM to retain it even after power loss.

## Features

- **Lock and Unlock**: Secure system with a servo motor that locks/unlocks based on user input.
- **EEPROM Code Storage**: Saves the lock code in EEPROM, allowing for persistent storage across power cycles.
- **LCD Feedback**: An LCD display provides status feedback such as "LOCKED", "UNLOCKED", or "CODE CHANGE".
- **LED Indicators**: Uses LEDs to show lock status: Red (locked), Green (unlocked), Yellow (code change).
- **Code Update Functionality**: Allows users to change the 4-digit lock code after verifying the existing code.

## Hardware Components

- **Arduino Board**: Compatible with most Arduino models (Uno, Nano, Mega, etc.).
- **Servo Motor**: Controls the physical lock/unlock mechanism.
- **4x4 Keypad or 5 Push Buttons**: For entering the lock code and initiating code changes.
- **Liquid Crystal Display (LCD) with I2C**: Displays lock status messages.
- **EEPROM**: Used to store the lock code securely.
- **LEDs (Red, Green, Yellow)**: Provides visual indicators for the system state.

## Pin Configuration

- **Servo**: Pin 9
- **Red LED**: Pin 11
- **Green LED**: Pin 12
- **Yellow LED**: Pin 13
- **Buttons**: Pins 2, 3, 4, 5, 6 (for code entry and code change)
- **LCD (I2C)**: Configured with address `0x20`, 16 columns, and 2 lines

## Setup Instructions

1. **Connect Hardware**: Connect the servo motor, LEDs, buttons, and LCD display to the Arduino as specified in the overview.
2. **Load Code**: Upload the code from `lock_system.ino` to your Arduino board.
3. **Initial Code**: The default code is `1,2,3,4`. Change this by following the steps for updating the code.
4. **Library Requirements**:
   - Install the `Servo` library.
   - Install the `EEPROM` library (built into Arduino IDE).
   - Install the `LiquidCrystal_I2C` library for LCD handling.

## Usage

1. **Locking and Unlocking**:
   - Enter the 4-digit code using the buttons. If correct, the lock will open for 5 seconds before automatically locking again.
2. **Updating the Code**:
   - Press the last button to initiate a code change.
   - Enter the new 4-digit code twice to confirm it. The code will then be saved to EEPROM.
3. **Status Feedback**:
   - LCD shows lock status (LOCKED/UNLOCKED).
   - Red LED indicates a locked state, Green for unlocked, and Yellow indicates code change mode.

## Code Structure

- **`setup()`**: Initializes components and checks EEPROM for a saved code.
- **`loop()`**: Runs the state machine for locked, unlocked, and update code states.
- **State Functions**:
   - `lockedState()`: Handles the locked state, waiting for the correct code.
   - `unlockedState()`: Opens the lock, starts a timer, and automatically re-locks after 5 seconds.
   - `updateCodeState()`: Manages the code change process, including verification.
- **EEPROM Functions**:
   - `loadCode()` and `saveCode()` handle reading and writing the code to EEPROM.
- **Button Handling**:
   - `buttonAction()`: Manages button debouncing and adds button input to the correct array.

## Customization

- **Change Default Code**: Modify `FACTORY_CODE` in the code if you'd like a different default code.
- **Adjust Servo Angle**: Change `pos` values in `rotateServo()` if needed for your specific locking mechanism.
- **Debounce & Timer**: Adjust `DEBOUNCE_DELAY` and `OPEN_TIME` constants as needed for your project.

## License

This project is open-source and available under the MIT License. 

Feel free to modify and adapt it for your own purposes.