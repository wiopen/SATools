// TaskMonitorDoc.h : interface of the CTaskMonitorDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_TASKMONITORDOC_H__337E1121_7474_4E4C_B90B_494A7910E334__INCLUDED_)
#define AFX_TASKMONITORDOC_H__337E1121_7474_4E4C_B90B_494A7910E334__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CTaskMonitorDoc : public COleDocument
{
protected: // create from serialization only
	CTaskMonitorDoc();
	DECLARE_DYNCREATE(CTaskMonitorDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTaskMonitorDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTaskMonitorDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CTaskMonitorDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TASKMONITORDOC_H__337E1121_7474_4E4C_B90B_494A7910E334__INCLUDED_)
