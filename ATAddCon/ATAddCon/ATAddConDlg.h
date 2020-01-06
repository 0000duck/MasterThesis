
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
	void OnGetSystemSettingsAnswer(const SystemSettingsDataT& systemSettings); // get system settings (to change measure mode)
	void OnGetReflectorsAnswer(const int            iTotalReflectors,
								const int            iInternalReflectorId,
								const ES_TargetType  targetType,
								const double         dSurfaceOffset,
								const unsigned short cReflectorName[32]);
	void OnGetTrackerInfoAnswer(const ES_LTSensorType trackerType,
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
									const int iMinLCPFirmwareVersion);
	void OnGetReflectorAnswer(const int iInternalReflectorId);
	void OnSetReflectorAnswer();
	void OnGetUnitsAnswer(const SystemUnitsDataT& unitsSettings);
	void OnPositionRelativeHVAnswer();
	void OnGetMeasurementModeAnswer(const ES_MeasMode measMode);
	void OnSetMeasurementModeAnswer();
	void OnGetLongSystemParamAnswer(const long lParameter);
	void OnFindReflectorAnswer();
	CMyESAPICommand m_EsCommand; // can call next command after other is finished

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
	afx_msg void OnBnClickedStore();
	CListBox m_measure_list;
	CButton m_store;
	CButton m_protocol;
	afx_msg void OnBnClickedProtocol();
	CStatic m_picture;
	CButton m_initialization;
	CButton m_measure;
	CButton m_apply;
	CStatic m_serial;


	CComboBox m_reflectorscombo; 
	int m_nReflectorCount;
	int m_reflIDMap[24];
	CString m_reflNameMap[24];
	void InitReflectorBox();

	
	afx_msg void OnBnClickedsetreflector();

	SystemSettingsDataT setting_parameters;
	double dis;//final_measure;
	double dis_dv;//final_deviation;
	double Hz;
	double Hz_dv;
	double Z;
	double Z_dv;
	std::vector<double> distances;//lengths;
	std::vector<double> distances_dv;//deviations;
	std::vector<double> horizontals;
	std::vector<double> horizontals_dv;
	std::vector<double> zenits;
	std::vector<double> zenits_dv;


	CStatic m_product;
	CEdit m_reflectorid;
	CButton m_stability;
	afx_msg void OnBnClickedRotate();
	CEdit m_compeinput;
	CStatic m_temperature;
	CStatic m_pressure;
	CStatic m_humidity;
	afx_msg void OnBnClickedIniOn();
	afx_msg void OnBnClickedIniOff();
	CButton m_ini_off;
	CButton m_ini_on;
	CComboBox m_measuremode;
	void InitMeasuremodeBox();
	afx_msg void OnBnClickedSetmeasure();
	CButton m_rotate;
	CButton m_setmeasure;
};