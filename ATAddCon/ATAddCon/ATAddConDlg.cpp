
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
	DDX_Control(pDX, IDC_ROTATE, m_rotate);
	DDX_Control(pDX, IDC_SETMEASURE, m_setmeasure);
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
	m_rotate.EnableWindow(false);
	m_setmeasure.EnableWindow(false);
	InitMeasuremodeBox();
	HBITMAP picture = (HBITMAP)LoadImage(NULL, L"baseline.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);// set picture
	m_picture.SetBitmap(picture);// set picture

	HICON hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON1)); // set icon
	SetIcon(hIcon, FALSE);
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	//SetIcon(m_hIcon, TRUE);			// Set big icon
	//SetIcon(m_hIcon, FALSE);		// Set small icon

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

HCURSOR CATAddConDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}





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
		m_setmeasure.EnableWindow(true);
		m_rotate.EnableWindow(true);
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
	
	//pMainWnd->final_measure = singleMeas.dVal3*sin(singleMeas.dVal2*M_PI/200);
	//pMainWnd->final_deviation = sqrt(pow(cos(singleMeas.dVal2*M_PI/200), 2)*pow(singleMeas.dVal3, 2)*pow(singleMeas.dStd2*M_PI/200, 2) + pow(singleMeas.dStd3, 2)* pow(sin(singleMeas.dVal2*M_PI / 200), 2));
	pMainWnd->dis = singleMeas.dVal3;
	pMainWnd->dis_dv = singleMeas.dStd3;
	pMainWnd->Hz = singleMeas.dVal1;
	pMainWnd->Hz_dv = singleMeas.dStd1;
	pMainWnd->Z= singleMeas.dVal2;
	pMainWnd->Z_dv = singleMeas.dStd2;

	CString single_measure;
	single_measure.Format(_T("Hz=%0.4lf +-%0.4lfg\nZ=%0.4lf +-%0.4lfg\nDistance=%0.4lf +-%0.4lfmm"), pMainWnd->Hz,pMainWnd->Hz_dv, pMainWnd->Z, pMainWnd->Z_dv, pMainWnd->dis, pMainWnd->dis_dv);

	pMainWnd->m_measure_result.SetWindowText(single_measure);
	pMainWnd->m_status.SetWindowText(_T("Measurement Done!"));

	pMainWnd->setting_parameters.bSendReflectorPositionData = true;
	m_EsCommand.SetSystemSettings(pMainWnd->setting_parameters);// back to tracking

	pMainWnd->m_store.EnableWindow(true);
	pMainWnd->m_measure.EnableWindow(true);

	CString temp; temp.Format(_T("%0.1f°C"), singleMeas.dTemperature);
	CString pres; pres.Format(_T("%0.1fmBar"), singleMeas.dPressure);
	CString hum; hum.Format(_T("%0.1f%%"), singleMeas.dHumidity);
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
	pMainWnd->m_measure.EnableWindow(true);// if bug during measurement
}





void CMyESAPIReceive::OnReflectorPosAnswer(const ReflectorPosResultT& reflPos)
{
		CString current_possition;
		current_possition.Format(_T("HZ=%0.4lfg, V=%0.4lfg, Distance=%0.4lfmm"), 
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
		distances.push_back(dis);// add to vector
		distances_dv.push_back(dis_dv);// add to vector
		horizontals.push_back(Hz);// add to vector
		horizontals_dv.push_back(Hz_dv);// add to vector
		zenits.push_back(Z);// add to vector
		zenits_dv.push_back(Z_dv);// add to vector


		auto size = distances.size(); // size of vector

		CString f_d;
		CString f_d_dv;
		CString f_Hz;
		CString f_Hz_dv;
		CString f_Z;
		CString f_Z_dv;
		CString sizeSTR;
		f_d.Format(_T("%0.4f"), dis);
		f_d_dv.Format(_T("%0.4f"), dis_dv);
		f_Hz.Format(_T("%0.4f"), Hz);
		f_Hz_dv.Format(_T("%0.4f"), Hz_dv);
		f_Z.Format(_T("%0.4f"), Z);
		f_Z_dv.Format(_T("%0.4f"), Z_dv);
		sizeSTR.Format(_T("%d"), size);

		m_measure_list.AddString(sizeSTR+")  Hz:"+f_Hz+" +- "+f_Hz_dv +"g  Z:"+ f_Z + " +- " + f_Z_dv + "g  Dis:"+ f_d + " +- " + f_d_dv + "mm ");// add to listview

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
		pMainWnd->m_status.SetWindowText(_T("Measurement stored"));
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

		CString temperature;
		m_temperature.GetWindowText(temperature);
		CString humidity;
		m_humidity.GetWindowText(humidity);
		CString pressure;
		m_pressure.GetWindowText(pressure);

		// stability control
		CString stabilitystr;
		if (distances.size() == 6){
			double stability = distances[0] + distances[1] - distances[4] - distances[5];
			stabilitystr.Format(_T("%0.4fmm"), stability);
		}
		else{
			stabilitystr.Format(_T("not included"));
		}
		// compensation ---- calculation
		CString compensationinput;
		m_compeinput.GetWindowText(compensationinput); // loading current compensation
		double currentcomp = _tstof(compensationinput);// from string to double

		std::vector <double>hor_dis;
		std::vector <double>hor_dis_dv;
		for (int i = 0; i < 4; i++) {
			hor_dis.push_back(sin(zenits[i] * M_PI / 200) * distances[i]);
			hor_dis_dv.push_back(sqrt(pow(distances_dv[i], 2) * pow(sin(zenits[i] * M_PI / 200), 2) + pow(cos(zenits[i] * M_PI / 200), 2)*pow(zenits_dv[i] * M_PI / 200, 2)));
		}
		double Hz1 = horizontals[0] - horizontals[1]; // angles
		double Hz2 = horizontals[2] - horizontals[3];

		double Hz1_dv = sqrt(pow(horizontals_dv[0], 2) + pow(horizontals_dv[1], 2)); // angles deviations
		double Hz2_dv = sqrt(pow(horizontals_dv[2], 2) + pow(horizontals_dv[3], 2));

		// cosine lenghts
		double L1 = sqrt(pow(hor_dis[0], 2) + pow(hor_dis[1], 2) - 2 * hor_dis[0] * hor_dis[1] * cos(Hz1 * M_PI / 200));
		double L2 = sqrt(pow(hor_dis[2], 2) + pow(hor_dis[3], 2) - 2 * hor_dis[2] * hor_dis[3] * cos(Hz2 * M_PI / 200));


		double L1_dv = sqrt(pow((hor_dis[0] - hor_dis[1] * cos(Hz1*M_PI / 200)) / L1, 2)*pow(hor_dis_dv[0], 2) + pow((hor_dis[1] - hor_dis[0] * cos(Hz1*M_PI / 200)) / L1, 2)*pow(hor_dis_dv[1], 2) + pow((hor_dis[0] * hor_dis[1] * sin(Hz1*M_PI / 200)) / L1, 2)*pow(Hz1_dv*M_PI / 200, 2));
		double L2_dv = sqrt(pow((hor_dis[2] - hor_dis[3] * cos(Hz2*M_PI / 200)) / L2, 2)*pow(hor_dis_dv[2], 2) + pow((hor_dis[3] - hor_dis[2] * cos(Hz2*M_PI / 200)) / L2, 2)*pow(hor_dis_dv[3], 2) + pow((hor_dis[2] * hor_dis[3] * sin(Hz2*M_PI / 200)) / L2, 2)*pow(Hz2_dv*M_PI / 200, 2));

		double comp_diff = (L2 - L1) / 2; // this is difference between new and old comp, because we are measuring with active additional constant
		double newcomp_dv= sqrt((pow(L1_dv, 2) + pow(L2_dv, 2)) / 4);
		double newcomp = currentcomp + comp_diff;


		// tolerance
		CString result;
		if (comp_diff < 0.005 && comp_diff > -0.005) {
			result.Format(_T("----Check is OK----"));
		}
		else {
			result.Format(_T("----Check is NOT OK,please use Tracker pilot to set new compensation----"));
		}
		int measurement_size = distances.size();
		// length edit (pushing zeros if size is 4)
		if (distances.size() == 4) {
			for (int i = 0; i < 2; i++) {
				distances.push_back(0);
				distances_dv.push_back(0);
				horizontals.push_back(0);
				horizontals_dv.push_back(0);
				zenits.push_back(0);
				zenits_dv.push_back(0);
			}
		}



		CString protocol;
		protocol.Format(_T(
"----------------Protocol--------------\n\
Time: %s\n\
RESULT:%s \n\
Total measurements: %d\n\
Stability control: %s\n\
New compensation: %0.4fmm \n\
Standart deviation: %0.4fmm\n\
Current compensation: %smm\n\
Deviation: %0.4fmm\n\
Tolerance used:%0.0f%% \n\
---------------Tracker-----------------\n\
Product name:%s\n\
Serial Number:%s\n\
IP adress: %s\n\
Target:%s   serial number: %s \n\
-------------Meteo Data----------------\n\
Temperature:%s\n\
Humidity:%s\n\
Pressure:%s\n\
-----------Measured data---------------\n\
First inside stand:\n\
Hz:%0.4f +- %0.4fg  Z:%0.4f +- %0.4fg  dis:%0.4f +- %0.4fmm\n\
Hz:%0.4f +- %0.4fg  Z:%0.4f +- %0.4fg  dis:%0.4f +- %0.4fmm\n\
Outside stand:\n\
Hz:%0.4f +- %0.4fg  Z:%0.4f +- %0.4fg  dis:%0.4f +- %0.4fmm\n\
Hz:%0.4f +- %0.4fg  Z:%0.4f +- %0.4fg  dis:%0.4f +- %0.4fmm\n\
Second inside stand:\n\
Hz:%0.4f +- %0.4fg  Z:%0.4f +- %0.4fg  dis:%0.4f +- %0.4fmm\n\
Hz:%0.4f +- %0.4fg  Z:%0.4f +- %0.4fg  dis:%0.4f +- %0.4fmm\n"), CTime::GetCurrentTime().Format("%H:%M, %d-%m-%Y"),
				result,
				measurement_size,
				stabilitystr,
				newcomp,
				newcomp_dv,
				compensationinput,
				comp_diff,
				comp_diff*20000,
				product,
				productid,
				ip_adress,
				reflector,
				reflectorid,
				temperature,
				humidity,
				pressure,
				horizontals[0], horizontals_dv[0], zenits[0], zenits_dv[0], distances[0], distances_dv[0],
				horizontals[1], horizontals_dv[1], zenits[1], zenits_dv[1], distances[1], distances_dv[1],
				horizontals[2], horizontals_dv[2], zenits[2], zenits_dv[2], distances[2], distances_dv[2],
				horizontals[3], horizontals_dv[3], zenits[3], zenits_dv[3], distances[3], distances_dv[3],
				horizontals[4], horizontals_dv[4], zenits[4], zenits_dv[4], distances[4], distances_dv[4],
				horizontals[5], horizontals_dv[5], zenits[5], zenits_dv[5], distances[5], distances_dv[5]
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
		m_rotate.EnableWindow(false);
		
	}
}

void CMyESAPIReceive::OnPositionRelativeHVAnswer()
{
	m_EsCommand.FindReflector(0.0001);
	pMainWnd->m_status.SetWindowText(_T("finding reflector..."));
	pMainWnd->m_rotate.EnableWindow(true);
	
}
void CMyESAPIReceive::OnFindReflectorAnswer()
{
	pMainWnd->m_status.SetWindowText(_T("Reflector found"));
}



void CATAddConDlg::OnBnClickedIniOn()
{
	if (m_bConnected == true) {
		ES_SystemParameter status;
		status = ES_SP_InclinationSensorState;
		m_EsCommand.SetLongSystemParameter(status, 2);
		pMainWnd->m_status.SetWindowText(_T("Inclination sensor on"));
	}
}


void CATAddConDlg::OnBnClickedIniOff()
{
	if (m_bConnected == true) {
		ES_SystemParameter status;
		status = ES_SP_InclinationSensorState;
		m_EsCommand.SetLongSystemParameter(status, 0);
		pMainWnd->m_status.SetWindowText(_T("Inclination sensor off"));
	}
}


void CATAddConDlg::InitMeasuremodeBox()
{
	m_measuremode.AddString(_T("Standard"));
	m_measuremode.AddString(_T("Fast"));
	m_measuremode.AddString(_T("Precise"));
	m_measuremode.AddString(_T("Outdoor"));
}

void CATAddConDlg::OnBnClickedSetmeasure()
{
	if (m_bConnected == true)
	{
		if (m_measuremode.EnableWindow(false))
		{
			m_measuremode.EnableWindow(true);
			m_setmeasure.SetWindowText(_T("Apply mode"));
		}
		else if (m_measuremode.EnableWindow(true))
		{
			CString current_mode;
			m_measuremode.GetWindowText(current_mode);
			ES_MeasMode mode;

			if (current_mode == "Standard") {
				mode = ES_MM_Standard;
				m_EsCommand.SetMeasurementMode(mode);
			}
			else if (current_mode == "Fast") {
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
			m_measuremode.EnableWindow(false);
			m_setmeasure.SetWindowText(_T("Change mode"));
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
	pMainWnd->m_measuremode.EnableWindow(false);
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



