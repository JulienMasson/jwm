#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

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
	fscanf(fp,"%*s %Lf %Lf %Lf %Lf", &user[index], &nice[index], &system[index], &idle[index]);
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
