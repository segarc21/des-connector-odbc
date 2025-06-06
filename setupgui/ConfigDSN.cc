// Copyright (c) 2007, 2024, Oracle and/or its affiliates.
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

#include "MYODBC_CONF.h"
#ifndef _WIN32
#include "MYODBC_MYSQL.h"
#include "installer.h"
#include "unicode_transcode.h"
#include <sql.h>
#endif

#include "setupgui.h"
#include "stringutil.h"

bool is_unicode = 0;

extern "C" {
BOOL Driver_Prompt(HWND hWnd, SQLWCHAR *instr, SQLUSMALLINT completion,
                   SQLWCHAR *outstr, SQLSMALLINT outmax, SQLSMALLINT *outlen,
                   SQLSMALLINT unicode_flag);
}


/*
   Entry point for GUI prompting from SQLDriverConnect().
*/
BOOL Driver_Prompt(HWND hWnd, SQLWCHAR *instr, SQLUSMALLINT completion,
                   SQLWCHAR *outstr, SQLSMALLINT outmax, SQLSMALLINT *outlen,
                   SQLSMALLINT unicode_flag)
{
  DataSource ds;
  BOOL rc= FALSE;
  SQLWSTRING out;
  size_t copy_len = 0;
  is_unicode = unicode_flag;
  /*
     parse the attr string, dsn lookup will have already been
     done in the driver
  */
  if (instr && *instr)
  {
    if (ds.from_kvpair(instr, (SQLWCHAR)';'))
      goto exit;
  }

  /* Show the dialog and handle result */
  if (ShowOdbcParamsDialog(&ds, hWnd, TRUE) == 1)
  {
    /* serialize to outstr */
    out = ds.to_kvpair((SQLWCHAR)';');
    size_t len = out.length();
    if (outlen)
      *outlen = (SQLSMALLINT)len;

    if (outstr == nullptr || outmax == 0)
    {
      copy_len = 0;
    }
    else if (len > outmax)
    {
      /* truncated, up to caller to see outmax < *outlen */
      copy_len = outmax;
    }
    else
    {
      copy_len = len;
    }

    if (copy_len)
    {
      memcpy(outstr, out.c_str(), copy_len * sizeof(SQLWCHAR));
      outstr[copy_len] = 0;
    }

    rc = TRUE;
  }

exit:
  return rc;
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
   Add, edit, or remove a Data Source Name (DSN). This function is
   called by "Data Source Administrator" on Windows, or similar
   application on Unix.
*/
BOOL INSTAPI ConfigDSNW(HWND hWnd, WORD nRequest, LPCWSTR pszDriver,
                        LPCWSTR pszAttributes)
{
  DataSource ds;
  BOOL rc= TRUE;
  SQLWSTRING origdsn;

  if (!utf8_charset_info) {
    my_sys_init();
    utf8_charset_info =
      desodbc::get_charset_by_csname(transport_charset, MYF(MY_CS_PRIMARY), MYF(0));
  }

  if (pszAttributes && *pszAttributes)
  {
    SQLWCHAR delim= ';';

#ifdef _WIN32
    /*
      if there's no ;, then it's most likely null-delimited

     NOTE: the double null-terminated strings are not working
     *      with UnixODBC-GUI-Qt (posted a bug )
    */
    if (!sqlwcharchr(pszAttributes, delim))
      delim= 0;
#endif

    if (ds.from_kvpair(pszAttributes, delim))
    {
      SQLPostInstallerError(ODBC_ERROR_INVALID_KEYWORD_VALUE,
                            W_INVALID_ATTR_STR);
      return FALSE;
    }
    if (ds.lookup() && nRequest != ODBC_ADD_DSN)
    {
      /* ds_lookup() will already set SQLInstallerError */
      return FALSE;
    }
    origdsn = (const SQLWCHAR*)ds.opt_DSN;
  }

  Driver driver;
  driver.name = pszDriver;

  // For certain operations failure to lookup the driver
  // is not a critical error.
  int driver_lookup_error = driver.lookup();

  if (!driver_lookup_error)
  {
    // Detecting the type of the driver using the driver library file name,
    // which is not ideal. This might give wrong result if user renames
    // the library. However, not much harm is done in this case because
    // it will allow editing a CHARSET option, which normally should be
    // disabled for Unicode version of the driver.
    if (static_cast<std::string>(driver.lib).find(
          "desodbcw."
        ) != std::string::npos)
    {
      is_unicode = 1;
    }
  }

  switch (nRequest)
  {
  case ODBC_ADD_DSN:
    {
      if (driver_lookup_error)
        return FALSE;

      if (hWnd)
      {
        /*
          hWnd means we will at least try to prompt, at which point
          the driver lib will be replaced by the name
        */
        ds.opt_DRIVER = driver.lib;
      }
      else
      {
        /*
          no hWnd is a likely a call from an app w/no prompting so
          we put the driver name immediately
        */
        ds.opt_DRIVER = driver.name;
      }
    }
  case ODBC_CONFIG_DSN:

    /*
      If hWnd is NULL, we try to add the dsn
      with what information was given
    */
    if (!hWnd || ShowOdbcParamsDialog(&ds, hWnd, FALSE) == 1)
    {
      /* save datasource */
      if (ds.add())
        rc= FALSE;
      /* if the name is changed, remove the old dsn */
      if (!origdsn.empty() && origdsn != (const SQLWSTRING&)ds.opt_DSN)
        SQLRemoveDSNFromIni(origdsn.c_str());
    }
    break;
  case ODBC_REMOVE_DSN:
    if (SQLRemoveDSNFromIni(ds.opt_DSN) != TRUE)
      rc= FALSE;
    break;
  }

  return rc;
}

#ifdef USE_IODBC
BOOL INSTAPI ConfigDSN(HWND hWnd, WORD nRequest, LPCSTR pszDriverA,
                       LPCSTR pszAttributesA)
{
  BOOL rc;

  size_t lenDriver = strlen(pszDriverA);
  size_t lenAttrib = strlen(pszAttributesA);

  /* We will assume using one-byte Latin string as a subset of UTF-8 */
  SQLWCHAR *pszDriverW= (SQLWCHAR *) myodbc_malloc((lenDriver + 1) *
                                                sizeof(SQLWCHAR), MYF(0));
  SQLWCHAR *pszAttributesW= (SQLWCHAR *)myodbc_malloc((lenAttrib + 1) *
                                                  sizeof(SQLWCHAR), MYF(0));

  utf8_as_sqlwchar(pszDriverW, lenDriver, (SQLCHAR* )pszDriverA, lenDriver);
  utf8_as_sqlwchar(pszAttributesW, lenAttrib, (SQLCHAR* )pszAttributesA,
                   lenAttrib);

  rc= ConfigDSNW(hWnd, nRequest, pszDriverW, pszAttributesW);

  x_free(pszDriverW);
  x_free(pszAttributesW);
  return rc;
}
#endif
