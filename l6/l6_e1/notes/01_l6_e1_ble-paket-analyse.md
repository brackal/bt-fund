# BLE-Paketanalyse – nRF-Sniffer-Capture

**Rohdaten (56 Byte):**

```
0e310003b933020a01261b0000e8152b11d6be898e461e23a29b6a692b
0201040e094e6f726469635f426561636f6e05ff59000900d1ea17
```

**Ergebnis in einem Satz:** Ein Nordic-Beacon mit der Zufallsadresse `2B:69:6A:9B:A2:23` sendet auf Kanal 38 ein scanbares, nicht verbindbares Advertising mit dem Namen „Nordic_Beacon" und 2 Byte herstellerspezifischen Daten; empfangen mit −27 dBm, CRC gültig.

---

## 1. Aufbau des Mitschnitts

Das Format ist `LINKTYPE_NORDIC_BLE`, Protokollversion 3. Nur die letzten 39 Byte wurden tatsächlich über die Luft übertragen:

| Bereich | Bytes | Herkunft |
|---|---|---|
| Sniffer-Header | 0–6 | vom Sniffer-Board erzeugt |
| Metadaten (Empfangsinfos) | 7–16 | vom Sniffer-Board erzeugt |
| Link-Layer-Paket | 17–55 | **über die Luft** |

Die ersten 17 Byte entstehen erst im Sniffer: die Firmware empfängt das Funkpaket, misst RSSI, Kanal, Zeitstempel und CRC-Ergebnis, stellt das als Pseudo-Header voran und schickt alles SLIP-kodiert über USB-CDC/COM an den PC. Dort baut der Python-Wrapper die pcap-Records, die Wireshark mit dem `nordic_ble`-Dissector aufdröselt.

Werte wie RSSI, Kanal und Zeitstempel *können* gar nicht im Paket stehen – das sind Eigenschaften des Empfangs, nicht des Senders. Genau dafür gibt es Meta-Header; dasselbe Prinzip nutzt Radiotap bei WLAN.

### Sniffer-Header (Byte 0–6)

| Bytes | Wert | Bedeutung |
|---|---|---|
| `0e` | 14 | Board-ID |
| `31 00` | 49 | Payload-Länge (LE) → 7 + 49 = 56 ✓ |
| `03` | 3 | Protokollversion |
| `b9 33` | 13241 | Paketzähler des Sniffers |
| `02` | 2 | Pakettyp = EVENT_PACKET_ADVERTISING |

### Metadaten (Byte 7–16)

| Bytes | Wert | Bedeutung |
|---|---|---|
| `0a` | 10 | Länge dieses Metadaten-Headers |
| `01` | `0b00000001` | Flags: **CRC OK**, unverschlüsselt, PHY = LE 1M |
| `26` | 38 | BLE-Kanal 38 (Advertising, 2426 MHz) |
| `1b` | 27 | RSSI = **−27 dBm** |
| `00 00` | 0 | Event Counter (nur bei Verbindungen relevant) |
| `e8 15 2b 11` | 0x112B15E8 | Zeitstempel = 288 036 328 µs ≈ **288,04 s** |

---

## 2. Das Link-Layer-Paket (Byte 17–55)

```
[Preamble 0xAA]  [AA d6be898e]  [PDU 46 1e + 30 Byte]  [CRC d1ea17]
 nicht im Dump    4 Byte          2 + 30 Byte            3 Byte
```

### Access Address

`d6 be 89 8e` → **0x8E89BED6**, die feste Advertising-Access-Address.

### PDU-Header `46 1e`

`0x46` = `0100 0110`:

| Bits | Wert | Bedeutung |
|---|---|---|
| 0–3 | `0110` | PDU-Typ = **ADV_SCAN_IND** (scanbar, nicht verbindbar, undirected) |
| 4 | 0 | RFU |
| 5 | 0 | ChSel |
| 6 | 1 | **TxAdd = 1 → Random Address** |
| 7 | 0 | RxAdd |

`0x1e` = 30 Byte Payload = 6 Byte Adresse + 24 Byte AdvData ✓

### Senderadresse

`23 a2 9b 6a 69 2b` → little-endian gelesen: **2B:69:6A:9B:A2:23**

Die obersten zwei Bits des MSB (`0x2B` = `00`…) machen es formal zu einer *non-resolvable private address*. Bei einem echten Beacon wäre eher eine Static Random Address zu erwarten (Top-Bits `11`).

### AdvData (24 Byte)

Drei AD-Strukturen nach dem Muster *Länge / Typ / Wert*:

| Bytes | Typ | Inhalt |
|---|---|---|
| `02 01 04` | 0x01 Flags | `0x04` = BR/EDR Not Supported (LE-only, kein Discoverable-Flag – passt zu non-connectable) |
| `0e 09 4e6f…6e` | 0x09 Complete Local Name | **„Nordic_Beacon"** (13 Zeichen) |
| `05 ff 59 00 09 00` | 0xFF Manufacturer Specific | Company ID `0x0059` = **Nordic Semiconductor ASA**, Nutzdaten `09 00` |

### CRC

`d1 ea 17` – 24-Bit-Prüfsumme über die PDU. Ob sie stimmt, steht nicht im Paket, sondern im Flags-Byte des Sniffer-Headers: die Prüfung macht der Sniffer.

---

## 3. Hintergrund: Preamble

### Was sie ist

Ein reines Aufweck-Bitmuster ganz am Anfang jedes Funkpakets – abwechselnd 0 und 1, sonst nichts:

| PHY | Länge | Muster | Dauer |
|---|---|---|---|
| LE 1M | 8 Bit | `0xAA` oder `0x55` | 8 µs |
| LE 2M | 16 Bit | `0xAAAA` oder `0x5555` | 8 µs |
| LE Coded | 80 Symbole | 10× `00111100` | 80 µs |

Beim 2M-PHY wird sie verdoppelt, damit sie in *Zeit* gleich lang bleibt – 8 µs sind das, was der Empfänger physikalisch braucht, unabhängig von der Symbolrate.

### Wofür

Der Empfänger schaltet sein Radio erst kurz vorher ein und weiß im ersten Moment nichts. In diesen 8 µs erledigt er drei analoge Hausaufgaben:

1. **AGC (Verstärkungsregelung).** −27 dBm wie hier oder −95 dBm am Rand der Reichweite: der Verstärker muss sich einpegeln, sonst übersteuert oder rauscht der Rest.
2. **Frequenz- und DC-Offset schätzen.** BLE nutzt GFSK: „1" = Träger + ~250 kHz, „0" = − ~250 kHz. Sender und Empfänger haben getrennte Quarze mit je bis zu ±50 ppm, bei 2,4 GHz also bis zu ±120 kHz Ablage – vergleichbar mit dem halben Nutzhub. Das strikte `1010…` liefert gleich viele Einsen und Nullen, der Träger mittelt sich heraus und die Entscheidungsschwelle steht.
3. **Bit-Timing zurückgewinnen.** Ein alternierendes Muster hat an *jeder* Symbolgrenze eine Flanke – maximale Flankendichte, schnellstes Einrasten der Taktrückgewinnung.

### `0xAA` oder `0x55`?

Regel: Ist das erste gesendete Bit der Access Address eine 0, wird `0xAA` genommen, sonst `0x55`. Gesendet wird LSB first:

```
AA = 0x8E89BED6 → zuerst raus geht 0xD6 = 1101 0110 → erstes Bit = 0
⇒ Preamble 0xAA, gesendet als 0 1 0 1 0 1 0 1
Bitstrom: 0101 0101 | 0 110 1011 …
                    ^ Alternation läuft nahtlos weiter
```

Zwei gleiche Bits an der Nahtstelle würden dem gerade erst einrastenden Timing-Regelkreis im ungünstigsten Moment eine flankenlose Lücke bescheren.

### Was sie nicht ist

- **Kein Sync-Wort.** 8 Bit Alternieren kommen im Rauschen ständig vor. Paketerkennung macht die Access Address.
- **Nicht gewhitened.** Whitening beginnt erst beim PDU-Header.
- **Nicht im CRC.** Das deckt nur die PDU ab.
- **Ohne Informationsgehalt.** Ihr Wert folgt zwingend aus der Access Address.

Deshalb fehlt sie im Dump: die Radio-Hardware verwirft sie nach der Synchronisation. Der Link-Typ heißt entsprechend „BLE link-layer packet *without preamble*".

Zum Vergleich: klassisches Bluetooth BR/EDR nutzt ein 68 bzw. 72 Bit langes Sync-Wort. BLE kommt mit einem Achtel davon aus – ein Teil davon, warum ein Beacon so kurz aufwachen und so wenig Energie verbrauchen kann.

---

## 4. Hintergrund: Access Address

### Zwei Jobs in einem Feld

Die 32 Bit hinter der Preamble sind das einzige Feld, das ungewhitened und ungeschützt übertragen wird – kein CRC, keine Verschlüsselung. Sie erfüllen zwei Aufgaben gleichzeitig:

1. **Sync-Wort.** Der Empfänger füttert den Bitstrom permanent in einen Korrelator. Passt das Muster, ist die Bitgrenze absolut definiert und alles Folgende wird stur abgezählt.
2. **Verbindungskennung.** Auf einem Datenkanal laufen viele fremde Verbindungen auf derselben Frequenz. Die AA trennt „mein Link" von „fremdem Link", lange bevor Entschlüsselung oder Adressprüfung greifen.

### Die feste Advertising-Adresse

`0x8E89BED6` nutzt jedes BLE-Gerät beim Advertising, Scannen und Verbindungsaufbau. Muss es auch: ein Scanner, der die Umgebung noch nicht kennt, kann nur auf ein Muster horchen, das alle teilen. Adressierung findet dort erst eine Ebene höher statt (AdvA, Filter-Policies).

Das Muster ist nicht willkürlich:

```
Wert (MSB→LSB):   1000 1110 1000 1001 1011 1110 1101 0110
On air (LSB→MSB): 0110 1011 0111 1101 1001 0001 0111 0001
```

18 Einsen zu 14 Nullen (nahezu gleichanteilsfrei), 17 Bitwechsel, längste Serie 4 gleiche Bits, flache Autokorrelation neben dem Hauptmaximum.

### Zufällige Adressen bei Verbindungen

Der **Initiator** würfelt beim Verbindungsaufbau eine neue AA aus und teilt sie im `CONNECT_IND` mit; sie gilt für die Lebensdauer der Verbindung. Dasselbe Prinzip bei Periodic Advertising (AA im SyncInfo-Feld) und isochronen Streams.

„Ausgewürfelt" heißt stark eingeschränkt. Der Core Spec verlangt:

- höchstens 6 gleiche Bits in Folge
- nicht die Advertising-AA, und keine, die sich nur in **einem** Bit von ihr unterscheidet
- nicht alle vier Oktette gleich
- höchstens 24 Bitwechsel
- mindestens 2 Bitwechsel in den obersten 6 Bit
- nicht identisch mit einer AA, die das Gerät schon nutzt
- für LE Coded zusätzlich: mindestens drei Einsen in den untersten 8 Bit, höchstens 11 Wechsel in den untersten 16 Bit

Durchgerechnet über den gesamten 32-Bit-Raum bleiben **2 871 059 806 gültige Werte (66,85 %)**, mit den Coded-Regeln 2 441 913 829 (56,86 %). Die Advertising-AA selbst erfüllt ihre eigenen Regeln nur knapp: exakt 2 Wechsel in den obersten sechs Bit, das Minimum.

Jede Regel hat einen physikalischen Grund: lange Serien lassen die Taktrückgewinnung driften und erzeugen Gleichanteil, zu viele Wechsel kosten Bandbreite und schwächen die Korrelationsspitze, und die Ein-Bit-Distanz zur Advertising-AA ist verboten, weil die überall in der Luft liegt – ein einzelner Bitfehler würde sonst eine Verwechslung erzeugen.

### Praktische Konsequenzen

- **Empfänger:** Bei Nordic-Chips landet die AA in den `BASE`/`PREFIX`-Registern; der Treffer erzeugt das `ADDRESS`-Event, bei dem typischerweise auch der RSSI gesampelt wird (hier `1b`). Korrelatoren tolerieren meist ein bis zwei Bitfehler, gelegentliche Fehltreffer fliegen beim CRC raus.
- **Sniffing:** Advertising ist immer mitlesbar, die AA ist bekannt. Eine Verbindung nicht – der Sniffer muss das `CONNECT_IND` erwischen, um die neue AA zu lernen. Deshalb fragt der nRF Sniffer in Wireshark, welchem Gerät er *folgen* soll. Tools wie Ubertooth rekonstruieren die AA stattdessen statistisch aus vielen Paketen.
- **Privacy:** Die AA ist ein zufälliger, aber über die gesamte Verbindung konstanter Identifier im Klartext. Resolvable Private Addresses schützen davor nicht.

### Verbreiteter Denkfehler

„Address" verführt dazu, das für die Geräteadresse zu halten. Ist es nicht – die steht in der PDU (`2B:69:6A:9B:A2:23`). Die Access Address adressiert eine Verbindung bzw. einen Kanal-Kontext, kein Gerät; auf den Advertising-Kanälen adressiert sie gar nichts. Sie geht auch nirgends sonst ein: das Whitening wird aus dem Kanalindex initialisiert, der CRC-Init kommt aus dem `CONNECT_IND`.

---

## Quellen

- [LINKTYPE_NORDIC_BLE – tcpdump.org](https://www.tcpdump.org/linktypes/LINKTYPE_NORDIC_BLE.html)
- Bluetooth Core Specification, Vol 6 Part B (Link Layer Specification) – Preamble, Access Address, Whitening
- [wireshark/epan/dissectors/packet-nordic_ble.c](https://github.com/wireshark/wireshark/blob/master/epan/dissectors/packet-nordic_ble.c)
