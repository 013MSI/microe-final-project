//-----------------------//
/*
=======================
========NOTES==========
=======================
*CONNECT STEPPER TO 5V








*/
//-----------------------//
#include <Stepper.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// all measurements are in mm
#define WATER_SENSOR_HEIGHT 42
#define POND_DIAMETER 183
#define STEM_DIAMETER 48
#define POND_DEPTH 67

// Stepper pins
#define STEPPER_IN1 26
#define STEPPER_IN2 25 // for some reason, In2 and In3 must be switched for stepper to work. IN2 is actually 33, IN3 is actually 25.
#define STEPPER_IN3 33
#define STEPPER_IN4 32

// touch sensor pins
#define TOUCH_SIG 36

// water sensor
#define WATER_READ 15
#define WATER_POWER 0

// photoresistor
#define LIGHT_PIN 4

// button pin
#define BUTTON_PIN 39

// declare variables
int waterRaw;                        // raw water sensor reading
int waterFillPercentage;             // water reading as a percent
int lightRaw;                        // raw light reading
int lightPercentage;                 // light reading as percentage
const int stepsPerRevolution = 2048; // change this to fit the number of steps per revolution for your motor
bool touchSensor = 0;                // boolean value measures if touch sensor is touched/activated
bool bloomieBloomin = 0;             // boolean measures if all environmental variables have been met so bloomie will bloom

int lastButtonState;    // the previous state of button
int currentButtonState; // the current state of button

extern const unsigned char smileyy[128];
extern const unsigned char waterSad[128];
extern const unsigned char sunSad[128];
extern const unsigned char thermSad[128];
extern const unsigned char touchSad[128];

// Multicore Tasks
TaskHandle_t stepperTask;

// initialize the stepper library on pins 8 through 11:
Stepper myStepper(stepsPerRevolution, STEPPER_IN1, STEPPER_IN2, STEPPER_IN3, STEPPER_IN4);

// calibration
int calibrationCurrentStage = 0; // What stage we're on.

// min and max readings from calibration
int WATER_LEVEL_MAX = 1023;
int WATER_LEVEL_MIN = 1023;

int LIGHT_LEVEL_MAX = 1023;

void setup() {
    Serial.begin(115200);
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) { // Address 0x3D for 128x64
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }
    pinMode(WATER_POWER, OUTPUT);
    pinMode(WATER_READ, INPUT);
    pinMode(LIGHT_PIN, INPUT);
    pinMode(BUTTON_PIN, INPUT);
    digitalWrite(WATER_POWER, LOW);
    // Clear the buffer
    display.clearDisplay();

    // set up stepper speed
    myStepper.setSpeed(15);
    // stepper multicore task
    xTaskCreatePinnedToCore(
        stepperTaskFunction, /* Function to implement the task */
        "stepperTask",       /* Name of the task */
        10000,               /* Stack size in words */
        NULL,                /* Task input parameter */
        0,                   /* Priority of the task */
        &stepperTask,        /* Task handle. */
        0);                  /* Core where the task should run */

    // button stuff
    currentButtonState = digitalRead(BUTTON_PIN);

    // CALIBRATION
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("We need to get a few things set up for your bloomie.");
    display.println("Make sure your water sensor is dry, then press the button");
    display.display();
    waitForButtonPress();
    digitalWrite(WATER_POWER, HIGH);
    delay(10);
    waterRaw = analogRead(WATER_READ);
    digitalWrite(WATER_POWER, LOW);
    WATER_LEVEL_MIN = waterRaw;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Great job! Now, fill up the bloomie pond with water so that it reaches the top of the sensor.");
    display.println("When you're done, press the button.");
    display.display();
    waitForButtonPress();
    digitalWrite(WATER_POWER, HIGH);
    delay(10);
    waterRaw = analogRead(WATER_READ);
    digitalWrite(WATER_POWER, LOW);
    WATER_LEVEL_MAX = waterRaw;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Terrific! You're all set up to play with your new bloomie!");
    display.display();
    delay(5000);
    display.clearDisplay();
    display.display();
}

void loop() {
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    // display OLED icons
    showBitmap(waterSad, 128 - 32, 0);
    showBitmap(smileyy, 128 - 32, 0 + 32);
    showBitmap(sunSad, 128 - 32 - 32, 0);
    showBitmap(thermSad, 128 - 32 - 32, 0 + 32);
    showBitmap(touchSad, 128 - 32 - 32 - 32, 0);
    // grab water percentage and print to serial monitor
    waterFillPercentage = getWaterPercentage();
    Serial.print("water:");
    Serial.println(waterFillPercentage);
    // grab light percentage and print to serial monitor
    lightPercentage = getLightPercentage();
    Serial.print("light:");
    Serial.println(lightPercentage);

    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println(waterFillPercentage);
    display.display();
    display.clearDisplay();
    delay(50);
}

int getWaterPercentage() {
    digitalWrite(WATER_POWER, HIGH);
    delay(10);
    waterRaw = analogRead(WATER_READ);
    digitalWrite(WATER_POWER, LOW);
    return constrain(map(waterRaw, WATER_LEVEL_MIN, WATER_LEVEL_MAX, 0, 100), 0, 100);
}

int getLightPercentage() {
    delay(10);
    lightRaw = analogRead(LIGHT_PIN);
    return lightRaw;
    // return map(lightRaw, 0, WATER_LEVEL_MAX, 0, 100);
}

void stepperTaskFunction(void *pvParameters) {
    for (;;) {
        // step one revolution  in one direction:
        Serial.println("clockwise");
        myStepper.step(stepsPerRevolution);
        delay(1000);

        // step one revolution in the other direction:
        Serial.println("counterclockwise");
        myStepper.step(-stepsPerRevolution);
        delay(1000);
    }
}

void waitForButtonPress() {
    for (;;) {
        if (buttonFunction()) {
            break;
        }
    }
}

bool buttonFunction() {
    lastButtonState = currentButtonState;         // save the last state
    currentButtonState = digitalRead(BUTTON_PIN); // read new state

    if (lastButtonState == HIGH && currentButtonState == LOW) {
        return true;
    } else {
        return false;
    }
}
