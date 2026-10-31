# HMS – Fűtésfelügyeleti rendszer

### HMS_Master
A rendszer központi egysége, amely a kazánházban található érzékelők adatait gyűjti és feldolgozza. Figyeli a fűtési rendszer hőmérsékleteit, a kazánok működését és a különböző hibajelzéseket. Az adatokat kijelzi, naplózza, valamint Wi-Fi-n keresztül elérhetővé teszi a többi eszköz és a központi felügyeleti rendszer számára. Vizuális és hangjelzésekkel figyelmeztet a rendellenes működésre.

### HMS_Display
A HMS_Master és a HMS_Kids_Thermometer adatait megjelenítő, önállóan működő kijelzőegység, amely a lakótérben helyezhető el. Érintőképernyőjén megjeleníti a fűtési rendszer aktuális állapotát, a hőmérsékleteket, a páratartalmat és a riasztásokat. Hang- és fényjelzésekkel figyelmeztet a fontosabb eseményekre. Wi-Fi-n keresztül kommunikál a többi eszközzel.

### HMS_Kids_Thermometer
Önálló, Wi-Fi-kapcsolattal rendelkező szobai hőmérséklet- és páratartalom-mérő eszköz. Az SHT30 érzékelő segítségével méri a környezeti hőmérsékletet és páratartalmat. A mért adatokat hálózaton keresztül elérhetővé teszi, így a szoba klímája távolról is ellenőrizhető. Elsősorban a gyerekszoba környezeti viszonyainak folyamatos megfigyelésére szolgál.

<p>
<img src="https://img.shields.io/badge/ESP32-E7352C?style=for-the-badge&logo=espressif&logoColor=black" alt="ESP32" />
<img src="https://img.shields.io/badge/cp%2b%2b-00599C?style=for-the-badge&logo=cplusplus&logoColor=black" alt="C%2b%2b" />
<img src="https://img.shields.io/badge/Arduino-00878F?style=for-the-badge&logo=arduino&logoColor=black" alt="Arduino" />
<img src="https://img.shields.io/badge/PlatformIO-F5822A?style=for-the-badge&logo=platformio&logoColor=black" alt="PlatformIO" />
<img src="https://img.shields.io/badge/HTML%20FastAPI-009688?style=for-the-badge&logo=data:image/svg%2bxml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAxMDAgMTAwIiB3aWR0aD0iMTAwIiBoZWlnaHQ9IjEwMCI+PHBhdGggZD0iTTc4IDQwYy0yLjMtMTIuNy0xMy44LTIyLTI2LjUtMjItOS40IDAtMTggNC43LTIzIDEyLjNDMjcgMzAgMjUuNCAzMCAyNCAzMGMtOS45IDAtMTggOC4xLTE4IDE4czguMSAxOCAxOCAxOGg1NGM4LjggMCAxNi03LjIgMTYtMTYgMC04LTUuOS0xNC43LTE0LTE2eiIgZmlsbD0iIzAwMDAwMCIvPjx0ZXh0IHg9IjUwIiB5PSI1NSIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSIyMCIgZm9udC13ZWlnaHQ9ImJvbGQiIGZpbGw9IiNGRkZGRkYiIHRleHQtYW5jaG9yPSJtaWRkbGUiPnt9PC90ZXh0Pjwvc3ZnPg==&logoColor=black" alt="HTML%20FastAPI" />
<img src="https://img.shields.io/badge/Json-000000?style=for-the-badge&logo=json&logoColor=white" alt="Json" />
<img src="https://img.shields.io/badge/I2C-00A4E4?style=for-the-badge&logo=data:image/svg%2bxml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAxMDAgMTAwIiB3aWR0aD0iNDgiIGhlaWdodD0iNDgiPgogIDxyZWN0IHg9IjQiIHk9IjQiIHdpZHRoPSI5MiIgaGVpZ2h0PSI5MiIgcng9IjE4IiByeT0iMTgiIGZpbGw9Im5vbmUiIHN0cm9rZT0iIzAwMDAwMCIgc3Ryb2tlLXdpZHRoPSI2Ii8+CiAgPGcgZm9udC1mYW1pbHk9IkFyaWFsLCBIZWx2ZXRpY2EsIHNhbnMtc2VyaWYiIGZvbnQtd2VpZ2h0PSJib2xkIj4KICAgIDx0ZXh0IHg9IjEyIiB5PSI3MyIgZm9udC1zaXplPSI1MiIgZmlsbD0iIzAwMDAwMCI+STwvdGV4dD4KICAgIDx0ZXh0IHg9IjM0IiB5PSI0NCIgZm9udC1zaXplPSIzMiIgZmlsbD0iIzAwMDAwMCI+MjwvdGV4dD4KICAgIDx0ZXh0IHg9IjU2IiB5PSI3MyIgZm9udC1zaXplPSI1MiIgZmlsbD0iIzAwMDAwMCI+QzwvdGV4dD4KICA8L2c+Cjwvc3ZnPg==&logoColor=black" alt="I2C" />
<img src="https://img.shields.io/badge/SPI-2ECC71?style=for-the-badge&logo=data:image/svg%2bxml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAxMDAgMTAwIiB3aWR0aD0iNDgiIGhlaWdodD0iNDgiPgogIDxyZWN0IHg9IjQiIHk9IjQiIHdpZHRoPSI5MiIgaGVpZ2h0PSI5MiIgcng9IjE4IiByeT0iMTgiIGZpbGw9Im5vbmUiIHN0cm9rZT0iIzAwMDAwMCIgc3Ryb2tlLXdpZHRoPSI2Ii8+CiAgPGcgZm9udC1mYW1pbHk9IkFyaWFsLCBIZWx2ZXRpY2EsIHNhbnMtc2VyaWYiIGZvbnQtd2VpZ2h0PSJib2xkIiB0ZXh0LWFuY2hvcj0ibWlkZGxlIj4KICAgIDx0ZXh0IHg9IjUwIiB5PSI2OCIgZm9udC1zaXplPSI0NCIgZmlsbD0iIzAwMDAwMCIgbGV0dGVyLXNwYWNpbmc9Ii0yIj5TUEk8L3RleHQ+CiAgPC9nPjwvc3ZnPg==&logoColor=black" alt="SPI" />
<img src="https://img.shields.io/badge/KiCad-314CB0?style=for-the-badge&logo=kicad&logoColor=black" alt="KiCad" />
<img src="https://img.shields.io/badge/Git-F03C2E?style=for-the-badge&logo=git&logoColor=black" alt="Git" />
</p>

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