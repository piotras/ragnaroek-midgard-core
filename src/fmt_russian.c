/* $Id: fmt_russian.c 9733 2006-05-30 13:10:00Z piotras $
 *
 * fmt_russian: Raw formatter (used for russian encodings and UTF-8 texts)
 *
 * Copyright (C) 1999 Jukka Zitting <jukka.zitting@iki.fi>
 * Copyright (C) 2000 The Midgard Project ry
 * Copyright (C) 1999-2003 Alexander Bokovoy <ab@altlinux.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <config.h>
#include "midgard/parser.h"
#include "fmt_russian.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

static int addplain(mgd_parser * parser, const char *str, int html)
{
	if (!str)
		return 0;
	while (*str) {
		for (; *str && (!html || (*str != '[' || *(str + 1) != '<'));
		     str++)
			if (*str == '&')
				mgd_parser_addstr(parser, "&amp;");
			else if (*str == '<')
				mgd_parser_addstr(parser, "&lt;");
			else if (*str == '>')
				mgd_parser_addstr(parser, "&gt;");
			else if (*str == '\"')
				mgd_parser_addstr(parser, "&quot;");
			else
				mgd_parser_addchar(parser, *str);
		if (*str && html && *str == '[' && *(str + 1) == '<')
			for (str += 2;
			     *str && (*str != '>' || *(str + 1) != ']'); str++)
				mgd_parser_addchar(parser, *str);
		if (*str && html && *str == '>' && *(str + 1) == ']')
			str += 2;
	}
	return 1;
}

static int addhtml(mgd_parser * parser, const char *str)
{
	if (!str)
		return 0;
	for (; *str; str++)
		mgd_parser_addchar(parser, *str);
	return 1;
}

static int mgd_parser_raw_p(mgd_parser * parser, void *data)
{
	return addplain(parser, (const char *) data, 0);
}

static int mgd_parser_raw_P(mgd_parser * parser, void *data)
{
	return addplain(parser, (const char *) data, 1);
}

static int mgd_parser_raw_h(mgd_parser * parser, void *data)
{
	return addplain(parser, (const char *) data, 1);
}

static int mgd_parser_raw_H(mgd_parser * parser, void *data)
{
	return addhtml(parser, (const char *) data);
}

mgd_parser *mgd_parser_init_raw(const char* name, const char* encoding)
{
	mgd_parser *parser;
	parser = mgd_parser_create(name, encoding, 0);
	mgd_parser_setcallback(parser, 'p', MGD_STR, mgd_parser_raw_p);
	mgd_parser_setcallback(parser, 'P', MGD_STR, mgd_parser_raw_P);
	mgd_parser_setcallback(parser, 'h', MGD_STR, mgd_parser_raw_h);
	mgd_parser_setcallback(parser, 'H', MGD_STR, mgd_parser_raw_H);
	return parser;
}
