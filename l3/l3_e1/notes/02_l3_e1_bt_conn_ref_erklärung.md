# Warum `my_conn = bt_conn_ref(conn);` und nicht einfach `my_conn = conn;`?

In Zephyr ist `struct bt_conn *` kein bloßer „normaler Pointer“, den man einfach irgendwo speichern kann. Ein Bluetooth-Verbindungsobjekt wird vom Stack verwaltet und hat einen Referenzzähler.

## Der Unterschied

### 1) Einfacher Pointer

```c
my_conn = conn;
```

Das ist nur eine Adresse. Du speicherst den Pointer, aber du erhöhst nicht die Lebensdauer der Verbindung.

Das Problem:
- Die Verbindung kann vom Bluetooth-Stack geschlossen oder freigegeben werden.
- Danach zeigt `my_conn` auf eine Verbindung, die nicht mehr gültig ist.
- Das nennt man dann einen dangling pointer.

### 2) Referenzzähler

```c
my_conn = bt_conn_ref(conn);
```

`bt_conn_ref(conn)` sagt dem Stack: „Ich verwende diese Verbindung noch weiter, bitte halte sie lebendig.“

Dadurch:
- wird der interne Zähler erhöht,
- die Verbindung bleibt gültig, solange du sie noch brauchst,
- und du musst sie später wieder mit `bt_conn_unref(my_conn)` freigeben.

## Warum ist das in `on_connected()` wichtig?

`on_connected()` bekommt einen Pointer `conn` für die gerade aufgebaute Verbindung. Wenn du diese Verbindung außerhalb des Callback speichern willst, z. B. in einer globalen Variable, dann musst du sie „refen“. Sonst kann sie nach einem Disconnect oder beim Recycling des Objekts plötzlich ungültig werden.

Das ist genau der typische Zephyr-Pattern:

```c
void on_connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("Connection error %d", err);
        return;
    }

    my_conn = bt_conn_ref(conn);
}

void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected. Reason %d", reason);
    bt_conn_unref(my_conn);
    my_conn = NULL;
}
```

## Warum nicht einfach nur `conn` speichern?

Weil `conn` nur ein Handle für die aktuelle Verbindung ist. Der Bluetooth-Stack kann dieses Objekt jederzeit intern verwalten und löschen. Ein globaler Zeiger auf das Objekt ohne Referenz ist also unsicher.

## Kurz gesagt

- `my_conn = conn;` → nur Pointer speichern
- `my_conn = bt_conn_ref(conn);` → Verbindung „besitzen“ und ihre Lebensdauer sichern

Das ist das Standardmuster in Zephyr, wenn eine Verbindung über Callback-Grenzen hinweg gespeichert werden soll.

## Merksatz

`bt_conn_ref()` ist wie: „Ich habe diese Verbindung noch gebraucht.“

`bt_conn_unref()` ist dann: „Ich bin fertig, du kannst sie wieder freigeben.“
