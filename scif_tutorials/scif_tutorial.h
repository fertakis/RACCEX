/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

#ifndef __SCIF_TUTORIAL_H__
#define __SCIF_TUTORIAL_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

#ifndef _WIN32
#include <unistd.h>
#include <sys/ioctl.h>
#else
#include <Windows.h>
#endif

/* Returns last errno */
int get_curr_status()
{
#ifdef _WIN32
	DWORD ecode;
	ecode = GetLastError();
	if (ecode == ERROR_CONNECTION_REFUSED)
		_set_errno(ECONNREFUSED);
	else if (ecode == ERROR_DEVICE_NOT_CONNECTED)
		_set_errno(ENXIO);
	else if (ecode == ERROR_REQUEST_ABORTED)
		_set_errno(EAGAIN);
	else
		_set_errno(ecode);
#endif
	return errno;
}

#endif
