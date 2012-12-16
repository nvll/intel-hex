#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int row_callback(uint32_t address, uint8_t *data, int len) {
	printf("address 0x%08X, len %d\n", address, len);
	return 0;
}

int main(int argc, char *argv[]) {

	if (argc < 2)
		return 1;

	return parse_ihex_file(argv[1], row_callback);
}
