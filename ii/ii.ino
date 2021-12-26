#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Keypad.h>
#include <TimerOne.h>
/////////////////////////////////////////////////////////////// HARDWARE ///////////////////////////////////////////////////////////////
// declarare display
#define TFT_CS 10
#define TFT_RST 9
#define TFT_DC 8
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

//declarare butoane utilizate de pe keypad
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}

};
//declarare pini pentru keypad
byte rowPins[ROWS] = {6, 5, 7, 12};
byte colPins[COLS] = {4, 3, 2};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

/////////////////////////////////////////////////////////////// SOFTWARE ///////////////////////////////////////////////////////////////
// stari
enum stateT {INDEX, INFO, TEMPERATURE, SECURITY, OPEN_DOOR, CHANGE_DOOR_CODE};
enum stateThermostat {STOP, START_HOT, START_COLD};

stateT state = INDEX;
stateThermostat thermostat = STOP;
// management erori
byte wrongCode = false;
// stare elemente monitorizate
byte window1Closed = false, window2Closed = false;
byte door1Closed = true, door2Closed = true;
short selectedDoor = 0;
// termostat
volatile float ambientTemperature = 21.0;
float setTemperature = 21.0;
// securitate
char code[2][4] = {{'0', '0', '0', '0'}, {'0', '0', '0', '0'}};
char userCode[4] = {' ', ' ', ' ', ' '};
char userCodeIndex = 0;


void setup() {
  Serial.begin(9600);
  tft.initR(INITR_BLACKTAB);
  Timer1.initialize(5000000);
  Timer1.attachInterrupt(changeAmbientTemperature);
  indexMenu();
}

void loop() {
  char key = keypad.getKey();

  if (key) {

    switch (state) {
      case INDEX:
        switch (key) {
          case '1': state = INFO; infoMenu(); break;
          case '2': state = TEMPERATURE; thermostatMenu(); break;
          case '3': state = SECURITY; locksMenu(); break;
          default: Serial.println("DK which button you pressed");
        }
        break;
      case INFO:
        switch (key) {
          case '1': handleInputChanged(1); break;
          case '2': handleInputChanged(2); break;
          case '4': state = OPEN_DOOR; resetInputCode(); selectedDoor = 1; openDoor(); break;
          case '5': state = OPEN_DOOR; resetInputCode(); selectedDoor = 2; openDoor(); break;
          case '*': state = INDEX;  indexMenu(); break;
          default: Serial.println("DK which button you pressed");
        }
        break;
      case OPEN_DOOR:
        switch (key) {
          case '*': state = INFO; wrongCode = false; infoMenu(); break;
          case '#': verifyCode(); break;
          default:
            Serial.println("You pressed: ");
            Serial.println(key);
            handleAddCode(key, 1);
        }
        break;
      case TEMPERATURE:
        switch (key) {
          case '2': increaseTemperature(); break;
          case '8': decreaseTemperature(); break;
          case '#':
            saveTemperature();
            state = INFO;
            infoMenu();
            break;
          case '*': state = INDEX; indexMenu(); break;
          default:
            Serial.println("You pressed: ");
            Serial.println(key);
        }
        break;
      case SECURITY:
        switch (key) {
          case '*': state = INDEX; indexMenu(); break;
          case '1': state = CHANGE_DOOR_CODE; resetInputCode(); selectedDoor = 1; changeDoorCode(); break;
          case '2': state = CHANGE_DOOR_CODE; resetInputCode(); selectedDoor = 2; changeDoorCode(); break;
          default:
            Serial.print("You pressed: ");
            Serial.println(key);

        }
        break;
      case CHANGE_DOOR_CODE:
        switch (key) {
          case '*': state = SECURITY; locksMenu(); break;
          case '#': confirmCode(); state = SECURITY; locksMenu(); break;
          default:
            Serial.println("You pressed: ");
            Serial.println(key);
            handleAddCode(key, 0);
        }
        break;
      default: Serial.println("Who am I?");
    }
  }
}

/////////////////////////////////////////////////////////////// COMMONS ///////////////////////////////////////////////////////////////
void initPage() {
  tft.fillScreen(ST7735_WHITE);
  tft.setTextSize(1);
  tft.setTextColor(ST7735_BLACK);
  tft.setTextWrap(true);
}
uint16_t getColor(byte item) {
  if (item)
    return ST7735_GREEN;
  else
    return ST7735_RED;
}
void resetInputCode() {
  for (char i = 0; i < 4; i++)
    userCode[i] = ' ';
}
/////////////////////////////////////////////////////////////// INDEX ///////////////////////////////////////////////////////////////
void indexMenu() {
  initPage();
  tft.setCursor(5, 50);
  tft.print("[1] Informatii");
  tft.setCursor(5, 80);
  tft.print("[2] Termostat");
  tft.setCursor(5, 110);
  tft.print("[3] Lacate");
}

/////////////////////////////////////////////////////////////// INFO ///////////////////////////////////////////////////////////////
void handleInputChanged(int input) {
  switch (input) {
    case 1: window1Closed = !window1Closed; break;
    case 2: window2Closed = !window2Closed; break;
    default: Serial.println("Only Jesus can help you now!");
  }
  infoMenu();
}
void infoMenu() {
  initPage();

  tft.setCursor(5, 5);
  tft.print("[1,2] Ferestre:");
  tft.setCursor(5, 15);
  tft.setTextColor(getColor(window1Closed));
  tft.print("win1 ");
  tft.setTextColor(getColor(window2Closed));
  tft.print("win2");

  tft.setTextColor(ST7735_BLACK);
  tft.setCursor(5, 35);
  tft.print("[4,5] Usi:");
  tft.setCursor(5, 45);
  tft.setTextColor(getColor(door1Closed));
  tft.print("usa1 ");
  tft.setTextColor(getColor(door2Closed));
  tft.print("usa2");

  tft.setTextColor(ST7735_BLACK);
  tft.setCursor(5, 65);
  tft.print("Temperatura:");
  tft.setCursor(5, 75);
  tft.print(ambientTemperature);
  tft.print(" | ");
  tft.print(setTemperature);

  tft.setCursor(5, 95);
  tft.print("Centrala:");
  tft.setCursor(5, 105);
  if (thermostat == START_HOT) {
    tft.setTextColor(ST7735_GREEN);
    tft.print("ON");
  }
  else {
    tft.setTextColor(ST7735_RED);
    tft.print("OFF");
  }

  tft.setTextColor(ST7735_BLACK);
  tft.setCursor(5, 125);
  tft.print("Aer conditionat:");
  tft.setCursor(5, 135);
  if (thermostat == START_COLD) {
    tft.setTextColor(ST7735_GREEN);
    tft.print("ON");
  }
  else {
    tft.setTextColor(ST7735_RED);
    tft.print("OFF");
  }
}
/////////////////////////////////////////////////////////////// OPEN_DOOR ///////////////////////////////////////////////////////////////
void handleAddCode(char key, byte _openDoor) {
  if (userCodeIndex + 1 > 4)
    userCodeIndex = 0;
  userCode[userCodeIndex] = ' ';
  userCode[userCodeIndex++] = key;
  if (_openDoor)
    openDoor();
  else
    changeDoorCode();
}
void openDoorWithCode() {
  initPage();
  tft.setCursor(5, 5);
  tft.print("      Usa: ");
  tft.print(selectedDoor);
  tft.setCursor(5, 15);
  tft.print(" Pentru a deschide    usa am nevoie de      codul de acces:");

  tft.setTextSize(2);
  tft.setCursor(20, 60);
  for (short i = 0; i < 4; i++) {
    if (userCode[i] == ' ')
      tft.print("_ ");
    else {
      tft.print(userCode[i]);
      tft.print(" ");
    }
  }

  if (wrongCode) {
    tft.setCursor(5, 100);
    tft.setTextColor(ST7735_RED);
    tft.print("WRONG CODE");
    tft.setTextColor(ST7735_BLACK);
  }
  tft.setTextSize(1);
  tft.setCursor(5, 140);
  tft.print(" Apasa pe ");
  tft.setTextColor(ST7735_RED);
  tft.print("# ");
  tft.setTextColor(ST7735_BLACK);
  tft.print("pentru a verifica codul");
}

void openDoor() {
  if (selectedDoor == 1) {
    if (door1Closed == false) {
      door1Closed = true;
      state = INFO;
      infoMenu();
    }
    else {
      openDoorWithCode();
    }
  }
  else {
    if (door2Closed == false) {
      door2Closed = true;
      state = INFO;
      infoMenu();
    }
    else {
      openDoorWithCode();
    }
  }

}
void verifyCode() {
  byte ok = true;
  for (char i = 0; i < 4; i++) {
    if (code[selectedDoor - 1][i] != userCode[i]) {
      ok = false;
      break;
    }
  }
  if (ok) {
    if (selectedDoor == 1)
      door1Closed = false;
    else
      door2Closed = false;
    wrongCode = false;
    state = INFO;
    infoMenu();
  }
  else {
    wrongCode = true;
    openDoor();
  }
}
/////////////////////////////////////////////////////////////// THERMOSTAT ///////////////////////////////////////////////////////////////
void increaseTemperature() {
  setTemperature += 1;
  thermostatMenu();
}
void decreaseTemperature() {
  setTemperature -= 1;
  thermostatMenu();
}

void changeAmbientTemperature() {
  if (thermostat != STOP) {
    if (ambientTemperature != setTemperature) {
      Serial.println("This is an inside job");
      if (thermostat == START_COLD)
        ambientTemperature -= 0.5;
      if (thermostat == START_HOT)
        ambientTemperature += 0.5;
      if (state == INFO)
        infoMenu();
    }
    else {
      thermostat = STOP;
      if (state == INFO)
        infoMenu();
    }
  }
}
void saveTemperature() {
  if (setTemperature < ambientTemperature)
    thermostat = START_COLD;
  else
    thermostat = START_HOT;
}
void thermostatMenu() {
  initPage();

  tft.setCursor(5, 5);
  tft.print("Ambient: ");
  tft.print(ambientTemperature);
  tft.setCursor(5, 25);
  tft.print("Termostat: ");
  tft.print(setTemperature);

  tft.setCursor(5, 45);
  tft.print("[2] Creste temperatura termostat");
  tft.setCursor(5, 75);
  tft.print("[8] Scade temperatura termostat");
  tft.setCursor(5, 95);
  tft.print("[#] Salveaza temperatura termostat");
}

/////////////////////////////////////////////////////////////// LOCKS ///////////////////////////////////////////////////////////////
void changeDoorCode() {
  initPage();
  tft.setCursor(5, 5);
  tft.print("      Usa: ");
  tft.print(selectedDoor);
  tft.setCursor(5, 15);
  tft.print(" Introdu noul cod de acces:");

  tft.setTextSize(2);
  tft.setCursor(20, 60);
  for (short i = 0; i < 4; i++) {
    if (userCode[i] == ' ')
      tft.print("_ ");
    else {
      tft.print(userCode[i]);
      tft.print(" ");
    }
  }
  tft.setTextSize(1);
  tft.setCursor(5, 140);
  tft.print(" Apasa pe ");
  tft.setTextColor(ST7735_RED);
  tft.print("# ");
  tft.setTextColor(ST7735_BLACK);
  tft.print("pentru a salva noul cod");
}
void confirmCode() {
  for (byte i = 0; i < 4; i++) {
    code[selectedDoor - 1][i] = userCode[i];
  }
}
void locksMenu() {
  initPage();

  tft.setCursor(5, 5);
  tft.print("Coduri de acces: ");
  tft.setCursor(5, 25);
  tft.print("[1] Usa1: ");
  for (char i = 0; i < 4; i++)
    tft.print(code[0][i]);
  tft.setCursor(5, 40);
  tft.print("[2] Usa2: ");
  for (char i = 0; i < 4; i++)
    tft.print(code[1][i]);

}
