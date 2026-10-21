#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <dk_buttons_and_leds.h>

#include <bluetooth/services/lbs.h>

#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/addr.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define RUN_LED_BLINK_INTERVAL 1000

static const struct bt_le_adv_param *bluetoothParams = BT_LE_ADV_PARAM(
    BT_LE_ADV_OPT_CONN, 
    BT_GAP_ADV_FAST_INT_MIN_2, 
    BT_GAP_ADV_FAST_INT_MAX_2, 
    NULL
);

// Advertising data
static const struct bt_data advertisingData[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

// Scan response data
static const struct bt_data scanData[] = {
};

// Function to start the bluetooth advertising
static void btAdvFunction(struct k_work *work);

static struct k_work advTask;

// Function that initializes the task via k_work_submit
void advertisingTask();