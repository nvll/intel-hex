// vim: set expandtab:ts=4:sw=4:sts=4
/*
 * Copyright (c) 2013, Chris Anderson
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "ihex.h"

/* 255 reclen max + other fields */
#define MAX_RECORD_LEN  261
#define IHEX_DEBUG 0

#if IHEX_DEBUG == 0
#define printf(x...)
#endif

uint8_t nibble(char c)
{
    if (c <= '9')
        return (c - '0');
    else
        return (c - 'A') + 10;
}

int read_record(FILE *fd, uint8_t *buf, size_t len, uint8_t *_reclen, uint16_t *_offset, uint8_t *_rectype)
{
    char c;
    uint8_t reclen = 0;
    uint16_t offset = 0;
    uint8_t rectype = 0;
    uint8_t checksum = 0;
    uint32_t total_checksum = 0;

    /* check mark */
    if (getc(fd) != ':')
        return 1;

    reclen |= nibble(getc(fd)) << 4;
    reclen |= nibble(getc(fd));
    total_checksum += reclen;

    offset |= nibble(getc(fd)) << 12;
    offset |= nibble(getc(fd)) << 8;
    offset |= nibble(getc(fd)) << 4;
    offset |= nibble(getc(fd));
    total_checksum += (offset & 0xFF) + ((offset >> 8) & 0xFF);

    rectype |= nibble(getc(fd)) << 4;
    rectype |= nibble(getc(fd));
    total_checksum += rectype;
    printf("reclen %02x, offset: %08x, type %02x [", reclen, offset, rectype);

    for (int i = 0; i < reclen; i++) {
        buf[i] = nibble(getc(fd)) << 4;
        buf[i] |= nibble(getc(fd));
        total_checksum += buf[i];
        printf("%02x", buf[i]);
    }
    printf("] \n");

    checksum |= nibble(getc(fd)) << 4;
    checksum |= nibble(getc(fd));
    total_checksum += checksum;

    if ((total_checksum & 0xFF) != 0)
        return 2;

    while ((c = getc(fd)) != '\n') ;

    *_reclen = reclen;
    *_rectype = rectype;
    *_offset = offset;
    return 0;
}

static void create_section(struct ihex_file *file_data, uint32_t addr)
{
    void *new_alloc = NULL;
    uint32_t sid = file_data->section_cnt;
    struct ihex_section *s;

    if (file_data->section_cnt >= file_data->alloc_section_cnt) {
        file_data->alloc_section_cnt += 2;
        new_alloc = realloc(file_data->sections, sizeof(struct ihex_section) * file_data->alloc_section_cnt);

        if (new_alloc != NULL) {
            file_data->sections = new_alloc;
        }
    }
    s = &file_data->sections[sid];
    s->len = 0;
    s->alloc_len = 32;
    s->data = malloc(s->alloc_len);
    s->addr = addr;
    file_data->section_cnt++;
}

static void resize_section_to_fit(struct ihex_section *s, uint32_t new_len)
{
    void *new_alloc = NULL;
    uint32_t old_alloc = s->alloc_len;
    uint32_t needed_size = s->len + new_len;

    if (s->alloc_len > needed_size) {
        return;
    }

    while (s->alloc_len <= needed_size) {
        if (s->alloc_len == 0) {
            s->alloc_len = 16;
        } else {
            s->alloc_len *= 2;
        }
    }

    new_alloc = realloc(s->data, s->alloc_len);

    if (new_alloc != NULL) {
        printf("resized section from %u to %u\n", old_alloc, s->alloc_len);
        s->data = new_alloc;
        // zero out the new heap allocation otherwise offset jumps may contain garbage
        memset(s->data + old_alloc, 0, s->alloc_len - old_alloc);
    }
}


int parse_ihex_file(const char *filename, struct ihex_file *file_data)
{
    FILE *fd;
    uint32_t base_addr = 0;
    uint32_t addr_expected = 0;
    int line = 1;
    uint8_t buf[MAX_RECORD_LEN];
    int ret = 0;
    uint8_t reclen = 0;
    uint16_t offset = 0;
    uint8_t rectype = 0;

    if (!(fd = fopen(filename, "r"))) {
        fprintf(stderr, "couldn't open %s: ", filename);
        perror(NULL);
        return -1;
    }

    if (!file_data) {
        ret = -2;
        goto exit;
    }

    file_data->section_cnt = 0;
    file_data->alloc_section_cnt = 0;
    file_data->sections = NULL;

    while ((read_record(fd, buf, sizeof(buf), &reclen, &offset, &rectype)) == 0) {
        switch (rectype) {
            case 0x0:
                if (file_data->section_cnt == 0) {
                    printf("creating initial section for 0x%08x\n", base_addr + offset);
                    create_section(file_data, base_addr + offset);
                }

                int section_id = file_data->section_cnt - 1;
                struct ihex_section *s = &file_data->sections[section_id];

                /* objcopy will sometimes jump to an offset that isn't contiguous and the offset becomes
                 * out of sync with the overall length. This handles that case */
                if (base_addr + offset != addr_expected && line != 1) {
                    uint32_t addr_diff = (base_addr + offset) - addr_expected;
                    printf("hex file jumps %04x bytes from 0x%08x to 0x%08x, adjusting\n", addr_diff, addr_expected, base_addr + offset);
                    resize_section_to_fit(s, s->len + addr_diff);
                    s->len += addr_diff;
                }

                resize_section_to_fit(s, reclen);
                memcpy(s->data + s->len, buf, reclen);
                s->len += reclen;
                addr_expected = base_addr + offset + reclen;
                break;
            case 0x1:
                ret = 0;
                goto exit;
                break;
            case 0x2:
                /* x86 real mode addressing. The data field contains a value which when shifted left by 4 becomes the new
                 * higher level address bits. Ideally everything would just use the 32 bit addressing, but alas. */
                base_addr = (buf[0] << 8 | buf[1]) << 4;
                break;
                // Extended Linear Address
            case 0x4:
                base_addr = htons(offset);
                base_addr <<= 16;
                if (base_addr != addr_expected) {
                    printf("creating new section for 0x%08x (base: 0x%08x + off: 0x%08x)\n", base_addr + offset, base_addr, offset);
                    create_section(file_data, base_addr);
                }
                addr_expected = base_addr;
                break;
            default:
                // 3 and 5 are not implemented
                break;
        }

        line++;
    }

exit:
    fclose(fd);
    return ret;
}

void free_ihex_file(struct ihex_file *file_data)
{
    for (int i = 0; i < file_data->section_cnt; i++) {
        free(file_data->sections[i].data);
    }

    free(file_data->sections);
}
