#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "btstack.h"
#include "btstack_run_loop.h"
#include "BLEServer.h"
#include "cheaterconch.h"
#include "ControllerInterfaceCWrapper.h"

#define MAX_FILE_SIZE (32 * 1024)  // Adjust to your expected max size

// File transfer state
static bool receiving = false;
volatile bool controller_should_run = false;
static size_t file_offset = 0;
static uint16_t expected_chunk_index = 0;
static uint8_t file_buffer[MAX_FILE_SIZE];

#define APP_AD_FLAGS 0x06
static uint8_t adv_data[] = {
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, APP_AD_FLAGS,
    0x12, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'C','h','e','a','t','e','r','C','o','n','t','r','o','l','l','e','r',
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x1a, 0x18,
};

static const uint8_t adv_data_len = sizeof(adv_data);
static btstack_timer_source_t controller_timer;

int le_notification_enabled;
hci_con_handle_t con_handle;
uint8_t upload_status = STATUS_IDLE;

void controller_interface_handler(btstack_timer_source_t *ts) {
    UNUSED(ts);

    if (controller_should_run) {
        controller_should_run = false;

        // Run the time-sensitive logic here
        printf("Running File\n");
        notify_upload_status(STATUS_RUNNING);
        run_ci_loop_c(file_buffer, file_offset);
        notify_upload_status(STATUS_IDLE);
        printf("Done Running File\n");
    }
}

void notify_upload_status(uint8_t status) {
    upload_status = status;
    if (le_notification_enabled) {
        att_server_notify(con_handle,
                          HANDLE_STATUS,
                          &upload_status, 1);
    }
}

void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET) return;

    uint8_t event_type = hci_event_packet_get_type(packet);
    switch(event_type){
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
            gap_local_bd_addr(local_addr);
            printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));

            uint16_t adv_int_min = 800;
            uint16_t adv_int_max = 800;
            uint8_t adv_type = 0;
            bd_addr_t null_addr;
            memset(null_addr, 0, 6);
            gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
            assert(adv_data_len <= 31);
            gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
            gap_advertisements_enable(1);
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            le_notification_enabled = 0;
            break;

        default:
            break;
    }
}

uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset,
                           uint8_t *buffer, uint16_t buffer_size) {
    UNUSED(connection_handle);
    UNUSED(buffer_size);

    // Read from UploadStatus characteristic
    if (att_handle == HANDLE_STATUS){
        if (offset == 0 && buffer) {
            buffer[0] = upload_status;
        }
        return 1;  // Status is always a single byte
    }

    return 0;
}

int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode,
                       uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    UNUSED(connection_handle);
    UNUSED(transaction_mode);
    UNUSED(offset);

    if (att_handle != HANDLE_FILE_DATA) return 0;

    if (buffer_size < sizeof(struct UploadChunkHeader)) {
        printf("[BLE] ERROR: Chunk too small!\n");
        notify_upload_status(STATUS_ERROR);
        return 0;
    }

    if (buffer_size > MAX_CHUNK_SIZE) {
        printf("[BLE] ERROR: Data too big! Got %u, max is %u\n", buffer_size, MAX_CHUNK_SIZE);
        notify_upload_status(STATUS_ERROR);
        return 0;
    }

    struct UploadChunkHeader *chunk = (struct UploadChunkHeader *)buffer;
    uint16_t chunk_index = chunk->chunk_index;
    uint8_t *payload = chunk->payload;
    size_t payload_len = buffer_size - sizeof(struct UploadChunkHeader);

    switch (chunk->type) {
        case 0: // START
            memset(file_buffer, 0, MAX_FILE_SIZE);
            receiving = true;
            file_offset = 0;
            expected_chunk_index = 0;
            notify_upload_status(STATUS_STARTED);
            printf("[BLE] Start of upload\n");
            break;

        case 1: // DATA
            if (!receiving) {
                printf("[BLE] ERROR: Received data chunk without start!\n");
                notify_upload_status(STATUS_ERROR);
                break;
            }
            if (chunk_index != expected_chunk_index) {
                printf("[BLE] ERROR: Unexpected chunk index. Expected %d, got %d\n", expected_chunk_index, chunk_index);
                notify_upload_status(STATUS_ERROR);
                receiving = false;
                file_offset = 0;
                expected_chunk_index = 0;
                break;
            }
            if (file_offset + payload_len > MAX_FILE_SIZE) {
                printf("[BLE] ERROR: Upload too large!\n");
                notify_upload_status(STATUS_ERROR);
                receiving = false;
                file_offset = 0;
                expected_chunk_index = 0;
                break;
            }
            memcpy(file_buffer + file_offset, payload, payload_len);
            file_offset += payload_len;
            expected_chunk_index++;
            break;

        case 2: // END
            if (!receiving) {
                printf("[BLE] ERROR: Received end without start!\n");
                notify_upload_status(STATUS_ERROR);
                receiving = false;
                file_offset = 0;
                expected_chunk_index = 0;
                break;
            }
            if (chunk_index != expected_chunk_index) {
                printf("[BLE] ERROR: Unexpected chunk index. Expected %d, got %d\n", expected_chunk_index, chunk_index);
                notify_upload_status(STATUS_ERROR);
                receiving = false;
                file_offset = 0;
                expected_chunk_index = 0;
                break;
            }
            receiving = false;
            notify_upload_status(STATUS_SUCCESS);
            printf("[BLE] Upload complete (%lu bytes)\n", file_offset);
            // Dump
            for (size_t i = 0; i < file_offset; i++) {
                printf("%02X ", file_buffer[i]);
                if ((i + 1) % 16 == 0) printf("\n");
            }
            printf("\nAs string: ");
            for (size_t i = 0; i < file_offset; ++i) {
                char c = (char)file_buffer[i];
                putchar((c >= 32 && c <= 126) ? c : '.');
            }
            putchar('\n');
            controller_should_run = true;
            btstack_run_loop_set_timer_handler(&controller_timer, &controller_interface_handler);
            btstack_run_loop_set_timer(&controller_timer, 1); // Run ASAP
            btstack_run_loop_add_timer(&controller_timer);
            expected_chunk_index = 0;
            break;

        default:
            printf("[BLE] ERROR: Unknown chunk type %d\n", chunk->type);
            notify_upload_status(STATUS_ERROR);
            receiving = false;
            file_offset = 0;
            expected_chunk_index = 0;
            break;
    }

    return 0;
}




