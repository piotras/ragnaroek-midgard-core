%{
/* $Id: preparse.l 20721 2009-02-28 19:48:04Z jval $
Copyright (C) 2000 Emile Heyns <emile@iris-advies.com>

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <config.h>
#include "midgard/pageresolve.h"
#include "midgard/midgard_legacy.h"

static mgd_parser_itf_t mgd_parser_itf;

#define mgd_parser_output_bytes(buffer, len) mgd_parser_itf.output.func(buffer, len, mgd_parser_itf.output.userdata)
#define mgd_parser_output_str(string) mgd_parser_itf.output.func(string, strlen(string), mgd_parser_itf.output.userdata)
#define mgd_parser_output_const(string) mgd_parser_itf.output.func(string, sizeof(string)-1, mgd_parser_itf.output.userdata)
#define mgd_parser_output_char(charp) mgd_parser_itf.output.func(charp, 1, mgd_parser_itf.output.userdata)

static GArray *parser_stack;
static int paren_stack = 0;

#if MIDGARD_PARSER_DEBUG
static int other_match = 0;
#endif

static void mgd_fmt_start(char *fmt)
{
   switch (*fmt) {
      case 'p':
      case 'P':
         mgd_parser_output_const("eval(' ?>'.mgd_preparse");
         break;

      case 'x':
         mgd_parser_output_const("mgd_execute_udf");
         break;

      default:
         mgd_parser_output_const("echo mgd_format");
         break;
   }
   mgd_parser_output_const("($");
}

static void mgd_fmt_end(char *fmt)
{
   switch (*fmt) {
      case 'p':
      case 'P':
         mgd_parser_output_const(").'<?php '");
         break;

      case 'x':
         mgd_parser_output_const(",'");
         mgd_parser_output_str(fmt + 1); /*[eeh] skip 'x' */
         mgd_parser_output_const("'");
	 break;
      case '\0':
         break;

      default:
         mgd_parser_output_const(", '");
         mgd_parser_output_str(fmt);
         mgd_parser_output_const("'");
         break;
   }

   mgd_parser_output_const(");");
}

static void mgd_preparse_ref(int mode) {
  char *guid;
  char *attrx, *attry;
  char *label;
  char *end;
  char *quot;
  char *textcpy = malloc ((mgdlibleng + 1) * sizeof(char));
  strcpy(textcpy, mgdlibtext);
  if ((attrx = strchr(mgdlibtext, ' ')) != NULL) {
    if (attrx < mgdlibtext + mgdlibleng - 1) {
      if ((guid = strstr(attrx, "\"guid://")) != NULL || (guid = strstr(attrx, "'guid://")) != NULL) {
	*guid = '\0';
	guid+=8;
	if (guid < mgdlibtext + mgdlibleng - 1) {
	  if ((attry = strchr(guid, '\"')) != NULL || (attry = strchr(guid, '\'')) != NULL) {
	    if (*(attry-1) == '/') {
	      *(attry-1) = '\0';
	    } else {
	      *attry = '\0';
	    }
	    attry++;
	    if (attry < mgdlibtext + mgdlibleng - 1 || mode) {
	      if ((label = strchr(attry, '>')) != NULL) {
		*label = '\0';
		label++;
                if (mode == 1) {
		mgd_parser_output_const("<?=mgd_ref(");
		mgd_parser_output_str(mode?"1,'":"0,'");
		mgd_parser_output_str(guid);
		mgd_parser_output_const("','");
		quot = attrx;
		while ((quot = strchr(quot, '\'')) != NULL) {
		  *quot = '"';
		}
		mgd_parser_output_str(attrx);
		mgd_parser_output_const("','");
		quot = attry;
		while ((quot = strchr(quot, '\'')) != NULL) {
		  *quot = '"';
		}
		mgd_parser_output_str(attry);
		  mgd_parser_output_const("')?>");
		} else {
		  if (label < mgdlibtext + mgdlibleng - 1) {
		    if ((end = strstr(label, "</a")) != NULL || (end = strstr(label, "</A")) != NULL) {
		      *end = '\0';
		      quot = label;
		      while ((quot = strchr(quot, '\'')) != NULL) {
			*quot = '"';
		      }
		      mgd_parser_output_const("<?php ob_start(); eval(' ?>'.mgd_preparse('");
		      mgd_parser_output_str(label);
		      mgd_parser_output_const("').'<?php '); $PMEarlbQW91234012PIGGY = ob_get_contents(); ob_end_clean();");
		      mgd_parser_output_const("echo mgd_ref(");
		      mgd_parser_output_str(mode?"1,'":"0,'");
		      mgd_parser_output_str(guid);
		      mgd_parser_output_const("','");
		      quot = attrx;
		      while ((quot = strchr(quot, '\'')) != NULL) {
			*quot = '"';
		      }
		      mgd_parser_output_str(attrx);
		      mgd_parser_output_const("','");
		      quot = attry;
		      while ((quot = strchr(quot, '\'')) != NULL) {
			*quot = '"';
		      }
		      mgd_parser_output_str(attry);
		      mgd_parser_output_const("',$PMEarlbQW91234012PIGGY); unset($PMEarlbQW91234012PIGGY); ?>");
		    } else {
		      mgd_parser_output_str(textcpy);     
		    }
		  } else {
		    mgd_parser_output_str(textcpy);     
		  }
		}	    
	      } else {
		mgd_parser_output_str(textcpy);     
	      }
	    } else {
	      mgd_parser_output_str(textcpy);     
	    }
	  } else {
	    mgd_parser_output_str(textcpy);     
	  }
	} else {
	  mgd_parser_output_str(textcpy);     
	}
      } else {
	mgd_parser_output_str(textcpy);     
      }
    } else {
      mgd_parser_output_str(textcpy);     
    }
  }    else {
    mgd_parser_output_str(textcpy);     
  }
  free (textcpy);
}

#if MIDGARD_PARSER_DEBUG
static void mgd_parser_log_other()
{
   if (other_match == 0) return;

   fprintf(stderr, "MidgardParser: %d other\n", other_match);
   other_match = 0;
}
#endif
%}

%x IN_PHP
%x IN_PHPQ
%x IN_PHPQQ
%x IN_LCOMMENT
%x IN_BCOMMENT
%x IN_SNIPPET
%x IN_SNIPPETQ
%x IN_SNIPPETQQ
%x IN_EVAL
%x IN_EVALQ
%x IN_EVALQQ

%option noyylineno
%option stack

dqstring   [\"][^\"\n]*[\"]
qstring    [\'][^\'\n]*[\']
ANY        .|{WHITESPACE}
LCOMMENT1  "//"
LCOMMENT2  "#"
quote      [\']
dquote     [\"]
WHITESPACE [ \n\r\t]
MIDGARD_TEMPLATE_NAME [-_ a-zA-Z0-9\x7f-\xff]+
MIDGARD_TEMPLATE (("<["{MIDGARD_TEMPLATE_NAME}"]>")|("<("{MIDGARD_TEMPLATE_NAME}")>"))
MIDGARD_VARPART (("_"[a-zA-Z0-9_]+)|([a-zA-Z_][a-zA-Z0-9_]*))
MIDGARD_FMTSPEC [a-zA-Z]*
MIDGARD_ARRINDEX ({MIDGARD_VARPART}|[0-9]+|{dqstring}|{qstring})
MIDGARD_VARSPEC_ATTR ((("."|"->"){MIDGARD_VARPART})|("["{MIDGARD_ARRINDEX}"]"))
MIDGARD_VARSPEC_FMT ":"{MIDGARD_FMTSPEC}
MIDGARD_VARSPEC {MIDGARD_VARPART}{MIDGARD_VARSPEC_ATTR}?{MIDGARD_VARSPEC_FMT}?
MIDGARD_LINK_ATTRS [^>]*
MIDGARD_LINK_LABEL ([^<]|"</"[^aA]|"<"[^/])*
MIDGARD_HYPERLINK "<"[aA]{MIDGARD_LINK_ATTRS}[hH][rR][eE][fF]"="({quote}|{dquote})"guid://"[^\"\']+({quote}|{dquote}){MIDGARD_LINK_ATTRS}">"{MIDGARD_LINK_LABEL}"</"[aA]">"
MIDGARD_IMG "<"[iI][mM][gG]{MIDGARD_LINK_ATTRS}[sS][rR][cC]"="({quote}|{dquote})"guid://"[^\"\']+({quote}|{dquote}){MIDGARD_LINK_ATTRS}">"



%%

<INITIAL>{MIDGARD_IMG} {
  mgd_preparse_ref(1);
}

<INITIAL>{MIDGARD_HYPERLINK} {
  mgd_preparse_ref(0);
}

<INITIAL>{MIDGARD_TEMPLATE}[\r\n]? {
   const char *content = NULL;
   YY_BUFFER_STATE bstate;

   /* skip trainling newline */
   if (mgdlibtext[mgdlibleng - 1] != '>') mgdlibleng--;

   /* truncate string to skip ]> */
   mgdlibtext[mgdlibleng - 2] = '\0';

#if MIDGARD_PARSER_DEBUG
   mgd_parser_log_other(); fprintf(stderr, "MidgardParser: template <[%s]>\n",  (const char*)mgdlibtext + 2);
#endif
   /* skip first two chars to get name, get content */
   if (mgd_parser_itf.get_element.func != NULL)
      content = mgd_parser_itf.get_element.func((const char*)mgdlibtext + 2, mgd_parser_itf.get_element.userdata);

   if (content != NULL) {
      bstate = yy_scan_string(content);
      g_array_append_val(parser_stack, bstate);
   }
}

<INITIAL>"&("{MIDGARD_VARSPEC}");" {
   char *fmt;
   char *attr;

   /* truncate string to skip ); */
   mgdlibtext[mgdlibleng - 2] = '\0';
   mgdlibtext += 2;

#if MIDGARD_PARSER_DEBUG
   mgd_parser_log_other(); fprintf(stderr, "MidgardParser: varformat &(%s);\n", mgdlibtext);
#endif
   if ((fmt = strchr(mgdlibtext, ':')) != NULL) {
      *fmt = '\0';
      fmt++;
   } else {
      fmt = "";
   }

   if ((attr = strchr(mgdlibtext, '.')) != NULL) {
      *attr = '\0';
      attr++;

      mgd_parser_output_const("<?php ");

#if MIDGARD_PREPARSER_FUDGE_ARRAYS
      mgd_parser_output_const("if (is_object($");
      mgd_parser_output_str(mgdlibtext);
      mgd_parser_output_const(")) { ");
#endif

      mgd_fmt_start(fmt);

      mgd_parser_output_str(mgdlibtext);
      mgd_parser_output_const("->");
      mgd_parser_output_str(attr);

      mgd_fmt_end(fmt);

#if MIDGARD_PREPARSER_FUDGE_ARRAYS
      mgd_parser_output_const(" } else { ");

      mgd_fmt_start(fmt);

      mgd_parser_output_str(mgdlibtext);
      mgd_parser_output_const("[");
      if (!isdigit(*attr)) mgd_parser_output_const("$");
      mgd_parser_output_str(attr);
      mgd_parser_output_const("]");

      mgd_fmt_end(fmt);
      mgd_parser_output_const(" }");
#endif
      mgd_parser_output_const(" ?>");
   } else {
      mgd_parser_output_const("<?php ");

      mgd_fmt_start(fmt);

      mgd_parser_output_str(mgdlibtext);

      mgd_fmt_end(fmt);

      mgd_parser_output_const(" ?>");
   }
}

<INITIAL>"<?php"|"<?"|"<script"{WHITESPACE}+"language"{WHITESPACE}*"="{WHITESPACE}*("php"|"\"php\""|"\'php\'"){WHITESPACE}*">" {
#if MIDGARD_PARSER_DEBUG
   mgd_parser_log_other(); fprintf(stderr, "MidgardParser: start PHP\n");
#endif
   mgd_parser_output_bytes(mgdlibtext, mgdlibleng);
   yy_push_state(IN_PHP);
}

<INITIAL>{ANY} {
#if MIDGARD_PARSER_DEBUG
   other_match++;
#endif
   mgd_parser_output_char(mgdlibtext);
}

<IN_PHP>("?>"|"</script"{WHITESPACE}*">") {
#if MIDGARD_PARSER_DEBUG
   mgd_parser_log_other(); fprintf(stderr, "MidgardParser: end PHP\n");
#endif
   mgd_parser_output_bytes(mgdlibtext, mgdlibleng);
   yy_pop_state();
}

<IN_PHP>"mgd_eval"{WHITESPACE}*"(" {
  mgd_parser_output_bytes(mgdlibtext + 4, mgdlibleng - 4);
  mgd_parser_output_const("' ?>'.mgd_preparse('<?php '.");
  yy_push_state(IN_EVAL);
  paren_stack=0;
}

<IN_PHP>"eval"{WHITESPACE}*"(" {
  mgd_parser_output_bytes(mgdlibtext, mgdlibleng);
#if MIDGARD_PARSER_HACK_EVAL
  mgd_parser_output_const("' ?>'.mgd_preparse('<?php '.");
  yy_push_state(IN_EVAL);
  paren_stack=0;
#endif
}

<IN_PHP>{quote} {
#if MIDGARD_PARSER_DEBUG
   mgd_parser_log_other(); fprintf(stderr, "MidgardParser: PHP quote\n");
#endif
   yy_push_state(IN_PHPQ);
   mgd_parser_output_char(mgdlibtext);
}

<IN_PHP>{dquote} {
#if MIDGARD_PARSER_DEBUG
   mgd_parser_log_other(); fprintf(stderr, "MidgardParser: PHP dquote\n");
#endif
   yy_push_state(IN_PHPQQ);
   mgd_parser_output_char(mgdlibtext);
}

<IN_PHPQ>"\\\\" {
#if MIDGARD_PARSER_DEBUG
   mgd_parser_log_other(); fprintf(stderr, "MidgardParser: PHP escape\n");
#endif
   mgd_parser_output_bytes(mgdlibtext, mgdlibleng);
}

<IN_PHPQ>"\\"{quote} {
#if MIDGARD_PARSER_DEBUG
   mgd_parser_log_other(); fprintf(stderr, "MidgardParser: PHP esc-quote\n");
#endif
   mgd_parser_output_bytes(mgdlibtext, mgdlibleng);
}

<IN_PHPQ>{quote} {
#if MIDGARD_PARSER_DEBUG
   mgd_parser_log_other(); fprintf(stderr, "MidgardParser: PHP end quote\n");
#endif
   mgd_parser_output_bytes(mgdlibtext, mgdlibleng);
   yy_pop_state();
}

<IN_PHPQ>{ANY} {
#if MIDGARD_PARSER_DEBUG
   other_match++;
#endif
   mgd_parser_output_char(mgdlibtext);
}

<IN_PHPQQ>"\\\\" {
#if MIDGARD_PARSER_DEBUG
   mgd_parser_log_other(); fprintf(stderr, "MidgardParser: PHP escape\n");
#endif
   mgd_parser_output_bytes(mgdlibtext, mgdlibleng);
}


<IN_PHPQQ>"\\"{dquote} {
#if MIDGARD_PARSER_DEBUG
   mgd_parser_log_other(); fprintf(stderr, "MidgardParser: PHP esc-dquote\n");
#endif
   mgd_parser_output_bytes(mgdlibtext, mgdlibleng);
}

<IN_PHPQQ>{dquote} {
#if MIDGARD_PARSER_DEBUG
   mgd_parser_log_other(); fprintf(stderr, "MidgardParser: PHP end dquote\n");
#endif
   mgd_parser_output_char(mgdlibtext);
   yy_pop_state();
}

<IN_PHPQQ>{ANY} {
#if MIDGARD_PARSER_DEBUG
   other_match++;
#endif
   mgd_parser_output_char(mgdlibtext);
}

<IN_PHP>({LCOMMENT1}|{LCOMMENT2}) {
#if MIDGARD_PARSER_DEBUG
   mgd_parser_log_other(); fprintf(stderr, "MidgardParser: PHP lcomment\n");
#endif

   mgd_parser_output_bytes(mgdlibtext, mgdlibleng);
   yy_push_state(IN_LCOMMENT);
}

<IN_LCOMMENT>("\n"|"?>"|"</script"{WHITESPACE}*">") {
#if MIDGARD_PARSER_DEBUG
   other_match++;
#endif
   mgd_parser_output_bytes(mgdlibtext, mgdlibleng);
   yy_pop_state(); // pop IN_LCOMMENT
   if(*mgdlibtext != '\n') yy_pop_state(); // pop IN_PHP
}

<IN_LCOMMENT>{ANY} {
#if MIDGARD_PARSER_DEBUG
   other_match++;
#endif
   mgd_parser_output_char(mgdlibtext);
}

<IN_PHP>"/*" {
#if MIDGARD_PARSER_DEBUG
   mgd_parser_log_other(); fprintf(stderr, "MidgardParser: PHP bcomment\n");
#endif
   mgd_parser_output_bytes(mgdlibtext, mgdlibleng);
   yy_push_state(IN_BCOMMENT);
}

<IN_BCOMMENT>"*/" {
#if MIDGARD_PARSER_DEBUG
   mgd_parser_log_other(); fprintf(stderr, "MidgardParser: PHP end bcomment\n");
#endif
   mgd_parser_output_bytes(mgdlibtext, mgdlibleng);
   yy_pop_state(); // pop IN_BCOMMENT
}

<IN_BCOMMENT>{ANY} {
#if MIDGARD_PARSER_DEBUG
   other_match++;
#endif
   mgd_parser_output_char(mgdlibtext);
}

<IN_PHP>"mgd_include_snippet"{WHITESPACE}*"(" {
#if MIDGARD_PARSER_DEBUG
   mgd_parser_log_other(); fprintf(stderr, "MidgardParser: mgd_include_snippet\n");
#endif
   yy_push_state(IN_SNIPPET);
   paren_stack = 0;
   mgd_parser_output_const("eval(' ?>'.mgd_preparse(mgd_snippet(");
}

<IN_PHP>"mgd_require_snippet"{WHITESPACE}*"(" {
#if MIDGARD_PARSER_DEBUG
   mgd_parser_log_other(); fprintf(stderr, "MidgardParser: mgd_require_snippet\n");
#endif
   yy_push_state(IN_SNIPPET);
   paren_stack = 0;
   mgd_parser_output_const("eval(' ?>'.mgd_preparse(mgd_snippet_required(");
}

<IN_PHP>"mgd_show_element"{WHITESPACE}*"(" {
#if MIDGARD_PARSER_DEBUG
        mgd_parser_log_other(); fprintf(stderr, "MidgardParser: mgd_show_element\n");
#endif
        yy_push_state(IN_SNIPPET);
        paren_stack = 0;
        mgd_parser_output_const("eval(' ?>'.mgd_preparse(mgd_template(");
}

<IN_PHP>{ANY} {
#if MIDGARD_PARSER_DEBUG
   other_match++;
#endif
   mgd_parser_output_char(mgdlibtext);
}

<IN_SNIPPET>{quote} {
#if MIDGARD_PARSER_DEBUG
   other_match++;
#endif
  yy_push_state(IN_SNIPPETQ);
  mgd_parser_output_char(mgdlibtext);
}

<IN_SNIPPET>{dquote} {
#if MIDGARD_PARSER_DEBUG
   other_match++;
#endif
  yy_push_state(IN_SNIPPETQQ);
  mgd_parser_output_char(mgdlibtext);
}

<IN_SNIPPETQ>[\\]{quote} {
#if MIDGARD_PARSER_DEBUG
   other_match++;
#endif
  mgd_parser_output_str(mgdlibtext);
}

<IN_SNIPPETQ>{quote} {
#if MIDGARD_PARSER_DEBUG
   other_match++;
#endif
  mgd_parser_output_char(mgdlibtext);
  yy_pop_state();
}

<IN_SNIPPETQ>{ANY} {
#if MIDGARD_PARSER_DEBUG
   other_match++;
#endif
  mgd_parser_output_char(mgdlibtext);
}

<IN_SNIPPETQQ>[\\]{dquote} {
#if MIDGARD_PARSER_DEBUG
   other_match++;
#endif
  mgd_parser_output_str(mgdlibtext);
}

<IN_SNIPPETQQ>{dquote} {
#if MIDGARD_PARSER_DEBUG
   other_match++;
#endif
  mgd_parser_output_char(mgdlibtext);
  yy_pop_state();
}

<IN_SNIPPETQQ>{ANY} {
#if MIDGARD_PARSER_DEBUG
   other_match++;
#endif
  mgd_parser_output_char(mgdlibtext);
}

<IN_SNIPPET>{ANY} {
#if MIDGARD_PARSER_DEBUG
   other_match++;
#endif
  if(mgdlibtext[0] == '(') paren_stack++;
  if(mgdlibtext[0] == ')') paren_stack--;
  mgd_parser_output_char(mgdlibtext);
  if(paren_stack < 0) {
    mgd_parser_output_const(").'<?php ')");
    yy_pop_state();
  }
}

<IN_EVAL>{quote} {
  yy_push_state(IN_EVALQ);
  mgd_parser_output_char(mgdlibtext);
}

<IN_EVAL>{dquote} {
  yy_push_state(IN_EVALQQ);
  mgd_parser_output_char(mgdlibtext);
}

<IN_EVALQ>[\\]{quote} {
  mgd_parser_output_str(mgdlibtext);
}

<IN_EVALQ>{quote} {
  mgd_parser_output_char(mgdlibtext);
  yy_pop_state();
}

<IN_EVALQ>{ANY} {
  mgd_parser_output_char(mgdlibtext);
}

<IN_EVALQQ>[\\]{dquote} {
  mgd_parser_output_str(mgdlibtext);
}

<IN_EVALQQ>{dquote} {
  mgd_parser_output_char(mgdlibtext);
  yy_pop_state();
}

<IN_EVALQQ>{ANY} {
  mgd_parser_output_char(mgdlibtext);
}

<IN_EVAL>{ANY} {
  if(yytext[0] == '(') paren_stack++;
  if(yytext[0] == ')') paren_stack--;
  if(paren_stack < 0) {
    mgd_parser_output_const(".' ?>').'<?php ')");
    yy_pop_state();
  } else {
    mgd_parser_output_char(mgdlibtext);
  }
}

%%

void mgd_init_parser_itf(mgd_parser_itf_t *itf)
{
}

void mgd_preparse_buffer(const char *buffer, mgd_parser_itf_t *itf)
{
   YY_BUFFER_STATE bstate;

   if (itf->output.func == NULL) return;

   mgd_parser_itf.output.func = itf->output.func;
   mgd_parser_itf.output.userdata = itf->output.userdata;
   mgd_parser_itf.get_element.func = itf->get_element.func;
   mgd_parser_itf.get_element.userdata = itf->get_element.userdata;

   mgdlibin = NULL;

   parser_stack = g_array_new(FALSE, FALSE, sizeof(YY_BUFFER_STATE));
   bstate = yy_scan_string(buffer);
   g_array_append_val(parser_stack, bstate);
   BEGIN(INITIAL);
   mgdliblex();
}

void mgd_preparse_file(FILE *f, mgd_parser_itf_t *itf)
{
   YY_BUFFER_STATE bstate;

   if (itf->output.func == NULL || f == NULL) return;

   mgd_parser_itf.output.func = itf->output.func;
   mgd_parser_itf.output.userdata = itf->output.userdata;
   mgd_parser_itf.get_element.func = itf->get_element.func;
   mgd_parser_itf.get_element.userdata = itf->get_element.userdata;

   /*[eeh] do not set this to the input file, or the final pop in
    * yywrap will crash
    */
   mgdlibin = NULL;

   parser_stack = g_array_new(FALSE, FALSE, sizeof(YY_BUFFER_STATE));
   bstate = yy_create_buffer(f, YY_BUF_SIZE);
   yy_switch_to_buffer(bstate);
   g_array_append_val(parser_stack, bstate);
   BEGIN(INITIAL);
   mgdliblex();
}

int mgdlibwrap()
{
YY_BUFFER_STATE bstate;

   if (parser_stack->len <= 0) {
      fprintf(stderr, "MidgardParser: fell out of bottom of parser stack!\n");
      return 1;
   }

   bstate = g_array_index(parser_stack, YY_BUFFER_STATE, parser_stack->len - 1);
   yy_delete_buffer(bstate);
   g_array_remove_index(parser_stack, parser_stack->len - 1);

   if (parser_stack->len == 0) return 1;

   bstate = g_array_index(parser_stack, YY_BUFFER_STATE, parser_stack->len - 1);
   yy_switch_to_buffer(bstate);
   return 0;

/*[eeh]  This needs looking into. It consumes &&(var); I can see that it
         does but don't know how to rewrite the regexp to fix it

<INITIAL>({WHITESPACE}|[^<&]|[<&][^:s?\[\(]){1,400} {
   fprintf(stderr, "[[%s]]", mgdlibtext);
   mgd_parser_output_bytes(mgdlibtext, mgdlibleng);
}
*/

}
