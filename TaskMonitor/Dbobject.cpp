/* dbobject.cpp : implementation file
/*
/*	Usage:
	CDBObject* pDBObj = new CDBObject;
	CString StrOpen = gApp->i7AnemoDSN;	// "DSN=" + CString("i7AnemoDB");
	CString SQLStmt = "select DeviceId, TxnDate, TxnTime from AnemoLog";
	if (pDBObj->Open(StrOpen))
	{
		CObArray* pStrArray  = new CObArray;
		CStringList* pSelFieldList = new CStringList;
		if (pDBObj->ExecuteSQLStmt(SQLStmt, pStrArray, pSelFieldList))
		{
			int nRecords = pStrArray->GetSize();
			int nFields  = pSelFieldList->GetCount();
			if (nRecords == 3)
			{	 
				CStringArray* pSeleData  = (CStringArray*)pStrArray->GetAt(0);
				CString TxnDate = pSeleData->GetAt(1); 
			}
		}
/* ----------------------------------------------------------------- */

#include "stdafx.h"
//#include "resource.h"
#include "dbobject.h"
#include <stdlib.h>

#ifdef _DEBUG        
// #undef THIS_FILE
// static char BASED_CODE THIS_FILE[] = __FILE__;
#define malloc(s)			_malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#endif

int	CDBObject::u_TraceSQL = 0;
int CDBObject::u_ShowErrorMsg = 0;
int CDBObject::u_Translate = 0;
HENV CDBObject::u_henv = NULL;
int	CDBObject::u_henvCount = 0;

#define STR_LEN 128+1

#define tSep            ","

#define SQL_CURSOR_TYPE 			6
#define SQL_CONCURRENCY 			7
#define SQL_KEYSET_SIZE 			8
#define SQL_ROWSET_SIZE 			9

/* SQL_CURSOR_TYPE options */
#define SQL_CURSOR_FORWARD_ONLY 	0UL
#define SQL_CURSOR_KEYSET_DRIVEN	1UL
#define SQL_CURSOR_DYNAMIC			2UL
#define SQL_CURSOR_STATIC			3UL
#define SQL_CURSOR_TYPE_DEFAULT		SQL_CURSOR_FORWARD_ONLY	/* Default value */

/* Operations in SQLSetPos */
#define SQL_POSITION				0		/*	1.0 FALSE */
/* Lock options in SQLSetPos */
#define SQL_LOCK_NO_CHANGE			0 		/*	1.0 FALSE */

/////////////////////////////////////////////////////////////////////////////
// CDBObject

// 2017-11-29, Replicating ODBC handles for recovery from ODBC hangs.
static	HSTMT	g_hStmt = (0);
static	HDBC	g_hdbc  = (0);
static  HENV	g_henv  = (0);
void releaseOdbcEnv()
{
	if (g_hStmt != NULL)
		SQLFreeStmt(g_hStmt, SQL_DROP);;

    if (g_hdbc != NULL)
    {
    	SQLDisconnect(g_hdbc);
    	SQLFreeConnect(g_hdbc);
    }
   	if (--CDBObject::u_henvCount == 0)
		SQLFreeEnv(g_henv);
}

IMPLEMENT_DYNCREATE(CDBObject, CObject)

CDBObject::CDBObject()
{
	if (u_henvCount++ == 0)
	{
	   SQLAllocEnv(&u_henv);		// Original statement.
	// 2017-03-11, Allocate an environment handle
	// SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &u_henv);
	// We want ODBC 3 support 
	// SQLSetEnvAttr(u_henv, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);

	// SQLSetConnectAttr(u_henv, SQL_ATTR_VERTICA_LOCALE, (SQLCHAR*)"big5", SQL_NTS);

	}

	u_nResultCols = 0;
	u_nRowSize = 1;                 
	u_hdbc = NULL;
	u_hStmt = NULL;
    u_pFieldNameList = new CStringList;
    u_pTypeList =  new CStringList;
	for (int i = 0; i < MAXCOLS; i++)
	{
		u_Data[i] = NULL;
		u_pData[i] = NULL;    //Update07181:
	    u_pOutLen[i] = NULL;  //Update07181:
	}
	u_StartTime = 0;
	u_EndTime = 0;
	u_DeadLockError = FALSE;
}


CDBObject::~CDBObject()
{
	if (u_pFieldNameList != NULL)    
	{
		u_pFieldNameList->RemoveAll();
		delete u_pFieldNameList;
	}	
	if (u_pTypeList != NULL)
	{
		u_pTypeList->RemoveAll();
		delete u_pTypeList;
    }

    int i;
	i = 0;  //Update07181:
    while (u_pData[i] != NULL)
    	delete u_pData[i++];
	i = 0;  //Update07181:
    while (u_pOutLen[i] != NULL)
    	delete u_pOutLen[i++];
    ReleaseData();

#ifdef MANNUAL_COMMIT
	if (mApp->u_bMannual_Commit)
	{
	SQLTransact(u_henv, u_hdbc, SQL_COMMIT);
	}
#endif
    
	if (u_hStmt != NULL)
		SQLFreeStmt(u_hStmt, SQL_DROP);;

    if (u_hdbc != NULL)
    {
    	SQLDisconnect(u_hdbc);
    	SQLFreeConnect(u_hdbc);
    }
    
   	if (--u_henvCount == 0)
	    SQLFreeEnv(u_henv);
}


BOOL CDBObject::CreateTable(CString TableName, CObArray *pFieldArray)
{
	int		RetValue;
    CString SQLStmt;

    SQLStmt = "CREATE TABLE " + TableName + "(";

	int nSize, i;
	
	nSize = pFieldArray->GetSize();
	if (nSize == 0)
		return FALSE;
		
	FieldData	*pFieldData;                           
	CString		csFieldStr;
	for (i = 0; i < nSize; i++)
	{                       
		pFieldData = (FieldData*)pFieldArray->GetAt(i);
		csFieldStr = CreateFieldString(pFieldData);
		if (!csFieldStr.IsEmpty())         
		{
			if (i == 0)
				SQLStmt += csFieldStr;
			else
				SQLStmt += ", " + csFieldStr;	
		}
		else
			return FALSE;
	}                                      
		
	SQLStmt += ")";
	    
    RetValue = ExecuteSQL(SQLStmt);

	return RetValue == SQL_SUCCESS || RetValue == SQL_SUCCESS_WITH_INFO;
}


BOOL CDBObject::DropTable(CString TableName)
{
    CString SQLStmt;
    SQLStmt = "DROP TABLE " + TableName;

    int	RetValue = ExecuteSQL(SQLStmt);

	return RetValue == SQL_SUCCESS || RetValue == SQL_SUCCESS_WITH_INFO;
}


BOOL CDBObject::CreateIndex(CString IndexName, CString TableName, CString IndexFields)
{
    CString SQLStmt;
    SQLStmt = "CREATE INDEX " + IndexName + " ON ";
    SQLStmt += TableName + " ( " + IndexFields + " ) "; 
	    
   	int	RetValue = ExecuteSQL(SQLStmt);
	    	
	return RetValue == SQL_SUCCESS || RetValue == SQL_SUCCESS_WITH_INFO;
}                      

BOOL CDBObject::DropIndex(CString IndexName)
{
    CString SQLStmt;
    SQLStmt = "DROP INDEX " + IndexName;

    int RetValue = ExecuteSQL(SQLStmt);

	return RetValue == SQL_SUCCESS || RetValue == SQL_SUCCESS_WITH_INFO;
}

BOOL CDBObject::AddField(CString TableName, CObArray *pFieldArray)
{
    CString SQLStmt;
    SQLStmt = "ALTER TABLE " + TableName;

	FieldData	*pFieldData;
	int			i, nRecords;
	
	nRecords = pFieldArray->GetSize();
	if (nRecords == 0)
		return FALSE;

	for (i = 0; i < nRecords; i++)
	{
		pFieldData = (FieldData*)pFieldArray->GetAt(i);
		if (pFieldData == NULL)
			continue;                          
		if (i != nRecords - 1)	
			SQLStmt += " ADD " + CreateFieldString(pFieldData) + " , ";	
		else	
			SQLStmt += " ADD " + CreateFieldString(pFieldData);				
	}
			
    int RetValue = ExecuteSQL(SQLStmt);

	return RetValue == SQL_SUCCESS || RetValue == SQL_SUCCESS_WITH_INFO;
}

BOOL CDBObject::DeleteField(CString TableName, CObArray *pFieldArray)
{
    CString SQLStmt;
    SQLStmt = "ALTER TABLE " + TableName;

	FieldData	*pFieldData;
	int			i, nRecords;
	
	nRecords = pFieldArray->GetSize();
	if (nRecords == 0)
		return FALSE;

	for (i = 0; i < nRecords; i++)
	{
		pFieldData = (FieldData*)pFieldArray->GetAt(i);
		if (pFieldData == NULL)
			continue;                          
		if (i != nRecords - 1)	
			SQLStmt += " DROP " + pFieldData->FieldName + " , ";	
		else	
			SQLStmt += " DROP " + pFieldData->FieldName;				
	}
			
    int RetValue = ExecuteSQL(SQLStmt);

	return RetValue == SQL_SUCCESS || RetValue == SQL_SUCCESS_WITH_INFO;
}

BOOL CDBObject::FetchData(CObArray *pStrArray, long irow)
{
	::ReleaseObArray(pStrArray);
	RETCODE Ret = FetchRecords(pStrArray, irow);
	if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO &&
		Ret != SQL_NO_DATA_FOUND)
	{
		return FALSE;
    }
	return TRUE;
}                                          

BOOL CDBObject::Query(CString TableName, 
					  CString SelFields, CString Key, CString SortCols, 
       		   		  CStringArray* csBindKey, CObArray* pObArray)
{
    HSTMT hStmt;
    RETCODE Ret = SQLAllocStmt(u_hdbc, &hStmt);
	if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
		return FALSE;
		
	Ret = InternalQuery(TableName, SelFields, Key, 
						SortCols, csBindKey, -1, &hStmt);
	if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
	{
		SQLFreeStmt(hStmt, SQL_DROP);
		return FALSE;
    }
    
	::ReleaseObArray(pObArray);
	Ret = FetchAllRecords(pObArray, &hStmt);
	SQLFreeStmt(hStmt, SQL_DROP);

	if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
		return FALSE;
    
	return TRUE;
}       		   		 

BOOL CDBObject::PreQuery(CString SQLStmt, int nRowSize)
{
	CheckSQL(SQLStmt);

	if (u_hStmt != NULL)
		SQLFreeStmt(u_hStmt, SQL_DROP);;
    RETCODE Ret = SQLAllocStmt(u_hdbc, &u_hStmt);
	if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
		return FALSE;
	if (!SetCursorOption(nRowSize, &u_hStmt))
		return FALSE;

	if (u_DBType == "ORACLE")
		Substitute(SQLStmt, "Substring (", "Substr(");

    Ret = SQLPrepare(u_hStmt, (UCHAR FAR*)(const char*)SQLStmt, SQL_NTS);
	if (Ret == SQL_SUCCESS)
	{
		Ret = SQLExecute(u_hStmt);
	    if (Ret == SQL_SUCCESS || Ret == SQL_SUCCESS_WITH_INFO)
			Ret = QueryBindCol(u_hStmt, nRowSize);
	    
		if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
		{
			GetErrorStatus(Ret, SQLStmt, u_hStmt);
			return FALSE;
		}
	}
	else
	{
		GetErrorStatus(Ret, SQLStmt, u_hStmt);
		return FALSE;
	}

    u_EndTime = ::GetTickCount();

	return TRUE;
}

BOOL CDBObject::PreQuery(CString TableName, 
					 	CString SelFields, CString Key, CString SortCols, 
       		   		 	CStringArray* csBindKey, int nRowSize)
{
	if (u_hStmt != NULL)
		SQLFreeStmt(u_hStmt, SQL_DROP);;
    RETCODE Ret = SQLAllocStmt(u_hdbc, &u_hStmt);
	if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
		return FALSE;
	
	Ret = InternalQuery(TableName, SelFields, Key, SortCols, 
							csBindKey, nRowSize, &u_hStmt);
	if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
		return FALSE;
    
	return TRUE;
}       		   		 

void CDBObject::GetResultSetInfo(int* pnCol, 
								 CStringArray* pFieldNameArray,
					             CDWordArray* pColLen) 
{
	SDWORD nCol;
	SQLColAttributes(u_hStmt, 1, SQL_COLUMN_COUNT, NULL, 0, NULL, &nCol);
	if (pFieldNameArray != NULL)
		pFieldNameArray->RemoveAll();
	if (pColLen != NULL)
		pColLen->RemoveAll();
	if (pnCol != NULL)
		*pnCol = nCol;
	for (int i = 0; i < nCol; i++)
	{
		char szBuffer[128];
		SWORD nDesc;
		SQLColAttributes(u_hStmt, i + 1, SQL_COLUMN_LABEL, szBuffer, 128, &nDesc, NULL);
		pFieldNameArray->Add(szBuffer);
		SDWORD nLen;
		SQLColAttributes(u_hStmt, i + 1, SQL_COLUMN_LENGTH, NULL, 0, NULL, &nLen);
		pColLen->Add(nLen);
	}
}

int CDBObject::InternalQuery(CString TableName, CString SelFields,
       			     		 CString Key, CString SortCols, 
       			     		 CStringArray* csBindKey, int nRowSize, HSTMT* phStmt)
{
    if (TableName == "" || SelFields == "")
        return SQL_EXEC_ERROR;

	int nCount = 0;
	CStringArray* pcsBindFields = NULL;
	CString* ValueSet = NULL;
	if (csBindKey != NULL)
	{
		nCount = csBindKey->GetSize();
    	if (nCount > 0)
    	{
    		ValueSet = new CString[nCount];
    		for (int i = 0; i < nCount; i++)
    			ValueSet[i] = csBindKey->GetAt(i);
    		pcsBindFields = new CStringArray;
    		if (!GetFieldArray(Key, pcsBindFields, nCount))
    		{
  				delete [] ValueSet;                     
  				delete pcsBindFields;
    			return SQL_ERROR;
    		}
    	}	
    }
    
    CString		SQLStmt;
    RETCODE		Ret;
	int			RetValue;
	
    if (SelFields == "")
		SelFields = "*";

	SQLStmt = "SELECT " + SelFields + " FROM " + TableName;
	
	if (Key != "")
		SQLStmt += " WHERE " + Key;  
        
    if (SortCols != "")
        SQLStmt += " ORDER BY " + SortCols;

	CheckSQL(SQLStmt);

    if (nRowSize >= 0)
    {
	    if (!SetCursorOption(nRowSize, phStmt))
		{
   			delete [] ValueSet;
			delete pcsBindFields;
			return SQL_ERROR;
		}
    }

	if (u_DBType == "ORACLE")
		Substitute(SQLStmt, "Substring (", "Substr(");

    Ret = SQLPrepare(*phStmt, (UCHAR FAR*)(const char*)SQLStmt, SQL_NTS);
	if (Ret == SQL_SUCCESS)
	{                                 
		GetColumnsName(TableName);
		for (int i = 0; i < nCount; i++)
		{
            SWORD sqlType = GetSQLType(pcsBindFields->GetAt(i));
			Ret = SQLSetParam(*phStmt, i + 1, SQL_C_CHAR,
							  sqlType,
							  ValueSet[i].GetLength(), 0,	    	
							  (PTR)(const char*)ValueSet[i], NULL);
	        if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
	        {
    	    	delete [] ValueSet;
  				delete pcsBindFields;
				return SQL_ERROR;
    	    }	
		}

		Ret = SQLExecute(*phStmt);
	    if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
		{
			RetValue = SQL_ERROR;
			GetErrorStatus(Ret, SQLStmt, *phStmt);
		}
	    else
	    {
			if (nRowSize >= 0)
				RetValue = QueryBindCol(*phStmt, nRowSize);
			else
				RetValue = QueryBindCol(*phStmt);
		}    
    }                            
    else
	{
        RetValue = SQL_ERROR;
		GetErrorStatus(Ret, SQLStmt, *phStmt);
	}

  	delete [] ValueSet;                     
  	delete pcsBindFields;

	u_EndTime = ::GetTickCount();

	return RetValue;
}

BOOL CDBObject::SetCursorOption(int nRowSize, HSTMT* phStmt)
{
	u_nRowSize = nRowSize;
	BOOL bSuccess = TRUE;
	RETCODE Ret = SQLSetStmtOption(*phStmt, SQL_CONCURRENCY, SQL_CONCUR_VALUES);
    bSuccess = bSuccess && (Ret == SQL_SUCCESS || Ret == SQL_SUCCESS_WITH_INFO);
	Ret = SQLSetStmtOption(*phStmt, SQL_CURSOR_TYPE, SQL_CURSOR_STATIC);
    bSuccess = bSuccess && (Ret == SQL_SUCCESS || Ret == SQL_SUCCESS_WITH_INFO);
	Ret = SQLSetStmtOption(*phStmt, SQL_ROWSET_SIZE, nRowSize);
    bSuccess = bSuccess && (Ret == SQL_SUCCESS || Ret == SQL_SUCCESS_WITH_INFO);
	Ret = SQLSetCursorName(*phStmt, (UCHAR FAR*)"C1", SQL_NTS);
    bSuccess = bSuccess && (Ret == SQL_SUCCESS || Ret == SQL_SUCCESS_WITH_INFO);
	return bSuccess;
}

BOOL CDBObject::GetFieldArray(CString KeyStr, CStringArray* psBindFields, int nCount)
{
    const char* pKeyStr = (const char*)KeyStr;
    int nSize = KeyStr.GetLength();
    for (int i = 0; i < nSize; i++)
    {
        if (pKeyStr[i] == '?') 
        {
            int nLast = -1;
            int nFirst = -1;
            for (int j = i - 1; j >= 0; j--)
            {
                char ch = pKeyStr[j];
                if (nLast == -1 && 
                	(ch == ' ' || ch == '>' || ch == '<' || ch == '='))
                {
                	continue;
                }
                else if (nLast == -1 && ch != ' ')
                	nLast = j;
                else if (nLast != -1 && (ch == ' ' || ch == '('))
                {	
                    nFirst = j + 1;
                    break;      
                }
                else if (nLast != -1 && j == 0)
                	nFirst = 0;    
            }
            if (nLast == -1 || nFirst == -1)
            	return FALSE;
            psBindFields->Add(KeyStr.Mid(nFirst, nLast - nFirst + 1));	
        }
    }
    if (nCount != psBindFields->GetSize())
    	return FALSE;
    return TRUE;	 
}

BOOL CDBObject::ExecuteSQLStmt(LPCTSTR pSQLStmt, int* pCount)
{
	try	// 2017-03-15, To trap and ignore all unexpected exceptions.
	{	
		CString SQLStmt = pSQLStmt;
		CheckSQL(SQLStmt);

		HSTMT hStmt;
   		int Ret = SQLAllocStmt(u_hdbc, &hStmt);
		if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
			return FALSE;
		
		// 2017-11-29, Replicating ODBC handle for recovery from ODBC hangs.
		g_hStmt = hStmt;
			
		if ((Ret = ExecuteSQL(SQLStmt, &hStmt)) == SQL_SUCCESS)
		{
    		if (pCount != NULL)
			{
				int nRetValue;
				Ret = SQLRowCount(hStmt, (SDWORD FAR*)&nRetValue);
    			if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
					nRetValue = -1;	
				*pCount = nRetValue;
			}
		}                           

		u_EndTime = ::GetTickCount();

		SQLFreeStmt(hStmt, SQL_DROP);

		if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
			return FALSE;	
	} catch(...){}
	return TRUE;
}

BOOL CDBObject::ExecuteSQLStmt(CString SQLStmt, CObArray* pObArray, CStringList* pSelectFields)
{
	CheckSQL(SQLStmt);

   	RETCODE Ret;
    HSTMT hStmt;
    Ret = SQLAllocStmt(u_hdbc, &hStmt);
    if (Ret != SQL_SUCCESS)
    	return FALSE;

	Ret = ExecuteSQL(SQLStmt, &hStmt);
    if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
	{
//		GetErrorStatus(Ret, SQLStmt, hStmt);
		return FALSE;
	}
	QueryBindCol(hStmt, pSelectFields);
	::ReleaseObArray(pObArray);
	Ret = FetchAllRecords(pObArray, &hStmt);
	SQLFreeStmt(hStmt, SQL_DROP);

	if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
		return FALSE;

    u_EndTime = ::GetTickCount();
    
	return TRUE;
}

int CDBObject::ExecuteSQL(CString SQLStmt, HSTMT* phStmt)
{                                  
    RETCODE Ret;
	int		RetValue;    
    BOOL    bInHtmt = (phStmt == NULL);                 
    HSTMT hStmt;
    if (bInHtmt)
    {
    	Ret = SQLAllocStmt(u_hdbc, &hStmt);
		if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
			return SQL_ERROR;
		phStmt = &hStmt;	
	}

	if (u_DBType == "ORACLE")
		Substitute(SQLStmt, "Substring (", "Substr(");

	int Len = SQLStmt.GetLength() + 1;
    Ret = SQLExecDirect(*phStmt, (UCHAR FAR*)SQLStmt.GetBuffer(Len), SQL_NTS);
    SQLStmt.ReleaseBuffer();
    if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
	{
        RetValue = SQL_ERROR;
		GetErrorStatus(Ret, SQLStmt, *phStmt);
	}
    else
		RetValue = SQL_SUCCESS;
	
	if (bInHtmt)
		Ret = SQLFreeStmt(hStmt, SQL_DROP);

    if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
		return SQL_ERROR;
		    
    return RetValue;
}                                                                     


int CDBObject::QueryBindCol(HSTMT hStmt, int nRowSize)
{
 	SWORD        ColType, ColNameLen, Nullable, Scale;
    RETCODE      Ret;
    UCHAR        ColName[32];
    int          i, RetValue;

    u_nResultColsR = 0;
    Ret = SQLNumResultCols(hStmt, &u_nResultColsR);
    if ((u_nResultColsR < MAXCOLS) && (Ret == SQL_SUCCESS || Ret == SQL_SUCCESS_WITH_INFO)) 
    {
        i = 0;
        while (u_pData[i] != NULL)
        {
    	    delete u_pData[i];
    	    u_pData[i++] = NULL;
    	}
        i = 0;
        while (u_pOutLen[i] != NULL)
        {
    	    delete u_pOutLen[i];
    	    u_pOutLen[i++] = NULL;
    	}

	   //u_pSeleFieldList->RemoveAll(); 
        for (i = 0; i < u_nResultColsR; i++)
        {
			Ret = SQLDescribeCol(hStmt, i + 1, ColName,
								(SWORD) sizeof(ColName), &ColNameLen, &ColType,
								&(u_ColLenR[i]), &Scale, &Nullable);  

			//u_pSeleFieldList->AddTail((const char*)ColName);
			
			if (Ret == SQL_SUCCESS || Ret == SQL_SUCCESS_WITH_INFO)
			{
				u_ColTypeR[i] = ColType;
				u_ColLenR[i] = Display_Size(ColType, u_ColLenR[i], ColNameLen);
				if (ColType != SQL_LONGVARCHAR)	 //not large data
				{
					u_pData[i] = (UCHAR *) new char[((int)u_ColLenR[i] + 1) * nRowSize];
					u_pOutLen[i] = new SDWORD[nRowSize];
					Ret = SQLBindCol(hStmt, i + 1, SQL_C_CHAR, u_pData[i],              
								 u_ColLenR[i] + 1, u_pOutLen[i]);
				}
				if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
				{					 
			    	RetValue = SQL_ERROR;
			    	break;
			    }	
			}
			else
			{					 
		    	RetValue = SQL_ERROR;
		    	break;
		    }	
			RetValue = SQL_SUCCESS;
        }
    }                     
    else 
     	RetValue = SQL_ERROR;              
    	
    return RetValue;	
}

int CDBObject::QueryBindCol(HSTMT hStmt, CStringList* pSelFields)
{
    SWORD        ColType, ColNameLen, Nullable, Scale;
    RETCODE      Ret;
    UCHAR        ColName[128];
    int          RetValue;

    u_nResultCols = 0;
    Ret = SQLNumResultCols(hStmt, &u_nResultCols);
    if (Ret == SQL_SUCCESS || Ret == SQL_SUCCESS_WITH_INFO)
    {
        if (u_nResultCols == 0)
        	return Ret;
        
		if (pSelFields != NULL)
			pSelFields->RemoveAll();

        ReleaseData();
        for (int i = 0; i < u_nResultCols; i++)
        {
			Ret = SQLDescribeCol(hStmt, i + 1, ColName,
								(SWORD) sizeof(ColName), &ColNameLen, &ColType,
								&(u_ColLen[i]), &Scale, &Nullable); 
			if (Ret == SQL_SUCCESS || Ret == SQL_SUCCESS_WITH_INFO)
			{
				if (pSelFields != NULL)
					pSelFields->AddTail((const char*)ColName);

				u_ColType[i] = ColType;	// ColType == -9 ? SQL_C_CHAR : ColType;
				u_ColLen[i] = Display_Size(ColType, u_ColLen[i], ColNameLen);
				if (ColType != SQL_LONGVARCHAR)	 //not large data
				{
					u_Data[i] = (UCHAR *) new char[(int) u_ColLen[i] + 1 + 2];	// 2017-03-11, adding +2 to avoid deletion error since u_ColLen[i] == 1 might cause to write "err"
					Ret = SQLBindCol(hStmt, i + 1, SQL_C_CHAR, u_Data[i],              
								 u_ColLen[i] + 1, &u_OutLen[i]);
				}
				if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
				{					 
			    	RetValue = SQL_ERROR;
			    	break;
			    }	
			}
			else
			{					 
		    	RetValue = SQL_ERROR;
		    	break;
		    }	
			RetValue = SQL_SUCCESS;
        }
    }                     
    else
    { 
    	RetValue = SQL_ERROR;              
    }
    	
    return RetValue;	
}


SDWORD CDBObject::Update(CString TableName, CString UpdFields, CString Key)
{                  
    if (TableName == "\0" || UpdFields == "\0")
        return -1;
        
    SDWORD  Records;
    CString SQLStmt;
    SQLStmt = "UPDATE " + TableName + " SET " + UpdFields;

    if (Key != "\0")
        SQLStmt += " WHERE " + Key;
                           
	CheckSQL(SQLStmt);

	RETCODE Ret;
    HSTMT hStmt;
    Ret = SQLAllocStmt(u_hdbc, &hStmt);
    if (Ret != SQL_SUCCESS)
    	return -1;

    if (ExecuteSQL(SQLStmt, &hStmt) == SQL_SUCCESS)
    {
    	Ret = SQLRowCount(hStmt, (SDWORD FAR*)&Records);
    	if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
			Records = -1;			
    }                           
    else
	{
		GetErrorStatus(Ret, SQLStmt, hStmt);
		Records = -1;
    }
	
    Ret = SQLFreeStmt(hStmt, SQL_DROP);

    u_EndTime = ::GetTickCount();

    return Records;
}
                              

SDWORD CDBObject::Update(CString TableName, CString UpdFields, 
						 CStringArray* Values, CString WhereStr)
{                  
	RETCODE RetCode;         
    SDWORD  RecordCount;
    CString UpdateStmt;
    
    if (TableName == "\0" || UpdFields == "\0")
        return -1;

    int FieldCnt = Values->GetSize();
    CString* FieldSet = new CString[FieldCnt];
    CString* ValueSet = new CString[FieldCnt];

    CString CheckUpdateStmt = "UPDATE " + TableName + " SET ";
    UpdateStmt = "UPDATE " + TableName + " SET ";
    //Get Update Fields and Bulid Update Statement
    int i = 0;
    char* pszBuffer = new char[UpdFields.GetLength() + 1];
	strcpy(pszBuffer, (const char*)UpdFields);
	int nCount = 0;
	CString StrTok;
	int n = 0;
    while (::GetTokStr(pszBuffer, StrTok, nCount, tSep))
    {
		if (i != 0)
		{
			UpdateStmt += ",";
			CheckUpdateStmt += ",";
		}
		UpdateStmt += StrTok + "=?";
		CheckUpdateStmt += StrTok + "=" + Values->GetAt(n++);

		if (i > FieldCnt - 1)
		{
    		delete [] FieldSet;
    		delete [] ValueSet;
			delete [] pszBuffer;
    		return -1;
		}

    	FieldSet[i++] = StrTok;
    }
	
	delete [] pszBuffer;

    if (WhereStr != "\0")
	{
		UpdateStmt += " Where " + WhereStr;
		CheckUpdateStmt += " Where " + WhereStr;
	}

    //Copy Update Values
    for (i = 0; i < FieldCnt; i++)
		ValueSet[i] = Values->GetAt(i);

	CheckSQL(CheckUpdateStmt);
	
    HSTMT hStmt;
    RetCode = SQLAllocStmt(u_hdbc, &hStmt);
    if (RetCode != SQL_SUCCESS && RetCode != SQL_SUCCESS_WITH_INFO)
    {
    	delete [] FieldSet;
    	delete [] ValueSet;
    	return -1;
    }
    	
	if (u_DBType == "ORACLE")
		Substitute(UpdateStmt, "Substring (", "Substr(");

    RetCode = SQLPrepare(hStmt, (UCHAR FAR*)(const char*)UpdateStmt, SQL_NTS);
    if (RetCode != SQL_SUCCESS && RetCode != SQL_SUCCESS_WITH_INFO)
    {
  		GetErrorStatus(RetCode, UpdateStmt, hStmt);
		SQLFreeStmt(hStmt, SQL_DROP);
    	delete [] FieldSet;
    	delete [] ValueSet;
    	return -1;
    }

	GetColumnsName(TableName);
	SDWORD cbValue = SQL_LEN_DATA_AT_EXEC(0);
	char szZero[] = "0";
	for (i = 0; i < FieldCnt; i++)
	{
        SWORD sqlType = GetSQLType(FieldSet[i]);
		if (sqlType == SQL_LONGVARCHAR)
		{
			RetCode = SQLBindParameter(hStmt, i+1, SQL_PARAM_INPUT,
						SQL_C_CHAR,	sqlType,
						ValueSet[i].GetLength(), 0,	    	
						(PTR)i, 0, &cbValue);
		}
		else if (sqlType != SQL_CHAR && sqlType != SQL_VARCHAR && ValueSet[i].GetLength() == 0)
		{ //if interger value is empty str, use 0 to avoid sql error
			RetCode = SQLSetParam(hStmt, i+1, SQL_C_CHAR,
						sqlType, 1, 0, szZero, NULL);
		}
		else
		{
			RetCode = SQLSetParam(hStmt, i + 1, SQL_C_CHAR,
							  sqlType,
							  ValueSet[i].GetLength(), 0,	    	
							  (PTR)(const char*)ValueSet[i], NULL);
		}
		if (RetCode != SQL_SUCCESS)
	    {
	    	SQLFreeStmt(hStmt, SQL_DROP);
	    	delete [] FieldSet;
	    	delete [] ValueSet;
	    	return -1;
	    }
	}                                    
	
	RetCode = SQLExecute(hStmt); 
	PTR		pToken;
	char	szBuffer[301];
	while (RetCode == SQL_NEED_DATA)
	{
		RetCode = SQLParamData(hStmt, &pToken);
		int NextPos = 0;
		if (RetCode == SQL_NEED_DATA)
			while (GetFromCString(ValueSet[(int)pToken], szBuffer, 300, &NextPos))
				SQLPutData(hStmt, szBuffer, SQL_NTS);
	}
	if (RetCode == SQL_SUCCESS || RetCode == SQL_SUCCESS_WITH_INFO)
    {
        RetCode = SQLRowCount(hStmt, (SDWORD FAR*) &RecordCount);
        if (RetCode != SQL_SUCCESS)
            RecordCount = -1;
	}		
	else
	{
		GetErrorStatus(RetCode, UpdateStmt, hStmt);
		RecordCount = -1;                       
	}

    SQLFreeStmt(hStmt, SQL_DROP);

    delete [] FieldSet;
    delete [] ValueSet;

    u_EndTime = ::GetTickCount();
	
	return RecordCount;    	
}
                              
SDWORD CDBObject::Insert(CString TableName, CString InsFields, CString Values)
{                
    SDWORD  Records;
    CString SQLStmt;

    if (TableName == "\0" || InsFields == "\0" || Values == "\0")
        return SQL_ERROR;
    else
    {
        SQLStmt = "INSERT INTO " + TableName +"(" + InsFields + ")";
        SQLStmt += " VALUES (" +Values + ")";
    }

	CheckSQL(SQLStmt);

   	RETCODE Ret;
    HSTMT hStmt;
    Ret = SQLAllocStmt(u_hdbc, &hStmt);
    if (Ret != SQL_SUCCESS)
    	return -1;

    if (ExecuteSQL(SQLStmt, &hStmt) == SQL_SUCCESS)
    {
    	Ret = SQLRowCount(hStmt, (SDWORD FAR*)&Records);
    	if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
			Records = -1;			
    }                           
    else
	{
    	Records = -1;
		GetErrorStatus(Ret, SQLStmt, hStmt);
	}
    	
    SQLFreeStmt(hStmt, SQL_DROP);

    u_EndTime = ::GetTickCount();

    return Records;
}


SDWORD CDBObject::Insert(CString TableName, CString InsFields, CStringArray* Values)
{       
	RETCODE RetCode;         
    SDWORD  RecordCount;
    CString InsertStmt;
    
    if (TableName == "\0" || InsFields == "\0")
        return -1;

    int		FieldCnt = Values->GetSize();
    CString* FieldSet = new CString[FieldCnt];
    CString* ValueSet = new CString[FieldCnt];

    InsertStmt = "INSERT INTO " + TableName;
    InsertStmt += "(" + InsFields + ") VALUES ";
	//Get Field names
    int i = 0;
    char* pszBuffer = new char[InsFields.GetLength() + 1];
	strcpy(pszBuffer, (const char*)InsFields);
    
	int nCount = 0;
	CString StrTok;
    while (::GetTokStr(pszBuffer, StrTok, nCount, tSep))
    {
    	if (i > FieldCnt - 1)
		{
    		delete [] FieldSet;
    		delete [] ValueSet;
			delete [] pszBuffer;
    		return -1;
		}

		FieldSet[i++] = StrTok;
    }
	delete [] pszBuffer;

	//Get field Values and append (?,?,..) to InsertStmt
	CString StrInsert = InsertStmt;
	for (i = 0; i < FieldCnt; i++)
	{   
		ValueSet[i] = Values->GetAt(i);
	    if (i == 0)
		{
	    	InsertStmt += "(";
			StrInsert += "(";
		}
	    else
		{
	    	InsertStmt += ",";
			StrInsert += ",";
		}
	    InsertStmt += "?";
		StrInsert += ValueSet[i];
	}
    InsertStmt += ")";
	StrInsert += ")";

	CheckSQL(StrInsert);
	
    HSTMT hStmt;
    RetCode = SQLAllocStmt(u_hdbc, &hStmt);
    if (RetCode != SQL_SUCCESS && RetCode != SQL_SUCCESS_WITH_INFO)
    {
    	delete [] FieldSet;
    	delete [] ValueSet;
    	return -1;
    }

	if (u_DBType == "ORACLE")
		Substitute(InsertStmt, "Substring (", "Substr(");

    RetCode = SQLPrepare(hStmt, (UCHAR FAR*)(const char*)InsertStmt, SQL_NTS);
    if (RetCode != SQL_SUCCESS && RetCode != SQL_SUCCESS_WITH_INFO)
    {
    	SQLFreeStmt(hStmt, SQL_DROP);
    	delete [] FieldSet;
    	delete [] ValueSet;
    	return -1;
    }

	GetColumnsName(TableName);
	SDWORD cbValue = SQL_LEN_DATA_AT_EXEC(0);  //large data
	char szZero[] = "0";
	for (i = 0; i < FieldCnt; i++)
	{
        SWORD sqlType = GetSQLType(FieldSet[i]);
		if (sqlType == SQL_LONGVARCHAR)
		{
			RetCode = SQLBindParameter(hStmt, i+1, SQL_PARAM_INPUT,
						SQL_C_CHAR,	sqlType,
						ValueSet[i].GetLength(), 0,	    	
						(PTR)i, 0, &cbValue);
		}
		else if (sqlType != SQL_CHAR && sqlType != SQL_VARCHAR && ValueSet[i].GetLength() == 0)
		{ //if interger value is empty str, use 0 to avoid sql error
			RetCode = SQLSetParam(hStmt, i+1, SQL_C_CHAR,
						sqlType, 1, 0, szZero, NULL);
		}
		else
		{
			RetCode = SQLSetParam(hStmt, i+1, SQL_C_CHAR,
						sqlType,
						ValueSet[i].GetLength(), 0,	    	
						(PTR)(const char*)ValueSet[i], NULL);
		}
		if (RetCode != SQL_SUCCESS)
	    {
	    	SQLFreeStmt(hStmt, SQL_DROP);
	    	delete [] FieldSet;
	    	delete [] ValueSet;
	    	return -1;
	    }
	}
         
	RetCode = SQLExecute(hStmt);  
	PTR		pToken;
	char	szBuffer[301];
	while (RetCode == SQL_NEED_DATA)
	{
		RetCode = SQLParamData(hStmt, &pToken);
		int NextPos = 0;
		if (RetCode == SQL_NEED_DATA)
			while (GetFromCString(ValueSet[(int)pToken], szBuffer, 300, &NextPos))
				SQLPutData(hStmt, szBuffer, SQL_NTS);
	}
 	if (RetCode == SQL_SUCCESS)
    {
        RetCode = SQLRowCount(hStmt, (SDWORD FAR*) &RecordCount);
        if (RetCode != SQL_SUCCESS)
            RecordCount = -1;
	}		
	else
	{
		RecordCount = -1;                       
		GetErrorStatus(RetCode, InsertStmt, hStmt);
	}
    RetCode = SQLFreeStmt(hStmt, SQL_DROP);

    delete [] FieldSet;
    delete [] ValueSet;
 
	u_EndTime = ::GetTickCount();
	
	return RecordCount;    	
}

BOOL CDBObject::GetFromCString(CString& Data, char* szBuffer, int nMax, int* NextRead)
{                 
	//will return TRUE once even if string is empty
	if (*NextRead > Data.GetLength())
		return FALSE;  //done get
	strcpy(szBuffer, (const char*)Data.Mid(*NextRead, nMax));
	*NextRead = *NextRead + nMax;
	return TRUE;
}

SDWORD CDBObject::Delete(CString TableName, CString Key)
{                 
    if (TableName == "\0")
    	return -1;
    	
    SDWORD  Records;
    CString SQLStmt;
   	RETCODE Ret;

    SQLStmt = "DELETE FROM " + TableName;

    if (Key != "\0")
        SQLStmt += " WHERE " + Key;
 
	CheckSQL(SQLStmt);

    HSTMT hStmt;
    Ret = SQLAllocStmt(u_hdbc, &hStmt);
    if (Ret != SQL_SUCCESS)
    	return -1;

    if (ExecuteSQL(SQLStmt, &hStmt) == SQL_SUCCESS)
    {
    	Ret = SQLRowCount(hStmt, (SDWORD FAR*)&Records);
    	if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
			Records = -1;			
    }                           
    else
	{
		GetErrorStatus(Ret, SQLStmt, hStmt);
		Records = -1;
    }
	
    SQLFreeStmt(hStmt, SQL_DROP);

    u_EndTime = ::GetTickCount();

    return Records;
}


BOOL CDBObject::DeleteByPos(CString TableName, long StartPos, long EndPos)
{

    HSTMT	hStmt;
	char	SqlStmt[128];
    RETCODE	Ret;
	UDWORD	crow;
	UWORD	*pRowStatus;
	
	sprintf(SqlStmt, "Delete From %s Where Current of C1", (const char*)TableName);
    Ret = SQLAllocStmt(u_hdbc, &hStmt);
	if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
    	return FALSE;

    UDWORD TopPos = 1;
	pRowStatus = new UWORD[u_nRowSize];
    Ret = SQLExtendedFetch(u_hStmt, SQL_FETCH_ABSOLUTE, TopPos, &crow, pRowStatus);
	if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
    {
        SQLFreeStmt(hStmt, SQL_DROP);
        delete pRowStatus;
    	return FALSE;
	}
	for (UDWORD i = (UDWORD)StartPos + 1; i <= (UDWORD)EndPos + 1; i++)
	{
	    if (i >= TopPos && i <= TopPos + crow - 1)
			Ret = SQLSetPos(u_hStmt, (UWORD)(i - TopPos + 1), SQL_POSITION, SQL_LOCK_NO_CHANGE);	
	    else
	    {
	    	TopPos = i;
	    	Ret = SQLExtendedFetch(u_hStmt, SQL_FETCH_ABSOLUTE, TopPos, &crow, pRowStatus);
			if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
		    {
		        SQLFreeStmt(hStmt, SQL_DROP);
		        delete pRowStatus;
		    	return FALSE;
			}
			Ret = SQLSetPos(u_hStmt, (UWORD)1, SQL_POSITION, SQL_LOCK_NO_CHANGE);	
    	}
		if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
	    {
	        SQLFreeStmt(hStmt, SQL_DROP);
	        delete pRowStatus;
	    	return FALSE;
		}
   		
   		Ret = SQLExecDirect(hStmt, (UCHAR FAR*)SqlStmt, SQL_NTS);
		if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
	    {
	        SQLFreeStmt(hStmt, SQL_DROP);
	        delete pRowStatus;
			GetErrorStatus(Ret, SqlStmt, u_hStmt);
			return FALSE;
		}
    }
    
    delete pRowStatus;
	Ret = SQLFreeStmt(hStmt, SQL_DROP);
	return Ret == SQL_SUCCESS || Ret == SQL_SUCCESS_WITH_INFO;
}

BOOL CDBObject::UpdateByPos(CString TableName, long nPos,
						   CString UpdFields, CStringArray* pUpdValue)
{
    char 	tSeps[] = ",";
    int		FieldCnt = 0;
    RETCODE	Ret;
	UDWORD	crow;
	UWORD	*pRowStatus;
	
	pRowStatus = new UWORD[u_nRowSize];

	CString WhereStr = "Current Of C1";
    Ret = SQLExtendedFetch(u_hStmt, SQL_FETCH_ABSOLUTE, nPos + 1, &crow, pRowStatus);
	if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
    {
        delete pRowStatus;
    	return FALSE;
	}

	Ret = SQLSetPos(u_hStmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE);	
	if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
    {
        delete pRowStatus;
    	return FALSE;
	}

    delete pRowStatus;

	long i = Update(TableName, UpdFields, pUpdValue, WhereStr);
    return i >= 0;
}


void CDBObject::ReleaseData()
{
	int i = 0;
	while (u_Data[i] != NULL)
	{
		delete u_Data[i];
		u_Data[i++] = NULL;
	} 	
}


int CDBObject::FetchAllRecords(CObArray *pStrArray, HSTMT* phStmt)
{
	int				i, n;
	RETCODE			RetCode;
	CStringArray*	pList;
	
	for (n = 0; n < MAX_QUERY_ROWS; n++)	
    {
        RetCode = SQLFetch(*phStmt);
        if (RetCode == SQL_SUCCESS || RetCode == SQL_SUCCESS_WITH_INFO)
	    {	    
    	    pList = new CStringArray;
			if (pList == (0))
			{
				u_MsgStr.Format("%s \n Records(%d) \n", "MEMORY Exhausted !", n);
				AfxMessageBox(u_MsgStr);
				break;
			}
        	pStrArray->SetAtGrow(n, pList);
        	// retrieves all columns for the selected row
        	for (i = 0; i < u_nResultCols; i++)
        	{
    			if (u_ColType[i] != SQL_LONGVARCHAR)
				{
					if (u_OutLen[i] == SQL_NULL_DATA)
               			strcpy((char*) u_Data[i], "\0");
            		else if ((UDWORD) u_OutLen[i] > u_ColLen[i])
                		strcpy((char*) u_Data[i], "err");
            		pList->Add((const char*) u_Data[i]);
				//	str = (const char*)u_Data[i]; 
				//	str.TrimLeft();
				//	str.TrimRight();
            	//	pList->Add(str);
				}
				else //large data
				{
					char	buffer[301];
					SDWORD	cbVal;
					RETCODE	ret;
					CString	str = "";
					do{
						ret = SQLGetData(*phStmt, i+1, SQL_C_CHAR, buffer, 300, &cbVal);
						if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
							str += buffer;
						else
							GetErrorStatus(ret, "SQLGetData", *phStmt);
					} while (ret == SQL_SUCCESS_WITH_INFO);
            		pList->Add(str);
				}
        	}
        }
        else // SQL_NO_DATA (RetCode == 100)
            break;                
	}
    return SQL_SUCCESS;
}

int CDBObject::FetchRecords(CObArray *pStrArray, long irow)
{                                
	int				i, n;
	RETCODE			RetCode;
	CStringArray*	pList;
	   
	UDWORD	crow;
	UWORD	*pRowStatus;
	
	irow++;
	pRowStatus = new UWORD[u_nRowSize];
    RetCode = SQLExtendedFetch(u_hStmt, SQL_FETCH_ABSOLUTE, irow, &crow, pRowStatus);
    if (RetCode == SQL_SUCCESS || RetCode == SQL_SUCCESS_WITH_INFO)
    {
		for (n = 0; n < (int)crow && n < MAX_QUERY_ROWS; n++)
		{
		    pList = new CStringArray;
	    	pStrArray->Add(pList);
	    	// retrieves all columns for the selected row
	    	for (i = 0; i < u_nResultColsR; i++)
	    	{
    			if (u_ColTypeR[i] != SQL_LONGVARCHAR)
				{
	        		if (*(u_pOutLen[i]+n) == SQL_NULL_DATA)
	           			strcpy((char*) (u_pData[i] + n*(u_ColLenR[i] + 1)), "\0");
	        		else if ((UDWORD) *(u_pOutLen[i]+n) > u_ColLenR[i])
	            		strcpy((char*) (u_pData[i] + n*(u_ColLenR[i] + 1)), "err");
	        		pList->Add((const char*)(u_pData[i] + n*(u_ColLenR[i] + 1)));
				//	str = (const char*)(u_pData[i] + n*(u_ColLenR[i] + 1)); 
				//	str.TrimLeft();
				//	str.TrimRight();
            	//	pList->Add(str);
		    	}
				else //large data
				{
					char	buffer[301];
					SDWORD	cbVal;
					RETCODE	ret;
					CString	str = "";
					ret = SQLSetPos(u_hStmt, (UWORD)n+1, SQL_POSITION, SQL_LOCK_NO_CHANGE);	
					do{
						ret = SQLGetData(u_hStmt, i+1, SQL_C_CHAR, buffer, 300, &cbVal);
						if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
							str += buffer;
						else
							GetErrorStatus(ret, "SQLGetData", u_hStmt);
					} while (ret == SQL_SUCCESS_WITH_INFO);
            		pList->Add(str);
				}
        	}
	    }
    }
    delete pRowStatus;
    return RetCode;
}

CStringList* CDBObject::GetColName(CString StrTableName)
{
	GetColumnsName(StrTableName);
	return u_pFieldNameList;
}

int** CDBObject::GetColumnAttrib(CString isTableName, CStringList& isColumns)
{
	CString		sColumns;
	CObArray	sColumnAttribs;
	POSITION	pos = isColumns.GetHeadPosition();
	for (int n = 0; n < isColumns.GetCount(); n++)
	{
		if (n) sColumns += ",";
		sColumns += isColumns.GetNext(pos);
	}
	GetColumnAttrib(isTableName, sColumns, sColumnAttribs);
	int  nSize = sColumnAttribs.GetSize();
	int* npQueryColumnTypes = new int[nSize];
	int* npQueryColumnSizes = new int[nSize];
	for (int i = 0; i < nSize; i++)
	{
		// AccessDB association
		//	SQLType	AccessType	Length		DataSize
		//		4	autono		4			4
		//		5	int			4			4
		//		7	float		4			4
		//		12	text		char(n)		text(n/2)
		FieldData* pFieldData = (FieldData*)sColumnAttribs.GetAt(i);
		npQueryColumnTypes[i] = pFieldData->SQLType; 
		npQueryColumnSizes[i] = pFieldData->Length;
	//	npQueryColumnSizes[i] = 3 * ((pFieldData->SQLType == 12) ? pFieldData->Length : pFieldData->Length*4);
	}
	::ReleaseObArray(&sColumnAttribs);
	int** npColumnDefs = (int**)new long[2];
	npColumnDefs[0] = npQueryColumnTypes; 
	npColumnDefs[1] = npQueryColumnSizes;
	return npColumnDefs;
}

int** CDBObject::GetColumnAttrib(CObArray *ipObStrArray, CStringList& isColumns)
{
	int  nSize = isColumns.GetCount();
	if (ipObStrArray->GetSize() <= 0 || nSize <= 0)
		return NULL;
	int* npQueryColumnTypes = new int[nSize];
	int* npQueryColumnSizes = new int[nSize];
	CStringArray *pValues   = (CStringArray *)ipObStrArray->GetAt(0);
	for (int i = 0; i < nSize; i++)
	{
		CString value  = pValues->GetAt(i);
		int		length = value.GetLength();
		if (value != "" && value.GetLength() > 1 && (value.Find("-") == 0 || value.Find("-") < 0 && value.Find(":") < 0 && value.GetAt(1) >= '0' && value.GetAt(1) <= '9'))
		{
			int nPoint = value.Find(".");
			npQueryColumnTypes[i] = nPoint < 0 ? 5 : 7; // Number: SQLType = 5 (INT), 7(Float)
			npQueryColumnSizes[i] = nPoint < 0 ? length : value.Find(".000") > 0 ? length-6 : length;
		}
		else
		{
			npQueryColumnTypes[i] = 12; 
			npQueryColumnSizes[i] = length*2+8;
		}
	}
	int** npColumnDefs = (int**)new long[2];
	npColumnDefs[0] = npQueryColumnTypes; 
	npColumnDefs[1] = npQueryColumnSizes;
	return npColumnDefs;
}

BOOL CDBObject::GetColumnAttrib(CString StrTableName, 
								CString StrColumn, CObArray& StrArray)
{
	BOOL bRet = TRUE;

	HSTMT hStmt;
	SQLAllocStmt(u_hdbc, &hStmt);
	
	CObArray* pObArray = new CObArray;

	RETCODE RetCode = SQLColumns(hStmt,
                    	 NULL, 0,              /* All qualifiers */
                  		 NULL, 0,              /* All owners     */
                     	 (UCHAR FAR*)(const char*)StrTableName, SQL_NTS,  /* Table Name */
                     	 NULL, 0);             /* All columns    */
                     	 
	UCHAR szColName[129];
	SDWORD Length;
	SWORD DataType;
	SDWORD cbColName, cbLength, cbDataType;
	if (RetCode == SQL_SUCCESS)             
	{    
		SQLBindCol(hStmt, 4, SQL_C_CHAR, szColName, 128, &cbColName); 
		SQLBindCol(hStmt, 5, SQL_C_SSHORT, &DataType, 0, &cbDataType); 
		SQLBindCol(hStmt, 8, SQL_C_SLONG, &Length, 0, &cbLength); 

		while (TRUE)
		{
			RetCode = SQLFetch(hStmt);
			if (RetCode == SQL_ERROR)
			{
				GetErrorStatus(RetCode, "GetColumnAttrib", hStmt);
				SQLFreeStmt(hStmt, SQL_DROP);
				delete pObArray;
				return FALSE;
			}
			else if (RetCode == SQL_SUCCESS || RetCode == SQL_SUCCESS_WITH_INFO)
			{
				FieldData* pFieldData = new FieldData;
				pFieldData->FieldName = szColName;
				// 2017-03-23, Converting MySQL VAR_CHAR(-9) code into normal SQL_VARCHAR(12) code
				pFieldData->SQLType = DataType == -9 ? SQL_VARCHAR : DataType;	// DataType;
				pFieldData->Length  = Length;
				pObArray->Add(pFieldData);
			}
			else
				break;
		}
	}
	else
	{
		GetErrorStatus(RetCode, "GetColumnAttrib", hStmt);
		SQLFreeStmt(hStmt, SQL_DROP);
		//delete [] pColName;
		::ReleaseObArray(pObArray);
		delete pObArray;
		return FALSE;
	}

	//delete [] pColName;
	SQLFreeStmt(hStmt, SQL_DROP);

	::ReleaseObArray(&StrArray);
	if (StrColumn != "*")
	{
		int nCount = 0;
		CString StrTok;
		while (::GetTokStr(StrColumn, StrTok, nCount, ","))
		{
			BOOL bFound = FALSE;

			// <UH, 2014-04-02, To extract aggregative field name from selected columns, e.g. Sum(KPI). />
			int  n = StrTok.Find("(");
			if (n > 0)
			{
				StrTok = StrTok.Mid(n+1);
				if ((n = StrTok.Find(")")) > 0)
					StrTok = StrTok.Left(n);
			}

			for (int i = 0; i < pObArray->GetSize(); i++)
			{
				FieldData* pFieldData = (FieldData*)pObArray->GetAt(i);
				if (pFieldData->FieldName.CompareNoCase(StrTok) == 0)
				{
				//	pObArray->RemoveAt(i);		// reserved for matching the next grouped arguments.
				//	StrArray.Add(pFieldData);
					FieldData *pFDDesc = new FieldData;
					pFDDesc->FieldName = pFieldData->FieldName;
					pFDDesc->SQLType   = pFieldData->SQLType;
					pFDDesc->Length	   = pFieldData->Length;
					StrArray.Add(pFDDesc);
					bFound = TRUE;
					break;
				}
			}

			if (!bFound)
				bRet = FALSE;
		}
	}
	else
	{
		for (int i = 0; i < pObArray->GetSize(); i++)
		{
			FieldData* pFieldData = (FieldData*)pObArray->GetAt(i);
			StrArray.Add(pFieldData);
		}
		pObArray->RemoveAll();	// 2014-04-22, keep all elements to be used by StrArray, otherwise will be deleted by ::ReleaseObArray(pObArray);
	}

	::ReleaseObArray(pObArray);
	delete pObArray;

	return bRet;
}

/*
BOOL CDBObject::GetColumnAttribX(CString StrTableName, 
								CString StrColumn, CObArray& StrArray)
{
	HSTMT hStmt;
	SQLAllocStmt(u_hdbc, &hStmt);
	
	CObArray* pObArray = new CObArray;

	RETCODE RetCode = SQLColumns(hStmt,
                    	 NULL, 0,              
                  		 NULL, 0,              
                     	 (UCHAR FAR*)(const char*)StrTableName, SQL_NTS,  
                     	 NULL, 0);            
                     	 
	UCHAR szColName[129];
	SDWORD Length;
	SWORD DataType;
	SDWORD cbColName, cbLength, cbDataType;
	if (RetCode == SQL_SUCCESS)             
	{    
		SQLBindCol(hStmt, 4, SQL_C_CHAR, szColName, 128, &cbColName); 
		SQLBindCol(hStmt, 5, SQL_C_SSHORT, &DataType, 0, &cbDataType); 
		SQLBindCol(hStmt, 8, SQL_C_SLONG, &Length, 0, &cbLength); 

		while (TRUE)
		{
			RetCode = SQLFetch(hStmt);
			if (RetCode == SQL_ERROR)
			{
				GetErrorStatus(RetCode, "GetColumnAttrib", hStmt);
				SQLFreeStmt(hStmt, SQL_DROP);
				delete pObArray;
				return FALSE;
			}
			else if (RetCode == SQL_SUCCESS || RetCode == SQL_SUCCESS_WITH_INFO)
			{
				FieldData* pFieldData = new FieldData;
				pFieldData->FieldName = szColName;
				pFieldData->SQLType = DataType;
				pFieldData->Length = Length;
				pObArray->Add(pFieldData);
			}
			else
				break;
		}
	}
	else
	{
		GetErrorStatus(RetCode, "GetColumnAttrib", hStmt);
		SQLFreeStmt(hStmt, SQL_DROP);
		//delete [] pColName;
		::ReleaseObArray(pObArray);
		delete pObArray;
		return FALSE;
	}

	//delete [] pColName;
	SQLFreeStmt(hStmt, SQL_DROP);

	BOOL bRet = TRUE;
	StrArray.RemoveAll();
	if (StrColumn != "*")
	{
		char * p = strtokex((LPSTR)(LPCTSTR)StrColumn, ",");
		while (p != NULL)
		{
			BOOL bFound = FALSE;
			for (int i = 0; i < pObArray->GetSize(); i++)
			{
				FieldData* pFieldData = (FieldData*)pObArray->GetAt(i);
				if (pFieldData->FieldName.CompareNoCase(p) == 0)
				{
					pObArray->RemoveAt(i);
					StrArray.Add(pFieldData);
					bFound = TRUE;
					break;
				}
			}
			
			if (!bFound)
				bRet = FALSE;

			p = strtokex(NULL, ",");
		}
	}
	else
	{
		for (int i = 0; i < pObArray->GetSize(); i++)
		{
			FieldData* pFieldData = (FieldData*)pObArray->GetAt(i);
			StrArray.Add(pFieldData);
		}
		pObArray->RemoveAll();
	}

	::ReleaseObArray(pObArray);
	delete pObArray;

	return bRet;
}
*/

void CDBObject::GetColumnsName(CString tTableName)//, HSTMT* phStmt)
{
    int          i;
    RETCODE      RetCode;
	UCHAR  		 szColName[STR_LEN];
	SWORD  		 ColType;
	SDWORD 		 cbColName, cbColType;

/* Declare storage locations for bytes available to return */
    HSTMT        hStmt;
 	SQLAllocStmt(u_hdbc, &hStmt);

	RetCode = SQLColumns(hStmt,
                    	 NULL, 0,              /* All qualifiers */
                  		 NULL, 0,              /* All owners     */
                     	 (UCHAR FAR*)(const char*)tTableName, SQL_NTS,  /* Table Name */
                     	 NULL, 0);             /* All columns    */
                     	 
	if (RetCode == SQL_SUCCESS)             
	{    
	    char		TempStr[255];

	    u_pFieldNameList->RemoveAll(); 
	    u_pTypeList->RemoveAll(); 
	    
		SQLBindCol(hStmt, 4, SQL_C_CHAR, szColName, STR_LEN, &cbColName);
		SQLBindCol(hStmt, 5, SQL_C_SHORT, &ColType, 0, &cbColType);
		i = 0;
		while (TRUE)
		{
			RetCode = SQLFetch(hStmt);
			if (RetCode == SQL_SUCCESS || RetCode == SQL_SUCCESS_WITH_INFO)
			{
				u_ColType[i++] = ColType;
				u_pFieldNameList->AddTail((const char*)szColName);
				u_pTypeList->AddTail((const char*)itoa(ColType, TempStr, 10));
			}
			else
			{
				break;
			}
		}	
	}                   
	SQLFreeStmt(hStmt, SQL_DROP);    
}

                
SWORD CDBObject::GetSQLType(CString FieldName)
{                           
    int i, size;
	CStringList* pList;
	POSITION pos;
	char szFieldStr[128];
	char* ptr;
	
	if ((ptr=strchr((const char*)FieldName,'[')) != NULL)
		*ptr = ' ';                                    
		if ((ptr=strchr((const char*)FieldName,']')) != NULL)
			*ptr = ' ';
	sscanf((const char*)FieldName, "%s", szFieldStr);
	strupr(szFieldStr);
	pList = u_pFieldNameList;
	size = pList->GetCount();	
	pos = pList->GetHeadPosition();
	for (i = 0; i < size; i++)
	{
		CString Str = pList->GetNext(pos);
		Str.MakeUpper();
		
		if (strcmp(szFieldStr, (const char*)Str) == 0)
		 	 return u_ColType[i];
	}

	return SQL_EXEC_ERROR;
}                                


void CDBObject::GetErrorStatus(RETCODE RetCode, CString ErrAPI, HSTMT hStmt)
{                                                                  
	//char MsgStr[512];  // DBY Lu
	//CString MsgStr;	 // Mask By LBY, add as member of DBOject u_MsgStr
	//MsgStr = new char[512];	// ABY Lu
	switch (RetCode)
	{
		case SQL_SUCCESS:
  			 u_MsgStr.Format("%s \n %s \n", ErrAPI, "SUCCESS");
			 break;
		case SQL_ERROR:
		case SQL_SUCCESS_WITH_INFO:
			{
	   		 char    szSQLState[20];
			 SDWORD  pfNativeError=0;
			 SWORD   cbErrorMsgMax;
			 char* pszErrorMsg = new char[1024];

			 SQLError(u_henv, u_hdbc, hStmt, //SQL_NULL_HSTMT,
   		     		  (UCHAR FAR*)szSQLState, &pfNativeError, 
   	   		 		  (UCHAR FAR*)pszErrorMsg, 512, &cbErrorMsgMax);
  			 if (CString(szSQLState) == "40001")
				 u_DeadLockError = TRUE;
			 u_MsgStr.Format("%d \n %s \n %s \n %s \n", 
  			 		 	   RetCode, ErrAPI, szSQLState, pszErrorMsg);

			 delete [] pszErrorMsg;
			 break;
			}
		case SQL_NEED_DATA:
  			 u_MsgStr.Format("%s \n %s \n", ErrAPI, "NEED_DATA");
			 break;
		case SQL_INVALID_HANDLE:
  			 u_MsgStr.Format("%s \n %s \n", ErrAPI, "INVALID_HANDLE");
			 break;
		default:
  			 u_MsgStr.Format("%s \n %s \n", ErrAPI, "OTHER");
	}

	if (u_ShowErrorMsg == 1 || u_ShowErrorMsg == 2)	// show message for programer	97/04/14
		AfxMessageBox(u_MsgStr);
	//else if (u_ShowErrorMsg == 0)	// show message for user	97/04/14
	//{
	//	MsgStr.Format("資料庫存取失敗!");
	//	AfxMessageBox(MsgStr);
	//}
	
	//AfxMessageBox(MsgStr);
}

                  
CString CDBObject::CreateFieldString(FieldData *pFieldData)
{              
	char	szFieldStr[512];                                    
	
    switch(pFieldData->SQLType)
    {
        case SQL_CHAR:
				sprintf(szFieldStr, "%s CHAR(%d)", 
					(const char*)pFieldData->FieldName,	pFieldData->Length);
			break;
	//	case -1:	// 2017-04-05, Access memo type (SQL_LONGVARCHAR)
 	    case -9:	// 2017-03-23, Converting MySQL VAR_CHAR(-9) code into normal SQL_VARCHAR(12) code
	//  case -10:	// 2017-09-09, SQL Type MEDIUMTEXT in MySQL
		case SQL_LONGVARCHAR:
        case SQL_VARCHAR:
				sprintf(szFieldStr, "%s VARCHAR(%d)", 
					(const char*)pFieldData->FieldName,	pFieldData->Length);
			break;
        case SQL_SMALLINT:
			sprintf(szFieldStr, "%s SMALLINT", 
					(const char*)pFieldData->FieldName);
			break;
        case SQL_TINYINT:
			sprintf(szFieldStr, "%s TINYINT", 
					(const char*)pFieldData->FieldName);
			break;
        case SQL_INTEGER:
			sprintf(szFieldStr, "%s INTEGER", 
					(const char*)pFieldData->FieldName);
			break;
        case SQL_NUMERIC:
			sprintf(szFieldStr, "%s NUMERIC", 
					(const char*)pFieldData->FieldName);
			break;
        case SQL_REAL:
			sprintf(szFieldStr, "%s REAL", 
					(const char*)pFieldData->FieldName);
			break;
        case SQL_FLOAT:
			sprintf(szFieldStr, "%s FLOAT", 
					(const char*)pFieldData->FieldName);
			break;
        case SQL_DOUBLE:
			sprintf(szFieldStr, "%s DOUBLE", 
					(const char*)pFieldData->FieldName);
			break;
        case SQL_TIME:
			sprintf(szFieldStr, "%s TIME", 
					(const char*)pFieldData->FieldName);
			break;
        case SQL_DATE:  
			sprintf(szFieldStr, "%s DATE", 
					(const char*)pFieldData->FieldName);
			break;
		case SQL_TIMESTAMP:
			sprintf(szFieldStr, "%s TIMESTAMP", 
					(const char*)pFieldData->FieldName);
			break;
        case SQL_BIT: // xBase logical field
			sprintf(szFieldStr, "%s BIT", 
					(const char*)pFieldData->FieldName);
			break;
        default:
             TRACE("unknown type %d\n", pFieldData->SQLType);
                 return "\0";
    }
    return CString(szFieldStr);
}


/////////////////////////////////////////////////////////////////////////////
/**   global function adapted from ODBC examples ***/

UDWORD CDBObject::Display_Size(SWORD ColType, UDWORD ColLen, SWORD ColNameLen)
{
    // computes column size based on field size and heading length
    switch(ColType)
    {
	//	case -1:			// 2017-04-05, Access memo type(SQL_LONGVARCHAR)
		case -9:			// 2017-03-11, MySQL VARCHAR type code
	//  case -10:	// 2017-09-09, SQL Type MEDIUMTEXT in MySQL
        case SQL_CHAR:
        case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
                 return(max(ColLen, (UDWORD) ColNameLen));
        case SQL_SMALLINT:
                 return(max(6, (int)ColNameLen));
        case SQL_TINYINT:
                 return(max(4, (int)ColNameLen));
        case SQL_INTEGER:
                 return(max(11, (int)ColNameLen));
        case SQL_DECIMAL:
        case SQL_NUMERIC:
        case SQL_REAL:
        case SQL_FLOAT:
        case SQL_DOUBLE:
                 return(max(MAX_NUM_STRING_SIZE, (int)ColNameLen));
        case SQL_TIME:
        case SQL_DATE:  
            return(min(10, (int)ColLen));
		case SQL_TIMESTAMP:
                 return(max((int)ColLen, (int)(UDWORD) ColNameLen));
        case SQL_BIT: // xBase logical field
             return(max(2, (int)ColNameLen));
        default:
             TRACE("unknown type %d\n", ColType);
                 return 1;
    }
    return 0;
}


SWORD CDBObject::SQLType_TO_CType(SWORD ColType)
{
    switch(ColType)
    {
        case SQL_TIMESTAMP:
             //return SQL_C_DATE;
	//	case -1:			// 2017-04-05, Access memo type (SQL_LONGVARCHAR)
 		case -9:			// 2017-03-11, MySQL VARCHAR type code
	//  case -10:	// 2017-09-09, SQL Type MEDIUMTEXT in MySQL
        case SQL_CHAR:
        case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
        case SQL_SMALLINT:
		case SQL_TINYINT:
        case SQL_INTEGER:
        case SQL_DECIMAL:
        case SQL_NUMERIC:
        case SQL_REAL:
        case SQL_FLOAT:
        case SQL_DOUBLE:
        case SQL_TIME:
        case SQL_BIT: // xBase logical field
			break;
        default:
             TRACE("unknown type %d\n", ColType);
    }
    return SQL_C_CHAR;
}

BOOL CDBObject::Open(LPCSTR lpszDSN)
{
	BOOL RetValue = TRUE;
	try	// 2017-03-15, To trap and ignore all unexpected exceptions.
	{	
		//	if (CString(lpszDSN).Find("UID=sa") > 0)
		//		u_LikeToken = '%';	// It is a SQL Server Data Source.
		//	else
		//		u_LikeToken = '*';	// It is not a SQL Server Data Source.
   
		RETCODE Ret = SQLAllocConnect(u_henv, &u_hdbc);

		// 2017-11-29, Replicating ODBC handle for recovery from ODBC hangs.
		g_henv = u_henv;
		g_hdbc = u_hdbc;

		// 2017-03-11, Allocate a connection handle
		// RETCODE Ret = SQLAllocHandle(SQL_HANDLE_DBC, u_henv, &u_hdbc);

		RetValue = RetValue && (Ret == SQL_SUCCESS || Ret == SQL_SUCCESS_WITH_INFO); 
    
		if (!RetValue)
 			GetErrorStatus(Ret,"SQLAllocConnect", u_hdbc);

		Ret = SQLSetConnectOption(u_hdbc, SQL_ODBC_CURSORS, SQL_CUR_USE_ODBC);
   		if (Ret != SQL_SUCCESS)
   			GetErrorStatus(Ret,"SQLSetConnectOption", u_hdbc);
	#ifdef MANNUAL_COMMIT
		if (mApp->u_bMannual_Commit)
		{
		Ret = SQLSetConnectOption(u_hdbc, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF);
   		if (Ret != SQL_SUCCESS)
   			GetErrorStatus(Ret,"SQLSetConnectOption", u_hdbc);
		}
	#endif
		RetValue = RetValue && (Ret == SQL_SUCCESS || Ret == SQL_SUCCESS_WITH_INFO); 

		UCHAR     szConnStrOut[256];
		SWORD     pcbConnStrOut;
	//	Ret = SQLDriverConnect(u_hdbc, NULL,
		Ret = SQLDriverConnect(u_hdbc, AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
	//   			(UCHAR FAR*)lpszDSN, SQL_NTS, (UCHAR FAR*)szConnStrOut,
	//    			(UCHAR FAR*)(LPCTSTR)(CString(lpszDSN)+";Stmt=Set Names \'latin1\';"), SQL_NTS, (UCHAR FAR*)szConnStrOut,
    				(UCHAR FAR*)(LPCTSTR)(CString(lpszDSN)+";Charset=big5;"), SQL_NTS, (UCHAR FAR*)szConnStrOut,

      				255, &pcbConnStrOut, SQL_DRIVER_COMPLETE);  
	// 2017-03-12, Finally, MySQL ODBC 5.3 driver Chinese coding problem is solved by appending "Charset=big5;" to DSN string
	// 2017-03-12, Results the same as "Stmt=Set Names \'latin1\';"
	//	SQLExec("SET NAMES \'latin1\'");		// 2017-03-12, show variables like 'character_set_%'; for MySQL ODBC 3.5 Driver
	//	SQLExec("SET NAMES \'big5\'");			// 2017-03-12, show variables like 'character_set_%'; for MySQL ODBC 5.1 Driver

		RetValue = RetValue && (Ret == SQL_SUCCESS || Ret == SQL_SUCCESS_WITH_INFO); 

		if (!RetValue)
		{
			::MessageBox(NULL,CString(lpszDSN)+"資料庫開啟失效, 無法繼續操作!","系統錯誤",MB_ICONEXCLAMATION);
    		u_hdbc = NULL;
		}
		else
			u_DBType = GetDBMSName();
   	} catch(...){}
    return RetValue;			
}


long CDBObject::GetTotalSize(CString TableName, CString StrField, CString Key, CStringArray* pBindKey)
{                                
    CObArray* pObArray = new CObArray;
	CString StrCount = "Count(" + StrField + ") ";
	if (Key != "")
		Key = " Where " + Key;
	CString SQLStmt = "Select " + StrCount + "From " + TableName + Key;
	BOOL RetCode = ExecuteSQLStmt(SQLStmt, pObArray);
	if (!RetCode)
		return -1; 
	if (pObArray->GetSize() == 0)
	{
		delete pObArray;
		return -1;
	}

	CStringArray* pStrArray = (CStringArray*)pObArray->GetAt(0);
	long Rows = atol((const char*)pStrArray->GetAt(0));
	
	::ReleaseObArray(pObArray);
	delete pObArray;
	
	return Rows;
}
int CDBObject::BeginTransaction()
{
	RETCODE ret = SQLSetConnectOption(u_hdbc, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF);
   	if (ret == SQL_ERROR)
   		GetErrorStatus(ret,"SQLSetConnectOption",u_hdbc);
	return (ret == SQL_SUCCESS);
}
int CDBObject::EndTransaction()
{
	RETCODE ret = SQLSetConnectOption(u_hdbc, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_ON);
   	if (ret == SQL_ERROR)
   		GetErrorStatus(ret,"SQLSetConnectOption",u_hdbc);
	return (ret == SQL_SUCCESS);
}
int CDBObject::Commit()
{
//	RETCODE ret = SQLTransact(u_henv, u_hdbc, SQL_COMMIT);
	RETCODE ret = SQLTransact(SQL_NULL_HENV, u_hdbc, SQL_COMMIT);	// for each connection only
	return (ret == SQL_SUCCESS);
}

int CDBObject::RollBack()
{
//	RETCODE ret = SQLTransact(u_henv, u_hdbc, SQL_ROLLBACK);
	RETCODE ret = SQLTransact(SQL_NULL_HENV, u_hdbc, SQL_ROLLBACK);	// for each connection only
	return (ret == SQL_SUCCESS);
}

BOOL CDBObject::DeadLockCheck()
{
	BOOL	CheckStatus = u_DeadLockError;
	u_DeadLockError = FALSE;
	return CheckStatus;
}

long CDBObject::GetTotalSize(SDWORD RowNumber, BOOL& bLast)
{                                
	RETCODE	RetCode;
	UDWORD	crow;
	UWORD	*pRowStatus;
	SDWORD	row;
	
	pRowStatus = new UWORD[u_nRowSize];
    if (RowNumber > 0)
    	RetCode = SQLExtendedFetch(u_hStmt, SQL_FETCH_ABSOLUTE, RowNumber, &crow, pRowStatus);
	else
    	RetCode = SQLExtendedFetch(u_hStmt, SQL_FETCH_LAST, 0, &crow, pRowStatus);
    delete pRowStatus;
  
	//if (RetCode != SQL_SUCCESS)
   	//	GetErrorStatus(RetCode,"SQLExtendedFetch",u_hdbc);
	
	if (RetCode == SQL_SUCCESS)
        bLast = FALSE;
    else if (RetCode == SQL_NO_DATA_FOUND)
        bLast = TRUE;
	RetCode = SQLRowCount(u_hStmt, &row);
    if (RetCode == SQL_SUCCESS)
	   	return row;
    
   	return 0;
}

void CDBObject::CheckSQL(CString& StrSQL)
{
	if (u_TraceSQL)
	{
	#if 0
		CDlgSQLTrace* pDlg = new CDlgSQLTrace;
		//pDlg->u_pAppTraceMode = u_pAppTraceMode;
		pDlg->u_StrSQL = StrSQL;
		if (u_EndTime - u_StartTime > 0)
			pDlg->u_TimePeriod = (double)((u_EndTime - u_StartTime) / 1000.0);
		if (pDlg->DoModal() == IDOK)
			StrSQL = pDlg->u_StrSQL;
		delete pDlg;
	#endif
	}
	u_StartTime = ::GetTickCount();
}

int	CDBObject::SQLExec(CString SQLString, void *Params,...)
{
//	CStringArray& Parms, CObArray *pResult
	char	**Values = (char**)&Params;
	int		pos, i=0, Size = SQLString.GetLength();
	CString	SQLStmt = "";
	while (1)
	{
		pos = SQLString.Find("?");
		if (pos < 0)
		{
			SQLStmt += SQLString;
			if (SQLStmt.Mid(0,6).CompareNoCase("select") == 0)
			{
				if (!ExecuteSQLStmt(SQLStmt, (CObArray *)*Values))
					return -1; 
				return ((CObArray *)*Values)->GetSize();
			}
			return ExecuteSQLStmt(SQLStmt);
		}
		if (pos > 0)
			SQLStmt += SQLString.Left(pos);
		SQLStmt += *((CString*)*(Values++));
		Size -= (pos+1);
		SQLString = SQLString.Right(Size);
	}
}

void CDBObject::EnumIndex(CString StrTable,
						  CStringArray& ColumnArray,
						  CStringArray& IndexArray)//, HSTMT* phStmt)
{
    int          i;
    RETCODE      RetCode;
	//UCHAR  		 szTableName[128];
	//SDWORD 		 cbTableName;

	UCHAR  		 szColName[128];
	SDWORD 		 cbColName;

	UCHAR  		 szIndexName[128];
	SDWORD 		 cbIndexName;
	
	/* Declare storage locations for bytes available to return */
    HSTMT        hStmt;
 	SQLAllocStmt(u_hdbc, &hStmt);

	RetCode = SQLStatistics(hStmt,
                    		NULL, 0,  /* All qualifiers */
                  			NULL, 0,  /* All owners     */
                     		(unsigned char*)(LPCTSTR)StrTable, SQL_NTS,/* Table Name */
                     		SQL_INDEX_ALL, 
							SQL_ENSURE);            /* All columns    */
                     	 
	if (RetCode == SQL_SUCCESS)             
	{    
		ColumnArray.RemoveAll();
		IndexArray.RemoveAll();

		SQLBindCol(hStmt, 6, SQL_C_CHAR, szIndexName, 128, &cbIndexName);
		SQLBindCol(hStmt, 9, SQL_C_CHAR, szColName, 128, &cbColName);
		i = 0;
		while (TRUE)
		{
			RetCode = SQLFetch(hStmt);
			if (RetCode == SQL_SUCCESS || RetCode == SQL_SUCCESS_WITH_INFO)
			{
				ColumnArray.Add((char*)szColName);
				IndexArray.Add((char*)szIndexName);
			}
			else if (RetCode == SQL_NO_DATA_FOUND)
			{
				break;
			}
			else if (RetCode == SQL_ERROR)
			{
				GetErrorStatus(RetCode, "SQLFetch", hStmt);
				break;
			}
		}	
	}                   
	SQLFreeStmt(hStmt, SQL_DROP);    
}

void CDBObject::EnumTable(CStringArray& TableArray)//, HSTMT* phStmt)
{
    int          i;
    RETCODE      RetCode;
	UCHAR  		 szTableName[128];
	SDWORD 		 cbTableName;

	UCHAR  		 szTableType[128];
	SDWORD 		 cbTableType;

/* Declare storage locations for bytes available to return */
    HSTMT        hStmt;
 	SQLAllocStmt(u_hdbc, &hStmt);

	RetCode = SQLTables(hStmt,
                    	NULL, 0,              /* All qualifiers */
                  		NULL, 0,              /* All owners     */
                     	NULL, 0, /* Table Name */
                     	NULL, 0);             /* All columns    */
                     	 
	if (RetCode == SQL_SUCCESS)             
	{    
		TableArray.RemoveAll();
		SQLBindCol(hStmt, 3, SQL_C_CHAR, szTableName, 128, &cbTableName);
		SQLBindCol(hStmt, 4, SQL_C_CHAR, szTableType, 128, &cbTableType);
		i = 0;
		while (TRUE)
		{
			RetCode = SQLFetch(hStmt);
			if (RetCode == SQL_SUCCESS || RetCode == SQL_SUCCESS_WITH_INFO)
			{
				if (CString(szTableType).CompareNoCase("TABLE") == 0)
					TableArray.Add((char*)szTableName);
			}
			else
			{
				break;
			}
		}	
	}                   
	SQLFreeStmt(hStmt, SQL_DROP);    
}

SWORD CDBObject::MapSQLType(CString StrType)
{
	if (StrType.CompareNoCase("char") == 0)
		return SQL_CHAR;
	else if (StrType.CompareNoCase("varchar") == 0)
		return SQL_VARCHAR;
	else if (StrType.CompareNoCase("smallint") == 0)
		return SQL_SMALLINT;
	else if (StrType.CompareNoCase("int") == 0)
		return SQL_INTEGER;
	else if (StrType.CompareNoCase("real") == 0)
		return SQL_REAL;
	else if (StrType.CompareNoCase("float") == 0)
		return SQL_FLOAT;
	else if (StrType.CompareNoCase("double") == 0)
		return SQL_DOUBLE;
	else if (StrType.CompareNoCase("tinyint") == 0)
		return SQL_TINYINT;

	return 0;
}

BOOL CDBObject::TableExists(CString TableName)
{
	int nOld = u_ShowErrorMsg;
	u_ShowErrorMsg = 0;
	BOOL b = ExecuteSQLStmt("SELECT * FROM " + TableName + " WHERE 1=2");
	u_ShowErrorMsg = nOld;
	return b;
}


BOOL GetTokStr(const char* pSource, CString& StrDest, int& Start, CString Delimeter)
{
    int Len = strlen(pSource);
    
    if (Start >= Len || Start < 0)
    	return FALSE;

    int i;             
    int Count;
    BOOL LeadByteAppear = FALSE; 
    for (i = Start; i < Len; i++)
    {
        if (i != Len - 1)
        {
        	if (LeadByteAppear)
        	{
            	LeadByteAppear = FALSE;
            	continue;
        	}
        	else if (IsDBCSLeadByte(pSource[i]))
        	{
            	LeadByteAppear = TRUE;
            	continue;
        	}
        }
        
		BOOL bFound = (Delimeter.Find(pSource[i]) >= 0);
		if (i == Len - 1 || bFound)
     	{  	
           	if (bFound)
           		Count = i - Start;
           	else 
           	    Count = i - Start + 1;
           	    
           	char* pDest = new char[Count + 1];
			strncpy(pDest, pSource + Start, Count);
            pDest[Count] = '\0';
			StrDest = pDest;
			delete [] pDest;
            Start = i + 1;
            return TRUE;
        } 
    }                 
    return FALSE;
}

void ReleaseObArray(CObArray* pReleasedArray)
{ 
    if (pReleasedArray == NULL)
    	return;
	int n = pReleasedArray->GetSize();
	while(--n >= 0) RemoveAndDelete(pReleasedArray, n);
//	int ArraySize;
//	ArraySize = pReleasedArray->GetSize();
//	for (int i = 0; i < ArraySize; i++)
//		RemoveAndDelete(pReleasedArray, 0);
}

void RemoveAndDelete(CObArray* pArray, int nIndex)
{
	CObject* pList = pArray->GetAt(nIndex);
	pArray->RemoveAt(nIndex);
    if (pList == NULL)
       	return;
    if (pList->IsKindOf(RUNTIME_CLASS(CGdiObject)))
       	((CGdiObject*)pList)->DeleteObject();
    delete pList;                      
}
/*
SWORD CDBObject::GetSQLType(CString FieldName, CStringArray* pFieldNameList)
{                           
    int i;
	CStringArray* pList;
	char* ptr;

	if ((ptr=strchr((const char*)FieldName,'[')) != NULL)
		*ptr = ' ';                                    
		if ((ptr=strchr((const char*)FieldName,']')) != NULL)
			*ptr = ' ';
	CString szFieldStr = FieldName;
	int	pos = FieldName.Find(".");
	if (pos > 0)
		szFieldStr = FieldName.Right(FieldName.GetLength() - pos - 1);
	szFieldStr.MakeUpper();
//	if (pFieldNameList == NULL)
//		pList = u_pFieldNameList;
//	else
		pList = pFieldNameList;
	for (i = 0; i < pList->GetSize(); i++)
	{
		CString Str = pList->GetAt(i);
		Str.MakeUpper();
		
		if (szFieldStr == Str)
		{
			switch(u_ColType[i])
			{
				case SQL_CHAR:
				case SQL_VARCHAR:
				case SQL_DATE:
				case SQL_TIME:
					 return SQL_CHAR;
			}
			return u_ColType[i];
		}
	}

	return SQL_EXEC_ERROR;
}                                

BOOL CDBObject::PreQuery(CString TableName, 
					 	CString SelFields, CString Key, CString SortCols, 
       		   		 	CStringArray* csBindKey, int nRowSize, CStringArray* pSelectFields)
{
	if (u_hStmt != NULL)
		SQLFreeStmt(u_hStmt, SQL_DROP);;
    RETCODE Ret = SQLAllocStmt(u_hdbc, &u_hStmt);
	if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
		return FALSE;
	
	Ret = InternalQuery(TableName, SelFields, Key, SortCols, 
							csBindKey, nRowSize, &u_hStmt);
	if (Ret != SQL_SUCCESS && Ret != SQL_SUCCESS_WITH_INFO)
		return FALSE;
    
	return TRUE;
}       		   		 
int CDBObject::GetSQLColDef(CString FieldName, CStringArray* pFieldList, CString *FieldDef)
{                           
    int i, size, ColLength;
	char szFieldStr[128], Digit[12];
	size = FieldName.Find(".");
	if (size >= 0)
		FieldName = FieldName.Right(FieldName.GetLength()-size-1);
	sscanf((const char*)FieldName, "%s", szFieldStr);
	strupr(szFieldStr);
	for (i = 0; i < pFieldList->GetSize(); i++)
	{
		CString Str = pFieldList->GetAt(i);
		Str.MakeUpper();
		if (strcmp(szFieldStr, (const char*)Str) == 0)
		{
			itoa((ColLength = u_ColLen[i]),Digit,10);
			switch(u_ColType[i])
			{
				case SQL_CHAR:
					sprintf(szFieldStr, "%-12s\tChar(%3s)    ", 
							(const char*)FieldName,	Digit);
					break;
				case SQL_VARCHAR:
					sprintf(szFieldStr, "%-12s\tVarChar(%3s) ", 
							(const char*)FieldName,	Digit);
					break;
				case SQL_SMALLINT:
					sprintf(szFieldStr, "%-12s\tSmallInt(%3s)", 
							(const char*)FieldName,	Digit);
					break;
				case SQL_INTEGER:
					sprintf(szFieldStr, "%-12s\tInteger(%3s) ", 
							(const char*)FieldName,	Digit);
					break;
				case SQL_NUMERIC:
					sprintf(szFieldStr, "%-12s\tNumeric(%3s) ", 
							(const char*)FieldName,	Digit);
					break;
				case SQL_REAL:
					sprintf(szFieldStr, "%-12s\tReal(%3s)    ", 
							(const char*)FieldName,	Digit);
					break;
				case SQL_FLOAT:
					sprintf(szFieldStr, "%-12s\tFloat(%3s)   ", 
							(const char*)FieldName,	Digit);
					break;
				case SQL_DOUBLE:
					sprintf(szFieldStr, "%-12s\tDouble(%3s)  ", 
							(const char*)FieldName,	Digit);
					break;
				case SQL_TIME:
					sprintf(szFieldStr, "%-12s\tTime(%3s)    ", 
							(const char*)FieldName,	Digit);
					break;
				case SQL_DATE:  
					sprintf(szFieldStr, "%-12s\tDate(%3s)    ", 
							(const char*)FieldName,	Digit);
					break;
				case SQL_TIMESTAMP:
					sprintf(szFieldStr, "%-12s\tTimeStamp(%3s)", 
							(const char*)FieldName,	Digit);
					break;
				case SQL_BIT: // xBase logical field
					sprintf(szFieldStr, "%-12s\tBit(%3s)     ", 
							(const char*)FieldName,	Digit);
					break;
				default:
					sprintf(szFieldStr, "%-12s\tUnDef()      ", 
							(const char*)FieldName);
			}
			if (FieldDef != NULL)
				*FieldDef = szFieldStr;
			return ColLength;
		}
	}
	if (FieldDef != NULL)
		*FieldDef = "";
	return -1;
}                                
*/

// Internal function used to change SQL command name for different DBMS.
// Usage: substitute ALL OldStr in SrcStr with NewStr.
void CDBObject::Substitute(CString& SrcStr, CString OldStr, CString NewStr)
{                           
	int length, start;
	CString TmpStr;

	TmpStr.Format("%s",(const char*) SrcStr);
	length = OldStr.GetLength();
	TmpStr.MakeUpper();
	OldStr.MakeUpper();
	// infinite loop when OldStr=NewStr or OldStr is subset of NewStr.
	start = TmpStr.Find(OldStr);
	while (start != -1)
	{
		TmpStr = TmpStr.Left(start) + NewStr + TmpStr.Mid(start+length);
		SrcStr = SrcStr.Left(start) + NewStr + SrcStr.Mid(start+length);
		start = TmpStr.Find(OldStr);
	}
}

CString CDBObject::GetDBMSName()
{
	RETCODE nRet;
	char* pValue = new char[256];
	SWORD nCount;
	nRet = SQLGetInfo(u_hdbc, SQL_DBMS_NAME, pValue, 256, &nCount);
	CString StrRet = "GENERAL";
	if (nRet != SQL_ERROR)
	{
		CString StrName = pValue;
		StrName.MakeUpper();
		if (StrName.Find("ORACLE") >= 0)
			StrRet = "ORACLE";
		if (StrName.Find("ACCESS") >= 0)
			StrRet = "ACCESS";
		if (StrName.Find("SQL SERVER") >= 0)
			StrRet = "SQL SERVER";
	}
	delete [] pValue;

	return StrRet;
}

void CDBObject::GetQuoteArray(CWordArray& QuoteArray, 
								 CString& StrField,
								 CObArray* pObArray,
								 CDBObject* pDBObj)
{
	StrField = "";
	for (int i = 0; i < pObArray->GetSize(); i++)
	{
		FieldData* pFieldData = (FieldData*)pObArray->GetAt(i);
		StrField += pFieldData->FieldName + ",";
		if (pFieldData->SQLType == -9 ||		// 2017-03-12, MySQL ODBC 5.1 driver
			pFieldData->SQLType == SQL_CHAR ||
			pFieldData->SQLType == SQL_VARCHAR)
			QuoteArray.Add(1);
		else
			QuoteArray.Add(0);
	}
	StrField = StrField.Left(StrField.GetLength() - 1);
}

void CDBObject::InsertXls(CString isDbXls, CString isTableName, CObArray* ipAccessScheme, CObArray* ipResultSet)
{
//	CString isDbXls = ".\\Data\\xlsData.xls";			// Create Excel File
	CString sDriver = "Microsoft Excel Driver (*.xls)";	// Excel Installation Driver
	CString sSql;
	CDatabase database;

	TRY
	{
		sSql.Format("DRIVER={%s};DSN=\'\';FIRSTROWHASNAMES=1;READONLY=FALSE;CREATE_DB=\"%s\";DBQ=%s",
					sDriver, isDbXls, isDbXls);
		if (database.OpenEx(sSql, CDatabase::noOdbcDialog))
		{
		//	sSql = "CREATE TABLE Basic (Name TEXT, Age NUMBER)";
			sSql = "CREATE TABLE " + isTableName + "(";
			for (int i = 0; i < ipAccessScheme->GetSize(); i++)
			{
				if (i > 0)
					sSql += ",";
				CStringArray* pScheme = (CStringArray*)ipAccessScheme->GetAt(i);
				sSql += pScheme->GetAt(0);
				if (*(LPCTSTR)pScheme->GetAt(1) == 's')
					sSql += " TEXT";
				else
					sSql += " NUMBER"; 
			}
			sSql += ")";
			database.ExecuteSQL(sSql);

		//	sSql = "INSERT INTO Basic (Name,Age) VALUES (\'Andrew Lin\',26)";
			for (int n = 0; n < ipResultSet->GetSize(); n++)
			{
				CString sQMark, sFields, sValues;
				CStringArray* pColumns = (CStringArray*)ipResultSet->GetAt(n);
				for (int i = 0; i < ipAccessScheme->GetSize() && i < pColumns->GetSize(); i++)
				{
					if (i > 0)
					{
						sFields += ",";
						sValues += ",";
					}
					CStringArray* pScheme = (CStringArray*)ipAccessScheme->GetAt(i);
					sFields += pScheme->GetAt(0);
					if (*(LPCTSTR)pScheme->GetAt(1) == 's')
						sQMark = "\'";
					else
						sQMark = "";
					sValues += sQMark + pColumns->GetAt(i) + sQMark;  
				}
				sSql = "INSERT INTO " + isTableName + "(" + sFields + ")";
				sSql += "VALUES (" + sValues + ")";				
				database.ExecuteSQL(sSql);
			}
		}      
		database.Close();
	}
	CATCH_ALL(e)
	{ 
		database.Close();
		TRACE1("Excel ODBC Access Error (%s)!",sDriver);
	}
	END_CATCH_ALL;
}
void CDBObject::DeleteXls(CString isDbXls, CString isTableName)
{
//	CString isDbXls = ".\\Data\\xlsData.xls";			// Create Excel File
	CString sDriver = "Microsoft Excel Driver (*.xls)";	// Excel Installation Driver
	CString sSql;
	CDatabase database;

	TRY
	{
		sSql.Format("DRIVER={%s};DSN=\'\';FIRSTROWHASNAMES=1;READONLY=FALSE;CREATE_DB=\"%s\";DBQ=%s",
					sDriver, isDbXls, isDbXls);
		if (database.OpenEx(sSql, CDatabase::noOdbcDialog))
		{
		//	isTableName = "[" + isTableName + "$A1:IV65536]";
			sSql = "DROP TABLE " + isTableName;
			database.ExecuteSQL(sSql);
		}      
		database.Close();
	}
	CATCH_ALL(e)
	{ TRACE1("Excel ODBC Access Error (%s)!",sDriver);}
	END_CATCH_ALL;
}

CObStrArray *CDBObject::QueryXls(CString isDbXls, CString isSql, CObArray* ipAccessScheme)
{
//	CString isDbXls = ".\\Data\\xlsData.xls";	// Excel file to be read
//	CString sSql;
    CString sDriver;
    CString sDsn;
    CDatabase database;
                                     
    // Check Excel Driver "Microsoft Excel Driver (*.xls)" 
    if ((sDriver = GetXlsDriver()) == "")
    {
        AfxMessageBox("Excel Driver is not available!");
        return NULL;
    }
    sDsn.Format("ODBC;DRIVER={%s};DSN='''';DBQ=%s", sDriver, isDbXls);

	CObStrArray *pResultSet = new CObStrArray;
	bool bGridToFile = (isSql.Find("GridToFile") > 0);
    TRY
    {
        // open Excel file
        database.Open(NULL, false, false, sDsn);
        CRecordset RecordSet(&database);
		//	Make up SQL statement.
		//	sSql = "SELECT Name, Age "       
		//		   "FROM Basic "                 
		//		   "ORDER BY Name ";
        // Exec SQL statement
        RecordSet.Open(CRecordset::forwardOnly, isSql, CRecordset::readOnly);

		//	int  rowCount = RecordSet.GetRowsetSize();
		//	int	 rowFetch = RecordSet.GetRowsFetched(); 
		//	long recCount = RecordSet.GetRecordCount();
		//	CRecordsetStatus rStatus;
		//	RecordSet.GetStatus(rStatus); 
		/*	struct CODBCFieldInfo
			{
			   CString m_strName;
			   SWORD m_nSQLType;
			   UDWORD m_nPrecision;
			   SWORD m_nScale;
			   SWORD m_nNullability;
			}; */

		// 2014-08-16, Retrieving field name from the header of excel file.
		CODBCFieldInfo fieldinfo;
		int nPoint;
		int rowCount = RecordSet.GetODBCFieldCount();
		CStringArray* pFileds = new CStringArray;
		for (int i = 0; i < rowCount; i++)
		{
			RecordSet.GetODBCFieldInfo((short)i, fieldinfo);
			CString Name = fieldinfo.m_strName;
			if (Name.Right(1) == "_")		// 2014-08-17, (bGridToFile) to trim off "_" at end of field name which was appended by SaveGridToFile()
				Name = Name.Left(Name.GetLength()-1);
			pFileds->Add(Name);
			if (i == rowCount-1)
				pResultSet->Add(pFileds);
		}
        while (!RecordSet.IsEOF())			// Extract the result
        {
			CStringArray* pResult = new CStringArray;
		//	Read Excel Internal value
		//	RecordSet.GetFieldValue("Name", sItem1);
		//	RecordSet.GetFieldValue("Age", sItem2);
		//	for (int i = 0; i < ipAccessScheme->GetSize(); i++)
			for (int i = 0; i < rowCount; i++)
			{
			//	CStringArray* pScheme = (CStringArray*)ipAccessScheme->GetAt(i);
			//	RecordSet.GetFieldValue("\""+pScheme->GetAt(0)+"\"", sValue);
				CString sValue;
				RecordSet.GetFieldValue((short)i, sValue);

				// 2014-08-18, Trim off ".0" while reading RecId with long integer type.
				if (pFileds->GetAt(i) == "RecId")
				{
					if ((nPoint = sValue.Find(".")) > 0)
						sValue = sValue.Left(nPoint);
				}
				pResult->Add(sValue);
			}
            RecordSet.MoveNext();		// Go to next
			pResultSet->Add(pResult);
        }
        database.Close();
		return pResultSet;
    }
    CATCH(CDBException, e)
    {
		if (!bGridToFile)
			AfxMessageBox("Database Exception: " + e->m_strError);
    }
    END_CATCH;
	delete pResultSet;
	return NULL;
}
#include "odbcinst.h"
CString CDBObject::GetXlsDriver()
{
    char szBuf[2048+1];
    char *pszScan = szBuf;
    WORD nSize;
    if (!SQLGetInstalledDrivers(szBuf, sizeof(szBuf)-1, &nSize))	// Get ODBC source name (ref odbcinst.h)
        return "";
	while (*pszScan != '\0')
    {
        if (strstr(pszScan, "Excel") != NULL)	// Check ODBC source of Excel...
            return pszScan;
        pszScan = strchr(pszScan, '\0') + 1;
    }
    return "";
}
// 2017-02-20, Added function to inquire simple query result set.
bool gbOdbcDiagnosisPassed = true;
CObStrArray* GetQueryResultSet(CString inDsn, CString SQLStmt)
{
	try
	{
		try
		{
			CDBObject DBObj;
			if (DBObj.Open(inDsn))
			{
				CStringList SelFieldList;
				CObStrArray* pStrArray = new CObStrArray;
				if (DBObj.ExecuteSQLStmt(SQLStmt, pStrArray, &SelFieldList))
				{
					if (pStrArray->GetSize() > 0)
					{
						gbOdbcDiagnosisPassed = true;
						return pStrArray;
					}
				}
				delete pStrArray;
			}
		} catch(...){}
	} catch(...)
	{
		int i = 0;
	}
	gbOdbcDiagnosisPassed = false;
	return (0);
}

#if 0
// 2017-11-29, Failed to execute SQLExec normally.
// 2017-02-20, Added function to inquire simple query result set.
CObStrArray* GetQueryResultSet(CDBObject* ipDBObj, CString SQLStmt)
{
	try
	{
		if (ipDBObj != (0))
		{
			CStringList SelFieldList;
			CObStrArray* pStrArray = new CObStrArray;
			if (ipDBObj->ExecuteSQLStmt(SQLStmt, pStrArray, &SelFieldList))
			{
				if (pStrArray->GetSize() > 0)
					return pStrArray;
			}
			delete pStrArray;
		}
	} catch(...){}
	return (0);
}
#endif