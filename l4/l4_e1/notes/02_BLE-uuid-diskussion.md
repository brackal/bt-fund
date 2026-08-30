# Bluetooth LE UUIDs – Nordic LBS Service & Waldmann Leuchten

## 1. Woher kommen die Zahlen in der Nordic LBS UUID?

Nordics LED Button Service (LBS) verwendet die UUID:

```
00001523-1212-EFDE-1523-785FEABCD123
```

Diese UUID ist **nicht offiziell** von der Bluetooth SIG vergeben, sondern eine von Nordic Semiconductor frei gewählte **custom 128-bit UUID**, die als "Basis" für eigene Beispiel-Services im nRF5 SDK / nRF Connect SDK benutzt wird.

### Aufbau einer 128-bit UUID

Allgemeines Format:
```
XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
```

Bei offiziellen Bluetooth-SIG-UUIDs (z. B. 0x180F für Battery Service) wird intern eine 128-Bit-UUID mit standardisierter Basis verwendet:
```
0000XXXX-0000-1000-8000-00805F9B34FB
```
Nur die ersten 4 Hex-Ziffern (`XXXX`) unterscheiden sich je nach Service/Characteristic – der Rest ist die offizielle "Bluetooth Base UUID".

### Nordics eigenes Schema

Nordic hat dasselbe Prinzip übernommen, aber mit einer **eigenen, selbst generierten Basis**:
```
0000XXXX-1212-EFDE-1523-785FEABCD123
```

Der Teil `1212-EFDE-1523-785FEABCD123` ist eine einmal zufällig erzeugte 128-Bit-UUID, die Nordic als feste Vorlage für alle eigenen Tutorial-Services wiederverwendet – ohne tiefere Bedeutung, einfach um Kollisionen mit fremden Services zu vermeiden.

Nur das erste Feld (`0000XXXX`) ändert sich je nach Service/Characteristic, z. B.:

| Element | UUID |
|---|---|
| Service (LBS) | `00001523-...` |
| Button-Characteristic | `00001524-...` |
| LED-Characteristic | `00001525-...` |

Der gemeinsame "Rest" zeigt, dass diese Characteristics zur selben Nordic-Service-Familie gehören; das erste Feld identifiziert den konkreten Service/die Characteristic.

---

## 2. Kann dieselbe UUID auch bei einem anderen Hersteller vorkommen?

**Theoretisch ja, praktisch fast ausgeschlossen.**

- Da die UUID **nicht bei der SIG registriert** ist, gibt es keine zentrale Instanz, die Kollisionen verhindert. Jeder Hersteller kann sich selbst eine 128-Bit-UUID ausdenken.
- Eine korrekt generierte v4-UUID hat einen riesigen Wertebereich (2^122 mögliche Werte) – eine zufällige Kollision ist astronomisch unwahrscheinlich.
- Nordics UUID ist kein reines v4-UUID (lesbares Schema statt reinem Zufall), aber da die Basis selbst zufällig gewählt wurde, bleibt das Kollisionsrisiko trotzdem sehr gering.

**Die eigentliche Gefahr in der Praxis:** Viele Entwickler kopieren Nordics Beispiel-UUID 1:1 aus dem Tutorial, ohne sie zu ändern. Dadurch tauchen tatsächlich mehrere unterschiedliche BLE-Produkte mit exakt derselben LBS-UUID auf – nicht durch Zufall, sondern durch Copy-Paste.

**Empfehlung:** Für eigene Custom Services eine echte, frisch generierte v4-UUID verwenden statt Nordics Beispiel-UUID zu übernehmen.

---

## 3. Wo kann man eine UUID generieren?

**Online-Tools:**
- uuidgenerator.net
- uuid.rocks

**Kommandozeile:**

Linux/macOS:
```bash
uuidgen
```

Python:
```bash
python3 -c "import uuid; print(uuid.uuid4())"
```

Node.js:
```bash
node -e "console.log(require('crypto').randomUUID())"
```

Für BLE-Zwecke: Nordic bietet auch einen Custom-UUID-Generator in nRF Connect for Desktop. Wichtig ist nur, dass es sich um eine echte, zufällig generierte UUID handelt und nicht um eine 1:1-Kopie aus einem Tutorial.

---

## 4. Waldmann Leuchten – gibt es dokumentierte UUIDs?

Herbert Waldmann GmbH & Co. KG stellt mit dem **TALK MODUL Bluetooth G1** ein nachrüstbares Funkmodul her, das über die **LIGHT ADMIN App** (Facility Manager) bzw. **LIGHT USER App** (Endnutzer) konfiguriert wird.

Recherche-Ergebnisse:
- Das TALK MODUL Bluetooth G1 ist ein nachrüstbares Funkmodul, das in Verbindung mit der Waldmann-Leuchtensteuerung auf An- und Abwesenheit reagiert und Leuchten drahtlos in Gruppen kommunizieren lässt (Schwarmsteuerung); Parametereinstellungen erfolgen über die LIGHT ADMIN App.
- Mit der LIGHT USER App können Waldmann-Leuchten mit integriertem TALK Bluetooth Modul eingestellt werden; die Verbindung erfolgt per QR-Code, ein separates Bluetooth-Pairing ist nicht nötig.

**Ergebnis:** Es gibt keine öffentlich dokumentierten, konkreten BLE Service-/Characteristic-UUIDs von Waldmann. Das ist proprietäre Firmenschnittstelle und wird – anders als z. B. Nordics öffentlich dokumentierte Tutorial-UUIDs – nicht offengelegt.

### Mögliche nächste Schritte, um an die UUIDs zu kommen:
1. Direkt bei Waldmann (technischer Support/Entwicklung) anfragen.
2. Die UUIDs selbst per BLE-Scan/Sniffing auslesen, z. B. mit der App **nRF Connect** (Android/iOS) während einer bestehenden Verbindung zur Leuchte – dort werden alle Services und Characteristics inklusive UUIDs live angezeigt.
