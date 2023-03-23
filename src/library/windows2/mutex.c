/* mutex.c -- Enhanced Dynamic Library for MS-Windows
 * Multi-thread support
 *
 * Copyright (c) 2005-2006  Alexis Wilke <alexis@m2osw.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * As a special exception to the GNU Lesser General Public License,
 * if you distribute this file as part of a program or library that
 * is built using GNU libtool, you may include it under the same
 * distribution terms that you use for the rest of that program.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA
 *
 * I hereby authorize anyone to use this library in closed source
 * software. However, if you make any modification to the library,
 * you must make these changes available to everyone (preferably
 * sent to me, but this is not a requirement.)
 */



/*
 * This file is the implementation of the dynamic linking using the
 * libbfd.a library.
 */
#include	"edll_internal.h"



/************************************************************/
/************************************************************/
/*** MUTEX HANDLING                                       ***/
/************************************************************/
/************************************************************/

static edll_mutex_lock		g_mutex_lock;
static edll_mutex_unlock	g_mutex_unlock;
static edll_mutex_seterror	g_mutex_seterror;
static edll_mutex_geterror	g_mutex_geterror;
static edll_ptr			g_mutex_userdata;


void edll_mutex_register(edll_mutex_lock lock_func,
			edll_mutex_unlock unlock_func,
			edll_mutex_seterror seterror_func,
			edll_mutex_geterror geterror_func,
			edll_ptr userdata)
{
	assert(g_mutex_lock == 0 && g_mutex_unlock == 0);

	g_mutex_lock     = lock_func;
	g_mutex_unlock   = unlock_func;
	g_mutex_seterror = seterror_func;
	g_mutex_geterror = geterror_func;
	g_mutex_userdata = userdata;
}


void edll_lock(void)
{
	if(g_mutex_lock) {
		(*g_mutex_lock)(g_mutex_userdata);
	}
}



void edll_unlock(void)
{
	if(g_mutex_unlock) {
		(*g_mutex_unlock)(g_mutex_userdata);
	}
}






/************************************************************/
/************************************************************/
/*** ERROR HANDLING                                       ***/
/************************************************************/
/************************************************************/

static edll_errno_t		edll_errno /* = edll_err_none */;

void edll_seterror(edll_errno_t err)
{
	edll_errno = err;
	if(g_mutex_seterror) {
		g_mutex_seterror(g_mutex_userdata, err);
	}
}


edll_errno_t edll_geterror(void)
{
	if(g_mutex_geterror) {
		return g_mutex_geterror(g_mutex_userdata);
	}
	return edll_errno;
}


const char *edll_strerror(void)
{
	struct edll_errmsg { const char *msg; };

#define	EDLL_ERROR(name, msg)	msg,
	static const char *msg[edll_err_max] = { edll_errors };
#undef EDLL_ERROR

	edll_errno_t idx;

	idx = edll_geterror();

	if(idx < 0 || idx >= edll_err_max) {
		/* in case a user added its own error # */
		idx = edll_err_unknown;
	}

	return msg[idx];
}






/* vim: ts=8
 */
