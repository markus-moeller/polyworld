/* edll_internal.h -- Enhanced Dynamic Library for MS-Windows
 * The internal header file (so we can hide tons of stuff)
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
#ifndef __EDLL_INTERNAL_H__
#define __EDLL_INTERNAL_H__


/*
 * This file is the header of the implementation of the enhanced
 * dynamic linking library.
 */
#include	"edll.h"

#ifndef EDLL_HAVE_WINDOWS_H
#error "windows.h missing"
#endif
#ifndef EDLL_HAVE_STDIO_H
#error "stdio.h missing"
#endif
#ifndef EDLL_HAVE_STDINT_H
#error "stdint.h missing"
#endif
#ifndef EDLL_HAVE_UNISTD_H
#error "unistd.h missing"
#endif


#include	<windows.h>
#ifdef EDLL_HAVE_ASSERT_H
#include	<assert.h>
#else
#define	assert(x)
#endif
#include	<stdio.h>
#include	<stdint.h>
#include	<unistd.h>




#if EDLL_USE_BFD
/* Use the BFD and iberty libraries */
#ifndef EDLL_HAVE_BFD_H
#error "bfd.h missing"
#endif
#include	<bfd.h>

#define	EDLL_SYMBOL_STATIC		BSF_LOCAL

#define	edll_get_section_size(hsec)		bfd_get_section_size(hsec)
#define	edll_is_common_section(module, hsec)	bfd_is_com_section(hsec)
#define	edll_get_section_contents(module, hsec)	bfd_get_section_contents(module->hbfd, hsec, hsec->userdata, 0, bfd_get_section_size(hsec))


#else
/* No BFD dependency */
#include	"coff.h"

typedef struct asection_struct
{
	coff_uint32_t		size;
	edll_ptr		userdata;
	edll_ptr		vma;		/* for .exe files (i.e. self) */
	coff_uint8_t		alignment_power;
	struct coff_section	*coff_section;
} asection;

typedef struct asymbol_struct
{
	const char		*name;
	coff_int32_t		value;
	asection		*section;
	coff_int8_t		storage_class;		/* direct COFF symbol storage class */
} asymbol;

#define	edll_get_section_size(hsec)		((hsec)->size)
#define	edll_is_common_section(module, hsec)	((hsec) == &(module)->common_section)

extern	int			edll_get_section_contents(edll_module *module, asection *hsec);

#endif





/*** string.c ***/
extern	int			edll_strlen(const char *str);
extern	char *			edll_strdup(const char *str);
/*extern	int		edll_strcmp(char *src1, const char *src2);*/
extern	int			edll_strcasecmp(char *src1, const char *src2);
extern	int			edll_strcat(char *dst, const char *src, int maxlen);


/*** mutex.c ***/
extern	void			edll_lock(void);
extern	void			edll_unlock(void);


/*** search_path.c ***/
extern	int			edll_canonize_filename(char *name, int maxlen, const char **ext, const char *filename);
extern	int			edll_find_file(char *name, int maxlen, const char *filename);


/*** callback.c ***/
#ifdef EDLL_VERSION_CHECK
extern	edll_check_version	g_edll_check_version_func;
#endif



enum edll_section_t {
	edll_section_ex_data,		/* .text */
	edll_section_ro_data,		/* .data which has READ ONLY */
	edll_section_rw_data,		/* .data & .bss & COMMON */
	edll_section_dependencies,	/* .load */
#ifdef EDLL_VERSION_CHECK
	edll_section_version,		/* .version */
#endif
	edll_section_ctors,		/* .ctors */
	edll_section_dtors,		/* .dtors */
	edll_section_max
};

typedef struct edll_section_struct {
	unsigned int		count;		/* number of sections and ptrs */
	unsigned int		max;		/* max number of sections */
	asection		**sections;	/* all the sections found in the source plug-in */
	edll_ptr		*ptr;		/* an array of count pointers defining all the section startup pointer */
	unsigned int		alignment_power;/* largest alignment power of 2 */
	unsigned int		total_size;	/* the total size of the data buffer */
	edll_ptr		data;		/* the buffer of all the sections data */
	edll_ptr		real_buffer;	/* unaligned buffer of data */
} edll_section;


typedef void (*edll_constructor_t)(void);
typedef void (*edll_destructor_t)(void);

struct edll_module_struct {
	edll_module	*next;		/* linked list of all modules */
	edll_module	*previous;
	char		*filename;	/* path as specified to edll_open() */
#ifdef EDLL_VERSION_CHECK
	char		*version;	/* the version of this module */
#endif
	int		refcount;	/* when closing and reaching 0, delete */
	HMODULE		module;		/* LoadLibrary() handle */
#if EDLL_USE_BFD
	bfd		*hbfd;		/* the BFD handle */
	int		symbols_count;
	asymbol		**symbols;
#else
	FILE		*file;		/* a COFF file */
	long		file_size;	/* the total size of the file */
	int		sections_count;	/* number of sections */
	asection	*sections;	/* sections defined in the header */
	asection	common_section;	/* the common section */
	struct coff_section common_coff_section;
	coff_uint32_t	symbols_count;	/* number of valid symbols (array will usually be longer) */
	asymbol		*symbols;	/* cannonalized symbols of interest */
	coff_uint32_t	strings_size;	/* the size of the strings buffer (size excluded) */
	char		*strings;	/* strings (symbol names, long section names, etc.) */
#endif
	edll_errno_t	err;		/* to test errors when exiting from bfd calls which use callbacks */
	edll_section	sorted_sections[edll_section_max];
	int		dependencies_count;
	edll_module	**dependencies;
	char		constructed;	/* all the constructors have been called */
	char		level;		/* who opened this module */
	char		self;		/* loading oneself */
	unsigned long	common_size;	/* COMMON block size for this module */
};


typedef struct edll_symbol_struct {
	asymbol			*symbol;	/* the BFD symbol */
	edll_module		*module;	/* useful whenever a module is released and we need to remove its symbols */
} edll_symbol;


/*** module.c ***/
extern	edll_module		*g_edll_modules;
extern	int			g_edll_symbols_count;
extern	edll_symbol		*g_edll_symbols;




extern	edll_module *	edll_module_open(const char *filename, int level);
extern	int		edll_module_free(edll_module *module);

/*** load.c ***/
/* the function to load & construct a plug-in module */
extern	int		edll_load_object(edll_module *module, int level);
extern	edll_ptr	edll_find_any_symbol(edll_module **module_ptr, edll_module *skip, const char *name);
extern	int		edll_destruct(edll_module *module);
extern	void		edll_remove_symbols(edll_module *module);

/* specialized functions (BFD, COFF, etc.) */
extern	int		edll_resolve_section(edll_module *module, edll_ptr vma, asection *hsec);
extern	int		edll_find_symbol(const char *name, int *closest);
extern	void		edll_record_section(edll_module *module, int idx, asection *hsec);
extern	int		edll_open_file(edll_module *module);




/* vim: ts=8
 */
#endif		/* #ifndef __EDLL_INTERNAL_H__ */
