#pragma once

#include "StreamSwitch/StreamRouter.h"
#include "RouterPins.h"

#define BT_SERIAL_BAUD 38400
#define ROUTER_SERIAL_BAUD 115200

HardwareSerial SerialFL(BT_SERIAL_FL_RX, BT_SERIAL_FL_TX);
HardwareSerial SerialRR(BT_SERIAL_RR_RX, BT_SERIAL_RR_TX);
HardwareSerial SerialReceiver(RECEIVER_SERIAL_RX, RECEIVER_SERIAL_TX);

StreamRouter<2> serialRouter(SerialReceiver);

void setup() {
    SerialFL.begin(BT_SERIAL_BAUD);
    SerialRR.begin(BT_SERIAL_BAUD);
    SerialReceiver.begin(ROUTER_SERIAL_BAUD);

    serialRouter.setStream(SerialFL, 0);
    serialRouter.setStream(SerialRR, 1);
}

void loop() {
    serialRouter.update();
}