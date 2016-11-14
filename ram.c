#include <stdio.h>
#include <errno.h>
#include <string.h>

#define MEM_INFO_PATH "/proc/meminfo"

char *
ram(void)
{
	static char ram[12];
	FILE *file;
	char line[256];
	int mem_total;
	int mem_free;
	int buffers;
	int cached;

	memset(ram, '\0', sizeof(ram));
	sprintf(ram, "RAM: ");

	if ((file = fopen(MEM_INFO_PATH, "r")) == NULL) {
		fprintf(stderr, "open: %s\n", strerror(errno));
		return "RAM FAILED\0";
	}

	while (fgets(line, sizeof(line), file)) {
		if (strstr(line, "MemTotal")) {
			sscanf(line, "MemTotal: %d kB", &mem_total);
		} else if (strstr(line, "MemFree"))
			sscanf(line, "MemFree: %d kB", &mem_free);
		else if (strstr(line, "Buffers"))
			sscanf(line, "Buffers: %d kB", &buffers);
		else if (strstr(line, "Cached"))
			sscanf(line, "Cached: %d kB", &cached);
	}

	sprintf(ram + strlen(ram), "%d MB", (mem_total - mem_free - buffers - cached) / 1000);

	fclose(file);
	return ram;
}
