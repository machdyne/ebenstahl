/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 * Copyright (c) 2025 Lone Dynamics Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "tusb.h"

#include "ebenstahl.h"
#include "mapper.h"
#include "led.h"
#include "drv_fram.h"
#include "drv_eeprom.h"
#include "drv_flash.h"

// 8KB (512 * 16) is the smallest size that windows allow to mount:
enum {
  DISK_BLOCK_SIZE = 512
};

// the mapper table has 16 entries, so LUN ids are in the range 0..15
#define ES_MAX_LUN 16

// Medium presence, tracked per LUN.
//
// The host ejects a LUN by sending START STOP UNIT with LoEj=1 and Start=0.
// If we keep reporting the medium as present after that, Linux and macOS
// simply rescan the LUN and re-mount it, which is why an eject appeared to
// have no effect. Once a LUN is marked ejected we report MEDIUM NOT PRESENT
// until the host explicitly loads it again or the device is re-enumerated.
static bool es_ejected[ES_MAX_LUN];

static inline bool es_lun_ejected(uint8_t lun) {
  return (lun < ES_MAX_LUN) ? es_ejected[lun] : false;
}

// Re-insert the medium on every LUN. Called from tud_mount_cb() so that
// unplugging and replugging the device undoes a previous eject.
void usb_msc_reset_eject(void) {
  for (int i = 0; i < ES_MAX_LUN; i++) es_ejected[i] = false;
  led_set_medium(LED_MEDIUM_PRESENT);
}

// any LUN still holding a medium counts as present; only LUNs the mapper
// actually defines get a vote, otherwise the unused slots (which are never
// ejected) would keep the medium looking present forever
static void es_update_medium_led(void) {
  int luns = mapper_luns();
  if (luns > ES_MAX_LUN) luns = ES_MAX_LUN;
  for (int i = 0; i < luns; i++) {
    if (!es_ejected[i]) { led_set_medium(LED_MEDIUM_PRESENT); return; }
  }
  led_set_medium(LED_MEDIUM_EJECTED);
}

// Invoked to determine max LUN
uint8_t tud_msc_get_maxlun_cb(void) {
  return mapper_luns();
}

// Invoked when received SCSI_CMD_INQUIRY
// Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
  (void) lun; // use same ID for both LUNs

  const char vid[] = "Machdyne";
  const char pid[] = "Ebenstahl";
  const char rev[] = "1.0";

  memcpy(vendor_id  , vid, strlen(vid));
  memcpy(product_id , pid, strlen(pid));
  memcpy(product_rev, rev, strlen(rev));
}

// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun) {

  // an ejected LUN must report as empty, otherwise the host re-mounts it
  if (es_lun_ejected(lun)) {
    tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3A, 0x00); // medium not present
    return false;
  }

  return true; // ready

}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
// Application update block count and block size
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size) {
  *block_count = mapper_lun_size(lun) / DISK_BLOCK_SIZE;
  *block_size  = DISK_BLOCK_SIZE;
}

// Invoked when received Start Stop Unit command
// - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// - Start = 1 : active mode, if load_eject = 1 : load disk storage
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
  (void) power_condition;

  if (lun >= ES_MAX_LUN) {
    led_note_fault();
    return false;
  }

  // a plain start/stop only changes the power condition; the medium is only
  // loaded or unloaded when LoEj is set
  if (load_eject) {
    if (start) {
      // load disk storage
      es_ejected[lun] = false;
    } else {
      // unload disk storage
      es_ejected[lun] = true;
    }
    es_update_medium_led();
  }

  return true;
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
// Handles transfers that may span multiple chips by splitting into per-chip chunks.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {

  // medium has been ejected
  if (es_lun_ejected(lun)) {
    tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3A, 0x00);
    led_note_fault();
    return -1;
  }

  uint32_t lun_size = (uint32_t) mapper_lun_size(lun);

  // out of space
  if (lba >= (lun_size / DISK_BLOCK_SIZE)) {
    tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x21, 0x00);
    led_note_fault();
    return -1;
  }

  uint32_t lun_addr = (lba * DISK_BLOCK_SIZE) + offset;
  uint32_t remaining = bufsize;
  uint8_t *buf_ptr = (uint8_t *) buffer;

  while (remaining > 0) {

    int rec = mapper_rec(lun, lun_addr);
    if (rec < 0) {
      tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x21, 0x00);
      led_note_fault();
      return -1;
    }

    int cs = mapper_get_cs(lun, lun_addr);
    int drv = mapper_get_drv(lun, lun_addr);
    uint32_t local_addr = mapper_get_local_addr(lun, lun_addr);
    int avail = mapper_remaining(lun, lun_addr);

    uint32_t chunk = (remaining < (uint32_t) avail) ? remaining : (uint32_t) avail;

    switch (drv) {
      case ES_DRV_EEPROM:
        eeprom_read(cs, (char *) buf_ptr, (int) local_addr, (int) chunk);
        break;
      case ES_DRV_FLASH:
        flash_read(cs, (char *) buf_ptr, (int) local_addr, (int) chunk);
        break;
      case ES_DRV_FRAM:
      default:
        fram_read(cs, (char *) buf_ptr, (int) local_addr, (int) chunk);
        break;
    }

    buf_ptr   += chunk;
    lun_addr  += chunk;
    remaining -= chunk;

  }

  led_note_activity();

  return (int32_t) bufsize;
}

bool tud_msc_is_writable_cb(uint8_t lun) {
  (void) lun;

  return !es_wp_is_on();

}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
// Handles transfers that may span multiple chips by splitting into per-chip chunks.
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {

  // medium has been ejected
  if (es_lun_ejected(lun)) {
    tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3A, 0x00);
    led_note_fault();
    return -1;
  }

  uint32_t lun_size = (uint32_t) mapper_lun_size(lun);

  // out of space
  if (lba >= (lun_size / DISK_BLOCK_SIZE)) {
    tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x21, 0x00);
    led_note_fault();
    return -1;
  }

  // write protect switch is on
  if (es_wp_is_on()) {
    led_note_fault();
    return -1;
  }

  uint32_t lun_addr = (lba * DISK_BLOCK_SIZE) + offset;
  uint32_t remaining = bufsize;
  uint8_t *buf_ptr = buffer;

  while (remaining > 0) {

    int rec = mapper_rec(lun, lun_addr);
    if (rec < 0) {
      tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x21, 0x00);
      led_note_fault();
      return -1;
    }

    int cs = mapper_get_cs(lun, lun_addr);
    int drv = mapper_get_drv(lun, lun_addr);
    uint32_t local_addr = mapper_get_local_addr(lun, lun_addr);
    int avail = mapper_remaining(lun, lun_addr);

    uint32_t chunk = (remaining < (uint32_t) avail) ? remaining : (uint32_t) avail;

    switch (drv) {
      case ES_DRV_EEPROM:
        eeprom_write(cs, (int) local_addr, (char *) buf_ptr, (int) chunk);
        break;
      case ES_DRV_FLASH:
        flash_write(cs, (int) local_addr, (char *) buf_ptr, (int) chunk);
        break;
      case ES_DRV_FRAM:
      default:
        fram_write(cs, (int) local_addr, (char *) buf_ptr, (int) chunk);
        break;
    }

    buf_ptr   += chunk;
    lun_addr  += chunk;
    remaining -= chunk;

  }

  led_note_activity();

  return (int32_t) bufsize;
}

// Callback invoked when received an SCSI command not in built-in list below
// - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
// - READ10 and WRITE10 has their own callbacks (MUST not be handled here)
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize) {
  void const* response = NULL;
  int32_t resplen = 0;

  // most scsi handled is input
  bool in_xfer = true;

  switch (scsi_cmd[0]) {
    default:
      // Set Sense = Invalid Command Operation
      tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);

      // negative means error -> tinyusb could stall and/or response with failed status
      return -1;
  }

  // return resplen must not larger than bufsize
  if (resplen > bufsize) resplen = bufsize;

  if (response && (resplen > 0)) {
    if (in_xfer) {
      memcpy(buffer, response, (size_t) resplen);
    } else {
      // SCSI output
    }
  }

  return resplen;
}
