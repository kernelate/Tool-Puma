#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(void)
{
	FILE *fp = fopen("test_client.c","rb");
	unsigned char checksum = 0;
	while (!feof(fp) && !ferror(fp)) {
   	checksum ^= fgetc(fp);
	}

	printf("checksum: %u\n",checksum);
	fclose(fp);

	return 0;
}

