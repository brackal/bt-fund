# BLE Pairing, Bonding und Sicherheit (Kurzfassung)

## Grundbegriffe

- **Verschlüsseln** = Encrypt
- **Entschlüsseln** = Decrypt
- **Authentifizierung** = Wer bist du?
- **Autorisierung** = Was darfst du?

---

## Pairing

Pairing ist der Prozess zum:

- Erzeugen von Schlüsseln
- Austauschen von Informationen
- Authentifizieren der Kommunikationspartner
- Vorbereiten der Verschlüsselung

Nach erfolgreichem Pairing kann die BLE-Verbindung verschlüsselt werden.

---

## Bonding

Bonding = Pairing + dauerhaftes Speichern der Schlüssel.

Typische gespeicherte Informationen:

- LTK (Long Term Key)
- IRK (Identity Resolving Key)
- CSRK
- weitere Bonding-Informationen

Vorteil:

- Beim nächsten Verbindungsaufbau ist kein erneutes Pairing notwendig.
- Die Geräte können direkt wieder eine verschlüsselte Verbindung aufbauen.

---

## Pairing-Methoden

### 1. Just Works

- Verschlüsselung: Ja
- Authentifizierung: Nein
- MITM-Schutz: Nein

Der Benutzer bestätigt lediglich die Verbindung.

Es wird nicht geprüft, ob die Gegenstelle tatsächlich das erwartete Gerät ist.

### 2. Passkey Entry

- Verschlüsselung: Ja
- Authentifizierung: Ja
- MITM-Schutz: Ja

Ein Gerät zeigt einen Passkey an, das andere gibt ihn ein.

### 3. Numeric Comparison

- Verschlüsselung: Ja
- Authentifizierung: Ja
- MITM-Schutz: Ja

Beide Geräte zeigen dieselbe Zahl an.

Der Benutzer bestätigt die Übereinstimmung.

### 4. Out Of Band (OOB)

- Verschlüsselung: Ja
- Authentifizierung: Ja
- MITM-Schutz: Ja

Die Authentifizierungsdaten werden über einen anderen Kanal übertragen, z. B.:

- NFC
- QR-Code
- Kabel

---

## Was bedeutet „Unauthenticated“ bei Just Works?

Die Geräte erzeugen zwar einen gemeinsamen Schlüssel und verschlüsseln die Verbindung.

Es existiert jedoch kein Nachweis, dass die Gegenstelle wirklich die gewünschte Gegenstelle ist.

Daher ist Just Works anfällig für Man-in-the-Middle-Angriffe während des Pairings.

---

## MITM bei Just Works

Der Angreifer knackt keinen Schlüssel.

Stattdessen baut er zwei getrennte Verbindungen auf:

```text
Client <-> MITM <-> Server
```

Dabei entstehen:

```text
Client <-> Schlüssel A <-> MITM <-> Schlüssel B <-> Server
```

Der Angreifer kennt beide Schlüssel und kann Daten lesen, verändern und erneut verschlüsseln.

---

## Kann sich ein MITM nachträglich einklinken?

Nein.

Wenn Pairing und Verschlüsselung bereits abgeschlossen sind, kann sich ein später auftauchender Angreifer nicht einfach in die bestehende Verbindung einklinken.

Der kritische Zeitpunkt ist das ursprüngliche Pairing.

---

## Ablauf einer BLE-Verbindung

```text
Advertising
    ↓
Verbindung (Connect)
    ↓
Pairing
    ↓
Schlüsselerzeugung
    ↓
Verschlüsselung
    ↓
Bonding (optional)
```

Wichtig:

```text
Connect ≠ Pairing
```

Eine Verbindung kann bereits bestehen, obwohl noch kein Pairing durchgeführt wurde.

---

## Wie wird die Pairing-Methode ausgewählt?

Die Auswahl basiert auf:

- OOB-Flag
- MITM-Flag
- I/O Capabilities

### I/O Capabilities

- NoInputNoOutput
- DisplayOnly
- KeyboardOnly
- DisplayYesNo
- KeyboardDisplay

Beispiele:

| Central | Peripheral | Ergebnis |
|----------|------------|-----------|
| NoInputNoOutput | NoInputNoOutput | Just Works |
| DisplayOnly | KeyboardOnly | Passkey Entry |
| DisplayYesNo | DisplayYesNo | Numeric Comparison |

---

## LE Secure Connections

Moderne BLE-Geräte verwenden typischerweise LE Secure Connections.

Ablauf:

1. Central erzeugt ECC-Schlüsselpaar.
2. Peripheral erzeugt ECC-Schlüsselpaar.
3. Austausch der Public Keys.
4. Berechnung eines gemeinsamen Secrets mittels ECDH.
5. Ableitung von LTK und weiteren Schlüsseln.
6. Aktivierung der Verschlüsselung.

Die Private Keys verlassen das jeweilige Gerät nicht.

---

## Wer erzeugt die Schlüssel?

Nicht die Anwendung.

Der Bluetooth-Stack übernimmt:

- Erzeugen der ECC-Schlüssel
- Austausch der Public Keys
- ECDH-Berechnung
- Schlüsselableitung
- Aktivierung der Verschlüsselung

Die Anwendung konfiguriert lediglich Sicherheitsanforderungen und reagiert auf Pairing-Events.

---

## Wie erkennen sich gebondete Geräte wieder?

### IRK (Identity Resolving Key)

Dient zur Wiedererkennung von Geräten.

### LTK (Long Term Key)

Dient zur Wiederherstellung einer verschlüsselten Verbindung.

Merksatz:

- IRK → Wer bist du?
- LTK → Lass uns verschlüsseln.

---

## Bekannte und unbekannte Geräte

Ein Gerät wird erst durch erfolgreiches Bonding zu einem bekannten Gerät.

Das Peripheral speichert die Bonding-Informationen.

Spätere Verbindungen desselben Geräts werden wiedererkannt.

Neue Geräte gelten zunächst als unbekannt.

Ob neue Pairings erlaubt sind, entscheidet das Peripheral.

Beispiele:

- Pairing immer erlauben
- Nur bereits gebondete Geräte zulassen
- Pairing nur im Commissioning-Modus erlauben

---

## Pairing Request

Der Pairing Request wird nach dem Verbindungsaufbau gesendet.

Typische Auslöser:

- Sicherheitsanforderung durch den Central
- Zugriff auf eine geschützte Characteristic
- Explizite Anforderung durch die Anwendung

Der Pairing Request enthält u. a.:

- Bonding gewünscht?
- MITM-Schutz gewünscht?
- OOB verfügbar?
- I/O Capabilities

Anschließend wird die passende Pairing-Methode bestimmt.

---

## Speicherung der Bonding-Schlüssel auf dem nRF52840

Der nRF52840 besitzt kein dediziertes Secure Flash oder Hardware Key Vault.

Typischerweise werden Bonding-Daten im normalen internen Flash gespeichert.

Gespeichert werden beispielsweise:

- LTK
- IRK
- CSRK
- Bonding-Informationen

Die während ECDH erzeugten temporären Private Keys verbleiben üblicherweise nur im RAM und werden anschließend verworfen.

Zum Schutz vor Auslesen sollte APPROTECT aktiviert werden.
