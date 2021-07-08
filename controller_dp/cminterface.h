#ifndef _CM_INTERFACE_H_
#define _CM_INTERFACE_H_

#pragma   pack(1)

#define APP_PROTOL_CANID_RSVD           (0x0)
#define APP_PROTOL_CANID_ALLOC_BEGIN    (0X1)
#define APP_PROTOL_CANID_ALLOC_END      (511)
#define APP_PROTOL_CANID_DYNAMIC_BEGIN  (512)
#define APP_PROTOL_CANID_DYNAMIC_END    (1022)
#define APP_PROTOL_CANID_BROADCAST      (0x3ff)
#define APP_PROTOL_CANID_INVALID        (0xFFFF)
#define APP_PROTOL_CANID_DYNAMIC_RANGE  (APP_PROTOL_CANID_DYNAMIC_END-APP_PROTOL_CANID_DYNAMIC_BEGIN)


#define APP_PROTOL_PACKET_LEN_POS      (0)
#define APP_PROTOL_PACKET_DEV_TYPE_POS (1)
#define APP_PROTOL_PACKET_MSG_TYPE_POS (2)
#define APP_PROTOL_PACKET_CONT_POS     (3)

#define APP_PROTOL_PACKET_HEADER       (3)

#define APP_PROTOL_PACKET_RSP_MASK     (0x80)

#define APP_HAND_SET_BEGIN_ADDRESS     (0x0A)
#define APP_HAND_SET_MAX_NUMBER        (0x08)
#define APP_HAND_SET_END_ADDRESS       (APP_HAND_SET_BEGIN_ADDRESS + APP_HAND_SET_MAX_NUMBER)

#define APP_RF_READER_BEGIN_ADDRESS     (0x14)
#define APP_RF_READER_MAX_NUMBER        (0x08)
#define APP_RF_READER_END_ADDRESS       (APP_RF_READER_BEGIN_ADDRESS + APP_RF_READER_MAX_NUMBER)

#define APP_SN_LENGTH                   (10)
#define APP_CAT_LENGTH                  (9)
#define APP_LOT_LENGTH                  (10)

#define APP_EXE_VALVE_NUM      (8)
#define APP_EXE_G_PUMP_NUM     (2)
#define APP_EXE_R_PUMP_NUM     (2)
#define APP_EXE_RECT_NUM       (3)
#define APP_EXE_EDI_NUM        (1)
#define APP_EXE_HO_VALVE_NUM   (2)

#define APP_EXE_SWITCHS        (APP_EXE_VALVE_NUM + APP_EXE_G_PUMP_NUM + APP_EXE_R_PUMP_NUM + APP_EXE_RECT_NUM + APP_EXE_EDI_NUM)
#define APP_EXE_SWITCHS_MASK   ((1<<APP_EXE_SWITCHS)-1)

#define APP_EXE_E1_NO          (0)
#define APP_EXE_E2_NO          (1)
#define APP_EXE_E3_NO          (2)
#define APP_EXE_E4_NO          (3)
#define APP_EXE_E5_NO          (4)
#define APP_EXE_E6_NO          (5)
#define APP_EXE_E9_NO          (6)
#define APP_EXE_E10_NO         (7)
#define APP_EXE_EMASK          ((1<<APP_EXE_VALVE_NUM) - 1)



#define APP_EXE_C3_NO         (8)  /* G pump */
#define APP_EXE_C4_NO         (9)  /* G pump */
#define APP_EXE_C1_NO         (10) /* R pump */
#define APP_EXE_C2_NO         (11) /* R pump */
#define APP_EXE_GCMASK          ((1<<APP_EXE_G_PUMP_NUM ) - 1)
#define APP_EXE_RCMASK          ((1<<APP_EXE_R_PUMP_NUM) - 1)

#define APP_EXE_N1_NO         (12)
#define APP_EXE_N2_NO         (13)
#define APP_EXE_N3_NO         (14)
#define APP_EXE_NMASK          ((1<<APP_EXE_RECT_NUM) - 1)

#define APP_EXE_T1_NO         (15)
#define APP_EXE_TMASK          ((1<<APP_EXE_EDI_NUM) - 1)


#define APP_EXE_HO_E7_NO      (17)
#define APP_EXE_HO_E8_NO      (18)
#define APP_EXE_TESTMASK      ((1<<APP_EXE_HO_VALVE_NUM) - 1)

#define APP_EXE_INNER_SWITCHS  (APP_EXE_SWITCHS_MASK & (~((1<<APP_EXE_N3_NO)|(1<<APP_EXE_C4_NO))))


#define APP_EXE_PRESSURE_METER (3)
#define APP_EXE_PM1_NO         (0)
#define APP_EXE_PM2_NO         (1)
#define APP_EXE_PM3_NO         (2)
#define APP_EXE_PM_MASK        ((1<<APP_EXE_PRESSURE_METER)-1)

#define APP_EXE_ECO_NUM       (5)
#define APP_EXE_I1_NO         (0)
#define APP_EXE_I2_NO         (1)
#define APP_EXE_I3_NO         (2)
#define APP_EXE_I4_NO         (3)
#define APP_EXE_I5_NO         (4)
#define APP_EXE_I_MASK        ((1<<APP_EXE_ECO_NUM)-1)

#define APP_EXE_IP_MASK       (APP_EXE_I_MASK|(APP_EXE_PM_MASK<<APP_EXE_ECO_NUM))

#define APP_EXE_RECT1_NO         (0)
#define APP_EXE_RECT2_NO         (1)
#define APP_EXE_RECT3_NO         (2)
#define APP_EXE_RECT_MASK        ((1<<APP_EXE_RECT_NUM)-1)


#define APP_EXE_DIN1_NO       (0) /* k3 */
#define APP_EXE_DIN2_NO       (1) /* k4 */
#define APP_EXE_DIN3_NO       (2) /* k5 */

#define APP_EXE_DIN_NUM       (3)

#define APP_EXE_DIN_MASK      ((1<<APP_EXE_DIN_NUM)-1)
#define APP_EXE_DIN_LEAK_KEY  (APP_EXE_DIN2_NO)  //2020.2.17 仅用于水箱溢流保护不再用于漏水保护
#define APP_EXE_DIN_IWP_KEY   (APP_EXE_DIN3_NO)  /* Incoming water pressure */
#define APP_EXE_DIN_RF_KEY    (APP_EXE_DIN1_NO)  /* Reverse Flush key */
#define APP_EXE_DIN_FAIL_MASK (APP_EXE_DIN_MASK & (~(1 << APP_EXE_DIN_IWP_KEY)))

#define APP_FM_FLOW_METER_NUM (4)
#define APP_FM_FM1_NO         (0)
#define APP_FM_FM2_NO         (1)
#define APP_FM_FM3_NO         (2)
#define APP_FM_FM4_NO         (3)
#define APP_FM_FM_MASK        ((1<<APP_FM_FLOW_METER_NUM)-1)

#define APP_EXE_INPUT_REG_LOOP_OFFSET      (0)
#define APP_EXE_INPUT_REG_LOOP_NUM         (APP_EXE_PRESSURE_METER)

#define APP_EXE_INPUT_REG_RECTIFIER_OFFSET (APP_EXE_INPUT_REG_LOOP_OFFSET + APP_EXE_INPUT_REG_LOOP_NUM)
#define APP_EXE_INPUT_REG_RECTIFIER_NUM    (APP_EXE_RECT_NUM)

#define APP_EXE_INPUT_REG_EDI_OFFSET       (APP_EXE_INPUT_REG_RECTIFIER_OFFSET + APP_EXE_INPUT_REG_RECTIFIER_NUM)
#define APP_EXE_INPUT_REG_EDI_NUM          (APP_EXE_EDI_NUM)

#define APP_EXE_INPUT_REG_PUMP_OFFSET      (APP_EXE_INPUT_REG_EDI_OFFSET + APP_EXE_INPUT_REG_EDI_NUM)
#define APP_EXE_INPUT_REG_PUMP_NUM         (APP_EXE_G_PUMP_NUM)

#define APP_EXE_INPUT_REG_REGPUMPI_OFFSET  (APP_EXE_INPUT_REG_PUMP_OFFSET + APP_EXE_INPUT_REG_PUMP_NUM)
#define APP_EXE_INPUT_REG_REGPUMPI_NUM     (APP_EXE_R_PUMP_NUM)

#define APP_EXE_INPUT_REG_REGPUMPV_OFFSET  (APP_EXE_INPUT_REG_REGPUMPI_OFFSET + APP_EXE_INPUT_REG_REGPUMPI_NUM)
#define APP_EXE_INPUT_REG_REGPUMPV_NUM     (APP_EXE_R_PUMP_NUM)

#define APP_EXE_MAX_ECO_NUMBER             (APP_EXE_ECO_NUM)

#define APP_EXE_INPUT_REG_QMI_OFFSET       (APP_EXE_INPUT_REG_REGPUMPV_OFFSET + APP_EXE_INPUT_REG_REGPUMPV_NUM)
#define APP_EXE_INPUT_REG_QMI_NUM          (APP_EXE_MAX_ECO_NUMBER*3)

#define APP_EXE_INPUT_REG_DIN_OFFSET       (APP_EXE_INPUT_REG_QMI_OFFSET + APP_EXE_INPUT_REG_QMI_NUM)
#define APP_EXE_INPUT_REG_DIN_NUM          (1)

#define APP_EXE_MAX_INPUT_REGISTERS        (APP_EXE_INPUT_REG_LOOP_NUM + APP_EXE_INPUT_REG_RECTIFIER_NUM + APP_EXE_INPUT_REG_RECTIFIER_NUM +APP_EXE_INPUT_REG_PUMP_NUM + APP_EXE_INPUT_REG_REGPUMPI_NUM + APP_EXE_INPUT_REG_REGPUMPV_NUM + APP_EXE_INPUT_REG_QMI_NUM + APP_EXE_INPUT_REG_DIN_NUM)


#define APP_EXE_HOLD_REG_REPORT_OFFSET    (0)
#define APP_EXE_HOLD_REG_REPORT_NUM       (2)

#define APP_EXE_HOLD_REG_RPUMP_OFFSET    (2)
#define APP_EXE_HOLD_REG_RPUMP_NUM       (2)

#define APP_EXE_MAX_HOLD_REGISTERS         (4)

#define APP_EXE_ECO_REPORT_MASK            ((1<<APP_EXE_MAX_ECO_NUMBER)-1)
#define APP_EXE_PM_REPORT_MASK             (((1<<APP_EXE_PRESSURE_METER)-1) << APP_EXE_MAX_ECO_NUMBER)

#define APP_EXE_GPUMP_REPORT_POS           ((APP_EXE_VALVE_NUM ))
#define APP_EXE_RPUMP_REPORT_POS           ((APP_EXE_VALVE_NUM + APP_EXE_G_PUMP_NUM))
#define APP_EXE_RECT_REPORT_POS            ((APP_EXE_VALVE_NUM + APP_EXE_G_PUMP_NUM + APP_EXE_R_PUMP_NUM))
#define APP_EXE_EDI_REPORT_POS             ((APP_EXE_VALVE_NUM + APP_EXE_G_PUMP_NUM + APP_EXE_G_PUMP_NUM + APP_EXE_RECT_NUM))


#define APP_EXE_GPUMP_REPORT_MASK          (APP_EXE_GCMASK << APP_EXE_GPUMP_REPORT_POS)
#define APP_EXE_RPUMP_REPORT_MASK          (APP_EXE_RCMASK << APP_EXE_RPUMP_REPORT_POS)
#define APP_EXE_RECT_REPORT_MASK           (APP_EXE_NMASK  << APP_EXE_RECT_REPORT_POS)
#define APP_EXE_EDI_REPORT_MASK            (APP_EXE_TMASK  << APP_EXE_EDI_REPORT_POS)

#define APP_TOC_SAMPLES_PER_SECOND         (2)
#define APP_TOC_OXIDIZATION_SECOND         (2)
#define APP_TOC_FLUSH2_SAMPLES             (4)

#define APP_ZIGBEE_SUB_PROTOL_LENGTH       (1)

#define APP_QTW_UNIT                       (100) // ml

#define APP_EXE_INPUT_REG_FM_NUM           APP_FM_FLOW_METER_NUM

#define APP_EXE_HB_NUM                      (3)

#define APP_TEMPERATURE_SCALE               (10)

typedef enum 
{
    APP_CAN_CHL_OUTER = 0,
    APP_CAN_CHL_INNER ,
    APP_CAN_CHL_NUM,
}APP_CAN_CHL_ENUM;

typedef enum
{
    APP_TRX_CAN = 0, 
    APP_TRX_ZIGBEE,  
    APP_TRX_NUM,
}APP_TRX_TYPE_ENUM;

enum TANK_WATER_LEVEL_ENUM {
    TANK_WATER_LEVEL_SCALE_00 = 0,
    TANK_WATER_LEVEL_SCALE_01,
    TANK_WATER_LEVEL_SCALE_02,
    TANK_WATER_LEVEL_SCALE_03,
    TANK_WATER_LEVEL_SCALE_04,
    TANK_WATER_LEVEL_SCALE_05,
    TANK_WATER_LEVEL_SCALE_06,
    TANK_WATER_LEVEL_SCALE_07,
    TANK_WATER_LEVEL_SCALE_08,
    TANK_WATER_LEVEL_SCALE_09,
    TANK_WATER_LEVEL_SCALE_10,
    TANK_WATER_LEVEL_SCALE_NUM,
};

enum PUMP_SPEED_ENUM {
    PUMP_SPEED_00 = 0,
    PUMP_SPEED_01,
    PUMP_SPEED_02,
    PUMP_SPEED_03,
    PUMP_SPEED_04,
    PUMP_SPEED_05,
    PUMP_SPEED_06,
    PUMP_SPEED_07,
    PUMP_SPEED_08,
    PUMP_SPEED_09,
    PUMP_SPEED_10,
    PUMP_SPEED_NUM,
};

typedef enum
{
    APP_LAN_ENG,
    APP_LAN_CHN,
    APP_LAN_SPA, /*2018/01/24 modify*/
    APP_LAN_FRE,
    APP_LAN_GER, 
    APP_LAN_ITA,
    APP_LAN_SKR,
    APP_LAN_RUS,
    APP_LAN_POR,
    APP_LAN_NUM   
}APP_LAN_TYPE_ENUM;


typedef enum
{
    APP_OBJ_N_PUMP,/* Normal Pump */
    APP_OBJ_R_PUMP,/* Regulator Pump*/
    APP_OBJ_VALVE, /* Magnetic Valve */ 
    APP_OBJ_RECT,  /* Rectifer */
    APP_OBJ_B,     /* Pressure Meter */
    APP_OBJ_I,     /* Sensor for Water Qulality Messure */
    APP_OBJ_S,     /* Sensor for Water Volumn Messure */
    APP_OBJ_EDI,   /* EDI */
}APP_OBJ_TYPE_ENUM;

typedef enum
{
   APP_OBJ_VALUE_U32 = 0,
   APP_OBJ_VALUE_FLOAT ,
   APP_OBJ_VALUE_CUST ,
}APP_OBJ_VALUE_ENUM;

typedef enum
{
   APP_DEV_TYPE_HOST = 0,
   APP_DEV_TYPE_MAIN_CTRL ,
   APP_DEV_TYPE_EXE_BOARD,
   APP_DEV_TYPE_FLOW_METER,
   APP_DEV_TYPE_HAND_SET,
   APP_DEV_TYPE_RF_READER,
   /*2018/03/04 add */
   APP_DEV_TYPE_EXE_COMBINED = 0x10,
   APP_DEV_TYPE_BROADCAST    = 0xff, 
}APP_DEV_TYPE_ENUM;


typedef enum
{
    APP_DEV_HS_SUB_UP = 0, // for ultra-pure water
    APP_DEV_HS_SUB_HP,     // for pure water
    APP_DEV_HS_SUB_NUM,
}APP_DEV_HS_SUB_TYPE_ENUM;

typedef enum
{
    APP_PAKCET_ADDRESS_HOST   = 0X00,
    APP_PAKCET_ADDRESS_MC     = 0X01 , 
    APP_PAKCET_ADDRESS_EXE    = 0X02 , 
    APP_PAKCET_ADDRESS_FM     = 0X03,
    APP_PAKCET_ADDRESS_HS     = 0X0A,
	APP_PAKCET_ADDRESS_RF	  = 0X14,
    
}APP_PACKET_ADDRESS_ENUM;


typedef enum
{
    APP_RFID_SUB_TYPE_PPACK_CLEANPACK   = 0, /* IN CLEAN STAGE CLEANPACK, OTHERWISE PPACK*/
    APP_RFID_SUB_TYPE_HPACK_ATPACK, 
    APP_RFID_SUB_TYPE_UPACK_HPACK , 
    APP_RFID_SUB_TYPE_PREPACK,
    APP_RFID_SUB_TYPE_ROPACK_OTHERS,
    APP_RFID_SUB_TYPE_NUM,
}APP_RFID_SUB_TYPE_ENUM;

typedef enum
{
    APP_WORK_MODE_NORMAL   = 0, /* IN CLEAN STAGE CLEANPACK, OTHERWISE PPACK*/
    APP_WORK_MODE_CLEAN,
    APP_WORK_MODE_INSTALL,
    APP_WORK_MODE_NUM,
}APP_WORK_MODE_ENUM;


typedef enum
{
    APP_PAKCET_COMM_ONLINE_NOTI   = 0X00,
    APP_PACKET_COMM_HEART_BEAT    = 0X01 , 
    APP_PACKET_COMM_HOST_RESET    = 0X02 , 
    APP_PACKET_COMM_ZIGBEE_IND    = 0X03 , 

    
    APP_PACKET_CLIENT_REPORT      = 0X10,
    APP_PACKET_HAND_OPERATION     = 0X11,
    APP_PACKET_STATE_INDICATION   = 0X12,
	APP_PACKET_RF_OPERATION	      = 0X13,
	APP_PACKET_EXE_OPERATION      = 0x14,
    
}APP_PACKET_ENUM;

typedef enum
{
    APP_PACKET_RPT_FM = 0,    // FLOW METER
        
}APP_PACKET_RPT_FLOW_METER_TYPE;

typedef enum
{
    APP_PACKET_RPT_ECO =0 ,    //  Electrical pod coeffience
    APP_PACKET_RPT_PM     ,    //  pressure meter report
    APP_PACKET_DIN_STATUS ,    //  DRY INPUT state
    APP_PACKET_MT_STATUS  ,    //  device state
    APP_PACKET_RPT_GPUMP  ,      
    APP_PACKET_RPT_RPUMP  ,    // 5    
    APP_PACKET_RPT_RECT   ,      
    APP_PACKET_RPT_EDI    ,       
    APP_PACKET_RPT_TOC    ,
    APP_PACKET_RPT_LEAK   ,   // 9
}APP_PACKET_RPT_EXE_BOARD_TYPE;

typedef enum
{
    APP_PACKET_HS_STATE_IDLE = 0,
    APP_PACKET_HS_STATE_RUN, /* System Is Ready */
    APP_PACKET_HS_STATE_QTW, /* Quantity Taking Water */
    APP_PACKET_HS_STATE_CIR, /* Circulation */
    APP_PACKET_HS_STATE_DEC, /* Decompress */
}APP_PACKET_HS_STATE_ENUM;

typedef enum
{
    APP_PACKET_HO_CIR = 0,    //  CIRCULATION
    APP_PACKET_HO_QTW   ,    //  Taking quantified water
    APP_PACKET_HO_STA   ,    //  status
    APP_PACKET_HO_SPEED ,
    APP_PACKET_HO_QL_UPDATE,
    APP_PACKET_HO_QTW_VOLUMN ,
    APP_PACKET_HO_ALARM_STATE_NOT,
    APP_PACKET_HO_SYSTEM_TEST,
    APP_PACKET_HO_WQ_NOTIFY,
	APP_PACKET_HO_ADR_RST = 0X10,    //  reset CAN address
	APP_PACKET_HO_ADR_SET       ,	 //  SET CAN ADR
	APP_PACKET_HO_ADR_QRY       ,	 //  QRY CAN ADR
    APP_PACKET_HO_ZB_IND = 0X20,
    
    // 2020/07/02 ADD for Next Generation Machine
    APP_PACKET_HO_EXT_START = 0X30,
    APP_PACKET_HO_EXT_RO_CLEAN,
    APP_PACKET_HO_EXT_SYSTEM_START,
    APP_PACKET_HO_EXT_RTI_QUERY   ,
    APP_PACKET_HO_EXT_NOTIFY      ,
    APP_PACKET_HO_EXT_CONFIG_QUERY,
    APP_PACKET_HO_EXT_CONFIG_SET,
    APP_PACKET_HO_EXT_RO_QUERY,
    APP_PACKET_HO_EXT_QUERY_CM_STATE,
    APP_PACKET_HO_EXT_QUERY_CM_INFO,   
    APP_PACKET_HO_EXT_RESET_CM,
    APP_PACKET_HO_EXT_QUERY_NOTI,
    APP_PACKET_HO_EXT_QUERY_ALARM,
    APP_PACKET_HO_EXT_MACH_PARAM_QUERY,   
    APP_PACKET_HO_EXT_QUERY_SYS_INFO,
    APP_PACKET_HO_EXT_MAIN_CTRL_STA,
    APP_PACKET_HO_EXT_EXPORT_HISTORY,
    APP_PACKET_HO_EXT_ACCESSORY_INSTALL_START,
    APP_PACKET_HO_EXT_ACCESSORY_INSTALL_FINISH,
    APP_PACKET_HO_EXT_ACCESSORY_INSTALL_QRY,
    APP_PACKET_HO_EXT_QUERY_CM_DETAIL_INFO,
}APP_PACKET_HAND_OPERATION_TYPE;


typedef enum
{
    APP_PACKET_HO_ERROR_CODE_SUCC  = 0,
    APP_PACKET_HO_ERROR_CODE_UNREADY  ,
    APP_PACKET_HO_ERROR_CODE_UNSUPPORT,
    APP_PACKET_HO_ERROR_CODE_TANK_EMPTY,
    APP_PACKET_HO_ERROR_CODE_UNKNOW,
    APP_PACKET_HO_ERROR_CODE_TIMEOUT,

}APP_PACKET_HO_ERROR_CODE_ENUM;

typedef enum
{
    APP_PACKET_RF_ERROR_CODE_SUCC = 0,
    APP_PACKET_RF_NO_DEVICE,
    APP_PACKET_RF_ACCESS_CONFLICT,
    APP_PACKET_RF_ACCESS_TIMEOUT,
}APP_PACKET_RF_ERROR_CODE_ENUM;

typedef enum
{
    CIR_TYPE_UP = 0, /* UP */
    CIR_TYPE_HP,    /* TANK CIR*/
    CIR_TYPE_NUM,
}APP_CIR_TYPE_ENUM;

// 2020/08/01 ADD
typedef enum
{
    WATER_TYPE_UP = 0, /* UP      */
    WATER_TYPE_HP_RO,  /* HP */  
    WATER_TYPE_HP_EDI, /* HP */
    WATER_TYPE_NUM,
}APP_WATER_TYPE_ENUM;


typedef enum
{ 
    APP_PACKET_RF_SEARCH = 0,
    APP_PACKET_RF_READ,    
    APP_PACKET_RF_WRITE   ,    
        
	APP_PACKET_RF_ADR_RST = 0X10,    //  reset CAN address
	APP_PACKET_RF_ADR_SET       ,	 //  SET CAN ADR
	APP_PACKET_RF_ADR_QRY       ,	 //  QRY CAN ADR
}APP_PACKET_RF_OPERATION_TYPE;

typedef enum
{
    APP_PACKET_RPT_RF_STATE = 0,    //  rfid state reprot
        
}APP_PACKET_RPT_RF_READER_TYPE;

typedef enum
{
    APP_PACKET_EXE_TOC = 0,    //  TOC
	APP_PACKET_EXE_NUM    ,	 
}APP_PACKET_EXE_OPERATION_TYPE;


typedef enum
{
    APP_PACKET_HO_SYS_TEST_TYPE_TEST = 0,
    APP_PACKET_HO_SYS_TEST_TYPE_DEPRESSURE, 
}APP_PACKET_HO_SYS_TEST_TYPE_ENUM;

typedef enum
{
   APP_CLEAN_RO = 0,
   APP_CLEAN_PH    ,
   APP_CLEAN_SYSTEM   ,
   APP_CLEAN_NUM,
}APP_CLEAN_ENUM;

typedef enum
{
     MACHINE_L_Genie = 0,
     MACHINE_L_UP,
     MACHINE_L_EDI_LOOP,
     MACHINE_L_RO_LOOP,
     MACHINE_Genie,
     MACHINE_UP,
     MACHINE_EDI,
     MACHINE_RO,
     MACHINE_PURIST,
     MACHINE_ADAPT,
     MACHINE_NUM,
}MACHINE_TYPE_ENUM;


/* FOR USER config & real time info display */
typedef enum 
{
   PARAM_DO_LAN      = 0,
   PARAM_DO_DATETIME    ,
   PARAM_DO_WaterQuality_Unit,
   PARAM_DO_Pressure_Unit,
   PARAM_DO_Barrel_Unit,
   PARAM_DO_Temp_Unit,
   PARAM_DO_FlowCoeff,
   PARAM_DO_UP,                 // for display: from I5
   PARAM_DO_RO_Output,          // for display
   /* 10 */
   PARAM_DO_RO_Input,           // for display 
   PARAM_DO_EDI_Output,         // for display 
   PARAM_DO_Tap_Water,          // for display 
   PARAM_DO_Tap_Water_Pressure, // for display 
   PARAM_DO_RO_Ongoing_Pressure,// for display 
   PARAM_DO_RO_REJ,             // for display 
   PARAM_DO_Misc_Flags,
   PARAM_DO_WIFI_IP,            // for display 
   PARAM_DO_ETH_IP,             // for display 
   PARAM_DO_MACH_TYPE,
   PARAM_DO_EXT_FLAGS,          // 2020/02/20 add
   PARAM_DO_REFILL_THD,
   PARAM_DO_TANKUV_INFO,  
   PARAM_DO_TANK_PARAM,         // for pure water tank param
   PARAM_DO_NUM,
}PARAM_DICT_OBJECT_ENUM;

// move from display.h to here
typedef enum
{
    DISP_ALARM_WaterInPressure = 0,  // jingshui yali
    DISP_ALARM_ROResidue ,           //  RO jieliulv
    DISP_ALARM_ROProduct,            //  RO qushui
    DISP_ALARM_EdiProduct ,          //   edi qushui
    DISP_ALARM_UPProduct,            //   up qushui
    DISP_ALARM_TOC,                  //   TOC
    DISP_ALARM_B_TANKOVERFLOW,
    DISP_ALARM_B_LEAK,
    DISP_ALARM_N1,                
    DISP_ALARM_N2,
    DISP_ALARM_N3,
    DISP_ALARM_EDI,
    DISP_ALARM_GP1,
    DISP_ALARM_GP2,
    DISP_ALARM_RPV1,
    DISP_ALARM_RPV2,
    DISP_ALARM_RPI1,
    DISP_ALARM_RPI2,
    DISP_ALARM_ROP_LV,
    DISP_ALARM_ROP_HV,
    DISP_ALARM_ROD_V,
    DISP_ALARM_NUM,
}DISP_ALARM_ENUM;

#define DISP_ALARM_PARAMS (DISP_ALARM_RPI2 - DISP_ALARM_N1 + 1)

#define ALARM_TLP  (1 << DISP_ALARM_WaterInPressure)
#define ALARM_REJ  (1 << DISP_ALARM_ROResidue)
#define ALARM_ROPW (1 << DISP_ALARM_ROProduct)
#define ALARM_EDI  (1 << DISP_ALARM_EdiProduct)
#define ALARM_UP   (1 << DISP_ALARM_UPProduct)
#define ALARM_TOC  (1 << DISP_ALARM_TOC)
#define ALARM_BO   (1 << DISP_ALARM_B_TANKOVERFLOW)
#define ALARM_BL   (1 << DISP_ALARM_B_LEAK)
#define ALARM_N1   (1 << DISP_ALARM_N1)
#define ALARM_N2   (1 << DISP_ALARM_N2)
#define ALARM_N3   (1 << DISP_ALARM_N3)
#define ALARM_ROPLV (1 << DISP_ALARM_ROP_LV)
#define ALARM_ROPHV (1 << DISP_ALARM_ROP_HV)
#define ALARM_RODV (1 << DISP_ALARM_ROD_V)

typedef enum
{
    DISP_ALARM_PART0 = 0,
    DISP_ALARM_PART1,
    DISP_ALARM_PART_NUM,
}DISP_ALARM_PART_ENUM;

typedef enum
{
    DISP_ALARM_PART0_254UV_OOP = 0,  //检查254UV
    DISP_ALARM_PART0_185UV_OOP ,     //检查185UV
    DISP_ALARM_PART0_TANKUV_OOP ,    //检查水箱UV
    DISP_ALARM_PART0_TUBEUV_OOP ,    //检查管道UV
    DISP_ALARM_PART0_PREPACK_OOP ,   //预处理柱脱落
    DISP_ALARM_PART0_ACPACK_OOP ,    //AC Pack脱落
    DISP_ALARM_PART0_PPACK_OOP ,     //P Pack脱落
    DISP_ALARM_PART0_ATPACK_OOP ,    //AT Pack脱落
    DISP_ALARM_PART0_HPACK_OOP ,     //H Pack脱落
    DISP_ALARM_PART0_UPACK_OOP ,     //U Pack脱落
    DISP_ALARM_PART0_NUM,
}DISP_ALARM_PART0_ENUM;

#define DISP_ALARM_DEFAULT_PART0 ((1<<DISP_ALARM_PART0_NUM)-1)

typedef enum
{
    DISP_ALARM_PART1_LOWER_SOURCE_WATER_PRESSURE = 0,     //自来水压力低
    DISP_ALARM_PART1_HIGER_SOURCE_WATER_CONDUCTIVITY,     //自来水电导率高
    DISP_ALARM_PART1_HIGER_RO_PRODUCT_CONDUCTIVITY,       //RO电导率高
    DISP_ALARM_PART1_LOWER_RO_RESIDUE_RATIO,              //RO截留率高
    DISP_ALARM_PART1_LOWER_EDI_PRODUCT_RESISTENCE,        //EDI电阻率低
    DISP_ALARM_PART1_LOWER_UP_PRODUCT_RESISTENCE,         //UP电阻率低
    DISP_ALARM_PART1_LOWER_TUBE_RESISTENCE,               //管路电阻率低
    DISP_ALARM_PART1_LOWER_PWTANKE_WATER_LEVEL,           //纯水箱液位低
    DISP_ALARM_PART1_LOWER_SWTANKE_WATER_LEVEL,           //原水箱液位低
    DISP_ALARM_PART1_LOWER_RO_PRODUCT_FLOWING_VELOCITY,   //RO产水流速低
    DISP_ALARM_PART1_LOWER_RO_WASTE_FLOWING_VELOCITY,     //RO弃水流速低
    DISP_ALARM_PART1_LOWER_CIR_WATER_CONDUCTIVITY,        //循环水质低   未启用
    DISP_ALARM_PART1_LOWER_HP_PRODUCT_WATER_CONDUCTIVITY, //HP水质低
    DISP_ALARM_PART1_LOWER_PWTANK_RESISTENCE,             //纯水箱水质低
    DISP_ALARM_PART1_HIGHER_SOURCE_WATER_TEMPERATURE,     //RO进水温度高
    DISP_ALARM_PART1_LOWER_SOURCE_WATER_TEMPERATURE,      //RO进水温度低
    DISP_ALARM_PART1_HIGHER_RO_PRODUCT_TEMPERATURE,       //RO温度高
    DISP_ALARM_PART1_LOWER_RO_PRODUCT_TEMPERATURE,        //RO温度低
    DISP_ALARM_PART1_HIGHER_EDI_PRODUCT_TEMPERATURE,      //EDI温度高
    DISP_ALARM_PART1_LOWER_EDI_PRODUCT_TEMPERATURE,       //EDI温度低
    DISP_ALARM_PART1_HIGHER_UP_PRODUCT_TEMPERATURE,       //UP温度高
    DISP_ALARM_PART1_LOWER_UP_PRODUCT_TEMPERATURE,        //UP 温度低
    DISP_ALARM_PART1_HIGHER_TUBE_TEMPERATURE,             //管路温度高
    DISP_ALARM_PART1_LOWER_TUBE_TEMPERATURE,              //管路温度低
    DISP_ALARM_PART1_HIGHER_TOC_SENSOR_TEMPERATURE,       //TOC温度高
    DISP_ALARM_PART1_LOWER_TOC_SENSOR_TEMPERATURE,        //TOC温度低
    DISP_ALARM_PART1_LOWER_TOC_SOURCE_WATER_RESISTENCE,   //TOC水质低
    DISP_ALARM_PART1_LEAK_OR_TANKOVERFLOW,                //漏水保护或溢流
    DISP_ALARM_PART1_HIGH_WORK_PRESSURE,                  //工作压力高
    DISP_ALARM_PART1_NUM,
}DISP_ALARM_PART1_ENUM;

#define DISP_ALARM_DEFAULT_PART1 ((1<<DISP_ALARM_PART1_NUM)-1)

#define DISP_ALARM_TOTAL_NUM (DISP_ALARM_PART0_NUM + DISP_ALARM_PART1_NUM)

#define DISP_ALARM_PART1_SOURCE_WATER_TEMP_ALARM ((1<<DISP_ALARM_PART1_HIGHER_SOURCE_WATER_TEMPERATURE) | (1<<DISP_ALARM_PART1_LOWER_SOURCE_WATER_TEMPERATURE))
#define DISP_ALARM_PART1_RO_TEMP_ALARM ((1<<DISP_ALARM_PART1_HIGHER_RO_PRODUCT_TEMPERATURE) | (1<<DISP_ALARM_PART1_LOWER_RO_PRODUCT_TEMPERATURE))
#define DISP_ALARM_PART1_EDI_TEMP_ALARM ((1<<DISP_ALARM_PART1_HIGHER_EDI_PRODUCT_TEMPERATURE) | (1<<DISP_ALARM_PART1_LOWER_EDI_PRODUCT_TEMPERATURE))
#define DISP_ALARM_PART1_UP_TEMP_ALARM ((1<<DISP_ALARM_PART1_HIGHER_UP_PRODUCT_TEMPERATURE) | (1<<DISP_ALARM_PART1_LOWER_UP_PRODUCT_TEMPERATURE))
#define DISP_ALARM_PART1_TUBE_TEMP_ALARM ((1<<DISP_ALARM_PART1_HIGHER_TUBE_TEMPERATURE) | (1<<DISP_ALARM_PART1_LOWER_TUBE_TEMPERATURE))
#define DISP_ALARM_PART1_TOC_TEMP_ALARM ((1<<DISP_ALARM_PART1_HIGHER_TOC_SENSOR_TEMPERATURE) | (1<<DISP_ALARM_PART1_LOWER_TOC_SENSOR_TEMPERATURE))

#define DISP_ALARM_PART1_LOWER_PWTANKE_WATER_LEVEL_ALARM ((1<<DISP_ALARM_PART1_LOWER_PWTANKE_WATER_LEVEL))
#define DISP_ALARM_PART1_LOWER_SWTANKE_WATER_LEVEL_ALARM ((1<<DISP_ALARM_PART1_LOWER_SWTANKE_WATER_LEVEL))

#define DISP_MAKE_ALARM(id) (1 << (id))

typedef enum
{
    DISP_SM_HaveB1 = 0,
    DISP_SM_HaveB2 ,
    DISP_SM_HaveB3 ,
    DISP_SM_HaveN1,
    DISP_SM_HaveN2 ,
    DISP_SM_HaveN3,
    DISP_SM_HaveT1,
    DISP_SM_HaveI,
    DISP_SM_HaveTOC,
    DISP_SM_P_PACK,
    DISP_SM_AT_PACK,
    DISP_SM_U_PACK,
    DISP_SM_H_PACK,
    /* 2017/11/11 add */
    DISP_SM_HaveSWValve,
    DISP_SM_ElecLeakProtector,
    DISP_SM_Printer,
    DISP_SM_TubeUV,
    DISP_SM_TubeDI,
    DISP_SM_HaveTubeFilter,
    DISP_SM_PreFilterColumn,
    /* 2017/12/25 add */
    DISP_SM_HP_Water_Cir,
    DISP_SM_RFID_Authorization,
    DISP_SM_User_Authorization, //2019.1.22 add
    DISP_SM_SUB_ACCOUNT,        //2019.10.15 add, for Sub-account
    DISP_SM_TankUV,
    DISP_SM_Pre_Filter,
    DISP_SM_HP_Electrode,
    DISP_SM_SW_PUMP,
    
    DISP_SM_NUM,
}DISP_SUB_MODULE_ENUM;

#define MODULES_ALL ((1 << DISP_SM_NUM) - 1)

typedef enum
{
    DISP_EXT_STATIC_IP = 0,
    DISP_EXT_NUM,
}DISP_EXT_FLAGS_ENUM;

#define DISP_EXT_FLAGS_ALL ((1 << DISP_EXT_NUM) - 1)

#if 0
#define DEFAULT_MODULES_L_Genie      (MODULES_ALL & (~(1 << DISP_SM_AT_PACK)))
#define DEFAULT_MODULES_L_UP         (MODULES_ALL & (~(1 << DISP_SM_AT_PACK)))
#define DEFAULT_MODULES_EDI_LOOP     (MODULES_ALL & (~(1 << DISP_SM_AT_PACK)))
#define DEFAULT_MODULES_RO_LOOP      (MODULES_ALL & (~(1 << DISP_SM_AT_PACK)))
#define DEFAULT_MODULES_Genie        (MODULES_ALL & (~(1 << DISP_SM_AT_PACK)))
#define DEFAULT_MODULES_UP           (MODULES_ALL & (~(1 << DISP_SM_AT_PACK)))
#define DEFAULT_MODULES_EDI          (MODULES_ALL & (~(1 << DISP_SM_AT_PACK)))
#define DEFAULT_MODULES_RO           (MODULES_ALL & (~(1 << DISP_SM_AT_PACK)))
#define DEFAULT_MODULES_PURIST       (MODULES_ALL & (~(1 << DISP_SM_AT_PACK)))
#define DEFAULT_MODULES_ADAPT        (MODULES_ALL & (~(1 << DISP_SM_AT_PACK)))
#endif

#define DEFAULT_MODULES_L_Genie      (MODULES_ALL & (~((1 << DISP_SM_AT_PACK) | (1 <<DISP_SM_Pre_Filter))))
#define DEFAULT_MODULES_L_UP         (MODULES_ALL & (~((1 << DISP_SM_AT_PACK) | (1 <<DISP_SM_Pre_Filter))))
#define DEFAULT_MODULES_EDI_LOOP     (MODULES_ALL & (~((1 << DISP_SM_AT_PACK) | (1 <<DISP_SM_Pre_Filter))))
#define DEFAULT_MODULES_RO_LOOP      (MODULES_ALL & (~((1 << DISP_SM_AT_PACK) | (1 <<DISP_SM_Pre_Filter))))
#define DEFAULT_MODULES_Genie        (MODULES_ALL & (~((1 << DISP_SM_AT_PACK) | (1 <<DISP_SM_Pre_Filter))))
#define DEFAULT_MODULES_UP           (MODULES_ALL & (~((1 << DISP_SM_AT_PACK) | (1 <<DISP_SM_Pre_Filter))))
#define DEFAULT_MODULES_EDI          (MODULES_ALL & (~((1 << DISP_SM_AT_PACK) | (1 <<DISP_SM_Pre_Filter))))
#define DEFAULT_MODULES_RO           (MODULES_ALL & (~((1 << DISP_SM_AT_PACK) | (1 <<DISP_SM_Pre_Filter))))
#define DEFAULT_MODULES_PURIST       (MODULES_ALL & (~((1 << DISP_SM_AT_PACK) | (1 <<DISP_SM_Pre_Filter))))
#define DEFAULT_MODULES_ADAPT        (MODULES_ALL & (~((1 << DISP_SM_AT_PACK) | (1 <<DISP_SM_Pre_Filter))))

typedef enum
{
    DISP_PRE_PACKLIFEDAY = 0,   //纯化柱:  0~999 DAYS    0~9999L 
    DISP_PRE_PACKLIFEL,     //纯化柱:   0~9999L
    DISP_AC_PACKLIFEDAY,   //AC PACK:  0~999 DAYS    0~9999L 
    DISP_AC_PACKLIFEL,     //AC PACK:   0~9999L
    DISP_T_PACKLIFEDAY,     //T PACK: 2018.10.12 ADD
    DISP_T_PACKLIFEL,       //T PACK: 2018.10.12 ADD
    DISP_P_PACKLIFEDAY ,    //纯化柱:  0~999 DAYS    0~9999L 
    DISP_P_PACKLIFEL ,      //纯化柱:   0~9999L 
    DISP_U_PACKLIFEDAY ,    //纯化柱:  0~999 DAYS    0~9999L 
    DISP_U_PACKLIFEL,       //纯化柱:   0~9999L
    DISP_H_PACKLIFEDAY ,    //纯化柱:  0~999 DAYS    0~9999L 
    DISP_H_PACKLIFEL,       //纯化柱:   0~9999L
    DISP_AT_PACKLIFEDAY ,   //纯化柱:  0~999 DAYS    0~9999L 
    DISP_AT_PACKLIFEL,      //纯化柱:   0~9999L
    DISP_N1_UVLIFEDAY ,     //  254UV  720天     8000小时 
    DISP_N1_UVLIFEHOUR,     // 
    DISP_N2_UVLIFEDAY,      // 185UV    720天     8000小时 
    DISP_N2_UVLIFEHOUR,     // 
    DISP_N3_UVLIFEDAY,      // 水箱UV    720天     8000小时 
    DISP_N3_UVLIFEHOUR,     // 
    DISP_N4_UVLIFEDAY,      //  管路UV    720天     8000小时 
    DISP_N4_UVLIFEHOUR,     // 
    DISP_N5_UVLIFEDAY,      //  TOC UV    720天     8000小时 
    DISP_N5_UVLIFEHOUR,     // 
    DISP_W_FILTERLIFE,      //水箱空气过滤器寿命 :0~999DAYS
    DISP_T_B_FILTERLIFE,    //终端过滤器寿命 :0~999DAYS
    DISP_T_A_FILTERLIFE,    //终端过滤器寿命 :0~999DAYS
    DISP_TUBE_FILTERLIFE,   //    管路过滤器    180天     
    DISP_TUBE_DI_LIFE,      //     管路DI    180天     
    DISP_ROC12LIFEDAY,      //RO Cl2清洗 :0~999DAYS
    DISP_CM_NUM,
}DISP_CM_ENUM;

typedef enum
{
    DISP_PRE_PACK = 0,  
    DISP_AC_PACK,  //2018.10.22 ADD
    DISP_T_PACK,  //2018.10.12 ADD
    DISP_P_PACK ,    
    DISP_U_PACK,
    DISP_AT_PACK,
    DISP_H_PACK,
    DISP_N1_UV, /*254*/
    DISP_N2_UV, /*185*/
    DISP_N3_UV, /*tank*/
    DISP_N4_UV, /* tube UV */
    DISP_N5_UV, /* TOC UV */
    DISP_W_FILTER,
    DISP_T_B_FILTER,
    DISP_T_A_FILTER,
    DISP_TUBE_FILTER,
    DISP_TUBE_DI,   
    DISP_CM_NAME_NUM,
}DISP_CM_NAME_ENUM;

typedef enum 
{
    APP_ACCESSORY_TYPE_CMAT = 0,
    APP_ACCESSORY_TYPE_MACH    
}APP_ACCESSORY_TYPE_ENUM;

enum CONSUMABLE_CATNO
{
    PREPACK_CATNO = 0,
    ACPACK_CATNO,
    PPACK_CATNO,
    UPACK_CATNO,
    HPACK_CATNO,
    ATPACK_CATNO,
    CLEANPACK_CATNO,

    TPACK_CATNO,
    ROPACK_CATNO,
    UV185_CATNO,
    UV254_CATNO,
    UVTANK_CATNO,
    ROPUMP_CATNO,
    UPPUMP_CATNO,
    FINALFILTER_A_CATNO,
    FINALFILTER_B_CATNO,
    EDI_CATNO,
    TANKVENTFILTER_CATNO,
    LOOPFILTER_CATNO,
    LOOPUV_CATNO,
    LOOPDI_CATNO,
    CAT_NUM
};

#define DISP_ROC12 DISP_CM_NAME_NUM
#define DISP_NOTIFY_DEFAULT ((1<<(DISP_ROC12+1))-1)

typedef enum
{
    DISP_MACHINERY_SOURCE_BOOSTER_PUMP = DISP_ROC12 + 1,    
    DISP_MACHINERY_TUBE_CIR_PUMP ,    
    DISP_MACHINERY_CIR_PUMP,
    DISP_MACHINERY_RO_MEMBRANE,
    DISP_MACHINERY_RO_BOOSTER_PUMP,
    DISP_MACHINERY_EDI,
    DISP_MACHINERY_LAST_NAME,
}DISP_MACHINERY_NAME_ENUM;

#define DISP_MACHINERY_NAME_NUM (DISP_MACHINERY_LAST_NAME - DISP_MACHINERY_SOURCE_BOOSTER_PUMP)

typedef enum
{
    CONDUCTIVITY_UINT_OMG = 0,
    CONDUCTIVITY_UINT_US,
    CONDUCTIVITY_UINT_NUM,    
}CONDUCTIVITY_UINT_ENUM;

typedef enum
{
    TEMERATURE_UINT_CELSIUS = 0,
    TEMERATURE_UINT_FAHRENHEIT,
    TEMERATURE_UINT_NUM,    
}TEMERATURE_UINT_ENUM;

typedef enum
{
    PRESSURE_UINT_BAR = 0,
    PRESSURE_UINT_MPA,
    PRESSURE_UINT_PSI,
    PRESSURE_UINT_NUM,    
}PRESSURE_UINT_ENUM;

typedef enum
{
    LIQUID_LEVEL_UINT_PERCENT = 0,
    LIQUID_LEVEL_UINT_LITRE,
    LIQUID_LEVEL_UINT_NUM,    
}LIQUID_LEVEL_UINT_ENUM;

typedef enum
{
    FLOW_VELOCITY_UINT_L_PER_MIN = 0,
    FLOW_VELOCITY_UINT_GAL_PER_MIN,
    FLOW_VELOCITY_UINT_NUM,    
}FLOW_VELOCITY_UINT_ENUM;

typedef enum
{
    MACHINE_PARAM_SP1 = 0,  // 0~1.6Mpa    B1进水压力下限
    MACHINE_PARAM_SP2,      // RO截留率下限,通过计算公式rejection=(I1b-I2)/I1b*100%
    MACHINE_PARAM_SP3,      // 0~100?s/cm  RO产水电导率上限（I2测得）
    MACHINE_PARAM_SP4,      // 0~18.2M .cm EDI产水电阻率下限（I3测得）
    MACHINE_PARAM_SP5,      // 0~2.0m  设备恢复产水液位（B2）
    MACHINE_PARAM_SP6,      // 0~2.0m  水箱低液位报警线（B2）
    MACHINE_PARAM_SP7,      // 0~18.2M .cm UP产水电阻率下限（I5）
    MACHINE_PARAM_SP8,      // 0~2.0m  源水箱补水液位（B3）
    MACHINE_PARAM_SP9,      // 0~2.0m  原水箱低位系统保护设定点（B3）
    MACHINE_PARAM_SP10,     // 0~18.2M .cm 水箱水水质下限（I4）
    MACHINE_PARAM_SP11,     // 0~18.2M .cm 水箱水水质上限（I4）
    MACHINE_PARAM_SP12,     // 0~18.2M .cm 纯水取水水质下限（I4） 没有用
    MACHINE_PARAM_SP13,     // 0~2000?s/cm 自来水电导率上限（I1a）
    MACHINE_PARAM_SP14,     // RO产水流速 上限100.0L/min     
    MACHINE_PARAM_SP15,     // RO产水流速 下限20.0L/min   
    MACHINE_PARAM_SP16,     // RO弃水流速 下限20.0L/min     
    MACHINE_PARAM_SP17,     // 管路水质   下限 1MΩ.cm     
    MACHINE_PARAM_SP18,     // 进水温度 上限45℃      
    MACHINE_PARAM_SP19,     // 进水温度 下限5℃     
    MACHINE_PARAM_SP20,     // RO产水温度 上限45℃ 
    MACHINE_PARAM_SP21,     // RO产水温度 下限5℃     
    MACHINE_PARAM_SP22,     // EDI产水温度 上限45℃  
    MACHINE_PARAM_SP23,     // EDI产水温度 下限5℃   
    MACHINE_PARAM_SP24,     // UP产水温度 上限45℃ 
    MACHINE_PARAM_SP25,     // UP产水温度 下限5℃     
    MACHINE_PARAM_SP26,     // TUBE产水温度 上限45℃  
    MACHINE_PARAM_SP27,     // TUBE产水温度  下限5℃  
    MACHINE_PARAM_SP28,     // TOC传感器温度 上限45℃  
    MACHINE_PARAM_SP29,     // TOC传感器温度 下限5℃   
    MACHINE_PARAM_SP30,     // TOC进水水质下限15.0MΩ.cm   
    MACHINE_PARAM_SP31,     // 水箱循环水质下限 用于T Pack耗材更换提醒  
    MACHINE_PARAM_SP32,     // HP 产水水质下限
    MACHINE_PARAM_SP33,     // RO工作压力上限
    MACHINE_PARAM_SP_NUM,

}MACHINE_PARAM_ENUM;

typedef enum
{
    SYSTEM_INFO_NAME_TYPE = 0,
    SYSTEM_INFO_NAME_CAT     ,
    SYSTEM_INFO_NAME_SN      ,
    SYSTEM_INFO_NAME_OWNER   ,
    SYSTEM_INFO_NAME_VERSION ,
    SYSTEM_INFO_NAME_NUM     ,
}SYSTEM_INFO_NAME_ENUM;


typedef enum
{
    HISTORY_EXPORT_TYPE_DAILY = 0,
    HISTORY_EXPORT_TYPE_WEEKLY   ,
    HISTORY_EXPORT_TYPE_MONTHLY  ,
    HISTORY_EXPORT_TYPE_3MONTHS  ,
    HISTORY_EXPORT_TYPE_6MONTHS  ,
    HISTORY_EXPORT_TYPE_YEARLY   ,
    HISTORY_EXPORT_TYPE_2YEARS   ,
    HISTORY_EXPORT_TYPE_NUM      ,
}HISTORY_EXPORT_TYPE_ENUM;


// moved from notify.h 20200811
typedef enum
{
    NOT_STATE_INIT = 0,
    NOT_STATE_WASH,    
    NOT_STATE_RUN,    
    NOT_STATE_LPP,     /* Low pressure protection state */
    NOT_STATE_QTW,     /* Quantity taking water  */
    NOT_STATE_CIR,     /* Circulation  */
    NOT_STATE_DEC,     /* Decompress  */
    NOT_STATE_OTHER,   /* Other state */
}NOT_STATE_ENUM;

typedef struct
{
    uint8_t ucLen;
    uint8_t ucTransId;
    uint8_t ucDevType; /* refer APP_DEV_TYPE_ENUM */
    uint8_t ucMsgType;
}APP_PACKET_COMM_STRU;

typedef struct
{
    APP_PACKET_COMM_STRU hdr;
    uint8_t acInfo[1];
}APP_PACKET_ONLINE_NOTI_IND_STRU;


typedef struct
{
    APP_PACKET_COMM_STRU hdr;
    uint8_t aucInfo[1];
}APP_PACKET_ONLINE_NOTI_CONF_STRU;

typedef struct
{
    uint16_t usHeartBeatPeriod;
}APP_PACKET_ONLINE_NOTI_CONF_EXE_STRU;

typedef APP_PACKET_ONLINE_NOTI_CONF_EXE_STRU APP_PACKET_ONLINE_NOTI_CONF_FM_STRU;

typedef APP_PACKET_ONLINE_NOTI_CONF_EXE_STRU APP_PACKET_ONLINE_NOTI_CONF_RFID_STRU;


typedef struct
{
    uint16_t usHeartBeatPeriod;
    uint8_t  ucLan;
    uint8_t  ucMode; 
    uint8_t  ucState;
    uint8_t  ucAlarmState;
    uint8_t  ucLiquidLevel;
    uint8_t  ucQtwSpeed;
    uint8_t  ucHaveTOC;
    uint8_t  ucAddData;
    uint8_t  ucWaterType;
    uint8_t  ucMainCtlState; // 2020/08/11 add
}APP_PACKET_ONLINE_NOTI_CONF_HANDLER_STRU;


typedef struct
{
    APP_PACKET_COMM_STRU hdr;
    uint8_t aucData[1];
}APP_PACKET_HEART_BEAT_REQ_STRU;

typedef struct
{
    APP_PACKET_COMM_STRU hdr;
    uint16_t usAddress;
}APP_PACKET_ZIGBEE_IND_STRU;


typedef struct
{
    APP_PACKET_COMM_STRU hdr;
    uint8_t ucRptType;
    uint8_t aucData[1];
}APP_PACKET_CLIENT_RPT_IND_STRU;

typedef struct
{
     uint8_t ucId;
     uint32_t ulValue;
}APP_PACKET_RPT_FM_STRU; // flow meter


typedef struct
{
    APP_PACKET_COMM_STRU hdr;
    uint8_t ucOpsType;   /* refer APP_PACKET_HAND_OPERATION_TYPE */
    uint8_t aucData[1];
}APP_PACKET_HO_STRU;

typedef struct
{
    uint8_t ucState;    /* refer: APP_PACKET_HS_STATE_ENUM */
    uint8_t ucResult;
    uint8_t ucAddData;  /*APP_PACKET_HS_STATE_QTW : TW flags    APP_PACKET_HS_STATE_CIR : CIR TYPE*/
    uint8_t ucAlarmState;
    uint8_t ucMainCtrlState;
}APP_PACKET_HO_STATE_STRU;


typedef enum
{
    APP_PACKET_HO_ACTION_STOP = 0,    //  CIRCULATION
    APP_PACKET_HO_ACTION_START  ,    //  Taking quantified water
        
}APP_PACKET_HO_ACTION_TYPE;


typedef struct
{    
     uint8_t ucAction;
}APP_PACKET_HO_CIR_REQ_STRU; // flow meter


typedef struct
{    
     uint8_t ucAction;
     uint8_t ucResult;
}APP_PACKET_HO_CIR_RSP_STRU; 

typedef struct
{    
     uint8_t  ucAction;
     uint32_t ulVolumn;        //  0XFFFFFFFF for normal takeing water, other: qtw
}APP_PACKET_HO_QTW_REQ_STRU; // flow meter

typedef struct
{    
     uint8_t ucAction;
     uint8_t ucResult;
     uint8_t ucUnit;
     uint16_t usPulseNum;  /* pulses/litre */
     uint32_t ulVolumn;     /* 0XFFFFFFFF for normal takeing water, other: qtw */
     uint8_t ucType;
}APP_PACKET_HO_QTW_RSP_STRU; 

typedef APP_PACKET_HO_CIR_REQ_STRU APP_PACKET_HO_SPEED_REQ_STRU;

typedef struct
{
    uint8_t ucAction;
    uint8_t ucType;    /* APP_PACKET_HO_SYS_TEST_TYPE_ENUM */
}APP_PACKET_HO_SYSTEMTEST_REQ_STRU;

typedef struct
{
    uint8_t ucAction;
    uint8_t ucType;    /* APP_PACKET_HO_SYS_TEST_TYPE_ENUM */
    uint8_t ucResult;
}APP_PACKET_HO_SYSTEMTEST_RSP_STRU;


typedef struct
{    
     uint8_t ucAction;
     uint8_t ucResult;
     uint8_t ucSpeed;
     uint8_t ucType; /* APP_DEV_HS_SUB_TYPE_ENUM */
}APP_PACKET_HO_SPEED_RSP_STRU;

enum APP_PACKET_HO_QL_TYPE_ENUM
{
    APP_PACKET_HO_QL_TYPE_LEVEL = 0x1,
    APP_PACKET_HO_QL_TYPE_PPB   = 0x2,
};


typedef struct
{    
     uint8_t ucMask;             /* bit0 for ucLevel, bit1 for ppb, others reserved */
     uint8_t ucLevel;
     float   fWaterQppb;
}APP_PACKET_HO_QL_UPD_STRU;

enum APP_PACKET_HO_ALARM_TYPE_ENUM
{
    APP_PACKET_HO_ALARM_TYPE_NOT = 0x1,
    APP_PACKET_HO_ALARM_TYPE_ALM = 0x2,
};

typedef struct
{    
     uint8_t ucMask;             /* bit0 for alarm, bit1 for notify, others reserved */
}APP_PACKET_HO_ALARM_STATE_NOT_STRU;


// 2020/07/30 add for HO extension
typedef struct
{    
     uint8_t  ucAction; /* 0 : OFF ,1 ON */
}APP_PACKET_HO_SYS_START_REQ_STRU; 

typedef APP_PACKET_HO_CIR_RSP_STRU APP_PACKET_HO_SYS_START_RSP_STRU; 

typedef struct
{    
     uint8_t  ucAction; /* 0 : OFF ,1 ON */
     uint8_t  ucType;   /* */
}APP_PACKET_HO_STERLIZE_REQ_STRU; 

typedef APP_PACKET_HO_CIR_RSP_STRU APP_PACKET_HO_STERLIZE_RSP_STRU; 

typedef struct
{    
     uint8_t  ucFirst;  /* 0 : consective ,1 first */
     uint8_t  ucDevType;   /* refer APP_DEV_HS_SUB_TYPE_ENUM */
}APP_PACKET_HO_RTI_QUERY_REQ_STRU; 

typedef struct
{    
     uint32_t  ulMap;          /* refer: APP_WATER_TYPE_ENUM */
     uint8_t  aucData[1];
}APP_PACKET_HO_RTI_QUERY_RSP_STRU; 

typedef struct
{    
     uint32_t  ulMap;                /* a Mapped for typed parameters */
}APP_PACKET_HO_CONFIG_QUERY_REQ_STRU; 

typedef struct
{    
     uint32_t  ulMap;             
     uint8_t   aucData[1];
}APP_PACKET_HO_CONFIG_QUERY_RSP_STRU; 


typedef APP_PACKET_HO_CONFIG_QUERY_RSP_STRU APP_PACKET_HO_CONFIG_SET_REQ_STRU;

typedef struct
{    
     uint8_t   ucResult;
}APP_PACKET_HO_CONFIG_SET_RSP_STRU; 

typedef struct
{    
     uint8_t   ucWashFlags;  // RO WASH 0x1,PH WASH 0x2, 0X0 IDLE,0X80 Ready
     uint32_t  aulLstCleanTime[2];
     uint32_t  aulPeriod[2];
}APP_PACKET_HO_STERLIZE_QUERY_RSP_STRU;

typedef struct
{    
     uint32_t   ulPart1Alarm;
     uint32_t   ulPart2Alarm;
}APP_PACKET_HO_ALARM_QUERY_RSP_STRU; 

typedef struct
{    
     uint32_t  aulMap[2];                /* a Mapped for typed parameters */
}APP_PACKET_HO_MACH_PARAM_QUERY_REQ_STRU; 


typedef struct
{    
     uint32_t  aulMap[2];                /* a Mapped for typed parameters */
     uint8_t   aucData[1];
}APP_PACKET_HO_MACH_PARAM_QUERY_RSP_STRU; 

typedef struct
{
    uint8_t ucLen;
    uint8_t aucData[1];
}APP_PACKET_HO_SYS_INFO_ITEM_STRU;

typedef struct
{
    uint8_t ucInit   ; // 0 for init install, other for general install
    uint8_t ucAccType; // accessory type
    uint8_t aucData[1];
}APP_PACKET_HO_ACCESSORY_INSTALL_START_STRU;

typedef struct
{
    uint8_t ucInit   ; // 0 for init install, other for general install
    uint8_t ucAccType; // accessory type
}APP_PACKET_HO_ACCESSORY_INSTALL_FINISH_STRU;

typedef struct
{
    char CATNO[APP_CAT_LENGTH+1];
    char LOTNO[APP_LOT_LENGTH+1];

}APP_PACKET_HO_CM_DETAIL_INFO_STRU;


typedef APP_PACKET_HO_CONFIG_QUERY_REQ_STRU APP_PACKET_HO_CM_INFO_QUERY_REQ_STRU;

typedef APP_PACKET_HO_CONFIG_QUERY_RSP_STRU APP_PACKET_HO_CM_INFO_QUERY_RSP_STRU;

typedef APP_PACKET_HO_CONFIG_QUERY_REQ_STRU APP_PACKET_HO_CM_RESET_STRU;

typedef APP_PACKET_HO_CONFIG_QUERY_RSP_STRU APP_PACKET_HO_CM_RESET_RSP_STRU;

typedef APP_PACKET_HO_CONFIG_QUERY_REQ_STRU APP_PACKET_HO_USAGE_QUERY_RSP_STRU;

typedef APP_PACKET_HO_CONFIG_QUERY_REQ_STRU APP_PACKET_HO_SYS_INFO_QUERY_REQ_STRU;

typedef APP_PACKET_HO_CONFIG_QUERY_RSP_STRU APP_PACKET_HO_SYS_INFO_QUERY_RSP_STRU;

typedef APP_PACKET_HO_CONFIG_QUERY_REQ_STRU APP_PACKET_HO_EXPORT_HISTORY_REQ_STRU;

typedef APP_PACKET_HO_CONFIG_SET_RSP_STRU  APP_PACKET_HO_EXPORT_HISTORY_RSP_STRU;

typedef APP_PACKET_HO_CONFIG_SET_RSP_STRU  APP_PACKET_HO_INSTALL_QRY_RSP_STRU;

typedef APP_PACKET_HO_CONFIG_QUERY_REQ_STRU APP_PACKET_HO_CM_DETAIL_QRY_REQ_STRU;

typedef APP_PACKET_HO_CONFIG_QUERY_RSP_STRU APP_PACKET_HO_CM_DETAIL_QRY_RSP_STRU;

// 2020/07/30 end for HO extension
typedef struct
{
    unsigned short usTemp;
    float          fWaterQ; /* Water Quality */
}APP_ECO_VALUE_STRU;

typedef struct
{
     uint8_t  ucId;
     APP_ECO_VALUE_STRU ev;
}APP_PACKET_RPT_ECO_STRU; // flow meter

typedef struct
{
     uint8_t  ucId;
     uint8_t  ucType;  /* refer: APP_OBJ_TYPE_ENUM */
     uint8_t  ucState;
}APP_PACKET_STATE_STRU; 

typedef struct
{
     uint8_t  ucState;
}APP_PACKET_RPT_DIN_STRU; 

typedef struct
{
    float          fB; 
    float          fP; 
}APP_PACKET_RPT_TOC_STRU; 

typedef struct
{
     uint32_t aulFm[APP_FM_FLOW_METER_NUM];
}APP_PACKET_ONLINE_NOTI_IND_FM_STRU;


typedef struct
{
     uint8_t ucHandsetAddr;
	 uint8_t aucSn[APP_SN_LENGTH];
     uint8_t ucDevType; // 2020/07/31 add
}APP_PACKET_ONLINE_NOTI_IND_HANDLE_STRU;

typedef struct
{
     uint16_t ausHoldRegs[APP_EXE_MAX_HOLD_REGISTERS];
}APP_PACKET_ONLINE_NOTI_IND_EXE_STRU;

typedef struct
{
     APP_PACKET_ONLINE_NOTI_IND_EXE_STRU exe;
     APP_PACKET_ONLINE_NOTI_IND_FM_STRU  fm;
}APP_PACKET_ONLINE_NOTI_IND_EXE_COMBINED_STRU;


typedef struct
{
    APP_PACKET_COMM_STRU hdr;
    uint8_t ucOpsType;   /* refer APP_PACKET_RF_OPERATION_TYPE */
    uint8_t aucData[1];
}APP_PACKET_RF_STRU;

typedef struct
{
	uint8_t ucLen;
    uint8_t ucOff;
}APP_PACKET_RF_READ_REQ_STRU;

typedef struct
{
	uint8_t ucLen;
    uint8_t ucOff;
    uint8_t aucData[1];
}APP_PACKET_RF_WRITE_REQ_STRU;

typedef struct
{
    uint8_t ucRslt;    
}APP_PACKET_RF_WRITE_RSP_STRU;

typedef struct
{
    uint8_t ucRslt;     
    uint8_t aucData[1];
}APP_PACKET_RF_READ_RSP_STRU;

typedef struct
{
    uint8_t ucLen;
    uint8_t ucOff;
    uint8_t aucData[1];
}APP_PACKET_RPT_RF_READ_CONT_STRU; // flow meter


typedef struct
{
     uint8_t  ucState;
     uint8_t  aucData[1];
}APP_PACKET_RPT_RF_STATE_STRU; // flow meter


typedef struct
{
     uint8_t  ucBlkNum;
     uint8_t  ucBlkSize;
     uint8_t  aucUid[8];
}APP_PACKET_RPT_RF_STATE_CONT_STRU; // flow meter


typedef struct
{
    APP_PACKET_COMM_STRU hdr;
    uint8_t ucOpsType;   /* refer APP_PACKET_RF_OPERATION_TYPE */
}APP_PACKET_RF_SEARCH_REQ_STRU;


typedef struct
{
	uint8_t ucLen;
    uint8_t aucData[1];
}APP_PACKET_RF_SEARCH_RSP_STRU;

typedef enum
{
    APP_PACKET_EXE_TOC_STAGE_FLUSH1 = 0,    //  FLUSH 1
    APP_PACKET_EXE_TOC_STAGE_OXDIZATION  ,  //  oxidize
    APP_PACKET_EXE_TOC_STAGE_FLUSH2      ,  //  FLUSH2
    APP_PACKET_EXE_TOC_STAGE_NUM         ,  // also used for exit TOC stage
}APP_PACKET_EXE_TOC_STAGE_ENUM;


/* TOC State */
typedef struct
{    
     uint8_t ucStage; /* refer APP_PACKET_EXE_TOC_STAGE_ENUM */
}APP_PACKET_EXE_TOC_REQ_STRU; 


typedef struct
{
    uint8_t ucRslt;    
}APP_PACKET_EXE_TOC_RSP_STRU;


typedef APP_PACKET_RPT_FM_STRU         APP_PACKET_RPT_RPUMP_STRU;
typedef APP_PACKET_RPT_FM_STRU         APP_PACKET_RPT_GPUMP_STRU;
typedef APP_PACKET_RPT_FM_STRU         APP_PACKET_RPT_EDI_STRU;
typedef APP_PACKET_RPT_FM_STRU         APP_PACKET_RPT_PM_STRU;
typedef APP_PACKET_RPT_FM_STRU         APP_PACKET_RPT_RECT_STRU;
typedef APP_PACKET_HEART_BEAT_REQ_STRU APP_PACKET_HEART_BEAT_RSP_STRU ;
typedef APP_PACKET_HEART_BEAT_REQ_STRU APP_PACKET_HOST_RESET_STRU ;
typedef APP_PACKET_HO_STRU             APP_PACKET_STATE_IND_STRU;
typedef APP_PACKET_HO_STRU             APP_PACKET_EXE_STRU;

#define APP_PROTOL_HEADER_LEN (sizeof(APP_PACKET_COMM_STRU))

#define APP_POROTOL_PACKET_ONLINE_NOTI_IND_TOTAL_LENGTH (sizeof(APP_PACKET_ONLINE_NOTI_IND_STRU))

#define APP_POROTOL_PACKET_ONLINE_NOTI_IND_PAYLOAD_LENGTH (APP_POROTOL_PACKET_ONLINE_NOTI_IND_TOTAL_LENGTH - APP_PROTOL_HEADER_LEN)

#define APP_POROTOL_PACKET_ONLINE_NOTI_CNF_TOTAL_LENGTH (sizeof(APP_PACKET_ONLINE_NOTI_CONF_STRU)-1)

#define APP_POROTOL_PACKET_ONLINE_NOTI_CNF_PAYLOAD_LENGTH (APP_POROTOL_PACKET_ONLINE_NOTI_CNF_TOTAL_LENGTH - APP_PROTOL_HEADER_LEN)

#define APP_POROTOL_PACKET_HEART_BEAT_RSP_TOTAL_LENGTH (sizeof(APP_PACKET_HEART_BEAT_RSP_STRU))

#define APP_POROTOL_PACKET_HEART_BEAT_RSP_PAYLOAD_LENGTH (APP_POROTOL_PACKET_HEART_BEAT_RSP_TOTAL_LENGTH - APP_PROTOL_HEADER_LEN)

#define APP_POROTOL_PACKET_HEART_BEAT_REQ_TOTAL_LENGTH (sizeof(APP_PACKET_HEART_BEAT_REQ_STRU))

#define APP_POROTOL_PACKET_HEART_BEAT_REQ_PAYLOAD_LENGTH (APP_POROTOL_PACKET_HEART_BEAT_REQ_TOTAL_LENGTH - APP_PROTOL_HEADER_LEN)

#define APP_POROTOL_PACKET_HOST_RESET_TOTAL_LENGTH (sizeof(APP_PACKET_HOST_RESET_STRU))

#define APP_POROTOL_PACKET_HOST_RESET_PAYLOAD_LENGTH (APP_POROTOL_PACKET_HOST_RESET_TOTAL_LENGTH - APP_PROTOL_HEADER_LEN)

#define APP_POROTOL_PACKET_CLIENT_RPT_IND_TOTAL_LENGTH (sizeof(APP_PACKET_CLIENT_RPT_IND_STRU) - 1)

#define APP_POROTOL_PACKET_CLIENT_RPT_IND_PAYLOAD_LENGTH (APP_POROTOL_PACKET_CLIENT_RPT_IND_TOTAL_LENGTH - APP_PROTOL_HEADER_LEN)

#define APP_POROTOL_PACKET_HO_COMMON_LENGTH (sizeof(APP_PACKET_HO_STRU) - 1)

#define APP_POROTOL_PACKET_RF_COMMON_LENGTH (sizeof(APP_PACKET_RF_STRU) - 1)

typedef enum
{
    MODBUS_FUNC_CODE_Read_Coil_Status = 0X1,
    MODBUS_FUNC_CODE_Read_Input_Status = 0X2,
    MODBUS_FUNC_CODE_Read_Holding_Registers = 0X3,
    MODBUS_FUNC_CODE_Read_Input_Registers   = 0X4,
    MODBUS_FUNC_CODE_Force_Single_Coil = 0x5,
    MODBUS_FUNC_CODE_Preset_Single_Register = 0x6,
    MODBUS_FUNC_CODE_Force_Multiple_Coils = 0xF,
    MODBUS_FUNC_CODE_Preset_Multiple_Registers = 0x10,
    MODBUS_FUNC_CODE_Mask_Write_0X_Register    = 0x80,
    MODBUS_FUNC_CODE_Mask_Write_4X_Register    = 0x81,

}MODBUS_FUNC_CODE_ENUM;


typedef struct
{
    uint8_t ucAddress;
    uint8_t ucFuncode;
    uint8_t aucData[1];
}MODBUS_PACKET_COMM_STRU;

#define MODBUS_PACKET_COMM_HDR_LEN (2)

#define INVALID_FM_VALUE (0xffffffff)


#pragma pack()

#endif

