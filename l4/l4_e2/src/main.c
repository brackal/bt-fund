/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <dk_buttons_and_leds.h>
#include "my_lbs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>

static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
    (BT_LE_ADV_OPT_CONN |
     BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use identity address */
    800,                          /* Min Advertising Interval 500ms (800*0.625ms) */
    801,                          /* Max Advertising Interval 500.625ms (801*0.625ms) */
    NULL);                        /* Set to NULL for undirected advertising */

LOG_MODULE_REGISTER(Lesson4_Exercise2, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define CON_STATUS_LED DK_LED2
#define USER_LED DK_LED3
#define USER_BUTTON DK_BTN1_MSK

#define STACKSIZE 1024
#define PRIORITY 7

#define RUN_LED_BLINK_INTERVAL 1000
/* STEP 17 - Define the interval at which you want to send data at */
#define NOTIFY_INTERVAL 500

static bool app_button_state;
static struct k_work adv_work;
/* STEP 15 - Define the data you want to stream over Bluetooth LE */
static uint32_t app_sensor_value = 100;

static bool app_button_state;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),

};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL),
};

int bt_read_remote_version(struct bt_conn *conn);
struct bt_conn_remote_info info;
static struct bt_conn *default_conn;
static struct k_work read_version_work;

static void log_local_version(void);

static void read_version_work_handler(struct k_work *work)
{
    int err = bt_read_remote_version(default_conn);
    if (err)
    {
        LOG_ERR("Failed to read remote version (err %d)", err);
    }

    printk("Reading remote version successfully started\n");
}

static void adv_work_handler(struct k_work *work)
{
    int err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

    if (err)
    {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    printk("Advertising successfully started\n");
}

static void advertising_start(void)
{
    k_work_submit(&adv_work);
}

static void recycled_cb(void)
{
    printk("Connection object available from previous conn. Disconnect is complete!\n");
    advertising_start();
}

/* STEP 16 - Define a function to simulate the data */
static void simulate_data(void)
{
    app_sensor_value++;
    if (app_sensor_value == 200)
    {
        app_sensor_value = 100;
    }
}

static void app_led_cb(bool led_state)
{
    dk_set_led(USER_LED, led_state);
}

static bool app_button_cb(void)
{
    return app_button_state;
}

static void remote_info_available_cb(struct bt_conn *conn, struct bt_conn_remote_info *remote_info)
{
    printk("Remote version: 0x%02x, manufacturer: 0x%04x, subversion: 0x%04x",
           remote_info->version, remote_info->manufacturer, remote_info->subversion);
}

static struct my_lbs_callbacks app_callbacks = {
    .set_led_state_cb = app_led_cb,
    .get_button_state_cb = app_button_cb,
};

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
    if (has_changed & USER_BUTTON)
    {
        uint32_t user_button_state = button_state & USER_BUTTON;

        /* STEP 6 - Send indication on a button press */
        my_lbs_send_button_state_indicate(user_button_state);

        app_button_state = user_button_state ? true : false;
    }
}

static void on_connected(struct bt_conn *conn, uint8_t err)
{
    if (err)
    {
        printk("Connection failed (err %u)\n", err);
        return;
    }

    printk("Connected\n");

    default_conn = bt_conn_ref(conn);

    k_work_init(&read_version_work, read_version_work_handler);
    k_work_submit(&read_version_work);

    dk_set_led_on(CON_STATUS_LED);
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("Disconnected (reason %u)\n", reason);

    dk_set_led_off(CON_STATUS_LED);
}

struct bt_conn_cb connection_callbacks = {
    .connected = on_connected,
    .disconnected = on_disconnected,
    .recycled = recycled_cb,
    .remote_info_available = remote_info_available_cb,
};

static int init_button(void)
{
    int err;

    err = dk_buttons_init(button_changed);
    if (err)
    {
        printk("Cannot init buttons (err: %d)\n", err);
    }

    return err;
}

int main(void)
{
    int blink_status = 0;
    int err;

    LOG_INF("Starting Lesson 4 - Exercise 2 \n");

    err = dk_leds_init();
    if (err)
    {
        LOG_ERR("LEDs init failed (err %d)\n", err);
        return -1;
    }

    err = init_button();
    if (err)
    {
        printk("Button init failed (err %d)\n", err);
        return -1;
    }

    err = bt_enable(NULL);
    if (err)
    {
        LOG_ERR("Bluetooth init failed (err %d)\n", err);
        return -1;
    }
    bt_conn_cb_register(&connection_callbacks);

    err = my_lbs_init(&app_callbacks);
    if (err)
    {
        printk("Failed to init LBS (err:%d)\n", err);
        return -1;
    }
    LOG_INF("Bluetooth initialized\n");

    log_local_version();

    k_work_init(&adv_work, adv_work_handler);
    advertising_start();
    for (;;)
    {
        dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
        k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
    }
}

/* STEP 18.1 - Define the thread function  */
void send_data_thread(void)
{
    while (1)
    {
        /* Simulate data */
        simulate_data();
        /* Send notification, the function sends notifications only if a client is subscribed */
        my_lbs_send_sensor_notify(app_sensor_value);

        k_sleep(K_MSEC(NOTIFY_INTERVAL));
    }
}

/* STEP 18.2 - Define and initialize a thread to send data periodically */
K_THREAD_DEFINE(send_data_thread_id, STACKSIZE, send_data_thread, NULL, NULL, NULL, PRIORITY, 0, 0);

int bt_read_remote_version(struct bt_conn *conn)
{
    struct bt_hci_cp_read_remote_version_info *cp;
    struct net_buf *buf;
    int err;

    uint16_t handle;

    err = bt_hci_get_conn_handle(conn, &handle);
    if (err)
    {
        printk("Failed to get connection handle (err %d)\n", err);
        return err;
    }

    buf = bt_hci_cmd_alloc(K_FOREVER);
    if (!buf)
    {
        printk("Failed to allocate HCI command buffer\n");
        return -ENOMEM;
    }

    cp = net_buf_add(buf, sizeof(*cp));
    cp->handle = sys_cpu_to_le16(handle);

    err = bt_hci_cmd_send(BT_HCI_OP_READ_REMOTE_VERSION_INFO, buf);
    if (err)
    {
        printk("Failed to send read remote version command (err %d)\n", err);
        return err;
    }

    printk("Read remote version command sent for conn handle 0x%04x\n", handle);

    return 0;
}

static void log_local_version(void)
{
    struct net_buf *rsp;
    int err;

    err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_VERSION_INFO, NULL, &rsp);
    if (err)
    {
        LOG_ERR("Failed to read local version info (err %d)", err);
        return;
    }

    struct bt_hci_rp_read_local_version_info *rp = (void *)rsp->data;

    LOG_INF("Local version: 0x%02x, manufacturer: 0x%04x, subversion: 0x%04x",
            rp->hci_version, sys_le16_to_cpu(rp->manufacturer),
            sys_le16_to_cpu(rp->hci_revision));

    net_buf_unref(rsp);
}