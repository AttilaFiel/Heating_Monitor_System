#include <Arduino.h>
#include <SPI.h>

#include "config.h"
#include "network.h"
#include "display.h"


// --------------------------------
// Egyetlen parancs küldése
// --------------------------------

void sendCommand(int csPin, uint8_t command)
{
    digitalWrite(DISPLAY_DC, LOW);

    digitalWrite(csPin, LOW);

    SPI.transfer(command);

    digitalWrite(csPin, HIGH);
}


// --------------------------------
// Adat küldése
// --------------------------------

void sendData(int csPin, uint8_t data)
{
    digitalWrite(DISPLAY_DC, HIGH);

    digitalWrite(csPin, LOW);

    SPI.transfer(data);

    digitalWrite(csPin, HIGH);
}


// --------------------------------
// SSD1306 inicializálása
// --------------------------------

void initializeDisplay(int csPin)
{
    sendCommand(csPin, 0xAE); // Display OFF

    sendCommand(csPin, 0xD5); // Clock divide
    sendCommand(csPin, 0x80);

    sendCommand(csPin, 0xA8); // Multiplex
    sendCommand(csPin, 0x3F); // 64 sor

    sendCommand(csPin, 0xD3); // Display offset
    sendCommand(csPin, 0x00);

    sendCommand(csPin, 0x40); // Start line

    sendCommand(csPin, 0x8D); // Charge pump
    sendCommand(csPin, 0x14);

    sendCommand(csPin, 0x20); // Memory addressing mode
    sendCommand(csPin, 0x00); // Horizontal addressing

    sendCommand(csPin, 0xA1); // Segment remap

    sendCommand(csPin, 0xC8); // COM scan direction

    sendCommand(csPin, 0xDA); // COM pins
    sendCommand(csPin, 0x12);

    sendCommand(csPin, 0x81); // Contrast
    sendCommand(csPin, 0xCF);

    sendCommand(csPin, 0xD9); // Pre-charge
    sendCommand(csPin, 0xF1);

    sendCommand(csPin, 0xDB); // VCOM detect
    sendCommand(csPin, 0x40);

    sendCommand(csPin, 0xA4); // Entire display ON from RAM

    sendCommand(csPin, 0xA6); // Normal display

    sendCommand(csPin, 0xAF); // Display ON
}


// --------------------------------
// Kijelző memória teljes törlése
// --------------------------------

void clearDisplay(int csPin)
{
    sendCommand(csPin, 0x21); // Column address
    sendCommand(csPin, 0);
    sendCommand(csPin, 127);

    sendCommand(csPin, 0x22); // Page address
    sendCommand(csPin, 0);
    sendCommand(csPin, 7);


    for (int i = 0; i < 1024; i++)
    {
        sendData(csPin, 0x00);
    }
}


// --------------------------------
// Teljes képernyő kitöltése
// --------------------------------

void fillDisplay(int csPin)
{
    sendCommand(csPin, 0x21);
    sendCommand(csPin, 0);
    sendCommand(csPin, 127);

    sendCommand(csPin, 0x22);
    sendCommand(csPin, 0);
    sendCommand(csPin, 7);


    for (int i = 0; i < 1024; i++)
    {
        sendData(csPin, 0xFF);
    }
}


// --------------------------------
// Csíkos tesztkép
// --------------------------------

void stripeDisplay(int csPin)
{
    sendCommand(csPin, 0x21);
    sendCommand(csPin, 0);
    sendCommand(csPin, 127);

    sendCommand(csPin, 0x22);
    sendCommand(csPin, 0);
    sendCommand(csPin, 7);


    for (int i = 0; i < 1024; i++)
    {
        if ((i / 16) % 2 == 0)
        {
            sendData(csPin, 0xFF);
        }
        else
        {
            sendData(csPin, 0x00);
        }
    }
}


// --------------------------------
// Rom busz alakú átmenet kirajzolása
//
// radius:
// 0   = csak a középső pixel
// 95  = gyakorlatilag teljes képernyő
//
// invert = false:
// rombusz FEHÉR, háttér FEKETE
//
// invert = true:
// rombusz FEKETE, háttér FEHÉR
// --------------------------------

void drawDiamond(int csPin, int radius, bool invert)
{
    sendCommand(csPin, 0x21);
    sendCommand(csPin, 0);
    sendCommand(csPin, 127);

    sendCommand(csPin, 0x22);
    sendCommand(csPin, 0);
    sendCommand(csPin, 7);


    for (int page = 0; page < 8; page++)
    {
        for (int x = 0; x < 128; x++)
        {
            uint8_t data = 0;

            for (int bit = 0; bit < 8; bit++)
            {
                int y = page * 8 + bit;

                int distance =
                    abs(x - 63) +
                    abs(y - 31);

                bool inside =
                    distance <= radius;


                bool pixelOn;

                if (invert)
                {
                    pixelOn = !inside;
                }
                else
                {
                    pixelOn = inside;
                }


                if (pixelOn)
                {
                    data |= (1 << bit);
                }
            }

            sendData(csPin, data);
        }
    }
}


// --------------------------------
// Ket kijelzo kozotti atmenet
// --------------------------------

void diamondTransition()
{
    logMessage("ROMBUSZ ATTUNES INDUL");


    // A teljes kepernyo lefedesehez
    // elegendo sugar

    const int maxRadius = 96;


    // Kozepbol kifele

    for (int radius = 0;
         radius <= maxRadius;
         radius += 2)
    {
        // DISPLAY1:
        // fekete hatteren no a feher rombusz

        drawDiamond(
            DISPLAY1_CS,
            radius,
            false
        );


        // DISPLAY2:
        // feher hatteren no a fekete rombusz

        drawDiamond(
            DISPLAY2_CS,
            radius,
            true
        );


        delay(10);
    }


    delay(200);


    // Visszafele

    for (int radius = maxRadius;
         radius >= 0;
         radius -= 2)
    {
        drawDiamond(
            DISPLAY1_CS,
            radius,
            false
        );

        drawDiamond(
            DISPLAY2_CS,
            radius,
            true
        );

        delay(10);
    }

    clearDisplay(DISPLAY1_CS);
    clearDisplay(DISPLAY2_CS);

    logMessage("ROMBUSZ ATTUNES VEGE");
}


// --------------------------------
// Bitmap kirajzolasa
// --------------------------------

void drawBitmap(int csPin, const uint8_t* bitmap)
{
    sendCommand(csPin, 0x21);
    sendCommand(csPin, 0);
    sendCommand(csPin, 127);

    sendCommand(csPin, 0x22);
    sendCommand(csPin, 0);
    sendCommand(csPin, 7);

    for (int i = 0; i < 1024; i++)
    {
        sendData(csPin, bitmap[i]);
    }
}

void drawCat(int csPin)
{
    sendCommand(csPin, 0x21);
    sendCommand(csPin, 0);
    sendCommand(csPin, 127);

    sendCommand(csPin, 0x22);
    sendCommand(csPin, 0);
    sendCommand(csPin, 7);

    for (int page = 0; page < 8; page++)
    {
        for (int x = 0; x < 128; x++)
        {
            uint8_t data = 0;

            for (int bit = 0; bit < 8; bit++)
            {
                int y = page * 8 + bit;

                bool pixelOn = false;


                // ========================================
                // CICA FEJ
                // ========================================

                // Kerekded fej

                if (x >= 38 && x <= 89 &&
                    y >= 20 && y <= 55)
                {
                    pixelOn = true;
                }


                // Bal fül

                if (y >= 8 && y <= 28)
                {
                    int leftEdge = 38 + (28 - y) / 2;

                    if (x >= leftEdge && x <= 55)
                    {
                        pixelOn = true;
                    }
                }


                // Jobb fül

                if (y >= 8 && y <= 28)
                {
                    int rightEdge = 89 - (28 - y) / 2;

                    if (x >= 72 && x <= rightEdge)
                    {
                        pixelOn = true;
                    }
                }


                // ========================================
                // SZEMEK
                // ========================================

                // Bal szem

                if (x >= 48 && x <= 57 &&
                    y >= 29 && y <= 40)
                {
                    pixelOn = false;
                }


                // Jobb szem

                if (x >= 70 && x <= 79 &&
                    y >= 29 && y <= 40)
                {
                    pixelOn = false;
                }


                // Szemcsillanások

                if ((x >= 50 && x <= 52 &&
                     y >= 31 && y <= 33) ||

                    (x >= 72 && x <= 74 &&
                     y >= 31 && y <= 33))
                {
                    pixelOn = true;
                }


                // ========================================
                // ORR
                // ========================================

                if (y >= 42 && y <= 46 &&
                    x >= 61 && x <= 66)
                {
                    pixelOn = false;
                }


                // Orr alsó csúcsa

                if (y == 47 &&
                    x >= 62 && x <= 65)
                {
                    pixelOn = false;
                }


                // ========================================
                // MOSOLY
                // ========================================

                if (y >= 48 && y <= 50)
                {
                    if ((x >= 55 && x <= 62) ||
                        (x >= 65 && x <= 72))
                    {
                        pixelOn = false;
                    }
                }


                // ========================================
                // BAJUSZ
                // ========================================

                if ((y == 45 || y == 49 || y == 53))
                {
                    if ((x >= 22 && x <= 45) ||
                        (x >= 82 && x <= 105))
                    {
                        pixelOn = true;
                    }
                }


                if (pixelOn)
                {
                    data |= (1 << bit);
                }
            }

            sendData(csPin, data);
        }
    }
}

void drawDog(int csPin)
{
    sendCommand(csPin, 0x21);
    sendCommand(csPin, 0);
    sendCommand(csPin, 127);

    sendCommand(csPin, 0x22);
    sendCommand(csPin, 0);
    sendCommand(csPin, 7);


    for (int page = 0; page < 8; page++)
    {
        for (int x = 0; x < 128; x++)
        {
            uint8_t data = 0;

            for (int bit = 0; bit < 8; bit++)
            {
                int y = page * 8 + bit;

                bool pixelOn = false;


                // ========================================
                // FEJ KONTUR
                // ========================================

                // Felső rész

                if (y >= 14 && y <= 16 &&
                    x >= 45 && x <= 82)
                {
                    pixelOn = true;
                }

                // Bal oldal

                if (x >= 43 && x <= 46 &&
                    y >= 16 && y <= 52)
                {
                    pixelOn = true;
                }

                // Jobb oldal

                if (x >= 81 && x <= 84 &&
                    y >= 16 && y <= 52)
                {
                    pixelOn = true;
                }

                // Alsó rész

                if (y >= 52 && y <= 55 &&
                    x >= 48 && x <= 79)
                {
                    pixelOn = true;
                }


                // ========================================
                // BAL LOGO FUL
                // ========================================

                if (x >= 28 && x <= 42 &&
                    y >= 20 && y <= 48)
                {
                    // lekerekítés a fül alján
                    if (!(y >= 45 && x < 32))
                    {
                        pixelOn = true;
                    }
                }


                // ========================================
                // JOBB LOGO FUL
                // ========================================

                if (x >= 85 && x <= 99 &&
                    y >= 20 && y <= 48)
                {
                    // lekerekítés a fül alján
                    if (!(y >= 45 && x > 95))
                    {
                        pixelOn = true;
                    }
                }


                // ========================================
                // SZEMEK - kontúrosak
                // ========================================

                if ((x >= 50 && x <= 59 &&
                     y >= 27 && y <= 37) ||

                    (x >= 68 && x <= 77 &&
                     y >= 27 && y <= 37))
                {
                    pixelOn = true;
                }


                // szem belseje

                if ((x >= 53 && x <= 57 &&
                     y >= 30 && y <= 34) ||

                    (x >= 70 && x <= 74 &&
                     y >= 30 && y <= 34))
                {
                    pixelOn = false;
                }


                // ========================================
                // POFA
                // ========================================

                // Bal pofa

                if (x >= 49 && x <= 62 &&
                    y >= 39 && y <= 51)
                {
                    pixelOn = true;
                }

                // Jobb pofa

                if (x >= 65 && x <= 78 &&
                    y >= 39 && y <= 51)
                {
                    pixelOn = true;
                }


                // ========================================
                // ORR
                // ========================================

                if (y >= 40 && y <= 44 &&
                    x >= 60 && x <= 67)
                {
                    pixelOn = false;
                }

                // Orr csúcsa

                if (y == 45 &&
                    x >= 62 && x <= 65)
                {
                    pixelOn = false;
                }


                // ========================================
                // MOSOLY
                // ========================================

                if (y == 47 &&
                    x >= 57 && x <= 61)
                {
                    pixelOn = false;
                }

                if (y == 47 &&
                    x >= 66 && x <= 70)
                {
                    pixelOn = false;
                }


                // ========================================
                // NYELV
                // ========================================

                if (x >= 61 && x <= 66 &&
                    y >= 50 && y <= 58)
                {
                    pixelOn = true;
                }

                // Nyelv közepe fekete vonal

                if (x >= 63 && x <= 64 &&
                    y >= 52 && y <= 57)
                {
                    pixelOn = false;
                }


                if (pixelOn)
                {
                    data |= (1 << bit);
                }
            }

            sendData(csPin, data);
        }
    }
}


// --------------------------------
// Kijelzők inicializálása
// --------------------------------

void displaySetup()
{
    logMessage("SSD1306 DIREKT SPI TESZT");

    // CS lábak

    pinMode(DISPLAY1_CS, OUTPUT);
    pinMode(DISPLAY2_CS, OUTPUT);

    digitalWrite(DISPLAY1_CS, HIGH);
    digitalWrite(DISPLAY2_CS, HIGH);


    // Közös DC

    pinMode(DISPLAY_DC, OUTPUT);
    digitalWrite(DISPLAY_DC, LOW);


    // Közös RESET

    pinMode(DISPLAY_RESET, OUTPUT);

    digitalWrite(DISPLAY_RESET, LOW);

    delay(20);

    digitalWrite(DISPLAY_RESET, HIGH);

    delay(50);


    // SPI

    SPI.begin(
        SPI_SCK,
        SPI_MISO,
        SPI_MOSI
    );


    logMessage("DISPLAY1 inicializalasa...");

    initializeDisplay(DISPLAY1_CS);

    clearDisplay(DISPLAY1_CS);


    logMessage("DISPLAY2 inicializalasa...");

    initializeDisplay(DISPLAY2_CS);

    clearDisplay(DISPLAY2_CS);


    logMessage("Direkt SPI inicializalas kesz.");
}


// --------------------------------
// Direkt SPI teszt
// --------------------------------

void displayTest()
{
    logMessage("");
    logMessage("================================");
    logMessage("SSD1306 DIREKT SPI TESZT INDUL");
    logMessage("================================");


    // DISPLAY1 teljesen fehér
    logMessage("DISPLAY1: FEHER KEPSZORO");

    fillDisplay(DISPLAY1_CS);

    delay(500);


    // DISPLAY1 csíkos
    logMessage("DISPLAY1: CSIKOS TESZT");

    stripeDisplay(DISPLAY1_CS);

    delay(500);


    // DISPLAY1 törlés
    clearDisplay(DISPLAY1_CS);


    // DISPLAY2 teljesen fehér
    logMessage("DISPLAY2: FEHER KEPSZORO");

    fillDisplay(DISPLAY2_CS);

    delay(500);


    // DISPLAY2 csíkos
    logMessage("DISPLAY2: CSIKOS TESZT");

    stripeDisplay(DISPLAY2_CS);

    delay(500);


    // DISPLAY2 törlés
    clearDisplay(DISPLAY2_CS);

    
    diamondTransition();

    drawCat(DISPLAY1_CS);
    drawDog(DISPLAY2_CS);

    logMessage("CICA es KUTYA kirajzolva.");

    logMessage("================================");
    logMessage("SSD1306 DIREKT SPI TESZT VEGE");
    logMessage("================================");
}