
// ATAddConDlg.h : header file
//

#pragma once
#include <Afxsock.h> // used for CSocket


#define ES_USE_EMSCON_NAMESPACE
#include "ES_CPP_API_Def.h"





class CMySocket : public CSocket
{
protected:
	/*virtual*/ void OnReceive(int nErrorCode); // will be invoked if something to read from socket
};


class CMyESAPICommand : public EmScon::CESAPICommand // prefix 'EmScon::' due namespace
{
protected:
	/*virtual*/ bool SendPacket(void* PacketStart, long PacketSize);
};





// TO DO, RENAME THIS STUPID NAME OF CLASS
class CMyESAPIReceive : public EmScon::CESAPIReceive // inheritance from CESAPIReceive in ES_CPP_APi
{
protected:
	///////////// Answers from lazertracket 
	void OnCommandAnswer(const BasicCommandRT&); // called for every command
	void OnErrorAnswer(const ErrorResponseT&); // called on unsolicited error, e.g. beam break
	void OnSystemStatusChange(const SystemStatusChangeT&);
	void OnInitializeAnswer();// initialization done? yes? 
	void OnReflectorPosAnswer(const ReflectorPosResultT& reflPos); // get position(tracking) 
	void OnSingleMeasurementAnswer(const SingleMeasResultT& singleMeas);// get single measurement answer (standard)
	void OnGetSystemSettingsAnswer(const SystemSettingsDataT& systemSettings); // get system settings
	void OnGetCompensationAnswer(const int iInternalCompensationId);
						
	


	CMyESAPICommand m_EsCommand; // Call next command after other is finnished
};





// CATAddConDlg dialog
class CATAddConDlg : public CDialogEx
{
// Construction
public:
	CATAddConDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ATADDCON_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	bool m_bConnected;
	CMyESAPICommand m_EsCommand;
public:
	CMySocket m_socket;
	CMyESAPIReceive m_EsReceive;
	CEdit m_ip_address;
	afx_msg void OnBnClickedconnect();
	CListBox m_status;
	CButton m_connect;
	afx_msg void OnBnClickedMeasure();
	afx_msg void OnBnClickedInitialization();
	CStatic m_current_position;
	CStatic m_measure_result;
	CStatic m_currentcomp;
};
