#pragma once

#include "Graphics/tigerIcon.h"
#include "ILI9341_t3.h"
#include "XPT2046_Touchscreen.h"
#include "drawIt.h"
#include "font_Arial.h"
#include "Graphics/ElementAppearance.h"
#include "AltSoftSerial.h"
#include "SoftwareSerial.h"
#include "DisplayPins.h"
#include "StreamSwitch/StreamReceiver.h"

// Configuration
//   Timings
#define LOOP_WAIT_TIME 100 // ms -> 1/Display refresh rate

//   Buffers
#define STREAM_INPUT_BUFFER_LEN 1024

//   Baud
#define BT_SERIAL_BAUD 38400
#define ROUTER_SERIAL_BAUD 115200


// Globals
typedef ILI9341_t3 Display;
ILI9341_t3 display = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_CLK, TFT_MISO);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);
Timer loopTimer;

// Elements
//   Details One Scale Menu
drawIt::button<Display> back(display,    1,   1, 238,  38); // top
drawIt::button<Display> down(display,    1, 201, 118,  38); // upper left
drawIt::button<Display> up(display,    121, 201, 118,  38); // upper right
drawIt::button<Display> minus(display,   1, 241, 118,  38); // middle left
drawIt::button<Display> plus(display,  121, 241, 118,  38); // middle right
drawIt::button<Display> reset(display,   1, 281, 118,  38); // bottom left
drawIt::button<Display> tare(display,  121, 281, 118,  38); // bottom right

// Serial
AltSoftSerial SerialRL;
// SimpleStreams (from Serial)
SimpleStreamAdapter streamFR(SerialFR);
SimpleStreamAdapter streamRL(SerialRL);
StreamReceiver<2> streamReceiver(SerialReceiver);

// Functions
void hideAll();
void showAll();
void updateAll();
void showMenu();
void hideMenu();
void updateMenu();
void writeTextAt(const char* text, uint16_t x, uint16_t y, ILI9341_t3_font_t font);


class WheelScale {
   public:
    WheelScale() {
        _touchArea.changeOrigin(0, 0);
        _touchArea.changeLength(240/2, 320/2);
    }

    void setStream(ISimpleStream* stream) {
        _stream = stream;
    }

    void send(char charToSend) {
        if (_stream != nullptr)
            _stream->write(charToSend);
    }

    void setName(const char* name) {
        _name = name;
    }

    void applyPositionOffset(uint16_t x, uint16_t y) {
        positionOffset.x = x;
        positionOffset.y = y;
        _touchArea.changeOrigin(x, y);
    }

    /**
     * @brief Draw the Text
     * 
     * @param forceUpdate true if also unchanged values should be drawn
     */
    void draw(bool forceUpdate = false) {
        char buffer[64];

        if (forDraw.newNeeded) {
            display.drawRect(positionOffset.x, positionOffset.y, 240/2, 320/2, DRAWIT_BLACK);
            writeTextAt(_name.c_str(), ONE_SCALE_NAME_X + positionOffset.x, ONE_SCALE_NAME_Y + positionOffset.y, ONE_SCALE_NAME_FONT);
        }
        
        if (gotGramms != forDraw.lastWrittenGramms || forDraw.newNeeded) {
            forDraw.lastWrittenGramms = gotGramms;

            display.fillRect(ONE_SCALE_KG_RECT_X + positionOffset.x, ONE_SCALE_KG_RECT_Y + positionOffset.y, ONE_SCALE_KG_RECT_W, ONE_SCALE_KG_RECT_H, DRAWIT_WHITE);

            sprintf(buffer, "%.2f%s", gotGramms / 1000, ONE_SCALE_KG_SUFFIX);
            writeTextAt(buffer, ONE_SCALE_KG_X + positionOffset.x, ONE_SCALE_KG_Y + positionOffset.y, ONE_SCALE_KG_FONT);
        }

        if (setCalFactor != forDraw.lastWrittenCal || forDraw.newNeeded) {
            forDraw.lastWrittenCal = setCalFactor;

            display.fillRect(ONE_SCALE_CAL_RECT_X + positionOffset.x, ONE_SCALE_CAL_RECT_Y + positionOffset.y, ONE_SCALE_CAL_RECT_W, ONE_SCALE_CAL_RECT_H, DRAWIT_WHITE);

            sprintf(buffer, "%.3f%s", setCalFactor, ONE_SCALE_CAL_SUFFIX);
            writeTextAt(buffer, ONE_SCALE_CAL_X + positionOffset.x, ONE_SCALE_CAL_Y + positionOffset.y, ONE_SCALE_CAL_FONT);
        }

        forDraw.newNeeded = false;
    }

    void show() {
        draw();
        _touchArea.setTouch(true);
    }

    void hide() {
        forDraw.newNeeded = true;
        _touchArea.setTouch(false);
    }

    bool touched(uint16_t x, uint16_t y) {
        return _touchArea.touched(x, y);
    }


    /**
     * @brief Loop this to update the buffer
     * 
     */
    void update() {
        while (_stream->available()) {
            char currentChar = _stream->read();

            if (currentChar == '\n') {
                parseBuffer();
            } else {
                if (currentBufferSize == STREAM_INPUT_BUFFER_LEN)
                    currentBufferSize = 0;
                
                _streamReadBuffer[currentBufferSize++] = currentChar;
            }
        }
    }

    float getGramms() {
        return gotGramms;
    }

    float getCal() {
        return setCalFactor;
    }

    float getCalF() {
        return setCalFactorFactor;
    }

    const char* getName() {
        return _name.c_str();
    }

   private:
    String _name;
    drawIt::touchArea _touchArea;
    ISimpleStream* _stream = nullptr;
    char _streamReadBuffer[STREAM_INPUT_BUFFER_LEN];
    uint16_t currentBufferSize = 0;

    // Got Values
    float gotGramms = -69000;
    float setCalFactor = 1;
    float setCalFactorFactor = 1;

    struct {
        uint16_t x = 0, y = 0;
    } positionOffset;

    struct {
        bool newNeeded = true;
        float lastWrittenGramms = -99000;
        float lastWrittenCal = -99;
    } forDraw;


    // ---------------------------------
    // From here on, only parsing Stream
    // ---------------------------------

    void parseBuffer() {
        bool parsedCorrectly = true;

        // Positions
        int16_t posW = getPosOfChar('W');
        int16_t posC = getPosOfChar('C');
        int16_t posF = getPosOfChar('F');

        // Float Temps
        float newGramms = 0;
        float newCal = 0;
        float newCalFactor = 0;

        // Error Checks
        if (posW == -1 ||
            posC == -1 ||
            posF == -1) {
            parsedCorrectly = false;
        }

        if (posC - posW < 2 ||
            posF - posC < 2 ||
            currentBufferSize - posF < 2) {
            parsedCorrectly = false;
        }

        if (parsedCorrectly) {
            bool NaNactive = false;
            NaNactive |= checkNaN(posW, posC);
            NaNactive |= checkNaN(posC, posF);
            NaNactive |= checkNaN(posF, currentBufferSize);
            if (NaNactive) {
                parsedCorrectly = false;
                gotGramms = -42000;
            }
        }

        // Parse values
        if (parsedCorrectly) {
            parsedCorrectly &= parseFloatBetween(newGramms, posW + 1, posC - 1);
            parsedCorrectly &= parseFloatBetween(newCal, posC + 1, posF - 1);
            parsedCorrectly &= parseFloatBetween(newCalFactor, posF + 1, currentBufferSize - 1);
        }

        // Copy values if correct
        if (parsedCorrectly) {
            gotGramms = newGramms;
            setCalFactor = newCal;
            setCalFactorFactor = newCalFactor;
        }

        // Reset buffer
        currentBufferSize = 0;
    }

    int16_t getPosOfChar(char charToSearch) {
        return getPosOfChar(charToSearch, 0, currentBufferSize);
    }

    int16_t getPosOfChar(char charToSearch, int16_t start, int16_t end) {
        int16_t returnVal = -1;

        for (int16_t i = start; i < end; ++i) {
            if (_streamReadBuffer[i] == charToSearch)
                return i;
        }

        return returnVal;
    }

    bool checkNaN(uint16_t start, uint16_t end) {
        if (end - start == 2) {
            if (_streamReadBuffer[start] == 'n' &&
                _streamReadBuffer[start + 1] == 'a' &&
                _streamReadBuffer[start + 2] == 'n')
                return true;
        }

        return false;
    }

    /**
     * @brief Parse Float, rangeing from start to end, both inclusive
     * 
     * @param dest 
     * @param start 
     * @param end 
     * @return true 
     * @return false 
     */
    bool parseFloatBetween(float& dest, int16_t start, int16_t end) {
        if (start > end)
            return false;
        
        float returnVal = 0;
        int16_t dotPos;
        bool sign = true; // -> false = negative, true = positive

        // First, check sign
        if (_streamReadBuffer[start] == '-'){
            sign = false;
            start++;
        } else if (_streamReadBuffer[start] == '+') {
            sign = true;
            start++;
        }

        if (start > end)
            return false;

        // Next, find dot
        dotPos = getPosOfChar('.', start, end + 1);

        // Add Places before the dot
        float currentMulti = 1;
        for (int16_t i = dotPos - 1; i >= start; --i) {
            uint8_t currentNum;
            if (getNumerical(currentNum, _streamReadBuffer[i])) {
                float currentNumFloat = currentNum;
                returnVal += currentNumFloat * currentMulti;
                currentMulti *= 10;
            } else {
                return false;
            }
        }

        // Add Places after the dot
        currentMulti = 0.1;
        for (int16_t i = dotPos + 1; i <= end; ++i) {
            uint8_t currentNum;
            if (getNumerical(currentNum, _streamReadBuffer[i])) {
                float currentNumFloat = currentNum;
                returnVal += currentNumFloat * currentMulti;
                currentMulti *= 0.1;
            } else {
                return false;
            }
        }

        // At last, add sign
        if (!sign)
            returnVal *= -1;

        // -> Parsing finished without error!
        dest = returnVal;
        return true;
    }

    bool getNumerical(uint8_t& dest, char inputChar) {
        uint8_t returnVal = 0;
        
        switch(inputChar) {
            case '0':
                returnVal = 0;
                break;

            case '1':
                returnVal = 1;
                break;

            case '2':
                returnVal = 2;
                break;
            
            case '3':
                returnVal = 3;
                break;
            
            case '4':
                returnVal = 4;
                break;
            
            case '5':
                returnVal = 5;
                break;
            
            case '6':
                returnVal = 6;
                break;
            
            case '7':
                returnVal = 7;
                break;
            
            case '8':
                returnVal = 8;
                break;
            
            case '9':
                returnVal = 9;
                break;

            default:
                return false;
        }

        dest = returnVal;
        return true;
    }
};

enum State : bool {
    State_Show_All = false,
    State_Show_One = true
};

#define SCALE_COUNT 4
WheelScale wheelScales[SCALE_COUNT];

void setup() {
    Serial.begin(9600);

    printf("TheWheelLoadScale by Timo Meyer - Display with Version: %s\n\n", SW_VERSION);

    display.begin();
    display.setRotation(0);
    display.fillScreen(DRAWIT_WHITE);

    touch.begin(display.width(), display.height(), touch.getEEPROMCalibration());
    bool stopTouched = false;

    // White -> Black
    for (uint8_t i = 17; i != 0 && !stopTouched; --i) {
        display.drawBitmap(0, 48, tigerIcon, 240, 224, Display::color565(i*15, i*15, i*15));
        stopTouched = touch.touched();
    }

    // Black -> Magenta
    for (uint8_t i = 0; i != 18 && !stopTouched; ++i) {
        display.drawBitmap(0, 48, tigerIcon, 240, 224, Display::color565(i*15, 0, i*15));
        stopTouched = touch.touched();
    }

    // Magenta -> Black
    for (uint8_t i = 17; i != 0 && !stopTouched; --i) {
        display.drawBitmap(0, 48, tigerIcon, 240, 224, Display::color565(i*15, 0, i*15));
        stopTouched = touch.touched();
    }

    // Black -> White
    for (uint8_t i = 0; i != 18 && !stopTouched; ++i) {
        display.drawBitmap(0, 48, tigerIcon, 240, 224, Display::color565(i*15, i*15, i*15));
        stopTouched = touch.touched();
    }

    printf("Starting Display\n");

    // Menu Settings
    plus.autoDraw(true);
    minus.autoDraw(true);
    up.autoDraw(true);
    down.autoDraw(true);
    back.autoDraw(true);
    tare.autoDraw(true);
    reset.autoDraw(true);

    // Serial begin's
    SerialReceiver.begin(ROUTER_SERIAL_BAUD);
    SerialFR.begin(BT_SERIAL_BAUD);
    SerialRL.begin(BT_SERIAL_BAUD);

    // Scale Displays (All Overview) Positions
    wheelScales[0].applyPositionOffset(0, 0);
    wheelScales[0].setName("Front Left");
    wheelScales[0].setStream(&streamReceiver.getStream(0));

    wheelScales[1].applyPositionOffset(240/2, 0);
    wheelScales[1].setName("Front Right");
    wheelScales[1].setStream(&streamFR);

    wheelScales[2].applyPositionOffset(0, 320/2);
    wheelScales[2].setName("Rear Left");
    wheelScales[2].setStream(&streamRL);

    wheelScales[3].applyPositionOffset(240/2, 320/2);
    wheelScales[3].setName("Rear Right");
    wheelScales[3].setStream(&streamReceiver.getStream(1));


    showAll();
    loopTimer.start();
}


State currentState = State_Show_All;
WheelScale* currentPrimaryScale = nullptr;
bool touchedBefore = false;


void loop() {
    if (currentState == State_Show_All) {
        updateAll();

        if (touch.touched()) {
            if (!touchedBefore) {
                touchedBefore = true;
                TS_Point p = touch.getPixel();

                for (uint8_t i = 0; i < SCALE_COUNT; ++i) {
                    if (wheelScales[i].touched(p.x, p.y)) {
                        currentPrimaryScale = &wheelScales[i];
                        currentState = State_Show_One;
                        hideAll();
                        showMenu();
                    }
                }
            }
        } else {
            touchedBefore = false;
        }
    }

    if (currentState == State_Show_One) {
        if (currentPrimaryScale == nullptr) {
            currentState = State_Show_All;
            hideMenu();
            showAll();
        } else {
            updateMenu();

            if (touch.touched()) {
                if (!touchedBefore) {
                    touchedBefore = true;
                    TS_Point p = touch.getPixel();

                    // Go through all buttons in the Menu
                    if (plus.touched(p.x, p.y)) {
                        currentPrimaryScale->send('+');

                    } else if (minus.touched(p.x, p.y)) {
                        currentPrimaryScale->send('-');

                    } else if (up.touched(p.x, p.y)) {
                        currentPrimaryScale->send('u');

                    } else if (down.touched(p.x, p.y)) {
                        currentPrimaryScale->send('d');

                    } else if (back.touched(p.x, p.y)) {
                        currentState = State_Show_All;
                        hideMenu();
                        showAll();

                    } else if (tare.touched(p.x, p.y)) {
                        currentPrimaryScale->send('t');

                    } else if (reset.touched(p.x, p.y)) {
                        currentPrimaryScale->send('r');
                    }
                }
            } else {
                touchedBefore = false;
            }
        }
    }

    if (loopTimer.read_ms() > LOOP_WAIT_TIME) {
        loopTimer.reset();
        loopTimer.start();
        for (uint8_t i = 0; i < SCALE_COUNT; ++i) {
            wheelScales[i].update();
        }
    }
}


void writeTextAt(const char* text, uint16_t x, uint16_t y, ILI9341_t3_font_t font) {
    display.setFont(font);
    display.setCursor(x, y);
    display.setTextColor(DRAWIT_BLACK);
    display.print(text);
}

void hideAll() {
    for (uint8_t i = 0; i < SCALE_COUNT; ++i) {
        wheelScales[i].hide();
    }
}

void showAll() {
    display.fillScreen(DRAWIT_WHITE);

    for (uint8_t i = 0; i < SCALE_COUNT; ++i) {
        wheelScales[i].show();
    }
}

void updateAll() {
    for (uint8_t i = 0; i < SCALE_COUNT; ++i) {
        wheelScales[i].draw();
    }
}

struct {
    bool newDraw = true;
    float lastGramms = 0;
    float lastCal = 0;
    float lastCalF = 0;
} forMenu;

void showMenu() {
    forMenu.newDraw = true;

    display.fillScreen(DRAWIT_WHITE);

    // First, draw all Buttons
    plus.setVisibility(true);
    minus.setVisibility(true);
    up.setVisibility(true);
    down.setVisibility(true);
    back.setVisibility(true);
    tare.setVisibility(true);
    reset.setVisibility(true);

    // Second, draw the writing of Buttons
    writeTextAt(BUTTON_BACK_WRITING, BUTTON_BACK_WRITING_X, BUTTON_BACK_WRITING_Y, MENU_BUTTON_FONT);
    writeTextAt(BUTTON_DOWN_WRITING, BUTTON_DOWN_WRITING_X, BUTTON_DOWN_WRITING_Y, MENU_BUTTON_FONT);
    writeTextAt(BUTTON_UP_WRITING, BUTTON_UP_WRITING_X, BUTTON_UP_WRITING_Y, MENU_BUTTON_FONT);
    writeTextAt(BUTTON_MINUS_WRITING, BUTTON_MINUS_WRITING_X, BUTTON_MINUS_WRITING_Y, MENU_BUTTON_FONT);
    writeTextAt(BUTTON_PLUS_WRITING, BUTTON_PLUS_WRITING_X, BUTTON_PLUS_WRITING_Y, MENU_BUTTON_FONT);
    writeTextAt(BUTTON_RESET_WRITING, BUTTON_RESET_WRITING_X, BUTTON_RESET_WRITING_Y, MENU_BUTTON_FONT);
    writeTextAt(BUTTON_TARE_WRITING, BUTTON_TARE_WRITING_X, BUTTON_TARE_WRITING_Y, MENU_BUTTON_FONT);

    // And the Name of the Scale
    if (currentPrimaryScale != nullptr) {
        writeTextAt(currentPrimaryScale->getName(), MENU_NAME_DISPLAY_X, MENU_NAME_DISPLAY_Y, MENU_NAME_DISPLAY_FONT);
    }

    // Last, add dynamic content
    updateMenu();
}

void hideMenu() {
    forMenu.newDraw = true;

    plus.setVisibility(false);
    minus.setVisibility(false);
    up.setVisibility(false);
    down.setVisibility(false);
    back.setVisibility(false);
    tare.setVisibility(false);
    reset.setVisibility(false);
}

void updateMenu() {
    if (currentPrimaryScale == nullptr)
        return;

    char buffer[64];

    // Write new values
    //   KG
    float currentGramms = currentPrimaryScale->getGramms();
    if (currentGramms != forMenu.lastGramms || forMenu.newDraw) {
        forMenu.lastGramms = currentGramms;
        display.fillRect(MENU_KG_DISPLAY_RECT_X, MENU_KG_DISPLAY_RECT_Y, MENU_KG_DISPLAY_RECT_W, MENU_KG_DISPLAY_RECT_H, DRAWIT_WHITE);

        sprintf(buffer, "%s%.2f%s", MENU_KG_DISPLAY_PREFIX, currentGramms / 1000, MENU_KG_DISPLAY_SUFFIX);
        writeTextAt(buffer, MENU_KG_DISPLAY_X, MENU_KG_DISPLAY_Y, MENU_KG_DISPLAY_FONT);
    }

    //   Cal
    float currentCal = currentPrimaryScale->getCal();
    if (currentCal != forMenu.lastCal || forMenu.newDraw) {
        forMenu.lastCal = currentCal;
        display.fillRect(MENU_CAL_DISPLAY_RECT_X, MENU_CAL_DISPLAY_RECT_Y, MENU_CAL_DISPLAY_RECT_W, MENU_CAL_DISPLAY_RECT_H, DRAWIT_WHITE);
        
        sprintf(buffer, "%s%.3f%s", MENU_CAL_DISPLAY_PREFIX, currentCal, MENU_CAL_DISPLAY_SUFFIX);
        writeTextAt(buffer, MENU_CAL_DISPLAY_X, MENU_CAL_DISPLAY_Y, MENU_CAL_DISPLAY_FONT);
    }

    //   CalF
    float currentCalF = currentPrimaryScale->getCalF();
    if (currentCalF != forMenu.lastCalF || forMenu.newDraw) {
        forMenu.lastCalF = currentCalF;
        display.fillRect(MENU_CALF_DISPLAY_RECT_X, MENU_CALF_DISPLAY_RECT_Y, MENU_CALF_DISPLAY_RECT_W, MENU_CALF_DISPLAY_RECT_H, DRAWIT_WHITE);
        
        sprintf(buffer, "%s%.3f%s", MENU_CALF_DISPLAY_PREFIX, currentCalF, MENU_CALF_DISPLAY_SUFFIX);
        writeTextAt(buffer, MENU_CALF_DISPLAY_X, MENU_CALF_DISPLAY_Y, MENU_CALF_DISPLAY_FONT);
    }


    forMenu.newDraw = false;
}