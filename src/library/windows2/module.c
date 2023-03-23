/* module.c -- Enhanced Dynamic Library for MS-Windows
 * Low level module handling
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
/*** LOW LEVEL MODULE FUNCTIONS                           ***/
/************************************************************/
/************************************************************/


edll_module		*g_edll_modules = 0;



int edll_module_free(edll_module *module)
{
	edll_section	*sec;
	int		r, i;

	r = 0;

	if(module) {
		module->refcount--;
		if(module->refcount <= 0) {
			edll_destruct(module);

			while(module->dependencies_count > 0) {
				--module->dependencies_count;
				/* at this time we ignore errors from dependencies */
				edll_module_free(module->dependencies[module->dependencies_count]);
			}

			edll_remove_symbols(module);

			if(!module->self) {
				for(i = 0; i < edll_section_max; ++i) {
					sec = module->sorted_sections + i;
					if(sec->real_buffer != 0) {
						VirtualFree(
							sec->real_buffer,
							sec->total_size + (1 << sec->alignment_power) - 1,
							MEM_RELEASE);
					}
					edll_free(sec->sections);
					edll_free(sec->ptr);
				}
			}

			if(module->module != 0) {
				FreeLibrary(module->module);
			}
#if EDLL_USE_BFD
			if(module->hbfd) {
				bfd_close(module->hbfd);
			}
#else
			if(module->file != 0) {
				fclose(module->file);
			}
			edll_free(module->sections);
			edll_free(module->strings);
#endif
			edll_free(module->symbols);
			edll_free(module->filename);
			edll_free(module->dependencies);
#ifdef EDLL_VERSION_CHECK
			edll_free(module->version);
#endif

			/* unlink module */
			if(module->previous) {
				g_edll_modules = module->previous;
				module->previous->next = module->next;
			}
			else {
				g_edll_modules = module->next;
			}
			if(module->next) {
				module->next->previous = module->previous;
			}

			edll_free(module);
		}
		else {
			edll_seterror(edll_err_still_referenced);
			r = -1;
		}
	}

	return 0;
}


static edll_module *edll_module_alloc(const char *filename)
{
	edll_module	*module;

	module = edll_alloc(sizeof(edll_module), 1);

	if(module) {
		module->refcount = 1;

#ifndef EDLL_USE_BFD
		module->common_coff_section.f_flags = COFF_SECTION_COMMON_DATA | COFF_SECTION_READ | COFF_SECTION_WRITE | COFF_SECTION_ALIGN_4BYTES;
		strcpy(module->common_coff_section.f_name, "*COM*");
		module->common_section.coff_section = &module->common_coff_section;
#endif

		if(filename != 0) {
			module->filename = edll_strdup(filename);
			if(!module->filename) {
				edll_module_free(module);
				return 0;
			}
		}

		/* link module */
		/* edll_lock(); -- already locked */
		if(g_edll_modules) {
			module->next = g_edll_modules;
			module->previous = g_edll_modules->previous;
			if(module->previous) {
				module->previous->next = module;
			}
			g_edll_modules->previous = module;
		}
		g_edll_modules = module;
		/* edll_unlock(); -- didn't lock here */
	}

	return module;
}


static edll_module *edll_module_find(const char *filename)
{
	edll_module	*module;

	module = g_edll_modules;

	/* TODO: instead of a linked list, we could have an array and
	 * then sort that array in a binary way so we can find objects
	 * much faster
	 */

	/* search first module */
	if(module != NULL) {
		while(module->previous != NULL) {
			module = module->previous;
		}
	}

	/* check out each module */
	while(module != NULL) {
		if(edll_strcasecmp(module->filename, filename) == 0) {
			break;
		}
		module = module->next;
	}

	return module;
}






edll_module *edll_module_open(const char *filename, int level)
{
	int		r;
	char		name[MAX_PATH * 2], fullname[MAX_PATH * 2];
	const char	*ext;
	edll_module	*module;

	if(!filename || !*filename) {
		/* to load ourself like other plugins (so we can link
		 * back; which is very important for C++ and multithreaded
		 * programs); we need to get the module filename
		 */
		ext = 0;	/* ignore extension */
		filename = 0;
		fullname[0] = '\0';
		GetModuleFileName(NULL, fullname, sizeof(fullname));
	}
	else {
		/* TODO:
		 * We don't support long path names at this time.
		 * We could check the error (edll_err_filename_overflow) to know
		 * if the name buffer needs to be enlarged and the canonize
		 * function called again.
		 */
		r = edll_canonize_filename(name, sizeof(name), &ext, filename);
		if(r != 0) {
			return 0;
		}

		edll_find_file(fullname, sizeof(fullname), name);
	}

	module = edll_module_find(fullname);
	if(module) {
		module->refcount++;
		return module;
	}

	module = edll_module_alloc(fullname);
	if(!module) {
		return 0;
	}

	module->level = level;
	module->self = filename == 0;

	r = 0;

	if(ext
	&& (ext[0] == 'd' || ext[0] == 'D')
	&& (ext[1] == 'l' || ext[1] == 'L')
	&& (ext[2] == 'l' || ext[2] == 'L')) {
		module->module = LoadLibrary(fullname);
		if(!module->module) {
			edll_seterror(edll_err_cant_load_dll);
			r = -1;
		}
	}
	else {
		/* this is our stuff! */
		r = edll_load_object(module, level);
	}

	if(r != 0) {
		edll_module_free(module);
		module = 0;
	}

	return module;
}








/* vim: ts=8
 */
