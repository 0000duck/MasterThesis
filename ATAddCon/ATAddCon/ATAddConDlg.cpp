
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

}


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
	DDX_Control(pDX, IDC_MEASURE_LIST, m_measure_list);
	DDX_Control(pDX, IDC_STORE, m_store);
	DDX_Control(pDX, IDC_PROTOCOL, m_protocol);
	DDX_Control(pDX, IDC_PICTURE, m_picture);
	DDX_Control(pDX, IDC_INITIALIZATION, m_initialization);
	DDX_Control(pDX, IDC_MEASURE, m_measure);
	DDX_Control(pDX, IDC_set_reflector, m_apply);
	DDX_Control(pDX, IDC_SERIAL, m_serial);
	DDX_Control(pDX, IDC_REFLECTORSCOMBO, m_reflectorscombo);
	DDX_Control(pDX, IDC_PRODUCT, m_product);
	DDX_Control(pDX, IDC_REFLECTORID, m_reflectorid);
	DDX_Control(pDX, IDC_STABILITY, m_stability);
	DDX_Control(pDX, IDC_COMPEINPUT, m_compeinput);
	DDX_Control(pDX, IDC_TEMPERATURE, m_temperature);
	DDX_Control(pDX, IDC_PRESSURE, m_pressure);
	DDX_Control(pDX, IDC_HUMIDITY, m_humidity);
	DDX_Control(pDX, IDC_INI_OFF, m_ini_off);
	DDX_Control(pDX, IDC_INI_ON, m_ini_on);
	DDX_Control(pDX, IDC_MEASUREMODE, m_measuremode);
}

BEGIN_MESSAGE_MAP(CATAddConDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_connect, &CATAddConDlg::OnBnClickedconnect)
	ON_BN_CLICKED(IDC_MEASURE, &CATAddConDlg::OnBnClickedMeasure)
	ON_BN_CLICKED(IDC_INITIALIZATION, &CATAddConDlg::OnBnClickedInitialization)
	ON_BN_CLICKED(IDC_STORE, &CATAddConDlg::OnBnClickedStore)
	ON_BN_CLICKED(IDC_PROTOCOL, &CATAddConDlg::OnBnClickedProtocol)
	ON_BN_CLICKED(IDC_set_reflector, &CATAddConDlg::OnBnClickedsetreflector)
	ON_BN_CLICKED(IDC_ROTATE, &CATAddConDlg::OnBnClickedRotate)
	ON_BN_CLICKED(IDC_INI_ON, &CATAddConDlg::OnBnClickedIniOn)
	ON_BN_CLICKED(IDC_INI_OFF, &CATAddConDlg::OnBnClickedIniOff)
	ON_BN_CLICKED(IDC_SETMEASURE, &CATAddConDlg::OnBnClickedSetmeasure)
END_MESSAGE_MAP()


// CATAddConDlg message handlers
BOOL CATAddConDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	AfxSocketInit();

	if (!m_socket.Create())
	{
		AfxMessageBox(_T("Failed to create socket - fatal error, exiting"));
		OnCancel();
	}

	m_ip_address.SetWindowText(_T("192.168.241.6"));
	m_store.EnableWindow(false);
	m_protocol.EnableWindow(false);
	m_initialization.EnableWindow(false);
	m_measure.EnableWindow(false);
	m_apply.EnableWindow(false);
	InitMeasuremodeBox();
	HBITMAP picture = (HBITMAP)LoadImage(NULL, L"baseline.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);// set picture
	m_picture.SetBitmap(picture);// set picture

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
		m_initialization.EnableWindow(true);
		m_apply.EnableWindow(true);
		m_measure.EnableWindow(true);
		m_EsCommand.SetCoordinateSystemType(ES_CS_SCW); // sets coordinate system  to ES_CS_SCW
		m_EsCommand.GetSystemSettings();				// set settings to tracking and save
		m_EsCommand.GetReflectors();					// reflectors name
		m_EsCommand.GetTrackerInfo();					// info about tracker
		m_EsCommand.GetUnits();							// current units
		m_EsCommand.GetMeasurementMode();				//current measure mode
		m_EsCommand.GetLongSystemParameter(ES_SP_InclinationSensorState); // check if inclination sensor is on or off
		AfxMessageBox(_T("Connection succesful"));
		m_status.SetWindowText(_T("Connection succesful"));

		HBITMAP picture = (HBITMAP)LoadImage(NULL, L"first.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);// set picture
		m_picture.SetBitmap(picture);// set picture
	}
	
}


// after connect, tracking activate
void CMyESAPIReceive::OnGetSystemSettingsAnswer(const SystemSettingsDataT& systemSettings)
{

	pMainWnd->setting_parameters = systemSettings; // getting current setting
	pMainWnd->setting_parameters.bSendReflectorPositionData = true; // tracking activated
	m_EsCommand.SetSystemSettings(pMainWnd->setting_parameters);

}

void CMyESAPIReceive::OnGetUnitsAnswer(const SystemUnitsDataT& unitsSettings)
{

	SystemUnitsDataT current_units = unitsSettings;
	current_units.angUnitType = ES_AU_Gon;
	m_EsCommand.SetUnits(current_units);

}



void CMyESAPIReceive::OnGetReflectorsAnswer(const int iTotalReflectors, const int iInternalReflectorId,const ES_TargetType  targetType,const double dSurfaceOffset,const unsigned short cReflectorName[32]) 
{
	static int nIndex = -1;
	nIndex++;
	pMainWnd->m_reflIDMap[nIndex] = iInternalReflectorId;

	const int size = 32;
	char buf[size];
	for (int i = 0; i < size; i++)
		buf[i] = LOBYTE(cReflectorName[i]);

	pMainWnd->m_reflNameMap[nIndex] = buf;

	if (nIndex == (iTotalReflectors - 1))
	{
		pMainWnd->m_nReflectorCount = iTotalReflectors;
		pMainWnd->InitReflectorBox();
	}
}

void CATAddConDlg::InitReflectorBox()
{
	for (int i = 0; i < m_nReflectorCount; i++)
		m_reflectorscombo.AddString(m_reflNameMap[i]);
		m_EsCommand.GetReflector();
}

void CMyESAPIReceive::OnGetReflectorAnswer(const int iInternalReflectorId) {
	for (int i = 0; i < pMainWnd->m_nReflectorCount; i++)
	{
		if (pMainWnd->m_reflIDMap[i] == iInternalReflectorId)
		{
			pMainWnd->m_reflectorscombo.SetCurSel(i);
			break;
		}
		
	}
	pMainWnd->m_reflectorscombo.EnableWindow(false);
	pMainWnd->m_apply.SetWindowText(_T("Change reflector"));

}
void CMyESAPIReceive::OnSetReflectorAnswer() {
	pMainWnd->m_status.SetWindowText(_T("Reflector changed"));
	pMainWnd->m_reflectorscombo.EnableWindow(false);
	pMainWnd->m_apply.SetWindowText(_T("Change reflector"));

}
void CATAddConDlg::OnBnClickedsetreflector()
{
	if (m_bConnected == true)
	{
		if (m_reflectorscombo.EnableWindow(false)) {
			m_reflectorscombo.EnableWindow(true);
			m_apply.SetWindowText(_T("Apply reflector"));
		}
		if (m_reflectorscombo.EnableWindow(true)) {
			int nIndex = m_reflectorscombo.GetCurSel();
			m_EsCommand.SetReflector(m_reflIDMap[nIndex]);
		}

	}
}


void CMyESAPIReceive::OnGetTrackerInfoAnswer(const ES_LTSensorType trackerType,
											const unsigned short cTrackerName[32],
											const long lSerialNumber,
											const int iCompensationIdNumber,
											const bool bHasADM,
											const bool bHasOverviewCamera,
											const bool bHasNivel,
											const double dNivelMountOffset,
											const double dMaxDistance,
											const double dMinDistance,
											const int iMaxDataRate,
											const int iNumberOfFaces,
											const double dHzAngleRange,
											const double dVtAngleRange,
											const ES_TrkAccuracyModel accuracyModel,
											const int iMajLCPFirmwareVersion,
											const int iMinLCPFirmwareVersion)
{
	CString serial;
	const long serial_number = lSerialNumber;
	serial.Format(_T("%d"), serial_number);
	pMainWnd->m_serial.SetWindowText(serial);

	const int size = 32;
	char buf[size];
	for (int i = 0; i < size; i++){
	buf[i] = LOBYTE(cTrackerName[i]);}
	CString name;
	name.Format(_T("%S"), buf);
	pMainWnd->m_product.SetWindowText(name);
	
	
}


void CATAddConDlg::OnBnClickedMeasure()
{
	if (m_bConnected == true)
	{
		pMainWnd->m_apply.EnableWindow(false);
		pMainWnd->m_measure.EnableWindow(false);
		setting_parameters.bSendReflectorPositionData = false;
		m_EsCommand.SetSystemSettings(setting_parameters); // tracking off

		m_current_position.SetWindowText(_T("")); // clean tracking window
		m_measure_result.SetWindowText(_T("")); // clean result window

		m_EsCommand.StartMeasurement();
	
		m_status.SetWindowText(_T("Measuring..."));
		

	}
}

void CMyESAPIReceive::OnSingleMeasurementAnswer(const SingleMeasResultT& singleMeas)
{
	
	pMainWnd->final_measure = singleMeas.dVal3*sin(singleMeas.dVal2*-M_PI);
	pMainWnd->final_deviation = singleMeas.dStd3;///////////////////////////////////////////////////////////////////////////////////////////////////////////////care !!!! dodelej

	CString single_measure;
	single_measure.Format(_T("Distance=%lf +-%lf mm"),
		pMainWnd->final_measure,
		pMainWnd->final_deviation);

	pMainWnd->m_measure_result.SetWindowText(single_measure);
	pMainWnd->m_status.SetWindowText(_T("Measurement Done!"));

	pMainWnd->setting_parameters.bSendReflectorPositionData = true;
	m_EsCommand.SetSystemSettings(pMainWnd->setting_parameters);// back to tracking

	pMainWnd->m_store.EnableWindow(true);
	pMainWnd->m_measure.EnableWindow(true);

	CString temp; temp.Format(_T("%0.1f"), singleMeas.dTemperature);
	CString pres; pres.Format(_T("%0.1f"), singleMeas.dPressure);
	CString hum; hum.Format(_T("%0.1f"), singleMeas.dHumidity);
	pMainWnd->m_temperature.SetWindowText(temp);
	pMainWnd->m_humidity.SetWindowText(pres);
	pMainWnd->m_pressure.SetWindowText(hum);
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
	//m_EsCommand.

}





void CMyESAPIReceive::OnReflectorPosAnswer(const ReflectorPosResultT& reflPos)
{
		CString current_possition;
		current_possition.Format(_T("HZ=%lf, V=%lf, Distance=%lf"), 
									reflPos.dVal1,
									reflPos.dVal2,
									reflPos.dVal3);
		pMainWnd->m_current_position.SetWindowText(current_possition);
}



void CATAddConDlg::OnBnClickedStore()
{
	if (m_bConnected == true)
	{
		pMainWnd->m_store.EnableWindow(false);
		pMainWnd->m_measure.EnableWindow(true);
		lengths.push_back(final_measure);// add to vector
		deviations.push_back(final_deviation);//add to vector
		auto size = lengths.size(); // size of vector

		CString fd;
		CString fm;
		CString sizeSTR;
		fd.Format(_T("%f"), final_measure);
		fm.Format(_T("%f"), final_deviation);
		sizeSTR.Format(_T("%d"), size);
		m_measure_list.AddString(sizeSTR+") "+fd+" +- "+fm +"mm");// add to listview

		if (size == 2) {
			HBITMAP picture = (HBITMAP)LoadImage(NULL, L"second.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);// set picture
			m_picture.SetBitmap(picture);// set picture
			AfxMessageBox(_T("Change position!"));
		}
		else if (size == 4 && m_stability.GetCheck() == false){
			m_protocol.EnableWindow(true);
			pMainWnd->m_measure.EnableWindow(false);
		}
		else if (size == 4) {
			HBITMAP picture = (HBITMAP)LoadImage(NULL, L"first.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);// set picture
			m_picture.SetBitmap(picture);// set picture
			AfxMessageBox(_T("Change position!"));
		}
		else if (size == 6) {
			m_protocol.EnableWindow(true);
			pMainWnd->m_measure.EnableWindow(false);
		}
	}
}


void CATAddConDlg::OnBnClickedProtocol()
{
	if (m_bConnected == true)
	{
		CStdioFile protocol_file;
		protocol_file.Open(L"protocol_file.txt", CFile::modeCreate | CFile::modeWrite);

		CString ip_adress;
		m_ip_address.GetWindowText(ip_adress);

		CString product;
		m_product.GetWindowText(product);

		CString productid;
		m_serial.GetWindowText(productid);

		CString reflectorid;
		m_reflectorid.GetWindowText(reflectorid);

		CString reflector;
		m_reflectorscombo.GetWindowText(reflector);

		// stability control
		CString stabilitystr;
		if (lengths.size() == 6){
			double stability = deviations[0] + deviations[1] - deviations[4] - deviations[5];
			stabilitystr.Format(_T("%f"), stability);
		}
		else{
			stabilitystr.Format(_T("not included"));
		}
		// compensation deviation
		CString compensationinput;
		m_compeinput.GetWindowText(compensationinput);
		double newcomp = (lengths[0] + lengths[1] - abs(lengths[2] - lengths[3])) / 2;
		double currentcomp = _tstof(compensationinput);
		double comp_deviation = newcomp - currentcomp;
		// tolerance
		CString result;
		if (comp_deviation < 5 && comp_deviation > -5) {
			result.Format(_T("----Check is OK----"));
		}
		else {
			result.Format(_T("----Check is NOT OK,please use Tracker pilot to set new compensation----"));
		}

		// length edit
		if (lengths.size() == 4) {
			for (int i = 0; i < 2; i++) {
				lengths.push_back(0);
				deviations.push_back(0);
			}
		}



		CString protocol;
		protocol.Format(_T("----------------Protocol--------------\n\
	Time: %s\n\
	RESULT:%s \n\
	Total measurements: %d\n\
	Stability control: %s\n\
	New compensation: %f \n\
	Standart deviation: %f\n\
	Current compensation: %s\n\
	Deviation: %f\n\
	Tolarence used:%f \n\
	---------------Tracker-----------------\n\
	Product name:%s\n\
	Serial Number:%s\n\
	IP adress: %s\n\
	Target:%s   serial number: %s \n\
	-------------Meteo Data----------------\n\
	Temperature:\n\
	Humidity:\n\
	Pressure:\n\
	-----------Measured data---------------\n\
	First inside stand:\n\
	%f +- %f mm\n\
	%f +- %f mm\n\
	Outside stand:\n\
	%f +- %f mm\n\
	%f +- %f mm\n\
	Second inside stand:\n\
	%f +- %f mm\n\
	%f +- %f mm\n"), CTime::GetCurrentTime().Format("%H:%M, %d-%m-%Y"),
				result,
				lengths.size(),
				stabilitystr,
				newcomp,
				sqrt(deviations[0] * deviations[0] + deviations[1] * deviations[1] + deviations[2] * deviations[2] + deviations[3] * deviations[3]),
				compensationinput,
				comp_deviation,
				comp_deviation*20,
				product,
				productid,
				ip_adress,
				reflector,
				reflectorid,
				lengths[0], deviations[0],
				lengths[1], deviations[1],
				lengths[2], deviations[2],
				lengths[3], deviations[3],
				lengths[4], deviations[4],
				lengths[5], deviations[5]
			);
		protocol_file.WriteString(protocol);
		protocol_file.Close();
		pMainWnd->m_status.SetWindowText(_T("Protocol created"));
		ShellExecute(NULL, _T("open"), _T("protocol_file.txt"), NULL,NULL, SW_SHOW);// opening TXT
	
	}
}





void CATAddConDlg::OnBnClickedRotate()
{
	if (m_bConnected == true)
	{
		m_EsCommand.PositionRelativeHV(200, 0);
		pMainWnd->m_status.SetWindowText(_T("Rotating..."));
	}
}

void CMyESAPIReceive::OnPositionRelativeHVAnswer()
{
	ES_SystemParameter active;
	active = ES_SP_PowerLockFunctionActive;
	m_EsCommand.SetLongSystemParameter(active, 1); // 0 off 1  on
	pMainWnd->m_status.SetWindowText(_T("Rotation done"));
}




void CATAddConDlg::OnBnClickedIniOn()
{
	ES_SystemParameter status;
	status = ES_SP_InclinationSensorState;
	m_EsCommand.SetLongSystemParameter(status, 2);
}


void CATAddConDlg::OnBnClickedIniOff()
{
	ES_SystemParameter status;
	status = ES_SP_InclinationSensorState;
	m_EsCommand.SetLongSystemParameter(status, 0);
}


void CATAddConDlg::InitMeasuremodeBox()
{
	m_measuremode.AddString(_T("Standart"));
	m_measuremode.AddString(_T("Fast"));
	m_measuremode.AddString(_T("Precise"));
	m_measuremode.AddString(_T("Outdoor"));
}

void CATAddConDlg::OnBnClickedSetmeasure()
{
	if (m_bConnected == true)
	{
		CString current_mode;
		m_measuremode.GetWindowText(current_mode);
		ES_MeasMode mode;

		if (current_mode == "Standart") {
			mode = ES_MM_Standard;
			m_EsCommand.SetMeasurementMode(mode);
		}
		else if (current_mode == 'Fast') {
			mode = ES_MM_Fast;
			m_EsCommand.SetMeasurementMode(mode);
		}
		else if (current_mode == "Precise") {
			mode = ES_MM_Precise;
			m_EsCommand.SetMeasurementMode(mode);
		}
		else if (current_mode == "Outdoor") {
			mode = ES_MM_Outdoor;
			m_EsCommand.SetMeasurementMode(mode);
		}
	}
}

void CMyESAPIReceive::OnGetMeasurementModeAnswer(const ES_MeasMode measMode)
{
	ES_MeasMode current_mode = measMode;
	if (current_mode == 20) {
		pMainWnd->m_measuremode.SetCurSel(1);
	}
	else if (current_mode == 21)
	{
		pMainWnd->m_measuremode.SetCurSel(0);
	}
	else if (current_mode == 22)
	{
		pMainWnd->m_measuremode.SetCurSel(2);
	}
	else if (current_mode == 23)
	{
		pMainWnd->m_measuremode.SetCurSel(3);
	}
}

void CMyESAPIReceive::OnSetMeasurementModeAnswer()
{
	pMainWnd->m_status.SetWindowText(_T("Measurement mode changed"));
}



void CMyESAPIReceive::OnGetLongSystemParamAnswer(const long lParameter)
{
	const long status = lParameter;
	if (status == 2) {
		pMainWnd->m_ini_on.SetCheck(true);
	}
	else if (status == 0) {
		pMainWnd->m_ini_off.SetCheck(true);
	}
}