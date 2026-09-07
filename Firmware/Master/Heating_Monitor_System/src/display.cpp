#include <Arduino.h>
#include <SPI.h>

#include "config.h"
#include "network.h"
#include "display.h"
#include "system_data.h"

// --------------------------------
// Szövegkirajzolás
// --------------------------------

// 128 x 64 pixel = 1024 byte
uint8_t displayBuffer[1024];


// --------------------------------
// 5x7 karakterkészlet
// --------------------------------

struct Glyph
{
    uint32_t code;
    uint8_t data[5];
};


// Alap ASCII karakterkészlet

const Glyph font[] =
{
    // Számok
    { '0', {0x3E,0x51,0x49,0x45,0x3E} },
    { '1', {0x00,0x42,0x7F,0x40,0x00} },
    { '2', {0x42,0x61,0x51,0x49,0x46} },
    { '3', {0x21,0x41,0x45,0x4B,0x31} },
    { '4', {0x18,0x14,0x12,0x7F,0x10} },
    { '5', {0x27,0x45,0x45,0x45,0x39} },
    { '6', {0x3C,0x4A,0x49,0x49,0x30} },
    { '7', {0x01,0x71,0x09,0x05,0x03} },
    { '8', {0x36,0x49,0x49,0x49,0x36} },
    { '9', {0x06,0x49,0x49,0x29,0x1E} },

    // Nagybetűk
    { 'A', {0x7E,0x11,0x11,0x11,0x7E} },
    { 'B', {0x7F,0x49,0x49,0x49,0x36} },
    { 'C', {0x3E,0x41,0x41,0x41,0x22} },
    { 'D', {0x7F,0x41,0x41,0x22,0x1C} },
    { 'E', {0x7F,0x49,0x49,0x49,0x41} },
    { 'F', {0x7F,0x09,0x09,0x09,0x01} },
    { 'G', {0x3E,0x41,0x49,0x49,0x7A} },
    { 'H', {0x7F,0x08,0x08,0x08,0x7F} },
    { 'I', {0x00,0x41,0x7F,0x41,0x00} },
    { 'J', {0x20,0x40,0x41,0x3F,0x01} },
    { 'K', {0x7F,0x08,0x14,0x22,0x41} },
    { 'L', {0x7F,0x40,0x40,0x40,0x40} },
    { 'M', {0x7F,0x02,0x0C,0x02,0x7F} },
    { 'N', {0x7F,0x04,0x08,0x10,0x7F} },
    { 'O', {0x3E,0x41,0x41,0x41,0x3E} },
    { 'P', {0x7F,0x09,0x09,0x09,0x06} },
    { 'Q', {0x3E,0x41,0x51,0x21,0x5E} },
    { 'R', {0x7F,0x09,0x19,0x29,0x46} },
    { 'S', {0x46,0x49,0x49,0x49,0x31} },
    { 'T', {0x01,0x01,0x7F,0x01,0x01} },
    { 'U', {0x3F,0x40,0x40,0x40,0x3F} },
    { 'V', {0x1F,0x20,0x40,0x20,0x1F} },
    { 'W', {0x7F,0x20,0x18,0x20,0x7F} },
    { 'X', {0x63,0x14,0x08,0x14,0x63} },
    { 'Y', {0x03,0x04,0x78,0x04,0x03} },
    { 'Z', {0x61,0x51,0x49,0x45,0x43} },

    // Kisbetűk
    { 'a', {0x20,0x54,0x54,0x54,0x78} },
    { 'b', {0x7F,0x48,0x44,0x44,0x38} },
    { 'c', {0x38,0x44,0x44,0x44,0x20} },
    { 'd', {0x38,0x44,0x44,0x48,0x7F} },
    { 'e', {0x38,0x54,0x54,0x54,0x18} },
    { 'f', {0x08,0x7E,0x09,0x01,0x02} },
    { 'g', {0x0C,0x52,0x52,0x52,0x3E} },
    { 'h', {0x7F,0x08,0x04,0x04,0x78} },
    { 'i', {0x00,0x44,0x7D,0x40,0x00} },
    { 'j', {0x20,0x40,0x44,0x3D,0x00} },
    { 'k', {0x7F,0x10,0x28,0x44,0x00} },
    { 'l', {0x00,0x41,0x7F,0x40,0x00} },
    { 'm', {0x7C,0x04,0x18,0x04,0x78} },
    { 'n', {0x7C,0x08,0x04,0x04,0x78} },
    { 'o', {0x38,0x44,0x44,0x44,0x38} },
    { 'p', {0x7C,0x14,0x14,0x14,0x08} },
    { 'q', {0x08,0x14,0x14,0x18,0x7C} },
    { 'r', {0x7C,0x08,0x04,0x04,0x08} },
    { 's', {0x48,0x54,0x54,0x54,0x20} },
    { 't', {0x04,0x3F,0x44,0x40,0x20} },
    { 'u', {0x3C,0x40,0x40,0x20,0x7C} },
    { 'v', {0x1C,0x20,0x40,0x20,0x1C} },
    { 'w', {0x3C,0x40,0x30,0x40,0x3C} },
    { 'x', {0x44,0x28,0x10,0x28,0x44} },
    { 'y', {0x0C,0x50,0x50,0x50,0x3C} },
    { 'z', {0x44,0x64,0x54,0x4C,0x44} },

    // Írásjelek
    { ' ', {0x00, 0x00, 0x00, 0x00, 0x00} },
    { '.', {0x00, 0x00, 0x40, 0x00, 0x00} },
    { ',', {0x00, 0x60, 0x40, 0x00, 0x00} },
    { ':', {0x00, 0x44, 0x00, 0x00, 0x00} },
    { '-', {0x08, 0x08, 0x08, 0x08, 0x08} },
    { '_', {0x40, 0x40, 0x40, 0x40, 0x40} },
    { '+', {0x08, 0x08, 0x7f, 0x08, 0x08} },
    { '/', {0x20, 0x10, 0x08, 0x04, 0x02} },
    { '%', {0x63, 0x13, 0x08, 0x64, 0x63} },
    { '(', {0x00, 0x1C, 0x22, 0x41, 0x00} },
    { ')', {0x00, 0x41, 0x22, 0x1C, 0x00} },
    { '?', {0x02, 0x01, 0x51, 0x09, 0x06} },
    { 0xB0, {0x00, 0x00, 0x02, 0x05, 0x02} }, // fokjel: °C
};

const int FONT_COUNT = sizeof(font) / sizeof(font[0]);

// --------------------------------
// Alap karakter keresése
// --------------------------------

const Glyph* findGlyph(char character)
{
    for (int i = 0; i < FONT_COUNT; i++)
    {
        if (font[i].code == character)
        {
            return &font[i];
        }
    }

    return nullptr;
}


// --------------------------------
// Pixel beállítása a framebufferben
// --------------------------------

void setPixel(int x, int y, bool state)
{
    if (x < 0 || x >= 128 ||
        y < 0 || y >= 64)
    {
        return;
    }

    int index = x + (y / 8) * 128;
    uint8_t mask = 1 << (y % 8);

    if (state)
    {
        displayBuffer[index] |= mask;
    }
    else
    {
        displayBuffer[index] &= ~mask;
    }
}


// --------------------------------
// Framebuffer törlése
// --------------------------------

void clearDisplayBuffer()
{
    memset(displayBuffer, 0, sizeof(displayBuffer));
}


// --------------------------------
// UTF-8 karakter dekódolása
// --------------------------------

uint32_t decodeUTF8(const char* text, int& index)
{
    uint8_t c = text[index];

    // ASCII
    if (c < 0x80)
    {
        index++;
        return c;
    }

    // 2 byte-os UTF-8
    if ((c & 0xE0) == 0xC0)
    {
        uint32_t result =
            ((c & 0x1F) << 6) |
            (text[index + 1] & 0x3F);

        index += 2;
        return result;
    }

    // 3 byte-os UTF-8
    if ((c & 0xF0) == 0xE0)
    {
        uint32_t result =
            ((c & 0x0F) << 12) |
            ((text[index + 1] & 0x3F) << 6) |
            (text[index + 2] & 0x3F);

        index += 3;
        return result;
    }

    // Hibás / nem támogatott karakter
    index++;
    return '?';
}


// --------------------------------
// Unicode karakter alapbetűje
// + ékezet típusa
// --------------------------------
//
// accent:
// 0 = nincs
// 1 = egy vessző
// 2 = két pont
// 3 = hosszú dupla ékezet
//

char unicodeToBase(uint32_t code, uint8_t &accent)
{
    accent = 0;

    switch (code)
    {
        // Kisbetűk
        case 0xE1: // á
            accent = 1;
            return 'a';

        case 0xE9: // é
            accent = 1;
            return 'e';

        case 0xED: // í
            accent = 1;
            return 'i';

        case 0xF3: // ó
            accent = 1;
            return 'o';

        case 0xF6: // ö
            accent = 2;
            return 'o';

        case 0x151: // ő
            accent = 3;
            return 'o';

        case 0xFA: // ú
            accent = 1;
            return 'u';

        case 0xFC: // ü
            accent = 2;
            return 'u';

        case 0x171: // ű
            accent = 3;
            return 'u';


        // Nagybetűk
        case 0xC1: // Á
            accent = 1;
            return 'A';

        case 0xC9: // É
            accent = 1;
            return 'E';

        case 0xCD: // Í
            accent = 1;
            return 'I';

        case 0xD3: // Ó
            accent = 1;
            return 'O';

        case 0xD6: // Ö
            accent = 2;
            return 'O';

        case 0x150: // Ő
            accent = 3;
            return 'O';

        case 0xDA: // Ú
            accent = 1;
            return 'U';

        case 0xDC: // Ü
            accent = 2;
            return 'U';

        case 0x170: // Ű
            accent = 3;
            return 'U';

        default:
            return (char)code;
    }
}


// --------------------------------
// Ékezet kirajzolása
// --------------------------------

void drawAccent(int x, int y, uint8_t accent)
{
    switch (accent)
    {
        // Éles ékezet: á é í ó ú
        case 1:
            setPixel(x + 4, y, true);
            setPixel(x + 3, y + 1, true);
            break;

        // Két pont: ö ü
        case 2:
            setPixel(x + 1, y, true);
            setPixel(x + 3, y, true);
            break;

        // Kettős ékezet: ő ű
        case 3:
            setPixel(x + 4, y, true);
            setPixel(x + 3, y + 1, true);
            setPixel(x + 2, y, true);
            setPixel(x + 1, y + 1, true);
            break;
    }
}


// --------------------------------
// Egy Unicode karakter kirajzolása
// --------------------------------

void drawUnicodeChar(
    uint32_t code,
    int x,
    int y,
    uint8_t scale
)
{
    // ---------------------------------------------------------
    // Ékezet-eltolások
    // ---------------------------------------------------------

    struct AccentOffset
    {
        int x;
        int y;
    };

    // {vízszint,függőleges}
    // Kisbetű
    AccentOffset smallSingle = {0, 1};   // á é í ó ú
    AccentOffset smallDouble = {0, 2};   // ö ü
    AccentOffset smallLong   = {0, 1};   // ő ű

    // Nagybetű
    AccentOffset upperSingle = {0, 0};   // Á É Í Ó Ú
    AccentOffset upperDouble = {0, 0};   // Ö Ü
    AccentOffset upperLong   = {0, 0};   // Ő Ű


    // ---------------------------------------------------------
    // Alapbetű és ékezet meghatározása
    // ---------------------------------------------------------

    uint8_t accent = 0;

    char baseCharacter =
        unicodeToBase(code, accent);

    const Glyph* glyph =
        findGlyph(baseCharacter);

    if (glyph == nullptr)
    {
        glyph = findGlyph('?');

        if (glyph == nullptr)
        {
            return;
        }
    }


    // ---------------------------------------------------------
    // Alapbetű kirajzolása
    // ---------------------------------------------------------

    for (int column = 0; column < 5; column++)
    {
        uint8_t columnData =
            glyph->data[column];

        for (int row = 0; row < 7; row++)
        {
            if (columnData & (1 << row))
            {
                for (int dx = 0; dx < scale; dx++)
                {
                    for (int dy = 0; dy < scale; dy++)
                    {
                        setPixel(
                            x + column * scale + dx,
                            y + (row + 2) * scale + dy,
                            true
                        );
                    }
                }
            }
        }
    }


    // ---------------------------------------------------------
    // Ékezet kirajzolása
    // ---------------------------------------------------------

    if (accent != 0)
    {
        bool isUppercase =
            (baseCharacter >= 'A' &&
             baseCharacter <= 'Z');

        AccentOffset offset = {0, 0};


        // Egyszeres ékezet
        // á é í ó ú / Á É Í Ó Ú
        if (accent == 1)
        {
            if (isUppercase)
            {
                offset = upperSingle;
            }
            else
            {
                offset = smallSingle;
            }
        }


        // Két pont
        // ö ü / Ö Ü
        else if (accent == 2)
        {
            if (isUppercase)
            {
                offset = upperDouble;
            }
            else
            {
                offset = smallDouble;
            }
        }


        // Dupla ékezet
        // ő ű / Ő Ű
        else if (accent == 3)
        {
            if (isUppercase)
            {
                offset = upperLong;
            }
            else
            {
                offset = smallLong;
            }
        }


        // -----------------------------------------------------
        // Ékezetek kirajzolása
        // -----------------------------------------------------

        switch (accent)
        {
            // -------------------------------------------------
            // Egyszeres ékezet
            // -------------------------------------------------

            case 1:

                for (int dx = 0; dx < scale; dx++)
                {
                    for (int dy = 0; dy < scale; dy++)
                    {
                        setPixel(
                            x +
                            3 * scale +
                            offset.x * scale +
                            dx,

                            y +
                            offset.y * scale +
                            dy,

                            true
                        );

                        setPixel(
                            x +
                            2 * scale +
                            offset.x * scale +
                            dx,

                            y +
                            scale +
                            offset.y * scale +
                            dy,

                            true
                        );
                    }
                }

                break;


            // -------------------------------------------------
            // Két pont
            // -------------------------------------------------

            case 2:

                for (int dx = 0; dx < scale; dx++)
                {
                    for (int dy = 0; dy < scale; dy++)
                    {
                        setPixel(
                            x +
                            1 * scale +
                            offset.x * scale +
                            dx,

                            y +
                            offset.y * scale +
                            dy,

                            true
                        );

                        setPixel(
                            x +
                            3 * scale +
                            offset.x * scale +
                            dx,

                            y +
                            offset.y * scale +
                            dy,

                            true
                        );
                    }
                }

                break;


            // -------------------------------------------------
            // Dupla ékezet
            // -------------------------------------------------

            case 3:

                for (int dx = 0; dx < scale; dx++)
                {
                    for (int dy = 0; dy < scale; dy++)
                    {
                        setPixel(
                            x +
                            4 * scale +
                            offset.x * scale +
                            dx,

                            y +
                            offset.y * scale +
                            dy,

                            true
                        );

                        setPixel(
                            x +
                            3 * scale +
                            offset.x * scale +
                            dx,

                            y +
                            scale +
                            offset.y * scale +
                            dy,

                            true
                        );

                        setPixel(
                            x +
                            2 * scale +
                            offset.x * scale +
                            dx,

                            y +
                            offset.y * scale +
                            dy,

                            true
                        );

                        setPixel(
                            x +
                            1 * scale +
                            offset.x * scale +
                            dx,

                            y +
                            scale +
                            offset.y * scale +
                            dy,

                            true
                        );
                    }
                }

                break;
        }
    }
}

// --------------------------------
// UTF-8 szöveg kirajzolása
// --------------------------------

void drawText(
    int x,
    int y,
    const char* text,
    uint8_t scale
)
{
    int cursorX = x;
    int cursorY = y;

    while (*text)
    {
        uint8_t c = (uint8_t)*text;

        // ASCII karakter
        if (c < 128)
        {
            drawUnicodeChar(
                c,
                cursorX,
                cursorY,
                scale
            );

            cursorX += 6 * scale;
            text++;
        }

        // UTF-8 kétbájtos karakter
        else if ((c & 0xE0) == 0xC0)
        {
            uint32_t code =
                ((uint32_t)(c & 0x1F) << 6) |
                ((uint32_t)(text[1] & 0x3F));

            drawUnicodeChar(
                code,
                cursorX,
                cursorY,
                scale
            );

            cursorX += 6 * scale;
            text += 2;
        }

        // UTF-8 hárombájtos karakter
        else if ((c & 0xF0) == 0xE0)
        {
            uint32_t code =
                ((uint32_t)(c & 0x0F) << 12) |
                ((uint32_t)(text[1] & 0x3F) << 6) |
                ((uint32_t)(text[2] & 0x3F));

            drawUnicodeChar(
                code,
                cursorX,
                cursorY,
                scale
            );

            cursorX += 6 * scale;
            text += 3;
        }

        else
        {
            text++;
        }
    }
}


// --------------------------------
// Framebuffer elküldése OLED-re
// --------------------------------
void sendCommand(int csPin, uint8_t command);
void sendData(int csPin, uint8_t data);

void updateDisplayFromBuffer(int csPin)
{
    sendCommand(csPin, 0x21);
    sendCommand(csPin, 0);
    sendCommand(csPin, 127);

    sendCommand(csPin, 0x22);
    sendCommand(csPin, 0);
    sendCommand(csPin, 7);


    for (int i = 0; i < 1024; i++)
    {
        sendData(
            csPin,
            displayBuffer[i]
        );
    }
}


// --------------------------------
// Ékezetes szöveg teszt
// --------------------------------

void displayTextTest()
{
    clearDisplayBuffer();

    drawText(
        0,
        0,
        "Fűtésrendszer",
        1
    );

    drawText(
        0,
        10,
        "ÁÉÍÓÖŐÚÜŰ",
        1
    );

    drawText(
        0,
        20,
        "áéíóöőúüű",
        1
    );

    drawText(
        0,
        30,
        "Hőmérséklet: 60.5",
        1
    );

    //drawText(
    //    0,
    //    40,
    //    "Visszatérő: 45.2",
    //    1
    //);

    drawText(
        0,
        40,
        "Fűtés OK",
        2
    );


    updateDisplayFromBuffer(
        DISPLAY1_CS
    );

    updateDisplayFromBuffer(
        DISPLAY2_CS
    );
}


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

    //fillDisplay(DISPLAY1_CS);

    //delay(500);


    // DISPLAY1 csíkos
    logMessage("DISPLAY1: CSIKOS TESZT");

    //stripeDisplay(DISPLAY1_CS);

    //delay(500);


    // DISPLAY1 törlés
    clearDisplay(DISPLAY1_CS);


    // DISPLAY2 teljesen fehér
    logMessage("DISPLAY2: FEHER KEPSZORO");

    //fillDisplay(DISPLAY2_CS);

    //delay(500);


    // DISPLAY2 csíkos
    logMessage("DISPLAY2: CSIKOS TESZT");

    //stripeDisplay(DISPLAY2_CS);

    //delay(500);


    // DISPLAY2 törlés
    clearDisplay(DISPLAY2_CS);

    
    //diamondTransition();

    //drawCat(DISPLAY1_CS);
    //drawDog(DISPLAY2_CS);

    logMessage("CICA es KUTYA kirajzolva.");

    

    // --------------------------------
    // Magyar szöveg teszt
    // --------------------------------

    logMessage("MAGYAR SZOVEG TESZT");

    displayTextTest();
    delay(1000);
    logMessage("MAGYAR SZOVEG TESZT VEGE");

    logMessage("================================");
    logMessage("SSD1306 DIREKT SPI TESZT VEGE");
    logMessage("================================");
}

const char* systemStateToString(SystemState state)
{
    switch (state)
    {
        case SYSTEM_OFF:
            return "KI";

        case SYSTEM_GAS:
            return "Gáz";

        case SYSTEM_WOOD:
            return "Fa";

        case SYSTEM_FIRE_WITHOUT_CIRCULATOR:
            return "Tűz!";

        case SYSTEM_STARTUP:
            return "Inditás";

        case SYSTEM_FAULT:
            return "HIBA";

        default:
            return "?";
    }
}

const char* alarmStateToString(AlarmState state)
{
    switch (state)
    {
        case ALARM_NONE:
            return "OK";

        case ALARM_WARNING:
            return "FIGYELM";

        case ALARM_CRITICAL:
            return "RIADÓ";

        default:
            return "?";
    }
}

const char* alarmReasonRawToString(AlarmReason reason)
{
    switch (reason)
    {
        case ALARM_REASON_NONE:
            return "REASON_NONE";

        case ALARM_REASON_BOTH_BRANCHES:
            return "BOTH_BRANCHES";

        case ALARM_REASON_FIRE_WITHOUT_CIRCULATOR:
            return "FIRE_WITHOUT_CIRCULATOR";

        case ALARM_REASON_WOOD_BOILER_OVERHEAT:
            return "WOOD_BOILER_OVERHEAT";

        case ALARM_REASON_STARTUP_FAULT:
            return "STARTUP_FAULT";

        case ALARM_REASON_SD_WARNING:
            return "SD_WARNING";

        case ALARM_REASON_SD_FULL:
            return "SD_FULL";

        case ALARM_REASON_STARTUP_WARNING:
            return "STARTUP_WARNING";

        case ALARM_REASON_WOOD_BOILER_WARNING:
            return "WOOD_BOILER_WARNING";

        default:
            return "ALARM_REASON_UNKNOWN";
    }
}

const char* alarmReasonToString(AlarmReason reason)
{
    switch (reason)
    {
        case ALARM_REASON_NONE:
            return "Nincs";

        case ALARM_REASON_BOTH_BRANCHES:
            return "Gáz+fa ág egyszerre";

        case ALARM_REASON_FIRE_WITHOUT_CIRCULATOR:
            return "Tűz!, nincs keringető";

        case ALARM_REASON_WOOD_BOILER_OVERHEAT:
            return "Kazán túlmelegedés";

        case ALARM_REASON_STARTUP_FAULT:
            return "Indítás sikertelen";

        case ALARM_REASON_SD_WARNING:
            return "SD kártya eltávolítva";

        case ALARM_REASON_SD_FULL:
            return "SD kártya megtelt";

        case ALARM_REASON_STARTUP_WARNING:
            return "Indítás megszakadt";

        case ALARM_REASON_WOOD_BOILER_WARNING:
            return "Kazán hő magas";

        default:
            return "Ismeretlen";
    }
}

void displayUpdate()
{
    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate < 500)
    {
        return;
    }

    lastUpdate = millis();

    // =========================================================
    // DISPLAY 1
    // =========================================================

    clearDisplayBuffer();

    // 1. sor – Aktív ág
    switch (systemData.activeBranch)
    {
        case HEATING_GAS:
            drawText(0, 0, "Gázkazán", 2);
            break;

        case HEATING_WOOD:
            drawText(0, 0, "Vegyes", 2);
            break;

        case HEATING_NONE:
            drawText(0, 0, "Üzemen", 2);
            drawText(0, 20, "kívül", 2);
            break;

        case HEATING_ERROR:
            drawText(0, 0, "HIBA", 2);
            break;

        default:
            drawText(0, 0, "HIBA", 2);
            break;
    }


    // 2. sor – Üzemtől függő hőmérséklet
    if (systemData.activeBranch == HEATING_GAS)
    {
        char text[32];

        snprintf(
            text,
            sizeof(text),
            "Előre:%.0f°C",
            systemData.gasFlowTemperature
        );

        drawText(0, 20, text, 2);
    }
    else if (systemData.activeBranch == HEATING_WOOD)
    {
        char text[32];

        snprintf(
            text,
            sizeof(text),
            "Kazán:%.0f°C",
            systemData.woodBoilerTemperature
        );

        drawText(0, 20, text, 2);
    }
    else
    {
        //drawText(0, 20, "--.-°C", 2);
    }


    // 4. sor – Státusz / riasztás
    char statusText[32];

    snprintf(
        statusText,
        sizeof(statusText),
        "%s/%s",
        systemStateToString(systemData.systemState),
        alarmStateToString(systemData.alarmState)
    );

    drawText(0, 40, statusText, 2);

    updateDisplayFromBuffer(DISPLAY1_CS);


    // =========================================================
    // DISPLAY 2
    // =========================================================

    clearDisplayBuffer();

    char text[32];


    // 1. sor – Előremenő
    if (systemData.activeBranch == HEATING_GAS)
    {
        snprintf(
            text,
            sizeof(text),
            "Előre:%.1f°C",
            systemData.gasFlowTemperature
        );
    }
    else
    {
        snprintf(
            text,
            sizeof(text),
            "Előre:%.1f°C",
            systemData.woodFlowTemperature
        );
    }

    drawText(0, 0, text, 1);


    // 2. sor – Visszatérő
    if (systemData.activeBranch == HEATING_GAS)
    {
        snprintf(
            text,
            sizeof(text),
            "Vissza:%.1f°C",
            systemData.gasReturnTemperature
        );
    }
    else
    {
        snprintf(
            text,
            sizeof(text),
            "Vissza:%.1f°C",
            systemData.woodReturnTemperature
        );
    }

    drawText(0, 10, text, 1);


    // 3. sor – Füstgáz
    snprintf(
        text,
        sizeof(text),
        "Füstgáz:%.1f°C",
        systemData.chimneyTemperature
    );

    drawText(0, 20, text, 1);


    // 4. és 5. sor – Alarm oka
    if (systemData.alarmState != ALARM_NONE)
    {
        snprintf(
            text,
            sizeof(text),
            "%s",
            alarmReasonRawToString(systemData.alarmReason)
        );

        drawText(0, 30, text, 1);

        snprintf(
            text,
            sizeof(text),
            "%s",
            alarmReasonToString(systemData.alarmReason)
        );

        drawText(0, 40, text, 1);
    }
    else
    {
        // 4. sor – Kazánház
        snprintf(
            text,
            sizeof(text),
            "Kazánház:%.1f°C",
            systemData.boilerRoomTemperature
        );

        drawText(0, 30, text, 1);

        // 5. sor – Páratartalom
        snprintf(
            text,
            sizeof(text),
            "Páratartalom:%.0f%%",
            systemData.boilerRoomHumidity
        );

        drawText(0, 40, text, 1);
    }


    // 6. sor – Dátum / idő
    snprintf(
        text,
        sizeof(text),
        "%04d.%02d.%02d %02d:%02d",
        systemData.timestamp.year(),
        systemData.timestamp.month(),
        systemData.timestamp.day(),
        systemData.timestamp.hour(),
        systemData.timestamp.minute()
    );

    drawText(0, 50, text, 1);

    updateDisplayFromBuffer(DISPLAY2_CS);
}