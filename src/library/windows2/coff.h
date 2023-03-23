/* coff.h -- Enhanced Dynamic Library for MS-Windows
 * COFF file definitions
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
#ifndef __EDLL_COFF_H__
#define __EDLL_COFF_H__




/************************************************************/
/************************************************************/
/*** DOS & COFF FILE FORMAT                               ***/
/************************************************************/
/************************************************************/


/*
 * We deal with two files (and an half, the DLL are not loaded
 * here for now!)
 *
 * The MS-Windows executables have an MS-DOS stub in front of
 * them so one can run them in MS-DOS and get a prompt saying
 * that it won't load "in DOS mode". Otherwise, they are very
 * similar to the COFF object files.
 *
 * The following describes a file. Since most of these sections
 * are not of a specific size, it is shown here mainly for
 * informational purposes (i.e. we cannot just declare a
 * C structure of the entire file!)
 *
 * struct pe_coff
 * {
 *	msdos_header		f_msdos_header;		.EXE only
 *	msdos_stub		f_stub;			.EXE only
 *	coff_pe			f_pe_magic;		.EXE only
 *	coff_header		f_header;
 *	coff_optional_header	f_optional_header;	variable size
 *	coff_data_directory	f_data_directory;	array of variable size
 *	coff_section		f_sections[n0];		array of sections
 *	coff_uint8_t		f_raw_data[n1];		all the sections data
 *	coff_relocation		f_relocations[n2];	up to one relocation per section
 * };
 */

/* for this one we need to make sure that we have
 * integers of exactly the size we want to use.
 */


typedef	int8_t			coff_int8_t;
typedef	uint8_t			coff_uint8_t;
typedef	int16_t			coff_int16_t;
typedef	uint16_t		coff_uint16_t;
typedef	int32_t			coff_int32_t;
typedef	uint32_t		coff_uint32_t;
typedef	int64_t			coff_int64_t;
typedef	uint64_t		coff_uint64_t;


/*
 * MS-DOS header (for .exe files)
 */
struct msdos_header
{
	coff_uint16_t	f_magic;		/* 0x5A4D */
	coff_uint16_t	f_bytes_in_last_block;
	coff_uint16_t	f_blocks_in_file;
	coff_uint16_t	f_num_relocs;
	coff_uint16_t	f_header_paragraphs;	/* must be 4 to be a valid MS-Windows executable (can be just 2 for MS-DOS!) */
	coff_uint16_t	f_min_extra_paragraphs;
	coff_uint16_t	f_max_extra_paragraphs;
	coff_uint16_t	f_ss;
	coff_uint16_t	f_sp;
	coff_uint16_t	f_checksum;
	coff_uint16_t	f_ip;
	coff_uint16_t	f_cs;
	coff_uint16_t	f_reloc_table_offset;
	coff_uint16_t	f_overlay_number;
	coff_uint32_t	f_unused1[2];
	coff_uint16_t	f_oemid;		/* ??? */
	coff_uint16_t	f_oeminfo;		/* ??? */
	coff_uint32_t	f_unused2[5];
	coff_uint32_t	f_pe_offset;		/* location of the PE image in the file */
};


/*
 * The actual default MS-DOS stub:
 *
 * 00000040-  0E 1F BA 0E 00 B4 09 CD 21 B8 01 4C CD 21 54 68 ........!..L.!Th
 * 00000050-  69 73 20 70 72 6F 67 72 61 6D 20 63 61 6E 6E 6F is program canno
 * 00000060-  74 20 62 65 20 72 75 6E 20 69 6E 20 44 4F 53 20 t be run in DOS 
 * 00000070-  6D 6F 64 65 2E 0D 0D 0A 24 00 00 00 00 00 00 00 mode....$.......
 *
 * Note that you should NOT assume that this IS the stub. That is, you could
 * very well have a Curses like version of a MS-Windows application (I have
 * never seen one though... 8-)
 *
 * Yet, if you want to create an MS-Windows application only, you can copy
 * this stub verbatim.
 */
struct msdos_stub
{
	coff_uint8_t	f_stub[0x40];
};



/*
 * The PE magic appears right before a COFF header.
 */
struct coff_pe
{
	coff_uint8_t	f_p;	/* 'P' */
	coff_uint8_t	f_e;	/* 'E' */
	coff_uint16_t	f_zero;	/* always zero */
};


/*
 * COFF header (Object files, i.e. our plugins)
 */
struct coff_header
{
	coff_uint16_t	f_magic;		/* 0x0145 for GNU object files */
	coff_int16_t	f_sections_count;	/* # of sections (supposedly limited to 96, for sure limited 32767) */
	coff_int32_t	f_time_date;		/* time_t with time of creation (or 0) */
	coff_uint32_t	f_symbols_pointer;	/* offset in file where the symbol table starts */
	coff_uint32_t	f_symbols_count;	/* the number of symbols */
	coff_uint16_t	f_optional_header;	/* size of the optional header */
	coff_uint16_t	f_flags;		/* header flags */
};

#define	COFF_MAGIC_UNKNOWN	0x000
#define	COFF_MAGIC_ALPHA	0x184
#define	COFF_MAGIC_ARM		0x1C0
#define	COFF_MAGIC_ALPHA64	0x284
#define	COFF_MAGIC_I386		0x14C	/* this is what our Plugins are on a usual PC */
#define	COFF_MAGIC_ARM		0x1C0
#define	COFF_MAGIC_IA64		0x200
#define	COFF_MAGIC_M68K		0x268
#define	COFF_MAGIC_MIPS16	0x266
#define	COFF_MAGIC_MIPSFPU	0x366
#define	COFF_MAGIC_MIPSFPU16	0x466
#define	COFF_MAGIC_POWERPC	0x1F0
#define	COFF_MAGIC_R3000	0x162
#define	COFF_MAGIC_R4000	0x166
#define	COFF_MAGIC_R10000	0x168
#define	COFF_MAGIC_SH3		0x1A2
#define	COFF_MAGIC_SH4		0x1A6
#define	COFF_MAGIC_THUMB	0x1C2


#define	COFF_HEADER_FLAGS_RELOCS_STRIPPED		0x0001
#define	COFF_HEADER_FLAGS_EXCUTABLE_IMAGE		0x0002
#define	COFF_HEADER_FLAGS_LINE_NUMS_STRIPPED		0x0004
#define	COFF_HEADER_FLAGS_LOCAL_SYMS_STRIPPED		0x0008
#define	COFF_HEADER_FLAGS_AGGRESSIVE_WS_TRIM		0x0010
#define	COFF_HEADER_FLAGS_LARGE_ADDRESS_AWARE		0x0020
#define	COFF_HEADER_FLAGS_16BIT_MACHINE			0x0040
#define	COFF_HEADER_FLAGS_BYTES_RESERVED_LO		0x0080
#define	COFF_HEADER_FLAGS_32BIT_MACHINE			0x0100
#define	COFF_HEADER_FLAGS_DEBUG_STRIPPED		0x0200
#define	COFF_HEADER_FLAGS_REMOVEABLE_RUN_FROM_SWAP	0x0800
#define	COFF_HEADER_FLAGS_SYSTEM			0x1000
#define	COFF_HEADER_FLAGS_DLL				0x2000
#define	COFF_HEADER_FLAGS_UP_SYSTEM_ONLY		0x4000
#define	COFF_HEADER_FLAGS_BYTES_REVERSED_HI		0x8000



/*
 * Note that the size of the optional header is not the size of
 * the following structure, instead, check the main header
 * field named 'f_optional_header'.
 */
struct coff_optional_header_magic
{
/* COFF required fields */
	coff_uint16_t	f_magic;		/* 0x10B or 0x20B */
	coff_uint8_t	f_major_linker_version;
	coff_uint8_t	f_minor_linker_version;
};


struct coff_optional_header
{
	struct coff_optional_header_magic f_header;	/* 0x10B */
	coff_uint32_t	f_size_of_text;		/* total size of all .text sections */
	coff_uint32_t	f_size_of_data;		/* total size of all .data sections */
	coff_uint32_t	f_size_of_bss;		/* total size of all .bss sections */
	coff_uint32_t	f_entry_point;		/* address when starting this executable */
	coff_uint32_t	f_base_of_text;		/* address where text needs to be loaded */
	coff_uint32_t	f_base_of_data;		/* address where data needs to be loaded */

/* Microsoft Windows required fields */
	coff_uint32_t	f_image_base;		/* for Win32 0x0040:0000 by default */
	coff_uint32_t	f_section_alignment;	/* usually the page size (4Kb) */
	coff_uint32_t	f_file_alignment;	/* the alignment to align raw data of sections */
	coff_uint16_t	f_major_os_version;	/* OS version required (4.0 - NT 4.0) */
	coff_uint16_t	f_minor_os_version;
	coff_uint16_t	f_major_image_version;	/* this executable version */
	coff_uint16_t	f_minor_image_version;
	coff_uint16_t	f_major_subsystem_version;
	coff_uint16_t	f_minor_subsystem_version;
	coff_uint32_t	f_reserved;
	coff_uint32_t	f_size_of_image;	/* total size of image multiple of section alignment */
	coff_uint32_t	f_size_of_headers;	/* size of all the headers (including sections, etc.) */
	coff_uint32_t	f_checksum;		/* see IMAGHELP.DLL, can be 0 */
	coff_uint16_t	f_subsystem;		/* OS required to run this image */
	coff_uint16_t	f_dll_characteristics;	/* see flags below */
	coff_uint32_t	f_size_of_stack_reserve;/* max. stack size */
	coff_uint32_t	f_size_of_stack_commit;	/* available at the start */
	coff_uint32_t	f_size_of_heap_reserve;	/* max. heap size */
	coff_uint32_t	f_size_of_heap_commit;	/* available at the start */
	coff_uint32_t	f_loader_flags;		/* obsolete */
	coff_uint32_t	f_data_directories_count;/* follow directly this header */
};

struct coff_optional_header64
{
	struct coff_optional_header_magic f_header;	/* 0x20B */
	coff_uint32_t	f_size_of_text;		/* total size of all .text sections */
	coff_uint32_t	f_size_of_data;		/* total size of all .data sections */
	coff_uint32_t	f_size_of_bss;		/* total size of all .bss sections */
	coff_uint32_t	f_entry_point;		/* address when starting this executable */
	coff_uint32_t	f_base_of_text;		/* address where text needs to be loaded */

/* Microsoft Windows required fields */
	coff_uint64_t	f_image_base;		/* for Win32 0x0040:0000 by default */
	coff_uint32_t	f_section_alignment;	/* usually the page size (4Kb) */
	coff_uint32_t	f_file_alignment;	/* the alignment to align raw data of sections */
	coff_uint16_t	f_major_os_version;	/* OS version required (4.0 - NT 4.0) */
	coff_uint16_t	f_minor_os_version;
	coff_uint16_t	f_major_image_version;	/* this executable version */
	coff_uint16_t	f_minor_image_version;
	coff_uint16_t	f_major_subsystem_version;
	coff_uint16_t	f_minor_subsystem_version;
	coff_uint32_t	f_reserved;
	coff_uint32_t	f_size_of_image;	/* total size of image multiple of section alignment */
	coff_uint32_t	f_size_of_headers;	/* size of all the headers (including sections, etc.) */
	coff_uint32_t	f_checksum;		/* see IMAGHELP.DLL, can be 0 */
	coff_uint16_t	f_subsystem;		/* OS required to run this image */
	coff_uint16_t	f_dll_characteristics;	/* see flags below */
	coff_uint64_t	f_size_of_stack_reserve;/* max. stack size */
	coff_uint64_t	f_size_of_stack_commit;	/* available at the start */
	coff_uint64_t	f_size_of_heap_reserve;	/* max. heap size */
	coff_uint64_t	f_size_of_heap_commit;	/* available at the start */
	coff_uint32_t	f_loader_flags;		/* obsolete */
	coff_uint32_t	f_data_directories_count;/* follow directly this header */
};

/* f_magic */
#define	COFF_MAGIC_PE		0x10B	/* PE32 */
#define	COFF_MAGIC_PE_PLUS	0x20B	/* PE32+ */

/* f_subsystem is one of the following */
#define	COFF_SUBSYSTEM_UNKNOWN			0
#define	COFF_SUBSYSTEM_NATIVE			1
#define	COFF_SUBSYSTEM_WINDOWS_GUI		2
#define	COFF_SUBSYSTEM_WINDOWS_CUI		3
#define	COFF_SUBSYSTEM_POSIX_CUI		7
#define	COFF_SUBSYSTEM_WINDOWS_CE_GUI		9
#define	COFF_SUBSYSTEM_EFI_APPLICATION		10
#define	COFF_SUBSYSTEM_EFI_BOOT_SERVIVE_DRIVE	11
#define	COFF_SUBSYSTEM_EFI_RUNTIME_DRIVER	12

/* f_dll_characteristics */
#define	COFF_DLL_NO_BIND			0x0800
#define	COFF_DLL_WDM_DRIVER			0x2000
#define	COFF_DLL_TERMINAL_SERVER_AWARE		0x8000


/*
 * The data directory used by the loader in some magical
 * way to do stuff. How's that? 8-)
 */
struct coff_data_directory
{
	coff_uint32_t		f_rva;		/* can be null */
	coff_uint32_t		f_size;		/* can be zero */
};

/* the following is a list of the data directories */
#define	COFF_DD_EXPORT		0
#define	COFF_DD_IMPORT		1
#define	COFF_DD_RESOURCE	2
#define	COFF_DD_EXCEPTION	3
#define	COFF_DD_CERTIFICATE	4	/* rva is the file pointer */
#define	COFF_DD_BASE_RELOCATION	5
#define	COFF_DD_DEBUG		6
#define	COFF_DD_ARCHITECTURE	7
#define	COFF_DD_GLOBALPTR	8
#define	COFF_DD_THREAD_LOCAL_STORAGE	9	/* TLS */
#define	COFF_DD_LOAD_CONFIG	10
#define	COFF_DD_BOUND_IMPORT	11
#define	COFF_DD_IMPORT_ADDR	12
#define	COFF_DD_DELAY_IMPORT	13
#define	COFF_DD_COM_PLUS	14
#define	COFF_DD_reserved	15	/* should always be present */



/*
 * The following describes a section.
 * The virtual address and size are supposed to be zero in object files.
 *
 * NOTE: multiple sections with names followed by $<string> need to be
 *	 sorted to generate an image file; edll ignores that and it works
 *	 fine. It must be important to ensure some minimal hit to unloaded
 *	 data of mapped image files.
 */
struct coff_section
{
	coff_uint8_t	f_name[8];	/* long names are defined in the string table at offset defined after the '/' (i.e. /54 is string #54) */
	coff_uint32_t	f_virtual_size;
	coff_uint32_t	f_virtual_address;
	coff_uint32_t	f_size;		/* size of data in the file (0 for BSS) */
	coff_uint32_t	f_raw_data;	/* offset in the file (from the start) to the raw data */
	coff_uint32_t	f_relocations;	/* offset in the file to the relocation for this section */
	coff_uint32_t	f_line_numbers;
	coff_uint16_t	f_relocations_count;	/* see COFF_SECTION_EXTENDED_RELOC when more than 65535 relocs! */
	coff_uint16_t	f_line_numbers_count;
	coff_uint32_t	f_flags;
};

/* f_flags */
/* flags not defined here are marked reserved */
#define	COFF_SECTION_NO_PAD		0x00000008	/* use ALIGN_1BYTES, for object files only */
#define	COFF_SECTION_TEXT		0x00000020	/* section contains text */
#define	COFF_SECTION_DATA		0x00000040	/* section contains data */
#define	COFF_SECTION_BSS		0x00000080	/* section is BSS */
#define	COFF_SECTION_INFO		0x00000200	/* only for objects */
#define	COFF_SECTION_REMOVE		0x00000800	/* remove when linking */
#define	COFF_SECTION_COMMON_DATA	0x00001000	/* COMMON data */
#define	COFF_SECTION_ALIGN_1BYTES	0x00100000
#define	COFF_SECTION_ALIGN_2BYTES	0x00200000
#define	COFF_SECTION_ALIGN_4BYTES	0x00300000
#define	COFF_SECTION_ALIGN_8BYTES	0x00400000
#define	COFF_SECTION_ALIGN_16BYTES	0x00500000
#define	COFF_SECTION_ALIGN_32BYTES	0x00600000
#define	COFF_SECTION_ALIGN_64BYTES	0x00700000
#define	COFF_SECTION_ALIGN_128BYTES	0x00800000
#define	COFF_SECTION_ALIGN_256BYTES	0x00900000
#define	COFF_SECTION_ALIGN_512BYTES	0x00A00000
#define	COFF_SECTION_ALIGN_1024BYTES	0x00B00000
#define	COFF_SECTION_ALIGN_2048BYTES	0x00C00000
#define	COFF_SECTION_ALIGN_4096BYTES	0x00D00000
#define	COFF_SECTION_ALIGN_8192BYTES	0x00E00000
#define	COFF_SECTION_ALIGN_MASK		0x00F00000
#define	COFF_SECTION_ALIGN_SHIFT	20
#define	COFF_SECTION_RELOCS_OVERFLOW	0x01000000
#define	COFF_SECTION_DISCARDABLE	0x02000000
#define	COFF_SECTION_NOT_CACHED		0x04000000
#define	COFF_SECTION_NOT_PAGED		0x08000000
#define	COFF_SECTION_SHARED		0x10000000
#define	COFF_SECTION_EXECUTE		0x20000000
#define	COFF_SECTION_READ		0x40000000
#define	COFF_SECTION_WRITE		0x80000000

/*
 * If the COFF_SECTION_RELOCS_OVERFLOW is set in f_flags
 * and the f_relocations_count field is 0xFFFF then
 * the first relocation f_virtual_address is actually the
 * total number of relocation.
 */




/*
 * The COFF relocation information
 * Do not forget that the very first can be used to define
 * the total number of relocation (see COFF_SECTION_RELOCS_OVERFLOW)
 */
struct coff_relocation
{
	coff_uint32_t	f_virtual_address;	/* offset in raw data section */
	coff_uint32_t	f_symbol_index;		/* relocation name */
	coff_uint16_t	f_type;
};
/***** WARNING *****/
/*
 * This structure is INVALID as is, you cannot use it as an array
 * unless you read 10 bytes each time or read all the relocs and
 * then canonize them.
 */
#define	COFF_RELOCATION_SIZEOF	10

/*
 * A symbol of type IMAGE_SYM_CLASS_SECTION has its address set to
 * the beginning of the section (i.e. .text, .data, etc.)
 */

/* f_type */
#define	COFF_RELOC_ABSOLUTE		0x00	/* do nothing */
#define	COFF_RELOC_DIR32		0x06
#define	COFF_RELOC_DIR32_NO_BASE	0x07
#define	COFF_RELOC_SECTION		0x0A	/* for debug only */
#define	COFF_RELOC_SECTION_RELATIVE	0x0B	/* for debug only */
#define	COFF_RELOC_REL32		0x14
/* TODO: 64bits still undefined? */



/*
 * The Symbol table is composed of symbols defined in an array.
 * Each entry is exactly 18 bytes, but the content can vary if you have
 * auxiliaries (and the type of auxiliary is defined by the storage class).
 * The interpretation of the different fields is defined after the structure.
 */
struct coff_symbol
{
	union {
		coff_uint8_t	f_name[8];
		struct {
			coff_uint32_t	f_zero;
			coff_uint32_t	f_offset;	/* string table offset */
		} f_long_name;
	};
	coff_uint32_t	f_value;
	coff_int16_t	f_section;
	coff_uint16_t	f_type;
	coff_uint8_t	f_storage_class;
	coff_uint8_t	f_auxiliaries_count;	/* # of symbols following this one used as auxiliaries */
};
/***** WARNING *****/
/* these structures needs to be read one by one or canonalized
 * because it is not a mutiple of 4
 */
#define	COFF_SYMBOL_SIZEOF		18



/* f_name -- up to 8 NOT null terminated string
 * if the name is larger than 8 chars, then the first 4 bytes are set to 0
 * and the following 4 bytes represent an offset in the string table.
 */

/* f_section special values */
#define	COFF_SECTION_UNDEFINED		0
#define	COFF_SECTION_ABSOLUTE		-1
#define	COFF_SECTION_DEBUG		-2


/* f_type -- usuall COFF_SYMBOL_TYPE_NULL or COFF_SYMBOL_TYPE_FUNCTION for MS-Windows COFF */
#define	COFF_SYMBOL_TYPE_NULL		0
#define	COFF_SYMBOL_TYPE_VOID		1
#define	COFF_SYMBOL_TYPE_CHAR		2	/* int8 */
#define	COFF_SYMBOL_TYPE_SHORT		3	/* int16 */
#define	COFF_SYMBOL_TYPE_INT		4	/* int32 or int64 */
#define	COFF_SYMBOL_TYPE_LONG		5	/* int32 */
#define	COFF_SYMBOL_TYPE_FLOAT		6
#define	COFF_SYMBOL_TYPE_DOUBLE		7
#define	COFF_SYMBOL_TYPE_STRUCT		8
#define	COFF_SYMBOL_TYPE_UNION		9
#define	COFF_SYMBOL_TYPE_ENUM		10
#define	COFF_SYMBOL_TYPE_ENUM_MEMBER	11
#define	COFF_SYMBOL_TYPE_BYTE		12	/* uint8 */
#define	COFF_SYMBOL_TYPE_WORD		13	/* uint16 */
#define	COFF_SYMBOL_TYPE_UINT		14	/* uint32 or uint64 */
#define	COFF_SYMBOL_TYPE_DWORD		15	/* uint32 */
/* add this with the basic type to define a pointer of, function or array of */
#define	COFF_SYMBOL_TYPE_POINTER	0x10
#define	COFF_SYMBOL_TYPE_FUNCTION	0x20
#define	COFF_SYMBOL_TYPE_ARRAY		0x30

/* Microsoft requires the COFF_SYMBOL_TYPE_FUNCTION for incremental linking */


/* f_storage_class */
#define	COFF_STORAGE_END_OF_FUNCTION	0xFF
#define	COFF_STORAGE_NULL		0
#define	COFF_STORAGE_AUTOMATIC		1
#define	COFF_STORAGE_EXTERNAL		2
#define	COFF_STORAGE_STATIC		3
#define	COFF_STORAGE_REGISTER		4
#define	COFF_STORAGE_EXTERNAL_DEF	5
#define	COFF_STORAGE_LABEL		6
#define	COFF_STORAGE_UNDEFINED_LABEL	7
#define	COFF_STORAGE_MEMBER_OF_STRUCT	8
#define	COFF_STORAGE_ARGUMENT		9
#define	COFF_STORAGE_STRUCT_TAG		10
#define	COFF_STORAGE_MEMBER_OF_UNION	11
#define	COFF_STORAGE_UNION_TAG		12
#define	COFF_STORAGE_TYPE_DEFINITION	13
#define	COFF_STORAGE_UNDEFINED_STATIC	14
#define	COFF_STORAGE_ENUM_TAG		15
#define	COFF_STORAGE_MEMBER_OF_ENUM	16
#define	COFF_STORAGE_REGISTER_PARAM	17
#define	COFF_STORAGE_BIT_FIELD		18
#define	COFF_STORAGE_BLOCK		100
#define	COFF_STORAGE_FUNCTION		101
#define	COFF_STORAGE_END_OF_STRUCT	102
#define	COFF_STORAGE_FILE		103
#define	COFF_STORAGE_SECTION		104
#define	COFF_STORAGE_WEAK_EXTERNAL	105



/* auxiliary information for f_storage_class == COFF_STORAGE_EXTERNAL */
struct coff_aux_external
{
	coff_uint32_t	f_function_begin_symbol_index;
	coff_uint32_t	f_function_size;
	coff_uint32_t	f_line_number_offset;
	coff_uint32_t	f_next_function;
	coff_uint16_t	f_unused;
};
/***** WARNING *****/
/* see comment about struct symbol; */


/* auxiliary information for f_storage_class == COFF_STORAGE_FUNCTION */
/* for .bf, .lf and .ef */
struct coff_aux_function
{
	coff_uint32_t	f_unused1;
	coff_uint16_t	f_line_number;
	coff_uint8_t	f_unused2[6];
	coff_uint32_t	f_next_function;	/* .bf only */
	coff_uint16_t	f_unused3;
};


/* auxiliary information for f_storage_class == COFF_STORAGE_WEAK_EXTERNAL */
struct coff_aux_weak_external
{
	coff_uint32_t	f_fallback_symbol;
	coff_uint32_t	f_type;
	coff_uint8_t	f_unused[10];
};

/* f_type */
#define	COFF_WEAK_EXTERNAL_SEARCH_NOLIBRARY	1
#define	COFF_WEAK_EXTERNAL_SEARCH_LIBRARY	2
#define	COFF_WEAK_EXTERNAL_SEARCH_ALIAS		3


/* auxiliary information for f_storage_class == COFF_STORAGE_FILE */
struct coff_aux_file
{
	coff_uint8_t	f_filename[18];
};
/* there can be several such auxiliaries for long filenames */



/* auxiliary information for f_storage_class == COFF_STORAGE_SECTION */
struct coff_aux_section
{
	coff_uint32_t	f_size;		/* see coff_section.f_size */
	coff_uint16_t	f_relocations_count;
	coff_uint16_t	f_lines_count;
	coff_uint32_t	f_checksum;
	coff_uint16_t	f_section_index;
	coff_uint8_t	f_selection;	/* COMMON data seclection number */
	coff_uint8_t	f_unused[3];
};

/* f_selection */
#define	COFF_COMMON_NO_DUPLICATE	1
#define	COFF_COMMON_ANY			2
#define	COFF_COMMON_SAME_SIZE		3
#define	COFF_COMMON_EXACT_MATCH		4
#define	COFF_COMMON_ASSOCIATIVE		5
#define	COFF_COMMON_LARGEST		6



/*
 * The string table directly follows the symbol table.
 * You know the size of the symbol table (i.e. # of symbols x 18)
 * so you can find the strings.
 */
struct coff_strings
{
	coff_uint32_t	f_size;		/* size is inclusive of this structure */
};
/* the structure is followed by null terminated strings */


/* edll will not accept to load an object file with a .tls section */



/* vim: ts=8
 */
#endif		/* #ifndef __EDLL_COFF_H__ */
