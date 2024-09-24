; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CTaskMonitorView
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "TaskMonitor.h"
LastPage=0

ClassCount=6
Class1=CTaskMonitorApp
Class2=CTaskMonitorDoc
Class3=CTaskMonitorView
Class4=CMainFrame

ResourceCount=7
Resource1=IDR_MAINFRAME
Resource2=IDR_CNTR_INPLACE
Resource4=IDD_ABOUTBOX
Class5=CLeftView
Class6=CAboutDlg
Resource7=IDD_TASKMONITOR_FORM

[CLS:CTaskMonitorApp]
Type=0
HeaderFile=TaskMonitor.h
ImplementationFile=TaskMonitor.cpp
Filter=N

[CLS:CTaskMonitorDoc]
Type=0
HeaderFile=TaskMonitorDoc.h
ImplementationFile=TaskMonitorDoc.cpp
Filter=N

[CLS:CTaskMonitorView]
Type=0
HeaderFile=TaskMonitorView.h
ImplementationFile=TaskMonitorView.cpp
Filter=D
BaseClass=CFormView
VirtualFilter=VWC
LastObject=CTaskMonitorView


[CLS:CMainFrame]
Type=0
HeaderFile=MainFrm.h
ImplementationFile=MainFrm.cpp
Filter=T



[CLS:CLeftView]
Type=0
HeaderFile=LeftView.h
ImplementationFile=LeftView.cpp
Filter=T

[CLS:CAboutDlg]
Type=0
HeaderFile=TaskMonitor.cpp
ImplementationFile=TaskMonitor.cpp
Filter=D

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[MNU:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_FILE_SAVE_AS
Command5=ID_FILE_PRINT
Command6=ID_FILE_PRINT_PREVIEW
Command7=ID_FILE_PRINT_SETUP
Command8=ID_FILE_SEND_MAIL
Command9=ID_FILE_MRU_FILE1
Command10=ID_APP_EXIT
Command11=ID_EDIT_UNDO
Command12=ID_EDIT_CUT
Command13=ID_EDIT_COPY
Command14=ID_EDIT_PASTE
Command15=ID_EDIT_PASTE_SPECIAL
Command16=ID_OLE_INSERT_NEW
Command17=ID_OLE_EDIT_LINKS
Command18=ID_OLE_VERB_FIRST
Command19=ID_VIEW_TOOLBAR
Command20=ID_VIEW_STATUS_BAR
Command21=ID_APP_ABOUT
CommandCount=21

[MNU:IDR_CNTR_INPLACE]
Type=1
Class=CTaskMonitorView
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_FILE_SAVE_AS
Command5=ID_FILE_PRINT
Command6=ID_FILE_PRINT_PREVIEW
Command7=ID_FILE_PRINT_SETUP
Command8=ID_FILE_SEND_MAIL
Command9=ID_FILE_MRU_FILE1
Command10=ID_APP_EXIT
CommandCount=10

[ACL:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_FILE_PRINT
Command5=ID_EDIT_UNDO
Command6=ID_EDIT_CUT
Command7=ID_EDIT_COPY
Command8=ID_EDIT_PASTE
Command9=ID_EDIT_UNDO
Command10=ID_EDIT_CUT
Command11=ID_EDIT_COPY
Command12=ID_EDIT_PASTE
Command13=ID_NEXT_PANE
Command14=ID_PREV_PANE
Command15=ID_CANCEL_EDIT_CNTR
CommandCount=15

[ACL:IDR_CNTR_INPLACE]
Type=1
Class=CTaskMonitorView
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_FILE_PRINT
Command5=ID_NEXT_PANE
Command6=ID_PREV_PANE
Command7=ID_CANCEL_EDIT_CNTR
CommandCount=7

[DLG:IDD_TASKMONITOR_FORM]
Type=1
Class=CTaskMonitorView
ControlCount=8
Control1=ICB_CHECK_TASK_ADSL,button,1342242819
Control2=IDB_BUTTON_EXIT,button,1342242816
Control3=ICB_CHECK_Restart_ON,button,1342242819
Control4=ICB_CHECK_Screen_OFF,button,1342242819
Control5=IDB_BUTTON_ScreenOff,button,1342242816
Control6=IDS_CpuUsage,static,1342373888
Control7=IDS_WebRestarts,static,1342373888
Control8=ICB_CHECK_CPU_USAGE,button,1342242819

[TB:IDR_MAINFRAME]
Type=1
Class=?
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_EDIT_CUT
Command5=ID_EDIT_COPY
Command6=ID_EDIT_PASTE
Command7=ID_FILE_PRINT
Command8=ID_APP_ABOUT
CommandCount=8

