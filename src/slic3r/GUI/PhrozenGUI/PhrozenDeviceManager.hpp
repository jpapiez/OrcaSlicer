#ifndef slic3r_PhrozenDeviceManager_hpp_
#define slic3r_PhrozenDeviceManager_hpp_

#include "../DeviceManager.hpp"
#include "../../Utils/Phrozen/PhrozenMachineDatas.hpp"
#include <atomic>

namespace Slic3r {

class PhrozenNetworkAgent;
class WorkerFuncSafe;

#pragma region PhrozenMachineObject
class PhrozenMachineObject : public MachineObject
{

public:
    PhrozenMachineObject( std::string name, std::string id, std::string ip );
    PhrozenMachineObject( std::string ip );
    ~PhrozenMachineObject();

    virtual float GetPhrozenBedTemperature();
    virtual float GetPhrozenNozzleTemperature();
    virtual float GetPhrozenPrintSpeed();
    virtual float GetPhrozenAuxiliaryCoolingSpeed();
    virtual float GetPhrozenPartCoolingSpeed();
    virtual float GetPhrozenShieldCoolingSpeed();

    virtual float GetPhrozenBedTargetTemperature();
    virtual float GetPhrozenNozzleTargetTemperature();
    virtual float GetPhrozenZOffset();

    virtual int GetPhrozenBedTemperature_limit();
    virtual int GetPhrozenNozzleTemperature_limit();
    virtual int GetPhrozenCoolingPower_limit();
    // print states
    virtual std::string GetPhrozenPrintStatus();
    virtual std::string GetPhrozenPrintFile();
    virtual std::string GetPhrozenThumbnailPath();
    virtual void GetPhrozenThumbnailInfo(std::string);
    virtual void GetPhrozenThumbnailImage(std::string);
    virtual bool GetPhrozenThumbnailAsBitmap(const std::string& gcodeName, wxBitmap& thumbnailBitmap);
    virtual float GetPhrozenPrintProgress();
    virtual float GetPhrozenPrintTime();
    virtual float GetPhrozenTotalTime();
    virtual float GetPhrozenPrintFilamentAmount();
    virtual std::string GetPhrozenSendPrintTime();
    virtual bool IsPrintPaused();

    virtual bool GetPhrozenCommand_lighting_enabled();

    virtual double GetPhrozenSendFileProgress();

    // set command to machine
    //control
    virtual void SetPhrozenCommand_bed_temp( int nTemp );
    virtual void SetPhrozenCommand_nozzle_temp( int nTemp );
    virtual void SetPhrozenCommand_cooling_auxiliary( int nPower );
    virtual void SetPhrozenCommand_cooling_part( int nPower );
    virtual void SetPhrozenCommand_cooling_shield( int nPower );
    virtual void SetPhrozenCommand_print_speed( float fValue );
    virtual void SetPhrozenCommand_nozzle_movement( std::string ,float fValue );
    virtual void SetPhrozenCommand_nozzle_offset(float fValue );
    //ams
    virtual void SetPhrozenCommand_load(int filament_id);
    virtual void SetPhrozenCommand_unload(int filament_id);
    virtual void SetPhrozenCommand_unload_all_slots();
    virtual void SetPhrozenCommand_nozzle_filament_check();
    //print control pause, resume,abort
    virtual bool SetPhrozenCommand_pause();
    virtual bool SetPhrozenCommand_resume();
    virtual bool SetPhrozenCommand_abort();
    virtual bool SetPhrozenCommand_sendandprint(std::string);

    virtual void SetPhrozenCommand_lighting_enabled(  bool bEnabled );

    virtual bool IsPhrozenConnected();
    virtual bool IsPhrozenStartReceiving();

    virtual std::string GetPhrozenConnectedMachineIp();
    
    // Calibration functions
    // Start calibration (async)
    virtual bool StartCalibration();
    
    // Start resonance compensation (async)
    virtual bool StartResonanceCompensation();
    
    // Start temperature calibration (async)
    virtual bool StartTemperatureCalibration();
    
    // Get calibration status (returns int: 0=STOPPED, 1=RUNNING, 2=COMPLETED, 3=ERROR)
    virtual int GetCalibrationStatus();
    virtual int GetResonanceCompensationStatus();
    virtual int GetTemperatureCalibrationStatus();
    
    // Get calibration progress (0-100)
    virtual float GetCalibrationProgress();
    virtual float GetResonanceCompensationProgress();
    virtual float GetTemperatureCalibrationProgress();
    
    // Check if any calibration is running
    virtual bool IsAnyCalibrationRunning();

    // link to console page by local webside
    virtual std::string GetConsolePageHyperlink();
};
#pragma endregion

#pragma region PhrozenMachineObject_Dev
class PhrozenMachineObject_Dev 
{
public:
    PhrozenMachineObject_Dev( std::string ip, PhrozenNetworkAgent* pAgent );
    ~PhrozenMachineObject_Dev();

    std::string GetMachineIp() { return m_strIp; }

    // Read from ui
    bool ReadDataFromWebcamSnapshot( std::vector<unsigned char>& data );
    bool ReadDataFromAMSInfoList( std::vector< PhrozenAMSInfo >& data );
    bool ReadDataFromCalibrationProgressInfo( PhrozenCalibrationProgressInfo& data );
    bool IsConnectedToAMS();
    bool IsMachineLED_On();
    bool IsNozzleDetectFilament();

    // Recieve from machine
    void MoveDataToWebcamSnapshot( std::vector<unsigned char>& data );
    void MoveDataToPrinterInfo( PhrozenPrinterInfo& data );
    void MoveDataToAMSInfoList( std::vector< PhrozenAMSInfo >& data );
    void MoveDataToCalibrationProgressInfo( PhrozenCalibrationProgressInfo& kData );
    void SetIsConnectedToAMS( const bool& bConnected );
    void SetIsMachineLED_On( const bool& bOn );
    void SetIsNozzleDetectFilament( const bool& bDetected );


    //PrinterInfo
    float GetPhrozenBedTemperature();
    float GetPhrozenNozzleTemperature();
    float GetPhrozenPrintSpeed();
    float GetPhrozenAuxiliaryCoolingSpeed();
    float GetPhrozenPartCoolingSpeed();
    float GetPhrozenShieldCoolingSpeed();
    float GetPhrozenBedTargetTemperature();
    float GetPhrozenNozzleTargetTemperature();
    std::string GetPhrozenPrintStatus();
    std::string GetPhrozenPrintFile();
    std::string GetPhrozenThumbnailPath();
    float GetPhrozenPrintProgress();
    float GetPhrozenPrintTime();
    float GetPhrozenTotalTime();
    float GetPhrozenPrintFilamentAmount();
    std::string GetPhrozenSendPrintTime();
    bool IsPrintPaused();

    int GetPhrozenBedTemperature_limit() { return 300; }
    int GetPhrozenNozzleTemperature_limit() { return 300; }




private:
    



private:
    PhrozenNetworkAgent* m_pNetworkAgent { nullptr };
    std::string m_strIp;

    DoubleBufferSP< std::vector<unsigned char> >* GetWebcameSnapshotPtr() { return &m_webcame_snapshot; }
    DoubleBufferSP< std::vector<unsigned char> > m_webcame_snapshot;

    DoubleBufferSP< PhrozenPrinterInfo >* PrinterInfoPtr() { return &m_printerInfo; }
    DoubleBufferSP< PhrozenPrinterInfo > m_printerInfo;

    DoubleBufferSP< PhrozenCalibrationProgressInfo > m_calibrationProgressInfo;

    DoubleBufferSP< bool > m_connectedToAMS;
    DoubleBufferSP< bool > m_machineLED_On;
    DoubleBufferSP< bool > m_nozzleDetectFilament;
    DoubleBufferSP< std::vector< PhrozenAMSInfo > > m_AMSInfoList;
     



};
#pragma endregion

#pragma region PhrozenDeviceSearchResult
class PhrozenDeviceSearchResult
{
public:
    PhrozenDeviceSearchResult(){}

    void ClearAll() { m_kFoundedListA.clear(); m_kFoundedListB.clear(); }
    std::map< std::string, std::string > GetFounded() { return *m_pReadBuffer; }

    void SetDataReady( bool bReady )
    {
        if ( bReady ) { m_bDataReady.store(true, std::memory_order_relaxed); }
        else          { m_bDataReady.store(false, std::memory_order_relaxed); }
    }
    bool IsDataReady()
    {
        return m_bDataReady.load(std::memory_order_relaxed);
    }

    void WriteDataAndSwap( std::map< std::string, std::string >& kData )
    {
        SetDataReady(false);
        m_pWriteBuffer->clear();
        ( *m_pWriteBuffer ) = std::move( kData );

        //swap buffer pointer
        auto tempBuffer = m_pReadBuffer;
        m_pReadBuffer = m_pWriteBuffer;
        m_pWriteBuffer = tempBuffer;

        SetDataReady(true);
    }

private:

    std::atomic<bool> m_bDataReady{false};

    std::map< std::string, std::string > m_kFoundedListA;
    std::map< std::string, std::string > m_kFoundedListB;
    std::map< std::string, std::string >* m_pWriteBuffer = &m_kFoundedListA;
    std::map< std::string, std::string >* m_pReadBuffer = &m_kFoundedListB;
};
#pragma endregion 

#pragma region PhrozenDeviceSearcher
class PhrozenDeviceSearcher
{
public:

static void StartSearch();
static void StopSearch();
static bool IsDataReady();
static std::map< std::string, std::string > GetList();

private:
static void Run() noexcept;
static void ProcessSearchMachine( std::map< std::string, std::string >& kResult );


static std::unique_ptr<boost::thread> t_;
static std::atomic<bool> stop_;



static std::exception_ptr eptr_;

static PhrozenDeviceSearchResult m_kSearchResult;
};
#pragma endregion 

#pragma region PhrozenDeviceManager
class PhrozenDeviceManager
{

public:
    PhrozenDeviceManager( PhrozenNetworkAgent* agent = nullptr);
    ~PhrozenDeviceManager();
    void set_agent( PhrozenNetworkAgent* agent);
    
    void DisconnectMachine();
    bool CreateAndConnectMachine( std::string strIp );
    PhrozenMachineObject_Dev* GetConnectingMachine();

    bool StartReceiveWebcam();
    void StopReceiveWebcam();
    // 非同步停止：立即釋放 m_spRecieveWebcam 所有權給 detached thread 執行 Stop()，
    // 返回後 m_spRecieveWebcam == nullptr，可立即呼叫 StartReceiveWebcam()。
    void StopReceiveWebcamAsync();

    bool StartSendMessage();
    void StopSendMessage();

    bool StartReceiveMessage();
    void StopReceiveMessage();

    bool IsMachineConnecting() { return m_spConnectedMachine != nullptr; }

private:
    bool CreateMachine( std::string dev_id , std::shared_ptr< PhrozenMachineObject_Dev >& spObject );
    
    std::shared_ptr< PhrozenMachineObject_Dev > m_spConnectedMachine{ nullptr };
    PhrozenNetworkAgent* m_pNetworkAgent { nullptr };

    std::vector<bool> m_kSendingList;
private:
    std::unique_ptr< WorkerFuncSafe > m_spRecieveWebcam{ nullptr };
    std::unique_ptr< WorkerFuncSafe > m_spSendMessage{ nullptr };
    std::unique_ptr< WorkerFuncSafe > m_spReceiveMessage{ nullptr };
};
#pragma endregion 

} // namespace Slic3r

#endif //  slic3r_DeviceManager_hpp_
