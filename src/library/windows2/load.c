/* load.c -- Enhanced Dynamic Library for MS-Windows
 * Load the actual module in memory
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
 * This file loads a COFF or some other image file.
 */
#include	"edll_internal.h"




/************************************************************/
/************************************************************/
/*** LOAD PLUG-IN MODULE (i.e. dynamic linking)           ***/
/************************************************************/
/************************************************************/



int			g_edll_symbols_count;
edll_symbol		*g_edll_symbols;







#ifdef EDLL_VERSION_CHECK
static char *			g_self_version;

void edll_set_self_version(const char *version)
{
	/* get the user value only if not yet defined */
	if(g_self_version == NULL) {
		g_self_version = edll_strdup(version);
	}
}
#endif







void edll_record_section(edll_module *module, int idx, asection *hsec)
{
	edll_section		*sec;

/** make sure we have a large enough array **/
	sec = module->sorted_sections + idx;
	if(sec->count >= sec->max) {
		sec->sections = edll_realloc(sec->sections,
				(sec->max + 16) * sizeof(asection *), -1);
		if(sec->sections == 0) {
			module->err = edll_err_out_of_memory;
			sec->max = 0;
			sec->count = 0;
			return;
		}
		sec->max += 16;
	}

/** save the section in our array **/
	sec->sections[sec->count] = hsec;
	sec->count++;
	if(sec->alignment_power < hsec->alignment_power) {
		sec->alignment_power = hsec->alignment_power;
	}

#if 0
#if EDLL_USE_BFD
printf("Added section [%s] at %d (total size: %d, alignment power: %d)\n",
	hsec->name, idx, module->sorted_sections[idx].total_size, module->sorted_sections[idx].alignment_power);
#else
printf("Added section [%s] at %d (total size: %d, alignment power: %d)\n",
	hsec->coff_section->f_name,
	idx,
	module->sorted_sections[idx].total_size,
	module->sorted_sections[idx].alignment_power);
#endif
#endif
}






static int edll_load_sections(edll_module *module)
{
	asection	*hsec;
	edll_section	*sec;
	int		i, j, max;
	long		align, size, sec_size;
	edll_ptr	ptr;

/* load the content of the sections we just saved in our array (including .load) */
	for(i = 0; i < edll_section_max; ++i) {
		sec = module->sorted_sections + i;
		max = sec->count;
		if(max > 0) {	/* any sections here? */

/* an array of pointers for each section start address in this block */
			sec->ptr = edll_alloc(max * sizeof(edll_ptr), 0);

/* for self, we just need to get the addresses as they are defined in the input module */
			if(module->self) {
				/*
				 * Note that sec->data & sec->real_buffer remain undefined.
				 * Also we ignore any constructor, destructor and dependency
				 * sections (there shouldn't be any, but just in case...)
				 */
				switch(i) {
				case edll_section_ex_data:
				case edll_section_ro_data:
				case edll_section_rw_data:
#if 0
printf("Section %d is at %08X for [%s]\n", i, (int) sec->sections[0]->vma, module->filename);
#endif
					for(j = 0; j < max; ++j) {
						hsec = sec->sections[j];
						sec->ptr[j] = hsec->userdata = (edll_ptr) hsec->vma;
					}
					break;

#ifdef EDLL_VERSION_CHECK
				case edll_section_version:
					hsec = sec->sections[0];
					sec->data = (edll_ptr) hsec->vma;
					break;
#endif

				}
			}
			else {
/* compute the size we need to allocate for these sections content */
				align = (1 << sec->alignment_power) - 1;
				size = 0;
				j = max;
				while(j > 0) {
					j--;
					hsec = sec->sections[j];
					if(edll_is_common_section(module, hsec)) {
						sec_size = module->common_size;
					}
					else {
						sec_size = edll_get_section_size(hsec);
					}
#if 0
printf("[LOAD %d,%d] Size %ld, %ld, %ld\n", i, j, sec_size, edll_get_section_size(hsec), edll_get_section_rawsize(hsec));
#endif
					size += (sec_size + align) & ~align;
				}

/*
 * Allocate a buffer large enough so we can make sure the alignment is proper;
 * we call that the real_buffer since that's the one we will be able to free
 * (vs. the data pointer which is properly aligned and calling free with it
 * will fail in many cases) -- note that the VirtualAlloc() function usually
 * returns paged aligned data and thus it rarely would be misaligned!
 */
/* the protection is changed once we loaded the data and relocated the sections */
				sec->real_buffer = VirtualAlloc(0, size + align, MEM_COMMIT, PAGE_READWRITE);
				sec->data = (edll_ptr) (((intptr_t) sec->real_buffer + align) & ~align);
				sec->total_size = size;

#if 0
printf("Section %d is at %8p for [%s]\n", i, sec->data, module->filename);
#endif

/* compute the start pointer of each section and load them in memory */
				ptr = sec->data;
				for(j = 0; j < max; ++j) {
					hsec = sec->sections[j];
					hsec->userdata = ptr;
					sec->ptr[j] = ptr;
					if(edll_is_common_section(module, hsec)) {
						sec_size = module->common_size;
						/* this is like BSS, clear it */
						memset(ptr, 0, sec_size);
					}
					else {
						sec_size = edll_get_section_size(hsec);
						if(sec_size > 0) {
							if(!edll_get_section_contents(module, hsec)) {
								edll_seterror(edll_err_io_error);
								return -1;
							}
						}
					}
#if 0
printf("[START] Size %p (%ld, %ld, %ld)\n", ptr, sec_size, edll_get_section_size(hsec), edll_get_section_rawsize(hsec));
#endif

					/* compute the next pointer, making sure it stays aligned */
					ptr = (edll_ptr) (((intptr_t) ptr + sec_size + align) & ~align);
				}
			}
		}
	}

	return 0;
}



#ifdef EDLL_VERSION_CHECK
const char *edll_module_version(edll_module *module)
{
	if(module == 0) {
		return g_self_version;
	}

	return module->version;
}



static int edll_get_version(edll_module *module)
{
	edll_section	*sec;
	edll_module	*self;
	int		r;

/** any dependencies? **/
	sec = module->sorted_sections + edll_section_version;
	if(sec->data == 0) {
		/* no version here */
		return 0;
	}

/** get a duplicate of the string version **/
	module->version = edll_strdup(sec->data);

#if 0
printf("Version of [%s] is [%s]\n", module->filename, module->version);
#endif

/** we need to know the self module version too **/
	if(module->self) {
		/* NOTE: the .exe file can keep the .version section
		 *	 only if you fix the ldscripts/i386pe.x
		 */
		/* we are self! */
		if(g_self_version == 0) {
			g_self_version = edll_strdup(module->version);
		}
	}
	else {
		/* if we don't yet know the self version, load it */
		if(g_self_version == 0) {
			self = edll_module_open(0, 1);
			if(self == 0) {
				return -1;
			}
			if(g_self_version == 0) {
				/* no version section in the main module */
				return 0;
			}
		}
		if(g_edll_check_version_func != 0) {
			/* call the user check version function */
			r = (*g_edll_check_version_func)(
					g_self_version, module->version);
		}
		else {
			r = strcmp(g_self_version, module->version) == 0;
		}
		if(!r) {
			edll_seterror(edll_err_version_mismatch);
			return -1;
		}
	}

	return 0;
}
#endif







enum {
	EDLL_SYM_EQUAL,			/* equal to the character */
	EDLL_SYM_IMP_EQUAL,		/* without the __imp__ it is equal */
	EDLL_SYM_UEQUAL,		/* equal when one underscore is removed in one of the names */
	EDLL_SYM_DIFFER			/* name1 != name2 */
};



/* use this function to decided which symbol to link to */
static int edll_symbol_cmp_name(const char *name1, const char *name2)
{
	int		result;

	result = EDLL_SYM_EQUAL;

	/* the following compares
	 * 	__imp__ in both name1 & name2 -> equal okay
	 * 	__imp__ in only one of name1 or name2 -> "imp equal"
	 *
	 * if neither name1 nor name2 starts with __imp__ we 1st
	 * skip all the common '_' then one more if *name1 != *name2
	 * and one of name1 or name2 starts with an underscore, we
	 * remove that underscore; if the name is then equal
	 * it will be said to be "u equal"
	 */
	if(name1[0] == '_'
	&& name1[1] == '_'
	&& name1[2] == 'i'
	&& name1[3] == 'm'
	&& name1[4] == 'p'
	&& name1[5] == '_'
	&& name1[6] == '_') {
		if(name2[0] == '_'
		&& name2[1] == '_'
		&& name2[2] == 'i'
		&& name2[3] == 'm'
		&& name2[4] == 'p'
		&& name2[5] == '_'
		&& name2[6] == '_') {
			name1 += 7;
			name2 += 7;
		}
		else {
			name1 += 7;
			result = EDLL_SYM_IMP_EQUAL;
		}
	}
	else if(name2[0] == '_'
	&& name2[1] == '_'
	&& name2[2] == 'i'
	&& name2[3] == 'm'
	&& name2[4] == 'p'
	&& name2[5] == '_'
	&& name2[6] == '_') {
		name2 += 7;
		result = EDLL_SYM_IMP_EQUAL;
	}

	/* skip all the common '_' */
	while(*name1 == '_' && *name2 == '_') {
		name1++;
		name2++;
	}

#if 1
/* we skip up to one more '_' in either name (weird if you ask me!) */
	if(*name1 != *name2) {
		if(*name2 == '_') {
			name2++;
			result = EDLL_SYM_UEQUAL;
		}
		else if(*name1 == '_') {
			name1++;
			result = EDLL_SYM_UEQUAL;
		}
	}
#else
/* this does not seem required and it could be really wrong */
	while(*name1 == '_') {
		name1++;
	}
	while(*name2 == '_') {
		name2++;
	}
#endif

	for(;;) {
		if(*name1 != *name2) {
			return EDLL_SYM_DIFFER;
		}
		if(*name1 == '\0') {
			return result;
		}
		name1++;
		name2++;
	}
	/*NOTREACHED*/
}



/* use this function to sort the names in our table of symbols */
static int edll_symbol_cmp_canonilized_name(const char *name1, const char *name2)
{
	/* ignore the __imp__ */
	if(name1[0] == '_'
	&& name1[1] == '_'
	&& name1[2] == 'i'
	&& name1[3] == 'm'
	&& name1[4] == 'p'
	&& name1[5] == '_'
	&& name1[6] == '_') {
		name1 += 7;
	}
	if(name2[0] == '_'
	&& name2[1] == '_'
	&& name2[2] == 'i'
	&& name2[3] == 'm'
	&& name2[4] == 'p'
	&& name2[5] == '_'
	&& name2[6] == '_') {
		name2 += 7;
	}

	/* ignore any number of underscores (_) */
	while(*name1 == '_') {
		name1++;
	}
	while(*name2 == '_') {
		name2++;
	}

	/* compare the rest of the names */
	return strcmp(name1, name2);
}




int edll_find_symbol(const char *name, int *closest)
{
	int		r, i, j, p;

	j = g_edll_symbols_count;
	if(j < 4) {
		/* binary searches don't work too well with a
		 * too small number of items
		 */
		for(p = 0; p < j; ++p) {
			r = edll_symbol_cmp_canonilized_name(g_edll_symbols[p].symbol->name, name);
			if(r == 0) {
				return p;
			}
			if(r < 0) {
				break;
			}
		}
	}
	else {
		i = 0;
		do {
			p = (j - i) / 2 + i;
			r = edll_symbol_cmp_canonilized_name(g_edll_symbols[p].symbol->name, name);
			if(r > 0) {
				/* change p in case we're in the last iteration */
				p = i = p + 1;
			}
			else if(r < 0) {
				j = p;
			}
			else {
				return p;
			}
		} while(i < j);
	}

	/* closest -- i.e. where we should insert a new symbol */
	if(closest) {
		*closest = p;
	}

	return -1;
}



static edll_ptr edll_search_dll(edll_module *module, const char *name)
{
	edll_ptr		result;

	result = GetProcAddress(module->module, name);
	if(result != 0) {
		return result;
	}
	if(name[0] == '_'
	&& name[1] == '_'
	&& name[2] == 'i'
	&& name[3] == 'm'
	&& name[4] == 'p'
	&& name[5] == '_'
	&& name[6] == '_') {
		name += 7;
		result = GetProcAddress(module->module, name);
		if(result != 0) {
			return result;
		}
	}
	if(name[0] == '_') {
		name++;
		result = GetProcAddress(module->module, name);
		if(result != 0) {
			return result;
		}
	}

	return 0;
}


static edll_ptr edll_search_dll_symbol(edll_module *module, const char *name)
{
	edll_ptr	result;
	char		n[1024];
	const char	*at;
	int		l;

	if(module->module) {
		result = edll_search_dll(module, name);
		if(result != 0) {
			return result;
		}
		at = strrchr(name, '@');
		l = at - name;
		if(at && l < sizeof(n)) {
			/* TODO: we may want to support a dynamic allocation
			 * but who has over 1024 chars symbols?!
			 */
			/* try again without the @<value> */
			memcpy(n, name, l);
			n[l] = '\0';
			return edll_search_dll(module, n);
		}
	}

	return 0;
}


edll_ptr edll_find_any_symbol(edll_module **module_ptr, edll_module *skip, const char *name)
{
	asymbol		*symbol, *best;
	edll_module	*module, *this_module;
	edll_ptr	result;
	int		r, pos, p, best_r;

/* first search the DLLs (this seems to make sense to me...) */
	if(*module_ptr == 0) {
		module = g_edll_modules;
		while(module != 0) {
			result = edll_search_dll_symbol(module, name);
			if(result != 0) {
				*module_ptr = module;
				return result;
			}
			module = module->next;
		}
		this_module = 0;
	}
	else {
		this_module = *module_ptr;
		if(this_module->module != 0) {
			/*
			 * The module in which the user is searching is a DLL.
			 * Search that DLL and return the result at once.
			 */
			return edll_search_dll_symbol(*module_ptr, name);
		}
	}

/* now search in all the dynamically linked modules (this is done at once) */
	pos = edll_find_symbol(name, 0);
	if(pos < 0) {
		return 0;
	}

/* search for the best match (first this position and before, then after) */
/* frankly I'd like to have a much better way: 1 symbol which matches perfectly... maybe in a later version */
	best = 0;
	best_r = EDLL_SYM_DIFFER;
	p = pos;
	while(p >= 0) {
		module = g_edll_symbols[p].module;
		symbol = g_edll_symbols[p].symbol;
		if(module != skip
		&& symbol->section->userdata != 0 /* just in case, should never happen */
		&& (this_module == 0 || module == this_module)) {
			r = edll_symbol_cmp_name(symbol->name, name);
			if(r == EDLL_SYM_EQUAL) {
				return (edll_ptr) ((unsigned char *) symbol->section->userdata + symbol->value);
			}
			if(r == EDLL_SYM_DIFFER) {
				break;
			}
			if(r == EDLL_SYM_UEQUAL && best_r == EDLL_SYM_DIFFER) {
				best_r = r;
				best = symbol;
			}
			else if(r == EDLL_SYM_IMP_EQUAL && best_r != r /* EDLL_SYM_IMP_EQUAL */) {
				best_r = r;
				best = symbol;
			}
		}
		--p;
	}
	p = pos + 1;
	while(p < g_edll_symbols_count) {
		module = g_edll_symbols[p].module;
		if(module != skip && (this_module == 0 || module == this_module)) {
			symbol = g_edll_symbols[p].symbol;
			r = edll_symbol_cmp_name(symbol->name, name);
			if(r == EDLL_SYM_EQUAL) {
				return (edll_ptr) ((unsigned char *) symbol->section->userdata + symbol->value);
			}
			if(r == EDLL_SYM_DIFFER) {
				break;
			}
			if(r == EDLL_SYM_UEQUAL && best_r == EDLL_SYM_DIFFER) {
				best_r = r;
				best = symbol;
			}
			else if(r == EDLL_SYM_IMP_EQUAL && best_r != r /* EDLL_SYM_IMP_EQUAL */) {
				best_r = r;
				best = symbol;
			}
		}
		++p;
	}

	if(best != 0) {
		return (edll_ptr) ((unsigned char *) best->section->userdata + best->value);
	}

	return 0;
}




void edll_remove_symbols(edll_module *module)
{
	int		idx;

	idx = g_edll_symbols_count;
	while(idx > 0) {
		--idx;
		if(g_edll_symbols[idx].module == module) {
			g_edll_symbols_count--;
			if(idx < g_edll_symbols_count) {
				memmove(g_edll_symbols + idx, g_edll_symbols + idx + 1, (g_edll_symbols_count - idx) * sizeof(edll_symbol));
			}
		}
	}
	/*
	 * We leave the buffer as is, we could realloc() to reduce the size
	 * but that will be done next time the user loads a plug-in
	 */
}





struct dep_idname {
	int		id;
	const char *	name;
};


int cmp_idnames(const void *a, const void *b)
{
	struct dep_idname *da;
	struct dep_idname *db;

	da = (struct dep_idname *) a;
	db = (struct dep_idname *) b;

	if(da->id < db->id) {
		return -1;
	}
	if(da->id > db->id) {
		return 1;
	}

	return 0;
}



static int edll_get_dependencies(edll_module *module)
{
	edll_section		*sec;
	int			idx, count, id;
	const char		*name, *end, *n;
	struct dep_idname	*idnames;

/** any dependencies? **/
	sec = module->sorted_sections + edll_section_dependencies;
	if(sec->data == 0) {
		/* nothing to load */
		return 0;
	}

/** count the number of dependencies **/
	name = sec->data;
	end = name + sec->total_size;
	count = 0;
	while(name < end) {
		while(name < end && *name != '\0') {
			name++;
		}
		while(name < end && *name == '\0') {
			name++;
		}
		count++;
	}
	if(count == 0) {
		/* TODO: should this case be considered an error? */
		return 0;
	}

/** allocate an array of modules which we need to release whenever this module is released **/
	/* we do not need to clear because we also have a counter (dependencies_count) */
	module->dependencies = edll_alloc(count * sizeof(edll_module *), 0);
	if(module->dependencies == 0) {
		return -1;
	}

/** sort the dependencies before to load them **/
	idnames = edll_alloc(count * sizeof(struct dep_idname), 0);
	if(idnames == 0) {
		return -1;
	}

	idx = 0;
	name = sec->data;
	for(idx = 0, name = sec->data; idx < count && name < end; idx++) {
		idnames[idx].name = name;
		id = 0;
		while(*name >= '0' && *name <= '9') {
			id = id * 10 + *name - '0';
			name++;
		}
		idnames[idx].id = id;
		if(name < end && *name == '>') {
			/* found the line #, this is a new module */
			name++;
			idnames[idx].name = name;
		}
		while(name < end && *name != '\0') {
			name++;
		}
		while(name < end && *name == '\0') {
			name++;
		}
	} ;

	qsort(idnames, count, sizeof(struct dep_idname), cmp_idnames);

/** load these sorted dependencies **/
	for(idx = 0; idx < count; idx++) {
#if 0
printf("Loading [%s] -> ...", idnames[idx].name);
#endif
		/* we use a level of 1 so constructors don't get executed */
		n = idnames[idx].name;
		if(strcmp(n, "*self*") == 0) {
			n = 0;
		}
		module->dependencies[module->dependencies_count] = edll_module_open(n, 1);
#if 0
printf("\b\b\b%p\n", module->dependencies[module->dependencies_count]);
#endif
		if(module->dependencies[module->dependencies_count] == 0) {
#if 0
printf("  ->>> loading of [%s] failed\n", idnames[idx].name);
#endif
			edll_free(idnames);
			return -1;
		}
		module->dependencies_count++;
	}

#if 0
printf("Load done! Freeing %8p\n", sec->data);
#endif

	edll_free(idnames);

	/* we don't need to keep this section data around */
	if(sec->real_buffer != 0) {
		VirtualFree(
			sec->real_buffer,
			sec->total_size + (1 << sec->alignment_power) - 1,
			MEM_RELEASE);
	}
	sec->data = 0;
	sec->total_size = 0;

	return 0;
}






static int edll_resolve_all(edll_module *module)
{
	int		r, i, j, err;
	edll_section	*sec;

	/* Self is already resolved (At link time!) */
	if(module->self) {
		return 0;
	}

	/*
	 * Now we can (try to) resolve our symbols; this means going
	 * through the relocs and looking up the corresponding symbols
	 */


	/*
	 * NOTE: we try to resolve all the symbols so we can have a complete list
	 *	 of all the symbols which can't be resolved instead of just the
	 *	 first symbol which can't be resolved. (see the 'err' variable)
	 */
	err = 0;
	for(i = 0; i < edll_section_max; ++i) {
		sec = module->sorted_sections + i;

		/* we skip empty sections, they can't have relocations! */
		if(sec->total_size > 0) {
			/* go over all the BFD sections */
			for(j = 0; j < sec->count; ++j) {
				r = edll_resolve_section(module, sec->ptr[j], sec->sections[j]);
				if(r != 0) {
					err = -1;
				}
			}
		}
	}

	return err;
}


static int edll_protect(edll_module *module)
{
	static const DWORD protections[edll_section_max] = {
		PAGE_EXECUTE_READ,		/* .text */
		PAGE_READONLY,			/* .data marked READ ONLY */
		PAGE_READWRITE,			/* .data & .bss & COMMON */
		PAGE_NOACCESS,			/* .load (n.a. here) */
#ifdef EDLL_VERSION_CHECK
		PAGE_READONLY,			/* .version */
#endif
		PAGE_READONLY,			/* .ctors */
		PAGE_READONLY			/* .dtors */
	};

	int		i;
	edll_section	*sec;
	DWORD		old_protection;

	/* protections are defined by the system for self */
	if(module->self) {
		return 0;
	}

	for(i = 0; i < edll_section_max; ++i) {
		sec = module->sorted_sections + i;
		if(sec->data != 0 && sec->total_size > 0) {

#if 0
printf("%d.  %p + %d %08lX <- ?\n",
	i,
	sec->real_buffer,
	sec->total_size,
	protections[i]
	);
#endif

			if(!VirtualProtect(
					sec->real_buffer,
					sec->total_size + (1 << sec->alignment_power) - 1,
					protections[i],
					&old_protection)) {
				edll_seterror(edll_err_bad_protection);
				return -1;
			}
		}
	}

	return 0;
}



static int edll_construct(edll_module *module)
{
	int			i, r, max;
	edll_section		*sec;
	edll_constructor_t	*ctors;

/* NOTE: we could skip this in case of self yet that will be automatic */

/* mark this module constructors as being called */
	if(module->constructed != 0) {
		return 0;
	}
	module->constructed = 1;

/* call all the dependency constructors first */
	for(i = 0; i < module->dependencies_count; ++i) {
		r = edll_construct(module->dependencies[i]);
		if(r != 0) {
			module->constructed = 0;
			return -1;
		}
	}

/* now call all the constructors of this module */
	sec = module->sorted_sections + edll_section_ctors;
	max = sec->total_size;
	ctors = (edll_constructor_t *) sec->data;
	for(i = 0; i < max; i += 4, ++ctors) {

#if 0
		printf("Call constructors at %p\n", *ctors);

unsigned char *p = (unsigned char *) ctors[0];
int i;
for(i = 0; i < 48; i++) {
if((i & 7) == 0) {
printf("%08lX:", (long) (p + i));
}
printf(" %02X", p[i]);
if((i & 7) == 7) {
printf("\n");
}
}
#endif

		(*ctors)();
	}

	return 0;
}


int edll_destruct(edll_module *module)
{
	int			i, max;
	edll_section		*sec;
	edll_destructor_t	*dtors;

/* NOTE: we could skip this in case of self yet that will be automatic */

/* mark this module constructors as being called */
	if(module->constructed == 0) {
		return 0;
	}
	module->constructed = 0;

/* call all the destructors of this module */
	sec = module->sorted_sections + edll_section_dtors;
	max = sec->total_size;
	dtors = (edll_destructor_t *) sec->data;
	for(i = 0; i < max; i += 4, ++dtors) {
		(*dtors)();
	}

	return 0;
}







/*
 * This function does this:
 *
 * Go through all the sections and keep a list of those which interest us
 * which are as follow:
 *
 *	1. text, (r)data, bss, common -- code & data from the actual software
 *	2. ctors, dtors -- an array of function pointers for C++
 *	                   constructors and destructors
 *	3. symbols -- used for the link process [not actually a section]
 *	4. reloc -- used for the link process [not actually a section]
 *	5. load -- modules & libraries necessary to load this one
 *	6. version -- this module version
 *
 * Then allocate buffers for the sections we are to load (text, data,
 * and bss). The buffer address is viewed as the relocation address for
 * the given sections. Note that the arrays of constructors and
 * destructors are attached to the text section (read only).
 *
 * Load the other modules. These may request symbols from us which is
 * why we needed to define the buffer addresses beforehand.
 *
 * Finally, link ourself which means resolving the symbols as we are
 * relocating the code. If a symbol can't be resolved, we break
 * (i.e. which means we can't load this module).
 */
#define SHOW_STEPS	0
int edll_load_object(edll_module *module, int level)
{
	int		r;

/* open the module */
#if SHOW_STEPS
printf("EDLL: edll_open_file()\n");
fflush(stdout);
#endif
	r = edll_open_file(module);
	if(r != 0) {
		return -1;
	}


/* load the section data in memory now that we can align them properly */
#if SHOW_STEPS
printf("EDLL: edll_load_sections()\n");
fflush(stdout);
#endif
	r = edll_load_sections(module);
	if(r != 0) {
		return -1;
	}

/* now we're ready to load other dependencies and resolve all their relocs */
#if SHOW_STEPS
printf("EDLL: edll_get_dependencies()\n");
fflush(stdout);
#endif
	r = edll_get_dependencies(module);
	if(r != 0) {
		return -1;
	}

#ifdef EDLL_VERSION_CHECK
/* save the version of this module */
#if SHOW_STEPS
printf("EDLL: edll_get_version()\n");
fflush(stdout);
#endif
	r = edll_get_version(module);
	if(r != 0) {
		return -1;
	}
#endif

/* with all the dependencies symbols available, resolve this module symbols */
#if SHOW_STEPS
printf("EDLL: edll_resolve_all()\n");
fflush(stdout);
#endif
	r = edll_resolve_all(module);
	if(r != 0) {
		return -1;
	}

/* setup the memory protection flags */
#if SHOW_STEPS
printf("EDLL: edll_protect()\n");
fflush(stdout);
#endif
	r = edll_protect(module);
	if(r != 0) {
		return -1;
	}

/* call the C++ constructors in all the modules we just loaded */
	if(level == 0) {
#if SHOW_STEPS
printf("EDLL: edll_construct()\n");
fflush(stdout);
#endif
		r = edll_construct(module);
		if(r != 0) {
			return -1;
		}
	}

	return 0;
}







/* vim: ts=8
 */
