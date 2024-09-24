// TaskMonitorView.cpp : implementation of the CTaskMonitorView class
//

#include "stdafx.h"
#include "TaskMonitor.h"
#include "DbObject.h"

#include "TaskMonitorDoc.h"
#include "CntrItem.h"
#include "TaskMonitorView.h"
#include "io.h"				// close()

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define	CCxTaskADSL			((CButton*)GetDlgItem(ICB_CHECK_TASK_ADSL))
#define	CCxTaskRestart		((CButton*)GetDlgItem(ICB_CHECK_Restart_ON))
#define	CCxCpuUsage			((CButton*)GetDlgItem(ICB_CHECK_CPU_USAGE))
#define	CSxCpuUsage			((CWnd*)GetDlgItem(IDS_CpuUsage))
#define	CSxWebRestart		((CWnd*)GetDlgItem(IDS_WebRestarts))

#define TIMER_CheckTask		1001
#define TIMER_RebootTask	1002
#define TIMER_OdbcDiagnosis	1003
#define TIMER_OdbcResponse	1004

#define TIMER_CPULoad		1005			// 2024-09-18, CPU Load Checking task
#define TIMER_CPULoadTime	10000			// 2024-09-18, CPU Load Checking time

#define TIMER_CheckTime		(120*1000)		// per minute
#define TIMER_RebootTime	(30)			// 2017-11-05, Rebooting system timeout per 30 minutes
#define TIMER_OdbcDiagTime	(600*1000)		// 2021-06-24, Validating ODBC access online diagnosis timeout.
#define TIMER_OdbcRespTime	(300*1000)		// 2021-06-24, Validating ODBC access online response timeout.
/////////////////////////////////////////////////////////////////////////////
// CTaskMonitorView

IMPLEMENT_DYNCREATE(CTaskMonitorView, CFormView)

BEGIN_MESSAGE_MAP(CTaskMonitorView, CFormView)
	//{{AFX_MSG_MAP(CTaskMonitorView)
	ON_WM_DESTROY()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_COMMAND(ID_OLE_INSERT_NEW, OnInsertObject)
	ON_COMMAND(ID_CANCEL_EDIT_CNTR, OnCancelEditCntr)
	ON_BN_CLICKED(IDB_BUTTON_EXIT, OnButtonExit)
	ON_BN_CLICKED(ICB_CHECK_TASK_ADSL, OnCheckTaskAdsl)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDB_BUTTON_ScreenOff, OnBUTTONScreenOff)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CFormView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CFormView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CFormView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTaskMonitorView construction/destruction

CTaskMonitorView::CTaskMonitorView()
	: CFormView(CTaskMonitorView::IDD)
{
	//{{AFX_DATA_INIT(CTaskMonitorView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pSelection = NULL;
	// TODO: add construction code here

}

CTaskMonitorView::~CTaskMonitorView()
{
}

void CTaskMonitorView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTaskMonitorView)
	DDX_Control(pDX, ICB_CHECK_Restart_ON, mCheckRestartUp);
	DDX_Control(pDX, ICB_CHECK_Screen_OFF, mCheckScreenOff);
	//}}AFX_DATA_MAP
}

BOOL CTaskMonitorView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CFormView::PreCreateWindow(cs);
}

void CTaskMonitorView::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();
	GetParentFrame()->RecalcLayout();
	ResizeParentToFit();


	// TODO: remove this code when final selection model code is written
	m_pSelection = NULL;    // initialize selection


	CCxTaskADSL->SetCheck(true);
	CCxCpuUsage->SetCheck(true);

	// 2021-11-07, Extending Web Connection diagnosis until Web Server 8080 initialization completed.
	// SetTimer(TIMER_CheckTask, TIMER_CheckTime, NULL);
	// OnTimer(TIMER_CheckTask);
	SetTimer(TIMER_CheckTask, TIMER_CheckTime*10, NULL);

	SetTimer(TIMER_CPULoad, TIMER_CPULoadTime, NULL);

//	SetTimer(TIMER_RebootTask, (TIMER_RebootTime * 60 - ((CTime::GetCurrentTime().GetMinute() % TIMER_RebootTime) * 60 + CTime::GetCurrentTime().GetSecond())) * 1000, (0));

	SetTimer(TIMER_OdbcDiagnosis, TIMER_OdbcDiagTime, NULL);

	mCheckRestartUp.SetCheck(true);
	mCheckScreenOff.SetCheck(true);
	
//	::PostMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 2);	// turn screen off
//	::PostMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER,-1);	// turn screen on
}

/////////////////////////////////////////////////////////////////////////////
// CTaskMonitorView printing

BOOL CTaskMonitorView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CTaskMonitorView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CTaskMonitorView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

void CTaskMonitorView::OnPrint(CDC* pDC, CPrintInfo* /*pInfo*/)
{
	// TODO: add customized printing code here
}

void CTaskMonitorView::OnDestroy()
{
	// Deactivate the item on destruction; this is important
	// when a splitter view is being used.
   CFormView::OnDestroy();
   COleClientItem* pActiveItem = GetDocument()->GetInPlaceActiveItem(this);
   if (pActiveItem != NULL && pActiveItem->GetActiveView() == this)
   {
      pActiveItem->Deactivate();
      ASSERT(GetDocument()->GetInPlaceActiveItem(this) == NULL);
   }
}


/////////////////////////////////////////////////////////////////////////////
// OLE Client support and commands

BOOL CTaskMonitorView::IsSelected(const CObject* pDocItem) const
{
	// The implementation below is adequate if your selection consists of
	//  only CTaskMonitorCntrItem objects.  To handle different selection
	//  mechanisms, the implementation here should be replaced.

	// TODO: implement this function that tests for a selected OLE client item

	return pDocItem == m_pSelection;
}

void CTaskMonitorView::OnInsertObject()
{
	// Invoke the standard Insert Object dialog box to obtain information
	//  for new CTaskMonitorCntrItem object.
	COleInsertDialog dlg;
	if (dlg.DoModal() != IDOK)
		return;

	BeginWaitCursor();

	CTaskMonitorCntrItem* pItem = NULL;
	TRY
	{
		// Create new item connected to this document.
		CTaskMonitorDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		pItem = new CTaskMonitorCntrItem(pDoc);
		ASSERT_VALID(pItem);

		// Initialize the item from the dialog data.
		if (!dlg.CreateItem(pItem))
			AfxThrowMemoryException();  // any exception will do
		ASSERT_VALID(pItem);
		
        if (dlg.GetSelectionType() == COleInsertDialog::createNewItem)
			pItem->DoVerb(OLEIVERB_SHOW, this);

		ASSERT_VALID(pItem);

		// As an arbitrary user interface design, this sets the selection
		//  to the last item inserted.

		// TODO: reimplement selection as appropriate for your application

		m_pSelection = pItem;   // set selection to last inserted item
		pDoc->UpdateAllViews(NULL);
	}
	CATCH(CException, e)
	{
		if (pItem != NULL)
		{
			ASSERT_VALID(pItem);
			pItem->Delete();
		}
		AfxMessageBox(IDP_FAILED_TO_CREATE);
	}
	END_CATCH

	EndWaitCursor();
}

// The following command handler provides the standard keyboard
//  user interface to cancel an in-place editing session.  Here,
//  the container (not the server) causes the deactivation.
void CTaskMonitorView::OnCancelEditCntr()
{
	// Close any in-place active item on this view.
	COleClientItem* pActiveItem = GetDocument()->GetInPlaceActiveItem(this);
	if (pActiveItem != NULL)
	{
		pActiveItem->Close();
	}
	ASSERT(GetDocument()->GetInPlaceActiveItem(this) == NULL);
}

// Special handling of OnSetFocus and OnSize are required for a container
//  when an object is being edited in-place.
void CTaskMonitorView::OnSetFocus(CWnd* pOldWnd)
{
	COleClientItem* pActiveItem = GetDocument()->GetInPlaceActiveItem(this);
	if (pActiveItem != NULL &&
		pActiveItem->GetItemState() == COleClientItem::activeUIState)
	{
		// need to set focus to this item if it is in the same view
		CWnd* pWnd = pActiveItem->GetInPlaceWindow();
		if (pWnd != NULL)
		{
			pWnd->SetFocus();   // don't call the base class
			return;
		}
	}

	CFormView::OnSetFocus(pOldWnd);
}

void CTaskMonitorView::OnSize(UINT nType, int cx, int cy)
{
	CFormView::OnSize(nType, cx, cy);
	COleClientItem* pActiveItem = GetDocument()->GetInPlaceActiveItem(this);
	if (pActiveItem != NULL)
		pActiveItem->SetItemRects();
}

/////////////////////////////////////////////////////////////////////////////
// CTaskMonitorView diagnostics

#ifdef _DEBUG
void CTaskMonitorView::AssertValid() const
{
	CFormView::AssertValid();
}

void CTaskMonitorView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CTaskMonitorDoc* CTaskMonitorView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CTaskMonitorDoc)));
	return (CTaskMonitorDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTaskMonitorView message handlers

void CTaskMonitorView::OnButtonExit() 
{
	// TODO: Add your control notification handler code here
	gMainFrm->PostMessage(WM_CLOSE);
}
static snWebConnectionErrors = 0;
void CTaskMonitorView::OnCheckTaskAdsl() 
{
	// TODO: Add your control notification handler code here
	if (CCxTaskADSL->GetCheck())
	{
	//	int nSocket = createSocket("114.32.104.178:50080");
	// 2022-06-22, HiNET tampered fixed ip address to 1.34.154.25/32 and lost lots of effort in setting DNS registrations and host applications.
	//	int nSocket = createSocket("114.32.104.178:8080");
	//	int nSocket = createSocket("1.34.154.25:8080");
	// 2024-03-14, 2024-03-14, Swapped port 8080/8443 to 80/443 as default URI.
	//	int nSocket = createSocket("www.johnfon.com:8080");
	// 2024-04-27, Checking ppp connection must be with real ip since www.johnfon.com obtained 127.0.0.1 via gethostbyname()
	//	int nSocket = createSocket("www.johnfon.com:80");
		int nSocket = createSocket("125.229.210.228:80");
		//  close(nSocket);
		if (nSocket < 0)
		{
		//	::WinExec("D:\\OpenWeb\\ip.Hinet.ADSL", SW_SHOW);
		// 2022-06-27, ADSL-HiNet settings [Security] [Auto login password]
		//			   must be selected, otherwise ADSL-AutoDialup failed to activate Web server 1.34.154.25.
			::WinExec("D:\\OpenWeb\\ADSL-AutoDialup.exe", SW_HIDE);
			::WinExec("D:\\OpenWeb\\Apache2.2\\Turnoff-process-4.bat", SW_HIDE);

		// 2021-11-04, Rebooting system once Web connection failed.
			if (mCheckRestartUp.GetCheck())
			if (++snWebConnectionErrors >= 5)
				::WinExec("cmd /c shutdown /r", SW_HIDE);

#if 0
			ShellExecute(NULL, 
				"runas",				// "shell",  
				"D:\\OpenWeb\\ADSL-AutoDialup.exe",  
				NULL,					// program arguments
				NULL,					// default directory
				SW_SHOWNORMAL  
			); 

			ShellExecute(NULL, 
				"runas",				// "shell",  
				"D:\\OpenWeb\\Apache2.2\\Turnoff-process-4.bat",  
				NULL,					// program arguments
				NULL,					// default directory
				SW_SHOWNORMAL  
			); 
#endif
			return;
		}

	// 2024-06-18, Trying to post visit command to server via socket, but no avail.
	//	char*post = "POST /visit&username=UUU&appname=ie_iWiringGx HTTP/1.1\r\n\r\n";
	//	int bytes = write(nSocket, post, strlen(post));

		snWebConnectionErrors = 0;
		close(nSocket);
	}
	else
	{
		::WinExec("cmd /c taskkill /F /IM ...",SW_HIDE);
	}
}

void CTaskMonitorView::RebootSystem()
{
	if (mCheckRestartUp.GetCheck())
	{
		int currentDay    = CTime::GetCurrentTime().GetDay();
		int currentHour   = CTime::GetCurrentTime().GetHour();
		int currentMinute = CTime::GetCurrentTime().GetMinute();
	//	if (currentHour & 0x01)
		if (currentDay == 3 ||  currentDay == 18)
		if (currentHour == 3 && currentMinute > 10 && currentMinute < 50 )
		{
			::WinExec("cmd /c taskkill /F /IM \"FileZilla server Interface.exe\"", SW_HIDE);
			::WinExec("cmd /c taskkill /F /IM \"FileZilla server.exe\"", SW_HIDE);
		//	Sleep(1000);
		// 2018-08-01, Using windows 10 command line example.
		//	system("shutdown -r -t 10");
		//	system("shutdown /r");
			::WinExec("cmd /c shutdown /r", SW_HIDE);

		}
	}
}

extern bool gbOdbcDiagnosisPassed;
void CTaskMonitorView::OdbcDiagnosis()
{
//	extern void doPost();
//	doPost();

	SetTimer(TIMER_OdbcResponse, TIMER_OdbcRespTime, NULL);
	CDBObject DBObj;
	CString StrOpen = "DSN=" + CString("iSupers");
	if (DBObj.Open(StrOpen))
	{
		CString SQLStmt = "select distinct UID from dAmBasic order by UID";
		CObStrArray* pObStrArray = GetQueryResultSet(StrOpen, SQLStmt);
		if (pObStrArray)
			gbOdbcDiagnosisPassed = true;
		delete pObStrArray;
	}
	if (gbOdbcDiagnosisPassed)
		KillTimer(TIMER_OdbcResponse);
}

// 2024-09-18, Created CPU Load Checking task to resstart Web server while Java(TM) Platform SE binary task load increased up to 75%.
#define meanListSZ  (sizeof(meanDiffList)/sizeof(float))
#define MAX_LOOPS			3		// 10: diffs(3500), 5: diffs(1000), 3: diffs(0.000ms)
#define MAX_SLEEP			150
#define MAX_MEAN_DIFF		666.666
#define MIN_MEAN_DIFF		 22.333
#define AGG_MEAN_DIFF		105.666	// Aggregation of meanDiffList[MAX_LOOPS]
#define WEB_RESET_PERIOD	600000	// 10 minutes

static unsigned long timeLast = ::GetTickCount();
static unsigned long webResetLast = ::GetTickCount();
static unsigned long timeDiff = 0;
static float meanDiffList[MAX_LOOPS] = {0.00};
static int   meanIndex  = 0;
static int   countDiffs = 0, restartCount = 0;

void MessageLoop(int inmTime)
{
	static BOOL sbInProgress = 0;
	MSG Msg;
	if (!sbInProgress) {
		sbInProgress = 1;
		while (inmTime >= 100) {
			while (::PeekMessage(&Msg, NULL, NULL, NULL, PM_REMOVE)) {
				   ::TranslateMessage(&Msg);
				   ::DispatchMessage(&Msg);
			}
			Sleep(50);
			inmTime -= 50;
		}
		sbInProgress = 0;
	}
	while (::PeekMessage(&Msg, NULL, NULL, NULL, PM_REMOVE)) { ::TranslateMessage(&Msg); ::DispatchMessage(&Msg); }
	if (inmTime > 0) Sleep(inmTime);
	return;
}
void CTaskMonitorView::validateCPULoad()
{
	time_t rawtime;
	// time (&rawtime);
	// struct tm * ptm;
	// ptm = gmtime (&rawtime);
	// printf ("UTC time: %2d:%02d\n", ptm->tm_hour, ptm->tm_min);
	//loads the SYSTEMTIME
	FILE *pFile;
	char *buffer = "cpu load io\r\n\0";
	pFile = fopen("cpufio.txt", "r");
	if (pFile == (0)) {
		pFile = fopen("cpufio.txt", "w");
		if (pFile != (0)) {
			fwrite(buffer, sizeof(char), sizeof(buffer), pFile);
			fclose(pFile);
		}
		pFile = fopen("cpufio.txt", "r");
	}
    SYSTEMTIME sysTime;
    GetSystemTime(&sysTime);
	FILETIME idleTime={0}, kernelTime={0}, userTime={0};
	int diffs = 0;
	int loops = MAX_LOOPS;
	while (loops-- > 0) {
		timeDiff = (rawtime = ::GetTickCount()) - timeLast;
		diffs += (timeDiff - TIMER_CPULoadTime - MAX_SLEEP);
		fread(buffer, sizeof(char), sizeof(buffer)/MAX_LOOPS, pFile);
	//	float CPULoad = CalculateCPULoad(FileTimeToInt64(idleTime), FileTimeToInt64(kernelTime)+FileTimeToInt64(userTime));
	//	TRACE("%s(%d) : [%d]printCPULoad(%.3f:%d;%d;%d;)!\r\n", __FILE__, __LINE__, loops, CPULoad, idleTime.dwHighDateTime, kernelTime.dwHighDateTime, userTime.dwHighDateTime); 
	//	TRACE("%s(%d) : [%d]printSysTime(%02d:%02d:%02d.%d-%d,%d)!\r\n", __FILE__, __LINE__, loops, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMinute, rawtime, timeDiff);
		Sleep(MAX_SLEEP);
	//	MessageLoop(MAX_SLEEP);	// 2024-09-21, caused too many restartings
	}
	if (pFile != (0))
		fclose(pFile);
	float meanDiffs = diffs*1.0f/MAX_LOOPS;
	meanDiffList[meanIndex++] = meanDiffs;
	meanIndex %= meanListSZ;
	CString text;
	text.Format("CPU Usage: %.3f, %.3f, %.3f", meanDiffList[meanIndex],meanDiffList[(meanIndex+1)%meanListSZ],meanDiffList[(meanIndex+2)%meanListSZ]);
	CSxCpuUsage->SetWindowText(text);
	// TRACE("%s(%d) : [*]printSysTime(%02d:%02d:%02d.%d-%d,%.3f)!\r\n", __FILE__, __LINE__, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMinute, rawtime, meanDiffs);
	if (CCxCpuUsage->GetCheck()) {
		if (meanDiffs >= MIN_MEAN_DIFF)		
		{
			if (++countDiffs >= MAX_LOOPS || meanDiffs > MAX_MEAN_DIFF || (meanDiffList[0] + meanDiffList[1] + meanDiffList[2]) > AGG_MEAN_DIFF)
			{
				// 2024-09-19, Web server restarting must be suspended while V3Cycle dialy stock database updating in progress.
				CString mTxnTime = CTime::GetCurrentTime().Format("%H:%M");
				if (mTxnTime < "16:15" || mTxnTime > "16:45") {
					// 2024-09-21, skipping first unstable samples.
					if (::GetTickCount() - webResetLast > WEB_RESET_PERIOD) {
						::WinExec("D:\\OpenWeb\\V3Cycle\\Data\\restartTomcat.bat", SW_HIDE);
						text.Format("Web Reset: %d(%.3f, %.3f, %.3f)", ++restartCount, meanDiffList[meanIndex],meanDiffList[(meanIndex+1)%meanListSZ],meanDiffList[(meanIndex+2)%meanListSZ]);
						CSxWebRestart->SetWindowText(text);
						webResetLast = ::GetTickCount();
					}
				}
				countDiffs = 0;
			//	TRACE("%s(%d) : [***]printSysTime(%02d:%02d:%02d.%d-%d,%.3f)!\r\n", __FILE__, __LINE__, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMinute, rawtime, meanDiffs);
			}
		}
		else countDiffs = 0;
	}
	timeLast = ::GetTickCount();
}

#if 0
// #include <Windows.h>
static unsigned long FileTimeToInt64(const FILETIME & ft) {return (((unsigned long)(ft.dwHighDateTime))<<32)|((unsigned long)ft.dwLowDateTime);}
static float CalculateCPULoad(unsigned long idleTicks, unsigned long totalTicks)
{
   static unsigned long _previousTotalTicks = 0;
   static unsigned long _previousIdleTicks = 0;

   unsigned long totalTicksSinceLastTime = totalTicks-_previousTotalTicks;
   unsigned long idleTicksSinceLastTime  = idleTicks-_previousIdleTicks;

   float ret = 1.0f-((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime)/totalTicksSinceLastTime : 0);

   _previousTotalTicks = totalTicks;
   _previousIdleTicks  = idleTicks;
   return ret;
}

static long toInteger(LARGE_INTEGER const & integer)
{
#ifdef INT64_MAX // Does the compiler natively support 64-bit integers?
        return integer.QuadPart;
#else
        return (static_cast<long>(integer.HighPart) << 32) | integer.LowPart;
#endif
}
// #include <windows.h>
// #include <vector>
// #include <iostream>
#include <winternl.h>
#pragma comment(lib, "Ntdll.lib")

typedef struct
_SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_R {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_R;

class CPU
{
public:
    long prev_idle;
    long prev_ker;
    long prev_user;
    long cur_idle;
    long cur_ker;
    long cur_user;

	CPU()
	{
		prev_idle = 0;
		prev_ker = 0;
		prev_user = 0;
		cur_idle = 0;
		cur_ker = 0;
		cur_user = 0;
	}

    double get()
    {
        SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_R *a = new SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_R[4];
        // 4 is the total of CPU (4 cores)
        NtQuerySystemInformation(SystemProcessorPerformanceInformation, a, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_R) * 4, NULL);

        prev_idle = cur_idle;
        prev_ker = cur_ker;
        prev_user = cur_user;

        cur_idle = 0;
        cur_ker = 0;
        cur_user = 0;

        // 4 is the total of CPU (4 cores)
        // Sum up the SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_R array so I can get the utilization from all of the CPU
        for (int i = 0; i < 4; ++i)
        {
            SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_R b = a[i];
            cur_idle += toInteger(b.IdleTime);
            cur_ker += toInteger(b.KernelTime);
            cur_user += toInteger(b.UserTime);
        }

        std::cout << "Cur idle " << cur_idle << '\n';
        std::cout << "Cur ker " << cur_ker << '\n';
        std::cout << "Cur user " << cur_user << '\n';

        long delta_idle = cur_idle - prev_idle;
        long delta_kernel = cur_ker - prev_ker;
        long delta_user = cur_user - prev_user;
        
        std::cout << "Delta idle " << delta_idle << '\n';
        std::cout << "Delta ker " << delta_kernel << '\n';
        std::cout << "Delta user " << delta_user << '\n';

        long total_sys = delta_kernel + delta_user;
        long kernel_total = delta_kernel - delta_idle;

        delete[] a;
        // return (total_sys - delta_idle) * 100.0 / total_sys;
        return (kernel_total + delta_user) * 100.0 / total_sys;
    }
};
// bool GetSystemTimes(FILETIME *idleTime, FILETIME *kernelTime, FILETIME *userTime);
// Returns 1.0f for "CPU fully pinned", 0.0f for "CPU idle", or somewhere in between
// You'll need to call this at regular intervals, since it measures the load between
// the previous call and the current one.  Returns -1.0 on error.
//	float GetCPULoad()
//	{
//	   FILETIME idleTime, kernelTime, userTime;
//	   return GetSystemTimes(&idleTime, &kernelTime, &userTime) ? CalculateCPULoad(FileTimeToInt64(idleTime), FileTimeToInt64(kernelTime)+FileTimeToInt64(userTime)) : -1.0f;
//	}
#endif

void CTaskMonitorView::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	switch(nIDEvent)
	{
	case TIMER_CPULoad:
		 KillTimer(TIMER_CPULoad);
		 validateCPULoad();
		 SetTimer(TIMER_CPULoad, TIMER_CPULoadTime, NULL);
		 break;
	case TIMER_CheckTask:
		 KillTimer(TIMER_CheckTask);
		 OnCheckTaskAdsl();
		 SetTimer(TIMER_CheckTask, TIMER_CheckTime, NULL);
	case TIMER_RebootTask:
		 KillTimer(TIMER_RebootTask);
	//	 SetTimer(TIMER_RebootTask, (TIMER_RebootTime * 60 - ((CTime::GetCurrentTime().GetMinute() % TIMER_RebootTime) * 60 + CTime::GetCurrentTime().GetSecond())) * 1000, (0));
	// 2021-08-04, It is not necessory to reboot system by daily check since doing TIMER_OdbcDiagnosis is enough.
	//	 RebootSystem();
		 break;
	case TIMER_OdbcDiagnosis:
		 if (mCheckScreenOff.GetCheck())
			::PostMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 2);	// turn screen off
		 // ::PostMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER,-1);	// turn screen on
		 KillTimer(TIMER_OdbcDiagnosis);
		 OdbcDiagnosis();
		 if (gbOdbcDiagnosisPassed)
			 SetTimer(TIMER_OdbcDiagnosis, TIMER_OdbcDiagTime, NULL);
		 else
			 SetTimer(TIMER_OdbcDiagnosis, TIMER_OdbcDiagTime/10, NULL);
		 break;
	case TIMER_OdbcResponse:
		 KillTimer(TIMER_OdbcResponse);
	//	 RebootSystem();
		 if (mCheckRestartUp.GetCheck())
			::WinExec("cmd /c shutdown /r", SW_HIDE);
		 break;
	}
	CFormView::OnTimer(nIDEvent);
}
int CTaskMonitorView::createSocket(const char* pszURL)
{
//	::WritePrivateProfileString("MobileSquare", "MITE", "2010", "vcde");	// MUST be executed at the 1st line in macro WEB_VALIDATE once the system is built at the first time
	int		nSocket;
	char	szIPAddress[20]={0};		// "255.255.255.255:65535\0", please more precisely specify the value 16!
	struct	sockaddr_in IPAddress = {0};
	char* pScan = strstr(pszURL, ":");	// ':' is the right delimiter of port ID, but not '@'
	if (pScan == 0 || (pScan-pszURL) >= 16)
	{
		printf("[NetAgent.cpp] createSocket: Invalid IP Address format (%s)!\n", pszURL);
		return -1;
	}
	strncpy(szIPAddress, pszURL, (pScan-pszURL));
	if ((IPAddress.sin_port = htons(atoi(++pScan))) <= 0)
	{
		printf("[NetAgent.cpp] createSocket: Invalid Service Port format (%s)\n", pszURL);
		return -2;
	}    
	IPAddress.sin_family = PF_INET;

	// 2023-03-30, HiNET tampered fixed ip address to 125.229.210.228/32 and caused INADDR_NONE issue.
	//	if ((IPAddress.sin_addr.s_addr = inet_addr((const char*)szIPAddress)) == INADDR_NONE)
	HOSTENT* webhost = gethostbyname(szIPAddress);
	memcpy(&IPAddress.sin_addr.s_addr, webhost->h_addr, webhost->h_length);
	if (IPAddress.sin_addr.s_addr == INADDR_NONE)
	{
		printf("[NetAgent.cpp] createSocket: Invalid Socket (%s) Port NumberIP Address!!", pszURL);
		return -3;
	}
	WSADATA		wsaData;
	if (WSAStartup(0x0101, &wsaData) != 0)
	{
		printf("[px.cpp] createSocket: Socket (%s) WSAStartup Error !!", pszURL);
		return -4;
	}
	if ((nSocket = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) 
	{
		printf("[NetAgent.cpp] createSocket: Socket (%s) Create Error !!", pszURL);
		return -5;
	}
	int nLoops = 1;
	while (nLoops-- > 0)
	{
		if (connect(nSocket, (struct sockaddr*)&IPAddress, sizeof(IPAddress)) < 0)
		{
			if (nLoops > 0)
			{
				Sleep(100);
				continue;
			}
			printf("[NetAgent] createSocket: Socket (%s) Connect Error !!", pszURL);
			close(nSocket);
			return -6;
		}
		break;
	}
	unsigned long iMode = 1;	// put the socket in non-blocking mode.
	if (ioctlsocket(nSocket, FIONBIO, &iMode) == SOCKET_ERROR)
	{
		printf("[px.cpp] createSocket: ioctlsocket set nonblocking error (%d)", WSAGetLastError());
	}
	return nSocket;
}

void CTaskMonitorView::OnBUTTONScreenOff() 
{
	// TODO: Add your control notification handler code here
	::PostMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 2);	// turn screen off
 // ::PostMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER,-1);	// turn screen on
	
}
// --------------------------------------------------------------------

HBRUSH CTaskMonitorView::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CFormView::OnCtlColor(pDC, pWnd, nCtlColor);
	int	   nId = pWnd->GetDlgCtrlID();
	unsigned long lColor_green	= RGB(185,248,82);
	unsigned long lColor_blue	= RGB( 30, 30,228);
	unsigned long lColor_yellow = RGB(230,230,150);
	unsigned long lColor_red	= RGB(255,0,0);
	CRect RectCell;
    switch (nCtlColor)
    {
    case CTLCOLOR_BTN:
	//	pWnd->GetClientRect(&RectCell);
	//	pDC->FillSolidRect(&RectCell,RGB(0, 0, 255));
	//	pWnd->InvalidateRect(RectCell);
	//	pWnd->ShowWindow(TRUE);
		pDC->SetTextColor(lColor_green);
		pDC->SetBkColor(lColor_yellow);
        return hbr;
    case CTLCOLOR_STATIC:
		pDC->SetTextColor((nId == IDS_CpuUsage) ? lColor_red : lColor_blue);
        return hbr;
    default:
        return hbr;
    }
	return hbr;
}
