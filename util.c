#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "util.h"

void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;

	if (!(p = calloc(nmemb, size)))
		perror(NULL);
	return p;
}

void
die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}

	exit(1);
}

static void
find_file(DIR *d, int max_depth, char *match, char *result, size_t size_max, int *found)
{
	struct dirent *de;
	int dfd, fd;
	char savedpath[size_max];

	dfd = dirfd(d);
	if (dfd < 0) {
		fprintf(stderr, "Failed to get dir file descriptor %d\n", dfd);
		goto out;
	}

	strcpy(savedpath, result);
	while ((de = readdir(d)) && max_depth > 0 && !(*found)) {
		DIR *d2;

		if (!strcmp(de->d_name, match)) {
			if (strlen(result) + strlen(match) > size_max)
				memset(result, '\0', size_max);
			else
				sprintf(result + strlen(result), "%s", match);
			*found = 1;
		}

		if ((de->d_type != DT_DIR && !(de->d_type == DT_LNK)) ||
		    de->d_name[0] == '.') {
			continue;
		}

		fd = openat(dfd, de->d_name, O_RDONLY | O_DIRECTORY);
		if (fd < 0) {
			fprintf(stderr, "cannot openat %d '%s' (%d: %s)\n", dfd, de->d_name,
			     errno, strerror(errno));
			continue;
		}

		d2 = fdopendir(fd);
		if (d2 != 0) {
			snprintf(result + strlen(result), size_max - strlen(result), "%s/", de->d_name);
			find_file(d2, max_depth - 1, match, result, size_max, found);
			closedir(d2);
			if (!(*found))
				strcpy(result, savedpath);
		}
		close(fd);
	}

	close(dfd);
out:
	return;
}

void
find_file_first_match(char *name, char *path, int depth, char *result, size_t size_max)
{
	DIR *d = opendir(path);
	int found = 0;
	memset(result, '\0', size_max);

	if (strlen(path) < size_max) {

		sprintf(result, "%s", path);

		if (d) {
			find_file(d, depth, name, result, size_max, &found);
			closedir(d);
		}

	}

	if (!found)
		memset(result, '\0', size_max);
}
