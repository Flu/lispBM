/*
    Copyright 2019 Joel Svensson	svenssonjoel@yahoo.se

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


// WORK IN PROGRESS

#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "tokpar.h"
#include "symrepr.h"
#include "heap.h"

#define TOKOPENPAR      0
#define TOKCLOSEPAR     1
#define TOKQUOTE        2
#define TOKSYMBOL       3
#define TOKINT          4
#define TOKUINT         5
#define TOKBOXEDINT     6
#define TOKBOXEDUINT    7
#define TOKBOXEDFLOAT   8
#define TOKSTRING       9
#define TOKCHAR         10
#define TOKENIZER_ERROR 1024
#define TOKENIZER_END   2048

typedef struct {

  unsigned int type;

  unsigned int text_len;
  union {
    char  c;
    char  *text;
    INT   i;
    UINT  u;
    FLOAT f;
  }data;
} token;


typedef struct {
  char *str;
  unsigned int pos;
} tokenizer_state;


int tok_openpar(char *str) {
  if (*str == '(')
    return 1;
  return 0;
}

int tok_closepar(char *str) {
  if (*str == ')')
    return 1;
  return 0;
}

int tok_quote(char *str) {
  if (*str == '\'')
    return 1;
  return 0;
}

bool symchar0(char c) {
  const char *allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+*-/";

  int i = 0;
  while (allowed[i] != 0) {
    if (c == allowed[i++]) return true;
  }
  return false;
}

bool symchar(char c) {
  const char *allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-*/";

  int i = 0;
  while (allowed[i] != 0) {
    if (c == allowed[i++]) return true;
  }
  return false;
}

int tok_symbol(char *str, char** res) {

  if (!symchar0(*str))  return 0;

  int i = 0;
  int len = 1;
  int n = 0;

  while (symchar(*(str+len))) {
    len++;
  }

  *res = malloc(len+1);
  memset(*res,0,len+1);

  for (i = 0; i < len; i ++) {
    (*res)[i] = str[i];
    n++;
  }

  return n;
}

int tok_string(char *str, char **res) {

  int i = 0;
  int n = 0;
  int len = 0;
  if (!(*str == '\"')) return 0;

  str++; n++;

  // compute length of string
  while (*(str + len) != 0 &&
	 *(str + len) != '\"') {
    len++;
  }

  // str ends before tokenized string is closed.
  if (*(str+len) != '\"') return 0;

  // allocate memory for result string
  *res = malloc(len+1);
  memset(*res, 0, len+1);

  for (i = 0; i < len; i ++) {
    (*res)[i] = str[i];
    n++;
  }

  return (n+1);
}

int tok_char(char *str, char *res) {

  // The simple char case
  if (*str == '\\' &&
      *(str+1) == '#' &&
      isgraph(*(str+2))) {
    *res = *(str+2);
    return 3;
  }
  return 0;
}

int tok_i(char *str, INT *res) {

  INT acc = 0;
  int n = 0;

  while ( *(str+n) >= '0' && *(str+n) <= '9' ){
    acc = (acc*10) + (*(str+n) - '0');
    n++;
  }

  // Not needed if strict adherence to ordering of calls to tokenizers.
  if (*(str+n) == 'U' ||
      *(str+n) == 'u' ||
      *(str+n) == '.' ||
      *(str+n) == 'I') return 0;

  *res = acc;
  return n;
}

int tok_I(char *str, INT *res) {
  INT acc = 0;
  int n = 0;

  while ( *(str+n) >= '0' && *(str+n) <= '9' ){
    acc = (acc*10) + (*(str+n) - '0');
    n++;
  }

  if (*(str+n) == 'I') {
    *res = acc;
    return n+1;
  }
  return 0;
}

int tok_u(char *str, UINT *res) {
  UINT acc = 0;
  int n = 0;

  while ( *(str+n) >= '0' && *(str+n) <= '9' ){
    acc = (acc*10) + (*(str+n) - '0');
    n++;
  }

  if (*(str+n) == 'u') {
    *res = acc;
    return n+1;
  }
  return 0;
}

int tok_U(char *str, UINT *res) {
  UINT acc = 0;
  int n = 0;

  while ( *(str+n) >= '0' && *(str+n) <= '9' ){
    acc = (acc*10) + (*(str+n) - '0');
    n++;
  }

  if (*(str+n) == 'U') {
    *res = acc;
    return n+1;
  }
  return 0;
}

int tok_F(char *str, FLOAT *res) {

  int n = 0;

  while ( *(str+n) >= '0' && *(str+n) <= '9') n++;

  if ( *(str+n) == '.') n++;
  else return 0;

  if ( !(*(str+n) >= '0' && *(str+n) <= '9')) return 0;
  while ( *(str+n) >= '0' && *(str+n) <= '9') n++;

  *res = atof(str);
  return n;
}


token next_token(tokenizer_state *ts) {

  token t;
  char *curr = ts->str + ts->pos;

  INT i_val;
  UINT u_val;
  char c_val;
  FLOAT f_val;
  int n = 0;

  // Eat whitespace and comments. 
  bool clean_whitespace = true;
  while ( clean_whitespace ){
    if ( *curr == ';' ) {
      while (*curr && *curr != '\n')  {
	curr++;
	n++;
      }
    } else if ( isspace(*curr)) {
      curr++;
      n++;
    } else {
      clean_whitespace = false;
    }
  }
      
  // eat whitespace
  //while ( *curr && isspace(*curr) ) {
  //  curr++;
  //  n++;
  //}
  
 
  ts->pos += n;

  // Check for end of string
  if ( *curr == 0 ) {
    t.type = TOKENIZER_END;
    return t;
  }

  n = 0;

  if ((n = tok_quote(curr))) {
    ts->pos += n;
    t.type = TOKQUOTE;
    return t;
  }

  if ((n = tok_openpar(curr))) {
    ts->pos += n;
    t.type = TOKOPENPAR;
    return t;
  }

  if ((n = tok_closepar(curr))) {
    ts->pos += n;
    t.type = TOKCLOSEPAR;
    return t;
  }

  if ((n = tok_symbol(curr, &t.data.text))) {
    ts->pos += n;
    t.text_len = n;
    t.type = TOKSYMBOL;
    return t;
  }

  if ((n = tok_char(curr, &c_val))) {
    ts->pos += n;
    t.data.c = c_val;
    t.type = TOKCHAR;
    return t;
  }

  if ((n = tok_string(curr, &t.data.text))) {
    ts->pos += n;
    t.text_len = n - 2;
    t.type = TOKSTRING;
    return t;
  }

  if ((n = tok_F(curr, &f_val))) {
    ts->pos += n;
    t.data.f = f_val;
    t.type = TOKBOXEDFLOAT;
    return t;
  }

  if ((n = tok_u(curr, &u_val))) {
    ts->pos += n;
    t.data.u = u_val;
    t.type = TOKUINT;
    return t;
  }

  if ((n = tok_U(curr, &u_val))) {
    ts->pos += n;
    t.data.u = u_val;
    t.type = TOKBOXEDUINT;
    return t;
  }

  if ((n = tok_I(curr, &i_val))) {
    ts->pos += n;
    t.data.i = i_val;
    t.type = TOKBOXEDINT;
    return t;
  }

  // Shortest form of integer match. Move to last in chain of numerical tokens.
  if ((n = tok_i(curr, &i_val))) {
    ts->pos += n;
    t.data.i = i_val;
    t.type = TOKINT;
    return t;
  }

  t.type = TOKENIZER_ERROR;
  return t;
}

VALUE parse_sexp(token tok, tokenizer_state *ts);
VALUE parse_sexp_list(token tok, tokenizer_state *ts);

VALUE parse_program(tokenizer_state *ts) {

  token tok = next_token(ts);
  VALUE head;
  VALUE tail;
  
  
  if (tok.type == TOKENIZER_END) {
    return enc_sym(symrepr_nil());
  }

  head = parse_sexp(tok, ts);
  tail = parse_program(ts);

  return cons(head, tail);
}

VALUE parse_sexp(token tok, tokenizer_state *ts) {

  VALUE v;
  token t;
  
  switch (tok.type) {
  case TOKENIZER_END:
    printf("Parse error:  end of token stream\n");
    break;
  case TOKOPENPAR:
    t = next_token(ts);
    return parse_sexp_list(t,ts);
  case TOKSYMBOL: {
    UINT symbol_id;
    
    if (symrepr_lookup(tok.data.text, &symbol_id)) {
      v = enc_sym(symbol_id);
    }
    else if (symrepr_addsym(tok.data.text, &symbol_id)) {
      v = enc_sym(symbol_id);
    } else {
      v = enc_sym(symrepr_rerror());
    }
    free(tok.data.text);
    return v;
  }
  case TOKSTRING: {
    heap_allocate_array(&v, tok.text_len+1, VAL_TYPE_CHAR);
    array_t *arr = (array_t*)car(v);
    memset(arr->data.c, 0, (tok.text_len+1) * sizeof(char));
    memcpy(arr->data.c, tok.data.text, tok.text_len * sizeof(char));
    free(tok.data.text);
    return v;
  }
  case TOKINT: 
    return enc_i(tok.data.i);
  case TOKUINT:
    return enc_u(tok.data.u);
  case TOKBOXEDINT: 
    return cons(tok.data.i, enc_sym(SPECIAL_SYM_BOXED_I));
  case TOKBOXEDUINT:
    return cons(tok.data.u, enc_sym(SPECIAL_SYM_BOXED_U));
  case TOKBOXEDFLOAT:
    return cons(tok.data.f, enc_sym(SPECIAL_SYM_BOXED_F));
  case TOKQUOTE: {
    t = next_token(ts);
    return cons(enc_sym(symrepr_quote()), cons (parse_sexp(t, ts), enc_sym(symrepr_nil()))); 
  }
  }
  return enc_sym(symrepr_rerror());
}

VALUE parse_sexp_list(token tok, tokenizer_state *ts) {

  token t;
  VALUE head;
  VALUE tail;
  
  switch (tok.type) {
  case TOKENIZER_END:
    printf("Parse error:  end of token stream\n");
    break;
  case TOKCLOSEPAR:
    return enc_sym(symrepr_nil());
  default:
    head = parse_sexp(tok, ts);
    t = next_token(ts);
    tail = parse_sexp_list(t, ts);
    return cons(head, tail);
  }
  
  return enc_sym(symrepr_rerror());
}

VALUE tokpar_parse(char *str) {

  tokenizer_state ts;
  ts.str = str;
  ts.pos = 0;

  return parse_program(&ts);
}


VALUE tokpar_parse_string(char *str) {

  printf("Parsing string: %s\n", str);

  tokenizer_state ts;
  ts.str = str;
  ts.pos = 0;

  token tok = next_token(&ts);
  while (tok.type != TOKENIZER_END) {

    switch(tok.type) {

    case TOKOPENPAR:
      printf("TOKEN: %d\t(\n", tok.type);
      break;
    case TOKCLOSEPAR:
      printf("TOKEN: %d\t)\n", tok.type);
      break;
    case TOKQUOTE:
      printf("TOKEN: %d\t'\n", tok.type);
      break;
    case TOKSYMBOL:
      printf("TOKEN: %d\t%s\n", tok.type, tok.data.text);
      free(tok.data.text);
      break;
    case TOKINT:
      printf("TOKEN: %d\t%d\n", tok.type, tok.data.i);
      break;
    case TOKUINT:
      printf("TOKEN: %d\t%u\n", tok.type, tok.data.u);
      break;
    case TOKBOXEDINT:
      printf("TOKEN: %d\t%d\n", tok.type, tok.data.i);
      break;
    case TOKBOXEDUINT:
      printf("TOKEN: %d\t%u\n", tok.type, tok.data.u);
      break;
    case TOKBOXEDFLOAT:
      printf("TOKEN: %d\t%f\n", tok.type, tok.data.f);
      break;
    case TOKSTRING:
      printf("TOKEN: %d\t%s\n", tok.type, tok.data.text);
      free(tok.data.text);
      break;
    case TOKCHAR:
      printf("TOKEN: %d\t%c\n", tok.type, tok.data.c);
      break;
    case TOKENIZER_ERROR:
      printf("Tokenizer error\n");
      return 0;
    default:
      printf("UNKNOWN!\n");
      return 0;
    }
    tok = next_token(&ts);
  }

  return 0;
}