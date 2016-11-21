#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "util.h"

#define PATH_CPU_TEMP "/sys/devices/virtual/thermal/"

char *
cpu_temp(void)
{
	static char batt[11];
	static char cpu_temp_path[PATH_MAX + 1] = "\0";
	char value[5];
	int fd = -1;

	memset(batt, '\0', sizeof(batt));
	sprintf(batt, "Temp: ");

	if (!strlen(cpu_temp_path))
		find_file_first_match("temp", PATH_CPU_TEMP, 2, cpu_temp_path, sizeof(cpu_temp_path));

	if (strlen(cpu_temp_path)) {

		if ((fd = open(cpu_temp_path, O_RDONLY)) < 0) {
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
	sprintf(batt + strlen(batt), "%d C",atoi(strtok(value, "\n")) / 1000);
	close(fd);
	return batt;
}

char *
cpu(void)
{
	static long time = 0;
	static int index = 0;
	static long double user[2];
	static long double nice[2];
	static long double system[2];
	static long double idle[2];
	static char cpu[10];
	FILE *fp;
	struct timeval tv;
	long double diff;

	if (!strlen(cpu))
		memset(cpu, '\0', sizeof(cpu));

	if (gettimeofday(&tv, NULL) == -1)
		fprintf(stderr, "gettimeofday failed");


	fp = fopen("/proc/stat","r");
	if (fscanf(fp,"%*s %Lf %Lf %Lf %Lf", &user[index], &nice[index], &system[index], &idle[index]) == EOF)
		fprintf(stderr, "fscanf failed: %s\n", strerror(errno));
	fclose(fp);

	diff = ((user[index]+nice[index]+system[index]) - (user[(index+1)%2]+nice[(index+1)%2]+system[(index+1)%2])) / ((user[index]+nice[index]+system[index]+idle[index]) - (user[(index+1)%2]+nice[(index+1)%2]+system[(index+1)%2]+idle[(index+1)%2]));

	int time_elapsed = (tv.tv_sec - time);
	if (time_elapsed > 0) {
		int perc = (int)((diff / time_elapsed)*100);
		sprintf(cpu, "CPU: %d%%", perc);
		index = (index+1)%2;
		time = tv.tv_sec;
	}
	return cpu;
}
