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

	return 1;
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
	

int parse_ihex_file(const char *file, int (*callback)(unsigned int address, unsigned char *data, int len)) {
	FILE *fd;
	uint32_t base_addr = 0;
	int len, ret, line = 1;
	char buf[512];

	if (!(fd = fopen(file, "r")))
		return 1;
	
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
