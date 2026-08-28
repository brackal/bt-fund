# Erklärung des `adv_work`-Codes

## 1) `adv_work_handler(...)`

```c
static void adv_work_handler(struct k_work *work)
{
    int err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    printk("Advertising successfully started\n");
}
```

Das ist der eigentliche Handler, der das BLE-Advertising startet.

- `bt_le_adv_start(...)` startet die Werbung
- `adv_param`: Parameter für das Advertising
- `ad`: Advertising-Data (normaler Payload)
- `ARRAY_SIZE(ad)`: Anzahl der AD-Elemente
- `sd`: Scan Response Data
- `ARRAY_SIZE(sd)`: Anzahl der Scan-Response-Elemente

Wenn `bt_le_adv_start(...)` einen Fehler zurückgibt, wird dieser mit `printk` ausgegeben.
Wenn es klappt, erscheint die Meldung:

```c
printk("Advertising successfully started\n");
```

---

## 2) `advertising_start()`

```c
static void advertising_start(void)
{
    k_work_submit(&adv_work);
}
```

Diese Funktion stellt die Arbeit für den Zephyr-Worker ein.

- `k_work_submit(&adv_work)` sagt dem System: „Starte später den Handler `adv_work_handler`.“
- Das ist ein asynchroner Mechanismus, also nicht direkt im Main-Thread.

Das ist sinnvoll, weil das Starten von Bluetooth-Advertising manchmal besser in einem Work-Task erfolgt.

---

## 3) `recycled_cb()`

```c
static void recycled_cb(void)
{
    printk("Connection object available from previous conn. Disconnect is complete!\n");
    advertising_start();
}
```

Das ist ein Callback für einen Verbindungs-Event.

- Wenn ein vorheriges Connection-Objekt wiederverwendet werden kann, wird dieser Callback aufgerufen.
- Danach wird erneut `advertising_start()` aufgerufen.
- Dadurch beginnt das Gerät wieder automatisch mit Advertising, nachdem eine vorherige Verbindung beendet wurde.

---

## 4) `BT_CONN_CB_DEFINE(conn_callbacks)`

```c
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .recycled = recycled_cb,
};
```

Damit wird der Callback registriert.

- `.recycled = recycled_cb` bedeutet:
  - Wenn ein „recycled“-Event eintritt, dann wird `recycled_cb()` ausgeführt.

Das ist die Stelle, an der der Code sagt: „Bei diesem Event starte nach einer Trennung wieder Advertising.“

---

## Gesamtverständnis

Der gesamte Block bedeutet:

1. `adv_work` ist ein Work-Objekt, das später `adv_work_handler` ausführt.
2. `advertising_start()` schiebt den Start des Advertisings in den Worker.
3. Wenn eine alte Verbindung vollständig beendet wurde, ruft `recycled_cb()` auf.
4. `recycled_cb()` startet erneut Advertising, damit das Gerät wieder sichtbar für andere Geräte ist.

Damit bleibt das Gerät nach einer Trennung automatisch erneut in BLE-Advertising aktiv.
