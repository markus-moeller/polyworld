/* search_path.c -- Enhanced Dynamic Library for MS-Windows
 * Handling of the set of paths to find plugins and DLLs
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
/*** FILENAME CANONIZING                                  ***/
/************************************************************/
/************************************************************/

int edll_canonize_filename(char *name, int maxlen, const char **ext, const char *filename)
{
	int		len;
	char		c, has_ext, *d;
	const char	*s;

	if(ext) {
		*ext = 0;
	}

	/* If filename is empty or null, we want to load ourself
	 * but that has to be caught outside this function!
	 */
	if(!filename || !*filename) {
		edll_seterror(edll_err_invalid_filename);
		return -1;
	}

	/*
	 * Check the name as we copy it.
	 * If there is no extension add '.' at the end of the filename.
	 * This (supposedly) prevents MS-Windows from adding .dll automatically.
	 * The path can also include '/' instead of '\'. This loop converts
	 * these so it is compatible with LoadLibrary().
	 */
	len = maxlen - 2;
	has_ext = 0;
	d = name;
	s = filename;
	while((c = *s++)) {
		switch(c) {
		case '.':
			has_ext = 1;
			if(ext) {
				*ext = s;
			}
			break;

		case '*':
		case '?':
		case '|':
		case '<':
		case '>':
		case '"':
			if(ext) {
				*ext = 0;
			}
			edll_seterror(edll_err_invalid_filename);
			return -1;

		/* avoid strcasecmp() later [should we do that in Unicode?] */
		case 'A' ... 'Z':
			*d++ = c | 0x20;
			continue;

#if __CYGWIN__
		case '/':
			has_ext = 0;
			if(d > name && d[-1] == '/') {
				/* avoid a double / */
				continue;
			}
			break;

#else
		case '/':
			has_ext = 0;
			if(d > name && d[-1] == '\\') {
				/* avoid a double \ */
				continue;
			}

			/* accept Unix like paths but fix them
			 * to use with MS-Windows LoadLibrary()
			 */
			c = '\\';
			break;

		case '\\':
		case ':':
			has_ext = 0;
			if(d > name && d[-1] == '\\') {
				/* avoid a double \ */
				continue;
			}
			break;

		}
#endif
		if(len == 0) {
			if(ext) {
				*ext = 0;
			}
			/* overflow, we can't safely use this name */
			edll_seterror(edll_err_filename_overflow);
			return -1;
		}
		*d++ = c;
		len--;
	}
	if(!has_ext && ext) {
		/* avoid the auto-.DLL'izing by Windows */
		*d++ = '.';
	}
	d[0] = '\0';

#if __CYGWIN__
	/*
	 * TODO: needs to be tested; could it be that we would need
	 *	 to canonilize AFTER this call instead?
	 *
	 * Note that the cygwin_conv_to_full_win32_path() function
	 * doesn't handle long file names (those of more than
	 * MAX_PATH characters)
	 */
	{
		char winpath[maxsize];
		cygwin_conv_to_full_win32_path(name, winpath);
		strncpy(name, winpath, maxsize);
	}
#endif

	return 0;
}






/************************************************************/
/************************************************************/
/*** SEARCH PATH FUNCTIONS                                ***/
/************************************************************/
/************************************************************/

static char			*g_searchpaths;






int edll_setsearchpath(const char *path)
{
	char		*p, *d, name[MAX_PATH * 2], temp[MAX_PATH * 2];
	const char	*s;
	int		len, r;

	len = edll_strlen(path) * 3;
	if(len == 0) {
		/* this is like reseting to the default */
		edll_lock();
		edll_free(g_searchpaths);
		g_searchpaths = 0;
		edll_unlock();
		return 0;
	}

	p = edll_alloc(len, 0);
	if(!p) {
		return -1;
	}
	*p = '\0';

	s = path;
	while(*s != '\0') {
		d = temp;
		while(*s != '\0' && *s != ';') {
			if(d - temp >= sizeof(temp) - 1) {
				edll_seterror(edll_err_filename_overflow);
				goto error;
			}
			*d++ = *s++;
		}
		if(d > temp) {
			if(*p != '\0') {
				r = edll_strcat(p, ";", len);
				if(r != 0) {
					goto error;
				}
			}
			*d = '\0';
			r = edll_canonize_filename(name, sizeof(name), 0, temp);
			if(r != 0) {
				goto error;
			}
			r = edll_strcat(p, name, len);
			if(r != 0) {
				goto error;
			}
		}
		while(*s == ';') {
			s++;
		}
	}

	edll_lock();
	edll_free(g_searchpaths);
	g_searchpaths = p;
	edll_unlock();

	return 0;

error:
	edll_free(p);

	return -1;
}


char *edll_getsearchpath(void)
{
	char		*p, *path;
	int		len, r;

	if(!g_searchpaths) {
		/* TODO:
		 * Change the path "." to where the executable is started
		 * from which is much more sensical! Also, instead of "."
		 * we should use getcwd() because "." is not a full path.
		 */

		/* create the default search path which is ".;%PATH%" */
		path = getenv("PATH");
		len = edll_strlen(path);
		p = edll_alloc(len + 3, 0);
		p[0] = '.';
		p[1] = ';';
		memcpy(p + 2, path, len + 1);
		r = edll_setsearchpath(p);
		if(r != 0) {
			return 0;
		}
	}

	edll_lock();
	p = edll_strdup(g_searchpaths);
	edll_unlock();

	return p;
}



int edll_find_file(char *name, int maxlen, const char *filename)
{
	char		c, *p, *s, *e;
	int		len, r;

/*
 * If the filename includes a full path already, we don't want
 * to change it
 */
	if(filename[0] == '\\'
	|| (((filename[0] >= 'a' && filename[0] <= 'z')
		|| (filename[0] >= 'A' && filename[0] <= 'Z'))
				&& filename[1] == ':')) {
		if(access(filename, R_OK) == 0) {
			return 0;
		}
		edll_seterror(edll_err_cant_find_module);
		return -1;
	}

/* get the user paths */
	p = edll_getsearchpath();
	if(!p) {
		return -1;
	}

	len = edll_strlen(filename);

	r = -1;
	s = p;
	while(*s != '\0') {
		e = s;
		while(*e != '\0' && *e != ';') {
			e++;
		}
		c = *e;
		*e = '\0';
		if(edll_strlen(s) + len + 2 > maxlen) {
			edll_seterror(edll_err_filename_overflow);
			break;
		}
		snprintf(name, maxlen, "%s\\%s", s, filename);
		if(access(name, R_OK) == 0) {
			r = 0;
			break;
		}
		s = e;
		if(c != '\0') {
			s++;
		}
	}

	edll_free(p);

	if(r != 0) {
		edll_seterror(edll_err_cant_find_module);
	}

	return r;
}







/* vim: ts=8
 */
