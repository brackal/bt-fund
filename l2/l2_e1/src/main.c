/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
/* STEP 3 - Include the header file of the Bluetooth LE stack */
/* that means the Bluetooth LE host */
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/addr.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/bluetooth/hci.h>

/* Nordic Semiconductor Company ID laut Bluetooth SIG */
#define COMPANY_ID_CODE 0x0059

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define RUN_LED_BLINK_INTERVAL 1000

/* Eigene Payload-Struktur: z. B. Temperatur (int16) + Batteriestand (uint8) */
typedef struct
{
    uint16_t company_id; /* Little Endian */
    int16_t temperature; /* z. B. in 0.01 °C-Schritten */
    uint8_t battery_pct; /* 0-100 % */
    uint8_t reserved;    /* Padding/Reserve */
} mfg_data_t;

static mfg_data_t mfg_data = {
    .company_id = COMPANY_ID_CODE,
    .temperature = 2350, /* = 23.50 °C */
    .battery_pct = 87,
    .reserved = 0x00,
};

LOG_MODULE_REGISTER(Lesson2_Exercise1, LOG_LEVEL_INF);

void print_all_identities(void);
void print_ad_data(void);
static void print_controller_info(void);

/* STEP 4.1.1 - Declare the advertising packet */
static const struct bt_data ad[] = {
    /* STEP 4.1.2 - Set the advertising flags */
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),

    BT_DATA(BT_DATA_MANUFACTURER_DATA, (uint8_t *)&mfg_data, sizeof(mfg_data)),

    /* STEP 4.1.3 - Set the advertising packet data  */
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN)

};

/* STEP 4.2.2 - Declare the URL data to include in the scan response */
static unsigned char url_data[] = {0x17, '/', '/', 'a', 'c', 'a', 'd', 'e', 'm', 'y', '.',
                                   'n', 'o', 'r', 'd', 'i', 'c', 's', 'e', 'm', 'i', '.',
                                   'c', 'o', 'm'};

/* STEP 4.2.1 - Declare the scan response packet */
static const struct bt_data sd[] = {
    /* 4.2.3 Include the URL data in the scan response packet*/
    BT_DATA(BT_DATA_URI, url_data, sizeof(url_data))

};

static const struct bt_le_adv_param *adv_param =
#if 1
    /* verstecke nicht die Adresse */
    BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY,
                    BT_GAP_ADV_FAST_INT_MIN_2,
                    BT_GAP_ADV_FAST_INT_MAX_2,
                    NULL);
#else
    /* verstecke die Adresse  */
    BT_LE_ADV_PARAM(0,
                    BT_GAP_ADV_FAST_INT_MIN_2,
                    BT_GAP_ADV_FAST_INT_MAX_2,
                    NULL);
#endif

int main(void)
{
    int blink_status = 0;
    int err;

    LOG_INF("Starting Lesson 2 - Exercise 1 \n");

    err = dk_leds_init();
    if (err)
    {
        LOG_ERR("LEDs init failed (err %d)\n", err);
        return -1;
    }

    /* STEP 5 - Enable the Bluetooth LE stack */
    err = bt_enable(NULL);
    if (err)
    {
        LOG_ERR("Bluetooth init failed (err %d)\n", err);
        return -1;
    }
    LOG_INF("Bluetooth initialized\n");

/* On "Add Build Configuration" select bt-ll-sw-split in Snippets */
#if defined(CONFIG_BT_LL_SW_SPLIT)
    LOG_INF("Controller: Zephyr Bluetooth LE Controller (open source)");
#elif defined(CONFIG_BT_LL_SOFTDEVICE)
    LOG_INF("Controller: Nordic SoftDevice Controller");
#else
    LOG_INF("Controller: unknown/other");
#endif

    print_controller_info();

    /* STEP 6 - Start advertising */
    err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err)
    {
        LOG_ERR("Advertising failed to start (err %d)\n", err);
        return -1;
    }
    LOG_INF("Advertising successfully started\n");

    print_all_identities();
    // print_ad_data();

    for (;;)
    {
        dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
        k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
    }
}

void print_all_identities(void)
{
    bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
    size_t count = CONFIG_BT_ID_MAX;

    bt_id_get(addrs, &count);

    LOG_INF("Anzahl registrierter Identities: %zu\n", count);

    for (size_t i = 0; i < count; i++)
    {
        char addr_str[BT_ADDR_LE_STR_LEN];
        bt_addr_le_to_str(&addrs[i], addr_str, sizeof(addr_str));
        // printk("Identity[%zu]: %s\n", i, addr_str);
        LOG_INF("Identity[%zu]: %s\n", i, addr_str);
    }
}

void print_ad_data(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(ad); i++)
    {
        LOG_INF("ad[%zu]: type=%u, len=%u", i, ad[i].type, ad[i].data_len);

        for (size_t j = 0; j < ad[i].data_len; j++)
        {
            LOG_INF("ad[%zu].data[%zu] = 0x%02x", i, j, ad[i].data[j]);
        }
    }
}

static void print_controller_info(void)
{
    struct net_buf *rsp;
    int err;

    err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_VERSION_INFO, NULL, &rsp);
    if (err)
    {
        LOG_ERR("Reading local version info failed (err %d)", err);
        return;
    }

    struct bt_hci_rp_read_local_version_info *rp = (void *)rsp->data;

    LOG_INF("HCI version: %u, Manufacturer (Company ID): 0x%04x",
            rp->hci_version, rp->manufacturer);

    net_buf_unref(rsp);
}

/*
Kurz zusammengefasst: BT_LE_ADV_NCONN setzt standardmäßig keine BT_LE_ADV_OPT_USE_IDENTITY-Option.
Ohne dieses Flag generiert Zephyr für non-connectable Advertising aus Privacy-Gründen automatisch
eine NRPA (Non-Resolvable Private Address) – unabhängig davon, ob CONFIG_BT_PRIVACY aktiv ist.
Diese NRPA wird bei jedem Boot neu zufällig erzeugt (weil sie nirgends persistiert wird),
während die Identity-Adresse aus dem FICR-Register hardwaregebunden und immer gleich bleibt.

Mit BT_LE_ADV_OPT_USE_IDENTITY sagst du dem Stack: "Nutze direkt meine Identity-Adresse fürs Advertising" –
dadurch stimmen Log und App jetzt überein.
*/