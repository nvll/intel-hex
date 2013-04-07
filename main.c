// vim: set expandtab:ts=4:sw=4:sts=4
/*
 * Copyright (c) 2013, Chris Anderson
 * All rights reserved.
 *
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
#include "ihex.h"

// Simple test binary for reading ihex files
void dump_hex(uint8_t *data, int len) {
    for (int x = 0; x < len; x++) {
        printf(" %02x", data[x]);
    }
    printf("\n");
}

void print_ihex_struct(struct ihex_file *file_data) {
    for (int i = 0; i < file_data->section_cnt; i++) {
        struct ihex_section *s = &file_data->sections[i];
        printf("address 0x%08X, len %d\n", s->addr, s->len);
        for (int y = 0; y < s->len; y += 16) {
            printf("%08x:", s->addr + y);
            dump_hex(s->data + y, (y + 16 < s->len) ? 16 : (s->len - y));
        }
    }
}

int main(int argc, char *argv[]) {
    struct ihex_file file;

    if (argc < 2) {
        printf("usage: %s [ihex file]\n", argv[0]);
        return 1;
    }

    parse_ihex_file(argv[1], &file);
    print_ihex_struct(&file);
    free_ihex_file(&file);

    return 0;
}
