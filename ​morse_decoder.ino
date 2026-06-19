#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// OLED Display Configuration (SSD1306 for crisp edge, SCL -> D1=GPIO5, SDA -> D2=GPIO4)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 5, /* data=*/ 4);

// Pin Definitions
#define MORSE_BTN       14  // NodeMCU D5 pin (Morse input button)
#define CLEAR_BTN       12  // NodeMCU D6 pin (Clear all text button)
#define BACKSPACE_BTN   0   // NodeMCU D3 pin (Delete last character button)
#define BUZZER          13  // NodeMCU D7 pin (Active Buzzer)

// Timing Settings (in milliseconds)
const unsigned long DASH_THRESHOLD = 250;   // Pressed longer than 250ms = DASH, shorter = DOT
const unsigned long LETTER_TIMEOUT = 800;   // Wait 0.8s without pressing to convert code to letter
const unsigned long WORD_TIMEOUT   = 2500;  // Wait 2.5s without pressing to insert a space

// Variables
String currentCode = ""; 
String decodedText = ""; 
unsigned long pressTime = 0;
unsigned long releaseTime = 0;
boolean isPressed = false;
boolean isConverted = true;
boolean isSpaceAdded = true;

// Function to convert Morse code to English characters
char morseToChar(String morse) {
  if (morse == ".-") return 'A';    if (morse == "-...") return 'B';  if (morse == "-.-.") return 'C';
  if (morse == "-..") return 'D';   if (morse == ".") return 'E';     if (morse == "..-.") return 'F';
  if (morse == "--.") return 'G';   if (morse == "....") return 'H';  if (morse == "..") return 'I';
  if (morse == ".---") return 'J';  if (morse == "-.-") return 'K';   if (morse == ".-..") return 'L';
  if (morse == "--") return 'M';    if (morse == "-.") return 'N';    if (morse == "---") return 'O';
  if (morse == ".--.") return 'P';  if (morse == "--.-") return 'Q';  if (morse == ".-.") return 'R';
  if (morse == "...") return 'S';   if (morse == "-") return 'T';     if (morse == "..-") return 'U';
  if (morse == "...-") return 'V';  if (morse == ".--") return 'W';   if (morse == "-..-") return 'X';
  if (morse == "-.--") return 'Y';  if (morse == "--..") return 'Z';
  
  if (morse == "-----") return '0'; if (morse == ".----") return '1'; if (morse == "..---") return '2';
  if (morse == "...--") return '3'; if (morse == "....-") return '4'; if (morse == ".....") return '5';
  if (morse == "-....") return '6'; if (morse == "--...") return '7'; if (morse == "---..") return '8';
  if (morse == "----.") return '9';
  
  return '?'; 
}

void refreshDisplay() {
  u8g2.clearBuffer();
  
  // Top Section: Live Morse Input
  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.drawStr(0, 15, "Code: ");
  u8g2.setCursor(35, 15);
  u8g2.print(currentCode);
  
  // Horizontal Separator Line
  u8g2.drawHLine(0, 22, 128);
  
  // Bottom Section: Final Translated Text
  u8g2.setFont(u8g2_font_9x15_tf); 
  u8g2.drawStr(0, 42, "Text:");
  u8g2.setCursor(0, 60);
  u8g2.print(decodedText);
  
  u8g2.sendBuffer();
}

void setup() {
  pinMode(MORSE_BTN, INPUT_PULLUP);
  pinMode(CLEAR_BTN, INPUT_PULLUP);
  pinMode(BACKSPACE_BTN, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  u8g2.begin();
  refreshDisplay();
  releaseTime = millis();
}

void loop() {
  int morseBtnState = digitalRead(MORSE_BTN);
  int clearBtnState = digitalRead(CLEAR_BTN);
  int backspaceBtnState = digitalRead(BACKSPACE_BTN);

  // 1. CLEAR ALL BUTTON (D6)
  if (clearBtnState == LOW) {
    decodedText = "";
    currentCode = "";
    digitalWrite(BUZZER, HIGH); delay(200); digitalWrite(BUZZER, LOW);
    refreshDisplay();
    delay(200);
  }

  // 2. BACKSPACE BUTTON (D3)
  if (backspaceBtnState == LOW) {
    if (decodedText.length() > 0) {
      // Remove the last character from the string
      decodedText = decodedText.substring(0, decodedText.length() - 1);
    }
    currentCode = ""; // Clear any pending/unconverted Morse code
    digitalWrite(BUZZER, HIGH); delay(70); digitalWrite(BUZZER, LOW); // Quick beep
    refreshDisplay();
    delay(250); // Debounce delay
  }

  // 3. MORSE INPUT BUTTON LOGIC (D5)
  if (morseBtnState == LOW && !isPressed) {
    pressTime = millis();
    isPressed = true;
    digitalWrite(BUZZER, HIGH); 
  }

  if (morseBtnState == HIGH && isPressed) {
    unsigned long duration = millis() - pressTime;
    digitalWrite(BUZZER, LOW); 
    isPressed = false;
    releaseTime = millis();
    isConverted = false;
    isSpaceAdded = false;

    if (duration < DASH_THRESHOLD) {
      currentCode += "."; 
    } else {
      currentCode += "-"; 
    }
    refreshDisplay();
  }

  // 4. TIMEOUT CHECKS WHEN BUTTON IS RELEASED
  if (!isPressed) {
    unsigned long idleTime = millis() - releaseTime;

    // Letter Timeout (Convert Morse string to a Character)
    if (idleTime > LETTER_TIMEOUT && !isConverted && currentCode.length() > 0) {
      char nextChar = morseToChar(currentCode);
      if (nextChar != '?') {
        decodedText += nextChar;
      }
      currentCode = ""; 
      isConverted = true;
      refreshDisplay();
    }

    // Word Timeout (Automatically insert a space between words)
    if (idleTime > WORD_TIMEOUT && !isSpaceAdded) {
      if (decodedText.length() > 0 && decodedText[decodedText.length() - 1] != ' ') {
        decodedText += " "; 
      }
      isSpaceAdded = true;
      refreshDisplay();
    }
  }
  delay(10);
}
