/* string.c -- Enhanced Dynamic Library for MS-Windows
 * Low level string handling helper function
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
/*** SOME STRING FUNCTIONS                                ***/
/************************************************************/
/************************************************************/

int edll_strlen(const char *str)
{
	if(str) {
		return strlen(str);
	}

	return 0;
}


char *edll_strdup(const char *str)
{
	int	len;
	char	*result;

	len = edll_strlen(str) + 1;

	result = edll_alloc(len, 0);
	if(result) {
		if(str) {
			memcpy(result, str, len);
		}
		else {
			result[0] = '\0';
		}
	}

	return result;
}


#if 0
int edll_strcmp(char *src1, const char *src2)
{
	if(src1 && *src1) {
		if(src2 && *src2) {
			return strcmp(src1, src2);
		}
		/* src1 > empty string */
		return 1;
	}
	else if(src2 && *src2) {
		/* empty string < src2 */
		return -1;
	}

	/* two empty strings are equal */
	return 0;
}
#endif


int edll_strcasecmp(char *src1, const char *src2)
{
	if(src1 && *src1) {
		if(src2 && *src2) {
			return strcasecmp(src1, src2);
		}
		/* src1 > empty string */
		return 1;
	}
	else if(src2 && *src2) {
		/* empty string < src2 */
		return -1;
	}

	/* two empty strings are equal */
	return 0;
}


int edll_strcat(char *dst, const char *src, int maxlen)
{
	if(!dst) {
		edll_seterror(edll_err_invalid_string);
		return -1;
	}

	while(*dst != '\0') {
		if(maxlen <= 0) {
			edll_seterror(edll_err_invalid_string);
			return -1;
		}
		dst++;
		maxlen--;
	}

	if(src) {
		while(*src != '\0') {
			if(maxlen <= 0) {
				edll_seterror(edll_err_invalid_string);
				return -1;
			}
			*dst++ = *src++;
			maxlen--;
		}
	}

	if(maxlen <= 0) {
		edll_seterror(edll_err_invalid_string);
		return -1;
	}
	*dst = '\0';

	return 0;
}





/* vim: ts=8
 */
