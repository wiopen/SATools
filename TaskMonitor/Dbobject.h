#ifndef __CDBOBJECT__  
#define __CDBOBJECT__  

#define MAXCOLS 	   		 128
#define MAXROWS 	    	 15000
#define MAX_BUF 	     	 1000
#define MAX_NUM_PRECISION    15
#define MAX_NUM_STRING_SIZE  (MAX_NUM_PRECISION + 5)
#define NEED_DB_CONNECT      -2  
#define SQL_EXEC_ERROR       -1
#define SQL_EXEC_SUCESS      1
#define GET_DATA_ERROR       0
#define GET_DATA_SUCESS      1 
#define GET_NO_DATA_FOUND	 2
#define FETCH_BLOCK			 0
#define FETCH_ALL			 1

#define MAX_QUERY_ROWS		 15200				// 2017-03-16, Limited query records to prevent QueryView overflow event.
												// 2017-03-16, 45,241 records retrieved.

#include "afxdb.h"

#undef  AFX_EXPORTS
#undef  AFX_IMPORTS
#define AFX_IMPORTS
#define AFX_EXPORTS 

void _cdecl ReleaseObArray(CObArray*);  
int  _cdecl GetTokStr(const char*, CString&, int&, CString);
void _cdecl RemoveAndDelete(CObArray* pArray, int nIndex);

class AFX_EXPORTS FieldData : public CObList
{
public:
	CString	FieldName;
	SWORD	SQLType;
	int		Length;
	BOOL	AllowZeroLen;
};                                         
	
class CObStrArray : public CObArray
{
public:
	CObStrArray(){};
	virtual ~CObStrArray()
	{
		// Clean the CStringArray objects up
		int n = GetSize();
		while (--n >= 0)
			delete (CStringArray*)GetAt(n);
	};
};

class AFX_EXPORTS CDBObject;
CObStrArray* _cdecl GetQueryResultSet(CDBObject* ipDBObj, CString SQLStmt);
CObStrArray* _cdecl GetQueryResultSet(CString inDsn, CString SQLStmt);
/////////////////////////////////////////////////////////////////////////////
// CDBObject document
class AFX_EXPORTS CDBObject : public CObject
{
	DECLARE_DYNCREATE(CDBObject)

public:
	CDBObject();			// protected constructor used by dynamic creation

// Attributes
public:
	HSTMT			u_hStmt;
    HDBC			u_hdbc;
	CString			u_MsgStr;	// 0 : Not Show ErrorMsg
								// 1 : Show ErrorMsg
								// 2 : Show ErrorMsg and Log to DB

	static AFX_IMPORTS int		u_ShowErrorMsg;		// 1: Show detail msg
										            // 0: Show Chinese simple msg
	static AFX_IMPORTS int     u_TraceSQL;
	static AFX_IMPORTS int     u_Translate;			// 0: Don't translate SQLServer syntax
													// 1: Translate to ORACLE syntax
	//int* AFX_IMPORTS  u_pAppTraceMode; 
	
protected:
    SWORD			u_ColType[MAXCOLS];
    SWORD			u_ColTypeR[MAXCOLS];
    UDWORD			u_ColLen[MAXCOLS]; 
    SDWORD			u_OutLen[MAXCOLS];    
    UDWORD			u_ColLenR[MAXCOLS];    //Update07181:
    UCHAR*			u_Data[MAXCOLS];
    UCHAR*			u_pData[MAXCOLS];    //Update07181:
    SDWORD*			u_pOutLen[MAXCOLS];  //Update07181:    
    SWORD			u_nResultCols;
    CStringList*	u_pFieldNameList;                        
    CStringList*	u_pTypeList;
	int				u_nRowSize;
    SWORD			u_nResultColsR;       //Update07181:
	BOOL			u_DeadLockError;
	DWORD			u_StartTime;
	DWORD			u_EndTime;
	CString			u_DBType;
public:
    static HENV		u_henv;
	static int		u_henvCount;

private:

// Operations
public:
	virtual	~CDBObject();

	virtual BOOL	CreateTable(CString TableName, CObArray *pFieldArray);
	virtual BOOL	DropTable(CString TableName);

	virtual BOOL	Open(LPCSTR lpszDSN);

	virtual SDWORD	Insert(CString TableName, CString InsFields, CString Values); 
	virtual SDWORD	Insert(CString TableName, CString InsFields, CStringArray* Values); 
	
	virtual SDWORD	Update(CString TableName, CString UpdFields, CString Key);
	virtual SDWORD	Update(CString TableName, CString UpdFields, 
						   CStringArray* Values, CString Where);
	virtual BOOL	UpdateByPos(CString TableName, long nPos,
					   			CString UpdFields, CStringArray* pUpdValue);
	
	virtual SDWORD	Delete(CString TableName, CString Key);
	virtual BOOL	DeleteByPos(CString TableName, long StartPos, long EndPos);
	
	virtual BOOL	Query(CString TableName, CString SelFields, 
						  CString Key, CString SortCols, 
						  CStringArray* csBindKey, CObArray* pObArray);

	virtual BOOL	PreQuery(CString TableName,	CString SelFields, 
							 CString Key, CString SortCols, 
							 CStringArray* csBindKey, int nRowSize);
   	virtual	BOOL 	FetchData(CObArray *pStrArray, long irow);

	virtual BOOL	CreateIndex(CString IndexName, CString TableName, CString IndexFields);                       
	virtual BOOL	DropIndex(CString IndexName);

	virtual BOOL	AddField(CString TableName, CObArray *pFieldArray);
	virtual BOOL	DeleteField(CString TableName, CObArray *pFieldArray);
	
	virtual BOOL	IsNetDBObject() { return 0; }
	virtual long 	GetTotalSize(CString TableName, CString StrField = "*", CString Key = "", CStringArray* pBindKey = NULL);
	virtual long	GetTotalSize(SDWORD RowNumber, BOOL& bLast);
	virtual BOOL 	ExecuteSQLStmt(LPCTSTR SQLStmt, int* pCount = NULL);
	virtual int		BeginTransaction();
	virtual int		EndTransaction();
	virtual int		Commit();
	virtual int		RollBack();
	virtual BOOL	DeadLockCheck();
	virtual int		SQLExec(CString SQLString, void *Params=NULL,...);
	virtual BOOL	ExecuteSQLStmt(CString SQLStmt, CObArray* pArray, 
								 CStringList* pSelectFields = NULL);
	virtual CStringList* GetColName(CString StrTableName);
	virtual CStringList* GetSelectColName() {return u_pFieldNameList;};
	virtual BOOL	PreQuery(CString SQLStmt, int nRowSize);
    //int	GetColLen(int i) {return u_ColLen[i];}; 
	virtual void    GetResultSetInfo(int* pnCol = NULL, 
		                     CStringArray* pFieldNameArray = NULL,
					         CDWordArray* pColLen = NULL);
	virtual BOOL	GetColumnAttrib(CString StrTableName, 
							CString StrColumn, CObArray& StrArray);
	//virtual BOOL	GetColumnAttribX(CString StrTableName, 
	//							CString StrColumn, CObArray& StrArray);
	virtual void	EnumTable(CStringArray& TableArray);
	
	CString GetDBName()	{return u_DBType;};
	void CDBObject::GetQuoteArray(CWordArray& QuoteArray, 
									 CString& StrField,
									 CObArray* pObArray,
									 CDBObject* pDBObj);

// Implementation
    SWORD	GetSQLType(CString StrField);
    void	GetColumnsName(CString);
	BOOL	TableExists(CString TableName);

	int**	GetColumnAttrib(CObArray  *ipObStrArray, CStringList& isColumns);
	int**	GetColumnAttrib(CString isTableName, CStringList& isColumns);
	CObStrArray* QueryXls(CString isDbXls, CString isSql, CObArray* ipAccessScheme=NULL);
	void	InsertXls(CString isDbXls, CString isTableName, CObArray* ipAccessScheme, CObArray* ipResultSet);
	void	DeleteXls(CString isDbXls, CString isTableName);
	CString	GetXlsDriver();

	////
	//SWORD	GetSQLType(CString FieldName, CStringArray* pFieldNameList);
	//int		GetSQLColDef(CString FieldName, CStringArray* pFieldList, CString *FieldDef=NULL);
	//BOOL	PreQuery(CString TableName, 
	//				 	CString SelFields, CString Key, CString SortCols, 
    //   		   		 	CStringArray* csBindKey, int nRowSize, CStringArray* pSelectFields);
	//CString	u_SQLErrorMsg;

protected:
	SWORD	MapSQLType(CString StrType);
	void	GetErrorStatus(RETCODE RetCode, CString ErrAPI, HSTMT hStmt);
	int 	InternalQuery(CString TableName, CString SelFields,
       		     		  CString Key, CString SortCols, 
       			     	  CStringArray* csBindKey, int nRowSize, HSTMT* phStmt);
	virtual	int	FetchRecords(CObArray *pStrArray, long irow);  	

	int		QueryBindCol(HSTMT hStmt, CStringList* pSelFields = NULL);
   	int 	QueryBindCol(HSTMT hStmt, int nRowSize);

	int 	FetchAllRecords(CObArray *pStrArray, HSTMT* phStmt);  			
	CString	CreateFieldString(FieldData *pFieldData);
    void 	ReleaseData();
	int 	ExecuteSQL(CString SQLStmt, HSTMT* p = NULL);
	BOOL 	GetFieldArray(CString KeyStr, CStringArray* psBindFields, int nCount);

	BOOL	GetFromCString(CString& Data, char* szBuffer, int nMax, int* NextRead);
	BOOL	SetCursorOption(int nRowSize, HSTMT* phStmt);
	void	CheckSQL(CString& StrSQL);
	void	EnumIndex(CString StrTable,
					  CStringArray& ColumnArray,
					  CStringArray& IndexArray);
	//BOOL    GetTokStr(const char* pSource, CString& StrDest, int& Start, CString Delimeter);
    //void    ReleaseObArray(CObArray* pReleasedArray);
    //void    RemoveAndDelete(CObArray* pArray, int nIndex);

	UDWORD  Display_Size(SWORD ColType, UDWORD ColLen, SWORD ColNameLen);
	SWORD   SQLType_TO_CType(SWORD ColType);    
	void	Substitute(CString& SrcStr, CString OldStr, CString NewStr);
	CString GetDBMSName();
};                     

#undef AFX_EXPORTS
#undef AFX_IMPORTS

#endif // __CDBOBJECT__

