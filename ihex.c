#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

int read_hex_ints(int fd, uint8_t *data, int len)
{
	char buf[128];
	char tmp[3];
	uint32_t cnt, p = 0;
	tmp[2] = '\0';

	cnt = read(fd, buf, len * 2);
	if (cnt <= 0)
		return 0;

	// grab byte pairs and convert
	for (int x = 0; x < cnt; x += 2) {
		strncpy(tmp, (char *)(buf + x), 2);
		data[p++] = strtol(tmp, NULL, 16);
	}
}
	

int parse_ihex_file(char *file, int (*callback)(uint32_t address, uint8_t *data, size_t len)) 
{
	int fd;
	uint32_t base_addr = 0;

	char buf[512];
	if ((fd = open(file, O_RDONLY)) == -1)
		return 1;
	
	while (1) {
		char start;
		int ret;
		uint8_t count, type, checksum, data[64];
		uint16_t address;
		// find a start address
		do {
			ret = read(fd, &start, 1);
		} while (start != ':' && ret >= 0);
			
		if (start != ':') {
			printf("start: '%c'\n", start);
			return 2;
		}
		read_hex_ints(fd, &count, 1);
		read_hex_ints(fd, (uint8_t *)&address, 2);
		address = htons(address);
		read_hex_ints(fd, &type, 1);
		switch (type) {
		case 0x0:
			read_hex_ints(fd, data, count);
			read_hex_ints(fd, &checksum, 1);

			callback(base_addr | address, data, count);
			break;
		case 0x1:
			return 0;
			break;
		case 0x4:
			read_hex_ints(fd, (uint8_t *)&base_addr, 4);
			base_addr = htons(base_addr);
			base_addr <<= 16;
			printf("base updated to 0x%08X\n", base_addr);
			continue;
			break;
		}
	}

	return 0;
}
