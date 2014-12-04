/* $Id: parser.h 10354 2006-12-07 00:22:45Z piotras $
 *
 * midgard-lib: database access for Midgard clients
 *
 * Copyright (C) 1999 Jukka Zitting <jukka.zitting@iki.fi>
 * Copyright (C) 2000 The Midgard Project ry
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
#ifndef MIDGARD_PARSER_H
#define MIDGARD_PARSER_H
#include <ctype.h> 
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include "midgard/midgard_defs.h"
#include "midgard/pool.h"

typedef struct _mgd_parser mgd_parser;
/* General callback function that processes addition to format buffer
   Each parser shoud have several predefined callbacks:
   'd': add integer number
   'D': add integer array
   'i': add integer number
   't': add date
   's': add string
   'q': add sql query
   'h': add html code
   'p': add plain text without without HTML conversion
   'P': add plain text with HTML conversion
   'f': add formatted text
   'F': add formatted text with headers
   'u': add url-encoded string
   By default, they will be initialized to Latin1 parser
*/
typedef int (*mgd_parser_callback)(mgd_parser *parser, void *data);

typedef enum {
    MGD_CHAR,
    MGD_INT,
    MGD_INTPTR,
    MGD_STR,
    MGD_PTR
} mgd_parser_type;

typedef struct {
    mgd_parser_callback func;
    mgd_parser_type ptype;
} mgd_parser_callback_info;

struct _mgd_parser {
    mgd_parser *next;
    char *name, *encoding;
    int need_qp;
    midgard_pool *pool;
    mgd_parser_callback_info *callbacks; 
};

extern mgd_parser* mgd_parser_create(const char *name, const char *encoding, int need_qp);
extern void mgd_parser_free(mgd_parser *parser);
extern void mgd_parser_free_all();
extern void mgd_parser_setcallback(mgd_parser *parser, char symbol, 
				    mgd_parser_type ptype,
				    mgd_parser_callback callback);
extern void mgd_parser_clearcallback(mgd_parser *parser, char symbol);
extern mgd_parser* mgd_parser_get(const char *name);
extern mgd_parser* mgd_parser_list();
extern int mgd_parser_activate(midgard *mgd, const char *name);

/* These are use explicit parser */
extern char *mgd_format_ext(mgd_parser *parser, midgard_pool *pool, const char *fmt, ...);
extern char *mgd_vformat_ext(mgd_parser *parser, midgard_pool *pool, const char *fmt, va_list args);

/* Parser API */

/* Hexadecimal table for conversions */
extern char mgd_parser_HexTable[16];

/* Add char to format buffer */
extern int mgd_parser_addchar(mgd_parser *parser, char ch);
/* Acc char to format buffer */
extern int mgd_parser_accchar(mgd_parser *parser, char ch);
/* Add number to format buffer */
extern int mgd_parser_addint(mgd_parser *parser, long num);
/* Add date to format buffer */
extern int mgd_parser_adddate(mgd_parser *parser, const char *str);
/* Add string to format buffer */
extern int mgd_parser_addstr(mgd_parser *parser, const char *str);
/* Add sql query to format buffer */
extern int mgd_parser_addsql(mgd_parser *parser, const char *str);
/* Add formatted text to format buffer */
extern int mgd_parser_addtext(mgd_parser *parser, const char *str, int headers);
/* Add url-encoded string to format buffer */
extern int mgd_parser_addurl(mgd_parser *parser, const char *str);

#define MGD_ADDPLAIN(p, s, h) p->callbacks[h ? 'P':'p'].func(p,(void*)(s))
	
#endif /* MIDGARD_RUSSIAN_H */
