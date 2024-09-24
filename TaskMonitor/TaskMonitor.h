// TaskMonitor.h : main header file for the TASKMONITOR application
//

#if !defined(AFX_TASKMONITOR_H__7EB55ACC_5930_47FD_A2B2_0391ED8FA6A8__INCLUDED_)
#define AFX_TASKMONITOR_H__7EB55ACC_5930_47FD_A2B2_0391ED8FA6A8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "MainFrm.h"


#define gMainFrm				((CMainFrame*)(gApp->m_pMainWnd))
#define gAppView                ((CTaskMonitorApp*)AfxGetApp())->GetActiveView()
#define gApp                    ((CTaskMonitorApp*)AfxGetApp())

/////////////////////////////////////////////////////////////////////////////
// CTaskMonitorApp:
// See TaskMonitor.cpp for the implementation of this class
//

class CTaskMonitorApp : public CWinApp
{
public:
	CTaskMonitorApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTaskMonitorApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CTaskMonitorApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TASKMONITOR_H__7EB55ACC_5930_47FD_A2B2_0391ED8FA6A8__INCLUDED_)
