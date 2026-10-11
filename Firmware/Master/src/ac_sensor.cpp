#include <Arduino.h>

#include "config.h"
#include "network.h"
#include "ac_sensor.h"


// AC jelenlét érzékelők inicializálása
void acSensorSetup()
{
    pinMode(GAS_AC_PRESENT, INPUT);

    pinMode(WOOD_AC_PRESENT, INPUT);

    logMessage("AC jelenlet erzekelok inicializalva.");
}

bool gasAcPresent()
{
    return digitalRead(GAS_AC_PRESENT) == LOW;
}


bool woodAcPresent()
{
    return digitalRead(WOOD_AC_PRESENT) == LOW;
}

// AC jelenlét érzékelők tesztje
void acSensorTest()
{
    logMessage("");
    logMessage("================================");
    logMessage("AC JELENLET ERZEKELŐ TESZT INDUL");
    logMessage("================================");


    // Bemenetek kiolvasása
    int gasAcState = digitalRead(GAS_AC_PRESENT);

    int woodAcState = digitalRead(WOOD_AC_PRESENT);


    char message[100];


    // Gázkazán áramellátás
    if (gasAcState == LOW)
    {
        logMessage("GAS_AC_PRESENT: AC JELEN VAN");
    }
    else
    {
        logMessage("GAS_AC_PRESENT: NINCS AC");
    }

    // Vegyeskazán áramellátás
    if (woodAcState == LOW)
    {
        logMessage("WOOD_AC_PRESENT: AC JELEN VAN");
    }
    else
    {
        logMessage("WOOD_AC_PRESENT: NINCS AC");
    }


    // Nyers értékek is hasznosak a bekötés ellenőrzéshez
    snprintf(
        message,
        sizeof(message),
        "GAS_AC_PRESENT nyers ertek: %d",
        gasAcState
    );

    logMessage(message);


    snprintf(
        message,
        sizeof(message),
        "WOOD_AC_PRESENT nyers ertek: %d",
        woodAcState
    );

    logMessage(message);
    
    logMessage("================================");
    logMessage("AC JELENLET ERZEKELŐ TESZT VEGE");
    logMessage("================================");
}