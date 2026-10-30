# HMS – Fűtésfelügyeleti rendszer

### HMS_Master
A rendszer központi egysége, amely a kazánházban található érzékelők adatait gyűjti és feldolgozza. Figyeli a fűtési rendszer hőmérsékleteit, a kazánok működését és a különböző hibajelzéseket. Az adatokat kijelzi, naplózza, valamint Wi-Fi-n keresztül elérhetővé teszi a többi eszköz és a központi felügyeleti rendszer számára. Vizuális és hangjelzésekkel figyelmeztet a rendellenes működésre.

### HMS_Display
A HMS_Master és a HMS_Kids_Thermometer adatait megjelenítő, önállóan működő kijelzőegység, amely a lakótérben helyezhető el. Érintőképernyőjén megjeleníti a fűtési rendszer aktuális állapotát, a hőmérsékleteket, a páratartalmat és a riasztásokat. Hang- és fényjelzésekkel figyelmeztet a fontosabb eseményekre. Wi-Fi-n keresztül kommunikál a többi eszközzel.

### HMS_Kids_Thermometer
Önálló, Wi-Fi-kapcsolattal rendelkező szobai hőmérséklet- és páratartalom-mérő eszköz. Az SHT30 érzékelő segítségével méri a környezeti hőmérsékletet és páratartalmat. A mért adatokat hálózaton keresztül elérhetővé teszi, így a szoba klímája távolról is ellenőrizhető. Elsősorban a gyerekszoba környezeti viszonyainak folyamatos megfigyelésére szolgál.

# HMS hardverlista

## HMS_Master

### Hardver:
- ESP32 DevKitC-32UE mikrokontroller
- 2× SSD1306 SPI OLED kijelzőmodul
- DS3231 RTC + AT24C32 EEPROM valós idejű óramodul
- MAX6675 K-típusú hőmérséklet-érzékelő modul
- 5× DS18B20 digitális hőmérséklet-érzékelő
- DHT11 hőmérséklet- és páratartalom-érzékelő
- microSD SPI memóriakártya-modul
- 2× MVPDM-1PHS váltakozó feszültség érzékelőmodul
- 3× közös katódos RGB LED
- Aktív hangjelző (buzzer)

### Tápellátás:
- 5 V-os tápbemenet
- Beépített 3,3 V-os feszültségszabályozó

---

## HMS_Display

### Hardver:
- ESP32 DevKitC-32UE mikrokontroller
- MSP2807 2,8"-os TFT SPI érintőképernyő-modul (ILI9341 + XPT2046)
- DS3231 RTC + AT24C32 EEPROM valós idejű óramodul
- SHT30-M hőmérséklet- és páratartalom-érzékelő
- Fényérzékelő (fotóellenállás)
- Közös katódos RGB LED
- 2× aktív hangjelző (buzzer)

### Tápellátás:
- 5 V-os USB-tápbemenet
- Beépített 3,3 V-os feszültségszabályozó

---

## HMS_Kids_Thermometer

### Hardver:
- ESP32-C3-SUPERMINI mikrokontroller
- SHT30 hőmérséklet- és páratartalom-érzékelő

### Tápellátás:
- 5 V-os tápbemenet
- Beépített 3,3 V-os feszültségszabályozó