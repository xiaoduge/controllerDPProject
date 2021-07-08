#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/times.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "log.h"

#ifdef __cplusplus
extern "C"
{
#endif
static char sSzBuffer[2048];

static int VosLogLevel = VOS_LOG_ERROR;
static int MLogLevel   = MLOG_DEBUG;

static int Logable = 0;

static int LogSeqNo = 0;

int VOS_logger(int level,const char *filename,int lineno,const char* fmt,...)
{
    if (level> VosLogLevel)
    {
        return 0;
    }
    {
    
        int  ret = 0;

        va_list   args;
        struct timeval now;
        struct  tm* tmNow;
        time_t tTime;
 
        va_start(args, fmt);

        gettimeofday(&now, NULL);   
        tTime = now.tv_sec;
        tmNow = localtime(&tTime);
        
        printf("[VOS_log FileName = %s LineNo = %05d %02d:%02d:%02d-%d]", filename,lineno,tmNow->tm_hour,tmNow->tm_min,tmNow->tm_sec,(int)(long)now.tv_usec/1000);
        vprintf(fmt,args);
        printf("\r\n");
  
        va_end(args);

        return  ret;
    }
}

void VOS_SetLogLevel(int iLevel)
{
    if (iLevel >=VOS_LOG_BUTT ||  iLevel < 0)
    {
        return ;
    }
    VosLogLevel = iLevel;
}


void VOS_ResetLogNo(void)
{
    time_t tTime;

    time(&tTime);
    
    LogSeqNo = tTime;
}

int User_logable(char *mount_string)
{
    FILE  *fp    = NULL;

    char cont[2048];
    
    fp = fopen("/proc/mounts","r");
    if (!fp)
    {
        printf("open /proc/mounts failed\r\n");
        return 0;
    }
    
    while (1) {
        memset(cont,0,sizeof(cont));
        if (!fgets(cont, sizeof(cont) - 1, fp)) {
            goto fail;
        }

        if(NULL != strstr(cont,mount_string))
        {
            goto succ;
        }
        
    }

succ:
    fclose(fp);
    return 1;

fail:
    fclose(fp);

    return 0;

}
static int bLogaleInit = 0;

int UserMan_LocalLog(const char *info)
{

    time_t tTime;
    struct  tm* tmNow;
    char szPath[128];
    char time_tag[128];
    char *p = szPath;
    FILE *fp;
    int iLen = 0;

#ifndef QT_PC
    if (!bLogaleInit)
    {
        bLogaleInit = 1;
         
        if (User_logable("/media/mmcblk0p1"))
        {
            Logable |= 0x2;
        }

        if (User_logable("/mnt/nfs"))
        {
            Logable |= 0x1;
        }
    }

    if (Logable & 0x1)
    {
        iLen = sprintf(p,"%s",".");
    }
    else if (Logable & 0x2)
    {
        iLen = sprintf(p,"%s","/media/mmcblk0p1");
    }
#else
    iLen = sprintf(p,"%s",".");
    if (!bLogaleInit)
    {
        bLogaleInit = 1;
        Logable = 1;
    }
#endif

    if (Logable)
    {
        struct timeval now;
        p += iLen;
        *p = '\0';

        gettimeofday(&now, NULL);   
        tTime = now.tv_sec;
        tmNow = localtime(&tTime);
        
        iLen = sprintf(p, "/%04d-%02d-%02d-%d.log", tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday,LogSeqNo);
        
        p += iLen;
        *p = '\0';
            
        fp = fopen(szPath, "a+t");
        if (fp == NULL)
        {
            printf("Open %s failed \r\n",szPath);
            return -1;
        }
        
        sprintf(time_tag,"%02d:%02d:%02d-%d",tmNow->tm_hour,tmNow->tm_min,tmNow->tm_sec,(int)(long)now.tv_usec/1000);
        
        fprintf(fp, "[%s] %s",time_tag, info);
        fputs("\n",fp);
        
        fclose(fp);
    }
    return 0;
}

int User_Log_Helper(int level,const char *filename,int lineno,const char *pszFormat, ...)
{
    va_list argList;

    int bufferIdx;
    
    char *szLogName = sSzBuffer;

    if (level > MLogLevel)
    {
        return 0;
    }

    bufferIdx = sprintf(szLogName, "[FileName = %s LineNo = %d]", filename,lineno);

    va_start(argList, pszFormat);
    vsprintf(szLogName + bufferIdx, pszFormat, argList);
    va_end(argList);

    UserMan_LocalLog(szLogName);
    
    return 0;
}

void User_Log_SetLogLevel(int iLevel)
{
    if (iLevel >= MLOG_BUTT ||  iLevel < 0)
    {
        return ;
    }
    MLogLevel = iLevel;
}

void VOS_ForceLog(int iLogFlag)
{
    if (iLogFlag)
    {
        bLogaleInit = 1;
        if (User_logable("/media/mmcblk0p1"))
        {
            Logable |= 0x2;
        }
        else
        {
            Logable |= 0x1;
        }
    }
    else
    {
        // let system to decide log option
        bLogaleInit = 0;
        Logable |= 0x0;
    }
}

#ifdef __cplusplus
}
#endif

