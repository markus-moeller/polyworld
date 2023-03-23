/* callback.c -- Enhanced Dynamic Library for MS-Windows
 * Define callbacks
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
/*** CHECK VERSION CALLBACK FUNCTION                      ***/
/************************************************************/
/************************************************************/

/* WARNING: we cannot use static on these since otherwise they
 * would not be visible in the other modules
 */
#ifdef EDLL_VERSION_CHECK
edll_check_version	g_edll_check_version_func = 0;
#endif




void edll_callback_register(
#ifdef EDLL_VERSION_CHECK
		edll_check_version check_version_func
#endif
	)
{
#ifdef EDLL_VERSION_CHECK
	g_edll_check_version_func = check_version_func;
#endif
}





/* vim: ts=8
 */
