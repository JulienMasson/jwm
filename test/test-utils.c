/*
 * This file is part of the jwm distribution:
 * https://github.com/JulienMasson/jwm
 *
 * Copyright (c) 2018 Julien Masson.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "core.h"
#include "utils.h"


START(file_access_pass)
{
	fail_unless(file_access("/etc/passwd") == true,
		    "Failed to access /etc/passwd");
}
END(file_access_pass);


START(file_access_fail)
{
	fail_unless(file_access("/etc/tatatat") == false,
		    "Shouldn't access to /etc/tatatat");
}
END(file_access_fail);


START(file_in_dir_pass)
{
	char **files = NULL;
	int count;
	files = file_in_dir("/etc/", &count);
	fail_unless(files != NULL, "Failed to read files in dir");
}
END(file_in_dir_pass);


START(file_in_dir_fail)
{
	char **files = NULL;
	files = file_in_dir("/tatatat", NULL);
	fail_unless(files == NULL, "Shouldn't access to files in /tatatat");
}
END(file_in_dir_fail);
