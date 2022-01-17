#ifndef _VOS_LOG_H_
#define _VOS_LOG_H_

#ifdef __cplusplus
extern "C"
{
#endif

/**
  ******************************************************************************
  * @file    voslog.h 
  * @author  VOS Team
  * @version V1.0.0
  * @date    10/10/2014
  * @brief   Interface for LOG access.
  ******************************************************************************
  *          DO NOT MODIFY ANY OF THE FOLLOWING CODE EXCEPT UNDER PERMISSION OF ITS PROPRIETOR
  * @copy
  *
  *
  * <h2><center>&copy; COPYRIGHT 2012 Shanghai ZhenNeng Corporation</center></h2>
  */ 


enum {
    VOS_LOG_ERROR    =  0,
    VOS_LOG_WARNING ,
    VOS_LOG_INFO  ,
    VOS_LOG_NOTICE ,
    VOS_LOG_DEBUG,
    VOS_LOG_BUTT,
};



enum {
    MLOG_ERROR    =  0,
    MLOG_WARNING ,
    MLOG_INFO  ,
    MLOG_NOTICE ,
    MLOG_DEBUG,
    MLOG_BUTT,
};

#define LOCAL_LOG
#ifdef LOCAL_LOG  //LocalLog
#define LOG(level,fmt, args...)   do { VOS_logger(level, __FILE__, __LINE__,fmt,##  args); User_Log_Helper(MLOG_DEBUG, __FILE__, __LINE__,fmt,##  args);\
    } while(0)

#else
#define LOG(level,fmt, args...)   do { VOS_logger(level, __FILE__, __LINE__,fmt,##  args);\
    } while(0)
#endif

#define VOS_LOG(level,fmt, args...)   VOS_logger(level, __FILE__, __LINE__,fmt,##  args)

#define User_Log(fmt, args...)        User_Log_Helper(MLOG_DEBUG, __FILE__, __LINE__,fmt,##  args)  

#define USER_LOG(level,fmt, args...)  User_Log_Helper(level, __FILE__, __LINE__,fmt,##  args)  

void VOS_SetLogLevel(int iLevel);

int VOS_logger(int level,const char *filename,int lineno,const char* fmt,...);

void VOS_ResetLogNo(void);

void VOS_ForceLog(int iLogFlag);


int User_Log_Helper(int level,const char *filename,int lineno,const char *pszFormat, ...);

void User_Log_SetLogLevel(int iLevel);

#ifdef __cplusplus
}
#endif

#endif

