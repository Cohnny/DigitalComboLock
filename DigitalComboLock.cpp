#include <Servo.h>		// Include the Servo library, for controlling the servo motor 
#include <EEPROM.h>		// Include the EEPROM library for reading and writing to EEPROM memory
#include <LiquidCrystal_I2C.h>

// Define the different states for the lock system
enum State {  
  locked,
  unlocked,
  updatecode
};

#define I2C_ADDR 0x20
#define LCD_COLUMNS 16
#define LCD_LINES 2

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES);

String currentDisplayMessage = "";  // Tracks current LCD message

Servo myServo;						// Create a Servo object to control the servo motor

int pos = 0;						// Variable to store the servo position between, begins at 0 (0-180 degrees)

// Constants for debounce delay and open time
const int DEBOUNCE_DELAY = 30;		// Delay to prevent button bouncing
const int OPEN_TIME = 5000;			// Time the lock remains open in milliseconds
unsigned long lastDebounceTime = 0;	// Timestamp for the last button press

// Define pin numbers for LEDs
const int YELLOW_LED = 13;			// YELLOW_LED connects to arduino port 13
const int GREEN_LED = 12;			// GREEN_LED connects to arduino port 12
const int RED_LED = 11;				// RED_LED connects to arduino port 11

// Define the number of buttons and their pin numbers
const int NUM_BUTTONS = 5;			
const int BUTTONS[NUM_BUTTONS] = {
  2,  // Button 1
  3,  // Button 2
  4,  // Button 3
  5,  // Button 4
  6,  // Button 5
};
const int RESET_BUTTON_PRESSED = -1;	// Constant to indicate no button is pressed

int lastPressedButton = RESET_BUTTON_PRESSED; // Variable to store the last pressed button

const int FACTORY_CODE[] = {1,2,3,4};	// Factory default code

// Arrays to store codes
int currentCode[4] = {};		// The current code used for unlocking
int newCode[4] = {};			// New code (used during code change)

int tempCode[4] = {};			// Temporary code during first input in code change
int tempCode2[4] = {};			// Temporary code during second input for verification

int userCode[4] = {};			// User's entered code

// Variables to handle code input
int inputCodeTries = 0;			// Number of tries during code change
int inputCodeCount = 0;			// Number of digits entered

// Boolean variables for different states
bool isUnlocked = false;		// Indicates if the lock is unlocked
bool isUpdatingCode = false;	// Indicates if a code change is in progress
bool isTimerStarted = false;	// Indicates if the unlock timer has started
bool isChangingCode = false;	// Indicates if the user is changing the code

unsigned long lastUnlockedTime;	// Timestamp for when the lock was unlocked

State state = State::locked;	// Set the initial state to "locked"

void setup() {
  Serial.begin(9600);			// Start serial communication for debugging, sets baudrate to 9600 (default setting)
  
  // Initialize button pins as input with lup resistors
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTONS[i], INPUT_PULLUP);
  }

  // Initialize LED pins as outputs
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  myServo.attach(9);			// Attach the servo to pin 9
  myServo.write(pos);			// Set the servo's initial position

  lcd.begin(LCD_COLUMNS, LCD_LINES);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  currentDisplayMessage = "LOCKED"; // Initialize display message
  lcd.setCursor(0, 0);
  lcd.print(currentDisplayMessage);
  
  loadCode();					// Load the saved code from EEPROM

  // Check if a valid code exists in EEPROM
  bool codeValid = false;
  for (int i = 0; i < 4; i++) {
    if (currentCode[i] != 0) {
      codeValid = true;
      Serial.println("Valid code found in EEPROM. Loaded saved code.");
      break;
    }
  }

  // If no valid code is found, use the factory default code
  if (!codeValid) {
    for (int i = 0; i < 4; i++) {
      currentCode[i] = FACTORY_CODE[i];
    }
    Serial.println("No valid code found in EEPROM. Loaded factory code.");
  }
}

// Function to check if the entered code matches the saved code
bool checkCode() {
  if (isUnlocked) {
    return true;
  }

  for (int i=0; i < 4; i++) {
    if (!(currentCode[i] == userCode[i])) {
      return false;				// The code does not match
    }
  }

  return true;					// The code matches
}	

// Function to remove the first digit in a code array (FIFO queue)
void deQueue(String codeArray) {
  if (codeArray == "userCode") {
    for (int i = 0; i < 3; i++) {
      userCode[i] = userCode[i+1];
    }
  } else if (codeArray == "tempCode") {
    for (int i = 0; i < 3; i++) {
      tempCode[i] = tempCode[i+1];
    }
  } else if (codeArray == "tempCode2") {
    for (int i = 0; i < 3; i++) {
      tempCode2[i] = tempCode2[i+1];
    }
  }
}

// Function to add a digit to the end of a code array
void enQueue(String codeArray, int value) {
  if (codeArray == "userCode") {
    userCode[3] = value;
  } else if (codeArray == "tempCode") {
    tempCode[3] = value;
  } else if (codeArray == "tempCode2") {
    tempCode2[3] = value;
  }
}

// Function to verify that the two entered new codes match during code change
bool verifyCodeChange() {
  for (int i = 0; i < 4; i++) {
    if (!(tempCode[i] == tempCode2[i])) {
      return false;					// The codes do not match
    }
  }

  return true;						// The codes match
}

// Function to load the saved code from EEPROM
void loadCode() {
  int retrievedCode[4];
  EEPROM.get(0, retrievedCode);		// Retrieve the code from memory address 0
  for (int i=0; i < 4; i++) {
  	currentCode[i] = retrievedCode[i];
  }
}

// Function to save the new code to EEPROM
void saveCode() {
  if (isChangingCode) {
    EEPROM.put(0, currentCode);		// Save the code to memory address 0
  }
}

// Function to handle input of a new code during code change
void inputNewCode(int value) {
  deQueue("tempCode");				// Remove the first digit
  enQueue("tempCode", value);		// Add the new digit
  inputCodeCount++;

  if (inputCodeCount == 4) {		// If four digits have been entered
    inputCodeTries++;
    inputCodeCount = 0;
  }
}

// Function to verify the new code during code change
void verifyNewCode(int value) {
  deQueue("tempCode2");
  enQueue("tempCode2", value);
  inputCodeCount++;

  if (inputCodeCount == 4) {
    if (verifyCodeChange()) {		// If the codes match
      for (int i = 0; i < 4; i++) {
        currentCode[i] = tempCode[i];	// Update the current code
      }
      saveCode();					// Save the new code to EEPROM
    }	
    inputCodeTries = 2;				// Exit the code change process
  }
}

// Function to add a digit to the appropriate code array based on the state
void addToCodeArray(int value) {
  if (isChangingCode) {
    if (inputCodeTries == 0) {
      inputNewCode(value);			// First input of the new code
    } else if (inputCodeTries == 1) {
      verifyNewCode(value);			// Second input for verification
    }
  } else {
    deQueue("userCode");
    enQueue("userCode", value);
    isUnlocked = checkCode();		// Check if the code is correct
  }
}

// Function to start the timer after unlocking
void startTimer() {
  lastUnlockedTime = millis();
  isTimerStarted = true;
}

// Function to reset the entered user code
void resetCode() {
  for (int i = 0; i < 4; i++) {
    userCode[i] = 0;
  }
}
// Function to handle button presses with debouncing
void buttonAction() {
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    lastDebounceTime = millis();

    for (int i = 0; i < NUM_BUTTONS; i++) {
      int buttonValue = digitalRead(BUTTONS[i]);

      if (buttonValue == LOW && lastPressedButton != i) {
        lastPressedButton = i;

        if (i < (NUM_BUTTONS - 1) && !(state == State::unlocked)) {
          int value = i + 1;		// The value corresponds to the button number
          addToCodeArray(value);	// Add the digit to the code array
        } else if (i == 4) {
          isUpdatingCode = true;	// Initiate the code change process
        }
      }

      if (buttonValue == HIGH && lastPressedButton == i) {
        lastPressedButton = RESET_BUTTON_PRESSED;	// Reset the button status
      }
    }
  }
}

// Function to rotate the servo to lock or unlock
void rotateServo(String state) {
  if (state == "unlocked") {
    for (pos = 0; pos <= 180; pos += 1) {
      myServo.write(pos);  			// Rotate the servo to the unlocked position            
    }
  }

  if (state == "locked") {
    for (pos = 180; pos >= 0; pos -= 1) { 
      myServo.write(pos);			// Rotate the servo to the locked position
    }
  }
}

// Function that handles the locked state
void lockedState() {
  // Turn on the red LED to indicate the locked state
  digitalWrite(RED_LED, HIGH);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);

  // Update LCD if the message isn't already "LOCKED"
  if (currentDisplayMessage != "LOCKED") {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("LOCKED");
    currentDisplayMessage = "LOCKED";
  }

  if (isUnlocked){
    rotateServo("unlocked");		// Rotate the servo to unlock
    resetCode();					// Reset the entered code
    state = State::unlocked;		// Switch to the unlocked state
  }
}

// Function that handles the unlocked state
void unlockedState() {
  // Turn on the green LED to indicate the unlocked state
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);

  // Update LCD if the message isn't already "UNLOCKED"
  if (currentDisplayMessage != "UNLOCKED") {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("UNLOCKED");
    currentDisplayMessage = "UNLOCKED";
  }

  if (isUnlocked && !isTimerStarted) {
    startTimer();					// Start the timer for automatic locking
  }
  
  if (isUnlocked && (millis() - lastUnlockedTime) > OPEN_TIME) {
    isTimerStarted = false;
    isUnlocked = false;
    rotateServo("locked");			// Rotate the servo to lock
    state = State::locked;			// Switch back to the locked state
  }
  
  if (isUnlocked && isUpdatingCode) {
    state = State::updatecode;		// Switch to the code change state
    isTimerStarted = false;
  }
}

// Function that handles the code change state
void updateCodeState() {
  // Turn on the yellow and green LEDs to indicate the code change state
  digitalWrite(YELLOW_LED, HIGH);
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  isChangingCode = true;

  if (currentDisplayMessage != "CODE CHANGE") {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CODE CHANGE");
    currentDisplayMessage = "CODE CHANGE";
  }
  
  // Wait for the user to enter the new code twice
  while (isChangingCode && (inputCodeTries < 2)) {
  	buttonAction();
  }

  // Reset variables after code change
  inputCodeTries = 0;
  inputCodeCount = 0;
  isChangingCode = false;
  isUnlocked = false;
  isUpdatingCode = false;

  rotateServo("locked");			// Rotate the servo to lock
  resetCode();						// Reset the entered code
  state = State::locked;			// Switch back to the locked state
}

// The main loop that runs continuously
void loop() {
  buttonAction();					// Handle button presses

  // Switch between different states based on the system status
  if (state == State::locked) {
    lockedState();
  } else if (state == State::unlocked) {
    unlockedState();
  } else if (state == State::updatecode) {
    updateCodeState();
  }
}