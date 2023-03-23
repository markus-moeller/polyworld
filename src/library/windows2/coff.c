/* coff.c -- Enhanced Dynamic Library for MS-Windows
 * Load a COFF file in a way similar to BFD
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
 * This file reimplements the COFF functionality otherwise provided
 * by the BFD library (to avoid the GPL of the BFD/iberty).
 */
#include	"edll_internal.h"




/************************************************************/
/************************************************************/
/*** DOS & COFF FILE FORMAT SUPPORT                       ***/
/************************************************************/
/************************************************************/



int edll_resolve_section(edll_module *module, edll_ptr vma, asection *hsec)
{
	struct coff_relocation	temp_reloc;
	struct coff_section	*section;
	edll_module		*ignore;
	asymbol			*symbol;
	int			idx, size, count, len, err;
	long			value;
	edll_ptr		ptr;
	unsigned char		*relocs, *r, *addr;

/** test whether the section has relocation information **/
	section = hsec->coff_section;
	count = section->f_relocations_count;
	if(count == 0 || section->f_relocations == 0) {
		return 0;
	}

/** define the size of the relocation table **/
	fseek(module->file, section->f_relocations, SEEK_SET);
	if(count == 0xFFFF && (section->f_flags & COFF_SECTION_RELOCS_OVERFLOW) != 0) {
		/* the first entry is the actual size */
		fread(&temp_reloc, 1, sizeof(struct coff_relocation), module->file);
		/*
		 * TODO: make sure that on an overflow, the count is inclusive
		 * (reading the Microsoft docs, it sounds like it is)
		 */
		count = temp_reloc.f_virtual_address - 1;
	}
	len = count * COFF_RELOCATION_SIZEOF;

/** allocate the file relocation table, it is a throw away **/
	relocs = edll_alloc(len, 0);
	if(relocs == 0) {
		return -1;
	}

/** read the raw relocation information **/
	size = fread(relocs, 1, len, module->file);
	if(size != len) {
		edll_free(relocs);
		edll_seterror(edll_err_cant_load_plugin);
		return -1;
	}

/** go through the relocation and resolve them **/
	r = relocs;
	err = 0;
	for(idx = 0; idx < count; ++idx, r += 10) {
		value = r[4] + (r[5] << 8)
				+ (r[6] << 16) + (r[7] << 24);
		if(value >= module->symbols_count) {
			err = -1;
			edll_seterror(edll_err_cant_load_plugin);
			continue;
		}
		symbol = module->symbols + value;
		if(symbol->storage_class == COFF_STORAGE_STATIC
		|| edll_is_common_section(module, symbol->section)) {
			/* static and common symbols are... right here! */
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
	symbol->name, hsec->coff_section->f_name, module->filename);
					err = -1;
					edll_seterror(edll_err_cant_load_plugin);
					continue;
				}
			}
		}

		value = r[0] + (r[1] << 8)
				+ (r[2] << 16) + (r[3] << 24);
		if(value + 4 > hsec->size) {
			/* overflow */
			err = -1;
			edll_seterror(edll_err_cant_load_plugin);
			continue;
		}
		addr = (unsigned char *) vma + value;
#if 0
printf("At (%p + %lx) -> %p before 0x%08X + %p\n", vma, value, addr,
	addr[0] + (addr[1] << 8) + (addr[2] << 16) + (addr[3] << 24), ptr);
#endif
		switch(r[8] + (r[9] << 8)) {
		case COFF_RELOC_REL32:
			ptr = (unsigned char *) ptr - (coff_uint32_t) addr - 4;
		case COFF_RELOC_DIR32:
		case COFF_RELOC_DIR32_NO_BASE:	/* I guess the base is in a register, but what base? */
			value = addr[0] + (addr[1] << 8) + (addr[2] << 16) + (addr[3] << 24) + (coff_uint32_t) ptr;
			addr[0] = value;
			addr[1] = value >> 8;
			addr[2] = value >> 16;
			addr[3] = value >> 24;
			break;

		}
#if 0
printf("At (%p + %lx) -> %p after 0x%08X\n", vma, value, addr,
	addr[0] + (addr[1] << 8) + (addr[2] << 16) + (addr[3] << 24));
#endif
	}

	edll_free(relocs);

	return err;
}




/*
 * Load the raw data.
 */
int edll_get_section_contents(edll_module *module, asection *hsec)
{
	if((hsec->coff_section->f_flags & COFF_SECTION_BSS) != 0) {
		memset(hsec->userdata, 0, hsec->size);
		return 1;
	}

	fseek(module->file, hsec->coff_section->f_raw_data, SEEK_SET);
	if(fread(hsec->userdata, 1, hsec->size, module->file) != hsec->size) {
		edll_seterror(edll_err_cant_load_plugin);
		return 0;
	}

	return 1;
}





/*
 * TODO: break this function in three or more (header, sections, symbols)
 */
int edll_open_file(edll_module *module)
{
/* the MS-DOS header is 64 bytes
 * the COFF header is 20 bytes
 * the optional header is usually 224 bytes
 */
	char					extra_header[1024];	/* optional header buffer */
	char					header[64];		/* MS-DOS and COFF */

	struct coff_optional_header_magic	*opt_header_magic;
	struct coff_optional_header		*opt_header;
	struct coff_optional_header64		*opt_header64;
	struct coff_header			*hdr;
	struct coff_section			*section;
	edll_symbol				*new_symbols;
	unsigned char				*sym;
	asymbol					*hsym;
	asection				*hsec;
	coff_uint32_t				start;
	int					idx, size, len, sec_idx, pos, closest;
	long					offset, vma;

/* open the file (if we want to support archives and ELF formats, we should have that outside!) */
	module->file = fopen(module->filename, "rb");
	if(module->file == 0) {
		edll_seterror(edll_err_cant_load_plugin);
		return -1;
	}

/* get the file size */
	fseek(module->file, 0, SEEK_END);
	module->file_size = ftell(module->file);
	rewind(module->file);

/* read a header, may be a COFF or MS-DOS header */
	size = fread(header, 1, sizeof(struct msdos_header), module->file);
	if(size < sizeof(struct coff_header)) {
		edll_seterror(edll_err_cant_load_plugin);
		return -1;
	}

/* if MS-DOS header, skip it */
	if(header[0] == 'M' && header[1] == 'Z') {
		/* we must have an MS-DOS header */
		struct msdos_header *msdos = (struct msdos_header *) header;
		if(size < 0x0040
		|| msdos->f_header_paragraphs != 4
		|| msdos->f_pe_offset < 0x0040
		|| msdos->f_pe_offset > module->file_size) {
			edll_seterror(edll_err_cant_load_plugin);
			return -1;
		}
		fseek(module->file, msdos->f_pe_offset, SEEK_SET);
		start = msdos->f_pe_offset + sizeof(struct coff_pe);
		/* make sure we have a PE file */
		size = fread(header, 1, sizeof(struct coff_pe), module->file);
		if(size < sizeof(struct coff_pe)) {
			edll_seterror(edll_err_cant_load_plugin);
			return -1;
		}
		if(header[0] != 'P'
		|| header[1] != 'E'
		|| header[2] != '\0'
		|| header[3] != '\0') {
			edll_seterror(edll_err_cant_load_plugin);
			return -1;
		}
		/* okay! now read the COFF header (we don't need to read more bytes here) */
		size = fread(header, 1, sizeof(struct coff_header), module->file);
	}
	else {
		fseek(module->file, sizeof(struct coff_header), SEEK_SET);
		start = 0;
	}

/* now check for a valid COFF header */
	if(size < sizeof(struct coff_header)) {
		edll_seterror(edll_err_cant_load_plugin);
		return -1;
	}

/* make sure it is an I386 COFF, and that we have some sections */
	hdr = (struct coff_header *) header;

	len = sizeof(struct coff_section) * hdr->f_sections_count;

	if(hdr->f_magic != COFF_MAGIC_I386
	&& (hdr->f_flags & COFF_HEADER_FLAGS_32BIT_MACHINE) != 0
	&& (hdr->f_flags & (COFF_HEADER_FLAGS_SYSTEM
				| COFF_HEADER_FLAGS_DLL
				| COFF_HEADER_FLAGS_UP_SYSTEM_ONLY)) == 0
	&& hdr->f_sections_count > 0
	&& start + sizeof(struct coff_header) + hdr->f_optional_header + len < module->file_size
	&& hdr->f_symbols_pointer + hdr->f_symbols_count * COFF_SYMBOL_SIZEOF + 4 < module->file_size) {
		edll_seterror(edll_err_cant_load_plugin);
		return -1;
	}

	/* TODO: we can use 64bits only if the COFF_HEADER_FLAGS_LARGE_ADDRESS_AWARE
	 *	 flag is set; otherwise the code is likely to fail.
	 */

/* we must read the optional header since it gives us the proper virtual address of executables */
	if(hdr->f_optional_header > 0) {
		len = hdr->f_optional_header;
		if(len > sizeof(extra_header)) {
			len = sizeof(extra_header);
		}
		size = fread(extra_header, 1, len, module->file);
		if(size != len) {
			edll_seterror(edll_err_cant_load_plugin);
			return -1;
		}
		/* skip the remainder if optional header was larger than sizeof(extra_header) */
		if(len < hdr->f_optional_header) {
			fseek(module->file, hdr->f_optional_header - len, SEEK_CUR);
		}
		opt_header_magic = (struct coff_optional_header_magic *) extra_header;
		switch(opt_header_magic->f_magic) {
		case COFF_MAGIC_PE:
			opt_header = (struct coff_optional_header *) extra_header;
			vma = opt_header->f_image_base;
			break;

		case COFF_MAGIC_PE_PLUS:
			opt_header64 = (struct coff_optional_header64 *) extra_header;
			vma = opt_header64->f_image_base;
			break;

		default:
			edll_seterror(edll_err_cant_load_plugin);
			return -1;
		}
	}
	else {
		vma = 0;
	}

/* allocate a buffer and read the sections */
	module->sections_count = hdr->f_sections_count;
	module->sections = edll_alloc(sizeof(asection) * hdr->f_sections_count + len, 0);
	if(module->sections == 0) {
		return -1;
	}
	/* read the COFF sections after the asection's */
	section = (struct coff_section *) (module->sections + hdr->f_sections_count);
	size = fread(section, 1, len, module->file);
	if(size != len) {
		edll_seterror(edll_err_cant_load_plugin);
		return -1;
	}
	/* initialize the canonilized asection's */
	hsec = module->sections;
	idx = hdr->f_sections_count;
	while(idx > 0) {
		--idx;

		/* we initialize and keep all the sections since a symbol may point to any of them */
		hsec->coff_section = section;
		hsec->size = section->f_size;
		hsec->userdata = 0;
		hsec->vma = (edll_ptr) section->f_virtual_address + vma;

		/* WARNING: at this time I do not use the f_virtual_size field therefore I clear
		 *	    it; this may later be a bug...
		 */
		section->f_virtual_size = 0;	/* nul terminate 8 chars names */

#if 0
printf("EDLL: recording section [%s]\n", section->f_name);
fflush(stdout);
#endif
		/* TODO: should we force the alignment to 4Kb on Intel and voila? */
		if((section->f_flags & COFF_SECTION_NO_PAD) != 0) {
			hsec->alignment_power = 1;
		}
		else {
			hsec->alignment_power = (section->f_flags & COFF_SECTION_ALIGN_MASK) >> COFF_SECTION_ALIGN_SHIFT;
			if(hsec->alignment_power < 1) {
				hsec->alignment_power = 1;
			}
			else if(hsec->alignment_power > 14) {
				hsec->alignment_power = 14;
			}
		}
		/* record sections that the EDLL is interested in */
		if(section->f_size > 0) {
			/** determine the section type **/
			sec_idx = -1;
			if(strcmp(section->f_name, ".load") == 0) {
				/*
				 * We found our special section which lists the necessary
				 * DLLs and other modules we need to load before we can
				 * proceed with the link process.
				 */
				sec_idx = edll_section_dependencies;
			}
#ifdef EDLL_VERSION_CHECK
			else if(strcmp(section->f_name, ".version") == 0) {
				/* includes the module version */
				sec_idx = edll_section_version;
			}
#endif
			else if(strcmp(section->f_name, ".ctors") == 0) {
				/* the ctors indicates the function(s) to call to construct the C++ static objects */
				sec_idx = edll_section_ctors;
			}
			else if(strcmp(section->f_name, ".dtors") == 0) {
				/* the dtors indicates the function(s) to call to destruct the C++ static objects */
				sec_idx = edll_section_dtors;
			}
			else if(section->f_flags & (COFF_SECTION_TEXT | COFF_SECTION_EXECUTE)) {
				/* we assume that .text sections are always read only */
				sec_idx = edll_section_ex_data;
			}
			else if(section->f_flags & (COFF_SECTION_DATA | COFF_SECTION_BSS | COFF_SECTION_WRITE)) {
				/* .data, .bss */
				sec_idx = edll_section_rw_data;
			}
			else if(section->f_flags & (COFF_SECTION_DATA | COFF_SECTION_READ)) {
				/* .rdata */
				sec_idx = edll_section_ro_data;
			}
			if(sec_idx >= 0) {
				edll_record_section(module, sec_idx, hsec);
			}
		}


		++hsec;
		++section;
	}

/*
 * Load the COFF symbols and strings if any, we will need them
 * so we don't have to wait later to get them. Also, we directly
 * canonilize them and we only take these symbols which are of
 * interest for EDLL.
 */
	if(hdr->f_symbols_pointer != 0) {
		/* allocate the canonilized symbols directly followed by the files symbols */
		len = COFF_SYMBOL_SIZEOF * hdr->f_symbols_count;
		module->symbols = edll_alloc(sizeof(asymbol) * hdr->f_symbols_count + len, 0);
		if(module->symbols == 0) {
			return -1;
		}
		sym = (unsigned char *) (module->symbols + hdr->f_symbols_count);

		fseek(module->file, hdr->f_symbols_pointer, SEEK_SET);
		size = fread(sym, 1, len, module->file);
		if(size != len) {
			edll_seterror(edll_err_cant_load_plugin);
			return -1;
		}

		/* now that we got the symbols, we can read the strings */
		size = fread(&module->strings_size, 1, sizeof(coff_uint32_t), module->file);
		if(size != sizeof(coff_uint32_t)) {
			edll_seterror(edll_err_cant_load_plugin);
			return -1;
		}
		module->strings_size -= 4;
		module->strings = edll_alloc(module->strings_size, 0);
		if(module->strings == 0) {
			return -1;
		}
		size = fread(module->strings, 1, module->strings_size, module->file);
		if(size != module->strings_size) {
			edll_seterror(edll_err_cant_load_plugin);
			return -1;
		}

		/* enlarge the global table of symbols */
		new_symbols = edll_realloc(g_edll_symbols, (g_edll_symbols_count + hdr->f_symbols_count) * sizeof(edll_symbol), -1);
		if(new_symbols == 0) {
			return -1;
		}
		g_edll_symbols = new_symbols;

		hsym = module->symbols;
		idx = hdr->f_symbols_count;
		module->symbols_count = idx;
		while(idx > 0) {
			hsym->storage_class = sym[16];
			hsym->value = sym[8] + (sym[9] << 8) + (sym[10] << 16) + (sym[11] << 24);
			sec_idx = sym[12] + (sym[13] << 8);
			if(sec_idx >= 1 && sec_idx <= hdr->f_sections_count) {
				hsym->section = module->sections + sec_idx - 1;
			}
			else if(sec_idx == 0 && hsym->value != 0 && hsym->storage_class == COFF_STORAGE_EXTERNAL) {
				hsym->section = &module->common_section;
				offset = module->common_size;
				module->common_size += hsym->value;
				hsym->value = offset;
			}
			else {
				/*
				 * absolute and debug sections are ignored
				 * the undefined section is not useful to edll
				 */
				hsym->section = 0;
			}
			if(sym[0] == 0 && sym[1] == 0 && sym[2] == 0 && sym[3] == 0) {
				/* long name taken from the strings table */
				hsym->name = module->strings + sym[4] + (sym[5] << 8)
					+ (sym[6] << 16) + (sym[7] << 24) - 4;
			}
			else {
				hsym->name = sym;

				/*
				 * we will clear the 1st byte of the value
				 * in case the name is 8 chars (we already
				 * read the value anyway)
				 */
				sym[8] = '\0';
			}

			/* we do not care about undefined, debug and common symbols */
			/* TODO: do we need absolute symbols? */
			if(hsym->section != NULL
			&& ((sec_idx > 0 && hsym->storage_class != COFF_STORAGE_EXTERNAL)
			  || edll_is_common_section(module, hsym->section)
			  || hsym->storage_class == COFF_STORAGE_EXTERNAL)) {
				pos = edll_find_symbol(hsym->name, &closest);
				if(pos < 0) {
					pos = closest;
				}
				memmove(g_edll_symbols + pos + 1, g_edll_symbols + pos, (g_edll_symbols_count - pos) * sizeof(edll_symbol));
				g_edll_symbols[pos].symbol = hsym;
				g_edll_symbols[pos].module = module;
#if 0
{
char *name;
if(hsym->section == 0) {
name = "<no section>";
} else {
name = hsym->section->coff_section->f_name;
}
printf("Adding symbol [%s] in section %p (%s)\n",
	hsym->name,
	hsym->section,
	name);
}
#endif
				g_edll_symbols_count++;
			}

			++hsym;

			len = sym[17] + 1;
			sym += len * COFF_SYMBOL_SIZEOF;
			idx -= len;

			/* we fill the holes since the relocations could point to these
			 * (unlikely though); but we need to keep the other symbols at
			 * the right index position!
			 */
			while(len > 1) {
				--len;
				hsym->section = 0;
				hsym->storage_class = 0;
				hsym->value = 0;
				hsym->name = 0;
				++hsym;
			}
		}
	}

/* if we found some symbols in the common section, record the common section now */
	if(module->common_size != 0) {
		edll_record_section(module, edll_section_rw_data, &module->common_section);
	}

	return 0;
}





/* vim: ts=8
 */
