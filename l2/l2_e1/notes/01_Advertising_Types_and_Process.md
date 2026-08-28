# Bluetooth Low Energy (BLE) – Advertising: Notizen

## 1. Advertising Intervals

> Advertising intervals: The interval at which an advertising packet is sent. In the range of 20 ms to 10.24 s, with a step increase of 0.625 ms.

**Bedeutung:**

- Der **Advertising Interval** ist der Zeitabstand zwischen zwei aufeinanderfolgenden Advertising-Paketen.
- Er kann einen beliebigen Wert **zwischen 20 ms und 10,24 s** annehmen.
- Die **0,625 ms** sind die **Schrittweite (Granularität)**, mit der dieser Wert eingestellt werden kann – nicht ein zusätzlicher, separater Sendetakt.

Intern wird der Intervall-Wert als Zähler in 0,625-ms-Einheiten kodiert:

```
advInterval = N × 0,625 ms
```

mit N zwischen 32 und 16384.

**Beispiele:**

| N | advInterval |
|---|---|
| 32 (Minimum) | 20 ms |
| 160 | 100 ms |
| 16384 (Maximum) | 10,24 s |

➡️ Es gibt also **nur einen** periodischen Sendeabstand – die 0,625 ms sind lediglich die kleinstmögliche Auflösung bei der Konfiguration dieses einen Abstands.

---

## 2. Zufälliger Delay (Kollisionsvermeidung)

> To avoid packet collisions, a random delay of 0-10 ms is added before each advertisement packet.

Vor **jedem einzelnen** Advertising-Paket wird zusätzlich ein zufälliger Wert (advDelay) zwischen 0 ms und 10 ms addiert:

```
tatsächliches Intervall = advInterval + advDelay   (advDelay ∈ [0 ms, 10 ms], zufällig)
```

### Rechenbeispiel mit N = 160

Basis-Intervall:

```
advInterval = 160 × 0,625 ms = 100 ms
```

Beispielhafter Sendeverlauf:

| Paket Nr. | advDelay (zufällig) | tatsächliches Intervall | Sendezeitpunkt (kumuliert) |
|---|---|---|---|
| 1 | 3 ms | 103 ms | 103 ms |
| 2 | 7 ms | 107 ms | 210 ms |
| 3 | 0 ms | 100 ms | 310 ms |
| 4 | 9 ms | 109 ms | 419 ms |
| 5 | 4 ms | 104 ms | 523 ms |

**Wichtig:**

- Das Basisintervall (100 ms) bleibt über die gesamte Advertising-Phase konstant.
- Nur der zusätzliche Zufalls-Offset (0–10 ms) ändert sich bei jedem Paket neu.
- Das tatsächliche Intervall liegt somit immer zwischen 100 ms und 110 ms.
- **Zweck:** Verhindert, dass Geräte mit gleichem Intervall (z. B. 100 ms) ihre Pakete immer zur exakt gleichen Zeit senden und dadurch kollidieren.

---

## 3. Advertising-Kanäle (37, 38, 39)

BLE nutzt 40 Kanäle im Bereich **2402 MHz – 2480 MHz** (2 MHz Abstand). Drei davon sind reine **Advertising-Kanäle**, die übrigen 37 sind **Datenkanäle**.

| Kanal | Frequenz | Funktion |
|---|---|---|
| 37 | 2402 MHz | Primary Advertisement |
| 0–10 | 2404–2424 MHz | Data transfer |
| 38 | 2426 MHz | Primary Advertisement |
| 11–36 | 2428–2478 MHz | Data transfer |
| 39 | 2480 MHz | Primary Advertisement |

### Warum liegen 37, 38, 39 nicht am Ende des Spektrums, sondern verteilt (Rand – Mitte – Rand)?

Grund ist die **Überlappung mit klassischen WLAN-Kanälen** im 2,4-GHz-Band:

- WiFi-Kanal 1: ~2412 MHz
- WiFi-Kanal 6: ~2437 MHz
- WiFi-Kanal 11: ~2462 MHz

Die BLE-Advertising-Kanäle 37, 38, 39 (2402 / 2426 / 2480 MHz) liegen genau in den **Lücken zwischen** diesen stark genutzten WiFi-Kanälen bzw. am Rand des Bandes. Damit wird das Risiko von Störungen zwischen Advertising-Paketen (wichtig für die Entdeckbarkeit eines Geräts) und WLAN-Verkehr minimiert.

➡️ Die Positionierung ist also **gezielt gewählt**, um Interferenzen mit gängigem WLAN-Verkehr zu vermeiden – nicht willkürlich am Ende der Nummerierung platziert.

---

## 4. Scan Request / Scan Response

> **Scan request**: A message sent by a central device to a peripheral to request additional information not present in the advertisement packet.
> 
> **Scan response**: A message sent as a response to a scan request, containing additional user data.

### Werden diese auf den Kanälen 37, 38, 39 kommuniziert?

**Ja** – im klassischen (Legacy) Advertising laufen Scan Request und Scan Response auf **demselben Kanal**, auf dem das ursprüngliche Advertising-Paket gesendet wurde.

**Ablauf:**

1. Peripheres Gerät sendet Advertising-Paket auf z. B. Kanal 37.
2. Central-Gerät sendet **sofort** (innerhalb des Inter Frame Space, IFS) einen **Scan Request** – auf **demselben Kanal 37**.
3. Peripheres Gerät antwortet direkt mit dem **Scan Response** – ebenfalls auf **Kanal 37**.

Dies gilt unabhängig davon, ob das Advertising auf Kanal 37, 38 oder 39 stattfindet.

**Warum auf demselben Kanal:**

- Sehr enges Zeitfenster (Mikrosekundenbereich) zwischen Advertisement → Scan Request → Scan Response
- Beide Geräte kennen den aktuellen Kanal bereits aus dem empfangenen Advertisement
- Ein Kanalwechsel wäre unnötig kompliziert und würde Zeit kosten

**Hinweis – Extended Advertising (seit Bluetooth 5):**

Hier dient das Paket auf Kanal 37/38/39 nur als Hinweis mit einem Zeiger auf ein sekundäres Advertising-Event. Die eigentlichen Daten sowie ggf. Scan Request/Response können dann auf den **Datenkanälen 0–36** stattfinden.

## 5. Advertising Typen

> **Connectable vs. non-connectable:** Determines whether the central can connect to the peripheral or not.  
> 
>**Scannable vs. non-scannable:** Determines if the peripheral accepts scan requests from a scanner.  
>**Directed vs. undirected:** Determines whether advertisement packets are targeted to a specific scanner or not.


|   |   |   |   |
|---|---|---|---|
||Connectable|Scannable|Directed|
|`ADV_IND`|x|x||
|`ADV_DIRECT_IND`|x||x|
|`ADV_SCAN_IND`||x||
|`ADV_NONCONN_IND`|||

**IND** steht für **"Indication"** (deutsch etwa: Hinweis/Anzeige). Es zeigt an, dass es sich um ein Advertising-Paket handelt – also ein Paket, mit dem ein Gerät seine Anwesenheit "anzeigt" bzw. bekannt macht, unabhängig davon, ob es verbindbar, gerichtet oder scanbar ist.
