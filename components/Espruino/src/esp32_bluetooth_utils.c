/*
 * This file is part of Espruino, a JavaScript interpreter for Microcontrollers
 *
 * Copyright (C) 2017 Gordon Williams <gw@pur3.co.uk>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 * ESP32 specific Bluetooth utils
 * ----------------------------------------------------------------------------
 */
#include "DeviceConfig.h"

#ifdef BLUETOOTH
#include "esp32_bluetooth_utils.h"
#include "esp32_gap_func.h"
#include <esp_gap_ble_api.h>
#include "esp32_gatts_func.h"
#include "esp32_gattc_func.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "freertos/FreeRTOS.h"
#include "jsvariterator.h"

esp_ble_debug_t bleEventDebug = 0;

esp_err_t initController()
{
  esp_err_t ret;
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ret = esp_bt_controller_init(&bt_cfg);
  if (ret)
  {
    jsWarn("init controller failed:%x\n", ret);
    return ret;
  }
  ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
  if (ret)
  {
    jsWarn("enable controller failed:%x\n", ret);
    return ret;
  }
  return ret;
}
esp_err_t initBluedroid()
{
  esp_err_t ret;
  ret = esp_bluedroid_init();
  if (ret)
  {
    jsWarn("init bluedroid failed:%x\n", ret);
    return ret;
  }
  ret = esp_bluedroid_enable();
  if (ret)
  {
    jsWarn("enable bluedroid failed:%x\n", ret);
    return ret;
  }
  return ret;
}
esp_err_t deinitController()
{
  esp_err_t ret;
  ret = esp_bt_controller_disable();
  if (ret)
  {
    jsWarn("disable bluetooth failed:%x\n");
    return ret;
  }
  ret = esp_bt_controller_deinit();
  if (ret)
  {
    jsWarn("deinit bluetooth failed:%x\n");
    return ret;
  }
  return ret;
}
esp_err_t deinitBluedroid()
{
  esp_err_t ret;
  ret = esp_bluedroid_disable();
  if (ret)
  {
    jsWarn("disable bluedroid failed:%x\n", ret);
    return ret;
  }
  ret = esp_bluedroid_deinit();
  if (ret)
  {
    jsWarn("deinit bluedroid failed:%x\n", ret);
    return ret;
  }
  return ret;
}
esp_err_t registerCallbacks()
{
  esp_err_t ret;
  ret = esp_ble_gap_register_callback(gap_event_handler);
  if (ret)
  {
    jsWarn("gap register error:%x\n", ret);
    return ret;
  }
  ret = esp_ble_gatts_register_callback(gatts_event_handler);
  if (ret)
  {
    jsWarn("gatts register error:%x\n", ret);
    return ret;
  }
  ret = esp_ble_gattc_register_callback(gattc_event_handler);
  if (ret)
  {
    jsWarn("gattc regigister error:%x\n", ret);
    return ret;
  }
  return ret;
}
esp_err_t setMtu()
{
  esp_err_t ret;
  ret = esp_ble_gatt_set_local_mtu(500);
  if (ret)
    jsWarn("set local MTU failed:%x\n", ret);
  return ret;
}

void ESP32_setBLE_Debug(int level)
{
  bleEventDebug = level;
}
static char *gattsEvent2String(esp_gatts_cb_event_t event)
{
  switch (event)
  {
  case ESP_GATTS_REG_EVT:
    return "REG";
  case ESP_GATTS_READ_EVT:
    return "READ";
  case ESP_GATTS_WRITE_EVT:
    return "WRITE";
  case ESP_GATTS_EXEC_WRITE_EVT:
    return "EXEC_WRITE";
  case ESP_GATTS_MTU_EVT:
    return "MTU";
  case ESP_GATTS_CONF_EVT:
    return "CONF";
  case ESP_GATTS_UNREG_EVT:
    return "UNREG";
  case ESP_GATTS_CREATE_EVT:
    return "CREATE";
  case ESP_GATTS_ADD_INCL_SRVC_EVT:
    return "ADD_INCL_SRVC";
  case ESP_GATTS_ADD_CHAR_EVT:
    return "ADD_CHAR";
  case ESP_GATTS_ADD_CHAR_DESCR_EVT:
    return "ADD_CHAR_DESCR";
  case ESP_GATTS_DELETE_EVT:
    return "DELETE";
  case ESP_GATTS_START_EVT:
    return "START";
  case ESP_GATTS_STOP_EVT:
    return "STOP";
  case ESP_GATTS_CONNECT_EVT:
    return "CONNECT";
  case ESP_GATTS_DISCONNECT_EVT:
    return "DISCONNECT";
  case ESP_GATTS_OPEN_EVT:
    return "OPEN";
  case ESP_GATTS_CANCEL_OPEN_EVT:
    return "CANCEL_OPEN";
  case ESP_GATTS_CLOSE_EVT:
    return "CLOSE";
  case ESP_GATTS_LISTEN_EVT:
    return "LISTEN";
  case ESP_GATTS_CONGEST_EVT:
    return "CONGEST";
  case ESP_GATTS_RESPONSE_EVT:
    return "RESPONSE";
  case ESP_GATTS_CREAT_ATTR_TAB_EVT:
    return "CREAT_ATTR_TAB";
  case ESP_GATTS_SET_ATTR_VAL_EVT:
    return "CREAT_ATTR_TAB";
  case ESP_GATTS_SEND_SERVICE_CHANGE_EVT:
    return "SEND_SERVICE";
  }
  return "unknown GattsEvent";
}
static char *gattcEvent2String(esp_gattc_cb_event_t event)
{
  switch (event)
  {
  case ESP_GATTC_REG_EVT:
    return "REG";
  case ESP_GATTC_UNREG_EVT:
    return "UNREG";
  case ESP_GATTC_OPEN_EVT:
    return "OPEN";
  case ESP_GATTC_READ_CHAR_EVT:
    return "READ_CHAR";
  case ESP_GATTC_WRITE_CHAR_EVT:
    return "WRITE_CHAR";
  case ESP_GATTC_CLOSE_EVT:
    return "CLOSE";
  case ESP_GATTC_SEARCH_CMPL_EVT:
    return "SEARCH_CMPL";
  case ESP_GATTC_SEARCH_RES_EVT:
    return "SEARCH_RES";
  case ESP_GATTC_READ_DESCR_EVT:
    return "READ_DESCR";
  case ESP_GATTC_WRITE_DESCR_EVT:
    return "WRITE_DESCR";
  case ESP_GATTC_NOTIFY_EVT:
    return "NOTIFY";
  case ESP_GATTC_PREP_WRITE_EVT:
    return "PREP_WRITE";
  case ESP_GATTC_EXEC_EVT:
    return "EXEC";
  case ESP_GATTC_ACL_EVT:
    return "ACL";
  case ESP_GATTC_CANCEL_OPEN_EVT:
    return "CANCEL_OPEN";
  case ESP_GATTC_SRVC_CHG_EVT:
    return "SRVC_CHG";
  case ESP_GATTC_ENC_CMPL_CB_EVT:
    return "ENC_CMPL_CB";
  case ESP_GATTC_CFG_MTU_EVT:
    return "CFG_MTU";
  case ESP_GATTC_ADV_DATA_EVT:
    return "ADV_DATA";
  case ESP_GATTC_MULT_ADV_ENB_EVT:
    return "MULT_ADV_ENB";
  case ESP_GATTC_MULT_ADV_UPD_EVT:
    return "ADV_ADV_UPD";
  case ESP_GATTC_MULT_ADV_DATA_EVT:
    return "MULT_ADV_DATA";
  case ESP_GATTC_MULT_ADV_DIS_EVT:
    return "MULT_ADV_DIS";
  case ESP_GATTC_CONGEST_EVT:
    return "CONGEST";
  case ESP_GATTC_BTH_SCAN_ENB_EVT:
    return "BTH_SCAN_ENB";
  case ESP_GATTC_BTH_SCAN_CFG_EVT:
    return "BTH_SCAN_CFG";
  case ESP_GATTC_BTH_SCAN_RD_EVT:
    return "BTH_SCAN_RD";
  case ESP_GATTC_BTH_SCAN_THR_EVT:
    return "BTH_SCAN_THR";
  case ESP_GATTC_BTH_SCAN_PARAM_EVT:
    return "BTH_SCAN_PARAM";
  case ESP_GATTC_BTH_SCAN_DIS_EVT:
    return "BTH_SCAN_DIS";
  case ESP_GATTC_SCAN_FLT_CFG_EVT:
    return "SCAN_FLT_CFG";
  case ESP_GATTC_SCAN_FLT_PARAM_EVT:
    return "SCAN_FLT_PARAM";
  case ESP_GATTC_SCAN_FLT_STATUS_EVT:
    return "SCAN_FLT_STATUS";
  case ESP_GATTC_ADV_VSC_EVT:
    return "ADV_VSC";
  case ESP_GATTC_REG_FOR_NOTIFY_EVT:
    return "REG_FOR_NOTIFY";
  case ESP_GATTC_UNREG_FOR_NOTIFY_EVT:
    return "UNREG_FOR_NOTIFY";
  case ESP_GATTC_CONNECT_EVT:
    return "CONNECT";
  case ESP_GATTC_DISCONNECT_EVT:
    return "DISCONNECT";
  case ESP_GATTC_READ_MULTIPLE_EVT:
    return "READ_MUTIPLE";
  case ESP_GATTC_QUEUE_FULL_EVT:
    return "QUEUE_FULL";
  case ESP_GATTC_SET_ASSOC_EVT:
    return "SET_ASSOC";
  case ESP_GATTC_GET_ADDR_LIST_EVT:
    return "GET_ADDR_LIST";
  case ESP_GATTC_DIS_SRVC_CMPL_EVT:
    return "DIS_SRVC_CMPL";
  }
  return "unknown GattcEvent";
}
static char *gapEvent2String(esp_gap_ble_cb_event_t event)
{
  switch (event)
  {
#if (BLE_42_FEATURE_SUPPORT == TRUE)
  case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
    return "ADV_DATA_SET_COMPLETE";
  case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
    return "SCAN_RSP_DATA_SET_COMPLETE";
  case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
    return "SCAN_PARAM_SET_COMPLETE";
  case ESP_GAP_BLE_SCAN_RESULT_EVT:
    return "SCAN_RESULT";
  case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
    return "ADV_DATA_RAW_SET_COMPLETE";
  case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
    return "SCAN_RSP_DATA_RAW_SET_COMPLETE";
  case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
    return "ADV_START_COMPLETE";
  case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
    return "SCAN_START_COMPLETE";
#endif
  case ESP_GAP_BLE_AUTH_CMPL_EVT:
    return "AUTH_CMPL";
  case ESP_GAP_BLE_KEY_EVT:
    return "KEY";
  case ESP_GAP_BLE_SEC_REQ_EVT:
    return "SEC_REQ";
  case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:
    return "PASSKEY_NOTIF";
  case ESP_GAP_BLE_PASSKEY_REQ_EVT:
    return "PASSKEY_REQ";
  case ESP_GAP_BLE_OOB_REQ_EVT:
    return "OOB_REQ";
  case ESP_GAP_BLE_LOCAL_IR_EVT:
    return "LOCAL_IR";
  case ESP_GAP_BLE_LOCAL_ER_EVT:
    return "LOCAL_ER";
  case ESP_GAP_BLE_NC_REQ_EVT:
    return "NC_REQ";
#if (BLE_42_FEATURE_SUPPORT == TRUE)
  case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
    return "ADV_STOP_COMPLETE";
  case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
    return "SCAN_STOP_COMPLETE";
#endif
  case ESP_GAP_BLE_SET_STATIC_RAND_ADDR_EVT:
    return "SET_STATIC_RAND_ADDR";
  case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
    return "UPDATE_CONN_PARAMS";
  case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT:
    return "SET_PKT_LENGTH_COMPLETE";
  case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT:
    return "SET_LOCAL_PRIVACY_COMPLETE";
  case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT:
    return "REMOVE_BOND_DEV_COMPLETE";
  case ESP_GAP_BLE_CLEAR_BOND_DEV_COMPLETE_EVT:
    return "CLEAR_BOND_DEV_COMPLETE";
  case ESP_GAP_BLE_GET_BOND_DEV_COMPLETE_EVT:
    return "GET_BOND_DEV_COMPLETE_EVT";
  case ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT:
    return "READ_RSSI_COMPLETE";
  case ESP_GAP_BLE_UPDATE_WHITELIST_COMPLETE_EVT:
    return "UPDATE_WHITELIST_COMPLETE";
#if (BLE_42_FEATURE_SUPPORT == TRUE)
  case ESP_GAP_BLE_UPDATE_DUPLICATE_EXCEPTIONAL_LIST_COMPLETE_EVT:
    return "UPDATE_DUPLICATE_EXCEPTIONAL_LIST_COMPLETE";
#endif
  case ESP_GAP_BLE_SET_CHANNELS_EVT:
    return "BLE_SET_CHANNELS";
#if (BLE_50_FEATURE_SUPPORT == TRUE)
  case ESP_GAP_BLE_READ_PHY_COMPLETE_EVT:
    return "READ_PHY_COMPLETE";
  case ESP_GAP_BLE_SET_PREFERED_DEFAULT_PHY_COMPLETE_EVT:
    return "BLE_SET_PREFERRED_DEFAULT_PHY_COMPLETE";
  case ESP_GAP_BLE_SET_PREFERED_PHY_COMPLETE_EVT:
    return "BLE_SET_PREFERRED_PHY_COMPLETE";
  case ESP_GAP_BLE_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT:
    return "BLE_EXT_ADV_SET_RAND_ADDR_COMPLETE";
  case ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT:
    return "BLE_EXT_ADV_SET_PARAMS_COMPLETE";
  case ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT:
    return "BLE_EXT_ADV_DATA_SET_COMPLETE";
  case ESP_GAP_BLE_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT:
    return "BLE_EXT_SCAN_RSP_DATA_SET_COMPLETE";
  case ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT:
    return "BLE_EXT_ADV_START_COMPLETE";
  case ESP_GAP_BLE_EXT_ADV_STOP_COMPLETE_EVT:
    return "BLE_EXT_ADV_STOP_COMPLETE";
  case ESP_GAP_BLE_EXT_ADV_SET_REMOVE_COMPLETE_EVT:
    return "BLE_EXT_ADV_SET_REMOVE_COMPLETE";
  case ESP_GAP_BLE_EXT_ADV_SET_CLEAR_COMPLETE_EVT:
    return "BLE_EXT_ADV_SET_CLEAR_COMPLETE";
  case ESP_GAP_BLE_PERIODIC_ADV_SET_PARAMS_COMPLETE_EVT:
    return "BLE_PERIODIC_ADV_SET_PARAMS_COMPLETE";
  case ESP_GAP_BLE_PERIODIC_ADV_DATA_SET_COMPLETE_EVT:
    return "BLE_PERIODIC_ADV_DATA_SET_COMPLETE";
  case ESP_GAP_BLE_PERIODIC_ADV_START_COMPLETE_EVT:
    return "BLE_PERIODIC_ADV_START_COMPLETE";
  case ESP_GAP_BLE_PERIODIC_ADV_STOP_COMPLETE_EVT:
    return "BLE_PERIODIC_ADV_STOP_COMPLETE";
  case ESP_GAP_BLE_PERIODIC_ADV_CREATE_SYNC_COMPLETE_EVT:
    return "BLE_PERIODIC_ADV_CREATE_SYNC_COMPLETE";
  case ESP_GAP_BLE_PERIODIC_ADV_SYNC_CANCEL_COMPLETE_EVT:
    return "BLE_PERIODIC_ADV_SYNC_CANCEL_COMPLETE";
  case ESP_GAP_BLE_PERIODIC_ADV_SYNC_TERMINATE_COMPLETE_EVT:
    return "BLE_PERIODIC_ADV_SYNC_TERMINATE_COMPLETE";
  case ESP_GAP_BLE_PERIODIC_ADV_ADD_DEV_COMPLETE_EVT:
    return "BLE_PERIODIC_ADV_ADD_DEV_COMPLETE";
  case ESP_GAP_BLE_PERIODIC_ADV_REMOVE_DEV_COMPLETE_EVT:
    return "BLE_PERIODIC_ADV_REMOVE_DEV_COMPLETE";
  case ESP_GAP_BLE_PERIODIC_ADV_CLEAR_DEV_COMPLETE_EVT:
    return "BLE_PERIODIC_ADV_CLEAR_DEV_COMPLETE";
  case ESP_GAP_BLE_SET_EXT_SCAN_PARAMS_COMPLETE_EVT:
    return "BLE_SET_EXT_SCAN_PARAMS_COMPLETE";
  case ESP_GAP_BLE_EXT_SCAN_START_COMPLETE_EVT:
    return "BLE_EXT_SCAN_START_COMPLETE";
  case ESP_GAP_BLE_EXT_SCAN_STOP_COMPLETE_EVT:
    return "BLE_EXT_SCAN_STOP_COMPLETE";
  case ESP_GAP_BLE_PREFER_EXT_CONN_PARAMS_SET_COMPLETE_EVT:
    return "BLE_PREFER_EXT_CONN_PARAMS_SET_COMPLETE";
  case ESP_GAP_BLE_PHY_UPDATE_COMPLETE_EVT:
    return "BLE_PHY_UPDATE_COMPLETE";
  case ESP_GAP_BLE_EXT_ADV_REPORT_EVT:
    return "BLE_EXT_ADV_REPORT";
  case ESP_GAP_BLE_SCAN_TIMEOUT_EVT:
    return "BLE_SCAN_TIMEOUT";
  case ESP_GAP_BLE_ADV_TERMINATED_EVT:
    return "BLE_ADV_TERMINATED";
  case ESP_GAP_BLE_SCAN_REQ_RECEIVED_EVT:
    return "BLE_SCAN_REQ_RECEIVED";
  case ESP_GAP_BLE_CHANNEL_SELETE_ALGORITHM_EVT:
    return "BLE_CHANNEL_SELECT_ALGORITHM";
  case ESP_GAP_BLE_PERIODIC_ADV_REPORT_EVT:
    return "BLE_PERIODIC_ADV_REPORT";
  case ESP_GAP_BLE_PERIODIC_ADV_SYNC_LOST_EVT:
    return "BLE_PERIODIC_ADV_SYNC_LOST";
  case ESP_GAP_BLE_PERIODIC_ADV_SYNC_ESTAB_EVT:
    return "BLE_PERIODIC_ADV_SYNC_ESTAB";
#endif // #if (BLE_50_FEATURE_SUPPORT == TRUE)
  case ESP_GAP_BLE_EVT_MAX:
    return "BLE_EVT_MAX";
  }
  return "unknown GapEvent";
}

void jsWarnGattsEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if)
{
  if (bleEventDebug & ESP_BLE_DEBUG_GATTS)
    jsWarn("Event:ESP_GATTS_%s_EVT gatts_if:%d\n", gattsEvent2String(event), gatts_if);
}
void jsWarnGattcEvent(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if)
{
  if (bleEventDebug & ESP_BLE_DEBUG_GATTC)
    jsWarn("Event:ESP_GATTC_%s_EVT gattc_if:%d\n", gattcEvent2String(event), gattc_if);
}
void jsWarnGapEvent(esp_gap_ble_cb_event_t event)
{
  if (bleEventDebug & ESP_BLE_DEBUG_GAP)
    jsWarn("Event:ESP_GAP_BLE_%s_EVT\n", gapEvent2String(event));
}

void jsWarnUUID(esp_bt_uuid_t char_uuid)
{
  if (char_uuid.len == ESP_UUID_LEN_16)
  {
    jsWarn("- - - Char UUID16: %x", char_uuid.uuid.uuid16);
  }
  else if (char_uuid.len == ESP_UUID_LEN_32)
  {
    jsWarn("- - - Char UUID32: %x", char_uuid.uuid.uuid32);
  }
  else if (char_uuid.len == ESP_UUID_LEN_128)
  {
    jsWarn("- - - Char UUID128: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", char_uuid.uuid.uuid128[0],
           char_uuid.uuid.uuid128[1], char_uuid.uuid.uuid128[2], char_uuid.uuid.uuid128[3],
           char_uuid.uuid.uuid128[4], char_uuid.uuid.uuid128[5], char_uuid.uuid.uuid128[6],
           char_uuid.uuid.uuid128[7], char_uuid.uuid.uuid128[8], char_uuid.uuid.uuid128[9],
           char_uuid.uuid.uuid128[10], char_uuid.uuid.uuid128[11], char_uuid.uuid.uuid128[12],
           char_uuid.uuid.uuid128[13], char_uuid.uuid.uuid128[14], char_uuid.uuid.uuid128[15]);
  }
  else
  {
    jsWarn("- - - Char UNKNOWN LEN %d\n", char_uuid.len);
  }
}
void jsWarnBDA(uint8_t *bda)
{
  jsWarn("bda %02x:%02x:%02x:%02x:%02x:%02x\n", bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
}
void jsWarnHeap(char *whereAmI)
{
  jsWarn("%s Heap:%d, jsVars:%d\n", whereAmI, esp_get_free_heap_size(), jsvGetMemoryUsage());
}

void bleGetHiddenName(char *eventName, char *hiddenName, uint16_t pos)
{
  strcpy(eventName, hiddenName);
  itostr(pos, &eventName[strlen(eventName)], 16);
}
bool bleRemoveChild(JsVar *parent, JsVar *blevar)
{
  bool ret = false;
  JsvObjectIterator it;
  jsvObjectIteratorNew(&it, parent);
  while (jsvObjectIteratorHasValue(&it))
  {
    JsVar *child = jsvObjectIteratorGetKey(&it);
    JsVar *name = jsvNewFromStringVar(child, 0, 10);
    if (jsvIsEqual(name, blevar))
    {
      jsvRemoveChild(parent, child);
      ret = true;
    }
    jsvUnLock(child);
    jsvUnLock(name);
    jsvObjectIteratorNext(&it);
  }
  jsvObjectIteratorFree(&it);
  return ret;
}
void bleRemoveChilds(JsVar *parent)
{
  JsVar *blevar = jsvNewFromString(BLE_CHAR_VALUE);
  while (bleRemoveChild(parent, blevar))
  {
  }
  jsvUnLock(blevar);
}

void bleuuid_TO_espbtuuid(ble_uuid_t ble_uuid, esp_bt_uuid_t *esp_uuid)
{
  switch (ble_uuid.type)
  {
  case BLE_UUID_TYPE_UNKNOWN:
    jsError("empty UUID type\n");
    break;
  case BLE_UUID_TYPE_BLE:
    esp_uuid->len = ESP_UUID_LEN_16;
    esp_uuid->uuid.uuid16 = ble_uuid.uuid;
    break;
  case BLE_UUID_TYPE_128:
    esp_uuid->len = ESP_UUID_LEN_128;
    for (int i = 0; i < 16; i++)
    {
      esp_uuid->uuid.uuid128[i] = ble_uuid.uuid128[i];
    }
    break;
  default:
    jsError("bleuuid_TO_espbtuuid unknown UUID TYPE\n");
  }
}

void espbtuuid_TO_bleuuid(esp_bt_uuid_t esp_uuid, ble_uuid_t *ble_uuid)
{
  switch (esp_uuid.len)
  {
  case ESP_UUID_LEN_16:
    ble_uuid->type = BLE_UUID_TYPE_BLE;
    ble_uuid->uuid = esp_uuid.uuid.uuid16;
    break;
  case ESP_UUID_LEN_128:
    ble_uuid->type = BLE_UUID_TYPE_128;
    for (int i = 0; i < 16; i++)
      ble_uuid->uuid128[i] = esp_uuid.uuid.uuid128[i];
    break;
  default:
    jsError("espbtuuid_TO_bleuuid unknown UUID TYPE\n");
  }
}

void bleuuid_To_uuid128(ble_uuid_t ble_uuid, uint8_t *ble128)
{
  uint8_t tmp[] = {0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00};
  switch (ble_uuid.type)
  {
  case BLE_UUID_TYPE_UNKNOWN:
    jsError("empty UUID type\n");
    break;
  case BLE_UUID_TYPE_BLE:
    for (int i = 0; i < 16; i++)
    {
      ble128[i] = tmp[i];
    }
    ble128[12] = ble_uuid.uuid & 0xff;
    ble128[13] = ble_uuid.uuid >> 8;
    break;
  case BLE_UUID_TYPE_128:
    for (int i = 0; i < 16; i++)
    {
      ble128[i] = ble_uuid.uuid128[i];
    }
    break;
  default:
    jsError("unknown UUID type\n");
  }
}

bool espbtuuid_equal(esp_bt_uuid_t a, esp_bt_uuid_t b)
{
  return (a.len == ESP_UUID_LEN_16 && b.len == ESP_UUID_LEN_16 && a.uuid.uuid16 == b.uuid.uuid16) ||
         (a.len == ESP_UUID_LEN_32 && b.len == ESP_UUID_LEN_32 && a.uuid.uuid32 == b.uuid.uuid32) ||
         (a.len == ESP_UUID_LEN_128 && b.len == ESP_UUID_LEN_128 && memcmp(a.uuid.uuid128, b.uuid.uuid128, ESP_UUID_LEN_128) == 0);
}

void bleaddr_TO_espbtaddr(ble_gap_addr_t ble_addr, esp_bd_addr_t remote_bda, esp_ble_addr_type_t *remote_bda_type)
{
  // reverse bytes in address and copy
  for (int i = 0; i < 6; i++)
    remote_bda[5 - i] = ble_addr.addr[i];
  // BLE_GAP_ADDR_TYPES and esp_ble_addr_type_t match (no need for conversion)
  *remote_bda_type = ble_addr.addr_type;
}

void espbtaddr_TO_bleaddr(esp_bd_addr_t remote_bda, esp_ble_addr_type_t remote_bda_type, ble_gap_addr_t *ble_addr)
{
  // reverse bytes in address and copy
  for (int i = 0; i < 6; i++)
    ble_addr->addr[i] = remote_bda[5 - i];
  // BLE_GAP_ADDR_TYPES and esp_ble_addr_type_t match (no need for conversion)
  ble_addr->addr_type = remote_bda_type;
}
#endif