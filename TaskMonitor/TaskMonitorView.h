// TaskMonitorView.h : interface of the CTaskMonitorView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_TASKMONITORVIEW_H__DD3F7686_9788_493C_A724_3A24DC3E5116__INCLUDED_)
#define AFX_TASKMONITORVIEW_H__DD3F7686_9788_493C_A724_3A24DC3E5116__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CTaskMonitorCntrItem;

class CTaskMonitorView : public CFormView
{
protected: // create from serialization only
	CTaskMonitorView();
	DECLARE_DYNCREATE(CTaskMonitorView)

public:
	//{{AFX_DATA(CTaskMonitorView)
	enum { IDD = IDD_TASKMONITOR_FORM };
	CButton	mCheckRestartUp;
	CButton	mCheckScreenOff;
	//}}AFX_DATA

// Attributes
public:
	CTaskMonitorDoc* GetDocument();
	// m_pSelection holds the selection to the current CTaskMonitorCntrItem.
	// For many applications, such a member variable isn't adequate to
	//  represent a selection, such as a multiple selection or a selection
	//  of objects that are not CTaskMonitorCntrItem objects.  This selection
	//  mechanism is provided just to help you get started.

	// TODO: replace this selection mechanism with one appropriate to your app.
	CTaskMonitorCntrItem* m_pSelection;

// Operations
public:
	int  createSocket(const char* pszURL);
	void RebootSystem();

	void validateCPULoad();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTaskMonitorView)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	virtual BOOL IsSelected(const CObject* pDocItem) const;// Container support
	//}}AFX_VIRTUAL

// Implementation
public:
	void OdbcDiagnosis();
	virtual ~CTaskMonitorView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CTaskMonitorView)
	afx_msg void OnDestroy();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnInsertObject();
	afx_msg void OnCancelEditCntr();
	afx_msg void OnButtonExit();
	afx_msg void OnCheckTaskAdsl();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnBUTTONScreenOff();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in TaskMonitorView.cpp
inline CTaskMonitorDoc* CTaskMonitorView::GetDocument()
   { return (CTaskMonitorDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TASKMONITORVIEW_H__DD3F7686_9788_493C_A724_3A24DC3E5116__INCLUDED_)
