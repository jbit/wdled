/*
 * wdled - Control the LED mode of WD My Passport Disks
 * 
 * https://jbit.net/wdled
 * 
 * Copyright 2020 James Lee (jbit@jbit.net)
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain
 *      the above copyright notice,
 *      this list of conditions
 *      and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce
 *      the above copyright notice,
 *      this list of conditions
 *      and the following disclaimer
 *      in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <scsi/sg_cmds_basic.h>
#include <scsi/sg_lib.h>

#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define CMD_NAME    "wdled"
#define CMD_VER     "v0.1"
#define CMD_URL     "https://jbit.net/wdled/"
#define PAGE_CODE   0x21
#define PAGE_MAGIC  0x30
#define PS_BIT      (1<<7) // Parameters saveable
#define SPF_BIT     (1<<6) // Sub page format

// A list of verified working WD product names
const char* wd_products[] = {
    "My Passport 259D",
    "My Passport 259E",
    "My Passport 259F",
    "My Passport 259A",
    "My Passport 25E1",
    "My Passport 25E2",
    NULL,
};

static const struct { const char* vendor; const char** products; } supported[] = {
    { vendor: "WD      ", products: wd_products },
    { vendor: NULL,       products: NULL },
};

struct page {
    // Header bytes
    uint8_t code; // Page code and PS/SPF bits
    uint8_t len;  // Length of parameters in bytes

    // Payload
    union {
        struct {
            // Guessed layout of the WD 0x21 mode page
            uint8_t magic; // Version? Always 0x30. Not modifiable
            uint8_t zeros0;
            uint8_t zeros1;
            uint8_t unknown1; // Flags? some bits modifiable
            uint8_t zeros2;
            uint8_t zeros3;
            uint8_t led;      // LED control 0x00=off, 0xff=on, other=Error
            uint8_t zeros4;
            uint8_t zeros5;
            uint8_t zeros6;
        } wd21;
        uint8_t bytes[32];
    };
};

// This can be entirely zero for a MODE SELECT packet
struct mode_parameter_header {
    uint16_t len;
    uint8_t  medium_type;
    uint8_t  flags0; // WP/DPOFUA bits
    uint8_t  flags1; // LONGLBA bit
    uint8_t  reserved;
    uint16_t block_descriptor_length;
};


int main(const int argc, const char* const argv[]) {
    if (argc < 2 || argc > 3 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-help") || !strcmp(argv[1], "-h")) {
        // Print basic help
        eprintf("%s %s (%s) - Control the LED mode of WD My Passport Disks\n", CMD_NAME, CMD_VER, CMD_URL);
        eprintf("sg_cmds v%s\n", sg_cmds_version());
        eprintf("Usage: %s DEVICE [VALUE]\n", argv[0]);
        eprintf("  DEVICE: SCSI device to control (e.g /dev/disk/by-id/usb-WD_My_Passport_...)\n");
        eprintf("  VALUE:  LED mode to set ('on' or 'off', 0 or 255)\n");
        eprintf("          Omit to read current mode\n");
        eprintf("          Prefix with 'save:' to have the disk remember the LED mode\n");  
        eprintf("\n");
        eprintf("Example: (to turn the LED off permanently)\n");
        eprintf("  %s /dev/disk/by-id/usb-WD_My_Passport_foo save:off\n", argv[0]);
        eprintf("\n");
        eprintf("Supported devices:\n");
        for (size_t vid=0; supported[vid].vendor; vid++) {
            for (size_t pid=0; supported[vid].products[pid]; pid++) {
                eprintf("  %s %s\n", supported[vid].vendor, supported[vid].products[pid]);
            }
        }
        return 1;
    }

    // Process argument
    bool force = false;
    bool save = false;
    int new = -1;
    if (argc > 2) {
        const char* arg = argv[2];
        if (!strcmp(arg, "FORCEGET")) {
            // Get value, with no vendor/product checks
            force = true;
        } else {
            const char* const force_str = "FORCESET:";
            if (!strncmp(arg, force_str, strlen(force_str))) {
                // Set value, with no vendor/product checks
                arg += strlen(force_str);
                force = true;
            }
            const char* const save_str = "save:";
            if (!strncasecmp(arg, save_str, strlen(save_str))) {
                // Set value, and save
                arg += strlen(save_str);
                save = true;
            }
            if (!strcmp(arg, "off")) {
                new = 0;
            } else if (!strcmp(arg, "on")) {
                new = 255;
            } else {
                char* endptr;
                new = strtol(arg, &endptr, 0);
                if (endptr != (arg + strlen(arg)) || new < 0x00 || new > 0xff) {
                    eprintf("Unknown value: %s\n", arg);
                    return 1;
                }
            }
        }
    }
    if (force) {
        eprintf("WARNING: Skipping supported vendor/product checks!\n");
    }

    const char* const device = argv[1];
    const bool read_only = new < 0;
    const int verbose = 0;
    const bool noisy = true;

    int fd = sg_cmds_open_device(device, read_only, verbose);
    if(fd < 0) {
        eprintf("%s: ERROR: Failed to open (%s)\n", device, safe_strerror(-fd));
        return 1;
    }

    // Verify that we know about the disk model

    struct sg_simple_inquiry_resp inquiry;
    int result = sg_simple_inquiry(fd, &inquiry, noisy, verbose);
    if(result != 0) {
        eprintf("%s: ERROR: Inquiry failed (%s)\n", device, safe_strerror(result));
        return 1;
    }
    eprintf("%s: %s %s (rev %s)\n", device, inquiry.vendor, inquiry.product, inquiry.revision);
    size_t vid = 0, pid = 0;
    for (vid=0; supported[vid].vendor; vid++) {
        if (!strcmp(supported[vid].vendor, inquiry.vendor)) {
            for (pid=0; supported[vid].products[pid]; pid++) {
                if (!strcmp(supported[vid].products[pid], inquiry.product)) {
                    break;
                }
            }
            break;
        }
    }
    if (!supported[vid].vendor) {
        if (!force) {
            eprintf("%s: ERROR: Unknown or unsupported vendor!\n", device);
            return 1;
        } else {
            eprintf("MANUALLY SKIPPED UNSUPPORTED VENDOR CHECK!\n");
        }
    } else {
        if (!supported[vid].products[pid]) {
            if (!force) {
                eprintf("%s: ERROR: Unknown or unsupported product!\n", device);
                return 1;
            } else {
                eprintf("MANUALLY SKIPPED UNSUPPORTED DEVICE CHECK!\n");
            }
        }
    }

    // Read the mode page we're interested in
    int page_len = sizeof(struct page);
    struct page current = {}, changeable = {}, original = {}, saved = {};
    void *arr[4] = { &current, &changeable, &original, &saved };
    result = sg_get_mode_page_controls(fd, false, PAGE_CODE, 0, true, false, page_len, NULL, arr, &page_len, verbose);
    if(result != 0) {
        eprintf("%s: ERROR: Get mode page failed (%s)\n", device, safe_strerror(result));
        return 1;
    }

    // Verify details about the modepage
    const uint8_t code = PAGE_CODE | PS_BIT;
    if (current.code != code || changeable.code != code || original.code != code || saved.code != code) {
        eprintf("%s: ERROR: Unexpected mode page id (0x%02x)\n", device, current.code);
        return 1;
    }
    const uint8_t wd21_len = sizeof(current.wd21);
    if (current.len != wd21_len || changeable.len != wd21_len || original.len != wd21_len || saved.len != wd21_len) {
        eprintf("%s: ERROR: Unexpected mode page length (0x%02x)\n", device, current.len);
        return 1;
    }
    if (current.wd21.magic != PAGE_MAGIC) {
        eprintf("%s: ERROR: Unexpected mode page magic (0x%02x)\n", device, current.wd21.magic);
        return 1;
    }
    if (changeable.wd21.led != 0xff) {
        eprintf("%s: ERROR: LED bits don't appear changeable (0x%02x)\n", device, changeable.wd21.led);
        return 1;
    }

    // Print the LED values!
    printf("LED: current=%d original=%d saved=%d\n", current.wd21.led, original.wd21.led, saved.wd21.led);

    if (new >= 0) {
        // Build a mode select parameter list payload
        struct { struct mode_parameter_header header; struct page page; } packet;
        memset(&packet, 0, sizeof(packet));
        memcpy(&packet.page, &current, sizeof(current));
        packet.page.code &= current.code & 0x7f; // Clear PS bit

        // Set the new LED mode value
        packet.page.wd21.led = new;

        // Send the mode select packet!
        const size_t packet_size = sizeof(packet.header) + 2 + sizeof(packet.page.wd21);
        const bool page_format = true;
        result = sg_ll_mode_select10(fd, page_format, save, &packet, packet_size, noisy, verbose);
        if(result != 0) {
            eprintf("%s: ERROR: Set mode page failed (%s)\n", device, safe_strerror(result));
            return 1;
        }
    }

    return 0;
}
