# BLE Grundbegriffe: GATT, ATT, Services und Characteristics

## ATT (Attribute Protocol)

ATT ist das **Basisprotokoll** auf der untersten Ebene, das den eigentlichen Datenaustausch zwischen zwei BLE-Geräten regelt. Es definiert, wie Daten als sogenannte "Attribute" organisiert und übertragen werden.

- Jedes Attribut hat ein **Handle** (eine Art Adresse/ID), einen **Typ** (UUID) und einen **Wert**
- Es gibt zwei Rollen: **ATT Server** (hält die Daten, z.B. ein Fitness-Tracker) und **ATT Client** (fragt Daten ab, z.B. dein Smartphone)
- ATT ist recht "roh" – es kennt nur einzelne Attribute, aber keine höhere Struktur

## GATT (Generic Attribute Profile)

GATT baut **auf ATT auf** und bringt Struktur ins Ganze. Es definiert, wie Attribute sinnvoll in Services und Characteristics organisiert werden.

**Kurz gesagt:** ATT ist das "Wie" (Transportmechanismus), GATT ist das "Was" (die Datenstruktur/Hierarchie).

Die Hierarchie sieht so aus:

```
GATT Server
 └── Service (z.B. "Heart Rate Service")
      ├── Characteristic (z.B. "Heart Rate Measurement")
      │    ├── Value (der eigentliche Messwert)
      │    └── Descriptor (Zusatzinfos, z.B. Einheit)
      └── Characteristic (z.B. "Body Sensor Location")
```

## Service

Ein Service ist eine **logische Gruppierung** von zusammengehörigen Daten und Funktionen. Man kann sich das wie einen "Ordner" vorstellen.

- Jeder Service hat eine eindeutige UUID (16-bit für Standard-Services, 128-bit für herstellerspezifische)
- Beispiele für Standard-Services: `Battery Service`, `Heart Rate Service`, `Device Information Service`
- Ein Gerät kann mehrere Services gleichzeitig anbieten

## Characteristic

Eine Characteristic ist der **eigentliche Datenpunkt** innerhalb eines Service – vergleichbar mit einer "Datei" im Ordner.

- Enthält einen konkreten Wert (z.B. den Batteriestand als Zahl)
- Hat Eigenschaften (Properties), die festlegen, was damit möglich ist:
  - **Read** – lesbar
  - **Write** – beschreibbar
  - **Notify** – Server schickt automatisch Updates, wenn sich der Wert ändert
  - **Indicate** – wie Notify, aber mit Bestätigung
- Kann zusätzliche **Descriptors** haben (z.B. eine textuelle Beschreibung oder Einheit)

## GATT Operations

Das sind die konkreten **Aktionen**, die Client und Server über die GATT-Struktur austauschen können:

| Operation                         | Bedeutung                                                                                   |
| --------------------------------- | ------------------------------------------------------------------------------------------- |
| **Discovery**                     | Client erkundet, welche Services/Characteristics der Server anbietet                        |
| **Read**                          | Client liest den Wert einer Characteristic                                                  |
| **Write** (with/without response) | Client schreibt einen Wert – mit oder ohne Bestätigung                                      |
| **Notify**                        | Server sendet unaufgefordert Updates, sobald sich ein Wert ändert (keine Bestätigung nötig) |
| **Indicate**                      | Wie Notify, aber der Client muss den Empfang bestätigen                                     |

## Zusammenfassung als Analogie

Stell dir ein Buch vor:
- **ATT** = die Regeln, wie man überhaupt Seiten umblättern und lesen kann
- **GATT** = die Buchstruktur mit Kapiteln und Abschnitten
- **Service** = ein Kapitel
- **Characteristic** = ein Absatz mit konkretem Inhalt
- **GATT Operations** = was du mit dem Buch machen kannst (lesen, reinschreiben, benachrichtigt werden bei Änderungen)

---

# Attribute im BLE-Kontext

Ein **Attribute** ist die kleinste Dateneinheit im ATT-Protokoll – quasi der Grundbaustein, aus dem alles andere (Services, Characteristics, Descriptors) zusammengesetzt wird. Jedes einzelne Element, das ein GATT Server anbietet, ist letztlich ein Attribute.

## Aufbau eines Attributes

Jedes Attribute besteht aus vier Teilen:

| Feld | Bedeutung |
|---|---|
| **Handle** | Eine eindeutige 16-bit Nummer, quasi die "Adresse" des Attributes innerhalb des Servers (z.B. `0x0001`, `0x0002`, ...) |
| **Type (UUID)** | Sagt aus, *was* dieses Attribute ist – z.B. "das ist ein Service", "das ist eine Characteristic-Deklaration", oder "das ist ein Battery-Level-Wert" |
| **Value** | Der eigentliche Dateninhalt (z.B. die Zahl `85` für Batteriestand) |
| **Permissions** | Zugriffsrechte – wer darf lesen/schreiben, braucht es Verschlüsselung/Authentifizierung? |

## Wichtig: Alles ist ein Attribute

Das Verwirrende (und gleichzeitig Elegante) am ATT-Protokoll ist: **Services, Characteristics und Descriptors sind selbst auch nur Attribute** – nur mit speziellen, reservierten Typen.

Beispiel einer Attribute-Tabelle auf einem Server:

```
Handle   Type (UUID)                          Value
0x0001   Primary Service Declaration          "Battery Service" (0x180F)
0x0002   Characteristic Declaration           Properties: Read, Notify → zeigt auf 0x0003
0x0003   Battery Level (0x2A19)                85 (der eigentliche Wert!)
0x0004   Client Characteristic Config (CCCD)   0x0000 (Notify an/aus)
```

Hier siehst du:
- Handle `0x0001` ist das Attribute, das den **Service selbst** deklariert
- Handle `0x0002` ist das Attribute, das die **Characteristic deklariert** (Metadaten: welche Rechte, wo liegt der Wert)
- Handle `0x0003` ist das Attribute mit dem **tatsächlichen Wert**
- Handle `0x0004` ist ein **Descriptor** – auch nur ein weiteres Attribute

## Warum diese Struktur?

Der Client kann diese flache Liste von Attributes durchsuchen (per Handle-Bereich oder UUID) und daraus die Hierarchie "Service → Characteristic → Value → Descriptor" rekonstruieren, obwohl auf ATT-Ebene eigentlich nur eine simple, sequentielle Tabelle existiert.

**Kurz gesagt:** GATT ist im Grunde nur eine *Konvention*, wie man Attributes anordnet und interpretiert, damit aus einer einfachen Liste eine sinnvolle Datenstruktur wird.

---

# UUID für Attribute vs. UUID für Service

Das sind **zwei unterschiedliche UUIDs**, die leicht verwechselt werden. Jedes Attribute hat nämlich potenziell UUIDs an zwei verschiedenen Stellen:

1. **Attribute Type** (die "Rolle" des Attributes – was ist das für eine Art Eintrag?)
2. **Attribute Value** (kann selbst wieder eine UUID enthalten – nämlich welcher konkrete Service/welche Characteristic das ist)

## Das Beispiel nochmal genauer

```
Handle   Attribute Type (UUID)              Attribute Value
0x0001   0x2800 ("Primary Service")    →    0x180F ("Battery Service")
0x0002   0x2803 ("Characteristic")     →    Properties + Handle + 0x2A19
0x0003   0x2A19 ("Battery Level")      →    85
```

### Zeile 0x0001 – Service-Deklaration
- **Attribute Type** = `0x2800` → das ist eine **reservierte, feste UUID**, die immer bedeutet: "Achtung, hier beginnt ein Primary Service"
- **Attribute Value** = `0x180F` → das ist die **UUID, die diesen spezifischen Service identifiziert** (in diesem Fall "Battery Service")

### Zeile 0x0003 – Characteristic-Wert
- **Attribute Type** = `0x2A19` → hier ist es anders! Bei der Value-Zeile einer Characteristic **ist** der Attribute Type direkt die UUID der Characteristic selbst (Battery Level)
- **Attribute Value** = `85` → der eigentliche Messwert

## Die Faustregel

| UUID-Art | Beispiele | Zweck |
|---|---|---|
| **Reservierte GATT-UUIDs** | `0x2800` (Service), `0x2803` (Characteristic Declaration), `0x2902` (CCCD) | Sagen aus: "Das ist strukturell ein Service / eine Characteristic / ein Descriptor" |
| **Spezifische Service/Characteristic-UUIDs** | `0x180F` (Battery Service), `0x2A19` (Battery Level), oder eine custom 128-bit UUID | Sagen aus: "Das ist inhaltlich DIESER Service / DIESE Characteristic" |

**Kurz gesagt:**
- Die UUID `0x2800` beantwortet die Frage: *"Was für eine Art Attribute ist das strukturell?"* → Antwort: ein Service
- Die UUID `0x180F` beantwortet die Frage: *"Welcher Service ist es inhaltlich?"* → Antwort: Battery Service

Man könnte sagen: `0x2800` ist wie das Wort "Kapitel" in einem Buch (strukturelle Markierung), während `0x180F` der tatsächliche Kapiteltitel ist ("Batteriestatus").

---

# Strukturelle Beschreibung: Heart Rate Service

Der **Heart Rate Service** (UUID `0x180D`) ist ein von Bluetooth SIG standardisierter Service. Er besteht aus mehreren Attributes, hierarchisch organisiert.

## Attribute-Tabelle

```
Handle   Attribute Type (UUID)          Attribute Value / Bedeutung
──────────────────────────────────────────────────────────────────
0x0010   0x2800 (Primary Service)   →   0x180D "Heart Rate Service"

0x0011   0x2803 (Characteristic)    →   Properties: Notify
                                         Value Handle: 0x0012
                                         UUID: 0x2A37

0x0012   0x2A37 (Heart Rate         →   z.B. [Flags=0x00, HR-Wert=72]
         Measurement)

0x0013   0x2902 (CCCD)              →   0x0001 (Notify aktiviert/deaktiviert)

──────────────────────────────────────────────────────────────────
0x0014   0x2803 (Characteristic)    →   Properties: Read
                                         Value Handle: 0x0015
                                         UUID: 0x2A38

0x0015   0x2A38 (Body Sensor        →   z.B. 0x02 ("Wrist")
         Location)

──────────────────────────────────────────────────────────────────
0x0016   0x2803 (Characteristic)    →   Properties: Write
                                         Value Handle: 0x0017
                                         UUID: 0x2A39

0x0017   0x2A39 (Heart Rate         →   z.B. 0x01 (Reset Energy Expended)
         Control Point)
```

## Aufgeschlüsselt nach Characteristics

### 1. Heart Rate Measurement (`0x2A37`) — Pflicht
- **Property:** `Notify`
- Der zentrale Wert – der Server (z.B. Brustgurt) schickt automatisch bei jeder neuen Messung ein Update
- Hat als Descriptor immer eine **CCCD** (`0x2902`), über die der Client Notifications an-/ausschaltet
- Der Value selbst ist kein simpler Byte-Wert, sondern strukturiert:
  - **Flags-Byte** (definiert z.B. ob 8-bit oder 16-bit HR-Wert, ob Energy Expended vorhanden ist, ob RR-Intervalle vorhanden sind)
  - **Heart Rate Value** (8 oder 16 bit)
  - optional: Energy Expended, RR-Intervalle

### 2. Body Sensor Location (`0x2A38`) — Optional
- **Property:** `Read`
- Gibt an, wo der Sensor am Körper sitzt (z.B. Brust, Handgelenk, Finger, Ohrläppchen)
- Einmaliger Wert, ändert sich normalerweise nicht

### 3. Heart Rate Control Point (`0x2A39`) — Optional
- **Property:** `Write`
- Client kann hierhin schreiben, um z.B. den "Energy Expended"-Zähler zurückzusetzen

## Visuelle Hierarchie

```
Heart Rate Service (0x180D)
│
├── Heart Rate Measurement (0x2A37)       [Notify]
│   └── CCCD (0x2902)                     [Read/Write]
│
├── Body Sensor Location (0x2A38)         [Read]
│
└── Heart Rate Control Point (0x2A39)     [Write]
```
