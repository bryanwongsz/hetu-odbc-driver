/*
  Copyright (c) 2001, 2012, Oracle and/or its affiliates. All rights reserved.
                2016, 2019 MariaDB Corporation AB

  The MySQL Connector/ODBC is licensed under the terms of the GPLv2
  <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>, like most
  MySQL Connectors. There are special exceptions to the terms and
  conditions of the GPLv2 as it is applied to this software, see the
  FLOSS License Exception
  <http://www.mysql.com/about/legal/licensing/foss-exception.html>.
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation; version 2 of the License.
  
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
  
  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "tap.h"
#include "ma_dsn.h"

MADB_Dsn   *Dsn;
char        CreatedDSN[4][32];
int         CreatedDsnCount=  0;
const char *DsnName=          "madb_connstring_test";
const char *DsnConnStr=       "DSN=madb_connstring_test";
const char *Description=      "MariaDB C/ODBC test DSN for automatic testing";

/****************************** Helpers ****************************/
#define RESET_DSN(dsn) MADB_DSN_Free(dsn);memset(dsn, 0, sizeof(MADB_Dsn))

BOOL VerifyOptionFields(MADB_Dsn *Dsn)
{
  int i= 0;

  while (DsnKeys[i].DsnKey != NULL)
  {
    if (DsnKeys[i].Type == DSN_TYPE_OPTION && DsnKeys[i].IsAlias == 0)
    {
      if (*(my_bool *)((char *)Dsn + DsnKeys[i].DsnOffset) != (DSN_OPTION(Dsn, DsnKeys[i].FlagValue) != 0 ? 1 : 0))
      {
        diag("Dsn field for %s do not match %d bit in the Options", DsnKeys[i].DsnKey,DsnKeys[i].FlagValue);
        return FALSE;
      }
    }
    ++i;
  }
  return TRUE;
}

#define VERIFY_OPTIONS(dsn, value) is_num(dsn->Options,value);IS(VerifyOptionFields(dsn))

/* Copy of the function from connector - required for (easier) linking */
char *trim(char *Str)
{
  char *end;
  /* I am not sure using iswspace, and not isspace makes any sense here. But probably does not hurt either */
  while (Str && iswspace(Str[0]))
    Str++;
  end= Str + strlen(Str) - 1;
  while (iswspace(*end))
    *end--= 0;
  return Str;
}


int CreateTestDsn(MADB_Dsn *dsn)
{
  if (CreatedDsnCount == sizeof(CreatedDSN)/sizeof(CreatedDSN[0]))
  {
    diag("Max number of test DSN's has to be increased");
    return FAIL;
  }

  if (MADB_SaveDSN(dsn))
  {
    strcpy_s(CreatedDSN[CreatedDsnCount], sizeof(CreatedDSN[0]), dsn->DSNName);
    ++CreatedDsnCount;
    return OK;
  }
  diag("An error occured while saved DSN %s", dsn->DSNName);
  return FAIL;
}

int PopDSN()
{
  if (SQLRemoveDSNFromIni(CreatedDSN[CreatedDsnCount - 1]) != FALSE)
  {
    --CreatedDsnCount;
    CreatedDSN[CreatedDsnCount][0]= 0;

    return 0;
  }
  return 1;
}

int CleanUp()
{
  int i, ret= OK;

  for (i= CreatedDsnCount; i > 0; --i)
  {
    if (PopDSN())
    {
      diag("Failed to remove DSN %s", CreatedDSN[i - 1]);
      ret= FAIL;
    }
  }

  return ret;
}

/****************************** Tests ********************************/

ODBC_TEST(connstring_test)
{
  char connstr4dsn[512];

  IS(SQLRemoveDSNFromIni(DsnName));
  FAIL_IF(MADB_DSN_Exists(DsnName), "DSN exsists!");

  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s;DESCRIPTION={%s};DRIVER=%s;UID=%s;PWD=%s;SERVER=%s%s;OPTIONS=%u;NO_PROMPT=1", DsnName,
    Description, my_drivername, my_uid, my_pwd, my_servername, ma_strport, MADB_OPT_FLAG_COMPRESSED_PROTO|MADB_OPT_FLAG_AUTO_RECONNECT);

  IS(MADB_ParseConnString(Dsn, connstr4dsn, SQL_NTS, ';'));

  VERIFY_OPTIONS(Dsn, MADB_OPT_FLAG_COMPRESSED_PROTO|MADB_OPT_FLAG_AUTO_RECONNECT|MADB_OPT_FLAG_NO_PROMPT);
  IS_STR(Dsn->Description, Description, strlen(Description) + 1);

  FAIL_IF(CreateTestDsn(Dsn) == FAIL, "Failed to create test DSN");
  IS(MADB_DSN_Exists(DsnName));

  RESET_DSN(Dsn);

  IS(MADB_ReadDSN(Dsn, DsnConnStr, TRUE));

  VERIFY_OPTIONS(Dsn, MADB_OPT_FLAG_COMPRESSED_PROTO|MADB_OPT_FLAG_AUTO_RECONNECT|MADB_OPT_FLAG_NO_PROMPT);
  is_num(Dsn->Port, my_port);
  IS_STR(Dsn->Description, Description, sizeof(Description));
  IS_STR(Dsn->Driver, my_drivername, strlen((const char*)my_drivername) + 1);

  FAIL_IF(Dsn->UserName == NULL, "User Name should not be NULL");
  FAIL_IF(strncmp(Dsn->UserName, (const char*)my_uid, strlen((const char*)my_uid) + 1), "Uid stored in/read from DSN incorrectly");
 
  if (Dsn->Password == NULL)
  { 
    FAIL_IF(my_pwd != NULL && *my_pwd != '\0', "Password shouldn't be NULL");
  }
  else
  {
    FAIL_IF(strncmp(Dsn->Password, (const char*)my_pwd, strlen((const char*)my_pwd) + 1), "Pwd stored in/read from DSN incorrectly");
  }

  IS_STR(Dsn->ServerName, my_servername, strlen((const char*)my_servername) + 1);

  /* Checking that value in the connection string prevails over value in the dsn */
  _snprintf(connstr4dsn, sizeof(connstr4dsn), "%s;OPTIONS=%u;DESCRIPTION=%s", DsnConnStr, MADB_OPT_FLAG_NO_PROMPT|MADB_OPT_FLAG_FOUND_ROWS,
    "Changed description");

  IS(MADB_ReadConnString(Dsn, connstr4dsn, SQL_NTS, ';'));
  VERIFY_OPTIONS(Dsn, MADB_OPT_FLAG_NO_PROMPT|MADB_OPT_FLAG_FOUND_ROWS);
  IS_STR(Dsn->Description, "Changed description", sizeof("Changed description"));

  RESET_DSN(Dsn);

  /* Same as previous, but also that last ent*/
  _snprintf(connstr4dsn, sizeof(connstr4dsn), "%s;OPTIONS=%u;NO_PROMPT=0;AUTO_RECONNECT=1", DsnConnStr,
    MADB_OPT_FLAG_NO_PROMPT|MADB_OPT_FLAG_FOUND_ROWS|MADB_OPT_FLAG_FORWARD_CURSOR|MADB_OPT_FLAG_NO_CACHE);

  IS(MADB_ReadConnString(Dsn, connstr4dsn, SQL_NTS, ';'));
  VERIFY_OPTIONS(Dsn, MADB_OPT_FLAG_FOUND_ROWS|MADB_OPT_FLAG_FORWARD_CURSOR|MADB_OPT_FLAG_NO_CACHE|MADB_OPT_FLAG_AUTO_RECONNECT);
  IS_STR(Dsn->Description, Description, sizeof(Description));

  return OK;
}


ODBC_TEST(options_test)
{
  const char LocalDSName[]= "madb_connstr_options_test";
  char LocalConnStr[sizeof(LocalDSName)+7];
  char connstr4dsn[512];
  unsigned int bit= 0, i= 0;

  while(1)
  {
    ++i;
    /* We need new DSN each time, otherwise buggy UnixODBC ini caching will return us wrong data
       (Seems like that will be fixed in 2.3.5) */
    _snprintf(LocalConnStr, sizeof(LocalConnStr), "DSN=%s%u", LocalDSName, i);
    _snprintf(connstr4dsn, sizeof(connstr4dsn), "%s;DRIVER=%s;OPTIONS=%u", LocalConnStr, my_drivername, bit);

    diag("%s:::%s", LocalConnStr, connstr4dsn);
    IS(MADB_ParseConnString(Dsn, connstr4dsn, SQL_NTS, ';'));

    VERIFY_OPTIONS(Dsn, bit);

    IS(CreateTestDsn(Dsn));

    RESET_DSN(Dsn);

    IS(MADB_ReadDSN(Dsn, LocalConnStr, TRUE));

    VERIFY_OPTIONS(Dsn, bit);

    if (bit == 0x80000000)
      break;
    bit= bit != 0 ? bit << 1 : 1;

    FAIL_IF(PopDSN(), "Could not remove DSN");
  }

  return OK;
}


unsigned int CharsSum(const char *str)
{
  unsigned int res= 0;
  while (*str)
  {
    res+= *str++;
  }
  return res;
}

ODBC_TEST(all_other_fields_test)
{
  char connstr4dsn[512], *opt_value, IntValue[20], *BoolValue= "1";
  int i= 5; /* After Options. Assuming that fields before Options are tested in other test(s) */

  if (Travis)
  {
    /* As already said - ini cache is buggy in UnixODBC, and it fails with UnixODBC version we have availabe in Travis tests.
       It's possible to change the test to pass in similar way as we did in some other tests - by either writing and reading
       the (new) DSN only once, or by creating new DSN for each keyword. */
    skip("Skipping with test in Travis")
  }
  FAIL_IF(!MADB_DSN_Exists(DsnName), "Something went wrong - DSN does not exsist");

  while(DsnKeys[i].DsnKey != NULL)
  {
    if (DsnKeys[i].IsAlias)
    {
      ++i;
      continue;
    }
    opt_value= DsnKeys[i].DsnKey;
    if (DsnKeys[i].Type == DSN_TYPE_BOOL || DsnKeys[i].Type == DSN_TYPE_OPTION)
    {
      opt_value= BoolValue;
    }
    else if (DsnKeys[i].Type == DSN_TYPE_INT)
    {
      _snprintf(IntValue, sizeof(IntValue), "%u", CharsSum(DsnKeys[i].DsnKey));
      opt_value= IntValue;
    }

    _snprintf(connstr4dsn, sizeof(connstr4dsn), "%s;%s=%s", DsnConnStr, DsnKeys[i].DsnKey, opt_value);

    IS(MADB_ReadConnString(Dsn, connstr4dsn, SQL_NTS, ';'));
    IS(MADB_SaveDSN(Dsn));
    ++i;
  }

  /* This fields were out of the loop */
  _snprintf(connstr4dsn, sizeof(connstr4dsn), "%s;DESCRIPTION=DESCRIPTION;DRIVER=%s", DsnConnStr, my_drivername);
  IS(MADB_ReadConnString(Dsn, connstr4dsn, SQL_NTS, ';'));
  IS(MADB_SaveDSN(Dsn));

  /* We saved all posible options. If DsnKeys contains erroneous info, some of values would have got corrupted */
  RESET_DSN(Dsn);
  IS(MADB_ReadDSN(Dsn, DsnConnStr, TRUE));
  /* 2 special fields */
  IS_STR(Dsn->Driver, (const char*)my_drivername, strlen((const char*)my_drivername) + 1);
  IS_STR(Dsn->Description, "DESCRIPTION", sizeof("DESCRIPTION"));
  i= 5;
  while(DsnKeys[i].DsnKey != NULL)
  {
    if (DsnKeys[i].IsAlias)
    {
      ++i;
      continue;
    }

    switch (DsnKeys[i].Type)
    {
    case DSN_TYPE_COMBO:
    case DSN_TYPE_STRING:
      IS_STR(*(char**)((char*)Dsn + DsnKeys[i].DsnOffset), DsnKeys[i].DsnKey, strlen(DsnKeys[i].DsnKey) + 1);
      break;
    case DSN_TYPE_INT:
      is_num(*(unsigned int*)((char*)Dsn + DsnKeys[i].DsnOffset), CharsSum(DsnKeys[i].DsnKey));
      break;
    case DSN_TYPE_BOOL:
    case DSN_TYPE_OPTION:
      /* IsNamedPipe is switched off when TcpIp is switched on(since TcpIp goes after NamedPipe in the DsnKeys.
         Do detect IsNamedPipe, comparing its offset in the MADB_Dsn with offset recorded in the DsnKeys */
      is_num(*(my_bool*)((char*)Dsn + DsnKeys[i].DsnOffset), DsnKeys[i].DsnOffset == (size_t)&((MADB_Dsn*)NULL)->IsNamedPipe ? 0 : 1);
    case DSN_TYPE_CBOXGROUP:
      break;
    }

    ++i;
  }

  RESET_DSN(Dsn);

  return OK;
}


ODBC_TEST(aliases_tests)
{
  const char *LocalDSName=  "madb_connstr_aliases_test";
  const char *LocalConnStr= "DSN=madb_connstr_aliases_test";
  char connstr4dsn[512];
  unsigned int options= 0xf0f0f0f0;
  unsigned int option= ~options;

  FAIL_IF(!MADB_DSN_Exists(DsnName), "Something went wrong - DSN does not exsist");

  RESET_DSN(Dsn);

  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s;DRIVER=%s;UID=%s;PWD=%s;SERVER=%s%s;DATABASE=%s;OPTIONS=%u", LocalDSName,
    my_drivername, my_uid, my_pwd, my_servername, ma_strport, my_schema, options);

  IS(MADB_ParseConnString(Dsn, connstr4dsn, SQL_NTS, ';'));
  IS(CreateTestDsn(Dsn));

  RESET_DSN(Dsn);
  IS(MADB_ReadConnString(Dsn, LocalConnStr, SQL_NTS, ';'));

  FAIL_IF(Dsn->UserName == NULL || strncmp(Dsn->UserName, (const char*)my_uid, strlen((const char*)my_uid) + 1), "Uid stored in/read from DSN incorrectly")

  if (Dsn->Password == NULL)
  { 
    FAIL_IF(my_pwd != NULL && *my_pwd != '\0', "Password shouldn't be NULL");
  }
  else
  {
    FAIL_IF(strncmp(Dsn->Password, (const char*)my_pwd, strlen((const char*)my_pwd) + 1), "Pwd stored in/read from DSN incorrectly");
  }

  IS_STR(Dsn->ServerName, my_servername, strlen((const char*)my_servername) + 1);
  IS_STR(Dsn->Catalog,    my_schema,     strlen((const char*)my_schema) + 1);
  is_num(Dsn->Options,    options);

  /* Now set all values via aliases. In fact we could generate the string automatically, but I guess there is not much need  */
  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s;USER=user;PASSWORD=password;SERVERNAME=servername;DB=randomdbname;OPTION=%u", LocalDSName, option);

  IS(MADB_ReadConnString(Dsn, connstr4dsn, SQL_NTS, ';'));

  IS_STR(Dsn->UserName,   "user",         sizeof("user"));
  IS_STR(Dsn->Password,   "password",     sizeof("password"));
  IS_STR(Dsn->ServerName, "servername",   sizeof("servername"));
  IS_STR(Dsn->Catalog,    "randomdbname", sizeof("randomdbname"));
  is_num(Dsn->Options,    option);

  FAIL_IF(PopDSN(), "Could not remove DSN");

  return OK;
}


ODBC_TEST(dependent_fields)
{
  const char *LocalDSName=  "madb_connstr_dependent_fields";
  const char *LocalConnStr= "DSN=madb_connstr_dependent_fields";
  char connstr4dsn[512];

  FAIL_IF(!MADB_DSN_Exists(DsnName), "Something went wrong - DSN does not exsist");

  RESET_DSN(Dsn);

  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s;DRIVER=%s;UID=%s;PWD=%s;SERVER=%s%s;DB=%s;OPTIONS=%u;TCPIP=1", LocalDSName,
    my_drivername, my_uid, my_pwd, my_servername, ma_strport, my_schema, MADB_OPT_FLAG_NAMED_PIPE);

  IS(MADB_ParseConnString(Dsn, connstr4dsn, SQL_NTS, ';'));
  IS(CreateTestDsn(Dsn));

  RESET_DSN(Dsn);
  IS(MADB_ReadConnString(Dsn, LocalConnStr, SQL_NTS, ';'));

  is_num(Dsn->IsTcpIp,     1);
  is_num(Dsn->IsNamedPipe, 0);
  is_num(Dsn->Options,     0);

  /* Now set all values via aliases. In fact we could generate the string automatically, but I guess there is not much need  */
  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s;NamedPipe=1", LocalDSName);

  IS(MADB_ReadConnString(Dsn, connstr4dsn, SQL_NTS, ';'));
  is_num(Dsn->IsTcpIp,     0);
  is_num(Dsn->IsNamedPipe, 1);
  is_num(Dsn->Options,     MADB_OPT_FLAG_NAMED_PIPE);

  FAIL_IF(PopDSN(), "Could not remove DSN");

  return OK;
}


/* I render following text from https://msdn.microsoft.com/en-us/library/ms715433%28v=vs.85%29.aspx
   "The driver checks whether the connection string passed to it by the Driver Manager contains the DSN or DRIVER keyword.
   If the connection string contains the DRIVER keyword, the driver cannot retrieve information about the data source from
   the system information. If the connection string contains the DSN keyword or does not contain either the DSN or the DRIVER
   keyword, the driver can retrieve information about the data source from the system information as follows:
    - If the connection string contains the DSN keyword, the driver retrieves the information for the specified data source.
    - If the connection string does not contain the DSN keyword, the specified data source is not found, or the DSN keyword is
    set to "DEFAULT", the driver retrieves the information for the Default data source."
  
    as DRIVER keyword should prevent driver from expanding DSN data. The testcase verifies that. */
ODBC_TEST(driver_vs_dsn)
{
  char connstr4dsn[512];

  FAIL_IF(!MADB_DSN_Exists(DsnName), "Something went wrong - DSN does not exsist");

  RESET_DSN(Dsn);

  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s;DRIVER=%s;UID=%s;PWD=%s;SERVER=%s%s;DB=%s;OPTIONS=%u", DsnName,
    my_drivername, my_uid, my_pwd, my_servername, ma_strport, my_schema, MADB_OPT_FLAG_NAMED_PIPE);

  IS(MADB_ParseConnString(Dsn, connstr4dsn, SQL_NTS, ';'));
  IS(MADB_SaveDSN(Dsn));

  RESET_DSN(Dsn);
  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s;DRIVER=%s;SERVER=%s;", DsnName, my_drivername, "some.other.host");

  IS(MADB_ReadConnString(Dsn, connstr4dsn, SQL_NTS, ';'));

  /* Natural in any case*/
  IS_STR(Dsn->ServerName, "some.other.host", sizeof("some.other.host"));
  /* CHeck that we have nothing from the DSN */
  is_num(Dsn->IsTcpIp,     0); /* It should not be set either */
  is_num(Dsn->IsNamedPipe, 0);
  is_num(Dsn->Options,     0);
  FAIL_IF(Dsn->Port != 0 && Dsn->Port == my_port, "Port value from DSN!");
  FAIL_IF(Dsn->UserName!= NULL && strncmp(Dsn->UserName, (const char*)my_uid, strlen((const char*)my_uid) + 1), "Uid value from DSN!");
  FAIL_IF(Dsn->Password!= NULL && strncmp(Dsn->Password, (const char*)my_pwd, strlen((const char*)my_pwd) + 1), "Pwd value from DSN!");
  FAIL_IF(Dsn->Catalog!= NULL && strncmp(Dsn->Catalog, (const char*)my_schema, strlen((const char*)my_schema) + 1), "DB value from DSN!");

  return OK;
}

/* Parsing of NULL terminated connstring of NULL terminated key=value pairs*/
ODBC_TEST(odbc_188)
{
  char connstr4dsn[512];

  IS(SQLRemoveDSNFromIni(DsnName));
  FAIL_IF(MADB_DSN_Exists(DsnName), "DSN exsists!");

  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s%cDESCRIPTION={%s}%cDRIVER=%s%cUID=%s%cPWD=%s%cSERVER=%s%cPORT=%u%cOPTIONS=%u%cNO_PROMPT=1\0\0", DsnName, '\0',
    Description, '\0', my_drivername, '\0', my_uid, '\0', my_pwd, '\0', my_servername, '\0', my_port, '\0', MADB_OPT_FLAG_COMPRESSED_PROTO|MADB_OPT_FLAG_AUTO_RECONNECT, '\0');

  IS(MADB_ParseConnString(Dsn, connstr4dsn, SQL_NTS, '\0'));

  VERIFY_OPTIONS(Dsn, MADB_OPT_FLAG_COMPRESSED_PROTO|MADB_OPT_FLAG_AUTO_RECONNECT|MADB_OPT_FLAG_NO_PROMPT);
  IS_STR(Dsn->Description, Description, strlen(Description) + 1);

  FAIL_IF(CreateTestDsn(Dsn) == FAIL, "Failed to create test DSN");
  IS(MADB_DSN_Exists(DsnName));

  RESET_DSN(Dsn);

  IS(MADB_ReadDSN(Dsn, DsnConnStr, TRUE));

  VERIFY_OPTIONS(Dsn, MADB_OPT_FLAG_COMPRESSED_PROTO|MADB_OPT_FLAG_AUTO_RECONNECT|MADB_OPT_FLAG_NO_PROMPT);
  is_num(Dsn->Port, my_port);
  IS_STR(Dsn->Description, Description, sizeof(Description));
  IS_STR(Dsn->Driver, my_drivername, strlen((const char*)my_drivername) + 1);

  FAIL_IF(Dsn->UserName == NULL, "User Name should not be NULL");
  FAIL_IF(strncmp(Dsn->UserName, (const char*)my_uid, strlen((const char*)my_uid) + 1), "Uid stored in/read from DSN incorrectly");

  if (Dsn->Password == NULL)
  {
    FAIL_IF(my_pwd != NULL && *my_pwd != '\0', "Password shouldn't be NULL");
  }
  else
  {
    FAIL_IF(strncmp(Dsn->Password, (const char*)my_pwd, strlen((const char*)my_pwd) + 1), "Pwd stored in/read from DSN incorrectly");
  }

  IS_STR(Dsn->ServerName, my_servername, strlen((const char*)my_servername) + 1);

  /* Checking that value in the connection string prevails over value in the dsn */
  _snprintf(connstr4dsn, sizeof(connstr4dsn), "%s%cOPTIONS=%u%cDESCRIPTION=%s\0\0", DsnConnStr, '\0', MADB_OPT_FLAG_NO_PROMPT|MADB_OPT_FLAG_FOUND_ROWS,
    '\0', "Changed description");

  IS(MADB_ReadConnString(Dsn, connstr4dsn, SQL_NTS, '\0'));
  VERIFY_OPTIONS(Dsn, MADB_OPT_FLAG_NO_PROMPT|MADB_OPT_FLAG_FOUND_ROWS);
  IS_STR(Dsn->Description, "Changed description", sizeof("Changed description"));

  RESET_DSN(Dsn);

  /* Same as previous, but also that last ent*/
  _snprintf(connstr4dsn, sizeof(connstr4dsn), "%s%cOPTIONS=%u%cNO_PROMPT=0%cAUTO_RECONNECT=1\0\0", DsnConnStr,
    '\0', MADB_OPT_FLAG_NO_PROMPT|MADB_OPT_FLAG_FOUND_ROWS|MADB_OPT_FLAG_FORWARD_CURSOR|MADB_OPT_FLAG_NO_CACHE, '\0', '\0');

  IS(MADB_ReadConnString(Dsn, connstr4dsn, SQL_NTS, '\0'));
  VERIFY_OPTIONS(Dsn, MADB_OPT_FLAG_FOUND_ROWS|MADB_OPT_FLAG_FORWARD_CURSOR|MADB_OPT_FLAG_NO_CACHE|MADB_OPT_FLAG_AUTO_RECONNECT);
  IS_STR(Dsn->Description, Description, sizeof(Description));

  return OK;
}


/* Only testing that opion is saved/red */
ODBC_TEST(odbc_229)
{
  const char *LocalDSName=  "madb_connstr_usecnf";
  const char *LocalConnStr= "DSN=madb_connstr_usecnf";
  char connstr4dsn[512];

  IS(SQLRemoveDSNFromIni(LocalDSName));
  FAIL_IF(MADB_DSN_Exists(LocalDSName), "DSN exsists!");

  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s;DRIVER=%s;USE_MYCNF=1", LocalDSName, my_drivername);

  IS(MADB_ParseConnString(Dsn, connstr4dsn, SQL_NTS, ';'));

  IS(Dsn->Options & MADB_OPT_FLAG_USE_CNF);

  IS(CreateTestDsn(Dsn));

  RESET_DSN(Dsn);

  IS(MADB_ReadDSN(Dsn, LocalConnStr, TRUE));

  diag("USE_MYCNF: %hu %u %u", (short)Dsn->ReadMycnf, Dsn->Options, Dsn->Options & MADB_OPT_FLAG_USE_CNF);
  IS(Dsn->ReadMycnf);
  IS(Dsn->Options & MADB_OPT_FLAG_USE_CNF);

  FAIL_IF(PopDSN(), "Could not remove DSN");

  return OK;
}

/* Testing that both numeric and string representation of TLSVERSION work */
ODBC_TEST(odbc_228)
{
  const char *LocalDSName=  "madb_connstr_tlsversion";
  const char *LocalConnStr= "DSN=madb_connstr_tlsversion";
  char connstr4dsn[512];

  IS(SQLRemoveDSNFromIni(LocalDSName));
  FAIL_IF(MADB_DSN_Exists(LocalDSName), "DSN exsists!");

  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s;DRIVER=%s;TLSVERSION=TLSv1.1,TLSv1.3;PORT=3307", LocalDSName, my_drivername);

  IS(MADB_ParseConnString(Dsn, connstr4dsn, SQL_NTS, ';'));

  is_num(Dsn->TlsVersion, MADB_TLSV11|MADB_TLSV13);
  is_num(Dsn->Port, 3307);

  IS(CreateTestDsn(Dsn));

  RESET_DSN(Dsn);

  IS(MADB_ReadDSN(Dsn, LocalConnStr, TRUE));

  diag("TlsVerion: %hu, Port: %u", (short)Dsn->TlsVersion, Dsn->Port);
  is_num(Dsn->TlsVersion, MADB_TLSV11|MADB_TLSV13);
  is_num(Dsn->Port, 3307);

  RESET_DSN(Dsn);

  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s;DRIVER=%s;TLSVERSION=6", LocalDSName, my_drivername);
  IS(MADB_ParseConnString(Dsn, connstr4dsn, SQL_NTS, ';'));
  is_num(Dsn->TlsVersion, MADB_TLSV12|MADB_TLSV13);

  RESET_DSN(Dsn);

  /* If not only meaningful bits are set. Maybe that should be an error? */
  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s;DRIVER=%s;TLSVERSION=65", LocalDSName, my_drivername);
  IS(MADB_ParseConnString(Dsn, connstr4dsn, SQL_NTS, ';'));
  is_num(Dsn->TlsVersion & MADB_TLSV11, MADB_TLSV11);
  is_num(Dsn->TlsVersion & MADB_TLSV12, 0);
  is_num(Dsn->TlsVersion & MADB_TLSV13, 0);

  RESET_DSN(Dsn);

  _snprintf(connstr4dsn, sizeof(connstr4dsn), "DSN=%s;DRIVER=%s;TLSVERSION=garbage tlsv1.2", LocalDSName, my_drivername);
  IS(MADB_ParseConnString(Dsn, connstr4dsn, SQL_NTS, ';'));
  is_num(Dsn->TlsVersion, MADB_TLSV12);

  FAIL_IF(PopDSN(), "Could not remove DSN");

  return OK;
}


MA_ODBC_TESTS my_tests[]=
{
  {connstring_test,       "connstring_parsing_test", NORMAL},
  {options_test,          "options_test",            NORMAL},
  {all_other_fields_test, "all_other_fields_test",   NORMAL},
  {aliases_tests,         "aliases_tests",           NORMAL},
  {dependent_fields,      "dependent_fields_tests",  NORMAL},
  {driver_vs_dsn,         "driver_vs_dsn",           NORMAL},
  {odbc_188,              "odbc188_nt_pairs",        NORMAL},
  {odbc_229,              "odbc229_usecnf",          NORMAL},
  {odbc_228,              "odbc228_tlsversion",      NORMAL},
  {NULL, NULL, 0}
};


int main(int argc, char **argv)
{
  int ret, tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;

  get_options(argc, argv);
  plan(tests);
  Dsn= MADB_DSN_Init();
  Dsn->FreeMe= FALSE; /* Let tests only reset dsn struct, but do not free it */

  ret= run_tests(my_tests);

  Dsn->FreeMe= TRUE;
  MADB_DSN_Free(Dsn);

  if (CleanUp() == FAIL)
  {
    ret= 1;
    diag("Clean-up after tests failed - %d test DSN's stay in the system:", CreatedDsnCount);
  }

  return ret;
}
