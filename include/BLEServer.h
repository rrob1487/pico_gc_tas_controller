/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SERVER_COMMON_H_
#define SERVER_COMMON_H_

#define STATUS_IDLE         0x00
#define STATUS_STARTED      0x01
#define STATUS_SUCCESS      0x02
#define STATUS_ERROR        0x03
#define STATUS_RUNNING      0x04

#define MAX_CHUNK_SIZE 23 // MAX BLE MTU

#define HANDLE_FILE_DATA   ATT_CHARACTERISTIC_69420690_1337_4200_BEEF_DEADBEEF4200_01_VALUE_HANDLE
#define HANDLE_STATUS      ATT_CHARACTERISTIC_69420691_1337_4200_BEEF_DEADBEEF4200_01_VALUE_HANDLE

extern int le_notification_enabled;
extern hci_con_handle_t con_handle;
extern uint8_t const profile_data[];
extern uint8_t upload_status;

#pragma pack(push, 1)
struct UploadChunkHeader {
    uint8_t type;
    uint16_t chunk_index; // 2-byte LE
    uint8_t payload[];    // flexible array member
};
#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

void notify_upload_status(uint8_t status);
void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size);
int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);

#ifdef __cplusplus
}
#endif
#endif