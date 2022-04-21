#ifndef CCONFIG_H
#define CCONFIG_H

#include "json/json.h"
#include <QtCore>


struct SystemCfgInfo 
{
    unsigned int ulSubModuleMask;  // module to be selected
    bool bHaveToc;
    bool bHaveSWTank;
    bool bHavePWTank;
    bool bHaveFlusher;
    bool bHaveTankUv;
};

struct ConsumablesCfg 
{
    unsigned int ulPart1; 
    unsigned int ulPart2; 
};

struct SetPointCfgInfo 
{
    unsigned int ulPart1; 
    unsigned int ulPart2; 
};

struct CalibrationCfgInfo 
{
    unsigned int ulPart1; 
};



class CConfig : public QObject
{
   Q_OBJECT ;
   
public:   
    CConfig();
    ~CConfig(){}
    
    enum {

        CONFIG_MachineType = 0,
        CONFIG_ALARM_POINT_STRU    ,
        CONFIG_DISP_TIME_PARAM_STRU         ,
        CONFIG_DISP_ALARM_SETTING_STRU      ,
        CONFIG_DISP_SUB_MODULE_SETTING_STRU ,
        CONFIG_DISP_CONSUME_MATERIAL_STRU   ,
        CONFIG_DISP_FM_SETTING_STRU         ,
        CONFIG_DISP_PM_SETTING_STRU         ,
        CONFIG_DISP_MISC_SETTING_STRU       ,
        CONFIG_DISP_CLEAN_SETTING_STRU      ,
        CONFIG_DISP_CONSUME_MATERIAL_SN_STRU ,
        CONFIG_DISP_MACHINERY_SN_STRU        ,
        CONFIG_DISP_PARAM_CALI_STRU          ,
        CONFIG_DISP_SENSOR_RANGE_STRU        ,
        CONFIG_DISP_ADDITIONAL_PARAM         ,
        CONFIG_NUM,
    };

    enum {
            TESTQUERY_OUTPUT = 0,
            TESTQUERY_INPUT     ,
            TESTQUERY_ECO       ,
            TESTQUERY_PM        ,
            TESTQUERY_RECT      ,
            TESTQUERY_EDI       ,
            TESTQUERY_RPUMP     ,
            TESTQUERY_GPUMP     ,
            TESTQUERY_NUM       ,
    };

    void buildJson4Param(int iFlags,std::string &rsp);

    Json::Value getJson4Param(int iFlags);

    void buildJson4Accessory(int iFlags,std::string &rsp);
    
    Json::Value getJson4Accessory(int iFlags);

    void buildJson4CmUsage(int iFlags,std::string &rsp);
    
    Json::Value getJson4CmUsage(int iFlags);

    void buildJson4Alarm(int iFlags,std::string &rsp);
    
    Json::Value getJson4Alarm(int iFlags);
    
    void buildJson4TestQuery(int iFlags,std::string &rsp);
    
    Json::Value getJson4TestQuery(int iFlags);

    void doQueryInitCfg(std::string &rsp);

    void doQuerySystemCfg(std::string &rsp);

    //dcj add
    void doQueryAlerts(std::string &rsp);
    void doQueryConsumablesCfg(std::string &rsp);
    void doQuerySetPointCfg(std::string &rsp);
    void doQueryCalibrationCfg(std::string &rsp);
    void doQuerySysTestCfg(std::string &rsp);
    void doQueryAlarmSetCfg(std::string &rsp);

    void doQueryWifi(std::string &rsp);

    void doQueryEth(std::string &json);

    void doQueryUser(std::string &json);

    void doQueryDevices(std::string &json);

    void doQuerySysTestInfo(int iMask,std::string &json);
    
    void doQueryDateTime(std::string &json);

    bool doParam(std::string &req,std::string &rsp);

    bool doSysTest(std::string &req,std::string &rsp);

    bool doWifiConfig(std::string &req,std::string &rsp);

    bool doEthnetConfig(std::string &req,std::string &rsp);

    bool doReset(std::string &req,std::string &rsp);

    bool doCmReset(std::string &req,std::string &rsp);

    bool doLogin(std::string &req,std::string &rsp);

    bool doUserMgr(std::string &req,std::string &rsp);

    bool doDateTimeCfg(std::string &req,std::string &rsp);

    bool doExportCfg(std::string &req,std::string &rsp);
    
    bool doDelDb(std::string &req,std::string &rsp);

    bool doDelCfg(std::string &req,std::string &rsp);

    void dbCfgDelSetResult(int iResult,QString strResult) {m_idbCfgDelResult = iResult;m_strResult = strResult;}

signals:
    void dbDel(int iChoice);
    void cfgDel(int iChoice);

public:
    bool m_bReset;

    bool m_bSysReboot;

    int  m_idbCfgDelResult;
    QString m_strResult;

    // 2020/08/28 add for web config
    SystemCfgInfo      m_SystemCfgInfo;
    SetPointCfgInfo    m_SetPointCfgInfo;
    CalibrationCfgInfo m_CalibrationCfgInfo;
    CalibrationCfgInfo m_SysTestCfgInfo;
    ConsumablesCfg     m_ConsumablesCfg;
    ConsumablesCfg     m_AlarmSetCfg;

};

extern CConfig gCConfig;

#endif // MAINWINDOW_H
