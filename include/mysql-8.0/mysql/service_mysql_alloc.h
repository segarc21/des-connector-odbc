/* Copyright (c) 2012, 2021, Oracle and/or its affiliates.

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
// ---------------------------------------------------------

#ifndef MYSQL_SERVICE_MYSQL_ALLOC_INCLUDED
#define MYSQL_SERVICE_MYSQL_ALLOC_INCLUDED

/**
  @file include/mysql/service_mysql_alloc.h
*/

#ifndef MYSQL_ABI_CHECK
#include <stdlib.h>
#endif

/* PSI_memory_key */
#include "mysql/components/services/psi_memory_bits.h"

/*
DESODBC: renaming myodbc to desodbc
*/
namespace desodbc
{

/* myf */
typedef int myf_t;

typedef void *(*mysql_malloc_t)(PSI_memory_key key, size_t size, myf_t flags);
typedef void *(*mysql_realloc_t)(PSI_memory_key key, void *ptr, size_t size,
                                 myf_t flags);
typedef void (*mysql_claim_t)(const void *ptr, bool claim);
typedef void (*mysql_free_t)(void *ptr);
typedef void *(*my_memdup_t)(PSI_memory_key key, const void *from,
                             size_t length, myf_t flags);
typedef char *(*my_strdup_t)(PSI_memory_key key, const char *from, myf_t flags);
typedef char *(*my_strndup_t)(PSI_memory_key key, const char *from,
                              size_t length, myf_t flags);

#ifdef MYSQL_DYNAMIC_PLUGIN

#define my_malloc mysql_malloc_service->mysql_malloc
#define my_realloc mysql_malloc_service->mysql_realloc
#define my_claim mysql_malloc_service->mysql_claim
#define my_free mysql_malloc_service->mysql_free
#define my_memdup mysql_malloc_service->my_memdup
#define my_strdup mysql_malloc_service->my_strdup
#define my_strndup mysql_malloc_service->my_strndup

#else

extern void *my_malloc(PSI_memory_key key, size_t size, myf_t flags);
extern void *my_realloc(PSI_memory_key key, void *ptr, size_t size,
                        myf_t flags);
extern void my_claim(const void *ptr, bool claim);
extern void my_free(void *ptr);
extern void *my_memdup(PSI_memory_key key, const void *from, size_t length,
                       myf_t flags);
extern char *my_strdup(PSI_memory_key key, const char *from, myf_t flags);
extern char *my_strndup(PSI_memory_key key, const char *from, size_t length,
                        myf_t flags);

#endif

} /* namespace myodbc */

#endif
