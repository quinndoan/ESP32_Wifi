#include "library.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "sdkconfig.h"


#define GATTS_TAG "GATTS_DEMO"

#include "bluetooth.h"


/// Declare the static function
static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

#define GATTS_SERVICE_UUID_TEST_A 0x00FF
#define GATTS_CHAR_UUID_TEST_A 0xFF01
#define GATTS_DESCR_UUID_TEST_A 0x3333
#define GATTS_NUM_HANDLE_TEST_A 4

#define TEST_DEVICE_NAME "Quinn"
#define TEST_MANUFACTURER_DATA_LEN 17

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40

#define PREPARE_BUF_MAX_SIZE 1024

#define MAX_BLE_DATA_LEN 256  // Kích thước tối đa bộ đệm dữ liệu BLE
static char ssid_buffer[32] = {0};    // Bộ đệm lưu SSID
static char password_buffer[64] = {0}; 
bool is_ssid_received = false; // Biến để kiểm tra trạng thái nhận SSID
bool is_password_received = false;

static uint8_t char1_str[] = {0x11, 0x22, 0x33};
static esp_gatt_char_prop_t a_property = 0;
static esp_gatt_char_prop_t b_property = 0;

static esp_attr_value_t gatts_demo_char1_val =
    {
        .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
        .attr_len = sizeof(char1_str),
        .attr_value = char1_str,
};

static uint8_t adv_config_done = 0;
#define adv_config_flag (1 << 0)
#define scan_rsp_config_flag (1 << 1)

static uint8_t adv_service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    // first uuid, 16bit, [12],[13] is the value
    0xfb,
    0x34,
    0x9b,
    0x5f,
    0x80,
    0x00,
    0x00,
    0x80,
    0x00,
    0x10,
    0x00,
    0x00,
    0xEE,
    0x00,
    0x00,
    0x00,
    // second uuid, 32bit, [12], [13], [14], [15] is the value
    0xfb,
    0x34,
    0x9b,
    0x5f,
    0x80,
    0x00,
    0x00,
    0x80,
    0x00,
    0x10,
    0x00,
    0x00,
    0xFF,
    0x00,
    0x00,
    0x00,
};

// The length of adv data must be less than 31 bytes
// static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
// adv data
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006, // slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, // slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = 0,       // TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    //.min_interval = 0x0006,
    //.max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0,       // TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

#define PROFILE_NUM 1
#define PROFILE_A_APP_ID 0

struct gatts_profile_inst
{
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gatts_cb = gatts_profile_a_event_handler,
        .gatts_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    }};

typedef struct
{
    uint8_t *prepare_buf;
    int prepare_len;
} prepare_type_env_t;

static prepare_type_env_t a_prepare_write_env;

typedef struct
{
    esp_gatt_if_t gatts_if;
    esp_ble_gatts_cb_param_t *param;
} ble_server_t;

static ble_server_t ble_server;

static ble_data_recv_handle_t ble_data_recv_handle = NULL;

void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        // advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(GATTS_TAG, "Advertising start failed\n");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(GATTS_TAG, "Advertising stop failed\n");
        }
        else
        {
            ESP_LOGI(GATTS_TAG, "Stop adv successfully\n");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(GATTS_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                 param->update_conn_params.status,
                 param->update_conn_params.min_int,
                 param->update_conn_params.max_int,
                 param->update_conn_params.conn_int,
                 param->update_conn_params.latency,
                 param->update_conn_params.timeout);
        break;
    default:
        break;
    }
}

void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    esp_gatt_status_t status = ESP_GATT_OK;
    if (param->write.need_rsp)
    {
        if (param->write.is_prep)
        {
            if (prepare_write_env->prepare_buf == NULL)
            {
                prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
                prepare_write_env->prepare_len = 0;
                if (prepare_write_env->prepare_buf == NULL)
                {
                    ESP_LOGE(GATTS_TAG, "Gatt_server prep no mem\n");
                    status = ESP_GATT_NO_RESOURCES;
                }
            }
            else
            {
                if (param->write.offset > PREPARE_BUF_MAX_SIZE)
                {
                    status = ESP_GATT_INVALID_OFFSET;
                }
                else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE)
                {
                    status = ESP_GATT_INVALID_ATTR_LEN;
                }
            }

            esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK)
            {
                ESP_LOGE(GATTS_TAG, "Send response error\n");
            }
            free(gatt_rsp);
            if (status != ESP_GATT_OK)
            {
                return;
            }
            memcpy(prepare_write_env->prepare_buf + param->write.offset,
                   param->write.value,
                   param->write.len);
            prepare_write_env->prepare_len += param->write.len;
        }
        else
        {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
        }
    }
}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC)
    {
        // esp_log_buffer_hex(GATTS_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
          ble_data_recv_handle(prepare_write_env->prepare_buf, prepare_write_env->prepare_len); // function poiter
    }
    else
    {
        ESP_LOGI(GATTS_TAG, "ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf)
    {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    ble_server.gatts_if = gatts_if;
    ble_server.param = param;
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gl_profile_tab[PROFILE_A_APP_ID].service_id.is_primary = true;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.inst_id = 0x00;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_A;

        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(TEST_DEVICE_NAME);
        if (set_dev_name_ret)
        {
            ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }
        // config adv data
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret)
        {
            ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x", ret);
        }
        adv_config_done |= adv_config_flag;
        // config scan response data
        ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        if (ret)
        {
            ESP_LOGE(GATTS_TAG, "config scan response data failed, error code = %x", ret);
        }
        adv_config_done |= scan_rsp_config_flag;

        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_A_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_A);
        break;
    case ESP_GATTS_READ_EVT:
    {
       ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, conn_id %u, trans_id %u, handle %u", 
         (unsigned int) param->read.conn_id, 
         (unsigned int) param->read.trans_id, 
         (unsigned int) param->read.handle);

        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 4;
        rsp.attr_value.value[0] = 0xde;
        rsp.attr_value.value[1] = 0xed;
        rsp.attr_value.value[2] = 0xbe;
        rsp.attr_value.value[3] = 0xef;
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT:
    {
        ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, conn_id %u, trans_id %u, handle %u", 
         (unsigned int)param->write.conn_id, 
         (unsigned int)param->write.trans_id, 
         (unsigned int)param->write.handle);

        if (!param->write.is_prep)  // cờ check dữ liệu ngắn hay dài
        {
            ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);

            ble_data_recv_handle(param->write.value, param->write.len); // function poiter

            if (gl_profile_tab[PROFILE_A_APP_ID].descr_handle == param->write.handle && param->write.len == 2)
            {
                uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
                if (descr_value == 0x0001)
                {
                    if (a_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY)
                    {
                        ESP_LOGI(GATTS_TAG, "notify enable");
                        uint8_t notify_data[15];
                    }
                }
                else if (descr_value == 0x0002)
                {
                    if (a_property & ESP_GATT_CHAR_PROP_BIT_INDICATE)
                    {
                        ESP_LOGI(GATTS_TAG, "indicate enable");
                    }
                }
                else if (descr_value == 0x0000)
                {
                    ESP_LOGI(GATTS_TAG, "notify/indicate disable ");
                }
                else
                {
                    ESP_LOGE(GATTS_TAG, "unknown descr value");
                    esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
                }
            }
        }
        example_write_event_env(gatts_if, &a_prepare_write_env, param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:  // khi dữ liệu dài
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        example_exec_write_event_env(&a_prepare_write_env, param);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gl_profile_tab[PROFILE_A_APP_ID].service_handle = param->create.service_handle;
        gl_profile_tab[PROFILE_A_APP_ID].char_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_A;

        esp_ble_gatts_start_service(gl_profile_tab[PROFILE_A_APP_ID].service_handle);
        a_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_A_APP_ID].service_handle, &gl_profile_tab[PROFILE_A_APP_ID].char_uuid,
                                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                        a_property,
                                                        &gatts_demo_char1_val, NULL);
        if (add_char_ret)
        {
            ESP_LOGE(GATTS_TAG, "add char failed, error code =%x", add_char_ret);
        }
        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
    {
        uint16_t length = 0;
        const uint8_t *prf_char;

        ESP_LOGI(GATTS_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        gl_profile_tab[PROFILE_A_APP_ID].char_handle = param->add_char.attr_handle;
        gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
        esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle, &length, &prf_char);
        if (get_attr_ret == ESP_FAIL)
        {
            ESP_LOGE(GATTS_TAG, "ILLEGAL HANDLE");
        }

        ESP_LOGI(GATTS_TAG, "the gatts demo char length = %x\n", length);
        for (int i = 0; i < length; i++)
        {
            ESP_LOGI(GATTS_TAG, "prf_char[%x] =%x\n", i, prf_char[i]);
        }
        esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_A_APP_ID].service_handle, &gl_profile_tab[PROFILE_A_APP_ID].descr_uuid,
                                                               ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
        if (add_descr_ret)
        {
            ESP_LOGE(GATTS_TAG, "add char descr failed, error code =%x", add_descr_ret);
        }
        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        gl_profile_tab[PROFILE_A_APP_ID].descr_handle = param->add_char_descr.attr_handle;
        ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
    {
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20; // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10; // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;  // timeout = 400*10ms = 4000ms
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gl_profile_tab[PROFILE_A_APP_ID].conn_id = param->connect.conn_id;
        // start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK)
        {
            esp_log_buffer_hex(GATTS_TAG, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT)
    {
        if (param->reg.status == ESP_GATT_OK)
        {
            gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        }
        else
        {
            ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d\n",
                     param->reg.app_id,
                     param->reg.status);
            return;
        }
    }

    /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do
    {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++)
        {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                gatts_if == gl_profile_tab[idx].gatts_if)
            {
                if (gl_profile_tab[idx].gatts_cb)
                {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

void app_ble_start(void)
{

    int ret;
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_init();
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "gatts register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "gap register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gatts_app_register(PROFILE_A_APP_ID);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
        return;
    }
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret)
    {
        ESP_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }

    return;
}

void app_ble_stop(void)
{
    uint8_t idx = 0;
    for (idx = 0; idx < PROFILE_NUM; idx++)
    {
        esp_ble_gatts_app_unregister(gl_profile_tab[idx].gatts_if);
    }
    if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED)
    {
        esp_bluedroid_disable();
    }
    if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_INITIALIZED)
    {
        esp_bluedroid_deinit();
    }
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED)
    {
        esp_bt_controller_disable();
    }
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED)
    {
        esp_bluedroid_deinit();
    }
}

void app_ble_send_data(uint8_t* data, uint16_t len)
{
    esp_ble_gatts_send_indicate(ble_server.gatts_if, ble_server.param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                len, data, false);
}

void app_ble_set_data_recv_callback(void *cb)
{
    if (cb)
    {
        ble_data_recv_handle = cb;
    }
}

void ble_data_received_callback(uint8_t *data, uint16_t length) {
    // Log dữ liệu nhận được dưới dạng hex để kiểm tra.
    esp_log_buffer_hex(TAG, data, length);

    // Chuyển dữ liệu nhận được thành chuỗi (string) để dễ dàng xử lý.
    char received_data[MAX_BLE_DATA_LEN] = {0};
    snprintf(received_data, sizeof(received_data), "%.*s", length, (char *)data);
    ESP_LOGI(TAG, "Received data from BLE: %s", received_data);

    // Kiểm tra nếu dữ liệu nhận được là SSID
    if (!is_ssid_received) {
        strncpy(ssid_buffer, received_data, sizeof(ssid_buffer) - 1);
        is_ssid_received = true; // Đã nhận SSID thành công
        ESP_LOGI(TAG, "SSID received: %s", ssid_buffer);

        // Gửi phản hồi BLE xác nhận đã nhận SSID và yêu cầu gửi Password
        char msg[] = "SSID received. Please send Password.";
        app_ble_send_data((uint8_t *)msg, strlen(msg));
    }
    // Nếu đã nhận SSID và bây giờ nhận Password
    else if (is_ssid_received && !is_password_received) {
        strncpy(password_buffer, received_data, sizeof(password_buffer) - 1);
        is_password_received = true; // Đã nhận Password thành công
        ESP_LOGI(TAG, "Password received: %s", password_buffer);

        // Sau khi nhận đầy đủ SSID và Password, lưu vào NVS và chuyển sang chế độ STA
        nvs_handle_t nvs_handle;
        esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
        if (err == ESP_OK) {
            // Lưu SSID
            err = nvs_set_str(nvs_handle, "ssid", ssid_buffer);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "SSID saved successfully.");
            } else {
                ESP_LOGE(TAG, "Failed to save SSID. Error: %s", esp_err_to_name(err));
            }

            // Lưu Password
            err = nvs_set_str(nvs_handle, "password", password_buffer);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Password saved successfully.");
            } else {
                ESP_LOGE(TAG, "Failed to save Password. Error: %s", esp_err_to_name(err));
            }

            // Cập nhật chế độ Wi-Fi thành STA Mode (0)
            err = nvs_set_i32(nvs_handle, "wifi_mode", 0);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Wi-Fi mode saved successfully.");
            } else {
                ESP_LOGE(TAG, "Failed to save Wi-Fi mode. Error: %s", esp_err_to_name(err));
            }

            // Commit các thay đổi vào NVS
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "NVS commit successful.");
            } else {
                ESP_LOGE(TAG, "Failed to commit NVS changes. Error: %s", esp_err_to_name(err));
            }

            // Đóng NVS sau khi hoàn thành
            nvs_close(nvs_handle);
        } else {
            ESP_LOGE(TAG, "Failed to open NVS handle. Error: %s", esp_err_to_name(err));
        }

        // Gửi phản hồi xác nhận đã lưu thành công qua BLE
        char success_msg[] = "SSID and Password saved. Switching to STA mode...";
        app_ble_send_data((uint8_t *)success_msg, strlen(success_msg));

        // Dừng BLE trước khi khởi động lại
        app_ble_stop();

        // Đợi một chút rồi khởi động lại thiết bị để chuyển sang chế độ STA
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();
    }
}
