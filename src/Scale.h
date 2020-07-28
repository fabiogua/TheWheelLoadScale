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
//#define BT_SERIAL_TX PA_2
//#define BT_SERIAL_RX PA_3
#define BT_SERIAL_BAUD 38400

//   Settings
#define SENDING_INTERVAL 200 // ms
#define MIN_BOUNDARY_TO_CHANGE_CAL_SIGN 1
#define CALIBRATION_INCREASE_FACTOR 0.05 // %

/*
    Protocoll:
        Receiving
            One Char == One Command (over Serial)

            + == Add 5% to calibration
            - == Remove 5% from calibration
            t == Tare now
            r == Reset -> Calibration == 1 -> EEPROM Reset
            p == Toggle PC Output

        Submitting
            Sending in a sentence all Info at once

            W[-]X.XC[-]X.X\n

            W == Weight in g, X.X is the Number (size not defined) with optional sign [-]
            C == Calibration factor, X.X is the Number (size not defined) with optional sign [-]
            \n == new Line/End of sentence
*/

enum eepromAddresses : uint16_t {
    SaveAddrCalibration = 0
};

// Global Vars
HX711 loadcell;
Timer sendingTimer;
float scaleFactor = 1;
bool pcOutput = false;

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
    Serial2.begin(BT_SERIAL_BAUD);

    // Begin sending Timer
    sendingTimer.start();
}


void loop() {
    // Receiving BT
    if (Serial2.available()) {
        gotChar(Serial2.read());
    }

    // Receiving PC
    if (Serial.available()) {
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
            scaleFactor *= CALIBRATION_INCREASE_FACTOR;
            loadcell.set_scale(scaleFactor);
            saveScaleFactor();
            break;
        }
        case '-': {
            scaleFactor *= (-1 * CALIBRATION_INCREASE_FACTOR);
            loadcell.set_scale(scaleFactor);
            saveScaleFactor();
            break;
        }
        case 'r': { // also including t -> tare -> no break
            loadcell.set_scale();
            scaleFactor = 1.0;
            saveScaleFactor();
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
    sprintf (buffer, "W%fC%f\n", gotGramms, scaleFactor);

    // Send over BT
    Serial2.print(buffer);

    if (pcOutput) {
        // Send optionally to PC too
        Serial.print(buffer);
    }
}