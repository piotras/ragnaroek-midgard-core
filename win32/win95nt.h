/* $Id: win95nt.h 6870 2002-08-19 15:22:50Z daniel $
 *
 * Defines and types for Windows 95/NT
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

#define HAVE_DECLARED_TIMEZONE
#define S_ISREG(mode)	(((mode) & S_IFMT) == S_IFREG)
#define strcasecmp(s1, s2) stricmp(s1, s2)
#define SIGPIPE 17
#define sleep(t) Sleep(t*1000)
#define getpid		_getpid
#define crypt(buf,salt)	    (buf)
