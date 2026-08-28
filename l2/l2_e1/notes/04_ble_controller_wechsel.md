# Wechsel von SoftDevice Controller zu Zephyr Bluetooth LE Controller (nRF52840DK, NCS v3.4.0)

## Hintergrund: NCS vs. Upstream Zephyr

- **Upstream Zephyr**: das reine Open-Source-RTOS-Projekt ([zephyrproject-rtos/zephyr](https://github.com/zephyrproject-rtos/zephyr)), herstellerübergreifend.
- **NCS (nRF Connect SDK)**: Nordics eigenes SDK, baut auf Zephyr auf und bringt zusätzlich Nordic-spezifische Module mit, u. a. `nrfxlib` mit dem **SoftDevice Controller (SDC)**.
- Erkennungsmerkmal: Ein NCS-Workspace enthält neben `zephyr/` auch die Ordner `nrf/` und `nrfxlib/`. Bei reinem Zephyr fehlen diese.
- Die NCS-Version findet man z. B. in `<workspace>/nrf/VERSION`.

## West Workspace

- **West** ist Zephyrs Meta-Tool zur Verwaltung mehrerer zusammengehöriger Git-Repos (Zephyr, nrf, nrfxlib, mcuboot, HALs, etc.).
- Ein **West Workspace** ist der Wurzelordner mit `.west/`-Konfiguration und Manifest-Repo (`west.yml`), der all diese Module referenziert und in kompatiblen Versionen hält.
- **Applications** (eigene Projekte, z. B. Tutorial-Ordner) liegen typischerweise *außerhalb* des Workspace und brauchen keinen eigenen `.west`-Ordner. Sie werden gegen einen bestehenden Workspace gebaut, z. B.:
  ```powershell
  cd C:\ncs\v3.4.0
  west build -b nrf52840dk/nrf52840 -d C:\GIT\bt-fund\l1\build C:\GIT\bt-fund\l1
  ```
  `-d` gibt den Build-Ordner an, das letzte Argument den Pfad zur Application.

## Controller-Wechsel: SDC → Zephyr Bluetooth LE Controller

### Methode je nach NCS-Version
- **Ältere NCS-Versionen (bis ca. 2.6):** in `prj.conf`
  ```
  CONFIG_BT_LL_SW_SPLIT=y
  ```
- **Neuere NCS-Versionen (ab ca. 2.7, inkl. 3.4.0):** über ein **Snippet**, nicht mehr direkt in `prj.conf`:
  ```powershell
  west build -b nrf52840dk/nrf52840 -d <build-dir> <app-dir> -p always -- -DSNIPPET=bt-ll-sw-split
  ```
  `-p always` erzwingt einen Pristine/Clean Build, was wegen der geänderten Kconfig-`choice` nötig ist.

### Dauerhaft im Projekt hinterlegen
Datei `snippets.yml` im Application-Ordner:
```yaml
append:
  SNIPPET:
    - bt-ll-sw-split
```
Oder in der **nRF Connect for VS Code**-Extension beim Anlegen/Bearbeiten einer Build-Konfiguration im Feld **„Snippets"** `bt-ll-sw-split` eintragen.

### Kompatibilität prüfen
- Kein Anpassungsbedarf an `prj.conf`, solange kein `CONFIG_BT_LL_SOFTDEVICE=y` explizit gesetzt ist und keine SDC-exklusiven Features (LLPM, LE Isochronous Channels) genutzt werden.
- Bei Nutzung von **LE Secure Connections**: zusätzlich
  ```
  CONFIG_BT_TINYCRYPT_ECC=y
  ```
  setzen, da der Zephyr-Controller kein ECDH in der Link-Layer-Bibliothek mitbringt.

### Verifikation nach dem Build
```powershell
findstr "BT_LL_SW_SPLIT BT_LL_SOFTDEVICE" <build-dir>\zephyr\.config
```
Erwartete Ausgabe:
```
CONFIG_BT_LL_SW_SPLIT=y
# CONFIG_BT_LL_SOFTDEVICE is not set
```

## API bleibt unverändert

Host und Controller sind über die standardisierte **HCI (Host Controller Interface)** getrennt. Der Zephyr Bluetooth **Host** (`bt_enable()`, `bt_gatt_*`, `bt_conn_*`, Advertising, etc.) bleibt in beiden Fällen identisch — nur die Controller-Implementierung unterhalb der HCI wechselt. Anwendungscode ändert sich nicht.

## Controller zur Laufzeit im Code anzeigen

### Variante 1 – Compile-Time (empfohlen)
```c
#if defined(CONFIG_BT_LL_SW_SPLIT)
    LOG_INF("Controller: Zephyr Bluetooth LE Controller (open source)");
#elif defined(CONFIG_BT_LL_SOFTDEVICE)
    LOG_INF("Controller: Nordic SoftDevice Controller");
#else
    LOG_INF("Controller: unknown/other");
#endif
```

### Variante 2 – Runtime per HCI-Kommando
```c
#include <zephyr/bluetooth/hci.h>

static void print_controller_info(void)
{
    struct net_buf *rsp;
    int err;

    err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_VERSION_INFO, NULL, &rsp);
    if (err) {
        LOG_ERR("Reading local version info failed (err %d)", err);
        return;
    }

    struct bt_hci_rp_read_local_version_info *rp = (void *)rsp->data;
    LOG_INF("HCI version: %u, Manufacturer (Company ID): 0x%04x",
             rp->hci_version, rp->manufacturer);

    net_buf_unref(rsp);
}
```

**Wichtiger Hinweis:** Die Manufacturer/Company ID (`0x0059` = Nordic Semiconductor) bleibt bei **beiden** Controllern gleich, da diese über `CONFIG_BT_CTLR_COMPANY_ID` fest für die Nordic-Zielhardware hinterlegt ist — unabhängig davon, ob SDC oder `BT_LL_SW_SPLIT` als Link-Layer-Implementierung läuft. Über die Company ID lässt sich der Controller **nicht** unterscheiden. Die zuverlässige Methode bleibt Variante 1 (Compile-Time-`#ifdef`).

## Troubleshooting beim Build/Flash (NCS 3.4.0, Windows)

### Build-Fehler: `ModuleNotFoundError: No module named 'elftools'`
Ursache: Fehlendes Python-Paket `pyelftools` in der verwendeten Python-Umgebung.

Fix:
```powershell
<python-pfad>\python.exe -m pip install -r C:\ncs\v3.4.0\zephyr\scripts\requirements.txt
```

### Flash-Fehler: `required program nrfutil not found`
Trat bei manuellem `west flash` über eine normale PowerShell auf. Über die **nRF Connect for VS Code**-Extension (Actions → Build/Flash) funktionierte es hingegen direkt — vermutlich weil die Extension eine eigene, mitgelieferte `nrfutil`-Installation nutzt bzw. den passenden Pfad automatisch einbindet.

## Praktisches Vorgehen: zwei Build-Konfigurationen parallel

Statt die bestehende Build-Konfiguration zu löschen, wurde eine **zweite Build-Konfiguration** (`build_1`) angelegt — identisch zur ursprünglichen, nur mit dem Snippet `bt-ll-sw-split` im Feld „Snippets" der VS-Code-Extension gesetzt. So lassen sich beide Controller-Varianten bequem parallel halten und über die Extension umschalten:

- **`build`** → SoftDevice Controller
- **`build_1`** → Zephyr Bluetooth LE Controller

### Erfolgreiche Verifikation im Log
```
[00:00:00.436,370] <inf> bt_hci_core: Identity: CA:CF:B8:93:B1:33 (random)
[00:00:00.436,401] <inf> bt_hci_core: HCI: version 5.4 (0x0d) revision 0x0000, manufacturer 0x0059
[00:00:00.436,431] <inf> bt_hci_core: LMP: version 5.4 (0x0d) subver 0xffff
[00:00:00.436,462] <inf> Lesson2_Exercise1: Bluetooth initialized
[00:00:00.436,462] <inf> Lesson2_Exercise1: Controller: Zephyr Bluetooth LE Controller (open source)
[00:00:00.436,584] <inf> Lesson2_Exercise1: HCI version: 13, Manufacturer (Company ID): 0x0059
```

(Hinweis: `0x0d` (hex) und `13` (dezimal) sind dieselbe HCI-Version — kein Widerspruch, nur unterschiedliche Zahlendarstellung an zwei verschiedenen Ausgabestellen im Code.)
