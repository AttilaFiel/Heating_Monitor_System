# HMS HTTP/REST API szerződés

- **Projekt:** Heating Monitor System (HMS)
- **Dokumentum típusa:** API-szerződés / implementációs alap
- **Verzió:** 1.0 – tervezett szerződés
- **Állapot:** jóváhagyásra és implementáció előtti ellenőrzésre
- **Érintett eszköz:** HMS-Master ESP32
- **Fogyasztók:** HMS-Display ESP32, Icinga/Nagios, későbbi olvasó kliensek

## 1. Cél és hatókör

A HMS-Master aktuális, már feldolgozott mérési és rendszerállapotait egységes, csak olvasható HTTP API-n keresztül teszi elérhetővé.

Az API kizárólag megfigyelési célú. Nem tartalmazhat fűtésvezérlő, relét kapcsoló, riasztást nyugtázó, konfigurációt módosító vagy Mastert újraindító műveletet.

A szenzorolvasás, adatfeldolgozás, helyi kijelzés, helyi riasztáskezelés és naplózás továbbra is a Master meglévő feladata marad. A HTTP-kérés nem indíthat új mérést.

## 2. Alapelvek

1. **Egyetlen adatforrás:** a válasz a Master meglévő `SystemData` adatmodelljéből készül.
2. **Közös adat-előállítás:** a régi `/status` és az új `/api/v1/status` ne tartalmazzon egymástól eltérő, duplikált adatleképezési logikát.
3. **Visszafelé kompatibilitás:** a meglévő HMS LOG weboldal `/status` végpontja az átállás során megmarad, jelenlegi formátumát nem szabad észrevétlenül megváltoztatni.
4. **Gépi értékek:** az új API állapotai stabil, nyelvfüggetlen azonosítókkal szerepelnek. A megjelenítési szöveget a kliens állítja elő.
5. **Érvényesség és frissesség:** a szenzorhiba és az elavult mérés külön állapot.
6. **Nem blokkoló működés:** az API nem akadályozhatja a mérési ciklust, a watchdogot, a naplózást vagy a helyi riasztásokat.
7. **Nincs vezérlés:** minden új API-végpont olvasási célú; az első verzió csak `GET` kéréseket használ.

## 3. Végpontok

### 3.1 `GET /api/v1/status`

Az aktuális méréseket, feldolgozott állapotokat, szenzorhibákat és diagnosztikai információkat adja vissza.

- Sikeres válasz: `200 OK`
- Tartalomtípus: `application/json`
- Nem használható mérés indítására vagy a Master állapotának módosítására.
- A válasz egy konzisztens pillanatképet képviseljen.
- A JSON válasz mérete maradjon korlátozott és kiszámítható.

### 3.2 `GET /api/v1/health`

Az API és a Master alapvető elérhetőségét jelzi.

- Sikeres válasz: `200 OK`
- Tartalomtípus: `application/json`
- A sikeres health válasz **nem jelenti automatikusan azt**, hogy minden szenzor érvényes vagy az összes adat friss. Ezeket a `/api/v1/status` válaszból kell megállapítani.

Javasolt válasz:

```json
{
  "schemaVersion": 1,
  "api": "ok",
  "uptimeSeconds": 86400
}
```

Az `uptimeSeconds` az ESP32 újraindítása óta eltelt idő. A számláló monoton, futásidő-alapú forrásból származzon, ne az RTC órájából.

## 4. `GET /api/v1/status` válaszséma

Az alábbi mezőnevek a tervezett API-szerződés részei. A megvalósítás előtt ellenőrizni kell őket a `http_communication_dev` branch tényleges `SystemData` és adatfrissítési kódja ellenében.

### 4.1 Felső szintű mezők

| Mező | Típus | Kötelező | Jelentés |
|---|---|---:|---|
| `schemaVersion` | integer | igen | A JSON-szerződés verziója; első verzióban `1`. |
| `timestamp` | string vagy `null` | igen | A Master RTC szerinti idő, ISO 8601 formátumban, például `2026-10-10T13:20:00+02:00`, ha az időzóna bizonyítottan ismert. Ha nem ismert, ne találjunk ki eltolást; használjunk dokumentált helyi időformátumot vagy `null` értéket. |
| `uptimeSeconds` | integer | igen | Az ESP32 újraindítása óta eltelt másodpercek. |
| `dataAgeMs` | integer vagy `null` | igen | Az utolsó sikeres, releváns adatfrissítés óta eltelt idő milliszekundumban. Csak akkor állítható elő, ha az implementáció tényleges frissítési eseményt rögzít. |
| `systemState` | string | igen | A `SystemState` stabil gépi azonosítója. |
| `activeBranch` | string | igen | A `HeatingBranch` stabil gépi azonosítója. |
| `alarmState` | string | igen | A `AlarmState` stabil gépi azonosítója. |
| `alarmReason` | string | igen | A `AlarmReason` stabil gépi azonosítója. |
| `firePresent` | boolean | igen | A feldolgozott tűzjelenlét-állapot. |
| `gasAcPresent` | boolean | igen | A meglévő HMS adatmodell szerinti gázos ág AC-állapota. |
| `woodAcPresent` | boolean | igen | A meglévő HMS adatmodell szerinti fatüzelésű ág AC-állapota. |
| `measurements` | object | igen | A nyolc meglévő mérési érték. |
| `sensorErrors` | object | igen | A hét meglévő szenzorhiba-jelző. |
| `diagnostics` | object | igen | Az API-hoz szükséges, elérhető diagnosztikai mezők. |

### 4.2 Stabil enum-azonosítók

A kliensek ne a jelenlegi magyar kijelzőszövegekre épüljenek.

**`systemState` – `SystemState`:**

- `SYSTEM_OFF`
- `SYSTEM_GAS`
- `SYSTEM_WOOD`
- `SYSTEM_FIRE_WITHOUT_CIRCULATOR`
- `SYSTEM_STARTUP`
- `SYSTEM_FAULT`
- `UNKNOWN` – csak nem várt vagy feldolgozhatatlan enumérték esetén

**`activeBranch` – `HeatingBranch`:**

- `HEATING_NONE`
- `HEATING_GAS`
- `HEATING_WOOD`
- `HEATING_ERROR`
- `UNKNOWN`

**`alarmState` – `AlarmState`:**

- `ALARM_NONE`
- `ALARM_WARNING`
- `ALARM_CRITICAL`
- `UNKNOWN`

**`alarmReason` – `AlarmReason`:**

- `ALARM_REASON_NONE`
- `ALARM_REASON_BOTH_BRANCHES`
- `ALARM_REASON_FIRE_WITHOUT_CIRCULATOR`
- `ALARM_REASON_WOOD_BOILER_OVERHEAT`
- `ALARM_REASON_STARTUP_FAULT`
- `ALARM_REASON_RTC_BATTERY`
- `ALARM_REASON_RTC_OSCILLATOR_STOP`
- `ALARM_REASON_SD_WARNING`
- `ALARM_REASON_SD_FULL`
- `ALARM_REASON_STARTUP_WARNING`
- `ALARM_REASON_WOOD_BOILER_WARNING`
- `ALARM_REASON_GAS_FLOW_SENSOR`
- `ALARM_REASON_GAS_RETURN_SENSOR`
- `ALARM_REASON_WOOD_FLOW_SENSOR`
- `ALARM_REASON_WOOD_RETURN_SENSOR`
- `ALARM_REASON_WOOD_BOILER_SENSOR`
- `ALARM_REASON_CHIMNEY_SENSOR`
- `ALARM_REASON_BOILER_ROOM_SENSOR`
- `UNKNOWN`

Az enumnevek a `system_data.h` jelenlegi definícióit követik. Új enumérték hozzáadásakor az API-leképezést és a szerződést is frissíteni kell. A numerikus enumértékeket ne tegyük közzé stabil API-azonosítóként, mert a C++ enumok sorrendjének módosítása megváltoztathatja őket.

### 4.3 `measurements` objektum

A mezőnevek megtartják a jelenlegi `SystemData` elnevezéseket.

| Mező | Típus | Mértékegység | Megjegyzés |
|---|---|---|---|
| `gasFlowTemperature` | number vagy `null` | °C | Gázos ág előremenő hőmérséklete |
| `gasReturnTemperature` | number vagy `null` | °C | Gázos ág visszatérő hőmérséklete |
| `woodFlowTemperature` | number vagy `null` | °C | Fatüzelésű ág előremenő hőmérséklete |
| `woodReturnTemperature` | number vagy `null` | °C | Fatüzelésű ág visszatérő hőmérséklete |
| `woodBoilerTemperature` | number vagy `null` | °C | Fatüzelésű kazán hőmérséklete |
| `chimneyTemperature` | number vagy `null` | °C | Füstgáz/kémény hőmérséklete |
| `boilerRoomTemperature` | number vagy `null` | °C | Kazánház hőmérséklete |
| `boilerRoomHumidity` | number vagy `null` | %RH | Kazánház relatív páratartalma |

Ha a megfelelő szenzor hibás, inicializálatlan vagy nem áll rendelkezésre érvényes érték, a JSON `null` értéket adjon vissza az adott mérésre. JSON-ban nem szabad `NaN` vagy `Infinity` értéket kibocsátani.

Az érvényesség elsődleges jelzője a `sensorErrors` megfelelő mezője. Ha az adott mérés érvénytelen, a kliens ne értelmezze a numerikus mezőt valós mérésként.

### 4.4 `sensorErrors` objektum

Mind a hét mező kötelező és boolean típusú. `true` azt jelenti, hogy a megfelelő szenzorhiba fennáll.

- `gasFlowSensorError`
- `gasReturnSensorError`
- `woodFlowSensorError`
- `woodReturnSensorError`
- `woodBoilerSensorError`
- `chimneySensorError`
- `boilerRoomSensorError`

Ezek a `SystemData` jelenlegi mezőit tükrözik. Nem helyettesíthetők kizárólag az összesített `alarmState` vagy `alarmReason` mezőkkel, mert azok nem feltétlenül írnak le minden egyidejű hibát.

### 4.5 `diagnostics` objektum

Az első verzióban csak olyan mező kerüljön ide, amelyhez a firmware tényleges, megbízható állapotot tud szolgáltatni.

| Mező | Típus | Státusz | Indoklás |
|---|---|---|---|
| `wifiConnected` | boolean | javasolt | Segít a hálózati állapot diagnosztizálásában, bár HTTP-válasz esetén a kliens és a Master közötti kapcsolat éppen működött. |
| `sdCardAvailable` | boolean | javasolt | A helyi naplózás diagnosztikája szempontjából hasznos. |
| `sdCardFull` | boolean vagy `null` | csak ha megbízhatóan lekérdezhető | A naplózás állapotát jelzi; ne duplikáljunk bizonytalan logikát. |
| `rtcBatteryError` | boolean vagy `null` | csak ha megbízhatóan hozzáférhető | Az időbélyegek megbízhatóságához fontos. |
| `rtcOscillatorError` | boolean vagy `null` | csak ha megbízhatóan hozzáférhető | Az RTC állapotának diagnosztikája. |

Az `uptimeSeconds` felső szintű mező, ezért nem kell még egyszer a `diagnostics` objektumban is szerepeltetni.

## 5. Frissesség és időkezelés

A `SystemData.timestamp` önmagában nem bizonyítja, hogy minden szenzorérték éppen akkor frissült. A szerződés ezért előírja:

1. A firmware rögzítsen monoton időalapú időpontot, amikor a releváns adatfrissítés sikeresen megtörténik.
2. A `dataAgeMs` ebből a monoton időalapból számítódjon, ne az RTC faliórájából.
3. Ha nincs még érvényes mérés/frissítés, a `dataAgeMs` legyen `null`, vagy az implementáció dokumentált, egyértelmű inicializálási állapotot adjon.
4. A kliens a `dataAgeMs` alapján állapítsa meg az elavultságot. A kezdő küszöbérték 30 000 ms lehet, de ezt a tényleges mérési ciklus ismeretében kell jóváhagyni.
5. Az RTC időbélyeg és a monoton frissességi mérés két külön célra szolgál: az egyik ember által értelmezhető időpont, a másik az adat korának megbízható mérése.

Ha a szenzorok különböző időközönként frissülnek, a későbbi verzió egyedi, szenzoronkénti adatkorokat is bevezethet. Az első verzióban ezt nem szükséges megvalósítani, amennyiben a közös adatfrissítési ciklus megfelelően jellemzi a közzétett adatokat.

## 6. HTTP hibakezelés

| Helyzet | Javasolt HTTP-válasz | Megjegyzés |
|---|---|---|
| Sikeres státuszlekérés | `200 OK` | Érvényes JSON |
| Sikeres health ellenőrzés | `200 OK` | Az API válaszol |
| Ismeretlen útvonal | `404 Not Found` | Ne adjon véletlenül HTML-oldalt JSON-ként |
| Hibás kérés | `400 Bad Request` | A végpontoknak nem kell query-paraméter |
| Ideiglenesen nem szolgálható ki az adat | `503 Service Unavailable` | Csak valós szolgáltatási hiba esetén |
| Váratlan belső hiba | `500 Internal Server Error` | Ne tartalmazzon titkot vagy belső érzékeny részletet |

A szenzorhiba önmagában nem feltétlenül HTTP-hiba: ha a Master API-ja működik, a válasz lehet `200 OK`, miközben a `sensorErrors` vagy a mérés `null` értéke jelzi a hibát. Ez lehetővé teszi a monitoring számára, hogy különbséget tegyen az eszköz elérhetetlensége és a szenzor hibája között.

## 7. Biztonsági és kompatibilitási követelmények

- Az API csak a helyi hálózaton legyen elérhető; ne legyen internetre porttovábbítva.
- Az API ne tartalmazzon vezérlési, újraindítási, fájltörlési vagy konfigurációmódosítási funkciót.
- A régi `/status` végpont jelenlegi HMS LOG fogyasztóját az átállás ne törje el.
- A régi és az új végpont ugyanazt a közös adat-előállító logikát használja.
- A kliens a `schemaVersion` értékét ellenőrizze, és ismeretlen verzió esetén biztonságosan jelezzen kompatibilitási hibát.
- A JSON válasz ne tartalmazzon Wi-Fi-jelszót, hálózati hitelesítő adatot vagy szükségtelen belső információt.
- A HTTP-kezelés nem blokkolhatja a szenzorolvasást, a naplózást, a watchdogot vagy a helyi riasztásokat.
- Az adatmodell olvasása legyen konzisztens; ha párhuzamos frissítés lehetséges, a szerver készítsen védett pillanatképet.

## 8. Példa teljes státuszválaszra

A számok és állapotok kizárólag szemléltető adatok.

```json
{
  "schemaVersion": 1,
  "timestamp": "2026-10-10T13:20:00+02:00",
  "uptimeSeconds": 86400,
  "dataAgeMs": 1200,
  "systemState": "SYSTEM_WOOD",
  "activeBranch": "HEATING_WOOD",
  "alarmState": "ALARM_NONE",
  "alarmReason": "ALARM_REASON_NONE",
  "firePresent": true,
  "gasAcPresent": false,
  "woodAcPresent": true,
  "measurements": {
    "gasFlowTemperature": null,
    "gasReturnTemperature": null,
    "woodFlowTemperature": 48.2,
    "woodReturnTemperature": 39.6,
    "woodBoilerTemperature": 72.5,
    "chimneyTemperature": 145.3,
    "boilerRoomTemperature": 24.1,
    "boilerRoomHumidity": 48.0
  },
  "sensorErrors": {
    "gasFlowSensorError": true,
    "gasReturnSensorError": true,
    "woodFlowSensorError": false,
    "woodReturnSensorError": false,
    "woodBoilerSensorError": false,
    "chimneySensorError": false,
    "boilerRoomSensorError": false
  },
  "diagnostics": {
    "wifiConnected": true,
    "sdCardAvailable": true,
    "sdCardFull": false,
    "rtcBatteryError": false,
    "rtcOscillatorError": false
  }
}
```

Ebben a példában a gázos hőmérsékletek `null` értékűek, mert a hozzájuk tartozó szenzorhibák aktívak. A példa nem írja felül a HMS saját riasztási prioritásait; a valós `alarmState` és `alarmReason` értékeket a meglévő feldolgozott állapotból kell átvenni.

## 9. Megvalósítás előtti ellenőrzőlista

- [ ] A `http_communication_dev` branch tényleges `system_data.h` és `data_manager.cpp` fájljait ellenőriztük.
- [ ] A hét szenzorhiba-mező pontos neve és jelentése megerősítve.
- [ ] Az enumok stabil, gépi szöveges leképezése rögzítve.
- [ ] A `dataAgeMs` alapjául szolgáló tényleges frissítési esemény azonosítva.
- [ ] Az RTC időzónájának kezelése tisztázva.
- [ ] A diagnosztikai mezőkhöz valódi, megbízható adatforrás tartozik.
- [ ] A régi `/status` kompatibilitása megőrizhető.
- [ ] A közös JSON-adat-előállítás és a HTTP-kezelés szétválasztásának módja meghatározva.
- [ ] A csak olvasható működés és a fűtésvezérlés kizárása rögzítve.

## 10. Következő fejlesztési lépés

A szerződés elfogadása után, a `http_communication_dev` branchen:

1. A meglévő `/status` JSON-építését külön, közös adat-előállító függvénybe kell szervezni.
2. Meg kell őrizni a HMS LOG régi válaszformátumát.
3. Létre kell hozni az új `/api/v1/status` útvonalat a szerződés szerinti mezőkkel.
4. A hét szenzorhibát és a megbízható frissességi adatot be kell vezetni.
5. Böngészőből vagy `curl`-lel ellenőrizni kell mindkét végpontot és a hibás eseteket.
6. Csak a Master API tesztjeinek sikeressége után kezdődhet a Display és az Icinga/Nagios kliensfejlesztése.
