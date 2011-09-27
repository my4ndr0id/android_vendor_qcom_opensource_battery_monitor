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
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include "battery_monitor.h"

#define KERNEL_EINVAL -22

void write_persisted_values()
{
	int value;
	int rc;
	DIR *dp;
	struct dirent *dirp;


	if ((dp = opendir(SYS_BMS)) == NULL)
		/* can't read directory */
		return;

	while ((dirp = readdir(dp)) != NULL) {
		/* ignore dot and dot-dot */
		if (strcmp(dirp->d_name, ".") == 0 ||
			strcmp(dirp->d_name, "..") == 0)
			continue;

		rc = read_int_from_file(PERSISTENT_LOCATION, dirp->d_name, &value);
		if (rc == 0 && value != KERNEL_EINVAL) {
			write_int_to_file(SYS_BMS, dirp->d_name, value);
		}

	}
	closedir(dp);
}

void read_values_and_store()
{
	int value;
	int rc;
	DIR *dp;
	struct dirent *dirp;

	if ((dp = opendir(SYS_BMS)) == NULL)
		/* can't read directory */
		return;

	while ((dirp = readdir(dp)) != NULL) {
		/* ignore dot and dot-dot */
		if (strcmp(dirp->d_name, ".") == 0 ||
			strcmp(dirp->d_name, "..") == 0)
			continue;

		rc = read_int_from_file(SYS_BMS, dirp->d_name, &value);
		if (rc == 0 && value != KERNEL_EINVAL) {
			write_int_to_file(PERSISTENT_LOCATION, "tmp", value);
			rename_file(PERSISTENT_LOCATION, "tmp", PERSISTENT_LOCATION, dirp->d_name);
		}

	}
	closedir(dp);
}

void sigaction_handler(int signum, siginfo_t *info, void *context)
{
	read_values_and_store();
}

void register_signals(void) {
	struct sigaction act;

	act.sa_sigaction = sigaction_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	act.sa_restorer = 0;
	sigaction(SIGUSR1, &act, 0);
	sigaction(SIGUSR2, &act, 0);
	sigaction(SIGFPE, &act, 0);
}

int main(int argc, char *argv[])
{
	int last_charge_increase;
	int last_charge_cycles;
	int last_ocv;
	int last_rbatt;
	int rc;
	pid_t mypid;

	if (!directory_exists(SYS_BMS))
		return 0;

	/* write the pid of this daemon in the killer process */
	mypid = getpid();
	rc = write_int_to_file(PERSISTENT_LOCATION, PID_FILENAME, mypid);
	if (rc < 0)
		return rc;

	register_signals();

	write_persisted_values();

	while(1)
	{
		read_values_and_store();

		/* sleep 30 minutes */
		sleep(30*60);
	}

}

