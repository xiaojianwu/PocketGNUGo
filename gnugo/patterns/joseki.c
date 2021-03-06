/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * This is GNU GO, a Go program. Contact gnugo@gnu.org, or see   *
 * http://www.gnu.org/software/gnugo/ for more information.      *
 *                                                               *
 * Copyright 1999 and 2000 by the Free Software Foundation.      *
 *                                                               *
 * This program is free software; you can redistribute it and/or *
 * modify it under the terms of the GNU General Public License   *
 * as published by the Free Software Foundation - version 2.     *
 *                                                               *
 * This program is distributed in the hope that it will be       *
 * useful, but WITHOUT ANY WARRANTY; without even the implied    *
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       *
 * PURPOSE.  See the GNU General Public License in file COPYING  *
 * for more details.                                             *
 *                                                               *
 * You should have received a copy of the GNU General Public     *
 * License along with this program; if not, write to the Free    *
 * Software Foundation, Inc., 59 Temple Place - Suite 330,       *
 * Boston, MA 02111, USA                                         *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */




/* Convert joseki from sgf format to patterns.db format. */
/* version 81-jd */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <tchar.h>

extern int isspace(int c);
extern int isalpha(int c);
extern int isdigit(int c);

#define USAGE "\
Usage : joseki prefix\n\
"

#define MAX_TOKEN_LENGTH 10
#define MAX_PARAM_LENGTH 1000
#define MAX_CLASSIFICATION_LENGTH 10
#define BOARD_SIZE 19
#define MAX_DEPTH 40
#define MAX_CONSTRAINT 25
char token[MAX_TOKEN_LENGTH];
char param[MAX_PARAM_LENGTH];
char readparam[MAX_PARAM_LENGTH];
char comment[MAX_PARAM_LENGTH];
char macro[MAX_PARAM_LENGTH];
char classification[MAX_CLASSIFICATION_LENGTH];
char board[BOARD_SIZE * BOARD_SIZE];
char board_c[BOARD_SIZE * BOARD_SIZE];
char stack_c[MAX_CONSTRAINT];
char stack_ordre[MAX_CONSTRAINT];
char stack[MAX_DEPTH][BOARD_SIZE * BOARD_SIZE];
int stack_pointer=0;
char color;
char movei, movej;
int value;
char extenti;
char extentj;
int pattern_finished;
int number;
char *prefix;
int nb_c;
int scan_val;

static void skip_sgf_header(FILE * sgf)
{
  int state=0;
  int c;
  /* Find first occurence of "(;", skipping possible mail header and
     other junk. */
  while ( (c = getc(sgf)) != EOF) {
    if (state == 0 && c == '(')
      state = 1;
    else if (state == 1) {
      if (c == ';')
	break;
      else
	state = 0;
    }
  }
  /* Then skip the properties in the root node. */
  do
    c = getc(sgf);
  while (c != ';' && c != EOF);
}

static void write_patterns_header(void)
{
  printf("# This file is automatically generated. Do not edit.\n\n");
}

static void read_until(FILE * sgf, char *buf, int terminator, int skip_whitespace,
	   int buflen)
{
  int c;
  int i, j;

  j = 0;
  for(i = 0; i < buflen; i++) {
    c = getc(sgf);
    if (c == EOF || c == terminator)
      break;
    if (skip_whitespace && isspace(c))
      continue;
    buf[j] = c;
    j++;
  }
  if (j < buflen)
    buf[j] = 0;

  if (i < buflen)
    buf[i] = 0;
  else {
    buf[buflen-1] = 0;
    fprintf(stderr, "Warning, sgf property or property value too long:\n%s\n",
	    buf);
  }
}

static int get_sgf_token(FILE * sgf)
{
  int c;
  char oldtoken[MAX_TOKEN_LENGTH];

  strcpy(oldtoken, token);
  strcpy(token, "");
  while ((c = getc(sgf)) != EOF) {
    if (isspace(c))
      continue;  /* Skip whitespace. */
    if (strchr(";()", c)) {
      sprintf(token, "%c", c);
      break;
    }
    if (isalpha(c)) {
      ungetc(c, sgf);
      read_until(sgf, token, '[', 1, sizeof(token));
      read_until(sgf, param, ']', 1, sizeof(param));
      break;
    }
    if (c == '[') {
      strcpy(token, oldtoken);
      read_until(sgf, param, ']', 1, sizeof(param));
      break;
    }
  }
  return !feof(sgf);
}

static void init_board(void)
{
  int i;
  for (i = 0; i < BOARD_SIZE * BOARD_SIZE; i++) {
    board[i]='.';
    board_c[i] = '.';
  }
  nb_c = 0;
}

static void push_stack(void)
{
  int i;
  for (i = 0; i < BOARD_SIZE * BOARD_SIZE; i++)
    stack[stack_pointer][i] = board[i];
  stack_pointer++;
/*
 *     fprintf(stderr,"push stack %d\n",stack_pointer);
 */
}

static void pop_stack(void)
{
  int i;
  stack_pointer--;
  if (stack_pointer < 0) /* Should only happen at end of sgf file. */
    return;
  for (i = 0; i < BOARD_SIZE * BOARD_SIZE; i++)
    board[i] = stack[stack_pointer][i];
/*
 *     fprintf(stderr,"pop stack %d\n",stack_pointer);
 */
}

static int move_to_index(char j, char i)
{
  return BOARD_SIZE*(i - 'a') + (j - 'a');
}


static void write_table(void)
{
  int i, j;
  char search[4];
  char *s_ptr;
  int one;

  if (nb_c == 0)
    return;
  one = 0;
  for (j = 0; j < 19; j++) {
    printf("\n");
    for (i = 0; i < 19; i++)
      printf("%c", board_c[BOARD_SIZE * i + j]);
  }
  printf("\n\n;");

  for (i = 0; i < nb_c; i++) {
    switch (stack_ordre[i]) {
    case 'o':
      if (one)
	printf("&&");
      printf("oarea(%c)", stack_c[i]);
      one = 1;
      break;
    case 'x':
      if (one)
	printf("&&");
      printf("xarea(%c)", stack_c[i]);
      one = 1;
      break;
    case 'e':
      if (one)
	printf("&&");
      printf("(!oarea(%c)&&!xarea(%c))", stack_c[i], stack_c[i]);
      one = 1;
      break;
    default:
      break;
    }
  }

  if (strlen(macro) > 0) {
    for (i = 0; i < nb_c; i++)
      if (isdigit((int) stack_ordre[i])) {
	sprintf(search, "(%c)", stack_ordre[i]);
	while ((s_ptr = strstr(macro, search)) != NULL)
	  s_ptr[1] = stack_c[i];
      }
    if (one)
      printf("&&(");
    printf("%s", macro);
    if (one)
      printf(")");
  }
  printf("\n\n");

}

static void write_comment(void)
{
  int l, i, k;
  char c;

  l = strlen(comment);
  k = 0;
  while (k < l) {
    i = 0;
    printf("# ");
    while (i < 71) {
      c = comment[k];
      if (c == '\0' || c == '#') {
	i = 72;
	k++;
      } else {
	printf("%c", c);
	k++;
	i++;
      }
    }
    printf("\n");
  }
  printf("\n");
}

static void add_tag(char c, char cj, char ci)
{
  nb_c++;
  stack_ordre[nb_c - 1] = c;
  stack_c[nb_c - 1] = 'a' + nb_c - 1;
  board_c[move_to_index(ci, cj)] = 'a' + nb_c - 1;
}


static void write_pattern(void)
{
  int height,width;
  int i,j;
  printf("\nPattern %s%d\n\n", prefix, number);
  height = extenti - 'a';
  width = extentj - 'a';
  for (j = 0; j < (BOARD_SIZE - width); j++)
    printf("-");
  printf("+\n");
  for (i = 0; i <= height ; i++) {
    for (j = width; j < BOARD_SIZE ; j++)
      switch (board[BOARD_SIZE * i + j]) {
      case 'B':
	printf("%c", color=='B' ? 'O' : 'X');
	break;
      case 'W':
	printf("%c", color=='B' ? 'X' : 'O');
	break;
      case '*':
	printf("*");
	break;
      case '.':
	printf(".");
	break;
      }
    printf("|\n");
  }
  if (prefix[0] == 'F')
    printf("\n:8,%d,0,s%s,0,0,0,-1,0,NULL\n", value, classification);
  else
    printf("\n:8,%d,0,s%s,0,0,0,-2,2,NULL\n", value, classification);

  number++;
}

static void finish_pattern(void)
{
  int where, i;
  if (pattern_finished) /* only write the pattern once */
    return;
  if (movei == 't' && movej == 't') /* pass, don't write the pattern */
    return;
  where = move_to_index(movej, movei);
  if (value>0) {
    board[where] = '*';
    write_pattern();
    write_table();
    write_comment();
  }
  board[where] = color;
  pattern_finished = 1;
  value = 0;
  classification[0]=0;
  nb_c = 0;
  for (i = 0; i < BOARD_SIZE * BOARD_SIZE; i++)
    board_c[i] = '.';
}

int main(int argc, char *argv[])
{

  if (argc != 2) {
    fputs(USAGE, stderr);
    exit(EXIT_FAILURE);
  }
  prefix=argv[1];
  
  skip_sgf_header(stdin);
  write_patterns_header();
  init_board();
  number=1;
  value = 0;
  strcpy(classification, "");
  strcpy(macro, "");
  strcpy(comment, "");
  push_stack();
  while (get_sgf_token(stdin)) {
    if (strncmp(token, ";", 1) == 0) /* new node */
      finish_pattern();
    else if (strncmp(token, "(", 1) == 0) { /* new variation */
      finish_pattern();
      push_stack();
    } else if (strncmp(token, ")", 1) == 0) {	/* end of variation */
      finish_pattern();
      pop_stack();
    } else if (strlen(token) == 1 && (
				       (strncmp(token, "B", 2) == 0) ||		/* black move */
				       (strncmp(token, "W", 2) == 0))) {	/* white move */
      pattern_finished = 0;
      color = token[0];
      if (!param[0]) {  /* FF4 style pass */
	movej = 't';
	movei = 't';
      } else {			/* move or FF1 -- FF3 style pass, i.e. tt */
	movej = param[0];
	movei = param[1];
      }
    } else if (strncmp(token, "LB", 2) == 0) {	/* Label */
      if (param[3] == 'A') {
      extentj = param[0];
      extenti = param[1];
      } else {
	add_tag(param[3], param[0], param[1]);
      }
    } else if (strncmp(token, "MA", 2) == 0) {	/* square= myarea test */
      add_tag('o', param[0], param[1]);
    } else if (strncmp(token, "CR", 2) == 0) {	/* circle=empty area test */
      add_tag('e', param[0], param[1]);
    } else if (strncmp(token, "TR", 2) == 0) {	/* triangle=other area test */
      add_tag('x', param[0], param[1]);
    }
    /* FIXME: Improve parsing of comments */
    else if (strlen(token) == 1 && strncmp(token, "C", 1) == 0) {	/* Comment */
      value = 0;
      strcpy(classification, "");
      strcpy(macro, "");
      strcpy(comment, "");
      strcpy(readparam, "");
      sscanf(param, "%d;%s", &value, readparam);
      if (strlen(readparam) > 0) {
	if (readparam[0] == ';') {
	  if (readparam[1] == '#')
	    sscanf(readparam, ";#%s", comment);
	  else
	    sscanf(readparam, ";%[^#]#%s", macro, comment);
	} else {
	  sscanf(readparam, "%[^;];%s", classification, param);
	  if (strlen(param) > 0) {
	    if (param[0] == '#')
	      sscanf(param, "#%s", comment);
    else
	      sscanf(param, "%[^#]#%s", macro, comment);
	  }
	}
      }
    } else if (strncmp(token, "PL", 2) == 0) {
    } else
      fprintf(stderr, "Warning, unknown sgf property %s.\n", token);
  }
  return 0;
}
      
/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
