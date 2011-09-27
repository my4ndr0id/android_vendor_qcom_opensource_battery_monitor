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

#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#define PERSISTENT_LOCATION	"/data/bms/"
#define SYS_BMS			"/sys/module/pm8921_bms/parameters/"

#define LAST_CHARGE_INCREASE	"last_charge_increase"
#define LAST_CHARGE_CYCLES	"last_chargecycles"
#define LAST_OCV		"last_ocv_uv"
#define LAST_RBATT		"last_rbatt"
#define PID_FILENAME		"daemon_pid"

int directory_exists(char *path);
static int file_exists(char *path);
int read_int_from_file(char *prefix, char *path, int *ret_val);
int write_string_to_file(char *prefix, char *path, char *string);
int write_int_to_file(char *prefix, char *path, int value);
int rename_file(char *oldprefix, char *oldpath, char *newprefix, char *newpath);
#endif  /* BATTERY_MONITOR_H */
