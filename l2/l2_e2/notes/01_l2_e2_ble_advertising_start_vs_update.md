# BLE Advertising: `bt_le_adv_start()` vs. `bt_le_adv_update_data()`

Zusammenfassung zum Zusammenspiel der beiden Zephyr-BLE-Funktionen.

## `bt_le_adv_start()`

- Konfiguriert den Bluetooth-Controller und **startet** das Advertising.
- Danach sendet der **Controller** (nicht der Host-Code) die Advertising-Pakete zyklisch/periodisch aus – autonom im Hintergrund.
- **Intervall:** liegt zwischen `interval_min` und `interval_max` (Einheiten von 0,625 ms); der Controller wählt selbst einen Wert dazwischen, u. a. um Kollisionen zu vermeiden.
- **Kanäle:** Standardmäßig werden die drei primären Advertising-Kanäle (37, 38, 39) bei jedem Advertising-Event nacheinander bedient.
- **Dauer:** Läuft weiter, bis `bt_le_adv_stop()` aufgerufen wird – außer bei speziellen Optionen (z. B. `BT_LE_ADV_OPT_ONE_TIME`, oder automatischer Stopp bei verbindbarem Advertising nach Connection).
- **Extended Advertising:** Bei `bt_le_ext_adv_start()` gibt es zusätzlich `timeout` und `num_events`, um die Paketanzahl zu begrenzen.

## `bt_le_adv_update_data()`

- Wird z. B. bei einem Button-Press aufgerufen, **während** das Advertising bereits läuft.
- **Sendet selbst kein zusätzliches Paket** und löst **kein Extra-Event** aus.
- Tauscht nur den **Inhalt** der Advertising Data (AD) / Scan Response Data aus, der beim **nächsten regulären** Advertising-Event ohnehin gesendet wird.
- Der zyklische Sende-Rhythmus (Intervall) bleibt unverändert – nur die Nutzdaten in den kommenden Paketen ändern sich.

### Voraussetzungen & Einschränkungen

- Advertising muss bereits laufen (`bt_le_adv_start()` vorher erfolgreich aufgerufen), sonst Fehler (typischerweise `-EAGAIN` o. ä., je nach Zephyr-Version).
- Der Advertising-**Typ** (connectable / non-connectable / scannable) kann über diese Funktion **nicht** geändert werden – nur die Nutzdaten.
- Längenbeschränkung bleibt bestehen: Legacy Advertising max. 31 Byte für AD bzw. Scan Response.

## Ablauf beim Button-Press

1. Advertising läuft bereits (`bt_le_adv_start()`), Controller sendet zyklisch Pakete mit den alten Daten.
2. Button-Press-Callback ruft `bt_le_adv_update_data()` auf.
3. Host schickt die neuen Daten über HCI an den Controller (`HCI_LE_Set_Advertising_Data` Command).
4. Ab dem **nächstfolgenden regulären** Advertising-Event sendet der Controller die neuen Daten – nicht sofort "zwischengeschoben", sondern eingebettet in den normalen zyklischen Ablauf.

### Praktischer Hinweis

Durch die HCI-Kommunikation (Host → Controller) kann eine minimale Latenz entstehen, bis die neuen Daten beim Controller ankommen (meist wenige Millisekunden). Für die meisten Anwendungsfälle unkritisch, aber bei sehr kurzen Advertising-Intervallen (z. B. 20 ms) theoretisch relevant: Ist der nächste Event bereits "unterwegs", bevor die neuen Daten ankommen, greifen die neuen Daten ggf. erst beim übernächsten Event.

## Legacy vs. Extended Advertising

- **Legacy Advertising:** `bt_le_adv_start()` / `bt_le_adv_update_data()`
- **Extended Advertising:** `bt_le_ext_adv_start()` / `bt_le_ext_adv_set_data()` – analoges Verhalten, aber mit zusätzlichen Nuancen (z. B. bei Periodic Advertising).
