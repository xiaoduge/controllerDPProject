#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "datatype.h"
#include "cminterface.h"
#include "notify.h"

#ifdef __cplusplus
 extern "C"
 {
#endif
 /* special address description flags for the CAN_ID */
#define CAN_EFF_FLAG 0x80000000U /* EFF/SFF is set in the MSB */
#define CAN_RTR_FLAG 0x40000000U /* remote transmission request */
#define CAN_ERR_FLAG 0x20000000U /* error frame */
 
 /* valid bits in CAN ID for frame formats */
#define CAN_SFF_MASK 0x000007FFU /* standard frame format (SFF) */
#define CAN_EFF_MASK 0x1FFFFFFFU /* extended frame format (EFF) */
#define CAN_ERR_MASK 0x1FFFFFFFU /* omit EFF, RTR, ERR flags */

#define MAX_CAN_MESSAGE_LEN 256

#define CAN_ADDRESS_MASK (0X3FF)

#define CAN_SRC_ADDRESS_POS (10) // BIT0~BIT9 in indentifer

#define CAN_SRC_ADDRESS(indenfier)((indenfier >> CAN_SRC_ADDRESS_POS) & CAN_ADDRESS_MASK)

#define CAN_DST_ADDRESS_POS (0) // BIT0~BIT9 in indentifer

#define CAN_DST_ADDRESS(indenfier)((indenfier >> CAN_DST_ADDRESS_POS) & CAN_ADDRESS_MASK)

#define CAN_BROAD_CAST_ADDRESS (0X3FF)

#define CAN_ADDRESS(indenfier)((indenfier) & CAN_ADDRESS_MASK)

#define CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,usSrcCanAdr,usDstCanAdr)(ulIdentifier=(usSrcCanAdr<<CAN_SRC_ADDRESS_POS)|(usDstCanAdr << CAN_DST_ADDRESS_POS)|CAN_EFF_FLAG)

#define CAN_INVALID_ADDRESS (0XFFFF)

#define INVALID_CAN_ADDRESS(usAddress) ((usAddress & CAN_BROAD_CAST_ADDRESS) == CAN_BROAD_CAST_ADDRESS)


#define B1_PRESSURE (16)   /* Unit: bar = 0.1Mpa */
#define B2_LEVEL    (2.0)  /* Unit: Millimeter */
#define B3_LEVEL    (2.0)  /* Unit: Millimeter */

#define B2_FULL  (100)    /* Unit: Percent */

#define B3_FULL  (100)    /* Unit: Percent */


#define PERIOD_EVENT_LENGTH (100) // MS

#define SECOND_EVENT_LENGTH (1000) // MS

#define UV_PFM_PEROID ((60*60)) // SECOND IN HOUR

#define TOMLPERMIN  (60 * (1000/PERIOD_EVENT_LENGTH))   /* convert to millilitre per Minute */
#define TOMLPERHOUR (60 * TOMLPERMIN)                   /* convert to millilitre per Hour */

#define FM_CALC_PERIOD          (3000)
#define FM_UPDATE_PERIOD        (30000)
#define FM_ALARM_CHECK_PERIOD   (10000)
#define FM_SIMULATION_PERIOD    (60000)

typedef enum
{
    DB_TBL_BEST_WATER_QUALITY = 0,
    DB_TBL_ALARM,
    DB_TBL_USER,
    DB_TBL_TAKE_WATER,
    DB_TBL_PRODUCE_WATER,
    DB_TBL_LOG,
    DB_TBL_NUM,
}DB_TABLE_ENUM;



typedef void * DISPHANDLE;


#define DISP_INVALID_HANDLE (NULL)
#define DISP_SPECIAL_HANDLE ((void *)1)
#define DISP_ALREADY_HANDLE ((void *)2)

typedef enum
{
    DISP_WORK_STATE_IDLE = 0,
    DISP_WORK_STATE_RUN,
    DISP_WORK_STATE_LPP,     /* Low pressure protect state */
    DISP_WORK_STATE_KP ,     /* Key Protect State */

    /* extra for prepare*/
    DISP_WORK_STATE_PREPARE = 0X80,
}DISP_WORK_STATE_ENUM;

typedef enum
{
   /* works availabe under IDLE State */
   
   DISP_WORK_SUB_IDLE    = 0,
   DISP_WORK_SUB_WASH, 
   DISP_WORK_SUB_IDLE_DEPRESSURE,

   /* works availabe under RUN State  */
   DISP_WORK_SUB_RUN_INIT,
   DISP_WORK_SUB_RUN_QTW,
   DISP_WORK_SUB_RUN_QTWING,
   DISP_WORK_SUB_RUN_CIR,
   DISP_WORK_SUB_RUN_CIRING,
   DISP_WORK_SUB_RUN_DEC,

   /* works available under */
   
   
}DISP_WORK_SUB_STATE_ENUM;

typedef enum
{
   /* works availabe under IDLE State */

   
   DISP_CMD_IDLE  = 0,  /* return to idle state */

   DISP_CMD_RUN      ,

   DISP_CMD_WASH  ,

   DISP_CMD_RESET,

   DISP_CMD_ENG_MODE,   /* 4 */

   DISP_CMD_EXIT_ENG_MODE,

   DISP_CMD_ENG_CMD,

   DISP_CMD_LEAK_RESET,

   DISP_CMD_TUBE_CIR,  /* 8 */

   DISP_CMD_TW,

   DISP_CMD_CIR,

   DISP_CMD_SWITCH_REPORT,

   DISP_CMD_CANCEL_WORK = 0x80,

   DISP_CMD_HALT,
   
   DISP_CMD_OPENALLVALVES,
}DISP_CMD_ENUM;


typedef enum
{
    DISP_ACT_TYPE_SWITCH = (1 << 0),
    DISP_ACT_TYPE_RPT = (1 << 1),

}DISP_ACT_TYPE_ENUM;


typedef union
{
   unsigned int       ulV;
   float              fV;
   APP_ECO_VALUE_STRU eV;
}DISP_OBJ_VALUE;

typedef struct
{
    int emDispObjType; /* refer APP_OBJ_TYPE_ENUM */
    int iDispObjId;
    int iState;        /* 0: normal , 1: breakdown */
    int iActive;       /* refer DISP_ACT_TYPE_ENUM, any combination of DISP_ACT_TYPE_ENUM */
    unsigned int ulSec;
    unsigned int ulDurSec;
    int iVChoice;
    DISP_OBJ_VALUE Value;
}DISP_OBJ_STRU;

/* default setting for MM parameters */
#define MM_DEFALUT_SP1 0.5//          0~1.6Mpa    B1进水压力下限
#define MM_DEFALUT_SP2 92//          RO截留率下限,通过计算公式rejection=(I1b-I2)/I1b*100%
#define MM_DEFALUT_SP3 50//          0~100?s/cm  RO产水电导率下限（I2测得）
#define MM_DEFALUT_SP4 1.0//          0~18.2M .cm EDI产水电阻率下限（I3测得）
#define MM_DEFALUT_SP5 80 //          0~2.0m  设备恢复产水液位（B2）
#define MM_DEFALUT_SP6 10 //          0~2.0m  水箱低液位报警线（B2）
#define MM_DEFALUT_SP7 16//          0~18.2M .cm UP产水电阻率下限（I5）
#define MM_DEFALUT_SP8 80//          0~2.0m  源水箱补水液位（B3）
#define MM_DEFALUT_SP9 10//          0~2.0m  原水箱低位系统保护设定点（B3）
#define MM_DEFALUT_SP10 1.0//        0~18.2M .cm 水箱水水质下限（I4）
#define MM_DEFALUT_SP11 2.0//        0~18.2M .cm 水箱水水质上限（I4）
#define MM_DEFALUT_SP12 1.0//        0~18.2M .cm 纯水取水水质下限（I4）
#define MM_DEFALUT_SP13 2000//        0~2000?s/cm 自来水电导率上限（I1a）
#define MM_DEFALUT_SP14 100//        RO产水流速 上限100.0L/min     
#define MM_DEFALUT_SP15 20//        RO产水流速 下限20.0L/min   
#define MM_DEFALUT_SP16 20//        RO弃水流速 下限20.0L/min     
#define MM_DEFALUT_SP17 1//        管路水质   下限 1MΩ.cm     
#define MM_DEFALUT_SP18 45//        进水温度 上限45℃      
#define MM_DEFALUT_SP19 5//        进水温度 下限5℃     
#define MM_DEFALUT_SP20 45//        RO产水温度 上限45℃ 下限5℃    
#define MM_DEFALUT_SP21 5//           
#define MM_DEFALUT_SP22 45//        EDI产水温度 上限45℃ 下限5℃   
#define MM_DEFALUT_SP23 5//           
#define MM_DEFALUT_SP24 45//        UP产水温度 上限45℃ 下限5℃   
#define MM_DEFALUT_SP25 5//           
#define MM_DEFALUT_SP26 45//        UP产水温度 上限45℃ 下限5℃   
#define MM_DEFALUT_SP27 5//           
#define MM_DEFALUT_SP28 45//        TOC传感器温度 上限45℃ 下限5℃  
#define MM_DEFALUT_SP29 5//           
#define MM_DEFALUT_SP30 10.0//        TOC进水水质下限15.0MΩ.cm   
#define MM_DEFALUT_SP31 1.0//        HP循环(水箱循环)水质下限15.0MΩ.cm   
#define MM_DEFALUT_SP32 0.0//        HP产水水质下限15.0MΩ.cm   
#define MM_DEFALUT_SP33 8.0//       bar 


typedef struct
{
    float SP[MACHINE_PARAM_SP_NUM];
}DISP_MACHINE_PARAM_STRU;

typedef enum
{
    TIME_PARAM_InitRunT1 = 0, //  系统冲洗时间 
    
    TIME_PARAM_NormRunT1,
    TIME_PARAM_NormRunT2,
    TIME_PARAM_NormRunT3,
    TIME_PARAM_NormRunT4,
    TIME_PARAM_NormRunT5,
    
    TIME_PARAM_N3Period,   // 水箱紫外照射周期
    TIME_PARAM_N3Duration, // 水箱紫外照射时间 
    
    TIME_PARAM_TOCT1,
    TIME_PARAM_TOCT2,
    TIME_PARAM_TOCT3,
    
    TIME_PARAM_IdleCirPeriod,
    TIME_PARAM_InterCirDuration,

    TIME_PARAM_LPP,
   
    TIME_PARAM_NUM,

}TIME_PARAM_ENUM;
typedef struct
{
    unsigned int aulTime[TIME_PARAM_NUM];
}DISP_TIME_PARAM_STRU;



typedef struct
{

    unsigned int   aulFlag[2]; // refer : DISP_ALARM_PART0_ENUM & DISP_ALARM_PART1_ENUM

    /* deprecated : before 2017/11/00*/
    unsigned int   ulFlags;
    float          fAlarms[DISP_ALARM_PARAMS];
    
}DISP_ALARM_SETTING_STRU;



typedef struct
{
    unsigned int ulFlags; // refer : DISP_SUB_MODULE_ENUM
}DISP_SUB_MODULE_SETTING_STRU;


typedef enum
{
    DISP_FM_FM1 = 0,
    DISP_FM_FM2 ,
    DISP_FM_FM3 ,
    DISP_FM_FM4,
    DISP_FM_NUM,
}DISP_FM_SETTING_ENUM;

typedef struct
{
    unsigned int aulCfg[DISP_FM_NUM]; // Pulse Number / Litre
}DISP_FM_SETTING_STRU;

typedef enum
{
    DISP_PM_PM1 = 0,
    DISP_PM_PM2 ,
    DISP_PM_PM3 ,
    DISP_PM_NUM,
}DISP_PM_SETTING_ENUM;

typedef enum
{
    DISP_WATER_BARREL_TYPE_010L = 0,
    DISP_WATER_BARREL_TYPE_030L,
    DISP_WATER_BARREL_TYPE_060L,    
    DISP_WATER_BARREL_TYPE_100L,    
    DISP_WATER_BARREL_TYPE_200L,    
    DISP_WATER_BARREL_TYPE_350L,    
    DISP_WATER_BARREL_TYPE_UDF,    
    DISP_WATER_BARREL_TYPE_NO,  
    DISP_WATER_BARREL_TYPE_NUM,  
}DISP_WATER_BARREL_TYPE_ENUM;

typedef struct
{
    int   aiBuckType[DISP_PM_NUM];
    float afCap[DISP_PM_NUM]; // litre for bucket, invalid for barometer
    float afDepth[DISP_PM_NUM]; // mm
}DISP_PM_SETTING_STRU;

typedef struct  {
       int iType; // refer APP_ACCESSORY_TYPE_ENUM
       int iId;   // refer DISP_CM_NAME_ENUM
       int iRfid; // refer APP_RFID_SUB_TYPE_ENUM
}DISP_ACCESSORY_STRU;


#define DISP_CAL_I_NUM (5)
#define DISP_CAL_F_NUM (4)
#define DISP_CAL_P_NUM (3)

typedef enum
{
    DISP_CAL_I1_K = 0,
    DISP_CAL_I1_C ,
    DISP_CAL_I1_T ,
    DISP_CAL_I2_K ,
    DISP_CAL_I2_C ,
    DISP_CAL_I2_T ,
    DISP_CAL_I3_K ,
    DISP_CAL_I3_C ,
    DISP_CAL_I3_T ,
    DISP_CAL_I4_K ,
    DISP_CAL_I4_C ,
    DISP_CAL_I4_T ,
    DISP_CAL_I5_K ,
    DISP_CAL_I5_C ,
    DISP_CAL_I5_T ,
    DISP_CAL_FM_1,
    DISP_CAL_FM_2,
    DISP_CAL_FM_3,
    DISP_CAL_FM_4,
    DISP_CAL_PM_1,
    DISP_CAL_PM_2,
    DISP_CAL_PM_3,
    DISP_CAL_NUM,
}DISP_CAL_SETTING_ENUM;

typedef enum
{
    DISP_PC_COFF_SOURCE_WATER_CONDUCT = 0,
    DISP_PC_COFF_SOURCE_WATER_TEMP ,
    DISP_PC_COFF_RO_WATER_CONDUCT ,
    DISP_PC_COFF_RO_WATER_TEMP ,
    DISP_PC_COFF_EDI_WATER_CONDUCT ,
    DISP_PC_COFF_EDI_WATER_TEMP ,
    DISP_PC_COFF_UP_WATER_CONDUCT ,
    DISP_PC_COFF_UP_WATER_TEMP ,
    DISP_PC_COFF_TOC_WATER_CONDUCT ,
    DISP_PC_COFF_TOC_WATER_TEMP ,
    DISP_PC_COFF_S1 ,
    DISP_PC_COFF_S2 ,
    DISP_PC_COFF_S3 ,
    DISP_PC_COFF_S4 ,
    DISP_PC_COFF_PW_TANK_LEVEL ,
    DISP_PC_COFF_SW_TANK_LEVEL ,
    DISP_PC_COFF_SYS_PRESSURE ,
    DISP_PC_COFF_NUM,
}DISP_PARAM_CALIBRATION_ENUM;

typedef struct
{
   float fk;
   float fc;
   float fv;
   
}DISP_PARAM_CALI_ITEM_STRU;


typedef struct
{
    DISP_PARAM_CALI_ITEM_STRU pc[DISP_PC_COFF_NUM];

}DISP_PARAM_CALI_STRU;


typedef struct
{
    float afCfg[DISP_CAL_NUM];
}DISP_CAL_SETTING_STRU;

typedef char CATNO[APP_CAT_LENGTH+1];

typedef char LOTNO[APP_LOT_LENGTH+1];

typedef struct
{
    unsigned int aulCms[DISP_CM_NUM];  /* service life / install data */  

}DISP_CONSUME_MATERIAL_STRU;

typedef char SN[APP_SN_LENGTH+1];

typedef struct
{
    CATNO aCn[DISP_CM_NAME_NUM];
    LOTNO aLn[DISP_CM_NAME_NUM];

}DISP_CONSUME_MATERIAL_SN_STRU;

typedef struct
{
    CATNO aCn[DISP_CM_NAME_NUM];
    LOTNO aLn[DISP_CM_NAME_NUM];

}DISP_MACHINERY_SN_STRU;


typedef struct
{
    unsigned int aulCumulatedData[DISP_CM_NUM]; // in ml

}DISP_CM_CUMULATED_STRU;

typedef struct
{
    CATNO catNo;
    LOTNO lotNo;
}DISP_CM_ID_INFO_STRU;


typedef struct
{
    DISP_CM_ID_INFO_STRU idInfo[DISP_CM_NAME_NUM];
}DISP_CM_ID_INFOS_STRU;


typedef struct
{
   unsigned int bit1PendingInfoSave;

   unsigned int ulUsageState;

   DISP_CM_CUMULATED_STRU      cmInfo; // temporary info
   DISP_CONSUME_MATERIAL_STRU  info;
   DISP_CM_ID_INFOS_STRU       idInfos;
   
}DISP_CM_USAGE_STRU;



typedef enum
{
   DISPLAY_SOUND_KEY_NOTIFY = 0,
   //DISPLAY_SOUND_SERVICE_NOTIFY,
   DISPLAY_SOUND_ALARM_NOTIFYM,
   DISPLAY_SOUND_NUM,
}DISPLAY_SOUND_ENUM;

typedef enum
{
   DISPLAY_NETWORK_CAN = 0,
   DISPLAY_NETWORK_ZIGBEE,
   DISPLAY_NETWORK_WIFI,
   DISPLAY_NETWORK_NUM,
}DISPLAY_NETWORK_ENUM;

typedef enum
{
    DISPLAY_UNIT_CONDUCTIVITY = 0,
    DISPLAY_UNIT_TEMPERATURE,
    DISPLAY_UNIT_PRESSURE,
    DISPLAY_UNIT_LIQUID_LEVEL,
    DISPLAY_UNIT_FLOW_VELOCITY,
    DISPLAY_UNIT_NUM,
}DISPLAY_UNIT_ENUM;


typedef enum
{
   DISPLAY_ALLOC_FLAG_SUNDAY = 0,
   DISPLAY_ALLOC_FLAG_MONDAY,
   DISPLAY_ALLOC_FLAG_TUESDAY,
   DISPLAY_ALLOC_FLAG_WEDNESDAY,
   DISPLAY_ALLOC_FLAG_THURSDAY,
   DISPLAY_ALLOC_FLAG_FRIDAY,
   DISPLAY_ALLOC_FLAG_SATURDY,
   DISPLAY_ALLOC_FLAG_SWITCHON,
   DISPLAY_ALLOC_FLAG_NUM,
}DISPLAY_ALLOC_ENUM;


typedef struct
{
    int           ulAllocMask;
    int           iTubeCirCycle;
    int           iTubeCirDuration;
    int           aiTubeCirTimeInfo[2]; /* index 0: start hour, index 1: end hour*/
    int           RPumpSetting[2];      /* regulated pump 2/3 voltage setting */
    int           iLan;                 /* Language */
    int           iBrightness;          /* 0~255 */
    int           iEnerySave;           /* 0: Dont save ,other : save */
    int           iUint4Conductivity;
    int           iUint4Temperature;
    int           iUint4Pressure;
    int           iUint4LiquidLevel;
    int           iUint4FlowVelocity;

    int           iSoundMask;
    int           iNetworkMask;
    int           ulMisFlags;
    int           ulExMisFlags;

    int           iTankUvOnTime;
    int           iAutoLogoutTime;
    int           iPowerOnFlushTime;
    
}DISP_MISC_SETTING_STRU;

typedef struct
{
    int lstTime;
    int period;
}DISP_CLEAN_ITEM;


typedef struct
{
    DISP_CLEAN_ITEM aCleans[APP_CLEAN_NUM];
    
}DISP_CLEAN_SETTING_STRU;

typedef struct
{
    unsigned int aulMask[DISP_ALARM_PART_NUM];
}MACHINE_ALARM_SETTING_STRU;

typedef struct
{
    unsigned int aulMask[2]; /*DISP_CM_NAME_NUM*/
}MACHINE_NOTIFY_SETTING_STRU;


typedef struct {
    int iMachineType;            /* refer MACHINE_TYPE_ENUM */
    DISP_MACHINE_PARAM_STRU      MMParam;
    DISP_TIME_PARAM_STRU         TMParam;
    DISP_ALARM_SETTING_STRU      AlarmSetting;
    DISP_SUB_MODULE_SETTING_STRU SubModSetting;
    DISP_CONSUME_MATERIAL_STRU   CMParam;
    DISP_FM_SETTING_STRU         FmParam;
    DISP_PM_SETTING_STRU         PmParam;
    DISP_MISC_SETTING_STRU       MiscParam;
    DISP_CLEAN_SETTING_STRU      CleanParam;
    DISP_CONSUME_MATERIAL_SN_STRU cmSn;
    DISP_MACHINERY_SN_STRU        macSn;
    DISP_PARAM_CALI_STRU          Caliparam;
}DISP_GLOBAL_PARAM_STRU;

typedef struct
{
    unsigned int ulCanId;
             int iCanChl;
             int iMsgLen;
    unsigned char ucCmd;
             int iTrxIndex; /* 2018/02/22 add */
             char data[1];
}IAP_NOTIFY_STRU;

#define IAP_NOTIFY_SIZE (sizeof(IAP_NOTIFY_STRU)-1)

typedef struct
{
    unsigned int ulCanId;
             int iCanChl;
    unsigned char ucCmd;
             int iPayLoadLen;
             char data[1];
}IAP_CAN_CMD_STRU;


typedef enum
{
    ENG_CMD_TYPE_SWITCH = 0,
    ENG_CMD_TYPE_IB        ,
    ENG_CMD_TYPE_FM        ,
    ENG_CMD_TYPE_RPUMP     ,
}ENG_CMD_TYPE_ENUM;


typedef struct
{
   int iType;
   int iDevId; /* default Dev ID */
   unsigned int iVolume; /* ml */
   
}DISP_CMD_TW_STRU;

typedef struct
{
   int iType;
   int iState;
}DISP_CMD_WASH_STRU;


typedef struct
{
   int iType;
}DISP_CMD_CIR_STRU;

typedef struct
{
    float fQuality;
    float fTemperature;

}ECO_INFO_STRU;

typedef struct
{
    float fShowInfo;
}SHOW_INFO_STRU;

typedef struct
{
   int iStart;    /* 0 : for stop ,otherwise start */
}DISP_CMD_TC_STRU; /*Tube circulation */

typedef struct
{
   unsigned char iRptFlag;
}DISP_CMD_SWITCH_STATE_REPORT_STRU;


typedef enum
{
    USER_TBL_FILED_ID = 0,
    USER_TBL_FILED_NAME ,
    USER_TBL_FILED_PAS,
    USER_TBL_FILED_PERM,

}USER_TBL_FILED_ENUM;


typedef enum
{
    HANDLER_TBL_FILED_ID = 0,
    HANDLER_TBL_FILED_NAME ,
    HANDLER_TBL_FILED_ADDRESS,
    HANDLER_TBL_FILED_TYPE,
    HANDLER_TBL_FILED_DEF,
}HANDLER_TBL_FILED_ENUM;

typedef struct {
    SN name; /* serial No */
    int address;
    int type;
    int def;
}DB_HANDLER_STRU;

typedef enum
{
    RFID_TBL_FILED_ID = 0,
    RFID_TBL_FILED_NAME ,
    RFID_TBL_FILED_ADDRESS,
    RFID_TBL_FILED_TYPE,
}RFID_TBL_FILED_ENUM;

typedef struct {
    SN name; /* serial No */
    int address;
    int type;
}DB_RFID_STRU;

enum
{
    RF_DATA_LAYOUT_HEAD_SIZE = 0,
    RF_DATA_LAYOUT_PROPERTY_SIZE,
    RF_DATA_LAYOUT_DEVICE,
    RF_DATA_LAYOUT_TYPE,
    RF_DATA_LAYOUT_TRADE_MARK,
    RF_DATA_LAYOUT_TRADE_NAME,
    RF_DATA_LAYOUT_CATALOGUE_NUM,
    RF_DATA_LAYOUT_UNKNOW,
    RF_DATA_LAYOUT_LOT_NUMBER,
    RF_DATA_LAYOUT_UNKNOW_DATE,
    RF_DATA_LAYOUT_INSTALL_DATE,
    RF_DATA_LAYOUT_UNKNOW_DATA,
    RF_DATA_LAYOUT_NUM

};

typedef struct
{
   int offset;
   int size;
   int write;
}RF_DATA_LAYOUT_ITEM;


typedef struct
{
    RF_DATA_LAYOUT_ITEM aItem[RF_DATA_LAYOUT_NUM];
    
}RF_DATA_LAYOUT_ITEMS;

typedef struct
{
    unsigned char data[256];
    
}RF_DATA_LAYOUT_STRU;

// moved from DWaterQualityPage
struct Show_History_Info
{
    float value1;
    float value2;
};

enum MSG_ENUM
{
    Tap_Cond,
    RO_Feed_Cond,
    RO_Product,
    RO_Rejection,
    EDI_Product,

    RO_Feed_Pressure,
    RO_Pressure,

    RO_Product_Rate,
    RO_Reject_Rate,
    RO_Feed_Rate,
    Tap_Rate,
    EDI_Product_Rate,
    EDI_Reject_Rate,

    Source_Tank_Level,
    Pure_Tank_Level,
    HP_Resis,
    HP_Disp_Rate,
    UP_IN,
    UP_Resis,
    TOC_Value,
    UP_Disp_Rate,

    MSG_NUM
};

enum UNIT_MSG
{
    UNIT_OMG,
    UNIT_USCM,
    UNIT_CELSIUS,//Celsius
    UNIT_F,
    UNIT_L_H,
    UNIT_L_MIN,
    UNIT_G_H,
    UNIT_G_MIN,
    UNIT_BAR,
    UNIT_MPA,
    UNIT_PSI,
    UNIT_PPB,
    UNIT_PERCENTAGE,//percentage
    UNIT_VOLUME,    //volume
    UNIT_MSG_NUM
};

typedef struct
{
    int dhcp;
    char ip[32];
    char mask[32];
    char gateway[32];
}NIF;

#define UNUSED(para) ((void *)(para))

#define DISP_MAKE_WORK_STATE(Main,Sub) (((Main)<<8)|(Sub))

#define DISP_GET_WORK_MAIN_STATE(State)(((State)>>8)&0xff)

#define DISP_GET_WORK_SUB_STATE(State) (((State)>>0)&0xff)

extern DISP_GLOBAL_PARAM_STRU gGlobalParam;
extern DISP_CM_USAGE_STRU     gCMUsage;

#define DISP_DAYININSECOND   (24*60*60)
#define INVALID_CONFIG_VALUE (0XFFFFFFFF)

#define toFahrenheit(value)((32+(value)*1.8))
#define toConductivity(value)(((value)==0?2000:1/(value)))
#define toMpa(value)((value)/10)
#define toPsi(value)((value)/0.0689476)
#define toGallon(value)((value)/3.785)

#define MAX_FILE_SIZE (256*1024)

extern unsigned char gFileMem[MAX_FILE_SIZE];

//for sub-account. 2019.10 15 add
int check_Sub_Account();

unsigned int DispGetCurSecond(void);
DB_RFID_STRU * getRfid(int addr);
DB_HANDLER_STRU * getHandler(int addr);
int getAlarmState(void);
int getTankLevel(void);
int getEventCount(void);

void ChangeNetworkConfig(NIF *nif);
void parseNetInterface(NIF *nif);

#ifdef __cplusplus
}
#endif

#endif

