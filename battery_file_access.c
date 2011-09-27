/*
Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of Code Aurora Forum, Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/rtc.h>
#include <linux/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "battery_monitor.h"

int directory_exists(char *path)
{
	struct stat s;
	int rv;

	rv = stat(path, &s);
	return rv ? 0 : S_ISDIR(s.st_mode);
}

int file_exists(char *path)
{
	struct stat s;
	int rv;

	rv = stat(path, &s);
	return rv ? 0 : S_ISREG(s.st_mode);
}

/*
 * prefix can be NULL, path must not be NULL
 * return
 *      >=0: fd
 *	<0: -errno
 */
static int open_file(char *prefix, char *path, int flags)
{
	int prefix_len = (prefix == NULL) ? 0 : strlen(prefix);
	int path_len = strlen(path);
	char *fullpath;
	int rv;

	fullpath = malloc(prefix_len + path_len + 1);
	if (fullpath == NULL)
		return -ENOMEM;

	memcpy(fullpath, prefix, prefix_len);
	memcpy(fullpath + prefix_len, path, path_len + 1);

	rv = open(fullpath, flags, 0660);
	if (rv < 0)
		rv = -errno;

	free(fullpath);
	return rv;
}

/*
 * block until end of file or the specified amount is read
 * <0: -errno
 */
static int read_from_fd(int fd, char *buf, ssize_t count)
{
	ssize_t pos = 0;
	ssize_t rv = 0;

	do {
		pos += rv;
		rv = read(fd, buf + pos, count - pos);
	} while (rv > 0);

	return (rv < 0) ? -errno : pos;
}

/*
 * block until all data is written out
 * <0: -errno
 */
static int write_to_fd(int fd, char *buf, ssize_t count)
{
	ssize_t pos = 0;
	ssize_t rv = 0;

	do {
		rv = write(fd, buf + pos, count - pos);
		if (rv < 0)
			return -errno;
		pos += rv;
	} while (count > pos);

	return count;
}

static int read_int_from_fd(int fd, int *ret_val)
{
	char buffer[16];
	char *endptr;
	long value;
	int rv;

	rv = read_from_fd(fd, buffer, sizeof(buffer) - 1);
	if (rv < 0)
		return rv;
	if (rv == 0)
		return -ENODATA;

	buffer[rv] = '\0';

	value = strtol(buffer, &endptr, 10);
	if (endptr == buffer)
		return -EBADMSG;
	/* if (value == LONG_MAX || value > INT_MAX)
		return -ERANGE;
		*/

	*ret_val = (int)value;
	return 0;
}

static int read_from_file(char *prefix, char *path, char *buf, size_t count)
{
	int fd;
	int rv;

	fd = open_file(prefix, path, O_RDONLY);
	if (fd < 0)
		return fd;

	rv = read_from_fd(fd, buf, count);
	close(fd);

	return rv;
}

int read_int_from_file(char *prefix, char *path, int *ret_val)
{
	int fd;
	int rv;

	fd = open_file(prefix, path, O_RDONLY);
	if (fd < 0)
		return fd;

	rv = read_int_from_fd(fd, ret_val);
	close(fd);

	return rv;
}

static int write_to_file(char *prefix, char *path, char *buf, ssize_t count)
{
	int fd;
	int rv;

	fd = open_file(prefix, path, O_WRONLY|O_CREAT|O_TRUNC);
	if (fd < 0) {
		return fd;
	}

	rv = write_to_fd(fd, buf, count);
	close(fd);

	return rv;
}

int write_string_to_file(char *prefix, char *path, char *string)
{
	int rv = 0;
	rv = write_to_file(prefix, path, string, strlen(string));
	sync();
	return rv;
}

int write_int_to_file(char *prefix, char *path, int value)
{
	char buffer[16];

	snprintf(buffer, sizeof(buffer) - 1, "%d", value);
	buffer[sizeof(buffer) - 1] = '\0';

	return write_string_to_file(prefix, path, buffer);
}

static int sync_file(char *prefix, char *path)
{
	int fd;
	int rv;

	fd = open_file(prefix, path, O_WRONLY);
	if (fd < 0)
		return fd;

	rv = fsync(fd);
	close(fd);

	return rv;
}

int rename_file(char *oldprefix, char *oldpath, char *newprefix, char *newpath)
{
	int oldprefix_len = (oldprefix == NULL) ? 0 : strlen(oldprefix);
	int oldpath_len = strlen(oldpath);
	int newprefix_len = (newprefix == NULL) ? 0 : strlen(newprefix);
	int newpath_len = strlen(newpath);
	char *oldfullpath;
	char *newfullpath;
	int rv;

	oldfullpath = malloc(oldprefix_len + oldpath_len + 1);
	if (oldfullpath == NULL) {
		rv = -ENOMEM;
		goto out;
	}

	newfullpath = malloc(newprefix_len + newpath_len + 1);
	if (newfullpath == NULL) {
		rv = -ENOMEM;
		goto free_oldpath;
	}

	memcpy(oldfullpath, oldprefix, oldprefix_len);
	memcpy(oldfullpath + oldprefix_len, oldpath, oldpath_len + 1);

	memcpy(newfullpath, newprefix, newprefix_len);
	memcpy(newfullpath + newprefix_len, newpath, newpath_len + 1);

	rv = rename(oldfullpath, newfullpath);
	if (rv < 0)
		rv = -errno;

	rv = sync_file(newprefix, newpath);
	if (rv < 0)
		rv = -errno;

	free(newfullpath);
free_oldpath:
	free(oldfullpath);
out:
	return rv;
}
