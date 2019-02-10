
// ATAddConDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ATAddCon.h"
#include "ATAddConDlg.h"
#include "afxdialogex.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CATAddConDlg dialog

#define RECV_BUFFER_SIZE 256*1024
#define pMainWnd ((CATAddConDlg*)AfxGetMainWnd())

void CMySocket::OnReceive(int nErrorCode)
{
	static char cBuffer[RECV_BUFFER_SIZE]; // 64K buffer - static for efficiency only
	int nSizeOfPacketSize = sizeof(long); // PacketHeaderT::lPacketSize (4 Bytes)
	PacketHeaderT *pHeader = NULL;
	int nCounter = 0;
	int nBytesRead = 0;
	int nBytesReadTotal = 0;
	int nPacketSize = 0;
	int nMissing;
	int nReady;

	// The data parser 'm_EsReceive.ReceiveData()' can only handle 'complete' packets.
	// We must ensure that only single and complete packets are passed. However, the
	// network may have a 'traffic-jam' so that several packets could be read with
	// one receive cylce. On the other hand - although unlikely, only a packet fragment 
	// once could be read. The following mechanism makes sure that these things work properly.

	// Earlier versions of that sample propagated to peek the packet-header (8 bytes).
	// In the meantime, the very seldom situation that a packet boundary runs across
	// the middle of a packet header was encountered! That is, the first part (4 Bytes)
	// of the header (of the next packet) is at the tail of the current packet. Hence 
	// we failed to peek the header for 8 bytes. Only 4 bytes were returned.
	// To avoid this problem, the function was revised in terms it now only peeks for
	// the packet-size (the first 4 Bytes).

	// just peek the header (read, but not remove from queue).
	nReady = pMainWnd->m_socket.Receive(cBuffer, nSizeOfPacketSize, MSG_PEEK);

	if (nReady == SOCKET_ERROR)
	{
		// WSAEWOULDBLOCK is not a 'real' error and will happen quite 
		// frequently. just sleep a little bit and continue.

		if (pMainWnd->m_socket.GetLastError() != WSAEWOULDBLOCK)
			Beep(1000, 200); // A real application should not just beep but handle the error

		Sleep(1); // may need to be increased in certain situations?
	} // if

	if (nReady < nSizeOfPacketSize)
		return; // non-fatal since only a peek, lets try next time!

	 // mask the data with emscon packet header structure
	pHeader = (PacketHeaderT*)cBuffer;  // only lPacketSize is valid so far...
	nPacketSize = pHeader->lPacketSize; // ...do not reference pHeader->type!

	// read in a loop just in case we only get a fragment of the packet (unlikely, so
	// the lopp will usually be passed only once).
	do
	{
		nCounter++;

		if (nBytesRead > 0)
			nBytesReadTotal += nBytesRead;

		nMissing = nPacketSize - nBytesReadTotal;

		// Only read the 'nMissing' amount of bytes. Make sure to apply the correct  
		// address offset to buffer (relevant if a packet needs more than one Receive cycle)
		nBytesRead = pMainWnd->m_socket.Receive(cBuffer + nBytesReadTotal, nMissing);

		if (nBytesRead == SOCKET_ERROR)
		{
			if (pMainWnd->m_socket.GetLastError() != WSAEWOULDBLOCK)
				Beep(1000, 200); // A real application should not just beep but handle the error

			Sleep(1); // may need to be increased in certain situations?
		} // if
		else if (nBytesRead == 0)
		{
			// Important: According to Microsoft documentation, a value of 0 only  
			// applies if connection closed. This is not true! On high frequency  
			// continuous measurements, it happens quite often that nBytesRead equals
			// to zero and we have to interpret this as a 'successful' reading. That
			// is, sleep a little bit, then retry reading. This effect almost never 
			// happens while running in debugger, it seldom happens when running a
			// debug version and mostly happens for sure for release builds!

			Sleep(1);
			continue;
		} // else if

		TRACE(_T("Bytes read %d\n"), nBytesRead);

		if (nCounter > 64) // loop emergency exit - just in case..
			return;

	} while (nBytesRead < nMissing);

	// contribution from last loop
	if (nBytesRead > 0)
		nBytesReadTotal += nBytesRead;

	// plausibility check
	if (nBytesRead == nMissing && nBytesReadTotal <= RECV_BUFFER_SIZE)
	{
		if (nBytesReadTotal == nPacketSize) // exactly one packet ready - process it
		{
			if (!pMainWnd->m_EsReceive.ReceiveData(cBuffer, nBytesReadTotal)) // exit procedure causes bytesRead = -1
				Beep(3000, 50); // error (status not OK or corrupt data)
		}
	} // if
	else
	{
		Beep(3000, 50); // something went wrong
		TRACE(_T("Read failure\n"));
	} // else

	CSocket::OnReceive(nErrorCode);
} // OnReceive()


bool CMyESAPICommand::SendPacket(void* PacketStart, long PacketSize)
{
	//pMainWnd->m_staticResult.SetWindowText(_T("")); // always clear result window (cosmetics)
	if (!pMainWnd->m_socket.Send(PacketStart, PacketSize))
		return false;

	return true;
}


void CMyESAPIReceive::OnCommandAnswer(const BasicCommandRT& cmd)
{

	CString s;
	s.Format(_T("OnCommandAnswer, command %d, status %d\n"), cmd.command, cmd.status);
	TRACE(s);

	if (cmd.status != ES_RS_AllOK)
	{
		Beep(2000, 100);   // command failed
		AfxMessageBox(s);  // show the coammand number and error status in a box
	}
}

void CMyESAPIReceive::OnErrorAnswer(const ErrorResponseT& err)
{
	CString s;
	s.Format(_T("OnErrorAnswer, command %d, error %d\n"), err.command, err.status);
	Beep(2000, 100);

	TRACE(s);
	AfxMessageBox(s);
}

void CMyESAPIReceive::OnSystemStatusChange(const SystemStatusChangeT& status)
{
	CString s;
	s.Format(_T("OnSystemStatusChange, status %d\n"), status.systemStatusChange);
	TRACE(s);

	//pMainWnd->m_staticResult.SetWindowText(s);
}























//////// - for program icon . useless
CATAddConDlg::CATAddConDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_ATADDCON_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CATAddConDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IP_ADDRESS, m_ip_address);
	DDX_Control(pDX, IDC_STATUS, m_status);
	DDX_Control(pDX, IDC_connect, m_connect);
	DDX_Control(pDX, IDC_CURRENT_POSITION, m_current_position);
	DDX_Control(pDX, IDC_MEASURE_RESULT, m_measure_result);
	DDX_Control(pDX, IDC_CURRENTCOMP, m_currentcomp);
}


/////// AUTOMATIC there  are addeded buttons
BEGIN_MESSAGE_MAP(CATAddConDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_connect, &CATAddConDlg::OnBnClickedconnect)
	ON_BN_CLICKED(IDC_MEASURE, &CATAddConDlg::OnBnClickedMeasure)
	ON_BN_CLICKED(IDC_INITIALIZATION, &CATAddConDlg::OnBnClickedInitialization)
END_MESSAGE_MAP()


// CATAddConDlg message handlers
// SOCKETS HANDLER need add
BOOL CATAddConDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	AfxSocketInit();

	if (!m_socket.Create())
	{
		AfxMessageBox(_T("Failed to create socket - fatal error, exiting"));
		OnCancel();
	}
	else
		//TRACE(_T("Socket creation OK\n"));
	m_ip_address.SetWindowText(_T("192.168.241.6"));

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.


// PAITING ... i dont need that
void CATAddConDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.

//// ICON i dodnt need that
HCURSOR CATAddConDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
////////////





void CATAddConDlg::OnBnClickedconnect()
{
	CString ip_address;
	m_ip_address.GetWindowText(ip_address);

	if (m_socket.Connect(ip_address, 700) == false)
	{

		m_status.SetWindowText(_T("Connection failed"));
		AfxMessageBox(_T("Connection failed. make sure server is running and IP address is correct"));
		
	}

	else
	{
		m_bConnected = true;
		m_connect.EnableWindow(false);
		m_EsCommand.SetCoordinateSystemType(ES_CS_SCW); // sets coordinate system  to ES_CS_SCW
		m_EsCommand.GetCompensation();					// getting COMPENSATIONS
		AfxMessageBox(_T("Connection succesful"));
		m_status.SetWindowText(_T("Connection succesful"));


	}
	
}










void CATAddConDlg::OnBnClickedMeasure()
{
	if (m_bConnected == true)
	{
		m_EsCommand.GetSystemSettings(); // changing measure style
		pMainWnd->m_status.SetWindowText(_T("Measuring..."));
		return;

	}
}
void CMyESAPIReceive::OnGetSystemSettingsAnswer(const SystemSettingsDataT& systemSettings)
{
	SystemSettingsDataT setting_parameters = systemSettings; // getting current setting
	if (setting_parameters.bSendReflectorPositionData == true)
	{
		setting_parameters.bSendReflectorPositionData = false; // tracking disable
		m_EsCommand.SetSystemSettings(setting_parameters);
		m_EsCommand.StartMeasurement();// starting measure
	}
	else {
		setting_parameters.bSendReflectorPositionData = true; // tracking activated
		m_EsCommand.SetSystemSettings(setting_parameters);
	}
}
void CMyESAPIReceive::OnSingleMeasurementAnswer(const SingleMeasResultT& singleMeas)
{
	CString single_measure;
	single_measure.Format(_T("HZ=%lf, V=%lf, Distance=%lf\n"),
			singleMeas.dVal1 ,// TO DO
			singleMeas.dVal2, // system PI pls this is horrible
			singleMeas.dVal3);
	pMainWnd->m_measure_result.SetWindowText(single_measure);
	pMainWnd->m_status.SetWindowText(_T("Measurement Done!"));
}





void CATAddConDlg::OnBnClickedInitialization()
{
	if (m_bConnected == true)
		m_EsCommand.Initialize();
		pMainWnd->m_status.SetWindowText(_T("Initializating...."));
	return;
}

void CMyESAPIReceive::OnInitializeAnswer()
{
	pMainWnd->m_status.SetWindowText(_T("Initialization done"));

}






void CMyESAPIReceive::OnReflectorPosAnswer(const ReflectorPosResultT& reflPos)
{
		CString current_possition;
		current_possition.Format(_T("HZ=%lf, V=%lf, Distance=%lf\n"), 
									reflPos.dVal1,
									reflPos.dVal2,
									reflPos.dVal3);
		pMainWnd->m_current_position.SetWindowText(current_possition);
}

void CMyESAPIReceive::OnGetCompensationAnswer(const int iInternalCompensationId)
{
	int current_compensation = iInternalCompensationId;
	CString a;
	a.Format(_T("%d"), current_compensation);
	pMainWnd->m_currentcomp.SetWindowText(a);
}