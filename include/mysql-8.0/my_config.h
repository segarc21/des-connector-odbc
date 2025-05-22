// Copyright (c) 2007, 2024, Oracle and/or its affiliates.
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

#ifndef MY_CONFIG_H
#define MY_CONFIG_H

/*
 * From configure.cmake, in order of appearance 
 */
/* #undef HAVE_LLVM_LIBCPP */

/* Libraries */
/* #undef HAVE_LIBM */
/* #undef HAVE_LIBNSL */
/* #undef HAVE_LIBCRYPT */
/* #undef HAVE_LIBSOCKET */
/* #undef HAVE_LIBDL */
/* #undef HAVE_LIBRT */
/* #undef HAVE_LIBWRAP */
/* #undef HAVE_LIBWRAP_PROTOTYPES */

/* Header files */
/* #undef HAVE_ALLOCA_H */
/* #undef HAVE_ARPA_INET_H */
/* #undef HAVE_DLFCN_H */
/* #undef HAVE_EXECINFO_H */
/* #undef HAVE_FPU_CONTROL_H */
/* #undef HAVE_GRP_H */
/* #undef HAVE_IEEEFP_H */
/* #undef HAVE_LANGINFO_H */
/* #undef HAVE_LSAN_INTERFACE_H */
/* #undef HAVE_MALLOC_H */
/* #undef HAVE_NETINET_IN_H */
/* #undef HAVE_POLL_H */
/* #undef HAVE_PWD_H */
/* #undef HAVE_STRINGS_H */
/* #undef HAVE_SYS_CDEFS_H */
/* #undef HAVE_SYS_IOCTL_H */
/* #undef HAVE_SYS_MMAN_H */
/* #undef HAVE_SYS_PRCTL_H */
/* #undef HAVE_SYS_RESOURCE_H */
/* #undef HAVE_SYS_SELECT_H */
/* #undef HAVE_SYS_SOCKET_H */
/* #undef HAVE_TERM_H */
/* #undef HAVE_TERMIOS_H */
/* #undef HAVE_TERMIO_H */
/* #undef HAVE_UNISTD_H */
/* #undef HAVE_SYS_WAIT_H */
/* #undef HAVE_SYS_PARAM_H */
/* #undef HAVE_FNMATCH_H */
/* #undef HAVE_SYS_UN_H */
/* #undef HAVE_VIS_H */
/* #undef HAVE_SASL_SASL_H */

/* Libevent */
/* #undef HAVE_DEVPOLL */
/* #undef HAVE_SYS_DEVPOLL_H */
/* #undef HAVE_SYS_EPOLL_H */
/* #undef HAVE_TAILQFOREACH */

/* Functions */
/* #undef HAVE_ALIGNED_MALLOC */
/* #undef HAVE_BACKTRACE */
/* #undef HAVE_PRINTSTACK */
/* #undef HAVE_INDEX */
/* #undef HAVE_CHOWN */
/* #undef HAVE_CUSERID */
/* #undef HAVE_DIRECTIO */
/* #undef HAVE_FTRUNCATE */
/* #undef HAVE_FCHMOD */
/* #undef HAVE_FCNTL */
/* #undef HAVE_FDATASYNC */
/* #undef HAVE_DECL_FDATASYNC */
/* #undef HAVE_FEDISABLEEXCEPT */
/* #undef HAVE_FSEEKO */
/* #undef HAVE_FSYNC */
/* #undef HAVE_GETHRTIME */
/* #undef HAVE_GETNAMEINFO */
/* #undef HAVE_GETPASS */
/* #undef HAVE_GETPASSPHRASE */
/* #undef HAVE_GETPWNAM */
/* #undef HAVE_GETPWUID */
/* #undef HAVE_GETRLIMIT */
/* #undef HAVE_GETRUSAGE */
/* #undef HAVE_INITGROUPS */
/* #undef HAVE_ISSETUGID */
/* #undef HAVE_GETUID */
/* #undef HAVE_GETEUID */
/* #undef HAVE_GETGID */
/* #undef HAVE_GETEGID */
/* #undef HAVE_LSAN_DO_RECOVERABLE_LEAK_CHECK */
/* #undef HAVE_MADVISE */
/* #undef HAVE_MALLOC_INFO */
/* #undef HAVE_MEMRCHR */
/* #undef HAVE_MLOCK */
/* #undef HAVE_MLOCKALL */
/* #undef HAVE_MMAP64 */
/* #undef HAVE_POLL */
/* #undef HAVE_POSIX_FALLOCATE */
/* #undef HAVE_POSIX_MEMALIGN */
/* #undef HAVE_PREAD */
/* #undef HAVE_PTHREAD_CONDATTR_SETCLOCK */
/* #undef HAVE_PTHREAD_SIGMASK */
/* #undef HAVE_SETFD */
/* #undef HAVE_SIGACTION */
/* #undef HAVE_SLEEP */
/* #undef HAVE_STPCPY */
/* #undef HAVE_STPNCPY */
/* #undef HAVE_STRLCPY */
/* #undef HAVE_STRLCAT */
/* #undef HAVE_STRSIGNAL */
/* #undef HAVE_FGETLN */
/* #undef HAVE_STRSEP */
/* #undef HAVE_TELL */
/* #undef HAVE_VASPRINTF */
/* #undef HAVE_MEMALIGN */
/* #undef HAVE_NL_LANGINFO */
/* #undef HAVE_HTONLL */
/* #undef HAVE_EPOLL */
/* #undef HAVE_EVENT_PORTS */
/* #undef HAVE_INET_NTOP */
/* #undef HAVE_WORKING_KQUEUE */
/* #undef HAVE_TIMERADD */
/* #undef HAVE_TIMERCLEAR */
/* #undef HAVE_TIMERCMP */
/* #undef HAVE_TIMERISSET */

/* WL2373 */
/* #undef HAVE_SYS_TIME_H */
/* #undef HAVE_SYS_TIMES_H */
/* #undef HAVE_TIMES */
/* #undef HAVE_GETTIMEOFDAY */

/* Symbols */
/* #undef HAVE_LRAND48 */
/* #undef GWINSZ_IN_SYS_IOCTL */
/* #undef FIONREAD_IN_SYS_IOCTL */
/* #undef FIONREAD_IN_SYS_FILIO */

#define HAVE_ISINF 1

/* #undef HAVE_KQUEUE */
/* #undef HAVE_KQUEUE_TIMERS */
/* #undef HAVE_POSIX_TIMERS */

/* Endianess */
/* #undef WORDS_BIGENDIAN */

/* Type sizes */
#define SIZEOF_VOIDP     8
#define SIZEOF_CHARP     8
#define SIZEOF_LONG      4
#define SIZEOF_SHORT     2
#define SIZEOF_INT       4
#define SIZEOF_LONG_LONG 8
#define SIZEOF_OFF_T     4
#define SIZEOF_TIME_T    8
/* #undef HAVE_ULONG */
/* #undef HAVE_U_INT32_T */

/* Support for tagging symbols with __attribute__((visibility("hidden"))) */
/* #undef HAVE_VISIBILITY_HIDDEN */

/* Code tests*/
/* #undef HAVE_CLOCK_GETTIME */
/* #undef HAVE_CLOCK_REALTIME */
/* #undef DNS_USE_CPU_CLOCK_FOR_ID */
#define STACK_DIRECTION -1
/* #undef TIME_WITH_SYS_TIME */
#define NO_FCNTL_NONBLOCK 1
/* #undef HAVE_PAUSE_INSTRUCTION */
/* #undef HAVE_FAKE_PAUSE_INSTRUCTION */
/* #undef HAVE_HMT_PRIORITY_INSTRUCTION */
/* #undef HAVE_ABI_CXA_DEMANGLE */
/* #undef HAVE_BUILTIN_UNREACHABLE */
/* #undef HAVE_BUILTIN_EXPECT */
/* #undef HAVE_BUILTIN_STPCPY */
/* #undef HAVE_GCC_ATOMIC_BUILTINS */
/* #undef HAVE_GCC_SYNC_BUILTINS */
/* #undef HAVE_VALGRIND */
/* #undef HAVE_SYS_GETTID */
/* #undef HAVE_PTHREAD_GETTHREADID_NP */
/* #undef HAVE_PTHREAD_THREADID_NP */
/* #undef HAVE_INTEGER_PTHREAD_SELF */
/* #undef HAVE_PTHREAD_SETNAME_NP */
/*
  This macro defines whether the compiler in use needs a 'typename' keyword
  to access the types defined inside a class template, such types are called
  dependent types. Some compilers require it, some others forbid it, and some
  others may work with or without it. For example, GCC requires the 'typename'
  keyword whenever needing to access a type inside a template, but msvc
  forbids it.
 */
#define HAVE_IMPLICIT_DEPENDENT_NAME_TYPING 1

/* IPV6 */
/* #undef HAVE_NETINET_IN6_H */
/* #undef HAVE_STRUCT_IN6_ADDR */

/*
 * Platform specific CMake files
 */
#define MACHINE_TYPE ""
/* #undef LINUX_ALPINE */
/* #undef HAVE_LINUX_LARGE_PAGES */
/* #undef HAVE_SOLARIS_LARGE_PAGES */
/* #undef HAVE_SOLARIS_ATOMIC */
#define SYSTEM_TYPE "Windows"
/* This should mean case insensitive file system */
/* #undef FN_NO_CASE_SENSE */

/*
 * From main CMakeLists.txt
 */
/* #undef MAX_INDEXES */
/* #undef WITH_INNODB_MEMCACHED */
/* #undef ENABLE_MEMCACHED_SASL */
/* #undef ENABLE_MEMCACHED_SASL_PWDB */
/* #undef ENABLED_PROFILING */
/* #undef HAVE_ASAN */
/* #undef HAVE_UBSAN */
/* #undef ENABLED_LOCAL_INFILE */
/* #undef DEFAULT_MYSQL_HOME */
#define SHAREDIR "share"
/* #undef DEFAULT_BASEDIR */
/* #undef MYSQL_DATADIR */
/* #undef MYSQL_KEYRINGDIR */
#define DEFAULT_CHARSET_HOME "C:/Users/sergi/Desktop/mysql-connector-odbc-9.0.0"
/* #undef PLUGINDIR */
/* #undef DEFAULT_SYSCONFDIR */
/* #undef DEFAULT_TMPDIR */
/* #undef INSTALL_SBINDIR */
/* #undef INSTALL_BINDIR */
/* #undef INSTALL_MYSQLSHAREDIR */
/* #undef INSTALL_SHAREDIR */
/* #undef INSTALL_PLUGINDIR */
/* #undef INSTALL_INCLUDEDIR */
/* #undef INSTALL_MYSQLDATADIR */
/* #undef INSTALL_MYSQLKEYRINGDIR */
/* #undef INSTALL_PLUGINTESTDIR */
/* #undef INSTALL_INFODIR */
/* #undef INSTALL_MYSQLTESTDIR */
/* #undef INSTALL_DOCREADMEDIR */
/* #undef INSTALL_DOCDIR */
/* #undef INSTALL_MANDIR */
/* #undef INSTALL_SUPPORTFILESDIR */
/* #undef INSTALL_LIBDIR */

/*
 * Readline
 */
/* #undef HAVE_MBSTATE_T */
/* #undef HAVE_LANGINFO_CODESET */
/* #undef HAVE_WCSDUP */
/* #undef HAVE_WCHAR_T */
/* #undef HAVE_WINT_T */
/* #undef HAVE_CURSES_H */
/* #undef HAVE_NCURSES_H */
/* #undef USE_LIBEDIT_INTERFACE */
/* #undef HAVE_HIST_ENTRY */
/* #undef USE_NEW_EDITLINE_INTERFACE */

/*
 * Libedit
 */
/* #undef HAVE_DECL_TGOTO */

/*
 * Character sets
 */
#define MYSQL_DEFAULT_CHARSET_NAME "latin1"
#define MYSQL_DEFAULT_COLLATION_NAME "latin1_swedish_ci"

/*
 * Performance schema
 */
/* #undef WITH_PERFSCHEMA_STORAGE_ENGINE */
/* #undef DISABLE_PSI_THREAD */
/* #undef DISABLE_PSI_MUTEX */
/* #undef DISABLE_PSI_RWLOCK */
/* #undef DISABLE_PSI_COND */
/* #undef DISABLE_PSI_FILE */
/* #undef DISABLE_PSI_TABLE */
/* #undef DISABLE_PSI_SOCKET */
/* #undef DISABLE_PSI_STAGE */
/* #undef DISABLE_PSI_STATEMENT */
/* #undef DISABLE_PSI_SP */
/* #undef DISABLE_PSI_PS */
/* #undef DISABLE_PSI_IDLE */
/* #undef DISABLE_PSI_ERROR */
/* #undef DISABLE_PSI_STATEMENT_DIGEST */
/* #undef DISABLE_PSI_METADATA */
/* #undef DISABLE_PSI_MEMORY */
/* #undef DISABLE_PSI_TRANSACTION */

/*
 * MySQL version
 */
#define MYSQL_VERSION_MAJOR 
#define MYSQL_VERSION_MINOR 
#define MYSQL_VERSION_PATCH 
#define MYSQL_VERSION_EXTRA ""
#define PACKAGE "mysql"
#define PACKAGE_BUGREPORT ""
#define PACKAGE_NAME "MySQL Server"
#define PACKAGE_STRING "MySQL Server "
#define PACKAGE_TARNAME "mysql"
#define PACKAGE_VERSION ""
#define VERSION ""
#define PROTOCOL_VERSION 10

/*
 * CPU info
 */
/* #undef CPU_LEVEL1_DCACHE_LINESIZE */

/*
 * NDB
 */
/* #undef WITH_NDBCLUSTER_STORAGE_ENGINE */
/* #undef HAVE_PTHREAD_SETSCHEDPARAM */

/*
 * Other
 */
/* #undef EXTRA_DEBUG */

/*
 * Hardcoded values needed by libevent/NDB/memcached
 */
#define HAVE_FCNTL_H 1
#define HAVE_GETADDRINFO 1
#define HAVE_INTTYPES_H 1
/* libevent's select.c is not Windows compatible */
#ifndef _WIN32
#define HAVE_SELECT 1
#endif
#define HAVE_SIGNAL_H 1
#define HAVE_STDARG_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRDUP 1
#define HAVE_STRTOK_R 1
#define HAVE_STRTOLL 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define SIZEOF_CHAR 1

/*
 * Needed by libevent
 */
/* #undef HAVE_SOCKLEN_T */

/* For --secure-file-priv */
/* #undef DEFAULT_SECURE_FILE_PRIV_DIR */
/* #undef HAVE_LIBNUMA */

/* For default value of --early_plugin_load */
/* #undef DEFAULT_EARLY_PLUGIN_LOAD */

#define SO_EXT ".dll"

#endif
