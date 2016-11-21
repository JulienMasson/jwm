#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include "util.h"

#define PATH_POWER_SUPPLY "/sys/class/power_supply/"

char *
battery(void)
{
	static char batt[11];
	static char capacity_path[PATH_MAX + 1] = "\0";
	char value[4];
	int fd = -1;

	memset(batt, '\0', sizeof(batt));
	sprintf(batt, "BAT: ");

	if (!strlen(capacity_path))
		find_file_first_match("capacity", PATH_POWER_SUPPLY, 2, capacity_path, sizeof(capacity_path));

	if (strlen(capacity_path)) {

		if ((fd = open(capacity_path, O_RDONLY)) < 0) {
			fprintf(stderr, "open: %s\n", strerror(errno));
			goto out;
		}

		if ((read(fd, value, sizeof(value))) < 0) {
			fprintf(stderr, "read: %s\n", strerror(errno));
			close(fd);
			goto out;
		}
	}

out:
	sprintf(batt + strlen(batt), "%s%%",strtok(value, "\n"));
	close(fd);
	return batt;
}
