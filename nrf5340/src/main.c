#include "main.h"

LOG_MODULE_REGISTER(Diploma, LOG_LEVEL_INF);

static void btAdvFunction(struct k_work *work){
	int err = bt_le_adv_start(
		bluetoothParams, 
		advertisingData, 
		ARRAY_SIZE(advertisingData), 
		scanData, 
		ARRAY_SIZE(scanData)
	);
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return;
	}
	LOG_INF("Advertising successfully started\n");
}

void advertisingTask(){
	k_work_submit(&advTask);
}

static void connected(struct bt_conn *conn, uint8_t err){
	if (err) {
		printk("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
		return;
	}
	printk("Connected\n");
}

static void disconnected(struct bt_conn *conn, uint8_t reason){
	printk("Disconnected, reason 0x%02x %s\n", reason, bt_hci_err_to_str(reason));
}

static void recycled_cb(void){
	printk("Connection object available from previous conn. Disconnect is complete!\n");
	advertisingTask();
}

// Defining which function to use upon an event
BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected        = connected,
	.disconnected     = disconnected,
	.recycled         = recycled_cb,
};

int main(void){
	int blink_status = 0;
	int err;

	bt_addr_le_t addr;
    err = bt_addr_le_from_str("FF:EE:DD:CC:BB:AA", "random", &addr);
    if (err) {
        printk("Invalid BT address (err %d)\n", err);
    }

    err = bt_id_create(&addr, NULL);
    if (err < 0) {
        printk("Creating new ID failed (err %d)\n", err);
    }
	
	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return -1;
	}

	LOG_INF("Bluetooth initialized\n");

	err = dk_leds_init();
    if (err < 0) {
        printk("Unnable to initialize LED (err %d)\n", err);
    }
	
	
	k_work_init(&advTask, btAdvFunction);
	advertisingTask();
	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
