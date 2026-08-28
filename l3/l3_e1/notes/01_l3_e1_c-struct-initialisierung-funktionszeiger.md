# C: Struct-Initialisierung mit Designated Initializers und Funktionszeigern

## Ausgangscode

```c
struct bt_conn_cb connection_callbacks = {
    .connected              = on_connected,
    .disconnected           = on_disconnected,
    .recycled               = on_recycled,
};
```

## 1. Was bedeutet der Punkt bei den Members? Welche Sprache ist das?

Das ist **C** (nicht C++). Der Code nutzt **Designated Initializers** (bezeichnete Initialisierer), ein Feature aus **C99**.

Der Punkt `.connected`, `.disconnected`, `.recycled` bezieht sich auf die **Feldnamen (Member) der Struct-Definition**. Man sagt damit explizit: "Initialisiere dieses bestimmte Feld der Struct mit diesem Wert" – statt sich auf die Reihenfolge der Felder zu verlassen.

Zugrunde liegende Struct-Definition (aus Zephyr, dem Bluetooth-Stack), vereinfacht:

```c
struct bt_conn_cb {
    void (*connected)(struct bt_conn *conn, uint8_t err);
    void (*disconnected)(struct bt_conn *conn, uint8_t reason);
    void (*recycled)(void);
    // ... weitere Felder
};
```

**Warum das nützlich ist:**

1. **Lesbarkeit** – man sieht sofort, welcher Wert zu welchem Feld gehört.
2. **Reihenfolge egal** – Felder müssen nicht in Deklarationsreihenfolge angegeben werden.
3. **Nicht angegebene Felder** werden automatisch mit `0`/`NULL` initialisiert.

## 2. Dieselbe Initialisierung ohne Punkte (klassisches C)

Ohne Designated Initializers (positionsbasiert, C89/ANSI-C-Stil) müssen die Werte in **exakt der Reihenfolge** stehen, in der die Felder in der Struct deklariert sind:

```c
struct beispiel_cb {
    void (*connected)(int err);
    void (*disconnected)(int reason);
    void (*recycled)(void);
};

struct beispiel_cb connection_callbacks = {
    on_connected,
    on_disconnected,
    on_recycled
};
```

Das ist funktional identisch zu:

```c
struct beispiel_cb connection_callbacks = {
    .connected    = on_connected,
    .disconnected = on_disconnected,
    .recycled     = on_recycled,
};
```

**Hinweis:** Bei der echten `bt_conn_cb`-Struct aus Zephyr gibt es sehr viele Felder (über 20 Callback-Pointer). Bei positionsbasierter Initialisierung müssten alle Felder in korrekter Reihenfolge angegeben werden – nicht benötigte als `NULL`. Genau deshalb sind Designated Initializers bei solchen Structs Standard: sicherer, robuster gegen API-Änderungen, weniger fehleranfällig.

## 3. Ist `void (*connected)(int err);` eine Callback-Funktion?

Ja – es handelt sich um einen **Funktionszeiger (function pointer)** als Struct-Feld, der als **Callback** verwendet wird.

**Syntax aufgeschlüsselt:**

```
void (*connected)(int err);
 │     │           │
 │     │           └── Parameter: nimmt ein int entgegen
 │     └── Name des Feldes/Zeigers: "connected"
 └── Rückgabetyp: void
```

Gelesen: *"`connected` ist ein Zeiger auf eine Funktion, die ein `int` als Parameter nimmt und `void` zurückgibt."*

**Warum die Klammern um `*connected` wichtig sind:**

```c
void (*connected)(int err);   // Zeiger AUF eine Funktion        ✓ (gewollt)
void *connected(int err);     // Funktion, die void* zurückgibt  ✗ (etwas anderes!)
```

**Zuweisung eines Callbacks:**

```c
connection_callbacks.connected = on_connected;
```

Die zugehörige Funktion muss exakt zur Signatur passen:

```c
void on_connected(int err) {
    // Code, der ausgeführt wird, sobald "connected" passiert
}
```

Der Bluetooth-Stack ruft dann intern z. B. auf:

```c
connection_callbacks.connected(0);   // ruft praktisch on_connected(0) auf
```

Das ist das Callback-Prinzip: Man übergibt der Library eine Funktion, die sie später selbst aufruft, wenn ein bestimmtes Ereignis eintritt.

## 4. Ist `connected` in `void (*connected)(int err);` ein Datentyp oder ein frei erfundener Name?

`connected` ist ein **frei gewählter Name**, kein Datentyp und kein Schlüsselwort. Der Autor hätte das Feld auch `foo`, `xyz` oder `on_conn` nennen können.

**Aufteilung Datentyp vs. Name:**

| Teil | Bedeutung |
|---|---|
| `void ( * ) (int err)` | **Datentyp**: "Zeiger auf Funktion, die `int` nimmt und `void` zurückgibt" |
| `connected` | **Name** des Struct-Feldes – frei gewählt |

**Vergleich mit einer normalen Variable:**

```c
int zahl;
```
- Datentyp: `int`
- Name: `zahl` (frei erfunden)

```c
void (*connected)(int err);
```
- Datentyp: `void (*)(int)` (Zeiger-auf-Funktion-Typ)
- Name: `connected` (frei erfunden)

Das `*` gehört zum **Datentyp** (es sagt "das ist ein Zeiger"), der Name daneben ist der Bezeichner für den Zugriff (`struct.connected = ...`). Der Name `connected` wurde nur aus Lesbarkeitsgründen gewählt, weil er beschreibt, wann der Callback ausgelöst wird – das ist reine Konvention, keine technische Notwendigkeit.

## 5. Was bedeutet `struct beispiel_cb connection_callbacks = { on_connected, on_disconnected, on_recycled };`? Was ist z. B. `on_connected`?

`on_connected` ist eine **Funktion** – genauer: hier wird der **Funktionsname als Funktionszeiger** verwendet.

**Aufschlüsselung der Zeile:**

| Teil | Bedeutung |
|---|---|
| `struct beispiel_cb` | Datentyp (Struct-Definition) |
| `connection_callbacks` | Name der Variable |
| `= { ... }` | Initialisierung mit Werten |
| `on_connected` | 1. Wert → Feld `connected` |
| `on_disconnected` | 2. Wert → Feld `disconnected` |
| `on_recycled` | 3. Wert → Feld `recycled` |

`on_connected` muss selbst irgendwo im Code definiert sein, z. B.:

```c
void on_connected(int err) {
    printf("Verbindung hergestellt, err = %d\n", err);
}
```

**Warum kein `&` oder `()` nötig ist:**

```c
on_connected()     // ✗ falsch – ruft die Funktion sofort AUF
&on_connected       // funktioniert, ist aber unnötig
on_connected         // ✓ richtig – ist die Adresse der Funktion
```

In C wird ein **Funktionsname ohne Klammern automatisch zu einem Zeiger auf diese Funktion** (*function-to-pointer decay*). Ein `&` davor ist erlaubt, aber redundant.

**Was am Ende im Speicher passiert:**

```
connection_callbacks.connected    = Adresse von on_connected
connection_callbacks.disconnected = Adresse von on_disconnected
connection_callbacks.recycled     = Adresse von on_recycled
```

Die Struct-Variable `connection_callbacks` enthält also drei Zeiger, die jeweils auf den Beginn der drei Funktionen im Programmspeicher zeigen. Ruft später z. B. die Bluetooth-Library `connection_callbacks.connected(0)` auf, wird tatsächlich `on_connected(0)` ausgeführt.
