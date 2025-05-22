/* Copyright (c) 2000, 2021, Oracle and/or its affiliates.

	Modified in 2025 by Sergio Miguel Garcia Jimenez <segarc21@ucm.es>
	(see the next block comment below).
	
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

// ---------------------------------------------------------
// Modified in 2025 by Sergio Miguel Garcia Jimenez <segarc21@ucm.es>,
// hereinafter the DESODBC developer, in the context of the GPLv2 derivate
// work DESODBC, an ODBC Driver of the open-source DBMS Datalog Educational
// System (DES) (see https://des.sourceforge.io/)
//
// The authorship of each section of this source file (comments,
// functions and other symbols) belongs to MyODBC unless we
// explicitly state otherwise.
// ---------------------------------------------------------

#ifndef _list_h_
#define _list_h_

/*
DESODBC: renaming myodbc to desodbc
*/
namespace desodbc
{

/**
  @file include/my_list.h
*/

typedef struct LIST {
#if defined(__cplusplus) && __cplusplus >= 201103L
  struct LIST *prev{nullptr}, *next{nullptr};
  void *data{nullptr};
#else
  struct LIST *prev, *next;
  void *data;
#endif
} LIST;

typedef int (*list_walk_action)(void *, void *);

extern LIST *list_add(LIST *root, LIST *element);
extern LIST *list_delete(LIST *root, LIST *element);
extern LIST *list_cons(void *data, LIST *root);
extern LIST *list_reverse(LIST *root);
extern void list_free(LIST *root, unsigned int free_data);
extern unsigned int list_length(LIST *);
extern int list_walk(LIST *, list_walk_action action, unsigned char *argument);

#define list_rest(a) ((a)->next)

} /* namespace myodbc */

#endif
