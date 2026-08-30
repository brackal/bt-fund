# BLE GATT: CCCD, Subscribing/Notifications und Service Discovery

## 1. Was ist die CCCD?

Der **Client Characteristic Configuration Descriptor (CCCD)** ist ein spezieller Characteristic-Descriptor, der benötigt wird, wenn eine Characteristic server-initiierte Operationen unterstützt (Notify und/oder Indicate).

- **UUID:** `0x2902`
- **Eigenschaften:** immer lesbar und schreibbar
- **Value-Feld:** nur 2 Bits relevant
  - Bit 0 = Notifications aktiviert/deaktiviert
  - Bit 1 = Indications aktiviert/deaktiviert

Werte:
| Value    | Bedeutung                        |
|----------|-----------------------------------|
| `0x0000` | Notify & Indicate aus             |
| `0x0001` | Notify aktiviert                  |
| `0x0002` | Indicate aktiviert                |

### Beispiel: Heart Rate Service
Die Heart Rate Measurement Characteristic im Heart Rate Service nutzt die CCCD, damit der GATT Client (z. B. ein Smartphone) sich für Updates registrieren kann. Der GATT Server (z. B. ein Brustgurt-Sensor) pusht dann neue Messwerte, ohne dass das Handy pollen muss.

---

## 2. Subscribing und Notifications – Ablauf

**Phase 1: Subscribing (CCCD schreiben)**

1. Client sendet `ATT Write Request` an das Handle der CCCD (UUID `0x2902`)
2. Value = `0x0001` (Notifications aktivieren)
3. Server antwortet mit `Write Response` (ACK)
4. Ab jetzt ist der Client für diese Characteristic subscribed

**Phase 2: Notifications empfangen**

- Bei jedem neuen Messwert sendet der Server unaufgefordert ein `ATT Handle Value Notification`-PDU
- Der Client muss dies **nicht** bestätigen ("fire and forget")

```
Client                                   Server
  |------ Write Request: CCCD -------------->|   Value = 0x0001 (Notify an)
  |<----- Write Response (ACK) --------------|
  |                                           |
  |            [subscribed]                  |
  |                                           |
  |<----- Notification: HR Measurement -------|
  |<----- Notification: HR Measurement -------|
  |<----- Notification: HR Measurement -------|
  |                 ...                       |
```

### Notify vs. Indicate

| | Notification | Indication |
|---|---|---|
| Bestätigung durch Client | Nein | Ja (Confirmation) |
| Geschwindigkeit | Schneller | Langsamer |
| Zuverlässigkeit | Keine Garantie | Garantiert (Server wartet auf ACK) |
| Typischer Einsatz | Häufige Werte (z. B. Herzfrequenz) | Kritische Daten (z. B. Alarme) |

---

## 3. Wie merkt sich der Server den Client?

Der Server speichert CCCD-Werte und andere Client-spezifische Daten **nicht pro Verbindung, sondern pro gebondetem Gerät** in einer Bonding-Datenbank.

Gespeicherte Daten pro Bonding-Eintrag:
- **Identity Address** (dauerhafte BD_ADDR bzw. aufgelöste Identität)
- **LTK** (Long Term Key, für Verschlüsselung)
- **IRK** (Identity Resolving Key, zur Auflösung von Private Resolvable Addresses)
- **CSRK** (für signierte Writes, falls genutzt)
- **CCCD-Werte** pro Characteristic für dieses Gerät

### Warum nicht einfach die aktuelle Connection-Adresse?

Viele Clients nutzen aus Datenschutzgründen **Private Resolvable Addresses (RPA)**, die sich regelmäßig ändern. Deshalb läuft die Wiedererkennung über den IRK:

1. Beim Pairing werden Schlüssel (u. a. IRK) ausgetauscht
2. Bei jeder neuen Verbindung sendet der Client seine (evtl. neue) RPA
3. Der Server versucht, die RPA mittels gespeicherter IRKs aufzulösen (**Address Resolution**)
4. Passt ein IRK → Client identifiziert → gespeicherte CCCD-Werte etc. werden geladen

**Ohne Bonding** gibt es diese Persistenz nicht – nach jedem Disconnect ist die CCCD zurückgesetzt und der Client muss erneut subscriben.

---

## 4. GATT Service Discovery (vor dem ersten Write)

Bevor der Client überhaupt das Handle der CCCD kennt, muss er die Server-Struktur per Service Discovery erkunden. Dies basiert auf ATT-Requests über Attribute-Handles (jeder Service, jede Characteristic, jeder Descriptor hat ein eindeutiges 16-Bit-Handle).

### Schritt 1: Primary Services entdecken
- `Read By Group Type Request` mit UUID-Filter `0x2800` (Primary Service)
- Range: `0x0001`–`0xFFFF`
- Antwort: Start-/End-Handle + Service-UUID (z. B. `0x180D` für Heart Rate Service)
- Wird ggf. mehrfach wiederholt bis `Attribute Not Found`

### Schritt 2: Characteristics entdecken
- Für jeden Service: `Read By Type Request` mit UUID `0x2803` (Characteristic Declaration)
- Antwort: Handle, Properties (z. B. ob Notify erlaubt ist), Value-Handle

### Schritt 3: Descriptors entdecken
- Für Characteristics mit Notify/Indicate-Property: `Find Information Request`
- Range: zwischen Value-Handle und nächstem Characteristic-Handle
- Ergebnis: CCCD mit UUID `0x2902` gefunden

### Schritt 4: CCCD beschreiben
- Erst jetzt kennt der Client das konkrete CCCD-Handle
- `Write Request` mit Value `0x0001` (siehe Abschnitt 2)

```
Handle 0x0010  Service: Heart Rate Service (0x180D)
Handle 0x0011  Characteristic Decl.: Heart Rate Measurement
Handle 0x0012  Value: Heart Rate Measurement (0x2A37)
Handle 0x0013  Descriptor: CCCD (0x2902)   <- hier wird geschrieben
Handle 0x0014  naechste Characteristic ...
```

### Praktischer Hinweis
Viele Clients (z. B. Smartphones) cachen die Discovery-Ergebnisse pro gebondetem Gerät, um sie nicht bei jeder Verbindung neu durchzuführen – außer der Server signalisiert über die **Service Changed**-Characteristic, dass sich seine GATT-Struktur geändert hat.
