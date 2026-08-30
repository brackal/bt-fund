# BLE: Notify vs. Indicate – Server-initiierte GATT-Operationen

## 1. Unterschied zwischen Notify und Indicate

Beide sind server-initiierte GATT-Operationen, mit denen ein Peripheral-Gerät (Server) ungefragt Daten an den Client sendet, sobald sich der Wert einer Characteristic ändert – ohne dass der Client aktiv pollen muss. Der Unterschied liegt in der Zuverlässigkeit der Übertragung.

### Notify

- **Kein Acknowledgement**: Der Server sendet den Wert und wartet nicht auf eine Bestätigung vom Client.
- **Schneller**: Da kein Handshake nötig ist, kann der Server sofort die nächste Notification senden.
- **Weniger zuverlässig**: Geht ein Paket verloren (z. B. durch Interferenz), merkt das weder Server noch Client automatisch.
- **Anwendungsfall**: Häufige, zeitkritische Daten, bei denen ein gelegentlicher Verlust unkritisch ist – z. B. Herzfrequenz-Messungen, Beschleunigungssensor-Daten, Live-Tracking.

### Indicate

- **Mit Acknowledgement**: Der Client muss den Empfang über ein GATT-Level-ACK (Handle Value Confirmation) bestätigen, bevor der Server die nächste Indication senden darf.
- **Langsamer**: Durch den Bestätigungs-Roundtrip entsteht zusätzliche Latenz.
- **Zuverlässiger**: Der Server weiß, ob die Daten angekommen sind, und kann bei Ausbleiben der Bestätigung reagieren (z. B. erneut senden oder Fehler behandeln).
- **Anwendungsfall**: Wichtige, nicht wiederholbare Daten, bei denen Verlust problematisch wäre – z. B. Konfigurationsänderungen, kritische Statuswechsel, medizinische Alarme.

### Technischer Hintergrund

Beide werden über das **Client Characteristic Configuration Descriptor (CCCD)** aktiviert – ein 2-Byte-Wert, bei dem Bit 0 Notify und Bit 1 Indicate aktiviert (0x0001 bzw. 0x0002). Eine Characteristic kann theoretisch beide Properties gleichzeitig unterstützen, meist wird aber nur eine genutzt.

| | Notify | Indicate |
|---|---|---|
| Bestätigung | Nein | Ja (ATT-Ebene) |
| Durchsatz | Höher | Niedriger |
| Zuverlässigkeit | Geringer | Höher |
| Typisches Beispiel | Sensordaten-Stream | Kritische Einzelwerte |

**Faustregel**: Wenn ein verlorenes Paket egal ist, weil bald der nächste Wert kommt → Notify. Wenn jeder einzelne Wert ankommen muss → Indicate.

---

## 2. Muss der Client sich für diese Nachrichten subscriben?

Ja, genau – der Client muss sich explizit "subscriben", damit er Notifications oder Indications empfängt.

### Wie funktioniert das technisch?

Der Client schreibt dazu einen Wert in das **CCCD (Client Characteristic Configuration Descriptor)** der jeweiligen Characteristic:

| Wert (Bit) | Bedeutung |
|---|---|
| `0x0000` | Weder Notify noch Indicate (abbestellt) |
| `0x0001` | Notify aktiviert |
| `0x0002` | Indicate aktiviert |

Das geschieht über einen ganz normalen **GATT Write Request** auf das CCCD-Handle (nicht auf die Characteristic selbst, sondern auf deren Deskriptor).

### Ablauf in der Praxis

1. Client entdeckt die Services/Characteristics des Peripherals (Service Discovery).
2. Client prüft, ob die Characteristic die Property `Notify` oder `Indicate` unterstützt (steht in den Characteristic Properties).
3. Client sucht das zugehörige CCCD (UUID `0x2902`).
4. Client schreibt `0x0001` (Notify) oder `0x0002` (Indicate) in dieses CCCD.
5. Ab diesem Zeitpunkt sendet der Server bei Wertänderungen automatisch Notifications/Indications an diesen Client.

### Wichtige Details

- **Ohne Subscription passiert nichts**: Der Server sendet standardmäßig keine Notify/Indicate-Pakete, selbst wenn die Characteristic das unterstützt.
- **Pro Client individuell**: Das CCCD ist üblicherweise pro Verbindung/Client separat gespeichert (bei Bonded Devices bleibt die Subscription meist über Verbindungsabbrüche hinweg erhalten, da der Server sie im Bond-Storage merkt).
- **Unsubscribe**: Der Client kann jederzeit `0x0000` schreiben, um sich wieder abzumelden.
- **In der Praxis übernehmen das oft Bibliotheken/APIs automatisch** (z. B. `setNotificationEnabled()` unter Android oder `setNotifyValue:forCharacteristic:` unter iOS) – im Hintergrund läuft aber genau dieser CCCD-Write ab.

**Kurz gesagt**: Kein Write ins CCCD → keine Notify/Indicate-Nachrichten, egal wie sehr sich der Wert auf dem Server ändert.

---

## 3. "Server sendet für diesen Client" – wie funktioniert die Adressierung?

Frage: Kann eine BLE-Nachricht/Paket nur für einen speziellen Client gesendet werden? Ist in der Nachricht eine ID oder Adresse für den Client gespeichert? Was machen die anderen Clients?

### BLE-Verbindungen sind Punkt-zu-Punkt

Anders als man vielleicht denkt, gibt es bei einer BLE-Connection **keine "Broadcast an alle Clients"**. Eine Notify/Indicate-Nachricht wird nicht wie eine Rundfunknachricht ausgestrahlt, die dann alle empfangen und anhand einer Adresse filtern. Stattdessen:

- Jede **Connection** zwischen Peripheral (Server) und Central (Client) ist eine **eigene, dedizierte Punkt-zu-Punkt-Verbindung** auf Link-Layer-Ebene.
- Diese Verbindung nutzt eine eigene **Access Address** (eine 32-Bit-Zufallszahl, die bei Verbindungsaufbau ausgehandelt wird) sowie eigene Timing-Parameter (Connection Interval, Channel-Hopping-Sequenz etc.).
- Beide Geräte "sprechen" quasi nur noch in diesem exklusiven Zeitfenster/Kanal-Muster miteinander.

Es steckt also **keine Client-ID in der Notify-Nachricht selbst** – sie wird schlicht nicht gebraucht, weil die ganze Verbindung physisch/logisch schon exklusiv zwischen genau diesen zwei Geräten läuft.

### Was ist mit mehreren Clients?

Das hängt vom Peripheral ab:

**Klassischer Fall (die meisten einfachen BLE-Geräte, z. B. Fitness-Tracker):**
- Das Peripheral erlaubt nur **eine aktive Verbindung gleichzeitig**.
- Verbindet sich Client B, während Client A schon verbunden ist, lehnt das Peripheral entweder ab oder trennt A.
- Andere Geräte in der Umgebung, die nicht verbunden sind, sehen nur die **Advertising-Pakete** (die tatsächlich Broadcast sind), aber keine Notify/Indicate-Daten.

**Peripherals mit Multi-Connection-Support:**
- Manche BLE-Chips/Stacks unterstützen mehrere gleichzeitige Verbindungen (abhängig von RAM/Controller, z. B. bis zu 4, 8 oder mehr Links).
- Dann unterhält der Server **für jede Connection einen eigenen CCCD-Wert** und **eigene Link-Layer-Ressourcen**.
- Wenn ein Wert sich ändert, prüft der GATT-Server-Stack intern: "Welche Connections haben für diese Characteristic Notify/Indicate aktiviert?" und sendet dann **separate Pakete auf jeder betroffenen Connection einzeln**, nacheinander im jeweiligen Connection-Event.
- Technisch ist das aus Sicht der Funkschicht wie mehrere unabhängige "Gespräche", die zeitlich verschachtelt ablaufen (Time-Division), nicht ein einziges Paket, das mehrere Empfänger hätten.

### Zum Vergleich: Advertising ist tatsächlich Broadcast

Bevor eine Connection besteht, sendet das Peripheral **Advertising Packets** – das *ist* echter Broadcast (ungerichtet, jeder in Reichweite kann mithören). Aber sobald eine Verbindung aufgebaut ist, verlässt man diesen Broadcast-Modus komplett und wechselt in den beschriebenen exklusiven Connection-Modus.

### Kurz zusammengefasst

| | Advertising | Notify/Indicate |
|---|---|---|
| Ziel | Alle in Reichweite | Nur der verbundene Client |
| Mechanismus | Echter Broadcast | Dedizierte Punkt-zu-Punkt-Verbindung |
| Adressierung im Paket nötig? | Ja (Advertiser-Adresse) | Nein (Verbindung selbst ist exklusiv) |
| Mehrere Empfänger möglich? | Ja, beliebig viele | Nur wenn Peripheral Multi-Connection unterstützt – dann separate Pakete pro Connection |

Die "Adressierung" passiert also nicht *innerhalb* der Nachricht, sondern schon *auf der Ebene, welche physische Verbindung überhaupt genutzt wird*.

---

## 4. Ablauf einer Indication im Detail (Zephyr-Beispiel)

Beispielcode (Server, Zephyr):

```c
int my_lbs_send_button_state_indicate(bool button_state)
{
    if (!indicate_button_enabled)
    {
        return -EACCES;
    }

    /* STEP 5.2 - Populate the indication */
    ind_params.attr = &my_lbs_svc.attrs[2];
    ind_params.func = indicate_cb;
    ind_params.destroy = NULL;
    ind_params.data = &button_state;
    ind_params.len = sizeof(button_state);
    return bt_gatt_indicate(NULL, &ind_params);
}
```

### Serverseite

```c
return bt_gatt_indicate(NULL, &ind_params);
```

Das löst auf ATT-Ebene das Senden einer **Handle Value Indication** PDU aus (ATT-Opcode `0x1D`). Diese enthält das Attribute-Handle (welche Characteristic) und den Wert (`button_state`).

### Clientseite – was passiert dort?

Der Client-Applikationscode muss dafür **nichts Explizites tun**. Der ATT/GATT-Layer im Bluetooth-Host-Stack des Clients (egal ob Zephyr, Android, iOS, ...) kümmert sich automatisch darum:

1. Der Client-Stack empfängt die Handle Value Indication PDU.
2. Der Client-Stack antwortet **automatisch** mit einer **Handle Value Confirmation** (ATT-Opcode `0x1E`) – reine Protokoll-Ebene, keine Anwendungslogik.
3. *Zusätzlich* ruft der Stack den vom Client registrierten **Notification/Indication-Callback** auf, damit die Client-App den empfangenen Wert verarbeiten kann (z. B. `bt_gatt_subscribe()` mit einem `notify_cb` in Zephyr).

Der Client-Entwickler muss also nur beim Subscriben (CCCD-Write) einen Callback registrieren, der aufgerufen wird, *sobald der Wert ankommt* – das Senden der Confirmation läuft im Hintergrund automatisch im Stack.

### Serverseite – `indicate_cb`

Diese Callback-Funktion auf **Serverseite** wird aufgerufen, **sobald die Confirmation vom Client beim Server ankommt** (oder ein Fehler/Timeout auftritt). Typische Signatur in Zephyr:

```c
static void indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
    // err == 0: Client hat bestätigt (Confirmation kam an)
    // err != 0: Fehler oder Timeout (Client hat nicht innerhalb der Zeit bestätigt)
}
```

### Zusammengefasster Ablauf

```
Server                                      Client
------                                      ------
bt_gatt_indicate()
  → sendet Handle Value Indication  ------->  ATT-Stack empfängt Indication
                                               ATT-Stack sendet automatisch
  ATT-Stack empfängt Confirmation  <-------    Handle Value Confirmation
                                               (App-Callback wird zusätzlich
  indicate_cb() wird aufgerufen                 aufgerufen, um Wert zu lesen)
  (err == 0 → erfolgreich bestätigt)
```

**Kurz:** Der Client "tut" auf Anwendungsebene nichts Explizites für die Confirmation – das übernimmt der Bluetooth-Host-Stack automatisch als Teil des ATT-Protokolls. Die App auf Client-Seite bekommt nur ihren registrierten Callback aufgerufen, um an den eigentlichen Wert zu kommen.
