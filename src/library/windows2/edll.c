/* edll.c -- Enhanced Dynamic Library for MS-Windows
 * Initialization and destruction
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
/*** INITIALIZATION AND DEINTIALIZATION                   ***/
/************************************************************/
/************************************************************/


const char *edll_getversion(void)
{
	return EDLL_VERSION;
}



/* TODO: should we protect this for multi-thread usage? */
static char	g_initialized = 0;

int edll_init(void)
{
#if EDLL_USE_BFD
	/* initialize the BFD only once */
	if(g_initialized == 0) {
		bfd_init();
	}
#endif

	++g_initialized;

	return 0;
}


int edll_exit(void)
{
	int		r, err;
	edll_module	*module;

	if(g_initialized == 0) {
		edll_seterror(edll_err_not_initialized);
		return -1;
	}

	--g_initialized;

	err = 0;

	if(g_initialized == 0) {
		/* de-initialize */
		module = g_edll_modules;
		/* NOTE: the module in this loop test is there so in case
		 *	 there is a bug, the loop still stops at some point
		 */
		while(g_edll_modules && module) {
			module = g_edll_modules;
			do {
				if(module->level == 0) {
					r = edll_close(module);
					if(r != 0) {
						err = -1;
					}
					break;
				}
				module = module->next;
			} while(module);
		}
	}

	return err;
}







/************************************************************/
/************************************************************/
/*** OPEN/CLOSE/SYM FUNCTIONS                             ***/
/************************************************************/
/************************************************************/




edll_module *edll_open(const char *filename)
{
	edll_module	*module;

	assert(g_initialized);

	edll_lock();
	module = edll_module_open(filename, 0);
	edll_unlock();

	return module;
}


int edll_close(edll_module *module)
{
	int		r;

	assert(g_initialized);

	edll_lock();
	r = edll_module_free(module);
	edll_unlock();

	return r;
}




edll_ptr edll_sym(edll_module *module, const char *name)
{
	edll_ptr	ptr;

	assert(g_initialized);

	edll_lock();
	ptr = edll_find_any_symbol(&module, 0, name);
	edll_unlock();

	return ptr;
}


edll_ptr edll_msym(edll_module **module_ptr, const char *name)
{
	edll_ptr	ptr;
	edll_module	*module;

	assert(g_initialized);

	if(module_ptr == 0) {
		module_ptr = &module;
	}
	*module_ptr = 0;

	edll_lock();
	ptr = edll_find_any_symbol(module_ptr, 0, name);
	edll_unlock();

	return ptr;
}






/* vim: ts=8
 */
