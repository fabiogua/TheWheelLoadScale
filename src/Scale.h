#pragma once

#include "HX711.h"
#include "EEPROM.h"

// Configuration
//   Pins
//     LoadCells
#define LOADCELL_DOUT_PIN 3
#define LOADCELL_SCK_PIN  2
#define STD_SCALE_FACTOR  1
#define LOADCELL_TARE_AFTER_TIME 5 // s

//     SerialComm
#define BT_SERIAL_TX PA_9
#define BT_SERIAL_RX PA_10
#define BT_SERIAL_BAUD 38400

//   Settings
#define SENDING_INTERVAL 200 // ms
#define MIN_BOUNDARY_TO_CHANGE_CAL_SIGN 1 // if scaleFactor lower than this, invert -> 0.99... would result in -1
#define INIT_CALIBRATION_INCREASE_FACTOR 0.05 // %
#define CALIBRATION_FACTOR_INCREMENT 0.1 // %

/*
    Protocoll:
        Receiving
            One Char == One Command (over Serial)

            + == Add X% to calibration
            - == Remove X% from calibration
            u == Add 10% to calibration factor
            d == Remove 10% from calibration factor
            t == Tare now
            r == Reset -> Calibration == 1 -> EEPROM Reset
            p == Toggle PC Output

        Submitting
            Sending in a sentence all Info at once

            W[-]X.XC[-]X.XFX.X\n

            W == Weight in g, X.X is the Number (size not defined) with optional sign [-]
            C == Calibration factor, X.X is the Number (size not defined) with optional sign [-]
            F == Factor for calibration factor setting
            \n == new Line/End of sentence
*/

enum eepromAddresses : uint16_t {
    SaveAddrCalibration = 0
};

// Global Vars
HardwareSerial SerialBT(BT_SERIAL_RX, BT_SERIAL_TX);
HX711 loadcell;
Timer sendingTimer;
float scaleFactor = 1;
bool pcOutput = false;
float calibrationIncreaseFactor = INIT_CALIBRATION_INCREASE_FACTOR;

// Functions
void sendSentence();
void gotChar(char command);
void saveScaleFactor();

void setup() {
    // Print Version to Serial
    Serial.begin(9600);
    printf("TheWheelLoadScale by Timo Meyer - Scale with Version: %s\n\n", SW_VERSION);

    // Init Scale
    EEPROM.get(SaveAddrCalibration, scaleFactor);
    loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    loadcell.set_scale(scaleFactor);
    wait(LOADCELL_TARE_AFTER_TIME); // Wait for tare
    loadcell.tare();

    // Start Serial Bluetooth Communication
    SerialBT.begin(BT_SERIAL_BAUD);

    // Begin sending Timer
    sendingTimer.start();
}


void loop() {
    // Receiving BT
    while (SerialBT.available()) {
        gotChar(SerialBT.read());
    }

    // Receiving PC
    while (Serial.available()) {
        gotChar(Serial.read());
    }

    // Sending
    if (sendingTimer.read_ms() >= SENDING_INTERVAL) {
        sendingTimer.reset();
        sendingTimer.start();

        sendSentence();
    }
}

inline void saveScaleFactor() {
    EEPROM.put(SaveAddrCalibration, scaleFactor);
}

void gotChar(char command) { // -> incoming protocoll parser
    switch(command) {
        case '+': {
            if (scaleFactor < 0) {
                scaleFactor -= scaleFactor * calibrationIncreaseFactor;
                if (scaleFactor > (-1 * MIN_BOUNDARY_TO_CHANGE_CAL_SIGN))
                    scaleFactor = MIN_BOUNDARY_TO_CHANGE_CAL_SIGN;
            } else {
                scaleFactor += scaleFactor * calibrationIncreaseFactor;
            }

            loadcell.set_scale(scaleFactor);
            saveScaleFactor();
            break;
        }
        case '-': {
            if (scaleFactor > 0) {
                scaleFactor -= scaleFactor * calibrationIncreaseFactor;
                if (scaleFactor < MIN_BOUNDARY_TO_CHANGE_CAL_SIGN)
                    scaleFactor = -1 * MIN_BOUNDARY_TO_CHANGE_CAL_SIGN;
            } else {
                scaleFactor += scaleFactor * calibrationIncreaseFactor;
            }

            loadcell.set_scale(scaleFactor);
            saveScaleFactor();
            break;
        }
        case 'u':
            calibrationIncreaseFactor += calibrationIncreaseFactor * CALIBRATION_FACTOR_INCREMENT;
            break;
        case 'd':
            calibrationIncreaseFactor -= calibrationIncreaseFactor * CALIBRATION_FACTOR_INCREMENT;
            break;
        case 'r': {
            loadcell.set_scale();
            scaleFactor = 1.0;
            saveScaleFactor();
            calibrationIncreaseFactor = INIT_CALIBRATION_INCREASE_FACTOR;
            break;
        }
        case 't':
            loadcell.tare();
            break;
        case 'p':
            pcOutput = !pcOutput;
            break;
    }
}

void sendSentence() {
    float gotGramms = loadcell.get_units(10);

    // Prep sentence
    char buffer[100];
    sprintf (buffer, "W%fC%fF%f\n", gotGramms, scaleFactor, calibrationIncreaseFactor);

    // Send over BT
    SerialBT.print(buffer);

    if (pcOutput) {
        // Send optionally to PC too
        Serial.print(buffer);
    }
}