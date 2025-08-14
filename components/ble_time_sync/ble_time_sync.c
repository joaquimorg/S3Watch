#include "ble_time_sync.h"
#include "rtc.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "host/ble_uuid.h"
#include "host/ble_store.h"
#include "console/console.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#define GATTS_TAG "BLE_GENERIC"

// Custom Service UUIDs
#define GATT_SVR_SVC_TIME_SYNC_UUID 0x00FF
#define GATT_SVR_CHR_TIME_SYNC_UUID 0xFF01

#define GATT_SVR_SVC_NOTIFICATION_UUID 0x00FE
#define GATT_SVR_CHR_NOTIFICATION_UUID 0xFE01

static uint8_t own_addr_type;

static int gatt_svr_chr_access_time_sync(uint16_t conn_handle, uint16_t attr_handle,
                                         struct ble_gatt_access_ctxt *ctxt, void *arg);
static int gatt_svr_chr_access_notification(uint16_t conn_handle, uint16_t attr_handle,
                                            struct ble_gatt_access_ctxt *ctxt, void *arg);
static int ble_gap_event(struct ble_gap_event *event, void *arg);
static int gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);

// GATT server definitions
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        // Time Synchronization Service
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GATT_SVR_SVC_TIME_SYNC_UUID),
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = BLE_UUID16_DECLARE(GATT_SVR_CHR_TIME_SYNC_UUID),
                .access_cb = gatt_svr_chr_access_time_sync,
                .flags = BLE_GATT_CHR_F_WRITE,
            }, {
                0,
            } /* No more characteristics in this service */
        },
    },
    {
        // Notification Service
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GATT_SVR_SVC_NOTIFICATION_UUID),
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = BLE_UUID16_DECLARE(GATT_SVR_CHR_NOTIFICATION_UUID),
                .access_cb = gatt_svr_chr_access_notification,
                .flags = BLE_GATT_CHR_F_WRITE,
            }, {
                0,
            } /* No more characteristics in this service */
        },
    },
    {
        0,
    } /* No more services */
};

static int gatt_svr_chr_access_time_sync(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid = ble_uuid_u16(ctxt->chr->uuid);

    if (uuid == GATT_SVR_CHR_TIME_SYNC_UUID) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            if (ctxt->om->om_len == sizeof(time_t)) {
                time_t received_time = *(time_t *)ctxt->om->om_data;
                struct tm *timeinfo = localtime(&received_time);
                if (timeinfo) {
                    rtc_set_time(timeinfo);
                    ESP_LOGI(GATTS_TAG, "Time set to: %s", asctime(timeinfo));
                } else {
                    ESP_LOGE(GATTS_TAG, "Failed to convert received time");
                }
            } else {
                ESP_LOGE(GATTS_TAG, "Invalid time data length: %d", ctxt->om->om_len);
            }
        }
    }
    return 0;
}

static int gatt_svr_chr_access_notification(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid = ble_uuid_u16(ctxt->chr->uuid);

    if (uuid == GATT_SVR_CHR_NOTIFICATION_UUID) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            // For now, just log the received notification
            ESP_LOGI(GATTS_TAG, "Received notification: %.*s", ctxt->om->om_len, (char *)ctxt->om->om_data);
        }
    }
    return 0;
}

static void ble_app_advertise(void)
{
    struct ble_hs_adv_fields fields;
    const char *name = "S3Watch_BLE";
    int rc;

    memset(&fields, 0, sizeof(fields));

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error setting advertisement data; rc=%d\n", rc);
        return;
    }

    /* Begin advertising */
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error enabling advertisement; rc=%d\n", rc);
        return;
    }
}

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            MODLOG_DFLT(INFO, "connection %s; status=%d\n",
                        event->connect.status == 0 ? "established" : "failed",
                        event->connect.status);
            if (event->connect.status != 0) {
                ble_app_advertise();
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            MODLOG_DFLT(INFO, "disconnect; reason=%d\n", event->disconnect.reason);
            ble_app_advertise();
            break;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            MODLOG_DFLT(INFO, "advertise complete; reason=%d\n",
                        event->adv_complete.reason);
            ble_app_advertise();
            break;

        case BLE_GAP_EVENT_SUBSCRIBE:
            MODLOG_DFLT(INFO, "subscribe event; conn_handle=%d attr_handle=%d "
                        "reason=%d prev_notify=%d cur_notify=%d
",
                        event->subscribe.conn_handle, event->subscribe.attr_handle,
                        event->subscribe.reason, event->subscribe.prev_notify,
                        event->subscribe.cur_notify);
            break;

        case BLE_GAP_EVENT_MTU_CHANGED:
            MODLOG_DFLT(INFO, "mtu change event; conn_handle=%d mtu=%d\n",
                        event->mtu_changed.conn_handle, event->mtu_changed.mtu);
            break;

        default:
            break;
    }
    return 0;
}

static int gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGI(GATTS_TAG, "service registered: %s",
                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf));
        break;
    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGI(GATTS_TAG, "characteristic registered: %s",
                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf));
        break;
    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGI(GATTS_TAG, "descriptor registered: %s",
                 ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf));
        break;
    default:
        break;
    }

    return 0;
}

static void ble_on_reset(int reason)
{
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

static void ble_on_sync(void)
{
    ble_hs_id_infer_auto(&own_addr_type);
    ble_app_advertise();
}

void ble_host_task(void *param)
{
    ESP_LOGI(GATTS_TAG, "BLE Host Task Started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

esp_err_t ble_time_sync_init(void)
{
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }

    nimble_port_init();

    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status;

    ble_store_config_init();

    // Set up the GATT server
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_gatts_count_cfg(gatt_svr_svcs);
    ble_gatts_add_svcs(gatt_svr_svcs);

    // Start the NimBLE host task
    nimble_port_freertos_init(ble_host_task);

    return ESP_OK;
}
