/*
 * Light and temperature controlled Algea incubator
 * 
 * Author: Romain Defferrard 
*/

#include <DS1302.h> // Library found on Github (https://github.com/msparks/arduino-ds1302)
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Define LCD
LiquidCrystal_I2C lcd(0x27, 20, 4);  // Set the LCD I2C address

#define ONE_WIRE_BUS 8
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int numberOfDevices; // Number of temperature devices found
DeviceAddress tempDeviceAddress; // Variable to store a found device address

// Initialize the RTC
DS1302 rtc(11, 10, 9);  // Replace pins as necessary
Time t;


// Define relay pin
const int relay1 = 7;
const int relay2 = 6;
const int relay3 = 5;
const int relay4 = 4;

// Define default starting and ending hours
volatile int onHour = 8;
volatile int offHour = 20;
volatile bool adjustingOnHour = true;

// Define switch pins
const int buttonIncreasePin = 3; // Button to increase onHour
const int buttonDecreasePin = 2; // Button to decrease offHour
const int switchAllTimeOn = A1; // Switch position 1: All time on
const int switchSetOffHours = A2; // Switch position 3: Set off hours

// Define debouncing related variables
volatile unsigned long lastInterruptTimeInc = 0;
volatile unsigned long lastInterruptTimeDec = 0;
const long debounceDelay = 200; // milliseconds

// Define temperatures variables
float tempIN = 0;
float tempOUT = 0;

// Define threshold to activate fan [°C]
const float TempThreshold = 0.3;

int state = 0;

void setup() {
  Serial.begin(9600);
  // Initialize the LCD
  lcd.init();
  lcd.backlight();

  sensors.begin();
  numberOfDevices = sensors.getDeviceCount();
  
  pinMode(buttonIncreasePin, INPUT_PULLUP);
  pinMode(buttonDecreasePin, INPUT_PULLUP);
  pinMode(switchAllTimeOn, INPUT_PULLUP);
  pinMode(switchSetOffHours, INPUT_PULLUP);

  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(buttonIncreasePin), increaseHour, FALLING);
  attachInterrupt(digitalPinToInterrupt(buttonDecreasePin), decreaseHour, FALLING);

   rtc.halt(false);
  rtc.writeProtect(false);
  //rtc.setDOW(WEDNESDAY);
  //rtc.setTime(16,38,30);    
  //rtc.setDate(24,07,2024);
}

void loop() {
  // Get the current time
  t = rtc.getTime();

  // Read temperatures
  sensors.requestTemperatures();
  for (int i = 0; i < sensors.getDeviceCount(); i++) {
    if (sensors.getAddress(tempDeviceAddress, i)) {
      float tempC = sensors.getTempC(tempDeviceAddress);
      if (i == 0) {
        tempOUT = tempC;
      } else if (i == 1) {
        tempIN = tempC;
      }
    }
  }

  // Determine the state based on the switch position
  if (digitalRead(switchAllTimeOn) == LOW) {
    state = 0; // On all the time
    lcd.clear();
  } else if (digitalRead(switchSetOffHours) == LOW) {
    state = 2; // Set off hours
    lcd.clear();
  } else {
    state = 1; // Set on hours (middle position)
    lcd.clear();
  }

  // Update adjustingOnHour based on state
  adjustingOnHour = (state == 1);

  // Display temperature & hours
  updateLCD(t, tempIN, tempOUT, state);

  // Assuming you want to turn LEDs on between ON and OFF hours
  if (state==0){
    digitalWrite(relay1, HIGH); // Turn on the relays
    digitalWrite(relay2, HIGH);
    digitalWrite(relay3, HIGH);
  } else {
    if (t.hour >= onHour && t.hour < offHour) {
      digitalWrite(relay1, HIGH); // Turn on the relays
      digitalWrite(relay2, HIGH);
      digitalWrite(relay3, HIGH);
    } else {
      digitalWrite(relay1, LOW);  // Turn off the Relays
      digitalWrite(relay2, LOW);
      digitalWrite(relay3, LOW);
    }   
  }
  

  // Regulate the fans
  if (tempIN > tempOUT + TempThreshold) {
    digitalWrite(relay4, HIGH);
  } else {
    digitalWrite(relay4, LOW);
  }

  delay(200);  // Update every 200ms
}

void updateLCD(Time t, float tempIN, float tempOUT, int state) {
  // Display the current time and setting state
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  lcd.print(t.hour);
  lcd.print(":");
  lcd.print(t.min < 10 ? "0" : "");
  lcd.print(t.min);

  // Display mode
  lcd.setCursor(0, 1);
  lcd.print("Mode: ");
  if (state == 0) {
    lcd.print("On all time");
    
  } else if (state == 1) {
    lcd.print("Set ON hours");
  } else if (state == 2) {
    lcd.print("Set OFF hours");
  }

  // Display on/off hours and which is being adjusted
  if (state == 1 || state == 2) {
    lcd.setCursor(0, 2);
    lcd.print(adjustingOnHour ? "->" : "  ");
    lcd.print("On: ");
    lcd.print(onHour);
    lcd.print(" ");
    lcd.print(!adjustingOnHour ? "->" : "  ");
    lcd.print("Off: ");
    lcd.print(offHour);
  }

  lcd.setCursor(0, 3);
  lcd.print("In:");
  lcd.print(tempIN, 1);
  lcd.print(" C");
  lcd.print("|");
  lcd.print("Out:");
  lcd.print(tempOUT, 1);
  lcd.print(" C");
}

void increaseHour() {
  if (millis() - lastInterruptTimeInc > debounceDelay) {
    if (adjustingOnHour) {
      onHour = (onHour + 1) % 24;
    } else {
      offHour = (offHour + 1) % 24;
    }
    lastInterruptTimeInc = millis();
  }
}

void decreaseHour() {
  if (millis() - lastInterruptTimeDec > debounceDelay) {
    if (adjustingOnHour) {
      onHour = (onHour - 1 + 24) % 24;
    } else {
      offHour = (offHour - 1 + 24) % 24;
    }
    lastInterruptTimeDec = millis();
  }
}
