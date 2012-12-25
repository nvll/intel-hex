/*
 * Copyright (c) 2012, Chris Anderson
 * All rights reserved.
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
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <ihex.h>

int read_record(FILE *fd, char *buf, int size) {
	char c;
	int len = 0;
	c = getc(fd);
	if (c != ':')
		return 0;

	while ((c = getc(fd))) {
		switch (c) {
		case '\r':
			getc(fd);
			return len;
			break;
		default:
			buf[len++] = c;
			break;
		}
	}

	return 0;
}
	
void read_hex_ints(char *buf, uint8_t *data, int len, uint32_t *offset) {
	char tmp[3];
	uint32_t p = 0;
	tmp[2] = '\0';

	// grab byte pairs and convert
	for (int x = 0; x < len * 2; x += 2) {
		strncpy(tmp, (char *)(buf + *offset + x), 2);
		data[p++] = strtol(tmp, NULL, 16);
	}
	*offset += len * 2;
}
	

 parse_ihex_file(const char *file, struct ihex_file *file_data, int (*callback)(unsigned int address, unsigned char *data, int len)) {
	FILE *fd;
	uint32_t base_addr = 0;
	int len, ret, line = 1;
	char buf[512];

	if (!(fd = fopen(file, "r")))
		return 1;

	if (file_data) {
		file_data->row_cnt = 0;
		file_data->rows = NULL;
	}
	
	while ((len = read_record(fd, buf, sizeof(buf))) > 0) {
		uint8_t start, count, type, checksum, data[64];
		uint16_t address;
		uint32_t sum, offset = 0;
		
		read_hex_ints(buf, &count, 1, &offset);
		read_hex_ints(buf, (uint8_t *)&address, 2, &offset);
		address = htons(address);
		read_hex_ints(buf, &type, 1, &offset);

		switch (type) {
		case 0x0:
			read_hex_ints(buf, data, count, &offset);
			read_hex_ints(buf, &checksum, 1, &offset);
			sum = count + address + (address >> 8) + type;
			for (int x = 0; x < count; x++)
				sum += data[x];

			if ((-sum & 0xFF) != checksum) {
				printf("bad checksum on line %d: %X != %X\n", line, (-sum) & 0xFF, checksum);
				return 1;
			}
		
			if (file_data) {
				file_data->rows = realloc(file_data->rows, sizeof(struct ihex_row) * (file_data->row_cnt + 1));
				file_data->rows[file_data->row_cnt].addr = base_addr | address;
				file_data->rows[file_data->row_cnt].count = count;
				file_data->rows[file_data->row_cnt].data = malloc(count);
				memcpy(file_data->rows[file_data->row_cnt].data, data, count);
				file_data->row_cnt++;
			}

			if (callback != NULL)
				callback(base_addr | address, data, count);
			break;
		case 0x1:
			return 0;
			break;
		case 0x4:
			read_hex_ints(buf, (uint8_t *)&base_addr, 4, &offset);
			base_addr = htons(base_addr);
			base_addr <<= 16;
			break;
		}

		line++;
	}

	return 0;
}

void free_ihex_file(struct ihex_file *file_data) {
	for (int i = 0; i < file_data->row_cnt; i++)
		free(file_data->rows[i].data);
	free(file_data->rows);
}
