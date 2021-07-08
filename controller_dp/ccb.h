#ifndef _CCB_H_
#define _CCB_H_


#include <QtCore>
#include <QMap>

#include "SatIpc.h"
#include "Errorcode.h"
#include "msgdef.h"
#include "msg.h"
#include "list.h"
#include "timer.h"
#include "rpc.h"
#include "sapp.h"
#include "datatype.h"
#include "inneripc.h"

#include "Display.h"
#include "cminterface.h"
#include "list.h"
#include "vtask.h"
#include "messageitf.h"


#define TIMER_SECOND 0
#define TIMER_MINUTE 1

#define MAX_HANDLER_NUM (APP_HAND_SET_MAX_NUMBER)

#define MAX_RFREADER_NUM (APP_RF_READER_MAX_NUMBER)

#define MAX_RFREADR_CONTENT_SIZE (256)

#define MAX_MODBUS_BUFFER_SIZE   (128)

#define haveB1()(gGlobalParam.SubModSetting.ulFlags & (1 << DISP_SM_HaveB1))
#define haveB3()(gGlobalParam.SubModSetting.ulFlags & (1 << DISP_SM_HaveB3))
#define haveTOC()(gGlobalParam.SubModSetting.ulFlags & (1 << DISP_SM_HaveTOC))
#define haveB2()(gGlobalParam.SubModSetting.ulFlags & (1 << DISP_SM_HaveB2))

#define haveHPCir()(gGlobalParam.MiscParam.ulMisFlags & (1 << DISP_SM_HP_Water_Cir))
#define haveZigbee()(gGlobalParam.MiscParam.iNetworkMask & (1 << DISPLAY_NETWORK_ZIGBEE))

class CCB ;

typedef void (*modbus_call_back)(CCB *,int,void *);

typedef void (*delay_call_back)(void *);


typedef void (*work_proc)(void *);

typedef union                                        
{
   float          f;
   unsigned char  auc[4];
   int            i;
   unsigned int   ul;
}un_data_type;

typedef struct
{
    list_t    list;
    work_proc Work;
    void      *Para;
}WORK_STRU;

typedef struct
{
    unsigned int ulEcoValidFlags;
    unsigned int ulPmValidFlags;
    unsigned int ulRectValidFlags;

    DISP_OBJ_STRU aValveObjs[APP_EXE_VALVE_NUM];
    DISP_OBJ_STRU aPMObjs[APP_EXE_PRESSURE_METER]; /* Pressure Meter */
    DISP_OBJ_STRU aGPumpObjs[APP_EXE_G_PUMP_NUM];
    DISP_OBJ_STRU aRPumpObjs[APP_EXE_R_PUMP_NUM];
    DISP_OBJ_STRU aRectObjs[APP_EXE_RECT_NUM];
    DISP_OBJ_STRU aEDIObjs[APP_EXE_EDI_NUM];
    DISP_OBJ_STRU aEcoObjs[APP_EXE_ECO_NUM];

    unsigned char ucDinState;
    unsigned char ucLeakState; //2020.2.17 增加用于新漏水保护信号

    unsigned short ausHoldRegister[APP_EXE_MAX_HOLD_REGISTERS];

    APP_PACKET_RPT_TOC_STRU tocInfo;  

    int                  iTrxMap; /* bit0 for CAN, bit1 for zigbee  */
    unsigned short       usShortAddr;
    
}EXE_BOARD_STRU;


typedef struct
{
    unsigned int   ulSec;
    int            iVChoice;
    DISP_OBJ_VALUE lstValue;
    DISP_OBJ_VALUE curValue;
}FM_HISTORY_STRU;

typedef struct
{
    unsigned int ulFmValidFlags;
    
    DISP_OBJ_STRU aFmObjs[APP_FM_FLOW_METER_NUM];

    /* for calc fm value */
    FM_HISTORY_STRU aHist[APP_FM_FLOW_METER_NUM];
    unsigned int    aulHistTotal[APP_FM_FLOW_METER_NUM];

    unsigned int    aulPwStart[APP_FM_FLOW_METER_NUM];
    unsigned int    aulPwEnd[APP_FM_FLOW_METER_NUM];
}FLOW_METER_STRU;

typedef struct
{
    unsigned int ulBgnFm;   //  in pulse
    unsigned int ulCurFm;   //  GEt cur volume = f(ulCurFm - ulBgnFm)
    
    unsigned int ulTotalFm; // in ml

    unsigned int ulBgnTm;
    unsigned int ulEndTm;
    
}QTW_MEAS_STRU;


typedef struct
{
   int dummy;
    
}RTI_STRU;

typedef struct
{
    unsigned int  bit1Qtw        : 1;
    unsigned int  bit1PendingQtw : 1;   /* for adapter */
    unsigned int  bit1Active     : 1;
    
    int                  iDevType;      /* APP_DEV_HS_SUB_TYPE_ENUM */

    SN                   sn;
    /* for temperory info */
    APP_PACKET_COMM_STRU CommHdr;
    int                  ulCanId;
    int                  iCanIdx;

    int                  iCurTrxIndex;

    QTW_MEAS_STRU        QtwMeas;

    /* for speed adjustment */
    int                  iAction; 
    int                  iSpeed; 

    int                  iTrxMap; /* bit0 for CAN, bit1 for zigbee  */
    unsigned short       usShortAddr;

    // 2020/08/03 add for RTI    
    struct Show_History_Info m_historyInfo[MSG_NUM];

    unsigned short ausHoldRegister[1];

}HANDLER_STRU;

typedef struct
{
   int bit1RfDetected  : 1;
   int bit1RfContValid : 1;
   int bit1Active      : 1;

   unsigned char ucBlkNum;
   unsigned char ucBlkSize;

   unsigned char aucUid[8];
   unsigned char aucRfCont[MAX_RFREADR_CONTENT_SIZE];

   INNER_IPC_STRU   Ipc;
   int              IpcRequest;
    
}RFREADER_STRU;


//typedef void (*work_proc)(void *);

typedef void (*work_cancel)(void *);


#define WORK_FLAG_CANCEL (1<<1)
#define WORK_FLAG_ACTIVE (1<<0)

typedef struct
{
    list_t      list;
    work_proc   proc;
    work_cancel cancel;
    int         flag;
    int         id;
    void        *para; // fast pointer to CCB
    void        *extra;
}WORK_ITEM_STRU;

typedef struct
{
    unsigned int ulMask;
    unsigned int ulValue;
}WORK_SETUP_REPORT_STRU;

typedef WORK_SETUP_REPORT_STRU WORK_SETUP_SWITCH_STRU;



typedef enum
{
    WORK_LIST_URGENT = 0,
    WORK_LIST_HP,         /* High Priority */
    WORK_LIST_LP,        /* Low  Priority */
    WORK_LIST_NUM,
}WORK_LIST_ENUM;

typedef struct
{
    int              iMainWorkState; // refer DISP_WORK_STATE_ENUM
    int              iSubWorkState;  // refer DISP_WORK_SUB_STATE_ENUM
    int              iMainWorkState4Pw;  // DISP_WORK_STATE_ENUM 
    int              iSubWorkState4Pw;  // DISP_WORK_SUB_STATE_ENUM

}WORK_STATE_STRU;

#define CCB_WORK_STATE_STACK_DEPTH (10)

typedef enum
{
    WORK_THREAD_STATE_BLK_MODBUS  = 0X1, 
    WORK_THREAD_STATE_BLK_TICKING = 0X2, 
}WORK_THREAD_STATE_ENUM;

#define MAX_HB_CHECK_COUNT 3
#define MAX_HB_CHECK_ITEMS 32

class ConsumableInstaller : public QObject
{
    Q_OBJECT;
public:
    explicit ConsumableInstaller(int id);
    void initPackConfig();
    void initOtherConfig();
    
    enum Consumable_Type
    {
        Type0 = 0,  //Pack
        Type1,
        Consumable_Type_Num
    };

    struct Consumable_Install_Info
    {
       int iRfid;
       QString strName;
    };

    void emitInstallMsg();

    bool pendingInstall() {return m_iType != 0xff;}
    
    
signals:
    void installConusumable();
    void setCheckStatus(bool busy);

public slots:
    void setConsumableName(int iType, const QString& catNo, const QString& lotNo);
    void updateConsumableInstall(int iType);

private:
    bool checkUserInfo(const QString& userName);

private:
    int m_iID;
    int m_iType; // refer DISP_CM_NAME_ENUM

    QString m_CatNo;
    QString m_LotNo;
    QString m_strPack;

    QMap<int, Consumable_Install_Info> m_map[Consumable_Type_Num];
    int        m_aiCmMask[Consumable_Type_Num];
    int        m_aiCmCfgedMask[Consumable_Type_Num];
    
};


class CCB : public QObject
{
   Q_OBJECT ;

private:    
   explicit CCB();   
   ~CCB(){ }

   friend class ConsumableInstaller;
   
   unsigned int     bit1InitRuned       : 1;
   unsigned int     bit1AlarmRej        : 1;  /* ro jieliulv */
   unsigned int     bit1AlarmRoPw       : 1;  /* ro chanshui diandaolv */
   unsigned int     bit1AlarmEDI        : 1;  /* edi alarm */
   unsigned int     bit1AlarmUP         : 1;
   unsigned int     bit1AlarmTOC        : 1;
   unsigned int     bit1AlarmTapInPress : 1;
   unsigned int     bit1NeedFillTank    : 1;  
   unsigned int     bit1B2Full          : 1;  /* tank full */
   unsigned int     bit1FillingTank     : 1;  
   unsigned int     bit1N3Active        : 1;  /* ultralight N3 period handle flag */
   unsigned int     bit1ProduceWater    : 1;  /* RUN state & producing water */
   unsigned int     bit1B2Empty         : 1;  /* tank empty */
   unsigned int     bit1I54Cir          : 1;  
   unsigned int     bit1I44Nw           : 1;  
   unsigned int     bit1LeakKey4Reset   : 1;  
   unsigned int     bit1EngineerMode    : 1;
   unsigned int     bit1NeedTubeCir     : 1;
   unsigned int     bit1TubeCirOngoing  : 1;
   unsigned int     bit1TocOngoing      : 1;
   unsigned int     bit1SysMonitorStateRpt  : 1;
   unsigned int     bit1PeriodFlushing  : 1;

   /* 2018/01/05 add for B1 check, Life cycle starts earlier than bit1ProduceWater */
   unsigned int     bit1B1Check4RuningState  : 1;
   unsigned int     bit1B1UnderPressureDetected  : 1;
   unsigned int     bit3RuningState              : 3;    

   unsigned int     bit1AlarmROPLV        : 1;  /* ROPV alarm */
   unsigned int     bit1AlarmROPHV        : 1;  /* ROPV alarm */
   unsigned int     bit1AlarmRODV         : 1;   /* RODV alarm */
   unsigned int     bit1CirSpeedAdjust    : 1;   /* RODV alarm */

   //2019.12.31增加
   unsigned int     bit1ROWashPause       : 1;  //系统清洗时，原水箱空，则暂定清洗
   unsigned int     bit1TocAlarmNeedCheck : 1;  //是否开启TOC报警检测
   unsigned int     bit1AlarmWorkPressHigh: 1;  //工作压力高报警

   unsigned int     ulRegisterMask;
   unsigned int     ulActiveMask;
   unsigned int     ulActiveShadowMask;
   unsigned int     ulActiveMask4HbCheck;

   unsigned char    aucHbCounts[MAX_HB_CHECK_ITEMS];
   
   EXE_BOARD_STRU   ExeBrd;

   FLOW_METER_STRU  FlowMeter;
   
   HANDLER_STRU     aHandler[MAX_HANDLER_NUM];

   RFREADER_STRU    aRfReader[MAX_RFREADER_NUM];

   int              m_aWaterType[APP_DEV_HS_SUB_NUM]; // refer : APP_WATER_TYPE_ENUM
   
   int              iCurTwIdx;

   int              iHbtCnt;

   INNER_IPC_STRU   Ipc;
   modbus_call_back ModbusCb;

   /* ipc for rfreader */
   
   INNER_IPC_STRU   Ipc4Rfid;
   int              iRfIdRequest;

   
   INNER_IPC_STRU   m_Ipc4DevMgr;
   WORK_STATE_STRU  curWorkState;

   WORK_STATE_STRU  aWorkStateStack[CCB_WORK_STATE_STACK_DEPTH];
   int              iWorkStateStackDepth;

   list_t            WorkList[WORK_LIST_NUM];
   int               iBusyWork;
   sp_thread_mutex_t WorkMutex;

   sp_thread_mutex_t ModbusMutex;
   
   sp_thread_mutex_t C12Mutex;

   sp_sem_t         Sem4Delay[WORK_LIST_NUM + 1];    /* Used in work Delay */
   SYS_TIMEO        to4Delay[WORK_LIST_NUM +1];

   unsigned int    ulHyperTwMask;         /* switch Mask for taking hyper water */
   unsigned int    ulNormalTwMask;        /* switch Mask for taking normal water */
   unsigned int    ulCirMask;             /* switch Mask for ciculation water */

   unsigned int    ulRunMask;            /* switch Mask for producing water */

   unsigned int    ulPMMask;             /* PM facilities for specific machine type */
   


   unsigned int     ulAlarmEDITick ;
   unsigned int     ulAlarmRejTick ;
   unsigned int     ulAlarmRoPwTick ;
   unsigned int     ulAlarmUPTick ;

   unsigned int     ulB2FullTick;
   unsigned int     ulLPPTick;

   /* 2018/01/05 add accroding to ZHANG chunhe*/
   unsigned int     ulB1UnderPressureTick;
   
   unsigned int     ulTubeIdleCirTick;
   unsigned int     ulTubeIdleCirIntervalTick;
   unsigned int     ulTubeCirStartDelayTick;
   unsigned int     ulProduceWaterBgnTime;

   unsigned int     ulCirTick;
   unsigned int     ulLstCirTick;
            int     iCirType;           /* refer APP_CIR_TYPE_ENUM */
            int     iTocStage;          /* refer TOC_STAGE_ENUM */
            int     iTocStageTimer;
            
            int     iInitRunTimer;

   unsigned int     ulN3Duration;
   unsigned int     ulN3PeriodTimer;

   unsigned int     ulFiredAlarmFlags;

   unsigned int     ulKeyWorkStates;
   unsigned char    ucLeakWorkStates; //2020.2.17 漏水保护状态

   WORK_SETUP_REPORT_STRU WorkRptSetParam4Exe;
   WORK_SETUP_REPORT_STRU WorkRptSetParam4Fm;
   WORK_SETUP_SWITCH_STRU WorkSwitchSetParam4Exe;

   QTW_MEAS_STRU          QtwMeas;

   unsigned  char   aucModbusBuffer[MAX_MODBUS_BUFFER_SIZE];

   unsigned int     ulWorkThdState; /* 0: free , other blocked */
   unsigned int     ulWorkThdAbort;

   /* 2017/10/24 add for wash */
   int              iWashType;  

   /* for CAN Healthy check */
   unsigned int     ulCanCheck;

   /* 2018/01/11 add begin:  for REJ check */
   unsigned int     ulRejCheckCount;
   /* 2018/01/11 add end */

   /* 2018/01/15 add begin:  for ROP velocity check */
   unsigned int     ulRopVelocity;
   unsigned int     ulLstRopFlow;
   unsigned int     ulLstRopTick;
   int              iRopVCheckLowEventCount;
   int              iRopVCheckLowRestoreCount;
 
   int              iRopVCheckHighEventCount;
   int              iRopVCheckHighRestoreCount;
   
   unsigned int     ulRodVelocity;
   unsigned int     ulLstRodFlow;
   unsigned int     ulLstRodTick;
   int              iRoDVCheckEventCount;
   int              iRoDVCheckRestoreCount;
   
   /* 2018/01/15 add end */

   /* 2018/01/24 ADD begin: for adapter */
   unsigned int     ulAdapterAgingCount;
   /* 2018/01/24 ADD end : for adapter */
    

   int              aiSpeed2ScaleMap[PUMP_SPEED_NUM];

   /* 2018/02/22 add for zigbee */
   int              aulActMask4Trx[APP_TRX_NUM];

   int              m_iInstallHdlIdx;

   QMap<int,const char *> m_WorkName;
   
   void CcbInit();
   void MainInitMsg();
   void MainDeInitMsg();
   void MainInitInnerIpc();
   void MainDeinitInnerIpc();
   void CcbInitMachineType();
   void MainResetModulers();
   void MainSndHeartBeat();
   int  CcbModbusWorkEntryWrapper(int id,int iChl,unsigned int ulCanId,unsigned char *pucModbus,int iPayLoad,int iTime,modbus_call_back cb);
   int CcbModbusWorkEntry(int iChl,unsigned int ulCanId,unsigned char *pucModbus,int iPayLoad,int iTime,modbus_call_back cb);
   void CcbWorMsgProc(SAT_MSG_HEAD *pucMsg);
   int is_B2_FULL(unsigned int ulValue);
   void CcbSetActiveMask(int iSrcAddr);
   
   void MainProcTimerMsg(SAT_MSG_HEAD *pMsg);
   void MainSecondTask();
   void MainMinuteTask();
   void MainSecondTask4Pw();
   void MainSndWorkMsg(int iSubMsg,unsigned char *data, int iPayLoadLen);
   void MainHeartBeatProc();
   void MainSecondTask4MainState();
   
   void CanCcbCanItfMsg(SAT_MSG_HEAD *pucMsg);
   void CcbAddExeSwitchWork(WORK_SETUP_REPORT_STRU *pRpt );
   void CcbAddExeReportWork(WORK_SETUP_REPORT_STRU *pRpt );
   void CcbAddFmReportWork(WORK_SETUP_REPORT_STRU *pRpt );
   void checkB3ToReInitRun();
   
   void CanCcbAfProc(MAIN_CANITF_MSG_STRU *pCanItfMsg);
   int CanCcbAfModbusMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
   void CanCcbAfRfIdMsg(int mask);
   void CcbReset();
   void CcbCleanup();
   void CanCcbExeReset();
   void CanCcbInnerReset(int iFlags);
   void CanCcbFMReset();
   int CanCcbGetHoState();
   int CanCcbGetMainCtrlState();
   int CanCcbGetWaterType(int iDevType);
   int CanCcbAfDataOnLineNotiIndMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
   int CanCcbAfDataHeartBeatRspMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
   void checkB1DuringRunState();
   void checkB2ToProcessB2Empty();
   void checkB2ToProcessB2Full();
   void checkB3ToProcessB3Full();
   void checkB3ToProcessB3NeedFill();
   void checkB3ToProcessB3Empty();
   void processB3Measure();
   void CanCcbPmMeasurePostProcess(int iPmId);
   void CcbNotAlarmFire(int iPart,int iId,int bFired);
   void Check_RO_EDI_Alarm(int iEcoId);
   void CanCcbEcoMeasurePostProcess(int iEcoId);
   void CanCcbRectNMeasurePostProcess(int rectID);
   void CcbEcoNotify();
   void CcbPmNotify();
   void CcbRectNotify();
   void CcbGPumpNotify();
   void CcbRPumpNotify();
   void CcbEDINotify();
   void CcbFmNotify();
   void CcbNotState(int state);
   void CcbNotSpeed(int iType,int iSpeed);
   void CcbNotDecPressure(int iType,int iAction);
   void CcbNotSWPressure();
   void CcbNotAscInfo(int iInfoIdx);
   void CcbNvStaticsNotify();
   void CcbPumpStaticsNotify();
   void CcbFmStaticsNotify();
   void CcbTwStaticsNotify(int iType,int iIdx,QTW_MEAS_STRU *pMeas);
   void CcbRealTimeQTwVolumnNotify(unsigned int value);
   void CcbQTwVolumnNotify(int iType,int iIdx,QTW_MEAS_STRU *pMeas);
   void CcbWashStateNotify(int iType,int state);
   void CcbSwithStateNotify();
   void CcbRPumpStateNotify(int ichl,int state);
   void CcbProduceWater(unsigned int ulBgnTm);
   void CcbPwVolumeStatistics();
   void CcbRfidNotify(int ucIndex);
   void CcbHandlerNotify(int ucIndex);
   void CcbExeBrdNotify();
   void CcbFmBrdNotify();
   void CcbTocNotify();
   void CanCcbAfDataClientRpt4ExeDinAck(MAIN_CANITF_MSG_STRU *pCanItfMsg);
   void CcbPushState();
   void CcbPopState();
   
   void CanCcbSaveQtw2(int iTrxIndex, int iDevId,unsigned int ulVolume);
  void CcbInitHandlerQtwMeas(int iIndex);
  void CcbFiniHandlerQtwMeas(int iIndex);
  void CcbUpdateSwitchObjState(unsigned int ulMask,unsigned int ulValue);
  
  unsigned int CcbGetSwitchObjState(unsigned int ulMask);
  void CcbUpdateRPumpObjState(int iChl,int iState);
  void CcbUpdatePmObjState(unsigned int ulMask,unsigned int ulValue);
  unsigned int CcbGetPmObjState(unsigned int ulMask);
  void CcbUpdateIObjState(unsigned int ulMask,unsigned int ulValue);
  unsigned int CcbGetIObjState(unsigned int ulMask);
  void CcbUpdateFmObjState(unsigned int ulMask,unsigned int ulValue);
  unsigned int CcbGetFmObjState(unsigned int ulMask);
  int CcbWorkDelayEntryWrapper(int id,unsigned int ulTime,sys_timeout_handler cb);
  
  int CcbWorkDelayEntry(int id,unsigned int ulTime,sys_timeout_handler cb);
  unsigned int CcbGetModbus2BytesData(int offset);
  unsigned int CcbGetModbus4BytesData(int offset);
  int CcbSwitchUpdateModbusWrapper(int id,unsigned int ulAddress, unsigned int ulMask,unsigned int ulSwitchs);
  int CcbSwitchSetModbusWrapper(int id,unsigned int ulAddress, unsigned int ulNums,unsigned int ulSwitchs);
  int CcbInnerSetSwitch(int id,unsigned int ulAddress, unsigned int ulSwitchs);
  int CcbSetSwitch(int id,unsigned int ulAddress, unsigned int ulSwitchs);
  int CcbInnerUpdateSwitch(int id,unsigned int ulAddress, unsigned int ulMask,unsigned int ulSwitchs);
  int CcbUpdateSwitch(int id,unsigned int ulAddress, unsigned int ulMask,unsigned int ulSwitchs);
  int CcbSetIAndBs(int id,unsigned int ulAddress, unsigned int ulSwitchs);
  int CcbUpdateIAndBs(int id,unsigned int ulAddress, unsigned int ulMask,unsigned int ulValue);
  int CcbSetFms(int id,unsigned int ulAddress, unsigned int ulSwitchs);
  int CcbUpdateFms(int id,unsigned int ulAddress, unsigned int ulMask,unsigned int ulValue);
  int CcbSetExeHoldRegs(int id,unsigned int ulAddress, unsigned int ulValue);
  int CcbGetExeHoldRegs(int id,unsigned int ulAddress,int Regs);
  int CcbGetHandsetHoldRegs(int iIndex,unsigned int ulAddress,int Regs);
  int CcbGetPmValue(int id,unsigned int ulAddress, int iPMs);
  int CcbGetIValue(int id,unsigned int ulAddress, int iIs);
  int CcbGetFmValue(int id,unsigned int ulAddress, int iFMs);
  int CcbGetDinState(int id);
  void CanCcbPrintFsm();
  void CanCcbTransState(int iDstMainState,int iDstSubState);
  void CanCcbTransState4Pw(int iDstMainState,int iDstSubState);
  DISPHANDLE SearchWork(work_proc proc);
  void CcbScheduleWork();
  void CcbAddWork(int iList,WORK_ITEM_STRU *pWorkItem);
  void work_cir_fail(int iWorkId);
  void work_cir_succ();
  DISPHANDLE CcbInnerWorkStartCirToc(int iStage);
  DISPHANDLE CcbInnerWorkStartCir(int iType);
  void work_stop_cir_succ();
  DISPHANDLE CcbInnerWorkStopCir();
  void work_speed_regulation_end(int iIndex,int iResult);
  DISPHANDLE CcbInnerWorkStartSpeedRegulation(int iIndex);

  void work_run_comm_proc(WORK_ITEM_STRU *pWorkItem, bool bInit);
  void work_normal_run_fail(int iWorkId);
  void work_normal_run_succ(int id);
  void work_init_run_succ(int iWorkId);
  void work_init_run_fail(int iWorkId);
  void work_normal_run_wrapper(void *para);
  void work_init_run_wrapper(void *para);
  DISPHANDLE CcbInnerWorkStopProduceWater();
  void work_fill_water_fail(int iWorkId);
  void work_fill_water_succ(void);
  DISPHANDLE CcbInnerWorkStartFillWater();
  void work_stop_water_succ();
  DISPHANDLE CcbInnerWorkStopFillWater();
  void work_N3_fail(int iWorkId);
  void work_N3_succ();
  DISPHANDLE CcbInnerWorkStartN3();
  void work_stop_N3_succ();
  DISPHANDLE CcbInnerWorkStopN3();
  void work_qtw_fail(int code,int iIndex,int iWorkId);
  void work_qtw_succ(int iIndex);
  void work_stop_qtw_succ(int iIndex);
  void work_start_qtw_helper(WORK_ITEM_STRU *pWorkItem);
  DISPHANDLE CcbInnerWorkStartQtw(int iIndex);
  DISPHANDLE CcbInnerWorkStopQtw(int iIndex);
  DISPHANDLE CcbInnerWorkRun();
  DISPHANDLE CcbInnerWorkIdle();
  void work_idle_succ();
  DISPHANDLE CcbInnerWorkStartTankFullLoop();
  DISPHANDLE CcbInnerWorkStopTankFullLoop();
  DISPHANDLE CcbInnerWorkInitRun();
  DISPHANDLE CcbInnerWorkSetupExeReport();
  DISPHANDLE CcbInnerWorkSetupFmReport();
  DISPHANDLE CcbInnerWorkSetupExeSwitch();
  void work_wash_fail_comm(int iWorkId);
  void work_rowash_fail(int iType,int iWorkId);
  void work_rowash_succ(int iType);
  int checkB3DuringROWash(int id, unsigned int uiDuration,unsigned int uiMask, unsigned int uiSwitchs);
  void work_idle_rowash_helper(WORK_ITEM_STRU *pWorkItem);
  void work_phwash_fail(int iType,int iWorkId);
  void work_phwash_succ(int iType);
  void work_idle_phwash_helper(WORK_ITEM_STRU *pWorkItem);
  DISPHANDLE CcbInnerWorkLpp();
  DISPHANDLE CcbInnerWorkStartTubeCir();
  DISPHANDLE CcbInnerWorkStopTubeCir();
  
  void CheckToStartAllocCir();
  void CheckToStopAllocCir();
  void TubeCirWithinSetTime();
  void TubeCirWithoutSetTime();
  void checkForTubeCir();
  DISPHANDLE DispCmdTubeCirProc(unsigned char *pucData, int iLength);
  DISPHANDLE DispCmdRunProc();
  void CcbCancelAllWork();
  DISPHANDLE CcbCancelWork(DISPHANDLE handle);
  void CcbRmvWork(work_proc proc);
  void CanCcbSndHoSpeedRspMsg(int iIndex,int iCode);
  int CanCcbAfDataHOSpeedReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHODecPressureReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHOSpeedRspMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  void CanCcbSndHOState(int iIndex , int iState);
  void CanCcbSndMainCtrlState(int iIndex , int iState);
  void CanCcbSaveQtwMsg(int iTrxIndex, void *pMsg);
  void CanCcbFillQtwRsp(APP_PACKET_HO_QTW_RSP_STRU *pQtwRsp,int iAct,int iCode);
  void CanCcbSndQtwRspMsg(int iIndex,int iCode);
  int CanCcbAfDataHOQtwReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHOQtwRspMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHODecPressureRspMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  void CanCcbIapProc(int iTrxIndex,void *msg);
  int CanCcbAfDataHOExtSysStartReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHOExtSterlizeReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHOExtRtiReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHOExtConfigSetReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHOExtSterlizeQryMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHandleOpsMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataRfSearchRsp(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataRfReadRsp(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataRfWriteRsp(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataRfOpsMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataStateIndMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataExeTocRspMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataExeOpsMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHOExtConfigQryReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHOExtCmInfoQryMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHOExtCmStateQryMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHOExtCmResetMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHOExtQryUsageMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHOExtQryAlarmMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHOExtMachParamQryMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHOExtSystemInfoQryMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHOExportHistoryReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  void CanCcbSndAccessoryInstallStartMsg(int iAccType,const QString& strCatNo,const QString& strLotNo,int id=0);
  void CanCcbSndAccessoryInstallFinishMsg(int iAccType,int id = 0);
  int CanCcbAfDataHOExtInstallStartRspMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHOExtInstallQryMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHOExtCmDetailInfoQryMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  void CanCcbHandlerMgrProc(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  void CanCcbRestore();
  void work_start_Leak_fail(int iKeyId);
  void work_start_Leak_succ();
  void work_stop_Leak_succ();
  int CanCcbAfDataClientRpt4ExeBoard(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataClientRpt4RFReader(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  DISPHANDLE CcbInnerWorkStartLeakWok();
  DISPHANDLE CcbInnerWorkStopLeakWork();
  void work_stop_key_succ();
  
  void work_start_key_fail(int iKeyId);
  void CanCcbLeakProcess();
  void work_start_key_succ();
  DISPHANDLE CcbInnerWorkStartKeyWok(int iKey);
  void CcbAbortWork(WORK_ITEM_STRU *pWorkItem);
  DISPHANDLE CcbInnerWorkStopKeyWork(int iKey);
  void CanCcbDinProcess();
  void CcbStopQtw();
  int CanCcbAfDataClientRpt4FlowMeter(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataClientRptMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHOCirReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  int CanCcbAfDataHOCirRspMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg);
  void CanCcbSaveSpeedMsg(int iTrxIndex,void *pMsg,int iSpeed);
  WORK_ITEM_STRU *CcbAllocWorkItem();
  void work_start_cir_helper(WORK_ITEM_STRU *pWorkItem);

  void emitNotify(unsigned char *pucData,int iLength);
  void emitIapNotify(IAP_NOTIFY_STRU *pIapNotify);
  void emitCmReset(uint32_t ulMap);

signals:
   void dispIndication(unsigned char *pucData,int iLength);
   void iapIndication(IAP_NOTIFY_STRU *pIapNotify);
   void cmresetindication(unsigned int ulMap);
   
      // declare as public member to facilitate mutual access
public:
      /* 2020/08/03 moved from DWaterQualityPage */
      struct Show_History_Info m_historyInfo[MSG_NUM];
      
      // 2020/08/07 moved from mainwindow
      int         m_aiAlarmRcdMask[2][DISP_ALARM_PART_NUM]; /* array 0 for fired alarm, array 1 for restored alarm */
      
      MACHINE_ALARM_SETTING_STRU  m_aMas[MACHINE_NUM];
      MACHINE_NOTIFY_SETTING_STRU m_cMas[MACHINE_NUM];

      ConsumableInstaller *m_pConsumableInstaller[APP_RFID_SUB_TYPE_NUM];

public:

  static CCB *instance;
  TASK_HANDLE m_taskHdl;
  TASK_HANDLE getTaskHdl(){return m_taskHdl;}    
  void start();
  void DispGetRPumpSwitchState(int iIndex ,int *pState);
  void DispSetHanlerConfig(DB_HANDLER_STRU *pHdl);
  void DispGetRPumpSpeed(int iIndex ,int *pValue);
  int DispReadRPumpSpeed();
  int DispConvertRPumpSpeed2Scale(int speed);
  int DispGetUpCirFlag();
  int DispGetTankCirFlag();
  int DispGetTocCirFlag();
  int DispGetUpQtwFlag();
  int DispGetEdiQtwFlag();
  int DispGetROWashFlag();
  int DispGetRoWashPauseFlag();
  int DispGetWashFlags();
  int DispGetREJ(float *pfValue);
  int DispGetSwitchState(int ulMask);
  int DispGetDinState();
  APP_ECO_VALUE_STRU& DispGetEco(int iIdx);
  int DispGetSingleSwitchState(int iChlNo);
  void DispGetRPumpRti(int iChlNo,int *pmV,int *pmA);
  void DispGetOtherCurrent(int iChlNo,int *pmA);
  void DispSndHoSpeed(int iType,int iSpeed);
  void DispSndHoQtwVolumn(int iType,float fValue);
  void DispSndHoAlarm(int iIndex,int iCode);
  void DispSndHoPpbAndTankLevel(int iIndex,int iMask,int iLevel,float fPpb);
  void DispSndHoSystemTest(int iIndex,int iAction,int iOpreatType);
  void DispSndHoEco(int iMask);
  int CcbReadRfid(int iIndex,int iTime,int offset,int len);
  
  int CcbGetRfidCont(int iIndex,int offset,int len,unsigned char *pucData);
  int CcbWriteRfid(int iIndex,int iTime,int offset,int len,unsigned char *pucData);
  int CcbScanRfid(int iIndex,int iTime);
  int CcbQueryDevice(int iTime);
  int CcbSetRPump(int id,int iChl,unsigned int ulValue);
  int DispSetRPump(int iChl,unsigned int ulValue);
  void DispSetTocState(int iStage);
  int DispGetH7State(uint16_t &usState);
  int DispGetH8State(uint16_t &usState);
  int CcbGetOtherKeyState(int iSelfKey);
  int DispGetHandsetIndexByType(int iType);
  
  struct Show_History_Info *DispGetHistInfo() {return m_historyInfo;}
  int CcbGetTwFlag();
  int CcbGetTwPendingFlag();
  int DispGetInitRunFlag();
  int DispGetTubeCirFlag();
  int DispGetPwFlag();
  int DispGetFwFlag();
  int DispGetTankFullLoopFlag();
  int DispGetRunningStateFlag();
  DISPHANDLE DispCmdIdleProc();
  DISPHANDLE DispCmdWashProc(unsigned char *pucData, int iLength);
  DISPHANDLE DispCmdResetProc();
  DISPHANDLE DispCmdLeakResetProc();
  DISPHANDLE DispCmdCancelWork(unsigned char *pucData, int iLength);
  DISPHANDLE DispCmdHaltProc();
  DISPHANDLE DispCmdEngProc(unsigned char *pucData, int iLength);
  DISPHANDLE DispCmdTw(unsigned char *pucData, int iLength);
  DISPHANDLE DispCmdCir(unsigned char *pucData, int iLength);
  DISPHANDLE DispCmdEngCmdProc(unsigned char *pucData, int iLength);
  DISPHANDLE DispCmdSwitchReport(unsigned char *pucData, int iLength);
  DISPHANDLE DispCmdEntry(int iCmdId,unsigned char *pucData, int iLength);
  int DispIapEntry(IAP_CAN_CMD_STRU *pIapCmd);
  int DispAfEntry(IAP_CAN_CMD_STRU *pIapCmd);
  int DispAfRfEntry(IAP_CAN_CMD_STRU *pIapCmd);
  int DispGetWorkState();
  int DispGetWorkState4Pw();
  int DispGetSubWorkState4Pw();
  void DispSetSubWorkState4Pw(int SubWorkState4Pw);
  char *DispGetAscInfo(int idx);
  int DispConvertVoltage2RPumpSpeed(float fv);
  int DispConvertRPumpSpeed2Voltage(int speed);
  void DispC1Regulator(int iC1Regulator);
  int CcbC2Regulator(int id,float fv,int on);
  
  int CcbGetTwFlagByDevType(int type);
  int CcbGetHandlerStatus(int iDevId);
  
  int CcbGetHandlerType(int iDevId);
  
  int CcbGetHandlerTwFlag(int iDevId);
  int CcbGetHandlerId2Index(int iDevId);
  float CcbConvert2Pm3Data(unsigned int ulValue);

  float CcbConvert2Pm3SP(unsigned int ulValue);
  float CcbConvert2Pm2Data(unsigned int ulValue);
  float CcbConvert2Pm2SP(unsigned int ulValue);
  float CcbConvert2Pm1Data(unsigned int ulValue);
  unsigned int CcbConvert2Fm1Data(unsigned int ulValue);
  unsigned int CcbConvert2Fm2Data(unsigned int ulValue);
  unsigned int CcbConvert2Fm3Data(unsigned int ulValue);
  unsigned int CcbConvert2Fm4Data(unsigned int ulValue);
  unsigned int CcbConvert2RectAndEdiData(unsigned int ulValue);
  unsigned int CcbConvert2GPumpData(unsigned int ulValue);
  unsigned int CcbGetRPumpVData(int iChl);
  unsigned int CcbConvert2RPumpIData(unsigned int ulValue);
  float CcbGetSp1();
  float CcbGetSp2();
  float CcbGetSp3();
  float CcbGetSp4();
  float CcbGetSp5();
  float CcbGetSp6();
  float CcbGetSp7();
  float CcbGetSp8();
  float CcbGetSp9();
  float CcbGetSp10();
  float CcbGetSp11();
  float CcbGetSp12();
  float CcbGetSp13();
  float CcbGetSp14();
  float CcbGetSp15();
  float CcbGetSp16();
  float CcbGetSp17();
  float CcbGetSp30();
  float CcbGetSp31();
  float CcbGetSp32();
  float CcbGetSp33();
  float CcbGetB2Full();
  void CanPrepare4Pm2Full();
  float CcbCalcREJ();
  QString accessoryName(int iType);

  void CcbSetDataTime(uint32_t ulTimeStamp);
  
  unsigned int getKeyState();
  unsigned char getLeakState();
  int Ex_FactoryTest(int select);
  void Ex_DispDecPressure();
  
  int getWaterTankCfgMask();
  void getInitInstallMask(int iType0Mask,int iType1Mask);
  uint8_t CcbGetPm2Percent(float fValue);
  void CcbSetPm2RefillThd(uint8_t RefillThd,uint8_t StopThd);
  
  static CCB *getInstance();
  static void MainMsgProc(void *para, SAT_MSG_HEAD *pMsg);
  static void Main_timer_handler(void *arg);
  static  void CanMsgMsgProc(void *para,SAT_MSG_HEAD *pMsg);
  static  void CcbDelayCallBack(void *para);
  static void CcbDefaultCancelWork(void *param);
  static void CcbDefaultModbusCallBack(CCB *pCcb,int code,void *param);
  static void work_stop_pw(void *para);
  static void work_start_cir(void *para);
  static void work_start_toc_cir(void *para);
  static void work_stop_cir(void *para);
  static void work_start_speed_regulation(void *para);
  static void work_init_run(void *para);
  static void work_normal_run(void *para);
  static void WorkCommEntry(void *para);
  static void work_start_fill_water(void *para);
  static void work_stop_fill_water(void *para);
  static void work_start_N3(void *para);
  static void work_stop_N3(void *para);
  static void work_start_qtw(void *para);
  static void work_stop_qtw(void *para);
  static void work_idle(void *para);
  static void work_tank_start_full_loop(void *para);
  static void work_tank_stop_full_loop(void *para);
  static void work_rpt_setup_exe(void *para);
  static void work_rpt_setup_fm(void *para);
  static void work_switch_setup_exe(void *para);
  static void work_idle_rowash(void *para);
  static void work_idle_syswash(void *para);
  static void work_idle_phwash(void *para);
  static void work_start_lpp(void *para);
  static void work_start_tube_cir(void *para);
  static void work_stop_tube_cir(void *para);
  static void CcbCancelKeyWork(void *param);
  static void work_start_Leak(void *para);
  static void work_stop_Leak(void *para);
  static void work_start_key(void *para);
  static void work_stop_key(void *para);
  
};

extern TASK_HANDLE TASK_HANDLE_MAIN ;
extern TASK_HANDLE TASK_HANDLE_CANITF;
extern TASK_HANDLE TASK_HANDLE_MOCAN ;

extern unsigned int gulSecond;

#endif

