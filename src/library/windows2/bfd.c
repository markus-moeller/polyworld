/* bfd.c -- Enhanced Dynamic Library for MS-Windows
 * BFD specific functions
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
/*** BFD LOADER                                           ***/
/************************************************************/
/************************************************************/




static int edll_record_common_section(edll_module *module)
{
	if(bfd_com_section_ptr == 0) {
		/* at this time this is impossible */
		return 0;
	}

	/* The common section may have symbols and their value
	 * define the true size of section.
	 */
	if(module->common_size == 0) {
		return 0;
	}

	edll_record_section(module, edll_section_rw_data, bfd_com_section_ptr);

	return 0;
}


static void edll_record_sections(bfd *hbfd, asection *hsec, PTR mod)
{
	int			idx;
	edll_module		*module = (edll_module *) mod;

#if 0
printf("[RECORD %s] Size %ld, %ld, %ld, %ld, %ld, f:%08lX\n",
		hsec->name, bfd_get_section_size(hsec),
		bfd_get_section_rawsize(hsec), (long)hsec->entsize,
		hsec->vma, hsec->lma, (long)hsec->flags);
#endif

/** skip empty sections **/
	if(bfd_get_section_size(hsec) == 0) {
		/* TODO: this could be a problem if this section is used in a
		 *	 relocation later. To be tested.
		 */
		return;
	}

/** determine the section type **/
	idx = -1;
	if(strcmp(hsec->name, ".load") == 0) {
		/*
		 * We found our special section which lists the necessary
		 * DLLs and other modules we need to load before we can
		 * proceed with the link process.
		 */
		idx = edll_section_dependencies;
	}
#ifdef EDLL_VERSION_CHECK
	else if(strcmp(hsec->name, ".version") == 0) {
		/* includes the module version */
		idx = edll_section_version;
	}
#endif
	else if(strcmp(hsec->name, ".ctors") == 0) {
		/* the ctors indicates the function(s) to call to construct the C++ static objects */
		idx = edll_section_ctors;
	}
	else if(strcmp(hsec->name, ".dtors") == 0) {
		/* the dtors indicates the function(s) to call to destruct the C++ static objects */
		idx = edll_section_dtors;
	}
	else if(hsec->flags & SEC_CODE) {
		/* we assume that .text sections are always read only */
		idx = edll_section_ex_data;
	}
	else if(hsec->flags & SEC_ALLOC) {
		/* .data, .rdata, .bss */
		if(hsec->flags & SEC_READONLY) {
			idx = edll_section_ro_data;
		}
		else {
			idx = edll_section_rw_data;
		}
	}

/** a section we want to keep around? **/
	if(idx < 0) {
		return;
	}

	edll_record_section(module, idx, hsec);
}









static int edll_get_symbols(edll_module *module)
{
	int		idx, size, closest, pos;
	long		offset;
	edll_symbol	*new_symbols;
	asymbol		*symbol;

/** first we get the symbols from the BFD file **/
	/* get the size in bytes of the symbols array */
	size = bfd_get_symtab_upper_bound(module->hbfd);
	if(size < 0) {
		edll_seterror(edll_err_bfd_error);
		return 0;
	}
	if(size == 0) {
		return 0;
	}

	/* allocate the symbol array */
	module->symbols = (asymbol **) edll_alloc(size, 0);
	if(module->symbols == 0) {
		return -1;
	}

	/* load the symbols and get a copy of the array */
	module->symbols_count = bfd_canonicalize_symtab(module->hbfd, module->symbols);
	if(module->symbols_count < 0) {
		edll_seterror(edll_err_bfd_error);
		return -1;
	}

/** now we add these symbols to our table **/
	/* enlarge the table */
	new_symbols = edll_realloc(g_edll_symbols, (g_edll_symbols_count + module->symbols_count) * sizeof(edll_symbol), -1);
	if(new_symbols == 0) {
		return -1;
	}
	g_edll_symbols = new_symbols;

	/* TODO: should we or not clear this value? */
	module->common_size = 0;

	/* add the new symbols to our table */
	for(idx = 0; idx < module->symbols_count; ++idx) {
		symbol = module->symbols[idx];
		if(bfd_is_und_section(symbol->section)) {
			/* symbols from the undefined section do not need to be searched later */
			continue;
		}

#if 0
printf("Adding symbol [%s] [0x%lX] [0x%08lX] in [%s]\n",
			module->symbols[idx]->name,
			(long)module->symbols[idx]->value,
			(long)module->symbols[idx]->flags,
			module->symbols[idx]->section->name
		);
#endif

		/*
		 * Add the size of all the symbols which are part of the COMMON section
		 * this total defines the total size of the COMMON section
		 */
		if(bfd_is_com_section(symbol->section)) {
			offset = module->common_size;
			module->common_size += symbol->value;
			symbol->value = offset;
		}

		pos = edll_find_symbol(symbol->name, &closest);
		if(pos < 0) {
			pos = closest;
		}
		memmove(g_edll_symbols + pos + 1, g_edll_symbols + pos, (g_edll_symbols_count - pos) * sizeof(edll_symbol));
		g_edll_symbols[pos].symbol = symbol;
		g_edll_symbols[pos].module = module;

		g_edll_symbols_count++;
	}

	return 0;
}






int edll_resolve_section(edll_module *module, edll_ptr vma, asection *hsec)
{
	int			i, size, count, err;
	arelent			**relocs, **r;
	asymbol			*symbol;
	edll_module		*ignore;
	edll_ptr		ptr;
#ifdef EDLL_BFD_PERFORM_RELOCATION
	bfd_reloc_status_type	result;
	asection		tsec, dsec;
#else
	int			value;
	unsigned char		*addr;
#endif

/** get the size of the table we need to allocate to access the relocation tables **/
	size = bfd_get_reloc_upper_bound(module->hbfd, hsec);
	if(size < 0) {
		/* this is either an I/O error or a memory error */
		edll_seterror(edll_err_bfd_error);
		return -1;
	}
	if(size == 0) {
		/* no relocs! */
		return 0;
	}

	/* size is in bytes, so we want to allocate bytes... */
	relocs = (arelent **) edll_alloc(size, 0);
	if(relocs == 0) {
		return -1;
	}

	count = bfd_canonicalize_reloc(module->hbfd, hsec, relocs, module->symbols);
	if(count < 0) {
		edll_free(relocs);
		/* this is either an I/O error or a memory error */
		edll_seterror(edll_err_bfd_error);
		return -1;
	}

	err = 0;

	r = relocs;
	for(i = 0; i < count; ++i, ++r) {
		symbol = *r[0]->sym_ptr_ptr;

		if((symbol->flags & BSF_LOCAL) != 0) {
			/* local symbols are... right here! */
			ptr = (edll_ptr) ((unsigned char *) symbol->section->userdata + symbol->value);
		}
		else {
			/* global symbols are searched in other modules, including DLLs */
			ignore = 0;
			ptr = edll_find_any_symbol(&ignore, module, symbol->name);
			if(ptr == 0) {
				ptr = edll_find_any_symbol(&ignore, 0, symbol->name);
				if(ptr == 0) {
printf("ERROR: cannot resolve symbol \"%s\" in section \"%s\" from module \"%s\".\n",
	bfd_asymbol_name(symbol), hsec->name, module->filename);

#if 0
if(strcmp(bfd_asymbol_name(symbol), "__ZN5myLib11g_copyrightE") == 0) {

	int pos = edll_find_symbol(symbol->name, 0);
printf("\n*** SPECIAL TEST *** (%d)\n", pos);
for(i = 0; i < g_edll_symbols_count; ++i) {
	printf("SYM: [%s]\n", g_edll_symbols[i].symbol->name);
}
printf("\n");
}
#endif

					err = -1;
				}
			}
#if 0
			else {
printf("%s resolved!\n", symbol->name);
			}
#endif
		}

#if 0
if(ptr) {
	printf("%08lX+%08lX %-7.7s %-7.7s %-8.8s %08lX %08lX %08lX %08X\n",
		(long) vma,
		r[0]->address + hsec->vma,
		symbol->section->name,
		r[0]->howto->name,
		symbol->name,
		(long) hsec->vma,
		(long) symbol->value,
		(long) ptr,
		symbol->flags);
}
#endif

#ifdef EDLL_BFD_PERFORM_RELOCATION
/*
 * This is an attempt in using the bfd_perform_relocation() function
 * which works once I clear the addend value in all cases.
 *
 * It looks to me that the addend has not only a reverse effect but
 * it looks like it is used to compensate something that we do not
 * compensate the other way around when computing our pointers.
 *
 * Look into:
 *
 *	bfd/reloc.c:
 *	bfd_perform_relocation()
 *
 *	bfd/coff-i386.c:
 *	coff_i386_reloc()
 *
 * The reloc.c is the generic relocation function which works in all
 * cases for any type of format. It is complicated to say the least
 * and it handles several special cases right there.
 *
 * The coff-i386.c handles the special cases of COFF such as the
 * PC relative position error, the doubling of the addend, etc.
 *
 * NOTE:
 * The bfd_perform_relocation() expects the value of a COMMON section
 * symbol to remain set to its size and the addend x 2 is used to
 * compensate... the x 2 is a craziness of the COFF format, not
 * something the BFD writers wanted.
 */
		memset(&tsec, 0, sizeof(tsec));
		tsec.vma = (long) ptr;
		if(!r[0]->howto->pc_relative) {
			tsec.vma -= symbol->value;
		}
		symbol->section->output_section = &tsec;
		symbol->section->output_offset = (long) 0;

		memset(&dsec, 0, sizeof(tsec));
		dsec.vma = (long) vma;
		hsec->output_section = &dsec;
		hsec->output_offset = (long) 0;

#if 0
		if(bfd_is_com_section(symbol->section)) {
			r[0]->addend = 0;
		}

if(r[0]->howto->pc_relative) {
printf("REL: ptr %p, vma %p + %lx before %lx -> %lx [%d], how %d (size) %lx (addend)\n",
			ptr, vma, r[0]->address,
			((long*)(vma + r[0]->address))[0],
			((long*)(vma + r[0]->address))[0]
				+ ((long)ptr - ((long)(vma + r[0]->address)) - 4),
			bfd_is_com_section(symbol->section),
			r[0]->howto->size, r[0]->addend);
}
else {
printf("ABS: ptr %p (%lx), vma %p + %lx before %lx -> %lx [%d], how %d (size) %lx (addend)\n",
			ptr, symbol->value, vma, r[0]->address,
			((long*)(vma + r[0]->address))[0],
			((long*)(vma + r[0]->address))[0] + (long)ptr,
			bfd_is_com_section(symbol->section),
			r[0]->howto->size, r[0]->addend);
}
#endif

		r[0]->addend = 0;
		result = bfd_perform_relocation(module->hbfd, r[0], vma, hsec, NULL, NULL);

#if 0
printf("          after %lx (%p)\n", ((long*)(vma + r[0]->address))[0],
			((long*)(vma + r[0]->address))[0] + vma + r[0]->address + 4);
#endif

		hsec->output_section = NULL;
		hsec->output_offset = 0;
		symbol->section->output_section = NULL;
		symbol->section->output_offset = 0;
		if(result != bfd_reloc_ok
		&& result != bfd_reloc_undefined) {
			edll_seterror(edll_err_bfd_error);
			err = -1;
		}
#else
		/* In this case, do the relocation ourselves */
		addr = (unsigned char *) vma + r[0]->address;
		if(r[0]->howto->pc_relative) {
			/*
			 * WARNING: I found the place where the (1 << size) adjustment is
			 *	    done in the BFD.
			 *	    Look at the function coff_i386_reloc() in coff-i386.c
			 *	    This means we should now be able to link ELF images
			 *	    as is too!
			 */
			ptr = (unsigned char *) ptr - (unsigned int) addr
				- (bfd_family_coff(module->hbfd) ? (1 << r[0]->howto->size) : 0);
		}
		/* TODO: we assume little endian... */
		switch(r[0]->howto->size) {
		case 0:
			addr[0] += (unsigned char) (unsigned int) ptr;
			break;

		case -1:
			ptr = (edll_ptr) - (unsigned int) ptr;
		case 1:
			value = addr[0] + (addr[1] << 8) + (unsigned int) ptr;
			addr[0] = value;
			addr[1] = value >> 8;
			break;

		case -2:
			ptr = (edll_ptr) - (unsigned int) ptr;
		case 2:
			value = addr[0] + (addr[1] << 8) + (addr[2] << 16) + (addr[3] << 24) + (unsigned int) ptr;
			addr[0] = value;
			addr[1] = value >> 8;
			addr[2] = value >> 16;
			addr[3] = value >> 24;
			break;

		/* TODO: add support for 64 bits */
		}
#endif
	}

	edll_free(relocs);

#if 0
fflush(stdout);
#endif

	return err;
}



int edll_open_file(edll_module *module)
{
	int		r;

/* open the module with BFD */
	module->hbfd = bfd_openr(module->filename, "default");
	if(!module->hbfd) {
		/* the load failed somehow... */
		edll_seterror(edll_err_cant_load_plugin);
		return -1;
	}

/* NOTE:
 * We only support objects at this time, later we may want
 * to support archives as well!
 */
	if(!bfd_check_format(module->hbfd, bfd_object)) {
		edll_seterror(edll_err_incompatible_format);
		return -1;
	}

/* create an array of sections to be loaded (including the .load section) */
	module->err = edll_err_none;
	bfd_map_over_sections(module->hbfd, edll_record_sections, module);
	if(module->err != edll_err_none) {
		/* it is likely already the current error */
		edll_seterror(module->err);
		return -1;
	}

/* load the symbols table */
	r = edll_get_symbols(module);
	if(r != 0) {
		return -1;
	}

/* record the COMMON section too since it includes BSS like data (same names need to be merged) */
	r = edll_record_common_section(module);
	if(r != 0) {
		return -1;
	}
	return 0;
}





/* vim: ts=8
 */
