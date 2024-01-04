#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino_FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <Keypad.h>

#define PIR_PIN 2
#define PIEZO_PIN A0
#define LED_PIN 11
#define BUZZER_PIN 14
#define KEYPAD_LED_PIN 8

LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte ROW_NUM = 4;
const byte COLUMN_NUM = 4;

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte pin_rows[ROW_NUM] = {22, 23, 24, 25};
byte pin_column[COLUMN_NUM] = {42, 43, 44, 45};

Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

SemaphoreHandle_t keypadSemaphore;
bool alarmEnabled = false;
bool sensorEnabled = false;
char enteredPassword[5] = "";
int passwordIndex = 0;

void keypadTask(void *pvParameters) {
  // Serial.println("keypad task start");
  while (1) {
    char key = keypad.getKey();
    if (key != NO_KEY) {
      // Serial.print("Key pressed: ");
      // Serial.println(key);

      if (key == 'B') {
        // Submit the password
        if (passwordIndex == 4) {
          // Check password and toggle alarm and sensor
          if (checkPassword(enteredPassword)) {
            alarmEnabled = false;
            sensorEnabled = false;
            digitalWrite(KEYPAD_LED_PIN, LOW);

            // Serial.println("Alarm and Sensor turned off");

            lcd.clear();
            lcd.print("Alarm and Sensor Off");
            delay(2000);

          } else {
            // Incorrect password, handle accordingly (e.g., display error on LCD)
            // Serial.println("Incorrect password");
            lcd.clear();
            lcd.print("Incorrect password");
            delay(2000);
            lcd.clear();
          }

          // Reset entered password
          memset(enteredPassword, 0, sizeof(enteredPassword));
          passwordIndex = 0;
        }
      } else if (key == 'A') {
        // Toggle alarm and sensor
        alarmEnabled = !alarmEnabled;
        sensorEnabled = !sensorEnabled;
        // Serial.println("Alarm and Sensor toggled");
        digitalWrite(KEYPAD_LED_PIN, sensorEnabled ? HIGH : LOW);

        lcd.clear();
        lcd.print("Alarm and Sensor ");
        lcd.print(sensorEnabled ? "On" : "Off");
        delay(2000);
        lcd.clear();
      } else if (passwordIndex < 4) {
        // Store pressed key for password entry
        enteredPassword[passwordIndex] = key;
        passwordIndex++;
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void securityTask(void *pvParameters) {
  // Serial.println("security task start");

  while (1) {
    // Serial.println("security task loop");

    if (digitalRead(PIR_PIN) == HIGH && sensorEnabled) {
      // Serial.println("PIR sensor triggered");
      digitalWrite(LED_PIN, HIGH);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Security: ");
      lcd.setCursor(0, 1);
      lcd.print("Mov det");

      if (analogRead(PIEZO_PIN) > 200) {
        // Serial.println("Intruders detected");
        digitalWrite(LED_PIN, HIGH);
        digitalWrite(BUZZER_PIN, HIGH);

        String s = 1 + "|";
        Serial.println(s);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Security: ");
        lcd.setCursor(8, 1);
        lcd.print("Int det");
      }
    } else {
      // Serial.println("No motion detected");
      digitalWrite(LED_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      lcd.clear();
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

bool checkPassword(const char *enteredPassword) {
  const char correctPassword[] = "1234"; // Change this to your desired password
  return strcmp(enteredPassword, correctPassword) == 0;
}

void setup() {
  Serial.begin(9600);
  // Serial.println("setup start");
  pinMode(PIR_PIN, INPUT);
  pinMode(PIEZO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(KEYPAD_LED_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();

  keypadSemaphore = xSemaphoreCreateBinary();

  xTaskCreate(keypadTask, "keypadTask", 128, NULL, 1, NULL);
  xTaskCreate(securityTask, "securityTask", 128, NULL, 2, NULL);

  vTaskStartScheduler();
}

void loop() {
  // Empty, tasks are scheduled by FreeRTOS
}
