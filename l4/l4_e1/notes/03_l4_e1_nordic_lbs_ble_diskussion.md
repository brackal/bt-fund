# Nordic LED Button Service (LBS) – BLE/GATT Diskussion

Zusammenfassung einer technischen Diskussion über den Zephyr/nRF Connect SDK
Beispielcode der **LED Button Service (LBS)**, basierend auf `my_lbs.h` und
`my_lbs.c` aus dem Nordic-Tutorial.

---

## 1. Überblick über den Code

Der Code implementiert die **LED Button Service (LBS)** – einen
benutzerdefinierten GATT-Dienst für Bluetooth LE mit zwei Characteristics:

- **Button-Characteristic** (lesbar): Client kann den aktuellen Button-Zustand
  auslesen.
- **LED-Characteristic** (schreibbar): Client kann eine LED ein-/ausschalten.

Die Hardware-Logik liegt nicht im Service selbst, sondern wird über
Callback-Funktionen (`led_cb`, `button_cb`) an die Anwendung angebunden, die
via `my_lbs_init()` registriert werden.

---

## 2. `read_button()` – wird beim Lesen der Button-Characteristic aufgerufen

```c
static ssize_t read_button(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  void *buf,
			  uint16_t len,
			  uint16_t offset)
```

**Wann?** Immer wenn ein verbundener BLE-Client eine **READ-Anfrage** auf die
Button-Characteristic schickt. Der Zephyr Bluetooth-Host-Stack ruft die
Funktion automatisch auf.

**Was macht sie?**
1. Holt einen Zeiger auf `attr->user_data` (übergeben als `&button_state`).
2. Ruft `lbs_cb.button_cb()` auf, um den aktuellen Button-Zustand von der
   Hardware abzufragen, und speichert ihn in `button_state`.
3. Kopiert den Wert mit `bt_gatt_attr_read()` in den Antwortpuffer `buf`.

`button_state` wird also **on-demand beim Lesezugriff** aktualisiert, nicht
kontinuierlich im Hintergrund.

---

## 3. `write_led()` – wird beim Schreiben der LED-Characteristic aufgerufen

```c
static ssize_t write_led(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr,
			 const void *buf,
			 uint16_t len, uint16_t offset, uint8_t flags)
```

**Wann?** Wenn ein Client eine **WRITE-Anfrage** auf die LED-Characteristic
sendet.

**Was macht sie?**
1. Prüft `len != 1U` → Fehler `BT_ATT_ERR_INVALID_ATTRIBUTE_LEN`.
2. Prüft `offset != 0` → Fehler `BT_ATT_ERR_INVALID_OFFSET`.
3. Liest das übertragene Byte.
4. Bei `0x00`/`0x01`: ruft `lbs_cb.led_cb(true/false)` auf.
5. Bei ungültigem Wert: `BT_ATT_ERR_VALUE_NOT_ALLOWED`.
6. Bei Erfolg: gibt `len` zurück.

---

## 4. Das Makro `BT_GATT_CHARACTERISTIC`

```c
BT_GATT_CHARACTERISTIC(BT_UUID_LBS_BUTTON,
                        BT_GATT_CHRC_READ,
                        BT_GATT_PERM_READ,
                        read_button,
                        NULL,
                        &button_state)
```

Erzeugt zwei GATT-Attribute:
1. **Characteristic Declaration** (`0x2803`) – UUID, Properties, Handle des
   Value-Attributs.
2. **Characteristic Value** – der eigentliche Wert.

| Parameter | Bedeutung |
|---|---|
| `BT_UUID_LBS_BUTTON` | 128-Bit UUID der Characteristic |
| `BT_GATT_CHRC_READ` | **Property**: sichtbar bei Discovery, teilt Client mit, dass Lesen möglich ist |
| `BT_GATT_PERM_READ` | **Permission**: tatsächliche Zugriffskontrolle auf ATT-Ebene |
| `read_button` | Read-Callback |
| `NULL` | Write-Callback (hier keiner) |
| `&button_state` | `user_data`-Zeiger, zugänglich über `attr->user_data` |

**Property vs. Permission:**

| Präfix | Bedeutung | Sichtbar für Client? |
|---|---|---|
| `BT_GATT_CHRC_*` | Was grundsätzlich möglich ist (Read/Write/Notify/Indicate) | Ja, bei Discovery |
| `BT_GATT_PERM_*` | Tatsächliche Zugriffskontrolle (z. B. Verschlüsselung nötig?) | Nein, nur serverintern |

**CHRC** = Abkürzung für **Characteristic**.

---

## 5. Wie kommt `button_state` in `buf`?

```c
const char *value = attr->user_data;   // zeigt auf dieselbe Adresse wie button_state
...
button_state = lbs_cb.button_cb();     // aktualisiert button_state (und damit *value)
return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(*value));
```

Datenfluss:

```
Hardware (GPIO)
   → lbs_cb.button_cb()
   → button_state (globale Variable)
   → (gleiche Adresse!) value / button_state_ptr
   → bt_gatt_attr_read() macht memcpy(buf, value, len)
   → buf
   → wird über ATT/BLE an den Client gesendet
```

`bt_gatt_attr_read()` übernimmt das Kopieren (inkl. Behandlung von `offset`
bei mehrteiligen Reads) in den Antwortpuffer `buf`.

**Achtung:** `value` war als `const char*` deklariert, `sizeof(*value)` liefert
also `sizeof(char)` = 1 Byte. Das funktioniert nur, *weil* `sizeof(bool) == 1`
auf den meisten Plattformen – kein garantierter C-Standard-Fakt. Sauberer:

```c
return bt_gatt_attr_read(conn, attr, buf, len, offset,
                          &button_state, sizeof(button_state));
```

---

## 6. `_t`-Namenskonvention

`_t` steht für **"type"** (POSIX-Konvention) und kennzeichnet Typnamen wie
`led_cb_t`, `uint8_t`, `size_t`.

```c
typedef void (*led_cb_t)(const bool led_state);
```

- `led_cb_t` = **Typname** ("Bauplan") für einen Funktionszeiger.
- `led_cb` (Struct-Feld) = **Variable** dieses Typs, die zur Laufzeit eine
  konkrete Funktionsadresse enthält.

### Kritikpunkt bei `struct my_lbs_cb_t`

```c
struct my_lbs_cb_t {
	led_cb_t led_cb;
	button_cb_t button_cb;
};
```

Hier ist `my_lbs_cb_t` nur ein **struct-Tag**, kein via `typedef` erzeugter
Alias – man muss weiterhin `struct my_lbs_cb_t` schreiben, nicht nur
`my_lbs_cb_t`. Das widerspricht der `_t`-Konvention, die einen direkt
nutzbaren Typ suggeriert. Zusätzlich reserviert POSIX `_t`-Endungen offiziell
für Systemtypen.

**Konsistente Alternativen:**

```c
// A: nur struct, kein _t
struct my_lbs_cb { ... };

// B: mit typedef, dann passt _t
typedef struct { ... } my_lbs_cb_t;
```

---

## 7. Bessere Callback-Namen

`button_cb` ist unpräzise. Besserer Name: **`get_button_state_cb`**, da der
Typ ein **Pull-Callback** ist:

```c
typedef bool (*button_cb_t)(void);
```

→ `get_` signalisiert Abfrage/Pull, `_state` sagt "aktueller Zustand".

Zum Vergleich: `led_cb_t` ist ein **Push-Callback** ("hier ist der neue
Zustand, setz ihn um"):

```c
typedef void (*led_cb_t)(const bool led_state);
```

Konsistente Umbenennung:

| Alt | Neu | Bedeutung |
|---|---|---|
| `led_cb_t` | `set_led_state_cb_t` | Push: Setze LED-Zustand |
| `button_cb_t` | `get_button_state_cb_t` | Pull: Frage Button-Zustand ab |

---

## 8. Warum ist `button_state_ptr` vom Typ `char*` statt `bool*`?

`attr->user_data` ist als `void*` deklariert. Die Zuweisung an
`const char *button_state_ptr` ist eine bewusste (aber nicht ideale)
"generischer Byte-Zeiger"-Konvention aus systemnahem C, praktisch für
byteweises Kopieren/Senden (z. B. via `memcpy`, wie es `bt_gatt_attr_read()`
letztlich tut).

Das funktioniert nur zufällig korrekt, weil `sizeof(bool) == sizeof(char)`
(implementierungsabhängig, aber in der Praxis meist so).

**Sauberere Alternative:**

```c
const bool *button_state_ptr = attr->user_data;
...
return bt_gatt_attr_read(conn, attr, buf, len, offset,
                          button_state_ptr, sizeof(*button_state_ptr));
```

---

## 9. Warum kein expliziter `(void*)`-Cast bei der Parameterübergabe?

In **C** wird jeder Objektzeiger-Typ implizit zu `void*` konvertiert (und
umgekehrt) – ganz ohne Cast. Das gilt nicht in **C++**, wo `void* → T*` einen
expliziten Cast braucht (`T* → void*` geht aber auch in C++ implizit).

| Konvertierung | In C | In C++ |
|---|---|---|
| `T* → void*` | implizit ✅ | implizit ✅ |
| `void* → T*` | implizit ✅ | Cast nötig ❌ |

Da Zephyr in C geschrieben ist (der `extern "C"`-Block dient nur dem Linking
aus C++), ist hier kein Cast notwendig.

---

## 10. Wohin returned `read_button()`? Wann wird die Response gesendet?

`read_button()` ist ein **Callback**, den der Zephyr Bluetooth-Host-Stack
(GATT-Server-Core) aufruft. Der Rückgabewert geht zurück in den
GATT-Server-internen Code, nicht direkt zum Client.

**Ablauf:**

```
1. Client sendet ATT "Read Request"
        ↓
2. Controller empfängt Paket über BLE-Funk
        ↓
3. Host Stack (ATT-Layer) identifiziert Attribut anhand Handle
        ↓
4. GATT-Server-Core ruft read_button() auf
        ↓
5. read_button() befüllt buf, gibt Byte-Anzahl zurück
        ↓
6. Stack verpackt buf in ATT "Read Response" (ATT_READ_RSP)
        ↓
7. Übergabe an Controller
        ↓
8. Controller sendet Response beim nächsten Connection Event über Funk
```

Terminologie: Die Antwort heißt offiziell **ATT_READ_RSP (Read Response)**,
ihr Payload ist der **Attribute Value** (Bluetooth Core Spec, Vol 3, Part F).

**Wichtig:** `read_button()` läuft im Kontext des Bluetooth-Host-Stacks (z. B.
RX-Thread) – keine langen Blockierungen (kein `k_sleep()` o. Ä.)!

---

## 11. Woher weiß man, dass `read_button` vom Typ `bt_gatt_attr_read_func_t` sein muss?

Nachverfolgung durch die Makro-Kette:

```
BT_GATT_CHARACTERISTIC (Makro, gatt.h)
    ↓ übergibt read_button an
struct bt_gatt_attr (Struct-Definition, gatt.h)
    ↓ Feld "read" ist vom Typ
bt_gatt_attr_read_func_t (Typedef, gatt.h)
    ↓ definiert exakte Signatur:
ssize_t (*)(struct bt_conn*, const struct bt_gatt_attr*, void*, uint16_t, uint16_t)
```

**Praktische Wege:**
1. IDE "Go to Definition" (VS Code/CLion mit `compile_commands.json`).
2. `grep -rn "bt_gatt_attr_read_func_t" zephyr/include/`
3. Offizielle Zephyr API-Doku (Doxygen): `docs.zephyrproject.org`
4. Compiler-Fehlermeldung bei falscher Signatur (`incompatible pointer type`).

---

## 12. `BT_GATT_PERM_READ` – Zugriffskontrolle und Verschlüsselung

Permission-Flags sind **Sicherheitsstufen**, keine reine Ja/Nein-Erlaubnis:

```c
BT_GATT_PERM_READ                  /* keine Security nötig */
BT_GATT_PERM_READ_ENCRYPT          /* Verschlüsselung erforderlich */
BT_GATT_PERM_READ_AUTHEN           /* authentifizierte Verschlüsselung (MITM-Schutz) */
BT_GATT_PERM_READ_LESC             /* LE Secure Connections erforderlich */
```

Der Stack prüft die Sicherheitsstufe der Verbindung **automatisch, bevor**
der Callback (`read_button`) aufgerufen wird:

| Flag | Voraussetzung |
|---|---|
| `BT_GATT_PERM_READ` | Keine – jeder verbundene Client |
| `BT_GATT_PERM_READ_ENCRYPT` | Verbindung muss verschlüsselt sein |
| `BT_GATT_PERM_READ_AUTHEN` | Verschlüsselung + authentifiziertes Pairing (kein "Just Works") |
| `BT_GATT_PERM_READ_LESC` | LE Secure Connections (modernes ECDH-Pairing) |

Bei unzureichender Sicherheit sendet der Stack direkt eine `ATT Error
Response` (z. B. `BT_ATT_ERR_INSUFFICIENT_ENCRYPTION`), **ohne** den Callback
aufzurufen. Zusätzlich kann eine Mindest-Schlüssellänge (`CONFIG_BT_ENC_KEY_SIZE_MIN`)
gefordert werden.

Aktueller Code (`BT_GATT_PERM_READ`) hat **keine** kryptografische
Zugriffskontrolle – jeder verbundene Client kann den Button-Zustand lesen.

---

## 13. Woher kommen die Texte "Nordic LED Button Service", "Button", "LED" in nRF Connect?

**Nicht vom Gerät/Firmware** – der Code enthält keinerlei Textinformation
(kein `BT_GATT_CUD(...)`).

Die Texte stammen aus einer **App-internen UUID-Datenbank** von nRF Connect,
die sowohl offizielle Bluetooth-SIG-UUIDs als auch Nordics eigene, firmen-
spezifische Tutorial-UUIDs (wie LBS) kennt.

**Mechanismus:**

```
1. App verbindet sich, führt Discovery durch
        ↓
2. App bekommt nur rohe 128-Bit UUIDs zurück
        ↓
3. App schlägt UUIDs in lokaler Datenbank nach
        ↓
4. Treffer → Anzeige "Nordic LED Button Service" etc.
```

Bei einer eigenen, neuen UUID würde nRF Connect nur die rohe UUID anzeigen.

---

## 14. `BT_GATT_CUD` korrekt einfügen

`BT_GATT_CUD` ist **kein Zusatzparameter** von `BT_GATT_CHARACTERISTIC`,
sondern ein **eigenständiges, zusätzliches Element** in der
Komma-getrennten Attributliste von `BT_GATT_SERVICE_DEFINE`:

```c
BT_GATT_SERVICE_DEFINE(my_lbs_svc, 
	BT_GATT_PRIMARY_SERVICE(BT_UUID_LBS),

	BT_GATT_CHARACTERISTIC(BT_UUID_LBS_BUTTON,
				BT_GATT_CHRC_READ,
				BT_GATT_PERM_READ, 
				read_button,
				NULL,
				&button_state),
	BT_GATT_CUD("Button", BT_GATT_PERM_READ),   // eigenes Element!

	BT_GATT_CHARACTERISTIC(BT_UUID_LBS_LED,
				BT_GATT_CHRC_WRITE,
				BT_GATT_PERM_WRITE,
				NULL, 
				write_led,
				NULL),
	BT_GATT_CUD("LED", BT_GATT_PERM_READ)       // letztes Element: kein Komma danach
);
```

**Typische Fehler:** CUD versehentlich *innerhalb* der `BT_GATT_CHARACTERISTIC(...)`-Klammer
eingefügt (falsche Argumentanzahl → Compiler-Fehler), fehlendes/zu viel Komma.

---

## 15. Gibt es `BT_GATT_CUD` für den Service selbst?

**Nein.** Der Characteristic User Description Descriptor (UUID `0x2901`) ist
laut Bluetooth Core Spec (Vol 3, Part G, 3.3.3.2) explizit an eine
Characteristic gebunden. Ein Service besteht nur aus Service-Deklaration und
enthaltenen Characteristics – kein Slot für Descriptors.

**Wege zu einem "beschreibbaren" Service:**
- `BT_GATT_CUD` auf **Characteristic-Ebene** (einziger spec-konformer Weg für
  Beschreibungen).
- **Bluetooth SIG-Registrierung**: offizieller Name in der öffentlichen
  "Assigned Numbers"-Liste, von allen konformen Apps automatisch erkannt.
- **Eigene Custom-UUID**: bleibt unbenannt, außer Apps kennen sie aus einer
  eigenen lokalen Datenbank (wie bei Nordics LBS-UUID).

Der GAP **Device Name** (`CONFIG_BT_DEVICE_NAME`) ist der Gerätename, nicht
service-spezifisch.
