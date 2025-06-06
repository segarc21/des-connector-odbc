// Copyright (c) 2012, 2024, Oracle and/or its affiliates.
// Modified in 2025 by Sergio Miguel Garcia Jimenez <segarc21@ucm.es>
// (see the next block comment below).
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2.0, as
// published by the Free Software Foundation.
//
// This program is designed to work with certain software (including
// but not limited to OpenSSL) that is licensed under separate terms, as
// designated in a particular file or component or in included license
// documentation. The authors of MySQL hereby grant you an additional
// permission to link the program and your derivative works with the
// separately licensed software that they have either included with
// the program or referenced in the documentation.
//
// Without limiting anything contained in the foregoing, this file,
// which is part of Connector/ODBC, is also subject to the
// Universal FOSS Exception, version 1.0, a copy of which can be found at
// https://oss.oracle.com/licenses/universal-foss-exception.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License, version 2.0, for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

// ---------------------------------------------------------
// Modified in 2025 by Sergio Miguel Garcia Jimenez <segarc21@ucm.es>,
// hereinafter the DESODBC developer, in the context of the GPLv2 derivate
// work DESODBC, an ODBC Driver of the open-source DBMS Datalog Educational
// System (DES) (see https://des.sourceforge.io/)
// ---------------------------------------------------------

/**
  @file  parse.h
  @brief utilities to parse queries
*/

#ifndef __MYODBC_PARSE_H__
# define __MYODBC_PARSE_H__

#include <vector>

typedef struct my_string
{
  char *str;
  uint chars; /* probably it is not needed and is useless */
  uint bytes;
} MY_STRING;

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
typedef enum desodbcQueryType
{
  desqtSelect= 0,
  desqtInsert,
  desqtUpdate,
  desqtDelete,
  desqtCall,
  desqtShow,
  desqtUse,        /*5*/
  desqtCreateTable,
  desqtCreateProc,
  desqtCreateFunc,
  desqtDropProc,
  desqtDropFunc,   /*10*/
  desqtOptimize,
  desqtProcess,
  desqtOther       /* Any type of query(including those above) that we do not
                     care about for that or other reason */
} QUERY_TYPE_ENUM;

typedef struct qt_resolving
{
  const MY_STRING *           keyword;
  uint                        pos_from;
  uint                        pos_thru;
  QUERY_TYPE_ENUM             query_type;
  const struct qt_resolving * and_rule;
  const struct qt_resolving * or_rule;
} QUERY_TYPE_RESOLVING;

typedef struct
{
  my_bool       returns_rs;
} DES_QUERY_TYPE;

/* DESODBC:
    Rename and modifications were made.
    Original author: MyODBC
    Modified by: DESODBC Developer
    */
/* To organize constant data needed for parsing - to keep it in required encoding */
typedef struct syntax_markers
{
  const MY_STRING quote[3];
  const MY_STRING query_sep[2];
  const MY_STRING *escape;
  const MY_STRING *odbc_escape_open;
  const MY_STRING *odbc_escape_close;
  const MY_STRING *param_marker;
  const MY_STRING hash_comment;
  const MY_STRING dash_comment;
  const MY_STRING c_style_open_comment;
  const MY_STRING c_style_close_comment;
  const MY_STRING c_var_open_comment;
  const MY_STRING new_line_end;

  struct des_keywords
  {
    const MY_STRING *process;
    const MY_STRING * select;
    const MY_STRING * insert;
    const MY_STRING * update;
    const MY_STRING *del;
    const MY_STRING * call;
    const MY_STRING * show;
    const MY_STRING * use;
    const MY_STRING * create;
    const MY_STRING * drop;
    const MY_STRING * table;
    const MY_STRING * procedure;
    const MY_STRING * function;

    const MY_STRING * where_;
    const MY_STRING * current;
    const MY_STRING * of;
    const MY_STRING * limit;

  } keyword;
  /* TODO: comments */

} DES_SYNTAX_MARKERS;

struct tempBuf {
  char *buf;
  size_t buf_len;
  size_t cur_pos;

  tempBuf(size_t size = 16384);

  tempBuf(const tempBuf &b);

  tempBuf(const char *src, size_t len);

  char *extend_buffer(char *to, size_t len);
  char *extend_buffer(size_t len);

  // Append data to the current buffer
  char *add_to_buffer(const char *from, size_t len);

  char *add_to_buffer(char *to, const char *from, size_t len);
  void remove_trail_zeroes();
  void reset();

  operator bool();

  void operator=(const tempBuf &b);

  ~tempBuf();
};


struct DES_PARSED_QUERY
{
  desodbc::CHARSET_INFO  *cs;                   /* We need it for parsing                  */
  tempBuf buf;
  const char          *query = nullptr;      /* Original query itself                   */
  const char          *query_end = nullptr;  /* query end                               */
  const char          *last_char; /* mainly for remove_braces                */
  //unsigned int  begin;    /* offset 1st meaningful character - 1st token */
  std::vector<uint> token2;     /* positions of tokens            */
  std::vector<uint> param_pos;  /* positions of parameter markers */

  QUERY_TYPE_ENUM query_type;
  const char *  is_batch;   /* Pointer to the begin of a 2nd query in a batch */

  DES_PARSED_QUERY();
  DES_PARSED_QUERY &operator=(const DES_PARSED_QUERY &src);
  ~DES_PARSED_QUERY();

  void reset(char *query, char *query_end, desodbc::CHARSET_INFO *cs);
  const char *get_token(uint index);
  const char *get_param_pos(uint index);
  bool returns_result();
  const char *get_cursor_name();
  size_t token_count();
  bool is_select_statement();
  bool is_insert_statement();
  bool is_update_statement();
  bool is_delete_statement();
  bool is_process_statement();
  size_t length() { return query_end - query; }
};

/* DESODBC:
    Renamed from the original MY_PARSER.
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
typedef struct parser
{
  const char        *pos;
  int               bytes_at_pos;
  int               ctype;
  const MY_STRING   *quote;  /* If quote was open - pointer to the quote char */
  DES_PARSED_QUERY   *query;
  BOOL hash_comment;      /* Comment starts with "#" and end with end of line */
  BOOL dash_comment;      /* Comment starts with "-- " and end with end of line  */
  BOOL c_style_comment;   /* C style comment */

  const DES_SYNTAX_MARKERS *syntax;
} DES_PARSER;

/* Those are taking pointer to DES_PARSED_QUERY as a parameter*/
#define GET_QUERY(pq) (pq)->query
#define GET_QUERY_END(pq) (pq)->query_end
#define GET_QUERY_LENGTH(pq) (GET_QUERY_END(pq) - GET_QUERY(pq))
#define PARAM_COUNT(pq) (pq).param_pos.size()
#define IS_BATCH(pq) ((pq)->is_batch != NULL)

DES_PARSER * init_parser(DES_PARSER *parser, DES_PARSED_QUERY *pq);

/* this will not work for some mb charsets */
#define END_NOT_REACHED(parser) ((parser)->pos < (parser)->query->query_end)
#define BYTES_LEFT(pq, pos) ((pq)->query_end - (pos))
#define CLOSE_QUOTE(parser) (parser)->quote= NULL
#define IS_SPACE(parser) ((parser)->ctype & _MY_SPC)
#define IS_SPL_CHAR(parser) ((parser)->ctype & _MY_CTR)

/* But returns bytes in current character */
int               get_ctype(DES_PARSER *parser);
BOOL              skip_spaces(DES_PARSER *parser);
void              add_token(DES_PARSER *parser);
BOOL              is_escape(DES_PARSER *parser);
const MY_STRING * is_quote(DES_PARSER *parser);
BOOL              open_quote(DES_PARSER *parser, const MY_STRING * quote);
BOOL              is_query_separator(DES_PARSER *parser);
/* Installs position on the character next after closing quote */
const char *            find_closing_quote(DES_PARSER *parser);
BOOL              is_param_marker(DES_PARSER *parser);
void              add_parameter(DES_PARSER *parser);
void              step_char(DES_PARSER *parser);
BOOL              tokenize(DES_PARSER *parser);
QUERY_TYPE_ENUM   detect_query_type(DES_PARSER *parser,
                                    const QUERY_TYPE_RESOLVING *rule);

BOOL              parser_compare     (DES_PARSER *parser, const MY_STRING *str);
BOOL              case_compare(DES_PARSED_QUERY *parser, const char *pos,
                               const MY_STRING *str);

BOOL              parse(DES_PARSED_QUERY *pq);


const char *mystr_get_prev_token(desodbc::CHARSET_INFO *charset,
                                        const char **query, const char *start);
const char *mystr_get_next_token(desodbc::CHARSET_INFO *charset,
                                        const char **query, const char *end);
const char *find_token(desodbc::CHARSET_INFO *charset, const char * begin,
                       const char * end, const char * target);
const char *find_first_token(desodbc::CHARSET_INFO *charset, const char * begin,
                       const char * end, const char * target);
const char *skip_leading_spaces(const char *str);

int         is_set_names_statement  (const char *query);
BOOL        is_drop_procedure       (const SQLCHAR * query);
BOOL        is_drop_function        (const SQLCHAR * query);
BOOL        is_create_procedure     (const SQLCHAR * query);
BOOL        is_create_function      (const SQLCHAR * query);
BOOL        is_use_db               (const SQLCHAR * query);
BOOL        is_call_procedure       (const DES_PARSED_QUERY *query);
BOOL        stmt_returns_result     (const DES_PARSED_QUERY *query);

BOOL        remove_braces           (DES_PARSER *query);

#endif
