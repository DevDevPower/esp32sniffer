#ifndef USE_LIBSQLITE3
/******************************************************************************
** This file is an amalgamation of many separate C source files from SQLite
** version 3.39.4.  By combining all the individual C code files into this
** single large file, the entire code can be compiled as a single translation
** unit.  This allows many compilers to do optimizations that would not be
** possible if the files were compiled separately.  Performance improvements
** of 5% or more are commonly seen when SQLite is compiled as a single
** translation unit.
**
** This file is all you need to compile SQLite.  To use SQLite in other
** programs, you need this file and the "sqlite3.h" header file that defines
** the programming interface to the SQLite library.  (If you do not have
** the "sqlite3.h" header file at hand, you will find a copy embedded within
** the text of this file.  Search for "Begin file sqlite3.h" to find the start
** of the embedded sqlite3.h header file.) Additional code files may be needed
** if you want a wrapper to interface SQLite with your choice of programming
** language. The code for the "sqlite3" command-line shell is also in a
** separate file. This file contains only code for the core SQLite library.
*/
#define SQLITE_CORE 1
#define SQLITE_AMALGAMATION 1
#ifndef SQLITE_PRIVATE
# define SQLITE_PRIVATE static
#endif
/************** Begin file sqliteInt.h ***************************************/
/*
** 2001 September 15
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** Internal interface definitions for SQLite.
**
*/
#ifndef SQLITEINT_H
#define SQLITEINT_H

/* Special Comments:
**
** Some comments have special meaning to the tools that measure test
** coverage:
**
**    NO_TEST                     - The branches on this line are not
**                                  measured by branch coverage.  This is
**                                  used on lines of code that actually
**                                  implement parts of coverage testing.
**
**    OPTIMIZATION-IF-TRUE        - This branch is allowed to alway be false
**                                  and the correct answer is still obtained,
**                                  though perhaps more slowly.
**
**    OPTIMIZATION-IF-FALSE       - This branch is allowed to alway be true
**                                  and the correct answer is still obtained,
**                                  though perhaps more slowly.
**
**    PREVENTS-HARMLESS-OVERREAD  - This branch prevents a buffer overread
**                                  that would be harmless and undetectable
**                                  if it did occur.
**
** In all cases, the special comment must be enclosed in the usual
** slash-asterisk...asterisk-slash comment marks, with no spaces between the
** asterisks and the comment text.
*/

/*
** Make sure the Tcl calling convention macro is defined.  This macro is
** only used by test code and Tcl integration code.
*/
#ifndef SQLITE_TCLAPI
#  define SQLITE_TCLAPI
#endif

/*
** Include the header file used to customize the compiler options for MSVC.
** This should be done first so that it can successfully prevent spurious
** compiler warnings due to subsequent content in this file and other files
** that are included by this file.
*/
/************** Include msvc.h in the middle of sqliteInt.h ******************/
/************** Begin file msvc.h ********************************************/
/*
** 2015 January 12
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
******************************************************************************
**
** This file contains code that is specific to MSVC.
*/
#ifndef SQLITE_MSVC_H
#define SQLITE_MSVC_H

#if defined(_MSC_VER)
#pragma warning(disable : 4054)
#pragma warning(disable : 4055)
#pragma warning(disable : 4100)
#pragma warning(disable : 4127)
#pragma warning(disable : 4130)
#pragma warning(disable : 4152)
#pragma warning(disable : 4189)
#pragma warning(disable : 4206)
#pragma warning(disable : 4210)
#pragma warning(disable : 4232)
#pragma warning(disable : 4244)
#pragma warning(disable : 4305)
#pragma warning(disable : 4306)
#pragma warning(disable : 4702)
#pragma warning(disable : 4706)
#endif /* defined(_MSC_VER) */

#if defined(_MSC_VER) && !defined(_WIN64)
#undef SQLITE_4_BYTE_ALIGNED_MALLOC
#define SQLITE_4_BYTE_ALIGNED_MALLOC
#endif /* defined(_MSC_VER) && !defined(_WIN64) */

#endif /* SQLITE_MSVC_H */

/************** End of msvc.h ************************************************/
/************** Continuing where we left off in sqliteInt.h ******************/

/*
** Special setup for VxWorks
*/
/************** Include vxworks.h in the middle of sqliteInt.h ***************/
/************** Begin file vxworks.h *****************************************/
/*
** 2015-03-02
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
******************************************************************************
**
** This file contains code that is specific to Wind River's VxWorks
*/
#if defined(__RTP__) || defined(_WRS_KERNEL)
/* This is VxWorks.  Set up things specially for that OS
*/
#include <vxWorks.h>
#include <pthread.h>  /* amalgamator: dontcache */
#define OS_VXWORKS 1
#define SQLITE_OS_OTHER 0
#define SQLITE_HOMEGROWN_RECURSIVE_MUTEX 1
#define SQLITE_OMIT_LOAD_EXTENSION 1
#define SQLITE_ENABLE_LOCKING_STYLE 0
#define HAVE_UTIME 1
#else
/* This is not VxWorks. */
#define OS_VXWORKS 0
#define HAVE_FCHOWN 1
#define HAVE_READLINK 1
#define HAVE_LSTAT 1
#endif /* defined(_WRS_KERNEL) */

/************** End of vxworks.h *********************************************/
/************** Continuing where we left off in sqliteInt.h ******************/

/*
** These #defines should enable >2GB file support on POSIX if the
** underlying operating system supports it.  If the OS lacks
** large file support, or if the OS is windows, these should be no-ops.
**
** Ticket #2739:  The _LARGEFILE_SOURCE macro must appear before any
** system #includes.  Hence, this block of code must be the very first
** code in all source files.
**
** Large file support can be disabled using the -DSQLITE_DISABLE_LFS switch
** on the compiler command line.  This is necessary if you are compiling
** on a recent machine (ex: Red Hat 7.2) but you want your code to work
** on an older machine (ex: Red Hat 6.0).  If you compile on Red Hat 7.2
** without this option, LFS is enable.  But LFS does not exist in the kernel
** in Red Hat 6.0, so the code won't work.  Hence, for maximum binary
** portability you should omit LFS.
**
** The previous paragraph was written in 2005.  (This paragraph is written
** on 2008-11-28.) These days, all Linux kernels support large files, so
** you should probably leave LFS enabled.  But some embedded platforms might
** lack LFS in which case the SQLITE_DISABLE_LFS macro might still be useful.
**
** Similar is true for Mac OS X.  LFS is only supported on Mac OS X 9 and later.
*/
#ifndef SQLITE_DISABLE_LFS
# define _LARGE_FILE       1
# ifndef _FILE_OFFSET_BITS
#   define _FILE_OFFSET_BITS 64
# endif
# define _LARGEFILE_SOURCE 1
#endif

/* The GCC_VERSION and MSVC_VERSION macros are used to
** conditionally include optimizations for each of these compilers.  A
** value of 0 means that compiler is not being used.  The
** SQLITE_DISABLE_INTRINSIC macro means do not use any compiler-specific
** optimizations, and hence set all compiler macros to 0
**
** There was once also a CLANG_VERSION macro.  However, we learn that the
** version numbers in clang are for "marketing" only and are inconsistent
** and unreliable.  Fortunately, all versions of clang also recognize the
** gcc version numbers and have reasonable settings for gcc version numbers,
** so the GCC_VERSION macro will be set to a correct non-zero value even
** when compiling with clang.
*/
#if defined(__GNUC__) && !defined(SQLITE_DISABLE_INTRINSIC)
# define GCC_VERSION (__GNUC__*1000000+__GNUC_MINOR__*1000+__GNUC_PATCHLEVEL__)
#else
# define GCC_VERSION 0
#endif
#if defined(_MSC_VER) && !defined(SQLITE_DISABLE_INTRINSIC)
# define MSVC_VERSION _MSC_VER
#else
# define MSVC_VERSION 0
#endif

/*
** Some C99 functions in "math.h" are only present for MSVC when its version
** is associated with Visual Studio 2013 or higher.
*/
#ifndef SQLITE_HAVE_C99_MATH_FUNCS
# if MSVC_VERSION==0 || MSVC_VERSION>=1800
#  define SQLITE_HAVE_C99_MATH_FUNCS (1)
# else
#  define SQLITE_HAVE_C99_MATH_FUNCS (0)
# endif
#endif

/* Needed for various definitions... */
#if defined(__GNUC__) && !defined(_GNU_SOURCE)
# define _GNU_SOURCE
#endif

#if defined(__OpenBSD__) && !defined(_BSD_SOURCE)
# define _BSD_SOURCE
#endif

/*
** Macro to disable warnings about missing "break" at the end of a "case".
*/
#if GCC_VERSION>=7000000
# define deliberate_fall_through __attribute__((fallthrough));
#else
# define deliberate_fall_through
#endif

/*
** For MinGW, check to see if we can include the header file containing its
** version information, among other things.  Normally, this internal MinGW
** header file would [only] be included automatically by other MinGW header
** files; however, the contained version information is now required by this
** header file to work around binary compatibility issues (see below) and
** this is the only known way to reliably obtain it.  This entire #if block
** would be completely unnecessary if there was any other way of detecting
** MinGW via their preprocessor (e.g. if they customized their GCC to define
** some MinGW-specific macros).  When compiling for MinGW, either the
** _HAVE_MINGW_H or _HAVE__MINGW_H (note the extra underscore) macro must be
** defined; otherwise, detection of conditions specific to MinGW will be
** disabled.
*/
#if defined(_HAVE_MINGW_H)
# include "mingw.h"
#elif defined(_HAVE__MINGW_H)
# include "_mingw.h"
#endif

/*
** For MinGW version 4.x (and higher), check to see if the _USE_32BIT_TIME_T
** define is required to maintain binary compatibility with the MSVC runtime
** library in use (e.g. for Windows XP).
*/
#if !defined(_USE_32BIT_TIME_T) && !defined(_USE_64BIT_TIME_T) && \
    defined(_WIN32) && !defined(_WIN64) && \
    defined(__MINGW_MAJOR_VERSION) && __MINGW_MAJOR_VERSION >= 4 && \
    defined(__MSVCRT__)
# define _USE_32BIT_TIME_T
#endif

/* Optionally #include a user-defined header, whereby compilation options
** may be set prior to where they take effect, but after platform setup.
** If SQLITE_CUSTOM_INCLUDE=? is defined, its value names the #include
** file.
*/
#ifdef SQLITE_CUSTOM_INCLUDE
# define INC_STRINGIFY_(f) #f
# define INC_STRINGIFY(f) INC_STRINGIFY_(f)
# include INC_STRINGIFY(SQLITE_CUSTOM_INCLUDE)
#endif

/* The public SQLite interface.  The _FILE_OFFSET_BITS macro must appear
** first in QNX.  Also, the _USE_32BIT_TIME_T macro must appear first for
** MinGW.
*/
/************** Include sqlite3.h in the middle of sqliteInt.h ***************/
/************** Begin file sqlite3.h *****************************************/
/*
** 2001-09-15
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This header file defines the interface that the SQLite library
** presents to client programs.  If a C-function, structure, datatype,
** or constant definition does not appear in this file, then it is
** not a published API of SQLite, is subject to change without
** notice, and should not be referenced by programs that use SQLite.
**
** Some of the definitions that are in this file are marked as
** "experimental".  Experimental interfaces are normally new
** features recently added to SQLite.  We do not anticipate changes
** to experimental interfaces but reserve the right to make minor changes
** if experience from use "in the wild" suggest such changes are prudent.
**
** The official C-language API documentation for SQLite is derived
** from comments in this file.  This file is the authoritative source
** on how SQLite interfaces are supposed to operate.
**
** The name of this file under configuration management is "sqlite.h.in".
** The makefile makes some minor changes to this file (such as inserting
** the version number) and changes its name to "sqlite3.h" as
** part of the build process.
*/
#ifndef SQLITE3_H
#define SQLITE3_H
#include <stdarg.h>     /* Needed for the definition of va_list */

/*
** Make sure we can call this stuff from C++.
*/
#if 0
extern "C" {
#endif


/*
** Facilitate override of interface linkage and calling conventions.
** Be aware that these macros may not be used within this particular
** translation of the amalgamation and its associated header file.
**
** The SQLITE_EXTERN and SQLITE_API macros are used to instruct the
** compiler that the target identifier should have external linkage.
**
** The SQLITE_CDECL macro is used to set the calling convention for
** public functions that accept a variable number of arguments.
**
** The SQLITE_APICALL macro is used to set the calling convention for
** public functions that accept a fixed number of arguments.
**
** The SQLITE_STDCALL macro is no longer used and is now deprecated.
**
** The SQLITE_CALLBACK macro is used to set the calling convention for
** function pointers.
**
** The SQLITE_SYSAPI macro is used to set the calling convention for
** functions provided by the operating system.
**
** Currently, the SQLITE_CDECL, SQLITE_APICALL, SQLITE_CALLBACK, and
** SQLITE_SYSAPI macros are used only when building for environments
** that require non-default calling conventions.
*/
#ifndef SQLITE_EXTERN
# define SQLITE_EXTERN extern
#endif
#ifndef SQLITE_API
# define SQLITE_API
#endif
#ifndef SQLITE_CDECL
# define SQLITE_CDECL
#endif
#ifndef SQLITE_APICALL
# define SQLITE_APICALL
#endif
#ifndef SQLITE_STDCALL
# define SQLITE_STDCALL SQLITE_APICALL
#endif
#ifndef SQLITE_CALLBACK
# define SQLITE_CALLBACK
#endif
#ifndef SQLITE_SYSAPI
# define SQLITE_SYSAPI
#endif

/*
** These no-op macros are used in front of interfaces to mark those
** interfaces as either deprecated or experimental.  New applications
** should not use deprecated interfaces - they are supported for backwards
** compatibility only.  Application writers should be aware that
** experimental interfaces are subject to change in point releases.
**
** These macros used to resolve to various kinds of compiler magic that
** would generate warning messages when they were used.  But that
** compiler magic ended up generating such a flurry of bug reports
** that we have taken it all out and gone back to using simple
** noop macros.
*/
#define SQLITE_DEPRECATED
#define SQLITE_EXPERIMENTAL

/*
** Ensure these symbols were not defined by some previous header file.
*/
#ifdef SQLITE_VERSION
# undef SQLITE_VERSION
#endif
#ifdef SQLITE_VERSION_NUMBER
# undef SQLITE_VERSION_NUMBER
#endif

/*
** CAPI3REF: Compile-Time Library Version Numbers
**
** ^(The [SQLITE_VERSION] C preprocessor macro in the sqlite3.h header
** evaluates to a string literal that is the SQLite version in the
** format "X.Y.Z" where X is the major version number (always 3 for
** SQLite3) and Y is the minor version number and Z is the release number.)^
** ^(The [SQLITE_VERSION_NUMBER] C preprocessor macro resolves to an integer
** with the value (X*1000000 + Y*1000 + Z) where X, Y, and Z are the same
** numbers used in [SQLITE_VERSION].)^
** The SQLITE_VERSION_NUMBER for any given release of SQLite will also
** be larger than the release from which it is derived.  Either Y will
** be held constant and Z will be incremented or else Y will be incremented
** and Z will be reset to zero.
**
** Since [version 3.6.18] ([dateof:3.6.18]),
** SQLite source code has been stored in the
** <a href="http://www.fossil-scm.org/">Fossil configuration management
** system</a>.  ^The SQLITE_SOURCE_ID macro evaluates to
** a string which identifies a particular check-in of SQLite
** within its configuration management system.  ^The SQLITE_SOURCE_ID
** string contains the date and time of the check-in (UTC) and a SHA1
** or SHA3-256 hash of the entire source tree.  If the source code has
** been edited in any way since it was last checked in, then the last
** four hexadecimal digits of the hash may be modified.
**
** See also: [sqlite3_libversion()],
** [sqlite3_libversion_number()], [sqlite3_sourceid()],
** [sqlite_version()] and [sqlite_source_id()].
*/
#define SQLITE_VERSION        "3.39.4"
#define SQLITE_VERSION_NUMBER 3039004
#define SQLITE_SOURCE_ID      "2022-09-29 15:55:41 a29f9949895322123f7c38fbe94c649a9d6e6c9cd0c3b41c96d694552f26b309"

/*
** CAPI3REF: Run-Time Library Version Numbers
** KEYWORDS: sqlite3_version sqlite3_sourceid
**
** These interfaces provide the same information as the [SQLITE_VERSION],
** [SQLITE_VERSION_NUMBER], and [SQLITE_SOURCE_ID] C preprocessor macros
** but are associated with the library instead of the header file.  ^(Cautious
** programmers might include assert() statements in their application to
** verify that values returned by these interfaces match the macros in
** the header, and thus ensure that the application is
** compiled with matching library and header files.
**
** <blockquote><pre>
** assert( sqlite3_libversion_number()==SQLITE_VERSION_NUMBER );
** assert( strncmp(sqlite3_sourceid(),SQLITE_SOURCE_ID,80)==0 );
** assert( strcmp(sqlite3_libversion(),SQLITE_VERSION)==0 );
** </pre></blockquote>)^
**
** ^The sqlite3_version[] string constant contains the text of [SQLITE_VERSION]
** macro.  ^The sqlite3_libversion() function returns a pointer to the
** to the sqlite3_version[] string constant.  The sqlite3_libversion()
** function is provided for use in DLLs since DLL users usually do not have
** direct access to string constants within the DLL.  ^The
** sqlite3_libversion_number() function returns an integer equal to
** [SQLITE_VERSION_NUMBER].  ^(The sqlite3_sourceid() function returns
** a pointer to a string constant whose value is the same as the
** [SQLITE_SOURCE_ID] C preprocessor macro.  Except if SQLite is built
** using an edited copy of [the amalgamation], then the last four characters
** of the hash might be different from [SQLITE_SOURCE_ID].)^
**
** See also: [sqlite_version()] and [sqlite_source_id()].
*/
SQLITE_API const char sqlite3_version[] = SQLITE_VERSION;
SQLITE_API const char *sqlite3_libversion(void);
SQLITE_API const char *sqlite3_sourceid(void);
SQLITE_API int sqlite3_libversion_number(void);

/*
** CAPI3REF: Run-Time Library Compilation Options Diagnostics
**
** ^The sqlite3_compileoption_used() function returns 0 or 1
** indicating whether the specified option was defined at
** compile time.  ^The SQLITE_ prefix may be omitted from the
** option name passed to sqlite3_compileoption_used().
**
** ^The sqlite3_compileoption_get() function allows iterating
** over the list of options that were defined at compile time by
** returning the N-th compile time option string.  ^If N is out of range,
** sqlite3_compileoption_get() returns a NULL pointer.  ^The SQLITE_
** prefix is omitted from any strings returned by
** sqlite3_compileoption_get().
**
** ^Support for the diagnostic functions sqlite3_compileoption_used()
** and sqlite3_compileoption_get() may be omitted by specifying the
** [SQLITE_OMIT_COMPILEOPTION_DIAGS] option at compile time.
**
** See also: SQL functions [sqlite_compileoption_used()] and
** [sqlite_compileoption_get()] and the [compile_options pragma].
*/
#ifndef SQLITE_OMIT_COMPILEOPTION_DIAGS
SQLITE_API int sqlite3_compileoption_used(const char *zOptName);
SQLITE_API const char *sqlite3_compileoption_get(int N);
#else
# define sqlite3_compileoption_used(X) 0
# define sqlite3_compileoption_get(X)  ((void*)0)
#endif

/*
** CAPI3REF: Test To See If The Library Is Threadsafe
**
** ^The sqlite3_threadsafe() function returns zero if and only if
** SQLite was compiled with mutexing code omitted due to the
** [SQLITE_THREADSAFE] compile-time option being set to 0.
**
** SQLite can be compiled with or without mutexes.  When
** the [SQLITE_THREADSAFE] C preprocessor macro is 1 or 2, mutexes
** are enabled and SQLite is threadsafe.  When the
** [SQLITE_THREADSAFE] macro is 0,
** the mutexes are omitted.  Without the mutexes, it is not safe
** to use SQLite concurrently from more than one thread.
**
** Enabling mutexes incurs a measurable performance penalty.
** So if speed is of utmost importance, it makes sense to disable
** the mutexes.  But for maximum safety, mutexes should be enabled.
** ^The default behavior is for mutexes to be enabled.
**
** This interface can be used by an application to make sure that the
** version of SQLite that it is linking against was compiled with
** the desired setting of the [SQLITE_THREADSAFE] macro.
**
** This interface only reports on the compile-time mutex setting
** of the [SQLITE_THREADSAFE] flag.  If SQLite is compiled with
** SQLITE_THREADSAFE=1 or =2 then mutexes are enabled by default but
** can be fully or partially disabled using a call to [sqlite3_config()]
** with the verbs [SQLITE_CONFIG_SINGLETHREAD], [SQLITE_CONFIG_MULTITHREAD],
** or [SQLITE_CONFIG_SERIALIZED].  ^(The return value of the
** sqlite3_threadsafe() function shows only the compile-time setting of
** thread safety, not any run-time changes to that setting made by
** sqlite3_config(). In other words, the return value from sqlite3_threadsafe()
** is unchanged by calls to sqlite3_config().)^
**
** See the [threading mode] documentation for additional information.
*/
SQLITE_API int sqlite3_threadsafe(void);

/*
** CAPI3REF: Database Connection Handle
** KEYWORDS: {database connection} {database connections}
**
** Each open SQLite database is represented by a pointer to an instance of
** the opaque structure named "sqlite3".  It is useful to think of an sqlite3
** pointer as an object.  The [sqlite3_open()], [sqlite3_open16()], and
** [sqlite3_open_v2()] interfaces are its constructors, and [sqlite3_close()]
** and [sqlite3_close_v2()] are its destructors.  There are many other
** interfaces (such as
** [sqlite3_prepare_v2()], [sqlite3_create_function()], and
** [sqlite3_busy_timeout()] to name but three) that are methods on an
** sqlite3 object.
*/
typedef struct sqlite3 sqlite3;

/*
** CAPI3REF: 64-Bit Integer Types
** KEYWORDS: sqlite_int64 sqlite_uint64
**
** Because there is no cross-platform way to specify 64-bit integer types
** SQLite includes typedefs for 64-bit signed and unsigned integers.
**
** The sqlite3_int64 and sqlite3_uint64 are the preferred type definitions.
** The sqlite_int64 and sqlite_uint64 types are supported for backwards
** compatibility only.
**
** ^The sqlite3_int64 and sqlite_int64 types can store integer values
** between -9223372036854775808 and +9223372036854775807 inclusive.  ^The
** sqlite3_uint64 and sqlite_uint64 types can store integer values
** between 0 and +18446744073709551615 inclusive.
*/
#ifdef SQLITE_INT64_TYPE
  typedef SQLITE_INT64_TYPE sqlite_int64;
# ifdef SQLITE_UINT64_TYPE
    typedef SQLITE_UINT64_TYPE sqlite_uint64;
# else
    typedef unsigned SQLITE_INT64_TYPE sqlite_uint64;
# endif
#elif defined(_MSC_VER) || defined(__BORLANDC__)
  typedef __int64 sqlite_int64;
  typedef unsigned __int64 sqlite_uint64;
#else
  typedef long long int sqlite_int64;
  typedef unsigned long long int sqlite_uint64;
#endif
typedef sqlite_int64 sqlite3_int64;
typedef sqlite_uint64 sqlite3_uint64;

/*
** If compiling for a processor that lacks floating point support,
** substitute integer for floating-point.
*/
#ifdef SQLITE_OMIT_FLOATING_POINT
# define double sqlite3_int64
#endif

/*
** CAPI3REF: Closing A Database Connection
** DESTRUCTOR: sqlite3
**
** ^The sqlite3_close() and sqlite3_close_v2() routines are destructors
** for the [sqlite3] object.
** ^Calls to sqlite3_close() and sqlite3_close_v2() return [SQLITE_OK] if
** the [sqlite3] object is successfully destroyed and all associated
** resources are deallocated.
**
** Ideally, applications should [sqlite3_finalize | finalize] all
** [prepared statements], [sqlite3_blob_close | close] all [BLOB handles], and
** [sqlite3_backup_finish | finish] all [sqlite3_backup] objects associated
** with the [sqlite3] object prior to attempting to close the object.
** ^If the database connection is associated with unfinalized prepared
** statements, BLOB handlers, and/or unfinished sqlite3_backup objects then
** sqlite3_close() will leave the database connection open and return
** [SQLITE_BUSY]. ^If sqlite3_close_v2() is called with unfinalized prepared
** statements, unclosed BLOB handlers, and/or unfinished sqlite3_backups,
** it returns [SQLITE_OK] regardless, but instead of deallocating the database
** connection immediately, it marks the database connection as an unusable
** "zombie" and makes arrangements to automatically deallocate the database
** connection after all prepared statements are finalized, all BLOB handles
** are closed, and all backups have finished. The sqlite3_close_v2() interface
** is intended for use with host languages that are garbage collected, and
** where the order in which destructors are called is arbitrary.
**
** ^If an [sqlite3] object is destroyed while a transaction is open,
** the transaction is automatically rolled back.
**
** The C parameter to [sqlite3_close(C)] and [sqlite3_close_v2(C)]
** must be either a NULL
** pointer or an [sqlite3] object pointer obtained
** from [sqlite3_open()], [sqlite3_open16()], or
** [sqlite3_open_v2()], and not previously closed.
** ^Calling sqlite3_close() or sqlite3_close_v2() with a NULL pointer
** argument is a harmless no-op.
*/
SQLITE_API int sqlite3_close(sqlite3*);
SQLITE_API int sqlite3_close_v2(sqlite3*);

/*
** The type for a callback function.
** This is legacy and deprecated.  It is included for historical
** compatibility and is not documented.
*/
typedef int (*sqlite3_callback)(void*,int,char**, char**);

/*
** CAPI3REF: One-Step Query Execution Interface
** METHOD: sqlite3
**
** The sqlite3_exec() interface is a convenience wrapper around
** [sqlite3_prepare_v2()], [sqlite3_step()], and [sqlite3_finalize()],
** that allows an application to run multiple statements of SQL
** without having to use a lot of C code.
**
** ^The sqlite3_exec() interface runs zero or more UTF-8 encoded,
** semicolon-separate SQL statements passed into its 2nd argument,
** in the context of the [database connection] passed in as its 1st
** argument.  ^If the callback function of the 3rd argument to
** sqlite3_exec() is not NULL, then it is invoked for each result row
** coming out of the evaluated SQL statements.  ^The 4th argument to
** sqlite3_exec() is relayed through to the 1st argument of each
** callback invocation.  ^If the callback pointer to sqlite3_exec()
** is NULL, then no callback is ever invoked and result rows are
** ignored.
**
** ^If an error occurs while evaluating the SQL statements passed into
** sqlite3_exec(), then execution of the current statement stops and
** subsequent statements are skipped.  ^If the 5th parameter to sqlite3_exec()
** is not NULL then any error message is written into memory obtained
** from [sqlite3_malloc()] and passed back through the 5th parameter.
** To avoid memory leaks, the application should invoke [sqlite3_free()]
** on error message strings returned through the 5th parameter of
** sqlite3_exec() after the error message string is no longer needed.
** ^If the 5th parameter to sqlite3_exec() is not NULL and no errors
** occur, then sqlite3_exec() sets the pointer in its 5th parameter to
** NULL before returning.
**
** ^If an sqlite3_exec() callback returns non-zero, the sqlite3_exec()
** routine returns SQLITE_ABORT without invoking the callback again and
** without running any subsequent SQL statements.
**
** ^The 2nd argument to the sqlite3_exec() callback function is the
** number of columns in the result.  ^The 3rd argument to the sqlite3_exec()
** callback is an array of pointers to strings obtained as if from
** [sqlite3_column_text()], one for each column.  ^If an element of a
** result row is NULL then the corresponding string pointer for the
** sqlite3_exec() callback is a NULL pointer.  ^The 4th argument to the
** sqlite3_exec() callback is an array of pointers to strings where each
** entry represents the name of corresponding result column as obtained
** from [sqlite3_column_name()].
**
** ^If the 2nd parameter to sqlite3_exec() is a NULL pointer, a pointer
** to an empty string, or a pointer that contains only whitespace and/or
** SQL comments, then no SQL statements are evaluated and the database
** is not changed.
**
** Restrictions:
**
** <ul>
** <li> The application must ensure that the 1st parameter to sqlite3_exec()
**      is a valid and open [database connection].
** <li> The application must not close the [database connection] specified by
**      the 1st parameter to sqlite3_exec() while sqlite3_exec() is running.
** <li> The application must not modify the SQL statement text passed into
**      the 2nd parameter of sqlite3_exec() while sqlite3_exec() is running.
** </ul>
*/
SQLITE_API int sqlite3_exec(
  sqlite3*,                                  /* An open database */
  const char *sql,                           /* SQL to be evaluated */
  int (*callback)(void*,int,char**,char**),  /* Callback function */
  void *,                                    /* 1st argument to callback */
  char **errmsg                              /* Error msg written here */
);

/*
** CAPI3REF: Result Codes
** KEYWORDS: {result code definitions}
**
** Many SQLite functions return an integer result code from the set shown
** here in order to indicate success or failure.
**
** New error codes may be added in future versions of SQLite.
**
** See also: [extended result code definitions]
*/
#define SQLITE_OK           0   /* Successful result */
/* beginning-of-error-codes */
#define SQLITE_ERROR        1   /* Generic error */
#define SQLITE_INTERNAL     2   /* Internal logic error in SQLite */
#define SQLITE_PERM         3   /* Access permission denied */
#define SQLITE_ABORT        4   /* Callback routine requested an abort */
#define SQLITE_BUSY         5   /* The database file is locked */
#define SQLITE_LOCKED       6   /* A table in the database is locked */
#define SQLITE_NOMEM        7   /* A malloc() failed */
#define SQLITE_READONLY     8   /* Attempt to write a readonly database */
#define SQLITE_INTERRUPT    9   /* Operation terminated by sqlite3_interrupt()*/
#define SQLITE_IOERR       10   /* Some kind of disk I/O error occurred */
#define SQLITE_CORRUPT     11   /* The database disk image is malformed */
#define SQLITE_NOTFOUND    12   /* Unknown opcode in sqlite3_file_control() */
#define SQLITE_FULL        13   /* Insertion failed because database is full */
#define SQLITE_CANTOPEN    14   /* Unable to open the database file */
#define SQLITE_PROTOCOL    15   /* Database lock protocol error */
#define SQLITE_EMPTY       16   /* Internal use only */
#define SQLITE_SCHEMA      17   /* The database schema changed */
#define SQLITE_TOOBIG      18   /* String or BLOB exceeds size limit */
#define SQLITE_CONSTRAINT  19   /* Abort due to constraint violation */
#define SQLITE_MISMATCH    20   /* Data type mismatch */
#define SQLITE_MISUSE      21   /* Library used incorrectly */
#define SQLITE_NOLFS       22   /* Uses OS features not supported on host */
#define SQLITE_AUTH        23   /* Authorization denied */
#define SQLITE_FORMAT      24   /* Not used */
#define SQLITE_RANGE       25   /* 2nd parameter to sqlite3_bind out of range */
#define SQLITE_NOTADB      26   /* File opened that is not a database file */
#define SQLITE_NOTICE      27   /* Notifications from sqlite3_log() */
#define SQLITE_WARNING     28   /* Warnings from sqlite3_log() */
#define SQLITE_ROW         100  /* sqlite3_step() has another row ready */
#define SQLITE_DONE        101  /* sqlite3_step() has finished executing */
/* end-of-error-codes */

/*
** CAPI3REF: Extended Result Codes
** KEYWORDS: {extended result code definitions}
**
** In its default configuration, SQLite API routines return one of 30 integer
** [result codes].  However, experience has shown that many of
** these result codes are too coarse-grained.  They do not provide as
** much information about problems as programmers might like.  In an effort to
** address this, newer versions of SQLite (version 3.3.8 [dateof:3.3.8]
** and later) include
** support for additional result codes that provide more detailed information
** about errors. These [extended result codes] are enabled or disabled
** on a per database connection basis using the
** [sqlite3_extended_result_codes()] API.  Or, the extended code for
** the most recent error can be obtained using
** [sqlite3_extended_errcode()].
*/
#define SQLITE_ERROR_MISSING_COLLSEQ   (SQLITE_ERROR | (1<<8))
#define SQLITE_ERROR_RETRY             (SQLITE_ERROR | (2<<8))
#define SQLITE_ERROR_SNAPSHOT          (SQLITE_ERROR | (3<<8))
#define SQLITE_IOERR_READ              (SQLITE_IOERR | (1<<8))
#define SQLITE_IOERR_SHORT_READ        (SQLITE_IOERR | (2<<8))
#define SQLITE_IOERR_WRITE             (SQLITE_IOERR | (3<<8))
#define SQLITE_IOERR_FSYNC             (SQLITE_IOERR | (4<<8))
#define SQLITE_IOERR_DIR_FSYNC         (SQLITE_IOERR | (5<<8))
#define SQLITE_IOERR_TRUNCATE          (SQLITE_IOERR | (6<<8))
#define SQLITE_IOERR_FSTAT             (SQLITE_IOERR | (7<<8))
#define SQLITE_IOERR_UNLOCK            (SQLITE_IOERR | (8<<8))
#define SQLITE_IOERR_RDLOCK            (SQLITE_IOERR | (9<<8))
#define SQLITE_IOERR_DELETE            (SQLITE_IOERR | (10<<8))
#define SQLITE_IOERR_BLOCKED           (SQLITE_IOERR | (11<<8))
#define SQLITE_IOERR_NOMEM             (SQLITE_IOERR | (12<<8))
#define SQLITE_IOERR_ACCESS            (SQLITE_IOERR | (13<<8))
#define SQLITE_IOERR_CHECKRESERVEDLOCK (SQLITE_IOERR | (14<<8))
#define SQLITE_IOERR_LOCK              (SQLITE_IOERR | (15<<8))
#define SQLITE_IOERR_CLOSE             (SQLITE_IOERR | (16<<8))
#define SQLITE_IOERR_DIR_CLOSE         (SQLITE_IOERR | (17<<8))
#define SQLITE_IOERR_SHMOPEN           (SQLITE_IOERR | (18<<8))
#define SQLITE_IOERR_SHMSIZE           (SQLITE_IOERR | (19<<8))
#define SQLITE_IOERR_SHMLOCK           (SQLITE_IOERR | (20<<8))
#define SQLITE_IOERR_SHMMAP            (SQLITE_IOERR | (21<<8))
#define SQLITE_IOERR_SEEK              (SQLITE_IOERR | (22<<8))
#define SQLITE_IOERR_DELETE_NOENT      (SQLITE_IOERR | (23<<8))
#define SQLITE_IOERR_MMAP              (SQLITE_IOERR | (24<<8))
#define SQLITE_IOERR_GETTEMPPATH       (SQLITE_IOERR | (25<<8))
#define SQLITE_IOERR_CONVPATH          (SQLITE_IOERR | (26<<8))
#define SQLITE_IOERR_VNODE             (SQLITE_IOERR | (27<<8))
#define SQLITE_IOERR_AUTH              (SQLITE_IOERR | (28<<8))
#define SQLITE_IOERR_BEGIN_ATOMIC      (SQLITE_IOERR | (29<<8))
#define SQLITE_IOERR_COMMIT_ATOMIC     (SQLITE_IOERR | (30<<8))
#define SQLITE_IOERR_ROLLBACK_ATOMIC   (SQLITE_IOERR | (31<<8))
#define SQLITE_IOERR_DATA              (SQLITE_IOERR | (32<<8))
#define SQLITE_IOERR_CORRUPTFS         (SQLITE_IOERR | (33<<8))
#define SQLITE_LOCKED_SHAREDCACHE      (SQLITE_LOCKED |  (1<<8))
#define SQLITE_LOCKED_VTAB             (SQLITE_LOCKED |  (2<<8))
#define SQLITE_BUSY_RECOVERY           (SQLITE_BUSY   |  (1<<8))
#define SQLITE_BUSY_SNAPSHOT           (SQLITE_BUSY   |  (2<<8))
#define SQLITE_BUSY_TIMEOUT            (SQLITE_BUSY   |  (3<<8))
#define SQLITE_CANTOPEN_NOTEMPDIR      (SQLITE_CANTOPEN | (1<<8))
#define SQLITE_CANTOPEN_ISDIR          (SQLITE_CANTOPEN | (2<<8))
#define SQLITE_CANTOPEN_FULLPATH       (SQLITE_CANTOPEN | (3<<8))
#define SQLITE_CANTOPEN_CONVPATH       (SQLITE_CANTOPEN | (4<<8))
#define SQLITE_CANTOPEN_DIRTYWAL       (SQLITE_CANTOPEN | (5<<8)) /* Not Used */
#define SQLITE_CANTOPEN_SYMLINK        (SQLITE_CANTOPEN | (6<<8))
#define SQLITE_CORRUPT_VTAB            (SQLITE_CORRUPT | (1<<8))
#define SQLITE_CORRUPT_SEQUENCE        (SQLITE_CORRUPT | (2<<8))
#define SQLITE_CORRUPT_INDEX           (SQLITE_CORRUPT | (3<<8))
#define SQLITE_READONLY_RECOVERY       (SQLITE_READONLY | (1<<8))
#define SQLITE_READONLY_CANTLOCK       (SQLITE_READONLY | (2<<8))
#define SQLITE_READONLY_ROLLBACK       (SQLITE_READONLY | (3<<8))
#define SQLITE_READONLY_DBMOVED        (SQLITE_READONLY | (4<<8))
#define SQLITE_READONLY_CANTINIT       (SQLITE_READONLY | (5<<8))
#define SQLITE_READONLY_DIRECTORY      (SQLITE_READONLY | (6<<8))
#define SQLITE_ABORT_ROLLBACK          (SQLITE_ABORT | (2<<8))
#define SQLITE_CONSTRAINT_CHECK        (SQLITE_CONSTRAINT | (1<<8))
#define SQLITE_CONSTRAINT_COMMITHOOK   (SQLITE_CONSTRAINT | (2<<8))
#define SQLITE_CONSTRAINT_FOREIGNKEY   (SQLITE_CONSTRAINT | (3<<8))
#define SQLITE_CONSTRAINT_FUNCTION     (SQLITE_CONSTRAINT | (4<<8))
#define SQLITE_CONSTRAINT_NOTNULL      (SQLITE_CONSTRAINT | (5<<8))
#define SQLITE_CONSTRAINT_PRIMARYKEY   (SQLITE_CONSTRAINT | (6<<8))
#define SQLITE_CONSTRAINT_TRIGGER      (SQLITE_CONSTRAINT | (7<<8))
#define SQLITE_CONSTRAINT_UNIQUE       (SQLITE_CONSTRAINT | (8<<8))
#define SQLITE_CONSTRAINT_VTAB         (SQLITE_CONSTRAINT | (9<<8))
#define SQLITE_CONSTRAINT_ROWID        (SQLITE_CONSTRAINT |(10<<8))
#define SQLITE_CONSTRAINT_PINNED       (SQLITE_CONSTRAINT |(11<<8))
#define SQLITE_CONSTRAINT_DATATYPE     (SQLITE_CONSTRAINT |(12<<8))
#define SQLITE_NOTICE_RECOVER_WAL      (SQLITE_NOTICE | (1<<8))
#define SQLITE_NOTICE_RECOVER_ROLLBACK (SQLITE_NOTICE | (2<<8))
#define SQLITE_WARNING_AUTOINDEX       (SQLITE_WARNING | (1<<8))
#define SQLITE_AUTH_USER               (SQLITE_AUTH | (1<<8))
#define SQLITE_OK_LOAD_PERMANENTLY     (SQLITE_OK | (1<<8))
#define SQLITE_OK_SYMLINK              (SQLITE_OK | (2<<8)) /* internal use only */

/*
** CAPI3REF: Flags For File Open Operations
**
** These bit values are intended for use in the
** 3rd parameter to the [sqlite3_open_v2()] interface and
** in the 4th parameter to the [sqlite3_vfs.xOpen] method.
**
** Only those flags marked as "Ok for sqlite3_open_v2()" may be
** used as the third argument to the [sqlite3_open_v2()] interface.
** The other flags have historically been ignored by sqlite3_open_v2(),
** though future versions of SQLite might change so that an error is
** raised if any of the disallowed bits are passed into sqlite3_open_v2().
** Applications should not depend on the historical behavior.
**
** Note in particular that passing the SQLITE_OPEN_EXCLUSIVE flag into
** [sqlite3_open_v2()] does *not* cause the underlying database file
** to be opened using O_EXCL.  Passing SQLITE_OPEN_EXCLUSIVE into
** [sqlite3_open_v2()] has historically be a no-op and might become an
** error in future versions of SQLite.
*/
#define SQLITE_OPEN_READONLY         0x00000001  /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_READWRITE        0x00000002  /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_CREATE           0x00000004  /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_DELETEONCLOSE    0x00000008  /* VFS only */
#define SQLITE_OPEN_EXCLUSIVE        0x00000010  /* VFS only */
#define SQLITE_OPEN_AUTOPROXY        0x00000020  /* VFS only */
#define SQLITE_OPEN_URI              0x00000040  /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_MEMORY           0x00000080  /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_MAIN_DB          0x00000100  /* VFS only */
#define SQLITE_OPEN_TEMP_DB          0x00000200  /* VFS only */
#define SQLITE_OPEN_TRANSIENT_DB     0x00000400  /* VFS only */
#define SQLITE_OPEN_MAIN_JOURNAL     0x00000800  /* VFS only */
#define SQLITE_OPEN_TEMP_JOURNAL     0x00001000  /* VFS only */
#define SQLITE_OPEN_SUBJOURNAL       0x00002000  /* VFS only */
#define SQLITE_OPEN_SUPER_JOURNAL    0x00004000  /* VFS only */
#define SQLITE_OPEN_NOMUTEX          0x00008000  /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_FULLMUTEX        0x00010000  /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_SHAREDCACHE      0x00020000  /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_PRIVATECACHE     0x00040000  /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_WAL              0x00080000  /* VFS only */
#define SQLITE_OPEN_NOFOLLOW         0x01000000  /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_EXRESCODE        0x02000000  /* Extended result codes */

/* Reserved:                         0x00F00000 */
/* Legacy compatibility: */
#define SQLITE_OPEN_MASTER_JOURNAL   0x00004000  /* VFS only */


/*
** CAPI3REF: Device Characteristics
**
** The xDeviceCharacteristics method of the [sqlite3_io_methods]
** object returns an integer which is a vector of these
** bit values expressing I/O characteristics of the mass storage
** device that holds the file that the [sqlite3_io_methods]
** refers to.
**
** The SQLITE_IOCAP_ATOMIC property means that all writes of
** any size are atomic.  The SQLITE_IOCAP_ATOMICnnn values
** mean that writes of blocks that are nnn bytes in size and
** are aligned to an address which is an integer multiple of
** nnn are atomic.  The SQLITE_IOCAP_SAFE_APPEND value means
** that when data is appended to a file, the data is appended
** first then the size of the file is extended, never the other
** way around.  The SQLITE_IOCAP_SEQUENTIAL property means that
** information is written to disk in the same order as calls
** to xWrite().  The SQLITE_IOCAP_POWERSAFE_OVERWRITE property means that
** after reboot following a crash or power loss, the only bytes in a
** file that were written at the application level might have changed
** and that adjacent bytes, even bytes within the same sector are
** guaranteed to be unchanged.  The SQLITE_IOCAP_UNDELETABLE_WHEN_OPEN
** flag indicates that a file cannot be deleted when open.  The
** SQLITE_IOCAP_IMMUTABLE flag indicates that the file is on
** read-only media and cannot be changed even by processes with
** elevated privileges.
**
** The SQLITE_IOCAP_BATCH_ATOMIC property means that the underlying
** filesystem supports doing multiple write operations atomically when those
** write operations are bracketed by [SQLITE_FCNTL_BEGIN_ATOMIC_WRITE] and
** [SQLITE_FCNTL_COMMIT_ATOMIC_WRITE].
*/
#define SQLITE_IOCAP_ATOMIC                 0x00000001
#define SQLITE_IOCAP_ATOMIC512              0x00000002
#define SQLITE_IOCAP_ATOMIC1K               0x00000004
#define SQLITE_IOCAP_ATOMIC2K               0x00000008
#define SQLITE_IOCAP_ATOMIC4K               0x00000010
#define SQLITE_IOCAP_ATOMIC8K               0x00000020
#define SQLITE_IOCAP_ATOMIC16K              0x00000040
#define SQLITE_IOCAP_ATOMIC32K              0x00000080
#define SQLITE_IOCAP_ATOMIC64K              0x00000100
#define SQLITE_IOCAP_SAFE_APPEND            0x00000200
#define SQLITE_IOCAP_SEQUENTIAL             0x00000400
#define SQLITE_IOCAP_UNDELETABLE_WHEN_OPEN  0x00000800
#define SQLITE_IOCAP_POWERSAFE_OVERWRITE    0x00001000
#define SQLITE_IOCAP_IMMUTABLE              0x00002000
#define SQLITE_IOCAP_BATCH_ATOMIC           0x00004000

/*
** CAPI3REF: File Locking Levels
**
** SQLite uses one of these integer values as the second
** argument to calls it makes to the xLock() and xUnlock() methods
** of an [sqlite3_io_methods] object.
*/
#define SQLITE_LOCK_NONE          0
#define SQLITE_LOCK_SHARED        1
#define SQLITE_LOCK_RESERVED      2
#define SQLITE_LOCK_PENDING       3
#define SQLITE_LOCK_EXCLUSIVE     4

/*
** CAPI3REF: Synchronization Type Flags
**
** When SQLite invokes the xSync() method of an
** [sqlite3_io_methods] object it uses a combination of
** these integer values as the second argument.
**
** When the SQLITE_SYNC_DATAONLY flag is used, it means that the
** sync operation only needs to flush data to mass storage.  Inode
** information need not be flushed. If the lower four bits of the flag
** equal SQLITE_SYNC_NORMAL, that means to use normal fsync() semantics.
** If the lower four bits equal SQLITE_SYNC_FULL, that means
** to use Mac OS X style fullsync instead of fsync().
**
** Do not confuse the SQLITE_SYNC_NORMAL and SQLITE_SYNC_FULL flags
** with the [PRAGMA synchronous]=NORMAL and [PRAGMA synchronous]=FULL
** settings.  The [synchronous pragma] determines when calls to the
** xSync VFS method occur and applies uniformly across all platforms.
** The SQLITE_SYNC_NORMAL and SQLITE_SYNC_FULL flags determine how
** energetic or rigorous or forceful the sync operations are and
** only make a difference on Mac OSX for the default SQLite code.
** (Third-party VFS implementations might also make the distinction
** between SQLITE_SYNC_NORMAL and SQLITE_SYNC_FULL, but among the
** operating systems natively supported by SQLite, only Mac OSX
** cares about the difference.)
*/
#define SQLITE_SYNC_NORMAL        0x00002
#define SQLITE_SYNC_FULL          0x00003
#define SQLITE_SYNC_DATAONLY      0x00010

/*
** CAPI3REF: OS Interface Open File Handle
**
** An [sqlite3_file] object represents an open file in the
** [sqlite3_vfs | OS interface layer].  Individual OS interface
** implementations will
** want to subclass this object by appending additional fields
** for their own use.  The pMethods entry is a pointer to an
** [sqlite3_io_methods] object that defines methods for performing
** I/O operations on the open file.
*/
typedef struct sqlite3_file sqlite3_file;
struct sqlite3_file {
  const struct sqlite3_io_methods *pMethods;  /* Methods for an open file */
};

/*
** CAPI3REF: OS Interface File Virtual Methods Object
**
** Every file opened by the [sqlite3_vfs.xOpen] method populates an
** [sqlite3_file] object (or, more commonly, a subclass of the
** [sqlite3_file] object) with a pointer to an instance of this object.
** This object defines the methods used to perform various operations
** against the open file represented by the [sqlite3_file] object.
**
** If the [sqlite3_vfs.xOpen] method sets the sqlite3_file.pMethods element
** to a non-NULL pointer, then the sqlite3_io_methods.xClose method
** may be invoked even if the [sqlite3_vfs.xOpen] reported that it failed.  The
** only way to prevent a call to xClose following a failed [sqlite3_vfs.xOpen]
** is for the [sqlite3_vfs.xOpen] to set the sqlite3_file.pMethods element
** to NULL.
**
** The flags argument to xSync may be one of [SQLITE_SYNC_NORMAL] or
** [SQLITE_SYNC_FULL].  The first choice is the normal fsync().
** The second choice is a Mac OS X style fullsync.  The [SQLITE_SYNC_DATAONLY]
** flag may be ORed in to indicate that only the data of the file
** and not its inode needs to be synced.
**
** The integer values to xLock() and xUnlock() are one of
** <ul>
** <li> [SQLITE_LOCK_NONE],
** <li> [SQLITE_LOCK_SHARED],
** <li> [SQLITE_LOCK_RESERVED],
** <li> [SQLITE_LOCK_PENDING], or
** <li> [SQLITE_LOCK_EXCLUSIVE].
** </ul>
** xLock() increases the lock. xUnlock() decreases the lock.
** The xCheckReservedLock() method checks whether any database connection,
** either in this process or in some other process, is holding a RESERVED,
** PENDING, or EXCLUSIVE lock on the file.  It returns true
** if such a lock exists and false otherwise.
**
** The xFileControl() method is a generic interface that allows custom
** VFS implementations to directly control an open file using the
** [sqlite3_file_control()] interface.  The second "op" argument is an
** integer opcode.  The third argument is a generic pointer intended to
** point to a structure that may contain arguments or space in which to
** write return values.  Potential uses for xFileControl() might be
** functions to enable blocking locks with timeouts, to change the
** locking strategy (for example to use dot-file locks), to inquire
** about the status of a lock, or to break stale locks.  The SQLite
** core reserves all opcodes less than 100 for its own use.
** A [file control opcodes | list of opcodes] less than 100 is available.
** Applications that define a custom xFileControl method should use opcodes
** greater than 100 to avoid conflicts.  VFS implementations should
** return [SQLITE_NOTFOUND] for file control opcodes that they do not
** recognize.
**
** The xSectorSize() method returns the sector size of the
** device that underlies the file.  The sector size is the
** minimum write that can be performed without disturbing
** other bytes in the file.  The xDeviceCharacteristics()
** method returns a bit vector describing behaviors of the
** underlying device:
**
** <ul>
** <li> [SQLITE_IOCAP_ATOMIC]
** <li> [SQLITE_IOCAP_ATOMIC512]
** <li> [SQLITE_IOCAP_ATOMIC1K]
** <li> [SQLITE_IOCAP_ATOMIC2K]
** <li> [SQLITE_IOCAP_ATOMIC4K]
** <li> [SQLITE_IOCAP_ATOMIC8K]
** <li> [SQLITE_IOCAP_ATOMIC16K]
** <li> [SQLITE_IOCAP_ATOMIC32K]
** <li> [SQLITE_IOCAP_ATOMIC64K]
** <li> [SQLITE_IOCAP_SAFE_APPEND]
** <li> [SQLITE_IOCAP_SEQUENTIAL]
** <li> [SQLITE_IOCAP_UNDELETABLE_WHEN_OPEN]
** <li> [SQLITE_IOCAP_POWERSAFE_OVERWRITE]
** <li> [SQLITE_IOCAP_IMMUTABLE]
** <li> [SQLITE_IOCAP_BATCH_ATOMIC]
** </ul>
**
** The SQLITE_IOCAP_ATOMIC property means that all writes of
** any size are atomic.  The SQLITE_IOCAP_ATOMICnnn values
** mean that writes of blocks that are nnn bytes in size and
** are aligned to an address which is an integer multiple of
** nnn are atomic.  The SQLITE_IOCAP_SAFE_APPEND value means
** that when data is appended to a file, the data is appended
** first then the size of the file is extended, never the other
** way around.  The SQLITE_IOCAP_SEQUENTIAL property means that
** information is written to disk in the same order as calls
** to xWrite().
**
** If xRead() returns SQLITE_IOERR_SHORT_READ it must also fill
** in the unread portions of the buffer with zeros.  A VFS that
** fails to zero-fill short reads might seem to work.  However,
** failure to zero-fill short reads will eventually lead to
** database corruption.
*/
typedef struct sqlite3_io_methods sqlite3_io_methods;
struct sqlite3_io_methods {
  int iVersion;
  int (*xClose)(sqlite3_file*);
  int (*xRead)(sqlite3_file*, void*, int iAmt, sqlite3_int64 iOfst);
  int (*xWrite)(sqlite3_file*, const void*, int iAmt, sqlite3_int64 iOfst);
  int (*xTruncate)(sqlite3_file*, sqlite3_int64 size);
  int (*xSync)(sqlite3_file*, int flags);
  int (*xFileSize)(sqlite3_file*, sqlite3_int64 *pSize);
  int (*xLock)(sqlite3_file*, int);
  int (*xUnlock)(sqlite3_file*, int);
  int (*xCheckReservedLock)(sqlite3_file*, int *pResOut);
  int (*xFileControl)(sqlite3_file*, int op, void *pArg);
  int (*xSectorSize)(sqlite3_file*);
  int (*xDeviceCharacteristics)(sqlite3_file*);
  /* Methods above are valid for version 1 */
  int (*xShmMap)(sqlite3_file*, int iPg, int pgsz, int, void volatile**);
  int (*xShmLock)(sqlite3_file*, int offset, int n, int flags);
  void (*xShmBarrier)(sqlite3_file*);
  int (*xShmUnmap)(sqlite3_file*, int deleteFlag);
  /* Methods above are valid for version 2 */
  int (*xFetch)(sqlite3_file*, sqlite3_int64 iOfst, int iAmt, void **pp);
  int (*xUnfetch)(sqlite3_file*, sqlite3_int64 iOfst, void *p);
  /* Methods above are valid for version 3 */
  /* Additional methods may be added in future releases */
};

/*
** CAPI3REF: Standard File Control Opcodes
** KEYWORDS: {file control opcodes} {file control opcode}
**
** These integer constants are opcodes for the xFileControl method
** of the [sqlite3_io_methods] object and for the [sqlite3_file_control()]
** interface.
**
** <ul>
** <li>[[SQLITE_FCNTL_LOCKSTATE]]
** The [SQLITE_FCNTL_LOCKSTATE] opcode is used for debugging.  This
** opcode causes the xFileControl method to write the current state of
** the lock (one of [SQLITE_LOCK_NONE], [SQLITE_LOCK_SHARED],
** [SQLITE_LOCK_RESERVED], [SQLITE_LOCK_PENDING], or [SQLITE_LOCK_EXCLUSIVE])
** into an integer that the pArg argument points to. This capability
** is used during testing and is only available when the SQLITE_TEST
** compile-time option is used.
**
** <li>[[SQLITE_FCNTL_SIZE_HINT]]
** The [SQLITE_FCNTL_SIZE_HINT] opcode is used by SQLite to give the VFS
** layer a hint of how large the database file will grow to be during the
** current transaction.  This hint is not guaranteed to be accurate but it
** is often close.  The underlying VFS might choose to preallocate database
** file space based on this hint in order to help writes to the database
** file run faster.
**
** <li>[[SQLITE_FCNTL_SIZE_LIMIT]]
** The [SQLITE_FCNTL_SIZE_LIMIT] opcode is used by in-memory VFS that
** implements [sqlite3_deserialize()] to set an upper bound on the size
** of the in-memory database.  The argument is a pointer to a [sqlite3_int64].
** If the integer pointed to is negative, then it is filled in with the
** current limit.  Otherwise the limit is set to the larger of the value
** of the integer pointed to and the current database size.  The integer
** pointed to is set to the new limit.
**
** <li>[[SQLITE_FCNTL_CHUNK_SIZE]]
** The [SQLITE_FCNTL_CHUNK_SIZE] opcode is used to request that the VFS
** extends and truncates the database file in chunks of a size specified
** by the user. The fourth argument to [sqlite3_file_control()] should
** point to an integer (type int) containing the new chunk-size to use
** for the nominated database. Allocating database file space in large
** chunks (say 1MB at a time), may reduce file-system fragmentation and
** improve performance on some systems.
**
** <li>[[SQLITE_FCNTL_FILE_POINTER]]
** The [SQLITE_FCNTL_FILE_POINTER] opcode is used to obtain a pointer
** to the [sqlite3_file] object associated with a particular database
** connection.  See also [SQLITE_FCNTL_JOURNAL_POINTER].
**
** <li>[[SQLITE_FCNTL_JOURNAL_POINTER]]
** The [SQLITE_FCNTL_JOURNAL_POINTER] opcode is used to obtain a pointer
** to the [sqlite3_file] object associated with the journal file (either
** the [rollback journal] or the [write-ahead log]) for a particular database
** connection.  See also [SQLITE_FCNTL_FILE_POINTER].
**
** <li>[[SQLITE_FCNTL_SYNC_OMITTED]]
** No longer in use.
**
** <li>[[SQLITE_FCNTL_SYNC]]
** The [SQLITE_FCNTL_SYNC] opcode is generated internally by SQLite and
** sent to the VFS immediately before the xSync method is invoked on a
** database file descriptor. Or, if the xSync method is not invoked
** because the user has configured SQLite with
** [PRAGMA synchronous | PRAGMA synchronous=OFF] it is invoked in place
** of the xSync method. In most cases, the pointer argument passed with
** this file-control is NULL. However, if the database file is being synced
** as part of a multi-database commit, the argument points to a nul-terminated
** string containing the transactions super-journal file name. VFSes that
** do not need this signal should silently ignore this opcode. Applications
** should not call [sqlite3_file_control()] with this opcode as doing so may
** disrupt the operation of the specialized VFSes that do require it.
**
** <li>[[SQLITE_FCNTL_COMMIT_PHASETWO]]
** The [SQLITE_FCNTL_COMMIT_PHASETWO] opcode is generated internally by SQLite
** and sent to the VFS after a transaction has been committed immediately
** but before the database is unlocked. VFSes that do not need this signal
** should silently ignore this opcode. Applications should not call
** [sqlite3_file_control()] with this opcode as doing so may disrupt the
** operation of the specialized VFSes that do require it.
**
** <li>[[SQLITE_FCNTL_WIN32_AV_RETRY]]
** ^The [SQLITE_FCNTL_WIN32_AV_RETRY] opcode is used to configure automatic
** retry counts and intervals for certain disk I/O operations for the
** windows [VFS] in order to provide robustness in the presence of
** anti-virus programs.  By default, the windows VFS will retry file read,
** file write, and file delete operations up to 10 times, with a delay
** of 25 milliseconds before the first retry and with the delay increasing
** by an additional 25 milliseconds with each subsequent retry.  This
** opcode allows these two values (10 retries and 25 milliseconds of delay)
** to be adjusted.  The values are changed for all database connections
** within the same process.  The argument is a pointer to an array of two
** integers where the first integer is the new retry count and the second
** integer is the delay.  If either integer is negative, then the setting
** is not changed but instead the prior value of that setting is written
** into the array entry, allowing the current retry settings to be
** interrogated.  The zDbName parameter is ignored.
**
** <li>[[SQLITE_FCNTL_PERSIST_WAL]]
** ^The [SQLITE_FCNTL_PERSIST_WAL] opcode is used to set or query the
** persistent [WAL | Write Ahead Log] setting.  By default, the auxiliary
** write ahead log ([WAL file]) and shared memory
** files used for transaction control
** are automatically deleted when the latest connection to the database
** closes.  Setting persistent WAL mode causes those files to persist after
** close.  Persisting the files is useful when other processes that do not
** have write permission on the directory containing the database file want
** to read the database file, as the WAL and shared memory files must exist
** in order for the database to be readable.  The fourth parameter to
** [sqlite3_file_control()] for this opcode should be a pointer to an integer.
** That integer is 0 to disable persistent WAL mode or 1 to enable persistent
** WAL mode.  If the integer is -1, then it is overwritten with the current
** WAL persistence setting.
**
** <li>[[SQLITE_FCNTL_POWERSAFE_OVERWRITE]]
** ^The [SQLITE_FCNTL_POWERSAFE_OVERWRITE] opcode is used to set or query the
** persistent "powersafe-overwrite" or "PSOW" setting.  The PSOW setting
** determines the [SQLITE_IOCAP_POWERSAFE_OVERWRITE] bit of the
** xDeviceCharacteristics methods. The fourth parameter to
** [sqlite3_file_control()] for this opcode should be a pointer to an integer.
** That integer is 0 to disable zero-damage mode or 1 to enable zero-damage
** mode.  If the integer is -1, then it is overwritten with the current
** zero-damage mode setting.
**
** <li>[[SQLITE_FCNTL_OVERWRITE]]
** ^The [SQLITE_FCNTL_OVERWRITE] opcode is invoked by SQLite after opening
** a write transaction to indicate that, unless it is rolled back for some
** reason, the entire database file will be overwritten by the current
** transaction. This is used by VACUUM operations.
**
** <li>[[SQLITE_FCNTL_VFSNAME]]
** ^The [SQLITE_FCNTL_VFSNAME] opcode can be used to obtain the names of
** all [VFSes] in the VFS stack.  The names are of all VFS shims and the
** final bottom-level VFS are written into memory obtained from
** [sqlite3_malloc()] and the result is stored in the char* variable
** that the fourth parameter of [sqlite3_file_control()] points to.
** The caller is responsible for freeing the memory when done.  As with
** all file-control actions, there is no guarantee that this will actually
** do anything.  Callers should initialize the char* variable to a NULL
** pointer in case this file-control is not implemented.  This file-control
** is intended for diagnostic use only.
**
** <li>[[SQLITE_FCNTL_VFS_POINTER]]
** ^The [SQLITE_FCNTL_VFS_POINTER] opcode finds a pointer to the top-level
** [VFSes] currently in use.  ^(The argument X in
** sqlite3_file_control(db,SQLITE_FCNTL_VFS_POINTER,X) must be
** of type "[sqlite3_vfs] **".  This opcodes will set *X
** to a pointer to the top-level VFS.)^
** ^When there are multiple VFS shims in the stack, this opcode finds the
** upper-most shim only.
**
** <li>[[SQLITE_FCNTL_PRAGMA]]
** ^Whenever a [PRAGMA] statement is parsed, an [SQLITE_FCNTL_PRAGMA]
** file control is sent to the open [sqlite3_file] object corresponding
** to the database file to which the pragma statement refers. ^The argument
** to the [SQLITE_FCNTL_PRAGMA] file control is an array of
** pointers to strings (char**) in which the second element of the array
** is the name of the pragma and the third element is the argument to the
** pragma or NULL if the pragma has no argument.  ^The handler for an
** [SQLITE_FCNTL_PRAGMA] file control can optionally make the first element
** of the char** argument point to a string obtained from [sqlite3_mprintf()]
** or the equivalent and that string will become the result of the pragma or
** the error message if the pragma fails. ^If the
** [SQLITE_FCNTL_PRAGMA] file control returns [SQLITE_NOTFOUND], then normal
** [PRAGMA] processing continues.  ^If the [SQLITE_FCNTL_PRAGMA]
** file control returns [SQLITE_OK], then the parser assumes that the
** VFS has handled the PRAGMA itself and the parser generates a no-op
** prepared statement if result string is NULL, or that returns a copy
** of the result string if the string is non-NULL.
** ^If the [SQLITE_FCNTL_PRAGMA] file control returns
** any result code other than [SQLITE_OK] or [SQLITE_NOTFOUND], that means
** that the VFS encountered an error while handling the [PRAGMA] and the
** compilation of the PRAGMA fails with an error.  ^The [SQLITE_FCNTL_PRAGMA]
** file control occurs at the beginning of pragma statement analysis and so
** it is able to override built-in [PRAGMA] statements.
**
** <li>[[SQLITE_FCNTL_BUSYHANDLER]]
** ^The [SQLITE_FCNTL_BUSYHANDLER]
** file-control may be invoked by SQLite on the database file handle
** shortly after it is opened in order to provide a custom VFS with access
** to the connection's busy-handler callback. The argument is of type (void**)
** - an array of two (void *) values. The first (void *) actually points
** to a function of type (int (*)(void *)). In order to invoke the connection's
** busy-handler, this function should be invoked with the second (void *) in
** the array as the only argument. If it returns non-zero, then the operation
** should be retried. If it returns zero, the custom VFS should abandon the
** current operation.
**
** <li>[[SQLITE_FCNTL_TEMPFILENAME]]
** ^Applications can invoke the [SQLITE_FCNTL_TEMPFILENAME] file-control
** to have SQLite generate a
** temporary filename using the same algorithm that is followed to generate
** temporary filenames for TEMP tables and other internal uses.  The
** argument should be a char** which will be filled with the filename
** written into memory obtained from [sqlite3_malloc()].  The caller should
** invoke [sqlite3_free()] on the result to avoid a memory leak.
**
** <li>[[SQLITE_FCNTL_MMAP_SIZE]]
** The [SQLITE_FCNTL_MMAP_SIZE] file control is used to query or set the
** maximum number of bytes that will be used for memory-mapped I/O.
** The argument is a pointer to a value of type sqlite3_int64 that
** is an advisory maximum number of bytes in the file to memory map.  The
** pointer is overwritten with the old value.  The limit is not changed if
** the value originally pointed to is negative, and so the current limit
** can be queried by passing in a pointer to a negative number.  This
** file-control is used internally to implement [PRAGMA mmap_size].
**
** <li>[[SQLITE_FCNTL_TRACE]]
** The [SQLITE_FCNTL_TRACE] file control provides advisory information
** to the VFS about what the higher layers of the SQLite stack are doing.
** This file control is used by some VFS activity tracing [shims].
** The argument is a zero-terminated string.  Higher layers in the
** SQLite stack may generate instances of this file control if
** the [SQLITE_USE_FCNTL_TRACE] compile-time option is enabled.
**
** <li>[[SQLITE_FCNTL_HAS_MOVED]]
** The [SQLITE_FCNTL_HAS_MOVED] file control interprets its argument as a
** pointer to an integer and it writes a boolean into that integer depending
** on whether or not the file has been renamed, moved, or deleted since it
** was first opened.
**
** <li>[[SQLITE_FCNTL_WIN32_GET_HANDLE]]
** The [SQLITE_FCNTL_WIN32_GET_HANDLE] opcode can be used to obtain the
** underlying native file handle associated with a file handle.  This file
** control interprets its argument as a pointer to a native file handle and
** writes the resulting value there.
**
** <li>[[SQLITE_FCNTL_WIN32_SET_HANDLE]]
** The [SQLITE_FCNTL_WIN32_SET_HANDLE] opcode is used for debugging.  This
** opcode causes the xFileControl method to swap the file handle with the one
** pointed to by the pArg argument.  This capability is used during testing
** and only needs to be supported when SQLITE_TEST is defined.
**
** <li>[[SQLITE_FCNTL_WAL_BLOCK]]
** The [SQLITE_FCNTL_WAL_BLOCK] is a signal to the VFS layer that it might
** be advantageous to block on the next WAL lock if the lock is not immediately
** available.  The WAL subsystem issues this signal during rare
** circumstances in order to fix a problem with priority inversion.
** Applications should <em>not</em> use this file-control.
**
** <li>[[SQLITE_FCNTL_ZIPVFS]]
** The [SQLITE_FCNTL_ZIPVFS] opcode is implemented by zipvfs only. All other
** VFS should return SQLITE_NOTFOUND for this opcode.
**
** <li>[[SQLITE_FCNTL_RBU]]
** The [SQLITE_FCNTL_RBU] opcode is implemented by the special VFS used by
** the RBU extension only.  All other VFS should return SQLITE_NOTFOUND for
** this opcode.
**
** <li>[[SQLITE_FCNTL_BEGIN_ATOMIC_WRITE]]
** If the [SQLITE_FCNTL_BEGIN_ATOMIC_WRITE] opcode returns SQLITE_OK, then
** the file descriptor is placed in "batch write mode", which
** means all subsequent write operations will be deferred and done
** atomically at the next [SQLITE_FCNTL_COMMIT_ATOMIC_WRITE].  Systems
** that do not support batch atomic writes will return SQLITE_NOTFOUND.
** ^Following a successful SQLITE_FCNTL_BEGIN_ATOMIC_WRITE and prior to
** the closing [SQLITE_FCNTL_COMMIT_ATOMIC_WRITE] or
** [SQLITE_FCNTL_ROLLBACK_ATOMIC_WRITE], SQLite will make
** no VFS interface calls on the same [sqlite3_file] file descriptor
** except for calls to the xWrite method and the xFileControl method
** with [SQLITE_FCNTL_SIZE_HINT].
**
** <li>[[SQLITE_FCNTL_COMMIT_ATOMIC_WRITE]]
** The [SQLITE_FCNTL_COMMIT_ATOMIC_WRITE] opcode causes all write
** operations since the previous successful call to
** [SQLITE_FCNTL_BEGIN_ATOMIC_WRITE] to be performed atomically.
** This file control returns [SQLITE_OK] if and only if the writes were
** all performed successfully and have been committed to persistent storage.
** ^Regardless of whether or not it is successful, this file control takes
** the file descriptor out of batch write mode so that all subsequent
** write operations are independent.
** ^SQLite will never invoke SQLITE_FCNTL_COMMIT_ATOMIC_WRITE without
** a prior successful call to [SQLITE_FCNTL_BEGIN_ATOMIC_WRITE].
**
** <li>[[SQLITE_FCNTL_ROLLBACK_ATOMIC_WRITE]]
** The [SQLITE_FCNTL_ROLLBACK_ATOMIC_WRITE] opcode causes all write
** operations since the previous successful call to
** [SQLITE_FCNTL_BEGIN_ATOMIC_WRITE] to be rolled back.
** ^This file control takes the file descriptor out of batch write mode
** so that all subsequent write operations are independent.
** ^SQLite will never invoke SQLITE_FCNTL_ROLLBACK_ATOMIC_WRITE without
** a prior successful call to [SQLITE_FCNTL_BEGIN_ATOMIC_WRITE].
**
** <li>[[SQLITE_FCNTL_LOCK_TIMEOUT]]
** The [SQLITE_FCNTL_LOCK_TIMEOUT] opcode is used to configure a VFS
** to block for up to M milliseconds before failing when attempting to
** obtain a file lock using the xLock or xShmLock methods of the VFS.
** The parameter is a pointer to a 32-bit signed integer that contains
** the value that M is to be set to. Before returning, the 32-bit signed
** integer is overwritten with the previous value of M.
**
** <li>[[SQLITE_FCNTL_DATA_VERSION]]
** The [SQLITE_FCNTL_DATA_VERSION] opcode is used to detect changes to
** a database file.  The argument is a pointer to a 32-bit unsigned integer.
** The "data version" for the pager is written into the pointer.  The
** "data version" changes whenever any change occurs to the corresponding
** database file, either through SQL statements on the same database
** connection or through transactions committed by separate database
** connections possibly in other processes. The [sqlite3_total_changes()]
** interface can be used to find if any database on the connection has changed,
** but that interface responds to changes on TEMP as well as MAIN and does
** not provide a mechanism to detect changes to MAIN only.  Also, the
** [sqlite3_total_changes()] interface responds to internal changes only and
** omits changes made by other database connections.  The
** [PRAGMA data_version] command provides a mechanism to detect changes to
** a single attached database that occur due to other database connections,
** but omits changes implemented by the database connection on which it is
** called.  This file control is the only mechanism to detect changes that
** happen either internally or externally and that are associated with
** a particular attached database.
**
** <li>[[SQLITE_FCNTL_CKPT_START]]
** The [SQLITE_FCNTL_CKPT_START] opcode is invoked from within a checkpoint
** in wal mode before the client starts to copy pages from the wal
** file to the database file.
**
** <li>[[SQLITE_FCNTL_CKPT_DONE]]
** The [SQLITE_FCNTL_CKPT_DONE] opcode is invoked from within a checkpoint
** in wal mode after the client has finished copying pages from the wal
** file to the database file, but before the *-shm file is updated to
** record the fact that the pages have been checkpointed.
** </ul>
**
** <li>[[SQLITE_FCNTL_EXTERNAL_READER]]
** The EXPERIMENTAL [SQLITE_FCNTL_EXTERNAL_READER] opcode is used to detect
** whether or not there is a database client in another process with a wal-mode
** transaction open on the database or not. It is only available on unix.The
** (void*) argument passed with this file-control should be a pointer to a
** value of type (int). The integer value is set to 1 if the database is a wal
** mode database and there exists at least one client in another process that
** currently has an SQL transaction open on the database. It is set to 0 if
** the database is not a wal-mode db, or if there is no such connection in any
** other process. This opcode cannot be used to detect transactions opened
** by clients within the current process, only within other processes.
** </ul>
**
** <li>[[SQLITE_FCNTL_CKSM_FILE]]
** Used by the cksmvfs VFS module only.
** </ul>
*/
#define SQLITE_FCNTL_LOCKSTATE               1
#define SQLITE_FCNTL_GET_LOCKPROXYFILE       2
#define SQLITE_FCNTL_SET_LOCKPROXYFILE       3
#define SQLITE_FCNTL_LAST_ERRNO              4
#define SQLITE_FCNTL_SIZE_HINT               5
#define SQLITE_FCNTL_CHUNK_SIZE              6
#define SQLITE_FCNTL_FILE_POINTER            7
#define SQLITE_FCNTL_SYNC_OMITTED            8
#define SQLITE_FCNTL_WIN32_AV_RETRY          9
#define SQLITE_FCNTL_PERSIST_WAL            10
#define SQLITE_FCNTL_OVERWRITE              11
#define SQLITE_FCNTL_VFSNAME                12
#define SQLITE_FCNTL_POWERSAFE_OVERWRITE    13
#define SQLITE_FCNTL_PRAGMA                 14
#define SQLITE_FCNTL_BUSYHANDLER            15
#define SQLITE_FCNTL_TEMPFILENAME           16
#define SQLITE_FCNTL_MMAP_SIZE              18
#define SQLITE_FCNTL_TRACE                  19
#define SQLITE_FCNTL_HAS_MOVED              20
#define SQLITE_FCNTL_SYNC                   21
#define SQLITE_FCNTL_COMMIT_PHASETWO        22
#define SQLITE_FCNTL_WIN32_SET_HANDLE       23
#define SQLITE_FCNTL_WAL_BLOCK              24
#define SQLITE_FCNTL_ZIPVFS                 25
#define SQLITE_FCNTL_RBU                    26
#define SQLITE_FCNTL_VFS_POINTER            27
#define SQLITE_FCNTL_JOURNAL_POINTER        28
#define SQLITE_FCNTL_WIN32_GET_HANDLE       29
#define SQLITE_FCNTL_PDB                    30
#define SQLITE_FCNTL_BEGIN_ATOMIC_WRITE     31
#define SQLITE_FCNTL_COMMIT_ATOMIC_WRITE    32
#define SQLITE_FCNTL_ROLLBACK_ATOMIC_WRITE  33
#define SQLITE_FCNTL_LOCK_TIMEOUT           34
#define SQLITE_FCNTL_DATA_VERSION           35
#define SQLITE_FCNTL_SIZE_LIMIT             36
#define SQLITE_FCNTL_CKPT_DONE              37
#define SQLITE_FCNTL_RESERVE_BYTES          38
#define SQLITE_FCNTL_CKPT_START             39
#define SQLITE_FCNTL_EXTERNAL_READER        40
#define SQLITE_FCNTL_CKSM_FILE              41

/* deprecated names */
#define SQLITE_GET_LOCKPROXYFILE      SQLITE_FCNTL_GET_LOCKPROXYFILE
#define SQLITE_SET_LOCKPROXYFILE      SQLITE_FCNTL_SET_LOCKPROXYFILE
#define SQLITE_LAST_ERRNO             SQLITE_FCNTL_LAST_ERRNO


/*
** CAPI3REF: Mutex Handle
**
** The mutex module within SQLite defines [sqlite3_mutex] to be an
** abstract type for a mutex object.  The SQLite core never looks
** at the internal representation of an [sqlite3_mutex].  It only
** deals with pointers to the [sqlite3_mutex] object.
**
** Mutexes are created using [sqlite3_mutex_alloc()].
*/
typedef struct sqlite3_mutex sqlite3_mutex;

/*
** CAPI3REF: Loadable Extension Thunk
**
** A pointer to the opaque sqlite3_api_routines structure is passed as
** the third parameter to entry points of [loadable extensions].  This
** structure must be typedefed in order to work around compiler warnings
** on some platforms.
*/
typedef struct sqlite3_api_routines sqlite3_api_routines;

/*
** CAPI3REF: OS Interface Object
**
** An instance of the sqlite3_vfs object defines the interface between
** the SQLite core and the underlying operating system.  The "vfs"
** in the name of the object stands for "virtual file system".  See
** the [VFS | VFS documentation] for further information.
**
** The VFS interface is sometimes extended by adding new methods onto
** the end.  Each time such an extension occurs, the iVersion field
** is incremented.  The iVersion value started out as 1 in
** SQLite [version 3.5.0] on [dateof:3.5.0], then increased to 2
** with SQLite [version 3.7.0] on [dateof:3.7.0], and then increased
** to 3 with SQLite [version 3.7.6] on [dateof:3.7.6].  Additional fields
** may be appended to the sqlite3_vfs object and the iVersion value
** may increase again in future versions of SQLite.
** Note that due to an oversight, the structure
** of the sqlite3_vfs object changed in the transition from
** SQLite [version 3.5.9] to [version 3.6.0] on [dateof:3.6.0]
** and yet the iVersion field was not increased.
**
** The szOsFile field is the size of the subclassed [sqlite3_file]
** structure used by this VFS.  mxPathname is the maximum length of
** a pathname in this VFS.
**
** Registered sqlite3_vfs objects are kept on a linked list formed by
** the pNext pointer.  The [sqlite3_vfs_register()]
** and [sqlite3_vfs_unregister()] interfaces manage this list
** in a thread-safe way.  The [sqlite3_vfs_find()] interface
** searches the list.  Neither the application code nor the VFS
** implementation should use the pNext pointer.
**
** The pNext field is the only field in the sqlite3_vfs
** structure that SQLite will ever modify.  SQLite will only access
** or modify this field while holding a particular static mutex.
** The application should never modify anything within the sqlite3_vfs
** object once the object has been registered.
**
** The zName field holds the name of the VFS module.  The name must
** be unique across all VFS modules.
**
** [[sqlite3_vfs.xOpen]]
** ^SQLite guarantees that the zFilename parameter to xOpen
** is either a NULL pointer or string obtained
** from xFullPathname() with an optional suffix added.
** ^If a suffix is added to the zFilename parameter, it will
** consist of a single "-" character followed by no more than
** 11 alphanumeric and/or "-" characters.
** ^SQLite further guarantees that
** the string will be valid and unchanged until xClose() is
** called. Because of the previous sentence,
** the [sqlite3_file] can safely store a pointer to the
** filename if it needs to remember the filename for some reason.
** If the zFilename parameter to xOpen is a NULL pointer then xOpen
** must invent its own temporary name for the file.  ^Whenever the
** xFilename parameter is NULL it will also be the case that the
** flags parameter will include [SQLITE_OPEN_DELETEONCLOSE].
**
** The flags argument to xOpen() includes all bits set in
** the flags argument to [sqlite3_open_v2()].  Or if [sqlite3_open()]
** or [sqlite3_open16()] is used, then flags includes at least
** [SQLITE_OPEN_READWRITE] | [SQLITE_OPEN_CREATE].
** If xOpen() opens a file read-only then it sets *pOutFlags to
** include [SQLITE_OPEN_READONLY].  Other bits in *pOutFlags may be set.
**
** ^(SQLite will also add one of the following flags to the xOpen()
** call, depending on the object being opened:
**
** <ul>
** <li>  [SQLITE_OPEN_MAIN_DB]
** <li>  [SQLITE_OPEN_MAIN_JOURNAL]
** <li>  [SQLITE_OPEN_TEMP_DB]
** <li>  [SQLITE_OPEN_TEMP_JOURNAL]
** <li>  [SQLITE_OPEN_TRANSIENT_DB]
** <li>  [SQLITE_OPEN_SUBJOURNAL]
** <li>  [SQLITE_OPEN_SUPER_JOURNAL]
** <li>  [SQLITE_OPEN_WAL]
** </ul>)^
**
** The file I/O implementation can use the object type flags to
** change the way it deals with files.  For example, an application
** that does not care about crash recovery or rollback might make
** the open of a journal file a no-op.  Writes to this journal would
** also be no-ops, and any attempt to read the journal would return
** SQLITE_IOERR.  Or the implementation might recognize that a database
** file will be doing page-aligned sector reads and writes in a random
** order and set up its I/O subsystem accordingly.
**
** SQLite might also add one of the following flags to the xOpen method:
**
** <ul>
** <li> [SQLITE_OPEN_DELETEONCLOSE]
** <li> [SQLITE_OPEN_EXCLUSIVE]
** </ul>
**
** The [SQLITE_OPEN_DELETEONCLOSE] flag means the file should be
** deleted when it is closed.  ^The [SQLITE_OPEN_DELETEONCLOSE]
** will be set for TEMP databases and their journals, transient
** databases, and subjournals.
**
** ^The [SQLITE_OPEN_EXCLUSIVE] flag is always used in conjunction
** with the [SQLITE_OPEN_CREATE] flag, which are both directly
** analogous to the O_EXCL and O_CREAT flags of the POSIX open()
** API.  The SQLITE_OPEN_EXCLUSIVE flag, when paired with the
** SQLITE_OPEN_CREATE, is used to indicate that file should always
** be created, and that it is an error if it already exists.
** It is <i>not</i> used to indicate the file should be opened
** for exclusive access.
**
** ^At least szOsFile bytes of memory are allocated by SQLite
** to hold the [sqlite3_file] structure passed as the third
** argument to xOpen.  The xOpen method does not have to
** allocate the structure; it should just fill it in.  Note that
** the xOpen method must set the sqlite3_file.pMethods to either
** a valid [sqlite3_io_methods] object or to NULL.  xOpen must do
** this even if the open fails.  SQLite expects that the sqlite3_file.pMethods
** element will be valid after xOpen returns regardless of the success
** or failure of the xOpen call.
**
** [[sqlite3_vfs.xAccess]]
** ^The flags argument to xAccess() may be [SQLITE_ACCESS_EXISTS]
** to test for the existence of a file, or [SQLITE_ACCESS_READWRITE] to
** test whether a file is readable and writable, or [SQLITE_ACCESS_READ]
** to test whether a file is at least readable.  The SQLITE_ACCESS_READ
** flag is never actually used and is not implemented in the built-in
** VFSes of SQLite.  The file is named by the second argument and can be a
** directory. The xAccess method returns [SQLITE_OK] on success or some
** non-zero error code if there is an I/O error or if the name of
** the file given in the second argument is illegal.  If SQLITE_OK
** is returned, then non-zero or zero is written into *pResOut to indicate
** whether or not the file is accessible.
**
** ^SQLite will always allocate at least mxPathname+1 bytes for the
** output buffer xFullPathname.  The exact size of the output buffer
** is also passed as a parameter to both  methods. If the output buffer
** is not large enough, [SQLITE_CANTOPEN] should be returned. Since this is
** handled as a fatal error by SQLite, vfs implementations should endeavor
** to prevent this by setting mxPathname to a sufficiently large value.
**
** The xRandomness(), xSleep(), xCurrentTime(), and xCurrentTimeInt64()
** interfaces are not strictly a part of the filesystem, but they are
** included in the VFS structure for completeness.
** The xRandomness() function attempts to return nBytes bytes
** of good-quality randomness into zOut.  The return value is
** the actual number of bytes of randomness obtained.
** The xSleep() method causes the calling thread to sleep for at
** least the number of microseconds given.  ^The xCurrentTime()
** method returns a Julian Day Number for the current date and time as
** a floating point value.
** ^The xCurrentTimeInt64() method returns, as an integer, the Julian
** Day Number multiplied by 86400000 (the number of milliseconds in
** a 24-hour day).
** ^SQLite will use the xCurrentTimeInt64() method to get the current
** date and time if that method is available (if iVersion is 2 or
** greater and the function pointer is not NULL) and will fall back
** to xCurrentTime() if xCurrentTimeInt64() is unavailable.
**
** ^The xSetSystemCall(), xGetSystemCall(), and xNestSystemCall() interfaces
** are not used by the SQLite core.  These optional interfaces are provided
** by some VFSes to facilitate testing of the VFS code. By overriding
** system calls with functions under its control, a test program can
** simulate faults and error conditions that would otherwise be difficult
** or impossible to induce.  The set of system calls that can be overridden
** varies from one VFS to another, and from one version of the same VFS to the
** next.  Applications that use these interfaces must be prepared for any
** or all of these interfaces to be NULL or for their behavior to change
** from one release to the next.  Applications must not attempt to access
** any of these methods if the iVersion of the VFS is less than 3.
*/
typedef struct sqlite3_vfs sqlite3_vfs;
typedef void (*sqlite3_syscall_ptr)(void);
struct sqlite3_vfs {
  int iVersion;            /* Structure version number (currently 3) */
  int szOsFile;            /* Size of subclassed sqlite3_file */
  int mxPathname;          /* Maximum file pathname length */
  sqlite3_vfs *pNext;      /* Next registered VFS */
  const char *zName;       /* Name of this virtual file system */
  void *pAppData;          /* Pointer to application-specific data */
  int (*xOpen)(sqlite3_vfs*, const char *zName, sqlite3_file*,
               int flags, int *pOutFlags);
  int (*xDelete)(sqlite3_vfs*, const char *zName, int syncDir);
  int (*xAccess)(sqlite3_vfs*, const char *zName, int flags, int *pResOut);
  int (*xFullPathname)(sqlite3_vfs*, const char *zName, int nOut, char *zOut);
  void *(*xDlOpen)(sqlite3_vfs*, const char *zFilename);
  void (*xDlError)(sqlite3_vfs*, int nByte, char *zErrMsg);
  void (*(*xDlSym)(sqlite3_vfs*,void*, const char *zSymbol))(void);
  void (*xDlClose)(sqlite3_vfs*, void*);
  int (*xRandomness)(sqlite3_vfs*, int nByte, char *zOut);
  int (*xSleep)(sqlite3_vfs*, int microseconds);
  int (*xCurrentTime)(sqlite3_vfs*, double*);
  int (*xGetLastError)(sqlite3_vfs*, int, char *);
  /*
  ** The methods above are in version 1 of the sqlite_vfs object
  ** definition.  Those that follow are added in version 2 or later
  */
  int (*xCurrentTimeInt64)(sqlite3_vfs*, sqlite3_int64*);
  /*
  ** The methods above are in versions 1 and 2 of the sqlite_vfs object.
  ** Those below are for version 3 and greater.
  */
  int (*xSetSystemCall)(sqlite3_vfs*, const char *zName, sqlite3_syscall_ptr);
  sqlite3_syscall_ptr (*xGetSystemCall)(sqlite3_vfs*, const char *zName);
  const char *(*xNextSystemCall)(sqlite3_vfs*, const char *zName);
  /*
  ** The methods above are in versions 1 through 3 of the sqlite_vfs object.
  ** New fields may be appended in future versions.  The iVersion
  ** value will increment whenever this happens.
  */
};

/*
** CAPI3REF: Flags for the xAccess VFS method
**
** These integer constants can be used as the third parameter to
** the xAccess method of an [sqlite3_vfs] object.  They determine
** what kind of permissions the xAccess method is looking for.
** With SQLITE_ACCESS_EXISTS, the xAccess method
** simply checks whether the file exists.
** With SQLITE_ACCESS_READWRITE, the xAccess method
** checks whether the named directory is both readable and writable
** (in other words, if files can be added, removed, and renamed within
** the directory).
** The SQLITE_ACCESS_READWRITE constant is currently used only by the
** [temp_store_directory pragma], though this could change in a future
** release of SQLite.
** With SQLITE_ACCESS_READ, the xAccess method
** checks whether the file is readable.  The SQLITE_ACCESS_READ constant is
** currently unused, though it might be used in a future release of
** SQLite.
*/
#define SQLITE_ACCESS_EXISTS    0
#define SQLITE_ACCESS_READWRITE 1   /* Used by PRAGMA temp_store_directory */
#define SQLITE_ACCESS_READ      2   /* Unused */

/*
** CAPI3REF: Flags for the xShmLock VFS method
**
** These integer constants define the various locking operations
** allowed by the xShmLock method of [sqlite3_io_methods].  The
** following are the only legal combinations of flags to the
** xShmLock method:
**
** <ul>
** <li>  SQLITE_SHM_LOCK | SQLITE_SHM_SHARED
** <li>  SQLITE_SHM_LOCK | SQLITE_SHM_EXCLUSIVE
** <li>  SQLITE_SHM_UNLOCK | SQLITE_SHM_SHARED
** <li>  SQLITE_SHM_UNLOCK | SQLITE_SHM_EXCLUSIVE
** </ul>
**
** When unlocking, the same SHARED or EXCLUSIVE flag must be supplied as
** was given on the corresponding lock.
**
** The xShmLock method can transition between unlocked and SHARED or
** between unlocked and EXCLUSIVE.  It cannot transition between SHARED
** and EXCLUSIVE.
*/
#define SQLITE_SHM_UNLOCK       1
#define SQLITE_SHM_LOCK         2
#define SQLITE_SHM_SHARED       4
#define SQLITE_SHM_EXCLUSIVE    8

/*
** CAPI3REF: Maximum xShmLock index
**
** The xShmLock method on [sqlite3_io_methods] may use values
** between 0 and this upper bound as its "offset" argument.
** The SQLite core will never attempt to acquire or release a
** lock outside of this range
*/
#define SQLITE_SHM_NLOCK        8


/*
** CAPI3REF: Initialize The SQLite Library
**
** ^The sqlite3_initialize() routine initializes the
** SQLite library.  ^The sqlite3_shutdown() routine
** deallocates any resources that were allocated by sqlite3_initialize().
** These routines are designed to aid in process initialization and
** shutdown on embedded systems.  Workstation applications using
** SQLite normally do not need to invoke either of these routines.
**
** A call to sqlite3_initialize() is an "effective" call if it is
** the first time sqlite3_initialize() is invoked during the lifetime of
** the process, or if it is the first time sqlite3_initialize() is invoked
** following a call to sqlite3_shutdown().  ^(Only an effective call
** of sqlite3_initialize() does any initialization.  All other calls
** are harmless no-ops.)^
**
** A call to sqlite3_shutdown() is an "effective" call if it is the first
** call to sqlite3_shutdown() since the last sqlite3_initialize().  ^(Only
** an effective call to sqlite3_shutdown() does any deinitialization.
** All other valid calls to sqlite3_shutdown() are harmless no-ops.)^
**
** The sqlite3_initialize() interface is threadsafe, but sqlite3_shutdown()
** is not.  The sqlite3_shutdown() interface must only be called from a
** single thread.  All open [database connections] must be closed and all
** other SQLite resources must be deallocated prior to invoking
** sqlite3_shutdown().
**
** Among other things, ^sqlite3_initialize() will invoke
** sqlite3_os_init().  Similarly, ^sqlite3_shutdown()
** will invoke sqlite3_os_end().
**
** ^The sqlite3_initialize() routine returns [SQLITE_OK] on success.
** ^If for some reason, sqlite3_initialize() is unable to initialize
** the library (perhaps it is unable to allocate a needed resource such
** as a mutex) it returns an [error code] other than [SQLITE_OK].
**
** ^The sqlite3_initialize() routine is called internally by many other
** SQLite interfaces so that an application usually does not need to
** invoke sqlite3_initialize() directly.  For example, [sqlite3_open()]
** calls sqlite3_initialize() so the SQLite library will be automatically
** initialized when [sqlite3_open()] is called if it has not be initialized
** already.  ^However, if SQLite is compiled with the [SQLITE_OMIT_AUTOINIT]
** compile-time option, then the automatic calls to sqlite3_initialize()
** are omitted and the application must call sqlite3_initialize() directly
** prior to using any other SQLite interface.  For maximum portability,
** it is recommended that applications always invoke sqlite3_initialize()
** directly prior to using any other SQLite interface.  Future releases
** of SQLite may require this.  In other words, the behavior exhibited
** when SQLite is compiled with [SQLITE_OMIT_AUTOINIT] might become the
** default behavior in some future release of SQLite.
**
** The sqlite3_os_init() routine does operating-system specific
** initialization of the SQLite library.  The sqlite3_os_end()
** routine undoes the effect of sqlite3_os_init().  Typical tasks
** performed by these routines include allocation or deallocation
** of static resources, initialization of global variables,
** setting up a default [sqlite3_vfs] module, or setting up
** a default configuration using [sqlite3_config()].
**
** The application should never invoke either sqlite3_os_init()
** or sqlite3_os_end() directly.  The application should only invoke
** sqlite3_initialize() and sqlite3_shutdown().  The sqlite3_os_init()
** interface is called automatically by sqlite3_initialize() and
** sqlite3_os_end() is called by sqlite3_shutdown().  Appropriate
** implementations for sqlite3_os_init() and sqlite3_os_end()
** are built into SQLite when it is compiled for Unix, Windows, or OS/2.
** When [custom builds | built for other platforms]
** (using the [SQLITE_OS_OTHER=1] compile-time
** option) the application must supply a suitable implementation for
** sqlite3_os_init() and sqlite3_os_end().  An application-supplied
** implementation of sqlite3_os_init() or sqlite3_os_end()
** must return [SQLITE_OK] on success and some other [error code] upon
** failure.
*/
SQLITE_API int sqlite3_initialize(void);
SQLITE_API int sqlite3_shutdown(void);
SQLITE_API int sqlite3_os_init(void);
SQLITE_API int sqlite3_os_end(void);

/*
** CAPI3REF: Configuring The SQLite Library
**
** The sqlite3_config() interface is used to make global configuration
** changes to SQLite in order to tune SQLite to the specific needs of
** the application.  The default configuration is recommended for most
** applications and so this routine is usually not necessary.  It is
** provided to support rare applications with unusual needs.
**
** <b>The sqlite3_config() interface is not threadsafe. The application
** must ensure that no other SQLite interfaces are invoked by other
** threads while sqlite3_config() is running.</b>
**
** The sqlite3_config() interface
** may only be invoked prior to library initialization using
** [sqlite3_initialize()] or after shutdown by [sqlite3_shutdown()].
** ^If sqlite3_config() is called after [sqlite3_initialize()] and before
** [sqlite3_shutdown()] then it will return SQLITE_MISUSE.
** Note, however, that ^sqlite3_config() can be called as part of the
** implementation of an application-defined [sqlite3_os_init()].
**
** The first argument to sqlite3_config() is an integer
** [configuration option] that determines
** what property of SQLite is to be configured.  Subsequent arguments
** vary depending on the [configuration option]
** in the first argument.
**
** ^When a configuration option is set, sqlite3_config() returns [SQLITE_OK].
** ^If the option is unknown or SQLite is unable to set the option
** then this routine returns a non-zero [error code].
*/
SQLITE_API int sqlite3_config(int, ...);

/*
** CAPI3REF: Configure database connections
** METHOD: sqlite3
**
** The sqlite3_db_config() interface is used to make configuration
** changes to a [database connection].  The interface is similar to
** [sqlite3_config()] except that the changes apply to a single
** [database connection] (specified in the first argument).
**
** The second argument to sqlite3_db_config(D,V,...)  is the
** [SQLITE_DBCONFIG_LOOKASIDE | configuration verb] - an integer code
** that indicates what aspect of the [database connection] is being configured.
** Subsequent arguments vary depending on the configuration verb.
**
** ^Calls to sqlite3_db_config() return SQLITE_OK if and only if
** the call is considered successful.
*/
SQLITE_API int sqlite3_db_config(sqlite3*, int op, ...);

/*
** CAPI3REF: Memory Allocation Routines
**
** An instance of this object defines the interface between SQLite
** and low-level memory allocation routines.
**
** This object is used in only one place in the SQLite interface.
** A pointer to an instance of this object is the argument to
** [sqlite3_config()] when the configuration option is
** [SQLITE_CONFIG_MALLOC] or [SQLITE_CONFIG_GETMALLOC].
** By creating an instance of this object
** and passing it to [sqlite3_config]([SQLITE_CONFIG_MALLOC])
** during configuration, an application can specify an alternative
** memory allocation subsystem for SQLite to use for all of its
** dynamic memory needs.
**
** Note that SQLite comes with several [built-in memory allocators]
** that are perfectly adequate for the overwhelming majority of applications
** and that this object is only useful to a tiny minority of applications
** with specialized memory allocation requirements.  This object is
** also used during testing of SQLite in order to specify an alternative
** memory allocator that simulates memory out-of-memory conditions in
** order to verify that SQLite recovers gracefully from such
** conditions.
**
** The xMalloc, xRealloc, and xFree methods must work like the
** malloc(), realloc() and free() functions from the standard C library.
** ^SQLite guarantees that the second argument to
** xRealloc is always a value returned by a prior call to xRoundup.
**
** xSize should return the allocated size of a memory allocation
** previously obtained from xMalloc or xRealloc.  The allocated size
** is always at least as big as the requested size but may be larger.
**
** The xRoundup method returns what would be the allocated size of
** a memory allocation given a particular requested size.  Most memory
** allocators round up memory allocations at least to the next multiple
** of 8.  Some allocators round up to a larger multiple or to a power of 2.
** Every memory allocation request coming in through [sqlite3_malloc()]
** or [sqlite3_realloc()] first calls xRoundup.  If xRoundup returns 0,
** that causes the corresponding memory allocation to fail.
**
** The xInit method initializes the memory allocator.  For example,
** it might allocate any required mutexes or initialize internal data
** structures.  The xShutdown method is invoked (indirectly) by
** [sqlite3_shutdown()] and should deallocate any resources acquired
** by xInit.  The pAppData pointer is used as the only parameter to
** xInit and xShutdown.
**
** SQLite holds the [SQLITE_MUTEX_STATIC_MAIN] mutex when it invokes
** the xInit method, so the xInit method need not be threadsafe.  The
** xShutdown method is only called from [sqlite3_shutdown()] so it does
** not need to be threadsafe either.  For all other methods, SQLite
** holds the [SQLITE_MUTEX_STATIC_MEM] mutex as long as the
** [SQLITE_CONFIG_MEMSTATUS] configuration option is turned on (which
** it is by default) and so the methods are automatically serialized.
** However, if [SQLITE_CONFIG_MEMSTATUS] is disabled, then the other
** methods must be threadsafe or else make their own arrangements for
** serialization.
**
** SQLite will never invoke xInit() more than once without an intervening
** call to xShutdown().
*/
typedef struct sqlite3_mem_methods sqlite3_mem_methods;
struct sqlite3_mem_methods {
  void *(*xMalloc)(int);         /* Memory allocation function */
  void (*xFree)(void*);          /* Free a prior allocation */
  void *(*xRealloc)(void*,int);  /* Resize an allocation */
  int (*xSize)(void*);           /* Return the size of an allocation */
  int (*xRoundup)(int);          /* Round up request size to allocation size */
  int (*xInit)(void*);           /* Initialize the memory allocator */
  void (*xShutdown)(void*);      /* Deinitialize the memory allocator */
  void *pAppData;                /* Argument to xInit() and xShutdown() */
};

/*
** CAPI3REF: Configuration Options
** KEYWORDS: {configuration option}
**
** These constants are the available integer configuration options that
** can be passed as the first argument to the [sqlite3_config()] interface.
**
** New configuration options may be added in future releases of SQLite.
** Existing configuration options might be discontinued.  Applications
** should check the return code from [sqlite3_config()] to make sure that
** the call worked.  The [sqlite3_config()] interface will return a
** non-zero [error code] if a discontinued or unsupported configuration option
** is invoked.
**
** <dl>
** [[SQLITE_CONFIG_SINGLETHREAD]] <dt>SQLITE_CONFIG_SINGLETHREAD</dt>
** <dd>There are no arguments to this option.  ^This option sets the
** [threading mode] to Single-thread.  In other words, it disables
** all mutexing and puts SQLite into a mode where it can only be used
** by a single thread.   ^If SQLite is compiled with
** the [SQLITE_THREADSAFE | SQLITE_THREADSAFE=0] compile-time option then
** it is not possible to change the [threading mode] from its default
** value of Single-thread and so [sqlite3_config()] will return
** [SQLITE_ERROR] if called with the SQLITE_CONFIG_SINGLETHREAD
** configuration option.</dd>
**
** [[SQLITE_CONFIG_MULTITHREAD]] <dt>SQLITE_CONFIG_MULTITHREAD</dt>
** <dd>There are no arguments to this option.  ^This option sets the
** [threading mode] to Multi-thread.  In other words, it disables
** mutexing on [database connection] and [prepared statement] objects.
** The application is responsible for serializing access to
** [database connections] and [prepared statements].  But other mutexes
** are enabled so that SQLite will be safe to use in a multi-threaded
** environment as long as no two threads attempt to use the same
** [database connection] at the same time.  ^If SQLite is compiled with
** the [SQLITE_THREADSAFE | SQLITE_THREADSAFE=0] compile-time option then
** it is not possible to set the Multi-thread [threading mode] and
** [sqlite3_config()] will return [SQLITE_ERROR] if called with the
** SQLITE_CONFIG_MULTITHREAD configuration option.</dd>
**
** [[SQLITE_CONFIG_SERIALIZED]] <dt>SQLITE_CONFIG_SERIALIZED</dt>
** <dd>There are no arguments to this option.  ^This option sets the
** [threading mode] to Serialized. In other words, this option enables
** all mutexes including the recursive
** mutexes on [database connection] and [prepared statement] objects.
** In this mode (which is the default when SQLite is compiled with
** [SQLITE_THREADSAFE=1]) the SQLite library will itself serialize access
** to [database connections] and [prepared statements] so that the
** application is free to use the same [database connection] or the
** same [prepared statement] in different threads at the same time.
** ^If SQLite is compiled with
** the [SQLITE_THREADSAFE | SQLITE_THREADSAFE=0] compile-time option then
** it is not possible to set the Serialized [threading mode] and
** [sqlite3_config()] will return [SQLITE_ERROR] if called with the
** SQLITE_CONFIG_SERIALIZED configuration option.</dd>
**
** [[SQLITE_CONFIG_MALLOC]] <dt>SQLITE_CONFIG_MALLOC</dt>
** <dd> ^(The SQLITE_CONFIG_MALLOC option takes a single argument which is
** a pointer to an instance of the [sqlite3_mem_methods] structure.
** The argument specifies
** alternative low-level memory allocation routines to be used in place of
** the memory allocation routines built into SQLite.)^ ^SQLite makes
** its own private copy of the content of the [sqlite3_mem_methods] structure
** before the [sqlite3_config()] call returns.</dd>
**
** [[SQLITE_CONFIG_GETMALLOC]] <dt>SQLITE_CONFIG_GETMALLOC</dt>
** <dd> ^(The SQLITE_CONFIG_GETMALLOC option takes a single argument which
** is a pointer to an instance of the [sqlite3_mem_methods] structure.
** The [sqlite3_mem_methods]
** structure is filled with the currently defined memory allocation routines.)^
** This option can be used to overload the default memory allocation
** routines with a wrapper that simulations memory allocation failure or
** tracks memory usage, for example. </dd>
**
** [[SQLITE_CONFIG_SMALL_MALLOC]] <dt>SQLITE_CONFIG_SMALL_MALLOC</dt>
** <dd> ^The SQLITE_CONFIG_SMALL_MALLOC option takes single argument of
** type int, interpreted as a boolean, which if true provides a hint to
** SQLite that it should avoid large memory allocations if possible.
** SQLite will run faster if it is free to make large memory allocations,
** but some application might prefer to run slower in exchange for
** guarantees about memory fragmentation that are possible if large
** allocations are avoided.  This hint is normally off.
** </dd>
**
** [[SQLITE_CONFIG_MEMSTATUS]] <dt>SQLITE_CONFIG_MEMSTATUS</dt>
** <dd> ^The SQLITE_CONFIG_MEMSTATUS option takes single argument of type int,
** interpreted as a boolean, which enables or disables the collection of
** memory allocation statistics. ^(When memory allocation statistics are
** disabled, the following SQLite interfaces become non-operational:
**   <ul>
**   <li> [sqlite3_hard_heap_limit64()]
**   <li> [sqlite3_memory_used()]
**   <li> [sqlite3_memory_highwater()]
**   <li> [sqlite3_soft_heap_limit64()]
**   <li> [sqlite3_status64()]
**   </ul>)^
** ^Memory allocation statistics are enabled by default unless SQLite is
** compiled with [SQLITE_DEFAULT_MEMSTATUS]=0 in which case memory
** allocation statistics are disabled by default.
** </dd>
**
** [[SQLITE_CONFIG_SCRATCH]] <dt>SQLITE_CONFIG_SCRATCH</dt>
** <dd> The SQLITE_CONFIG_SCRATCH option is no longer used.
** </dd>
**
** [[SQLITE_CONFIG_PAGECACHE]] <dt>SQLITE_CONFIG_PAGECACHE</dt>
** <dd> ^The SQLITE_CONFIG_PAGECACHE option specifies a memory pool
** that SQLite can use for the database page cache with the default page
** cache implementation.
** This configuration option is a no-op if an application-defined page
** cache implementation is loaded using the [SQLITE_CONFIG_PCACHE2].
** ^There are three arguments to SQLITE_CONFIG_PAGECACHE: A pointer to
** 8-byte aligned memory (pMem), the size of each page cache line (sz),
** and the number of cache lines (N).
** The sz argument should be the size of the largest database page
** (a power of two between 512 and 65536) plus some extra bytes for each
** page header.  ^The number of extra bytes needed by the page header
** can be determined using [SQLITE_CONFIG_PCACHE_HDRSZ].
** ^It is harmless, apart from the wasted memory,
** for the sz parameter to be larger than necessary.  The pMem
** argument must be either a NULL pointer or a pointer to an 8-byte
** aligned block of memory of at least sz*N bytes, otherwise
** subsequent behavior is undefined.
** ^When pMem is not NULL, SQLite will strive to use the memory provided
** to satisfy page cache needs, falling back to [sqlite3_malloc()] if
** a page cache line is larger than sz bytes or if all of the pMem buffer
** is exhausted.
** ^If pMem is NULL and N is non-zero, then each database connection
** does an initial bulk allocation for page cache memory
** from [sqlite3_malloc()] sufficient for N cache lines if N is positive or
** of -1024*N bytes if N is negative, . ^If additional
** page cache memory is needed beyond what is provided by the initial
** allocation, then SQLite goes to [sqlite3_malloc()] separately for each
** additional cache line. </dd>
**
** [[SQLITE_CONFIG_HEAP]] <dt>SQLITE_CONFIG_HEAP</dt>
** <dd> ^The SQLITE_CONFIG_HEAP option specifies a static memory buffer
** that SQLite will use for all of its dynamic memory allocation needs
** beyond those provided for by [SQLITE_CONFIG_PAGECACHE].
** ^The SQLITE_CONFIG_HEAP option is only available if SQLite is compiled
** with either [SQLITE_ENABLE_MEMSYS3] or [SQLITE_ENABLE_MEMSYS5] and returns
** [SQLITE_ERROR] if invoked otherwise.
** ^There are three arguments to SQLITE_CONFIG_HEAP:
** An 8-byte aligned pointer to the memory,
** the number of bytes in the memory buffer, and the minimum allocation size.
** ^If the first pointer (the memory pointer) is NULL, then SQLite reverts
** to using its default memory allocator (the system malloc() implementation),
** undoing any prior invocation of [SQLITE_CONFIG_MALLOC].  ^If the
** memory pointer is not NULL then the alternative memory
** allocator is engaged to handle all of SQLites memory allocation needs.
** The first pointer (the memory pointer) must be aligned to an 8-byte
** boundary or subsequent behavior of SQLite will be undefined.
** The minimum allocation size is capped at 2**12. Reasonable values
** for the minimum allocation size are 2**5 through 2**8.</dd>
**
** [[SQLITE_CONFIG_MUTEX]] <dt>SQLITE_CONFIG_MUTEX</dt>
** <dd> ^(The SQLITE_CONFIG_MUTEX option takes a single argument which is a
** pointer to an instance of the [sqlite3_mutex_methods] structure.
** The argument specifies alternative low-level mutex routines to be used
** in place the mutex routines built into SQLite.)^  ^SQLite makes a copy of
** the content of the [sqlite3_mutex_methods] structure before the call to
** [sqlite3_config()] returns. ^If SQLite is compiled with
** the [SQLITE_THREADSAFE | SQLITE_THREADSAFE=0] compile-time option then
** the entire mutexing subsystem is omitted from the build and hence calls to
** [sqlite3_config()] with the SQLITE_CONFIG_MUTEX configuration option will
** return [SQLITE_ERROR].</dd>
**
** [[SQLITE_CONFIG_GETMUTEX]] <dt>SQLITE_CONFIG_GETMUTEX</dt>
** <dd> ^(The SQLITE_CONFIG_GETMUTEX option takes a single argument which
** is a pointer to an instance of the [sqlite3_mutex_methods] structure.  The
** [sqlite3_mutex_methods]
** structure is filled with the currently defined mutex routines.)^
** This option can be used to overload the default mutex allocation
** routines with a wrapper used to track mutex usage for performance
** profiling or testing, for example.   ^If SQLite is compiled with
** the [SQLITE_THREADSAFE | SQLITE_THREADSAFE=0] compile-time option then
** the entire mutexing subsystem is omitted from the build and hence calls to
** [sqlite3_config()] with the SQLITE_CONFIG_GETMUTEX configuration option will
** return [SQLITE_ERROR].</dd>
**
** [[SQLITE_CONFIG_LOOKASIDE]] <dt>SQLITE_CONFIG_LOOKASIDE</dt>
** <dd> ^(The SQLITE_CONFIG_LOOKASIDE option takes two arguments that determine
** the default size of lookaside memory on each [database connection].
** The first argument is the
** size of each lookaside buffer slot and the second is the number of
** slots allocated to each database connection.)^  ^(SQLITE_CONFIG_LOOKASIDE
** sets the <i>default</i> lookaside size. The [SQLITE_DBCONFIG_LOOKASIDE]
** option to [sqlite3_db_config()] can be used to change the lookaside
** configuration on individual connections.)^ </dd>
**
** [[SQLITE_CONFIG_PCACHE2]] <dt>SQLITE_CONFIG_PCACHE2</dt>
** <dd> ^(The SQLITE_CONFIG_PCACHE2 option takes a single argument which is
** a pointer to an [sqlite3_pcache_methods2] object.  This object specifies
** the interface to a custom page cache implementation.)^
** ^SQLite makes a copy of the [sqlite3_pcache_methods2] object.</dd>
**
** [[SQLITE_CONFIG_GETPCACHE2]] <dt>SQLITE_CONFIG_GETPCACHE2</dt>
** <dd> ^(The SQLITE_CONFIG_GETPCACHE2 option takes a single argument which
** is a pointer to an [sqlite3_pcache_methods2] object.  SQLite copies of
** the current page cache implementation into that object.)^ </dd>
**
** [[SQLITE_CONFIG_LOG]] <dt>SQLITE_CONFIG_LOG</dt>
** <dd> The SQLITE_CONFIG_LOG option is used to configure the SQLite
** global [error log].
** (^The SQLITE_CONFIG_LOG option takes two arguments: a pointer to a
** function with a call signature of void(*)(void*,int,const char*),
** and a pointer to void. ^If the function pointer is not NULL, it is
** invoked by [sqlite3_log()] to process each logging event.  ^If the
** function pointer is NULL, the [sqlite3_log()] interface becomes a no-op.
** ^The void pointer that is the second argument to SQLITE_CONFIG_LOG is
** passed through as the first parameter to the application-defined logger
** function whenever that function is invoked.  ^The second parameter to
** the logger function is a copy of the first parameter to the corresponding
** [sqlite3_log()] call and is intended to be a [result code] or an
** [extended result code].  ^The third parameter passed to the logger is
** log message after formatting via [sqlite3_snprintf()].
** The SQLite logging interface is not reentrant; the logger function
** supplied by the application must not invoke any SQLite interface.
** In a multi-threaded application, the application-defined logger
** function must be threadsafe. </dd>
**
** [[SQLITE_CONFIG_URI]] <dt>SQLITE_CONFIG_URI
** <dd>^(The SQLITE_CONFIG_URI option takes a single argument of type int.
** If non-zero, then URI handling is globally enabled. If the parameter is zero,
** then URI handling is globally disabled.)^ ^If URI handling is globally
** enabled, all filenames passed to [sqlite3_open()], [sqlite3_open_v2()],
** [sqlite3_open16()] or
** specified as part of [ATTACH] commands are interpreted as URIs, regardless
** of whether or not the [SQLITE_OPEN_URI] flag is set when the database
** connection is opened. ^If it is globally disabled, filenames are
** only interpreted as URIs if the SQLITE_OPEN_URI flag is set when the
** database connection is opened. ^(By default, URI handling is globally
** disabled. The default value may be changed by compiling with the
** [SQLITE_USE_URI] symbol defined.)^
**
** [[SQLITE_CONFIG_COVERING_INDEX_SCAN]] <dt>SQLITE_CONFIG_COVERING_INDEX_SCAN
** <dd>^The SQLITE_CONFIG_COVERING_INDEX_SCAN option takes a single integer
** argument which is interpreted as a boolean in order to enable or disable
** the use of covering indices for full table scans in the query optimizer.
** ^The default setting is determined
** by the [SQLITE_ALLOW_COVERING_INDEX_SCAN] compile-time option, or is "on"
** if that compile-time option is omitted.
** The ability to disable the use of covering indices for full table scans
** is because some incorrectly coded legacy applications might malfunction
** when the optimization is enabled.  Providing the ability to
** disable the optimization allows the older, buggy application code to work
** without change even with newer versions of SQLite.
**
** [[SQLITE_CONFIG_PCACHE]] [[SQLITE_CONFIG_GETPCACHE]]
** <dt>SQLITE_CONFIG_PCACHE and SQLITE_CONFIG_GETPCACHE
** <dd> These options are obsolete and should not be used by new code.
** They are retained for backwards compatibility but are now no-ops.
** </dd>
**
** [[SQLITE_CONFIG_SQLLOG]]
** <dt>SQLITE_CONFIG_SQLLOG
** <dd>This option is only available if sqlite is compiled with the
** [SQLITE_ENABLE_SQLLOG] pre-processor macro defined. The first argument should
** be a pointer to a function of type void(*)(void*,sqlite3*,const char*, int).
** The second should be of type (void*). The callback is invoked by the library
** in three separate circumstances, identified by the value passed as the
** fourth parameter. If the fourth parameter is 0, then the database connection
** passed as the second argument has just been opened. The third argument
** points to a buffer containing the name of the main database file. If the
** fourth parameter is 1, then the SQL statement that the third parameter
** points to has just been executed. Or, if the fourth parameter is 2, then
** the connection being passed as the second parameter is being closed. The
** third parameter is passed NULL In this case.  An example of using this
** configuration option can be seen in the "test_sqllog.c" source file in
** the canonical SQLite source tree.</dd>
**
** [[SQLITE_CONFIG_MMAP_SIZE]]
** <dt>SQLITE_CONFIG_MMAP_SIZE
** <dd>^SQLITE_CONFIG_MMAP_SIZE takes two 64-bit integer (sqlite3_int64) values
** that are the default mmap size limit (the default setting for
** [PRAGMA mmap_size]) and the maximum allowed mmap size limit.
** ^The default setting can be overridden by each database connection using
** either the [PRAGMA mmap_size] command, or by using the
** [SQLITE_FCNTL_MMAP_SIZE] file control.  ^(The maximum allowed mmap size
** will be silently truncated if necessary so that it does not exceed the
** compile-time maximum mmap size set by the
** [SQLITE_MAX_MMAP_SIZE] compile-time option.)^
** ^If either argument to this option is negative, then that argument is
** changed to its compile-time default.
**
** [[SQLITE_CONFIG_WIN32_HEAPSIZE]]
** <dt>SQLITE_CONFIG_WIN32_HEAPSIZE
** <dd>^The SQLITE_CONFIG_WIN32_HEAPSIZE option is only available if SQLite is
** compiled for Windows with the [SQLITE_WIN32_MALLOC] pre-processor macro
** defined. ^SQLITE_CONFIG_WIN32_HEAPSIZE takes a 32-bit unsigned integer value
** that specifies the maximum size of the created heap.
**
** [[SQLITE_CONFIG_PCACHE_HDRSZ]]
** <dt>SQLITE_CONFIG_PCACHE_HDRSZ
** <dd>^The SQLITE_CONFIG_PCACHE_HDRSZ option takes a single parameter which
** is a pointer to an integer and writes into that integer the number of extra
** bytes per page required for each page in [SQLITE_CONFIG_PAGECACHE].
** The amount of extra space required can change depending on the compiler,
** target platform, and SQLite version.
**
** [[SQLITE_CONFIG_PMASZ]]
** <dt>SQLITE_CONFIG_PMASZ
** <dd>^The SQLITE_CONFIG_PMASZ option takes a single parameter which
** is an unsigned integer and sets the "Minimum PMA Size" for the multithreaded
** sorter to that integer.  The default minimum PMA Size is set by the
** [SQLITE_SORTER_PMASZ] compile-time option.  New threads are launched
** to help with sort operations when multithreaded sorting
** is enabled (using the [PRAGMA threads] command) and the amount of content
** to be sorted exceeds the page size times the minimum of the
** [PRAGMA cache_size] setting and this value.
**
** [[SQLITE_CONFIG_STMTJRNL_SPILL]]
** <dt>SQLITE_CONFIG_STMTJRNL_SPILL
** <dd>^The SQLITE_CONFIG_STMTJRNL_SPILL option takes a single parameter which
** becomes the [statement journal] spill-to-disk threshold.
** [Statement journals] are held in memory until their size (in bytes)
** exceeds this threshold, at which point they are written to disk.
** Or if the threshold is -1, statement journals are always held
** exclusively in memory.
** Since many statement journals never become large, setting the spill
** threshold to a value such as 64KiB can greatly reduce the amount of
** I/O required to support statement rollback.
** The default value for this setting is controlled by the
** [SQLITE_STMTJRNL_SPILL] compile-time option.
**
** [[SQLITE_CONFIG_SORTERREF_SIZE]]
** <dt>SQLITE_CONFIG_SORTERREF_SIZE
** <dd>The SQLITE_CONFIG_SORTERREF_SIZE option accepts a single parameter
** of type (int) - the new value of the sorter-reference size threshold.
** Usually, when SQLite uses an external sort to order records according
** to an ORDER BY clause, all fields required by the caller are present in the
** sorted records. However, if SQLite determines based on the declared type
** of a table column that its values are likely to be very large - larger
** than the configured sorter-reference size threshold - then a reference
** is stored in each sorted record and the required column values loaded
** from the database as records are returned in sorted order. The default
** value for this option is to never use this optimization. Specifying a
** negative value for this option restores the default behaviour.
** This option is only available if SQLite is compiled with the
** [SQLITE_ENABLE_SORTER_REFERENCES] compile-time option.
**
** [[SQLITE_CONFIG_MEMDB_MAXSIZE]]
** <dt>SQLITE_CONFIG_MEMDB_MAXSIZE
** <dd>The SQLITE_CONFIG_MEMDB_MAXSIZE option accepts a single parameter
** [sqlite3_int64] parameter which is the default maximum size for an in-memory
** database created using [sqlite3_deserialize()].  This default maximum
** size can be adjusted up or down for individual databases using the
** [SQLITE_FCNTL_SIZE_LIMIT] [sqlite3_file_control|file-control].  If this
** configuration setting is never used, then the default maximum is determined
** by the [SQLITE_MEMDB_DEFAULT_MAXSIZE] compile-time option.  If that
** compile-time option is not set, then the default maximum is 1073741824.
** </dl>
*/
#define SQLITE_CONFIG_SINGLETHREAD  1  /* nil */
#define SQLITE_CONFIG_MULTITHREAD   2  /* nil */
#define SQLITE_CONFIG_SERIALIZED    3  /* nil */
#define SQLITE_CONFIG_MALLOC        4  /* sqlite3_mem_methods* */
#define SQLITE_CONFIG_GETMALLOC     5  /* sqlite3_mem_methods* */
#define SQLITE_CONFIG_SCRATCH       6  /* No longer used */
#define SQLITE_CONFIG_PAGECACHE     7  /* void*, int sz, int N */
#define SQLITE_CONFIG_HEAP          8  /* void*, int nByte, int min */
#define SQLITE_CONFIG_MEMSTATUS     9  /* boolean */
#define SQLITE_CONFIG_MUTEX        10  /* sqlite3_mutex_methods* */
#define SQLITE_CONFIG_GETMUTEX     11  /* sqlite3_mutex_methods* */
/* previously SQLITE_CONFIG_CHUNKALLOC 12 which is now unused. */
#define SQLITE_CONFIG_LOOKASIDE    13  /* int int */
#define SQLITE_CONFIG_PCACHE       14  /* no-op */
#define SQLITE_CONFIG_GETPCACHE    15  /* no-op */
#define SQLITE_CONFIG_LOG          16  /* xFunc, void* */
#define SQLITE_CONFIG_URI          17  /* int */
#define SQLITE_CONFIG_PCACHE2      18  /* sqlite3_pcache_methods2* */
#define SQLITE_CONFIG_GETPCACHE2   19  /* sqlite3_pcache_methods2* */
#define SQLITE_CONFIG_COVERING_INDEX_SCAN 20  /* int */
#define SQLITE_CONFIG_SQLLOG       21  /* xSqllog, void* */
#define SQLITE_CONFIG_MMAP_SIZE    22  /* sqlite3_int64, sqlite3_int64 */
#define SQLITE_CONFIG_WIN32_HEAPSIZE      23  /* int nByte */
#define SQLITE_CONFIG_PCACHE_HDRSZ        24  /* int *psz */
#define SQLITE_CONFIG_PMASZ               25  /* unsigned int szPma */
#define SQLITE_CONFIG_STMTJRNL_SPILL      26  /* int nByte */
#define SQLITE_CONFIG_SMALL_MALLOC        27  /* boolean */
#define SQLITE_CONFIG_SORTERREF_SIZE      28  /* int nByte */
#define SQLITE_CONFIG_MEMDB_MAXSIZE       29  /* sqlite3_int64 */

/*
** CAPI3REF: Database Connection Configuration Options
**
** These constants are the available integer configuration options that
** can be passed as the second argument to the [sqlite3_db_config()] interface.
**
** New configuration options may be added in future releases of SQLite.
** Existing configuration options might be discontinued.  Applications
** should check the return code from [sqlite3_db_config()] to make sure that
** the call worked.  ^The [sqlite3_db_config()] interface will return a
** non-zero [error code] if a discontinued or unsupported configuration option
** is invoked.
**
** <dl>
** [[SQLITE_DBCONFIG_LOOKASIDE]]
** <dt>SQLITE_DBCONFIG_LOOKASIDE</dt>
** <dd> ^This option takes three additional arguments that determine the
** [lookaside memory allocator] configuration for the [database connection].
** ^The first argument (the third parameter to [sqlite3_db_config()] is a
** pointer to a memory buffer to use for lookaside memory.
** ^The first argument after the SQLITE_DBCONFIG_LOOKASIDE verb
** may be NULL in which case SQLite will allocate the
** lookaside buffer itself using [sqlite3_malloc()]. ^The second argument is the
** size of each lookaside buffer slot.  ^The third argument is the number of
** slots.  The size of the buffer in the first argument must be greater than
** or equal to the product of the second and third arguments.  The buffer
** must be aligned to an 8-byte boundary.  ^If the second argument to
** SQLITE_DBCONFIG_LOOKASIDE is not a multiple of 8, it is internally
** rounded down to the next smaller multiple of 8.  ^(The lookaside memory
** configuration for a database connection can only be changed when that
** connection is not currently using lookaside memory, or in other words
** when the "current value" returned by
** [sqlite3_db_status](D,[SQLITE_CONFIG_LOOKASIDE],...) is zero.
** Any attempt to change the lookaside memory configuration when lookaside
** memory is in use leaves the configuration unchanged and returns
** [SQLITE_BUSY].)^</dd>
**
** [[SQLITE_DBCONFIG_ENABLE_FKEY]]
** <dt>SQLITE_DBCONFIG_ENABLE_FKEY</dt>
** <dd> ^This option is used to enable or disable the enforcement of
** [foreign key constraints].  There should be two additional arguments.
** The first argument is an integer which is 0 to disable FK enforcement,
** positive to enable FK enforcement or negative to leave FK enforcement
** unchanged.  The second parameter is a pointer to an integer into which
** is written 0 or 1 to indicate whether FK enforcement is off or on
** following this call.  The second parameter may be a NULL pointer, in
** which case the FK enforcement setting is not reported back. </dd>
**
** [[SQLITE_DBCONFIG_ENABLE_TRIGGER]]
** <dt>SQLITE_DBCONFIG_ENABLE_TRIGGER</dt>
** <dd> ^This option is used to enable or disable [CREATE TRIGGER | triggers].
** There should be two additional arguments.
** The first argument is an integer which is 0 to disable triggers,
** positive to enable triggers or negative to leave the setting unchanged.
** The second parameter is a pointer to an integer into which
** is written 0 or 1 to indicate whether triggers are disabled or enabled
** following this call.  The second parameter may be a NULL pointer, in
** which case the trigger setting is not reported back.
**
** <p>Originally this option disabled all triggers.  ^(However, since
** SQLite version 3.35.0, TEMP triggers are still allowed even if
** this option is off.  So, in other words, this option now only disables
** triggers in the main database schema or in the schemas of ATTACH-ed
** databases.)^ </dd>
**
** [[SQLITE_DBCONFIG_ENABLE_VIEW]]
** <dt>SQLITE_DBCONFIG_ENABLE_VIEW</dt>
** <dd> ^This option is used to enable or disable [CREATE VIEW | views].
** There should be two additional arguments.
** The first argument is an integer which is 0 to disable views,
** positive to enable views or negative to leave the setting unchanged.
** The second parameter is a pointer to an integer into which
** is written 0 or 1 to indicate whether views are disabled or enabled
** following this call.  The second parameter may be a NULL pointer, in
** which case the view setting is not reported back.
**
** <p>Originally this option disabled all views.  ^(However, since
** SQLite version 3.35.0, TEMP views are still allowed even if
** this option is off.  So, in other words, this option now only disables
** views in the main database schema or in the schemas of ATTACH-ed
** databases.)^ </dd>
**
** [[SQLITE_DBCONFIG_ENABLE_FTS3_TOKENIZER]]
** <dt>SQLITE_DBCONFIG_ENABLE_FTS3_TOKENIZER</dt>
** <dd> ^This option is used to enable or disable the
** [fts3_tokenizer()] function which is part of the
** [FTS3] full-text search engine extension.
** There should be two additional arguments.
** The first argument is an integer which is 0 to disable fts3_tokenizer() or
** positive to enable fts3_tokenizer() or negative to leave the setting
** unchanged.
** The second parameter is a pointer to an integer into which
** is written 0 or 1 to indicate whether fts3_tokenizer is disabled or enabled
** following this call.  The second parameter may be a NULL pointer, in
** which case the new setting is not reported back. </dd>
**
** [[SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION]]
** <dt>SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION</dt>
** <dd> ^This option is used to enable or disable the [sqlite3_load_extension()]
** interface independently of the [load_extension()] SQL function.
** The [sqlite3_enable_load_extension()] API enables or disables both the
** C-API [sqlite3_load_extension()] and the SQL function [load_extension()].
** There should be two additional arguments.
** When the first argument to this interface is 1, then only the C-API is
** enabled and the SQL function remains disabled.  If the first argument to
** this interface is 0, then both the C-API and the SQL function are disabled.
** If the first argument is -1, then no changes are made to state of either the
** C-API or the SQL function.
** The second parameter is a pointer to an integer into which
** is written 0 or 1 to indicate whether [sqlite3_load_extension()] interface
** is disabled or enabled following this call.  The second parameter may
** be a NULL pointer, in which case the new setting is not reported back.
** </dd>
**
** [[SQLITE_DBCONFIG_MAINDBNAME]] <dt>SQLITE_DBCONFIG_MAINDBNAME</dt>
** <dd> ^This option is used to change the name of the "main" database
** schema.  ^The sole argument is a pointer to a constant UTF8 string
** which will become the new schema name in place of "main".  ^SQLite
** does not make a copy of the new main schema name string, so the application
** must ensure that the argument passed into this DBCONFIG option is unchanged
** until after the database connection closes.
** </dd>
**
** [[SQLITE_DBCONFIG_NO_CKPT_ON_CLOSE]]
** <dt>SQLITE_DBCONFIG_NO_CKPT_ON_CLOSE</dt>
** <dd> Usually, when a database in wal mode is closed or detached from a
** database handle, SQLite checks if this will mean that there are now no
** connections at all to the database. If so, it performs a checkpoint
** operation before closing the connection. This option may be used to
** override this behaviour. The first parameter passed to this operation
** is an integer - positive to disable checkpoints-on-close, or zero (the
** default) to enable them, and negative to leave the setting unchanged.
** The second parameter is a pointer to an integer
** into which is written 0 or 1 to indicate whether checkpoints-on-close
** have been disabled - 0 if they are not disabled, 1 if they are.
** </dd>
**
** [[SQLITE_DBCONFIG_ENABLE_QPSG]] <dt>SQLITE_DBCONFIG_ENABLE_QPSG</dt>
** <dd>^(The SQLITE_DBCONFIG_ENABLE_QPSG option activates or deactivates
** the [query planner stability guarantee] (QPSG).  When the QPSG is active,
** a single SQL query statement will always use the same algorithm regardless
** of values of [bound parameters].)^ The QPSG disables some query optimizations
** that look at the values of bound parameters, which can make some queries
** slower.  But the QPSG has the advantage of more predictable behavior.  With
** the QPSG active, SQLite will always use the same query plan in the field as
** was used during testing in the lab.
** The first argument to this setting is an integer which is 0 to disable
** the QPSG, positive to enable QPSG, or negative to leave the setting
** unchanged. The second parameter is a pointer to an integer into which
** is written 0 or 1 to indicate whether the QPSG is disabled or enabled
** following this call.
** </dd>
**
** [[SQLITE_DBCONFIG_TRIGGER_EQP]] <dt>SQLITE_DBCONFIG_TRIGGER_EQP</dt>
** <dd> By default, the output of EXPLAIN QUERY PLAN commands does not
** include output for any operations performed by trigger programs. This
** option is used to set or clear (the default) a flag that governs this
** behavior. The first parameter passed to this operation is an integer -
** positive to enable output for trigger programs, or zero to disable it,
** or negative to leave the setting unchanged.
** The second parameter is a pointer to an integer into which is written
** 0 or 1 to indicate whether output-for-triggers has been disabled - 0 if
** it is not disabled, 1 if it is.
** </dd>
**
** [[SQLITE_DBCONFIG_RESET_DATABASE]] <dt>SQLITE_DBCONFIG_RESET_DATABASE</dt>
** <dd> Set the SQLITE_DBCONFIG_RESET_DATABASE flag and then run
** [VACUUM] in order to reset a database back to an empty database
** with no schema and no content. The following process works even for
** a badly corrupted database file:
** <ol>
** <li> If the database connection is newly opened, make sure it has read the
**      database schema by preparing then discarding some query against the
**      database, or calling sqlite3_table_column_metadata(), ignoring any
**      errors.  This step is only necessary if the application desires to keep
**      the database in WAL mode after the reset if it was in WAL mode before
**      the reset.
** <li> sqlite3_db_config(db, SQLITE_DBCONFIG_RESET_DATABASE, 1, 0);
** <li> [sqlite3_exec](db, "[VACUUM]", 0, 0, 0);
** <li> sqlite3_db_config(db, SQLITE_DBCONFIG_RESET_DATABASE, 0, 0);
** </ol>
** Because resetting a database is destructive and irreversible, the
** process requires the use of this obscure API and multiple steps to help
** ensure that it does not happen by accident.
**
** [[SQLITE_DBCONFIG_DEFENSIVE]] <dt>SQLITE_DBCONFIG_DEFENSIVE</dt>
** <dd>The SQLITE_DBCONFIG_DEFENSIVE option activates or deactivates the
** "defensive" flag for a database connection.  When the defensive
** flag is enabled, language features that allow ordinary SQL to
** deliberately corrupt the database file are disabled.  The disabled
** features include but are not limited to the following:
** <ul>
** <li> The [PRAGMA writable_schema=ON] statement.
** <li> The [PRAGMA journal_mode=OFF] statement.
** <li> Writes to the [sqlite_dbpage] virtual table.
** <li> Direct writes to [shadow tables].
** </ul>
** </dd>
**
** [[SQLITE_DBCONFIG_WRITABLE_SCHEMA]] <dt>SQLITE_DBCONFIG_WRITABLE_SCHEMA</dt>
** <dd>The SQLITE_DBCONFIG_WRITABLE_SCHEMA option activates or deactivates the
** "writable_schema" flag. This has the same effect and is logically equivalent
** to setting [PRAGMA writable_schema=ON] or [PRAGMA writable_schema=OFF].
** The first argument to this setting is an integer which is 0 to disable
** the writable_schema, positive to enable writable_schema, or negative to
** leave the setting unchanged. The second parameter is a pointer to an
** integer into which is written 0 or 1 to indicate whether the writable_schema
** is enabled or disabled following this call.
** </dd>
**
** [[SQLITE_DBCONFIG_LEGACY_ALTER_TABLE]]
** <dt>SQLITE_DBCONFIG_LEGACY_ALTER_TABLE</dt>
** <dd>The SQLITE_DBCONFIG_LEGACY_ALTER_TABLE option activates or deactivates
** the legacy behavior of the [ALTER TABLE RENAME] command such it
** behaves as it did prior to [version 3.24.0] (2018-06-04).  See the
** "Compatibility Notice" on the [ALTER TABLE RENAME documentation] for
** additional information. This feature can also be turned on and off
** using the [PRAGMA legacy_alter_table] statement.
** </dd>
**
** [[SQLITE_DBCONFIG_DQS_DML]]
** <dt>SQLITE_DBCONFIG_DQS_DML</td>
** <dd>The SQLITE_DBCONFIG_DQS_DML option activates or deactivates
** the legacy [double-quoted string literal] misfeature for DML statements
** only, that is DELETE, INSERT, SELECT, and UPDATE statements. The
** default value of this setting is determined by the [-DSQLITE_DQS]
** compile-time option.
** </dd>
**
** [[SQLITE_DBCONFIG_DQS_DDL]]
** <dt>SQLITE_DBCONFIG_DQS_DDL</td>
** <dd>The SQLITE_DBCONFIG_DQS option activates or deactivates
** the legacy [double-quoted string literal] misfeature for DDL statements,
** such as CREATE TABLE and CREATE INDEX. The
** default value of this setting is determined by the [-DSQLITE_DQS]
** compile-time option.
** </dd>
**
** [[SQLITE_DBCONFIG_TRUSTED_SCHEMA]]
** <dt>SQLITE_DBCONFIG_TRUSTED_SCHEMA</td>
** <dd>The SQLITE_DBCONFIG_TRUSTED_SCHEMA option tells SQLite to
** assume that database schemas are untainted by malicious content.
** When the SQLITE_DBCONFIG_TRUSTED_SCHEMA option is disabled, SQLite
** takes additional defensive steps to protect the application from harm
** including:
** <ul>
** <li> Prohibit the use of SQL functions inside triggers, views,
** CHECK constraints, DEFAULT clauses, expression indexes,
** partial indexes, or generated columns
** unless those functions are tagged with [SQLITE_INNOCUOUS].
** <li> Prohibit the use of virtual tables inside of triggers or views
** unless those virtual tables are tagged with [SQLITE_VTAB_INNOCUOUS].
** </ul>
** This setting defaults to "on" for legacy compatibility, however
** all applications are advised to turn it off if possible. This setting
** can also be controlled using the [PRAGMA trusted_schema] statement.
** </dd>
**
** [[SQLITE_DBCONFIG_LEGACY_FILE_FORMAT]]
** <dt>SQLITE_DBCONFIG_LEGACY_FILE_FORMAT</td>
** <dd>The SQLITE_DBCONFIG_LEGACY_FILE_FORMAT option activates or deactivates
** the legacy file format flag.  When activated, this flag causes all newly
** created database file to have a schema format version number (the 4-byte
** integer found at offset 44 into the database header) of 1.  This in turn
** means that the resulting database file will be readable and writable by
** any SQLite version back to 3.0.0 ([dateof:3.0.0]).  Without this setting,
** newly created databases are generally not understandable by SQLite versions
** prior to 3.3.0 ([dateof:3.3.0]).  As these words are written, there
** is now scarcely any need to generated database files that are compatible
** all the way back to version 3.0.0, and so this setting is of little
** practical use, but is provided so that SQLite can continue to claim the
** ability to generate new database files that are compatible with  version
** 3.0.0.
** <p>Note that when the SQLITE_DBCONFIG_LEGACY_FILE_FORMAT setting is on,
** the [VACUUM] command will fail with an obscure error when attempting to
** process a table with generated columns and a descending index.  This is
** not considered a bug since SQLite versions 3.3.0 and earlier do not support
** either generated columns or decending indexes.
** </dd>
** </dl>
*/
#define SQLITE_DBCONFIG_MAINDBNAME            1000 /* const char* */
#define SQLITE_DBCONFIG_LOOKASIDE             1001 /* void* int int */
#define SQLITE_DBCONFIG_ENABLE_FKEY           1002 /* int int* */
#define SQLITE_DBCONFIG_ENABLE_TRIGGER        1003 /* int int* */
#define SQLITE_DBCONFIG_ENABLE_FTS3_TOKENIZER 1004 /* int int* */
#define SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION 1005 /* int int* */
#define SQLITE_DBCONFIG_NO_CKPT_ON_CLOSE      1006 /* int int* */
#define SQLITE_DBCONFIG_ENABLE_QPSG           1007 /* int int* */
#define SQLITE_DBCONFIG_TRIGGER_EQP           1008 /* int int* */
#define SQLITE_DBCONFIG_RESET_DATABASE        1009 /* int int* */
#define SQLITE_DBCONFIG_DEFENSIVE             1010 /* int int* */
#define SQLITE_DBCONFIG_WRITABLE_SCHEMA       1011 /* int int* */
#define SQLITE_DBCONFIG_LEGACY_ALTER_TABLE    1012 /* int int* */
#define SQLITE_DBCONFIG_DQS_DML               1013 /* int int* */
#define SQLITE_DBCONFIG_DQS_DDL               1014 /* int int* */
#define SQLITE_DBCONFIG_ENABLE_VIEW           1015 /* int int* */
#define SQLITE_DBCONFIG_LEGACY_FILE_FORMAT    1016 /* int int* */
#define SQLITE_DBCONFIG_TRUSTED_SCHEMA        1017 /* int int* */
#define SQLITE_DBCONFIG_MAX                   1017 /* Largest DBCONFIG */

/*
** CAPI3REF: Enable Or Disable Extended Result Codes
** METHOD: sqlite3
**
** ^The sqlite3_extended_result_codes() routine enables or disables the
** [extended result codes] feature of SQLite. ^The extended result
** codes are disabled by default for historical compatibility.
*/
SQLITE_API int sqlite3_extended_result_codes(sqlite3*, int onoff);

/*
** CAPI3REF: Last Insert Rowid
** METHOD: sqlite3
**
** ^Each entry in most SQLite tables (except for [WITHOUT ROWID] tables)
** has a unique 64-bit signed
** integer key called the [ROWID | "rowid"]. ^The rowid is always available
** as an undeclared column named ROWID, OID, or _ROWID_ as long as those
** names are not also used by explicitly declared columns. ^If
** the table has a column of type [INTEGER PRIMARY KEY] then that column
** is another alias for the rowid.
**
** ^The sqlite3_last_insert_rowid(D) interface usually returns the [rowid] of
** the most recent successful [INSERT] into a rowid table or [virtual table]
** on database connection D. ^Inserts into [WITHOUT ROWID] tables are not
** recorded. ^If no successful [INSERT]s into rowid tables have ever occurred
** on the database connection D, then sqlite3_last_insert_rowid(D) returns
** zero.
**
** As well as being set automatically as rows are inserted into database
** tables, the value returned by this function may be set explicitly by
** [sqlite3_set_last_insert_rowid()]
**
** Some virtual table implementations may INSERT rows into rowid tables as
** part of committing a transaction (e.g. to flush data accumulated in memory
** to disk). In this case subsequent calls to this function return the rowid
** associated with these internal INSERT operations, which leads to
** unintuitive results. Virtual table implementations that do write to rowid
** tables in this way can avoid this problem by restoring the original
** rowid value using [sqlite3_set_last_insert_rowid()] before returning
** control to the user.
**
** ^(If an [INSERT] occurs within a trigger then this routine will
** return the [rowid] of the inserted row as long as the trigger is
** running. Once the trigger program ends, the value returned
** by this routine reverts to what it was before the trigger was fired.)^
**
** ^An [INSERT] that fails due to a constraint violation is not a
** successful [INSERT] and does not change the value returned by this
** routine.  ^Thus INSERT OR FAIL, INSERT OR IGNORE, INSERT OR ROLLBACK,
** and INSERT OR ABORT make no changes to the return value of this
** routine when their insertion fails.  ^(When INSERT OR REPLACE
** encounters a constraint violation, it does not fail.  The
** INSERT continues to completion after deleting rows that caused
** the constraint problem so INSERT OR REPLACE will always change
** the return value of this interface.)^
**
** ^For the purposes of this routine, an [INSERT] is considered to
** be successful even if it is subsequently rolled back.
**
** This function is accessible to SQL statements via the
** [last_insert_rowid() SQL function].
**
** If a separate thread performs a new [INSERT] on the same
** database connection while the [sqlite3_last_insert_rowid()]
** function is running and thus changes the last insert [rowid],
** then the value returned by [sqlite3_last_insert_rowid()] is
** unpredictable and might not equal either the old or the new
** last insert [rowid].
*/
SQLITE_API sqlite3_int64 sqlite3_last_insert_rowid(sqlite3*);

/*
** CAPI3REF: Set the Last Insert Rowid value.
** METHOD: sqlite3
**
** The sqlite3_set_last_insert_rowid(D, R) method allows the application to
** set the value returned by calling sqlite3_last_insert_rowid(D) to R
** without inserting a row into the database.
*/
SQLITE_API void sqlite3_set_last_insert_rowid(sqlite3*,sqlite3_int64);

/*
** CAPI3REF: Count The Number Of Rows Modified
** METHOD: sqlite3
**
** ^These functions return the number of rows modified, inserted or
** deleted by the most recently completed INSERT, UPDATE or DELETE
** statement on the database connection specified by the only parameter.
** The two functions are identical except for the type of the return value
** and that if the number of rows modified by the most recent INSERT, UPDATE
** or DELETE is greater than the maximum value supported by type "int", then
** the return value of sqlite3_changes() is undefined. ^Executing any other
** type of SQL statement does not modify the value returned by these functions.
**
** ^Only changes made directly by the INSERT, UPDATE or DELETE statement are
** considered - auxiliary changes caused by [CREATE TRIGGER | triggers],
** [foreign key actions] or [REPLACE] constraint resolution are not counted.
**
** Changes to a view that are intercepted by
** [INSTEAD OF trigger | INSTEAD OF triggers] are not counted. ^The value
** returned by sqlite3_changes() immediately after an INSERT, UPDATE or
** DELETE statement run on a view is always zero. Only changes made to real
** tables are counted.
**
** Things are more complicated if the sqlite3_changes() function is
** executed while a trigger program is running. This may happen if the
** program uses the [changes() SQL function], or if some other callback
** function invokes sqlite3_changes() directly. Essentially:
**
** <ul>
**   <li> ^(Before entering a trigger program the value returned by
**        sqlite3_changes() function is saved. After the trigger program
**        has finished, the original value is restored.)^
**
**   <li> ^(Within a trigger program each INSERT, UPDATE and DELETE
**        statement sets the value returned by sqlite3_changes()
**        upon completion as normal. Of course, this value will not include
**        any changes performed by sub-triggers, as the sqlite3_changes()
**        value will be saved and restored after each sub-trigger has run.)^
** </ul>
**
** ^This means that if the changes() SQL function (or similar) is used
** by the first INSERT, UPDATE or DELETE statement within a trigger, it
** returns the value as set when the calling statement began executing.
** ^If it is used by the second or subsequent such statement within a trigger
** program, the value returned reflects the number of rows modified by the
** previous INSERT, UPDATE or DELETE statement within the same trigger.
**
** If a separate thread makes changes on the same database connection
** while [sqlite3_changes()] is running then the value returned
** is unpredictable and not meaningful.
**
** See also:
** <ul>
** <li> the [sqlite3_total_changes()] interface
** <li> the [count_changes pragma]
** <li> the [changes() SQL function]
** <li> the [data_version pragma]
** </ul>
*/
SQLITE_API int sqlite3_changes(sqlite3*);
SQLITE_API sqlite3_int64 sqlite3_changes64(sqlite3*);

/*
** CAPI3REF: Total Number Of Rows Modified
** METHOD: sqlite3
**
** ^These functions return the total number of rows inserted, modified or
** deleted by all [INSERT], [UPDATE] or [DELETE] statements completed
** since the database connection was opened, including those executed as
** part of trigger programs. The two functions are identical except for the
** type of the return value and that if the number of rows modified by the
** connection exceeds the maximum value supported by type "int", then
** the return value of sqlite3_total_changes() is undefined. ^Executing
** any other type of SQL statement does not affect the value returned by
** sqlite3_total_changes().
**
** ^Changes made as part of [foreign key actions] are included in the
** count, but those made as part of REPLACE constraint resolution are
** not. ^Changes to a view that are intercepted by INSTEAD OF triggers
** are not counted.
**
** The [sqlite3_total_changes(D)] interface only reports the number
** of rows that changed due to SQL statement run against database
** connection D.  Any changes by other database connections are ignored.
** To detect changes against a database file from other database
** connections use the [PRAGMA data_version] command or the
** [SQLITE_FCNTL_DATA_VERSION] [file control].
**
** If a separate thread makes changes on the same database connection
** while [sqlite3_total_changes()] is running then the value
** returned is unpredictable and not meaningful.
**
** See also:
** <ul>
** <li> the [sqlite3_changes()] interface
** <li> the [count_changes pragma]
** <li> the [changes() SQL function]
** <li> the [data_version pragma]
** <li> the [SQLITE_FCNTL_DATA_VERSION] [file control]
** </ul>
*/
SQLITE_API int sqlite3_total_changes(sqlite3*);
SQLITE_API sqlite3_int64 sqlite3_total_changes64(sqlite3*);

/*
** CAPI3REF: Interrupt A Long-Running Query
** METHOD: sqlite3
**
** ^This function causes any pending database operation to abort and
** return at its earliest opportunity. This routine is typically
** called in response to a user action such as pressing "Cancel"
** or Ctrl-C where the user wants a long query operation to halt
** immediately.
**
** ^It is safe to call this routine from a thread different from the
** thread that is currently running the database operation.  But it
** is not safe to call this routine with a [database connection] that
** is closed or might close before sqlite3_interrupt() returns.
**
** ^If an SQL operation is very nearly finished at the time when
** sqlite3_interrupt() is called, then it might not have an opportunity
** to be interrupted and might continue to completion.
**
** ^An SQL operation that is interrupted will return [SQLITE_INTERRUPT].
** ^If the interrupted SQL operation is an INSERT, UPDATE, or DELETE
** that is inside an explicit transaction, then the entire transaction
** will be rolled back automatically.
**
** ^The sqlite3_interrupt(D) call is in effect until all currently running
** SQL statements on [database connection] D complete.  ^Any new SQL statements
** that are started after the sqlite3_interrupt() call and before the
** running statement count reaches zero are interrupted as if they had been
** running prior to the sqlite3_interrupt() call.  ^New SQL statements
** that are started after the running statement count reaches zero are
** not effected by the sqlite3_interrupt().
** ^A call to sqlite3_interrupt(D) that occurs when there are no running
** SQL statements is a no-op and has no effect on SQL statements
** that are started after the sqlite3_interrupt() call returns.
*/
SQLITE_API void sqlite3_interrupt(sqlite3*);

/*
** CAPI3REF: Determine If An SQL Statement Is Complete
**
** These routines are useful during command-line input to determine if the
** currently entered text seems to form a complete SQL statement or
** if additional input is needed before sending the text into
** SQLite for parsing.  ^These routines return 1 if the input string
** appears to be a complete SQL statement.  ^A statement is judged to be
** complete if it ends with a semicolon token and is not a prefix of a
** well-formed CREATE TRIGGER statement.  ^Semicolons that are embedded within
** string literals or quoted identifier names or comments are not
** independent tokens (they are part of the token in which they are
** embedded) and thus do not count as a statement terminator.  ^Whitespace
** and comments that follow the final semicolon are ignored.
**
** ^These routines return 0 if the statement is incomplete.  ^If a
** memory allocation fails, then SQLITE_NOMEM is returned.
**
** ^These routines do not parse the SQL statements thus
** will not detect syntactically incorrect SQL.
**
** ^(If SQLite has not been initialized using [sqlite3_initialize()] prior
** to invoking sqlite3_complete16() then sqlite3_initialize() is invoked
** automatically by sqlite3_complete16().  If that initialization fails,
** then the return value from sqlite3_complete16() will be non-zero
** regardless of whether or not the input SQL is complete.)^
**
** The input to [sqlite3_complete()] must be a zero-terminated
** UTF-8 string.
**
** The input to [sqlite3_complete16()] must be a zero-terminated
** UTF-16 string in native byte order.
*/
SQLITE_API int sqlite3_complete(const char *sql);
SQLITE_API int sqlite3_complete16(const void *sql);

/*
** CAPI3REF: Register A Callback To Handle SQLITE_BUSY Errors
** KEYWORDS: {busy-handler callback} {busy handler}
** METHOD: sqlite3
**
** ^The sqlite3_busy_handler(D,X,P) routine sets a callback function X
** that might be invoked with argument P whenever
** an attempt is made to access a database table associated with
** [database connection] D when another thread
** or process has the table locked.
** The sqlite3_busy_handler() interface is used to implement
** [sqlite3_busy_timeout()] and [PRAGMA busy_timeout].
**
** ^If the busy callback is NULL, then [SQLITE_BUSY]
** is returned immediately upon encountering the lock.  ^If the busy callback
** is not NULL, then the callback might be invoked with two arguments.
**
** ^The first argument to the busy handler is a copy of the void* pointer which
** is the third argument to sqlite3_busy_handler().  ^The second argument to
** the busy handler callback is the number of times that the busy handler has
** been invoked previously for the same locking event.  ^If the
** busy callback returns 0, then no additional attempts are made to
** access the database and [SQLITE_BUSY] is returned
** to the application.
** ^If the callback returns non-zero, then another attempt
** is made to access the database and the cycle repeats.
**
** The presence of a busy handler does not guarantee that it will be invoked
** when there is lock contention. ^If SQLite determines that invoking the busy
** handler could result in a deadlock, it will go ahead and return [SQLITE_BUSY]
** to the application instead of invoking the
** busy handler.
** Consider a scenario where one process is holding a read lock that
** it is trying to promote to a reserved lock and
** a second process is holding a reserved lock that it is trying
** to promote to an exclusive lock.  The first process cannot proceed
** because it is blocked by the second and the second process cannot
** proceed because it is blocked by the first.  If both processes
** invoke the busy handlers, neither will make any progress.  Therefore,
** SQLite returns [SQLITE_BUSY] for the first process, hoping that this
** will induce the first process to release its read lock and allow
** the second process to proceed.
**
** ^The default busy callback is NULL.
**
** ^(There can only be a single busy handler defined for each
** [database connection].  Setting a new busy handler clears any
** previously set handler.)^  ^Note that calling [sqlite3_busy_timeout()]
** or evaluating [PRAGMA busy_timeout=N] will change the
** busy handler and thus clear any previously set busy handler.
**
** The busy callback should not take any actions which modify the
** database connection that invoked the busy handler.  In other words,
** the busy handler is not reentrant.  Any such actions
** result in undefined behavior.
**
** A busy handler must not close the database connection
** or [prepared statement] that invoked the busy handler.
*/
SQLITE_API int sqlite3_busy_handler(sqlite3*,int(*)(void*,int),void*);

/*
** CAPI3REF: Set A Busy Timeout
** METHOD: sqlite3
**
** ^This routine sets a [sqlite3_busy_handler | busy handler] that sleeps
** for a specified amount of time when a table is locked.  ^The handler
** will sleep multiple times until at least "ms" milliseconds of sleeping
** have accumulated.  ^After at least "ms" milliseconds of sleeping,
** the handler returns 0 which causes [sqlite3_step()] to return
** [SQLITE_BUSY].
**
** ^Calling this routine with an argument less than or equal to zero
** turns off all busy handlers.
**
** ^(There can only be a single busy handler for a particular
** [database connection] at any given moment.  If another busy handler
** was defined  (using [sqlite3_busy_handler()]) prior to calling
** this routine, that other busy handler is cleared.)^
**
** See also:  [PRAGMA busy_timeout]
*/
SQLITE_API int sqlite3_busy_timeout(sqlite3*, int ms);

/*
** CAPI3REF: Convenience Routines For Running Queries
** METHOD: sqlite3
**
** This is a legacy interface that is preserved for backwards compatibility.
** Use of this interface is not recommended.
**
** Definition: A <b>result table</b> is memory data structure created by the
** [sqlite3_get_table()] interface.  A result table records the
** complete query results from one or more queries.
**
** The table conceptually has a number of rows and columns.  But
** these numbers are not part of the result table itself.  These
** numbers are obtained separately.  Let N be the number of rows
** and M be the number of columns.
**
** A result table is an array of pointers to zero-terminated UTF-8 strings.
** There are (N+1)*M elements in the array.  The first M pointers point
** to zero-terminated strings that  contain the names of the columns.
** The remaining entries all point to query results.  NULL values result
** in NULL pointers.  All other values are in their UTF-8 zero-terminated
** string representation as returned by [sqlite3_column_text()].
**
** A result table might consist of one or more memory allocations.
** It is not safe to pass a result table directly to [sqlite3_free()].
** A result table should be deallocated using [sqlite3_free_table()].
**
** ^(As an example of the result table format, suppose a query result
** is as follows:
**
** <blockquote><pre>
**        Name        | Age
**        -----------------------
**        Alice       | 43
**        Bob         | 28
**        Cindy       | 21
** </pre></blockquote>
**
** There are two columns (M==2) and three rows (N==3).  Thus the
** result table has 8 entries.  Suppose the result table is stored
** in an array named azResult.  Then azResult holds this content:
**
** <blockquote><pre>
**        azResult&#91;0] = "Name";
**        azResult&#91;1] = "Age";
**        azResult&#91;2] = "Alice";
**        azResult&#91;3] = "43";
**        azResult&#91;4] = "Bob";
**        azResult&#91;5] = "28";
**        azResult&#91;6] = "Cindy";
**        azResult&#91;7] = "21";
** </pre></blockquote>)^
**
** ^The sqlite3_get_table() function evaluates one or more
** semicolon-separated SQL statements in the zero-terminated UTF-8
** string of its 2nd parameter and returns a result table to the
** pointer given in its 3rd parameter.
**
** After the application has finished with the result from sqlite3_get_table(),
** it must pass the result table pointer to sqlite3_free_table() in order to
** release the memory that was malloced.  Because of the way the
** [sqlite3_malloc()] happens within sqlite3_get_table(), the calling
** function must not try to call [sqlite3_free()] directly.  Only
** [sqlite3_free_table()] is able to release the memory properly and safely.
**
** The sqlite3_get_table() interface is implemented as a wrapper around
** [sqlite3_exec()].  The sqlite3_get_table() routine does not have access
** to any internal data structures of SQLite.  It uses only the public
** interface defined here.  As a consequence, errors that occur in the
** wrapper layer outside of the internal [sqlite3_exec()] call are not
** reflected in subsequent calls to [sqlite3_errcode()] or
** [sqlite3_errmsg()].
*/
SQLITE_API int sqlite3_get_table(
  sqlite3 *db,          /* An open database */
  const char *zSql,     /* SQL to be evaluated */
  char ***pazResult,    /* Results of the query */
  int *pnRow,           /* Number of result rows written here */
  int *pnColumn,        /* Number of result columns written here */
  char **pzErrmsg       /* Error msg written here */
);
SQLITE_API void sqlite3_free_table(char **result);

/*
** CAPI3REF: Formatted String Printing Functions
**
** These routines are work-alikes of the "printf()" family of functions
** from the standard C library.
** These routines understand most of the common formatting options from
** the standard library printf()
** plus some additional non-standard formats ([%q], [%Q], [%w], and [%z]).
** See the [built-in printf()] documentation for details.
**
** ^The sqlite3_mprintf() and sqlite3_vmprintf() routines write their
** results into memory obtained from [sqlite3_malloc64()].
** The strings returned by these two routines should be
** released by [sqlite3_free()].  ^Both routines return a
** NULL pointer if [sqlite3_malloc64()] is unable to allocate enough
** memory to hold the resulting string.
**
** ^(The sqlite3_snprintf() routine is similar to "snprintf()" from
** the standard C library.  The result is written into the
** buffer supplied as the second parameter whose size is given by
** the first parameter. Note that the order of the
** first two parameters is reversed from snprintf().)^  This is an
** historical accident that cannot be fixed without breaking
** backwards compatibility.  ^(Note also that sqlite3_snprintf()
** returns a pointer to its buffer instead of the number of
** characters actually written into the buffer.)^  We admit that
** the number of characters written would be a more useful return
** value but we cannot change the implementation of sqlite3_snprintf()
** now without breaking compatibility.
**
** ^As long as the buffer size is greater than zero, sqlite3_snprintf()
** guarantees that the buffer is always zero-terminated.  ^The first
** parameter "n" is the total size of the buffer, including space for
** the zero terminator.  So the longest string that can be completely
** written will be n-1 characters.
**
** ^The sqlite3_vsnprintf() routine is a varargs version of sqlite3_snprintf().
**
** See also:  [built-in printf()], [printf() SQL function]
*/
SQLITE_API char *sqlite3_mprintf(const char*,...);
SQLITE_API char *sqlite3_vmprintf(const char*, va_list);
SQLITE_API char *sqlite3_snprintf(int,char*,const char*, ...);
SQLITE_API char *sqlite3_vsnprintf(int,char*,const char*, va_list);

/*
** CAPI3REF: Memory Allocation Subsystem
**
** The SQLite core uses these three routines for all of its own
** internal memory allocation needs. "Core" in the previous sentence
** does not include operating-system specific [VFS] implementation.  The
** Windows VFS uses native malloc() and free() for some operations.
**
** ^The sqlite3_malloc() routine returns a pointer to a block
** of memory at least N bytes in length, where N is the parameter.
** ^If sqlite3_malloc() is unable to obtain sufficient free
** memory, it returns a NULL pointer.  ^If the parameter N to
** sqlite3_malloc() is zero or negative then sqlite3_malloc() returns
** a NULL pointer.
**
** ^The sqlite3_malloc64(N) routine works just like
** sqlite3_malloc(N) except that N is an unsigned 64-bit integer instead
** of a signed 32-bit integer.
**
** ^Calling sqlite3_free() with a pointer previously returned
** by sqlite3_malloc() or sqlite3_realloc() releases that memory so
** that it might be reused.  ^The sqlite3_free() routine is
** a no-op if is called with a NULL pointer.  Passing a NULL pointer
** to sqlite3_free() is harmless.  After being freed, memory
** should neither be read nor written.  Even reading previously freed
** memory might result in a segmentation fault or other severe error.
** Memory corruption, a segmentation fault, or other severe error
** might result if sqlite3_free() is called with a non-NULL pointer that
** was not obtained from sqlite3_malloc() or sqlite3_realloc().
**
** ^The sqlite3_realloc(X,N) interface attempts to resize a
** prior memory allocation X to be at least N bytes.
** ^If the X parameter to sqlite3_realloc(X,N)
** is a NULL pointer then its behavior is identical to calling
** sqlite3_malloc(N).
** ^If the N parameter to sqlite3_realloc(X,N) is zero or
** negative then the behavior is exactly the same as calling
** sqlite3_free(X).
** ^sqlite3_realloc(X,N) returns a pointer to a memory allocation
** of at least N bytes in size or NULL if insufficient memory is available.
** ^If M is the size of the prior allocation, then min(N,M) bytes
** of the prior allocation are copied into the beginning of buffer returned
** by sqlite3_realloc(X,N) and the prior allocation is freed.
** ^If sqlite3_realloc(X,N) returns NULL and N is positive, then the
** prior allocation is not freed.
**
** ^The sqlite3_realloc64(X,N) interfaces works the same as
** sqlite3_realloc(X,N) except that N is a 64-bit unsigned integer instead
** of a 32-bit signed integer.
**
** ^If X is a memory allocation previously obtained from sqlite3_malloc(),
** sqlite3_malloc64(), sqlite3_realloc(), or sqlite3_realloc64(), then
** sqlite3_msize(X) returns the size of that memory allocation in bytes.
** ^The value returned by sqlite3_msize(X) might be larger than the number
** of bytes requested when X was allocated.  ^If X is a NULL pointer then
** sqlite3_msize(X) returns zero.  If X points to something that is not
** the beginning of memory allocation, or if it points to a formerly
** valid memory allocation that has now been freed, then the behavior
** of sqlite3_msize(X) is undefined and possibly harmful.
**
** ^The memory returned by sqlite3_malloc(), sqlite3_realloc(),
** sqlite3_malloc64(), and sqlite3_realloc64()
** is always aligned to at least an 8 byte boundary, or to a
** 4 byte boundary if the [SQLITE_4_BYTE_ALIGNED_MALLOC] compile-time
** option is used.
**
** The pointer arguments to [sqlite3_free()] and [sqlite3_realloc()]
** must be either NULL or else pointers obtained from a prior
** invocation of [sqlite3_malloc()] or [sqlite3_realloc()] that have
** not yet been released.
**
** The application must not read or write any part of
** a block of memory after it has been released using
** [sqlite3_free()] or [sqlite3_realloc()].
*/
SQLITE_API void *sqlite3_malloc(int);
SQLITE_API void *sqlite3_malloc64(sqlite3_uint64);
SQLITE_API void *sqlite3_realloc(void*, int);
SQLITE_API void *sqlite3_realloc64(void*, sqlite3_uint64);
SQLITE_API void sqlite3_free(void*);
SQLITE_API sqlite3_uint64 sqlite3_msize(void*);

/*
** CAPI3REF: Memory Allocator Statistics
**
** SQLite provides these two interfaces for reporting on the status
** of the [sqlite3_malloc()], [sqlite3_free()], and [sqlite3_realloc()]
** routines, which form the built-in memory allocation subsystem.
**
** ^The [sqlite3_memory_used()] routine returns the number of bytes
** of memory currently outstanding (malloced but not freed).
** ^The [sqlite3_memory_highwater()] routine returns the maximum
** value of [sqlite3_memory_used()] since the high-water mark
** was last reset.  ^The values returned by [sqlite3_memory_used()] and
** [sqlite3_memory_highwater()] include any overhead
** added by SQLite in its implementation of [sqlite3_malloc()],
** but not overhead added by the any underlying system library
** routines that [sqlite3_malloc()] may call.
**
** ^The memory high-water mark is reset to the current value of
** [sqlite3_memory_used()] if and only if the parameter to
** [sqlite3_memory_highwater()] is true.  ^The value returned
** by [sqlite3_memory_highwater(1)] is the high-water mark
** prior to the reset.
*/
SQLITE_API sqlite3_int64 sqlite3_memory_used(void);
SQLITE_API sqlite3_int64 sqlite3_memory_highwater(int resetFlag);

/*
** CAPI3REF: Pseudo-Random Number Generator
**
** SQLite contains a high-quality pseudo-random number generator (PRNG) used to
** select random [ROWID | ROWIDs] when inserting new records into a table that
** already uses the largest possible [ROWID].  The PRNG is also used for
** the built-in random() and randomblob() SQL functions.  This interface allows
** applications to access the same PRNG for other purposes.
**
** ^A call to this routine stores N bytes of randomness into buffer P.
** ^The P parameter can be a NULL pointer.
**
** ^If this routine has not been previously called or if the previous
** call had N less than one or a NULL pointer for P, then the PRNG is
** seeded using randomness obtained from the xRandomness method of
** the default [sqlite3_vfs] object.
** ^If the previous call to this routine had an N of 1 or more and a
** non-NULL P then the pseudo-randomness is generated
** internally and without recourse to the [sqlite3_vfs] xRandomness
** method.
*/
SQLITE_API void sqlite3_randomness(int N, void *P);

/*
** CAPI3REF: Compile-Time Authorization Callbacks
** METHOD: sqlite3
** KEYWORDS: {authorizer callback}
**
** ^This routine registers an authorizer callback with a particular
** [database connection], supplied in the first argument.
** ^The authorizer callback is invoked as SQL statements are being compiled
** by [sqlite3_prepare()] or its variants [sqlite3_prepare_v2()],
** [sqlite3_prepare_v3()], [sqlite3_prepare16()], [sqlite3_prepare16_v2()],
** and [sqlite3_prepare16_v3()].  ^At various
** points during the compilation process, as logic is being created
** to perform various actions, the authorizer callback is invoked to
** see if those actions are allowed.  ^The authorizer callback should
** return [SQLITE_OK] to allow the action, [SQLITE_IGNORE] to disallow the
** specific action but allow the SQL statement to continue to be
** compiled, or [SQLITE_DENY] to cause the entire SQL statement to be
** rejected with an error.  ^If the authorizer callback returns
** any value other than [SQLITE_IGNORE], [SQLITE_OK], or [SQLITE_DENY]
** then the [sqlite3_prepare_v2()] or equivalent call that triggered
** the authorizer will fail with an error message.
**
** When the callback returns [SQLITE_OK], that means the operation
** requested is ok.  ^When the callback returns [SQLITE_DENY], the
** [sqlite3_prepare_v2()] or equivalent call that triggered the
** authorizer will fail with an error message explaining that
** access is denied.
**
** ^The first parameter to the authorizer callback is a copy of the third
** parameter to the sqlite3_set_authorizer() interface. ^The second parameter
** to the callback is an integer [SQLITE_COPY | action code] that specifies
** the particular action to be authorized. ^The third through sixth parameters
** to the callback are either NULL pointers or zero-terminated strings
** that contain additional details about the action to be authorized.
** Applications must always be prepared to encounter a NULL pointer in any
** of the third through the sixth parameters of the authorization callback.
**
** ^If the action code is [SQLITE_READ]
** and the callback returns [SQLITE_IGNORE] then the
** [prepared statement] statement is constructed to substitute
** a NULL value in place of the table column that would have
** been read if [SQLITE_OK] had been returned.  The [SQLITE_IGNORE]
** return can be used to deny an untrusted user access to individual
** columns of a table.
** ^When a table is referenced by a [SELECT] but no column values are
** extracted from that table (for example in a query like
** "SELECT count(*) FROM tab") then the [SQLITE_READ] authorizer callback
** is invoked once for that table with a column name that is an empty string.
** ^If the action code is [SQLITE_DELETE] and the callback returns
** [SQLITE_IGNORE] then the [DELETE] operation proceeds but the
** [truncate optimization] is disabled and all rows are deleted individually.
**
** An authorizer is used when [sqlite3_prepare | preparing]
** SQL statements from an untrusted source, to ensure that the SQL statements
** do not try to access data they are not allowed to see, or that they do not
** try to execute malicious statements that damage the database.  For
** example, an application may allow a user to enter arbitrary
** SQL queries for evaluation by a database.  But the application does
** not want the user to be able to make arbitrary changes to the
** database.  An authorizer could then be put in place while the
** user-entered SQL is being [sqlite3_prepare | prepared] that
** disallows everything except [SELECT] statements.
**
** Applications that need to process SQL from untrusted sources
** might also consider lowering resource limits using [sqlite3_limit()]
** and limiting database size using the [max_page_count] [PRAGMA]
** in addition to using an authorizer.
**
** ^(Only a single authorizer can be in place on a database connection
** at a time.  Each call to sqlite3_set_authorizer overrides the
** previous call.)^  ^Disable the authorizer by installing a NULL callback.
** The authorizer is disabled by default.
**
** The authorizer callback must not do anything that will modify
** the database connection that invoked the authorizer callback.
** Note that [sqlite3_prepare_v2()] and [sqlite3_step()] both modify their
** database connections for the meaning of "modify" in this paragraph.
**
** ^When [sqlite3_prepare_v2()] is used to prepare a statement, the
** statement might be re-prepared during [sqlite3_step()] due to a
** schema change.  Hence, the application should ensure that the
** correct authorizer callback remains in place during the [sqlite3_step()].
**
** ^Note that the authorizer callback is invoked only during
** [sqlite3_prepare()] or its variants.  Authorization is not
** performed during statement evaluation in [sqlite3_step()], unless
** as stated in the previous paragraph, sqlite3_step() invokes
** sqlite3_prepare_v2() to reprepare a statement after a schema change.
*/
SQLITE_API int sqlite3_set_authorizer(
  sqlite3*,
  int (*xAuth)(void*,int,const char*,const char*,const char*,const char*),
  void *pUserData
);

/*
** CAPI3REF: Authorizer Return Codes
**
** The [sqlite3_set_authorizer | authorizer callback function] must
** return either [SQLITE_OK] or one of these two constants in order
** to signal SQLite whether or not the action is permitted.  See the
** [sqlite3_set_authorizer | authorizer documentation] for additional
** information.
**
** Note that SQLITE_IGNORE is also used as a [conflict resolution mode]
** returned from the [sqlite3_vtab_on_conflict()] interface.
*/
#define SQLITE_DENY   1   /* Abort the SQL statement with an error */
#define SQLITE_IGNORE 2   /* Don't allow access, but don't generate an error */

/*
** CAPI3REF: Authorizer Action Codes
**
** The [sqlite3_set_authorizer()] interface registers a callback function
** that is invoked to authorize certain SQL statement actions.  The
** second parameter to the callback is an integer code that specifies
** what action is being authorized.  These are the integer action codes that
** the authorizer callback may be passed.
**
** These action code values signify what kind of operation is to be
** authorized.  The 3rd and 4th parameters to the authorization
** callback function will be parameters or NULL depending on which of these
** codes is used as the second parameter.  ^(The 5th parameter to the
** authorizer callback is the name of the database ("main", "temp",
** etc.) if applicable.)^  ^The 6th parameter to the authorizer callback
** is the name of the inner-most trigger or view that is responsible for
** the access attempt or NULL if this access attempt is directly from
** top-level SQL code.
*/
/******************************************* 3rd ************ 4th ***********/
#define SQLITE_CREATE_INDEX          1   /* Index Name      Table Name      */
#define SQLITE_CREATE_TABLE          2   /* Table Name      NULL            */
#define SQLITE_CREATE_TEMP_INDEX     3   /* Index Name      Table Name      */
#define SQLITE_CREATE_TEMP_TABLE     4   /* Table Name      NULL            */
#define SQLITE_CREATE_TEMP_TRIGGER   5   /* Trigger Name    Table Name      */
#define SQLITE_CREATE_TEMP_VIEW      6   /* View Name       NULL            */
#define SQLITE_CREATE_TRIGGER        7   /* Trigger Name    Table Name      */
#define SQLITE_CREATE_VIEW           8   /* View Name       NULL            */
#define SQLITE_DELETE                9   /* Table Name      NULL            */
#define SQLITE_DROP_INDEX           10   /* Index Name      Table Name      */
#define SQLITE_DROP_TABLE           11   /* Table Name      NULL            */
#define SQLITE_DROP_TEMP_INDEX      12   /* Index Name      Table Name      */
#define SQLITE_DROP_TEMP_TABLE      13   /* Table Name      NULL            */
#define SQLITE_DROP_TEMP_TRIGGER    14   /* Trigger Name    Table Name      */
#define SQLITE_DROP_TEMP_VIEW       15   /* View Name       NULL            */
#define SQLITE_DROP_TRIGGER         16   /* Trigger Name    Table Name      */
#define SQLITE_DROP_VIEW            17   /* View Name       NULL            */
#define SQLITE_INSERT               18   /* Table Name      NULL            */
#define SQLITE_PRAGMA               19   /* Pragma Name     1st arg or NULL */
#define SQLITE_READ                 20   /* Table Name      Column Name     */
#define SQLITE_SELECT               21   /* NULL            NULL            */
#define SQLITE_TRANSACTION          22   /* Operation       NULL            */
#define SQLITE_UPDATE               23   /* Table Name      Column Name     */
#define SQLITE_ATTACH               24   /* Filename        NULL            */
#define SQLITE_DETACH               25   /* Database Name   NULL            */
#define SQLITE_ALTER_TABLE          26   /* Database Name   Table Name      */
#define SQLITE_REINDEX              27   /* Index Name      NULL            */
#define SQLITE_ANALYZE              28   /* Table Name      NULL            */
#define SQLITE_CREATE_VTABLE        29   /* Table Name      Module Name     */
#define SQLITE_DROP_VTABLE          30   /* Table Name      Module Name     */
#define SQLITE_FUNCTION             31   /* NULL            Function Name   */
#define SQLITE_SAVEPOINT            32   /* Operation       Savepoint Name  */
#define SQLITE_COPY                  0   /* No longer used */
#define SQLITE_RECURSIVE            33   /* NULL            NULL            */

/*
** CAPI3REF: Tracing And Profiling Functions
** METHOD: sqlite3
**
** These routines are deprecated. Use the [sqlite3_trace_v2()] interface
** instead of the routines described here.
**
** These routines register callback functions that can be used for
** tracing and profiling the execution of SQL statements.
**
** ^The callback function registered by sqlite3_trace() is invoked at
** various times when an SQL statement is being run by [sqlite3_step()].
** ^The sqlite3_trace() callback is invoked with a UTF-8 rendering of the
** SQL statement text as the statement first begins executing.
** ^(Additional sqlite3_trace() callbacks might occur
** as each triggered subprogram is entered.  The callbacks for triggers
** contain a UTF-8 SQL comment that identifies the trigger.)^
**
** The [SQLITE_TRACE_SIZE_LIMIT] compile-time option can be used to limit
** the length of [bound parameter] expansion in the output of sqlite3_trace().
**
** ^The callback function registered by sqlite3_profile() is invoked
** as each SQL statement finishes.  ^The profile callback contains
** the original statement text and an estimate of wall-clock time
** of how long that statement took to run.  ^The profile callback
** time is in units of nanoseconds, however the current implementation
** is only capable of millisecond resolution so the six least significant
** digits in the time are meaningless.  Future versions of SQLite
** might provide greater resolution on the profiler callback.  Invoking
** either [sqlite3_trace()] or [sqlite3_trace_v2()] will cancel the
** profile callback.
*/
SQLITE_API SQLITE_DEPRECATED void *sqlite3_trace(sqlite3*,
   void(*xTrace)(void*,const char*), void*);
SQLITE_API SQLITE_DEPRECATED void *sqlite3_profile(sqlite3*,
   void(*xProfile)(void*,const char*,sqlite3_uint64), void*);

/*
** CAPI3REF: SQL Trace Event Codes
** KEYWORDS: SQLITE_TRACE
**
** These constants identify classes of events that can be monitored
** using the [sqlite3_trace_v2()] tracing logic.  The M argument
** to [sqlite3_trace_v2(D,M,X,P)] is an OR-ed combination of one or more of
** the following constants.  ^The first argument to the trace callback
** is one of the following constants.
**
** New tracing constants may be added in future releases.
**
** ^A trace callback has four arguments: xCallback(T,C,P,X).
** ^The T argument is one of the integer type codes above.
** ^The C argument is a copy of the context pointer passed in as the
** fourth argument to [sqlite3_trace_v2()].
** The P and X arguments are pointers whose meanings depend on T.
**
** <dl>
** [[SQLITE_TRACE_STMT]] <dt>SQLITE_TRACE_STMT</dt>
** <dd>^An SQLITE_TRACE_STMT callback is invoked when a prepared statement
** first begins running and possibly at other times during the
** execution of the prepared statement, such as at the start of each
** trigger subprogram. ^The P argument is a pointer to the
** [prepared statement]. ^The X argument is a pointer to a string which
** is the unexpanded SQL text of the prepared statement or an SQL comment
** that indicates the invocation of a trigger.  ^The callback can compute
** the same text that would have been returned by the legacy [sqlite3_trace()]
** interface by using the X argument when X begins with "--" and invoking
** [sqlite3_expanded_sql(P)] otherwise.
**
** [[SQLITE_TRACE_PROFILE]] <dt>SQLITE_TRACE_PROFILE</dt>
** <dd>^An SQLITE_TRACE_PROFILE callback provides approximately the same
** information as is provided by the [sqlite3_profile()] callback.
** ^The P argument is a pointer to the [prepared statement] and the
** X argument points to a 64-bit integer which is the estimated of
** the number of nanosecond that the prepared statement took to run.
** ^The SQLITE_TRACE_PROFILE callback is invoked when the statement finishes.
**
** [[SQLITE_TRACE_ROW]] <dt>SQLITE_TRACE_ROW</dt>
** <dd>^An SQLITE_TRACE_ROW callback is invoked whenever a prepared
** statement generates a single row of result.
** ^The P argument is a pointer to the [prepared statement] and the
** X argument is unused.
**
** [[SQLITE_TRACE_CLOSE]] <dt>SQLITE_TRACE_CLOSE</dt>
** <dd>^An SQLITE_TRACE_CLOSE callback is invoked when a database
** connection closes.
** ^The P argument is a pointer to the [database connection] object
** and the X argument is unused.
** </dl>
*/
#define SQLITE_TRACE_STMT       0x01
#define SQLITE_TRACE_PROFILE    0x02
#define SQLITE_TRACE_ROW        0x04
#define SQLITE_TRACE_CLOSE      0x08

/*
** CAPI3REF: SQL Trace Hook
** METHOD: sqlite3
**
** ^The sqlite3_trace_v2(D,M,X,P) interface registers a trace callback
** function X against [database connection] D, using property mask M
** and context pointer P.  ^If the X callback is
** NULL or if the M mask is zero, then tracing is disabled.  The
** M argument should be the bitwise OR-ed combination of
** zero or more [SQLITE_TRACE] constants.
**
** ^Each call to either sqlite3_trace() or sqlite3_trace_v2() overrides
** (cancels) any prior calls to sqlite3_trace() or sqlite3_trace_v2().
**
** ^The X callback is invoked whenever any of the events identified by
** mask M occur.  ^The integer return value from the callback is currently
** ignored, though this may change in future releases.  Callback
** implementations should return zero to ensure future compatibility.
**
** ^A trace callback is invoked with four arguments: callback(T,C,P,X).
** ^The T argument is one of the [SQLITE_TRACE]
** constants to indicate why the callback was invoked.
** ^The C argument is a copy of the context pointer.
** The P and X arguments are pointers whose meanings depend on T.
**
** The sqlite3_trace_v2() interface is intended to replace the legacy
** interfaces [sqlite3_trace()] and [sqlite3_profile()], both of which
** are deprecated.
*/
SQLITE_API int sqlite3_trace_v2(
  sqlite3*,
  unsigned uMask,
  int(*xCallback)(unsigned,void*,void*,void*),
  void *pCtx
);

/*
** CAPI3REF: Query Progress Callbacks
** METHOD: sqlite3
**
** ^The sqlite3_progress_handler(D,N,X,P) interface causes the callback
** function X to be invoked periodically during long running calls to
** [sqlite3_exec()], [sqlite3_step()] and [sqlite3_get_table()] for
** database connection D.  An example use for this
** interface is to keep a GUI updated during a large query.
**
** ^The parameter P is passed through as the only parameter to the
** callback function X.  ^The parameter N is the approximate number of
** [virtual machine instructions] that are evaluated between successive
** invocations of the callback X.  ^If N is less than one then the progress
** handler is disabled.
**
** ^Only a single progress handler may be defined at one time per
** [database connection]; setting a new progress handler cancels the
** old one.  ^Setting parameter X to NULL disables the progress handler.
** ^The progress handler is also disabled by setting N to a value less
** than 1.
**
** ^If the progress callback returns non-zero, the operation is
** interrupted.  This feature can be used to implement a
** "Cancel" button on a GUI progress dialog box.
**
** The progress handler callback must not do anything that will modify
** the database connection that invoked the progress handler.
** Note that [sqlite3_prepare_v2()] and [sqlite3_step()] both modify their
** database connections for the meaning of "modify" in this paragraph.
**
*/
SQLITE_API void sqlite3_progress_handler(sqlite3*, int, int(*)(void*), void*);

/*
** CAPI3REF: Opening A New Database Connection
** CONSTRUCTOR: sqlite3
**
** ^These routines open an SQLite database file as specified by the
** filename argument. ^The filename argument is interpreted as UTF-8 for
** sqlite3_open() and sqlite3_open_v2() and as UTF-16 in the native byte
** order for sqlite3_open16(). ^(A [database connection] handle is usually
** returned in *ppDb, even if an error occurs.  The only exception is that
** if SQLite is unable to allocate memory to hold the [sqlite3] object,
** a NULL will be written into *ppDb instead of a pointer to the [sqlite3]
** object.)^ ^(If the database is opened (and/or created) successfully, then
** [SQLITE_OK] is returned.  Otherwise an [error code] is returned.)^ ^The
** [sqlite3_errmsg()] or [sqlite3_errmsg16()] routines can be used to obtain
** an English language description of the error following a failure of any
** of the sqlite3_open() routines.
**
** ^The default encoding will be UTF-8 for databases created using
** sqlite3_open() or sqlite3_open_v2().  ^The default encoding for databases
** created using sqlite3_open16() will be UTF-16 in the native byte order.
**
** Whether or not an error occurs when it is opened, resources
** associated with the [database connection] handle should be released by
** passing it to [sqlite3_close()] when it is no longer required.
**
** The sqlite3_open_v2() interface works like sqlite3_open()
** except that it accepts two additional parameters for additional control
** over the new database connection.  ^(The flags parameter to
** sqlite3_open_v2() must include, at a minimum, one of the following
** three flag combinations:)^
**
** <dl>
** ^(<dt>[SQLITE_OPEN_READONLY]</dt>
** <dd>The database is opened in read-only mode.  If the database does not
** already exist, an error is returned.</dd>)^
**
** ^(<dt>[SQLITE_OPEN_READWRITE]</dt>
** <dd>The database is opened for reading and writing if possible, or reading
** only if the file is write protected by the operating system.  In either
** case the database must already exist, otherwise an error is returned.</dd>)^
**
** ^(<dt>[SQLITE_OPEN_READWRITE] | [SQLITE_OPEN_CREATE]</dt>
** <dd>The database is opened for reading and writing, and is created if
** it does not already exist. This is the behavior that is always used for
** sqlite3_open() and sqlite3_open16().</dd>)^
** </dl>
**
** In addition to the required flags, the following optional flags are
** also supported:
**
** <dl>
** ^(<dt>[SQLITE_OPEN_URI]</dt>
** <dd>The filename can be interpreted as a URI if this flag is set.</dd>)^
**
** ^(<dt>[SQLITE_OPEN_MEMORY]</dt>
** <dd>The database will be opened as an in-memory database.  The database
** is named by the "filename" argument for the purposes of cache-sharing,
** if shared cache mode is enabled, but the "filename" is otherwise ignored.
** </dd>)^
**
** ^(<dt>[SQLITE_OPEN_NOMUTEX]</dt>
** <dd>The new database connection will use the "multi-thread"
** [threading mode].)^  This means that separate threads are allowed
** to use SQLite at the same time, as long as each thread is using
** a different [database connection].
**
** ^(<dt>[SQLITE_OPEN_FULLMUTEX]</dt>
** <dd>The new database connection will use the "serialized"
** [threading mode].)^  This means the multiple threads can safely
** attempt to use the same database connection at the same time.
** (Mutexes will block any actual concurrency, but in this mode
** there is no harm in trying.)
**
** ^(<dt>[SQLITE_OPEN_SHAREDCACHE]</dt>
** <dd>The database is opened [shared cache] enabled, overriding
** the default shared cache setting provided by
** [sqlite3_enable_shared_cache()].)^
**
** ^(<dt>[SQLITE_OPEN_PRIVATECACHE]</dt>
** <dd>The database is opened [shared cache] disabled, overriding
** the default shared cache setting provided by
** [sqlite3_enable_shared_cache()].)^
**
** [[OPEN_EXRESCODE]] ^(<dt>[SQLITE_OPEN_EXRESCODE]</dt>
** <dd>The database connection comes up in "extended result code mode".
** In other words, the database behaves has if
** [sqlite3_extended_result_codes(db,1)] where called on the database
** connection as soon as the connection is created. In addition to setting
** the extended result code mode, this flag also causes [sqlite3_open_v2()]
** to return an extended result code.</dd>
**
** [[OPEN_NOFOLLOW]] ^(<dt>[SQLITE_OPEN_NOFOLLOW]</dt>
** <dd>The database filename is not allowed to be a symbolic link</dd>
** </dl>)^
**
** If the 3rd parameter to sqlite3_open_v2() is not one of the
** required combinations shown above optionally combined with other
** [SQLITE_OPEN_READONLY | SQLITE_OPEN_* bits]
** then the behavior is undefined.  Historic versions of SQLite
** have silently ignored surplus bits in the flags parameter to
** sqlite3_open_v2(), however that behavior might not be carried through
** into future versions of SQLite and so applications should not rely
** upon it.  Note in particular that the SQLITE_OPEN_EXCLUSIVE flag is a no-op
** for sqlite3_open_v2().  The SQLITE_OPEN_EXCLUSIVE does *not* cause
** the open to fail if the database already exists.  The SQLITE_OPEN_EXCLUSIVE
** flag is intended for use by the [sqlite3_vfs|VFS interface] only, and not
** by sqlite3_open_v2().
**
** ^The fourth parameter to sqlite3_open_v2() is the name of the
** [sqlite3_vfs] object that defines the operating system interface that
** the new database connection should use.  ^If the fourth parameter is
** a NULL pointer then the default [sqlite3_vfs] object is used.
**
** ^If the filename is ":memory:", then a private, temporary in-memory database
** is created for the connection.  ^This in-memory database will vanish when
** the database connection is closed.  Future versions of SQLite might
** make use of additional special filenames that begin with the ":" character.
** It is recommended that when a database filename actually does begin with
** a ":" character you should prefix the filename with a pathname such as
** "./" to avoid ambiguity.
**
** ^If the filename is an empty string, then a private, temporary
** on-disk database will be created.  ^This private database will be
** automatically deleted as soon as the database connection is closed.
**
** [[URI filenames in sqlite3_open()]] <h3>URI Filenames</h3>
**
** ^If [URI filename] interpretation is enabled, and the filename argument
** begins with "file:", then the filename is interpreted as a URI. ^URI
** filename interpretation is enabled if the [SQLITE_OPEN_URI] flag is
** set in the third argument to sqlite3_open_v2(), or if it has
** been enabled globally using the [SQLITE_CONFIG_URI] option with the
** [sqlite3_config()] method or by the [SQLITE_USE_URI] compile-time option.
** URI filename interpretation is turned off
** by default, but future releases of SQLite might enable URI filename
** interpretation by default.  See "[URI filenames]" for additional
** information.
**
** URI filenames are parsed according to RFC 3986. ^If the URI contains an
** authority, then it must be either an empty string or the string
** "localhost". ^If the authority is not an empty string or "localhost", an
** error is returned to the caller. ^The fragment component of a URI, if
** present, is ignored.
**
** ^SQLite uses the path component of the URI as the name of the disk file
** which contains the database. ^If the path begins with a '/' character,
** then it is interpreted as an absolute path. ^If the path does not begin
** with a '/' (meaning that the authority section is omitted from the URI)
** then the path is interpreted as a relative path.
** ^(On windows, the first component of an absolute path
** is a drive specification (e.g. "C:").)^
**
** [[core URI query parameters]]
** The query component of a URI may contain parameters that are interpreted
** either by SQLite itself, or by a [VFS | custom VFS implementation].
** SQLite and its built-in [VFSes] interpret the
** following query parameters:
**
** <ul>
**   <li> <b>vfs</b>: ^The "vfs" parameter may be used to specify the name of
**     a VFS object that provides the operating system interface that should
**     be used to access the database file on disk. ^If this option is set to
**     an empty string the default VFS object is used. ^Specifying an unknown
**     VFS is an error. ^If sqlite3_open_v2() is used and the vfs option is
**     present, then the VFS specified by the option takes precedence over
**     the value passed as the fourth parameter to sqlite3_open_v2().
**
**   <li> <b>mode</b>: ^(The mode parameter may be set to either "ro", "rw",
**     "rwc", or "memory". Attempting to set it to any other value is
**     an error)^.
**     ^If "ro" is specified, then the database is opened for read-only
**     access, just as if the [SQLITE_OPEN_READONLY] flag had been set in the
**     third argument to sqlite3_open_v2(). ^If the mode option is set to
**     "rw", then the database is opened for read-write (but not create)
**     access, as if SQLITE_OPEN_READWRITE (but not SQLITE_OPEN_CREATE) had
**     been set. ^Value "rwc" is equivalent to setting both
**     SQLITE_OPEN_READWRITE and SQLITE_OPEN_CREATE.  ^If the mode option is
**     set to "memory" then a pure [in-memory database] that never reads
**     or writes from disk is used. ^It is an error to specify a value for
**     the mode parameter that is less restrictive than that specified by
**     the flags passed in the third parameter to sqlite3_open_v2().
**
**   <li> <b>cache</b>: ^The cache parameter may be set to either "shared" or
**     "private". ^Setting it to "shared" is equivalent to setting the
**     SQLITE_OPEN_SHAREDCACHE bit in the flags argument passed to
**     sqlite3_open_v2(). ^Setting the cache parameter to "private" is
**     equivalent to setting the SQLITE_OPEN_PRIVATECACHE bit.
**     ^If sqlite3_open_v2() is used and the "cache" parameter is present in
**     a URI filename, its value overrides any behavior requested by setting
**     SQLITE_OPEN_PRIVATECACHE or SQLITE_OPEN_SHAREDCACHE flag.
**
**  <li> <b>psow</b>: ^The psow parameter indicates whether or not the
**     [powersafe overwrite] property does or does not apply to the
**     storage media on which the database file resides.
**
**  <li> <b>nolock</b>: ^The nolock parameter is a boolean query parameter
**     which if set disables file locking in rollback journal modes.  This
**     is useful for accessing a database on a filesystem that does not
**     support locking.  Caution:  Database corruption might result if two
**     or more processes write to the same database and any one of those
**     processes uses nolock=1.
**
**  <li> <b>immutable</b>: ^The immutable parameter is a boolean query
**     parameter that indicates that the database file is stored on
**     read-only media.  ^When immutable is set, SQLite assumes that the
**     database file cannot be changed, even by a process with higher
**     privilege, and so the database is opened read-only and all locking
**     and change detection is disabled.  Caution: Setting the immutable
**     property on a database file that does in fact change can result
**     in incorrect query results and/or [SQLITE_CORRUPT] errors.
**     See also: [SQLITE_IOCAP_IMMUTABLE].
**
** </ul>
**
** ^Specifying an unknown parameter in the query component of a URI is not an
** error.  Future versions of SQLite might understand additional query
** parameters.  See "[query parameters with special meaning to SQLite]" for
** additional information.
**
** [[URI filename examples]] <h3>URI filename examples</h3>
**
** <table border="1" align=center cellpadding=5>
** <tr><th> URI filenames <th> Results
** <tr><td> file:data.db <td>
**          Open the file "data.db" in the current directory.
** <tr><td> file:/home/fred/data.db<br>
**          file:///home/fred/data.db <br>
**          file://localhost/home/fred/data.db <br> <td>
**          Open the database file "/home/fred/data.db".
** <tr><td> file://darkstar/home/fred/data.db <td>
**          An error. "darkstar" is not a recognized authority.
** <tr><td style="white-space:nowrap">
**          file:///C:/Documents%20and%20Settings/fred/Desktop/data.db
**     <td> Windows only: Open the file "data.db" on fred's desktop on drive
**          C:. Note that the %20 escaping in this example is not strictly
**          necessary - space characters can be used literally
**          in URI filenames.
** <tr><td> file:data.db?mode=ro&cache=private <td>
**          Open file "data.db" in the current directory for read-only access.
**          Regardless of whether or not shared-cache mode is enabled by
**          default, use a private cache.
** <tr><td> file:/home/fred/data.db?vfs=unix-dotfile <td>
**          Open file "/home/fred/data.db". Use the special VFS "unix-dotfile"
**          that uses dot-files in place of posix advisory locking.
** <tr><td> file:data.db?mode=readonly <td>
**          An error. "readonly" is not a valid option for the "mode" parameter.
**          Use "ro" instead:  "file:data.db?mode=ro".
** </table>
**
** ^URI hexadecimal escape sequences (%HH) are supported within the path and
** query components of a URI. A hexadecimal escape sequence consists of a
** percent sign - "%" - followed by exactly two hexadecimal digits
** specifying an octet value. ^Before the path or query components of a
** URI filename are interpreted, they are encoded using UTF-8 and all
** hexadecimal escape sequences replaced by a single byte containing the
** corresponding octet. If this process generates an invalid UTF-8 encoding,
** the results are undefined.
**
** <b>Note to Windows users:</b>  The encoding used for the filename argument
** of sqlite3_open() and sqlite3_open_v2() must be UTF-8, not whatever
** codepage is currently defined.  Filenames containing international
** characters must be converted to UTF-8 prior to passing them into
** sqlite3_open() or sqlite3_open_v2().
**
** <b>Note to Windows Runtime users:</b>  The temporary directory must be set
** prior to calling sqlite3_open() or sqlite3_open_v2().  Otherwise, various
** features that require the use of temporary files may fail.
**
** See also: [sqlite3_temp_directory]
*/
SQLITE_API int sqlite3_open(
  const char *filename,   /* Database filename (UTF-8) */
  sqlite3 **ppDb          /* OUT: SQLite db handle */
);
SQLITE_API int sqlite3_open16(
  const void *filename,   /* Database filename (UTF-16) */
  sqlite3 **ppDb          /* OUT: SQLite db handle */
);
SQLITE_API int sqlite3_open_v2(
  const char *filename,   /* Database filename (UTF-8) */
  sqlite3 **ppDb,         /* OUT: SQLite db handle */
  int flags,              /* Flags */
  const char *zVfs        /* Name of VFS module to use */
);

/*
** CAPI3REF: Obtain Values For URI Parameters
**
** These are utility routines, useful to [VFS|custom VFS implementations],
** that check if a database file was a URI that contained a specific query
** parameter, and if so obtains the value of that query parameter.
**
** The first parameter to these interfaces (hereafter referred to
** as F) must be one of:
** <ul>
** <li> A database filename pointer created by the SQLite core and
** passed into the xOpen() method of a VFS implemention, or
** <li> A filename obtained from [sqlite3_db_filename()], or
** <li> A new filename constructed using [sqlite3_create_filename()].
** </ul>
** If the F parameter is not one of the above, then the behavior is
** undefined and probably undesirable.  Older versions of SQLite were
** more tolerant of invalid F parameters than newer versions.
**
** If F is a suitable filename (as described in the previous paragraph)
** and if P is the name of the query parameter, then
** sqlite3_uri_parameter(F,P) returns the value of the P
** parameter if it exists or a NULL pointer if P does not appear as a
** query parameter on F.  If P is a query parameter of F and it
** has no explicit value, then sqlite3_uri_parameter(F,P) returns
** a pointer to an empty string.
**
** The sqlite3_uri_boolean(F,P,B) routine assumes that P is a boolean
** parameter and returns true (1) or false (0) according to the value
** of P.  The sqlite3_uri_boolean(F,P,B) routine returns true (1) if the
** value of query parameter P is one of "yes", "true", or "on" in any
** case or if the value begins with a non-zero number.  The
** sqlite3_uri_boolean(F,P,B) routines returns false (0) if the value of
** query parameter P is one of "no", "false", or "off" in any case or
** if the value begins with a numeric zero.  If P is not a query
** parameter on F or if the value of P does not match any of the
** above, then sqlite3_uri_boolean(F,P,B) returns (B!=0).
**
** The sqlite3_uri_int64(F,P,D) routine converts the value of P into a
** 64-bit signed integer and returns that integer, or D if P does not
** exist.  If the value of P is something other than an integer, then
** zero is returned.
**
** The sqlite3_uri_key(F,N) returns a pointer to the name (not
** the value) of the N-th query parameter for filename F, or a NULL
** pointer if N is less than zero or greater than the number of query
** parameters minus 1.  The N value is zero-based so N should be 0 to obtain
** the name of the first query parameter, 1 for the second parameter, and
** so forth.
**
** If F is a NULL pointer, then sqlite3_uri_parameter(F,P) returns NULL and
** sqlite3_uri_boolean(F,P,B) returns B.  If F is not a NULL pointer and
** is not a database file pathname pointer that the SQLite core passed
** into the xOpen VFS method, then the behavior of this routine is undefined
** and probably undesirable.
**
** Beginning with SQLite [version 3.31.0] ([dateof:3.31.0]) the input F
** parameter can also be the name of a rollback journal file or WAL file
** in addition to the main database file.  Prior to version 3.31.0, these
** routines would only work if F was the name of the main database file.
** When the F parameter is the name of the rollback journal or WAL file,
** it has access to all the same query parameters as were found on the
** main database file.
**
** See the [URI filename] documentation for additional information.
*/
SQLITE_API const char *sqlite3_uri_parameter(const char *zFilename, const char *zParam);
SQLITE_API int sqlite3_uri_boolean(const char *zFile, const char *zParam, int bDefault);
SQLITE_API sqlite3_int64 sqlite3_uri_int64(const char*, const char*, sqlite3_int64);
SQLITE_API const char *sqlite3_uri_key(const char *zFilename, int N);

/*
** CAPI3REF:  Translate filenames
**
** These routines are available to [VFS|custom VFS implementations] for
** translating filenames between the main database file, the journal file,
** and the WAL file.
**
** If F is the name of an sqlite database file, journal file, or WAL file
** passed by the SQLite core into the VFS, then sqlite3_filename_database(F)
** returns the name of the corresponding database file.
**
** If F is the name of an sqlite database file, journal file, or WAL file
** passed by the SQLite core into the VFS, or if F is a database filename
** obtained from [sqlite3_db_filename()], then sqlite3_filename_journal(F)
** returns the name of the corresponding rollback journal file.
**
** If F is the name of an sqlite database file, journal file, or WAL file
** that was passed by the SQLite core into the VFS, or if F is a database
** filename obtained from [sqlite3_db_filename()], then
** sqlite3_filename_wal(F) returns the name of the corresponding
** WAL file.
**
** In all of the above, if F is not the name of a database, journal or WAL
** filename passed into the VFS from the SQLite core and F is not the
** return value from [sqlite3_db_filename()], then the result is
** undefined and is likely a memory access violation.
*/
SQLITE_API const char *sqlite3_filename_database(const char*);
SQLITE_API const char *sqlite3_filename_journal(const char*);
SQLITE_API const char *sqlite3_filename_wal(const char*);

/*
** CAPI3REF:  Database File Corresponding To A Journal
**
** ^If X is the name of a rollback or WAL-mode journal file that is
** passed into the xOpen method of [sqlite3_vfs], then
** sqlite3_database_file_object(X) returns a pointer to the [sqlite3_file]
** object that represents the main database file.
**
** This routine is intended for use in custom [VFS] implementations
** only.  It is not a general-purpose interface.
** The argument sqlite3_file_object(X) must be a filename pointer that
** has been passed into [sqlite3_vfs].xOpen method where the
** flags parameter to xOpen contains one of the bits
** [SQLITE_OPEN_MAIN_JOURNAL] or [SQLITE_OPEN_WAL].  Any other use
** of this routine results in undefined and probably undesirable
** behavior.
*/
SQLITE_API sqlite3_file *sqlite3_database_file_object(const char*);

/*
** CAPI3REF: Create and Destroy VFS Filenames
**
** These interfces are provided for use by [VFS shim] implementations and
** are not useful outside of that context.
**
** The sqlite3_create_filename(D,J,W,N,P) allocates memory to hold a version of
** database filename D with corresponding journal file J and WAL file W and
** with N URI parameters key/values pairs in the array P.  The result from
** sqlite3_create_filename(D,J,W,N,P) is a pointer to a database filename that
** is safe to pass to routines like:
** <ul>
** <li> [sqlite3_uri_parameter()],
** <li> [sqlite3_uri_boolean()],
** <li> [sqlite3_uri_int64()],
** <li> [sqlite3_uri_key()],
** <li> [sqlite3_filename_database()],
** <li> [sqlite3_filename_journal()], or
** <li> [sqlite3_filename_wal()].
** </ul>
** If a memory allocation error occurs, sqlite3_create_filename() might
** return a NULL pointer.  The memory obtained from sqlite3_create_filename(X)
** must be released by a corresponding call to sqlite3_free_filename(Y).
**
** The P parameter in sqlite3_create_filename(D,J,W,N,P) should be an array
** of 2*N pointers to strings.  Each pair of pointers in this array corresponds
** to a key and value for a query parameter.  The P parameter may be a NULL
** pointer if N is zero.  None of the 2*N pointers in the P array may be
** NULL pointers and key pointers should not be empty strings.
** None of the D, J, or W parameters to sqlite3_create_filename(D,J,W,N,P) may
** be NULL pointers, though they can be empty strings.
**
** The sqlite3_free_filename(Y) routine releases a memory allocation
** previously obtained from sqlite3_create_filename().  Invoking
** sqlite3_free_filename(Y) where Y is a NULL pointer is a harmless no-op.
**
** If the Y parameter to sqlite3_free_filename(Y) is anything other
** than a NULL pointer or a pointer previously acquired from
** sqlite3_create_filename(), then bad things such as heap
** corruption or segfaults may occur. The value Y should not be
** used again after sqlite3_free_filename(Y) has been called.  This means
** that if the [sqlite3_vfs.xOpen()] method of a VFS has been called using Y,
** then the corresponding [sqlite3_module.xClose() method should also be
** invoked prior to calling sqlite3_free_filename(Y).
*/
SQLITE_API char *sqlite3_create_filename(
  const char *zDatabase,
  const char *zJournal,
  const char *zWal,
  int nParam,
  const char **azParam
);
SQLITE_API void sqlite3_free_filename(char*);

/*
** CAPI3REF: Error Codes And Messages
** METHOD: sqlite3
**
** ^If the most recent sqlite3_* API call associated with
** [database connection] D failed, then the sqlite3_errcode(D) interface
** returns the numeric [result code] or [extended result code] for that
** API call.
** ^The sqlite3_extended_errcode()
** interface is the same except that it always returns the
** [extended result code] even when extended result codes are
** disabled.
**
** The values returned by sqlite3_errcode() and/or
** sqlite3_extended_errcode() might change with each API call.
** Except, there are some interfaces that are guaranteed to never
** change the value of the error code.  The error-code preserving
** interfaces include the following:
**
** <ul>
** <li> sqlite3_errcode()
** <li> sqlite3_extended_errcode()
** <li> sqlite3_errmsg()
** <li> sqlite3_errmsg16()
** <li> sqlite3_error_offset()
** </ul>
**
** ^The sqlite3_errmsg() and sqlite3_errmsg16() return English-language
** text that describes the error, as either UTF-8 or UTF-16 respectively.
** ^(Memory to hold the error message string is managed internally.
** The application does not need to worry about freeing the result.
** However, the error string might be overwritten or deallocated by
** subsequent calls to other SQLite interface functions.)^
**
** ^The sqlite3_errstr() interface returns the English-language text
** that describes the [result code], as UTF-8.
** ^(Memory to hold the error message string is managed internally
** and must not be freed by the application)^.
**
** ^If the most recent error references a specific token in the input
** SQL, the sqlite3_error_offset() interface returns the byte offset
** of the start of that token.  ^The byte offset returned by
** sqlite3_error_offset() assumes that the input SQL is UTF8.
** ^If the most recent error does not reference a specific token in the input
** SQL, then the sqlite3_error_offset() function returns -1.
**
** When the serialized [threading mode] is in use, it might be the
** case that a second error occurs on a separate thread in between
** the time of the first error and the call to these interfaces.
** When that happens, the second error will be reported since these
** interfaces always report the most recent result.  To avoid
** this, each thread can obtain exclusive use of the [database connection] D
** by invoking [sqlite3_mutex_enter]([sqlite3_db_mutex](D)) before beginning
** to use D and invoking [sqlite3_mutex_leave]([sqlite3_db_mutex](D)) after
** all calls to the interfaces listed here are completed.
**
** If an interface fails with SQLITE_MISUSE, that means the interface
** was invoked incorrectly by the application.  In that case, the
** error code and message may or may not be set.
*/
SQLITE_API int sqlite3_errcode(sqlite3 *db);
SQLITE_API int sqlite3_extended_errcode(sqlite3 *db);
SQLITE_API const char *sqlite3_errmsg(sqlite3*);
SQLITE_API const void *sqlite3_errmsg16(sqlite3*);
SQLITE_API const char *sqlite3_errstr(int);
SQLITE_API int sqlite3_error_offset(sqlite3 *db);

/*
** CAPI3REF: Prepared Statement Object
** KEYWORDS: {prepared statement} {prepared statements}
**
** An instance of this object represents a single SQL statement that
** has been compiled into binary form and is ready to be evaluated.
**
** Think of each SQL statement as a separate computer program.  The
** original SQL text is source code.  A prepared statement object
** is the compiled object code.  All SQL must be converted into a
** prepared statement before it can be run.
**
** The life-cycle of a prepared statement object usually goes like this:
**
** <ol>
** <li> Create the prepared statement object using [sqlite3_prepare_v2()].
** <li> Bind values to [parameters] using the sqlite3_bind_*()
**      interfaces.
** <li> Run the SQL by calling [sqlite3_step()] one or more times.
** <li> Reset the prepared statement using [sqlite3_reset()] then go back
**      to step 2.  Do this zero or more times.
** <li> Destroy the object using [sqlite3_finalize()].
** </ol>
*/
typedef struct sqlite3_stmt sqlite3_stmt;

/*
** CAPI3REF: Run-time Limits
** METHOD: sqlite3
**
** ^(This interface allows the size of various constructs to be limited
** on a connection by connection basis.  The first parameter is the
** [database connection] whose limit is to be set or queried.  The
** second parameter is one of the [limit categories] that define a
** class of constructs to be size limited.  The third parameter is the
** new limit for that construct.)^
**
** ^If the new limit is a negative number, the limit is unchanged.
** ^(For each limit category SQLITE_LIMIT_<i>NAME</i> there is a
** [limits | hard upper bound]
** set at compile-time by a C preprocessor macro called
** [limits | SQLITE_MAX_<i>NAME</i>].
** (The "_LIMIT_" in the name is changed to "_MAX_".))^
** ^Attempts to increase a limit above its hard upper bound are
** silently truncated to the hard upper bound.
**
** ^Regardless of whether or not the limit was changed, the
** [sqlite3_limit()] interface returns the prior value of the limit.
** ^Hence, to find the current value of a limit without changing it,
** simply invoke this interface with the third parameter set to -1.
**
** Run-time limits are intended for use in applications that manage
** both their own internal database and also databases that are controlled
** by untrusted external sources.  An example application might be a
** web browser that has its own databases for storing history and
** separate databases controlled by JavaScript applications downloaded
** off the Internet.  The internal databases can be given the
** large, default limits.  Databases managed by external sources can
** be given much smaller limits designed to prevent a denial of service
** attack.  Developers might also want to use the [sqlite3_set_authorizer()]
** interface to further control untrusted SQL.  The size of the database
** created by an untrusted script can be contained using the
** [max_page_count] [PRAGMA].
**
** New run-time limit categories may be added in future releases.
*/
SQLITE_API int sqlite3_limit(sqlite3*, int id, int newVal);

/*
** CAPI3REF: Run-Time Limit Categories
** KEYWORDS: {limit category} {*limit categories}
**
** These constants define various performance limits
** that can be lowered at run-time using [sqlite3_limit()].
** The synopsis of the meanings of the various limits is shown below.
** Additional information is available at [limits | Limits in SQLite].
**
** <dl>
** [[SQLITE_LIMIT_LENGTH]] ^(<dt>SQLITE_LIMIT_LENGTH</dt>
** <dd>The maximum size of any string or BLOB or table row, in bytes.<dd>)^
**
** [[SQLITE_LIMIT_SQL_LENGTH]] ^(<dt>SQLITE_LIMIT_SQL_LENGTH</dt>
** <dd>The maximum length of an SQL statement, in bytes.</dd>)^
**
** [[SQLITE_LIMIT_COLUMN]] ^(<dt>SQLITE_LIMIT_COLUMN</dt>
** <dd>The maximum number of columns in a table definition or in the
** result set of a [SELECT] or the maximum number of columns in an index
** or in an ORDER BY or GROUP BY clause.</dd>)^
**
** [[SQLITE_LIMIT_EXPR_DEPTH]] ^(<dt>SQLITE_LIMIT_EXPR_DEPTH</dt>
** <dd>The maximum depth of the parse tree on any expression.</dd>)^
**
** [[SQLITE_LIMIT_COMPOUND_SELECT]] ^(<dt>SQLITE_LIMIT_COMPOUND_SELECT</dt>
** <dd>The maximum number of terms in a compound SELECT statement.</dd>)^
**
** [[SQLITE_LIMIT_VDBE_OP]] ^(<dt>SQLITE_LIMIT_VDBE_OP</dt>
** <dd>The maximum number of instructions in a virtual machine program
** used to implement an SQL statement.  If [sqlite3_prepare_v2()] or
** the equivalent tries to allocate space for more than this many opcodes
** in a single prepared statement, an SQLITE_NOMEM error is returned.</dd>)^
**
** [[SQLITE_LIMIT_FUNCTION_ARG]] ^(<dt>SQLITE_LIMIT_FUNCTION_ARG</dt>
** <dd>The maximum number of arguments on a function.</dd>)^
**
** [[SQLITE_LIMIT_ATTACHED]] ^(<dt>SQLITE_LIMIT_ATTACHED</dt>
** <dd>The maximum number of [ATTACH | attached databases].)^</dd>
**
** [[SQLITE_LIMIT_LIKE_PATTERN_LENGTH]]
** ^(<dt>SQLITE_LIMIT_LIKE_PATTERN_LENGTH</dt>
** <dd>The maximum length of the pattern argument to the [LIKE] or
** [GLOB] operators.</dd>)^
**
** [[SQLITE_LIMIT_VARIABLE_NUMBER]]
** ^(<dt>SQLITE_LIMIT_VARIABLE_NUMBER</dt>
** <dd>The maximum index number of any [parameter] in an SQL statement.)^
**
** [[SQLITE_LIMIT_TRIGGER_DEPTH]] ^(<dt>SQLITE_LIMIT_TRIGGER_DEPTH</dt>
** <dd>The maximum depth of recursion for triggers.</dd>)^
**
** [[SQLITE_LIMIT_WORKER_THREADS]] ^(<dt>SQLITE_LIMIT_WORKER_THREADS</dt>
** <dd>The maximum number of auxiliary worker threads that a single
** [prepared statement] may start.</dd>)^
** </dl>
*/
#define SQLITE_LIMIT_LENGTH                    0
#define SQLITE_LIMIT_SQL_LENGTH                1
#define SQLITE_LIMIT_COLUMN                    2
#define SQLITE_LIMIT_EXPR_DEPTH                3
#define SQLITE_LIMIT_COMPOUND_SELECT           4
#define SQLITE_LIMIT_VDBE_OP                   5
#define SQLITE_LIMIT_FUNCTION_ARG              6
#define SQLITE_LIMIT_ATTACHED                  7
#define SQLITE_LIMIT_LIKE_PATTERN_LENGTH       8
#define SQLITE_LIMIT_VARIABLE_NUMBER           9
#define SQLITE_LIMIT_TRIGGER_DEPTH            10
#define SQLITE_LIMIT_WORKER_THREADS           11

/*
** CAPI3REF: Prepare Flags
**
** These constants define various flags that can be passed into
** "prepFlags" parameter of the [sqlite3_prepare_v3()] and
** [sqlite3_prepare16_v3()] interfaces.
**
** New flags may be added in future releases of SQLite.
**
** <dl>
** [[SQLITE_PREPARE_PERSISTENT]] ^(<dt>SQLITE_PREPARE_PERSISTENT</dt>
** <dd>The SQLITE_PREPARE_PERSISTENT flag is a hint to the query planner
** that the prepared statement will be retained for a long time and
** probably reused many times.)^ ^Without this flag, [sqlite3_prepare_v3()]
** and [sqlite3_prepare16_v3()] assume that the prepared statement will
** be used just once or at most a few times and then destroyed using
** [sqlite3_finalize()] relatively soon. The current implementation acts
** on this hint by avoiding the use of [lookaside memory] so as not to
** deplete the limited store of lookaside memory. Future versions of
** SQLite may act on this hint differently.
**
** [[SQLITE_PREPARE_NORMALIZE]] <dt>SQLITE_PREPARE_NORMALIZE</dt>
** <dd>The SQLITE_PREPARE_NORMALIZE flag is a no-op. This flag used
** to be required for any prepared statement that wanted to use the
** [sqlite3_normalized_sql()] interface.  However, the
** [sqlite3_normalized_sql()] interface is now available to all
** prepared statements, regardless of whether or not they use this
** flag.
**
** [[SQLITE_PREPARE_NO_VTAB]] <dt>SQLITE_PREPARE_NO_VTAB</dt>
** <dd>The SQLITE_PREPARE_NO_VTAB flag causes the SQL compiler
** to return an error (error code SQLITE_ERROR) if the statement uses
** any virtual tables.
** </dl>
*/
#define SQLITE_PREPARE_PERSISTENT              0x01
#define SQLITE_PREPARE_NORMALIZE               0x02
#define SQLITE_PREPARE_NO_VTAB                 0x04

/*
** CAPI3REF: Compiling An SQL Statement
** KEYWORDS: {SQL statement compiler}
** METHOD: sqlite3
** CONSTRUCTOR: sqlite3_stmt
**
** To execute an SQL statement, it must first be compiled into a byte-code
** program using one of these routines.  Or, in other words, these routines
** are constructors for the [prepared statement] object.
**
** The preferred routine to use is [sqlite3_prepare_v2()].  The
** [sqlite3_prepare()] interface is legacy and should be avoided.
** [sqlite3_prepare_v3()] has an extra "prepFlags" option that is used
** for special purposes.
**
** The use of the UTF-8 interfaces is preferred, as SQLite currently
** does all parsing using UTF-8.  The UTF-16 interfaces are provided
** as a convenience.  The UTF-16 interfaces work by converting the
** input text into UTF-8, then invoking the corresponding UTF-8 interface.
**
** The first argument, "db", is a [database connection] obtained from a
** prior successful call to [sqlite3_open()], [sqlite3_open_v2()] or
** [sqlite3_open16()].  The database connection must not have been closed.
**
** The second argument, "zSql", is the statement to be compiled, encoded
** as either UTF-8 or UTF-16.  The sqlite3_prepare(), sqlite3_prepare_v2(),
** and sqlite3_prepare_v3()
** interfaces use UTF-8, and sqlite3_prepare16(), sqlite3_prepare16_v2(),
** and sqlite3_prepare16_v3() use UTF-16.
**
** ^If the nByte argument is negative, then zSql is read up to the
** first zero terminator. ^If nByte is positive, then it is the
** number of bytes read from zSql.  ^If nByte is zero, then no prepared
** statement is generated.
** If the caller knows that the supplied string is nul-terminated, then
** there is a small performance advantage to passing an nByte parameter that
** is the number of bytes in the input string <i>including</i>
** the nul-terminator.
**
** ^If pzTail is not NULL then *pzTail is made to point to the first byte
** past the end of the first SQL statement in zSql.  These routines only
** compile the first statement in zSql, so *pzTail is left pointing to
** what remains uncompiled.
**
** ^*ppStmt is left pointing to a compiled [prepared statement] that can be
** executed using [sqlite3_step()].  ^If there is an error, *ppStmt is set
** to NULL.  ^If the input text contains no SQL (if the input is an empty
** string or a comment) then *ppStmt is set to NULL.
** The calling procedure is responsible for deleting the compiled
** SQL statement using [sqlite3_finalize()] after it has finished with it.
** ppStmt may not be NULL.
**
** ^On success, the sqlite3_prepare() family of routines return [SQLITE_OK];
** otherwise an [error code] is returned.
**
** The sqlite3_prepare_v2(), sqlite3_prepare_v3(), sqlite3_prepare16_v2(),
** and sqlite3_prepare16_v3() interfaces are recommended for all new programs.
** The older interfaces (sqlite3_prepare() and sqlite3_prepare16())
** are retained for backwards compatibility, but their use is discouraged.
** ^In the "vX" interfaces, the prepared statement
** that is returned (the [sqlite3_stmt] object) contains a copy of the
** original SQL text. This causes the [sqlite3_step()] interface to
** behave differently in three ways:
**
** <ol>
** <li>
** ^If the database schema changes, instead of returning [SQLITE_SCHEMA] as it
** always used to do, [sqlite3_step()] will automatically recompile the SQL
** statement and try to run it again. As many as [SQLITE_MAX_SCHEMA_RETRY]
** retries will occur before sqlite3_step() gives up and returns an error.
** </li>
**
** <li>
** ^When an error occurs, [sqlite3_step()] will return one of the detailed
** [error codes] or [extended error codes].  ^The legacy behavior was that
** [sqlite3_step()] would only return a generic [SQLITE_ERROR] result code
** and the application would have to make a second call to [sqlite3_reset()]
** in order to find the underlying cause of the problem. With the "v2" prepare
** interfaces, the underlying reason for the error is returned immediately.
** </li>
**
** <li>
** ^If the specific value bound to a [parameter | host parameter] in the
** WHERE clause might influence the choice of query plan for a statement,
** then the statement will be automatically recompiled, as if there had been
** a schema change, on the first [sqlite3_step()] call following any change
** to the [sqlite3_bind_text | bindings] of that [parameter].
** ^The specific value of a WHERE-clause [parameter] might influence the
** choice of query plan if the parameter is the left-hand side of a [LIKE]
** or [GLOB] operator or if the parameter is compared to an indexed column
** and the [SQLITE_ENABLE_STAT4] compile-time option is enabled.
** </li>
** </ol>
**
** <p>^sqlite3_prepare_v3() differs from sqlite3_prepare_v2() only in having
** the extra prepFlags parameter, which is a bit array consisting of zero or
** more of the [SQLITE_PREPARE_PERSISTENT|SQLITE_PREPARE_*] flags.  ^The
** sqlite3_prepare_v2() interface works exactly the same as
** sqlite3_prepare_v3() with a zero prepFlags parameter.
*/
SQLITE_API int sqlite3_prepare(
  sqlite3 *db,            /* Database handle */
  const char *zSql,       /* SQL statement, UTF-8 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const char **pzTail     /* OUT: Pointer to unused portion of zSql */
);
SQLITE_API int sqlite3_prepare_v2(
  sqlite3 *db,            /* Database handle */
  const char *zSql,       /* SQL statement, UTF-8 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const char **pzTail     /* OUT: Pointer to unused portion of zSql */
);
SQLITE_API int sqlite3_prepare_v3(
  sqlite3 *db,            /* Database handle */
  const char *zSql,       /* SQL statement, UTF-8 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  unsigned int prepFlags, /* Zero or more SQLITE_PREPARE_ flags */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const char **pzTail     /* OUT: Pointer to unused portion of zSql */
);
SQLITE_API int sqlite3_prepare16(
  sqlite3 *db,            /* Database handle */
  const void *zSql,       /* SQL statement, UTF-16 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const void **pzTail     /* OUT: Pointer to unused portion of zSql */
);
SQLITE_API int sqlite3_prepare16_v2(
  sqlite3 *db,            /* Database handle */
  const void *zSql,       /* SQL statement, UTF-16 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const void **pzTail     /* OUT: Pointer to unused portion of zSql */
);
SQLITE_API int sqlite3_prepare16_v3(
  sqlite3 *db,            /* Database handle */
  const void *zSql,       /* SQL statement, UTF-16 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  unsigned int prepFlags, /* Zero or more SQLITE_PREPARE_ flags */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const void **pzTail     /* OUT: Pointer to unused portion of zSql */
);

/*
** CAPI3REF: Retrieving Statement SQL
** METHOD: sqlite3_stmt
**
** ^The sqlite3_sql(P) interface returns a pointer to a copy of the UTF-8
** SQL text used to create [prepared statement] P if P was
** created by [sqlite3_prepare_v2()], [sqlite3_prepare_v3()],
** [sqlite3_prepare16_v2()], or [sqlite3_prepare16_v3()].
** ^The sqlite3_expanded_sql(P) interface returns a pointer to a UTF-8
** string containing the SQL text of prepared statement P with
** [bound parameters] expanded.
** ^The sqlite3_normalized_sql(P) interface returns a pointer to a UTF-8
** string containing the normalized SQL text of prepared statement P.  The
** semantics used to normalize a SQL statement are unspecified and subject
** to change.  At a minimum, literal values will be replaced with suitable
** placeholders.
**
** ^(For example, if a prepared statement is created using the SQL
** text "SELECT $abc,:xyz" and if parameter $abc is bound to integer 2345
** and parameter :xyz is unbound, then sqlite3_sql() will return
** the original string, "SELECT $abc,:xyz" but sqlite3_expanded_sql()
** will return "SELECT 2345,NULL".)^
**
** ^The sqlite3_expanded_sql() interface returns NULL if insufficient memory
** is available to hold the result, or if the result would exceed the
** the maximum string length determined by the [SQLITE_LIMIT_LENGTH].
**
** ^The [SQLITE_TRACE_SIZE_LIMIT] compile-time option limits the size of
** bound parameter expansions.  ^The [SQLITE_OMIT_TRACE] compile-time
** option causes sqlite3_expanded_sql() to always return NULL.
**
** ^The strings returned by sqlite3_sql(P) and sqlite3_normalized_sql(P)
** are managed by SQLite and are automatically freed when the prepared
** statement is finalized.
** ^The string returned by sqlite3_expanded_sql(P), on the other hand,
** is obtained from [sqlite3_malloc()] and must be freed by the application
** by passing it to [sqlite3_free()].
**
** ^The sqlite3_normalized_sql() interface is only available if
** the [SQLITE_ENABLE_NORMALIZE] compile-time option is defined.
*/
SQLITE_API const char *sqlite3_sql(sqlite3_stmt *pStmt);
SQLITE_API char *sqlite3_expanded_sql(sqlite3_stmt *pStmt);
#ifdef SQLITE_ENABLE_NORMALIZE
SQLITE_API const char *sqlite3_normalized_sql(sqlite3_stmt *pStmt);
#endif

/*
** CAPI3REF: Determine If An SQL Statement Writes The Database
** METHOD: sqlite3_stmt
**
** ^The sqlite3_stmt_readonly(X) interface returns true (non-zero) if
** and only if the [prepared statement] X makes no direct changes to
** the content of the database file.
**
** Note that [application-defined SQL functions] or
** [virtual tables] might change the database indirectly as a side effect.
** ^(For example, if an application defines a function "eval()" that
** calls [sqlite3_exec()], then the following SQL statement would
** change the database file through side-effects:
**
** <blockquote><pre>
**    SELECT eval('DELETE FROM t1') FROM t2;
** </pre></blockquote>
**
** But because the [SELECT] statement does not change the database file
** directly, sqlite3_stmt_readonly() would still return true.)^
**
** ^Transaction control statements such as [BEGIN], [COMMIT], [ROLLBACK],
** [SAVEPOINT], and [RELEASE] cause sqlite3_stmt_readonly() to return true,
** since the statements themselves do not actually modify the database but
** rather they control the timing of when other statements modify the
** database.  ^The [ATTACH] and [DETACH] statements also cause
** sqlite3_stmt_readonly() to return true since, while those statements
** change the configuration of a database connection, they do not make
** changes to the content of the database files on disk.
** ^The sqlite3_stmt_readonly() interface returns true for [BEGIN] since
** [BEGIN] merely sets internal flags, but the [BEGIN|BEGIN IMMEDIATE] and
** [BEGIN|BEGIN EXCLUSIVE] commands do touch the database and so
** sqlite3_stmt_readonly() returns false for those commands.
**
** ^This routine returns false if there is any possibility that the
** statement might change the database file.  ^A false return does
** not guarantee that the statement will change the database file.
** ^For example, an UPDATE statement might have a WHERE clause that
** makes it a no-op, but the sqlite3_stmt_readonly() result would still
** be false.  ^Similarly, a CREATE TABLE IF NOT EXISTS statement is a
** read-only no-op if the table already exists, but
** sqlite3_stmt_readonly() still returns false for such a statement.
**
** ^If prepared statement X is an [EXPLAIN] or [EXPLAIN QUERY PLAN]
** statement, then sqlite3_stmt_readonly(X) returns the same value as
** if the EXPLAIN or EXPLAIN QUERY PLAN prefix were omitted.
*/
SQLITE_API int sqlite3_stmt_readonly(sqlite3_stmt *pStmt);

/*
** CAPI3REF: Query The EXPLAIN Setting For A Prepared Statement
** METHOD: sqlite3_stmt
**
** ^The sqlite3_stmt_isexplain(S) interface returns 1 if the
** prepared statement S is an EXPLAIN statement, or 2 if the
** statement S is an EXPLAIN QUERY PLAN.
** ^The sqlite3_stmt_isexplain(S) interface returns 0 if S is
** an ordinary statement or a NULL pointer.
*/
SQLITE_API int sqlite3_stmt_isexplain(sqlite3_stmt *pStmt);

/*
** CAPI3REF: Determine If A Prepared Statement Has Been Reset
** METHOD: sqlite3_stmt
**
** ^The sqlite3_stmt_busy(S) interface returns true (non-zero) if the
** [prepared statement] S has been stepped at least once using
** [sqlite3_step(S)] but has neither run to completion (returned
** [SQLITE_DONE] from [sqlite3_step(S)]) nor
** been reset using [sqlite3_reset(S)].  ^The sqlite3_stmt_busy(S)
** interface returns false if S is a NULL pointer.  If S is not a
** NULL pointer and is not a pointer to a valid [prepared statement]
** object, then the behavior is undefined and probably undesirable.
**
** This interface can be used in combination [sqlite3_next_stmt()]
** to locate all prepared statements associated with a database
** connection that are in need of being reset.  This can be used,
** for example, in diagnostic routines to search for prepared
** statements that are holding a transaction open.
*/
SQLITE_API int sqlite3_stmt_busy(sqlite3_stmt*);

/*
** CAPI3REF: Dynamically Typed Value Object
** KEYWORDS: {protected sqlite3_value} {unprotected sqlite3_value}
**
** SQLite uses the sqlite3_value object to represent all values
** that can be stored in a database table. SQLite uses dynamic typing
** for the values it stores.  ^Values stored in sqlite3_value objects
** can be integers, floating point values, strings, BLOBs, or NULL.
**
** An sqlite3_value object may be either "protected" or "unprotected".
** Some interfaces require a protected sqlite3_value.  Other interfaces
** will accept either a protected or an unprotected sqlite3_value.
** Every interface that accepts sqlite3_value arguments specifies
** whether or not it requires a protected sqlite3_value.  The
** [sqlite3_value_dup()] interface can be used to construct a new
** protected sqlite3_value from an unprotected sqlite3_value.
**
** The terms "protected" and "unprotected" refer to whether or not
** a mutex is held.  An internal mutex is held for a protected
** sqlite3_value object but no mutex is held for an unprotected
** sqlite3_value object.  If SQLite is compiled to be single-threaded
** (with [SQLITE_THREADSAFE=0] and with [sqlite3_threadsafe()] returning 0)
** or if SQLite is run in one of reduced mutex modes
** [SQLITE_CONFIG_SINGLETHREAD] or [SQLITE_CONFIG_MULTITHREAD]
** then there is no distinction between protected and unprotected
** sqlite3_value objects and they can be used interchangeably.  However,
** for maximum code portability it is recommended that applications
** still make the distinction between protected and unprotected
** sqlite3_value objects even when not strictly required.
**
** ^The sqlite3_value objects that are passed as parameters into the
** implementation of [application-defined SQL functions] are protected.
** ^The sqlite3_value objects returned by [sqlite3_vtab_rhs_value()]
** are protected.
** ^The sqlite3_value object returned by
** [sqlite3_column_value()] is unprotected.
** Unprotected sqlite3_value objects may only be used as arguments
** to [sqlite3_result_value()], [sqlite3_bind_value()], and
** [sqlite3_value_dup()].
** The [sqlite3_value_blob | sqlite3_value_type()] family of
** interfaces require protected sqlite3_value objects.
*/
typedef struct sqlite3_value sqlite3_value;

/*
** CAPI3REF: SQL Function Context Object
**
** The context in which an SQL function executes is stored in an
** sqlite3_context object.  ^A pointer to an sqlite3_context object
** is always first parameter to [application-defined SQL functions].
** The application-defined SQL function implementation will pass this
** pointer through into calls to [sqlite3_result_int | sqlite3_result()],
** [sqlite3_aggregate_context()], [sqlite3_user_data()],
** [sqlite3_context_db_handle()], [sqlite3_get_auxdata()],
** and/or [sqlite3_set_auxdata()].
*/
typedef struct sqlite3_context sqlite3_context;

/*
** CAPI3REF: Binding Values To Prepared Statements
** KEYWORDS: {host parameter} {host parameters} {host parameter name}
** KEYWORDS: {SQL parameter} {SQL parameters} {parameter binding}
** METHOD: sqlite3_stmt
**
** ^(In the SQL statement text input to [sqlite3_prepare_v2()] and its variants,
** literals may be replaced by a [parameter] that matches one of following
** templates:
**
** <ul>
** <li>  ?
** <li>  ?NNN
** <li>  :VVV
** <li>  @VVV
** <li>  $VVV
** </ul>
**
** In the templates above, NNN represents an integer literal,
** and VVV represents an alphanumeric identifier.)^  ^The values of these
** parameters (also called "host parameter names" or "SQL parameters")
** can be set using the sqlite3_bind_*() routines defined here.
**
** ^The first argument to the sqlite3_bind_*() routines is always
** a pointer to the [sqlite3_stmt] object returned from
** [sqlite3_prepare_v2()] or its variants.
**
** ^The second argument is the index of the SQL parameter to be set.
** ^The leftmost SQL parameter has an index of 1.  ^When the same named
** SQL parameter is used more than once, second and subsequent
** occurrences have the same index as the first occurrence.
** ^The index for named parameters can be looked up using the
** [sqlite3_bind_parameter_index()] API if desired.  ^The index
** for "?NNN" parameters is the value of NNN.
** ^The NNN value must be between 1 and the [sqlite3_limit()]
** parameter [SQLITE_LIMIT_VARIABLE_NUMBER] (default value: 32766).
**
** ^The third argument is the value to bind to the parameter.
** ^If the third parameter to sqlite3_bind_text() or sqlite3_bind_text16()
** or sqlite3_bind_blob() is a NULL pointer then the fourth parameter
** is ignored and the end result is the same as sqlite3_bind_null().
** ^If the third parameter to sqlite3_bind_text() is not NULL, then
** it should be a pointer to well-formed UTF8 text.
** ^If the third parameter to sqlite3_bind_text16() is not NULL, then
** it should be a pointer to well-formed UTF16 text.
** ^If the third parameter to sqlite3_bind_text64() is not NULL, then
** it should be a pointer to a well-formed unicode string that is
** either UTF8 if the sixth parameter is SQLITE_UTF8, or UTF16
** otherwise.
**
** [[byte-order determination rules]] ^The byte-order of
** UTF16 input text is determined by the byte-order mark (BOM, U+FEFF)
** found in first character, which is removed, or in the absence of a BOM
** the byte order is the native byte order of the host
** machine for sqlite3_bind_text16() or the byte order specified in
** the 6th parameter for sqlite3_bind_text64().)^
** ^If UTF16 input text contains invalid unicode
** characters, then SQLite might change those invalid characters
** into the unicode replacement character: U+FFFD.
**
** ^(In those routines that have a fourth argument, its value is the
** number of bytes in the parameter.  To be clear: the value is the
** number of <u>bytes</u> in the value, not the number of characters.)^
** ^If the fourth parameter to sqlite3_bind_text() or sqlite3_bind_text16()
** is negative, then the length of the string is
** the number of bytes up to the first zero terminator.
** If the fourth parameter to sqlite3_bind_blob() is negative, then
** the behavior is undefined.
** If a non-negative fourth parameter is provided to sqlite3_bind_text()
** or sqlite3_bind_text16() or sqlite3_bind_text64() then
** that parameter must be the byte offset
** where the NUL terminator would occur assuming the string were NUL
** terminated.  If any NUL characters occurs at byte offsets less than
** the value of the fourth parameter then the resulting string value will
** contain embedded NULs.  The result of expressions involving strings
** with embedded NULs is undefined.
**
** ^The fifth argument to the BLOB and string binding interfaces controls
** or indicates the lifetime of the object referenced by the third parameter.
** These three options exist:
** ^ (1) A destructor to dispose of the BLOB or string after SQLite has finished
** with it may be passed. ^It is called to dispose of the BLOB or string even
** if the call to the bind API fails, except the destructor is not called if
** the third parameter is a NULL pointer or the fourth parameter is negative.
** ^ (2) The special constant, [SQLITE_STATIC], may be passsed to indicate that
** the application remains responsible for disposing of the object. ^In this
** case, the object and the provided pointer to it must remain valid until
** either the prepared statement is finalized or the same SQL parameter is
** bound to something else, whichever occurs sooner.
** ^ (3) The constant, [SQLITE_TRANSIENT], may be passed to indicate that the
** object is to be copied prior to the return from sqlite3_bind_*(). ^The
** object and pointer to it must remain valid until then. ^SQLite will then
** manage the lifetime of its private copy.
**
** ^The sixth argument to sqlite3_bind_text64() must be one of
** [SQLITE_UTF8], [SQLITE_UTF16], [SQLITE_UTF16BE], or [SQLITE_UTF16LE]
** to specify the encoding of the text in the third parameter.  If
** the sixth argument to sqlite3_bind_text64() is not one of the
** allowed values shown above, or if the text encoding is different
** from the encoding specified by the sixth parameter, then the behavior
** is undefined.
**
** ^The sqlite3_bind_zeroblob() routine binds a BLOB of length N that
** is filled with zeroes.  ^A zeroblob uses a fixed amount of memory
** (just an integer to hold its size) while it is being processed.
** Zeroblobs are intended to serve as placeholders for BLOBs whose
** content is later written using
** [sqlite3_blob_open | incremental BLOB I/O] routines.
** ^A negative value for the zeroblob results in a zero-length BLOB.
**
** ^The sqlite3_bind_pointer(S,I,P,T,D) routine causes the I-th parameter in
** [prepared statement] S to have an SQL value of NULL, but to also be
** associated with the pointer P of type T.  ^D is either a NULL pointer or
** a pointer to a destructor function for P. ^SQLite will invoke the
** destructor D with a single argument of P when it is finished using
** P.  The T parameter should be a static string, preferably a string
** literal. The sqlite3_bind_pointer() routine is part of the
** [pointer passing interface] added for SQLite 3.20.0.
**
** ^If any of the sqlite3_bind_*() routines are called with a NULL pointer
** for the [prepared statement] or with a prepared statement for which
** [sqlite3_step()] has been called more recently than [sqlite3_reset()],
** then the call will return [SQLITE_MISUSE].  If any sqlite3_bind_()
** routine is passed a [prepared statement] that has been finalized, the
** result is undefined and probably harmful.
**
** ^Bindings are not cleared by the [sqlite3_reset()] routine.
** ^Unbound parameters are interpreted as NULL.
**
** ^The sqlite3_bind_* routines return [SQLITE_OK] on success or an
** [error code] if anything goes wrong.
** ^[SQLITE_TOOBIG] might be returned if the size of a string or BLOB
** exceeds limits imposed by [sqlite3_limit]([SQLITE_LIMIT_LENGTH]) or
** [SQLITE_MAX_LENGTH].
** ^[SQLITE_RANGE] is returned if the parameter
** index is out of range.  ^[SQLITE_NOMEM] is returned if malloc() fails.
**
** See also: [sqlite3_bind_parameter_count()],
** [sqlite3_bind_parameter_name()], and [sqlite3_bind_parameter_index()].
*/
SQLITE_API int sqlite3_bind_blob(sqlite3_stmt*, int, const void*, int n, void(*)(void*));
SQLITE_API int sqlite3_bind_blob64(sqlite3_stmt*, int, const void*, sqlite3_uint64,
                        void(*)(void*));
SQLITE_API int sqlite3_bind_double(sqlite3_stmt*, int, double);
SQLITE_API int sqlite3_bind_int(sqlite3_stmt*, int, int);
SQLITE_API int sqlite3_bind_int64(sqlite3_stmt*, int, sqlite3_int64);
SQLITE_API int sqlite3_bind_null(sqlite3_stmt*, int);
SQLITE_API int sqlite3_bind_text(sqlite3_stmt*,int,const char*,int,void(*)(void*));
SQLITE_API int sqlite3_bind_text16(sqlite3_stmt*, int, const void*, int, void(*)(void*));
SQLITE_API int sqlite3_bind_text64(sqlite3_stmt*, int, const char*, sqlite3_uint64,
                         void(*)(void*), unsigned char encoding);
SQLITE_API int sqlite3_bind_value(sqlite3_stmt*, int, const sqlite3_value*);
SQLITE_API int sqlite3_bind_pointer(sqlite3_stmt*, int, void*, const char*,void(*)(void*));
SQLITE_API int sqlite3_bind_zeroblob(sqlite3_stmt*, int, int n);
SQLITE_API int sqlite3_bind_zeroblob64(sqlite3_stmt*, int, sqlite3_uint64);

/*
** CAPI3REF: Number Of SQL Parameters
** METHOD: sqlite3_stmt
**
** ^This routine can be used to find the number of [SQL parameters]
** in a [prepared statement].  SQL parameters are tokens of the
** form "?", "?NNN", ":AAA", "$AAA", or "@AAA" that serve as
** placeholders for values that are [sqlite3_bind_blob | bound]
** to the parameters at a later time.
**
** ^(This routine actually returns the index of the largest (rightmost)
** parameter. For all forms except ?NNN, this will correspond to the
** number of unique parameters.  If parameters of the ?NNN form are used,
** there may be gaps in the list.)^
**
** See also: [sqlite3_bind_blob|sqlite3_bind()],
** [sqlite3_bind_parameter_name()], and
** [sqlite3_bind_parameter_index()].
*/
SQLITE_API int sqlite3_bind_parameter_count(sqlite3_stmt*);

/*
** CAPI3REF: Name Of A Host Parameter
** METHOD: sqlite3_stmt
**
** ^The sqlite3_bind_parameter_name(P,N) interface returns
** the name of the N-th [SQL parameter] in the [prepared statement] P.
** ^(SQL parameters of the form "?NNN" or ":AAA" or "@AAA" or "$AAA"
** have a name which is the string "?NNN" or ":AAA" or "@AAA" or "$AAA"
** respectively.
** In other words, the initial ":" or "$" or "@" or "?"
** is included as part of the name.)^
** ^Parameters of the form "?" without a following integer have no name
** and are referred to as "nameless" or "anonymous parameters".
**
** ^The first host parameter has an index of 1, not 0.
**
** ^If the value N is out of range or if the N-th parameter is
** nameless, then NULL is returned.  ^The returned string is
** always in UTF-8 encoding even if the named parameter was
** originally specified as UTF-16 in [sqlite3_prepare16()],
** [sqlite3_prepare16_v2()], or [sqlite3_prepare16_v3()].
**
** See also: [sqlite3_bind_blob|sqlite3_bind()],
** [sqlite3_bind_parameter_count()], and
** [sqlite3_bind_parameter_index()].
*/
SQLITE_API const char *sqlite3_bind_parameter_name(sqlite3_stmt*, int);

/*
** CAPI3REF: Index Of A Parameter With A Given Name
** METHOD: sqlite3_stmt
**
** ^Return the index of an SQL parameter given its name.  ^The
** index value returned is suitable for use as the second
** parameter to [sqlite3_bind_blob|sqlite3_bind()].  ^A zero
** is returned if no matching parameter is found.  ^The parameter
** name must be given in UTF-8 even if the original statement
** was prepared from UTF-16 text using [sqlite3_prepare16_v2()] or
** [sqlite3_prepare16_v3()].
**
** See also: [sqlite3_bind_blob|sqlite3_bind()],
** [sqlite3_bind_parameter_count()], and
** [sqlite3_bind_parameter_name()].
*/
SQLITE_API int sqlite3_bind_parameter_index(sqlite3_stmt*, const char *zName);

/*
** CAPI3REF: Reset All Bindings On A Prepared Statement
** METHOD: sqlite3_stmt
**
** ^Contrary to the intuition of many, [sqlite3_reset()] does not reset
** the [sqlite3_bind_blob | bindings] on a [prepared statement].
** ^Use this routine to reset all host parameters to NULL.
*/
SQLITE_API int sqlite3_clear_bindings(sqlite3_stmt*);

/*
** CAPI3REF: Number Of Columns In A Result Set
** METHOD: sqlite3_stmt
**
** ^Return the number of columns in the result set returned by the
** [prepared statement]. ^If this routine returns 0, that means the
** [prepared statement] returns no data (for example an [UPDATE]).
** ^However, just because this routine returns a positive number does not
** mean that one or more rows of data will be returned.  ^A SELECT statement
** will always have a positive sqlite3_column_count() but depending on the
** WHERE clause constraints and the table content, it might return no rows.
**
** See also: [sqlite3_data_count()]
*/
SQLITE_API int sqlite3_column_count(sqlite3_stmt *pStmt);

/*
** CAPI3REF: Column Names In A Result Set
** METHOD: sqlite3_stmt
**
** ^These routines return the name assigned to a particular column
** in the result set of a [SELECT] statement.  ^The sqlite3_column_name()
** interface returns a pointer to a zero-terminated UTF-8 string
** and sqlite3_column_name16() returns a pointer to a zero-terminated
** UTF-16 string.  ^The first parameter is the [prepared statement]
** that implements the [SELECT] statement. ^The second parameter is the
** column number.  ^The leftmost column is number 0.
**
** ^The returned string pointer is valid until either the [prepared statement]
** is destroyed by [sqlite3_finalize()] or until the statement is automatically
** reprepared by the first call to [sqlite3_step()] for a particular run
** or until the next call to
** sqlite3_column_name() or sqlite3_column_name16() on the same column.
**
** ^If sqlite3_malloc() fails during the processing of either routine
** (for example during a conversion from UTF-8 to UTF-16) then a
** NULL pointer is returned.
**
** ^The name of a result column is the value of the "AS" clause for
** that column, if there is an AS clause.  If there is no AS clause
** then the name of the column is unspecified and may change from
** one release of SQLite to the next.
*/
SQLITE_API const char *sqlite3_column_name(sqlite3_stmt*, int N);
SQLITE_API const void *sqlite3_column_name16(sqlite3_stmt*, int N);

/*
** CAPI3REF: Source Of Data In A Query Result
** METHOD: sqlite3_stmt
**
** ^These routines provide a means to determine the database, table, and
** table column that is the origin of a particular result column in
** [SELECT] statement.
** ^The name of the database or table or column can be returned as
** either a UTF-8 or UTF-16 string.  ^The _database_ routines return
** the database name, the _table_ routines return the table name, and
** the origin_ routines return the column name.
** ^The returned string is valid until the [prepared statement] is destroyed
** using [sqlite3_finalize()] or until the statement is automatically
** reprepared by the first call to [sqlite3_step()] for a particular run
** or until the same information is requested
** again in a different encoding.
**
** ^The names returned are the original un-aliased names of the
** database, table, and column.
**
** ^The first argument to these interfaces is a [prepared statement].
** ^These functions return information about the Nth result column returned by
** the statement, where N is the second function argument.
** ^The left-most column is column 0 for these routines.
**
** ^If the Nth column returned by the statement is an expression or
** subquery and is not a column value, then all of these functions return
** NULL.  ^These routines might also return NULL if a memory allocation error
** occurs.  ^Otherwise, they return the name of the attached database, table,
** or column that query result column was extracted from.
**
** ^As with all other SQLite APIs, those whose names end with "16" return
** UTF-16 encoded strings and the other functions return UTF-8.
**
** ^These APIs are only available if the library was compiled with the
** [SQLITE_ENABLE_COLUMN_METADATA] C-preprocessor symbol.
**
** If two or more threads call one or more
** [sqlite3_column_database_name | column metadata interfaces]
** for the same [prepared statement] and result column
** at the same time then the results are undefined.
*/
SQLITE_API const char *sqlite3_column_database_name(sqlite3_stmt*,int);
SQLITE_API const void *sqlite3_column_database_name16(sqlite3_stmt*,int);
SQLITE_API const char *sqlite3_column_table_name(sqlite3_stmt*,int);
SQLITE_API const void *sqlite3_column_table_name16(sqlite3_stmt*,int);
SQLITE_API const char *sqlite3_column_origin_name(sqlite3_stmt*,int);
SQLITE_API const void *sqlite3_column_origin_name16(sqlite3_stmt*,int);

/*
** CAPI3REF: Declared Datatype Of A Query Result
** METHOD: sqlite3_stmt
**
** ^(The first parameter is a [prepared statement].
** If this statement is a [SELECT] statement and the Nth column of the
** returned result set of that [SELECT] is a table column (not an
** expression or subquery) then the declared type of the table
** column is returned.)^  ^If the Nth column of the result set is an
** expression or subquery, then a NULL pointer is returned.
** ^The returned string is always UTF-8 encoded.
**
** ^(For example, given the database schema:
**
** CREATE TABLE t1(c1 VARIANT);
**
** and the following statement to be compiled:
**
** SELECT c1 + 1, c1 FROM t1;
**
** this routine would return the string "VARIANT" for the second result
** column (i==1), and a NULL pointer for the first result column (i==0).)^
**
** ^SQLite uses dynamic run-time typing.  ^So just because a column
** is declared to contain a particular type does not mean that the
** data stored in that column is of the declared type.  SQLite is
** strongly typed, but the typing is dynamic not static.  ^Type
** is associated with individual values, not with the containers
** used to hold those values.
*/
SQLITE_API const char *sqlite3_column_decltype(sqlite3_stmt*,int);
SQLITE_API const void *sqlite3_column_decltype16(sqlite3_stmt*,int);

/*
** CAPI3REF: Evaluate An SQL Statement
** METHOD: sqlite3_stmt
**
** After a [prepared statement] has been prepared using any of
** [sqlite3_prepare_v2()], [sqlite3_prepare_v3()], [sqlite3_prepare16_v2()],
** or [sqlite3_prepare16_v3()] or one of the legacy
** interfaces [sqlite3_prepare()] or [sqlite3_prepare16()], this function
** must be called one or more times to evaluate the statement.
**
** The details of the behavior of the sqlite3_step() interface depend
** on whether the statement was prepared using the newer "vX" interfaces
** [sqlite3_prepare_v3()], [sqlite3_prepare_v2()], [sqlite3_prepare16_v3()],
** [sqlite3_prepare16_v2()] or the older legacy
** interfaces [sqlite3_prepare()] and [sqlite3_prepare16()].  The use of the
** new "vX" interface is recommended for new applications but the legacy
** interface will continue to be supported.
**
** ^In the legacy interface, the return value will be either [SQLITE_BUSY],
** [SQLITE_DONE], [SQLITE_ROW], [SQLITE_ERROR], or [SQLITE_MISUSE].
** ^With the "v2" interface, any of the other [result codes] or
** [extended result codes] might be returned as well.
**
** ^[SQLITE_BUSY] means that the database engine was unable to acquire the
** database locks it needs to do its job.  ^If the statement is a [COMMIT]
** or occurs outside of an explicit transaction, then you can retry the
** statement.  If the statement is not a [COMMIT] and occurs within an
** explicit transaction then you should rollback the transaction before
** continuing.
**
** ^[SQLITE_DONE] means that the statement has finished executing
** successfully.  sqlite3_step() should not be called again on this virtual
** machine without first calling [sqlite3_reset()] to reset the virtual
** machine back to its initial state.
**
** ^If the SQL statement being executed returns any data, then [SQLITE_ROW]
** is returned each time a new row of data is ready for processing by the
** caller. The values may be accessed using the [column access functions].
** sqlite3_step() is called again to retrieve the next row of data.
**
** ^[SQLITE_ERROR] means that a run-time error (such as a constraint
** violation) has occurred.  sqlite3_step() should not be called again on
** the VM. More information may be found by calling [sqlite3_errmsg()].
** ^With the legacy interface, a more specific error code (for example,
** [SQLITE_INTERRUPT], [SQLITE_SCHEMA], [SQLITE_CORRUPT], and so forth)
** can be obtained by calling [sqlite3_reset()] on the
** [prepared statement].  ^In the "v2" interface,
** the more specific error code is returned directly by sqlite3_step().
**
** [SQLITE_MISUSE] means that the this routine was called inappropriately.
** Perhaps it was called on a [prepared statement] that has
** already been [sqlite3_finalize | finalized] or on one that had
** previously returned [SQLITE_ERROR] or [SQLITE_DONE].  Or it could
** be the case that the same database connection is being used by two or
** more threads at the same moment in time.
**
** For all versions of SQLite up to and including 3.6.23.1, a call to
** [sqlite3_reset()] was required after sqlite3_step() returned anything
** other than [SQLITE_ROW] before any subsequent invocation of
** sqlite3_step().  Failure to reset the prepared statement using
** [sqlite3_reset()] would result in an [SQLITE_MISUSE] return from
** sqlite3_step().  But after [version 3.6.23.1] ([dateof:3.6.23.1],
** sqlite3_step() began
** calling [sqlite3_reset()] automatically in this circumstance rather
** than returning [SQLITE_MISUSE].  This is not considered a compatibility
** break because any application that ever receives an SQLITE_MISUSE error
** is broken by definition.  The [SQLITE_OMIT_AUTORESET] compile-time option
** can be used to restore the legacy behavior.
**
** <b>Goofy Interface Alert:</b> In the legacy interface, the sqlite3_step()
** API always returns a generic error code, [SQLITE_ERROR], following any
** error other than [SQLITE_BUSY] and [SQLITE_MISUSE].  You must call
** [sqlite3_reset()] or [sqlite3_finalize()] in order to find one of the
** specific [error codes] that better describes the error.
** We admit that this is a goofy design.  The problem has been fixed
** with the "v2" interface.  If you prepare all of your SQL statements
** using [sqlite3_prepare_v3()] or [sqlite3_prepare_v2()]
** or [sqlite3_prepare16_v2()] or [sqlite3_prepare16_v3()] instead
** of the legacy [sqlite3_prepare()] and [sqlite3_prepare16()] interfaces,
** then the more specific [error codes] are returned directly
** by sqlite3_step().  The use of the "vX" interfaces is recommended.
*/
SQLITE_API int sqlite3_step(sqlite3_stmt*);

/*
** CAPI3REF: Number of columns in a result set
** METHOD: sqlite3_stmt
**
** ^The sqlite3_data_count(P) interface returns the number of columns in the
** current row of the result set of [prepared statement] P.
** ^If prepared statement P does not have results ready to return
** (via calls to the [sqlite3_column_int | sqlite3_column()] family of
** interfaces) then sqlite3_data_count(P) returns 0.
** ^The sqlite3_data_count(P) routine also returns 0 if P is a NULL pointer.
** ^The sqlite3_data_count(P) routine returns 0 if the previous call to
** [sqlite3_step](P) returned [SQLITE_DONE].  ^The sqlite3_data_count(P)
** will return non-zero if previous call to [sqlite3_step](P) returned
** [SQLITE_ROW], except in the case of the [PRAGMA incremental_vacuum]
** where it always returns zero since each step of that multi-step
** pragma returns 0 columns of data.
**
** See also: [sqlite3_column_count()]
*/
SQLITE_API int sqlite3_data_count(sqlite3_stmt *pStmt);

/*
** CAPI3REF: Fundamental Datatypes
** KEYWORDS: SQLITE_TEXT
**
** ^(Every value in SQLite has one of five fundamental datatypes:
**
** <ul>
** <li> 64-bit signed integer
** <li> 64-bit IEEE floating point number
** <li> string
** <li> BLOB
** <li> NULL
** </ul>)^
**
** These constants are codes for each of those types.
**
** Note that the SQLITE_TEXT constant was also used in SQLite version 2
** for a completely different meaning.  Software that links against both
** SQLite version 2 and SQLite version 3 should use SQLITE3_TEXT, not
** SQLITE_TEXT.
*/
#define SQLITE_INTEGER  1
#define SQLITE_FLOAT    2
#define SQLITE_BLOB     4
#define SQLITE_NULL     5
#ifdef SQLITE_TEXT
# undef SQLITE_TEXT
#else
# define SQLITE_TEXT     3
#endif
#define SQLITE3_TEXT     3

/*
** CAPI3REF: Result Values From A Query
** KEYWORDS: {column access functions}
** METHOD: sqlite3_stmt
**
** <b>Summary:</b>
** <blockquote><table border=0 cellpadding=0 cellspacing=0>
** <tr><td><b>sqlite3_column_blob</b><td>&rarr;<td>BLOB result
** <tr><td><b>sqlite3_column_double</b><td>&rarr;<td>REAL result
** <tr><td><b>sqlite3_column_int</b><td>&rarr;<td>32-bit INTEGER result
** <tr><td><b>sqlite3_column_int64</b><td>&rarr;<td>64-bit INTEGER result
** <tr><td><b>sqlite3_column_text</b><td>&rarr;<td>UTF-8 TEXT result
** <tr><td><b>sqlite3_column_text16</b><td>&rarr;<td>UTF-16 TEXT result
** <tr><td><b>sqlite3_column_value</b><td>&rarr;<td>The result as an
** [sqlite3_value|unprotected sqlite3_value] object.
** <tr><td>&nbsp;<td>&nbsp;<td>&nbsp;
** <tr><td><b>sqlite3_column_bytes</b><td>&rarr;<td>Size of a BLOB
** or a UTF-8 TEXT result in bytes
** <tr><td><b>sqlite3_column_bytes16&nbsp;&nbsp;</b>
** <td>&rarr;&nbsp;&nbsp;<td>Size of UTF-16
** TEXT in bytes
** <tr><td><b>sqlite3_column_type</b><td>&rarr;<td>Default
** datatype of the result
** </table></blockquote>
**
** <b>Details:</b>
**
** ^These routines return information about a single column of the current
** result row of a query.  ^In every case the first argument is a pointer
** to the [prepared statement] that is being evaluated (the [sqlite3_stmt*]
** that was returned from [sqlite3_prepare_v2()] or one of its variants)
** and the second argument is the index of the column for which information
** should be returned. ^The leftmost column of the result set has the index 0.
** ^The number of columns in the result can be determined using
** [sqlite3_column_count()].
**
** If the SQL statement does not currently point to a valid row, or if the
** column index is out of range, the result is undefined.
** These routines may only be called when the most recent call to
** [sqlite3_step()] has returned [SQLITE_ROW] and neither
** [sqlite3_reset()] nor [sqlite3_finalize()] have been called subsequently.
** If any of these routines are called after [sqlite3_reset()] or
** [sqlite3_finalize()] or after [sqlite3_step()] has returned
** something other than [SQLITE_ROW], the results are undefined.
** If [sqlite3_step()] or [sqlite3_reset()] or [sqlite3_finalize()]
** are called from a different thread while any of these routines
** are pending, then the results are undefined.
**
** The first six interfaces (_blob, _double, _int, _int64, _text, and _text16)
** each return the value of a result column in a specific data format.  If
** the result column is not initially in the requested format (for example,
** if the query returns an integer but the sqlite3_column_text() interface
** is used to extract the value) then an automatic type conversion is performed.
**
** ^The sqlite3_column_type() routine returns the
** [SQLITE_INTEGER | datatype code] for the initial data type
** of the result column.  ^The returned value is one of [SQLITE_INTEGER],
** [SQLITE_FLOAT], [SQLITE_TEXT], [SQLITE_BLOB], or [SQLITE_NULL].
** The return value of sqlite3_column_type() can be used to decide which
** of the first six interface should be used to extract the column value.
** The value returned by sqlite3_column_type() is only meaningful if no
** automatic type conversions have occurred for the value in question.
** After a type conversion, the result of calling sqlite3_column_type()
** is undefined, though harmless.  Future
** versions of SQLite may change the behavior of sqlite3_column_type()
** following a type conversion.
**
** If the result is a BLOB or a TEXT string, then the sqlite3_column_bytes()
** or sqlite3_column_bytes16() interfaces can be used to determine the size
** of that BLOB or string.
**
** ^If the result is a BLOB or UTF-8 string then the sqlite3_column_bytes()
** routine returns the number of bytes in that BLOB or string.
** ^If the result is a UTF-16 string, then sqlite3_column_bytes() converts
** the string to UTF-8 and then returns the number of bytes.
** ^If the result is a numeric value then sqlite3_column_bytes() uses
** [sqlite3_snprintf()] to convert that value to a UTF-8 string and returns
** the number of bytes in that string.
** ^If the result is NULL, then sqlite3_column_bytes() returns zero.
**
** ^If the result is a BLOB or UTF-16 string then the sqlite3_column_bytes16()
** routine returns the number of bytes in that BLOB or string.
** ^If the result is a UTF-8 string, then sqlite3_column_bytes16() converts
** the string to UTF-16 and then returns the number of bytes.
** ^If the result is a numeric value then sqlite3_column_bytes16() uses
** [sqlite3_snprintf()] to convert that value to a UTF-16 string and returns
** the number of bytes in that string.
** ^If the result is NULL, then sqlite3_column_bytes16() returns zero.
**
** ^The values returned by [sqlite3_column_bytes()] and
** [sqlite3_column_bytes16()] do not include the zero terminators at the end
** of the string.  ^For clarity: the values returned by
** [sqlite3_column_bytes()] and [sqlite3_column_bytes16()] are the number of
** bytes in the string, not the number of characters.
**
** ^Strings returned by sqlite3_column_text() and sqlite3_column_text16(),
** even empty strings, are always zero-terminated.  ^The return
** value from sqlite3_column_blob() for a zero-length BLOB is a NULL pointer.
**
** ^Strings returned by sqlite3_column_text16() always have the endianness
** which is native to the platform, regardless of the text encoding set
** for the database.
**
** <b>Warning:</b> ^The object returned by [sqlite3_column_value()] is an
** [unprotected sqlite3_value] object.  In a multithreaded environment,
** an unprotected sqlite3_value object may only be used safely with
** [sqlite3_bind_value()] and [sqlite3_result_value()].
** If the [unprotected sqlite3_value] object returned by
** [sqlite3_column_value()] is used in any other way, including calls
** to routines like [sqlite3_value_int()], [sqlite3_value_text()],
** or [sqlite3_value_bytes()], the behavior is not threadsafe.
** Hence, the sqlite3_column_value() interface
** is normally only useful within the implementation of
** [application-defined SQL functions] or [virtual tables], not within
** top-level application code.
**
** These routines may attempt to convert the datatype of the result.
** ^For example, if the internal representation is FLOAT and a text result
** is requested, [sqlite3_snprintf()] is used internally to perform the
** conversion automatically.  ^(The following table details the conversions
** that are applied:
**
** <blockquote>
** <table border="1">
** <tr><th> Internal<br>Type <th> Requested<br>Type <th>  Conversion
**
** <tr><td>  NULL    <td> INTEGER   <td> Result is 0
** <tr><td>  NULL    <td>  FLOAT    <td> Result is 0.0
** <tr><td>  NULL    <td>   TEXT    <td> Result is a NULL pointer
** <tr><td>  NULL    <td>   BLOB    <td> Result is a NULL pointer
** <tr><td> INTEGER  <td>  FLOAT    <td> Convert from integer to float
** <tr><td> INTEGER  <td>   TEXT    <td> ASCII rendering of the integer
** <tr><td> INTEGER  <td>   BLOB    <td> Same as INTEGER->TEXT
** <tr><td>  FLOAT   <td> INTEGER   <td> [CAST] to INTEGER
** <tr><td>  FLOAT   <td>   TEXT    <td> ASCII rendering of the float
** <tr><td>  FLOAT   <td>   BLOB    <td> [CAST] to BLOB
** <tr><td>  TEXT    <td> INTEGER   <td> [CAST] to INTEGER
** <tr><td>  TEXT    <td>  FLOAT    <td> [CAST] to REAL
** <tr><td>  TEXT    <td>   BLOB    <td> No change
** <tr><td>  BLOB    <td> INTEGER   <td> [CAST] to INTEGER
** <tr><td>  BLOB    <td>  FLOAT    <td> [CAST] to REAL
** <tr><td>  BLOB    <td>   TEXT    <td> [CAST] to TEXT, ensure zero terminator
** </table>
** </blockquote>)^
**
** Note that when type conversions occur, pointers returned by prior
** calls to sqlite3_column_blob(), sqlite3_column_text(), and/or
** sqlite3_column_text16() may be invalidated.
** Type conversions and pointer invalidations might occur
** in the following cases:
**
** <ul>
** <li> The initial content is a BLOB and sqlite3_column_text() or
**      sqlite3_column_text16() is called.  A zero-terminator might
**      need to be added to the string.</li>
** <li> The initial content is UTF-8 text and sqlite3_column_bytes16() or
**      sqlite3_column_text16() is called.  The content must be converted
**      to UTF-16.</li>
** <li> The initial content is UTF-16 text and sqlite3_column_bytes() or
**      sqlite3_column_text() is called.  The content must be converted
**      to UTF-8.</li>
** </ul>
**
** ^Conversions between UTF-16be and UTF-16le are always done in place and do
** not invalidate a prior pointer, though of course the content of the buffer
** that the prior pointer references will have been modified.  Other kinds
** of conversion are done in place when it is possible, but sometimes they
** are not possible and in those cases prior pointers are invalidated.
**
** The safest policy is to invoke these routines
** in one of the following ways:
**
** <ul>
**  <li>sqlite3_column_text() followed by sqlite3_column_bytes()</li>
**  <li>sqlite3_column_blob() followed by sqlite3_column_bytes()</li>
**  <li>sqlite3_column_text16() followed by sqlite3_column_bytes16()</li>
** </ul>
**
** In other words, you should call sqlite3_column_text(),
** sqlite3_column_blob(), or sqlite3_column_text16() first to force the result
** into the desired format, then invoke sqlite3_column_bytes() or
** sqlite3_column_bytes16() to find the size of the result.  Do not mix calls
** to sqlite3_column_text() or sqlite3_column_blob() with calls to
** sqlite3_column_bytes16(), and do not mix calls to sqlite3_column_text16()
** with calls to sqlite3_column_bytes().
**
** ^The pointers returned are valid until a type conversion occurs as
** described above, or until [sqlite3_step()] or [sqlite3_reset()] or
** [sqlite3_finalize()] is called.  ^The memory space used to hold strings
** and BLOBs is freed automatically.  Do not pass the pointers returned
** from [sqlite3_column_blob()], [sqlite3_column_text()], etc. into
** [sqlite3_free()].
**
** As long as the input parameters are correct, these routines will only
** fail if an out-of-memory error occurs during a format conversion.
** Only the following subset of interfaces are subject to out-of-memory
** errors:
**
** <ul>
** <li> sqlite3_column_blob()
** <li> sqlite3_column_text()
** <li> sqlite3_column_text16()
** <li> sqlite3_column_bytes()
** <li> sqlite3_column_bytes16()
** </ul>
**
** If an out-of-memory error occurs, then the return value from these
** routines is the same as if the column had contained an SQL NULL value.
** Valid SQL NULL returns can be distinguished from out-of-memory errors
** by invoking the [sqlite3_errcode()] immediately after the suspect
** return value is obtained and before any
** other SQLite interface is called on the same [database connection].
*/
SQLITE_API const void *sqlite3_column_blob(sqlite3_stmt*, int iCol);
SQLITE_API double sqlite3_column_double(sqlite3_stmt*, int iCol);
SQLITE_API int sqlite3_column_int(sqlite3_stmt*, int iCol);
SQLITE_API sqlite3_int64 sqlite3_column_int64(sqlite3_stmt*, int iCol);
SQLITE_API const unsigned char *sqlite3_column_text(sqlite3_stmt*, int iCol);
SQLITE_API const void *sqlite3_column_text16(sqlite3_stmt*, int iCol);
SQLITE_API sqlite3_value *sqlite3_column_value(sqlite3_stmt*, int iCol);
SQLITE_API int sqlite3_column_bytes(sqlite3_stmt*, int iCol);
SQLITE_API int sqlite3_column_bytes16(sqlite3_stmt*, int iCol);
SQLITE_API int sqlite3_column_type(sqlite3_stmt*, int iCol);

/*
** CAPI3REF: Destroy A Prepared Statement Object
** DESTRUCTOR: sqlite3_stmt
**
** ^The sqlite3_finalize() function is called to delete a [prepared statement].
** ^If the most recent evaluation of the statement encountered no errors
** or if the statement is never been evaluated, then sqlite3_finalize() returns
** SQLITE_OK.  ^If the most recent evaluation of statement S failed, then
** sqlite3_finalize(S) returns the appropriate [error code] or
** [extended error code].
**
** ^The sqlite3_finalize(S) routine can be called at any point during
** the life cycle of [prepared statement] S:
** before statement S is ever evaluated, after
** one or more calls to [sqlite3_reset()], or after any call
** to [sqlite3_step()] regardless of whether or not the statement has
** completed execution.
**
** ^Invoking sqlite3_finalize() on a NULL pointer is a harmless no-op.
**
** The application must finalize every [prepared statement] in order to avoid
** resource leaks.  It is a grievous error for the application to try to use
** a prepared statement after it has been finalized.  Any use of a prepared
** statement after it has been finalized can result in undefined and
** undesirable behavior such as segfaults and heap corruption.
*/
SQLITE_API int sqlite3_finalize(sqlite3_stmt *pStmt);

/*
** CAPI3REF: Reset A Prepared Statement Object
** METHOD: sqlite3_stmt
**
** The sqlite3_reset() function is called to reset a [prepared statement]
** object back to its initial state, ready to be re-executed.
** ^Any SQL statement variables that had values bound to them using
** the [sqlite3_bind_blob | sqlite3_bind_*() API] retain their values.
** Use [sqlite3_clear_bindings()] to reset the bindings.
**
** ^The [sqlite3_reset(S)] interface resets the [prepared statement] S
** back to the beginning of its program.
**
** ^If the most recent call to [sqlite3_step(S)] for the
** [prepared statement] S returned [SQLITE_ROW] or [SQLITE_DONE],
** or if [sqlite3_step(S)] has never before been called on S,
** then [sqlite3_reset(S)] returns [SQLITE_OK].
**
** ^If the most recent call to [sqlite3_step(S)] for the
** [prepared statement] S indicated an error, then
** [sqlite3_reset(S)] returns an appropriate [error code].
**
** ^The [sqlite3_reset(S)] interface does not change the values
** of any [sqlite3_bind_blob|bindings] on the [prepared statement] S.
*/
SQLITE_API int sqlite3_reset(sqlite3_stmt *pStmt);

/*
** CAPI3REF: Create Or Redefine SQL Functions
** KEYWORDS: {function creation routines}
** METHOD: sqlite3
**
** ^These functions (collectively known as "function creation routines")
** are used to add SQL functions or aggregates or to redefine the behavior
** of existing SQL functions or aggregates. The only differences between
** the three "sqlite3_create_function*" routines are the text encoding
** expected for the second parameter (the name of the function being
** created) and the presence or absence of a destructor callback for
** the application data pointer. Function sqlite3_create_window_function()
** is similar, but allows the user to supply the extra callback functions
** needed by [aggregate window functions].
**
** ^The first parameter is the [database connection] to which the SQL
** function is to be added.  ^If an application uses more than one database
** connection then application-defined SQL functions must be added
** to each database connection separately.
**
** ^The second parameter is the name of the SQL function to be created or
** redefined.  ^The length of the name is limited to 255 bytes in a UTF-8
** representation, exclusive of the zero-terminator.  ^Note that the name
** length limit is in UTF-8 bytes, not characters nor UTF-16 bytes.
** ^Any attempt to create a function with a longer name
** will result in [SQLITE_MISUSE] being returned.
**
** ^The third parameter (nArg)
** is the number of arguments that the SQL function or
** aggregate takes. ^If this parameter is -1, then the SQL function or
** aggregate may take any number of arguments between 0 and the limit
** set by [sqlite3_limit]([SQLITE_LIMIT_FUNCTION_ARG]).  If the third
** parameter is less than -1 or greater than 127 then the behavior is
** undefined.
**
** ^The fourth parameter, eTextRep, specifies what
** [SQLITE_UTF8 | text encoding] this SQL function prefers for
** its parameters.  The application should set this parameter to
** [SQLITE_UTF16LE] if the function implementation invokes
** [sqlite3_value_text16le()] on an input, or [SQLITE_UTF16BE] if the
** implementation invokes [sqlite3_value_text16be()] on an input, or
** [SQLITE_UTF16] if [sqlite3_value_text16()] is used, or [SQLITE_UTF8]
** otherwise.  ^The same SQL function may be registered multiple times using
** different preferred text encodings, with different implementations for
** each encoding.
** ^When multiple implementations of the same function are available, SQLite
** will pick the one that involves the least amount of data conversion.
**
** ^The fourth parameter may optionally be ORed with [SQLITE_DETERMINISTIC]
** to signal that the function will always return the same result given
** the same inputs within a single SQL statement.  Most SQL functions are
** deterministic.  The built-in [random()] SQL function is an example of a
** function that is not deterministic.  The SQLite query planner is able to
** perform additional optimizations on deterministic functions, so use
** of the [SQLITE_DETERMINISTIC] flag is recommended where possible.
**
** ^The fourth parameter may also optionally include the [SQLITE_DIRECTONLY]
** flag, which if present prevents the function from being invoked from
** within VIEWs, TRIGGERs, CHECK constraints, generated column expressions,
** index expressions, or the WHERE clause of partial indexes.
**
** For best security, the [SQLITE_DIRECTONLY] flag is recommended for
** all application-defined SQL functions that do not need to be
** used inside of triggers, view, CHECK constraints, or other elements of
** the database schema.  This flags is especially recommended for SQL
** functions that have side effects or reveal internal application state.
** Without this flag, an attacker might be able to modify the schema of
** a database file to include invocations of the function with parameters
** chosen by the attacker, which the application will then execute when
** the database file is opened and read.
**
** ^(The fifth parameter is an arbitrary pointer.  The implementation of the
** function can gain access to this pointer using [sqlite3_user_data()].)^
**
** ^The sixth, seventh and eighth parameters passed to the three
** "sqlite3_create_function*" functions, xFunc, xStep and xFinal, are
** pointers to C-language functions that implement the SQL function or
** aggregate. ^A scalar SQL function requires an implementation of the xFunc
** callback only; NULL pointers must be passed as the xStep and xFinal
** parameters. ^An aggregate SQL function requires an implementation of xStep
** and xFinal and NULL pointer must be passed for xFunc. ^To delete an existing
** SQL function or aggregate, pass NULL pointers for all three function
** callbacks.
**
** ^The sixth, seventh, eighth and ninth parameters (xStep, xFinal, xValue
** and xInverse) passed to sqlite3_create_window_function are pointers to
** C-language callbacks that implement the new function. xStep and xFinal
** must both be non-NULL. xValue and xInverse may either both be NULL, in
** which case a regular aggregate function is created, or must both be
** non-NULL, in which case the new function may be used as either an aggregate
** or aggregate window function. More details regarding the implementation
** of aggregate window functions are
** [user-defined window functions|available here].
**
** ^(If the final parameter to sqlite3_create_function_v2() or
** sqlite3_create_window_function() is not NULL, then it is destructor for
** the application data pointer. The destructor is invoked when the function
** is deleted, either by being overloaded or when the database connection
** closes.)^ ^The destructor is also invoked if the call to
** sqlite3_create_function_v2() fails.  ^When the destructor callback is
** invoked, it is passed a single argument which is a copy of the application
** data pointer which was the fifth parameter to sqlite3_create_function_v2().
**
** ^It is permitted to register multiple implementations of the same
** functions with the same name but with either differing numbers of
** arguments or differing preferred text encodings.  ^SQLite will use
** the implementation that most closely matches the way in which the
** SQL function is used.  ^A function implementation with a non-negative
** nArg parameter is a better match than a function implementation with
** a negative nArg.  ^A function where the preferred text encoding
** matches the database encoding is a better
** match than a function where the encoding is different.
** ^A function where the encoding difference is between UTF16le and UTF16be
** is a closer match than a function where the encoding difference is
** between UTF8 and UTF16.
**
** ^Built-in functions may be overloaded by new application-defined functions.
**
** ^An application-defined function is permitted to call other
** SQLite interfaces.  However, such calls must not
** close the database connection nor finalize or reset the prepared
** statement in which the function is running.
*/
SQLITE_API int sqlite3_create_function(
  sqlite3 *db,
  const char *zFunctionName,
  int nArg,
  int eTextRep,
  void *pApp,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
  void (*xStep)(sqlite3_context*,int,sqlite3_value**),
  void (*xFinal)(sqlite3_context*)
);
SQLITE_API int sqlite3_create_function16(
  sqlite3 *db,
  const void *zFunctionName,
  int nArg,
  int eTextRep,
  void *pApp,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
  void (*xStep)(sqlite3_context*,int,sqlite3_value**),
  void (*xFinal)(sqlite3_context*)
);
SQLITE_API int sqlite3_create_function_v2(
  sqlite3 *db,
  const char *zFunctionName,
  int nArg,
  int eTextRep,
  void *pApp,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
  void (*xStep)(sqlite3_context*,int,sqlite3_value**),
  void (*xFinal)(sqlite3_context*),
  void(*xDestroy)(void*)
);
SQLITE_API int sqlite3_create_window_function(
  sqlite3 *db,
  const char *zFunctionName,
  int nArg,
  int eTextRep,
  void *pApp,
  void (*xStep)(sqlite3_context*,int,sqlite3_value**),
  void (*xFinal)(sqlite3_context*),
  void (*xValue)(sqlite3_context*),
  void (*xInverse)(sqlite3_context*,int,sqlite3_value**),
  void(*xDestroy)(void*)
);

/*
** CAPI3REF: Text Encodings
**
** These constant define integer codes that represent the various
** text encodings supported by SQLite.
*/
#define SQLITE_UTF8           1    /* IMP: R-37514-35566 */
#define SQLITE_UTF16LE        2    /* IMP: R-03371-37637 */
#define SQLITE_UTF16BE        3    /* IMP: R-51971-34154 */
#define SQLITE_UTF16          4    /* Use native byte order */
#define SQLITE_ANY            5    /* Deprecated */
#define SQLITE_UTF16_ALIGNED  8    /* sqlite3_create_collation only */

/*
** CAPI3REF: Function Flags
**
** These constants may be ORed together with the
** [SQLITE_UTF8 | preferred text encoding] as the fourth argument
** to [sqlite3_create_function()], [sqlite3_create_function16()], or
** [sqlite3_create_function_v2()].
**
** <dl>
** [[SQLITE_DETERMINISTIC]] <dt>SQLITE_DETERMINISTIC</dt><dd>
** The SQLITE_DETERMINISTIC flag means that the new function always gives
** the same output when the input parameters are the same.
** The [abs|abs() function] is deterministic, for example, but
** [randomblob|randomblob()] is not.  Functions must
** be deterministic in order to be used in certain contexts such as
** with the WHERE clause of [partial indexes] or in [generated columns].
** SQLite might also optimize deterministic functions by factoring them
** out of inner loops.
** </dd>
**
** [[SQLITE_DIRECTONLY]] <dt>SQLITE_DIRECTONLY</dt><dd>
** The SQLITE_DIRECTONLY flag means that the function may only be invoked
** from top-level SQL, and cannot be used in VIEWs or TRIGGERs nor in
** schema structures such as [CHECK constraints], [DEFAULT clauses],
** [expression indexes], [partial indexes], or [generated columns].
** The SQLITE_DIRECTONLY flags is a security feature which is recommended
** for all [application-defined SQL functions], and especially for functions
** that have side-effects or that could potentially leak sensitive
** information.
** </dd>
**
** [[SQLITE_INNOCUOUS]] <dt>SQLITE_INNOCUOUS</dt><dd>
** The SQLITE_INNOCUOUS flag means that the function is unlikely
** to cause problems even if misused.  An innocuous function should have
** no side effects and should not depend on any values other than its
** input parameters. The [abs|abs() function] is an example of an
** innocuous function.
** The [load_extension() SQL function] is not innocuous because of its
** side effects.
** <p> SQLITE_INNOCUOUS is similar to SQLITE_DETERMINISTIC, but is not
** exactly the same.  The [random|random() function] is an example of a
** function that is innocuous but not deterministic.
** <p>Some heightened security settings
** ([SQLITE_DBCONFIG_TRUSTED_SCHEMA] and [PRAGMA trusted_schema=OFF])
** disable the use of SQL functions inside views and triggers and in
** schema structures such as [CHECK constraints], [DEFAULT clauses],
** [expression indexes], [partial indexes], and [generated columns] unless
** the function is tagged with SQLITE_INNOCUOUS.  Most built-in functions
** are innocuous.  Developers are advised to avoid using the
** SQLITE_INNOCUOUS flag for application-defined functions unless the
** function has been carefully audited and found to be free of potentially
** security-adverse side-effects and information-leaks.
** </dd>
**
** [[SQLITE_SUBTYPE]] <dt>SQLITE_SUBTYPE</dt><dd>
** The SQLITE_SUBTYPE flag indicates to SQLite that a function may call
** [sqlite3_value_subtype()] to inspect the sub-types of its arguments.
** Specifying this flag makes no difference for scalar or aggregate user
** functions. However, if it is not specified for a user-defined window
** function, then any sub-types belonging to arguments passed to the window
** function may be discarded before the window function is called (i.e.
** sqlite3_value_subtype() will always return 0).
** </dd>
** </dl>
*/
#define SQLITE_DETERMINISTIC    0x000000800
#define SQLITE_DIRECTONLY       0x000080000
#define SQLITE_SUBTYPE          0x000100000
#define SQLITE_INNOCUOUS        0x000200000

/*
** CAPI3REF: Deprecated Functions
** DEPRECATED
**
** These functions are [deprecated].  In order to maintain
** backwards compatibility with older code, these functions continue
** to be supported.  However, new applications should avoid
** the use of these functions.  To encourage programmers to avoid
** these functions, we will not explain what they do.
*/
#ifndef SQLITE_OMIT_DEPRECATED
SQLITE_API SQLITE_DEPRECATED int sqlite3_aggregate_count(sqlite3_context*);
SQLITE_API SQLITE_DEPRECATED int sqlite3_expired(sqlite3_stmt*);
SQLITE_API SQLITE_DEPRECATED int sqlite3_transfer_bindings(sqlite3_stmt*, sqlite3_stmt*);
SQLITE_API SQLITE_DEPRECATED int sqlite3_global_recover(void);
SQLITE_API SQLITE_DEPRECATED void sqlite3_thread_cleanup(void);
SQLITE_API SQLITE_DEPRECATED int sqlite3_memory_alarm(void(*)(void*,sqlite3_int64,int),
                      void*,sqlite3_int64);
#endif

/*
** CAPI3REF: Obtaining SQL Values
** METHOD: sqlite3_value
**
** <b>Summary:</b>
** <blockquote><table border=0 cellpadding=0 cellspacing=0>
** <tr><td><b>sqlite3_value_blob</b><td>&rarr;<td>BLOB value
** <tr><td><b>sqlite3_value_double</b><td>&rarr;<td>REAL value
** <tr><td><b>sqlite3_value_int</b><td>&rarr;<td>32-bit INTEGER value
** <tr><td><b>sqlite3_value_int64</b><td>&rarr;<td>64-bit INTEGER value
** <tr><td><b>sqlite3_value_pointer</b><td>&rarr;<td>Pointer value
** <tr><td><b>sqlite3_value_text</b><td>&rarr;<td>UTF-8 TEXT value
** <tr><td><b>sqlite3_value_text16</b><td>&rarr;<td>UTF-16 TEXT value in
** the native byteorder
** <tr><td><b>sqlite3_value_text16be</b><td>&rarr;<td>UTF-16be TEXT value
** <tr><td><b>sqlite3_value_text16le</b><td>&rarr;<td>UTF-16le TEXT value
** <tr><td>&nbsp;<td>&nbsp;<td>&nbsp;
** <tr><td><b>sqlite3_value_bytes</b><td>&rarr;<td>Size of a BLOB
** or a UTF-8 TEXT in bytes
** <tr><td><b>sqlite3_value_bytes16&nbsp;&nbsp;</b>
** <td>&rarr;&nbsp;&nbsp;<td>Size of UTF-16
** TEXT in bytes
** <tr><td><b>sqlite3_value_type</b><td>&rarr;<td>Default
** datatype of the value
** <tr><td><b>sqlite3_value_numeric_type&nbsp;&nbsp;</b>
** <td>&rarr;&nbsp;&nbsp;<td>Best numeric datatype of the value
** <tr><td><b>sqlite3_value_nochange&nbsp;&nbsp;</b>
** <td>&rarr;&nbsp;&nbsp;<td>True if the column is unchanged in an UPDATE
** against a virtual table.
** <tr><td><b>sqlite3_value_frombind&nbsp;&nbsp;</b>
** <td>&rarr;&nbsp;&nbsp;<td>True if value originated from a [bound parameter]
** </table></blockquote>
**
** <b>Details:</b>
**
** These routines extract type, size, and content information from
** [protected sqlite3_value] objects.  Protected sqlite3_value objects
** are used to pass parameter information into the functions that
** implement [application-defined SQL functions] and [virtual tables].
**
** These routines work only with [protected sqlite3_value] objects.
** Any attempt to use these routines on an [unprotected sqlite3_value]
** is not threadsafe.
**
** ^These routines work just like the corresponding [column access functions]
** except that these routines take a single [protected sqlite3_value] object
** pointer instead of a [sqlite3_stmt*] pointer and an integer column number.
**
** ^The sqlite3_value_text16() interface extracts a UTF-16 string
** in the native byte-order of the host machine.  ^The
** sqlite3_value_text16be() and sqlite3_value_text16le() interfaces
** extract UTF-16 strings as big-endian and little-endian respectively.
**
** ^If [sqlite3_value] object V was initialized
** using [sqlite3_bind_pointer(S,I,P,X,D)] or [sqlite3_result_pointer(C,P,X,D)]
** and if X and Y are strings that compare equal according to strcmp(X,Y),
** then sqlite3_value_pointer(V,Y) will return the pointer P.  ^Otherwise,
** sqlite3_value_pointer(V,Y) returns a NULL. The sqlite3_bind_pointer()
** routine is part of the [pointer passing interface] added for SQLite 3.20.0.
**
** ^(The sqlite3_value_type(V) interface returns the
** [SQLITE_INTEGER | datatype code] for the initial datatype of the
** [sqlite3_value] object V. The returned value is one of [SQLITE_INTEGER],
** [SQLITE_FLOAT], [SQLITE_TEXT], [SQLITE_BLOB], or [SQLITE_NULL].)^
** Other interfaces might change the datatype for an sqlite3_value object.
** For example, if the datatype is initially SQLITE_INTEGER and
** sqlite3_value_text(V) is called to extract a text value for that
** integer, then subsequent calls to sqlite3_value_type(V) might return
** SQLITE_TEXT.  Whether or not a persistent internal datatype conversion
** occurs is undefined and may change from one release of SQLite to the next.
**
** ^(The sqlite3_value_numeric_type() interface attempts to apply
** numeric affinity to the value.  This means that an attempt is
** made to convert the value to an integer or floating point.  If
** such a conversion is possible without loss of information (in other
** words, if the value is a string that looks like a number)
** then the conversion is performed.  Otherwise no conversion occurs.
** The [SQLITE_INTEGER | datatype] after conversion is returned.)^
**
** ^Within the [xUpdate] method of a [virtual table], the
** sqlite3_value_nochange(X) interface returns true if and only if
** the column corresponding to X is unchanged by the UPDATE operation
** that the xUpdate method call was invoked to implement and if
** and the prior [xColumn] method call that was invoked to extracted
** the value for that column returned without setting a result (probably
** because it queried [sqlite3_vtab_nochange()] and found that the column
** was unchanging).  ^Within an [xUpdate] method, any value for which
** sqlite3_value_nochange(X) is true will in all other respects appear
** to be a NULL value.  If sqlite3_value_nochange(X) is invoked anywhere other
** than within an [xUpdate] method call for an UPDATE statement, then
** the return value is arbitrary and meaningless.
**
** ^The sqlite3_value_frombind(X) interface returns non-zero if the
** value X originated from one of the [sqlite3_bind_int|sqlite3_bind()]
** interfaces.  ^If X comes from an SQL literal value, or a table column,
** or an expression, then sqlite3_value_frombind(X) returns zero.
**
** Please pay particular attention to the fact that the pointer returned
** from [sqlite3_value_blob()], [sqlite3_value_text()], or
** [sqlite3_value_text16()] can be invalidated by a subsequent call to
** [sqlite3_value_bytes()], [sqlite3_value_bytes16()], [sqlite3_value_text()],
** or [sqlite3_value_text16()].
**
** These routines must be called from the same thread as
** the SQL function that supplied the [sqlite3_value*] parameters.
**
** As long as the input parameter is correct, these routines can only
** fail if an out-of-memory error occurs during a format conversion.
** Only the following subset of interfaces are subject to out-of-memory
** errors:
**
** <ul>
** <li> sqlite3_value_blob()
** <li> sqlite3_value_text()
** <li> sqlite3_value_text16()
** <li> sqlite3_value_text16le()
** <li> sqlite3_value_text16be()
** <li> sqlite3_value_bytes()
** <li> sqlite3_value_bytes16()
** </ul>
**
** If an out-of-memory error occurs, then the return value from these
** routines is the same as if the column had contained an SQL NULL value.
** Valid SQL NULL returns can be distinguished from out-of-memory errors
** by invoking the [sqlite3_errcode()] immediately after the suspect
** return value is obtained and before any
** other SQLite interface is called on the same [database connection].
*/
SQLITE_API const void *sqlite3_value_blob(sqlite3_value*);
SQLITE_API double sqlite3_value_double(sqlite3_value*);
SQLITE_API int sqlite3_value_int(sqlite3_value*);
SQLITE_API sqlite3_int64 sqlite3_value_int64(sqlite3_value*);
SQLITE_API void *sqlite3_value_pointer(sqlite3_value*, const char*);
SQLITE_API const unsigned char *sqlite3_value_text(sqlite3_value*);
SQLITE_API const void *sqlite3_value_text16(sqlite3_value*);
SQLITE_API const void *sqlite3_value_text16le(sqlite3_value*);
SQLITE_API const void *sqlite3_value_text16be(sqlite3_value*);
SQLITE_API int sqlite3_value_bytes(sqlite3_value*);
SQLITE_API int sqlite3_value_bytes16(sqlite3_value*);
SQLITE_API int sqlite3_value_type(sqlite3_value*);
SQLITE_API int sqlite3_value_numeric_type(sqlite3_value*);
SQLITE_API int sqlite3_value_nochange(sqlite3_value*);
SQLITE_API int sqlite3_value_frombind(sqlite3_value*);

/*
** CAPI3REF: Finding The Subtype Of SQL Values
** METHOD: sqlite3_value
**
** The sqlite3_value_subtype(V) function returns the subtype for
** an [application-defined SQL function] argument V.  The subtype
** information can be used to pass a limited amount of context from
** one SQL function to another.  Use the [sqlite3_result_subtype()]
** routine to set the subtype for the return value of an SQL function.
*/
SQLITE_API unsigned int sqlite3_value_subtype(sqlite3_value*);

/*
** CAPI3REF: Copy And Free SQL Values
** METHOD: sqlite3_value
**
** ^The sqlite3_value_dup(V) interface makes a copy of the [sqlite3_value]
** object D and returns a pointer to that copy.  ^The [sqlite3_value] returned
** is a [protected sqlite3_value] object even if the input is not.
** ^The sqlite3_value_dup(V) interface returns NULL if V is NULL or if a
** memory allocation fails. ^If V is a [pointer value], then the result
** of sqlite3_value_dup(V) is a NULL value.
**
** ^The sqlite3_value_free(V) interface frees an [sqlite3_value] object
** previously obtained from [sqlite3_value_dup()].  ^If V is a NULL pointer
** then sqlite3_value_free(V) is a harmless no-op.
*/
SQLITE_API sqlite3_value *sqlite3_value_dup(const sqlite3_value*);
SQLITE_API void sqlite3_value_free(sqlite3_value*);

/*
** CAPI3REF: Obtain Aggregate Function Context
** METHOD: sqlite3_context
**
** Implementations of aggregate SQL functions use this
** routine to allocate memory for storing their state.
**
** ^The first time the sqlite3_aggregate_context(C,N) routine is called
** for a particular aggregate function, SQLite allocates
** N bytes of memory, zeroes out that memory, and returns a pointer
** to the new memory. ^On second and subsequent calls to
** sqlite3_aggregate_context() for the same aggregate function instance,
** the same buffer is returned.  Sqlite3_aggregate_context() is normally
** called once for each invocation of the xStep callback and then one
** last time when the xFinal callback is invoked.  ^(When no rows match
** an aggregate query, the xStep() callback of the aggregate function
** implementation is never called and xFinal() is called exactly once.
** In those cases, sqlite3_aggregate_context() might be called for the
** first time from within xFinal().)^
**
** ^The sqlite3_aggregate_context(C,N) routine returns a NULL pointer
** when first called if N is less than or equal to zero or if a memory
** allocate error occurs.
**
** ^(The amount of space allocated by sqlite3_aggregate_context(C,N) is
** determined by the N parameter on first successful call.  Changing the
** value of N in any subsequent call to sqlite3_aggregate_context() within
** the same aggregate function instance will not resize the memory
** allocation.)^  Within the xFinal callback, it is customary to set
** N=0 in calls to sqlite3_aggregate_context(C,N) so that no
** pointless memory allocations occur.
**
** ^SQLite automatically frees the memory allocated by
** sqlite3_aggregate_context() when the aggregate query concludes.
**
** The first parameter must be a copy of the
** [sqlite3_context | SQL function context] that is the first parameter
** to the xStep or xFinal callback routine that implements the aggregate
** function.
**
** This routine must be called from the same thread in which
** the aggregate SQL function is running.
*/
SQLITE_API void *sqlite3_aggregate_context(sqlite3_context*, int nBytes);

/*
** CAPI3REF: User Data For Functions
** METHOD: sqlite3_context
**
** ^The sqlite3_user_data() interface returns a copy of
** the pointer that was the pUserData parameter (the 5th parameter)
** of the [sqlite3_create_function()]
** and [sqlite3_create_function16()] routines that originally
** registered the application defined function.
**
** This routine must be called from the same thread in which
** the application-defined function is running.
*/
SQLITE_API void *sqlite3_user_data(sqlite3_context*);

/*
** CAPI3REF: Database Connection For Functions
** METHOD: sqlite3_context
**
** ^The sqlite3_context_db_handle() interface returns a copy of
** the pointer to the [database connection] (the 1st parameter)
** of the [sqlite3_create_function()]
** and [sqlite3_create_function16()] routines that originally
** registered the application defined function.
*/
SQLITE_API sqlite3 *sqlite3_context_db_handle(sqlite3_context*);

/*
** CAPI3REF: Function Auxiliary Data
** METHOD: sqlite3_context
**
** These functions may be used by (non-aggregate) SQL functions to
** associate metadata with argument values. If the same value is passed to
** multiple invocations of the same SQL function during query execution, under
** some circumstances the associated metadata may be preserved.  An example
** of where this might be useful is in a regular-expression matching
** function. The compiled version of the regular expression can be stored as
** metadata associated with the pattern string.
** Then as long as the pattern string remains the same,
** the compiled regular expression can be reused on multiple
** invocations of the same function.
**
** ^The sqlite3_get_auxdata(C,N) interface returns a pointer to the metadata
** associated by the sqlite3_set_auxdata(C,N,P,X) function with the Nth argument
** value to the application-defined function.  ^N is zero for the left-most
** function argument.  ^If there is no metadata
** associated with the function argument, the sqlite3_get_auxdata(C,N) interface
** returns a NULL pointer.
**
** ^The sqlite3_set_auxdata(C,N,P,X) interface saves P as metadata for the N-th
** argument of the application-defined function.  ^Subsequent
** calls to sqlite3_get_auxdata(C,N) return P from the most recent
** sqlite3_set_auxdata(C,N,P,X) call if the metadata is still valid or
** NULL if the metadata has been discarded.
** ^After each call to sqlite3_set_auxdata(C,N,P,X) where X is not NULL,
** SQLite will invoke the destructor function X with parameter P exactly
** once, when the metadata is discarded.
** SQLite is free to discard the metadata at any time, including: <ul>
** <li> ^(when the corresponding function parameter changes)^, or
** <li> ^(when [sqlite3_reset()] or [sqlite3_finalize()] is called for the
**      SQL statement)^, or
** <li> ^(when sqlite3_set_auxdata() is invoked again on the same
**       parameter)^, or
** <li> ^(during the original sqlite3_set_auxdata() call when a memory
**      allocation error occurs.)^ </ul>
**
** Note the last bullet in particular.  The destructor X in
** sqlite3_set_auxdata(C,N,P,X) might be called immediately, before the
** sqlite3_set_auxdata() interface even returns.  Hence sqlite3_set_auxdata()
** should be called near the end of the function implementation and the
** function implementation should not make any use of P after
** sqlite3_set_auxdata() has been called.
**
** ^(In practice, metadata is preserved between function calls for
** function parameters that are compile-time constants, including literal
** values and [parameters] and expressions composed from the same.)^
**
** The value of the N parameter to these interfaces should be non-negative.
** Future enhancements may make use of negative N values to define new
** kinds of function caching behavior.
**
** These routines must be called from the same thread in which
** the SQL function is running.
*/
SQLITE_API void *sqlite3_get_auxdata(sqlite3_context*, int N);
SQLITE_API void sqlite3_set_auxdata(sqlite3_context*, int N, void*, void (*)(void*));


/*
** CAPI3REF: Constants Defining Special Destructor Behavior
**
** These are special values for the destructor that is passed in as the
** final argument to routines like [sqlite3_result_blob()].  ^If the destructor
** argument is SQLITE_STATIC, it means that the content pointer is constant
** and will never change.  It does not need to be destroyed.  ^The
** SQLITE_TRANSIENT value means that the content will likely change in
** the near future and that SQLite should make its own private copy of
** the content before returning.
**
** The typedef is necessary to work around problems in certain
** C++ compilers.
*/
typedef void (*sqlite3_destructor_type)(void*);
#define SQLITE_STATIC      ((sqlite3_destructor_type)0)
#define SQLITE_TRANSIENT   ((sqlite3_destructor_type)-1)

/*
** CAPI3REF: Setting The Result Of An SQL Function
** METHOD: sqlite3_context
**
** These routines are used by the xFunc or xFinal callbacks that
** implement SQL functions and aggregates.  See
** [sqlite3_create_function()] and [sqlite3_create_function16()]
** for additional information.
**
** These functions work very much like the [parameter binding] family of
** functions used to bind values to host parameters in prepared statements.
** Refer to the [SQL parameter] documentation for additional information.
**
** ^The sqlite3_result_blob() interface sets the result from
** an application-defined function to be the BLOB whose content is pointed
** to by the second parameter and which is N bytes long where N is the
** third parameter.
**
** ^The sqlite3_result_zeroblob(C,N) and sqlite3_result_zeroblob64(C,N)
** interfaces set the result of the application-defined function to be
** a BLOB containing all zero bytes and N bytes in size.
**
** ^The sqlite3_result_double() interface sets the result from
** an application-defined function to be a floating point value specified
** by its 2nd argument.
**
** ^The sqlite3_result_error() and sqlite3_result_error16() functions
** cause the implemented SQL function to throw an exception.
** ^SQLite uses the string pointed to by the
** 2nd parameter of sqlite3_result_error() or sqlite3_result_error16()
** as the text of an error message.  ^SQLite interprets the error
** message string from sqlite3_result_error() as UTF-8. ^SQLite
** interprets the string from sqlite3_result_error16() as UTF-16 using
** the same [byte-order determination rules] as [sqlite3_bind_text16()].
** ^If the third parameter to sqlite3_result_error()
** or sqlite3_result_error16() is negative then SQLite takes as the error
** message all text up through the first zero character.
** ^If the third parameter to sqlite3_result_error() or
** sqlite3_result_error16() is non-negative then SQLite takes that many
** bytes (not characters) from the 2nd parameter as the error message.
** ^The sqlite3_result_error() and sqlite3_result_error16()
** routines make a private copy of the error message text before
** they return.  Hence, the calling function can deallocate or
** modify the text after they return without harm.
** ^The sqlite3_result_error_code() function changes the error code
** returned by SQLite as a result of an error in a function.  ^By default,
** the error code is SQLITE_ERROR.  ^A subsequent call to sqlite3_result_error()
** or sqlite3_result_error16() resets the error code to SQLITE_ERROR.
**
** ^The sqlite3_result_error_toobig() interface causes SQLite to throw an
** error indicating that a string or BLOB is too long to represent.
**
** ^The sqlite3_result_error_nomem() interface causes SQLite to throw an
** error indicating that a memory allocation failed.
**
** ^The sqlite3_result_int() interface sets the return value
** of the application-defined function to be the 32-bit signed integer
** value given in the 2nd argument.
** ^The sqlite3_result_int64() interface sets the return value
** of the application-defined function to be the 64-bit signed integer
** value given in the 2nd argument.
**
** ^The sqlite3_result_null() interface sets the return value
** of the application-defined function to be NULL.
**
** ^The sqlite3_result_text(), sqlite3_result_text16(),
** sqlite3_result_text16le(), and sqlite3_result_text16be() interfaces
** set the return value of the application-defined function to be
** a text string which is represented as UTF-8, UTF-16 native byte order,
** UTF-16 little endian, or UTF-16 big endian, respectively.
** ^The sqlite3_result_text64() interface sets the return value of an
** application-defined function to be a text string in an encoding
** specified by the fifth (and last) parameter, which must be one
** of [SQLITE_UTF8], [SQLITE_UTF16], [SQLITE_UTF16BE], or [SQLITE_UTF16LE].
** ^SQLite takes the text result from the application from
** the 2nd parameter of the sqlite3_result_text* interfaces.
** ^If the 3rd parameter to the sqlite3_result_text* interfaces
** is negative, then SQLite takes result text from the 2nd parameter
** through the first zero character.
** ^If the 3rd parameter to the sqlite3_result_text* interfaces
** is non-negative, then as many bytes (not characters) of the text
** pointed to by the 2nd parameter are taken as the application-defined
** function result.  If the 3rd parameter is non-negative, then it
** must be the byte offset into the string where the NUL terminator would
** appear if the string where NUL terminated.  If any NUL characters occur
** in the string at a byte offset that is less than the value of the 3rd
** parameter, then the resulting string will contain embedded NULs and the
** result of expressions operating on strings with embedded NULs is undefined.
** ^If the 4th parameter to the sqlite3_result_text* interfaces
** or sqlite3_result_blob is a non-NULL pointer, then SQLite calls that
** function as the destructor on the text or BLOB result when it has
** finished using that result.
** ^If the 4th parameter to the sqlite3_result_text* interfaces or to
** sqlite3_result_blob is the special constant SQLITE_STATIC, then SQLite
** assumes that the text or BLOB result is in constant space and does not
** copy the content of the parameter nor call a destructor on the content
** when it has finished using that result.
** ^If the 4th parameter to the sqlite3_result_text* interfaces
** or sqlite3_result_blob is the special constant SQLITE_TRANSIENT
** then SQLite makes a copy of the result into space obtained
** from [sqlite3_malloc()] before it returns.
**
** ^For the sqlite3_result_text16(), sqlite3_result_text16le(), and
** sqlite3_result_text16be() routines, and for sqlite3_result_text64()
** when the encoding is not UTF8, if the input UTF16 begins with a
** byte-order mark (BOM, U+FEFF) then the BOM is removed from the
** string and the rest of the string is interpreted according to the
** byte-order specified by the BOM.  ^The byte-order specified by
** the BOM at the beginning of the text overrides the byte-order
** specified by the interface procedure.  ^So, for example, if
** sqlite3_result_text16le() is invoked with text that begins
** with bytes 0xfe, 0xff (a big-endian byte-order mark) then the
** first two bytes of input are skipped and the remaining input
** is interpreted as UTF16BE text.
**
** ^For UTF16 input text to the sqlite3_result_text16(),
** sqlite3_result_text16be(), sqlite3_result_text16le(), and
** sqlite3_result_text64() routines, if the text contains invalid
** UTF16 characters, the invalid characters might be converted
** into the unicode replacement character, U+FFFD.
**
** ^The sqlite3_result_value() interface sets the result of
** the application-defined function to be a copy of the
** [unprotected sqlite3_value] object specified by the 2nd parameter.  ^The
** sqlite3_result_value() interface makes a copy of the [sqlite3_value]
** so that the [sqlite3_value] specified in the parameter may change or
** be deallocated after sqlite3_result_value() returns without harm.
** ^A [protected sqlite3_value] object may always be used where an
** [unprotected sqlite3_value] object is required, so either
** kind of [sqlite3_value] object can be used with this interface.
**
** ^The sqlite3_result_pointer(C,P,T,D) interface sets the result to an
** SQL NULL value, just like [sqlite3_result_null(C)], except that it
** also associates the host-language pointer P or type T with that
** NULL value such that the pointer can be retrieved within an
** [application-defined SQL function] using [sqlite3_value_pointer()].
** ^If the D parameter is not NULL, then it is a pointer to a destructor
** for the P parameter.  ^SQLite invokes D with P as its only argument
** when SQLite is finished with P.  The T parameter should be a static
** string and preferably a string literal. The sqlite3_result_pointer()
** routine is part of the [pointer passing interface] added for SQLite 3.20.0.
**
** If these routines are called from within the different thread
** than the one containing the application-defined function that received
** the [sqlite3_context] pointer, the results are undefined.
*/
SQLITE_API void sqlite3_result_blob(sqlite3_context*, const void*, int, void(*)(void*));
SQLITE_API void sqlite3_result_blob64(sqlite3_context*,const void*,
                           sqlite3_uint64,void(*)(void*));
SQLITE_API void sqlite3_result_double(sqlite3_context*, double);
SQLITE_API void sqlite3_result_error(sqlite3_context*, const char*, int);
SQLITE_API void sqlite3_result_error16(sqlite3_context*, const void*, int);
SQLITE_API void sqlite3_result_error_toobig(sqlite3_context*);
SQLITE_API void sqlite3_result_error_nomem(sqlite3_context*);
SQLITE_API void sqlite3_result_error_code(sqlite3_context*, int);
SQLITE_API void sqlite3_result_int(sqlite3_context*, int);
SQLITE_API void sqlite3_result_int64(sqlite3_context*, sqlite3_int64);
SQLITE_API void sqlite3_result_null(sqlite3_context*);
SQLITE_API void sqlite3_result_text(sqlite3_context*, const char*, int, void(*)(void*));
SQLITE_API void sqlite3_result_text64(sqlite3_context*, const char*,sqlite3_uint64,
                           void(*)(void*), unsigned char encoding);
SQLITE_API void sqlite3_result_text16(sqlite3_context*, const void*, int, void(*)(void*));
SQLITE_API void sqlite3_result_text16le(sqlite3_context*, const void*, int,void(*)(void*));
SQLITE_API void sqlite3_result_text16be(sqlite3_context*, const void*, int,void(*)(void*));
SQLITE_API void sqlite3_result_value(sqlite3_context*, sqlite3_value*);
SQLITE_API void sqlite3_result_pointer(sqlite3_context*, void*,const char*,void(*)(void*));
SQLITE_API void sqlite3_result_zeroblob(sqlite3_context*, int n);
SQLITE_API int sqlite3_result_zeroblob64(sqlite3_context*, sqlite3_uint64 n);


/*
** CAPI3REF: Setting The Subtype Of An SQL Function
** METHOD: sqlite3_context
**
** The sqlite3_result_subtype(C,T) function causes the subtype of
** the result from the [application-defined SQL function] with
** [sqlite3_context] C to be the value T.  Only the lower 8 bits
** of the subtype T are preserved in current versions of SQLite;
** higher order bits are discarded.
** The number of subtype bytes preserved by SQLite might increase
** in future releases of SQLite.
*/
SQLITE_API void sqlite3_result_subtype(sqlite3_context*,unsigned int);

/*
** CAPI3REF: Define New Collating Sequences
** METHOD: sqlite3
**
** ^These functions add, remove, or modify a [collation] associated
** with the [database connection] specified as the first argument.
**
** ^The name of the collation is a UTF-8 string
** for sqlite3_create_collation() and sqlite3_create_collation_v2()
** and a UTF-16 string in native byte order for sqlite3_create_collation16().
** ^Collation names that compare equal according to [sqlite3_strnicmp()] are
** considered to be the same name.
**
** ^(The third argument (eTextRep) must be one of the constants:
** <ul>
** <li> [SQLITE_UTF8],
** <li> [SQLITE_UTF16LE],
** <li> [SQLITE_UTF16BE],
** <li> [SQLITE_UTF16], or
** <li> [SQLITE_UTF16_ALIGNED].
** </ul>)^
** ^The eTextRep argument determines the encoding of strings passed
** to the collating function callback, xCompare.
** ^The [SQLITE_UTF16] and [SQLITE_UTF16_ALIGNED] values for eTextRep
** force strings to be UTF16 with native byte order.
** ^The [SQLITE_UTF16_ALIGNED] value for eTextRep forces strings to begin
** on an even byte address.
**
** ^The fourth argument, pArg, is an application data pointer that is passed
** through as the first argument to the collating function callback.
**
** ^The fifth argument, xCompare, is a pointer to the collating function.
** ^Multiple collating functions can be registered using the same name but
** with different eTextRep parameters and SQLite will use whichever
** function requires the least amount of data transformation.
** ^If the xCompare argument is NULL then the collating function is
** deleted.  ^When all collating functions having the same name are deleted,
** that collation is no longer usable.
**
** ^The collating function callback is invoked with a copy of the pArg
** application data pointer and with two strings in the encoding specified
** by the eTextRep argument.  The two integer parameters to the collating
** function callback are the length of the two strings, in bytes. The collating
** function must return an integer that is negative, zero, or positive
** if the first string is less than, equal to, or greater than the second,
** respectively.  A collating function must always return the same answer
** given the same inputs.  If two or more collating functions are registered
** to the same collation name (using different eTextRep values) then all
** must give an equivalent answer when invoked with equivalent strings.
** The collating function must obey the following properties for all
** strings A, B, and C:
**
** <ol>
** <li> If A==B then B==A.
** <li> If A==B and B==C then A==C.
** <li> If A&lt;B THEN B&gt;A.
** <li> If A&lt;B and B&lt;C then A&lt;C.
** </ol>
**
** If a collating function fails any of the above constraints and that
** collating function is registered and used, then the behavior of SQLite
** is undefined.
**
** ^The sqlite3_create_collation_v2() works like sqlite3_create_collation()
** with the addition that the xDestroy callback is invoked on pArg when
** the collating function is deleted.
** ^Collating functions are deleted when they are overridden by later
** calls to the collation creation functions or when the
** [database connection] is closed using [sqlite3_close()].
**
** ^The xDestroy callback is <u>not</u> called if the
** sqlite3_create_collation_v2() function fails.  Applications that invoke
** sqlite3_create_collation_v2() with a non-NULL xDestroy argument should
** check the return code and dispose of the application data pointer
** themselves rather than expecting SQLite to deal with it for them.
** This is different from every other SQLite interface.  The inconsistency
** is unfortunate but cannot be changed without breaking backwards
** compatibility.
**
** See also:  [sqlite3_collation_needed()] and [sqlite3_collation_needed16()].
*/
SQLITE_API int sqlite3_create_collation(
  sqlite3*,
  const char *zName,
  int eTextRep,
  void *pArg,
  int(*xCompare)(void*,int,const void*,int,const void*)
);
SQLITE_API int sqlite3_create_collation_v2(
  sqlite3*,
  const char *zName,
  int eTextRep,
  void *pArg,
  int(*xCompare)(void*,int,const void*,int,const void*),
  void(*xDestroy)(void*)
);
SQLITE_API int sqlite3_create_collation16(
  sqlite3*,
  const void *zName,
  int eTextRep,
  void *pArg,
  int(*xCompare)(void*,int,const void*,int,const void*)
);

/*
** CAPI3REF: Collation Needed Callbacks
** METHOD: sqlite3
**
** ^To avoid having to register all collation sequences before a database
** can be used, a single callback function may be registered with the
** [database connection] to be invoked whenever an undefined collation
** sequence is required.
**
** ^If the function is registered using the sqlite3_collation_needed() API,
** then it is passed the names of undefined collation sequences as strings
** encoded in UTF-8. ^If sqlite3_collation_needed16() is used,
** the names are passed as UTF-16 in machine native byte order.
** ^A call to either function replaces the existing collation-needed callback.
**
** ^(When the callback is invoked, the first argument passed is a copy
** of the second argument to sqlite3_collation_needed() or
** sqlite3_collation_needed16().  The second argument is the database
** connection.  The third argument is one of [SQLITE_UTF8], [SQLITE_UTF16BE],
** or [SQLITE_UTF16LE], indicating the most desirable form of the collation
** sequence function required.  The fourth parameter is the name of the
** required collation sequence.)^
**
** The callback function should register the desired collation using
** [sqlite3_create_collation()], [sqlite3_create_collation16()], or
** [sqlite3_create_collation_v2()].
*/
SQLITE_API int sqlite3_collation_needed(
  sqlite3*,
  void*,
  void(*)(void*,sqlite3*,int eTextRep,const char*)
);
SQLITE_API int sqlite3_collation_needed16(
  sqlite3*,
  void*,
  void(*)(void*,sqlite3*,int eTextRep,const void*)
);

#ifdef SQLITE_ENABLE_CEROD
/*
** Specify the activation key for a CEROD database.  Unless
** activated, none of the CEROD routines will work.
*/
SQLITE_API void sqlite3_activate_cerod(
  const char *zPassPhrase        /* Activation phrase */
);
#endif

/*
** CAPI3REF: Suspend Execution For A Short Time
**
** The sqlite3_sleep() function causes the current thread to suspend execution
** for at least a number of milliseconds specified in its parameter.
**
** If the operating system does not support sleep requests with
** millisecond time resolution, then the time will be rounded up to
** the nearest second. The number of milliseconds of sleep actually
** requested from the operating system is returned.
**
** ^SQLite implements this interface by calling the xSleep()
** method of the default [sqlite3_vfs] object.  If the xSleep() method
** of the default VFS is not implemented correctly, or not implemented at
** all, then the behavior of sqlite3_sleep() may deviate from the description
** in the previous paragraphs.
*/
SQLITE_API int sqlite3_sleep(int);

/*
** CAPI3REF: Name Of The Folder Holding Temporary Files
**
** ^(If this global variable is made to point to a string which is
** the name of a folder (a.k.a. directory), then all temporary files
** created by SQLite when using a built-in [sqlite3_vfs | VFS]
** will be placed in that directory.)^  ^If this variable
** is a NULL pointer, then SQLite performs a search for an appropriate
** temporary file directory.
**
** Applications are strongly discouraged from using this global variable.
** It is required to set a temporary folder on Windows Runtime (WinRT).
** But for all other platforms, it is highly recommended that applications
** neither read nor write this variable.  This global variable is a relic
** that exists for backwards compatibility of legacy applications and should
** be avoided in new projects.
**
** It is not safe to read or modify this variable in more than one
** thread at a time.  It is not safe to read or modify this variable
** if a [database connection] is being used at the same time in a separate
** thread.
** It is intended that this variable be set once
** as part of process initialization and before any SQLite interface
** routines have been called and that this variable remain unchanged
** thereafter.
**
** ^The [temp_store_directory pragma] may modify this variable and cause
** it to point to memory obtained from [sqlite3_malloc].  ^Furthermore,
** the [temp_store_directory pragma] always assumes that any string
** that this variable points to is held in memory obtained from
** [sqlite3_malloc] and the pragma may attempt to free that memory
** using [sqlite3_free].
** Hence, if this variable is modified directly, either it should be
** made NULL or made to point to memory obtained from [sqlite3_malloc]
** or else the use of the [temp_store_directory pragma] should be avoided.
** Except when requested by the [temp_store_directory pragma], SQLite
** does not free the memory that sqlite3_temp_directory points to.  If
** the application wants that memory to be freed, it must do
** so itself, taking care to only do so after all [database connection]
** objects have been destroyed.
**
** <b>Note to Windows Runtime users:</b>  The temporary directory must be set
** prior to calling [sqlite3_open] or [sqlite3_open_v2].  Otherwise, various
** features that require the use of temporary files may fail.  Here is an
** example of how to do this using C++ with the Windows Runtime:
**
** <blockquote><pre>
** LPCWSTR zPath = Windows::Storage::ApplicationData::Current->
** &nbsp;     TemporaryFolder->Path->Data();
** char zPathBuf&#91;MAX_PATH + 1&#93;;
** memset(zPathBuf, 0, sizeof(zPathBuf));
** WideCharToMultiByte(CP_UTF8, 0, zPath, -1, zPathBuf, sizeof(zPathBuf),
** &nbsp;     NULL, NULL);
** sqlite3_temp_directory = sqlite3_mprintf("%s", zPathBuf);
** </pre></blockquote>
*/
SQLITE_API char *sqlite3_temp_directory;

/*
** CAPI3REF: Name Of The Folder Holding Database Files
**
** ^(If this global variable is made to point to a string which is
** the name of a folder (a.k.a. directory), then all database files
** specified with a relative pathname and created or accessed by
** SQLite when using a built-in windows [sqlite3_vfs | VFS] will be assumed
** to be relative to that directory.)^ ^If this variable is a NULL
** pointer, then SQLite assumes that all database files specified
** with a relative pathname are relative to the current directory
** for the process.  Only the windows VFS makes use of this global
** variable; it is ignored by the unix VFS.
**
** Changing the value of this variable while a database connection is
** open can result in a corrupt database.
**
** It is not safe to read or modify this variable in more than one
** thread at a time.  It is not safe to read or modify this variable
** if a [database connection] is being used at the same time in a separate
** thread.
** It is intended that this variable be set once
** as part of process initialization and before any SQLite interface
** routines have been called and that this variable remain unchanged
** thereafter.
**
** ^The [data_store_directory pragma] may modify this variable and cause
** it to point to memory obtained from [sqlite3_malloc].  ^Furthermore,
** the [data_store_directory pragma] always assumes that any string
** that this variable points to is held in memory obtained from
** [sqlite3_malloc] and the pragma may attempt to free that memory
** using [sqlite3_free].
** Hence, if this variable is modified directly, either it should be
** made NULL or made to point to memory obtained from [sqlite3_malloc]
** or else the use of the [data_store_directory pragma] should be avoided.
*/
SQLITE_API char *sqlite3_data_directory;

/*
** CAPI3REF: Win32 Specific Interface
**
** These interfaces are available only on Windows.  The
** [sqlite3_win32_set_directory] interface is used to set the value associated
** with the [sqlite3_temp_directory] or [sqlite3_data_directory] variable, to
** zValue, depending on the value of the type parameter.  The zValue parameter
** should be NULL to cause the previous value to be freed via [sqlite3_free];
** a non-NULL value will be copied into memory obtained from [sqlite3_malloc]
** prior to being used.  The [sqlite3_win32_set_directory] interface returns
** [SQLITE_OK] to indicate success, [SQLITE_ERROR] if the type is unsupported,
** or [SQLITE_NOMEM] if memory could not be allocated.  The value of the
** [sqlite3_data_directory] variable is intended to act as a replacement for
** the current directory on the sub-platforms of Win32 where that concept is
** not present, e.g. WinRT and UWP.  The [sqlite3_win32_set_directory8] and
** [sqlite3_win32_set_directory16] interfaces behave exactly the same as the
** sqlite3_win32_set_directory interface except the string parameter must be
** UTF-8 or UTF-16, respectively.
*/
SQLITE_API int sqlite3_win32_set_directory(
  unsigned long type, /* Identifier for directory being set or reset */
  void *zValue        /* New value for directory being set or reset */
);
SQLITE_API int sqlite3_win32_set_directory8(unsigned long type, const char *zValue);
SQLITE_API int sqlite3_win32_set_directory16(unsigned long type, const void *zValue);

/*
** CAPI3REF: Win32 Directory Types
**
** These macros are only available on Windows.  They define the allowed values
** for the type argument to the [sqlite3_win32_set_directory] interface.
*/
#define SQLITE_WIN32_DATA_DIRECTORY_TYPE  1
#define SQLITE_WIN32_TEMP_DIRECTORY_TYPE  2

/*
** CAPI3REF: Test For Auto-Commit Mode
** KEYWORDS: {autocommit mode}
** METHOD: sqlite3
**
** ^The sqlite3_get_autocommit() interface returns non-zero or
** zero if the given database connection is or is not in autocommit mode,
** respectively.  ^Autocommit mode is on by default.
** ^Autocommit mode is disabled by a [BEGIN] statement.
** ^Autocommit mode is re-enabled by a [COMMIT] or [ROLLBACK].
**
** If certain kinds of errors occur on a statement within a multi-statement
** transaction (errors including [SQLITE_FULL], [SQLITE_IOERR],
** [SQLITE_NOMEM], [SQLITE_BUSY], and [SQLITE_INTERRUPT]) then the
** transaction might be rolled back automatically.  The only way to
** find out whether SQLite automatically rolled back the transaction after
** an error is to use this function.
**
** If another thread changes the autocommit status of the database
** connection while this routine is running, then the return value
** is undefined.
*/
SQLITE_API int sqlite3_get_autocommit(sqlite3*);

/*
** CAPI3REF: Find The Database Handle Of A Prepared Statement
** METHOD: sqlite3_stmt
**
** ^The sqlite3_db_handle interface returns the [database connection] handle
** to which a [prepared statement] belongs.  ^The [database connection]
** returned by sqlite3_db_handle is the same [database connection]
** that was the first argument
** to the [sqlite3_prepare_v2()] call (or its variants) that was used to
** create the statement in the first place.
*/
SQLITE_API sqlite3 *sqlite3_db_handle(sqlite3_stmt*);

/*
** CAPI3REF: Return The Schema Name For A Database Connection
** METHOD: sqlite3
**
** ^The sqlite3_db_name(D,N) interface returns a pointer to the schema name
** for the N-th database on database connection D, or a NULL pointer of N is
** out of range.  An N value of 0 means the main database file.  An N of 1 is
** the "temp" schema.  Larger values of N correspond to various ATTACH-ed
** databases.
**
** Space to hold the string that is returned by sqlite3_db_name() is managed
** by SQLite itself.  The string might be deallocated by any operation that
** changes the schema, including [ATTACH] or [DETACH] or calls to
** [sqlite3_serialize()] or [sqlite3_deserialize()], even operations that
** occur on a different thread.  Applications that need to
** remember the string long-term should make their own copy.  Applications that
** are accessing the same database connection simultaneously on multiple
** threads should mutex-protect calls to this API and should make their own
** private copy of the result prior to releasing the mutex.
*/
SQLITE_API const char *sqlite3_db_name(sqlite3 *db, int N);

/*
** CAPI3REF: Return The Filename For A Database Connection
** METHOD: sqlite3
**
** ^The sqlite3_db_filename(D,N) interface returns a pointer to the filename
** associated with database N of connection D.
** ^If there is no attached database N on the database
** connection D, or if database N is a temporary or in-memory database, then
** this function will return either a NULL pointer or an empty string.
**
** ^The string value returned by this routine is owned and managed by
** the database connection.  ^The value will be valid until the database N
** is [DETACH]-ed or until the database connection closes.
**
** ^The filename returned by this function is the output of the
** xFullPathname method of the [VFS].  ^In other words, the filename
** will be an absolute pathname, even if the filename used
** to open the database originally was a URI or relative pathname.
**
** If the filename pointer returned by this routine is not NULL, then it
** can be used as the filename input parameter to these routines:
** <ul>
** <li> [sqlite3_uri_parameter()]
** <li> [sqlite3_uri_boolean()]
** <li> [sqlite3_uri_int64()]
** <li> [sqlite3_filename_database()]
** <li> [sqlite3_filename_journal()]
** <li> [sqlite3_filename_wal()]
** </ul>
*/
SQLITE_API const char *sqlite3_db_filename(sqlite3 *db, const char *zDbName);

/*
** CAPI3REF: Determine if a database is read-only
** METHOD: sqlite3
**
** ^The sqlite3_db_readonly(D,N) interface returns 1 if the database N
** of connection D is read-only, 0 if it is read/write, or -1 if N is not
** the name of a database on connection D.
*/
SQLITE_API int sqlite3_db_readonly(sqlite3 *db, const char *zDbName);

/*
** CAPI3REF: Determine the transaction state of a database
** METHOD: sqlite3
**
** ^The sqlite3_txn_state(D,S) interface returns the current
** [transaction state] of schema S in database connection D.  ^If S is NULL,
** then the highest transaction state of any schema on database connection D
** is returned.  Transaction states are (in order of lowest to highest):
** <ol>
** <li value="0"> SQLITE_TXN_NONE
** <li value="1"> SQLITE_TXN_READ
** <li value="2"> SQLITE_TXN_WRITE
** </ol>
** ^If the S argument to sqlite3_txn_state(D,S) is not the name of
** a valid schema, then -1 is returned.
*/
SQLITE_API int sqlite3_txn_state(sqlite3*,const char *zSchema);

/*
** CAPI3REF: Allowed return values from [sqlite3_txn_state()]
** KEYWORDS: {transaction state}
**
** These constants define the current transaction state of a database file.
** ^The [sqlite3_txn_state(D,S)] interface returns one of these
** constants in order to describe the transaction state of schema S
** in [database connection] D.
**
** <dl>
** [[SQLITE_TXN_NONE]] <dt>SQLITE_TXN_NONE</dt>
** <dd>The SQLITE_TXN_NONE state means that no transaction is currently
** pending.</dd>
**
** [[SQLITE_TXN_READ]] <dt>SQLITE_TXN_READ</dt>
** <dd>The SQLITE_TXN_READ state means that the database is currently
** in a read transaction.  Content has been read from the database file
** but nothing in the database file has changed.  The transaction state
** will advanced to SQLITE_TXN_WRITE if any changes occur and there are
** no other conflicting concurrent write transactions.  The transaction
** state will revert to SQLITE_TXN_NONE following a [ROLLBACK] or
** [COMMIT].</dd>
**
** [[SQLITE_TXN_WRITE]] <dt>SQLITE_TXN_WRITE</dt>
** <dd>The SQLITE_TXN_WRITE state means that the database is currently
** in a write transaction.  Content has been written to the database file
** but has not yet committed.  The transaction state will change to
** to SQLITE_TXN_NONE at the next [ROLLBACK] or [COMMIT].</dd>
*/
#define SQLITE_TXN_NONE  0
#define SQLITE_TXN_READ  1
#define SQLITE_TXN_WRITE 2

/*
** CAPI3REF: Find the next prepared statement
** METHOD: sqlite3
**
** ^This interface returns a pointer to the next [prepared statement] after
** pStmt associated with the [database connection] pDb.  ^If pStmt is NULL
** then this interface returns a pointer to the first prepared statement
** associated with the database connection pDb.  ^If no prepared statement
** satisfies the conditions of this routine, it returns NULL.
**
** The [database connection] pointer D in a call to
** [sqlite3_next_stmt(D,S)] must refer to an open database
** connection and in particular must not be a NULL pointer.
*/
SQLITE_API sqlite3_stmt *sqlite3_next_stmt(sqlite3 *pDb, sqlite3_stmt *pStmt);

/*
** CAPI3REF: Commit And Rollback Notification Callbacks
** METHOD: sqlite3
**
** ^The sqlite3_commit_hook() interface registers a callback
** function to be invoked whenever a transaction is [COMMIT | committed].
** ^Any callback set by a previous call to sqlite3_commit_hook()
** for the same database connection is overridden.
** ^The sqlite3_rollback_hook() interface registers a callback
** function to be invoked whenever a transaction is [ROLLBACK | rolled back].
** ^Any callback set by a previous call to sqlite3_rollback_hook()
** for the same database connection is overridden.
** ^The pArg argument is passed through to the callback.
** ^If the callback on a commit hook function returns non-zero,
** then the commit is converted into a rollback.
**
** ^The sqlite3_commit_hook(D,C,P) and sqlite3_rollback_hook(D,C,P) functions
** return the P argument from the previous call of the same function
** on the same [database connection] D, or NULL for
** the first call for each function on D.
**
** The commit and rollback hook callbacks are not reentrant.
** The callback implementation must not do anything that will modify
** the database connection that invoked the callback.  Any actions
** to modify the database connection must be deferred until after the
** completion of the [sqlite3_step()] call that triggered the commit
** or rollback hook in the first place.
** Note that running any other SQL statements, including SELECT statements,
** or merely calling [sqlite3_prepare_v2()] and [sqlite3_step()] will modify
** the database connections for the meaning of "modify" in this paragraph.
**
** ^Registering a NULL function disables the callback.
**
** ^When the commit hook callback routine returns zero, the [COMMIT]
** operation is allowed to continue normally.  ^If the commit hook
** returns non-zero, then the [COMMIT] is converted into a [ROLLBACK].
** ^The rollback hook is invoked on a rollback that results from a commit
** hook returning non-zero, just as it would be with any other rollback.
**
** ^For the purposes of this API, a transaction is said to have been
** rolled back if an explicit "ROLLBACK" statement is executed, or
** an error or constraint causes an implicit rollback to occur.
** ^The rollback callback is not invoked if a transaction is
** automatically rolled back because the database connection is closed.
**
** See also the [sqlite3_update_hook()] interface.
*/
SQLITE_API void *sqlite3_commit_hook(sqlite3*, int(*)(void*), void*);
SQLITE_API void *sqlite3_rollback_hook(sqlite3*, void(*)(void *), void*);

/*
** CAPI3REF: Autovacuum Compaction Amount Callback
** METHOD: sqlite3
**
** ^The sqlite3_autovacuum_pages(D,C,P,X) interface registers a callback
** function C that is invoked prior to each autovacuum of the database
** file.  ^The callback is passed a copy of the generic data pointer (P),
** the schema-name of the attached database that is being autovacuumed,
** the the size of the database file in pages, the number of free pages,
** and the number of bytes per page, respectively.  The callback should
** return the number of free pages that should be removed by the
** autovacuum.  ^If the callback returns zero, then no autovacuum happens.
** ^If the value returned is greater than or equal to the number of
** free pages, then a complete autovacuum happens.
**
** <p>^If there are multiple ATTACH-ed database files that are being
** modified as part of a transaction commit, then the autovacuum pages
** callback is invoked separately for each file.
**
** <p><b>The callback is not reentrant.</b> The callback function should
** not attempt to invoke any other SQLite interface.  If it does, bad
** things may happen, including segmentation faults and corrupt database
** files.  The callback function should be a simple function that
** does some arithmetic on its input parameters and returns a result.
**
** ^The X parameter to sqlite3_autovacuum_pages(D,C,P,X) is an optional
** destructor for the P parameter.  ^If X is not NULL, then X(P) is
** invoked whenever the database connection closes or when the callback
** is overwritten by another invocation of sqlite3_autovacuum_pages().
**
** <p>^There is only one autovacuum pages callback per database connection.
** ^Each call to the sqlite3_autovacuum_pages() interface overrides all
** previous invocations for that database connection.  ^If the callback
** argument (C) to sqlite3_autovacuum_pages(D,C,P,X) is a NULL pointer,
** then the autovacuum steps callback is cancelled.  The return value
** from sqlite3_autovacuum_pages() is normally SQLITE_OK, but might
** be some other error code if something goes wrong.  The current
** implementation will only return SQLITE_OK or SQLITE_MISUSE, but other
** return codes might be added in future releases.
**
** <p>If no autovacuum pages callback is specified (the usual case) or
** a NULL pointer is provided for the callback,
** then the default behavior is to vacuum all free pages.  So, in other
** words, the default behavior is the same as if the callback function
** were something like this:
**
** <blockquote><pre>
** &nbsp;   unsigned int demonstration_autovac_pages_callback(
** &nbsp;     void *pClientData,
** &nbsp;     const char *zSchema,
** &nbsp;     unsigned int nDbPage,
** &nbsp;     unsigned int nFreePage,
** &nbsp;     unsigned int nBytePerPage
** &nbsp;   ){
** &nbsp;     return nFreePage;
** &nbsp;   }
** </pre></blockquote>
*/
SQLITE_API int sqlite3_autovacuum_pages(
  sqlite3 *db,
  unsigned int(*)(void*,const char*,unsigned int,unsigned int,unsigned int),
  void*,
  void(*)(void*)
);


/*
** CAPI3REF: Data Change Notification Callbacks
** METHOD: sqlite3
**
** ^The sqlite3_update_hook() interface registers a callback function
** with the [database connection] identified by the first argument
** to be invoked whenever a row is updated, inserted or deleted in
** a [rowid table].
** ^Any callback set by a previous call to this function
** for the same database connection is overridden.
**
** ^The second argument is a pointer to the function to invoke when a
** row is updated, inserted or deleted in a rowid table.
** ^The first argument to the callback is a copy of the third argument
** to sqlite3_update_hook().
** ^The second callback argument is one of [SQLITE_INSERT], [SQLITE_DELETE],
** or [SQLITE_UPDATE], depending on the operation that caused the callback
** to be invoked.
** ^The third and fourth arguments to the callback contain pointers to the
** database and table name containing the affected row.
** ^The final callback parameter is the [rowid] of the row.
** ^In the case of an update, this is the [rowid] after the update takes place.
**
** ^(The update hook is not invoked when internal system tables are
** modified (i.e. sqlite_sequence).)^
** ^The update hook is not invoked when [WITHOUT ROWID] tables are modified.
**
** ^In the current implementation, the update hook
** is not invoked when conflicting rows are deleted because of an
** [ON CONFLICT | ON CONFLICT REPLACE] clause.  ^Nor is the update hook
** invoked when rows are deleted using the [truncate optimization].
** The exceptions defined in this paragraph might change in a future
** release of SQLite.
**
** The update hook implementation must not do anything that will modify
** the database connection that invoked the update hook.  Any actions
** to modify the database connection must be deferred until after the
** completion of the [sqlite3_step()] call that triggered the update hook.
** Note that [sqlite3_prepare_v2()] and [sqlite3_step()] both modify their
** database connections for the meaning of "modify" in this paragraph.
**
** ^The sqlite3_update_hook(D,C,P) function
** returns the P argument from the previous call
** on the same [database connection] D, or NULL for
** the first call on D.
**
** See also the [sqlite3_commit_hook()], [sqlite3_rollback_hook()],
** and [sqlite3_preupdate_hook()] interfaces.
*/
SQLITE_API void *sqlite3_update_hook(
  sqlite3*,
  void(*)(void *,int ,char const *,char const *,sqlite3_int64),
  void*
);

/*
** CAPI3REF: Enable Or Disable Shared Pager Cache
**
** ^(This routine enables or disables the sharing of the database cache
** and schema data structures between [database connection | connections]
** to the same database. Sharing is enabled if the argument is true
** and disabled if the argument is false.)^
**
** ^Cache sharing is enabled and disabled for an entire process.
** This is a change as of SQLite [version 3.5.0] ([dateof:3.5.0]).
** In prior versions of SQLite,
** sharing was enabled or disabled for each thread separately.
**
** ^(The cache sharing mode set by this interface effects all subsequent
** calls to [sqlite3_open()], [sqlite3_open_v2()], and [sqlite3_open16()].
** Existing database connections continue to use the sharing mode
** that was in effect at the time they were opened.)^
**
** ^(This routine returns [SQLITE_OK] if shared cache was enabled or disabled
** successfully.  An [error code] is returned otherwise.)^
**
** ^Shared cache is disabled by default. It is recommended that it stay
** that way.  In other words, do not use this routine.  This interface
** continues to be provided for historical compatibility, but its use is
** discouraged.  Any use of shared cache is discouraged.  If shared cache
** must be used, it is recommended that shared cache only be enabled for
** individual database connections using the [sqlite3_open_v2()] interface
** with the [SQLITE_OPEN_SHAREDCACHE] flag.
**
** Note: This method is disabled on MacOS X 10.7 and iOS version 5.0
** and will always return SQLITE_MISUSE. On those systems,
** shared cache mode should be enabled per-database connection via
** [sqlite3_open_v2()] with [SQLITE_OPEN_SHAREDCACHE].
**
** This interface is threadsafe on processors where writing a
** 32-bit integer is atomic.
**
** See Also:  [SQLite Shared-Cache Mode]
*/
SQLITE_API int sqlite3_enable_shared_cache(int);

/*
** CAPI3REF: Attempt To Free Heap Memory
**
** ^The sqlite3_release_memory() interface attempts to free N bytes
** of heap memory by deallocating non-essential memory allocations
** held by the database library.   Memory used to cache database
** pages to improve performance is an example of non-essential memory.
** ^sqlite3_release_memory() returns the number of bytes actually freed,
** which might be more or less than the amount requested.
** ^The sqlite3_release_memory() routine is a no-op returning zero
** if SQLite is not compiled with [SQLITE_ENABLE_MEMORY_MANAGEMENT].
**
** See also: [sqlite3_db_release_memory()]
*/
SQLITE_API int sqlite3_release_memory(int);

/*
** CAPI3REF: Free Memory Used By A Database Connection
** METHOD: sqlite3
**
** ^The sqlite3_db_release_memory(D) interface attempts to free as much heap
** memory as possible from database connection D. Unlike the
** [sqlite3_release_memory()] interface, this interface is in effect even
** when the [SQLITE_ENABLE_MEMORY_MANAGEMENT] compile-time option is
** omitted.
**
** See also: [sqlite3_release_memory()]
*/
SQLITE_API int sqlite3_db_release_memory(sqlite3*);

/*
** CAPI3REF: Impose A Limit On Heap Size
**
** These interfaces impose limits on the amount of heap memory that will be
** by all database connections within a single process.
**
** ^The sqlite3_soft_heap_limit64() interface sets and/or queries the
** soft limit on the amount of heap memory that may be allocated by SQLite.
** ^SQLite strives to keep heap memory utilization below the soft heap
** limit by reducing the number of pages held in the page cache
** as heap memory usages approaches the limit.
** ^The soft heap limit is "soft" because even though SQLite strives to stay
** below the limit, it will exceed the limit rather than generate
** an [SQLITE_NOMEM] error.  In other words, the soft heap limit
** is advisory only.
**
** ^The sqlite3_hard_heap_limit64(N) interface sets a hard upper bound of
** N bytes on the amount of memory that will be allocated.  ^The
** sqlite3_hard_heap_limit64(N) interface is similar to
** sqlite3_soft_heap_limit64(N) except that memory allocations will fail
** when the hard heap limit is reached.
**
** ^The return value from both sqlite3_soft_heap_limit64() and
** sqlite3_hard_heap_limit64() is the size of
** the heap limit prior to the call, or negative in the case of an
** error.  ^If the argument N is negative
** then no change is made to the heap limit.  Hence, the current
** size of heap limits can be determined by invoking
** sqlite3_soft_heap_limit64(-1) or sqlite3_hard_heap_limit(-1).
**
** ^Setting the heap limits to zero disables the heap limiter mechanism.
**
** ^The soft heap limit may not be greater than the hard heap limit.
** ^If the hard heap limit is enabled and if sqlite3_soft_heap_limit(N)
** is invoked with a value of N that is greater than the hard heap limit,
** the the soft heap limit is set to the value of the hard heap limit.
** ^The soft heap limit is automatically enabled whenever the hard heap
** limit is enabled. ^When sqlite3_hard_heap_limit64(N) is invoked and
** the soft heap limit is outside the range of 1..N, then the soft heap
** limit is set to N.  ^Invoking sqlite3_soft_heap_limit64(0) when the
** hard heap limit is enabled makes the soft heap limit equal to the
** hard heap limit.
**
** The memory allocation limits can also be adjusted using
** [PRAGMA soft_heap_limit] and [PRAGMA hard_heap_limit].
**
** ^(The heap limits are not enforced in the current implementation
** if one or more of following conditions are true:
**
** <ul>
** <li> The limit value is set to zero.
** <li> Memory accounting is disabled using a combination of the
**      [sqlite3_config]([SQLITE_CONFIG_MEMSTATUS],...) start-time option and
**      the [SQLITE_DEFAULT_MEMSTATUS] compile-time option.
** <li> An alternative page cache implementation is specified using
**      [sqlite3_config]([SQLITE_CONFIG_PCACHE2],...).
** <li> The page cache allocates from its own memory pool supplied
**      by [sqlite3_config]([SQLITE_CONFIG_PAGECACHE],...) rather than
**      from the heap.
** </ul>)^
**
** The circumstances under which SQLite will enforce the heap limits may
** changes in future releases of SQLite.
*/
SQLITE_API sqlite3_int64 sqlite3_soft_heap_limit64(sqlite3_int64 N);
SQLITE_API sqlite3_int64 sqlite3_hard_heap_limit64(sqlite3_int64 N);

/*
** CAPI3REF: Deprecated Soft Heap Limit Interface
** DEPRECATED
**
** This is a deprecated version of the [sqlite3_soft_heap_limit64()]
** interface.  This routine is provided for historical compatibility
** only.  All new applications should use the
** [sqlite3_soft_heap_limit64()] interface rather than this one.
*/
SQLITE_API SQLITE_DEPRECATED void sqlite3_soft_heap_limit(int N);


/*
** CAPI3REF: Extract Metadata About A Column Of A Table
** METHOD: sqlite3
**
** ^(The sqlite3_table_column_metadata(X,D,T,C,....) routine returns
** information about column C of table T in database D
** on [database connection] X.)^  ^The sqlite3_table_column_metadata()
** interface returns SQLITE_OK and fills in the non-NULL pointers in
** the final five arguments with appropriate values if the specified
** column exists.  ^The sqlite3_table_column_metadata() interface returns
** SQLITE_ERROR if the specified column does not exist.
** ^If the column-name parameter to sqlite3_table_column_metadata() is a
** NULL pointer, then this routine simply checks for the existence of the
** table and returns SQLITE_OK if the table exists and SQLITE_ERROR if it
** does not.  If the table name parameter T in a call to
** sqlite3_table_column_metadata(X,D,T,C,...) is NULL then the result is
** undefined behavior.
**
** ^The column is identified by the second, third and fourth parameters to
** this function. ^(The second parameter is either the name of the database
** (i.e. "main", "temp", or an attached database) containing the specified
** table or NULL.)^ ^If it is NULL, then all attached databases are searched
** for the table using the same algorithm used by the database engine to
** resolve unqualified table references.
**
** ^The third and fourth parameters to this function are the table and column
** name of the desired column, respectively.
**
** ^Metadata is returned by writing to the memory locations passed as the 5th
** and subsequent parameters to this function. ^Any of these arguments may be
** NULL, in which case the corresponding element of metadata is omitted.
**
** ^(<blockquote>
** <table border="1">
** <tr><th> Parameter <th> Output<br>Type <th>  Description
**
** <tr><td> 5th <td> const char* <td> Data type
** <tr><td> 6th <td> const char* <td> Name of default collation sequence
** <tr><td> 7th <td> int         <td> True if column has a NOT NULL constraint
** <tr><td> 8th <td> int         <td> True if column is part of the PRIMARY KEY
** <tr><td> 9th <td> int         <td> True if column is [AUTOINCREMENT]
** </table>
** </blockquote>)^
**
** ^The memory pointed to by the character pointers returned for the
** declaration type and collation sequence is valid until the next
** call to any SQLite API function.
**
** ^If the specified table is actually a view, an [error code] is returned.
**
** ^If the specified column is "rowid", "oid" or "_rowid_" and the table
** is not a [WITHOUT ROWID] table and an
** [INTEGER PRIMARY KEY] column has been explicitly declared, then the output
** parameters are set for the explicitly declared column. ^(If there is no
** [INTEGER PRIMARY KEY] column, then the outputs
** for the [rowid] are set as follows:
**
** <pre>
**     data type: "INTEGER"
**     collation sequence: "BINARY"
**     not null: 0
**     primary key: 1
**     auto increment: 0
** </pre>)^
**
** ^This function causes all database schemas to be read from disk and
** parsed, if that has not already been done, and returns an error if
** any errors are encountered while loading the schema.
*/
SQLITE_API int sqlite3_table_column_metadata(
  sqlite3 *db,                /* Connection handle */
  const char *zDbName,        /* Database name or NULL */
  const char *zTableName,     /* Table name */
  const char *zColumnName,    /* Column name */
  char const **pzDataType,    /* OUTPUT: Declared data type */
  char const **pzCollSeq,     /* OUTPUT: Collation sequence name */
  int *pNotNull,              /* OUTPUT: True if NOT NULL constraint exists */
  int *pPrimaryKey,           /* OUTPUT: True if column part of PK */
  int *pAutoinc               /* OUTPUT: True if column is auto-increment */
);

/*
** CAPI3REF: Load An Extension
** METHOD: sqlite3
**
** ^This interface loads an SQLite extension library from the named file.
**
** ^The sqlite3_load_extension() interface attempts to load an
** [SQLite extension] library contained in the file zFile.  If
** the file cannot be loaded directly, attempts are made to load
** with various operating-system specific extensions added.
** So for example, if "samplelib" cannot be loaded, then names like
** "samplelib.so" or "samplelib.dylib" or "samplelib.dll" might
** be tried also.
**
** ^The entry point is zProc.
** ^(zProc may be 0, in which case SQLite will try to come up with an
** entry point name on its own.  It first tries "sqlite3_extension_init".
** If that does not work, it constructs a name "sqlite3_X_init" where the
** X is consists of the lower-case equivalent of all ASCII alphabetic
** characters in the filename from the last "/" to the first following
** "." and omitting any initial "lib".)^
** ^The sqlite3_load_extension() interface returns
** [SQLITE_OK] on success and [SQLITE_ERROR] if something goes wrong.
** ^If an error occurs and pzErrMsg is not 0, then the
** [sqlite3_load_extension()] interface shall attempt to
** fill *pzErrMsg with error message text stored in memory
** obtained from [sqlite3_malloc()]. The calling function
** should free this memory by calling [sqlite3_free()].
**
** ^Extension loading must be enabled using
** [sqlite3_enable_load_extension()] or
** [sqlite3_db_config](db,[SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION],1,NULL)
** prior to calling this API,
** otherwise an error will be returned.
**
** <b>Security warning:</b> It is recommended that the
** [SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION] method be used to enable only this
** interface.  The use of the [sqlite3_enable_load_extension()] interface
** should be avoided.  This will keep the SQL function [load_extension()]
** disabled and prevent SQL injections from giving attackers
** access to extension loading capabilities.
**
** See also the [load_extension() SQL function].
*/
SQLITE_API int sqlite3_load_extension(
  sqlite3 *db,          /* Load the extension into this database connection */
  const char *zFile,    /* Name of the shared library containing extension */
  const char *zProc,    /* Entry point.  Derived from zFile if 0 */
  char **pzErrMsg       /* Put error message here if not 0 */
);

/*
** CAPI3REF: Enable Or Disable Extension Loading
** METHOD: sqlite3
**
** ^So as not to open security holes in older applications that are
** unprepared to deal with [extension loading], and as a means of disabling
** [extension loading] while evaluating user-entered SQL, the following API
** is provided to turn the [sqlite3_load_extension()] mechanism on and off.
**
** ^Extension loading is off by default.
** ^Call the sqlite3_enable_load_extension() routine with onoff==1
** to turn extension loading on and call it with onoff==0 to turn
** it back off again.
**
** ^This interface enables or disables both the C-API
** [sqlite3_load_extension()] and the SQL function [load_extension()].
** ^(Use [sqlite3_db_config](db,[SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION],..)
** to enable or disable only the C-API.)^
**
** <b>Security warning:</b> It is recommended that extension loading
** be enabled using the [SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION] method
** rather than this interface, so the [load_extension()] SQL function
** remains disabled. This will prevent SQL injections from giving attackers
** access to extension loading capabilities.
*/
SQLITE_API int sqlite3_enable_load_extension(sqlite3 *db, int onoff);

/*
** CAPI3REF: Automatically Load Statically Linked Extensions
**
** ^This interface causes the xEntryPoint() function to be invoked for
** each new [database connection] that is created.  The idea here is that
** xEntryPoint() is the entry point for a statically linked [SQLite extension]
** that is to be automatically loaded into all new database connections.
**
** ^(Even though the function prototype shows that xEntryPoint() takes
** no arguments and returns void, SQLite invokes xEntryPoint() with three
** arguments and expects an integer result as if the signature of the
** entry point where as follows:
**
** <blockquote><pre>
** &nbsp;  int xEntryPoint(
** &nbsp;    sqlite3 *db,
** &nbsp;    const char **pzErrMsg,
** &nbsp;    const struct sqlite3_api_routines *pThunk
** &nbsp;  );
** </pre></blockquote>)^
**
** If the xEntryPoint routine encounters an error, it should make *pzErrMsg
** point to an appropriate error message (obtained from [sqlite3_mprintf()])
** and return an appropriate [error code].  ^SQLite ensures that *pzErrMsg
** is NULL before calling the xEntryPoint().  ^SQLite will invoke
** [sqlite3_free()] on *pzErrMsg after xEntryPoint() returns.  ^If any
** xEntryPoint() returns an error, the [sqlite3_open()], [sqlite3_open16()],
** or [sqlite3_open_v2()] call that provoked the xEntryPoint() will fail.
**
** ^Calling sqlite3_auto_extension(X) with an entry point X that is already
** on the list of automatic extensions is a harmless no-op. ^No entry point
** will be called more than once for each database connection that is opened.
**
** See also: [sqlite3_reset_auto_extension()]
** and [sqlite3_cancel_auto_extension()]
*/
SQLITE_API int sqlite3_auto_extension(void(*xEntryPoint)(void));

/*
** CAPI3REF: Cancel Automatic Extension Loading
**
** ^The [sqlite3_cancel_auto_extension(X)] interface unregisters the
** initialization routine X that was registered using a prior call to
** [sqlite3_auto_extension(X)].  ^The [sqlite3_cancel_auto_extension(X)]
** routine returns 1 if initialization routine X was successfully
** unregistered and it returns 0 if X was not on the list of initialization
** routines.
*/
SQLITE_API int sqlite3_cancel_auto_extension(void(*xEntryPoint)(void));

/*
** CAPI3REF: Reset Automatic Extension Loading
**
** ^This interface disables all automatic extensions previously
** registered using [sqlite3_auto_extension()].
*/
SQLITE_API void sqlite3_reset_auto_extension(void);

/*
** The interface to the virtual-table mechanism is currently considered
** to be experimental.  The interface might change in incompatible ways.
** If this is a problem for you, do not use the interface at this time.
**
** When the virtual-table mechanism stabilizes, we will declare the
** interface fixed, support it indefinitely, and remove this comment.
*/

/*
** Structures used by the virtual table interface
*/
typedef struct sqlite3_vtab sqlite3_vtab;
typedef struct sqlite3_index_info sqlite3_index_info;
typedef struct sqlite3_vtab_cursor sqlite3_vtab_cursor;
typedef struct sqlite3_module sqlite3_module;

/*
** CAPI3REF: Virtual Table Object
** KEYWORDS: sqlite3_module {virtual table module}
**
** This structure, sometimes called a "virtual table module",
** defines the implementation of a [virtual table].
** This structure consists mostly of methods for the module.
**
** ^A virtual table module is created by filling in a persistent
** instance of this structure and passing a pointer to that instance
** to [sqlite3_create_module()] or [sqlite3_create_module_v2()].
** ^The registration remains valid until it is replaced by a different
** module or until the [database connection] closes.  The content
** of this structure must not change while it is registered with
** any database connection.
*/
struct sqlite3_module {
  int iVersion;
  int (*xCreate)(sqlite3*, void *pAux,
               int argc, const char *const*argv,
               sqlite3_vtab **ppVTab, char**);
  int (*xConnect)(sqlite3*, void *pAux,
               int argc, const char *const*argv,
               sqlite3_vtab **ppVTab, char**);
  int (*xBestIndex)(sqlite3_vtab *pVTab, sqlite3_index_info*);
  int (*xDisconnect)(sqlite3_vtab *pVTab);
  int (*xDestroy)(sqlite3_vtab *pVTab);
  int (*xOpen)(sqlite3_vtab *pVTab, sqlite3_vtab_cursor **ppCursor);
  int (*xClose)(sqlite3_vtab_cursor*);
  int (*xFilter)(sqlite3_vtab_cursor*, int idxNum, const char *idxStr,
                int argc, sqlite3_value **argv);
  int (*xNext)(sqlite3_vtab_cursor*);
  int (*xEof)(sqlite3_vtab_cursor*);
  int (*xColumn)(sqlite3_vtab_cursor*, sqlite3_context*, int);
  int (*xRowid)(sqlite3_vtab_cursor*, sqlite3_int64 *pRowid);
  int (*xUpdate)(sqlite3_vtab *, int, sqlite3_value **, sqlite3_int64 *);
  int (*xBegin)(sqlite3_vtab *pVTab);
  int (*xSync)(sqlite3_vtab *pVTab);
  int (*xCommit)(sqlite3_vtab *pVTab);
  int (*xRollback)(sqlite3_vtab *pVTab);
  int (*xFindFunction)(sqlite3_vtab *pVtab, int nArg, const char *zName,
                       void (**pxFunc)(sqlite3_context*,int,sqlite3_value**),
                       void **ppArg);
  int (*xRename)(sqlite3_vtab *pVtab, const char *zNew);
  /* The methods above are in version 1 of the sqlite_module object. Those
  ** below are for version 2 and greater. */
  int (*xSavepoint)(sqlite3_vtab *pVTab, int);
  int (*xRelease)(sqlite3_vtab *pVTab, int);
  int (*xRollbackTo)(sqlite3_vtab *pVTab, int);
  /* The methods above are in versions 1 and 2 of the sqlite_module object.
  ** Those below are for version 3 and greater. */
  int (*xShadowName)(const char*);
};

/*
** CAPI3REF: Virtual Table Indexing Information
** KEYWORDS: sqlite3_index_info
**
** The sqlite3_index_info structure and its substructures is used as part
** of the [virtual table] interface to
** pass information into and receive the reply from the [xBestIndex]
** method of a [virtual table module].  The fields under **Inputs** are the
** inputs to xBestIndex and are read-only.  xBestIndex inserts its
** results into the **Outputs** fields.
**
** ^(The aConstraint[] array records WHERE clause constraints of the form:
**
** <blockquote>column OP expr</blockquote>
**
** where OP is =, &lt;, &lt;=, &gt;, or &gt;=.)^  ^(The particular operator is
** stored in aConstraint[].op using one of the
** [SQLITE_INDEX_CONSTRAINT_EQ | SQLITE_INDEX_CONSTRAINT_ values].)^
** ^(The index of the column is stored in
** aConstraint[].iColumn.)^  ^(aConstraint[].usable is TRUE if the
** expr on the right-hand side can be evaluated (and thus the constraint
** is usable) and false if it cannot.)^
**
** ^The optimizer automatically inverts terms of the form "expr OP column"
** and makes other simplifications to the WHERE clause in an attempt to
** get as many WHERE clause terms into the form shown above as possible.
** ^The aConstraint[] array only reports WHERE clause terms that are
** relevant to the particular virtual table being queried.
**
** ^Information about the ORDER BY clause is stored in aOrderBy[].
** ^Each term of aOrderBy records a column of the ORDER BY clause.
**
** The colUsed field indicates which columns of the virtual table may be
** required by the current scan. Virtual table columns are numbered from
** zero in the order in which they appear within the CREATE TABLE statement
** passed to sqlite3_declare_vtab(). For the first 63 columns (columns 0-62),
** the corresponding bit is set within the colUsed mask if the column may be
** required by SQLite. If the table has at least 64 columns and any column
** to the right of the first 63 is required, then bit 63 of colUsed is also
** set. In other words, column iCol may be required if the expression
** (colUsed & ((sqlite3_uint64)1 << (iCol>=63 ? 63 : iCol))) evaluates to
** non-zero.
**
** The [xBestIndex] method must fill aConstraintUsage[] with information
** about what parameters to pass to xFilter.  ^If argvIndex>0 then
** the right-hand side of the corresponding aConstraint[] is evaluated
** and becomes the argvIndex-th entry in argv.  ^(If aConstraintUsage[].omit
** is true, then the constraint is assumed to be fully handled by the
** virtual table and might not be checked again by the byte code.)^ ^(The
** aConstraintUsage[].omit flag is an optimization hint. When the omit flag
** is left in its default setting of false, the constraint will always be
** checked separately in byte code.  If the omit flag is change to true, then
** the constraint may or may not be checked in byte code.  In other words,
** when the omit flag is true there is no guarantee that the constraint will
** not be checked again using byte code.)^
**
** ^The idxNum and idxPtr values are recorded and passed into the
** [xFilter] method.
** ^[sqlite3_free()] is used to free idxPtr if and only if
** needToFreeIdxPtr is true.
**
** ^The orderByConsumed means that output from [xFilter]/[xNext] will occur in
** the correct order to satisfy the ORDER BY clause so that no separate
** sorting step is required.
**
** ^The estimatedCost value is an estimate of the cost of a particular
** strategy. A cost of N indicates that the cost of the strategy is similar
** to a linear scan of an SQLite table with N rows. A cost of log(N)
** indicates that the expense of the operation is similar to that of a
** binary search on a unique indexed field of an SQLite table with N rows.
**
** ^The estimatedRows value is an estimate of the number of rows that
** will be returned by the strategy.
**
** The xBestIndex method may optionally populate the idxFlags field with a
** mask of SQLITE_INDEX_SCAN_* flags. Currently there is only one such flag -
** SQLITE_INDEX_SCAN_UNIQUE. If the xBestIndex method sets this flag, SQLite
** assumes that the strategy may visit at most one row.
**
** Additionally, if xBestIndex sets the SQLITE_INDEX_SCAN_UNIQUE flag, then
** SQLite also assumes that if a call to the xUpdate() method is made as
** part of the same statement to delete or update a virtual table row and the
** implementation returns SQLITE_CONSTRAINT, then there is no need to rollback
** any database changes. In other words, if the xUpdate() returns
** SQLITE_CONSTRAINT, the database contents must be exactly as they were
** before xUpdate was called. By contrast, if SQLITE_INDEX_SCAN_UNIQUE is not
** set and xUpdate returns SQLITE_CONSTRAINT, any database changes made by
** the xUpdate method are automatically rolled back by SQLite.
**
** IMPORTANT: The estimatedRows field was added to the sqlite3_index_info
** structure for SQLite [version 3.8.2] ([dateof:3.8.2]).
** If a virtual table extension is
** used with an SQLite version earlier than 3.8.2, the results of attempting
** to read or write the estimatedRows field are undefined (but are likely
** to include crashing the application). The estimatedRows field should
** therefore only be used if [sqlite3_libversion_number()] returns a
** value greater than or equal to 3008002. Similarly, the idxFlags field
** was added for [version 3.9.0] ([dateof:3.9.0]).
** It may therefore only be used if
** sqlite3_libversion_number() returns a value greater than or equal to
** 3009000.
*/
struct sqlite3_index_info {
  /* Inputs */
  int nConstraint;           /* Number of entries in aConstraint */
  struct sqlite3_index_constraint {
     int iColumn;              /* Column constrained.  -1 for ROWID */
     unsigned char op;         /* Constraint operator */
     unsigned char usable;     /* True if this constraint is usable */
     int iTermOffset;          /* Used internally - xBestIndex should ignore */
  } *aConstraint;            /* Table of WHERE clause constraints */
  int nOrderBy;              /* Number of terms in the ORDER BY clause */
  struct sqlite3_index_orderby {
     int iColumn;              /* Column number */
     unsigned char desc;       /* True for DESC.  False for ASC. */
  } *aOrderBy;               /* The ORDER BY clause */
  /* Outputs */
  struct sqlite3_index_constraint_usage {
    int argvIndex;           /* if >0, constraint is part of argv to xFilter */
    unsigned char omit;      /* Do not code a test for this constraint */
  } *aConstraintUsage;
  int idxNum;                /* Number used to identify the index */
  char *idxStr;              /* String, possibly obtained from sqlite3_malloc */
  int needToFreeIdxStr;      /* Free idxStr using sqlite3_free() if true */
  int orderByConsumed;       /* True if output is already ordered */
  double estimatedCost;           /* Estimated cost of using this index */
  /* Fields below are only available in SQLite 3.8.2 and later */
  sqlite3_int64 estimatedRows;    /* Estimated number of rows returned */
  /* Fields below are only available in SQLite 3.9.0 and later */
  int idxFlags;              /* Mask of SQLITE_INDEX_SCAN_* flags */
  /* Fields below are only available in SQLite 3.10.0 and later */
  sqlite3_uint64 colUsed;    /* Input: Mask of columns used by statement */
};

/*
** CAPI3REF: Virtual Table Scan Flags
**
** Virtual table implementations are allowed to set the
** [sqlite3_index_info].idxFlags field to some combination of
** these bits.
*/
#define SQLITE_INDEX_SCAN_UNIQUE      1     /* Scan visits at most 1 row */

/*
** CAPI3REF: Virtual Table Constraint Operator Codes
**
** These macros define the allowed values for the
** [sqlite3_index_info].aConstraint[].op field.  Each value represents
** an operator that is part of a constraint term in the WHERE clause of
** a query that uses a [virtual table].
**
** ^The left-hand operand of the operator is given by the corresponding
** aConstraint[].iColumn field.  ^An iColumn of -1 indicates the left-hand
** operand is the rowid.
** The SQLITE_INDEX_CONSTRAINT_LIMIT and SQLITE_INDEX_CONSTRAINT_OFFSET
** operators have no left-hand operand, and so for those operators the
** corresponding aConstraint[].iColumn is meaningless and should not be
** used.
**
** All operator values from SQLITE_INDEX_CONSTRAINT_FUNCTION through
** value 255 are reserved to represent functions that are overloaded
** by the [xFindFunction|xFindFunction method] of the virtual table
** implementation.
**
** The right-hand operands for each constraint might be accessible using
** the [sqlite3_vtab_rhs_value()] interface.  Usually the right-hand
** operand is only available if it appears as a single constant literal
** in the input SQL.  If the right-hand operand is another column or an
** expression (even a constant expression) or a parameter, then the
** sqlite3_vtab_rhs_value() probably will not be able to extract it.
** ^The SQLITE_INDEX_CONSTRAINT_ISNULL and
** SQLITE_INDEX_CONSTRAINT_ISNOTNULL operators have no right-hand operand
** and hence calls to sqlite3_vtab_rhs_value() for those operators will
** always return SQLITE_NOTFOUND.
**
** The collating sequence to be used for comparison can be found using
** the [sqlite3_vtab_collation()] interface.  For most real-world virtual
** tables, the collating sequence of constraints does not matter (for example
** because the constraints are numeric) and so the sqlite3_vtab_collation()
** interface is no commonly needed.
*/
#define SQLITE_INDEX_CONSTRAINT_EQ          2
#define SQLITE_INDEX_CONSTRAINT_GT          4
#define SQLITE_INDEX_CONSTRAINT_LE          8
#define SQLITE_INDEX_CONSTRAINT_LT         16
#define SQLITE_INDEX_CONSTRAINT_GE         32
#define SQLITE_INDEX_CONSTRAINT_MATCH      64
#define SQLITE_INDEX_CONSTRAINT_LIKE       65
#define SQLITE_INDEX_CONSTRAINT_GLOB       66
#define SQLITE_INDEX_CONSTRAINT_REGEXP     67
#define SQLITE_INDEX_CONSTRAINT_NE         68
#define SQLITE_INDEX_CONSTRAINT_ISNOT      69
#define SQLITE_INDEX_CONSTRAINT_ISNOTNULL  70
#define SQLITE_INDEX_CONSTRAINT_ISNULL     71
#define SQLITE_INDEX_CONSTRAINT_IS         72
#define SQLITE_INDEX_CONSTRAINT_LIMIT      73
#define SQLITE_INDEX_CONSTRAINT_OFFSET     74
#define SQLITE_INDEX_CONSTRAINT_FUNCTION  150

/*
** CAPI3REF: Register A Virtual Table Implementation
** METHOD: sqlite3
**
** ^These routines are used to register a new [virtual table module] name.
** ^Module names must be registered before
** creating a new [virtual table] using the module and before using a
** preexisting [virtual table] for the module.
**
** ^The module name is registered on the [database connection] specified
** by the first parameter.  ^The name of the module is given by the
** second parameter.  ^The third parameter is a pointer to
** the implementation of the [virtual table module].   ^The fourth
** parameter is an arbitrary client data pointer that is passed through
** into the [xCreate] and [xConnect] methods of the virtual table module
** when a new virtual table is be being created or reinitialized.
**
** ^The sqlite3_create_module_v2() interface has a fifth parameter which
** is a pointer to a destructor for the pClientData.  ^SQLite will
** invoke the destructor function (if it is not NULL) when SQLite
** no longer needs the pClientData pointer.  ^The destructor will also
** be invoked if the call to sqlite3_create_module_v2() fails.
** ^The sqlite3_create_module()
** interface is equivalent to sqlite3_create_module_v2() with a NULL
** destructor.
**
** ^If the third parameter (the pointer to the sqlite3_module object) is
** NULL then n