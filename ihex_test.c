/*
 * Copyright (c) 2012, Chris Anderson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution. 
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
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies, 
 * either expressed or implied, of the FreeBSD Project.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ihex.h>

// Simple  test binary for reading ihex files
void dump_hex(uint8_t *data, int len) {
	for (int x = 0; x < len; x++) {
		printf("%02X%c", data[x], ((x + 1) % 32) ? ' ' : '\n');
	}
	printf("\n");
}

int row_callback(unsigned int address, unsigned char *data, int len) {
	printf("address 0x%08X, len %d\n", address, len);
	dump_hex(data, len);
	return 0;
}

void print_ihex_struct(struct ihex_file *file_data) {
	for (int i = 0; i < file_data->row_cnt; i++) {
		printf("address 0x%08X, len %d\n", file_data->rows[i].addr, file_data->rows[i].count);
		dump_hex(file_data->rows[i].data, file_data->rows[i].count);
	}
}

int main(int argc, char *argv[]) {
	struct ihex_file file;

	if (argc < 2) {
		printf("usage: %s [ihex file]\n", argv[0]);
		return 1;
	}

	printf("data from callback: \n");
	parse_ihex_file(argv[1], &file, row_callback);
	printf("\ndata from structure: \n");
	print_ihex_struct(&file);
	free_ihex_file(&file);

	return 0;
}
