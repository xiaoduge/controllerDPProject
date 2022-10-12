#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <libgen.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <libgen.h>
#include <fcntl.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <net/if.h>
#include <linux/rtnetlink.h>
#include <netinet/in.h>
#include <linux/rtc.h>


#include "ccb.h"
#include "canItf.h"
#include "msg.h"
#include "messageitf.h"
#include "memory.h"
#include "cminterface.h"
#include "Interface.h"
#include "DefaultParams.h"
#include "log.h"

#include "notify.h"
#include "exdisplay.h"
#include "MyParams.h"
#include "exconfig.h"
#include "config.h"
#include "mainwindow.h"

#include <QTimer>
#include <QDebug>

typedef void (* RUN_COMM_CALL_BACK )(int id);

TIMER_PARAM_STRU gTps;

TASK_HANDLE TASK_HANDLE_MAIN ;
TASK_HANDLE TASK_HANDLE_CANITF;
TASK_HANDLE TASK_HANDLE_MOCAN ;

unsigned int gulSecond = 0;

unsigned char gaucIapBuffer[1024];

#define HEART_BEAT_PERIOD (15)

#define HEART_BEAT_MARGIN (5)

#define GET_B_MASK(no) (1<<(no+APP_EXE_ECO_NUM))

#define MAKE_B_MASK(mask) (mask <<(APP_EXE_ECO_NUM))

#define GET_RECT_MASK(no) (1<<(no+APP_EXE_ECO_NUM + APP_EXE_PRESSURE_METER))

#define MAKE_RECT_MASK(mask) (mask <<(APP_EXE_ECO_NUM + APP_EXE_PRESSURE_METER))

#define is_B3_FULL(ulValue) ((ulValue / gGlobalParam.PmParam.afDepth[DISP_PM_PM3] * 100) >= B3_FULL)

#define is_SWPUMP_FRONT(pCcb) (gGlobalParam.MiscParam.ulMisFlags & (1 << DISP_SM_SW_PUMP))

static const float s_fTankHOffset = 0.08; //水箱高度偏差

typedef enum
{
    WORK_MSG_TYPE_START_QTW = 0,
    WORK_MSG_TYPE_STOP_QTW,
    WORK_MSG_TYPE_INIT_RUN,
    WORK_MSG_TYPE_WASH,
    WORK_MSG_TYPE_RUN,
    
    WORK_MSG_TYPE_SFW, /* Start fill water */
    WORK_MSG_TYPE_EFW, /* End fill water */

    WORK_MSG_TYPE_SCIR, /* Start circulation     */
    WORK_MSG_TYPE_ECIR, /* End circulation water */

    WORK_MSG_TYPE_SN3, /* Start N3     */
    WORK_MSG_TYPE_EN3, /* End   N3 */

    WORK_MSG_TYPE_SKH,  /* Start Key Handle */
    WORK_MSG_TYPE_EKH,  /* End Key Handle */

    WORK_MSG_TYPE_SLH,  /* Start Leak Handle */
    WORK_MSG_TYPE_ELH,  /* End Leak Handle */

    WORK_MSG_TYPE_EPW,  /* End  */

    WORK_MSG_TYPE_ESR,  /* End of Speed Regulation */

    WORK_MSG_TYPE_IDLE,  /* Return to Idle */
    

    WORK_MSG_TYPE_STATE = 0x80,  /* State Changed */

    WORK_MSG_TYPE_STATE4PW
    
}WORK_MSG_TYPE_ENUM;

const char *notify_info[] =
{
    "start taking water succ",
    "start taking water fail",
    "stop taking water succ",
    "",
    "init run succ",
    "init run fail",
    "wash succ",
    "wash fail",
    "run succ",
    "run fail",
    "start filling tank",
    "fill tank fail",
    "stop filling tank ",
    "",
    "start circulation",
    "circulation fail",
    "stop circulation ",
    "",
    "start n3",
    "n3 fail",
    "stop n3 ",
    "",
    "start key handle succ",
    "start key handle fail",
    "stop key handle ",
    "",
    "return to idle ",
    "",
};

// forward declaration
extern QString MainGetSysInfo(int iType);
extern int copyHistoryToUsb(QDateTime& startTime, QDateTime& endTime);

// end 

void printhex(unsigned char *buf,int len)
{
    int iLoop ;    
    for (iLoop = 0; iLoop < len;iLoop++)  
    {      

        if (0 == iLoop % 16)
        {
            printf("\r\n"); 
        }

        printf("0x%02x, ",buf[iLoop]); 

    }   
    printf("\r\n"); 
    
}

int MakeThdState(int id,int flag)
{
    return flag << (id*2);
}

int GetThdState(int id,int flag)
{
    return (flag >> (id*2)) & ((1 << 2)-1);
}

int CcbSndCanCmd(int iChl,unsigned int ulCanId,unsigned char ucCmd0,unsigned char ucCmd1,unsigned char *data, int iPayLoadLen)
{
    int iRet;
    int iCanMsgLen = iPayLoadLen + RPC_FRAME_HDR_SZ + RPC_UART_FRAME_OVHD;
    unsigned char        *sbBuf ;
    MAIN_CANITF_MSG_STRU *pMsg = (MAIN_CANITF_MSG_STRU *)SatAllocMsg(MAIN_CANITF_MSG_SIZE + iCanMsgLen);//malloc(sizeof(TIMER_MSG_STRU));
    if (pMsg)
    {
        unsigned char ucFcs = 0;
        int           iLen;
        int           iIdx;

        pMsg->msgHead.event = MAIN_CANITF_MSG;
        pMsg->ulCanId       = ulCanId;
        pMsg->iCanChl       = iChl;
        pMsg->iMsgLen       = iCanMsgLen;
        pMsg->aucData[0]    = RPC_UART_SOF;
        
        sbBuf = pMsg->aucData + 1;
        sbBuf[RPC_POS_LEN]  = iPayLoadLen; // iLen for data area (NOT INCLUDE CMD0&CMD1&LEN itself)
        sbBuf[RPC_POS_CMD0] = ucCmd0;
        sbBuf[RPC_POS_CMD1] = ucCmd1;//SAPP_CMD_DATA;
        memcpy(&sbBuf[RPC_POS_DAT0],data,iPayLoadLen);
   
        iLen = sbBuf[RPC_POS_LEN] + RPC_FRAME_HDR_SZ;
        for ( iIdx = RPC_POS_LEN; iIdx < iLen; iIdx++)
        {
          ucFcs ^= sbBuf[iIdx];
        }
        sbBuf[iLen] = ucFcs;
       
        iRet = VOS_SndMsg(TASK_HANDLE_CANITF,(SAT_MSG_HEAD *)pMsg);
        if (0 != iRet)
        {
            return -1;
        }

        return 0;
    }

    return -1;

}



int CcbSndAfCanCmd(int iChl,unsigned int ulCanId,unsigned char ucCmd,unsigned char *data, int iPayLoadLen)
{
    return CcbSndCanCmd(iChl,ulCanId,RPC_SYS_AF,ucCmd,data,iPayLoadLen);
}

int CcbSndIapCanCmd(int iChl,unsigned int ulCanId,unsigned char ucCmd,unsigned char *data, int iPayLoadLen)
{
    return CcbSndCanCmd(iChl,ulCanId,RPC_SYS_BOOT,ucCmd,data,iPayLoadLen);
}

int get_net_itf(char *itf,char *ipaddr,char *mask)
{
    int i    = 0;
    int iCnt = 0;
    int sockfd;
    struct ifconf ifconf;
    char buf[512];
    struct ifreq *ifreq;

    ifconf.ifc_len = sizeof(buf);
    ifconf.ifc_buf = (caddr_t)buf;

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0))<0)
    {
        perror("socket");
        return 0;
    }  
    ioctl(sockfd, SIOCGIFCONF, &ifconf); 
    ifreq = (struct ifreq*)buf;  
    for(i=(ifconf.ifc_len/sizeof(struct ifreq)); i > 0; i--,ifreq++)
    {
       struct ifreq tmp;  
       
       if (strncmp(ifreq->ifr_name,itf,strlen(itf)))
       {
           continue;
       }

       strcpy(ipaddr,inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr));

       bzero(&tmp,sizeof(struct ifreq));    
       memcpy(tmp.ifr_name, ifreq->ifr_name, sizeof(tmp.ifr_name));    
       ioctl(sockfd, SIOCGIFNETMASK, &tmp );

       strcpy(mask,inet_ntoa(((struct sockaddr_in*)&(tmp.ifr_addr))->sin_addr));

       iCnt++;
       break;
    }
    
    close(sockfd);
    
    return iCnt;
}

#define BUFSIZE 8192   

struct route_info{   
unsigned int dstAddr;   
unsigned int srcAddr;   
unsigned int gateWay;   
char ifName[IF_NAMESIZE];   
}; 

int readNlSock(int sockFd, char *bufPtr, unsigned int seqNum, int pId)
{
    struct nlmsghdr *nlHdr;
    unsigned int readLen = 0, msgLen = 0;
    do
    {
        if((readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0)) < 0)
        {
          return -1;
        }
    
        nlHdr = (struct nlmsghdr *)bufPtr;
        if((NLMSG_OK(nlHdr, readLen) == 0) || (nlHdr->nlmsg_type == NLMSG_ERROR))
        {
          return -1;
        }
        if(nlHdr->nlmsg_type == NLMSG_DONE)
        {
          break;
        }
        else
        {
    
          bufPtr += readLen;
          msgLen += readLen;
        }
        if((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0)
        {
    
         break;
        }
    } 
    while((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != (unsigned int)pId));
    return msgLen;
}

void parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo,char gateway[])
{
      struct rtmsg *rtMsg;
      struct rtattr *rtAttr;
      int rtLen;
      char *tempBuf = NULL;
      struct in_addr dst;
      struct in_addr gate;
      tempBuf = (char *)malloc(100);
      rtMsg = (struct rtmsg *)NLMSG_DATA(nlHdr);
      // If the route is not for AF_INET or does not belong to main routing table
      //then return.
      if((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))
      return;

      rtAttr = (struct rtattr *)RTM_RTA(rtMsg);
      rtLen = RTM_PAYLOAD(nlHdr);
      for(;RTA_OK(rtAttr,rtLen);rtAttr = RTA_NEXT(rtAttr,rtLen)){
       switch(rtAttr->rta_type) {
           case RTA_OIF:
            if_indextoname(*(int *)RTA_DATA(rtAttr), rtInfo->ifName);
            break;
           case RTA_GATEWAY:
            rtInfo->gateWay = *(u_int *)RTA_DATA(rtAttr);
            break;
           case RTA_PREFSRC:
            rtInfo->srcAddr = *(u_int *)RTA_DATA(rtAttr);
            break;
           case RTA_DST:
            rtInfo->dstAddr = *(u_int *)RTA_DATA(rtAttr);
            break;
       }
      }
      dst.s_addr = rtInfo->dstAddr;
      if (strstr((char *)inet_ntoa(dst), "0.0.0.0"))
      {
        gate.s_addr = rtInfo->gateWay;
        sprintf(gateway, "%s",(char *)inet_ntoa(gate));
        gate.s_addr = rtInfo->srcAddr;
        gate.s_addr = rtInfo->dstAddr;
      }
      free(tempBuf);
      return;
}

int get_gateway(char gateway[])
{
     struct nlmsghdr *nlMsg;
//     struct rtmsg *rtMsg;
     struct route_info *rtInfo;
     char msgBuf[BUFSIZE];

     int sock;
     unsigned int len, msgSeq = 0;

     if((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)
     {
      return -1;
     }
     memset(msgBuf, 0, BUFSIZE);
     nlMsg = (struct nlmsghdr *)msgBuf;
//     rtMsg = (struct rtmsg *)NLMSG_DATA(nlMsg);
     nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.
     nlMsg->nlmsg_type = RTM_GETROUTE; // Get the routes from kernel routing table .
     nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.
     nlMsg->nlmsg_seq = msgSeq++; // Sequence of the message packet.
     nlMsg->nlmsg_pid = getpid(); // PID of process sending the request.
     if(send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0){
      return -1;
     }
     if((len = readNlSock(sock, msgBuf, msgSeq, getpid())) < 0) {
      return -1;
     }

     rtInfo = (struct route_info *)malloc(sizeof(struct route_info));
     for(;NLMSG_OK(nlMsg,len);nlMsg = NLMSG_NEXT(nlMsg,len)){
      memset(rtInfo, 0, sizeof(struct route_info));
      parseRoutes(nlMsg, rtInfo,gateway);
     }
     free(rtInfo);
     close(sock);
     return 0;
}


char* skip_whitespace(const char *s)
{
	/* In POSIX/C locale (the only locale we care about: do we REALLY want
	 * to allow Unicode whitespace in, say, .conf files? nuts!)
	 * isspace is only these chars: "\t\n\v\f\r" and space.
	 * "\t\n\v\f\r" happen to have ASCII codes 9,10,11,12,13.
	 * Use that.
	 */
	while (*s == ' ' || (unsigned char)(*s - 9) <= (13 - 9))
		s++;

	return (char *) s;
}

char* skip_non_whitespace(const char *s)
{
	while (*s != '\0' && *s != ' ' && (unsigned char)(*s - 9) > (13 - 9))
		s++;

	return (char *) s;
}

/* Returns pointer to the next word, or NULL.
 * In 1st case, advances *buf to the word after this one.
 */
static char *next_word(char **buf)
{
	unsigned length;
	char *word;

	/* Skip over leading whitespace */
	word = skip_whitespace(*buf);

	/* Stop on EOL */
	if (*word == '\0')
		return NULL;

	/* Find the length of this word (can't be 0) */
	length = strcspn(word, " \t\n");

	/* Unless we are already at NUL, store NUL and advance */
	if (word[length] != '\0')
		word[length++] = '\0';

	*buf = skip_whitespace(word + length);

	return word;
}

void parseNetInterface(NIF *nif)
{
    char buf[512];

	const char *interfaces = "/etc/network/interfaces";
    
	enum { NONE, IFACE, MAPPING } currently_processing = NONE;
    
	FILE *fp;

	char *first_word;
	char *rest_of_line;
    
    fp = fopen(interfaces, "r");

    if (!fp)
    {
        return;
    }

    while(fgets(buf,511,fp))
    {
		rest_of_line = buf;
		first_word = next_word(&rest_of_line);
		if (!first_word || *first_word == '#') {
			continue; /* blank/comment line */
		}

        if (strcmp(first_word, "mapping") == 0) {
            
            currently_processing = IFACE;
        }
        else if (strcmp(first_word, "auto") == 0)
        {   
            currently_processing = NONE;
        }
		else if (strcmp(first_word, "iface") == 0) 
        {
			char *iface_name;
			char *address_family_name;
			char *method_name;

            /* ylf: iface eth0 inet static */
			iface_name = next_word(&rest_of_line);
			address_family_name = next_word(&rest_of_line);
			method_name = next_word(&rest_of_line);

			/* ship any trailing whitespace */
			rest_of_line = skip_whitespace(rest_of_line);

            (void)address_family_name;
            if (0 == strcmp(iface_name,"eth0"))
            {
		        currently_processing = IFACE;
                if (0 == strcmp(method_name,"dhcp"))
                {
                    nif->dhcp = 1;            
                }
                else
                {
                    nif->dhcp = 0;            
                }
            }
		}
        else 
        {
			switch (currently_processing) 
            {
			case IFACE:
				if (rest_of_line[0] != '\0')
                {
                    char *name = first_word;
                    char *value = next_word(&rest_of_line);

                    if (strcmp(name, "address") == 0)
                    {
                        strcpy(nif->ip,value);
                    }
                    else if (strcmp(name, "netmask") == 0)
                    {
                        strcpy(nif->mask,value);
                    }
                    else if (strcmp(name, "gateway") == 0)
                    {
                        strcpy(nif->gateway,value);
                    }
                }
				break;
			case NONE:
                break;
			default:
                break;
			}
		}        
    }

    fclose(fp);
}

void ChangeNetworkConfig(NIF *nif)
{
    const char *interfaces   = "/etc/network/interfaces";
    const char *interfaces_d = "/etc/network/interfaces_dhcp";
    const char *interfaces_s = "/etc/network/interfaces_static";

    if (nif->dhcp)
    {
       FILE *fp = fopen(interfaces_d,"w");
       if (fp)
       {
           char cmd[256];
       
           fputs("# Configure Loopback\n", fp);
           fputs("auto lo\n", fp);
           fputs("iface lo inet loopback\n", fp);
           fputs("\n", fp);
           fputs("auto eth0\n", fp);
           fputs("iface eth0 inet dhcp\n", fp);
           fclose(fp);

           sprintf(cmd,"mv %s %s",interfaces_d,interfaces);

           if (system(cmd))
           {
           }
           sync();
       }
       return ;
    }
    else
    {
       FILE *fp = fopen(interfaces_s,"w");
       if (fp)
       {
           char cmd[256];
       
           fputs("# Configure Loopback\n", fp);
           fputs("auto lo\n", fp);
           fputs("iface lo inet loopback\n", fp);
           fputs("\n", fp);
           fputs("auto eth0\n", fp);
           fputs("iface eth0 inet static\n", fp);

           sprintf(cmd,"address %s\n",nif->ip);
           fputs(cmd, fp);
           sprintf(cmd,"netmask %s\n",nif->mask);
           fputs(cmd, fp);
           sprintf(cmd,"gateway %s\n",nif->gateway);
           fputs(cmd, fp);

           sprintf(cmd,"nameserver %s\n",nif->gateway);
           fputs(cmd, fp);
           
           fclose(fp);

           sprintf(cmd,"mv %s %s",interfaces_s,interfaces);

           if (system(cmd))
           {
           }
           sync();
       }    
    }
}

// inner member
ConsumableInstaller::ConsumableInstaller(int id) :
    m_iID(id)
{
    // MUST installed for Init Setup
    initOtherConfig();
    initPackConfig();

    m_iType = 0XFF;
}

void ConsumableInstaller::emitInstallMsg()
{
    emit installConusumable();
}

void ConsumableInstaller::updateConsumableInstall(int type)
{
    CCB *pCcb = CCB::getInstance();
 
    // notify handler install finished
    if ((1 << type) & m_aiCmMask[Type0])
    {
        m_aiCmCfgedMask[Type0] |= (1 << type);
    }
    else if ((1 << type) & m_aiCmMask[Type1])
    {
        m_aiCmCfgedMask[Type1] |= (1 << type);
    }
    else
    {
        //return ;
    }

    pCcb->CanCcbSndAccessoryInstallFinishMsg(type,m_iID);

    m_iType = 0XFF;
    
    // notify handle,current catriger installed successfully
}

void ConsumableInstaller::setConsumableName(int iType, const QString &catNo, const QString &lotNo)
{
    emitInstallMsg();
#if 0
    CCB *pCcb = CCB::getInstance();

    m_iType = iType;
    m_CatNo = catNo;
    m_LotNo = lotNo;

    m_strPack = pCcb->accessoryName(iType);

    // notify handler a new pack & machinery waiting to be installed
    pCcb->CanCcbSndAccessoryInstallStartMsg(m_iType,m_CatNo,m_LotNo,m_iID);

#endif
    LOG(VOS_LOG_DEBUG,"Prepare to install CAT %s & LOT %s",m_CatNo.toLocal8Bit().data(),m_LotNo.toLocal8Bit().data());

}


void ConsumableInstaller::initPackConfig()
{
    Consumable_Install_Info install_info;

    m_aiCmMask[Type0]      = 0;
    m_aiCmCfgedMask[Type0] = 0;
    m_map[Type0].clear();

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_PURIST:
    case MACHINE_C_D:
        install_info.iRfid = APP_RFID_SUB_TYPE_UPACK_HPACK;
        install_info.strName = tr("U Pack");
        m_map[Type0].insert(DISP_U_PACK, install_info);
        m_aiCmMask[Type0] |= 1 << DISP_U_PACK;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_PURIST:
    case MACHINE_C_D:
    case MACHINE_RO_H:
        install_info.iRfid = APP_RFID_SUB_TYPE_HPACK_ATPACK;
        install_info.strName = tr("H Pack");
        m_map[Type0].insert(DISP_H_PACK, install_info);
        m_aiCmMask[Type0] |= 1 << DISP_H_PACK;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_RO_H:
    case MACHINE_C:
        install_info.iRfid = APP_RFID_SUB_TYPE_PPACK_CLEANPACK;
        install_info.strName = tr("P Pack");
        m_map[Type0].insert(DISP_P_PACK, install_info);
        m_aiCmMask[Type0] |= 1 << DISP_P_PACK;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_EDI:
        install_info.iRfid = APP_RFID_SUB_TYPE_ROPACK_OTHERS;
        install_info.strName = tr("AC Pack");
        m_map[Type0].insert(DISP_AC_PACK, install_info);
        m_aiCmMask[Type0] |= 1 << DISP_AC_PACK;
        break;
    default:
        break;
    }
}

void ConsumableInstaller::initOtherConfig()
{
    Consumable_Install_Info install_info;

    m_aiCmMask[Type1]      = 0;
    m_aiCmCfgedMask[Type1] = 0;
    m_map[Type1].clear();

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_RO_H:
    case MACHINE_C:
        install_info.iRfid = APP_RFID_SUB_TYPE_ROPACK_OTHERS;
        install_info.strName = tr("RO Membrane");
        m_map[Type1].insert(DISP_MACHINERY_RO_MEMBRANE, install_info);
        m_aiCmMask[Type1] |= 1 << DISP_MACHINERY_RO_MEMBRANE;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_PURIST:
    case MACHINE_C_D:
        install_info.iRfid = APP_RFID_SUB_TYPE_ROPACK_OTHERS;
        if(0 == gAdditionalCfgParam.productInfo.iCompany)
        {
             install_info.strName = tr("Final Fliter B");
        }
        else
        {
            install_info.strName = tr("Bio-filter");
        }
        m_map[Type1].insert(DISP_T_B_FILTER, install_info);
        m_aiCmMask[Type1] |= 1 << DISP_T_B_FILTER;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_RO_H:
    case MACHINE_C:
    case MACHINE_PURIST:
    case MACHINE_C_D:
        install_info.iRfid = APP_RFID_SUB_TYPE_ROPACK_OTHERS;
        install_info.strName = tr("Final Fliter A"); //0.2um
        m_map[Type1].insert(DISP_T_A_FILTER, install_info);
        m_aiCmMask[Type1] |= 1 << DISP_T_A_FILTER;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_EDI:
    case MACHINE_C_D:
        install_info.iRfid = APP_RFID_SUB_TYPE_ROPACK_OTHERS;
        install_info.strName = tr("254 UV Lamp");
        m_map[Type1].insert(DISP_N1_UV, install_info);
        m_aiCmMask[Type1] |= 1 << DISP_N1_UV;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_PURIST:
        install_info.iRfid = APP_RFID_SUB_TYPE_ROPACK_OTHERS;
        install_info.strName = tr("185 UV Lamp");
        m_map[Type1].insert(DISP_N2_UV, install_info);
        m_aiCmMask[Type1] |= 1 << DISP_N2_UV;
        break;
    }

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_RO_H:
    case MACHINE_C:
        install_info.iRfid = APP_RFID_SUB_TYPE_ROPACK_OTHERS;
        install_info.strName = tr("Tank UV Lamp");
        m_map[Type1].insert(DISP_N3_UV, install_info);
        m_aiCmMask[Type1] |= 1 << DISP_N3_UV;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_RO_H:
    case MACHINE_C:
        install_info.iRfid = APP_RFID_SUB_TYPE_ROPACK_OTHERS;
        install_info.strName = tr("Tank Vent Filter");
        m_map[Type1].insert(DISP_W_FILTER, install_info);
        m_aiCmMask[Type1] |= 1 << DISP_W_FILTER;
        break;
    default:
        break;
    }
}

QString CCB::accessoryName(int iType)
{
    QString m_strPack;
    
    switch(iType)
    {
    case DISP_P_PACK:
        m_strPack = "P Pack";
        break;
    case DISP_AC_PACK:
        m_strPack = "AC Pack";
        break;
    case DISP_U_PACK:
         m_strPack = "U Pack";
        break;
    case DISP_H_PACK:
         m_strPack = "H Pack";
        break;
    case DISP_PRE_PACK:
         m_strPack = "Prefilter";
        break;
    case DISP_P_PACK | (1 << 16):
         m_strPack = "Clean Pack";
        break;
    case DISP_AT_PACK:
         m_strPack = "AT Pack";
        break;
    case DISP_T_PACK:
         m_strPack = "T Pack";
        break;
    case DISP_MACHINERY_RO_MEMBRANE:
         m_strPack = "RO Membrane";
        break;
    case DISP_N2_UV:
         m_strPack = "185 UV Lamp";
        break;
    case DISP_N1_UV:
         m_strPack = "254 UV Lamp";
        break;
    case DISP_N3_UV:
         m_strPack = "Tank UV Lamp";
        break;
    case DISP_MACHINERY_RO_BOOSTER_PUMP:
         m_strPack = "RO Pump";
        break;
    case DISP_MACHINERY_CIR_PUMP:
         m_strPack = "Recir. Pump";
        break;
    case DISP_T_A_FILTER:
         m_strPack = "Final Fliter A";
        break;
    case DISP_T_B_FILTER:
        if(0 == gAdditionalCfgParam.productInfo.iCompany)
        {
              m_strPack = "Final Fliter B";
        }
        else
        {
             m_strPack = "Bio-filter";
        }
        break;
    case DISP_MACHINERY_EDI:
         m_strPack = "EDI Module";
        break;
    case DISP_W_FILTER:
         m_strPack = "Tank Vent Filter";
        break;
    case DISP_TUBE_FILTERLIFE:
         m_strPack = "Loop Filter";
        break;
    case DISP_N4_UV:
         m_strPack = "Tube UV";
        break;
    case DISP_TUBE_DI:
         m_strPack = "Loop DI";
    default:
        m_strPack = "unknow";
        break;
    }
    return m_strPack;
}

int CCB::is_B2_FULL(unsigned int ulValue)
{
    if(DISP_WATER_BARREL_TYPE_UDF == gGlobalParam.PmParam.aiBuckType[DISP_PM_PM2])
    {
        return ((ulValue / gGlobalParam.PmParam.afDepth[DISP_PM_PM2] * 100) >= B2_FULL);
    }
    else
    {
        float correction = s_fTankHOffset;
        if(ulValue > correction)
        {
            ulValue = ulValue - correction;
        }
        else
        {
            ulValue = 0;
        }
        return ((ulValue / (gGlobalParam.PmParam.afDepth[DISP_PM_PM2] - correction) * 100) >= B2_FULL);
    }

}


void CCB::CcbSetActiveMask(int iSrcAddr)
{
    ulActiveMask   |= (1 << iSrcAddr);
    if (iSrcAddr < MAX_HB_CHECK_ITEMS) aucHbCounts[iSrcAddr] = MAX_HB_CHECK_COUNT;    
}

void CCB::CcbDefaultModbusCallBack(CCB *pCcb,int code,void *param)
{
    MODBUS_PACKET_COMM_STRU *pmg = (MODBUS_PACKET_COMM_STRU *)param;

    int iContLen = 0;

    if (ERROR_CODE_SUCC == code)
    {
        if (pmg)
        {
            switch(pmg->ucFuncode)
            {
            case MODBUS_FUNC_CODE_Read_Coil_Status:
            case MODBUS_FUNC_CODE_Read_Input_Status:
            case MODBUS_FUNC_CODE_Read_Holding_Registers:
            case MODBUS_FUNC_CODE_Read_Input_Registers:
                iContLen = pmg->aucData[0] + 2;
                break;
            case MODBUS_FUNC_CODE_Force_Single_Coil:
            case MODBUS_FUNC_CODE_Preset_Single_Register:
            case MODBUS_FUNC_CODE_Force_Multiple_Coils:
            case MODBUS_FUNC_CODE_Preset_Multiple_Registers:
                iContLen = 5;
                break;
            case MODBUS_FUNC_CODE_Mask_Write_0X_Register:
            case MODBUS_FUNC_CODE_Mask_Write_4X_Register:
                iContLen = 7;
                break;
            }

            if (0 == iContLen)
            {
                return;
            }

            if (iContLen <= MAX_MODBUS_BUFFER_SIZE)
            {
                memcpy(pCcb->aucModbusBuffer,(unsigned char *)pmg,iContLen);
            }
        }
    }
}

int CCB::DispGetUpCirFlag()
{
    if (DISP_WORK_STATE_RUN == curWorkState.iMainWorkState4Pw
        && DISP_WORK_SUB_RUN_CIR == curWorkState.iSubWorkState4Pw)
    {
       return (CIR_TYPE_UP == iCirType);
    }

    return 0;
}

int CCB::DispGetTankCirFlag()
{
    if (DISP_WORK_STATE_RUN == curWorkState.iMainWorkState4Pw
        && DISP_WORK_SUB_RUN_CIR == curWorkState.iSubWorkState4Pw)
    {
       return (CIR_TYPE_HP == iCirType);
    }

    return 0;

}

int CCB::DispGetTocCirFlag()
{
    if (DISP_WORK_STATE_RUN == curWorkState.iMainWorkState4Pw
        && DISP_WORK_SUB_RUN_CIR == curWorkState.iSubWorkState4Pw)
    {
        return bit1TocOngoing;
    }

    return 0;
}

int CCB::DispGetUpQtwFlag()
{
    int iLoop;

    for (iLoop = 0; iLoop < MAX_HANDLER_NUM; iLoop++)
    {
        if (aHandler[iLoop].bit1Qtw 
            && APP_DEV_HS_SUB_UP == aHandler[iLoop].iDevType) 
        {
            return 1;
        }
    }

    return 0;
}

int CCB::DispGetEdiQtwFlag()
{
    int iLoop;

    for (iLoop = 0; iLoop < MAX_HANDLER_NUM; iLoop++)
    {
        if (aHandler[iLoop].bit1Qtw 
            && APP_DEV_HS_SUB_HP == aHandler[iLoop].iDevType) 
        {
            return 1;
        }
    }

    return 0;

}

int CCB::DispGetROWashFlag()
{
    if (DISP_WORK_STATE_IDLE == curWorkState.iMainWorkState
        && DISP_WORK_SUB_WASH == curWorkState.iSubWorkState)
    {
       return 1;
    }

    return 0;
}

int CCB::DispGetRoWashPauseFlag()
{
    if (DISP_WORK_STATE_IDLE == curWorkState.iMainWorkState
        && DISP_WORK_SUB_WASH == curWorkState.iSubWorkState)
    {
       return bit1ROWashPause;
    }

    return 0;
}

int CCB::DispGetWashFlags()
{
    if (DISP_WORK_STATE_IDLE == curWorkState.iMainWorkState
        && DISP_WORK_SUB_WASH == curWorkState.iSubWorkState)
    {
       return (1 << iWashType);
    }

    return 0;
}



int CCB::DispGetREJ(float *pfValue)
{
    if (DISP_WORK_STATE_RUN == curWorkState.iMainWorkState)
    {
       *pfValue = CcbCalcREJ();
       
       return (1);
    }

    return 0;

}

int CCB::DispGetSwitchState(int ulMask)
{
   return CcbGetSwitchObjState(ulMask);
}

int CCB::DispGetSingleSwitchState(int iChlNo)
{
   return !!(ExeBrd.aValveObjs[iChlNo-APP_EXE_E1_NO].iActive & DISP_ACT_TYPE_SWITCH) ;
}

int CCB::DispGetDinState()
{
   int usState = ExeBrd.ucDinState;

   if (ExeBrd.ucLeakState)
   {
        usState |= 1 << APP_EXE_DIN_NUM;
   }

   return usState;
}

APP_ECO_VALUE_STRU& CCB::DispGetEco(int iIdx)
{
   return ExeBrd.aEcoObjs[iIdx].Value.eV;
}


void CCB::DispGetOtherCurrent(int iChlNo,int *pmA)
{
   switch(iChlNo)
   {
   case APP_EXE_N1_NO ... APP_EXE_N3_NO:
       *pmA = CcbConvert2RectAndEdiData(ExeBrd.aRectObjs[iChlNo-APP_EXE_N1_NO].Value.ulV);
       break;
   case APP_EXE_T1_NO:
       *pmA = CcbConvert2RectAndEdiData(ExeBrd.aEDIObjs[iChlNo-APP_EXE_T1_NO].Value.ulV);
       break;
   case APP_EXE_C3_NO ... APP_EXE_C4_NO:
       *pmA = CcbConvert2GPumpData(ExeBrd.aGPumpObjs[iChlNo-APP_EXE_T1_NO].Value.ulV);
       break;
   default:
       return ;
   }
}

void CCB::DispGetRPumpRti(int iChlNo,int *pmV,int *pmA)
{
   if (iChlNo >= APP_EXE_C1_NO && iChlNo <= APP_EXE_C2_NO )
   {
       unsigned int ulTemp = ExeBrd.aRPumpObjs[iChlNo-APP_EXE_C1_NO].Value.ulV;
       
       *pmA = CcbConvert2RPumpIData(ulTemp);

       *pmV = DispConvertRPumpSpeed2Voltage(ExeBrd.ausHoldRegister[APP_EXE_HOLD_REG_RPUMP_OFFSET + iChlNo - APP_EXE_C1_NO]);
   
   }
}

int CCB::CcbGetTwFlag()
{
    int iLoop;

    int iFlags = 0;

    for (iLoop = 0; iLoop < MAX_HANDLER_NUM; iLoop++)
    {
        if (aHandler[iLoop].bit1Qtw) iFlags |= 1 << iLoop;
    }

    return iFlags;
}

int CCB::CcbGetTwPendingFlag()
{
    int iLoop;

    int iFlags = 0;

    for (iLoop = 0; iLoop < MAX_HANDLER_NUM; iLoop++)
    {
        if (aHandler[iLoop].bit1PendingQtw) iFlags |= 1 << iLoop;
    }

    return iFlags;
    
}

int CCB::DispGetInitRunFlag()
{
    if (DISP_WORK_STATE_PREPARE == curWorkState.iMainWorkState
        && DISP_WORK_SUB_IDLE == curWorkState.iSubWorkState)
    {
       return 1;
    }

    return 0;
}

int CCB::DispGetTubeCirFlag()
{
   return !!bit1TubeCirOngoing;
}

int CCB::DispGetPwFlag()
{
   return !!bit1ProduceWater;
}

int CCB::DispGetFwFlag()
{
   return !!bit1FillingTank;
}

int CCB::DispGetTankFullLoopFlag()
{
   return !!bit1PeriodFlushing;
}

int CCB::DispGetRunningStateFlag()
{
   return bit3RuningState;
}

int CCB::CcbGetTwFlagByDevType(int type)
{
    int iLoop;
    //int iFlags = 0;

    for (iLoop = 0; iLoop < MAX_HANDLER_NUM; iLoop++)
    {
        if (aHandler[iLoop].bit1Qtw
            && type == aHandler[iLoop].iDevType
            && aHandler[iLoop].bit1Active) 
        {
            return iLoop; 
        }
    }
    return MAX_HANDLER_NUM;
}

int CCB::CcbGetHandlerStatus(int iDevId)
{
    int iIdx = iDevId - APP_HAND_SET_BEGIN_ADDRESS;

    if (iIdx >= 0 && iIdx < APP_HAND_SET_MAX_NUMBER)
    {
        return aHandler[iIdx].bit1Active;
    }

    return 0;
    
}

int CCB::CcbGetHandlerType(int iDevId)
{
    int iIdx = iDevId - APP_HAND_SET_BEGIN_ADDRESS;

    if (iIdx >= 0 && iIdx < APP_HAND_SET_MAX_NUMBER)
    {
        return aHandler[iIdx].iDevType;
    }

    return 0;
    
}


int CCB::CcbGetHandlerTwFlag(int iDevId)
{
    int iIdx = iDevId - APP_HAND_SET_BEGIN_ADDRESS;

    if (iIdx >= 0 && iIdx < APP_HAND_SET_MAX_NUMBER)
    {
        return aHandler[iIdx].bit1Qtw;
    }

    return 0;
    
}

int CCB::CcbGetHandlerId2Index(int iDevId)
{
    int iIdx = iDevId - APP_HAND_SET_BEGIN_ADDRESS;

    if (iIdx >= 0 && iIdx < APP_HAND_SET_MAX_NUMBER)
    {
        return iIdx;
    }

    return APP_HAND_SET_MAX_NUMBER;
    
}

void CCB::CanCcbSaveQtw2(int iTrxIndex, int iDevId,unsigned int ulVolume)
{

    int iIndex   = (iDevId - APP_HAND_SET_BEGIN_ADDRESS);
    int ulCanId;

    CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,iDevId,APP_DEV_TYPE_MAIN_CTRL);

    aHandler[iIndex].CommHdr.ucDevType = APP_DEV_TYPE_HAND_SET;
    aHandler[iIndex].CommHdr.ucTransId = 0XF0;
    aHandler[iIndex].CommHdr.ucMsgType = APP_PACKET_HAND_OPERATION;

    aHandler[iIndex].ulCanId = ulCanId;
    aHandler[iIndex].iCanIdx = 0;
    if(INVALID_FM_VALUE != ulVolume)
    {
        QtwMeas.ulTotalFm        = ulVolume - gGlobalParam.Caliparam.pc[DISP_PC_COFF_S1].fc;
    }
    else
    {
        QtwMeas.ulTotalFm        = ulVolume;
    }

    aHandler[iIndex].iCurTrxIndex = iTrxIndex;

    
    LOG(VOS_LOG_WARNING,"%s:%d",__FUNCTION__,QtwMeas.ulTotalFm);

}

void CCB::CcbInitHandlerQtwMeas(int iIndex)
{
    QTW_MEAS_STRU         *pQtwMeas;

    /* DO NOT init ulTotalFm */
    pQtwMeas = &aHandler[iIndex].QtwMeas;
    
    pQtwMeas->ulBgnFm   = INVALID_FM_VALUE;
    pQtwMeas->ulCurFm   = INVALID_FM_VALUE;

    pQtwMeas->ulBgnTm   = time(NULL);
    pQtwMeas->ulEndTm   = pQtwMeas->ulBgnTm;

    pQtwMeas = &QtwMeas;

    pQtwMeas->ulBgnFm   = INVALID_FM_VALUE;
    pQtwMeas->ulCurFm   = INVALID_FM_VALUE;
    
    pQtwMeas->ulBgnTm   = time(NULL);
    pQtwMeas->ulEndTm   = pQtwMeas->ulBgnTm;
        
}

void CCB::CcbFiniHandlerQtwMeas(int iIndex)
{
    QTW_MEAS_STRU         *pQtwMeas;

    pQtwMeas = &aHandler[iIndex].QtwMeas;

    pQtwMeas->ulEndTm   = time(NULL);

    pQtwMeas = &QtwMeas;

    pQtwMeas->ulEndTm   = time(NULL);
    //pQtwMeas->ulEndTm   = pQtwMeas->ulEndTm;
}


void CCB::MainSndWorkMsg(int iSubMsg,unsigned char *data, int iPayLoadLen)
{
    // send message to main
    int ret;

    MAIN_WORK_MSG_STRU *pWorkMsg = NULL;
    
    pWorkMsg = (MAIN_WORK_MSG_STRU *)SatAllocMsg(MAIN_WORK_MSG_SIZE + iPayLoadLen);//malloc(sizeof(MAIN_CANITF_MSG_STRU) + CanRcvBuff[ucRcvBufIndex].len);
    if (NULL == pWorkMsg)
    {
        return ;
    }
    
    pWorkMsg->msgHead.event  = MAIN_WORK_MSG;
    pWorkMsg->iMsgLen        = iPayLoadLen;
    pWorkMsg->iSubMsg        = iSubMsg;
 
    memcpy(pWorkMsg->aucData,data,iPayLoadLen);

    ret = VOS_SndMsg(m_taskHdl,(SAT_MSG_HEAD *)pWorkMsg);

    if (0 != ret)
    {
    }
}

float CCB::CcbConvert2Pm3Data(unsigned int ulValue)
{
    float fValue = (ulValue * 3300);

    //LOG(VOS_LOG_DEBUG,"pm3 raw %d",ulValue); 

    fValue /= (4096*125);

    /* I to pm value */
    if (fValue <= 4) fValue = 4;

    fValue = (fValue - 4) / 16; /* to percent */
    
    return fValue * gSensorRange.fFeedSRange * 10;//B3_LEVEL;
}

// Source Tank
float CCB::CcbConvert2Pm3SP(unsigned int ulValue)
{
    float fValue = CcbConvert2Pm3Data(ulValue);

    if (0 == gGlobalParam.PmParam.afDepth[DISP_PM_PM3])
    {
        return 100.0;
    }

    //ex_dcj
    float tmp = (fValue / gGlobalParam.PmParam.afDepth[DISP_PM_PM3]) * 100 ;
    tmp *= gGlobalParam.Caliparam.pc[DISP_PC_COFF_SW_TANK_LEVEL].fk;
    return tmp;
   // return (fValue / gGlobalParam.PmParam.afDepth[DISP_PM_PM3]) * 100 ;  /* convert to 0~100 percent */
}



float CCB::CcbConvert2Pm2Data(unsigned int ulValue)
{
    float fValue = (ulValue * 3300);

    fValue /= (4096*125);

    if (fValue <= 4) fValue = 4;

    fValue = (fValue - 4) / 16; 
    
    return fValue * gSensorRange.fPureSRange * 10; //B2_LEVEL;
}

//Pure Tank
float CCB::CcbConvert2Pm2SP(unsigned int ulValue)
{
    float fValue = CcbConvert2Pm2Data(ulValue);
    float tmp;
    float correction_fValue = fValue;
    float correction = 0.0;

    if (0 == gGlobalParam.PmParam.afDepth[DISP_PM_PM2])
    {
        return 100.0;
    }

    switch(gGlobalParam.PmParam.aiBuckType[DISP_PM_PM2])
    {
    case DISP_WATER_BARREL_TYPE_010L:
        //tmp = (correction_fValue / gGlobalParam.PmParam.afDepth[DISP_PM_PM2]) * 100 ;
        correction = s_fTankHOffset - 0.01;
        if(fValue > correction)
        {
            correction_fValue = fValue - correction;
        }
        else
        {
            correction_fValue = 0;
        }
        tmp = (correction_fValue / (gGlobalParam.PmParam.afDepth[DISP_PM_PM2] - correction)) * 100 ;
        break;
    case DISP_WATER_BARREL_TYPE_030L:
    case DISP_WATER_BARREL_TYPE_060L:
    case DISP_WATER_BARREL_TYPE_100L:
    case DISP_WATER_BARREL_TYPE_200L:
    case DISP_WATER_BARREL_TYPE_350L:
        correction = s_fTankHOffset;
        if(fValue > correction)
        {
            correction_fValue = fValue - correction;
        }
        else
        {
            correction_fValue = 0;
        }
        tmp = (correction_fValue / (gGlobalParam.PmParam.afDepth[DISP_PM_PM2] - correction)) * 100 ;
        break;
    case DISP_WATER_BARREL_TYPE_UDF:
        tmp = (correction_fValue / gGlobalParam.PmParam.afDepth[DISP_PM_PM2]) * 100 ;
        break;
    case DISP_WATER_BARREL_TYPE_NO:
        break;
    }
    
    tmp *= gGlobalParam.Caliparam.pc[DISP_PC_COFF_PW_TANK_LEVEL].fk;
    return tmp;
}

uint8_t CCB::CcbGetPm2Percent(float fValue)
{
    int iResult;
    if (0 == gGlobalParam.PmParam.afDepth[DISP_PM_PM2])
    {
        return 100.0;
    }

    iResult =  100 * (fValue / (gGlobalParam.PmParam.afDepth[DISP_PM_PM2])) ;

    if (iResult >= 100 ) return 100;
    
    return iResult;
}

void CCB::CcbSetPm2RefillThd(uint8_t RefillThd,uint8_t StopThd)
{

    float fTmp;
    
    if (0 == gGlobalParam.PmParam.afDepth[DISP_PM_PM2])
    {
        return ;
    }
    gGlobalParam.MMParam.SP[MACHINE_PARAM_SP5] = RefillThd;
    gGlobalParam.MMParam.SP[MACHINE_PARAM_SP6] = StopThd;
#if 0
    fTmp = gGlobalParam.PmParam.afDepth[DISP_PM_PM2] * RefillThd/100;

    gGlobalParam.MMParam.SP[MACHINE_PARAM_SP5] = fTmp;

    fTmp = gGlobalParam.PmParam.afDepth[DISP_PM_PM2] * StopThd/100;

    gGlobalParam.MMParam.SP[MACHINE_PARAM_SP6] = fTmp;
#endif
    

}




//work pressure
float CCB::CcbConvert2Pm1Data(unsigned int ulValue)
{
    float fValue = (ulValue * 3300);

    fValue /= (4096*125);

    if (fValue <= 4) fValue = 4;

    fValue = (fValue - 4) / 16; 
    //ex_dcj
    fValue *= B1_PRESSURE;
    fValue *= gGlobalParam.Caliparam.pc[DISP_PC_COFF_SYS_PRESSURE].fk;
    return fValue;
    
    //return fValue * B1_PRESSURE;
}

/**
 * 计算过水量
 * @param  ulValue [脉冲数]
 * @return         [过水量，单位ml]
 */
unsigned int CCB::CcbConvert2Fm1Data(unsigned int ulValue)
{
    /* x pulse per litre */
    if (!gGlobalParam.FmParam.aulCfg[0])
    {
        LOG(VOS_LOG_WARNING,"FMParam.aulCfg[0] = 0");    
    
        return 0;
    }
    unsigned int tmp = (ulValue * 1000)/ gGlobalParam.FmParam.aulCfg[0]; //ml
    return tmp;
}

unsigned int CCB::CcbConvert2Fm2Data(unsigned int ulValue)
{
    /* x pulse per litre */
    if (!gGlobalParam.FmParam.aulCfg[1])
    {
        LOG(VOS_LOG_WARNING,"FMParam.aulCfg[1] = 0");    
    
        return 0;
    }
    unsigned int tmp = (ulValue * 1000)/ gGlobalParam.FmParam.aulCfg[1];
    return tmp;
}

unsigned int CCB::CcbConvert2Fm3Data(unsigned int ulValue)
{
    /* x pulse per litre */
    if (!gGlobalParam.FmParam.aulCfg[2])
    {
        LOG(VOS_LOG_WARNING,"FMParam.aulCfg[2] = 0");    
    
        return 0;
    }
    unsigned int tmp = (ulValue * 1000)/ gGlobalParam.FmParam.aulCfg[2];
    return tmp;
}

unsigned int CCB::CcbConvert2Fm4Data(unsigned int ulValue)
{   
    /* x pulse per litre */
    if (!gGlobalParam.FmParam.aulCfg[3])
    {
        LOG(VOS_LOG_WARNING,"FMParam.aulCfg[3] = 0");    
    
        return 0;
    }
    unsigned int tmp = (ulValue * 1000)/ gGlobalParam.FmParam.aulCfg[3];
    return tmp;
}

unsigned int CCB::CcbConvert2RectAndEdiData(unsigned int ulValue)
{
    return ((ulValue * 100)/ 198) ; // ->ma
}

unsigned int CCB::CcbConvert2GPumpData(unsigned int ulValue)
{
    return (((ulValue  & 0XFFFF))*100)/99 ; // ->ma
}

unsigned int CCB::CcbGetRPumpVData(int iChl)
{
    return DispConvertRPumpSpeed2Voltage( ExeBrd.ausHoldRegister[APP_EXE_HOLD_REG_RPUMP_OFFSET + iChl]);
}

unsigned int CCB::CcbConvert2RPumpIData(unsigned int ulValue)
{
    
    return (((ulValue  & 0XFFFF))*100)/99 ; // ->ma
}

float CCB::CcbGetSp1()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP1];
}

float CCB::CcbGetSp2()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP2];

}

float CCB::CcbGetSp3()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP3];

}

float CCB::CcbGetSp4()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP4];

}

//设备恢复产水液位（B2）
float CCB::CcbGetSp5()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP5];
}

//水箱低液位报警线（B2）
float CCB::CcbGetSp6()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP6];
}

float CCB::CcbGetSp7()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP7];
}

//原水箱补水液位
float CCB::CcbGetSp8()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP8];
}

//原水低液位
float CCB::CcbGetSp9()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP9];
}

float CCB::CcbGetSp10()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP10];
}

float CCB::CcbGetSp11()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP11];
}

float CCB::CcbGetSp12()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP12];
}

float CCB::CcbGetSp13()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP13];
}

float CCB::CcbGetSp14()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP14];
}

float CCB::CcbGetSp15()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP15];
}

float CCB::CcbGetSp16()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP16];
}

float CCB::CcbGetSp17()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP17];
}

float CCB::CcbGetSp30()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP30];
}

float CCB::CcbGetSp31()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP31];
}

float CCB::CcbGetSp32()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP32];
}

float CCB::CcbGetSp33()
{
    return gGlobalParam.MMParam.SP[MACHINE_PARAM_SP33];
}

float CCB::CcbGetB2Full()
{
    return 0.9;
}

void CCB::CanPrepare4Pm2Full()
{
    if (!bit1B2Full)
    {
        bit1B2Full = TRUE;
        ulB2FullTick = gulSecond;
    }
}

float CCB::CcbCalcREJ()
{
    float fTmp;
    if (0 == ExeBrd.aEcoObjs[APP_EXE_I1_NO].Value.eV.fWaterQ) return 0;

    fTmp = (ExeBrd.aEcoObjs[APP_EXE_I1_NO].Value.eV.fWaterQ - ExeBrd.aEcoObjs[APP_EXE_I2_NO].Value.eV.fWaterQ);

    return (fTmp/ExeBrd.aEcoObjs[APP_EXE_I1_NO].Value.eV.fWaterQ)*100;
}



void CCB::CcbUpdateSwitchObjState(unsigned int ulMask,unsigned int ulValue)
{
    int iLoop;
    for (iLoop = 0; iLoop < APP_EXE_SWITCHS; iLoop++)
    {
        if (ulMask & (1 << iLoop))
        {
            switch(iLoop)
            {
            case APP_EXE_E1_NO ... APP_EXE_E10_NO:
                if (ulValue & (1 << iLoop))
                {
                    ExeBrd.aValveObjs[iLoop-APP_EXE_E1_NO].iActive |= DISP_ACT_TYPE_SWITCH;
                }
                else
                {
                    ExeBrd.aValveObjs[iLoop-APP_EXE_E1_NO].iActive &= ~DISP_ACT_TYPE_SWITCH;
                }
                break;
            case APP_EXE_C3_NO ... APP_EXE_C4_NO:
                if (ulValue & (1 << iLoop))
                {
                    if (!ExeBrd.aGPumpObjs[iLoop-APP_EXE_C3_NO].iActive)
                    {
                         ExeBrd.aGPumpObjs[iLoop-APP_EXE_C3_NO].ulSec = DispGetCurSecond();
                    }
                    
                    ExeBrd.aGPumpObjs[iLoop-APP_EXE_C3_NO].iActive |= DISP_ACT_TYPE_SWITCH;
                }
                else
                {
                    if (ExeBrd.aGPumpObjs[iLoop-APP_EXE_C3_NO].iActive)
                    {
                        unsigned int ulEndSec = DispGetCurSecond();
                        
                        if (ulEndSec > ExeBrd.aGPumpObjs[iLoop-APP_EXE_C3_NO].ulSec)
                        {
                            ExeBrd.aGPumpObjs[iLoop-APP_EXE_C3_NO].ulDurSec += ulEndSec - ExeBrd.aGPumpObjs[iLoop-APP_EXE_C3_NO].ulSec;
                        }
                    }
                
                    ExeBrd.aGPumpObjs[iLoop-APP_EXE_C3_NO].iActive &= ~DISP_ACT_TYPE_SWITCH;
                }
                break;
            case APP_EXE_C1_NO ... APP_EXE_C2_NO:
                if (ulValue & (1 << iLoop))
                {
                    CcbUpdateRPumpObjState(iLoop-APP_EXE_C1_NO,0XFF00);
                }
                else
                {
                    CcbUpdateRPumpObjState(iLoop-APP_EXE_C1_NO,0X0000);
                }
                break;
            case APP_EXE_N1_NO ... APP_EXE_N3_NO:
                if (ulValue & (1 << iLoop))
                {
                    if (!ExeBrd.aRectObjs[iLoop-APP_EXE_N1_NO].iActive)
                    {
                        ExeBrd.aRectObjs[iLoop-APP_EXE_N1_NO].ulSec = DispGetCurSecond();
                    }
                    ExeBrd.aRectObjs[iLoop-APP_EXE_N1_NO].iActive |= DISP_ACT_TYPE_SWITCH;
                }
                else
                {
                    if (ExeBrd.aRectObjs[iLoop-APP_EXE_N1_NO].iActive)
                    {
                        unsigned int ulEndSec = DispGetCurSecond();
                        
                        if (ulEndSec > ExeBrd.aRectObjs[iLoop-APP_EXE_N1_NO].ulSec)
                        {
                            ExeBrd.aRectObjs[iLoop-APP_EXE_N1_NO].ulDurSec += ulEndSec - ExeBrd.aRectObjs[iLoop-APP_EXE_N1_NO].ulSec;
                        }
                    }
                    ExeBrd.aRectObjs[iLoop-APP_EXE_N1_NO].iActive &= ~DISP_ACT_TYPE_SWITCH;
                }
                break;
            case APP_EXE_T1_NO:
                if (ulValue & (1 << iLoop))
                {
                    ExeBrd.aEDIObjs[iLoop-APP_EXE_T1_NO].iActive |= DISP_ACT_TYPE_SWITCH;
                }
                else
                {
                    ExeBrd.aEDIObjs[iLoop-APP_EXE_T1_NO].iActive &= ~DISP_ACT_TYPE_SWITCH;
                }
                break;
            }
        }
    }
    if (bit1SysMonitorStateRpt) CcbSwithStateNotify();
}

unsigned int CCB::CcbGetSwitchObjState(unsigned int ulMask)
{
    int iLoop    ;
    unsigned int ulValue = 0;
    
    for (iLoop = 0; iLoop < APP_EXE_SWITCHS; iLoop++)
    {
        if (ulMask & (1 << iLoop))
        {
            switch(iLoop)
            {
            case APP_EXE_E1_NO ... APP_EXE_E10_NO:
                if (ExeBrd.aValveObjs[iLoop-APP_EXE_E1_NO].iActive & DISP_ACT_TYPE_SWITCH)  ulValue |= (1 << iLoop);
                break;
            case APP_EXE_C3_NO ... APP_EXE_C4_NO:
                if (ExeBrd.aGPumpObjs[iLoop-APP_EXE_C3_NO].iActive & DISP_ACT_TYPE_SWITCH)  ulValue |= (1 << iLoop);
                break;
            case APP_EXE_C1_NO ... APP_EXE_C2_NO:
                if (ExeBrd.aRPumpObjs[iLoop-APP_EXE_C1_NO].iActive & DISP_ACT_TYPE_SWITCH)  ulValue |= (1 << iLoop);
                break;
            case APP_EXE_N1_NO ... APP_EXE_N3_NO:
                if (ExeBrd.aRectObjs[iLoop-APP_EXE_N1_NO].iActive & DISP_ACT_TYPE_SWITCH)  ulValue |= (1 << iLoop);
                break;
            case APP_EXE_T1_NO:
                if (ExeBrd.aEDIObjs[iLoop-APP_EXE_T1_NO].iActive & DISP_ACT_TYPE_SWITCH)  ulValue |= (1 << iLoop);
                break;
            }
        }
    }
    return ulValue;
}

void CCB::CcbUpdateRPumpObjState(int iChl,int iState)
{
    if ((iState >> 8) & (0XFF))
    {
        if (!ExeBrd.aRPumpObjs[iChl].iActive)
        {
             ExeBrd.aRPumpObjs[iChl].ulSec = DispGetCurSecond();
        }

        ExeBrd.aRPumpObjs[iChl].iActive |= DISP_ACT_TYPE_SWITCH;

        if (bit1SysMonitorStateRpt) CcbRPumpStateNotify(iChl,1);
        
    }
    else
    {
        if (ExeBrd.aRPumpObjs[iChl].iActive)
        {
            unsigned int ulEndSec = DispGetCurSecond();
            
            if (ulEndSec > ExeBrd.aRPumpObjs[iChl].ulSec)
            {
                ExeBrd.aRPumpObjs[iChl].ulDurSec += 
                       ulEndSec - ExeBrd.aRPumpObjs[iChl].ulSec;
            }
        }                
    
        ExeBrd.aRPumpObjs[iChl].iActive &= ~DISP_ACT_TYPE_SWITCH;
        
        if (bit1SysMonitorStateRpt) CcbRPumpStateNotify(iChl,0);
    }
}


void CCB::CcbUpdatePmObjState(unsigned int ulMask,unsigned int ulValue)
{
    int iLoop;
    for (iLoop = 0; iLoop < APP_EXE_PRESSURE_METER; iLoop++)
    {
        if (ulMask & GET_B_MASK(iLoop))
        {
            if (ulValue & GET_B_MASK(iLoop))
            {
                ExeBrd.aPMObjs[iLoop].iActive |= DISP_ACT_TYPE_RPT;
            }
            else
            {
                ExeBrd.aPMObjs[iLoop].iActive &= ~DISP_ACT_TYPE_RPT;
            }
        }
    }
}

unsigned int CCB::CcbGetPmObjState(unsigned int ulMask)
{
    int iLoop    ;
    unsigned int ulValue = 0;
    
    for (iLoop = 0; iLoop < APP_EXE_PRESSURE_METER; iLoop++)
    {
        if (ulMask & (1 << iLoop))
        {
            if (ExeBrd.aPMObjs[iLoop].iActive & DISP_ACT_TYPE_RPT)  ulValue |= (1 << iLoop);
        }
    }
    return ulValue;
}

void CCB::CcbUpdateIObjState(unsigned int ulMask,unsigned int ulValue)
{
    int iLoop;
    for (iLoop = 0; iLoop < APP_EXE_ECO_NUM; iLoop++)
    {
        if (ulMask & (1 << iLoop))
        {
            if (ulValue & (1 << iLoop))
            {
                ExeBrd.aEcoObjs[iLoop].iActive |= DISP_ACT_TYPE_RPT;
            }
            else
            {
                ExeBrd.aEcoObjs[iLoop].iActive &= ~DISP_ACT_TYPE_RPT;
            }
        }
    }
}

unsigned int CCB::CcbGetIObjState(unsigned int ulMask)
{
    int iLoop    ;
    unsigned int ulValue = 0;
    
    for (iLoop = 0; iLoop < APP_EXE_ECO_NUM; iLoop++)
    {
        if (ulMask & (1 << iLoop))
        {
            if (ExeBrd.aEcoObjs[iLoop].iActive & DISP_ACT_TYPE_RPT)  ulValue |= (1 << iLoop);
        }
    }
    return ulValue;
}


void CCB::CcbUpdateFmObjState(unsigned int ulMask,unsigned int ulValue)
{
    int iLoop;
    for (iLoop = 0; iLoop < APP_FM_FLOW_METER_NUM; iLoop++)
    {
        if (ulMask & (1 << iLoop))
        {
            if (ulValue & (1 << iLoop))
            {
                FlowMeter.aFmObjs[iLoop].iActive |= DISP_ACT_TYPE_RPT;
            }
            else
            {
                FlowMeter.aFmObjs[iLoop].iActive &= ~DISP_ACT_TYPE_RPT;
            }
        }
    }
}

unsigned int CCB::CcbGetFmObjState(unsigned int ulMask)
{
    int iLoop    ;
    unsigned int ulValue = 0;
    
    for (iLoop = 0; iLoop < APP_FM_FLOW_METER_NUM; iLoop++)
    {
        if (ulMask & (1 << iLoop))
        {
            if (FlowMeter.aFmObjs[iLoop].iActive & DISP_ACT_TYPE_RPT)  ulValue |= (1 << iLoop);
        }
    }
    return ulValue;
}

int CCB::CcbWorkDelayEntryWrapper(int id,unsigned int ulTime,sys_timeout_handler cb)
{
    timer_reset(&to4Delay[id],ulTime,TIMER_ONE_SHOT,cb,(void *)&Sem4Delay[id]);  
    
    sp_thread_sem_pend(&Sem4Delay[id]);

    if (ulWorkThdAbort & MakeThdState(id,WORK_THREAD_STATE_BLK_TICKING))
    {
        return -1;
    }

    return 0;
}


void CCB::CcbDelayCallBack(void *para)
{
    sp_sem_t *sem = (sp_sem_t *)para;

    sp_thread_sem_post(sem);

}


int CCB::CcbWorkDelayEntry(int id,unsigned int ulTime,sys_timeout_handler cb)
{
    int iRet;
    
    sp_thread_mutex_lock( &WorkMutex );
    ulWorkThdState |= MakeThdState(id,WORK_THREAD_STATE_BLK_TICKING);
    sp_thread_mutex_unlock( &WorkMutex );

    iRet = CcbWorkDelayEntryWrapper(id,ulTime,cb);    

    sp_thread_mutex_lock( &WorkMutex );
    ulWorkThdState &= ~ MakeThdState(id,WORK_THREAD_STATE_BLK_TICKING);
    ulWorkThdAbort &= ~ MakeThdState(id,WORK_THREAD_STATE_BLK_TICKING);
    sp_thread_mutex_unlock( &WorkMutex );

    return iRet; // success
}


unsigned int CCB::CcbGetModbus2BytesData(int offset)
{
    return BUILD_UINT16(aucModbusBuffer[offset+1],aucModbusBuffer[offset]);
}

unsigned int CCB::CcbGetModbus4BytesData(int offset)
{
    return BUILD_UINT32(aucModbusBuffer[offset+3],aucModbusBuffer[offset+2],aucModbusBuffer[offset+1],aucModbusBuffer[offset+0]);
}


int CCB::CcbSwitchUpdateModbusWrapper(int id,unsigned int ulAddress, unsigned int ulMask,unsigned int ulSwitchs)
{
    int iSkip = 1;
    
    unsigned int ulIdentifier;

    unsigned char aucModbusBuf[16];

    int iRet = -1;

    int iIdx = 0;

    MODBUS_PACKET_COMM_STRU *pModBus = (MODBUS_PACKET_COMM_STRU *)aucModbusBuf;

    if ((((1 << APP_EXE_E1_NO)|(1 << APP_EXE_C1_NO) ) == (ulMask & ((1 << APP_EXE_E1_NO)|(1 << APP_EXE_C1_NO))))
        && (0 == (((1 << APP_EXE_E1_NO)|(1 << APP_EXE_C1_NO) ) & ulSwitchs) ))
    {

        unsigned int  ulState = CcbGetSwitchObjState( (1 << APP_EXE_E1_NO)|(1 << APP_EXE_C1_NO));  

        if (((1 << APP_EXE_E1_NO)|(1 << APP_EXE_C1_NO)) == ulState)
        {
            iSkip = 0;
        }
    }

    if (!iSkip)
    {
        iIdx = 0;

        /* 0. Node Address (2018/03/04 add for zigbee) */
        pModBus->ucAddress = APP_PAKCET_ADDRESS_EXE; 
        
        pModBus->ucFuncode = MODBUS_FUNC_CODE_Mask_Write_0X_Register;

        ulMask &=  ~(1<<APP_EXE_E1_NO);

        
        /* 1.Reg Address */
        pModBus->aucData[iIdx++] = (ulAddress >> 8) & 0XFF; 
        pModBus->aucData[iIdx++] = (ulAddress >> 0) & 0XFF;
        
        /* 2.Mask Value */
        pModBus->aucData[iIdx++] = (ulMask >> 8) & 0XFF;
        pModBus->aucData[iIdx++] = (ulMask >> 0) & 0XFF;
        
        /* 3. Value */
        pModBus->aucData[iIdx++] = (ulSwitchs >> 8) & 0XFF;
        pModBus->aucData[iIdx++] = (ulSwitchs >> 0) & 0XFF;
        
        CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_PAKCET_ADDRESS_MC,APP_PAKCET_ADDRESS_EXE);
        
        iRet = CcbModbusWorkEntryWrapper(id,APP_CAN_CHL_INNER,ulIdentifier,aucModbusBuf,iIdx + MODBUS_PACKET_COMM_HDR_LEN,2000,CcbDefaultModbusCallBack); // don care modbus exe result
        
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbSwitchUpdateModbusWrapper Fail %d",iRet);    
            /* notify ui (late implemnt) */
            return iRet;
        }

        iRet = CcbWorkDelayEntry(id,5000,CcbDelayCallBack);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbSwitchUpdateModbusWrapper Fail %d",iRet);    
            /* notify ui (late implemnt) */
            return iRet;
        }

        ulMask = 1 << APP_EXE_E1_NO;
        ulSwitchs &= 1 << APP_EXE_E1_NO;
        
        iIdx = 0;
        /* 0. Node Address (2018/03/04 add for zigbee) */
        pModBus->ucAddress = APP_PAKCET_ADDRESS_EXE; 
        pModBus->ucFuncode = MODBUS_FUNC_CODE_Mask_Write_0X_Register;
        
        /* 1.Reg Address */
        pModBus->aucData[iIdx++] = (ulAddress >> 8) & 0XFF; 
        pModBus->aucData[iIdx++] = (ulAddress >> 0) & 0XFF;
        
        /* 2.Mask Value */
        pModBus->aucData[iIdx++] = (ulMask >> 8) & 0XFF;
        pModBus->aucData[iIdx++] = (ulMask >> 0) & 0XFF;
        
        /* 3. Value */
        pModBus->aucData[iIdx++] = (ulSwitchs >> 8) & 0XFF;
        pModBus->aucData[iIdx++] = (ulSwitchs >> 0) & 0XFF;
        
        CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_PAKCET_ADDRESS_MC,APP_PAKCET_ADDRESS_EXE);
        
        iRet = CcbModbusWorkEntryWrapper(id,APP_CAN_CHL_INNER,ulIdentifier,aucModbusBuf,iIdx + MODBUS_PACKET_COMM_HDR_LEN,2000,CcbDefaultModbusCallBack); // don care modbus exe result
        
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbSwitchUpdateModbusWrapper Fail %d",iRet);    
            /* notify ui (late implemnt) */
            return iRet;
        }
        
    }
    else
    {

        iIdx = 0;
        /* 0. Node Address (2018/03/04 add for zigbee) */
        pModBus->ucAddress = APP_PAKCET_ADDRESS_EXE; 
        pModBus->ucFuncode = MODBUS_FUNC_CODE_Mask_Write_0X_Register;
        
        /* 1.Reg Address */
        pModBus->aucData[iIdx++] = (ulAddress >> 8) & 0XFF; 
        pModBus->aucData[iIdx++] = (ulAddress >> 0) & 0XFF;
        
        /* 2.Mask Value */
        pModBus->aucData[iIdx++] = (ulMask >> 8) & 0XFF;
        pModBus->aucData[iIdx++] = (ulMask >> 0) & 0XFF;
        
        /* 3. Value */
        pModBus->aucData[iIdx++] = (ulSwitchs >> 8) & 0XFF;
        pModBus->aucData[iIdx++] = (ulSwitchs >> 0) & 0XFF;
        
        CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_PAKCET_ADDRESS_MC,APP_PAKCET_ADDRESS_EXE);
        
        iRet = CcbModbusWorkEntryWrapper(id,APP_CAN_CHL_INNER,ulIdentifier,aucModbusBuf,iIdx + MODBUS_PACKET_COMM_HDR_LEN,2000,CcbDefaultModbusCallBack); // don care modbus exe result
        
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbSwitchUpdateModbusWrapper Fail %d",iRet);    
            /* notify ui (late implemnt) */
            return iRet;
        }
    }

    return iRet;
}

int CCB::CcbSwitchSetModbusWrapper(int id,unsigned int ulAddress, unsigned int ulNums,unsigned int ulSwitchs)
{
    unsigned int ulIdentifier;

    unsigned char aucModbusBuf[16];

    int iTmp;

    int iRet = -1;

    int iIdx = 0;

    int iSkip = 1;

    unsigned int ulMask = 0;
    unsigned int ulSwMask = 0;

    MODBUS_PACKET_COMM_STRU *pModBus = (MODBUS_PACKET_COMM_STRU *)aucModbusBuf;

    ulMask = ((1 << ulNums) - 1) << ulAddress;

    ulSwMask = ulSwitchs << ulAddress;

    if ((((1 << APP_EXE_E1_NO)|(1 << APP_EXE_C1_NO) ) == (ulMask & ((1 << APP_EXE_E1_NO)|(1 << APP_EXE_C1_NO))))
        && (0 == (((1 << APP_EXE_E1_NO)|(1 << APP_EXE_C1_NO) ) & ulSwMask )))
    {

        unsigned int  ulState = CcbGetSwitchObjState( (1 << APP_EXE_E1_NO)|(1 << APP_EXE_C1_NO));  

        if (((1 << APP_EXE_E1_NO)|(1 << APP_EXE_C1_NO)) == ulState)
        {
            iSkip = 0;
        }
    }

    if (!iSkip)
    {
        int iNewAdrOff = APP_EXE_C1_NO - ulAddress;
        
        iIdx = 0;
        /* 0. Node Address (2018/03/04 add for zigbee) */
        pModBus->ucAddress = APP_PAKCET_ADDRESS_EXE; 
        pModBus->ucFuncode = MODBUS_FUNC_CODE_Force_Multiple_Coils;
        
        /* 1.Start Address */
        pModBus->aucData[iIdx++] = (APP_EXE_C1_NO >> 8) & 0XFF; 
        pModBus->aucData[iIdx++] = (APP_EXE_C1_NO >> 0) & 0XFF;
        
        /* 2.Reg Numbers */
        iTmp = ulNums - iNewAdrOff;
        pModBus->aucData[iIdx++] = (iTmp >> 8) & 0XFF;
        pModBus->aucData[iIdx++] = (iTmp >> 0) & 0XFF;
        
        /* 3. data counts */
        pModBus->aucData[iIdx++] = 2;
        
        /* set  valves */
        pModBus->aucData[iIdx++] = (ulSwitchs >> iNewAdrOff) & 0XFF;
        pModBus->aucData[iIdx++] = (ulSwitchs >> (8 + iNewAdrOff)) & 0XFF;
        
        CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_PAKCET_ADDRESS_MC,APP_PAKCET_ADDRESS_EXE);
        
        iRet = CcbModbusWorkEntryWrapper(id,APP_CAN_CHL_INNER,ulIdentifier,aucModbusBuf,iIdx + MODBUS_PACKET_COMM_HDR_LEN,2000,CcbDefaultModbusCallBack); // don care modbus exe result
        
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbModbusWorkEntryWrapper Fail %d",iRet);    
            /* notify ui (late implemnt) */
            return iRet;
        }

        iRet = CcbWorkDelayEntry(id,5000,CcbDelayCallBack);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbWorkDelayEntry Fail %d",iRet);    
            /* notify ui (late implemnt) */
            return iRet;
        }
        

        iIdx = 0;
        /* 0. Node Address (2018/03/04 add for zigbee) */
        pModBus->ucAddress = APP_PAKCET_ADDRESS_EXE; 
        pModBus->ucFuncode = MODBUS_FUNC_CODE_Force_Multiple_Coils;
        
        /* 1.Start Address */
        pModBus->aucData[iIdx++] = (ulAddress >> 8) & 0XFF; 
        pModBus->aucData[iIdx++] = (ulAddress >> 0) & 0XFF;
        
        /* 2.Reg Numbers */
        iTmp = iNewAdrOff;
        pModBus->aucData[iIdx++] = (iTmp >> 8) & 0XFF;
        pModBus->aucData[iIdx++] = (iTmp >> 0) & 0XFF;
        
        /* 3. data counts */
        pModBus->aucData[iIdx++] = 2;
        
        /* set  valves */
        pModBus->aucData[iIdx++] = (ulSwitchs >> 0) & 0XFF;
        pModBus->aucData[iIdx++] = (ulSwitchs >> 8) & 0XFF;
        
        CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_PAKCET_ADDRESS_MC,APP_PAKCET_ADDRESS_EXE);
        
        iRet = CcbModbusWorkEntryWrapper(id,APP_CAN_CHL_INNER,ulIdentifier,aucModbusBuf,iIdx + MODBUS_PACKET_COMM_HDR_LEN,2000,CcbDefaultModbusCallBack); // don care modbus exe result
        
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbModbusWorkEntryWrapper Fail %d",iRet);    
            /* notify ui (late implemnt) */
            return iRet;
        }
         
    }
    else
    {

        iIdx = 0;
        /* 0. Node Address (2018/03/04 add for zigbee) */
        pModBus->ucAddress = APP_PAKCET_ADDRESS_EXE; 
        pModBus->ucFuncode = MODBUS_FUNC_CODE_Force_Multiple_Coils;
        
        /* 1.Start Address */
        pModBus->aucData[iIdx++] = (ulAddress >> 8) & 0XFF; 
        pModBus->aucData[iIdx++] = (ulAddress >> 0) & 0XFF;
        
        /* 2.Reg Numbers */
        iTmp = ulNums;
        pModBus->aucData[iIdx++] = (iTmp >> 8) & 0XFF;
        pModBus->aucData[iIdx++] = (iTmp >> 0) & 0XFF;
        
        /* 3. data counts */
        pModBus->aucData[iIdx++] = 2;
        
        /* set  valves */
        pModBus->aucData[iIdx++] = (ulSwitchs >> 0) & 0XFF;
        pModBus->aucData[iIdx++] = (ulSwitchs >> 8) & 0XFF;
        
        CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_PAKCET_ADDRESS_MC,APP_PAKCET_ADDRESS_EXE);
        
        iRet = CcbModbusWorkEntryWrapper(id,APP_CAN_CHL_INNER,ulIdentifier,aucModbusBuf,iIdx + MODBUS_PACKET_COMM_HDR_LEN,2000,CcbDefaultModbusCallBack); // don care modbus exe result
        
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbModbusWorkEntryWrapper Fail %d",iRet);    
            /* notify ui (late implemnt) */
            return iRet;
        }
    }

    //CcbUpdateSwitchObjState(APP_EXE_SWITCHS_MASK,iTmp);

    return iRet;
}



int CCB::CcbInnerSetSwitch(int id,unsigned int ulAddress, unsigned int ulSwitchs)
{

    int iTmp;
    int iRet = -1;

    LOG(VOS_LOG_WARNING,"CcbInnerSetSwitch id=%x %x",id,ulSwitchs);    
    
    /* 2.Reg Numbers */
    iTmp = APP_EXE_VALVE_NUM + APP_EXE_G_PUMP_NUM + APP_EXE_R_PUMP_NUM + APP_EXE_EDI_NUM + APP_EXE_RECT_NUM;
    
    iRet = CcbSwitchSetModbusWrapper(id,ulAddress,iTmp,ulSwitchs); // don care modbus exe result
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
        /* notify ui (late implemnt) */
        return iRet;
    }

    CcbUpdateSwitchObjState(APP_EXE_SWITCHS_MASK,ulSwitchs);

    return iRet;
}


int CCB::CcbSetSwitch(int id,unsigned int ulAddress, unsigned int ulSwitchs)
{
    int iRet = -1;

    sp_thread_mutex_lock( &C12Mutex );
    
    iRet = CcbInnerSetSwitch(id,ulAddress,ulSwitchs);

    sp_thread_mutex_unlock( &C12Mutex );

    return iRet;
}


int CCB::CcbInnerUpdateSwitch(int id,unsigned int ulAddress, unsigned int ulMask,unsigned int ulSwitchs)
{
    int iRet = -1;

    LOG(VOS_LOG_WARNING,"updswitch %x %x",ulMask,ulSwitchs);    

    iRet = CcbSwitchUpdateModbusWrapper(id,ulAddress,ulMask,ulSwitchs); // don care modbus exe result
    
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbSwitchUpdateModbusWrapper Fail %d",iRet);    
        /* notify ui (late implemnt) */
        return iRet;
    }

    CcbUpdateSwitchObjState(ulMask,ulSwitchs);

    return iRet;
}


int CCB::CcbUpdateSwitch(int id,unsigned int ulAddress, unsigned int ulMask,unsigned int ulSwitchs)
{
    int iRet = -1;

    sp_thread_mutex_lock( &C12Mutex );
    
    iRet = CcbInnerUpdateSwitch(id,ulAddress,ulMask,ulSwitchs);

    sp_thread_mutex_unlock( &C12Mutex );

    return iRet;
}

int CCB::CcbSetIAndBs(int id,unsigned int ulAddress, unsigned int ulSwitchs)
{
    unsigned int ulIdentifier;

    unsigned char aucModbusBuf[16];

    int iRet = -1;

    int iIdx = 0;

    MODBUS_PACKET_COMM_STRU *pModBus = (MODBUS_PACKET_COMM_STRU *)aucModbusBuf;

    iIdx = 0;
    /* 0. Node Address (2018/03/04 add for zigbee) */
    pModBus->ucAddress = APP_PAKCET_ADDRESS_EXE; 
    pModBus->ucFuncode = MODBUS_FUNC_CODE_Preset_Single_Register;
    
    /* 1.Start Address */
    pModBus->aucData[iIdx++] = (ulAddress >> 8) & 0XFF; 
    pModBus->aucData[iIdx++] = (ulAddress >> 0) & 0XFF;
    
    /* 2. I&B ID */
    pModBus->aucData[iIdx++] = (ulSwitchs >> 8) & 0XFF;
    pModBus->aucData[iIdx++] = (ulSwitchs >> 0) & 0XFF;
    
    CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_PAKCET_ADDRESS_MC,APP_PAKCET_ADDRESS_EXE);

    
    iRet = CcbModbusWorkEntryWrapper(id,APP_CAN_CHL_INNER,ulIdentifier,aucModbusBuf,iIdx + MODBUS_PACKET_COMM_HDR_LEN,2000,CcbDefaultModbusCallBack); // don care modbus exe result
    
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
        /* notify ui (late implemnt) */
        return iRet;
    }

    CcbUpdateIObjState(APP_EXE_IP_MASK,ulSwitchs);
    
    CcbUpdatePmObjState(APP_EXE_IP_MASK,ulSwitchs);

    return iRet;
}

int CCB::CcbUpdateIAndBs(int id,unsigned int ulAddress, unsigned int ulMask,unsigned int ulValue)
{
    unsigned int ulIdentifier;

    unsigned char aucModbusBuf[16];

    int iRet = -1;

    int iIdx = 0;

    MODBUS_PACKET_COMM_STRU *pModBus = (MODBUS_PACKET_COMM_STRU *)aucModbusBuf;

    iIdx = 0;
    /* 0. Node Address (2018/03/04 add for zigbee) */
    pModBus->ucAddress = APP_PAKCET_ADDRESS_EXE; 
    pModBus->ucFuncode = MODBUS_FUNC_CODE_Mask_Write_4X_Register;
    
    /* 1.Reg Address */
    pModBus->aucData[iIdx++] = (ulAddress >> 8) & 0XFF; 
    pModBus->aucData[iIdx++] = (ulAddress >> 0) & 0XFF;
    
    /* 2.Mask Value  */
    pModBus->aucData[iIdx++] = (ulMask >> 8) & 0XFF;
    pModBus->aucData[iIdx++] = (ulMask >> 0) & 0XFF;
    
    /* 3.Value       */
    pModBus->aucData[iIdx++] = (ulValue >> 8) & 0XFF;
    pModBus->aucData[iIdx++] = (ulValue >> 0) & 0XFF;
    
    CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_PAKCET_ADDRESS_MC,APP_PAKCET_ADDRESS_EXE);
    
    iRet = CcbModbusWorkEntryWrapper(id,APP_CAN_CHL_INNER,ulIdentifier,aucModbusBuf,iIdx + MODBUS_PACKET_COMM_HDR_LEN,2000,CcbDefaultModbusCallBack); // don care modbus exe result
    
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
        /* notify ui (late implemnt) */
        return iRet;
    }

    CcbUpdateIObjState(ulMask,ulValue);
    
    CcbUpdatePmObjState(ulMask,ulValue);

    return iRet;
}


int CCB::CcbSetFms(int id,unsigned int ulAddress, unsigned int ulSwitchs)
{
    unsigned int ulIdentifier;

    unsigned char aucModbusBuf[16];

    int iRet = -1;

    int iIdx = 0;

    MODBUS_PACKET_COMM_STRU *pModBus = (MODBUS_PACKET_COMM_STRU *)aucModbusBuf;

    iIdx = 0;
    /* 0. Node Address (2018/03/04 add for zigbee) */
    pModBus->ucAddress = APP_PAKCET_ADDRESS_FM; 
    pModBus->ucFuncode = MODBUS_FUNC_CODE_Preset_Single_Register;
    
    /* 1.Start Address */
    pModBus->aucData[iIdx++] = (ulAddress >> 8) & 0XFF; 
    pModBus->aucData[iIdx++] = (ulAddress >> 0) & 0XFF;
    
    
    /* 2. I&B ID */
    pModBus->aucData[iIdx++] = (ulSwitchs >> 8) & 0XFF;
    pModBus->aucData[iIdx++] = (ulSwitchs >> 0) & 0XFF;
    
    CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_PAKCET_ADDRESS_MC,APP_PAKCET_ADDRESS_FM);

    
    iRet = CcbModbusWorkEntryWrapper(id,APP_CAN_CHL_INNER,ulIdentifier,aucModbusBuf,iIdx + MODBUS_PACKET_COMM_HDR_LEN,2000,CcbDefaultModbusCallBack); // don care modbus exe result
    
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
        /* notify ui (late implemnt) */
        return iRet;
    }

    CcbUpdateFmObjState(APP_FM_FM_MASK,ulSwitchs);

    return iRet;
}


int CCB::CcbUpdateFms(int id,unsigned int ulAddress, unsigned int ulMask,unsigned int ulValue)
{
    unsigned int ulIdentifier;

    unsigned char aucModbusBuf[16];

    int iRet = -1;

    int iIdx = 0;

    MODBUS_PACKET_COMM_STRU *pModBus = (MODBUS_PACKET_COMM_STRU *)aucModbusBuf;

    iIdx = 0;
    /* 0. Node Address (2018/03/04 add for zigbee) */
    pModBus->ucAddress = APP_PAKCET_ADDRESS_FM; 
    pModBus->ucFuncode = MODBUS_FUNC_CODE_Mask_Write_4X_Register;
    
    /* 1.Reg Address */
    pModBus->aucData[iIdx++] = (ulAddress >> 8) & 0XFF; 
    pModBus->aucData[iIdx++] = (ulAddress >> 0) & 0XFF;
    
    /* 2.Mask Value */
    pModBus->aucData[iIdx++] = (ulMask >> 8) & 0XFF;
    pModBus->aucData[iIdx++] = (ulMask >> 0) & 0XFF;
    
    /* 3. Value */
    pModBus->aucData[iIdx++] = (ulValue >> 8) & 0XFF;
    pModBus->aucData[iIdx++] = (ulValue >> 0) & 0XFF;
    
    CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_PAKCET_ADDRESS_MC,APP_PAKCET_ADDRESS_FM);
    
    iRet = CcbModbusWorkEntryWrapper(id,APP_CAN_CHL_INNER,ulIdentifier,aucModbusBuf,iIdx + MODBUS_PACKET_COMM_HDR_LEN,2000,CcbDefaultModbusCallBack); // don care modbus exe result
    
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
        /* notify ui (late implemnt) */
        return iRet;
    }

    CcbUpdateFmObjState(ulMask,ulValue);

    return iRet;
}


int CCB::CcbSetExeHoldRegs(int id,unsigned int ulAddress, unsigned int ulValue)
{
    unsigned int ulIdentifier;

    unsigned char aucModbusBuf[16];

    int iRet = -1;

    int iIdx = 0;

    MODBUS_PACKET_COMM_STRU *pModBus = (MODBUS_PACKET_COMM_STRU *)aucModbusBuf;

    iIdx = 0;
    /* 0. Node Address (2018/03/04 add for zigbee) */
    pModBus->ucAddress       = APP_PAKCET_ADDRESS_EXE; 
    pModBus->ucFuncode       = MODBUS_FUNC_CODE_Preset_Multiple_Registers;
    
    /* 1.Reg Address */
    pModBus->aucData[iIdx++] = (ulAddress >> 8) & 0XFF; 
    pModBus->aucData[iIdx++] = (ulAddress >> 0) & 0XFF;

    /* 1.Reg NUM */
    pModBus->aucData[iIdx++] = (1 >> 8) & 0XFF; 
    pModBus->aucData[iIdx++] = (1 >> 0) & 0XFF;

    /* byte counts */
    pModBus->aucData[iIdx++] = 2;
    
    /* 2.Value   */
    pModBus->aucData[iIdx++] = (ulValue >> 8) & 0XFF;
    pModBus->aucData[iIdx++] = (ulValue >> 0) & 0XFF;
    
    CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_PAKCET_ADDRESS_MC,APP_PAKCET_ADDRESS_EXE);
    
    iRet = CcbModbusWorkEntryWrapper(id,APP_CAN_CHL_INNER,ulIdentifier,aucModbusBuf,iIdx + MODBUS_PACKET_COMM_HDR_LEN,2000,CcbDefaultModbusCallBack); // don care modbus exe result
    
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
        /* notify ui (late implemnt) */
        return iRet;
    }

    /* update exe */
    for (iIdx = 0; iIdx < 1; iIdx++)
    {
       ExeBrd.ausHoldRegister[ulAddress] = ulValue;

       if (ulAddress >= APP_EXE_HOLD_REG_RPUMP_OFFSET)
       {
           CcbUpdateRPumpObjState(ulAddress - APP_EXE_HOLD_REG_RPUMP_OFFSET ,ulValue);
       }
       
    }
    return iRet;
}



int CCB::CcbGetExeHoldRegs(int id,unsigned int ulAddress,int Regs)
{
    unsigned int ulIdentifier;

    unsigned char aucModbusBuf[16];

    int iRet = -1;

    int iIdx = 0;

    MODBUS_PACKET_COMM_STRU *pModBus = (MODBUS_PACKET_COMM_STRU *)aucModbusBuf;

    iIdx = 0;
    /* 0. Node Address (2018/03/04 add for zigbee) */
    pModBus->ucAddress = APP_PAKCET_ADDRESS_EXE; 
    pModBus->ucFuncode       = MODBUS_FUNC_CODE_Read_Holding_Registers;
    
    /* 1.Reg Address */
    pModBus->aucData[iIdx++] = (ulAddress >> 8) & 0XFF; 
    pModBus->aucData[iIdx++] = (ulAddress >> 0) & 0XFF;

    /* 1.Reg NUM */
    pModBus->aucData[iIdx++] = (Regs >> 8) & 0XFF; 
    pModBus->aucData[iIdx++] = (Regs >> 0) & 0XFF;
    
    CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_PAKCET_ADDRESS_MC,APP_PAKCET_ADDRESS_EXE);
    
    iRet = CcbModbusWorkEntryWrapper(id,APP_CAN_CHL_INNER,ulIdentifier,aucModbusBuf,iIdx + MODBUS_PACKET_COMM_HDR_LEN,2000,CcbDefaultModbusCallBack); // don care modbus exe result
    
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
        /* notify ui (late implemnt) */
        return iRet;
    }

    /* decode modebus contents */
    {
        int iTmp = MODBUS_PACKET_COMM_HDR_LEN + 1; /* 1byet for code + 1byte for len */
        for (iIdx = 0; iIdx < Regs; iIdx++)
        {
            ExeBrd.ausHoldRegister[ulAddress + iIdx] = CcbGetModbus2BytesData(iTmp);
            iTmp += 2;
        }
    }

    return iRet;
}

int CCB::DispGetH7State(uint16_t &usState)
{
    int iIndex;
    for (iIndex = 0; iIndex < MAX_HANDLER_NUM;iIndex++)
    {
        int iRet;
        if (!aHandler[iIndex].bit1Active)
        {
            continue;
        }
        if (APP_DEV_HS_SUB_UP != aHandler[iIndex].iDevType)
        {
            continue;
        }

        iRet = CcbGetHandsetHoldRegs(iIndex,0,1);
        if (0 == iRet)
        {
            usState = aHandler[iIndex].ausHoldRegister[0];

            return 0;
        }
    }

    return -1;
}

int CCB::DispGetH8State(uint16_t &usState)
{
    int iIndex;
    for (iIndex = 0; iIndex < MAX_HANDLER_NUM;iIndex++)
    {
        int iRet;
        if (!aHandler[iIndex].bit1Active)
        {
            continue;
        }
        if (APP_DEV_HS_SUB_HP != aHandler[iIndex].iDevType)
        {
            continue;
        }

        iRet = CcbGetHandsetHoldRegs(iIndex,0,1);
        if (0 == iRet)
        {
            usState = aHandler[iIndex].ausHoldRegister[0];

            return 0;
        }
    }

    return -1;
}

int CCB::DispGetHandsetIndexByType(int iType)
{
    int iIndex ;
    for (iIndex = 0; iIndex < MAX_HANDLER_NUM;iIndex++)
    {
        //int iRet;
        if (!aHandler[iIndex].bit1Active)
        {
            continue;
        }
        if (iType != aHandler[iIndex].iDevType)
        {
            continue;
        }

        return iIndex;
    }

    return -1;
}


int CCB::CcbGetHandsetHoldRegs(int iIndex,unsigned int ulAddress,int Regs)
{
    unsigned int ulIdentifier;

    unsigned char aucModbusBuf[16];

    int iRet = -1;

    int iIdx = 0;

    MODBUS_PACKET_COMM_STRU *pModBus = (MODBUS_PACKET_COMM_STRU *)aucModbusBuf;

    if (aHandler[iIndex].bit1Active)
    {
        iIdx = 0;
        
        /* 0. Node Address (2018/03/04 add for zigbee) */
        pModBus->ucAddress = APP_PAKCET_ADDRESS_HS; 
        pModBus->ucFuncode = MODBUS_FUNC_CODE_Read_Holding_Registers;
        
        /* 1.Reg Address */
        pModBus->aucData[iIdx++] = (ulAddress >> 8) & 0XFF; 
        pModBus->aucData[iIdx++] = (ulAddress >> 0) & 0XFF;
    
        /* 1.Reg NUM */
        pModBus->aucData[iIdx++] = (Regs >> 8) & 0XFF; 
        pModBus->aucData[iIdx++] = (Regs >> 0) & 0XFF;
        
        CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_PAKCET_ADDRESS_MC,(iIndex + APP_HAND_SET_BEGIN_ADDRESS));
        
        iRet = CcbModbusWorkEntryWrapper(WORK_LIST_NUM,APP_CAN_CHL_OUTER,ulIdentifier,aucModbusBuf,iIdx + MODBUS_PACKET_COMM_HDR_LEN,2000,CcbDefaultModbusCallBack); // don care modbus exe result
        
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
            /* notify ui (late implemnt) */
            return iRet;
        }
    
        /* decode modebus contents */
        {
            int iTmp = MODBUS_PACKET_COMM_HDR_LEN + 1; /* 1byet for code + 1byte for len */
            for (iIdx = 0; iIdx < Regs; iIdx++)
            {
                if (0 == (ulAddress + iIdx))
                {
                    aHandler[iIndex].ausHoldRegister[ulAddress + iIdx] = CcbGetModbus2BytesData(iTmp);
                }
                iTmp += 2;
            }
        }
        
        LOG(VOS_LOG_DEBUG,"Handset%d: CcbGetHandsetHoldRegs state %d,type%d",iIndex,aHandler[iIndex].ausHoldRegister[ulAddress],aHandler[iIndex].iDevType);    
    }

    return iRet;
}



int CCB::CcbGetPmValue(int id,unsigned int ulAddress, int iPMs)
{
    int          iIdx = 0;
    int          iTmp;
    int          iRet;
    unsigned int ulIdentifier;
    unsigned char aucModbusBuf[32];
    

    MODBUS_PACKET_COMM_STRU *pModBus = (MODBUS_PACKET_COMM_STRU *)aucModbusBuf;

    /* get Bx from exe */
    iIdx = 0;
    /* 0. Node Address (2018/03/04 add for zigbee) */
    pModBus->ucAddress = APP_PAKCET_ADDRESS_EXE; 
    pModBus->ucFuncode = MODBUS_FUNC_CODE_Read_Input_Registers;
   
    /* 1.Start Address */
    pModBus->aucData[iIdx++] = (ulAddress >> 8) & 0XFF; 
    pModBus->aucData[iIdx++] = (ulAddress >> 0) & 0XFF;
    
    
    /* 2. Regs  */
    iTmp = iPMs;
    pModBus->aucData[iIdx++] = (iTmp >> 8) & 0XFF;
    pModBus->aucData[iIdx++] = (iTmp >> 0) & 0XFF;
    
    CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_PAKCET_ADDRESS_MC,APP_PAKCET_ADDRESS_EXE);

    
    iRet = CcbModbusWorkEntryWrapper(id,APP_CAN_CHL_INNER,ulIdentifier,aucModbusBuf,iIdx + MODBUS_PACKET_COMM_HDR_LEN,2000,CcbDefaultModbusCallBack); // don care modbus exe result
    
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
        /* notify ui (late implemnt) */
        return iRet;
    }

    iTmp = MODBUS_PACKET_COMM_HDR_LEN + 1; /* 1byet for code + 1byte for len */

    for (iIdx = 0; iIdx < iPMs; iIdx++)
    {
        if (APP_EXE_PM1_NO + ulAddress + iIdx < APP_EXE_PRESSURE_METER)
        {
            ExeBrd.aPMObjs[APP_EXE_PM1_NO + ulAddress + iIdx].Value.ulV = CcbGetModbus2BytesData(iTmp);
        }
        iTmp += 2;
    }

    return 0;

}

int CCB::CcbGetIValue(int id,unsigned int ulAddress, int iIs)
{
    int          iIdx = 0;
    int          iTmp;
    int          iRet;
    unsigned int ulIdentifier;
    unsigned char aucModbusBuf[32];
    

    MODBUS_PACKET_COMM_STRU *pModBus = (MODBUS_PACKET_COMM_STRU *)aucModbusBuf;

    /* get Bx from exe */
    iIdx = 0;
    /* 0. Node Address (2018/03/04 add for zigbee) */
    pModBus->ucAddress = APP_PAKCET_ADDRESS_EXE; 
    pModBus->ucFuncode = MODBUS_FUNC_CODE_Read_Input_Registers;
   
    /* 1.Start Address */
    pModBus->aucData[iIdx++] = (ulAddress >> 8) & 0XFF; 
    pModBus->aucData[iIdx++] = (ulAddress >> 0) & 0XFF;
    
    
    /* 2. Regs  */
    iTmp = iIs;
    pModBus->aucData[iIdx++] = (iTmp >> 8) & 0XFF;
    pModBus->aucData[iIdx++] = (iTmp >> 0) & 0XFF;
    
    CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_PAKCET_ADDRESS_MC,APP_PAKCET_ADDRESS_EXE);

    
    iRet = CcbModbusWorkEntryWrapper(id,APP_CAN_CHL_INNER,ulIdentifier,aucModbusBuf,iIdx + MODBUS_PACKET_COMM_HDR_LEN,2000,CcbDefaultModbusCallBack); // don care modbus exe result
    
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
        /* notify ui (late implemnt) */
        return iRet;
    }

    iTmp = MODBUS_PACKET_COMM_HDR_LEN + 1; /* 1byet for code + 1byte for len */

    ulAddress -= APP_EXE_INPUT_REG_QMI_OFFSET;

    for (iIdx = 0; iIdx < iIs; iIdx++)
    {
        if (APP_EXE_I1_NO + ulAddress + iIdx < APP_EXE_ECO_NUM)
        {
            un_data_type data;

            ulIdentifier = CcbGetModbus2BytesData(iTmp);
            data.auc[3] = (ulIdentifier >> 8) & 0xff;
            data.auc[2] = (ulIdentifier >> 0) & 0xff;
            ulIdentifier = CcbGetModbus2BytesData(iTmp+2);
            data.auc[1] = (ulIdentifier >> 8) & 0xff;
            data.auc[0] = (ulIdentifier >> 0) & 0xff;
            ExeBrd.aEcoObjs[APP_EXE_I1_NO + ulAddress + iIdx].Value.eV.fWaterQ = data.f;

            ulIdentifier = CcbGetModbus2BytesData(iTmp+2);
            ExeBrd.aEcoObjs[APP_EXE_I1_NO + ulAddress + iIdx].Value.eV.usTemp = (unsigned short)ulIdentifier;
            
        }
        iTmp += 6;
    }

    return 0;

}

int CCB::CcbGetFmValue(int id,unsigned int ulAddress, int iFMs)
{
    int          iIdx = 0;
    int          iTmp;
    int          iRet;
    unsigned int ulIdentifier;
    unsigned char aucModbusBuf[32];
    

    MODBUS_PACKET_COMM_STRU *pModBus = (MODBUS_PACKET_COMM_STRU *)aucModbusBuf;

    /* get Bx from exe */
    iIdx = 0;
    /* 0. Node Address (2018/03/04 add for zigbee) */
    pModBus->ucAddress = APP_PAKCET_ADDRESS_FM; 
    pModBus->ucFuncode = MODBUS_FUNC_CODE_Read_Input_Registers;
   
    /* 1.Start Address */
    pModBus->aucData[iIdx++] = (ulAddress >> 8) & 0XFF; 
    pModBus->aucData[iIdx++] = (ulAddress >> 0) & 0XFF;
    
    
    /* 2. Regs  */
    iTmp = iFMs*2; 
    pModBus->aucData[iIdx++] = (iTmp >> 8) & 0XFF;
    pModBus->aucData[iIdx++] = (iTmp >> 0) & 0XFF;
    
    CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_PAKCET_ADDRESS_MC,APP_PAKCET_ADDRESS_FM);

    
    iRet = CcbModbusWorkEntryWrapper(id,APP_CAN_CHL_INNER,ulIdentifier,aucModbusBuf,iIdx + MODBUS_PACKET_COMM_HDR_LEN,2000,CcbDefaultModbusCallBack); // don care modbus exe result
    
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
        /* notify ui (late implemnt) */
        return iRet;
    }

    iTmp = MODBUS_PACKET_COMM_HDR_LEN + 1; 

    for (iIdx = 0; iIdx < iFMs; iIdx++)
    {
        if (APP_FM_FM1_NO + ulAddress + iIdx < APP_EXE_PRESSURE_METER)
        {
            FlowMeter.aFmObjs[APP_FM_FM1_NO + ulAddress + iIdx].Value.ulV = CcbGetModbus4BytesData(iTmp);
        }
        iTmp += 4;
    }

    return 0;

}

int CCB::CcbGetDinState(int id)
{
    int          iIdx = 0;
    int          iTmp;
    int          iRet;
    unsigned int ulIdentifier;
    unsigned char aucModbusBuf[32];
    
    MODBUS_PACKET_COMM_STRU *pModBus = (MODBUS_PACKET_COMM_STRU *)aucModbusBuf;

    /* get Bx from exe */
    iIdx = 0;
    
    /* 0. Node Address (2018/03/04 add for zigbee) */
    pModBus->ucAddress = APP_PAKCET_ADDRESS_EXE; 
    pModBus->ucFuncode = MODBUS_FUNC_CODE_Read_Input_Registers;
   
    /* 1.Start Address */
    pModBus->aucData[iIdx++] = (APP_EXE_INPUT_REG_DIN_OFFSET >> 8) & 0XFF; 
    pModBus->aucData[iIdx++] = (APP_EXE_INPUT_REG_DIN_OFFSET >> 0) & 0XFF;
    
    /* 2.Regs  */
    iTmp = 1;
    pModBus->aucData[iIdx++] = (iTmp >> 8) & 0XFF;
    pModBus->aucData[iIdx++] = (iTmp >> 0) & 0XFF;
    
    CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_PAKCET_ADDRESS_MC,APP_PAKCET_ADDRESS_EXE);

    iRet = CcbModbusWorkEntryWrapper(id,APP_CAN_CHL_INNER,ulIdentifier,aucModbusBuf,iIdx + MODBUS_PACKET_COMM_HDR_LEN,2000,CcbDefaultModbusCallBack); // don care modbus exe result
    
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
        /* notify ui (late implemnt) */
        return iRet;
    }

    iTmp = MODBUS_PACKET_COMM_HDR_LEN + 1; 

    ExeBrd.ucDinState = CcbGetModbus2BytesData(iTmp);

    ExeBrd.ucDinState = (~(ExeBrd.ucDinState)) & APP_EXE_DIN_MASK;

    return 0;
}

void CCB::CanCcbPrintFsm()
{
    char buffer[1024];
    int iIdx = 0;

    switch(curWorkState.iMainWorkState)
    {
    case DISP_WORK_STATE_IDLE:
        iIdx += sprintf(&buffer[iIdx],"Main:%s","DISP_WORK_STATE_IDLE ");
        break;
    case DISP_WORK_STATE_RUN:
        iIdx += sprintf(&buffer[iIdx],"Main:%s","DISP_WORK_STATE_RUN ");
        break;
    case DISP_WORK_STATE_LPP:
        iIdx += sprintf(&buffer[iIdx],"Main:%s","DISP_WORK_STATE_LPP ");
        break;
    case DISP_WORK_STATE_KP:
        iIdx += sprintf(&buffer[iIdx],"Main:%s","DISP_WORK_STATE_KP ");
        break;
    case DISP_WORK_STATE_PREPARE:
        iIdx += sprintf(&buffer[iIdx],"Main:%s","DISP_WORK_STATE_PREPARE ");
        break;
    }

    switch(curWorkState.iSubWorkState)
    {
    case DISP_WORK_SUB_IDLE:
        iIdx += sprintf(&buffer[iIdx],"Sub:%s","DISP_WORK_SUB_IDLE ");
        break;
    case DISP_WORK_SUB_WASH:
        iIdx += sprintf(&buffer[iIdx],"Sub:%s","DISP_WORK_SUB_WASH ");
        break;
    case DISP_WORK_SUB_IDLE_DEPRESSURE:
        iIdx += sprintf(&buffer[iIdx],"Sub:%s","DISP_WORK_SUB_IDLE_DEPRESSURE ");
        break;
    case DISP_WORK_SUB_RUN_INIT:
        iIdx += sprintf(&buffer[iIdx],"Sub:%s","DISP_WORK_SUB_RUN_INIT ");
        break;
    case DISP_WORK_SUB_RUN_QTW:
        iIdx += sprintf(&buffer[iIdx],"Sub:%s","DISP_WORK_SUB_RUN_QTW ");
        break;
    case DISP_WORK_SUB_RUN_QTWING:
        iIdx += sprintf(&buffer[iIdx],"Sub:%s","DISP_WORK_SUB_RUN_QTWING ");
        break;
    case DISP_WORK_SUB_RUN_CIR:
        iIdx += sprintf(&buffer[iIdx],"Sub:%s","DISP_WORK_SUB_RUN_CIR ");
        break;
    case DISP_WORK_SUB_RUN_CIRING:
        iIdx += sprintf(&buffer[iIdx],"Sub:%s","DISP_WORK_SUB_RUN_CIRING ");
        break;
    case DISP_WORK_SUB_RUN_DEC:
        iIdx += sprintf(&buffer[iIdx],"Sub:%s","DISP_WORK_SUB_RUN_DEC ");
        break;
    }   
    
    LOG(VOS_LOG_INFO,"%s",buffer);  
}

void CCB::CanCcbTransState(int iDstMainState,int iDstSubState)
{
    int aiCont[2] = {iDstMainState,iDstSubState};

    curWorkState.iMainWorkState = iDstMainState;
    curWorkState.iSubWorkState  = iDstSubState;

    CanCcbPrintFsm();

    /* do stuff here */
    switch(iDstMainState)
    {
    case DISP_WORK_STATE_RUN:
        switch(iDstSubState)
        {
        case DISP_WORK_SUB_IDLE:
            //ulB2FullTick = gulSecond;
            break;
        }
        break;
    case DISP_WORK_STATE_LPP:
        ulLPPTick = gulSecond;
        break;
    default:
        break;
    }

    /* Notify Ui */
    // if (((int)this) != tls_getkey(&pthread_tno_key))
    {
        MainSndWorkMsg(WORK_MSG_TYPE_STATE,(unsigned char *)aiCont,sizeof(aiCont));
    }
}


void CCB::CanCcbTransState4Pw(int iDstMainState,int iDstSubState)
{
    int aiCont[2] = {iDstMainState,iDstSubState};

    curWorkState.iMainWorkState4Pw = iDstMainState;
    curWorkState.iSubWorkState4Pw  = iDstSubState;

    MainSndWorkMsg(WORK_MSG_TYPE_STATE4PW,(unsigned char *)aiCont,sizeof(aiCont));
}


DISPHANDLE CCB::SearchWork(work_proc proc)
{
    int iLoop;
    sp_thread_mutex_lock(&WorkMutex);
    for (iLoop = 0; iLoop < WORK_LIST_NUM; iLoop++)
    {
        list_t *pos, *n;
        list_for_each_safe(pos,n,&WorkList[iLoop])
        {
            WORK_ITEM_STRU *pWorkItem = list_entry(pos,WORK_ITEM_STRU,list) ;

            if ((pWorkItem->proc == proc)
                && (!(pWorkItem->flag & WORK_FLAG_CANCEL)))
            {
                sp_thread_mutex_unlock(&WorkMutex);
                
                return (DISPHANDLE)pWorkItem;
            }
        }
    }
    sp_thread_mutex_unlock(&WorkMutex);

    return DISP_INVALID_HANDLE; 
}

/*
  Protect by Mutex
*/
void CCB::CcbDefaultCancelWork(void *param)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)param;
    CCB *pCcb = (CCB *)pWorkItem->para;

    pCcb->CcbAbortWork(pWorkItem);
}

WORK_ITEM_STRU *CCB::CcbAllocWorkItem(void)
{
   WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)malloc(sizeof(WORK_ITEM_STRU));

   if (!pWorkItem) return NULL;

   memset(pWorkItem,0,sizeof(WORK_ITEM_STRU));

   /* Do all Init stuff here */
   INIT_LIST_HEAD(&pWorkItem->list);

   pWorkItem->proc   = NULL;
   pWorkItem->cancel = CcbDefaultCancelWork;
   pWorkItem->flag   = 0;
   pWorkItem->id     = 0;
   pWorkItem->para   = this;
   pWorkItem->extra  = NULL;

   return pWorkItem;
}


void CCB::CcbScheduleWork(void)
{
     int iLoop;
     
     for (iLoop = 0; iLoop < WORK_LIST_NUM; iLoop++)
     {
         if (iBusyWork & (1 << iLoop))
         {
             continue;
         }

         if (!list_empty(&WorkList[iLoop]))
         {
             int iRet;
             WORK_ITEM_STRU *pWorkItem = list_entry(WorkList[iLoop].next,WORK_ITEM_STRU,list) ;

             iRet = Task_DispatchWorkTask(WorkCommEntry,pWorkItem);
             if (iRet)
             {
                 /* dispatch failed */
                 LOG(VOS_LOG_WARNING,"dispatch_threadpool Fail %d",iRet);    
             }
         }
     }

     /* notify ui (late implement) */
}


void CCB::CcbAddWork(int iList,WORK_ITEM_STRU *pWorkItem)
{
    sp_thread_mutex_lock(&WorkMutex);
    pWorkItem->id = iList;
    list_add_tail(&pWorkItem->list,&WorkList[iList]);
    sp_thread_mutex_unlock(&WorkMutex);
     
    CcbScheduleWork();
}

void CCB::WorkCommEntry(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;
    CCB *pCcb = (CCB *)pWorkItem->para;

    /* prevent repeated excuting */
    sp_thread_mutex_lock(&pCcb->WorkMutex);
    if (pCcb->iBusyWork & (1 << pWorkItem->id))
    {
        sp_thread_mutex_unlock(&pCcb->WorkMutex);
        return ;
    }
    pCcb->iBusyWork |= 1 << pWorkItem->id;
    sp_thread_mutex_unlock(&pCcb->WorkMutex);

    pWorkItem->flag |= WORK_FLAG_ACTIVE;

    if (!(pWorkItem->flag & WORK_FLAG_CANCEL))
    {
        pWorkItem->proc(pWorkItem);
    }

    sp_thread_mutex_lock(&pCcb->WorkMutex);
    pCcb->iBusyWork &= ~(1 << pWorkItem->id);
    list_del_init(&pWorkItem->list);
    sp_thread_mutex_unlock(&pCcb->WorkMutex);

    free(pWorkItem);

    pCcb->CcbScheduleWork();
}

/*********************************************************************
 * Function:        void work_stop_pw(void *para)
 *
 * PreCondition:    None
 *
 * Input:           para  type of WORK_ITEM_STRU 
 *
 * Output:          Stop producing water
 *
 * Side Effects:    None
 *
 * Overview:        Stop producing water
 *
 * Note:            None.
 ********************************************************************/
void CCB::work_stop_pw(void *para)
{
    int iTmp;

    int iRet;

    int aiCont[1] = {0};

    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;

    CCB *pCcb = (CCB *)pWorkItem->para;

    iTmp = pCcb->ulRunMask;

    iRet = pCcb->CcbUpdateSwitch(pWorkItem->id,0,iTmp,0);
    
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);    
        /* notify ui (late implemnt) */
    }

    //2018.11.22
    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_EDI:
        iTmp  = (1 << APP_EXE_I1_NO)|(1<<APP_EXE_I2_NO)|(1<<APP_EXE_I3_NO);
        iRet = pCcb->CcbUpdateIAndBs(pWorkItem->id, 0, iTmp, 0);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbUpdateIAndBs Fail %d",iRet);
            /* notify ui (late implemnt) */
        }
        break;
    case MACHINE_UP:
    case MACHINE_RO:
    case MACHINE_RO_H:
    case MACHINE_C:
        iTmp  = (1 << APP_EXE_I1_NO)|(1<<APP_EXE_I2_NO);
        iRet = pCcb->CcbUpdateIAndBs(pWorkItem->id, 0, iTmp, 0);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbUpdateIAndBs Fail %d",iRet);
            /* notify ui (late implemnt) */
        }
        break;
    case MACHINE_PURIST:
    case MACHINE_C_D:
        break;
    }
    //end

    pCcb->MainSndWorkMsg(WORK_MSG_TYPE_EPW,(unsigned char *)aiCont,sizeof(aiCont));

    pCcb->bit1ProduceWater        = FALSE; 
    pCcb->bit1B1Check4RuningState = FALSE;

    for (iTmp = 0; iTmp < APP_FM_FLOW_METER_NUM; iTmp++)
    {
       pCcb->FlowMeter.aulPwEnd[iTmp] = pCcb->FlowMeter.aHist[iTmp].curValue.ulV;
    }

}

DISPHANDLE CCB::CcbInnerWorkStopProduceWater()
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc = work_stop_pw;
    pWorkItem->para = this;

    CcbAddWork(WORK_LIST_LP,pWorkItem);

    return (DISPHANDLE)pWorkItem;

}

void CCB::work_fill_water_fail(int iWorkId)
{
    int aiCont[1] = {-1};
    
    if (CcbGetSwitchObjState((1 << APP_EXE_E10_NO)))
    {
        CcbUpdateSwitch(iWorkId,0,(1 << APP_EXE_E10_NO),0);
    }
    
    if (CcbGetPmObjState((1 << APP_EXE_PM3_NO)))
    {
        CcbUpdateIAndBs(iWorkId,0,GET_B_MASK(APP_EXE_PM3_NO),0);
    }
    
    MainSndWorkMsg(WORK_MSG_TYPE_SFW,(unsigned char *)aiCont,sizeof(aiCont));

}

void CCB::work_fill_water_succ(void)
{
    int aiCont[1] = {0};
    
    MainSndWorkMsg(WORK_MSG_TYPE_SFW,(unsigned char *)aiCont,sizeof(aiCont));

}


void CCB::work_start_fill_water(void *para)
{
    int iTmp;
    int iRet;

    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;
    CCB *pCcb = (CCB *)pWorkItem->para;
    
    
    /* late implemnt */
    /* E10 ON*/
    /* set  valves */
    iTmp = (1 << APP_EXE_E10_NO);
    iRet = pCcb->CcbUpdateSwitch(pWorkItem->id, 0,iTmp,iTmp);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);    
        pCcb->work_fill_water_fail(pWorkItem->id);
        return ;
    }

    iTmp  = (GET_B_MASK(APP_EXE_PM3_NO));
    iRet = pCcb->CcbUpdateIAndBs(pWorkItem->id,0,iTmp,iTmp);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbUpdateIAndBs Fail %d",iRet);    
        /* notify ui (late implemnt) */
        pCcb->work_fill_water_fail(pWorkItem->id);
        return ;
    }     

    pCcb->bit1FillingTank = TRUE;

    pCcb->work_fill_water_succ();
}


DISPHANDLE CCB::CcbInnerWorkStartFillWater()
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc = work_start_fill_water;
    pWorkItem->para = this;

    CcbAddWork(WORK_LIST_URGENT,pWorkItem);

    return (DISPHANDLE)pWorkItem;
}

void CCB::work_stop_water_succ(void)
{
    int aiCont[1] = {0};
    
    MainSndWorkMsg(WORK_MSG_TYPE_EFW,(unsigned char *)aiCont,sizeof(aiCont));

}

void CCB::work_stop_fill_water(void *para)
{
    int iTmp;
    int iRet;

    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;
    CCB *pCcb = (CCB *)pWorkItem->para;
    
    /* late implemnt */
    /* E10 ON        */
    /* set  valves   */
    iTmp = (1 << APP_EXE_E10_NO);
    iRet = pCcb->CcbUpdateSwitch(pWorkItem->id,0,iTmp,0);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);    
    }
    //2019.12.2 修改，停止取水时不需要关闭原水箱检测
#if 0
    iTmp  = (GET_B_MASK(APP_EXE_PM3_NO));
    iRet = pCcb->CcbUpdateIAndBs(pWorkItem->id,0,iTmp,0);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbUpdateIAndBs Fail %d",iRet);    
        /* notify ui (late implemnt) */
    }     
#endif
    pCcb->bit1FillingTank = FALSE;
    pCcb->bit1NeedFillTank = FALSE;

    pCcb->work_stop_water_succ();
    
}


DISPHANDLE CCB::CcbInnerWorkStopFillWater()
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc = work_stop_fill_water;
    pWorkItem->para = this;

    CcbAddWork(WORK_LIST_URGENT,pWorkItem);

    return (DISPHANDLE)pWorkItem;
}


void CCB::work_N3_fail(int iWorkId)
{
    int aiCont[1] = {-1};
    
    if (CcbGetSwitchObjState((1 << APP_EXE_N3_NO)))
    {
        CcbUpdateSwitch(iWorkId,0,(1 << APP_EXE_N3_NO),0);
    }
    
    MainSndWorkMsg(WORK_MSG_TYPE_SN3,(unsigned char *)aiCont,sizeof(aiCont));

}

void CCB::work_N3_succ(void)
{
    int aiCont[1] = {0};
    
    MainSndWorkMsg(WORK_MSG_TYPE_SN3,(unsigned char *)aiCont,sizeof(aiCont));

}


void CCB::work_start_N3(void *para)
{
    int iTmp;
    int iRet;
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;
    CCB *pCcb = (CCB *)pWorkItem->para;
    
    /* late implemnt */
    /* E10 ON*/
    /* set  valves */
    iTmp = (1 << APP_EXE_N3_NO);
    iRet = pCcb->CcbUpdateSwitch(pWorkItem->id,0,iTmp,iTmp);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);  
        
        pCcb->work_N3_fail(pWorkItem->id);
        return ;
    }
    pCcb->work_N3_succ();
}


DISPHANDLE CCB::CcbInnerWorkStartN3()
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc = work_start_N3;
    pWorkItem->para = this;

    CcbAddWork(WORK_LIST_URGENT,pWorkItem);

    return (DISPHANDLE)pWorkItem;
}

void CCB::work_stop_N3_succ(void)
{
    int aiCont[1] = {0};
    
    MainSndWorkMsg(WORK_MSG_TYPE_EN3,(unsigned char *)aiCont,sizeof(aiCont));

}

void CCB::work_stop_N3(void *para)
{
    int iTmp;
    int iRet;

    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;
    CCB *pCcb = (CCB *)pWorkItem->para;
    
    /* late implemnt */
    /* E10 ON        */
    /* set  valves   */
    iTmp = (1 << APP_EXE_N3_NO);
    iRet = pCcb->CcbUpdateSwitch(pWorkItem->id,0,iTmp,0);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);    
    }

    pCcb->work_stop_N3_succ();
}


DISPHANDLE CCB::CcbInnerWorkStopN3()
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc = work_stop_N3;
    pWorkItem->para = this;

    CcbAddWork(WORK_LIST_URGENT,pWorkItem);

    return (DISPHANDLE)pWorkItem;
}


void CCB::work_qtw_fail(int code,int iIndex,int iWorkId)
{
    int aiCont[2] = {code,iIndex};

    unsigned int ulTmp;

    if (APP_DEV_HS_SUB_UP == aHandler[iIndex].iDevType)
    {
        ulTmp = ulHyperTwMask;
    }
    else
    {
        ulTmp = ulNormalTwMask;
    }
    
    if (CcbGetSwitchObjState(APP_EXE_SWITCHS_MASK))
    {
        CcbUpdateSwitch(iWorkId,0,ulTmp,0);
    }
    
    if (CcbGetIObjState((1 << APP_EXE_I4_NO)|(1 << APP_EXE_I5_NO)))
    {
        CcbUpdateIAndBs(iWorkId,0,(1 << APP_EXE_I4_NO)|(1 << APP_EXE_I5_NO),0);
    }

    if (CcbGetFmObjState(APP_FM_FM1_NO))
    {
        CcbUpdateFms(iWorkId,0,APP_FM_FM1_NO,0);
    }

    CanCcbTransState4Pw(DISP_WORK_STATE_RUN,DISP_WORK_SUB_IDLE);

    MainSndWorkMsg(WORK_MSG_TYPE_START_QTW,(unsigned char *)aiCont,sizeof(aiCont));

}

void CCB::work_qtw_succ(int iIndex)
{
    int aiCont[2] = {0,iIndex};
    
    MainSndWorkMsg(WORK_MSG_TYPE_START_QTW,(unsigned char *)aiCont,sizeof(aiCont));
}

void CCB::work_stop_qtw_succ(int iIndex)
{
    int aiCont[2] = {0,iIndex};
    
    MainSndWorkMsg(WORK_MSG_TYPE_STOP_QTW,(unsigned char *)aiCont,sizeof(aiCont));
}

void CCB::work_start_qtw_helper(WORK_ITEM_STRU *pWorkItem)
{
    int iIndex = (int)pWorkItem->extra;
    unsigned int ulQtwSwMask = 0;
    int iTmp;
    int iRet;

    bit1B2Empty = FALSE;
    bit1AlarmUP = FALSE;

    CcbInitHandlerQtwMeas(iIndex);

    CanCcbTransState4Pw(DISP_WORK_STATE_RUN,DISP_WORK_SUB_RUN_QTWING);

    if(aHandler[iIndex].iDevType == APP_DEV_HS_SUB_UP)
    {
        gEx_Ccb.Ex_Auto_Cir_Tick.ulUPAutoCirTick = ex_gulSecond; //UP Auto cir 60min
    }

    if((gGlobalParam.iMachineType == MACHINE_C) && (aHandler[iIndex].iDevType == APP_DEV_HS_SUB_HP))
    {
        gEx_Ccb.Ex_Auto_Cir_Tick.ulHPAutoCirTick = ex_gulSecond; //NuZar C HP Auto cir 60min
    }

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_PURIST:
    case MACHINE_C_D:
        if (haveB2())
        {
            iRet = CcbGetPmValue(pWorkItem->id,APP_EXE_PM2_NO,1);
            if (iRet )
            {
                VOS_LOG(VOS_LOG_WARNING,"CcbGetPmValue Fail %d",iRet);  
                work_qtw_fail(APP_PACKET_HO_ERROR_CODE_TANK_EMPTY,iIndex,pWorkItem->id);        
                return ;
            }
        
            if (CcbConvert2Pm2SP(ExeBrd.aPMObjs[APP_EXE_PM2_NO].Value.ulV) < CcbGetSp6())
            {
                bit1B2Empty = TRUE;
                work_qtw_fail(APP_PACKET_HO_ERROR_CODE_TANK_EMPTY,iIndex,pWorkItem->id);    
                return;
            }
        }

        iTmp = (1<<APP_EXE_E4_NO)|(1<<APP_EXE_E6_NO)|(1<<APP_EXE_C2_NO)|(1<<APP_EXE_N2_NO);
        
        if (!haveB2())
        {

            iTmp |= (1<<APP_EXE_E10_NO);
        }

        iRet = CcbUpdateSwitch(pWorkItem->id, 0, ulHyperTwMask, iTmp);
        if (iRet )
        {
            VOS_LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);    
            work_qtw_fail(APP_PACKET_HO_ERROR_CODE_UNKNOW,iIndex,pWorkItem->id);
            return ;
        }

        if ((iTmp & (1 << APP_EXE_C2_NO))
           && bit1CirSpeedAdjust)
        {
            CcbC2Regulator(pWorkItem->id,24,TRUE);
        }

        iTmp = (1 << APP_EXE_I2_NO)|(1 << APP_EXE_I4_NO)|(1 << APP_EXE_I5_NO);

        iRet = CcbUpdateIAndBs(pWorkItem->id,0,iTmp,iTmp);
        if (iRet )
        {
            VOS_LOG(VOS_LOG_WARNING,"CcbUpdateIAndBs Fail %d",iRet);
            work_qtw_fail(APP_PACKET_HO_ERROR_CODE_UNKNOW, iIndex, pWorkItem->id);        
            return ;
        }
    
        iTmp = (1 << APP_FM_FM1_NO); /* start flow report */
        
        iRet = CcbUpdateFms(pWorkItem->id,0,iTmp,iTmp);
        if (iRet )
        {
            VOS_LOG(VOS_LOG_WARNING,"CcbUpdateFms Fail %d",iRet);    
            work_qtw_fail(APP_PACKET_HO_ERROR_CODE_UNKNOW, iIndex, pWorkItem->id);        
            return ;
        }
        iCurTwIdx = iIndex;
        work_qtw_succ(iIndex);
        if(QtwMeas.ulTotalFm != INVALID_FM_VALUE)
        {
            int iValue = (QtwMeas.ulTotalFm  * 1000) / (gGlobalParam.Caliparam.pc[ DISP_PC_COFF_S1].fk * 1000/60);
            emit startQtwTimer(iValue);
        }
        break;
    default:
        /* check B2 & get B2 reports from exe */
        if (haveB2())
        {
            iRet = CcbGetPmValue(pWorkItem->id,APP_EXE_PM2_NO,1);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbGetPmValue Fail %d",iRet);  
        
                work_qtw_fail(APP_PACKET_HO_ERROR_CODE_TANK_EMPTY,iIndex,pWorkItem->id);        
                return ;
            }
        
            if (CcbConvert2Pm2SP(ExeBrd.aPMObjs[APP_EXE_PM2_NO].Value.ulV) < CcbGetSp6())
            {
                bit1B2Empty = TRUE;
            
                work_qtw_fail(APP_PACKET_HO_ERROR_CODE_TANK_EMPTY,iIndex,pWorkItem->id);        
                return;
            }
        }
    
        /* set  switchs */
        if (APP_DEV_HS_SUB_UP == aHandler[iIndex].iDevType)
        {
            iTmp = (1<<APP_EXE_E4_NO)|(1<<APP_EXE_E6_NO)|(1<<APP_EXE_C2_NO)|(1<<APP_EXE_N2_NO);
            ulQtwSwMask = ulHyperTwMask;
        }
        else
        {
            if(gGlobalParam.iMachineType != MACHINE_C)
            {
                iTmp = (1<<APP_EXE_E4_NO)|(1<<APP_EXE_C2_NO);
            }
            else
            {
                iTmp = (1<<APP_EXE_E4_NO)|(1<<APP_EXE_C2_NO)|(1<<APP_EXE_N2_NO);
            }
            
            ulQtwSwMask = ulNormalTwMask;
        }
        
        iRet = CcbUpdateSwitch(pWorkItem->id,0,ulQtwSwMask,iTmp);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);    
            work_qtw_fail(APP_PACKET_HO_ERROR_CODE_UNKNOW,iIndex,pWorkItem->id);
            return ;
        }

        if ((ulQtwSwMask & (1 << APP_EXE_C2_NO))
           && bit1CirSpeedAdjust)
        {
            CcbC2Regulator(pWorkItem->id,24,TRUE);
        }

        iTmp = (1 << APP_EXE_I4_NO)|(1 << APP_EXE_I5_NO);

        switch(gGlobalParam.iMachineType)
        {
        case MACHINE_PURIST:
        case MACHINE_C_D:
            iTmp |= (1 << APP_EXE_I2_NO);
            break;
        case MACHINE_UP:
        case MACHINE_RO:
        case MACHINE_RO_H:
        case MACHINE_C:
            iTmp |= (1 << APP_EXE_I3_NO);
            break;
        default:

            break;
        }

        iRet = CcbUpdateIAndBs(pWorkItem->id,0,iTmp,iTmp);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbUpdateIAndBs Fail %d",iRet);
            
            /* notify ui (late implemnt) */
            work_qtw_fail(APP_PACKET_HO_ERROR_CODE_UNKNOW,iIndex,pWorkItem->id);        
            return ;
        }
    
        iTmp = (1 << APP_FM_FM1_NO); /* start flow report */
        
        iRet = CcbUpdateFms(pWorkItem->id,0,iTmp,iTmp);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbUpdateFms Fail %d",iRet);    
            /* notify ui (late implemnt) */
            work_qtw_fail(APP_PACKET_HO_ERROR_CODE_UNKNOW,iIndex,pWorkItem->id);        
            return ;
        }
    
        iCurTwIdx = iIndex;
    
        work_qtw_succ(iIndex);    
        
        if(QtwMeas.ulTotalFm != INVALID_FM_VALUE)
        {
            int iValue = (QtwMeas.ulTotalFm  * 1000) / (gGlobalParam.Caliparam.pc[ DISP_PC_COFF_S1].fk * 1000/60);
            emit startQtwTimer(iValue);
        }
        
        break;
    }
    
}


void CCB::work_start_qtw(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;

    CCB *pCcb = (CCB *)pWorkItem->para;

    pCcb->work_start_qtw_helper(pWorkItem);

}

DISPHANDLE CCB::CcbInnerWorkStartQtw(int iIndex)
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc  = work_start_qtw;
    pWorkItem->para  = this;
    pWorkItem->extra = (void *)iIndex;

    CcbAddWork(WORK_LIST_HP,pWorkItem);

    return (DISPHANDLE)pWorkItem;
}

void CCB::work_stop_qtw(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;

    CCB *pCcb = (CCB *)pWorkItem->para;

    int iTmp;
    int iRet;

    int iIndex = (int)pWorkItem->extra;

    if (APP_DEV_HS_SUB_UP == pCcb->aHandler[iIndex].iDevType)
    {
        iTmp = pCcb->ulHyperTwMask;
    }
    else
    {
        iTmp = pCcb->ulNormalTwMask;
    }

    pCcb->CcbFiniHandlerQtwMeas(iIndex);
    
    /* late implemnt */
    /* E10 ON        */
    /* set  valves   */
    iRet = pCcb->CcbUpdateSwitch(pWorkItem->id,0,iTmp,0);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);    
    }

    iTmp  = (1 << APP_EXE_I4_NO) | (1 << APP_EXE_I5_NO);

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_PURIST:
    case MACHINE_C_D:
        iTmp |= (1 << APP_EXE_I2_NO);
        break;
    case MACHINE_UP:
    case MACHINE_RO:
    case MACHINE_RO_H:
    case MACHINE_C:
        iTmp |= (1 << APP_EXE_I3_NO);
        break;
    default:
        break;
    }

    iRet = pCcb->CcbUpdateIAndBs(pWorkItem->id,0,iTmp,0);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbSetIAndBs Fail %d",iRet);    
        /* notify ui (late implemnt) */
    }     

    iTmp  = (1 << APP_FM_FM1_NO);
    iRet = pCcb->CcbUpdateFms(pWorkItem->id,0,iTmp,0);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbSetIAndBs Fail %d",iRet);    
        /* notify ui (late implemnt) */
    }

    pCcb->work_stop_qtw_succ(iIndex);

    pCcb->ulAdapterAgingCount = gulSecond;
}

DISPHANDLE CCB::CcbInnerWorkStopQtw(int iIndex)
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }
    
    LOG(VOS_LOG_WARNING,"CcbInnerWorkStopQtw");    

    pWorkItem->proc = work_stop_qtw;
    pWorkItem->para = this;
    pWorkItem->extra = (void *)iIndex;

    CcbAddWork(WORK_LIST_HP,pWorkItem);

    return (DISPHANDLE)pWorkItem;
}

void CCB::work_cir_fail(int iWorkId)
{
    int aiCont[1] = {-1};
    
    if (CcbGetSwitchObjState(APP_EXE_INNER_SWITCHS))
    {
        CcbUpdateSwitch(iWorkId,0,ulCirMask,0);
    }
    
    if (CcbGetIObjState((1 << APP_EXE_I4_NO)|(1 << APP_EXE_I5_NO)))
    {
        int iTmp = (1 << APP_EXE_I4_NO)|(1 << APP_EXE_I5_NO);
        CcbUpdateIAndBs(iWorkId,0,iTmp,0);
    }

    CanCcbTransState4Pw(DISP_WORK_STATE_RUN,DISP_WORK_SUB_IDLE);

    MainSndWorkMsg(WORK_MSG_TYPE_SCIR,(unsigned char *)aiCont,sizeof(aiCont));

}

void CCB::work_cir_succ()
{
    int aiCont[1] = {0};
    
    MainSndWorkMsg(WORK_MSG_TYPE_SCIR,(unsigned char *)aiCont,sizeof(aiCont));
}

void CCB::work_start_toc_cir(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;

    CCB *pCcb = (CCB *)pWorkItem->para;

    int iTocStage = (int)pWorkItem->extra;

    int iTmp;
    int iRet;

    pCcb->iTocStage   = iTocStage;

    pCcb->iTocStageTimer = 0;

    LOG(VOS_LOG_DEBUG,"%s : %d",__FUNCTION__,iTocStage);  
    

    switch(iTocStage)
    {
    case APP_PACKET_EXE_TOC_STAGE_OXDIZATION:
        iTmp = (1 << APP_EXE_E5_NO) | (1 << APP_EXE_C2_NO)| (1 << APP_EXE_N2_NO);
       
        iRet = pCcb->CcbUpdateSwitch(pWorkItem->id,0,pCcb->ulCirMask,iTmp);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);    
            pCcb->work_cir_fail(pWorkItem->id);
            return ;
        }

        /* tell exe to calc toc */
                
        break;
    case APP_PACKET_EXE_TOC_STAGE_FLUSH2:
         iTmp = (1 << APP_EXE_E5_NO) | (1 << APP_EXE_E6_NO) | (1 << APP_EXE_C2_NO)| (1 << APP_EXE_N2_NO);
 
         iRet = pCcb->CcbUpdateSwitch(pWorkItem->id,0,pCcb->ulCirMask,iTmp);
         if (iRet )
         {
             LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);    
             pCcb->work_cir_fail(pWorkItem->id);
             return ;
         }
         
        break;
    }
    
    pCcb->DispSetTocState(iTocStage);
}

void CCB::work_start_cir_helper(WORK_ITEM_STRU *pWorkItem)
{
    int iTmp;
    int iRet;
    int iLoop; //ex

    bit1I54Cir   = FALSE;
    iCirType = (int)pWorkItem->extra;
    bit1B2Empty  = FALSE;
    bit1AlarmTOC = FALSE;

    CanCcbTransState4Pw(DISP_WORK_STATE_RUN,DISP_WORK_SUB_RUN_CIRING);

    if (haveB2())
    {
        if (CcbConvert2Pm2SP(ExeBrd.aPMObjs[APP_EXE_PM2_NO].Value.ulV) < CcbGetSp6())
        {
            bit1B2Empty = TRUE;
        
            work_cir_fail(pWorkItem->id);
    
            LOG(VOS_LOG_WARNING,"< CcbGetSp6 ");    
            
            return;
        }  
    }

    if(iCirType == CIR_TYPE_UP)
    {
        gEx_Ccb.Ex_Auto_Cir_Tick.ulUPAutoCirTick = ex_gulSecond; //UP Auto cir 60min
    }

    if((gGlobalParam.iMachineType == MACHINE_C) && (iCirType == CIR_TYPE_HP))
    {
        gEx_Ccb.Ex_Auto_Cir_Tick.ulHPAutoCirTick = ex_gulSecond; //NuZar C HP Auto cir 60min
    }
        
    switch(iCirType)
    {
    case CIR_TYPE_UP:
         if (haveTOC())
         {           
             iTmp = (1 << APP_EXE_E5_NO) | (1 << APP_EXE_E6_NO)| (1 << APP_EXE_C2_NO);
     
             iRet = CcbUpdateSwitch(pWorkItem->id,0,ulCirMask ,iTmp);
             if (iRet )
             {
                 LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);    
                 work_cir_fail(pWorkItem->id);
                 return ;
             }

             if ((ulCirMask & (1 << APP_EXE_C2_NO))
                && bit1CirSpeedAdjust)
             {
                 CcbC2Regulator(pWorkItem->id,8,TRUE);
             }

             iTmp = (1 << APP_EXE_I4_NO)|(1 << APP_EXE_I5_NO);
             iRet = CcbUpdateIAndBs(pWorkItem->id,0,iTmp,iTmp);
             if (iRet )
             {
                 LOG(VOS_LOG_WARNING,"CcbUpdateIAndBs Fail %d",iRet);    
                 /* notify ui (late implemnt) */
                 work_cir_fail(pWorkItem->id);        
                 return ;
             }

             iTmp = (1 << APP_FM_FM1_NO); /* start flow report */

             iRet = CcbUpdateFms(pWorkItem->id,0,iTmp,iTmp);
             if (iRet )
             {
                 LOG(VOS_LOG_WARNING,"CcbUpdateFms Fail %d",iRet);
                 /* notify ui (late implemnt) */
                 work_cir_fail(pWorkItem->id);
                 return ;
             }

             iTocStage      = APP_PACKET_EXE_TOC_STAGE_FLUSH1;
             iTocStageTimer = 0;
             bit1TocOngoing = 1;
             /* toc3 left to main loop */
             
         }
         else
         {
             iTmp = (1 << APP_EXE_E5_NO) | (1 << APP_EXE_E6_NO) | (1 << APP_EXE_C2_NO);
             iRet = CcbUpdateSwitch(pWorkItem->id,0,ulCirMask,iTmp);
             if (iRet )
             {
                 LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);    
                 work_cir_fail(pWorkItem->id);
                 return ;
             }

             if ((ulCirMask & (1 << APP_EXE_C2_NO))
                && bit1CirSpeedAdjust)
             {
                 CcbC2Regulator(pWorkItem->id,8,TRUE);
             }
     
             iTmp = (1 << APP_EXE_I4_NO)|(1 << APP_EXE_I5_NO);
             iRet = CcbUpdateIAndBs(pWorkItem->id,0,iTmp,iTmp);
             if (iRet )
             {
                 LOG(VOS_LOG_WARNING,"CcbUpdateIAndBs Fail %d",iRet);    
                 /* notify ui (late implemnt) */
                 work_cir_fail(pWorkItem->id);        
                 return ;
             }

             iTmp = (1 << APP_FM_FM1_NO); /* start flow report */

             iRet = CcbUpdateFms(pWorkItem->id,0,iTmp,iTmp);
             if (iRet )
             {
                 LOG(VOS_LOG_WARNING,"CcbUpdateFms Fail %d",iRet);
                 /* notify ui (late implemnt) */
                 work_cir_fail(pWorkItem->id);
                 return ;
             }

             for (iLoop = 0; iLoop < 3; iLoop++) //延时启动N2
             {
                 iRet = CcbWorkDelayEntry(pWorkItem->id,1000,CcbDelayCallBack);
                 if (iRet )
                 {
                     LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);
                     work_cir_fail(pWorkItem->id);
                     return;
                 }
             }
 
             iTmp = (1 << APP_EXE_E5_NO) | (1 << APP_EXE_E6_NO) | (1 << APP_EXE_C2_NO)| (1 << APP_EXE_N2_NO);
             iRet = CcbUpdateSwitch(pWorkItem->id,0,ulCirMask,iTmp);
             
             iTocStage      = APP_PACKET_EXE_TOC_STAGE_NUM;
         }
         bit1AlarmUP = FALSE;
         break;
    case CIR_TYPE_HP:
        {
            if(gGlobalParam.iMachineType != MACHINE_C)
            {
                iRet = CcbGetPmValue(pWorkItem->id,APP_EXE_PM2_NO,1);
                if (iRet )
                {
                    LOG(VOS_LOG_WARNING,"CcbGetPmValue Fail %d",iRet);  
                    work_cir_fail(pWorkItem->id);
                    return ;
                }
                if (haveB2())
                {
                    if (CcbConvert2Pm2SP(ExeBrd.aPMObjs[APP_EXE_PM2_NO].Value.ulV) < CcbGetSp6())
                    {
                        bit1B2Empty = TRUE;
                        work_cir_fail(pWorkItem->id);
                        return;
                    }
                }
                
                iTmp = (1 << APP_EXE_E4_NO) | (1 << APP_EXE_E6_NO) | (1 << APP_EXE_C2_NO);

                iRet = CcbUpdateSwitch(pWorkItem->id,0,ulCirMask ,iTmp);
                if (iRet )
                {
                    LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);    
                    work_cir_fail(pWorkItem->id);
                    return ;
                }

                if ((ulCirMask & (1 << APP_EXE_C2_NO))
                && bit1CirSpeedAdjust)
                {
                    CcbC2Regulator(pWorkItem->id,8,TRUE);
                }
            }
            else
            {
                iTmp = (1 << APP_EXE_E5_NO) | (1 << APP_EXE_E6_NO) | (1 << APP_EXE_C2_NO);
                iRet = CcbUpdateSwitch(pWorkItem->id,0,ulCirMask,iTmp);
                if (iRet )
                {
                     LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);    
                     work_cir_fail(pWorkItem->id);
                     return ;
                }

                if ((ulCirMask & (1 << APP_EXE_C2_NO))
                && bit1CirSpeedAdjust)
                {
                    CcbC2Regulator(pWorkItem->id,8,TRUE);
                }

                for (iLoop = 0; iLoop < 3; iLoop++) //延时启动N2
                {
                    iRet = CcbWorkDelayEntry(pWorkItem->id,1000,CcbDelayCallBack);
                    if (iRet )
                    {
                     LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);
                     work_cir_fail(pWorkItem->id);
                     return;
                    }
                }
                iTmp = (1 << APP_EXE_E5_NO) | (1 << APP_EXE_E6_NO) | (1 << APP_EXE_C2_NO)| (1 << APP_EXE_N2_NO);
                iRet = CcbUpdateSwitch(pWorkItem->id,0,ulCirMask,iTmp);
            }
            
            iTmp = (1 << APP_EXE_I4_NO);

            switch (gGlobalParam.iMachineType)
            {
            case MACHINE_UP:
            case MACHINE_RO:
            case MACHINE_RO_H:
            case MACHINE_C:
                iTmp |= (1 << APP_EXE_I3_NO); // && haveHPCir())
                break;
            }

            iRet = CcbUpdateIAndBs(pWorkItem->id,0,iTmp,iTmp);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbUpdateIAndBs Fail %d",iRet);    
                work_cir_fail(pWorkItem->id);        
                return ;
            } 

            iTmp = (1 << APP_FM_FM1_NO); 
            iRet = CcbUpdateFms(pWorkItem->id,0,iTmp,iTmp);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbUpdateFms Fail %d",iRet);    
                work_cir_fail(pWorkItem->id);        
                return ;
            } 
        }
        break;
    }

    work_cir_succ();
}

void CCB::work_start_cir(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;

    CCB *pCcb = (CCB *)pWorkItem->para;

    pCcb->work_start_cir_helper(pWorkItem);

}

DISPHANDLE CCB::CcbInnerWorkStartCirToc(int iStage)
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    LOG(VOS_LOG_WARNING,"%s : %d",__FUNCTION__,iStage);    

    pWorkItem->proc  = work_start_toc_cir;
    pWorkItem->para  = this;
    pWorkItem->extra  = (void *)iStage;

    CcbAddWork(WORK_LIST_HP,pWorkItem);

    return (DISPHANDLE)pWorkItem;
}


DISPHANDLE CCB::CcbInnerWorkStartCir(int iType)
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    LOG(VOS_LOG_WARNING,"%s : %d",__FUNCTION__,iType);    

    pWorkItem->proc  = work_start_cir;
    pWorkItem->para  = this;
    pWorkItem->extra  = (void *)iType;

    CcbAddWork(WORK_LIST_HP,pWorkItem);

    return (DISPHANDLE)pWorkItem;
}

void CCB::work_stop_cir_succ(int iQtwFlag)
{
    int aiCont[2] = {0, iQtwFlag};
    
    MainSndWorkMsg(WORK_MSG_TYPE_ECIR,(unsigned char *)aiCont,sizeof(aiCont));

}

void CCB::work_stop_cir(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;

    CCB *pCcb = (CCB *)pWorkItem->para;
    int iQtwFlag = (int)pWorkItem->extra;

    int iTmp;
    int iRet;

    pCcb->bit1I44Nw = FALSE;

    pCcb->bit1TocOngoing = FALSE;

    /* set  valves   */
    iTmp = pCcb->ulCirMask;
    iRet = pCcb->CcbUpdateSwitch(pWorkItem->id,0,iTmp,0);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);    
    }

    if ((pCcb->ulCirMask & (1 << APP_EXE_C2_NO))
       && pCcb->bit1CirSpeedAdjust)
    {
        pCcb->CcbC2Regulator(pWorkItem->id,24,FALSE);
    }

    iTmp = (1 << APP_EXE_I4_NO)|(1 << APP_EXE_I5_NO);

    switch (gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_RO:
    case MACHINE_RO_H:
    case MACHINE_C:
        iTmp |= (1 << APP_EXE_I3_NO); // && haveHPCir())
        break;
    }
    //endl

    iRet = pCcb->CcbUpdateIAndBs(pWorkItem->id,0,iTmp,0);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbSetIAndBs Fail %d",iRet);    
        /* notify ui (late implemnt) */
    }     

    iTmp  = (APP_FM_FM1_NO << 1);
    iRet = pCcb->CcbUpdateFms(pWorkItem->id,0,iTmp,0);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbUpdateFms Fail %d",iRet);    
        /* notify ui (late implemnt) */
    }     

    pCcb->work_stop_cir_succ(iQtwFlag);

    pCcb->ulHPMinCirTimes = 0;
}


DISPHANDLE CCB::CcbInnerWorkStopCir_ext(int iQtwFlag)
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    LOG(VOS_LOG_WARNING,"CcbInnerWorkStopCir");    

    pWorkItem->proc = work_stop_cir;
    pWorkItem->para = this;
    pWorkItem->extra = (void *)iQtwFlag;

    CcbAddWork(WORK_LIST_HP,pWorkItem);

    return (DISPHANDLE)pWorkItem;
}

DISPHANDLE CCB::CcbInnerWorkStopCir(void)
{
    return CcbInnerWorkStopCir_ext(0);
}


void CCB::work_speed_regulation_end(int iIndex,int iResult)
{
    int aiCont[2] = {iResult,iIndex};
    
    MainSndWorkMsg(WORK_MSG_TYPE_ESR,(unsigned char *)aiCont,sizeof(aiCont));
}


void CCB::work_start_speed_regulation(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;

    CCB *pCcb = (CCB *)pWorkItem->para;
    int iIndex = (int)pWorkItem->extra;

    int iTmp;
    int iActive;

    int iRet;

    int iSpeed = pCcb->aHandler[iIndex].iSpeed;

    float fv; //2019.1.7

    pCcb->DispGetRPumpSwitchState(APP_EXE_C2_NO - APP_EXE_C1_NO,&iActive);

    //2019.1.7
    if(10 <= iSpeed)
    {
        fv = 24.0;
    }
    else if(iSpeed == 9)
    {
        fv = 18.0;
    }
    else if(iSpeed == 8)
    {
        fv = 15.0;
    }
    else
    {
        fv = iSpeed + 6.0;
    }
    iTmp = pCcb->DispConvertVoltage2RPumpSpeed(fv);
    //end

   // iTmp = DispConvertVoltage2RPumpSpeed(((24.0 - 5.0)*iSpeed)/PUMP_SPEED_NUM + 5);
    
    if (iActive) iTmp = 0XFF00 | iTmp;

    iRet = pCcb->CcbSetRPump(pWorkItem->id ,APP_EXE_C2_NO - APP_EXE_C1_NO,iTmp);

    pCcb->work_speed_regulation_end(iIndex,iRet);
}


DISPHANDLE CCB::CcbInnerWorkStartSpeedRegulation(int iIndex)
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc  = work_start_speed_regulation;
    pWorkItem->para  = this;
    pWorkItem->extra = (void *)iIndex;

    CcbAddWork(WORK_LIST_HP,pWorkItem);

    return (DISPHANDLE)pWorkItem;
}


void CCB::work_normal_run_fail(int iWorkId)
{
    int aiCont[1] = {-1};

    int iTmp;

    /* close all switchs */
    if (CcbGetSwitchObjState(APP_EXE_SWITCHS_MASK))
    {
        CcbUpdateSwitch(iWorkId,0,ulRunMask,0);
    }
    
    /*stop all report */
    if (CcbGetPmObjState((1 << APP_EXE_PM1_NO))
        || CcbGetIObjState((1 << APP_EXE_I1_NO)|(1 << APP_EXE_I2_NO)|(1 << APP_EXE_I3_NO)))
    {
        iTmp = GET_B_MASK(APP_EXE_PM1_NO) |(1 << APP_EXE_I1_NO)|(1 << APP_EXE_I2_NO)|(1 << APP_EXE_I3_NO) ;
    
        CcbUpdateIAndBs(iWorkId,0,iTmp,0);
    }

    iTmp = (1 << APP_FM_FM2_NO)|(1<<APP_FM_FM3_NO)|(1<<APP_FM_FM4_NO);
    if (CcbGetFmObjState(iTmp))
    {
        CcbUpdateFms(iWorkId,0,iTmp,0);
    }
    
    bit1AlarmRej = FALSE;
    bit1AlarmRoPw = FALSE;
    bit1AlarmEDI = FALSE;
    bit3RuningState = NOT_RUNING_STATE_NONE;

    MainSndWorkMsg(WORK_MSG_TYPE_RUN,(unsigned char *)aiCont,sizeof(aiCont));
    
}

void CCB::work_normal_run_succ(int id)
{
    int aiCont[1] = {0};

    (void )id;
    
    bit3RuningState = NOT_RUNING_STATE_NONE;
    
    MainSndWorkMsg(WORK_MSG_TYPE_RUN,(unsigned char *)aiCont,sizeof(aiCont));
}


void CCB::work_normal_run_wrapper(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;

    /* init */
    bit1AlarmRej     = FALSE;
    bit1ProduceWater = FALSE;
    bit1AlarmRoPw    = FALSE;
    bit1AlarmEDI     = FALSE;
    bit1PeriodFlushing = FALSE;
    bit1B1Check4RuningState = FALSE;
    bit1B1UnderPressureDetected = FALSE;
    ulRejCheckCount = 0;

    /* 2018/01/15 add begin*/
    bit1AlarmROPHV    = FALSE;
    bit1AlarmROPLV    = FALSE;
    bit1AlarmRODV    = FALSE;
    ulRopVelocity    = 0;
    ulLstRopFlow     = INVALID_FM_VALUE;
    ulLstRopTick     = 0;
    iRopVCheckLowEventCount  = 0;
    iRopVCheckLowRestoreCount  = 0;
    
    ulRodVelocity    = 0;
    ulLstRodFlow     = INVALID_FM_VALUE;
    ulLstRodTick     = 0;
    iRoDVCheckEventCount  = 0;
    iRoDVCheckRestoreCount  = 0;
    /* 2018/01/15 add end*/

    work_run_comm_proc(pWorkItem, false);

}

void CCB::work_normal_run(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;
    CCB *pCcb = (CCB *)pWorkItem->para;

    pCcb->work_normal_run_wrapper(para);
}

DISPHANDLE CCB::CcbInnerWorkRun()
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc = work_normal_run;
    pWorkItem->para = this;

    CcbAddWork(WORK_LIST_LP,pWorkItem);

    return (DISPHANDLE)pWorkItem;

}

DISPHANDLE CCB::CcbInnerWorkIdle(void)
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc = work_idle;
    pWorkItem->para = this;

    CcbAddWork(WORK_LIST_HP,pWorkItem);

    return (DISPHANDLE)pWorkItem;
}

int CCB::CanCcbAfModbusMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    MODBUS_PACKET_COMM_STRU *pmg = (MODBUS_PACKET_COMM_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    if (ModbusCb)
    {
        (ModbusCb)(this,0,pmg);
        sp_thread_mutex_lock  ( &Ipc.mutex );
        sp_thread_cond_signal ( &Ipc.cond  );// ylf: all thread killed
        sp_thread_mutex_unlock( &Ipc.mutex );
    }
    
    return 0;
}

void CCB::CanCcbAfRfIdMsg(int mask)
{
    if (iRfIdRequest & mask)
    {
        sp_thread_mutex_lock  ( &Ipc4Rfid.mutex );
        sp_thread_cond_signal ( &Ipc4Rfid.cond  );// ylf: all thread killed
        sp_thread_mutex_unlock( &Ipc4Rfid.mutex );
    }
    
}

void CCB::CcbReset()
{
    bit1AlarmRej      = 0;
    bit1AlarmRoPw     = 0;
    bit1AlarmEDI      = 0;
    bit1NeedFillTank  = 0;  
    bit1B2Full        = 0;  /* tank full */
    bit1FillingTank   = 0;  
    bit1N3Active      = 0;  /* ultralight N3 period handle flag */
    bit1ProduceWater  = 0;  /* RUN state & producing water */
    bit1B1Check4RuningState = 0;
    bit1B1UnderPressureDetected = 0;
    bit3RuningState = 0;

    iWorkStateStackDepth = 0;
    bit1LeakKey4Reset    = 0;
    //bit1NeedTubeCir      = 0;
    bit1TubeCirOngoing   = 0;
    bit1PeriodFlushing   = 0;

}

void CCB::CcbCleanup(void)
{
   switch(curWorkState.iMainWorkState)
   {
   case DISP_WORK_STATE_IDLE:
        CcbReset();
        break;
   case DISP_WORK_STATE_LPP:
        bit3RuningState = 0;
        bit1FillingTank = 0;
        bit1AlarmRej      = 0;
        bit1AlarmRoPw     = 0;
        bit1AlarmEDI      = 0;
        break;
   }
}

void CCB::CanCcbExeReset()
{
    ExeBrd.ulEcoValidFlags = 0;
    ExeBrd.ulPmValidFlags = 0;
    if(ExeBrd.ucDinState == 0XFF)
    {
        ExeBrd.ucDinState = 0;
    }
   // ExeBrd.ucDinState = 0;
}

void CCB::CanCcbFMReset()
{
    FlowMeter.ulFmValidFlags = 0;
}

void CCB::CanCcbInnerReset(int iFlags)
{
    int iLoop;
    
    if ((iFlags & (1 << APP_DEV_TYPE_EXE_BOARD))
        || (iFlags & (1 << APP_DEV_TYPE_FLOW_METER)))
    {
        LOG(VOS_LOG_DEBUG,"starting CanCcbInnerReset\r\n");
    
        CcbCancelAllWork();
        
        /* reset to default state */
        CcbReset(); 

        if (ulActiveMask & (1 << APP_DEV_TYPE_EXE_BOARD))
        {
            /* shutdown all switches */
            CcbSetSwitch(WORK_LIST_NUM,0,0);
            
            CcbSetIAndBs(WORK_LIST_NUM,0,0);  

            CanCcbExeReset();
            
        }
    
        if (ulActiveMask & (1 << APP_DEV_TYPE_FLOW_METER))
        {
            /* shutdown all switches */
            CcbSetFms(WORK_LIST_NUM,0,0);

            CanCcbFMReset();
        }
                
        if (DISP_WORK_STATE_IDLE != curWorkState.iMainWorkState)
        {
            work_idle_succ();
        }
        LOG(VOS_LOG_DEBUG,"CanCcbInnerReset completed\r\n");
        
        return ;
    }

    /* handle reset */
    iFlags >>= APP_HAND_SET_BEGIN_ADDRESS;

    for (iLoop = 0; iLoop < MAX_HANDLER_NUM; iLoop++)
    {
        if (iFlags & (1 << iLoop))
        {
            if (aHandler[iLoop].bit1Qtw)
            {
                /* stop taking water */
                CcbInnerWorkStopQtw(iLoop);
            }
        }
    }
    
    
}

int CCB::CanCcbGetHoState()
{
    uint8_t ucDstState ;
    
    switch(curWorkState.iMainWorkState4Pw)
    {
    default :
        ucDstState = APP_PACKET_HS_STATE_IDLE;
        break;
    case DISP_WORK_STATE_IDLE:        
        switch(curWorkState.iSubWorkState4Pw)
        {
        default:
            ucDstState = APP_PACKET_HS_STATE_IDLE;
            break;
        case DISP_WORK_SUB_IDLE_DEPRESSURE:
            ucDstState = APP_PACKET_HS_STATE_DEC;
            break;
        }
        break;
    case DISP_WORK_STATE_RUN:
        switch(curWorkState.iSubWorkState4Pw)
        {
        default:
            ucDstState = APP_PACKET_HS_STATE_RUN;
            break;
        case DISP_WORK_SUB_RUN_QTW:
        case DISP_WORK_SUB_RUN_QTWING:
            ucDstState = APP_PACKET_HS_STATE_QTW;
            break;
        case DISP_WORK_SUB_RUN_CIR:
        case DISP_WORK_SUB_RUN_CIRING:
            ucDstState = APP_PACKET_HS_STATE_CIR;
            break;
        case DISP_WORK_SUB_RUN_DEC:
            ucDstState = APP_PACKET_HS_STATE_DEC;
            break;
        }
        break;
    }

    return ucDstState;

}


int CCB::CanCcbGetMainCtrlState()
{
    int iState = -1;
    switch(curWorkState.iMainWorkState)
    {
    case DISP_WORK_STATE_IDLE:
        switch(curWorkState.iSubWorkState)
        {
        case DISP_WORK_SUB_IDLE:
             iState = NOT_STATE_INIT;
             break;
        case DISP_WORK_SUB_WASH:
             iState = NOT_STATE_WASH;
             break;
        }
        break;
        //2018.12.18
    case DISP_WORK_STATE_PREPARE:
        switch(curWorkState.iSubWorkState)
        {
        case DISP_WORK_SUB_IDLE:
            iState = NOT_STATE_RUN;
            break;
        }
        break;
        //end
    case DISP_WORK_STATE_RUN:
         switch(curWorkState.iSubWorkState)
         {
         case DISP_WORK_SUB_IDLE:
         case DISP_WORK_SUB_RUN_INIT:
             iState = NOT_STATE_RUN;
             break;
         }
         break;
    case DISP_WORK_STATE_LPP:
         iState = NOT_STATE_LPP;
         break;
     //ex
    case DISP_WORK_STATE_KP:
        switch(curWorkState.iSubWorkState)
        {
        case DISP_WORK_SUB_IDLE:
             iState = NOT_STATE_INIT;
             break;
        }
        break;
     //end
    }
    return iState;
}


// 20200730 add: late implement
int CCB::CanCcbGetWaterType(int iDevType)
{
    return m_aWaterType[iDevType];
}

int CCB::CanCcbAfDataOnLineNotiIndMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{

    APP_PACKET_ONLINE_NOTI_IND_STRU *pmg = (APP_PACKET_ONLINE_NOTI_IND_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0];

    int iSrcAdr     = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);
    int iFlags      = 0;
    //int isHandler   = FALSE;
    int iHandlerIdx = 0;

    LOG(VOS_LOG_DEBUG,"addr%d: OnLine Ind 0x%x\r\n",iSrcAdr,pCanItfMsg->ulCanId);    
    
    switch(pmg->hdr.ucDevType)
    {
    case APP_DEV_TYPE_EXE_BOARD:
        if (iSrcAdr != pmg->hdr.ucDevType)
        {
            return -1;
        }
        iFlags = 1 << iSrcAdr;  

        {
            /* update current FM information */
            APP_PACKET_ONLINE_NOTI_IND_EXE_STRU *pFm = (APP_PACKET_ONLINE_NOTI_IND_EXE_STRU *)pmg->acInfo;
            
            for (iHandlerIdx = 0; iHandlerIdx < APP_EXE_MAX_HOLD_REGISTERS; iHandlerIdx++)
            {
                ExeBrd.ausHoldRegister[iHandlerIdx] = pFm->ausHoldRegs[iHandlerIdx];
            }
        }    
        break;
    case APP_DEV_TYPE_FLOW_METER:
        if (iSrcAdr != pmg->hdr.ucDevType)
        {
            return -1;
        }
        iFlags = 1 << iSrcAdr;

        {
            /* update current FM information */
            APP_PACKET_ONLINE_NOTI_IND_FM_STRU *pFm = (APP_PACKET_ONLINE_NOTI_IND_FM_STRU *)pmg->acInfo;
            
            for (iHandlerIdx = 0; iHandlerIdx < APP_FM_FLOW_METER_NUM; iHandlerIdx++)
            {
                if (  (FlowMeter.aHist[iHandlerIdx].curValue.ulV != 
                       FlowMeter.aHist[iHandlerIdx].lstValue.ulV)
                    &&(FlowMeter.aHist[iHandlerIdx].curValue.ulV != INVALID_FM_VALUE
                       && FlowMeter.aHist[iHandlerIdx].lstValue.ulV != INVALID_FM_VALUE)
                    && (FlowMeter.aHist[iHandlerIdx].curValue.ulV > 
                        FlowMeter.aHist[iHandlerIdx].lstValue.ulV ))
                {
                    FlowMeter.aulHistTotal[iHandlerIdx] += FlowMeter.aHist[iHandlerIdx].curValue.ulV - FlowMeter.aHist[iHandlerIdx].lstValue.ulV;
                }
                
                FlowMeter.aHist[iHandlerIdx].curValue.ulV = pFm->aulFm[iHandlerIdx];
                FlowMeter.aHist[iHandlerIdx].lstValue.ulV = INVALID_FM_VALUE;
            }
        }
        break;
    case APP_DEV_TYPE_HAND_SET:
        if (iSrcAdr < APP_HAND_SET_BEGIN_ADDRESS
            || iSrcAdr > APP_HAND_SET_END_ADDRESS )
        {
            return -1;
        }
        /* fill infomation */
        {
            int iIdx = iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS;

            APP_PACKET_ONLINE_NOTI_IND_HANDLE_STRU *pCont = (APP_PACKET_ONLINE_NOTI_IND_HANDLE_STRU *)pmg->acInfo;
            
            // get SN
            memcpy(aHandler[iIdx].sn,pCont->aucSn,APP_SN_LENGTH);
            
            aHandler[iIdx].bit1Active = TRUE;

            aHandler[iIdx].iDevType = pCont->ucDevType;

            aHandler[iIdx].iTrxMap |= (1 << APP_TRX_CAN);

            iHandlerIdx = iIdx;

            if (-1 == m_iInstallHdlIdx) 
            {
               m_iInstallHdlIdx = iIdx;
            }

            LOG(VOS_LOG_DEBUG,"addr%d: DevType 0x%x\r\n",iSrcAdr,pCont->ucDevType);    
        }
        iFlags = 1 << iSrcAdr;
        break;
    case APP_DEV_TYPE_RF_READER:
        if (iSrcAdr < APP_RF_READER_BEGIN_ADDRESS
            || iSrcAdr > APP_RF_READER_END_ADDRESS )
        {
            return -1;
        }
        /* fill infomation */
        {
            int iIdx = iSrcAdr - APP_RF_READER_BEGIN_ADDRESS;
            aRfReader[iIdx].bit1Active = TRUE;

        }
        aulActMask4Trx[APP_TRX_CAN] |= (1 << iSrcAdr);

        iFlags = 1 << iSrcAdr;
        break;
    default:
        LOG(VOS_LOG_DEBUG,"Addr%d: unknow dev type %x\r\n",iSrcAdr,pmg->hdr.ucDevType);
        return -1;
    }
    
    ulRegisterMask |= (1 << iSrcAdr);

    CcbSetActiveMask(iSrcAdr);

    /* Send response message */
    {
        char buf[64];
        APP_PACKET_ONLINE_NOTI_CONF_STRU *pNotiCnf = (APP_PACKET_ONLINE_NOTI_CONF_STRU *)buf;

        unsigned int ulIdentifier ;

        pNotiCnf->hdr.ucLen         = 0;
        pNotiCnf->hdr.ucTransId     = pmg->hdr.ucTransId;
        pNotiCnf->hdr.ucDevType     = APP_DEV_TYPE_MAIN_CTRL;
        pNotiCnf->hdr.ucMsgType     = pmg->hdr.ucMsgType|0X80;

        if (APP_DEV_TYPE_HAND_SET == pmg->hdr.ucDevType)
        {
            APP_PACKET_ONLINE_NOTI_CONF_HANDLER_STRU *pInfo = (APP_PACKET_ONLINE_NOTI_CONF_HANDLER_STRU *)pNotiCnf->aucInfo;
            
            pInfo->usHeartBeatPeriod = HEART_BEAT_PERIOD*1000;
            pInfo->ucLan             = gGlobalParam.MiscParam.iLan;
            pInfo->ucMode            = aHandler[iHandlerIdx].iDevType;
            pInfo->ucState           = CanCcbGetHoState();
            pInfo->ucAlarmState      = getAlarmState();
            pInfo->ucLiquidLevel     = getTankLevel();
            pInfo->ucQtwSpeed        = DispConvertRPumpSpeed2Scale(ExeBrd.ausHoldRegister[APP_EXE_HOLD_REG_RPUMP_OFFSET + APP_EXE_C2_NO - APP_EXE_C1_NO]);
            pInfo->ucHaveTOC         = !!haveTOC();
            pInfo->ucTankCap          = (uint8_t)gGlobalParam.PmParam.afCap[DISP_PM_PM2];
            
            switch(pInfo->ucState)
            {
            case APP_PACKET_HS_STATE_QTW:
                pInfo->ucAddData    = CcbGetTwFlag();
                break;
            case APP_PACKET_HS_STATE_CIR:
                pInfo->ucAddData    = iCirType;
                break;
            default :
                pInfo->ucAddData    = 0;
                break;
            }
            
            pInfo->ucWaterType      = CanCcbGetWaterType(aHandler[iHandlerIdx].iDevType); 

            pInfo->ucMainCtlState   = CanCcbGetMainCtrlState(); 

            pNotiCnf->hdr.ucLen     += sizeof(APP_PACKET_ONLINE_NOTI_CONF_HANDLER_STRU);

            CcbNotSpeed(pInfo->ucMode,pInfo->ucQtwSpeed);
        }
        else
        {
            APP_PACKET_ONLINE_NOTI_CONF_EXE_STRU *pInfo = (APP_PACKET_ONLINE_NOTI_CONF_EXE_STRU *)pNotiCnf->aucInfo;
            
            pInfo->usHeartBeatPeriod = HEART_BEAT_PERIOD*1000;
            pNotiCnf->hdr.ucLen     += sizeof(APP_PACKET_ONLINE_NOTI_CONF_EXE_STRU);
        }

        CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_PAKCET_ADDRESS_MC,iSrcAdr);

        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulIdentifier,SAPP_CMD_DATA,(unsigned char *)buf,APP_PROTOL_HEADER_LEN + pNotiCnf->hdr.ucLen );
    }

    //if (isHandler)
    //{
    //    uint8_t ucDstState = CanCcbGetHoState();
    //    CanCcbSndHOState(iHandlerIdx,ucDstState);        
    //}

    /* leave to reset handler */
    CanCcbInnerReset(iFlags);

    switch(pmg->hdr.ucDevType)
    {
    case APP_DEV_TYPE_EXE_BOARD:
        {
            CcbExeBrdNotify();
        }
        break;
    case APP_DEV_TYPE_FLOW_METER:
        {
            CcbFmBrdNotify();
        }
        break;
    case APP_DEV_TYPE_HAND_SET:
        {
            CcbHandlerNotify(iHandlerIdx);
        }
        break;
    }

    return 0;
}


int CCB::CanCcbAfDataHeartBeatRspMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    int              iHandlerIdx     = 0;
    unsigned int     ulOldActiveMask = ulActiveShadowMask;

    APP_PACKET_ONLINE_NOTI_CONF_STRU *pmg = (APP_PACKET_ONLINE_NOTI_CONF_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    int iSrcAdr = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    int iDelta = 0;

    //printf("Hbt response 0x%x\r\n",pCanItfMsg->ulCanId);
    
    switch(pmg->hdr.ucDevType)
    {
    case APP_DEV_TYPE_EXE_BOARD:
    case APP_DEV_TYPE_FLOW_METER:
        if (iSrcAdr != pmg->hdr.ucDevType)
        {
            return -1;
        }
        break;
    case APP_DEV_TYPE_HAND_SET:
        if (iSrcAdr < APP_HAND_SET_BEGIN_ADDRESS)
        {
            return -1;
        }
        iHandlerIdx = iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS;
        break;
    case APP_DEV_TYPE_RF_READER:
        if (iSrcAdr < APP_RF_READER_BEGIN_ADDRESS)
        {
            return -1;
        }
        
        iHandlerIdx = iSrcAdr - APP_RF_READER_BEGIN_ADDRESS;
        break;
    default:
        LOG(VOS_LOG_DEBUG,"Addr%d: unknow dev type %x\r\n",iSrcAdr,pmg->hdr.ucDevType);
        return -1;
    }
    
    aulActMask4Trx[APP_TRX_ZIGBEE] |= (1 << iSrcAdr);
    CcbSetActiveMask(iSrcAdr);

    iDelta = ulOldActiveMask ^ ulActiveMask;

    if (iDelta && (iDelta & (1 << iSrcAdr)))
    {
        switch(pmg->hdr.ucDevType)
        {
        case APP_DEV_TYPE_EXE_BOARD:
            CcbExeBrdNotify();
            break;
        case APP_DEV_TYPE_FLOW_METER:
            CcbFmBrdNotify();
            break;
        case APP_DEV_TYPE_HAND_SET:
            CcbHandlerNotify(iHandlerIdx);
            break;
        }
    }

    return 0;
}

/**
 * 在运行产水状态实时检查工作压力
 * @param iPmId [description]
 */
void CCB::checkB1DuringRunState()
{
    if (bit1B1Check4RuningState)
    {
        float fValue = CcbConvert2Pm1Data(ExeBrd.aPMObjs[APP_EXE_PM1_NO].Value.ulV);
        float fThd   = 2; /* for desktop machine(unit Bar) */

        {
            if (fValue >= fThd)
            {
                bit1B1UnderPressureDetected = 0;
                if (bit1AlarmWorkPressLow)
                {
                    bit1AlarmWorkPressLow = FALSE;
                    CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_WORK_PRESSURE,FALSE);
                }
            }
            else
            {
                if (!bit1B1UnderPressureDetected)
                {
                    bit1B1UnderPressureDetected = TRUE;
                    ulB1UnderPressureTick = gulSecond;
                }
            }
        }

        //high work pressure
        if(fValue >= CcbGetSp33())
        {
            if(!bit1AlarmWorkPressHigh)
            {
                bit1AlarmWorkPressHigh = TRUE;
                CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_HIGH_WORK_PRESSURE,TRUE);
            }
        }
        else
        {
            if(bit1AlarmWorkPressHigh)
            {
                bit1AlarmWorkPressHigh = FALSE;
                CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_HIGH_WORK_PRESSURE,FALSE);
            }
        }
        //end     
    }
}

/**
 * 检查纯水箱是否为空以确定是否停止取水
 * @param iPmId [压力传感器id]
 */
void CCB::checkB2ToProcessB2Empty()
{
    if (haveB2())
    {
        if (CcbConvert2Pm2SP(ExeBrd.aPMObjs[APP_EXE_PM2_NO].Value.ulV) < CcbGetSp6())
        {
            bit1B2Empty = TRUE;
            /* stop taking water */
            CcbStopQtw();
        }
        else
        {
            bit1B2Empty = FALSE;
        } 
    }
}

/**
 * 检查原水箱是否需要启动补水
 * @param iPmId [压力传感器id]
 */
void CCB::checkB2ToProcessB2NeedFill()
{
    float fValue = CcbConvert2Pm2SP(ExeBrd.aPMObjs[APP_EXE_PM2_NO].Value.ulV);

    if (ExeBrd.ucLeakState || (DISP_WORK_STATE_KP == curWorkState.iMainWorkState))
    {
        return;
    }

    if((getKeyState() & (1 << APP_EXE_DIN_LEAK_KEY)) ||  getLeakState())
    {
        return;
    }

    if(fValue < CcbGetSp5())
    {
        bit1NeedFillTank = 1;
        if (!SearchWork(work_start_fill_water))
        {
            CcbInnerWorkStartFillWater();
        }                
    } 
}


/**
 * 检查纯水箱是否已满，满则停止产水
 * @param iPmId [压力传感器id]
 */
void CCB::checkB2ToProcessB2Full()
{
    if (haveB2())
    {
        float fValue = CcbConvert2Pm2SP(ExeBrd.aPMObjs[APP_EXE_PM2_NO].Value.ulV);
        if (fValue >= B2_FULL)
        {
            /* check if EXE already stopped producing water */
            CanPrepare4Pm2Full();
            
            if (bit1ProduceWater )
            {
               /* stop producing water */
               if (!SearchWork(work_stop_pw))
               {
                   CcbInnerWorkStopProduceWater();
               }
            }
        }
        
        if((MACHINE_PURIST == gGlobalParam.iMachineType) || (MACHINE_C_D == gGlobalParam.iMachineType))
        {
            if (fValue >= B2_FULL)
            {
                if (!SearchWork(work_stop_fill_water))
                {
                    CcbInnerWorkStopFillWater();
                }
            }
            else
            {
                checkB2ToProcessB2NeedFill();
            }
        }
    } 
}

/**
 * 检查原水箱是否已满，满则停止原水箱注水
 * @param iPmId [压力传感器id]
 */
void CCB::checkB3ToProcessB3Full()
{
    float fValue = CcbConvert2Pm3SP(ExeBrd.aPMObjs[APP_EXE_PM3_NO].Value.ulV);
    if((fValue >= B3_FULL) || (getKeyState() & (1 << APP_EXE_DIN_LEAK_KEY)) || getLeakState())
    {
        if (!SearchWork(work_stop_fill_water))
        {
            CcbInnerWorkStopFillWater();
        }                
    }  
}

/**
 * 检查原水箱是否需要启动补水
 * @param iPmId [压力传感器id]
 */
void CCB::checkB3ToProcessB3NeedFill()
{
    float fValue = CcbConvert2Pm3SP(ExeBrd.aPMObjs[APP_EXE_PM3_NO].Value.ulV);

    if((getKeyState() & (1 << APP_EXE_DIN_LEAK_KEY)) ||  getLeakState())
    {
        return;
    }
    
    if(fValue < CcbGetSp8())
    {
        bit1NeedFillTank = 1;
        if (!SearchWork(work_start_fill_water))
        {
            CcbInnerWorkStartFillWater();
        }                
    } 
}

/**
 * 检查原水箱是否为空，空则停止产水
 * @param iPmId [压力传感器id]
 */
void CCB::checkB3ToProcessB3Empty()
{
    float fValue = CcbConvert2Pm3SP(ExeBrd.aPMObjs[APP_EXE_PM3_NO].Value.ulV);
    if(fValue < CcbGetSp9())
    {
        bit1NeedFillTank = 1;
        if (bit1ProduceWater)
        {
            /* stop producing water */
            if (!SearchWork(work_stop_pw))
            {
                CcbInnerWorkStopProduceWater();
            }
        }
    }
}

/**
 * 检查处理B3(原水箱)压力传感器
 */
void CCB::processB3Measure()
{
    if(haveB3())
    {
        if (bit1FillingTank)
        {
            checkB3ToProcessB3Full();
        }
        else
        {
            checkB3ToProcessB3NeedFill();
        }  
        checkB3ToProcessB3Empty();
    }
}

/**
 * 分析执行板上报的压力数据，有压力数据上报时调用
 * @param iPmId [压力传感器编号]
 */
void CCB::CanCcbPmMeasurePostProcess(int iPmId)
{
    switch(curWorkState.iMainWorkState4Pw)
    {
    case DISP_WORK_STATE_RUN:
        {
            switch(curWorkState.iSubWorkState4Pw)
            {
            case DISP_WORK_SUB_RUN_QTW:
                {
                    if(APP_EXE_PM2_NO == iPmId)
                    {
                        checkB2ToProcessB2Empty();
                    }
                }
                break;
            default:
                break;
            }
            
            if(APP_EXE_PM2_NO == iPmId)
            {
                checkB2ToProcessB2Full();
            }

            if(APP_EXE_PM3_NO == iPmId)
            {
                processB3Measure();
            }
        }
        break;
    case DISP_WORK_STATE_LPP:
        break;
    default:
        break;
    }

    switch(curWorkState.iMainWorkState)
    {
    case DISP_WORK_STATE_RUN:
        {
            switch(iPmId)
            {
            case APP_EXE_PM1_NO:
                checkB1DuringRunState();
                break;
            case APP_EXE_PM2_NO:
                checkB2ToProcessB2Full();
                break;
            case APP_EXE_PM3_NO:
                processB3Measure();
                break;
            default:
                break;
            }
        }
        break;
    case DISP_WORK_STATE_LPP:
        break;
    default:
        break;
    }

}

void CCB::CcbNotAlarmFire(int iPart,int iId,int bFired)
{

    int iLength;
    unsigned char buffer[128];
    
    NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_ALARM_ITEM_STRU *pItem = (NOT_ALARM_ITEM_STRU *)pNotify->aucData;

    printf("alarm %d:%d \r\n",iId,bFired);

    pNotify->Hdr.ucCode = DISP_NOT_ALARM;
    iLength             = sizeof(NOT_HEADER_STRU);

    pItem->ucFlag = bFired;
    pItem->ucPart = iPart;
    pItem->ucId   = iId;
    iLength      += sizeof(NOT_ALARM_ITEM_STRU);

    pItem++;
    pItem->ucId = 0XFF;
    iLength    += sizeof(NOT_ALARM_ITEM_STRU);

    emitNotify(buffer,iLength);

}

//2018.12.21 add
void CCB::Check_RO_EDI_Alarm(int iEcoId)
{
    float fRej;

    if(!DispGetPwFlag())
    {
        return;
    }

    switch(iEcoId)
    {
    case 1:
        if((gGlobalParam.iMachineType != MACHINE_PURIST) && (gGlobalParam.iMachineType != MACHINE_C_D))
        {
            fRej = CcbCalcREJ();
            if (fRej < CcbGetSp2())
            {
                if(!bit1AlarmRej)
                {
                    bit1AlarmRej = TRUE;
                    ulAlarmRejTick = gulSecond;
                }
                else
                {
                    if (!(ulFiredAlarmFlags  & ALARM_REJ))
                    {
                        if ((gulSecond - ulAlarmRejTick) >= 60*5) //60*5
                        {
                            ulFiredAlarmFlags |= ALARM_REJ;

                            CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_RO_RESIDUE_RATIO, TRUE);
                        }
                    }
                }

            }
            else
            {
                if (ulFiredAlarmFlags  & ALARM_REJ)
                {
                    ulFiredAlarmFlags &=  ~ALARM_REJ;
                    CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_RO_RESIDUE_RATIO, FALSE);
                }

                if (bit1AlarmRej) bit1AlarmRej = FALSE;
            }
        }
        //RO
        if (ExeBrd.aEcoObjs[APP_EXE_I2_NO].Value.eV.fWaterQ > CcbGetSp3())
        {
            if(!bit1AlarmRoPw)
            {
                bit1AlarmRoPw   = TRUE;
                ulAlarmRoPwTick = gulSecond;
            }
            else
            {
                if (!(ulFiredAlarmFlags  & ALARM_ROPW))
                {
                    if ((gulSecond - ulAlarmRoPwTick) >= 60*5)
                    {
                        ulFiredAlarmFlags |= ALARM_ROPW;

                        CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_HIGER_RO_PRODUCT_CONDUCTIVITY,TRUE);
                    }
                }
            }
        }
        else
        {
            if (ulFiredAlarmFlags  & ALARM_ROPW)
            {
                ulFiredAlarmFlags &=  ~ALARM_ROPW;

                CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_HIGER_RO_PRODUCT_CONDUCTIVITY, FALSE);
            }

            if (bit1AlarmRoPw) bit1AlarmRoPw = FALSE;
        }
        break;
    case 2:
        if (ExeBrd.aEcoObjs[APP_EXE_I3_NO].Value.eV.fWaterQ < CcbGetSp4())
        {
            if(!bit1AlarmEDI)
            {
                bit1AlarmEDI   = TRUE;
                ulAlarmEDITick = gulSecond;
            }
            else
            {
                if (!(ulFiredAlarmFlags  & ALARM_EDI))
                {
                    if ((gulSecond - ulAlarmEDITick) >= 60*5)
                    {
                        ulFiredAlarmFlags |= ALARM_EDI;
                        CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_EDI_PRODUCT_RESISTENCE,TRUE);
                    }
                }
            }
        }
        else
        {
            if (ulFiredAlarmFlags  & ALARM_EDI)
            {
                ulFiredAlarmFlags &=  ~ALARM_EDI;

                CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_EDI_PRODUCT_RESISTENCE,FALSE);
            }

            if (bit1AlarmEDI) bit1AlarmEDI = FALSE;
        }
        break;
    default:
        break;
    }
}

void CCB::CanCcbEcoMeasurePostProcess(int iEcoId)
{
    switch(curWorkState.iMainWorkState4Pw)
    {
    case DISP_WORK_STATE_RUN:
        {
            switch(curWorkState.iSubWorkState4Pw)
            {
            case DISP_WORK_SUB_RUN_CIR:
                {
                    switch(iCirType)
                    {
                    case CIR_TYPE_UP:
                    {
                        if (iEcoId == APP_EXE_I5_NO)
                        {
                            float fValue = ExeBrd.aEcoObjs[APP_EXE_I5_NO].Value.eV.fWaterQ;
                            if (fValue < CcbGetSp7())
                            {
                                if (!bit1I54Cir )
                                {
                                    bit1I54Cir = TRUE;
                                }
                            }  
                            else
                            {
                                if (bit1I54Cir) 
                                {
                                    bit1I54Cir = FALSE;
                                }
                            }

                            //2018.12.7 add
                            if (fValue < CcbGetSp7())
                            {
                                if (!bit1AlarmUP)
                                {
                                    bit1AlarmUP = TRUE;
                                    ulAlarmUPTick = gulSecond;
                                }
                                else
                                {
                                    /* fire alarm */

                                    if (!(ulFiredAlarmFlags  & ALARM_UP))
                                    {
                                        if (gulSecond - ulAlarmUPTick >= 30)
                                        {
                                            ulFiredAlarmFlags |= ALARM_UP;

                                            CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_UP_PRODUCT_RESISTENCE,TRUE);
                                        }
                                    }
                                }

                            }
                            else
                            {
                                if (ulFiredAlarmFlags & ALARM_UP)
                                {
                                    ulFiredAlarmFlags &=  ~ALARM_UP;

                                    CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_UP_PRODUCT_RESISTENCE,FALSE);
                                }

                                if (bit1AlarmUP) bit1AlarmUP = FALSE;
                            }
                            //
                        }  
                        else if (iEcoId == APP_EXE_I4_NO)
                        {
                            if (bit1TocOngoing  && bit1TocAlarmNeedCheck)
                            {
                                float fValue = ExeBrd.aEcoObjs[APP_EXE_I4_NO].Value.eV.fWaterQ;
                                if(fValue < CcbGetSp30())
                                {
                                    if(!(ulFiredAlarmFlags & ALARM_TOC))
                                    {
                                        bit1AlarmTOC   = TRUE;

                                        ulFiredAlarmFlags |= ALARM_TOC;

                                        CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_TOC_SOURCE_WATER_RESISTENCE,TRUE);
                                    }
                                }

                                else
                                {
                                    if ( ulFiredAlarmFlags & ALARM_TOC)
                                    {
                                        ulFiredAlarmFlags &= ~ALARM_TOC;

                                        CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_TOC_SOURCE_WATER_RESISTENCE,FALSE);
                                    }
                                    if(bit1AlarmTOC) bit1AlarmTOC = FALSE;

                                }
                            }
                            
                        }
                        break;
                    }
                    case CIR_TYPE_HP:
                        {
                            if(gGlobalParam.iMachineType == MACHINE_RO_H)
                            {
                                if(iEcoId == APP_EXE_I3_NO)
                                {
                                    float fValue = ExeBrd.aEcoObjs[APP_EXE_I3_NO].Value.eV.fWaterQ;
                                    if (fValue >= CcbGetSp11())
                                    {
                                        ulHPMinCirTimes++;
                                        if(ulHPMinCirTimes > 60)
                                        {
                                            CcbInnerWorkStopCir();
                                        }
                                    }
                                    else
                                    {
                                        ulHPMinCirTimes = 0;
                                    }
                                }
                            }
                            else if(gGlobalParam.iMachineType == MACHINE_C)
                            {
                                //do nothing
                            }
                            else
                            {
                                if (iEcoId == APP_EXE_I4_NO)
                                {
                                    float fValue = ExeBrd.aEcoObjs[APP_EXE_I4_NO].Value.eV.fWaterQ;
                                    if (fValue >= CcbGetSp11())
                                    {
                                        ulHPMinCirTimes++;
                                        if(ulHPMinCirTimes > 60)
                                        {
                                            CcbInnerWorkStopCir();
                                        }
                                    }
                                    else
                                    {
                                        ulHPMinCirTimes = 0;
                                    }
                                }
                            }

                        }
                        break;
                    }
                }
                break;
            case DISP_WORK_SUB_RUN_QTW:
                switch(iEcoId)
                {
                case APP_EXE_I3_NO:
                    if((gGlobalParam.iMachineType == MACHINE_UP) 
                        || (gGlobalParam.iMachineType == MACHINE_RO) 
                        || (gGlobalParam.iMachineType == MACHINE_RO_H)
                        || (gGlobalParam.iMachineType == MACHINE_C))
                    {
                        float fValue = ExeBrd.aEcoObjs[APP_EXE_I3_NO].Value.eV.fWaterQ;
                        bit1I44Nw = fValue >= CcbGetSp10() ? TRUE : FALSE ;
                    }
                    break;
                case APP_EXE_I4_NO:
                    {
                        float fValue = ExeBrd.aEcoObjs[APP_EXE_I4_NO].Value.eV.fWaterQ;

                        /* 2018/05/04 Modify from CcbGetSp11->CcbGetSp10*/
                        bit1I44Nw = fValue >= CcbGetSp10() ? TRUE : FALSE ; 
                    }
                    break;
                case APP_EXE_I5_NO:
                    {
                        if (iCurTwIdx < MAX_HANDLER_NUM)
                        {
                            if (APP_DEV_HS_SUB_UP == aHandler[iCurTwIdx].iDevType)
                            {
                                float fValue = ExeBrd.aEcoObjs[APP_EXE_I5_NO].Value.eV.fWaterQ;
                                if (fValue < CcbGetSp7())
                                {
                                    if (!bit1AlarmUP)
                                    {
                                        bit1AlarmUP = TRUE;
                                        ulAlarmUPTick = gulSecond;
                                    }
                                    else
                                    {
                                        /* fire alarm */
                                        
                                        if (!(ulFiredAlarmFlags  & ALARM_UP))
                                        {
                                            if (gulSecond - ulAlarmUPTick >= 30)
                                            {
                                                ulFiredAlarmFlags |= ALARM_UP;
                                            
                                                CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_UP_PRODUCT_RESISTENCE,TRUE);
                                            }
                                        }
                                    }
                                    
                                }
                                else 
                                {
                                    if (ulFiredAlarmFlags & ALARM_UP)
                                    {
                                        ulFiredAlarmFlags &=  ~ALARM_UP;

                                        CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_UP_PRODUCT_RESISTENCE,FALSE);
                                    }

                                    if (bit1AlarmUP) bit1AlarmUP = FALSE;
                                }
                            }
                        }
                    }
                    break;
                }               
            default:
                break;
            }
        }
        break;
    case DISP_WORK_STATE_LPP:
        break;
    default:
        break;
    }
    Check_RO_EDI_Alarm(iEcoId);
}

void CCB::CanCcbRectNMeasurePostProcess(int rectID)
{
    int iTmpData;
    UNUSED(rectID);
    //N1
    if(gEx_Ccb.Ex_Rect_State.EX_N_NO[EX_RECT_N1] == 1)
    {
        DispGetOtherCurrent(APP_EXE_N1_NO, &iTmpData);
        if((ex_gulSecond - gEx_Ccb.Ex_Alarm_Tick.ulAlarmNRectTick[EX_RECT_N1]) > 15)
        {
            if(iTmpData < 100)
            {
                //Alaram
                if(!gEx_Ccb.Ex_Alarm_Bit.bit1AlarmN1)
                {
                    //2018.10.23
                    if(!gEx_Ccb.Ex_Alarm_Bit.bit1AlarmDelayN1)
                    {
                        gEx_Ccb.Ex_Alarm_Tick.ulAlarmNRectDelay[EX_RECT_N1] = ex_gulSecond;
                        gEx_Ccb.Ex_Alarm_Bit.bit1AlarmDelayN1 = 1;
                    }
                    else
                    {
                        if((ex_gulSecond - gEx_Ccb.Ex_Alarm_Tick.ulAlarmNRectDelay[EX_RECT_N1]) > 5)
                        {
                            gEx_Ccb.Ex_Alarm_Bit.bit1AlarmN1 = 1;
                            ulFiredAlarmFlags |= ALARM_N1;
                            CcbNotAlarmFire(DISP_ALARM_PART0,DISP_ALARM_PART0_254UV_OOP,TRUE);
                        }
                    }
                    //
                }
            }
            else
            {
                if(!!gEx_Ccb.Ex_Alarm_Bit.bit1AlarmN1)
                {
                    gEx_Ccb.Ex_Alarm_Bit.bit1AlarmDelayN1 = 0;
                    gEx_Ccb.Ex_Alarm_Bit.bit1AlarmN1 = 0;
                    ulFiredAlarmFlags &= ~ALARM_N1;
                    CcbNotAlarmFire(DISP_ALARM_PART0,DISP_ALARM_PART0_254UV_OOP, FALSE);
                }
            }
        }
    }
    //N2
    if(gEx_Ccb.Ex_Rect_State.EX_N_NO[EX_RECT_N2] == 1)
    {
        DispGetOtherCurrent(APP_EXE_N2_NO, &iTmpData);
        if((ex_gulSecond - gEx_Ccb.Ex_Alarm_Tick.ulAlarmNRectTick[EX_RECT_N2]) > 15)
        {
            if(iTmpData < 100)
            {
                //Alaram
                if(!gEx_Ccb.Ex_Alarm_Bit.bit1AlarmN2)
                {
                    //2018.10.23
                    if(!gEx_Ccb.Ex_Alarm_Bit.bit1AlarmDelayN2)
                    {
                        gEx_Ccb.Ex_Alarm_Tick.ulAlarmNRectDelay[EX_RECT_N2] = ex_gulSecond;
                        gEx_Ccb.Ex_Alarm_Bit.bit1AlarmDelayN2 = 1;
                    }
                    else
                    {
                        if((ex_gulSecond - gEx_Ccb.Ex_Alarm_Tick.ulAlarmNRectDelay[EX_RECT_N2]) > 5)
                        {
                            gEx_Ccb.Ex_Alarm_Bit.bit1AlarmN2 = 1;
                            ulFiredAlarmFlags |= ALARM_N2;
                            CcbNotAlarmFire(DISP_ALARM_PART0,DISP_ALARM_PART0_185UV_OOP,TRUE);
                        }
                    }
                    //
                }
            }
            else
            {
                if(!!gEx_Ccb.Ex_Alarm_Bit.bit1AlarmN2)
                {
                    gEx_Ccb.Ex_Alarm_Bit.bit1AlarmDelayN2 = 0;
                    gEx_Ccb.Ex_Alarm_Bit.bit1AlarmN2 = 0;
                    ulFiredAlarmFlags &= ~ALARM_N2;
                    CcbNotAlarmFire(DISP_ALARM_PART0,DISP_ALARM_PART0_185UV_OOP, FALSE);
                }
            }
        }
    }

    //N3
    if(gEx_Ccb.Ex_Rect_State.EX_N_NO[EX_RECT_N3] == 1)
    {
        DispGetOtherCurrent(APP_EXE_N3_NO, &iTmpData);
        if((ex_gulSecond - gEx_Ccb.Ex_Alarm_Tick.ulAlarmNRectTick[EX_RECT_N3]) > 15)
        {
            if(iTmpData < 100)
            {
                //Alaram
                if(!gEx_Ccb.Ex_Alarm_Bit.bit1AlarmN3)
                {
                    //2018.10.23
                    if(!gEx_Ccb.Ex_Alarm_Bit.bit1AlarmDelayN3)
                    {
                        gEx_Ccb.Ex_Alarm_Tick.ulAlarmNRectDelay[EX_RECT_N3] = ex_gulSecond;
                        gEx_Ccb.Ex_Alarm_Bit.bit1AlarmDelayN3 = 1;
                    }
                    else
                    {
                        if((ex_gulSecond - gEx_Ccb.Ex_Alarm_Tick.ulAlarmNRectDelay[EX_RECT_N3]) > 5)
                        {
                            gEx_Ccb.Ex_Alarm_Bit.bit1AlarmN3 = 1;
                            ulFiredAlarmFlags |= ALARM_N3;
                            CcbNotAlarmFire(DISP_ALARM_PART0,DISP_ALARM_PART0_TANKUV_OOP,TRUE);
                        }
                    }
                    //
                }

            }
            else
            {
                if(!!gEx_Ccb.Ex_Alarm_Bit.bit1AlarmN3)
                {
                    gEx_Ccb.Ex_Alarm_Bit.bit1AlarmDelayN3 = 0;
                    gEx_Ccb.Ex_Alarm_Bit.bit1AlarmN3 = 0;
                    ulFiredAlarmFlags &= ~ALARM_N3;
                    CcbNotAlarmFire(DISP_ALARM_PART0,DISP_ALARM_PART0_TANKUV_OOP, FALSE);
                }
            }
        }
    }
}


void CCB::CcbEcoNotify(void)
{
    int iLoop ;

    int iLength;

    int iMask;

    unsigned char buffer[128];
    
    NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_ECO_ITEM_STRU *pItem = (NOT_ECO_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_ECO;

    iLength = sizeof(NOT_HEADER_STRU);

    iMask = CcbGetIObjState(APP_EXE_I_MASK);

    for (iLoop = 0; iLoop < APP_EXE_ECO_NUM; iLoop++ )
    {
        if (iMask & (1 << iLoop))
        {
            pItem->ucId           = (unsigned char )iLoop;
            pItem->fValue         = ExeBrd.aEcoObjs[iLoop].Value.eV.fWaterQ;
            pItem->usValue        = ExeBrd.aEcoObjs[iLoop].Value.eV.usTemp;
            pItem++;   

            iLength += sizeof(NOT_ECO_ITEM_STRU);
        }
    }


    pItem->ucId = 0XFF;

    iLength += sizeof(NOT_ECO_ITEM_STRU);

    emitNotify(buffer,iLength);

    
//    LOG(VOS_LOG_DEBUG,"wq %x",iMask);                       
}

void CCB::CcbPmNotify()
{
    int iLoop ;

    int iLength;

    int iMask;
    unsigned char buffer[128];
    
    NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_PM_ITEM_STRU *pItem = (NOT_PM_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_PM;

    iLength = sizeof(NOT_HEADER_STRU);

    iMask = CcbGetPmObjState(APP_EXE_PM_MASK);

    for (iLoop = 0; iLoop < APP_EXE_PRESSURE_METER; iLoop++ )
    {

        if (iMask & (1 << iLoop))
        {
            pItem->ucId          = (unsigned char )iLoop;
            pItem->ulValue       = ExeBrd.aPMObjs[iLoop].Value.ulV;
            pItem++;   

            iLength += sizeof(NOT_PM_ITEM_STRU);
        }
    }

    pItem->ucId = 0XFF;

    iLength += sizeof(NOT_PM_ITEM_STRU);

    emitNotify(buffer,iLength);

    
}


void CCB::CcbRectNotify()
{
    int iLoop ;

    int iLength;

    int iMask;
    unsigned char buffer[128];
    
    NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_PM_ITEM_STRU *pItem = (NOT_PM_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_RECT;

    iLength = sizeof(NOT_HEADER_STRU);


    iMask = CcbGetSwitchObjState((APP_EXE_RECT_MASK << APP_EXE_RECT_REPORT_POS));

    for (iLoop = 0; iLoop < APP_EXE_RECT_NUM; iLoop++ )
    {

        if ((iMask >> APP_EXE_RECT_REPORT_POS) & (1 << iLoop))
        {
            pItem->ucId          = (unsigned char )iLoop;
            pItem->ulValue       = ExeBrd.aRectObjs[iLoop].Value.ulV;
            pItem++;   

            iLength += sizeof(NOT_RECT_ITEM_STRU);
        }
    }

    pItem->ucId = 0XFF;

    iLength += sizeof(NOT_RECT_ITEM_STRU);

    emitNotify(buffer,iLength);

    
}

void CCB::CcbGPumpNotify()
{
    int iLoop ;

    int iLength;

    int iMask;
    unsigned char buffer[128];
    
    NOT_INFO_STRU       *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_GPUMP_ITEM_STRU *pItem   = (NOT_GPUMP_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_GPUMP;

    iLength = sizeof(NOT_HEADER_STRU);


    iMask = CcbGetSwitchObjState((APP_EXE_GCMASK << APP_EXE_GPUMP_REPORT_POS));

    for (iLoop = 0; iLoop < APP_EXE_G_PUMP_NUM; iLoop++ )
    {

        if ((iMask >> APP_EXE_VALVE_NUM) & (1 << iLoop))
        {
            pItem->ucId          = (unsigned char )iLoop;
            pItem->ulValue       = ExeBrd.aGPumpObjs[iLoop].Value.ulV;
            pItem++;   

            iLength += sizeof(NOT_GPUMP_ITEM_STRU);
        }
    }

    pItem->ucId = 0XFF;

    iLength += sizeof(NOT_GPUMP_ITEM_STRU);

    emitNotify(buffer,iLength);

    
}

void CCB::CcbRPumpNotify()
{
    int iLoop ;

    int iLength;

    int iMask;

    unsigned char buffer[128];
    
    NOT_INFO_STRU       *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_RPUMP_ITEM_STRU *pItem   = (NOT_RPUMP_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_RPUMP;

    iLength = sizeof(NOT_HEADER_STRU);


    iMask = CcbGetSwitchObjState((APP_EXE_RCMASK << APP_EXE_RPUMP_REPORT_POS));

    for (iLoop = 0; iLoop < APP_EXE_R_PUMP_NUM; iLoop++ )
    {

        if ((iMask >> APP_EXE_RPUMP_REPORT_POS) & (1 << iLoop))
        {
            pItem->ucId          = (unsigned char )iLoop;
            pItem->ulValue       = ExeBrd.aRPumpObjs[iLoop].Value.ulV;
            pItem++;   

            iLength += sizeof(NOT_RPUMP_ITEM_STRU);
        }
    }

    pItem->ucId = 0XFF;

    iLength += sizeof(NOT_RPUMP_ITEM_STRU);

    emitNotify(buffer,iLength);

    
}


void CCB::CcbEDINotify()
{
    int iLoop ;

    int iLength;

    int iMask;
    unsigned char buffer[128];
    
    NOT_INFO_STRU   *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_EDI_ITEM_STRU *pItem = (NOT_EDI_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_EDI;

    iLength = sizeof(NOT_HEADER_STRU);


    iMask = CcbGetSwitchObjState((APP_EXE_TMASK << APP_EXE_EDI_REPORT_POS));

    for (iLoop = 0; iLoop < APP_EXE_EDI_NUM; iLoop++ )
    {

        if ((iMask >> APP_EXE_EDI_REPORT_POS) & (1 << iLoop))
        {
            pItem->ucId          = (unsigned char )iLoop;
            pItem->ulValue       = ExeBrd.aEDIObjs[iLoop].Value.ulV;
            pItem++;   

            iLength += sizeof(NOT_EDI_ITEM_STRU);
        }
    }

    pItem->ucId = 0XFF;

    iLength += sizeof(NOT_EDI_ITEM_STRU);

    emitNotify(buffer,iLength);
    
}


void CCB::CcbFmNotify()
{
    int iLoop ;

    int iLength;

    int iMask;
    unsigned char buffer[128];
    
    NOT_INFO_STRU  *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_FM_ITEM_STRU *pItem = (NOT_FM_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_FM;

    iLength = sizeof(NOT_HEADER_STRU);

    iMask = CcbGetFmObjState(APP_FM_FM_MASK);

    for (iLoop = 0; iLoop < APP_FM_FLOW_METER_NUM; iLoop++ )
    {

        if (iMask & (1 << iLoop))
        {
            pItem->ucId          = (unsigned char )iLoop;
            pItem->ulValue       = FlowMeter.aFmObjs[iLoop].Value.ulV;
            pItem++;   

            iLength += sizeof(NOT_FM_ITEM_STRU);
        }
    }

    pItem->ucId = 0XFF;

    iLength += sizeof(NOT_FM_ITEM_STRU);

    emitNotify(buffer,iLength);
}


void CCB::CcbNotState(int state)
{

    int iLength;
    unsigned char buffer[128];
    
    NOT_INFO_STRU       *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_STATE_ITEM_STRU *pItem   = (NOT_STATE_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_STATE;

    iLength = sizeof(NOT_HEADER_STRU);

    pItem->ucId   = state;
    iLength += sizeof(NOT_STATE_ITEM_STRU);

    pItem++;
    pItem->ucId = 0XFF;
    iLength += sizeof(NOT_STATE_ITEM_STRU);

    emitNotify(buffer,iLength);

    
}

void CCB::CcbNotSpeed(int iType,int iSpeed)
{

    int iLength;
    unsigned char buffer[128];
    
    NOT_INFO_STRU       *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_SPEED_ITEM_STRU *pItem   = (NOT_SPEED_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_SPEED;

    iLength = sizeof(NOT_HEADER_STRU);

    pItem->iType   = iType;
    pItem->iSpeed  = iSpeed;
    iLength += sizeof(NOT_SPEED_ITEM_STRU);

    emitNotify(buffer,iLength);

    
}

void CCB::CcbNotDecPressure(int iType,int iAction)
{

    int iLength;
    unsigned char buffer[128];
    
    NOT_INFO_STRU       *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_DECPRE_ITEM_STRU *pItem   = (NOT_DECPRE_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_DEC_PRESSURE;

    iLength = sizeof(NOT_HEADER_STRU);

    pItem->iType   = iType;
    pItem->iAction = iAction;
    
    iLength += sizeof(NOT_DECPRE_ITEM_STRU);

    emitNotify(buffer,iLength);
    
}

void CCB::CcbNotSWPressure()
{
    unsigned char buffer[128];

    int iLength;
    
    NOT_INFO_STRU       *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_PM_ITEM_STRU *pItem   = (NOT_PM_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_SW_PRESSURE;

    iLength = sizeof(NOT_HEADER_STRU);

    pItem->ucId      = APP_EXE_PM1_NO;
    pItem->ulValue   = ExeBrd.aPMObjs[APP_EXE_PM1_NO].Value.ulV;;
    
    iLength += sizeof(NOT_PM_ITEM_STRU);

    pItem++;
    pItem->ucId = 0XFF;
    iLength += sizeof(NOT_PM_ITEM_STRU);

    emitNotify(buffer,iLength);

    
}

void CCB::CcbNotAscInfo(int iInfoIdx)
{
    UNUSED(iInfoIdx);
}

void CCB::CcbNvStaticsNotify()
{
    unsigned char buffer[128];
    int iLoop ;

    int iLength;

    int iReportFlag = FALSE;
    
    NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_NVS_ITEM_STRU *pItem = (NOT_NVS_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_NV_STAT;

    iLength = sizeof(NOT_HEADER_STRU);

    for (iLoop = 0; iLoop < APP_EXE_RECT_NUM; iLoop++ )
    {

        if (ExeBrd.aRectObjs[iLoop].ulDurSec)
        {
            iReportFlag = TRUE;
        
            pItem->ucId          = (unsigned char )iLoop;
            pItem->ulValue       = ExeBrd.aRectObjs[iLoop].ulDurSec;
            ExeBrd.aRectObjs[iLoop].ulDurSec = 0;
            ExeBrd.aRectObjs[iLoop].ulSec    = DispGetCurSecond();
            pItem++;  
            
            iLength += sizeof(NOT_NVS_ITEM_STRU);
        }

    }

    pItem->ucId = 0XFF;

    iLength += sizeof(NOT_NVS_ITEM_STRU);

    if (iReportFlag) emitNotify(buffer,iLength);
}

/* current only for C4 */
void CCB::CcbPumpStaticsNotify()
{
    int iIdx = (APP_EXE_C4_NO - APP_EXE_C3_NO);

    if (ExeBrd.aGPumpObjs[iIdx].ulDurSec)
    {
        int iLength;
        unsigned char buffer[128];
        
        int iReportFlag = FALSE;
        
        NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;
        
        NOT_FMS_ITEM_STRU *pItem = (NOT_FMS_ITEM_STRU *)pNotify->aucData;
        
        pNotify->Hdr.ucCode = DISP_NOT_TUBEUV_STATE;
        
        iLength = sizeof(NOT_HEADER_STRU);
    
        iReportFlag = TRUE;
    
        pItem->ucId          = (unsigned char )iIdx;
        pItem->ulValue       = ExeBrd.aGPumpObjs[iIdx].ulDurSec;
        ExeBrd.aGPumpObjs[iIdx].ulDurSec = 0;
        ExeBrd.aGPumpObjs[iIdx].ulSec    = DispGetCurSecond();
        pItem++;  
        
        iLength += sizeof(NOT_FMS_ITEM_STRU);

        pItem->ucId = 0XFF;
    
        iLength += sizeof(NOT_FMS_ITEM_STRU);
    
        if (iReportFlag) emitNotify(buffer,iLength);
        
    }

}



/* current only for C4 */
void CCB::CcbFmStaticsNotify()
{
    int iLoop ;
    int iLength;
    int iReportFlag = FALSE;
    unsigned char buffer[128];
    
    NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_FM_ITEM_STRU *pItem = (NOT_FM_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_FM_STAT;

    iLength = sizeof(NOT_HEADER_STRU);

    for (iLoop = 0; iLoop < APP_FM_FLOW_METER_NUM; iLoop++ )
    {

        if (INVALID_FM_VALUE != FlowMeter.aHist[iLoop].curValue.ulV
            && INVALID_FM_VALUE != FlowMeter.aHist[iLoop].lstValue.ulV)
        {
            iReportFlag = TRUE;
        
            pItem->ucId          = (unsigned char )iLoop;
            pItem->ulValue       = FlowMeter.aHist[iLoop].curValue.ulV 
                                   - FlowMeter.aHist[iLoop].lstValue.ulV
                                   + FlowMeter.aulHistTotal[iLoop];
            
            FlowMeter.aHist[iLoop].curValue.ulV = INVALID_FM_VALUE;
            FlowMeter.aHist[iLoop].lstValue.ulV = INVALID_FM_VALUE;
            FlowMeter.aulHistTotal[iLoop] = 0;
            
            pItem++;  
            
            iLength += sizeof(NOT_FM_ITEM_STRU);
        }

    }

    pItem->ucId = 0XFF;

    iLength += sizeof(NOT_FM_ITEM_STRU);

    if (iReportFlag) emitNotify(buffer,iLength);

}


void CCB::CcbTwStaticsNotify(int iType,int iIdx,QTW_MEAS_STRU *pMeas)
{
    int iLength;
    unsigned char buffer[128];

    NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_NVS_ITEM_STRU *pItem = (NOT_NVS_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_TW_STAT;

    iLength = sizeof(NOT_HEADER_STRU);

    
    pItem->ucType        = (unsigned char )iType;
    pItem->ucId          = (unsigned char )iIdx;
    pItem->ulValue       = pMeas->ulCurFm - pMeas->ulBgnFm;
    pItem->ulBgnTime     = pMeas->ulBgnTm;
    pItem->ulEndTime     = pMeas->ulEndTm;
    pItem++;  
    
    iLength += sizeof(NOT_NVS_ITEM_STRU);


    pItem->ucId = 0XFF;

    iLength += sizeof(NOT_NVS_ITEM_STRU);

    emitNotify(buffer,iLength);
}

void CCB::CcbRealTimeQTwVolumnNotify(unsigned int value)
{
    int iLength;
    unsigned char buffer[128];

    NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_REALTIME_QTW_VOLUME_ITEM_STRU *pItem = (NOT_REALTIME_QTW_VOLUME_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_REALTIME_QTW_VOLUME;

    iLength = sizeof(NOT_HEADER_STRU);

    pItem->ulValue = value;

    iLength += sizeof(NOT_REALTIME_QTW_VOLUME_ITEM_STRU);

    emitNotify(buffer,iLength);

}

void CCB::CcbQTwVolumnNotify(int iType,int iIdx,QTW_MEAS_STRU *pMeas)
{
    if (INVALID_FM_VALUE != pMeas->ulTotalFm)
    {
        int iLength;
        unsigned char buffer[128];
        
        NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;
        
        NOT_QTW_VOLUME_ITEM_STRU *pItem = (NOT_QTW_VOLUME_ITEM_STRU *)pNotify->aucData;
        
        pNotify->Hdr.ucCode = DISP_NOT_QTW_VOLUME;
        
        iLength = sizeof(NOT_HEADER_STRU);
        
        pItem->ucType        = (unsigned char )iType;
        pItem->ucId          = (unsigned char )iIdx;
        pItem->ulValue       = pMeas->ulTotalFm;
        pItem++;  
        
        iLength += sizeof(NOT_QTW_VOLUME_ITEM_STRU);
        
        pItem->ucId = 0XFF;
        
        iLength += sizeof(NOT_QTW_VOLUME_ITEM_STRU);
        
        emitNotify(buffer,iLength);
    }
}



void CCB::CcbWashStateNotify(int iType,int state)
{
    int iLength;

    unsigned char buffer[128];

    NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_WH_ITEM_STRU *pItem = (NOT_WH_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_WH_STAT;

    iLength = sizeof(NOT_HEADER_STRU);

    
    pItem->ucType        = (unsigned char )iType;
    pItem->ucState       = (unsigned char )state;
    pItem++;  
    
    iLength += sizeof(NOT_WH_ITEM_STRU);

    emitNotify(buffer,iLength);
}




void CCB::CcbSwithStateNotify()
{
    int iLength;

    unsigned char buffer[128];

    NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;

    pNotify->Hdr.ucCode = DISP_NOT_SWITCH_STATE;

    iLength = sizeof(NOT_HEADER_STRU);

    emitNotify(buffer,iLength);
}

void CCB::CcbRPumpStateNotify(int ichl,int state)
{
    int iLength;

    unsigned char buffer[128];

    NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_RPUMP_STATE_ITEM_STRU *pItem = (NOT_RPUMP_STATE_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_RPUMP_STATE;

    iLength = sizeof(NOT_HEADER_STRU);

    pItem->ucId = ichl;

    pItem->ucState = state;

    emitNotify(buffer,iLength);
}


void CCB::CcbProduceWater(unsigned int ulBgnTm)
{
    int iLength;
    unsigned char buffer[128];

    NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_WP_ITEM_STRU *pItem = (NOT_WP_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_PWDURATION_STAT;

    iLength = sizeof(NOT_HEADER_STRU);

    
    pItem->ucId          = 0;
    pItem->ulBgnTime     = ulBgnTm;
    pItem++;  
    
    iLength += sizeof(NOT_WP_ITEM_STRU);

    pItem->ucId = 0XFF;

    iLength += sizeof(NOT_WP_ITEM_STRU);

    emitNotify(buffer,iLength);
}

void CCB::CcbPwVolumeStatistics()
{
    int iLength;

    int iLoop;

    unsigned char buffer[128];

    NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_FMS_ITEM_STRU *pItem = (NOT_FMS_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_PWVOLUME_STAT;

    iLength = sizeof(NOT_HEADER_STRU);


    for (iLoop = APP_FM_FM3_NO ; iLoop <= APP_FM_FM4_NO; iLoop++)
    {
        pItem->ucId = iLoop;

        if (FlowMeter.aulPwStart[iLoop] != INVALID_FM_VALUE
            && FlowMeter.aulPwEnd[iLoop]!= INVALID_FM_VALUE)
        {
             pItem->ulValue                   = FlowMeter.aulPwEnd[iLoop] - FlowMeter.aulPwStart[iLoop];
             FlowMeter.aulPwStart[iLoop] = INVALID_FM_VALUE;
             FlowMeter.aulPwEnd[iLoop]   = INVALID_FM_VALUE;
             iLength += sizeof(NOT_FMS_ITEM_STRU);
             pItem++;  
        }

    }
    pItem->ucId = 0XFF;

    iLength += sizeof(NOT_FMS_ITEM_STRU);

    emitNotify(buffer,iLength);
}



void CCB::CcbRfidNotify(int ucIndex)
{

    int iLength;

    unsigned char buffer[128];

    NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_RFID_ITEM_STRU *pItem = (NOT_RFID_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_RF_STATE;

    iLength = sizeof(NOT_HEADER_STRU);

    pItem->ucId    = (unsigned char )ucIndex;
    pItem->ucState = (aRfReader[ucIndex].bit1RfDetected && aRfReader[ucIndex].bit1Active);
    
    pItem++;   

    iLength     += sizeof(NOT_RFID_ITEM_STRU);

    pItem->ucId = 0XFF;

    iLength += sizeof(NOT_RFID_ITEM_STRU);

    emitNotify(buffer,iLength);

    
}

void CCB::CcbHandlerNotify(int ucIndex)
{

    int iLength;

    unsigned char buffer[128];

    NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_HANDLER_ITEM_STRU *pItem = (NOT_HANDLER_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_HANDLER_STATE;

    iLength = sizeof(NOT_HEADER_STRU);

    pItem->ucId    = (unsigned char )ucIndex;
    pItem->ucState = aHandler[ucIndex].bit1Active;
    
    pItem++;   

    iLength    += sizeof(NOT_HANDLER_ITEM_STRU);

    pItem->ucId = 0XFF;

    iLength += sizeof(NOT_HANDLER_ITEM_STRU);

    emitNotify(buffer,iLength);

    
}

void CCB::CcbExeBrdNotify()
{

    int iLength;
    unsigned char buffer[128];

    NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_EXEBRD_ITEM_STRU *pItem = (NOT_EXEBRD_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_EXEBRD_STATE;

    iLength = sizeof(NOT_HEADER_STRU);

    pItem->ucId    = 0;
    pItem->ucState = !!(ulActiveMask & (1 << APP_PAKCET_ADDRESS_EXE));
    
    pItem++;   

    iLength     += sizeof(NOT_EXEBRD_ITEM_STRU);

    pItem->ucId = 0XFF;

    iLength += sizeof(NOT_EXEBRD_ITEM_STRU);

    emitNotify(buffer,iLength);

    
}

void CCB::CcbFmBrdNotify()
{

    int iLength;
    unsigned char buffer[128];

    NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_FMBRD_ITEM_STRU *pItem = (NOT_FMBRD_ITEM_STRU *)pNotify->aucData;

    pNotify->Hdr.ucCode = DISP_NOT_FMBRD_STATE;

    iLength = sizeof(NOT_HEADER_STRU);

    pItem->ucId    = 0;
    pItem->ucState = !!(ulActiveMask & (1 << APP_PAKCET_ADDRESS_FM));
    
    pItem++;   

    iLength     += sizeof(NOT_FMBRD_ITEM_STRU);

    pItem->ucId = 0XFF;

    iLength += sizeof(NOT_FMBRD_ITEM_STRU);

    emitNotify(buffer,iLength);

    
}

void CCB::CcbTocNotify()
{
    int iLength;
    unsigned char buffer[128];

    NOT_INFO_STRU *pNotify = (NOT_INFO_STRU *)buffer;

    NOT_TOC_ITEM_STRU *pItem = (NOT_TOC_ITEM_STRU *)pNotify->aucData;

    LOG(VOS_LOG_DEBUG,"Enter %s",__FUNCTION__);                       

    pNotify->Hdr.ucCode = DISP_NOT_TOC;

    iLength = sizeof(NOT_HEADER_STRU);

    pItem->fB = ExeBrd.tocInfo.fB;
    pItem->fP = ExeBrd.tocInfo.fP;

    iLength += sizeof(NOT_TOC_ITEM_STRU);

    emitNotify(buffer,iLength);
   
}


void CCB::CanCcbAfDataClientRpt4ExeDinAck(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    /* send response */
    unsigned int ulCanId;

    int iPayLoad;
    
    APP_PACKET_CLIENT_RPT_IND_STRU *pmg = (APP_PACKET_CLIENT_RPT_IND_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0];

    pmg->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
    pmg->hdr.ucMsgType |= 0x80;

    CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,APP_PAKCET_ADDRESS_EXE);

    iPayLoad = APP_POROTOL_PACKET_CLIENT_RPT_IND_TOTAL_LENGTH + sizeof(APP_PACKET_RPT_DIN_STRU);

    CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,(unsigned char *)pmg,iPayLoad);

}

void CCB::CcbPushState()
{   
    if (iWorkStateStackDepth < CCB_WORK_STATE_STACK_DEPTH)
    {
        aWorkStateStack[iWorkStateStackDepth++] = curWorkState;
    }
}

void CCB::CcbPopState()
{
    if (iWorkStateStackDepth > 0)
    {
        curWorkState = aWorkStateStack[iWorkStateStackDepth--];
    }

}

void CCB::CcbCancelKeyWork(void *param)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)param;
    pWorkItem = pWorkItem;
}

//NOTE: 2020.2.17 增加漏水单独处理程序, 需要测试
/**
 * @Author: dcj
 * @Date: 2020-02-17 12:20:07
 * @LastEditTime: 2020-02-17 12:20:07
 * @Description: 处理漏水保护信号出错时，设置当前状态未发生漏水且发送失败通知
 * @Param : 
 * @Return: 
 */
void CCB::work_start_Leak_fail(int iKeyId)
{
    int aiCont[1] = {-1};
    UNUSED(iKeyId);
    ucLeakWorkStates = 0; 
    MainSndWorkMsg(WORK_MSG_TYPE_SLH, (unsigned char *)aiCont, sizeof(aiCont));
}

/**
 * @Author: dcj
 * @Date: 2020-02-17 12:21:45
 * @LastEditTime: 2020-02-17 12:21:45
 * @Description: 处理漏水保护信号成功时，发送成功通知
 * @Param : 
 * @Return: 
 */
void CCB::work_start_Leak_succ()
{
    int aiCont[1] = {0};
    MainSndWorkMsg(WORK_MSG_TYPE_SLH, (unsigned char *)aiCont, sizeof(aiCont));
}

/**
 * @Author: dcj
 * @Date: 2020-02-17 12:24:51
 * @LastEditTime: 2020-02-17 12:24:51
 * @Description: 漏水保护处理任务：发生漏水时，关闭所有负载并触发漏水保护报警
 * @Param : 
 * @Return: 
 */
void CCB::work_start_Leak(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;
    CCB                 *pCcb = (CCB *)pWorkItem->para;
    int iTmp = 0;

    /* stop all */
    int iRet = pCcb->CcbSetSwitch(pWorkItem->id, 0, iTmp); // don care modbus exe result
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);
        /* notify ui (late implemnt) */
        pCcb->work_start_Leak_fail((int)pWorkItem->extra);
        return ;
    }

    pCcb->bit1LeakKey4Reset = TRUE;
    pCcb->CcbNotAlarmFire(0XFF, DISP_ALARM_B_LEAK, TRUE);

    pCcb->CanCcbTransState(DISP_WORK_STATE_KP, DISP_WORK_SUB_IDLE);
    pCcb->work_start_Leak_succ();
}

/**
 * @Author: dcj
 * @Date: 2020-02-17 12:26:43
 * @LastEditTime: 2020-02-17 12:26:43
 * @Description: 
 * @Param : 
 * @Return: 
 */
void CCB::work_stop_Leak_succ()
{
    int aiCont[1] = {0};
    MainSndWorkMsg(WORK_MSG_TYPE_ELH, (unsigned char *)aiCont, sizeof(aiCont));
}

/**
 * @Author: dcj
 * @Date: 2020-02-17 12:27:00
 * @LastEditTime: 2020-02-17 12:27:00
 * @Description: 漏水保护恢复处理任务：漏水保护恢复后，消除漏水报警，并设置当前未发生漏水
 * @Param : 
 * @Return: 
 */
void CCB::work_stop_Leak(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;
    CCB *pCcb = (CCB *)pWorkItem->para;

    pCcb->CcbNotAlarmFire(0XFF, DISP_ALARM_B_LEAK, FALSE);
    pCcb->ucLeakWorkStates = 0; //设置当前未发生漏水保护
    pCcb->work_stop_Leak_succ();

    pCcb->DispCmdEntry(DISP_CMD_LEAK_RESET,NULL,0);
}

/**
 * @Author: dcj
 * @Date: 2020-02-17 12:29:15
 * @LastEditTime: Do not edit
 * @Description: 发生漏水保护时设备停止取水、停止运行，并启动漏水保护处理任务
 * @Param : 
 * @Return: 
 */
DISPHANDLE CCB::CcbInnerWorkStartLeakWok()
{
    int iLoop;
    //如果漏水保护停止机器运行
    if(ExeBrd.ucLeakState)
    {
        if (DISP_WORK_STATE_IDLE != DispGetWorkState())
        {
            DispCmdEntry(DISP_CMD_HALT, NULL, 0);
        }

        for (iLoop = 0; iLoop < MAX_HANDLER_NUM; iLoop++)
        {
            if (aHandler[iLoop].bit1Qtw)
            {
                aHandler[iLoop].bit1Qtw = 0;
            }
        }
    } 

    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc   = work_start_Leak;
    pWorkItem->cancel = CcbCancelKeyWork;
    pWorkItem->para   = this;

    CcbAddWork(WORK_LIST_URGENT, pWorkItem);

    return (DISPHANDLE)pWorkItem;
}

/**
 * @Author: dcj
 * @Date: 2020-02-17 12:10:45
 * @LastEditTime: 2020-02-17 12:10:45
 * @Description: 漏水信号消失时启动对应的处理任务
 * @Param : 
 * @Return: 
 */
DISPHANDLE CCB::CcbInnerWorkStopLeakWork()
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc   = work_stop_Leak;
    pWorkItem->cancel = NULL;
    pWorkItem->para   = this;
    CcbAddWork(WORK_LIST_LP, pWorkItem);

    return (DISPHANDLE)pWorkItem;
}

/**
 * @Author: dcj
 * @Date: 2020-02-17 12:09:08
 * @LastEditTime: 2020-02-17 
 * @Description: 用于处理从Can总线获取的漏水保护信号
 * @Param : 
 * @Return: 
 */
void CCB::CanCcbLeakProcess()
{
    LOG(VOS_LOG_WARNING, "%s %d", __FUNCTION__, ExeBrd.ucLeakState);    
    
    if(ExeBrd.ucLeakState)
    {
        if(!ucLeakWorkStates)
        {
            CcbInnerWorkStartLeakWok();
            ucLeakWorkStates = ExeBrd.ucLeakState;
            return;
        }
    }
    else
    {
        if(ucLeakWorkStates)
        {
            CcbInnerWorkStopLeakWork();
        }
    }
}

void CCB::work_start_key_fail(int iKeyId)
{
    int aiCont[1] = {-1};

    ulKeyWorkStates &= ~(1<<iKeyId); //  finish work
    
    MainSndWorkMsg(WORK_MSG_TYPE_SKH,(unsigned char *)aiCont,sizeof(aiCont));
}

void CCB::work_start_key_succ(void)
{
    int aiCont[1] = {0};
    
    MainSndWorkMsg(WORK_MSG_TYPE_SKH,(unsigned char *)aiCont,sizeof(aiCont));
}

//12.18
void CCB::work_start_key(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;
    CCB            *pCcb      = (CCB *)pWorkItem->para;

    int             iTmp,iRet;

    switch((int)pWorkItem->extra)
    {
    case APP_EXE_DIN_RF_KEY:
        {
            /* E10 ON */
            if(is_SWPUMP_FRONT())
            {
                iTmp = (1 << APP_EXE_E10_NO)|(1 << APP_EXE_C3_NO);
            }
            else
            {
                iTmp = (1 << APP_EXE_E10_NO);
            }

            iRet = pCcb->CcbUpdateSwitch(pWorkItem->id,0,iTmp,iTmp);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);
                pCcb->work_start_key_fail((int)pWorkItem->extra);

                return ;
            }

            //CanCcbTransState(DISP_WORK_STATE_KP,DISP_WORK_SUB_IDLE);
        }
        break;
    case APP_EXE_DIN_LEAK_KEY:
        {
            /* stop all */
#if 0
            iTmp = 0;
            iRet = pCcb->CcbSetSwitch(pWorkItem->id,0,iTmp); // don care modbus exe result
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);

                /* notify ui (late implemnt) */
                pCcb->work_start_key_fail((int)pWorkItem->extra);

                return ;
            }
#endif
            /* E3 ON */
            iTmp = (1 << APP_EXE_E3_NO);
            iRet = pCcb->CcbUpdateSwitch(pWorkItem->id,0,iTmp,iTmp);
            
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);
                pCcb->work_start_key_fail((int)pWorkItem->extra);

                return ;
            }


            pCcb->bit1LeakKey4Reset = TRUE;
            pCcb->CcbNotAlarmFire(0XFF, DISP_ALARM_B_TANKOVERFLOW, TRUE);
            
            pCcb->CanCcbTransState(DISP_WORK_STATE_KP,DISP_WORK_SUB_IDLE);
        }
        break;
    }

    /* push state */
    // CcbPushState();

    pCcb->work_start_key_succ();

}

void CCB::work_stop_key_succ()
{
    int aiCont[1] = {0};
    
    MainSndWorkMsg(WORK_MSG_TYPE_EKH,(unsigned char *)aiCont,sizeof(aiCont));
}

void CCB::work_stop_key(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;
    CCB            *pCcb      = (CCB *)pWorkItem->para;
    int             iRet;
    int             iTmp;

    /* stop all */
    switch((int)pWorkItem->extra)
    {
    case APP_EXE_DIN_RF_KEY:
        {
            /* E10 OFF */
            if(is_SWPUMP_FRONT())
            {
                iTmp = (1 << APP_EXE_E10_NO)|(1 << APP_EXE_C3_NO);
            }
            else
            {
                iTmp = 1 << APP_EXE_E10_NO;
            }
            iRet = pCcb->CcbUpdateSwitch(pWorkItem->id,0,iTmp,0);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);
            }


            if (DISP_WORK_STATE_IDLE == pCcb->DispGetWorkState())
            {
                pCcb->DispCmdEntry(DISP_CMD_RUN,NULL, 0);
            }
        }
        break;
    case APP_EXE_DIN_LEAK_KEY:
        {
            iTmp = (1 << APP_EXE_E3_NO);
            iRet = pCcb->CcbUpdateSwitch(pWorkItem->id, 0, iTmp, 0);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);
            }
            
            pCcb->CcbNotAlarmFire(0XFF, DISP_ALARM_B_TANKOVERFLOW, FALSE);
            
            pCcb->DispCmdEntry(DISP_CMD_LEAK_RESET,NULL,0);
        }
        break;
    }

    iTmp = (int)pWorkItem->extra;

    pCcb->ulKeyWorkStates &= ~(1 << iTmp);

    pCcb->work_stop_key_succ();

}

DISPHANDLE CCB::CcbInnerWorkStartKeyWok(int iKey)
{
    int iLoop;
    //2018.12.18 add
    if(iKey == APP_EXE_DIN_RF_KEY || iKey == APP_EXE_DIN_LEAK_KEY)
    {
        if (DISP_WORK_STATE_IDLE != DispGetWorkState())
        {
            DispCmdEntry(DISP_CMD_HALT,NULL,0);
        }

        for (iLoop = 0; iLoop < MAX_HANDLER_NUM; iLoop++)
        {
            if (aHandler[iLoop].bit1Qtw)
            {
                aHandler[iLoop].bit1Qtw = 0;
            }
        }
    } //end

    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc = work_start_key;
    pWorkItem->para = this;
    pWorkItem->extra = (void *)iKey;
    pWorkItem->cancel = CcbCancelKeyWork;

    CcbAddWork(WORK_LIST_URGENT,pWorkItem);

    return (DISPHANDLE)pWorkItem;

}

/*
  Protect by Mutex
*/
void CCB::CcbAbortWork(WORK_ITEM_STRU *pWorkItem)
{

    int id = pWorkItem->id;

    if (!ulWorkThdState)
    {
        return;
    }
    
    LOG(VOS_LOG_WARNING,"WorkList%d: CcbAbortWork state %d",id,ulWorkThdState);    

    if (ulWorkThdState & MakeThdState(id, WORK_THREAD_STATE_BLK_MODBUS))
    {
        ulWorkThdAbort |= MakeThdState(id,WORK_THREAD_STATE_BLK_MODBUS);
        sp_thread_mutex_lock  ( &Ipc.mutex );
        sp_thread_cond_signal ( &Ipc.cond  );// ylf: all thread killed
        sp_thread_mutex_unlock( &Ipc.mutex );
    }

    if (ulWorkThdState & MakeThdState(id, WORK_THREAD_STATE_BLK_TICKING))
    {
        ulWorkThdAbort |= MakeThdState(id, WORK_THREAD_STATE_BLK_TICKING);
        sp_thread_sem_post(&Sem4Delay[id]);
    }
    
}

DISPHANDLE CCB::CcbInnerWorkStopKeyWork(int iKey)
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc = work_stop_key;
    pWorkItem->para = this;
    pWorkItem->extra = (void *)iKey;
    pWorkItem->cancel = NULL;

    CcbAddWork(WORK_LIST_LP,pWorkItem);

    return (DISPHANDLE)pWorkItem;

}

void CCB::CanCcbDinProcess()
{
    int iLoop;

    LOG(VOS_LOG_WARNING,"%s %d",__FUNCTION__,ExeBrd.ucDinState);    
     
    
    /* check state */
    for (iLoop = 0; iLoop < APP_EXE_DIN_NUM; iLoop++)
    {
        int iDinMask = (1<<iLoop);

        if (iDinMask & APP_EXE_DIN_FAIL_MASK)
        {
            if ((iDinMask & ulKeyWorkStates) && (iDinMask & ExeBrd.ucDinState))
            {
                // work ongoing
                return ;
            }
        
            if ((iDinMask & ulKeyWorkStates) && (!(iDinMask & ExeBrd.ucDinState)))
            {
                // stop working
                CcbInnerWorkStopKeyWork(iLoop);
                
                return ;
            }
        
            if ((!ulKeyWorkStates) && (iDinMask & ExeBrd.ucDinState))
            {
                /* append work */
                CcbInnerWorkStartKeyWok(iLoop);
        
                ulKeyWorkStates |= (1<<iLoop); // prevent further action
                return ;
            }
        }
    }

}

void CCB::DispC1Regulator(int iC1Regulator)
{
//    if((!bit1B1Check4RuningState) && (DISP_ACT_TYPE_SWITCH & ExeBrd.aRPumpObjs[0].iActive))
    if(!bit1B1Check4RuningState)
    {
        CcbUpdateRPumpObjState(0, 0X0000);
        return;
    }
    sp_thread_mutex_lock( &C12Mutex );
    
    if (DISP_ACT_TYPE_SWITCH & ExeBrd.aRPumpObjs[0].iActive)
    {
        float ft = (0.1 * ExeBrd.aEcoObjs[APP_EXE_I1_NO].Value.eV.usTemp);
        float fv = 0.012*ft*ft - 0.8738*ft + 33;  //0.012*水温2 - 0.8738*水温 + 33

        if(!iC1Regulator)
        {
            fv = 24.0;
        }
        
        int speed = DispConvertVoltage2RPumpSpeed(fv);

        if (speed != (0X00FF & ExeBrd.ausHoldRegister[APP_EXE_HOLD_REG_RPUMP_OFFSET]))
        {
            DispSetRPump(0,(0XFF00|speed));
        }
    }
    
    sp_thread_mutex_unlock( &C12Mutex );
}

int CCB::CcbC2Regulator(int id,float fv,int on)
{
    int speed = DispConvertVoltage2RPumpSpeed(fv);
    int iRet = 0;

    if (speed != (0X00FF & ExeBrd.ausHoldRegister[APP_EXE_HOLD_REG_RPUMP_OFFSET+1]))
    {
        if (on)
        {
           iRet = CcbSetRPump(id,1,(0XFF00|speed));
        }
        else
        {
            iRet = CcbSetRPump(id,1,(speed));
        }
    }

    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbC2Regulator Fail %d",iRet);    
        return iRet;
    }   

    return 0;
}

void CCB::smoothI2Data(float newValue)
{
    static float oldValue = 1000.0;
    static int discard;

    float multiple = newValue/oldValue;
    if(multiple < 3 || discard >= 10)
    {
        ExeBrd.aEcoObjs[APP_EXE_I2_NO].Value.eV.fWaterQ = newValue;
        oldValue = newValue;
    }
    else
    {
        discard++;
        return;
    }

    discard = 0;
}


int CCB::CanCcbAfDataClientRpt4ExeBoard(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_CLIENT_RPT_IND_STRU *pmg = (APP_PACKET_CLIENT_RPT_IND_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0];

    switch(pmg->ucRptType)
    {
    case APP_PACKET_RPT_ECO:
        {
            int iPackets = (pmg->hdr.ucLen - 1) / sizeof (APP_PACKET_RPT_ECO_STRU);
            APP_PACKET_RPT_ECO_STRU *pEco = (APP_PACKET_RPT_ECO_STRU *)pmg->aucData;
            int iLoop;
            int iEco4ZigbeeMask = 0;
            
            for (iLoop = 0; iLoop < iPackets; iLoop++,pEco++ )
            {
                if (pEco->ucId < APP_EXE_ECO_NUM)
                {
                    if(APP_EXE_I2_NO == pEco->ucId && bit1ProduceWater)
                    {
                        smoothI2Data(pEco->ev.fWaterQ);
                    }
                    else
                    {
                        ExeBrd.aEcoObjs[pEco->ucId].Value.eV.fWaterQ = pEco->ev.fWaterQ;
                    }
                    //ExeBrd.aEcoObjs[pEco->ucId].Value.eV.fWaterQ = pEco->ev.fWaterQ;
                    ExeBrd.aEcoObjs[pEco->ucId].Value.eV.usTemp = pEco->ev.usTemp;
                    ExeBrd.ulEcoValidFlags |= 1 << pEco->ucId;
                    
                    switch(pEco->ucId)
                    {
                    case 0:
                        ExeBrd.aEcoObjs[pEco->ucId].Value.eV.fWaterQ *= gGlobalParam.Caliparam.pc[DISP_PC_COFF_SOURCE_WATER_CONDUCT].fk;
                        ExeBrd.aEcoObjs[pEco->ucId].Value.eV.usTemp *= gGlobalParam.Caliparam.pc[DISP_PC_COFF_SOURCE_WATER_TEMP].fk;
                        break;
                    case 1:
                        ExeBrd.aEcoObjs[pEco->ucId].Value.eV.fWaterQ *= gGlobalParam.Caliparam.pc[DISP_PC_COFF_RO_WATER_CONDUCT].fk;
                        ExeBrd.aEcoObjs[pEco->ucId].Value.eV.usTemp *= gGlobalParam.Caliparam.pc[DISP_PC_COFF_RO_WATER_TEMP].fk;
                        break;
                    case 2:
                        ExeBrd.aEcoObjs[pEco->ucId].Value.eV.fWaterQ *= gGlobalParam.Caliparam.pc[DISP_PC_COFF_EDI_WATER_CONDUCT].fk;
                        ExeBrd.aEcoObjs[pEco->ucId].Value.eV.usTemp *= gGlobalParam.Caliparam.pc[DISP_PC_COFF_EDI_WATER_TEMP].fk;

                        if(gGlobalParam.iMachineType != MACHINE_C)
                        {
                            if(ExeBrd.aEcoObjs[pEco->ucId].Value.eV.fWaterQ > 16)
                                ExeBrd.aEcoObjs[pEco->ucId].Value.eV.fWaterQ = 16;
                        }
                        else
                        {
                            if(ExeBrd.aEcoObjs[pEco->ucId].Value.eV.fWaterQ > 18.2)
                                ExeBrd.aEcoObjs[pEco->ucId].Value.eV.fWaterQ = 18.2;
                        }
                        break;
                    case 3:
                        ExeBrd.aEcoObjs[pEco->ucId].Value.eV.fWaterQ *= gGlobalParam.Caliparam.pc[DISP_PC_COFF_TOC_WATER_CONDUCT].fk;
                        ExeBrd.aEcoObjs[pEco->ucId].Value.eV.usTemp *= gGlobalParam.Caliparam.pc[DISP_PC_COFF_TOC_WATER_TEMP].fk;
                        if(ExeBrd.aEcoObjs[pEco->ucId].Value.eV.fWaterQ > 18.2)
                            ExeBrd.aEcoObjs[pEco->ucId].Value.eV.fWaterQ = 18.2;
                        break;
                    case 4:
                        ExeBrd.aEcoObjs[pEco->ucId].Value.eV.fWaterQ *= gGlobalParam.Caliparam.pc[DISP_PC_COFF_UP_WATER_CONDUCT].fk;
                        ExeBrd.aEcoObjs[pEco->ucId].Value.eV.usTemp *= gGlobalParam.Caliparam.pc[DISP_PC_COFF_UP_WATER_TEMP].fk;
                        if(ExeBrd.aEcoObjs[pEco->ucId].Value.eV.fWaterQ > 18.2)
                            ExeBrd.aEcoObjs[pEco->ucId].Value.eV.fWaterQ = 18.2;
                        break;
                    }

                    if(ExeBrd.aEcoObjs[pEco->ucId].Value.eV.fWaterQ < 0.0)
                        ExeBrd.aEcoObjs[pEco->ucId].Value.eV.fWaterQ = 0;

                    if(ExeBrd.aEcoObjs[pEco->ucId].Value.eV.usTemp < 0)
                        ExeBrd.aEcoObjs[pEco->ucId].Value.eV.usTemp = 0;

                    CanCcbEcoMeasurePostProcess(pEco->ucId);

                    switch(gGlobalParam.iMachineType)
                    {
                    case MACHINE_EDI:
                        if(!haveHPCir() && (APP_EXE_I3_NO == pEco->ucId))
                        {
                            iEco4ZigbeeMask |= 0x1 << APP_EXE_I4_NO;
                        }
                        break;
                    case MACHINE_UP:
                    case MACHINE_RO:
                    case MACHINE_RO_H:
                    case MACHINE_C:
                        if(!haveHPCir() && (APP_EXE_I2_NO == pEco->ucId))
                        {
                            iEco4ZigbeeMask |= 0x1 << APP_EXE_I4_NO;
                        }
                        break;
                    }

                    if (APP_EXE_I4_NO == pEco->ucId)
                    {
                        iEco4ZigbeeMask |= 0x1 << APP_EXE_I4_NO;
                    }
                    else if (APP_EXE_I5_NO == pEco->ucId)
                    {
                        iEco4ZigbeeMask |= 0x1 << APP_EXE_I5_NO;;
                    }
                    //LOG(VOS_LOG_DEBUG,"wq %d&%f&%u",pEco->ucId,pEco->ev.fWaterQ,pEco->ev.usTemp);                       
                }
            }

            if (iEco4ZigbeeMask)
            {
                DispSndHoEco(iEco4ZigbeeMask);
            }

            CcbEcoNotify();
        }
        break;
    case APP_PACKET_RPT_PM:
        {
            int iPackets = (pmg->hdr.ucLen - 1) / sizeof (APP_PACKET_RPT_PM_STRU);
            APP_PACKET_RPT_PM_STRU *pPm = (APP_PACKET_RPT_PM_STRU *)pmg->aucData;
            int iLoop;
            
            for (iLoop = 0; iLoop < iPackets; iLoop++,pPm++ )
            {
                if (pPm->ucId < APP_EXE_PRESSURE_METER)
                {
                    ExeBrd.aPMObjs[pPm->ucId].Value.ulV = pPm->ulValue;
                    ExeBrd.ulPmValidFlags |= 1 << (pPm->ucId);

                    CanCcbPmMeasurePostProcess(pPm->ucId);
                    
                }
            }
            
            CcbPmNotify();
        }
        break;
    case APP_PACKET_RPT_RECT:
        {
            int iPackets = (pmg->hdr.ucLen - 1) / sizeof (APP_PACKET_RPT_RECT_STRU);
            APP_PACKET_RPT_PM_STRU *pPm = (APP_PACKET_RPT_RECT_STRU *)pmg->aucData;
            int iLoop;
            
            for (iLoop = 0; iLoop < iPackets; iLoop++,pPm++ )
            {
                if (pPm->ucId < APP_EXE_PRESSURE_METER)
                {
                    ExeBrd.aRectObjs[pPm->ucId].Value.ulV = pPm->ulValue;
                    ExeBrd.ulRectValidFlags |= 1 << (pPm->ucId);
                }
            }

            CcbRectNotify();
            //CanCcbRectNMeasurePostProcess(pPm->ucId); //ex
        }
        break;    
    case APP_PACKET_RPT_GPUMP:
        {
            int iPackets = (pmg->hdr.ucLen - 1) / sizeof (APP_PACKET_RPT_GPUMP_STRU);
            APP_PACKET_RPT_GPUMP_STRU *pPm = (APP_PACKET_RPT_GPUMP_STRU *)pmg->aucData;
            int iLoop;
            
            for (iLoop = 0; iLoop < iPackets; iLoop++,pPm++ )
            {
                if (pPm->ucId < APP_EXE_G_PUMP_NUM)
                {
                    ExeBrd.aGPumpObjs[pPm->ucId].Value.ulV = pPm->ulValue;

                }
            }
            
            CcbGPumpNotify();
        }
        break;             
    case APP_PACKET_RPT_RPUMP:
        {
            int iPackets = (pmg->hdr.ucLen - 1) / sizeof (APP_PACKET_RPT_RPUMP_STRU);
            APP_PACKET_RPT_RPUMP_STRU *pPm = (APP_PACKET_RPT_RPUMP_STRU *)pmg->aucData;
            int iLoop;
            
            for (iLoop = 0; iLoop < iPackets; iLoop++,pPm++ )
            {
                if (pPm->ucId < APP_EXE_R_PUMP_NUM)
                {
                    ExeBrd.aRPumpObjs[pPm->ucId].Value.ulV = pPm->ulValue;

                }
            }
            
            CcbRPumpNotify();
        }
        break;       
    case APP_PACKET_RPT_EDI:
        {
            int iPackets = (pmg->hdr.ucLen - 1) / sizeof (APP_PACKET_RPT_EDI_STRU);
            APP_PACKET_RPT_EDI_STRU *pPm = (APP_PACKET_RPT_EDI_STRU *)pmg->aucData;
            int iLoop;
            
            for (iLoop = 0; iLoop < iPackets; iLoop++,pPm++ )
            {
                if (pPm->ucId < APP_EXE_EDI_NUM)
                {
                    ExeBrd.aEDIObjs[pPm->ucId].Value.ulV = pPm->ulValue;

                }
            }
            
            CcbEDINotify();
        }
        break;               
    case APP_PACKET_DIN_STATUS:
        {
            APP_PACKET_RPT_DIN_STRU *pDin = (APP_PACKET_RPT_DIN_STRU *)pmg->aucData;
            ExeBrd.ucDinState = (~pDin->ucState) & APP_EXE_DIN_MASK;
            CanCcbAfDataClientRpt4ExeDinAck(pCanItfMsg);
            CanCcbDinProcess();
        }        
        break;
    case APP_PACKET_RPT_LEAK:
        {
            APP_PACKET_RPT_DIN_STRU *pDin = (APP_PACKET_RPT_DIN_STRU *)pmg->aucData;
            ExeBrd.ucLeakState = pDin->ucState;
            CanCcbAfDataClientRpt4ExeDinAck(pCanItfMsg);
            CanCcbLeakProcess();
        }
        break;
    case APP_PACKET_MT_STATUS:
        break;
    case APP_PACKET_RPT_TOC:
        {
            APP_PACKET_RPT_TOC_STRU *pToc = (APP_PACKET_RPT_TOC_STRU *)pmg->aucData;

            ExeBrd.tocInfo = *pToc;

            CcbTocNotify();
        }        
        break;
    }
    return 0;
}

int CCB::CanCcbAfDataClientRpt4RFReader(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_CLIENT_RPT_IND_STRU *pmg = (APP_PACKET_CLIENT_RPT_IND_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0];
    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);
    unsigned char ucResult;

    uint8_t ucIndex  = (iSrcAdr - APP_RF_READER_BEGIN_ADDRESS);

    if (ucIndex >=  MAX_RFREADER_NUM)
    {
        ucResult = APP_PACKET_RF_NO_DEVICE;
        goto end;
    }
        
    switch(pmg->ucRptType)
    {
    case APP_PACKET_RPT_RF_STATE:
        {
            APP_PACKET_RPT_RF_STATE_STRU *pRfState = (APP_PACKET_RPT_RF_STATE_STRU *)pmg->aucData;

            RFREADER_STRU *pRfReader = &aRfReader[ucIndex];

            pRfReader->bit1RfDetected = pRfState->ucState;

            if (pRfState->ucState)
            {
                APP_PACKET_RPT_RF_STATE_CONT_STRU *pCnt = (APP_PACKET_RPT_RF_STATE_CONT_STRU *)pRfState->aucData;
                pRfReader->ucBlkNum  = pCnt->ucBlkNum;
                pRfReader->ucBlkSize = pCnt->ucBlkSize;
                memcpy(pRfReader->aucUid,pCnt->aucUid,8);
                
                LOG(VOS_LOG_WARNING,"RF detected %d&%d",pCnt->ucBlkNum,pCnt->ucBlkSize);  
            }
            else
            {
                pRfReader->bit1RfContValid = FALSE;
                LOG(VOS_LOG_WARNING,"RF lost");  
            }
            CcbRfidNotify(ucIndex);
        }
        break;
    default:
        break;
    }

    return 0;

end : 
    return ucResult;

}


void CCB::CcbSetDataTime(uint32_t ulTimeStamp)
{
    int       fb;
    time_t    Time_t = ulTimeStamp;
    struct rtc_time rtctime ;
    struct tm *tmp_ptr = NULL;

    stime(&Time_t);  
    
    tmp_ptr = gmtime(&Time_t);

    rtctime.tm_sec  = tmp_ptr->tm_sec;
    rtctime.tm_min  = tmp_ptr->tm_min;
    rtctime.tm_hour = tmp_ptr->tm_hour;
    rtctime.tm_mday = tmp_ptr->tm_mday;
    rtctime.tm_mon  = tmp_ptr->tm_mon;
    rtctime.tm_year = tmp_ptr->tm_year;
    rtctime.tm_isdst= -1;

    fb = open("/dev/rtc0", QIODevice::ReadWrite);
    if (fb != -1)
    {
        int ret = ioctl(fb , RTC_SET_TIME , &rtctime);

        if (ret)
        {
            qDebug() << " ioctl : " << ret;
        }

        ::close(fb);

        LOG(VOS_LOG_WARNING,"set RTC to %u: %04d-%02d-%02d %02d:%02d:%02d",Time_t,tmp_ptr->tm_year,tmp_ptr->tm_mon,tmp_ptr->tm_mday,tmp_ptr->tm_hour,tmp_ptr->tm_min,tmp_ptr->tm_sec);    
    }                        
}    

int CCB::CcbReadRfid(int iIndex,int iTime,int offset,int len)
{
    int iRet;

    struct timeval now;
    struct timespec outtime;
    
    sp_thread_mutex_lock( &Ipc4Rfid.mutex );

    {
        char buf[64];
        IAP_CAN_CMD_STRU *pCmd = (IAP_CAN_CMD_STRU *)buf;
    
        APP_PACKET_RF_STRU *pHo = (APP_PACKET_RF_STRU *)pCmd->data;

        pCmd->iCanChl     = APP_CAN_CHL_OUTER;
        pCmd->ucCmd       = SAPP_CMD_DATA;
        pCmd->iPayLoadLen = APP_PROTOL_HEADER_LEN;
        pCmd->ulCanId     = iIndex;
    
        pHo->hdr.ucLen     = 1 + sizeof(APP_PACKET_RF_READ_REQ_STRU);
        pHo->hdr.ucTransId = 0XF0;
        pHo->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHo->hdr.ucMsgType = APP_PACKET_RF_OPERATION;
    
        pHo->ucOpsType     = APP_PACKET_RF_READ;

        {
            APP_PACKET_RF_READ_REQ_STRU *pRd = (APP_PACKET_RF_READ_REQ_STRU *)pHo->aucData;

            pRd->ucLen = len;
            pRd->ucOff = offset;
            
        }   
        pCmd->iPayLoadLen += pHo->hdr.ucLen;   
        iRet = DispAfRfEntry(pCmd);
        
        if (0 != iRet)
        {
            sp_thread_mutex_unlock( &Ipc4Rfid.mutex );
            
            return -1;
        }
    }

    iRfIdRequest = 0X1;
    
    gettimeofday(&now, NULL);
    outtime.tv_sec  = now.tv_sec + iTime/1000;
    outtime.tv_nsec = (now.tv_usec + (iTime  % 1000)*1000)* 1000;

    iRet = sp_thread_cond_timedwait( &Ipc4Rfid.cond, &Ipc4Rfid.mutex ,&outtime);//ylf: thread sleep here
    if(ETIMEDOUT == iRet)
    {
        iRfIdRequest &= ~0X1;
    
        sp_thread_mutex_unlock( &Ipc4Rfid.mutex );
        return -1;
    }

    iRfIdRequest &= ~0X1;

    sp_thread_mutex_unlock( &Ipc4Rfid.mutex );

    return 0;
}

int CCB::CcbGetRfidCont(int iIndex,int offset,int len,unsigned char *pucData)
{
    if (iIndex >= MAX_RFREADER_NUM)
    {
        return 0;
    }
    if (aRfReader[iIndex].bit1RfContValid)
    {
        memcpy(pucData,&aRfReader[iIndex].aucRfCont[offset],len);

        return len;
    }

    return 0;
}


int CCB::CcbWriteRfid(int iIndex,int iTime,int offset,int len,unsigned char *pucData)
{
    int iRet;

    struct timeval now;
    struct timespec outtime;
    
    sp_thread_mutex_lock( &Ipc4Rfid.mutex );

    {
        char buf[64];
        IAP_CAN_CMD_STRU *pCmd = (IAP_CAN_CMD_STRU *)buf;
        
        APP_PACKET_RF_STRU *pHo = (APP_PACKET_RF_STRU *)pCmd->data;
        
        pCmd->iCanChl     = APP_CAN_CHL_OUTER;
        pCmd->ucCmd       = SAPP_CMD_DATA;
        pCmd->iPayLoadLen = APP_PROTOL_HEADER_LEN;
        pCmd->ulCanId     = iIndex ;
        
        pHo->hdr.ucLen     = 1 + 2;
        pHo->hdr.ucTransId = 0XF0;
        pHo->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHo->hdr.ucMsgType = APP_PACKET_RF_OPERATION;
        
        pHo->ucOpsType     = APP_PACKET_RF_WRITE;
        
        {
            APP_PACKET_RF_WRITE_REQ_STRU *pWr = (APP_PACKET_RF_WRITE_REQ_STRU *)pHo->aucData;
        
            pWr->ucLen = len;
            pWr->ucOff = offset;
            
            pHo->hdr.ucLen += pWr->ucLen;
            
            memcpy(pWr->aucData,pucData,len);
            
        }   
        pCmd->iPayLoadLen += pHo->hdr.ucLen;  
        
        iRet = DispAfRfEntry(pCmd);      

        if (0 != iRet)
        {
            sp_thread_mutex_unlock( &Ipc4Rfid.mutex );
            
            return -1;
        }
    }

    iRfIdRequest = 0X2;
    
    gettimeofday(&now, NULL);
    outtime.tv_sec  = now.tv_sec + iTime/1000;
    outtime.tv_nsec = (now.tv_usec + (iTime  % 1000)*1000)* 1000;

    iRet = sp_thread_cond_timedwait( &Ipc4Rfid.cond, &Ipc4Rfid.mutex ,&outtime);//ylf: thread sleep here
    if(ETIMEDOUT == iRet)
    {
        iRfIdRequest &= ~0X2;
    
        sp_thread_mutex_unlock( &Ipc4Rfid.mutex );
        return -1;
    }

    iRfIdRequest &= ~0X2;

    sp_thread_mutex_unlock( &Ipc4Rfid.mutex );

    return 0;
}


int CCB::CcbScanRfid(int iIndex,int iTime)
{
    int iRet;

    struct timeval now;
    struct timespec outtime;
    
    sp_thread_mutex_lock( &Ipc4Rfid.mutex );

    {
        char buf[64];
        IAP_CAN_CMD_STRU *pCmd = (IAP_CAN_CMD_STRU *)buf;
    
        APP_PACKET_RF_STRU *pHo = (APP_PACKET_RF_STRU *)pCmd->data;

        pCmd->iCanChl     = APP_CAN_CHL_OUTER;
        pCmd->ucCmd       = SAPP_CMD_DATA;
        pCmd->iPayLoadLen = APP_PROTOL_HEADER_LEN;
        pCmd->ulCanId     = iIndex ;
    
        pHo->hdr.ucLen     = 1 + 0;
        pHo->hdr.ucTransId = 0XF0;
        pHo->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHo->hdr.ucMsgType = APP_PACKET_RF_OPERATION;
        pHo->ucOpsType     = APP_PACKET_RF_SEARCH;

        pCmd->iPayLoadLen += pHo->hdr.ucLen;  
        
        iRet = DispAfRfEntry(pCmd);    

        if (0 != iRet)
        {
            sp_thread_mutex_unlock( &Ipc4Rfid.mutex );
            
            return -1;
        }
    }

    iRfIdRequest = 0X4;
    
    gettimeofday(&now, NULL);
    outtime.tv_sec  = now.tv_sec + iTime/1000;
    outtime.tv_nsec = (now.tv_usec + (iTime  % 1000)*1000)* 1000;

    iRet = sp_thread_cond_timedwait( &Ipc4Rfid.cond, &Ipc4Rfid.mutex ,&outtime);//ylf: thread sleep here
    if(ETIMEDOUT == iRet)
    {
        iRfIdRequest &= ~0X4;
    
        sp_thread_mutex_unlock( &Ipc4Rfid.mutex );
        return -1;
    }

    iRfIdRequest &= ~0X4;

    sp_thread_mutex_unlock( &Ipc4Rfid.mutex );

    return 0;
}

void CCB::CanCcbAfDevQueryMsg(int mask)
{
    if (iDevRequest & mask)
    {
        sp_thread_mutex_lock  ( &m_Ipc4DevMgr.mutex );
        sp_thread_cond_signal ( &m_Ipc4DevMgr.cond  );
        sp_thread_mutex_unlock( &m_Ipc4DevMgr.mutex );
    }
    
}

int CCB::CcbQueryDeviceVersion(int iTime, int addr)
{
    int iRet;

    struct timeval now;
    struct timespec outtime;
    
    sp_thread_mutex_lock( &m_Ipc4DevMgr.mutex );

    {
        IAP_CAN_CMD_STRU Cmd;
        if(2 == addr)
        {
            Cmd.iCanChl = APP_CAN_CHL_INNER;
        }
        else
        {
            Cmd.iCanChl = APP_CAN_CHL_OUTER;
        }
        
        Cmd.ucCmd       = SBL_QUERY_VERSION_CMD;
        Cmd.iPayLoadLen = 0;
        Cmd.ulCanId     = addr;
        iRet = DispIapEntry(&Cmd);
        if (0 != iRet)
        {
            sp_thread_mutex_unlock( &m_Ipc4DevMgr.mutex );
            return -1;
        }
    }

    iDevRequest = 0X2;
    
    gettimeofday(&now, NULL);
    outtime.tv_sec  = now.tv_sec + iTime/1000;
    outtime.tv_nsec = (now.tv_usec + (iTime  % 1000)*1000)* 1000;

    iRet = sp_thread_cond_timedwait( &m_Ipc4DevMgr.cond, &m_Ipc4DevMgr.mutex ,&outtime);//ylf: thread sleep here
    if(ETIMEDOUT == iRet)
    {
        iDevRequest &= ~0X2;
        sp_thread_mutex_unlock( &m_Ipc4DevMgr.mutex );
        return -1;
    }
    
    iDevRequest &= ~0X2;
    sp_thread_mutex_unlock( &m_Ipc4DevMgr.mutex );

    return 0;

}

int CCB::CcbQueryDevice(int iTime)
{
    int iRet;

    struct timeval now;
    struct timespec outtime;
    
    sp_thread_mutex_lock( &m_Ipc4DevMgr.mutex );

    {
        IAP_CAN_CMD_STRU Cmd;
        
        Cmd.iCanChl     = APP_CAN_CHL_INNER;
        Cmd.ucCmd       = SBL_QUERY_ID_CMD;
        Cmd.iPayLoadLen = 0;
        Cmd.ulCanId     = 0x3ff;
        iRet = DispIapEntry(&Cmd);
        if (0 != iRet)
        {
            sp_thread_mutex_unlock( &m_Ipc4DevMgr.mutex );
            
            return -1;
        }

        Cmd.iCanChl     = APP_CAN_CHL_OUTER;
        Cmd.ucCmd       = SBL_QUERY_ID_CMD;
        Cmd.iPayLoadLen = 0;
        Cmd.ulCanId     = 0x3ff;
        iRet = DispIapEntry(&Cmd);
        if (0 != iRet)
        {
            sp_thread_mutex_unlock( &m_Ipc4DevMgr.mutex );
            
            return -1;
        }
    }

    iDevRequest = 0X1;
    gettimeofday(&now, NULL);
    outtime.tv_sec  = now.tv_sec + iTime/1000;
    outtime.tv_nsec = (now.tv_usec + (iTime  % 1000)*1000)* 1000;

    iRet = sp_thread_cond_timedwait( &m_Ipc4DevMgr.cond, &m_Ipc4DevMgr.mutex ,&outtime);//ylf: thread sleep here
    if(ETIMEDOUT == iRet)
    {
        iDevRequest &= ~0X1;
        sp_thread_mutex_unlock( &m_Ipc4DevMgr.mutex );
        return -1;
    }
    
    iDevRequest &= ~0X1;
    sp_thread_mutex_unlock( &m_Ipc4DevMgr.mutex );

    return 0;
}


int CCB::CcbSetRPump(int id,int iChl,unsigned int ulValue)
{
   return CcbSetExeHoldRegs(id,APP_EXE_HOLD_REG_RPUMP_OFFSET + iChl,ulValue);
}

int CCB::DispSetRPump(int iChl,unsigned int ulValue)
{
    if (0 == (ulActiveMask & (1 << APP_DEV_TYPE_EXE_BOARD)))
    {
        return -1;
    }

   return CcbSetRPump(WORK_LIST_NUM,iChl,ulValue);
}

void CCB::DispSetTocState(int iStage)
{        char buf[32];
    
    APP_PACKET_EXE_STRU *pExe = (APP_PACKET_EXE_STRU *)buf;

    APP_PACKET_EXE_TOC_REQ_STRU *pReq = (APP_PACKET_EXE_TOC_REQ_STRU *)pExe->aucData;

    unsigned int ulIdentifier ;

    LOG(VOS_LOG_DEBUG,"Enter %s:%d",__FUNCTION__,iStage);  

    pExe->hdr.ucLen         = 1;
    pExe->hdr.ucTransId     = APP_DEV_TYPE_MAIN_CTRL;
    pExe->hdr.ucDevType     = APP_DEV_TYPE_MAIN_CTRL;
    pExe->hdr.ucMsgType     = APP_PACKET_EXE_OPERATION;

    pExe->ucOpsType         = APP_PACKET_EXE_TOC;

    pReq->ucStage           = iStage;
    pExe->hdr.ucLen        += sizeof(APP_PACKET_EXE_TOC_REQ_STRU);

    CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_DEV_TYPE_MAIN_CTRL,APP_DEV_TYPE_EXE_BOARD);

    CcbSndAfCanCmd(APP_CAN_CHL_INNER,ulIdentifier,SAPP_CMD_DATA,(unsigned char *)buf,APP_PROTOL_HEADER_LEN + pExe->hdr.ucLen );
}


void CCB::CcbStopQtw()
{
    int iLoop;

    LOG(VOS_LOG_WARNING,"CcbStopQtw");    
    
    if (!SearchWork(work_stop_qtw))
    {
        for (iLoop = 0; iLoop < MAX_HANDLER_NUM; iLoop++)
        {
            if (aHandler[iLoop].bit1Qtw)
            {
                CcbInnerWorkStopQtw(iLoop);
            }
        }
    }    

}

int CCB::CcbGetOtherKeyState(int iSelfKey)
{
    return (ExeBrd.ucDinState & (~(1<<iSelfKey)));
}

int CCB::CanCcbAfDataClientRpt4FlowMeter(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_CLIENT_RPT_IND_STRU *pmg = (APP_PACKET_CLIENT_RPT_IND_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0];

    switch(pmg->ucRptType)
    {
    case APP_PACKET_RPT_FM:
        {
            int iPackets = (pmg->hdr.ucLen - 1) / sizeof (APP_PACKET_RPT_FM_STRU);
            APP_PACKET_RPT_FM_STRU *pFM = (APP_PACKET_RPT_FM_STRU *)pmg->aucData;
            int iLoop;
            int iTmp = 0;
            
            for (iLoop = 0; iLoop < iPackets; iLoop++,pFM++ )
            {
                if (pFM->ucId < APP_FM_FLOW_METER_NUM)
                {
                    FlowMeter.aFmObjs[pFM->ucId].Value.ulV = pFM->ulValue;
                    FlowMeter.ulFmValidFlags |= 1 << pFM->ucId;

                    if (INVALID_FM_VALUE == FlowMeter.aHist[pFM->ucId].lstValue.ulV)
                    {
                        FlowMeter.aHist[pFM->ucId].lstValue.ulV = FlowMeter.aHist[pFM->ucId].curValue.ulV ;
                    }

                    FlowMeter.aHist[pFM->ucId].curValue.ulV = pFM->ulValue;

                    iTmp |= 1 << iLoop;

                    if (INVALID_FM_VALUE == FlowMeter.aulPwStart[pFM->ucId])
                    {
                        FlowMeter.aulPwStart[pFM->ucId] = pFM->ulValue;
                    }
                    
                }
            }
            CcbFmNotify();

            if (iTmp & (1 << APP_FM_FM1_NO))
            {
                if (DISP_WORK_STATE_RUN == curWorkState.iMainWorkState4Pw
                    && DISP_WORK_SUB_RUN_QTW == curWorkState.iSubWorkState4Pw)
                {
                    QTW_MEAS_STRU *pQtw = &QtwMeas;
                    
                    if (INVALID_FM_VALUE == pQtw->ulBgnFm )
                    {
                        pQtw->ulBgnFm = FlowMeter.aFmObjs[APP_FM_FM1_NO].Value.ulV;
                    }

                    pQtw->ulCurFm = FlowMeter.aFmObjs[APP_FM_FM1_NO].Value.ulV;

                    if (iCurTwIdx < MAX_HANDLER_NUM)
                    {
                        pQtw = &aHandler[iCurTwIdx].QtwMeas; 
                        
                        if (INVALID_FM_VALUE == pQtw->ulBgnFm )
                        {
                            pQtw->ulBgnFm = FlowMeter.aFmObjs[APP_FM_FM1_NO].Value.ulV;
                        }
                        
                        pQtw->ulCurFm = FlowMeter.aFmObjs[APP_FM_FM1_NO].Value.ulV;
                    }

#if 0
                    if (CcbConvert2Fm1Data(QtwMeas.ulCurFm - QtwMeas.ulBgnFm) >= QtwMeas.ulTotalFm
                        && QtwMeas.ulTotalFm != INVALID_FM_VALUE)
                    {
                        /* enough water has been taken */
                        CcbStopQtw();
                    }
                    CcbRealTimeQTwVolumnNotify(CcbConvert2Fm1Data(QtwMeas.ulCurFm - QtwMeas.ulBgnFm));
#endif
                }
            }
        }
        break;
    default:
        break;
    }

    return 0;
}


int CCB::CanCcbAfDataClientRptMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{

    APP_PACKET_CLIENT_RPT_IND_STRU *pmg = (APP_PACKET_CLIENT_RPT_IND_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0];

    int iSrcAdr = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    /* message validation  */
    switch(pmg->hdr.ucDevType)
    {
    case APP_DEV_TYPE_EXE_BOARD:
    case APP_DEV_TYPE_FLOW_METER:
        if (iSrcAdr != pmg->hdr.ucDevType
            && iSrcAdr != 0XFF)
        {
            return -1;
        }
        break;
    case APP_DEV_TYPE_RF_READER:
        if (iSrcAdr < APP_RF_READER_BEGIN_ADDRESS
            || iSrcAdr > APP_RF_READER_END_ADDRESS )
        {
            return -1;
        }
        break;        
    default:
        LOG(VOS_LOG_WARNING,"addr%d(chl%d-long%x): unknow dev type %x\r\n",iSrcAdr,pCanItfMsg->iCanChl,pCanItfMsg->ulCanId,pmg->hdr.ucDevType);
        return -1;
    }

    switch(pmg->hdr.ucDevType)
    {
    case APP_DEV_TYPE_EXE_BOARD:
        return CanCcbAfDataClientRpt4ExeBoard(pCanItfMsg);
    case APP_DEV_TYPE_FLOW_METER:
        return CanCcbAfDataClientRpt4FlowMeter(pCanItfMsg);
    case APP_DEV_TYPE_RF_READER:
        return CanCcbAfDataClientRpt4RFReader(pCanItfMsg);
    default:
        break;
    }

    return 0;
}

int CCB::CanCcbAfDataHOCirReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    APP_PACKET_HO_CIR_REQ_STRU *pCirReq = (APP_PACKET_HO_CIR_REQ_STRU *)pmg->aucData;

    uint8_t ucResult = APP_PACKET_HO_ERROR_CODE_UNSUPPORT;

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);

    LOG(VOS_LOG_WARNING,"Hdl%d: Enter %s Act%d",ucIndex,__FUNCTION__,pCirReq->ucAction);    

    if (ucIndex >=  MAX_HANDLER_NUM)
    {
        ucResult = APP_PACKET_HO_ERROR_CODE_UNKNOW;
        goto end;
    }

    if(CIR_TYPE_HP == pCirReq->ucHandleType)
    {
        if (!haveHPCir())
        {
            ucResult = APP_PACKET_HO_ERROR_CODE_UNKNOW;
            goto end;
        }
    }

    switch(pCirReq->ucAction)
    {
    case APP_PACKET_HO_ACTION_START:
        {
            if (DISP_WORK_STATE_RUN == curWorkState.iMainWorkState4Pw
                && DISP_WORK_SUB_IDLE == curWorkState.iSubWorkState4Pw)
            {
                if (!SearchWork(work_start_cir))
                {
                    CcbInnerWorkStartCir(pCirReq->ucHandleType);
                }
                ucResult = APP_PACKET_HO_ERROR_CODE_SUCC;
            }
        }
        break;
    default:
        if (DISP_WORK_STATE_RUN == curWorkState.iMainWorkState4Pw
            && DISP_WORK_SUB_RUN_CIR == curWorkState.iSubWorkState4Pw)
        {
           if (!SearchWork(work_stop_cir))
           {
               CcbInnerWorkStopCir();
           }   
           ucResult = APP_PACKET_HO_ERROR_CODE_SUCC;
        }
        break;
    }

end:
    {
        /* send response */
        unsigned char buf[32];

        unsigned int ulCanId;

        int iPayLoad;

        APP_PACKET_HO_STRU *pHoRsp = (APP_PACKET_HO_STRU *)buf;
        APP_PACKET_HO_CIR_RSP_STRU *pCirRsp = (APP_PACKET_HO_CIR_RSP_STRU *)pHoRsp->aucData;

        pHoRsp->hdr = pmg->hdr;

        pHoRsp->hdr.ucLen     = sizeof(APP_PACKET_HO_CIR_RSP_STRU) + 1; // 1 for ops type
        pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHoRsp->hdr.ucMsgType |= 0x80;
        pHoRsp->ucOpsType     = pmg->ucOpsType;

        pCirRsp->ucAction     = pCirReq->ucAction;
        pCirRsp->ucResult     = ucResult;

        iPayLoad = APP_POROTOL_PACKET_HO_COMMON_LENGTH + sizeof(APP_PACKET_HO_CIR_RSP_STRU);

        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,0x1,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));

        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);

    }
    return 0;
}

int CCB::CanCcbAfDataHOCirRspMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    /* do nothing here */
    pCanItfMsg = pCanItfMsg;
    return 0;
}

void CCB::CanCcbSaveSpeedMsg(int iTrxIndex,void *pMsg,int iSpeed)
{
    //DrawSpeed(aHandler[iIndex].iDevType,aHandler[iIndex].iSpeed);
    switch(iTrxIndex)
    {
    case APP_TRX_CAN:
        {
            MAIN_CANITF_MSG_STRU *pCanItfMsg = (MAIN_CANITF_MSG_STRU *)pMsg;
            
            APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 
            
            APP_PACKET_HO_SPEED_REQ_STRU *pReq = (APP_PACKET_HO_SPEED_REQ_STRU *)pmg->aucData;
            
            int iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);
            int iIndex   = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);
            
            aHandler[iIndex].CommHdr = pmg->hdr;
            aHandler[iIndex].ulCanId = pCanItfMsg->ulCanId;
            aHandler[iIndex].iCanIdx = pCanItfMsg->iCanChl;
            
            aHandler[iIndex].iAction = pReq->ucAction;  
            aHandler[iIndex].iSpeed = iSpeed;
            
            aHandler[iIndex].iCurTrxIndex = iTrxIndex;

        }
        break;
    default:
                
        break;
    }    
}

void CCB::CanCcbSndHoSpeedRspMsg(int iIndex,int iCode)
{
    /* send response */
    unsigned char buf[32];

    unsigned int ulCanId;
    int iPayLoad;

    APP_PACKET_HO_STRU *pHoRsp          = (APP_PACKET_HO_STRU *)buf;
    APP_PACKET_HO_SPEED_RSP_STRU  *pRsp = (APP_PACKET_HO_SPEED_RSP_STRU *)pHoRsp->aucData;

    pHoRsp->hdr           = aHandler[iIndex].CommHdr;
    pHoRsp->hdr.ucLen     = sizeof(APP_PACKET_HO_QTW_RSP_STRU) + 1; // 1 for ops type
    pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
    pHoRsp->hdr.ucMsgType |= 0x80;
    pHoRsp->ucOpsType     = APP_PACKET_HO_SPEED;

    pRsp->ucAction     = aHandler[iIndex].iAction;
    pRsp->ucResult     = iCode;
    pRsp->ucSpeed      = aHandler[iIndex].iSpeed;
    pRsp->ucType       = aHandler[iIndex].iDevType;

    iPayLoad = APP_POROTOL_PACKET_HO_COMMON_LENGTH + sizeof(APP_PACKET_HO_SPEED_RSP_STRU);

    if (APP_TRX_CAN == aHandler[iIndex].iCurTrxIndex)
    {
         CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_DEV_TYPE_MAIN_CTRL,CAN_SRC_ADDRESS(aHandler[iIndex].ulCanId));
         
         CcbSndAfCanCmd(aHandler[iIndex].iCanIdx,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);

         LOG(VOS_LOG_WARNING,"%s:%x",__FUNCTION__,ulCanId);
    }


}

void CCB::DispSndHoSpeed(int iType,int iSpeed)
{
    /* send response */
    unsigned char buf[32];

    unsigned int ulCanId;
             int iPayLoad;

    APP_PACKET_HO_STRU *pHoRsp          = (APP_PACKET_HO_STRU *)buf;
    APP_PACKET_HO_SPEED_RSP_STRU  *pRsp = (APP_PACKET_HO_SPEED_RSP_STRU *)pHoRsp->aucData;

    pHoRsp->hdr.ucTransId = 0XFE;
    pHoRsp->hdr.ucLen     = sizeof(APP_PACKET_HO_SPEED_RSP_STRU) + 1; // 1 for ops type
    pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
    pHoRsp->hdr.ucMsgType = APP_PACKET_HAND_OPERATION | 0x80;
    pHoRsp->ucOpsType     = APP_PACKET_HO_SPEED;

    pRsp->ucAction     = APP_PACKET_HO_ACTION_START;
    pRsp->ucResult     = ERROR_CODE_SUCC;
    pRsp->ucSpeed      = iSpeed;
    pRsp->ucType       = iType;

    iPayLoad = APP_POROTOL_PACKET_HO_COMMON_LENGTH + sizeof(APP_PACKET_HO_SPEED_RSP_STRU);

    CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_BROAD_CAST_ADDRESS);

    CcbSndAfCanCmd(APP_CAN_CHL_OUTER,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);

}

void CCB::DispSndHoQtwVolumn(int iType,float fValue)
{
    /* send response */
    unsigned char buf[32];

    unsigned int ulCanId;
             int iPayLoad;

    APP_PACKET_HO_STRU *pHoReq          = (APP_PACKET_HO_STRU *)buf;
    APP_PACKET_HO_QTW_REQ_STRU  *pReq   = (APP_PACKET_HO_QTW_REQ_STRU *)pHoReq->aucData;

    pHoReq->hdr.ucTransId = 0XFE;
    pHoReq->hdr.ucLen     = sizeof(APP_PACKET_HO_QTW_REQ_STRU) + 1; // 1 for ops type
    pHoReq->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
    pHoReq->hdr.ucMsgType = APP_PACKET_HAND_OPERATION | 0x80;
    pHoReq->ucOpsType     = APP_PACKET_HO_QTW_VOLUMN;

    pReq->ucAction     = iType;
    pReq->ulVolumn     = fValue;

    iPayLoad = APP_POROTOL_PACKET_HO_COMMON_LENGTH + sizeof(APP_PACKET_HO_QTW_REQ_STRU);

    CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_BROAD_CAST_ADDRESS);

    CcbSndAfCanCmd(APP_CAN_CHL_OUTER,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);


}

void CCB::DispSndHoAlarm(int iIndex,int iCode)
{

    /* send response */
    unsigned char buf[32];

    unsigned int ulCanId;
    int          iPayLoad;

    APP_PACKET_HO_STRU *pHoState = (APP_PACKET_HO_STRU *)buf;

    APP_PACKET_HO_ALARM_STATE_NOT_STRU *pLoad = (APP_PACKET_HO_ALARM_STATE_NOT_STRU *)pHoState->aucData;

    pHoState->hdr.ucLen      = 1 + sizeof(APP_PACKET_HO_ALARM_STATE_NOT_STRU) ; // 1 for ops type
    pHoState->hdr.ucDevType  = APP_DEV_TYPE_MAIN_CTRL;
    pHoState->hdr.ucMsgType  = APP_PACKET_HAND_OPERATION;
    pHoState->ucOpsType      = APP_PACKET_HO_ALARM_STATE_NOT;

    pLoad->ucMask            = iCode;

    iPayLoad = APP_POROTOL_PACKET_HO_COMMON_LENGTH +  sizeof(APP_PACKET_HO_ALARM_STATE_NOT_STRU) ;

    if (iIndex == APP_PROTOL_CANID_BROADCAST)
    {
        /* broadcast */
        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_BROAD_CAST_ADDRESS);
    }
    else
    {
        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,(iIndex + APP_HAND_SET_BEGIN_ADDRESS));
    
    }
    CcbSndAfCanCmd(APP_CAN_CHL_OUTER,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);


}

void CCB::DispSndHoPpbAndTankLevel(int iIndex,int iMask,int iLevel,float fPpb)
{
    /* send response */
    unsigned char buf[32];

    unsigned int ulCanId;
    int          iPayLoad;

    APP_PACKET_HO_STRU *pHoState     = (APP_PACKET_HO_STRU *)buf;
    APP_PACKET_HO_QL_UPD_STRU *pLoad  = (APP_PACKET_HO_QL_UPD_STRU *)pHoState->aucData;

    pHoState->hdr.ucLen      = 1 + sizeof(APP_PACKET_HO_QL_UPD_STRU) ; // 1 for ops type
    pHoState->hdr.ucDevType  = APP_DEV_TYPE_MAIN_CTRL;
    pHoState->hdr.ucMsgType  = APP_PACKET_HAND_OPERATION;
    pHoState->ucOpsType      = APP_PACKET_HO_QL_UPDATE;

    pLoad->ucMask            = iMask;
    pLoad->ucLevel           = iLevel;
    pLoad->fWaterQppb        = fPpb;

    iPayLoad = APP_POROTOL_PACKET_HO_COMMON_LENGTH +  sizeof(APP_PACKET_HO_QL_UPD_STRU) ;

    if (iIndex == APP_PROTOL_CANID_BROADCAST)
    {
        /* broadcast */
        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_BROAD_CAST_ADDRESS);
    }
    else
    {
        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,(iIndex + APP_HAND_SET_BEGIN_ADDRESS));
    }
    CcbSndAfCanCmd(APP_CAN_CHL_OUTER,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);


}

void CCB::DispSndHoSystemTest(int iIndex,int iAction,int iOpreatType)
{
    /* send response */
    unsigned char buf[32];

    unsigned int ulCanId;
    int          iPayLoad;

    APP_PACKET_HO_STRU *pHoState     = (APP_PACKET_HO_STRU *)buf;
    APP_PACKET_HO_SYSTEMTEST_REQ_STRU *pLoad = (APP_PACKET_HO_SYSTEMTEST_REQ_STRU *)pHoState->aucData;

    pHoState->hdr.ucLen      = 1 + sizeof(APP_PACKET_HO_SYSTEMTEST_REQ_STRU) ; // 1 for ops type
    pHoState->hdr.ucDevType  = APP_DEV_TYPE_MAIN_CTRL;
    pHoState->hdr.ucMsgType  = APP_PACKET_HAND_OPERATION;
    pHoState->ucOpsType      = APP_PACKET_HO_SYSTEM_TEST;

    pLoad->ucAction          = iAction;
    pLoad->ucType            = iOpreatType;

    iPayLoad = APP_POROTOL_PACKET_HO_COMMON_LENGTH +  sizeof(APP_PACKET_HO_SYSTEMTEST_REQ_STRU) ;

    LOG(VOS_LOG_DEBUG,"DispSndHoSystemTest %d&%d\r\n",iAction,iOpreatType);

    if (iIndex == APP_PROTOL_CANID_BROADCAST)
    {
        /* broadcast */
        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_BROAD_CAST_ADDRESS);
    }
    else
    {
        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,(iIndex + APP_HAND_SET_BEGIN_ADDRESS));
    }

    if (APP_PACKET_HO_SYS_TEST_TYPE_DEPRESSURE == iOpreatType)
    {
        CcbNotDecPressure(aHandler[iIndex].iDevType ,iAction);
    }
    
    CcbSndAfCanCmd(APP_CAN_CHL_OUTER,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);

}

void CCB::DispSndHoEco(int iMask)
{
    unsigned char buf[32];

    unsigned int ulCanId;
    int          iPayLoad;

    APP_PACKET_HO_STRU *pHoMsg = (APP_PACKET_HO_STRU *)buf;

    APP_PACKET_RPT_ECO_STRU *pEco = (APP_PACKET_RPT_ECO_STRU *)pHoMsg->aucData;

    pHoMsg->hdr.ucLen      = 1 ; // 1 for ops type
    pHoMsg->hdr.ucDevType  = APP_DEV_TYPE_MAIN_CTRL;
    pHoMsg->hdr.ucMsgType  = APP_PACKET_HAND_OPERATION;
    pHoMsg->ucOpsType      = APP_PACKET_HO_WQ_NOTIFY;

    if (iMask & (1 << APP_EXE_I4_NO))
    {
        pEco->ucId         = APP_EXE_I4_NO;
        //ex
        switch(gGlobalParam.iMachineType)
        {
        case MACHINE_EDI:
            if(haveHPCir())
            {
                pEco->ev = ExeBrd.aEcoObjs[APP_EXE_I4_NO].Value.eV;
            }
            else
            {
                pEco->ev = ExeBrd.aEcoObjs[APP_EXE_I3_NO].Value.eV;
            }
            break;
        case MACHINE_UP:
        case MACHINE_RO:
        case MACHINE_RO_H:
        case MACHINE_C:
            if(haveHPCir())
            {
                pEco->ev = ExeBrd.aEcoObjs[APP_EXE_I3_NO].Value.eV;
            }
            else
            {
                pEco->ucId         = APP_EXE_I2_NO;
                pEco->ev = ExeBrd.aEcoObjs[APP_EXE_I2_NO].Value.eV;
            }
            break;
        default:
            pEco->ev = ExeBrd.aEcoObjs[APP_EXE_I4_NO].Value.eV;
            break;
        }
        //end

        //pEco->ev           = ExeBrd.aEcoObjs[APP_EXE_I4_NO].Value.eV;
        pHoMsg->hdr.ucLen += sizeof(APP_PACKET_RPT_ECO_STRU);
        pEco++;
    }

    if (iMask & (1 << APP_EXE_I5_NO))
    {
        pEco->ucId         = APP_EXE_I5_NO;
        pEco->ev           = ExeBrd.aEcoObjs[APP_EXE_I5_NO].Value.eV;
        pHoMsg->hdr.ucLen += sizeof(APP_PACKET_RPT_ECO_STRU);
        pEco++;
    }

    iPayLoad = APP_PROTOL_HEADER_LEN +  pHoMsg->hdr.ucLen ;

    CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_DEV_TYPE_MAIN_CTRL,CAN_BROAD_CAST_ADDRESS);
    CcbSndAfCanCmd(APP_CAN_CHL_OUTER,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);

}


int CCB::CanCcbAfDataHOSpeedReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    APP_PACKET_HO_SPEED_REQ_STRU *pSpeedReq = (APP_PACKET_HO_SPEED_REQ_STRU *)pmg->aucData;

    uint8_t ucResult = APP_PACKET_HO_ERROR_CODE_UNSUPPORT;

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);

    int     iSpeed   = PUMP_SPEED_00;
        //PUMP_SPEED_NUM;

    int     iActive  = 0;

    int     iType = APP_DEV_HS_SUB_NUM;
    

    LOG(VOS_LOG_WARNING,"Hdl%d: Enter %s Act%d",ucIndex,__FUNCTION__,pSpeedReq->ucAction);    

    if (ucIndex >=  MAX_HANDLER_NUM)
    {
        ucResult = APP_PACKET_HO_ERROR_CODE_UNKNOW;
        goto end;
    }

    iType = aHandler[ucIndex].iDevType;

    DispGetRPumpSwitchState(APP_EXE_C2_NO - APP_EXE_C1_NO,&iActive);

    iSpeed = DispConvertRPumpSpeed2Scale(ExeBrd.ausHoldRegister[APP_EXE_HOLD_REG_RPUMP_OFFSET + APP_EXE_C2_NO - APP_EXE_C1_NO]);

    switch(pSpeedReq->ucAction)
    {
    case APP_PACKET_HO_ACTION_START:
        {
            if (iSpeed >= PUMP_SPEED_10)
            {
                ucResult = APP_PACKET_HO_ERROR_CODE_SUCC;
                goto end;
            } 

            iSpeed += 1; 

            CanCcbSaveSpeedMsg(APP_TRX_CAN,pCanItfMsg,iSpeed);

            if (!SearchWork(work_start_speed_regulation))
            {
                CcbInnerWorkStartSpeedRegulation(ucIndex);
            }
            
            return 0;
            
        }
        break;
    default:
        {
            if (iSpeed <= PUMP_SPEED_00)
            {
                ucResult = APP_PACKET_HO_ERROR_CODE_SUCC;
                goto end;
            } 

            iSpeed -= 1; 
            
            CanCcbSaveSpeedMsg(APP_TRX_CAN,pCanItfMsg,iSpeed);
            
            if (!SearchWork(work_start_speed_regulation))
            {
                CcbInnerWorkStartSpeedRegulation(ucIndex);
            }

            return 0;
        }
        break;
    }

end:
    {
        /* send response */
        unsigned char buf[32];

        unsigned int ulCanId;

        int iPayLoad;

        APP_PACKET_HO_STRU           *pHoRsp  = (APP_PACKET_HO_STRU *)buf;
        APP_PACKET_HO_SPEED_RSP_STRU *pSpeedRsp = (APP_PACKET_HO_SPEED_RSP_STRU *)pHoRsp->aucData;

        pHoRsp->hdr = pmg->hdr;

        pHoRsp->hdr.ucLen     = sizeof(APP_PACKET_HO_SPEED_RSP_STRU) + 1; // 1 for ops type
        pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHoRsp->hdr.ucMsgType |= 0x80;
        pHoRsp->ucOpsType     = pmg->ucOpsType;

        pSpeedRsp->ucAction     = pSpeedReq->ucAction;
        pSpeedRsp->ucResult     = ucResult;
        pSpeedRsp->ucSpeed      = iSpeed;
        pSpeedRsp->ucType       = iType;

        iPayLoad = APP_POROTOL_PACKET_HO_COMMON_LENGTH + sizeof(APP_PACKET_HO_SPEED_RSP_STRU);

        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,0x1,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));

        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);

    }
    return 0;
}

int CCB::CanCcbAfDataHODecPressureReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    APP_PACKET_HO_SYSTEMTEST_REQ_STRU *pDecPreReq = (APP_PACKET_HO_SYSTEMTEST_REQ_STRU *)pmg->aucData;

    uint8_t ucResult = APP_PACKET_HO_ERROR_CODE_UNSUPPORT;

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);

    int     iType = APP_DEV_HS_SUB_NUM;
    

    LOG(VOS_LOG_WARNING,"Hdl%d: Enter %s Act%d",ucIndex,__FUNCTION__,pDecPreReq->ucAction);    

    if (ucIndex >=  MAX_HANDLER_NUM)
    {
         return -1;
    }

    iType = aHandler[ucIndex].iDevType;

    if (DISP_WORK_STATE_IDLE == curWorkState.iMainWorkState4Pw)
    {
        unsigned char buf[32];
    
        unsigned int ulCanId;
        int          iPayLoad;
    
        APP_PACKET_HO_STRU *pHoState             = (APP_PACKET_HO_STRU *)buf;
        APP_PACKET_HO_SYSTEMTEST_RSP_STRU *pLoad = (APP_PACKET_HO_SYSTEMTEST_RSP_STRU *)pHoState->aucData;

        CcbNotDecPressure(iType,pDecPreReq->ucAction);
    
        pHoState->hdr.ucLen      = 1 + sizeof(APP_PACKET_HO_SYSTEMTEST_RSP_STRU) ; // 1 for ops type
        pHoState->hdr.ucDevType  = APP_DEV_TYPE_MAIN_CTRL;
        pHoState->hdr.ucMsgType  = APP_PACKET_HAND_OPERATION;
        pHoState->ucOpsType      = pmg->ucOpsType;
    
        pLoad->ucAction          = pDecPreReq->ucAction;
        pLoad->ucType            = pDecPreReq->ucType;
        pLoad->ucResult          = ucResult;
    
        iPayLoad = APP_POROTOL_PACKET_HO_COMMON_LENGTH +  sizeof(APP_PACKET_HO_SYSTEMTEST_RSP_STRU) ;
    
        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,0x1,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));
        
        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);
    }
    return 0;
}

int CCB::CanCcbAfDataHOSpeedRspMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    /* do nothing here */
    pCanItfMsg = pCanItfMsg;
    return 0;
}

void CCB::CanCcbSndMainCtrlState(int iIndex , int iState)
{
    /* send response */
    unsigned char buf[32];

    unsigned int ulCanId;
    int          iPayLoad;

    APP_PACKET_HO_STRU *pHoState = (APP_PACKET_HO_STRU *)buf;

    APP_PACKET_HO_STATE_STRU *pLoad = (APP_PACKET_HO_STATE_STRU *)pHoState->aucData;


    pHoState->hdr.ucLen      = 1 + sizeof(APP_PACKET_HO_STATE_STRU) ; // 1 for ops type
    pHoState->hdr.ucDevType  = APP_DEV_TYPE_MAIN_CTRL;
    pHoState->hdr.ucMsgType  = APP_PACKET_HAND_OPERATION;
    pHoState->ucOpsType      = APP_PACKET_HO_EXT_MAIN_CTRL_STA;

    pLoad->ucState           = iState;
    pLoad->ucResult          = APP_PACKET_HO_ERROR_CODE_SUCC;

    pLoad->ucAddData     = 0;
    pLoad->ucResult      =  APP_PACKET_HO_ERROR_CODE_SUCC;

    iPayLoad = APP_POROTOL_PACKET_HO_COMMON_LENGTH +  sizeof(APP_PACKET_HO_STATE_STRU) ;

    if (iIndex == APP_PROTOL_CANID_BROADCAST)
    {
        /* broadcast */
        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_DEV_TYPE_MAIN_CTRL,CAN_BROAD_CAST_ADDRESS);
    }
    else
    {
        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_DEV_TYPE_MAIN_CTRL,(iIndex + APP_HAND_SET_BEGIN_ADDRESS));
    
    }
    CcbSndAfCanCmd(APP_CAN_CHL_OUTER,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);
}


void CCB::CanCcbSndHOState(int iIndex , int iState)
{

    /* send response */
    unsigned char buf[32];

    unsigned int ulCanId;
    int          iPayLoad;

    APP_PACKET_HO_STRU *pHoState = (APP_PACKET_HO_STRU *)buf;

    APP_PACKET_HO_STATE_STRU *pLoad = (APP_PACKET_HO_STATE_STRU *)pHoState->aucData;

    LOG(VOS_LOG_WARNING,"Enter %s:%d:%d",__FUNCTION__,iIndex,iState);    


    pHoState->hdr.ucLen      = 1 + sizeof(APP_PACKET_HO_STATE_STRU) ; // 1 for ops type
    pHoState->hdr.ucDevType  = APP_DEV_TYPE_MAIN_CTRL;
    pHoState->hdr.ucMsgType  = APP_PACKET_HAND_OPERATION;
    pHoState->ucOpsType      = APP_PACKET_HO_STA;

    pLoad->ucState           = iState;
    pLoad->ucResult          = APP_PACKET_HO_ERROR_CODE_SUCC;

    pLoad->ucAlarmState      = getAlarmState();  //ex
    
    switch(iState)
    {
    case APP_PACKET_HS_STATE_QTW:
        pLoad->ucAddData    = CcbGetTwFlag();
        break;
    case APP_PACKET_HS_STATE_CIR:
        pLoad->ucAddData    = iCirType;
        break;
    default :
        pLoad->ucAddData    = 0;
        break;
    }
    
    if (bit1B2Empty)
    {
        pLoad->ucResult      =  APP_PACKET_HO_ERROR_CODE_TANK_EMPTY;
    }

    pLoad->ucMainCtrlState = CanCcbGetMainCtrlState();
    
    iPayLoad = APP_POROTOL_PACKET_HO_COMMON_LENGTH +  sizeof(APP_PACKET_HO_STATE_STRU) ;

    if (iIndex == APP_PROTOL_CANID_BROADCAST)
    {
        /* broadcast */
        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_DEV_TYPE_MAIN_CTRL,CAN_BROAD_CAST_ADDRESS);
    }
    else
    {
        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_DEV_TYPE_MAIN_CTRL,(iIndex + APP_HAND_SET_BEGIN_ADDRESS));
    
    }
    CcbSndAfCanCmd(APP_CAN_CHL_OUTER,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);


}


void CCB::CanCcbSaveQtwMsg(int iTrxIndex, void *pMsg)
{
   switch(iTrxIndex)
    {
    case APP_TRX_CAN:
        {
            MAIN_CANITF_MSG_STRU *pCanItfMsg = (MAIN_CANITF_MSG_STRU *)pMsg;
            
            APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 
            
            APP_PACKET_HO_QTW_REQ_STRU *pQtwReq = (APP_PACKET_HO_QTW_REQ_STRU *)pmg->aucData;
            
            int iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);
            int iIndex   = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);
            
            aHandler[iIndex].CommHdr = pmg->hdr;
            aHandler[iIndex].ulCanId = pCanItfMsg->ulCanId;
            aHandler[iIndex].iCanIdx = pCanItfMsg->iCanChl;
            if(INVALID_FM_VALUE != pQtwReq->ulVolumn)
            {
                QtwMeas.ulTotalFm        = pQtwReq->ulVolumn - gGlobalParam.Caliparam.pc[DISP_PC_COFF_S1].fc;
            }
            else
            {
                QtwMeas.ulTotalFm        = pQtwReq->ulVolumn;
            }

            aHandler[iIndex].iCurTrxIndex = iTrxIndex;
        }
        break;
    default:
        {
        }        
        break;
    }
    
}

void CCB::CanCcbFillQtwRsp(APP_PACKET_HO_QTW_RSP_STRU *pQtwRsp,int iAct,int iCode)
{
    pQtwRsp->ucAction     = iAct;
    pQtwRsp->ucResult     = iCode;
    pQtwRsp->ucUnit       = gGlobalParam.MiscParam.iUint4Conductivity;
    pQtwRsp->usPulseNum   = gGlobalParam.FmParam.aulCfg[0];
    pQtwRsp->ulVolumn     = QtwMeas.ulTotalFm;
    
    LOG(VOS_LOG_WARNING,"%s:%d",__FUNCTION__,QtwMeas.ulTotalFm);
}

void CCB::CanCcbSndQtwRspMsg(int iIndex,int iCode)
{
    /* send response */
    unsigned char buf[32];

    unsigned int ulCanId;
    int iPayLoad;

    APP_PACKET_HO_STRU *pHoRsp          = (APP_PACKET_HO_STRU *)buf;
    APP_PACKET_HO_QTW_RSP_STRU *pQtwRsp = (APP_PACKET_HO_QTW_RSP_STRU *)pHoRsp->aucData;

    pHoRsp->hdr           = aHandler[iIndex].CommHdr;
    pHoRsp->hdr.ucLen     = sizeof(APP_PACKET_HO_QTW_RSP_STRU) + 1; // 1 for ops type
    pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
    pHoRsp->hdr.ucMsgType |= 0x80;
    pHoRsp->ucOpsType     = APP_PACKET_HO_QTW;

    CanCcbFillQtwRsp(pQtwRsp,APP_PACKET_HO_ACTION_START,iCode);

    pQtwRsp->ucType = aHandler[iIndex].iDevType;

    if (APP_TRX_CAN == aHandler[iIndex].iCurTrxIndex)
    {
        iPayLoad = APP_POROTOL_PACKET_HO_COMMON_LENGTH + sizeof(APP_PACKET_HO_QTW_RSP_STRU);
        
        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_DEV_TYPE_MAIN_CTRL,CAN_SRC_ADDRESS(aHandler[iIndex].ulCanId));
        
        CcbSndAfCanCmd(aHandler[iIndex].iCanIdx,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);

        LOG(VOS_LOG_WARNING,"%s:%x",__FUNCTION__,ulCanId);
    }
    else
    {
    }    


}

int CCB::CanCcbAfDataHOQtwReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{

    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    APP_PACKET_HO_QTW_REQ_STRU *pQtwReq = (APP_PACKET_HO_QTW_REQ_STRU *)pmg->aucData;

    uint8_t ucResult = APP_PACKET_HO_ERROR_CODE_SUCC;

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);
    uint8_t ucIndex  = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);
    int     iType = APP_DEV_HS_SUB_NUM;

    LOG(VOS_LOG_WARNING,"Hdl%d: Enter %s Act%d Vol%d",ucIndex,__FUNCTION__,pQtwReq->ucAction,pQtwReq->ulVolumn);    

    qDebug() << "dcj: index: " << ucIndex << " Action: " << pQtwReq->ucAction;
    if (ucIndex >=  MAX_HANDLER_NUM)
    {
        ucResult = APP_PACKET_HO_ERROR_CODE_UNKNOW;
        goto end;
    }
    
    iType = aHandler[ucIndex].iDevType;

    switch(pQtwReq->ucAction)
    {
    case APP_PACKET_HO_ACTION_START:
        {
            /* late implement */
            if (DISP_WORK_STATE_RUN != curWorkState.iMainWorkState4Pw)
            {
                ucResult = APP_PACKET_HO_ERROR_CODE_UNREADY; // failed
            }
            else
            {
               /* setup work */
               if (CcbGetTwFlag())
               {
                    if (!aHandler[ucIndex].bit1Qtw)
                    {
                        ucResult = APP_PACKET_HO_ERROR_CODE_UNSUPPORT; // current only support one tw
                    }
                    else
                    {
                        ucResult = APP_PACKET_HO_ERROR_CODE_SUCC;
                        CcbInnerWorkStopQtw(ucIndex);
                        pQtwReq->ucAction = APP_PACKET_HO_ACTION_STOP;
                    }
               }
               else if(CcbGetTwPendingFlag())
               {
                   if(!(CcbGetTwPendingFlag() & (1 << ucIndex)))
                   {
                       ucResult = APP_PACKET_HO_ERROR_CODE_UNSUPPORT; // current only support one tw
                   }
               }
               else
               {
                   CanCcbSaveQtwMsg(APP_TRX_CAN,pCanItfMsg);

                   if (DISP_WORK_STATE_RUN == curWorkState.iMainWorkState4Pw
                        && DISP_WORK_SUB_RUN_CIR == curWorkState.iSubWorkState4Pw)
                    {
                        if (!SearchWork(work_stop_cir))
                        {
                        	CcbInnerWorkStopCir_ext(1);
                        } 
                    }
                   /* remove cir work if any */
                   CcbRmvWork(work_start_cir);
                    
                   /* construct work */
                   CcbInnerWorkStartQtw(ucIndex);

                   return 0;
               }
            }
        }
        break;
    case APP_PACKET_HO_ACTION_STOP:
        {
            /* setup work */
            if (!aHandler[ucIndex].bit1Qtw)
            {
                ucResult = APP_PACKET_HO_ERROR_CODE_SUCC; //  

                /* 2018/01/24 add for adpater because of its prolonged tw procedure */
                {
                    DISPHANDLE hdl ;
                    
                    hdl = SearchWork(work_start_qtw);
                    
                    if (hdl)
                    {
                        CcbCancelWork(hdl);
                    }
                }
                
            }
            else
            {
                CcbInnerWorkStopQtw(ucIndex);
            }
        }
        break;
    default:
        ucResult = APP_PACKET_HO_ERROR_CODE_UNKNOW; // failed
        break;
    }

end:
    {
        /* send response */
        unsigned char buf[32];

        unsigned int ulCanId;
        int iPayLoad;

        APP_PACKET_HO_STRU *pHoRsp = (APP_PACKET_HO_STRU *)buf;
        APP_PACKET_HO_QTW_RSP_STRU *pQtwRsp = (APP_PACKET_HO_QTW_RSP_STRU *)pHoRsp->aucData;

        pHoRsp->hdr           = pmg->hdr;
        pHoRsp->hdr.ucLen     = sizeof(APP_PACKET_HO_QTW_RSP_STRU) + 1; // 1 for ops type
        pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHoRsp->hdr.ucMsgType |= 0x80;
        pHoRsp->ucOpsType     = pmg->ucOpsType;

        CanCcbFillQtwRsp(pQtwRsp,pQtwReq->ucAction,ucResult);
        pQtwRsp->ucType       = iType;
        
        iPayLoad = APP_POROTOL_PACKET_HO_COMMON_LENGTH + sizeof(APP_PACKET_HO_QTW_RSP_STRU);

        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));

        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);

    }

    return 0;
}


int CCB::CanCcbAfDataHOQtwRspMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{

    pCanItfMsg = pCanItfMsg;

    /* do nothing here */

    return 0;
}

int CCB::CanCcbAfDataHODecPressureRspMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{

    pCanItfMsg = pCanItfMsg;

    /* do nothing here */

    return 0;
}


void CCB::CanCcbIapProc(int iTrxIndex,void *msg)
{
   /* Notify Ui */
   IAP_NOTIFY_STRU *pNotify = (IAP_NOTIFY_STRU *)gaucIapBuffer;

   switch(iTrxIndex)
   {
   case APP_TRX_CAN:
       {
           MAIN_CANITF_MSG_STRU *pCanItfMsg = (MAIN_CANITF_MSG_STRU *)msg;
           pNotify->ulCanId = pCanItfMsg->ulCanId;
           pNotify->iCanChl = pCanItfMsg->iCanChl;
           pNotify->iMsgLen = pCanItfMsg->iMsgLen;

           memcpy(pNotify->data,pCanItfMsg->aucData,pNotify->iMsgLen);  

       }
       break;
   default:
       
       break;
   }

   pNotify->iTrxIndex = iTrxIndex;

   emitIapNotify(pNotify);

}

void CCB::CanCcbSndAccessoryInstallStartMsg(int iAccType,const QString& strCatNo,const QString& strLotNo,int id)
{
    if (-1 != m_iInstallHdlIdx)
    {
       unsigned char buf[128];
       
       unsigned int ulCanId;
       
       APP_PACKET_HO_STRU *pHoState = (APP_PACKET_HO_STRU *)buf;
       
       APP_PACKET_HO_ACCESSORY_INSTALL_START_STRU *pLoad = (APP_PACKET_HO_ACCESSORY_INSTALL_START_STRU *)pHoState->aucData;
       
       pHoState->hdr.ucLen      = 1 + sizeof(APP_PACKET_HO_ACCESSORY_INSTALL_START_STRU) - 1; // 1 for ops type
       pHoState->hdr.ucDevType  = APP_DEV_TYPE_MAIN_CTRL;
       pHoState->hdr.ucMsgType  = APP_PACKET_HAND_OPERATION;
       pHoState->ucOpsType      = APP_PACKET_HO_EXT_ACCESSORY_INSTALL_START;
       pHoState->hdr.ucTransId  = id;
       
       pLoad->ucInit            = gAdditionalCfgParam.initMachine;
       pLoad->ucAccType         = iAccType;
       
       uint8_t  *pMsg = pLoad->aucData;

       {
           {
               APP_PACKET_HO_SYS_INFO_ITEM_STRU *pItem = (APP_PACKET_HO_SYS_INFO_ITEM_STRU *)pMsg;
               
               pItem->ucLen = strCatNo.toLocal8Bit().length();
               memcpy(pItem->aucData,strCatNo.toLocal8Bit().data(),pItem->ucLen);
               
               pMsg += sizeof(APP_PACKET_HO_SYS_INFO_ITEM_STRU) - 1 + pItem->ucLen;
           }

           {
               APP_PACKET_HO_SYS_INFO_ITEM_STRU *pItem = (APP_PACKET_HO_SYS_INFO_ITEM_STRU *)pMsg;
               
               pItem->ucLen = strLotNo.toLocal8Bit().length();
               memcpy(pItem->aucData,strLotNo.toLocal8Bit().data(),pItem->ucLen);
               
               pMsg += sizeof(APP_PACKET_HO_SYS_INFO_ITEM_STRU) - 1 + pItem->ucLen;
           }   
           
           // append terminator
           {
               APP_PACKET_HO_SYS_INFO_ITEM_STRU *pItem = (APP_PACKET_HO_SYS_INFO_ITEM_STRU *)pMsg;
               pItem->ucLen = 0;
               pMsg += sizeof(APP_PACKET_HO_SYS_INFO_ITEM_STRU) - 1;
           }            
       }

       pHoState->hdr.ucLen += pMsg - pLoad->aucData;
       
       CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,(m_iInstallHdlIdx + APP_HAND_SET_BEGIN_ADDRESS));
       
       CcbSndAfCanCmd(APP_CAN_CHL_OUTER,ulCanId,SAPP_CMD_DATA,buf,APP_PROTOL_HEADER_LEN + pHoState->hdr.ucLen);
    }
}


void CCB::CanCcbSndAccessoryInstallFinishMsg(int iAccType,int id)
{
    if (-1 != m_iInstallHdlIdx)
    {
       unsigned char buf[128];
       
       unsigned int ulCanId;
       int          iPayLoad;
       
       APP_PACKET_HO_STRU *pHoState = (APP_PACKET_HO_STRU *)buf;
       
       APP_PACKET_HO_ACCESSORY_INSTALL_FINISH_STRU *pLoad = (APP_PACKET_HO_ACCESSORY_INSTALL_FINISH_STRU *)pHoState->aucData;
       
       pHoState->hdr.ucLen      = 1 + sizeof(APP_PACKET_HO_ACCESSORY_INSTALL_FINISH_STRU);
       pHoState->hdr.ucDevType  = APP_DEV_TYPE_MAIN_CTRL;
       pHoState->hdr.ucMsgType  = APP_PACKET_HAND_OPERATION;
       pHoState->ucOpsType      = APP_PACKET_HO_EXT_ACCESSORY_INSTALL_FINISH;
       pHoState->hdr.ucTransId  = id;
       
       pLoad->ucInit            = gAdditionalCfgParam.initMachine;
       pLoad->ucAccType         = iAccType;
       
       iPayLoad = APP_PROTOL_HEADER_LEN + pHoState->hdr.ucLen;
       
       CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,(m_iInstallHdlIdx + APP_HAND_SET_BEGIN_ADDRESS));
       
       CcbSndAfCanCmd(APP_CAN_CHL_OUTER,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);
    }
}


int CCB::CanCcbAfDataHOExtSysStartReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    APP_PACKET_HO_SYS_START_REQ_STRU *pHoSubReq = (APP_PACKET_HO_SYS_START_REQ_STRU *)pmg->aucData;

    uint8_t ucResult = APP_PACKET_HO_ERROR_CODE_UNREADY;

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);

    LOG(VOS_LOG_WARNING,"Hdl%d:Enter %s Act%d",ucIndex,__FUNCTION__,pHoSubReq->ucAction);    


    if (ucIndex >=  MAX_HANDLER_NUM)
    {
        ucResult = APP_PACKET_HO_ERROR_CODE_UNKNOW;
        goto end;
    }
    {
        DISPHANDLE hdl;
        
        switch(pHoSubReq->ucAction)
        {
        case APP_PACKET_HO_ACTION_START:
            {
                hdl = DispCmdEntry(DISP_CMD_RUN,NULL,0);
            }
            break;
        default:
            {
                hdl = DispCmdEntry(DISP_CMD_HALT,NULL,0);
            }
            break;
        }
        if (hdl)
        {
            ucResult = APP_PACKET_HO_ERROR_CODE_SUCC;
        }
    }
end:
    {
        /* send response */
        unsigned char buf[32];

        unsigned int ulCanId;

        int iPayLoad;

        APP_PACKET_HO_STRU *pHoRsp = (APP_PACKET_HO_STRU *)buf;
        APP_PACKET_HO_SYS_START_RSP_STRU *pCirRsp = (APP_PACKET_HO_SYS_START_RSP_STRU *)pHoRsp->aucData;

        pHoRsp->hdr = pmg->hdr;

        pHoRsp->hdr.ucLen     = sizeof(APP_PACKET_HO_SYS_START_RSP_STRU) + 1; // 1 for ops type
        pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHoRsp->hdr.ucMsgType |= 0x80;
        pHoRsp->ucOpsType     = pmg->ucOpsType;

        pCirRsp->ucAction     = pHoSubReq->ucAction;
        pCirRsp->ucResult     = ucResult;

        iPayLoad = APP_POROTOL_PACKET_HO_COMMON_LENGTH + sizeof(APP_PACKET_HO_SYS_START_RSP_STRU);

        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));

        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);

    }
    return 0;
}

int CCB::CanCcbAfDataHOExtSterlizeReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    APP_PACKET_HO_STERLIZE_REQ_STRU *pHoSubReq = (APP_PACKET_HO_STERLIZE_REQ_STRU *)pmg->aucData;

    uint8_t ucResult = APP_PACKET_HO_ERROR_CODE_UNREADY;

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);

    LOG(VOS_LOG_WARNING,"Hdl%d:Enter %s Act%d",ucIndex,__FUNCTION__,pHoSubReq->ucAction);    


    if (ucIndex >=  MAX_HANDLER_NUM)
    {
        ucResult = APP_PACKET_HO_ERROR_CODE_UNKNOW;
        goto end;
    }
    {
        DISPHANDLE hdl;
        DISP_CMD_WASH_STRU WashCmd;

        WashCmd.iState = pHoSubReq->ucAction;
        WashCmd.iType  = pHoSubReq->ucType;
        
        hdl = DispCmdEntry(DISP_CMD_WASH,(unsigned char *)&WashCmd,sizeof(DISP_CMD_WASH_STRU));
        if (hdl)
        {
            ucResult = APP_PACKET_HO_ERROR_CODE_SUCC;

            if (WashCmd.iState)
            {
                gGlobalParam.CleanParam.aCleans[pHoSubReq->ucType].lstTime = QDateTime::currentDateTime().toTime_t();
                gGlobalParam.CleanParam.aCleans[pHoSubReq->ucType].period = QDateTime::currentDateTime().addDays(gGlobalParam.CMParam.aulCms[DISP_ROC12LIFEDAY]).toTime_t();
                
                MainSaveCleanParam(gGlobalParam.iMachineType, gGlobalParam.CleanParam);
            }
            
        }
    }
end:
    {
        /* send response */
        unsigned char buf[32];

        unsigned int ulCanId;

        int iPayLoad;

        APP_PACKET_HO_STRU *pHoRsp = (APP_PACKET_HO_STRU *)buf;
        APP_PACKET_HO_STERLIZE_RSP_STRU *pCirRsp = (APP_PACKET_HO_STERLIZE_RSP_STRU *)pHoRsp->aucData;

        pHoRsp->hdr = pmg->hdr;

        pHoRsp->hdr.ucLen     = sizeof(APP_PACKET_HO_STERLIZE_RSP_STRU) + 1; // 1 for ops type
        pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHoRsp->hdr.ucMsgType |= 0x80;
        pHoRsp->ucOpsType     = pmg->ucOpsType;

        pCirRsp->ucAction     = pHoSubReq->ucAction;
        pCirRsp->ucResult     = ucResult;

        iPayLoad = APP_PROTOL_HEADER_LEN + pHoRsp->hdr.ucLen;

        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));

        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);

    }
    return 0;
}


int CCB::CanCcbAfDataHOExtRtiReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    APP_PACKET_HO_RTI_QUERY_REQ_STRU *pHoSubReq = (APP_PACKET_HO_RTI_QUERY_REQ_STRU *)pmg->aucData;

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);


    if (ucIndex >=  MAX_HANDLER_NUM)
    {
        return -1;
    }
    
    {
        /* send response: 
           if payload exceeds 196 bytes,then split
        */
        unsigned char buf[196];  

        unsigned int ulCanId;

        int iPayLoad;

        APP_PACKET_HO_STRU *pHoRsp = (APP_PACKET_HO_STRU *)buf;
        APP_PACKET_HO_RTI_QUERY_RSP_STRU *pSubRsp = (APP_PACKET_HO_RTI_QUERY_RSP_STRU *)pHoRsp->aucData;

        pHoRsp->hdr           = pmg->hdr;

        pHoRsp->hdr.ucLen     = 1 + sizeof(APP_PACKET_HO_RTI_QUERY_RSP_STRU) - 1; // 1 for ops type
        pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHoRsp->hdr.ucMsgType |= 0x80;
        pHoRsp->ucOpsType     = pmg->ucOpsType;

        // fill all kinds of real time info here
        {
            HANDLER_STRU *pHdl = &aHandler[ucIndex];
            uint32_t ulMap = 0;

            LOG(VOS_LOG_DEBUG,"Hdl%d: First%d,DevType%d",ucIndex,pHoSubReq->ucFirst,pHoSubReq->ucDevType);    
            if (pHoSubReq->ucFirst)
            {
                //memset(pHdl->m_historyInfo,0,sizeof(struct Show_History_Info) * MSG_NUM);
                int iLoop;
                for (iLoop = 0; iLoop < MSG_NUM; iLoop++)
                {
                    pHdl->m_historyInfo[iLoop].value1 = 0;
                    pHdl->m_historyInfo[iLoop].value2 = 0;
                }
            }
            pHdl->iDevType = pHoSubReq->ucDevType;

            uint8_t *pMsg = pSubRsp->aucData;
            bool bFirst   = (pHoSubReq->ucFirst != 0);

            switch(m_aWaterType[pHdl->iDevType])
            {
            case WATER_TYPE_UP:
                {
                    if (m_historyInfo[UP_Resis].value1 != pHdl->m_historyInfo[UP_Resis].value1
                        || m_historyInfo[UP_Resis].value2 != pHdl->m_historyInfo[UP_Resis].value2
                        || bFirst)
                    {
                        pHdl->m_historyInfo[UP_Resis] = m_historyInfo[UP_Resis];
                    
                        ulMap |= 1 << PARAM_DO_UP;

                        APP_ECO_VALUE_STRU value;
                        
                        value.fWaterQ = m_historyInfo[UP_Resis].value1;
                        value.usTemp  = m_historyInfo[UP_Resis].value2 * APP_TEMPERATURE_SCALE;

                        memcpy(pMsg, &value,sizeof(APP_ECO_VALUE_STRU));
                        
                        pMsg += sizeof(APP_ECO_VALUE_STRU);
                    }
                }
                break;
            case WATER_TYPE_HP_RO:
                {
                    if (m_historyInfo[RO_Product].value1 != pHdl->m_historyInfo[RO_Product].value1
                        || m_historyInfo[RO_Product].value2 != pHdl->m_historyInfo[RO_Product].value2
                        || bFirst)
                    {
                        pHdl->m_historyInfo[RO_Product] = m_historyInfo[RO_Product];
                    
                        ulMap |= 1 << PARAM_DO_RO_Output;

                        APP_ECO_VALUE_STRU value;
                        
                        value.fWaterQ = m_historyInfo[RO_Product].value1;
                        value.usTemp  = m_historyInfo[RO_Product].value2 * APP_TEMPERATURE_SCALE;

                        memcpy(pMsg, &value,sizeof(APP_ECO_VALUE_STRU));
                        pMsg += sizeof(APP_ECO_VALUE_STRU);
                    }   
                    
                    if (m_historyInfo[RO_Feed_Cond].value1 != pHdl->m_historyInfo[RO_Feed_Cond].value1
                        || m_historyInfo[RO_Feed_Cond].value2 != pHdl->m_historyInfo[RO_Feed_Cond].value2
                        || bFirst)
                    {
                        pHdl->m_historyInfo[RO_Feed_Cond] = m_historyInfo[RO_Feed_Cond];
                    
                        ulMap |= 1 << PARAM_DO_RO_Input;

                        APP_ECO_VALUE_STRU value;
                        
                        value.fWaterQ = m_historyInfo[RO_Feed_Cond].value1;
                        value.usTemp  = m_historyInfo[RO_Feed_Cond].value2 * APP_TEMPERATURE_SCALE;

                        memcpy(pMsg, &value,sizeof(APP_ECO_VALUE_STRU));
                        pMsg += sizeof(APP_ECO_VALUE_STRU);
                    }
                    
                    if (m_historyInfo[Tap_Cond].value1 != pHdl->m_historyInfo[Tap_Cond].value1
                        || m_historyInfo[Tap_Cond].value2 != pHdl->m_historyInfo[Tap_Cond].value2
                        || bFirst)
                    {
                        pHdl->m_historyInfo[Tap_Cond] = m_historyInfo[Tap_Cond];
                    
                        ulMap |= 1 << PARAM_DO_Tap_Water;

                        APP_ECO_VALUE_STRU value;
                        
                        value.fWaterQ = m_historyInfo[Tap_Cond].value1;
                        value.usTemp  = m_historyInfo[Tap_Cond].value2 * APP_TEMPERATURE_SCALE;

                        memcpy(pMsg, &value,sizeof(APP_ECO_VALUE_STRU));
                        pMsg += sizeof(APP_ECO_VALUE_STRU);
                    }   
                    
                    if (m_historyInfo[RO_Feed_Pressure].value1 != pHdl->m_historyInfo[RO_Feed_Pressure].value1
                        || bFirst)
                    {
                        pHdl->m_historyInfo[RO_Feed_Pressure].value1  = m_historyInfo[RO_Feed_Pressure].value1 ;
                    
                        ulMap |= 1 << PARAM_DO_Tap_Water_Pressure;
                    
                    
                        memcpy(pMsg, &m_historyInfo[RO_Feed_Pressure].value1,sizeof(float));
                        pMsg += 4;

                    }   
                    
                    if (m_historyInfo[RO_Pressure].value1 != pHdl->m_historyInfo[RO_Pressure].value1
                        || bFirst)
                    {
                        pHdl->m_historyInfo[RO_Pressure].value1  = m_historyInfo[RO_Pressure].value1 ;
                    
                        ulMap |= 1 << PARAM_DO_RO_Ongoing_Pressure;
                    
                    
                        memcpy(pMsg, &m_historyInfo[RO_Pressure].value1,sizeof(float));
                        pMsg += 4;

                    }       

                    if (m_historyInfo[RO_Rejection].value1 != pHdl->m_historyInfo[RO_Rejection].value1 
                        || bFirst)
                    {
                        pHdl->m_historyInfo[RO_Rejection].value1  = m_historyInfo[RO_Rejection].value1 ;
                    
                        ulMap |= 1 << PARAM_DO_RO_REJ;
                    
                    
                        memcpy(pMsg, &m_historyInfo[RO_Rejection].value1,sizeof(float));
                        pMsg += 4;
                    }  
                    
                }
                break;
            case WATER_TYPE_HP_EDI:
                {

                    if (m_historyInfo[RO_Product].value1 != pHdl->m_historyInfo[RO_Product].value1
                        || m_historyInfo[RO_Product].value2 != pHdl->m_historyInfo[RO_Product].value2
                        || bFirst)
                    {
                        pHdl->m_historyInfo[RO_Product] = m_historyInfo[RO_Product];
                    
                        ulMap |= 1 << PARAM_DO_RO_Output;

                        APP_ECO_VALUE_STRU value;
                        
                        value.fWaterQ = m_historyInfo[RO_Product].value1;
                        value.usTemp  = m_historyInfo[RO_Product].value2 * APP_TEMPERATURE_SCALE;

                        memcpy(pMsg, &value,sizeof(APP_ECO_VALUE_STRU));
                        pMsg += sizeof(APP_ECO_VALUE_STRU);
                    }   

                    if (m_historyInfo[RO_Feed_Cond].value1 != pHdl->m_historyInfo[RO_Feed_Cond].value1
                        || m_historyInfo[RO_Feed_Cond].value2 != pHdl->m_historyInfo[RO_Feed_Cond].value2
                        || bFirst)
                    {
                        pHdl->m_historyInfo[RO_Feed_Cond] = m_historyInfo[RO_Feed_Cond];
                    
                        ulMap |= 1 << PARAM_DO_RO_Input;

                        APP_ECO_VALUE_STRU value;
                        
                        value.fWaterQ = m_historyInfo[RO_Feed_Cond].value1;
                        value.usTemp  = m_historyInfo[RO_Feed_Cond].value2 * APP_TEMPERATURE_SCALE;

                        memcpy(pMsg, &value,sizeof(APP_ECO_VALUE_STRU));
                        pMsg += sizeof(APP_ECO_VALUE_STRU);
                    }
                                        

                    if (m_historyInfo[EDI_Product].value1 != pHdl->m_historyInfo[EDI_Product].value1
                        || m_historyInfo[EDI_Product].value2 != pHdl->m_historyInfo[EDI_Product].value2
                        || bFirst)
                    {
                        pHdl->m_historyInfo[EDI_Product] = m_historyInfo[EDI_Product];
                    
                        ulMap |= 1 << PARAM_DO_EDI_Output;

                        APP_ECO_VALUE_STRU value;
                        
                        value.fWaterQ = m_historyInfo[EDI_Product].value1;
                        value.usTemp  = m_historyInfo[EDI_Product].value2 * APP_TEMPERATURE_SCALE;

                        memcpy(pMsg, &value,sizeof(APP_ECO_VALUE_STRU));
                        pMsg += sizeof(APP_ECO_VALUE_STRU);
                    }



                    if (m_historyInfo[Tap_Cond].value1 != pHdl->m_historyInfo[Tap_Cond].value1
                        || m_historyInfo[Tap_Cond].value2 != pHdl->m_historyInfo[Tap_Cond].value2
                        || bFirst)
                    {
                        pHdl->m_historyInfo[Tap_Cond] = m_historyInfo[Tap_Cond];
                    
                        ulMap |= 1 << PARAM_DO_Tap_Water;

                        APP_ECO_VALUE_STRU value;
                        
                        value.fWaterQ = m_historyInfo[Tap_Cond].value1;
                        value.usTemp  = m_historyInfo[Tap_Cond].value2 * APP_TEMPERATURE_SCALE;

                        memcpy(pMsg, &value,sizeof(APP_ECO_VALUE_STRU));
                        pMsg += sizeof(APP_ECO_VALUE_STRU);
                    }   
                    
                    if (m_historyInfo[RO_Feed_Pressure].value1 != pHdl->m_historyInfo[RO_Feed_Pressure].value1
                        || bFirst)
                    {
                        pHdl->m_historyInfo[RO_Feed_Pressure].value1  = m_historyInfo[RO_Feed_Pressure].value1 ;
                    
                        ulMap |= 1 << PARAM_DO_Tap_Water_Pressure;
                    
                        memcpy(pMsg, &m_historyInfo[RO_Feed_Pressure].value1,sizeof(float));
                        pMsg += 4;

                    }   
                    
                    if (m_historyInfo[RO_Pressure].value1 != pHdl->m_historyInfo[RO_Pressure].value1
                        || bFirst)
                    {
                        pHdl->m_historyInfo[RO_Pressure].value1  = m_historyInfo[RO_Pressure].value1 ;
                    
                        ulMap |= 1 << PARAM_DO_RO_Ongoing_Pressure;
                    
                        memcpy(pMsg, &m_historyInfo[RO_Pressure].value1,sizeof(float));
                        pMsg += 4;
                    }  

                    if (m_historyInfo[RO_Rejection].value1 != pHdl->m_historyInfo[RO_Rejection].value1 
                        || bFirst)
                    {
                        pHdl->m_historyInfo[RO_Rejection].value1  = m_historyInfo[RO_Rejection].value1 ;
                    
                        ulMap |= 1 << PARAM_DO_RO_REJ;
                    
                        memcpy(pMsg, &m_historyInfo[RO_Rejection].value1,sizeof(float));
                        pMsg += 4;
                    }  
                    
                }
                break;
            default:
                break;
            }

            pHoRsp->hdr.ucLen += pMsg - pSubRsp->aucData;
            pSubRsp->ulMap = ulMap;
        }

        iPayLoad = APP_PROTOL_HEADER_LEN + pHoRsp->hdr.ucLen;

        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));

        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);

    }
    return 0;
}

void SetPureTankParam()
{
    switch (gGlobalParam.PmParam.aiBuckType[DISP_PM_PM2])
    {
    case DISP_WATER_BARREL_TYPE_010L:
        gGlobalParam.PmParam.afCap[DISP_PM_PM2] = 10;
        gGlobalParam.PmParam.afDepth[DISP_PM_PM2] = 0.32;
        break;
    case DISP_WATER_BARREL_TYPE_030L:
        gGlobalParam.PmParam.afCap[DISP_PM_PM2] = 30;
        gGlobalParam.PmParam.afDepth[DISP_PM_PM2] = 0.3;
        break;
    case DISP_WATER_BARREL_TYPE_060L:
        gGlobalParam.PmParam.afCap[DISP_PM_PM2] = 60;
        gGlobalParam.PmParam.afDepth[DISP_PM_PM2] = 0.6;
        break;
    case DISP_WATER_BARREL_TYPE_100L:
        gGlobalParam.PmParam.afCap[DISP_PM_PM2] = 100;
        gGlobalParam.PmParam.afDepth[DISP_PM_PM2] = 0.95;
        break;
    case DISP_WATER_BARREL_TYPE_200L:
        gGlobalParam.PmParam.afCap[DISP_PM_PM2] = 200;
        gGlobalParam.PmParam.afDepth[DISP_PM_PM2] = 0.95;
        break;
    case DISP_WATER_BARREL_TYPE_350L:
        gGlobalParam.PmParam.afCap[DISP_PM_PM2] = 350;
        gGlobalParam.PmParam.afDepth[DISP_PM_PM2] = 0.95;
        break;
    case DISP_WATER_BARREL_TYPE_UDF:
        break;
    case DISP_WATER_BARREL_TYPE_NO:
        break;
    default:
        break;
    }

}

int CCB::CanCcbAfDataHOExtConfigQryReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    APP_PACKET_HO_CONFIG_QUERY_REQ_STRU *pHoSubReq = (APP_PACKET_HO_CONFIG_QUERY_REQ_STRU *)pmg->aucData;

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);


    if (ucIndex >=  MAX_HANDLER_NUM)
    {
        return -1;
    }
    
    {
        /* send response */
        unsigned char buf[256];

        unsigned int ulCanId;

        int iPayLoad;

        APP_PACKET_HO_STRU *pHoRsp = (APP_PACKET_HO_STRU *)buf;
        APP_PACKET_HO_CONFIG_QUERY_RSP_STRU *pSubRsp = (APP_PACKET_HO_CONFIG_QUERY_RSP_STRU *)pHoRsp->aucData;

        pHoRsp->hdr = pmg->hdr;

        pHoRsp->hdr.ucLen     = 1 + sizeof(APP_PACKET_HO_CONFIG_QUERY_RSP_STRU) - 1; // 1 for ops type
        pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHoRsp->hdr.ucMsgType |= 0x80;
        pHoRsp->ucOpsType     = pmg->ucOpsType;

        pSubRsp->ulMap = pHoSubReq->ulMap;

        uint8_t *pOutMsg = pSubRsp->aucData;

        for (int iLoop = 0; iLoop < PARAM_DO_NUM; iLoop++)
        {
            if (pHoSubReq->ulMap & 0x1)
            {
                switch(iLoop)
                {
                case PARAM_DO_LAN:
                    {
                        *pOutMsg = gGlobalParam.MiscParam.iLan;
                        pOutMsg++;
                    }
                    break;
                case PARAM_DO_DATETIME:
                    {
                        time_t tTime = time(NULL);
                        
                        memcpy(pOutMsg, &tTime,sizeof(uint32_t));
                        
                        pOutMsg += 4;
                    }
                    break;  
                case PARAM_DO_WaterQuality_Unit:
                    {
                        *pOutMsg = gGlobalParam.MiscParam.iUint4Conductivity;
                        pOutMsg++;
                    }
                    break;
                case PARAM_DO_Pressure_Unit:
                    {
                        *pOutMsg = gGlobalParam.MiscParam.iUint4Pressure;
                        pOutMsg++;
                    }
                    break;  
                case PARAM_DO_Barrel_Unit:
                    {
                        *pOutMsg = gGlobalParam.MiscParam.iUint4LiquidLevel;
                        pOutMsg++;
                    }
                    break;                        
                case PARAM_DO_Temp_Unit:
                    {
                        *pOutMsg = gGlobalParam.MiscParam.iUint4Temperature;
                        pOutMsg++;
                    }
                    break;                        
                case PARAM_DO_FlowCoeff:
                    {
                        memcpy(pOutMsg, &gGlobalParam.Caliparam.pc[ DISP_PC_COFF_S1].fk,sizeof(float));
                        pOutMsg += 4;
                    }
                    break;  
                case PARAM_DO_Misc_Flags:
                    {
                        memcpy(pOutMsg, &gGlobalParam.MiscParam.ulMisFlags,sizeof(uint32_t));
                        pOutMsg += 4;
                    }
                    break;    
                case PARAM_DO_SubMod_Flags:
                    {
                        memcpy(pOutMsg, &gGlobalParam.SubModSetting.ulFlags, sizeof(uint32_t));
                        pOutMsg += 4;
                    }
                    break; 
                case PARAM_DO_WIFI_IP:
                    {
                        char ip[20];
                        char mask[20];
                        uint32_t outIp = 0;
                        
                        if (get_net_itf("wlan0",ip,mask))
                        {
                            outIp  = inet_addr(ip);
                            
                        }
                        memcpy(pOutMsg, &outIp,sizeof(uint32_t));
                        
                        pOutMsg += 4;
                    }                    
                    break;
                case PARAM_DO_ETH_IP:
                    {
                        char ip[20];
                        char mask[20];
                        uint32_t outIp = 0;
                        
                        if (get_net_itf("eth0",ip,mask))
                        {
                            outIp = inet_addr(ip);
                        }
                        memcpy(pOutMsg, &outIp,sizeof(uint32_t));
                       
                        pOutMsg += 4;
                    }                    
                    break;
                case PARAM_DO_MACH_TYPE:
                    {
                        *pOutMsg = gGlobalParam.iMachineType;
                        pOutMsg++;
                    }                    
                    break;
                case PARAM_DO_EXT_FLAGS:
                    {
                        uint32_t outFlags = gGlobalParam.MiscParam.ulExMisFlags;
#if 0
                        NIF nif;
                        
                        parseNetInterface(&nif);

                        if (!nif.dhcp)
                        {
                          outFlags |= 1 << DISP_EXT_STATIC_IP;
                        }
#endif
                        memcpy(pOutMsg, &outFlags,sizeof(uint32_t));
                       
                        pOutMsg += 4;                        
                    }
                    break;
                case PARAM_DO_REFILL_THD:
                    {
                    #if 0
                        pOutMsg[0] = CcbGetPm2Percent(CcbGetSp5());
                        pOutMsg[1] = CcbGetPm2Percent(CcbGetSp6());
                    #endif
                        pOutMsg[0] = (uint8_t)CcbGetSp5();
                        pOutMsg[1] = (uint8_t)CcbGetSp6();
                        pOutMsg += 2;
                        
                    }
                    break;
                case PARAM_DO_TANKUV_INFO:
                    {
                        pOutMsg[0] = gGlobalParam.MiscParam.iTankUvOnTime;

                        pOutMsg += 1;                        
                    }
                    break;
                case PARAM_DO_TANK_PARAM:
                    {
                        pOutMsg[0] = gGlobalParam.PmParam.aiBuckType[DISP_PM_PM2];
                        pOutMsg += 1;

                        float fRang = gSensorRange.fPureSRange * 10; //bar -> m
                        memcpy(pOutMsg, &fRang,sizeof(float));
                        pOutMsg += sizeof(float);
                        memcpy(pOutMsg, &gGlobalParam.PmParam.afCap[DISP_PM_PM2],sizeof(float));
                        pOutMsg += sizeof(float);
                        memcpy(pOutMsg, &gGlobalParam.PmParam.afDepth[DISP_PM_PM2],sizeof(float));
                        pOutMsg += sizeof(float);
                        
                    }
                    break;
                }                
            }
            pHoSubReq->ulMap >>= 1;

            if (!pHoSubReq->ulMap)
            {
                break;
            }
        }

        pHoRsp->hdr.ucLen += (pOutMsg - pSubRsp->aucData);

        iPayLoad = APP_PROTOL_HEADER_LEN + pHoRsp->hdr.ucLen;

        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));

        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);

    }
    return 0;
}

int CCB::CanCcbAfDataHOExtConfigSetReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    APP_PACKET_HO_CONFIG_SET_REQ_STRU *pSubReq = (APP_PACKET_HO_CONFIG_SET_REQ_STRU *)pmg->aucData;

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);


    if (ucIndex >=  MAX_HANDLER_NUM)
    {
        return -1;
    }
    
    {
        /* send response */

        uint8_t *pInMsg = pSubReq->aucData;

        bool bCali = false;
        bool bMm = false;
        bool bPm = false;
        bool bSensor = false;
        uint32_t ulEthIp = 0;
        uint32_t ulExtFlags = gGlobalParam.MiscParam.ulExMisFlags;

        for (int iLoop = 0; iLoop < PARAM_DO_NUM; iLoop++)
        {
            if (pSubReq->ulMap & 0x1)
            {
                switch(iLoop)
                {
                case PARAM_DO_LAN:
                    {
                        gGlobalParam.MiscParam.iLan = *pInMsg;
                        pInMsg++;
                    }
                    break;
                case PARAM_DO_DATETIME:
                    {
                        unsigned int Time_t;

                        memcpy(&Time_t,pInMsg,sizeof(uint32_t));

                        pInMsg += 4;

                        CcbSetDataTime(Time_t);                    
                    }
                    break;  
                case PARAM_DO_WaterQuality_Unit:
                    {
                        gGlobalParam.MiscParam.iUint4Conductivity = *pInMsg;
                        pInMsg++;
                    }
                    break;
                case PARAM_DO_Pressure_Unit:
                    {
                        gGlobalParam.MiscParam.iUint4Pressure= *pInMsg;
                        pInMsg++;
                    }
                    break;  
                case PARAM_DO_Barrel_Unit:
                    {
                        gGlobalParam.MiscParam.iUint4LiquidLevel= *pInMsg;
                        pInMsg++;
                    }
                    break;                        
                case PARAM_DO_Temp_Unit:
                    {
                        gGlobalParam.MiscParam.iUint4Temperature= *pInMsg;
                        pInMsg++;
                    }
                    break;                        
                case PARAM_DO_FlowCoeff:
                    {
                        float fValue ;

                        memcpy(&fValue,pInMsg,sizeof(float));
                        gGlobalParam.Caliparam.pc[ DISP_PC_COFF_S1].fk = fValue;

                        fValue += sizeof(float);

                        bCali = true;
                    }
                    break;   
                case PARAM_DO_Misc_Flags:
                    {
                        memcpy(&gGlobalParam.MiscParam.ulMisFlags, pInMsg, sizeof(uint32_t));
                        
                        pInMsg += 4;
                    }
                    break; 
                case PARAM_DO_SubMod_Flags:
                    {
                        memcpy(&gGlobalParam.SubModSetting.ulFlags, pInMsg, sizeof(uint32_t));
                        pInMsg += 4;
                    }
                    break; 
                case PARAM_DO_ETH_IP:
                    {
                        memcpy(&ulEthIp,pInMsg,sizeof(uint32_t));
                        
                        pInMsg += 4;
                    }                    
                    break;
                case PARAM_DO_WIFI_IP:
                    {
                        pInMsg += 4;
                    }
                    break;
                case PARAM_DO_EXT_FLAGS:
                    {
                        memcpy(&ulExtFlags,pInMsg,sizeof(uint32_t));
                        gGlobalParam.MiscParam.ulExMisFlags = ulExtFlags;
                        
                        
                        pInMsg += 4;                        
                    }
                    break;
                case PARAM_DO_REFILL_THD:
                    {
                        uint8_t ucRefill,ucStop;

                        ucRefill = pInMsg[0];
                        ucStop   = pInMsg[1];
                        pInMsg  += 2;
                        
                        CcbSetPm2RefillThd(ucRefill,ucStop);

                        bMm = true;
                    }
                    break;
                case PARAM_DO_TANKUV_INFO:
                    {
                        uint8_t ucTankUv = pInMsg[0];
                        pInMsg += 1;
                        gGlobalParam.MiscParam.iTankUvOnTime = ucTankUv;
                    }
                    break;
                case PARAM_DO_TANK_PARAM:
                    {
                        float fRange;
                        uint8_t tankType = pInMsg[0];
                        pInMsg += 1;
                        gGlobalParam.PmParam.aiBuckType[DISP_PM_PM2] = tankType;
                        SetPureTankParam();
                        
                        memcpy(&fRange,pInMsg,sizeof(float));
                        pInMsg  += sizeof(float);

                        gSensorRange.fPureSRange = fRange / 10; //m -> bar

                        bPm     = true;
                        bSensor = true;
                        
                    }
                    break;
                }                
            }
            pSubReq->ulMap >>= 1;

            if (!pSubReq->ulMap)
            {
                break;
            }
        }
        
        {
            unsigned char buf[256];
            unsigned int ulCanId;
            int iPayLoad;
            APP_PACKET_HO_STRU *pHoRsp = (APP_PACKET_HO_STRU *)buf;
            APP_PACKET_HO_CONFIG_SET_RSP_STRU *pSubRsp = (APP_PACKET_HO_CONFIG_SET_RSP_STRU *)pHoRsp->aucData;
    
            pHoRsp->hdr = pmg->hdr;
    
            pHoRsp->hdr.ucLen     = 1 + sizeof(APP_PACKET_HO_CONFIG_SET_RSP_STRU); // 1 for ops type
            pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
            pHoRsp->hdr.ucMsgType |= 0x80;
            pHoRsp->ucOpsType     = pmg->ucOpsType;
    
            pSubRsp->ucResult = 0;
            
    
            iPayLoad = APP_PROTOL_HEADER_LEN + pHoRsp->hdr.ucLen;
    
            CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));
    
            CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);

            // save param
            {
                DISP_SUB_MODULE_SETTING_STRU  smParam = gGlobalParam.SubModSetting;
                if(DISP_WATER_BARREL_TYPE_NO == gGlobalParam.PmParam.aiBuckType[DISP_PM_PM2])
                {
                    smParam.ulFlags &= ~(1 << DISP_SM_HaveB2);
                }
                else
                {
                    smParam.ulFlags |= (1 << DISP_SM_HaveB2);
                }
                MainSaveSubModuleSetting(gGlobalParam.iMachineType, smParam);
            }

            if (bCali)
            {
                QMap<int, DISP_PARAM_CALI_ITEM_STRU> map;
                DISP_PARAM_CALI_ITEM_STRU values;
 
                values.fk = gGlobalParam.Caliparam.pc[ DISP_PC_COFF_S1].fk;
                values.fc = 1;
                values.fv = 1;
            
                map.insert(DISP_PC_COFF_S1, values);                

                MainSaveCalibrateParam(gGlobalParam.iMachineType,map);
            }

            if (bMm)
            {
                DISP_MACHINE_PARAM_STRU  &MMParam = gGlobalParam.MMParam;  
            
                MainSaveMachineParam(gGlobalParam.iMachineType,MMParam);
            }

            if (bPm)
            {
                DISP_PM_SETTING_STRU  &PmParam = gGlobalParam.PmParam;  
            
                MainSavePMParam(gGlobalParam.iMachineType,PmParam);            
            }

            if (bSensor)
            {
                MainSaveSensorRange(gGlobalParam.iMachineType);
            }

            // change ip
            {
                NIF nif;
                char ip[20];
                char mask[20];
                uint32_t outIp = 0;
                
                if (get_net_itf("eth0",ip,mask))
                {
                    outIp = inet_addr(ip);
                }
                
                parseNetInterface(&nif);
                if (ulExtFlags & (1 << DISP_EXT_STATIC_IP))
                {
                    if (nif.dhcp || outIp != ((192<<0)|(168<<8)|(10<<16)|(100<<24)))
                    {
                        nif.dhcp = 0;
                        strcpy(nif.ip,"192.168.10.100");
                        strcpy(nif.mask,"255.255.255.0");
                        strcpy(nif.gateway,"192.168.10.1");
                        ChangeNetworkConfig(&nif);
                        gCConfig.m_bSysReboot = true;
                    }
                }
                else
                {
                    if (!nif.dhcp)
                    {
                        nif.dhcp = 1;
                        ChangeNetworkConfig(&nif);
                        gCConfig.m_bSysReboot = true;
                    }
                }

            }

            {
                DISP_MISC_SETTING_STRU  &MiscParam = gGlobalParam.MiscParam;  
                MainSaveMiscParam(gGlobalParam.iMachineType, MiscParam);
            }
        }
    }
    return 0;
}


int CCB::CanCcbAfDataHOExtSterlizeQryMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 


    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);


    if (ucIndex >=  MAX_HANDLER_NUM)
    {
        return -1;
    }
    
    /* send response */
    {
        unsigned char buf[256];
        unsigned int ulCanId;
        int iPayLoad;
        APP_PACKET_HO_STRU *pHoRsp = (APP_PACKET_HO_STRU *)buf;
        APP_PACKET_HO_STERLIZE_QUERY_RSP_STRU *pSubRsp = (APP_PACKET_HO_STERLIZE_QUERY_RSP_STRU *)pHoRsp->aucData;

        pHoRsp->hdr = pmg->hdr;

        pHoRsp->hdr.ucLen     = 1 + sizeof(APP_PACKET_HO_STERLIZE_QUERY_RSP_STRU); // 1 for ops type
        pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHoRsp->hdr.ucMsgType |= 0x80;
        pHoRsp->ucOpsType     = pmg->ucOpsType;

        if (DispGetROWashFlag())
        {
           pSubRsp->ucWashFlags = (1 << iWashType);
        }
        else
        {
           pSubRsp->ucWashFlags = 0;
        }

        pSubRsp->aulLstCleanTime[APP_CLEAN_RO] = gGlobalParam.CleanParam.aCleans[APP_CLEAN_RO].lstTime;
        pSubRsp->aulLstCleanTime[APP_CLEAN_PH] = gGlobalParam.CleanParam.aCleans[APP_CLEAN_PH].lstTime;

        pSubRsp->aulPeriod[APP_CLEAN_RO] = gGlobalParam.CleanParam.aCleans[APP_CLEAN_RO].period;
        pSubRsp->aulPeriod[APP_CLEAN_PH] = gGlobalParam.CleanParam.aCleans[APP_CLEAN_PH].period;

        iPayLoad = APP_PROTOL_HEADER_LEN + pHoRsp->hdr.ucLen;

        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));

        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);
    }

    return 0;
}

int CCB::CanCcbAfDataHOExtCmStateQryMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    APP_PACKET_HO_CM_INFO_QUERY_REQ_STRU *pSubReq = (APP_PACKET_HO_CM_INFO_QUERY_REQ_STRU *)pmg->aucData;

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);


    if (ucIndex >=  MAX_HANDLER_NUM)
    {
        return -1;
    }
    
    /* send response */
    {
        unsigned char buf[256];
        unsigned int ulCanId;
        int iPayLoad;
        APP_PACKET_HO_STRU *pHoRsp = (APP_PACKET_HO_STRU *)buf;
        APP_PACKET_HO_CM_INFO_QUERY_RSP_STRU *pSubRsp = (APP_PACKET_HO_CM_INFO_QUERY_RSP_STRU *)pHoRsp->aucData;

        pHoRsp->hdr           = pmg->hdr;

        pHoRsp->hdr.ucLen     = 1 + sizeof(APP_PACKET_HO_CM_INFO_QUERY_RSP_STRU) - 1; // 1 for ops type
        pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHoRsp->hdr.ucMsgType |= 0x80;
        pHoRsp->ucOpsType     = pmg->ucOpsType;

        // retrive information
        {
             uint32_t ulDstMap  = 0;
             
             int      iMask = m_cMas[gGlobalParam.iMachineType].aulMask[0];
             uint32_t tmp;
             uint8_t  *pMsg = pSubRsp->aucData;
         
             if (iMask & (1 << DISP_PRE_PACK))
             {
                 tmp = gCMUsage.info.aulCms[DISP_PRE_PACKLIFEDAY]; //安装日期
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_PRE_PACKLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_PRE_PACKLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gCMUsage.info.aulCms[DISP_PRE_PACKLIFEL];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_PRE_PACKLIFEL))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_PRE_PACKLIFEL);
                     pMsg += 4;
                 }
         
             }    
   
             if (iMask & (1 << DISP_AC_PACK))
             {
                 tmp = gCMUsage.info.aulCms[DISP_AC_PACKLIFEDAY];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_AC_PACKLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_AC_PACKLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gCMUsage.info.aulCms[DISP_AC_PACKLIFEL];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_AC_PACKLIFEL))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_AC_PACKLIFEL);
                     pMsg += 4;
                 }
         
             }
         
             if ((iMask & (1 << DISP_T_PACK)))
             {
                 tmp = gCMUsage.info.aulCms[DISP_T_PACKLIFEDAY];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_T_PACKLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_T_PACKLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gCMUsage.info.aulCms[DISP_T_PACKLIFEL];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_T_PACKLIFEL))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_T_PACKLIFEL);
                     pMsg += 4;
                 }                 
         
             }

             if ((iMask & (1 << DISP_P_PACK)))
             {
                 tmp = gCMUsage.info.aulCms[DISP_P_PACKLIFEDAY];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_P_PACKLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_P_PACKLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gCMUsage.info.aulCms[DISP_P_PACKLIFEL];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_P_PACKLIFEL))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_P_PACKLIFEL);
                     pMsg += 4;
                 }                  
         
             }
         
             if ((iMask & (1 << DISP_U_PACK)))
             {
                 tmp = gCMUsage.info.aulCms[DISP_U_PACKLIFEDAY];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_U_PACKLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_U_PACKLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gCMUsage.info.aulCms[DISP_U_PACKLIFEL];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_U_PACKLIFEL))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_U_PACKLIFEL);
                     pMsg += 4;
                 }                     
         
             }    

             if ((iMask & (1 << DISP_AT_PACK)))
             {
                 tmp = gCMUsage.info.aulCms[DISP_AT_PACKLIFEDAY];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_AT_PACKLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_AT_PACKLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gCMUsage.info.aulCms[DISP_AT_PACKLIFEL];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_AT_PACKLIFEL))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_AT_PACKLIFEL);
                     pMsg += 4;
                 }                     
                
             }         

             if ((iMask & (1 << DISP_H_PACK)))
             {
                 tmp = gCMUsage.info.aulCms[DISP_H_PACKLIFEDAY];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_H_PACKLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_H_PACKLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gCMUsage.info.aulCms[DISP_H_PACKLIFEL];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_H_PACKLIFEL))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_H_PACKLIFEL);
                     pMsg += 4;
                 }                        
         
             }
         
             if ((iMask & (1 << DISP_N1_UV)))
             {
                 tmp = gCMUsage.info.aulCms[DISP_N1_UVLIFEDAY];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N1_UVLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N1_UVLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gCMUsage.info.aulCms[DISP_N1_UVLIFEHOUR];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N1_UVLIFEHOUR))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N1_UVLIFEHOUR);
                     pMsg += 4;
                 }                    
         
             } 
             
             if ((iMask & (1 << DISP_N2_UV)))
             {
                 tmp = gCMUsage.info.aulCms[DISP_N2_UVLIFEDAY];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N2_UVLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N2_UVLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gCMUsage.info.aulCms[DISP_N2_UVLIFEHOUR];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N2_UVLIFEHOUR))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N2_UVLIFEHOUR);
                     pMsg += 4;
                 }                
             }        
         
             if ((iMask & (1 << DISP_N3_UV)))
             {
                 tmp = gCMUsage.info.aulCms[DISP_N3_UVLIFEDAY];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N3_UVLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N3_UVLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gCMUsage.info.aulCms[DISP_N3_UVLIFEHOUR];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N3_UVLIFEHOUR))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N3_UVLIFEHOUR);
                     pMsg += 4;
                 }                   
             }        
         
             if ((iMask & (1 << DISP_N4_UV)))
             {
                 tmp = gCMUsage.info.aulCms[DISP_N4_UVLIFEDAY];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N4_UVLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N4_UVLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gCMUsage.info.aulCms[DISP_N4_UVLIFEHOUR];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N4_UVLIFEHOUR))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N4_UVLIFEHOUR);
                     pMsg += 4;
                 }                   
         
             }        
             
             if ((iMask & (1 << DISP_N5_UV)))
             {
                 tmp = gCMUsage.info.aulCms[DISP_N5_UVLIFEDAY];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N5_UVLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N5_UVLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gCMUsage.info.aulCms[DISP_N5_UVLIFEHOUR];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N5_UVLIFEHOUR))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N5_UVLIFEHOUR);
                     pMsg += 4;
                 }            
             }        
         
             if ((iMask & (1 << DISP_W_FILTER)))
             {
                 tmp = gCMUsage.info.aulCms[DISP_W_FILTERLIFE];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_W_FILTERLIFE))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_W_FILTERLIFE);
                     pMsg += 4;
                 }
                 
             }
         
             if ((iMask & (1 << DISP_T_B_FILTER)))
             {
                 tmp = gCMUsage.info.aulCms[DISP_T_B_FILTERLIFE];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_T_B_FILTERLIFE))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_T_B_FILTERLIFE);
                     pMsg += 4;
                 }                 
             }
         
             if ((iMask & (1 << DISP_T_A_FILTER)))
             {
                 tmp = gCMUsage.info.aulCms[DISP_T_A_FILTERLIFE];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_T_A_FILTERLIFE))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_T_A_FILTERLIFE);
                     pMsg += 4;
                 }                 
                 
             }    
         
             if ((iMask & (1 << DISP_TUBE_FILTER)))
             {
                 tmp = gCMUsage.info.aulCms[DISP_TUBE_FILTERLIFE];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_TUBE_FILTERLIFE))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_TUBE_FILTERLIFE);
                     pMsg += 4;
                 }                                 
             }  
         
             if ((iMask & (1 << DISP_TUBE_DI)))
             {
                 tmp = gCMUsage.info.aulCms[DISP_TUBE_DI_LIFE];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_TUBE_DI_LIFE))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_TUBE_DI_LIFE);
                     pMsg += 4;
                 }                 
             } 
             
             pSubRsp->ulMap = ulDstMap;
             pHoRsp->hdr.ucLen += pMsg - pSubRsp->aucData;
        }
        iPayLoad = APP_PROTOL_HEADER_LEN + pHoRsp->hdr.ucLen;

        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));

        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);
    }

    return 0;
}


int CCB::CanCcbAfDataHOExtCmInfoQryMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    APP_PACKET_HO_CM_INFO_QUERY_REQ_STRU *pSubReq = (APP_PACKET_HO_CM_INFO_QUERY_REQ_STRU *)pmg->aucData;

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);


    if (ucIndex >=  MAX_HANDLER_NUM)
    {
        return -1;
    }
    
    /* send response */
    {
        unsigned char buf[256];
        unsigned int ulCanId;
        int iPayLoad;
        APP_PACKET_HO_STRU *pHoRsp = (APP_PACKET_HO_STRU *)buf;
        APP_PACKET_HO_CM_INFO_QUERY_RSP_STRU *pSubRsp = (APP_PACKET_HO_CM_INFO_QUERY_RSP_STRU *)pHoRsp->aucData;

        pHoRsp->hdr           = pmg->hdr;

        pHoRsp->hdr.ucLen     = 1 + sizeof(APP_PACKET_HO_CM_INFO_QUERY_RSP_STRU) - 1; // 1 for ops type
        pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHoRsp->hdr.ucMsgType |= 0x80;
        pHoRsp->ucOpsType     = pmg->ucOpsType;

        // retrive information
        {
             uint32_t ulDstMap  = 0;
             int      iMask = m_cMas[gGlobalParam.iMachineType].aulMask[0];
             uint32_t tmp;
             uint8_t  *pMsg = pSubRsp->aucData;
         
             if (iMask & (1 << DISP_PRE_PACK))
             {
                 tmp = gGlobalParam.CMParam.aulCms[DISP_PRE_PACKLIFEDAY] ;
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_PRE_PACKLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_PRE_PACKLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gGlobalParam.CMParam.aulCms[DISP_PRE_PACKLIFEL] ;
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_PRE_PACKLIFEL))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_PRE_PACKLIFEL);
                     pMsg += 4;
                 }
         
             }    

             if (iMask & (1 << DISP_AC_PACK))
             {
                 tmp = gGlobalParam.CMParam.aulCms[DISP_AC_PACKLIFEDAY];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_AC_PACKLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_AC_PACKLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gGlobalParam.CMParam.aulCms[DISP_AC_PACKLIFEL];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_AC_PACKLIFEL))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_AC_PACKLIFEL);
                     pMsg += 4;
                 }
         
             }
         
             //2018.10.12 T_Pack
             if ((iMask & (1 << DISP_T_PACK)))
             {
                 tmp = gGlobalParam.CMParam.aulCms[DISP_T_PACKLIFEDAY] ;
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_T_PACKLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_T_PACKLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gGlobalParam.CMParam.aulCms[DISP_T_PACKLIFEL] ;
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_T_PACKLIFEL))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_T_PACKLIFEL);
                     pMsg += 4;
                 }                 
         
             }

             if ((iMask & (1 << DISP_P_PACK)))
             {
                 tmp = gGlobalParam.CMParam.aulCms[DISP_P_PACKLIFEDAY] ;
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_P_PACKLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_P_PACKLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gGlobalParam.CMParam.aulCms[DISP_P_PACKLIFEL];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_P_PACKLIFEL))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_P_PACKLIFEL);
                     pMsg += 4;
                 }                  
         
             }
         
             if ( (iMask & (1 << DISP_U_PACK)))
             {
                 tmp = gGlobalParam.CMParam.aulCms[DISP_U_PACKLIFEDAY];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_U_PACKLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_U_PACKLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gGlobalParam.CMParam.aulCms[DISP_U_PACKLIFEL];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_U_PACKLIFEL))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_U_PACKLIFEL);
                     pMsg += 4;
                 }                     
         
             } 
             
             if ((iMask & (1 << DISP_AT_PACK)))
             {
                 tmp = gGlobalParam.CMParam.aulCms[DISP_AT_PACKLIFEDAY];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_AT_PACKLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_AT_PACKLIFEDAY);
                     pMsg += 4;
                 }
             
                 tmp = gGlobalParam.CMParam.aulCms[DISP_AT_PACKLIFEL];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_AT_PACKLIFEL))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_AT_PACKLIFEL);
                     pMsg += 4;
                 }                     
             }         

             if ((iMask & (1 << DISP_H_PACK)))
             {
                 tmp = gGlobalParam.CMParam.aulCms[DISP_H_PACKLIFEDAY] ;
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_H_PACKLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_H_PACKLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gGlobalParam.CMParam.aulCms[DISP_H_PACKLIFEL];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_H_PACKLIFEL))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_H_PACKLIFEL);
                     pMsg += 4;
                 }                        
         
             }
         
             if ((iMask & (1 << DISP_N1_UV)))
             {
                 tmp = gGlobalParam.CMParam.aulCms[DISP_N1_UVLIFEDAY];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N1_UVLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N1_UVLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gGlobalParam.CMParam.aulCms[DISP_N1_UVLIFEHOUR];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N1_UVLIFEHOUR))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N1_UVLIFEHOUR);
                     pMsg += 4;
                 }                    
         
             }        
             
             if ((iMask & (1 << DISP_N2_UV)))
             {
                 tmp = gGlobalParam.CMParam.aulCms[DISP_N2_UVLIFEDAY] ;
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N2_UVLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N2_UVLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gGlobalParam.CMParam.aulCms[DISP_N2_UVLIFEHOUR] ;
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N2_UVLIFEHOUR))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N2_UVLIFEHOUR);
                     pMsg += 4;
                 }                
             }        
         
             if ((iMask & (1 << DISP_N3_UV)))
             {
                 tmp = gGlobalParam.CMParam.aulCms[DISP_N3_UVLIFEDAY] ;
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N3_UVLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N3_UVLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gGlobalParam.CMParam.aulCms[DISP_N3_UVLIFEHOUR] ;
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N3_UVLIFEHOUR))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N3_UVLIFEHOUR);
                     pMsg += 4;
                 }                   
             }        
         
             if ((iMask & (1 << DISP_N4_UV)))
             {
                 tmp = gGlobalParam.CMParam.aulCms[DISP_N4_UVLIFEDAY] ;
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N4_UVLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N4_UVLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gGlobalParam.CMParam.aulCms[DISP_N4_UVLIFEHOUR];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N4_UVLIFEHOUR))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N4_UVLIFEHOUR);
                     pMsg += 4;
                 }                   
         
             }        
             
             if ((iMask & (1 << DISP_N5_UV)))
             {
                 tmp = gGlobalParam.CMParam.aulCms[DISP_N5_UVLIFEDAY] ;
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N5_UVLIFEDAY))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N5_UVLIFEDAY);
                     pMsg += 4;
                 }

                 tmp = gGlobalParam.CMParam.aulCms[DISP_N5_UVLIFEHOUR] ;
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_N5_UVLIFEHOUR))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_N5_UVLIFEHOUR);
                     pMsg += 4;
                 }            
             }        
         
             if ((iMask & (1 << DISP_W_FILTER)))
             {
                 tmp = gGlobalParam.CMParam.aulCms[DISP_W_FILTERLIFE];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_W_FILTERLIFE))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_W_FILTERLIFE);
                     pMsg += 4;
                 }
                 
             }
         
             if ((iMask & (1 << DISP_T_B_FILTER)))
             {
                 tmp = gGlobalParam.CMParam.aulCms[DISP_T_B_FILTERLIFE] ;
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_T_B_FILTERLIFE))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_T_B_FILTERLIFE);
                     pMsg += 4;
                 }                 
             }
         
             if ((iMask & (1 << DISP_T_A_FILTER)))
             {
                 tmp = gGlobalParam.CMParam.aulCms[DISP_T_A_FILTERLIFE] ;
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_T_A_FILTERLIFE))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_T_A_FILTERLIFE);
                     pMsg += 4;
                 }                 
                 
             }    
         
             if ((iMask & (1 << DISP_TUBE_FILTER)))
             {
                 tmp = gGlobalParam.CMParam.aulCms[DISP_TUBE_FILTERLIFE] ;
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_TUBE_FILTERLIFE))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_TUBE_FILTERLIFE);
                     pMsg += 4;
                 }                 
                                  
             }  
         
             if ( (iMask & (1 << DISP_TUBE_DI)))
             {
                 tmp = gGlobalParam.CMParam.aulCms[DISP_TUBE_DI_LIFE];
                 if (tmp < 0) tmp = 0;
                 if (pSubReq->ulMap & (1 << DISP_TUBE_DI_LIFE))
                 {
                     *(uint32_t *)pMsg = tmp;
                     ulDstMap |= (1 << DISP_TUBE_DI_LIFE);
                     pMsg += 4;
                 }                 
             } 
             
             pSubRsp->ulMap = ulDstMap;
             pHoRsp->hdr.ucLen += pMsg - pSubRsp->aucData;
        }
        iPayLoad = APP_PROTOL_HEADER_LEN + pHoRsp->hdr.ucLen;

        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));

        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);
    }

    return 0;
}

int CCB::CanCcbAfDataHOExtCmDetailInfoQryMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    APP_PACKET_HO_CM_DETAIL_QRY_REQ_STRU *pSubReq = (APP_PACKET_HO_CM_DETAIL_QRY_REQ_STRU *)pmg->aucData;

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);


    if (ucIndex >=  MAX_HANDLER_NUM)
    {
        return -1;
    }
    
    /* send response */
    if (pSubReq->ulMap < DISP_CM_NAME_NUM)
    {
        unsigned char buf[256];
        unsigned int ulCanId;
        int iPayLoad;
        APP_PACKET_HO_STRU *pHoRsp = (APP_PACKET_HO_STRU *)buf;
        APP_PACKET_HO_CM_DETAIL_QRY_RSP_STRU *pSubRsp = (APP_PACKET_HO_CM_DETAIL_QRY_RSP_STRU *)pHoRsp->aucData;

        pHoRsp->hdr           = pmg->hdr;

        pHoRsp->hdr.ucLen     = 1 + sizeof(APP_PACKET_HO_CM_DETAIL_QRY_RSP_STRU) - 1; // 1 for ops type
        pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHoRsp->hdr.ucMsgType |= 0x80;
        pHoRsp->ucOpsType     = pmg->ucOpsType;

        // retrive information
        {
            APP_PACKET_HO_CM_DETAIL_INFO_STRU *pDetailInfo = (APP_PACKET_HO_CM_DETAIL_INFO_STRU *)pSubRsp->aucData;
       
            pSubRsp->ulMap = pSubReq->ulMap;
            
            pHoRsp->hdr.ucLen += sizeof(APP_PACKET_HO_CM_DETAIL_INFO_STRU);

            strcpy(pDetailInfo->CATNO,gGlobalParam.cmSn.aCn[pSubReq->ulMap]);
            strcpy(pDetailInfo->LOTNO,gGlobalParam.cmSn.aLn[pSubReq->ulMap]);
        }
        iPayLoad = APP_PROTOL_HEADER_LEN + pHoRsp->hdr.ucLen;

        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));

        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);

        
        return 0;
    }

    return -1;
}


int CCB::CanCcbAfDataHOExtCmResetMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    APP_PACKET_HO_CM_RESET_STRU *pSubReq = (APP_PACKET_HO_CM_RESET_STRU *)pmg->aucData;

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);


    if (ucIndex >=  MAX_HANDLER_NUM)
    {
        return -1;
    }
    
    /* send response */
    {
        unsigned char buf[256];
        unsigned int ulCanId;
        int iPayLoad;
        int iLoop;
        APP_PACKET_HO_STRU *pHoRsp = (APP_PACKET_HO_STRU *)buf;
        APP_PACKET_HO_CM_INFO_QUERY_RSP_STRU *pSubRsp = (APP_PACKET_HO_CM_RESET_RSP_STRU *)pHoRsp->aucData;

        pHoRsp->hdr           = pmg->hdr;

        pHoRsp->hdr.ucLen     = 1 + sizeof(APP_PACKET_HO_CM_RESET_RSP_STRU) - 1; // 1 for ops type
        pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHoRsp->hdr.ucMsgType |= 0x80;
        pHoRsp->ucOpsType     = pmg->ucOpsType;

       {
           uint32_t ulDstMap  = 0;
           int      iMask = m_cMas[gGlobalParam.iMachineType].aulMask[0];
           uint32_t tmp;
           uint8_t  *pMsg = pSubRsp->aucData;
   
           // retrive information
           for (iLoop = 0; iLoop < DISP_CM_NAME_NUM; iLoop++)
           {
                if (!(iMask & pSubReq->ulMap & (1 << iLoop)))
                {
                    continue;
                }
                switch(iLoop)
                {
                case DISP_PRE_PACK:
                    {
                        tmp = gGlobalParam.CMParam.aulCms[DISP_PRE_PACKLIFEDAY] ;
                        if (tmp < 0) tmp = 0;
                        
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_PRE_PACKLIFEDAY);
                            pMsg += 4;
                        }
                    
                        tmp = gGlobalParam.CMParam.aulCms[DISP_PRE_PACKLIFEL] ;
                        if (tmp < 0) tmp = 0;
                        
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_PRE_PACKLIFEL);
                            pMsg += 4;
                        }
                        
                    }
                    break;
#ifdef AUTO_CM_RESET                    
                case DISP_AC_PACK:
                    {
                        tmp = gGlobalParam.CMParam.aulCms[DISP_AC_PACKLIFEDAY];
                        if (tmp < 0) tmp = 0;
                    
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_AC_PACKLIFEDAY);
                            pMsg += 4;
                        }
                    
                        tmp = gGlobalParam.CMParam.aulCms[DISP_AC_PACKLIFEL];
                        if (tmp < 0) tmp = 0;

                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_AC_PACKLIFEL);
                            pMsg += 4;
                        }
                        
                    }
                    break;
#endif                    
                case DISP_T_PACK:
                    {
                        tmp = gGlobalParam.CMParam.aulCms[DISP_T_PACKLIFEDAY] ;
                        if (tmp < 0) tmp = 0;
                    
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_T_PACKLIFEDAY);
                            pMsg += 4;
                        }
                    
                        tmp = gGlobalParam.CMParam.aulCms[DISP_T_PACKLIFEL] ;
                        if (tmp < 0) tmp = 0;

                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_T_PACKLIFEL);
                            pMsg += 4;
                        }                 
                        
                    }
                    break;
#ifdef AUTO_CM_RESET                    
                case DISP_P_PACK:
                    {
                        tmp = gGlobalParam.CMParam.aulCms[DISP_P_PACKLIFEDAY] ;
                        if (tmp < 0) tmp = 0;
                    
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_P_PACKLIFEDAY);
                            pMsg += 4;
                        }
                    
                        tmp = gGlobalParam.CMParam.aulCms[DISP_P_PACKLIFEL];
                        if (tmp < 0) tmp = 0;

                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_P_PACKLIFEL);
                            pMsg += 4;
                        }                  
                    }
                    break;
                case DISP_U_PACK:
                    {
                        tmp = gGlobalParam.CMParam.aulCms[DISP_U_PACKLIFEDAY];
                        if (tmp < 0) tmp = 0;
                    
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_U_PACKLIFEDAY);
                            pMsg += 4;
                        }
                    
                        tmp = gGlobalParam.CMParam.aulCms[DISP_U_PACKLIFEL];
                        if (tmp < 0) tmp = 0;

                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_U_PACKLIFEL);
                            pMsg += 4;
                        }                     
                    }
                    break;
                case DISP_AT_PACK:
                    {
                        tmp = gGlobalParam.CMParam.aulCms[DISP_AT_PACKLIFEDAY];
                        if (tmp < 0) tmp = 0;
                    
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_AT_PACKLIFEDAY);
                            pMsg += 4;
                        }
                    
                        tmp = gGlobalParam.CMParam.aulCms[DISP_U_PACKLIFEL];
                        if (tmp < 0) tmp = 0;
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_AT_PACKLIFEL);
                            pMsg += 4;
                        }                     
                    }
                    break;
                case DISP_H_PACK:
                    {
                        tmp = gGlobalParam.CMParam.aulCms[DISP_H_PACKLIFEDAY] ;
                        if (tmp < 0) tmp = 0;
                    
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_H_PACKLIFEDAY);
                            pMsg += 4;
                        }
                    
                        tmp = gGlobalParam.CMParam.aulCms[DISP_H_PACKLIFEL];
                        if (tmp < 0) tmp = 0;
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_H_PACKLIFEL);
                            pMsg += 4;
                        }                        
                    }
                    break;
#endif                    
                case DISP_N1_UV:
                    {
                        tmp = gGlobalParam.CMParam.aulCms[DISP_N1_UVLIFEDAY];
                        if (tmp < 0) tmp = 0;
                    
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_N1_UVLIFEDAY);
                            pMsg += 4;
                        }
                    
                        tmp = gGlobalParam.CMParam.aulCms[DISP_N1_UVLIFEHOUR];
                        if (tmp < 0) tmp = 0;
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_N1_UVLIFEHOUR);
                            pMsg += 4;
                        }                        
                    }
                    break;
                case DISP_N2_UV:
                    {
                        tmp = gGlobalParam.CMParam.aulCms[DISP_N2_UVLIFEDAY] ;
                        if (tmp < 0) tmp = 0;
                    
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_N2_UVLIFEDAY);
                            pMsg += 4;
                        }
                    
                        tmp = gGlobalParam.CMParam.aulCms[DISP_N2_UVLIFEHOUR] ;
                        if (tmp < 0) tmp = 0;
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_N2_UVLIFEHOUR);
                            pMsg += 4;
                        }                       
                    }
                    break;
                case DISP_N3_UV:
                    {
                        tmp = gGlobalParam.CMParam.aulCms[DISP_N3_UVLIFEDAY] ;
                        if (tmp < 0) tmp = 0;
                    
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_N3_UVLIFEDAY);
                            pMsg += 4;
                        }
                    
                        tmp = gGlobalParam.CMParam.aulCms[DISP_N3_UVLIFEHOUR] ;
                        if (tmp < 0) tmp = 0;
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_N3_UVLIFEHOUR);
                            pMsg += 4;
                        }                     
                    }
                    break;
                case DISP_N4_UV:
                    {
                        tmp = gGlobalParam.CMParam.aulCms[DISP_N4_UVLIFEDAY] ;
                        if (tmp < 0) tmp = 0;
                    
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_N4_UVLIFEDAY);
                            pMsg += 4;
                        }
                    
                        tmp = gGlobalParam.CMParam.aulCms[DISP_N4_UVLIFEHOUR];
                        if (tmp < 0) tmp = 0;
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_N4_UVLIFEHOUR);
                            pMsg += 4;
                        }                   
                    }
                    break;
                case DISP_N5_UV:
                    {
                        tmp = gGlobalParam.CMParam.aulCms[DISP_N5_UVLIFEDAY] ;
                        if (tmp < 0) tmp = 0;
                    
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_N5_UVLIFEDAY);
                            pMsg += 4;
                        }
                    
                        tmp = gGlobalParam.CMParam.aulCms[DISP_N5_UVLIFEHOUR] ;
                        if (tmp < 0) tmp = 0;
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_N5_UVLIFEHOUR);
                            pMsg += 4;
                        }              
                    }
                    break;
                case DISP_W_FILTER:
                    {
                        tmp = gGlobalParam.CMParam.aulCms[DISP_W_FILTERLIFE];
                        if (tmp < 0) tmp = 0;
                    
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_W_FILTERLIFE);
                            pMsg += 4;
                        }
                    }
                    break;
                case DISP_T_B_FILTER:
                    {
                        tmp = gGlobalParam.CMParam.aulCms[DISP_T_B_FILTERLIFE] ;
                        if (tmp < 0) tmp = 0;
                    
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_T_B_FILTERLIFE);
                            pMsg += 4;
                        }                 
                    }
                    break;
                case DISP_T_A_FILTER:
                    {
                        tmp = gGlobalParam.CMParam.aulCms[DISP_T_A_FILTERLIFE] ;
                        if (tmp < 0) tmp = 0;
                    
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_T_A_FILTERLIFE);
                            pMsg += 4;
                        }                    
                    }
                    break;
                case DISP_TUBE_FILTER:
                    {
                        tmp = gGlobalParam.CMParam.aulCms[DISP_TUBE_FILTERLIFE] ;
                        if (tmp < 0) tmp = 0;
                    
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_TUBE_FILTERLIFE);
                            pMsg += 4;
                        }                  
                    }
                    break;
                case DISP_TUBE_DI:
                    {
                        tmp = gGlobalParam.CMParam.aulCms[DISP_TUBE_DI_LIFE];
                        if (tmp < 0) tmp = 0;
                    
                        {
                            *(uint32_t *)pMsg = tmp;
                            ulDstMap |= (1 << DISP_TUBE_DI_LIFE);
                            pMsg += 4;
                        }                 
                    }
                    break;
                }
           }

           emitCmReset(pSubReq->ulMap);                
           
           pSubRsp->ulMap = ulDstMap;
           pHoRsp->hdr.ucLen += pMsg - pSubRsp->aucData;
           iPayLoad = APP_PROTOL_HEADER_LEN + pHoRsp->hdr.ucLen;
       }

        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));

        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);
    }

    return 0;
}


int CCB::CanCcbAfDataHOExtQryUsageMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);


    if (ucIndex >=  MAX_HANDLER_NUM)
    {
        return -1;
    }
    
    /* send response */
    {
        unsigned char buf[256];
        unsigned int ulCanId;
        int iPayLoad;
        APP_PACKET_HO_STRU *pHoRsp = (APP_PACKET_HO_STRU *)buf;
        APP_PACKET_HO_USAGE_QUERY_RSP_STRU *pSubRsp = (APP_PACKET_HO_USAGE_QUERY_RSP_STRU *)pHoRsp->aucData;

        pHoRsp->hdr           = pmg->hdr;

        pHoRsp->hdr.ucLen     = 1 + sizeof(APP_PACKET_HO_USAGE_QUERY_RSP_STRU); // 1 for ops type
        pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHoRsp->hdr.ucMsgType |= 0x80;
        pHoRsp->ucOpsType     = pmg->ucOpsType;

           
        pSubRsp->ulMap = gCMUsage.ulUsageState;
        
        iPayLoad = APP_PROTOL_HEADER_LEN + pHoRsp->hdr.ucLen;

        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));

        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);
    }

    return 0;
}



int CCB::CanCcbAfDataHOExtQryAlarmMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);


    if (ucIndex >=  MAX_HANDLER_NUM)
    {
        return -1;
    }
    
    /* send response */
    {
        unsigned char buf[256];
        unsigned int ulCanId;
        int iPayLoad;
        APP_PACKET_HO_STRU *pHoRsp = (APP_PACKET_HO_STRU *)buf;
        APP_PACKET_HO_ALARM_QUERY_RSP_STRU *pSubRsp = (APP_PACKET_HO_ALARM_QUERY_RSP_STRU *)pHoRsp->aucData;

        pHoRsp->hdr           = pmg->hdr;

        pHoRsp->hdr.ucLen     = 1 + sizeof(APP_PACKET_HO_ALARM_QUERY_RSP_STRU); // 1 for ops type
        pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHoRsp->hdr.ucMsgType |= 0x80;
        pHoRsp->ucOpsType     = pmg->ucOpsType;

           
        pSubRsp->ulPart1Alarm = m_aiAlarmRcdMask[0][DISP_ALARM_PART0];
        pSubRsp->ulPart2Alarm = m_aiAlarmRcdMask[0][DISP_ALARM_PART1];
        
        iPayLoad = APP_PROTOL_HEADER_LEN + pHoRsp->hdr.ucLen;

        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));

        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);
    }

    return 0;
}

int CCB::CanCcbAfDataHOExtMachParamQryMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);


    if (ucIndex >=  MAX_HANDLER_NUM)
    {
        return -1;
    }
    
    APP_PACKET_HO_MACH_PARAM_QUERY_REQ_STRU *pHoSubReq = (APP_PACKET_HO_MACH_PARAM_QUERY_REQ_STRU *)pmg->aucData;
    
    /* send response */
    {
        unsigned char buf[256];
        unsigned int ulCanId;
        int iPayLoad;
        int iLoop;
        APP_PACKET_HO_STRU *pHoRsp = (APP_PACKET_HO_STRU *)buf;
        APP_PACKET_HO_MACH_PARAM_QUERY_RSP_STRU *pSubRsp = (APP_PACKET_HO_MACH_PARAM_QUERY_RSP_STRU *)pHoRsp->aucData;

        pHoRsp->hdr           = pmg->hdr;

        pHoRsp->hdr.ucLen     = 1 + sizeof(APP_PACKET_HO_MACH_PARAM_QUERY_RSP_STRU) - 1; // 1 for ops type
        pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHoRsp->hdr.ucMsgType |= 0x80;
        pHoRsp->ucOpsType     = pmg->ucOpsType;

        pSubRsp->aulMap[0] = pHoSubReq->aulMap[0];
        pSubRsp->aulMap[1] = pHoSubReq->aulMap[1];

        uint8_t  *pMsg = pSubRsp->aucData;

        for (iLoop = 0; iLoop < 32; iLoop++)
        {
            if (pHoSubReq->aulMap[0] & (1 << iLoop))
            {
                memcpy(pMsg,&gGlobalParam.MMParam.SP[iLoop],sizeof(float));
                pMsg += 4;
            }
        }

        for (iLoop = 32; iLoop < MACHINE_PARAM_SP_NUM; iLoop++)
        {
            if (pHoSubReq->aulMap[1] & (1 << (iLoop - 32)))
            {
                memcpy(pMsg,&gGlobalParam.MMParam.SP[iLoop],sizeof(float));
                pMsg += 4;
            }
        }

        pHoRsp->hdr.ucLen += pMsg - pSubRsp->aucData;

           
        iPayLoad = APP_PROTOL_HEADER_LEN + pHoRsp->hdr.ucLen;

        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));

        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);
    }

    return 0;
}

int CCB::CanCcbAfDataHOExtSystemInfoQryMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);


    if (ucIndex >=  MAX_HANDLER_NUM)
    {
        return -1;
    }
    
    APP_PACKET_HO_SYS_INFO_QUERY_REQ_STRU *pHoSubReq = (APP_PACKET_HO_SYS_INFO_QUERY_REQ_STRU *)pmg->aucData;
    
    /* send response */
    {
        unsigned char buf[256];
        unsigned int ulCanId;
        int iPayLoad;
        int iLoop;
        APP_PACKET_HO_STRU *pHoRsp = (APP_PACKET_HO_STRU *)buf;
        APP_PACKET_HO_SYS_INFO_QUERY_RSP_STRU *pSubRsp = (APP_PACKET_HO_SYS_INFO_QUERY_RSP_STRU *)pHoRsp->aucData;

        pHoRsp->hdr           = pmg->hdr;

        pHoRsp->hdr.ucLen     = 1 + sizeof(APP_PACKET_HO_SYS_INFO_QUERY_RSP_STRU) - 1; // 1 for ops type
        pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHoRsp->hdr.ucMsgType |= 0x80;
        pHoRsp->ucOpsType     = pmg->ucOpsType;

        pSubRsp->ulMap = pHoSubReq->ulMap;

        uint8_t  *pMsg = pSubRsp->aucData;

        for (iLoop = 0; iLoop < SYSTEM_INFO_NAME_NUM; iLoop++)
        {
            if (pHoSubReq->ulMap & (1 << iLoop))
            {
                APP_PACKET_HO_SYS_INFO_ITEM_STRU *pItem = (APP_PACKET_HO_SYS_INFO_ITEM_STRU *)pMsg;
                
                QString strType = MainGetSysInfo(iLoop);

                pItem->ucLen = strType.toLocal8Bit().length();
                memcpy(pItem->aucData,strType.toLocal8Bit().data(),pItem->ucLen);
                
                pMsg += sizeof(APP_PACKET_HO_SYS_INFO_ITEM_STRU) - 1 + pItem->ucLen;
            }
        }

        // append terminator
        {
            APP_PACKET_HO_SYS_INFO_ITEM_STRU *pItem = (APP_PACKET_HO_SYS_INFO_ITEM_STRU *)pMsg;
            pItem->ucLen = 0;
            pMsg += sizeof(APP_PACKET_HO_SYS_INFO_ITEM_STRU) - 1;
        }

        pHoRsp->hdr.ucLen += pMsg - pSubRsp->aucData;

        iPayLoad = APP_PROTOL_HEADER_LEN + pHoRsp->hdr.ucLen;

        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));

        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);
    }

    return 0;
}


int CCB::CanCcbAfDataHOExportHistoryReqMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_HAND_SET_BEGIN_ADDRESS);


    if (ucIndex >=  MAX_HANDLER_NUM)
    {
        return -1;
    }
    
    APP_PACKET_HO_EXPORT_HISTORY_REQ_STRU *pHoSubReq = (APP_PACKET_HO_EXPORT_HISTORY_REQ_STRU *)pmg->aucData;
    
    /* send response */
    {
        unsigned char buf[256];
        unsigned int ulCanId;
        int iPayLoad;
        //int iLoop;
        APP_PACKET_HO_STRU *pHoRsp = (APP_PACKET_HO_STRU *)buf;
        APP_PACKET_HO_EXPORT_HISTORY_RSP_STRU *pSubRsp = (APP_PACKET_HO_EXPORT_HISTORY_RSP_STRU *)pHoRsp->aucData;

        pHoRsp->hdr           = pmg->hdr;

        pHoRsp->hdr.ucLen     = 1 + sizeof(APP_PACKET_HO_SYS_INFO_QUERY_RSP_STRU) ; // 1 for ops type
        pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHoRsp->hdr.ucMsgType |= 0x80;
        pHoRsp->ucOpsType     = pmg->ucOpsType;

        pSubRsp->ucResult = 0;

        QDateTime startTime;
        QDateTime endTime;

        QDateTime currentDt = QDateTime::currentDateTime();
        QDate date = currentDt.date();

        switch(pHoSubReq->ulMap)
        {
        default:
        case HISTORY_EXPORT_TYPE_DAILY:
            {
                startTime.setDate(date);
                endTime.setDate(date);

                startTime.setTime(QTime(0,0,0));
                endTime.setTime(QTime(23,59,59));
            }
            break;
        case HISTORY_EXPORT_TYPE_WEEKLY:
            {
                QDate tmpDate ;

                tmpDate = date.addDays(-7) ;
                startTime.setDate(tmpDate);

                endTime.setDate(date);

                startTime.setTime(QTime(0,0,0));
                endTime.setTime(QTime(23,59,59));
            }
            break;
        case HISTORY_EXPORT_TYPE_MONTHLY:
            {
                QDate tmpDate ;

                tmpDate = date.addMonths(-1) ;
                startTime.setDate(tmpDate);

                endTime.setDate(date);

                startTime.setTime(QTime(0,0,0));
                endTime.setTime(QTime(23,59,59));
            }
            break;
        case HISTORY_EXPORT_TYPE_3MONTHS:
           {
                QDate tmpDate;

                tmpDate = date.addMonths(-3) ;
                startTime.setDate(tmpDate);

                endTime.setDate(date);

                startTime.setTime(QTime(0,0,0));
                endTime.setTime(QTime(23,59,59));
            }            
            break;
        case HISTORY_EXPORT_TYPE_6MONTHS:
            {
                QDate tmpDate;

                tmpDate = date.addMonths(-6) ;
                startTime.setDate(tmpDate);

                endTime.setDate(date);

                startTime.setTime(QTime(0,0,0));
                endTime.setTime(QTime(23,59,59));
            }            
            break;
        case HISTORY_EXPORT_TYPE_YEARLY:
           {
                QDate tmpDate;

                tmpDate = date.addYears(-1) ;
                startTime.setDate(tmpDate);

                endTime.setDate(date);

                startTime.setTime(QTime(0,0,0));
                endTime.setTime(QTime(23,59,59));
            }            
            break;
        case HISTORY_EXPORT_TYPE_2YEARS:
            {
                 QDate tmpDate;
            
                 tmpDate = date.addYears(-2) ;
                 startTime.setDate(tmpDate);
            
                 endTime.setDate(date);
            
                 startTime.setTime(QTime(0,0,0));
                 endTime.setTime(QTime(23,59,59));
            }            
            
            break;
        }

        pSubRsp->ucResult = copyHistoryToUsb(startTime,endTime);

        iPayLoad = APP_PROTOL_HEADER_LEN + pHoRsp->hdr.ucLen;

        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));

        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,iPayLoad);
    }

    return 0;
}

int CCB::CanCcbAfDataHOExtInstallStartRspMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    if (pmg->hdr.ucTransId < APP_RFID_SUB_TYPE_NUM)
    {
       m_pConsumableInstaller[pmg->hdr.ucTransId]->emitInstallMsg();
    }
    return 0;
}

int CCB::CanCcbAfDataHOExtInstallQryMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    //APP_PACKET_HO_SYS_INFO_QUERY_REQ_STRU *pHoSubReq = (APP_PACKET_HO_SYS_INFO_QUERY_REQ_STRU *)pmg->aucData;
    
    /* send response */
    {
        unsigned char buf[256];
        unsigned int ulCanId;
        APP_PACKET_HO_STRU *pHoRsp = (APP_PACKET_HO_STRU *)buf;
        APP_PACKET_HO_INSTALL_QRY_RSP_STRU *pSubRsp = (APP_PACKET_HO_INSTALL_QRY_RSP_STRU *)pHoRsp->aucData;


        pHoRsp->hdr           = pmg->hdr;

        pHoRsp->hdr.ucLen     = 1 + sizeof(APP_PACKET_HO_SYS_INFO_QUERY_RSP_STRU); // 1 for ops type
        pHoRsp->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL;
        pHoRsp->hdr.ucMsgType |= 0x80;
        pHoRsp->ucOpsType     = pmg->ucOpsType;

        pSubRsp->ucResult = 0;

        if (pmg->hdr.ucTransId < APP_RFID_SUB_TYPE_NUM )
        {
            pSubRsp->ucResult = m_pConsumableInstaller[pmg->hdr.ucTransId]->pendingInstall() ? 1 : 0;
        }

        CAN_BUILD_ADDRESS_IDENTIFIER(ulCanId,APP_PAKCET_ADDRESS_MC,CAN_SRC_ADDRESS(pCanItfMsg->ulCanId));

        CcbSndAfCanCmd(pCanItfMsg->iCanChl,ulCanId,SAPP_CMD_DATA,buf,APP_PROTOL_HEADER_LEN + pHoRsp->hdr.ucLen);
    }

    return 0;
}


int CCB::CanCcbAfDataHandleOpsMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    switch(pmg->ucOpsType)
    {
    case APP_PACKET_HO_CIR:
        if (!(pmg->hdr.ucMsgType & 0x80))
        {
            return CanCcbAfDataHOCirReqMsg(pCanItfMsg);
        }
        return CanCcbAfDataHOCirRspMsg(pCanItfMsg);
    case APP_PACKET_HO_QTW:
        if (!(pmg->hdr.ucMsgType & 0x80))
        {
            return CanCcbAfDataHOQtwReqMsg(pCanItfMsg);
        }
        return CanCcbAfDataHOQtwRspMsg(pCanItfMsg);
    case APP_PACKET_HO_SYSTEM_TEST:
        if (!(pmg->hdr.ucMsgType & 0x80))
        {
           return CanCcbAfDataHODecPressureReqMsg(pCanItfMsg);
        }
        return CanCcbAfDataHODecPressureRspMsg(pCanItfMsg);
        break;
    case APP_PACKET_HO_SPEED:
        if (!(pmg->hdr.ucMsgType & 0x80))
        {
            return CanCcbAfDataHOSpeedReqMsg(pCanItfMsg);
        }
        return CanCcbAfDataHOSpeedRspMsg(pCanItfMsg);
    case APP_PACKET_HO_ADR_RST:
    case APP_PACKET_HO_ADR_SET:
    case APP_PACKET_HO_ADR_QRY:
        if ((pmg->hdr.ucMsgType & 0x80))
        {
            CanCcbIapProc(APP_TRX_CAN,pCanItfMsg);  /* route to UI */   
        }
        break;
    case APP_PACKET_HO_EXT_SYSTEM_START:
        {
            return CanCcbAfDataHOExtSysStartReqMsg(pCanItfMsg);
        }
        break;
    case APP_PACKET_HO_EXT_RO_CLEAN:
        {
            return CanCcbAfDataHOExtSterlizeReqMsg(pCanItfMsg);
        }
        break;
    case APP_PACKET_HO_EXT_RTI_QUERY:
        {
            return CanCcbAfDataHOExtRtiReqMsg(pCanItfMsg);
        }
        break;
    case APP_PACKET_HO_EXT_CONFIG_QUERY:
        {
            return CanCcbAfDataHOExtConfigQryReqMsg(pCanItfMsg);
        }
        break;
    case APP_PACKET_HO_EXT_CONFIG_SET:
        {
            return CanCcbAfDataHOExtConfigSetReqMsg(pCanItfMsg);
        }        
        break;
    case APP_PACKET_HO_EXT_RO_QUERY:
        {
            return CanCcbAfDataHOExtSterlizeQryMsg(pCanItfMsg);
        }
        break;
    case APP_PACKET_HO_EXT_QUERY_CM_STATE:
        {
            return CanCcbAfDataHOExtCmStateQryMsg(pCanItfMsg);
        }
        break;
    case APP_PACKET_HO_EXT_QUERY_CM_INFO:
        {
            return CanCcbAfDataHOExtCmInfoQryMsg(pCanItfMsg);
        }
        break;
    case APP_PACKET_HO_EXT_RESET_CM:
        {
            return CanCcbAfDataHOExtCmResetMsg(pCanItfMsg);
        }
        break;
    case APP_PACKET_HO_EXT_QUERY_NOTI:
        {
            return CanCcbAfDataHOExtQryUsageMsg(pCanItfMsg);
        }
        break;
    case APP_PACKET_HO_EXT_QUERY_ALARM:
        {
            return CanCcbAfDataHOExtQryAlarmMsg(pCanItfMsg);
        }
        break;
    case APP_PACKET_HO_EXT_MACH_PARAM_QUERY:
        {
            return CanCcbAfDataHOExtMachParamQryMsg(pCanItfMsg);
        }
        break;
    case APP_PACKET_HO_EXT_QUERY_SYS_INFO:
        {
            return CanCcbAfDataHOExtSystemInfoQryMsg(pCanItfMsg);
        }
        break;
    case APP_PACKET_HO_EXT_EXPORT_HISTORY:
        {
            return CanCcbAfDataHOExportHistoryReqMsg(pCanItfMsg);
        }
        break;
    case APP_PACKET_HO_EXT_ACCESSORY_INSTALL_START:
        if (pmg->hdr.ucMsgType & 0x80)
        {
            return CanCcbAfDataHOExtInstallStartRspMsg(pCanItfMsg);
        }
        break;
    case APP_PACKET_HO_EXT_ACCESSORY_INSTALL_QRY:
        {
            return CanCcbAfDataHOExtInstallQryMsg(pCanItfMsg);
        }
    case APP_PACKET_HO_EXT_QUERY_CM_DETAIL_INFO:
        {
            return CanCcbAfDataHOExtCmDetailInfoQryMsg(pCanItfMsg);
        }
        break;
    }
    return 0;
}



int CCB::CanCcbAfDataRfSearchRsp(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_RF_STRU *pmg = (APP_PACKET_RF_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    APP_PACKET_RF_SEARCH_RSP_STRU *pRsp = (APP_PACKET_RF_SEARCH_RSP_STRU *)pmg->aucData;

    uint8_t ucResult = APP_PACKET_HO_ERROR_CODE_UNSUPPORT;

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_RF_READER_BEGIN_ADDRESS);

    if (ucIndex >=  MAX_RFREADER_NUM)
    {
        ucResult = APP_PACKET_RF_NO_DEVICE;
        goto end;
    }

    {
        RFREADER_STRU *pRfReader = &aRfReader[ucIndex];
    
        if (pRsp->ucLen)
        {
            APP_PACKET_RPT_RF_STATE_CONT_STRU *pCnt = (APP_PACKET_RPT_RF_STATE_CONT_STRU *)pRsp->aucData;
            pRfReader->ucBlkNum  = pCnt->ucBlkNum;
            pRfReader->ucBlkSize = pCnt->ucBlkSize;
            memcpy(pRfReader->aucUid,pCnt->aucUid,8);
    
            if (!pRfReader->bit1RfDetected)
            {
                pRfReader->bit1RfDetected  = TRUE;
                pRfReader->bit1RfContValid = FALSE;
            }
        }
        else
        {
            pRfReader->bit1RfDetected  = FALSE;
            pRfReader->bit1RfContValid = FALSE;
        }
    }

    return 0;
end:
    
    return ucResult;
}

int CCB::CanCcbAfDataRfReadRsp(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_RF_STRU *pmg = (APP_PACKET_RF_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    APP_PACKET_RF_READ_RSP_STRU *pRsp = (APP_PACKET_RF_READ_RSP_STRU *)pmg->aucData;

    uint8_t ucResult = APP_PACKET_HO_ERROR_CODE_SUCC;

    int     iSrcAdr  = CAN_SRC_ADDRESS(pCanItfMsg->ulCanId);

    uint8_t ucIndex  = (iSrcAdr - APP_RF_READER_BEGIN_ADDRESS);

    if (ucIndex >=  MAX_RFREADER_NUM)
    {
        ucResult = APP_PACKET_RF_NO_DEVICE;
    }
    else
    {
        RFREADER_STRU *pRfReader = &aRfReader[ucIndex];
    
        if (APP_PACKET_RF_ERROR_CODE_SUCC == pRsp->ucRslt)
        {
            APP_PACKET_RPT_RF_READ_CONT_STRU *pCnt = (APP_PACKET_RPT_RF_READ_CONT_STRU *)pRsp->aucData;
    
            if (pCnt->ucOff + pCnt->ucLen <= MAX_RFREADR_CONTENT_SIZE)
            {
                memcpy(&pRfReader->aucRfCont[pCnt->ucOff],pCnt->aucData,pCnt->ucLen);
    
                pRfReader->bit1RfContValid = TRUE;

                //LOG(VOS_LOG_DEBUG,"ucOff = %d,ucLen = %d",pCnt->ucOff,pCnt->ucLen);

                //printhex(pCnt->aucData,pCnt->ucLen);
            }
    
        }
    }

    return ucResult;
}



int CCB::CanCcbAfDataRfWriteRsp(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    (void)pCanItfMsg;
    LOG(VOS_LOG_WARNING,"Enter %s",__FUNCTION__);    
    return 0;
}


int CCB::CanCcbAfDataRfOpsMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_HO_STRU *pmg = (APP_PACKET_HO_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 


    switch(pmg->ucOpsType)
    {
    case APP_PACKET_RF_SEARCH:
        if ((pmg->hdr.ucMsgType & 0x80))
        {
            CanCcbAfDataRfSearchRsp(pCanItfMsg); 
            
            CanCcbAfRfIdMsg(0x4);
            /* pump to ui */
            CanCcbIapProc(APP_TRX_CAN,pCanItfMsg);
        }
        break;
    case APP_PACKET_RF_READ:
        if ((pmg->hdr.ucMsgType & 0x80))
        {
            /* pump to ui */
            CanCcbAfDataRfReadRsp(pCanItfMsg);
            
            CanCcbAfRfIdMsg(0x1);
            
            CanCcbIapProc(APP_TRX_CAN,pCanItfMsg);
        }
        break;
    case APP_PACKET_RF_WRITE:
        if ((pmg->hdr.ucMsgType & 0x80))
        {
            CanCcbAfDataRfWriteRsp(pCanItfMsg);
            
            CanCcbAfRfIdMsg(0x2);
            
            /* pump to ui */
            CanCcbIapProc(APP_TRX_CAN,pCanItfMsg);  
        }
        break;
    case APP_PACKET_RF_ADR_RST:
    case APP_PACKET_RF_ADR_SET:
    case APP_PACKET_RF_ADR_QRY:
        if ((pmg->hdr.ucMsgType & 0x80))
        {
            CanCcbIapProc(APP_TRX_CAN,pCanItfMsg);  
        }
        break;
    }
    return 0;
}


int CCB::CanCcbAfDataStateIndMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_STATE_IND_STRU *pmg = (APP_PACKET_STATE_IND_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    {
        int iPackets = (pmg->hdr.ucLen ) / sizeof (APP_PACKET_STATE_STRU);
        APP_PACKET_STATE_STRU *pState = (APP_PACKET_STATE_STRU *)pmg->aucData;
        int iLoop;
        
        for (iLoop = 0; iLoop < iPackets; iLoop++,pState++ )
        {
            switch(pState->ucType)
            {
            case APP_OBJ_N_PUMP:   
                if (pState->ucId < APP_EXE_G_PUMP_NUM)
                {
                    ExeBrd.aGPumpObjs[pState->ucId].iState = !!pState->ucState;
                }
                break;
            case APP_OBJ_R_PUMP:   
                if (pState->ucId < APP_EXE_R_PUMP_NUM)
                {
                    ExeBrd.aRPumpObjs[pState->ucId].iState = !!pState->ucState;
                }
                break;
            }
        }
    }

    return 0;
}

int CCB::CanCcbAfDataExeTocRspMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    /* do nothing here */
    pCanItfMsg = pCanItfMsg;

   
   LOG(VOS_LOG_DEBUG,"Enter %s",__FUNCTION__);    
    
    return 0;
}

int CCB::CanCcbAfDataExeOpsMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_EXE_STRU *pmg = (APP_PACKET_EXE_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 

    LOG(VOS_LOG_DEBUG,"Enter %s",__FUNCTION__);    

    switch(pmg->ucOpsType)
    {
    case APP_PACKET_EXE_TOC:
        if ((pmg->hdr.ucMsgType & 0x80))
        {
            return CanCcbAfDataExeTocRspMsg(pCanItfMsg);
        }
        break;
    default:
        break;

    }
    return 0;
}

int CCB::CanCcbAfDataMsg(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    APP_PACKET_COMM_STRU *pmg = (APP_PACKET_COMM_STRU *)&pCanItfMsg->aucData[1 + RPC_POS_DAT0]; 
    switch((pmg->ucMsgType & 0x7f))
    {
    case APP_PAKCET_COMM_ONLINE_NOTI:
        if (!(pmg->ucMsgType & 0x80))
        {
            return CanCcbAfDataOnLineNotiIndMsg(pCanItfMsg);
        }
        break;
    case APP_PACKET_COMM_HEART_BEAT:
        if ((pmg->ucMsgType & 0x80))
        {
            return CanCcbAfDataHeartBeatRspMsg(pCanItfMsg);
        }
        break;
    case APP_PACKET_CLIENT_REPORT:
        CanCcbAfDataClientRptMsg(pCanItfMsg);
        break;
    case APP_PACKET_HAND_OPERATION:
        CanCcbAfDataHandleOpsMsg(pCanItfMsg);
        break;
    case APP_PACKET_RF_OPERATION:
        CanCcbAfDataRfOpsMsg(pCanItfMsg);
        break;
    case APP_PACKET_STATE_INDICATION:
        CanCcbAfDataStateIndMsg(pCanItfMsg);
        break;
    case APP_PACKET_EXE_OPERATION:
        CanCcbAfDataExeOpsMsg(pCanItfMsg);
        break;
    default: // yet to be implemented
        break;
    }

    return 0;
}


void CCB::CanCcbAfProc(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
    switch((pCanItfMsg->aucData[1 + RPC_POS_CMD1] & 0X7F))
    {
    case SAPP_CMD_DATA:
        CanCcbAfDataMsg(pCanItfMsg);
        break;
    case SAPP_CMD_MODBUS:
        CanCcbAfModbusMsg(pCanItfMsg);
        break;
    }
}

void CCB::CanCcbHandlerMgrProc(MAIN_CANITF_MSG_STRU *pCanItfMsg)
{
   /* Notify Ui */
   IAP_NOTIFY_STRU *pNotify = (IAP_NOTIFY_STRU *)gaucIapBuffer;

   pNotify->ulCanId = pCanItfMsg->ulCanId;
   pNotify->iCanChl = pCanItfMsg->iCanChl;
   pNotify->iMsgLen = pCanItfMsg->iMsgLen;
   pNotify->iTrxIndex = APP_TRX_CAN;

   memcpy(pNotify->data,pCanItfMsg->aucData,pNotify->iMsgLen);

   emitIapNotify(pNotify);

}

void CCB::CanCcbCanItfMsg(SAT_MSG_HEAD *pucMsg)
{
    MAIN_CANITF_MSG_STRU *pCanItfMsg = (MAIN_CANITF_MSG_STRU *)pucMsg;

    switch(pCanItfMsg->aucData[1 + RPC_POS_CMD0])
    {
    case RPC_SYS_AF:
        CanCcbAfProc(pCanItfMsg);
        break;
    case RPC_SYS_BOOT:
        CanCcbIapProc(APP_TRX_CAN,pCanItfMsg);
        break;
    default:
        break;
    }
}


void CCB::CanCcbRestore()
{
    if (bit1LeakKey4Reset)
    {
        return ;
    }

    switch(curWorkState.iMainWorkState)
    {
    case DISP_WORK_STATE_RUN:
        curWorkState.iSubWorkState = DISP_WORK_SUB_IDLE;
        CcbCancelAllWork();
        CcbInnerWorkRun();
        break;
    default:
        break;
    }
}

void CCB::CcbWorMsgProc(SAT_MSG_HEAD *pucMsg)
{
    MAIN_WORK_MSG_STRU *pWorkMsg = (MAIN_WORK_MSG_STRU *)pucMsg;


    switch(pWorkMsg->iSubMsg)
    {
    case WORK_MSG_TYPE_START_QTW:
        {
            int aResult[2];

            memcpy(aResult,pWorkMsg->aucData,sizeof(aResult));
            
            aHandler[aResult[1]].bit1Qtw = aResult[0] == 0 ? 1 : 0;

            aHandler[aResult[1]].bit1PendingQtw = 0;
            
            if(aResult[1] < VIRTUAL_HANDLER)
            {
                CanCcbSndQtwRspMsg(aResult[1],aResult[0]);
            }

            if (!aResult[0])
            {
                LOG(VOS_LOG_WARNING,"CanCcbTransState4Pw");    
            
                CanCcbTransState4Pw(DISP_WORK_STATE_RUN,DISP_WORK_SUB_RUN_QTW);

                CcbQTwVolumnNotify(aHandler[aResult[1]].iDevType,aResult[1],&QtwMeas);
                
            }
            CcbNotAscInfo(pWorkMsg->iSubMsg*2 + !!aResult[0]);

        }
        break;
        
    case WORK_MSG_TYPE_STOP_QTW:
        {
            int aResult[2];
    
            memcpy(aResult,pWorkMsg->aucData,sizeof(aResult));
    
            aHandler[aResult[1]].bit1Qtw = 0;

            aHandler[aResult[1]].bit1PendingQtw = 0;
            
            CcbNotAscInfo(pWorkMsg->iSubMsg*2 + !!aResult[0]);

            LOG(VOS_LOG_WARNING,"CanCcbTransState4Pw");    

            /* report */
            CanCcbTransState4Pw(DISP_WORK_STATE_RUN,DISP_WORK_SUB_IDLE);

            if (APP_DEV_HS_SUB_UP == aHandler[aResult[1]].iDevType)
            {
                //if (QtwMeas.ulCurFm > QtwMeas.ulBgnFm)
                {
                   CcbTwStaticsNotify(APP_DEV_HS_SUB_UP,aResult[1],&QtwMeas);
                }
            
                /* start circulation */
               if (!SearchWork(work_start_cir))
               {
                   CcbInnerWorkStartCir(CIR_TYPE_UP);
               }   
            }
            else
            {
                //if (QtwMeas.ulCurFm > QtwMeas.ulBgnFm)
                {
                   CcbTwStaticsNotify(APP_DEV_HS_SUB_HP,aResult[1],&QtwMeas);
                }

                switch(gGlobalParam.iMachineType)
                {
                case MACHINE_C:
                    if (!SearchWork(work_start_cir))
                    {
                        CcbInnerWorkStartCir(CIR_TYPE_HP);
                    }  
                    break;
                case MACHINE_PURIST:
                case MACHINE_C_D:
                    break;
                default:
                    /* start circulation if any */
                    if (!bit1I44Nw)
                    {
                        if (haveHPCir())
                        {
                             if (!SearchWork(work_start_cir))
                             {
                                 CcbInnerWorkStartCir(CIR_TYPE_HP);
                                 break;
                             }  
                        }
                    }
                    break;
                }
                
            }
            
        }
        break;
    case WORK_MSG_TYPE_SCIR:
        {
            int aResult[1];
    
            memcpy(aResult,pWorkMsg->aucData,sizeof(aResult));
    
            CcbNotAscInfo(pWorkMsg->iSubMsg*2 + !!aResult[0]);

            if (0 == aResult[0])
            {
                ulCirTick = gulSecond;

                CanCcbTransState4Pw(DISP_WORK_STATE_RUN,DISP_WORK_SUB_RUN_CIR);
            
            }
            else
            {
                LOG(VOS_LOG_WARNING,"CanCcbTransState4Pw");   
                
                CanCcbTransState4Pw(DISP_WORK_STATE_RUN,DISP_WORK_SUB_IDLE);
            }
        }        
        break;
    case WORK_MSG_TYPE_ECIR:
        {
            int aResult[2];
    
            memcpy(aResult,pWorkMsg->aucData,sizeof(aResult));
    
            CcbNotAscInfo(pWorkMsg->iSubMsg*2 + !!aResult[0]);

            ulLstCirTick = gulSecond;

            switch(iCirType)
            {
            case CIR_TYPE_UP:
                {
                    float fValue = ExeBrd.aEcoObjs[APP_EXE_I5_NO].Value.eV.fWaterQ;
                    if (fValue < CcbGetSp7())
                    {
                        /* fire alarm */
                        if (!(ulFiredAlarmFlags  & ALARM_UP))
                        {
                            if (gulSecond - ulAlarmUPTick >= 30)
                            {
                                ulFiredAlarmFlags |= ALARM_UP;
                            
                                CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_UP_PRODUCT_RESISTENCE,TRUE);
                            }
                        }
                    }                   
                }
                break;
            default:
                break;
            }

            LOG(VOS_LOG_WARNING,"CanCcbTransState4Pw");    
            
            if (!aResult[1])
            {
                CanCcbTransState4Pw(DISP_WORK_STATE_RUN,DISP_WORK_SUB_IDLE);
            }
            else
            {
                curWorkState.iMainWorkState4Pw = DISP_WORK_STATE_RUN;
                curWorkState.iSubWorkState4Pw  = DISP_WORK_SUB_IDLE;
            }

        }        
        break;
    case WORK_MSG_TYPE_ESR:
        {
            int aResult[2];

            memcpy(aResult,pWorkMsg->aucData,sizeof(aResult));

            CanCcbSndHoSpeedRspMsg(aResult[1],aResult[0]);

            if (aResult[0])
            {
                LOG(VOS_LOG_WARNING,"Ho Speed Regulation Failed");    
            }

            /* Notify UI */
            CcbNotSpeed(aHandler[aResult[1]].iDevType,aHandler[aResult[1]].iSpeed);
            
        }
        break;        
    case WORK_MSG_TYPE_INIT_RUN:
        {
            int aResult[1];
    
            memcpy(aResult,pWorkMsg->aucData,sizeof(aResult));
           
            CcbNotAscInfo(pWorkMsg->iSubMsg*2 + !!aResult[0]);

            if (0 == aResult[0])
            {
                /* continue to produce water */
                if (!(DISP_WORK_STATE_RUN == curWorkState.iMainWorkState
                     && curWorkState.iSubWorkState > DISP_WORK_SUB_RUN_INIT))
                {
                    LOG(VOS_LOG_WARNING,"CanCcbTransState");    
                    CanCcbTransState(DISP_WORK_STATE_RUN,DISP_WORK_SUB_IDLE);
                
                }
                
                ulN3PeriodTimer = 0;
                ulN3Duration    = 0;
                bit1N3Active    = 0;
            }
            else
            {
                bit1ProduceWater = FALSE;
                bit1B1Check4RuningState = FALSE;
            
                CanCcbTransState(DISP_WORK_STATE_IDLE,DISP_WORK_SUB_IDLE);
            }

            /* check isolated alarms */
            if (  (ulFiredAlarmFlags  & ALARM_ROPW)
                && (!bit1AlarmRoPw))
            {
                ulFiredAlarmFlags &= ~ALARM_ROPW;
                
                CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_HIGER_RO_PRODUCT_CONDUCTIVITY,FALSE);
            
            }
            if (  (ulFiredAlarmFlags  & ALARM_REJ)
                && (!bit1AlarmRej))
            {
                ulFiredAlarmFlags &= ~ALARM_REJ;
                
                CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_RO_RESIDUE_RATIO,FALSE);
            
            }
            
            if (  (ulFiredAlarmFlags  & ALARM_EDI)
                && (!bit1AlarmEDI))
            {
                ulFiredAlarmFlags &= ~ALARM_EDI;
                
                CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_EDI_PRODUCT_RESISTENCE,FALSE);
            
            }
            
            if (  (ulFiredAlarmFlags  & ALARM_ROPLV)
                && (!bit1AlarmROPLV))
            {
                ulFiredAlarmFlags &= ~ALARM_ROPLV;
                
                CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_RO_PRODUCT_FLOWING_VELOCITY,FALSE);
            }
            
            if (  (ulFiredAlarmFlags  & ALARM_RODV)
                && (!bit1AlarmRODV))
            {
                ulFiredAlarmFlags &= ~ALARM_RODV;
                
                CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_RO_WASTE_FLOWING_VELOCITY,FALSE);
            }      
        }
        break;
    case WORK_MSG_TYPE_RUN:
        {
            int aResult[1];
    
            memcpy(aResult,pWorkMsg->aucData,sizeof(aResult));
            CcbNotAscInfo(pWorkMsg->iSubMsg*2 + !!aResult[0]);

            if (0 == aResult[0])
            {
                if (!(DISP_WORK_STATE_RUN == curWorkState.iMainWorkState
                     && curWorkState.iSubWorkState > DISP_WORK_SUB_RUN_INIT))
                {
                    LOG(VOS_LOG_WARNING,"CanCcbTransState");   
                    
                    CanCcbTransState(DISP_WORK_STATE_RUN,DISP_WORK_SUB_IDLE);
                    
                    //CanCcbSndHOState(APP_PROTOL_CANID_BROADCAST,APP_PACKET_HS_STATE_RUN);
                }
            }
            else
            {
                bit1ProduceWater        = FALSE;
                bit1B1Check4RuningState = FALSE;
            }

            /* check isolated alarms */
            if (  (ulFiredAlarmFlags  & ALARM_ROPW)
                && (!bit1AlarmRoPw))
            {
                ulFiredAlarmFlags &= ~ALARM_ROPW;
                
                CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_HIGER_RO_PRODUCT_CONDUCTIVITY,FALSE);
            
            }
            if (  (ulFiredAlarmFlags  & ALARM_REJ)
                && (!bit1AlarmRej))
            {
                ulFiredAlarmFlags &= ~ALARM_REJ;
                
                CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_RO_RESIDUE_RATIO,FALSE);
            
            }

            if (  (ulFiredAlarmFlags  & ALARM_EDI)
                && (!bit1AlarmEDI))
            {
                ulFiredAlarmFlags &= ~ALARM_EDI;
                
                CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_EDI_PRODUCT_RESISTENCE,FALSE);
            
            }

            if (  (ulFiredAlarmFlags  & ALARM_ROPLV)
                && (!bit1AlarmROPLV))
            {
                ulFiredAlarmFlags &= ~ALARM_ROPLV;
                
                CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_RO_PRODUCT_FLOWING_VELOCITY,FALSE);
            }
            
            if (  (ulFiredAlarmFlags  & ALARM_RODV)
                && (!bit1AlarmRODV))
            {
                ulFiredAlarmFlags &= ~ALARM_RODV;
                
                CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_RO_WASTE_FLOWING_VELOCITY,FALSE);
            }            
        }
        break;  
    case WORK_MSG_TYPE_WASH:
        {
            int aResult[2];

            CanCcbTransState(DISP_WORK_STATE_IDLE,DISP_WORK_SUB_IDLE);   
            
            memcpy(aResult,pWorkMsg->aucData,sizeof(aResult));
            
            CcbNotAscInfo(pWorkMsg->iSubMsg*2 + !!aResult[0]);

            CcbWashStateNotify(iWashType, 0);

            // 以下代码可以在清洗完成后，进入运行状态
            // if (0 == aResult[0])
            // {
            //     CcbInnerWorkInitRun(); // move to RUN state
            // }
        }
        break;
    case WORK_MSG_TYPE_SFW:
        {
            int aResult[1];
    
            memcpy(aResult,pWorkMsg->aucData,sizeof(aResult));
            CcbNotAscInfo(pWorkMsg->iSubMsg*2 + !!aResult[0]);
        }
        break;
    case WORK_MSG_TYPE_EFW:
        {
            int aResult[1];
    
            memcpy(aResult,pWorkMsg->aucData,sizeof(aResult));
            CcbNotAscInfo(pWorkMsg->iSubMsg*2 + !!aResult[0]);
        }
        break;
    case WORK_MSG_TYPE_SN3:
        {
            int aResult[1];
    
            memcpy(aResult,pWorkMsg->aucData,sizeof(aResult));
            CcbNotAscInfo(pWorkMsg->iSubMsg*2 + !!aResult[0]);

            if (0 == aResult[0])
            {
                bit1N3Active = 1;
                ulN3Duration = 0;
            }
        }
        break;
    case WORK_MSG_TYPE_EN3:
        {
            int aResult[1];
    
            memcpy(aResult,pWorkMsg->aucData,sizeof(aResult));
            CcbNotAscInfo(pWorkMsg->iSubMsg*2 + !!aResult[0]);
            bit1N3Active = 0;
        }
        break;
    case WORK_MSG_TYPE_SKH:
        {
            int aResult[1];
    
            memcpy(aResult,pWorkMsg->aucData,sizeof(aResult));
            CcbNotAscInfo(pWorkMsg->iSubMsg*2 + !!aResult[0]);

            if (aResult[0])
            {
                CanCcbDinProcess();
            }
        }
        break;
    case WORK_MSG_TYPE_EKH:
        {
            if (ExeBrd.ucDinState & APP_EXE_DIN_FAIL_MASK)
            {
                CanCcbDinProcess();
            }
            else
            {
                /* restore to previous working state */
                CanCcbRestore();
            }
        }
        break;
    case WORK_MSG_TYPE_SLH:
        {
            int aResult[1];
    
            memcpy(aResult,pWorkMsg->aucData,sizeof(aResult));
            CcbNotAscInfo(pWorkMsg->iSubMsg*2 + !!aResult[0]);

            if (aResult[0])
            {
                CanCcbLeakProcess();
            }
        }
        break;
    case WORK_MSG_TYPE_ELH:
        {
            if (ExeBrd.ucLeakState)
            {
               CanCcbLeakProcess();
            }
            else
            {
                /* restore to previous working state */
                CanCcbRestore();
            }
        }
        break;
    case WORK_MSG_TYPE_EPW:
        {
            CcbProduceWater(ulProduceWaterBgnTime);

            CcbPwVolumeStatistics();

        }
        break;
    case WORK_MSG_TYPE_STATE:
        {
           switch(curWorkState.iMainWorkState)
           {
           case DISP_WORK_STATE_IDLE:
               switch(curWorkState.iSubWorkState)
               {
               case DISP_WORK_SUB_IDLE:
                    CcbNotState(NOT_STATE_INIT);
                    break;
               case DISP_WORK_SUB_WASH:
                    CcbNotState(NOT_STATE_WASH);
                    break;
               }
               break;
               //2018.12.18
           case DISP_WORK_STATE_PREPARE:
               switch(curWorkState.iSubWorkState)
               {
               case DISP_WORK_SUB_IDLE:
                   CcbNotState(NOT_STATE_RUN);
                   break;
               }
               break;
               //end
           case DISP_WORK_STATE_RUN:
                switch(curWorkState.iSubWorkState)
                {
                case DISP_WORK_SUB_IDLE:
                case DISP_WORK_SUB_RUN_INIT:
                    CcbNotState(NOT_STATE_RUN);
                    break;
                }
                break;
           case DISP_WORK_STATE_LPP:
                CcbNotState(NOT_STATE_LPP);
                CcbCleanup();
                break;
            //ex
           case DISP_WORK_STATE_KP:
               switch(curWorkState.iSubWorkState)
               {
               case DISP_WORK_SUB_IDLE:
                    CcbNotState(NOT_STATE_INIT);
                    break;
               }
               break;
            //end
           }
        }
        break;
    case WORK_MSG_TYPE_STATE4PW:
        {
           switch(curWorkState.iMainWorkState4Pw)
           {
           case DISP_WORK_STATE_IDLE:
               switch(curWorkState.iSubWorkState4Pw)
               {
               default:
                   CanCcbSndHOState(APP_PROTOL_CANID_BROADCAST,APP_PACKET_HS_STATE_IDLE);
                   break;
               case DISP_WORK_SUB_IDLE_DEPRESSURE:
                   CanCcbSndHOState(APP_PROTOL_CANID_BROADCAST,APP_PACKET_HS_STATE_DEC);
                   break;
               }
               break;
           case DISP_WORK_STATE_RUN:
                switch(curWorkState.iSubWorkState4Pw)
                {
                case DISP_WORK_SUB_IDLE:
                    CcbNotState(NOT_STATE_OTHER);
                    CanCcbSndHOState(APP_PROTOL_CANID_BROADCAST,APP_PACKET_HS_STATE_RUN);
                    break;
                case DISP_WORK_SUB_RUN_QTW:
                    CcbNotState(NOT_STATE_QTW);
                    CanCcbSndHOState(APP_PROTOL_CANID_BROADCAST,APP_PACKET_HS_STATE_QTW);
                case DISP_WORK_SUB_RUN_QTWING:
                    break;
                case DISP_WORK_SUB_RUN_CIR:
                    CcbNotState(NOT_STATE_CIR);
                    CanCcbSndHOState(APP_PROTOL_CANID_BROADCAST,APP_PACKET_HS_STATE_CIR);
                case DISP_WORK_SUB_RUN_CIRING:
                    //if (CIR_TYPE_UP == iCirType) 
                    break;
                case DISP_WORK_SUB_RUN_DEC:
                    CcbNotState(NOT_STATE_DEC);
                    CanCcbSndHOState(APP_PROTOL_CANID_BROADCAST,APP_PACKET_HS_STATE_DEC);
                    break;
                }
                break;
           }
        }
        break;
    case WORK_MSG_TYPE_IDLE:
        {
            int aResult[1];

            memcpy(aResult,pWorkMsg->aucData,sizeof(aResult));
            
            CcbNotAscInfo(pWorkMsg->iSubMsg*2 + !!aResult[0]);

            CanCcbTransState(DISP_WORK_STATE_IDLE,DISP_WORK_SUB_IDLE);

            CanCcbTransState4Pw(DISP_WORK_STATE_IDLE,DISP_WORK_SUB_IDLE);

            CcbCleanup();
        }
        break;
    default :
        break;
    }

}


int CCB::CcbModbusWorkEntry(int iChl,unsigned int ulCanId,unsigned char *pucModbus,int iPayLoad,int iTime,modbus_call_back cb)
{
    int iRet;
    
    struct timeval now;
    struct timespec outtime;
    
    sp_thread_mutex_lock( &Ipc.mutex );
    
    iRet = CcbSndAfCanCmd(iChl,ulCanId,SAPP_CMD_MODBUS,pucModbus,iPayLoad);
    if (0 != iRet)
    {
        sp_thread_mutex_unlock( &Ipc.mutex );
        
        return -1;
    }

    gettimeofday(&now, NULL);
    outtime.tv_sec  = now.tv_sec + iTime/1000;
    outtime.tv_nsec = (now.tv_usec + (iTime  % 1000)*1000)* 1000;

    ModbusCb = cb;

    iRet = sp_thread_cond_timedwait( &Ipc.cond, &Ipc.mutex ,&outtime);//ylf: thread sleep here
    if(ETIMEDOUT == iRet)
    {
        sp_thread_mutex_unlock( &Ipc.mutex );
        if (ModbusCb )(ModbusCb)(this,iRet,NULL);
        ModbusCb = NULL;
        return -1;
    }

    if (ulWorkThdAbort & WORK_THREAD_STATE_BLK_MODBUS)
    {
        sp_thread_mutex_unlock( &Ipc.mutex );
        if (ModbusCb )(ModbusCb)(this,-1,NULL);
        ModbusCb = NULL;
        return -2;
    }

    ModbusCb = NULL;
    sp_thread_mutex_unlock( &Ipc.mutex );  
    
    return 0; // success
}

int  CCB::CcbModbusWorkEntryWrapper(int id,int iChl,unsigned int ulCanId,unsigned char *pucModbus,int iPayLoad,int iTime,modbus_call_back cb)
{
    int iRet;

    sp_thread_mutex_lock( &WorkMutex );
    ulWorkThdState |= MakeThdState(id,WORK_THREAD_STATE_BLK_MODBUS);
    sp_thread_mutex_unlock( &WorkMutex );

    sp_thread_mutex_lock( &ModbusMutex );
    iRet = CcbModbusWorkEntry(iChl,ulCanId,pucModbus,iPayLoad,iTime,cb);
    sp_thread_mutex_unlock( &ModbusMutex );

    sp_thread_mutex_lock( &WorkMutex );
    ulWorkThdState &= ~ MakeThdState(id,WORK_THREAD_STATE_BLK_MODBUS);
    ulWorkThdAbort &= ~ MakeThdState(id,WORK_THREAD_STATE_BLK_MODBUS);
    sp_thread_mutex_unlock( &WorkMutex );

    return iRet;
}

void CCB::work_init_run_fail(int iWorkId)
{
    int aiCont[1] = {-1};

    /* close all switchs */
    if (CcbGetSwitchObjState(APP_EXE_SWITCHS_MASK))
    {
        if(haveB3())
        {
            CcbUpdateSwitch(iWorkId, 0, APP_EXE_INNER_SWITCHS, 0);
        }
        else
        {
            CcbUpdateSwitch(iWorkId, 0, APP_EXE_INNER_SWITCHS | (1 << APP_EXE_E10_NO), 0);
        }
    }
    
    /*stop all report */
    if (CcbGetPmObjState(APP_EXE_PM_MASK)
        || CcbGetIObjState(APP_EXE_I_MASK))
    {
        CcbSetIAndBs(iWorkId,0,0);
        
    }

    if (CcbGetFmObjState(APP_FM_FM_MASK))
    {
        CcbSetFms(iWorkId,0,0);
    }
    
    bit1AlarmRej  = FALSE;
    bit1AlarmRoPw = FALSE;
    bit1AlarmEDI  = FALSE;
    bit3RuningState = NOT_RUNING_STATE_NONE;
    bit1InitRuned = FALSE;
    MainSndWorkMsg(WORK_MSG_TYPE_INIT_RUN,(unsigned char *)aiCont,sizeof(aiCont));
    
}


void CCB::work_init_run_succ(int iWorkId)
{
    int aiCont[1] = {0};

    (void )iWorkId;
    bit3RuningState = NOT_RUNING_STATE_NONE;
    
    MainSndWorkMsg(WORK_MSG_TYPE_INIT_RUN,(unsigned char *)aiCont,sizeof(aiCont));

}

//设备运行10s冲洗，10s测压，清洗，制水
void CCB::work_run_comm_proc(WORK_ITEM_STRU *pWorkItem, bool bInit)
{
    int iRet;
    int iTmp;
    int iLoop;

    ulProduceWaterBgnTime  = time(NULL); 

    for (iLoop = 0; iLoop < APP_FM_FLOW_METER_NUM; iLoop++)
    {
        FlowMeter.aulPwStart[iLoop] = INVALID_FM_VALUE;
        FlowMeter.aulPwEnd[iLoop]   = INVALID_FM_VALUE;
    }

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_EDI:
        {
            /* get B2 reports from exe */
            iRet = CcbGetPmValue(pWorkItem->id,APP_EXE_PM2_NO,1);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbGetPmValue Fail %d",iRet);    
                /* notify ui (late implemnt) */
                bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);   
                return ;
            }    
        
            if (CcbConvert2Pm2SP(ExeBrd.aPMObjs[APP_EXE_PM2_NO].Value.ulV) >= B2_FULL)
            {
                CanPrepare4Pm2Full();
                
                /* close all switchs */    
                iTmp = 0; 
                iRet = CcbUpdateSwitch(pWorkItem->id,0,ulRunMask,iTmp);
                if (iRet )
                {
                    LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);
                    
                    bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
                    return ;
                }
                
                /* 1. ui promote */
                bInit ? work_init_run_succ(pWorkItem->id): work_normal_run_succ(pWorkItem->id);        
                return;
            }     

            /* 2018/01/05 add extra 10 seconds for flush according to ZHANG Chunhe */
            if(haveB3())
            {
                iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E2_NO)|(1<<APP_EXE_E3_NO)|(1<<APP_EXE_C3_NO);
            }
            else
            {
                iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E2_NO)|(1<<APP_EXE_E3_NO)
                        |(1 << APP_EXE_E10_NO)|(1<<APP_EXE_C3_NO);
            }
            
            iRet = CcbUpdateSwitch(pWorkItem->id,0,ulRunMask,iTmp);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet); 
        
                bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
                
                /* notify ui (late implemnt) */
                return ;
            }

            iRet = CcbWorkDelayEntry(pWorkItem->id,10000,CcbDelayCallBack);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);  
                
                bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
                
                return;
            }

            bit3RuningState = NOT_RUNING_STATE_CLEAN;
            CcbNotState(NOT_STATE_OTHER);
                        
            /* E1,E3 ON*/
            LOG(VOS_LOG_WARNING,"E1,E3 ON"); 
            
            /* set  valves */
            if(haveB3())
            {
                iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E3_NO)|(1<<APP_EXE_C3_NO);
            }
            else
            {
                iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E3_NO)|(1 << APP_EXE_E10_NO)|(1 << APP_EXE_C3_NO);
            }

            iRet = CcbUpdateSwitch(pWorkItem->id,0,ulRunMask,iTmp);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);    
                /* notify ui (late implemnt) */
                bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
                return ;
            }
            
            for (iLoop = 0; iLoop < 10; iLoop++)
            {
                /* get B1 reports from exe */
                iRet = CcbGetPmValue(pWorkItem->id,APP_EXE_PM1_NO,1);
                if (iRet )
                {
                    LOG(VOS_LOG_WARNING,"CcbGetPmValue Fail %d",iRet);    
                    /* notify ui (late implemnt) */
                    bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
                    return ;
                }    
        
                CcbNotSWPressure();
        
                iRet = CcbWorkDelayEntry(pWorkItem->id,1000,CcbDelayCallBack);
                if (iRet )
                {
                    LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);  
                    
                    bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
                    
                    return;
                }
        
                
            }
        
            if (CcbConvert2Pm1Data(ExeBrd.aPMObjs[APP_EXE_PM1_NO].Value.ulV) < CcbGetSp1())
            {
                LOG(VOS_LOG_WARNING,"Low pressure PM0 %d",ExeBrd.aPMObjs[APP_EXE_PM1_NO].Value.ulV);  
            
                /* 1. ui promote */
                iTmp = 0; 
                iRet = CcbUpdateSwitch(pWorkItem->id,0,ulRunMask,iTmp);
                if (iRet )
                {
                    LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);
        
                    bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
                    return;
                }
        
                /* fire alarm */
                if (!bit1AlarmTapInPress)
                {
                    bit1AlarmTapInPress   = TRUE;
            
                    ulFiredAlarmFlags |= ALARM_TLP;
            
                    CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_SOURCE_WATER_PRESSURE,TRUE);
                }
            
                CanCcbTransState(DISP_WORK_STATE_LPP,DISP_WORK_SUB_IDLE);        
                
                return;
            }

             /* 2018/01/05 begin : add for B1 under pressure check according to ZHANG chunhe */
             bit1B1Check4RuningState  = TRUE;  
             /* 2018/01/05 end : add for B1 under pressure check */
        
             /* clear alarm */
            if (bit1AlarmTapInPress)
            {
                bit1AlarmTapInPress   = FALSE;
           
                ulFiredAlarmFlags &= ~ALARM_TLP;
           
                CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_SOURCE_WATER_PRESSURE,FALSE);
            }
        
            LOG(VOS_LOG_WARNING,"E1,E3,C1 ON; I1b,I2,B1"); 
        
            /*E1,E3,C1 ON; I1b,I2,B1*/
            if(haveB3())
            {
                iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E3_NO)|(1<<APP_EXE_C1_NO)|(1<<APP_EXE_C3_NO);
            }
            else
            {
                iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E3_NO)|(1<<APP_EXE_C1_NO)|(1 << APP_EXE_E10_NO)|(1 << APP_EXE_C3_NO);
            }
            iRet = CcbUpdateSwitch(pWorkItem->id,0,ulRunMask,iTmp);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
                /* notify ui (late implemnt) */
                bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
                return ;
            }
            
            iTmp = (1 << APP_EXE_I1_NO)|(1 << APP_EXE_I2_NO)|(GET_B_MASK(APP_EXE_PM1_NO));
            iRet = CcbUpdateIAndBs(pWorkItem->id,0,iTmp,iTmp);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
                /* notify ui (late implemnt) */
                bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
                return ;
            }
            
            iRet = CcbWorkDelayEntry(pWorkItem->id,3000,CcbDelayCallBack);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
                /* notify ui (late implemnt) */
                bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
                return ;
            }
        
            /* check data */
            {
                {
                    int iValidCount = 0;
                    
                    float fRej ;
                    
                    /* check appromixly 5*60 seconds */
                    for (iLoop = 0; iLoop < DEFAULT_REG_CHECK_DURATION / DEFAULT_REJ_CHECK_PERIOD; iLoop++)                                    {
                        iRet = CcbWorkDelayEntry(pWorkItem->id,DEFAULT_REJ_CHECK_PERIOD*1000,CcbDelayCallBack);
                        if (iRet )
                        {
                            LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
                            /* notify ui (late implemnt) */
                            bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);                
                            return ;
                        }  
                
                        fRej = CcbCalcREJ();
                
                        if (fRej >= CcbGetSp2() 
                            && ExeBrd.aEcoObjs[APP_EXE_I2_NO].Value.eV.fWaterQ < CcbGetSp3())
                        {
                            iValidCount ++;
                
                            if (iValidCount >= DEFAULT_REJ_CHECK_NUMBER)
                            {
                                break;
                            }
                        }
                        else
                        {
                            iValidCount = 0;
                        }
                    }
                
                    if (fRej < CcbGetSp2())
                    {
                        /*alarem */
                        bit1AlarmRej = TRUE;
                        ulAlarmRejTick = gulSecond;
                    }
                
                    if (ExeBrd.aEcoObjs[APP_EXE_I2_NO].Value.eV.fWaterQ > CcbGetSp3())
                    {
                        bit1AlarmRoPw   = TRUE;
                        ulAlarmRoPwTick = gulSecond;
                    }
                }

                bit3RuningState = NOT_RUNING_STATE_NONE;
                CcbNotState(NOT_STATE_OTHER);
        
                LOG(VOS_LOG_WARNING,"E1,C1,T,N1 ON I1b,I2,I3,B1,B2,S2"); 
                /* produce water */
                /* E1,C1,T,N1 ON I1b,I2,I3,B1,B2 */
                if(haveB3())
                {
                    iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_C1_NO)|(1<<APP_EXE_C3_NO);
                }
                else
                {
                    iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_C1_NO)|(1 << APP_EXE_E10_NO)|(1<<APP_EXE_C3_NO);
                }

                iTmp |=(1 << APP_EXE_T1_NO)|(1 << APP_EXE_N1_NO); 
                iRet = CcbUpdateSwitch(pWorkItem->id,0,ulRunMask,iTmp);
                if (iRet )
                {
                    LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);    
                    /* notify ui (late implemnt) */
                    bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
                    return ;
                }
         
                iTmp  = (1 << APP_EXE_I1_NO)|(1<<APP_EXE_I2_NO)|(1<<APP_EXE_I3_NO);
                iTmp |= (GET_B_MASK(APP_EXE_PM1_NO))|(GET_B_MASK(APP_EXE_PM2_NO));
                iRet = CcbUpdateIAndBs(pWorkItem->id,0,iTmp,iTmp);
                if (iRet )
                {
                    LOG(VOS_LOG_WARNING,"CcbSetIAndBs Fail %d",iRet);    
                    /* notify ui (late implemnt) */
                    bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
                    return ;
                }      
         
                /* wait a moment for I3 check */
                iRet = CcbWorkDelayEntry(pWorkItem->id,3000,CcbDelayCallBack); 
                if (iRet )
                {
                    LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);  
                    
                    /* notify ui (late implemnt) */
                    bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);   
                    
                    return ;
                }  
        
                if (!(ExeBrd.aEcoObjs[APP_EXE_I3_NO].Value.eV.fWaterQ >= CcbGetSp4()))
                {
                    /*alarem */
                    bit1AlarmEDI   = TRUE;
                    ulAlarmEDITick = gulSecond;
                }
            }    
        }
        break;
    case MACHINE_UP:
    case MACHINE_RO:    
    case MACHINE_RO_H:
    case MACHINE_C:
        /* get B2 reports from exe */
        iRet = CcbGetPmValue(pWorkItem->id,APP_EXE_PM2_NO,1);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbGetPmValue Fail %d",iRet);    
            /* notify ui (late implemnt) */
            bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);   
            
            return ;
        }    
    
        if (CcbConvert2Pm2SP(ExeBrd.aPMObjs[APP_EXE_PM2_NO].Value.ulV) >= B2_FULL)
        {
            CanPrepare4Pm2Full();
            
            /* close all switchs */    
            iTmp = 0; 
            iRet = CcbUpdateSwitch(pWorkItem->id,0,ulRunMask,iTmp);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);
                
                bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
                return ;
            }
            
            /* 1. ui promote */
            bInit ? work_init_run_succ(pWorkItem->id): work_normal_run_succ(pWorkItem->id);        
            return;
        }    

        /* 2018/01/05 add extra 10 seconds for flush according to ZHANG Chunhe */
        /* 2018/01/05 add E3 according to ZHANG */
        if(haveB3())
        {
            iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E2_NO)|(1<<APP_EXE_E3_NO)|(1<<APP_EXE_C3_NO);
        }
        else
        {
            iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E2_NO)|(1<<APP_EXE_E3_NO)
                    |(1 << APP_EXE_E10_NO)|(1<<APP_EXE_C3_NO);
        }
        
        iRet = CcbUpdateSwitch(pWorkItem->id,0,ulRunMask,iTmp);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet); 
    
            bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
            
            /* notify ui (late implemnt) */
            return ;
        }

        iRet = CcbWorkDelayEntry(pWorkItem->id,10000,CcbDelayCallBack);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);  
            
            bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
            
            return;
        }
        bit3RuningState = NOT_RUNING_STATE_CLEAN;
        
        CcbNotState(NOT_STATE_OTHER);
                
        /* E1,C3 ON*/
        LOG(VOS_LOG_WARNING,"E1,E3 ON"); 
        
        /* set  valves */
        if(haveB3())
        {
            iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E3_NO)|(1<<APP_EXE_C3_NO);
        }
        else
        {
            iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E3_NO)|(1 << APP_EXE_E10_NO)|(1 << APP_EXE_C3_NO);
        }
        iRet = CcbUpdateSwitch(pWorkItem->id,0,ulRunMask,iTmp);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);    
            /* notify ui (late implemnt) */
            bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
            return ;
        }
        
        for (iLoop = 0; iLoop < 10; iLoop++)
        {
            /* get B1 reports from exe */
            iRet = CcbGetPmValue(pWorkItem->id,APP_EXE_PM1_NO,1);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbGetPmValue Fail %d",iRet);    
                /* notify ui (late implemnt) */
                bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
                return ;
            }    
    
            CcbNotSWPressure();
    
            iRet = CcbWorkDelayEntry(pWorkItem->id,1000,CcbDelayCallBack);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);  
                
                bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
                
                return;
            }
    
        }
    
        if (CcbConvert2Pm1Data(ExeBrd.aPMObjs[APP_EXE_PM1_NO].Value.ulV) < CcbGetSp1())
        {
            /* 1. ui promote */
            iTmp = 0; 
            iRet = CcbUpdateSwitch(pWorkItem->id,0,ulRunMask,iTmp);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);
    
                bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
                return;
            }
    
            /* fire alarm */
            if (!bit1AlarmTapInPress)
            {
                bit1AlarmTapInPress   = TRUE;
        
                ulFiredAlarmFlags |= ALARM_TLP;
        
                CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_SOURCE_WATER_PRESSURE,TRUE);
            }
        
            CanCcbTransState(DISP_WORK_STATE_LPP,DISP_WORK_SUB_IDLE);        
            
            return;
        }

         /* 2018/01/05 begin : add for B1 under pressure check according to ZHANG chunhe */
         bit1B1Check4RuningState  = TRUE;  
         /* 2018/01/05 end : add for B1 under pressure check */
    
         /* clear alarm */
        if (bit1AlarmTapInPress)
        {
            bit1AlarmTapInPress   = FALSE;
       
            ulFiredAlarmFlags &= ~ALARM_TLP;
       
            CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_SOURCE_WATER_PRESSURE,FALSE);
        }
    
        LOG(VOS_LOG_WARNING,"E1,E3,C1 ON; I1b,I2,B1"); 
    
        /*E1,E3,C1 ON; I1b,I2,B1*/
        if(haveB3())
        {
            iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E3_NO)|(1<<APP_EXE_C1_NO)|(1<<APP_EXE_C3_NO);
        }
        else
        {
            iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E3_NO)|(1<<APP_EXE_C1_NO)
                    |(1 << APP_EXE_E10_NO)|(1 << APP_EXE_C3_NO);
        }
        iRet = CcbUpdateSwitch(pWorkItem->id,0,ulRunMask,iTmp);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
            /* notify ui (late implemnt) */
            bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
            return ;
        }
        
        iTmp = (1 << APP_EXE_I1_NO)|(1 << APP_EXE_I2_NO)|(GET_B_MASK(APP_EXE_PM1_NO));
        iRet = CcbUpdateIAndBs(pWorkItem->id,0,iTmp,iTmp);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
            /* notify ui (late implemnt) */
            bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
            return ;
        }
        
        iRet = CcbWorkDelayEntry(pWorkItem->id,3000,CcbDelayCallBack);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
            /* notify ui (late implemnt) */
            bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
            return ;
        }
    
        /* check data */
        {
            {
                int iValidCount = 0;
                
                float fRej ;
                
                /* check appromixly 5*60 seconds */
                for (iLoop = 0; iLoop < DEFAULT_REG_CHECK_DURATION / DEFAULT_REJ_CHECK_PERIOD; iLoop++)                                {
                    iRet = CcbWorkDelayEntry(pWorkItem->id,DEFAULT_REJ_CHECK_PERIOD*1000,CcbDelayCallBack);
                    if (iRet )
                    {
                        LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
                        /* notify ui (late implemnt) */
                        bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);                
                        return ;
                    }  
            
                    fRej = CcbCalcREJ();
            
                    if (fRej >= CcbGetSp2() 
                        && ExeBrd.aEcoObjs[APP_EXE_I2_NO].Value.eV.fWaterQ < CcbGetSp3())
                    {
                        iValidCount ++;
            
                        if (iValidCount >= DEFAULT_REJ_CHECK_NUMBER)
                        {
                            break;
                        }
                    }
                    else
                    {
                        iValidCount = 0;
                    }
                }
            
                if (fRej < CcbGetSp2())
                {
                    /*alarem */
                    bit1AlarmRej = TRUE;
                    ulAlarmRejTick = gulSecond;
                }
            
                if (ExeBrd.aEcoObjs[APP_EXE_I2_NO].Value.eV.fWaterQ > CcbGetSp3())
                {
                    bit1AlarmRoPw   = TRUE;
                    ulAlarmRoPwTick = gulSecond;
                }
                
            }

            bit3RuningState = NOT_RUNING_STATE_NONE;
            CcbNotState(NOT_STATE_OTHER);
    
            LOG(VOS_LOG_WARNING,"E1,C1 ON I1b,I2,B1,B2"); 
            /* produce water */
            /* E1,C1 ON I1b,I2,B1,B2 */
            if(haveB3())
            {
                iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_C1_NO)|(1<<APP_EXE_C3_NO);
            }
            else
            {
                iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_C1_NO)|(1 << APP_EXE_E10_NO)|(1<<APP_EXE_C3_NO);
            }
            iRet = CcbUpdateSwitch(pWorkItem->id,0,ulRunMask,iTmp);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);    
                /* notify ui (late implemnt) */
                bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
                return ;
            }
     
            iTmp  = (1 << APP_EXE_I1_NO)|(1<<APP_EXE_I2_NO);
            iTmp |= (GET_B_MASK(APP_EXE_PM1_NO))|(GET_B_MASK(APP_EXE_PM2_NO));
            iRet = CcbUpdateIAndBs(pWorkItem->id,0,iTmp,iTmp);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbSetIAndBs Fail %d",iRet);    
                /* notify ui (late implemnt) */
                bInit ? work_init_run_fail(pWorkItem->id): work_normal_run_fail(pWorkItem->id);        
                return ;
            }
        }                
        break;
    case MACHINE_PURIST:
    case MACHINE_C_D:
        break;
    }

    bit1ProduceWater = TRUE;

    bInit ? work_init_run_succ(pWorkItem->id): work_normal_run_succ(pWorkItem->id);

}

//设备运行冲洗过程
void CCB::work_init_run_wrapper(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;

    int iTmp;
    int iRet;
    int iLoop;

    /* notify ui (late implemnt) */
    bit1AlarmRej     = FALSE;
    bit1ProduceWater = FALSE;
    bit1LeakKey4Reset= FALSE;
    iInitRunTimer    = 0;
    bit1B1Check4RuningState = FALSE;
    bit1B1UnderPressureDetected = FALSE;

    /* 2018/01/15 add begin*/
    bit1AlarmROPHV    = FALSE;
    bit1AlarmROPLV    = FALSE;
    bit1AlarmRODV     = FALSE;
    ulRopVelocity     = 0;
    ulLstRopFlow      = INVALID_FM_VALUE;
    ulLstRopTick      = 0;
    iRopVCheckLowEventCount   = 0;
    iRopVCheckLowRestoreCount = 0;
    
    ulRodVelocity    = 0;
    ulLstRodFlow     = INVALID_FM_VALUE;
    ulLstRodTick     = 0;
    iRoDVCheckEventCount   = 0;
    iRoDVCheckRestoreCount = 0;
    /* 2018/01/15 add end*/

    if (!bit1InitRuned)
    {
        bit1InitRuned = TRUE;
        
        CanCcbTransState4Pw(DISP_WORK_STATE_RUN,DISP_WORK_SUB_IDLE);
    }

    CanCcbTransState(DISP_WORK_STATE_PREPARE,DISP_WORK_SUB_IDLE);

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_RO_H:
    case MACHINE_C:
        {
            iRet = CcbUpdateIAndBs(pWorkItem->id, 0, ulPMMask, ulPMMask);    
            if (iRet )
            {
                qDebug() << QString("CcbUpdatePmObjState Fail %1").arg(iRet);
                LOG(VOS_LOG_WARNING,"CcbUpdatePmObjState Fail %d",iRet);    
        
                /* notify ui (late implemnt) */
                work_init_run_fail(pWorkItem->id);
                
                return ;
            }

            qDebug() << QString("Flush for Init Run");
            LOG(VOS_LOG_WARNING,"Flush for Init Run"); 
    
            /* 运行冲洗 */
            /* E1,E2,E3 ON*/
            /* set  valves */
            if(haveB3())
            {
                iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E2_NO)|(1 << APP_EXE_E3_NO)|(1<<APP_EXE_C3_NO);
            }
            else
            {
                iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E2_NO)|(1 << APP_EXE_E3_NO)
                        |(1 << APP_EXE_E10_NO)|(1 << APP_EXE_C3_NO);
            }
            
            iRet = CcbSetSwitch(pWorkItem->id, 0, iTmp);
            if (iRet )
            {
                qDebug() << QString("CcbSetSwitch Fail %d").arg(iRet);
                LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet); 
        
                work_init_run_fail(pWorkItem->id);  
                
                /* notify ui (late implemnt) */
                return ;
            }
        
            /* enable I1 reports */
            iTmp = 1 << APP_EXE_I1_NO;
            iRet = CcbUpdateIAndBs(pWorkItem->id,0,iTmp,iTmp);
            if (iRet )
            {
                qDebug() << QString("CcbSetIAndBs Fail %1").arg(iRet);
                LOG(VOS_LOG_WARNING,"CcbSetIAndBs Fail %d",iRet);   
        
                work_init_run_fail(pWorkItem->id);  
                
                /* notify ui (late implemnt) */
                return ;
            }
    
            int iInitFlushTime = gGlobalParam.MiscParam.iPowerOnFlushTime*60*1000;
    
            bit3RuningState = NOT_RUNING_STATE_FLUSH;
    
            CcbNotState(NOT_STATE_OTHER);
    
            //新P Pack冲洗20min，否则按设置冲洗时间冲洗
            if(g_bNewPPack)
            {
                g_bNewPPack = 0;
                iInitFlushTime = 20*60*1000;
            }

            LOG(VOS_LOG_WARNING,"iPowerOnFlushTime %d",iInitFlushTime/(60*1000));    
            qDebug() << QString("iPowerOnFlushTime %1").arg(iInitFlushTime/(60*1000));
            
            iRet = CcbWorkDelayEntry(pWorkItem->id,
                                     iInitFlushTime,
                                     CcbDelayCallBack);
    
            if (iRet )
            {
                qDebug() << QString("CcbWorkDelayEntry Fail %1").arg(iRet);
                LOG(VOS_LOG_WARNING,"CcbWorkDelayEntry Fail %d",iRet);    
                work_init_run_fail(pWorkItem->id);  
                /* notify ui (late implemnt) */
                return ;
            }     
        }
        break;
    case MACHINE_PURIST:
    case MACHINE_C_D:
        break;
    }
    //冲洗完成，进入运行状态
    CanCcbTransState(DISP_WORK_STATE_RUN,DISP_WORK_SUB_RUN_INIT);
    /* come to normal RUN proc*/
    work_run_comm_proc(pWorkItem, true);
}

void CCB::work_init_run(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;
    CCB *pCcb = (CCB *)pWorkItem->para;

    pCcb->work_init_run_wrapper(para);
}

void CCB::work_tank_start_full_loop(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;

    CCB *pCcb = (CCB *)pWorkItem->para;

    int iTmp;

    int iRet;

    /*E1,E2,C1,C3 ON*/
    if(haveB3())
    {
        iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E2_NO)|(1<<APP_EXE_C1_NO)|(1<<APP_EXE_C3_NO);
    }
    else
    {
        iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E2_NO)|(1<<APP_EXE_C1_NO)|(1 << APP_EXE_E10_NO)|(1<<APP_EXE_C3_NO);
    }

    iRet = pCcb->CcbUpdateSwitch(pWorkItem->id,0,iTmp,iTmp);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);    
        /* notify ui (late implemnt) */
        return ;
    }

    pCcb->bit1PeriodFlushing = TRUE;

    /* update for next round full tank loop */
    pCcb->ulB2FullTick = gulSecond; 

    pCcb->CcbNotState(NOT_STATE_OTHER);

}


DISPHANDLE CCB::CcbInnerWorkStartTankFullLoop()
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc = work_tank_start_full_loop;
    pWorkItem->para = this;

    CcbAddWork(WORK_LIST_LP,pWorkItem);

    return (DISPHANDLE)pWorkItem;

}

void CCB::work_tank_stop_full_loop(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;

    CCB *pCcb = (CCB *)pWorkItem->para;

    int iTmp;

    int iRet;

    /*E1,E2,C1,C3 OFF */
    if(haveB3())
    {
        iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E2_NO)|(1<<APP_EXE_C1_NO)|(1<<APP_EXE_C3_NO);
    }
    else
    {
        iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E2_NO)|(1<<APP_EXE_C1_NO)|(1 << APP_EXE_E10_NO)|(1<<APP_EXE_C3_NO);
    }

    iRet = pCcb->CcbUpdateSwitch(pWorkItem->id,0,iTmp,0);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);    
        /* notify ui (late implemnt) */
    }

    pCcb->bit1PeriodFlushing = FALSE;

    pCcb->CcbNotState(NOT_STATE_OTHER);
}

DISPHANDLE CCB::CcbInnerWorkStopTankFullLoop()
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc = work_tank_stop_full_loop;
    pWorkItem->para = this;

    CcbAddWork(WORK_LIST_LP,pWorkItem);

    return (DISPHANDLE)pWorkItem;

}

DISPHANDLE CCB::CcbInnerWorkInitRun(void)
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc = work_init_run;
    pWorkItem->para = this;

    CcbAddWork(WORK_LIST_LP,pWorkItem);

    return (DISPHANDLE)pWorkItem;
}


void CCB::work_rpt_setup_exe(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;

    CCB *pCcb = (CCB *)pWorkItem->para;

    int iRet;

    LOG(VOS_LOG_WARNING,"%s : %x & %x",__FUNCTION__,pCcb->WorkRptSetParam4Exe.ulMask,pCcb->WorkRptSetParam4Exe.ulValue);    

    iRet = pCcb->CcbUpdateIAndBs(pWorkItem->id,0,pCcb->WorkRptSetParam4Exe.ulMask,pCcb->WorkRptSetParam4Exe.ulValue);    

    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbUpdateIAndBs Fail %d",iRet);    
        /* notify ui (late implemnt) */
        return ;
    }  
}

void CCB::work_rpt_setup_fm(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;

    CCB *pCcb = (CCB *)pWorkItem->para;

    int iRet;
    LOG(VOS_LOG_WARNING,"%s: %x & %x",__FUNCTION__,pCcb->WorkRptSetParam4Fm.ulMask,pCcb->WorkRptSetParam4Fm.ulValue);    

    iRet = pCcb->CcbUpdateFms(pWorkItem->id,0,pCcb->WorkRptSetParam4Fm.ulMask,pCcb->WorkRptSetParam4Fm.ulValue);    

    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbUpdateFms Fail %d",iRet);    
        /* notify ui (late implemnt) */
        return ;
    }  
}



void CCB::work_switch_setup_exe(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;

    CCB *pCcb = (CCB *)pWorkItem->para;

    int iRet;
    
    LOG(VOS_LOG_WARNING,"%s : %x & %x",__FUNCTION__,pCcb->WorkSwitchSetParam4Exe.ulMask,pCcb->WorkSwitchSetParam4Exe.ulValue);    

    iRet = pCcb->CcbUpdateSwitch(pWorkItem->id,0,pCcb->WorkSwitchSetParam4Exe.ulMask,pCcb->WorkSwitchSetParam4Exe.ulValue);    

    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);    
        /* notify ui (late implemnt) */
        return ;
    }  
}


DISPHANDLE CCB::CcbInnerWorkSetupExeReport(void)
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc = work_rpt_setup_exe;
    pWorkItem->para = this;

    CcbAddWork(WORK_LIST_LP,pWorkItem);

    return (DISPHANDLE)pWorkItem;
}



DISPHANDLE CCB::CcbInnerWorkSetupFmReport(void)
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc = work_rpt_setup_fm;
    pWorkItem->para = this;

    CcbAddWork(WORK_LIST_LP,pWorkItem);

    return (DISPHANDLE)pWorkItem;
}

DISPHANDLE CCB::CcbInnerWorkSetupExeSwitch(void)
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc = work_switch_setup_exe;
    pWorkItem->para = this;

    CcbAddWork(WORK_LIST_LP,pWorkItem);

    return (DISPHANDLE)pWorkItem;
}

void CCB::work_idle_succ(void)
{
    int aiCont[1] = {0};
    
    MainSndWorkMsg(WORK_MSG_TYPE_IDLE,(unsigned char *)aiCont,sizeof(aiCont));
    
}

void CCB::work_idle(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;

    CCB *pCcb = (CCB *)pWorkItem->para;

    int iTmp;

    LOG(VOS_LOG_WARNING,"work_idle");    

    pCcb->bit1InitRuned = FALSE;
    
    /* 1. close all valves */
    iTmp = 0;
    pCcb->CcbSetSwitch(pWorkItem->id,0,iTmp); // don care modbus exe result
    
    /* 2. disable all reports from exe */
    pCcb->CcbSetIAndBs(pWorkItem->id,0,iTmp); // don care modbus exe result
    
    /* 3. disable all reports from exe */
    pCcb->CcbSetFms(pWorkItem->id,0,iTmp); // don care modbus exe result

    //切换到待机状态时如果设备正在制水，则记录一次产水历史数据
    if(pCcb->DispGetPwFlag())
    {
        pCcb->CcbProduceWater(pCcb->ulProduceWaterBgnTime);

    }
  
    /* notify hand set (late implement) */     
    pCcb->work_idle_succ();
    /* notify ui (late implemnt) */
       
}

void CCB::work_wash_fail_comm(int iWorkId)
{
    /* close all switchs */
    if (CcbGetSwitchObjState(APP_EXE_SWITCHS_MASK))
    {
        CcbSetSwitch(iWorkId,0,0);
    }
    
    /*stop all report */
    if (CcbGetPmObjState(APP_EXE_PM_MASK)
        || CcbGetIObjState(APP_EXE_I_MASK))
    {
    
        CcbSetIAndBs(iWorkId,0,0);

        CcbUpdateIObjState(APP_EXE_I_MASK,0);
    }

    if (CcbGetFmObjState(APP_FM_FM_MASK))
    {
        CcbSetFms(iWorkId,0,0);
    }
}

void CCB::work_rowash_fail(int iType,int iWorkId)
{
    int aiCont[2] = {-1,iType};
    work_wash_fail_comm(iWorkId);
    MainSndWorkMsg(WORK_MSG_TYPE_WASH,(unsigned char *)aiCont,sizeof(aiCont));

}

void CCB::work_rowash_succ(int iType)
{
    int aiCont[2] = {0,iType};
    
    MainSndWorkMsg(WORK_MSG_TYPE_WASH,(unsigned char *)aiCont,sizeof(aiCont));

}

/**
 * [用于RO清洗过程中检测原水箱是否为空，原水箱则暂停RO清洗]
 * @param  id         [任务id]
 * @param  uiDuration [持续时间]
 * @param  uiMask     [负载掩码]
 * @param  uiSwitchs  [负载]
 * @return            [FALSE:出现异常; TRUE:运行正常]
 */
int CCB::checkB3DuringROWash(int id, unsigned int uiDuration,unsigned int uiMask, unsigned int uiSwitchs)
{
    int iRet;
    int iLoop;

    iRet = CcbUpdateSwitch(id, 0, uiMask, uiSwitchs);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING, "CcbSetSwitch Fail %d", iRet);          
        return FALSE;
    }

    for(iLoop = 0; iLoop < uiDuration / 1000;)
    {
        iRet = CcbWorkDelayEntry(id, 1000, CcbDelayCallBack);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbWorkDelayEntry Fail %d",iRet);         
            return FALSE;
        }

        if (CcbConvert2Pm3SP(ExeBrd.aPMObjs[APP_EXE_PM3_NO].Value.ulV) < CcbGetSp9())
        {
            iRet = CcbUpdateSwitch(id, 0, uiSwitchs, 0);
            if (iRet )
            {
                LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);
                return FALSE;
            }
            bit1ROWashPause = TRUE;
            bit1NeedFillTank = 1;
        } 
        else
        {
            if(bit1ROWashPause)
            {
                if (CcbConvert2Pm3SP(ExeBrd.aPMObjs[APP_EXE_PM3_NO].Value.ulV) > 50)
                {
                    iRet = CcbUpdateSwitch(id, 0, uiMask, uiSwitchs); // don care modbus exe result
                    if (iRet)
                    {
                        LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);          
                        return FALSE;
                    }
                    bit1ROWashPause = FALSE;
                    iLoop++;
                }
            }
            else
            {
                iLoop++;
            }
        }
    }
    return TRUE;
}

/**
 * RO清洗逻辑：
 * 第一步：进水阀、弃水阀、排放阀、原水泵、原水阀(根据配置，决定是否开启)工作13min
 * 第二步：进水阀、排放阀、RO增压泵、原水泵、原水阀(根据配置，决定是否开启)工作5min
 */
void CCB::work_idle_rowash_helper(WORK_ITEM_STRU *pWorkItem)
{
    int iTmp;
    unsigned int uiMask = APP_EXE_INNER_SWITCHS;
    if(haveB3())
    {
        uiMask &= ~(1 << APP_EXE_E10_NO);
    }

    int iRet;

    int iType = (int)pWorkItem->extra;

    iWashType = iType;
    bit1ROWashPause = FALSE;

    /* notify ui (late implemnt) */
    LOG(VOS_LOG_WARNING,"%s",__FUNCTION__);    

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_RO_H:
    case MACHINE_C:
        CanCcbTransState(DISP_WORK_STATE_IDLE,DISP_WORK_SUB_WASH);        
   
        // 第一步：进水阀、弃水阀、排放阀、原水泵、原水阀工作13min
        iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E2_NO)|(1<<APP_EXE_E3_NO)|(1 << APP_EXE_C3_NO)|(1 << APP_EXE_E10_NO);
        iRet = CcbSetSwitch(pWorkItem->id, 0, iTmp); // don care modbus exe result
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);    
            /* notify ui (late implemnt) */
            work_rowash_fail(iType,pWorkItem->id);        
            return ;
        }
        iRet = CcbWorkDelayEntry(pWorkItem->id, gROWashDuration[ROClWash_FirstStep], CcbDelayCallBack);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbWorkDelayEntry Fail %d",iRet);    
            /* notify ui (late implemnt) */
            work_rowash_fail(iType,pWorkItem->id);        
            return ;
        }
        
        // 第二步：进水阀、排放阀、RO增压泵、原水泵、原水阀工作5min
        iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E3_NO)|(1<<APP_EXE_C1_NO)|(1 << APP_EXE_C3_NO)|(1 << APP_EXE_E10_NO);
        iRet = CcbUpdateSwitch(pWorkItem->id, 0, uiMask, iTmp); // don care modbus exe result
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);    
           /* notify ui (late implemnt) */
           work_rowash_fail(iType,pWorkItem->id);        
           return ;
        }
        
        iRet = CcbWorkDelayEntry(pWorkItem->id, gROWashDuration[ROClWash_SecondStep], CcbDelayCallBack);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbWorkDelayEntry Fail %d",iRet);    
            /* notify ui (late implemnt) */
            work_rowash_fail(iType,pWorkItem->id);        
            return ;
        }
        
        /* close all valves */
        iTmp = 0;
        iRet = CcbSetSwitch(pWorkItem->id, 0, iTmp); // don care modbus exe result
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);    
            /* notify ui (late implemnt) */
            work_rowash_fail(iType,pWorkItem->id);        
            return ;
        }
        break;
    default:
        break; // invlaid value
    }
    /* notify ui (late implement) */
    work_rowash_succ(iType);  
}

void CCB::work_idle_rowash(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;

    CCB *pCcb = (CCB *)pWorkItem->para;

    pCcb->work_idle_rowash_helper(pWorkItem);
}

void CCB::work_idle_syswash(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;

    CCB *pCcb = (CCB *)pWorkItem->para;

    int iType = (int)pWorkItem->extra;

    pCcb->iWashType = iType;

    /* notify ui (late implemnt) */
    LOG(VOS_LOG_WARNING,"%s",__FUNCTION__);    

    /* notify ui (late implement) */
    pCcb->work_rowash_succ(iType);
}


void CCB::work_phwash_fail(int iType,int iWorkId)
{
    int aiCont[2] = {-1,iType};

    work_wash_fail_comm(iWorkId);
    
    MainSndWorkMsg(WORK_MSG_TYPE_WASH,(unsigned char *)aiCont,sizeof(aiCont));
}

void CCB::work_phwash_succ(int iType)
{
    int aiCont[2] = {0,iType};

    MainSndWorkMsg(WORK_MSG_TYPE_WASH,(unsigned char *)aiCont,sizeof(aiCont));

}

/**
 * SuperGenie PH清洗逻辑
 * 第一步：进水阀、排放阀、原水泵，持续35s 
 * 第二步：排放阀(浸泡负载不工作)，持续60min  
 * 第三步：进水阀、排放阀、RO增压泵、原水泵，持续40min
 *
 * 小机型 PH清洗逻辑
 * 第一步：进水阀、弃水阀、排放阀、原水泵，持续10s
 * 第二步：排放阀，RO增压泵，持续60min
 * 第三步：进水阀、弃水阀、排放阀、原水泵，持续10min
 * 第四步：进水阀、排放阀、RO增压泵、原水泵，持续10min 
 */
void CCB::work_idle_phwash_helper(WORK_ITEM_STRU *pWorkItem)
{

    int iTmp;
    unsigned int uiMask = APP_EXE_INNER_SWITCHS;
    if(haveB3())
    {
        uiMask &= ~(1 << APP_EXE_E10_NO);
    }

    int iRet;

    int iType = (int)pWorkItem->extra;

    iWashType = iType;

    /* notify ui (late implemnt) */
    LOG(VOS_LOG_WARNING,"%s",__FUNCTION__);    

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_RO_H:
    case MACHINE_C:
        CanCcbTransState(DISP_WORK_STATE_IDLE,DISP_WORK_SUB_WASH);   

        // 第一步：进水阀、弃水阀、排放阀、原水泵，持续10s
        iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E2_NO)|(1<<APP_EXE_E3_NO)|(1 << APP_EXE_C3_NO)|(1 << APP_EXE_E10_NO);
        iRet = CcbSetSwitch(pWorkItem->id, 0, iTmp); // don care modbus exe result
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbModbusWorkEntry Fail %d",iRet);    
            /* notify ui (late implemnt) */
            work_phwash_fail(iType,pWorkItem->id);        
            return ;
        }
        iRet = CcbWorkDelayEntry(pWorkItem->id, gROWashDuration[ROPHWash_FirstStep], CcbDelayCallBack);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbWorkDelayEntry Fail %d",iRet);    
            /* notify ui (late implemnt) */
            work_phwash_fail(iType,pWorkItem->id);        
            return ;
        }
        
        // 第二步：排放阀，RO增压泵，持续60min
        iTmp = (1<<APP_EXE_E3_NO)|(1<<APP_EXE_C1_NO);
        iRet = CcbUpdateSwitch(pWorkItem->id, 0, uiMask, iTmp); // don care modbus exe result
        if (iRet)
        {
            LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);    
            /* notify ui (late implemnt) */
            work_phwash_fail(iType,pWorkItem->id);        
            return ;
        }
        iRet = CcbWorkDelayEntry(pWorkItem->id, gROWashDuration[ROPHWash_SecondStep], CcbDelayCallBack);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbWorkDelayEntry Fail %d",iRet);    
            /* notify ui (late implemnt) */
            work_phwash_fail(iType,pWorkItem->id);        
            return ;
        }
  
        // 第三步：进水阀、弃水阀、排放阀、原水泵，持续10min
        iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E2_NO)|(1<<APP_EXE_E3_NO)|(1 << APP_EXE_C3_NO)|(1 << APP_EXE_E10_NO);
        iRet = CcbUpdateSwitch(pWorkItem->id, 0, uiMask, iTmp); // don care modbus exe result
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);    
           /* notify ui (late implemnt) */
           work_phwash_fail(iType,pWorkItem->id);        
           return ;
        }
        iRet = CcbWorkDelayEntry(pWorkItem->id, gROWashDuration[ROPHWash_ThirdStep], CcbDelayCallBack);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbWorkDelayEntry Fail %d",iRet);    
            /* notify ui (late implemnt) */
            work_phwash_fail(iType,pWorkItem->id);        
            return ;
        }
        
        // 第四步：进水阀、排放阀、RO增压泵、原水泵，持续10min
        iTmp = (1<<APP_EXE_E1_NO)|(1<<APP_EXE_E3_NO)|(1<<APP_EXE_C1_NO)|(1<<APP_EXE_C3_NO)|(1<<APP_EXE_E10_NO);
        iRet = CcbUpdateSwitch(pWorkItem->id, 0, uiMask, iTmp); // don care modbus exe result
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);    
           /* notify ui (late implemnt) */
           work_rowash_fail(iType,pWorkItem->id);        
           return ;
        }
        iRet = CcbWorkDelayEntry(pWorkItem->id, gROWashDuration[ROPHWash_FourthStep], CcbDelayCallBack);
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbWorkDelayEntry Fail %d",iRet);    
            /* notify ui (late implemnt) */
            work_phwash_fail(iType,pWorkItem->id);        
            return ;
        }

        /* close all valves */
        iTmp = 0;
        iRet = CcbSetSwitch(pWorkItem->id, 0, iTmp); // don care modbus exe result
        if (iRet )
        {
            LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);    
            /* notify ui (late implemnt) */
            work_phwash_fail(iType,pWorkItem->id);        
            return ;
        }

        break;
    default:
        break;
    }
    /* notify ui (late implement) */
    work_phwash_succ(iType);
}

void CCB::work_idle_phwash(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;

    CCB *pCcb = (CCB *)pWorkItem->para;

    pCcb->work_idle_phwash_helper(pWorkItem);
}

void CCB::work_start_lpp(void *para)
{
    int iTmp;
    int iRet;
    
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;

    CCB *pCcb = (CCB *)pWorkItem->para;

    iTmp = 0; 
    iRet = pCcb->CcbUpdateSwitch(pWorkItem->id,0,pCcb->ulRunMask,iTmp);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);
    }
    
    /* fire alarm */
   if (!pCcb->bit1AlarmWorkPressLow)
    {
        pCcb->bit1AlarmWorkPressLow   = TRUE;
    
        //gCcb.ulFiredAlarmFlags |= ALARM_TLP;
    
        pCcb->CcbNotAlarmFire(DISP_ALARM_PART1,DISP_ALARM_PART1_LOWER_WORK_PRESSURE,TRUE);
    }
        
    LOG(VOS_LOG_WARNING," DISP_WORK_STATE_LPP %d",iRet);  
    
    pCcb->CanCcbTransState(DISP_WORK_STATE_LPP,DISP_WORK_SUB_IDLE);
}

DISPHANDLE CCB::CcbInnerWorkLpp(void)
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc = work_start_lpp;
    pWorkItem->para = this;

    CcbAddWork(WORK_LIST_LP,pWorkItem);

    return (DISPHANDLE)pWorkItem;
}

DISPHANDLE CCB::DispCmdIdleProc()
{
    DISPHANDLE handle;

    if (DISP_WORK_STATE_IDLE == curWorkState.iMainWorkState
        && DISP_WORK_SUB_IDLE == curWorkState.iSubWorkState)
    {
        return DISP_ALREADY_HANDLE;
    }

    /* just put into work queue */
    handle = SearchWork(work_idle);

    if (!handle)
    {
        handle = CcbInnerWorkIdle();
    }
    return handle;
}

DISPHANDLE CCB::DispCmdWashProc(unsigned char *pucData, int iLength)
{
    DISP_CMD_WASH_STRU *pCmd = (DISP_CMD_WASH_STRU *)pucData;

    UNUSED(iLength);

    /* 2018/01/05 add stop logic to cleaning process */
    if (pCmd->iState)
    {

        if (DISP_WORK_STATE_IDLE == curWorkState.iMainWorkState
            && DISP_WORK_SUB_WASH == curWorkState.iSubWorkState
            && iWashType == pCmd->iType)
        {
            LOG(VOS_LOG_WARNING,"Performing Wash Type %d",iWashType);  
        
            return DISP_ALREADY_HANDLE;
        }

        if (DISP_WORK_STATE_IDLE != curWorkState.iMainWorkState
            || DISP_WORK_SUB_IDLE != curWorkState.iSubWorkState)
        {
            LOG(VOS_LOG_WARNING," Busy , current state %d & %d",curWorkState.iMainWorkState,curWorkState.iSubWorkState);  
        
            return DISP_INVALID_HANDLE;
        }

        LOG(VOS_LOG_WARNING,"preparing Wash Type %d",iWashType);  
        
        /* just put into work queue */
        {
            WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();
        
            if (!pWorkItem)
            {
                return DISP_INVALID_HANDLE;
            }
        
            switch(pCmd->iType)
            {
            case APP_CLEAN_RO:
                pWorkItem->proc = work_idle_rowash;
                break;
            case APP_CLEAN_PH:
                pWorkItem->proc = work_idle_phwash;
                break;
            /*
            case APP_CLEAN_SYSTEM:
                pWorkItem->proc = work_idle_syswash;
                break;
           */
            }
            pWorkItem->para = this;
            pWorkItem->extra = (void *)pCmd->iType;
        
            CcbAddWork(WORK_LIST_LP, pWorkItem);
        
            return (DISPHANDLE)pWorkItem;
        }
    }
    else
    {
        if (DISP_WORK_STATE_IDLE == curWorkState.iMainWorkState
            && DISP_WORK_SUB_WASH == curWorkState.iSubWorkState)
        {
            CcbCancelAllWork();
            CcbCleanup();
        }

        return DISP_SPECIAL_HANDLE;
    }
}


DISPHANDLE CCB::DispCmdResetProc(void)
{
    DISPHANDLE handle;

    if (DISP_WORK_STATE_LPP != curWorkState.iMainWorkState
        || DISP_WORK_SUB_IDLE != curWorkState.iSubWorkState)
    {
        return DISP_INVALID_HANDLE;
    }

    handle = SearchWork(work_init_run);

    if (!handle)
    {
        handle = CcbInnerWorkInitRun();
    }

    return handle;

}

//#define LEAK_RESET_RUN
DISPHANDLE CCB::DispCmdLeakResetProc()
{
    if (DISP_WORK_STATE_KP != curWorkState.iMainWorkState
        || DISP_WORK_SUB_IDLE != curWorkState.iSubWorkState)
    {
        return DISP_INVALID_HANDLE;
    }

    // CcbPopState();

    ucLeakWorkStates = 0; //2020.2.17 add for leak
    ExeBrd.ucLeakState = 0;
    
    ulKeyWorkStates   &= ~(1 << APP_EXE_DIN_LEAK_KEY) ;    
    ExeBrd.ucDinState &= ~(1 << APP_EXE_DIN_LEAK_KEY) ;   
    bit1LeakKey4Reset = FALSE;

    // WARNING: 2019.12.26修改需要测试
    CanCcbTransState(DISP_WORK_STATE_IDLE, DISP_WORK_SUB_IDLE);

#if 0 
    if (ExeBrd.ucDinState
        & APP_EXE_DIN_FAIL_MASK)
    {
        CanCcbDinProcess();
    }
        
    else
    {
        //CanCcbRestore();
        curWorkState.iMainWorkState = DISP_WORK_STATE_IDLE;
        curWorkState.iSubWorkState  = DISP_WORK_SUB_IDLE;

    }
#endif
    
    return DISP_SPECIAL_HANDLE;

}

/*********************************************************************
 * Function:        void work_start_tube_cir(void *para)
 *
 * PreCondition:    None
 *
 * Input:           para  type of WORK_ITEM_STRU 
 *
 * Output:          Start tube Ciculation
 *
 * Side Effects:    None
 *
 * Overview:        Start tube circulation
 *
 * Note:            None.
 ********************************************************************/
void CCB::work_start_tube_cir(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;
    CCB *pCcb = (CCB *)pWorkItem->para;

    int iTmp;
    int iRet;
    

    /* check water level */
    iRet = pCcb->CcbGetPmValue(pWorkItem->id,APP_EXE_PM2_NO,1);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbGetPmValue Fail %d",iRet);  

        return ;
    }

    if (pCcb->CcbConvert2Pm2SP(pCcb->ExeBrd.aPMObjs[APP_EXE_PM2_NO].Value.ulV) < pCcb->CcbGetSp6())
    {
        pCcb->bit1B2Empty = TRUE;
        
        return;
    }

    LOG(VOS_LOG_DEBUG,"work_start_tube_cir");    
    
    /* set  valves   */
    iTmp = (1 << APP_EXE_C4_NO);
    iRet = pCcb->CcbUpdateSwitch(pWorkItem->id,0,iTmp,iTmp);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbUpdateSwitch Fail %d",iRet);    
    }

    /* set  valves   */
    iTmp = 1 << APP_EXE_I4_NO;
    iRet = pCcb->CcbUpdateIAndBs(pWorkItem->id,0,iTmp,iTmp);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbUpdateIAndBs Fail %d",iRet);    
        /* notify ui (late implemnt) */
        return ;
    }

    pCcb->bit1TubeCirOngoing    = TRUE;
    pCcb->ulTubeIdleCirTick         = 0;
    pCcb->ulTubeIdleCirIntervalTick = 0;
}

DISPHANDLE CCB::CcbInnerWorkStartTubeCir(void)
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc = work_start_tube_cir;
    pWorkItem->para = this;

    CcbAddWork(WORK_LIST_URGENT,pWorkItem);

    return (DISPHANDLE)pWorkItem;

}


/*********************************************************************
 * Function:        void work_stop_tube_cir(void *para)
 *
 * PreCondition:    None
 *
 * Input:           para  type of WORK_ITEM_STRU 
 *
 * Output:          Stop producing water
 *
 * Side Effects:    None
 *
 * Overview:        Stop producing water
 *
 * Note:            None.
 ********************************************************************/
void CCB::work_stop_tube_cir(void *para)
{
    WORK_ITEM_STRU *pWorkItem = (WORK_ITEM_STRU *)para;
    CCB *pCcb = (CCB *)pWorkItem->para;

    int iTmp;
    int iRet;

    /* set  valves   */
    iTmp = (1 << APP_EXE_C4_NO);
    iRet = pCcb->CcbUpdateSwitch(pWorkItem->id,0,iTmp,0);
    if (iRet )
    {
        LOG(VOS_LOG_WARNING,"CcbSetSwitch Fail %d",iRet);    
    }
    
    pCcb->bit1TubeCirOngoing = FALSE;
}

DISPHANDLE CCB::CcbInnerWorkStopTubeCir(void)
{
    WORK_ITEM_STRU *pWorkItem = CcbAllocWorkItem();

    if (!pWorkItem)
    {
        return DISP_INVALID_HANDLE;
    }

    pWorkItem->proc = work_stop_tube_cir;
    pWorkItem->para = this;

    CcbAddWork(WORK_LIST_URGENT,pWorkItem);

    return (DISPHANDLE)pWorkItem;

}

/**
 * 启动分配循环前先检查纯水箱液位，启动条件：纯水箱液位高于(低液位 + 5.0)
 */
void CCB::CheckToStartAllocCir()
{
    if((getKeyState() & (1 << APP_EXE_DIN_LEAK_KEY)) ||  getLeakState())
    {
        LOG(VOS_LOG_DEBUG,"CheckToStartAllocCir Error: APP_EXE_DIN_LEAK_KEY %d&%d",getKeyState(),getLeakState()); 
        return;
    }
    
    if(CcbConvert2Pm2SP(ExeBrd.aPMObjs[APP_EXE_PM2_NO].Value.ulV) > (CcbGetSp6() + 5.0))
    {
        if (!SearchWork(work_start_tube_cir))
        {
            CcbInnerWorkStartTubeCir();
            LOG(VOS_LOG_DEBUG,"CcbInnerWorkStartTubeCir %d:%d:%d",iCirType,bit1NeedTubeCir,bit1TubeCirOngoing);    
        }  
    }
}

/**
 * 分配循环开启后，检测纯水箱液位，低液位时关闭分配循环
 */
void CCB::CheckToStopAllocCir()
{
    if((CcbConvert2Pm2SP(ExeBrd.aPMObjs[APP_EXE_PM2_NO].Value.ulV)  < CcbGetSp6())
       || (getKeyState() & (1 << APP_EXE_DIN_LEAK_KEY))
       ||  getLeakState())
    {
        if (!SearchWork(work_stop_tube_cir))
        {
            CcbInnerWorkStopTubeCir();
        } 
    }
}

/**
 * 在设置的分配时间段内，启动分配循环
 */
void CCB::TubeCirWithinSetTime()
{
    if (!bit1TubeCirOngoing)
    {
        ulTubeCirStartDelayTick++;
        if(ulTubeCirStartDelayTick >= 10)
        {
            CheckToStartAllocCir();
            ulTubeCirStartDelayTick = 0;
        }
        
    }
    else
    {
        CheckToStopAllocCir();
    }

}

/**
 * 在设置的分配控制时间段外，启用空闲分配控制
 */
void CCB::TubeCirWithoutSetTime()
{
    if (!bit1TubeCirOngoing)
    {
        ulTubeIdleCirIntervalTick++;
        if((ulTubeIdleCirIntervalTick >= (unsigned int)gGlobalParam.MiscParam.iTubeCirCycle * 60)
            && (gGlobalParam.MiscParam.iTubeCirDuration > 0))
        {
            CheckToStartAllocCir();
        }
    }
    else
    {
        ulTubeIdleCirTick++;
        if ((ulTubeIdleCirTick >= (unsigned int)gGlobalParam.MiscParam.iTubeCirDuration * 60)
            || (getKeyState() & (1 << APP_EXE_DIN_LEAK_KEY)) ||  getLeakState()) /* Minute to second */
        {
            if (!SearchWork(work_stop_tube_cir))
            {
                CcbInnerWorkStopTubeCir();
            }                      
        }

        CheckToStopAllocCir();
    }
}

void CCB::checkForTubeCir()
{
    // NOTE: 纯水分配控制
    if (bit1NeedTubeCir)
    {
        TubeCirWithinSetTime();    
    }  
    else
    {
        TubeCirWithoutSetTime();
    } 
}

DISPHANDLE CCB::DispCmdTubeCirProc(unsigned char *pucData, int iLength)
{
    DISP_CMD_TC_STRU *pTc = (DISP_CMD_TC_STRU *)pucData;
    (void)iLength;

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_RO_H:
    case MACHINE_C:
        return DISP_INVALID_HANDLE;
    default:
        return DISP_INVALID_HANDLE; 
    }
}

DISPHANDLE CCB::DispCmdRunProc()
{
    DISPHANDLE handle;

    //LOG(VOS_LOG_WARNING,"DispCmdRunProc Main & Sub %d %d",curWorkState.iMainWorkState, curWorkState.iSubWorkState);    
    qDebug() << QString("DispCmdRunProc Main %1 & Sub %2").arg(curWorkState.iMainWorkState).arg(curWorkState.iSubWorkState);
    
    if (DISP_WORK_STATE_IDLE != curWorkState.iMainWorkState
        || DISP_WORK_SUB_IDLE != curWorkState.iSubWorkState)
    {  
        return DISP_ALREADY_HANDLE;
    }

    //检查是否有柱子脱落
    if(!gpMainWnd->PackDetected())
    {
        qDebug() << QString("DispCmdRunProc Pack Not Detected");  
        return DISP_INVALID_HANDLE;
    }

    handle = SearchWork(work_init_run);
    if (!handle)
    {
        handle = CcbInnerWorkInitRun();
    }
    else
    {
        qDebug() << "DispCmdRunProc Have buffered Init Run";
        //LOG(VOS_LOG_WARNING,"DispCmdRunProc Have buffered Init Run");    
    }
    return handle;

}


void CCB::CcbCancelAllWork()
{
    int iLoop;

    sp_thread_mutex_lock(&WorkMutex);

    for (iLoop = 0; iLoop < WORK_LIST_NUM; iLoop++)
    {
        list_t *pos, *n;
        list_for_each_safe(pos,n,&WorkList[iLoop])
        {
            WORK_ITEM_STRU *pWorkItem = list_entry(pos,WORK_ITEM_STRU,list) ;
            {
                pWorkItem->flag |= WORK_FLAG_CANCEL; // append cancel flag

                if (pWorkItem->flag & WORK_FLAG_ACTIVE)
                {
                    if (pWorkItem->cancel) pWorkItem->cancel(pWorkItem);
                }
            }
        }
    }
    sp_thread_mutex_unlock(&WorkMutex);

}


DISPHANDLE CCB::CcbCancelWork(DISPHANDLE handle)
{
    int iLoop;

    sp_thread_mutex_lock(&WorkMutex);

    for (iLoop = 0; iLoop < WORK_LIST_NUM; iLoop++)
    {
        list_t *pos, *n;
        list_for_each_safe(pos,n,&WorkList[iLoop])
        {
            WORK_ITEM_STRU *pWorkItem = list_entry(pos,WORK_ITEM_STRU,list) ;

            if (((DISPHANDLE)pWorkItem) == handle)
            {
                pWorkItem->flag |= WORK_FLAG_CANCEL; // append cancel flag

                if (pWorkItem->flag & WORK_FLAG_ACTIVE)
                {
                    if (pWorkItem->cancel) pWorkItem->cancel(pWorkItem);
                }
                sp_thread_mutex_unlock(&WorkMutex);
                return handle;
            }
        }
    }
    sp_thread_mutex_unlock(&WorkMutex);

    return DISP_INVALID_HANDLE;

}

void CCB::CcbRmvWork(work_proc proc)
{
    int iLoop;

    sp_thread_mutex_lock(&WorkMutex);

    for (iLoop = 0; iLoop < WORK_LIST_NUM; iLoop++)
    {
        list_t *pos, *n;
        list_for_each_safe(pos,n,&WorkList[iLoop])
        {
            WORK_ITEM_STRU *pWorkItem = list_entry(pos,WORK_ITEM_STRU,list) ;

            if ((pWorkItem->proc) == proc)
            {
                pWorkItem->flag |= WORK_FLAG_CANCEL; // append cancel flag

                if (pWorkItem->flag & WORK_FLAG_ACTIVE)
                {
                    if (pWorkItem->cancel) pWorkItem->cancel(pWorkItem);
                }
            }
        }
    }
    sp_thread_mutex_unlock(&WorkMutex);

}



DISPHANDLE CCB::DispCmdCancelWork(unsigned char *pucData, int iLength)
{
    DISPHANDLE handle = (DISPHANDLE)(*((int *)pucData));

    if (iLength != sizeof(void *))
    {
        return DISP_INVALID_HANDLE;
    }


    return CcbCancelWork(handle); 
}

DISPHANDLE CCB::DispCmdHaltProc(void)
{
    DISPHANDLE handle;

    if(CcbGetTwFlag())
    {
        //CcbStopQtw();
        return NULL;
    }

    CcbCancelAllWork();

    handle = SearchWork(work_idle);

    if (!handle)
    {
        handle = CcbInnerWorkIdle();
    }  
    else
    {
        LOG(VOS_LOG_WARNING,"work_idle in queue: %d",iBusyWork);    
    }

    return handle;

}

DISPHANDLE CCB::DispCmdEngProc(unsigned char *pucData, int iLength)
{
    (void)iLength;

    if (!pucData[0])
    {
        bit1EngineerMode = FALSE;
       //return DISP_SPECIAL_HANDLE;
    }
    else
    {
        bit1EngineerMode = TRUE;
       /* cancel all work & return to idle */
       //return DispCmdHaltProc();
    }
    return DispCmdHaltProc();
}

void CCB::DispStartKeyQtw()
{
    if (DISP_WORK_STATE_RUN != curWorkState.iMainWorkState4Pw)
    {
        return;
    }
    else
    {
        if (CcbGetTwFlag() || CcbGetTwPendingFlag())
        {
            return;
        }
        else
        {
            QtwMeas.ulTotalFm = INVALID_FM_VALUE;
            CcbInnerWorkStartQtw(VIRTUAL_HANDLER);
        }
    }

}

void CCB::DispStopKeyQtw()
{
    if (aHandler[VIRTUAL_HANDLER].bit1Qtw)
    {
        CcbInnerWorkStopQtw(VIRTUAL_HANDLER);
    }
}

void CCB::DispKeyQtw()
{
    switch (gGlobalParam.iMachineType)  
    {
    case MACHINE_C_D:
        aHandler[VIRTUAL_HANDLER].iDevType = APP_DEV_HS_SUB_UP;
        break;
    case MACHINE_C:
        aHandler[VIRTUAL_HANDLER].iDevType = APP_DEV_HS_SUB_HP;
        break;
    default:
        aHandler[VIRTUAL_HANDLER].iDevType = APP_DEV_HS_SUB_HP;
        break;
    }
    //aHandler[VIRTUAL_HANDLER].iSpeed = PUMP_SPEED_10;
    
    if(aHandler[VIRTUAL_HANDLER].bit1Qtw)
    {
        if(!(ExeBrd.ucDinState & (1 << APP_EXE_DIN_IWP_KEY)))
        {
            DispStopKeyQtw();
        }
    }
    else
    {
        if(ExeBrd.ucDinState & (1 << APP_EXE_DIN_IWP_KEY))
        {
            DispStartKeyQtw();
        }
    }
}


DISPHANDLE CCB::DispCmdTw(unsigned char *pucData, int iLength)
{
    DISP_CMD_TW_STRU *pCmd = (DISP_CMD_TW_STRU *)pucData;
    (void)iLength;

    int iIdx    = CcbGetHandlerId2Index(pCmd->iDevId);

    int iTwFlag = CcbGetTwFlag();

    LOG(VOS_LOG_DEBUG,"qtw: %d & %d",curWorkState.iMainWorkState4Pw ,curWorkState.iSubWorkState4Pw);    
    

    if (DISP_WORK_STATE_RUN != curWorkState.iMainWorkState4Pw)
    {
        return DISP_INVALID_HANDLE;
    }

    switch(pCmd->iType)
    {
    case APP_DEV_HS_SUB_UP:
    case APP_DEV_HS_SUB_HP:
        if (CcbGetHandlerTwFlag(pCmd->iDevId))
        {
           /* try to stop */
           return CcbInnerWorkStopQtw(iIdx);
           
        }
        else if (iTwFlag) /* at least another one handler is busy */
        {
            return DISP_INVALID_HANDLE;
        }
        else 
        {
            int iPendingTwFlag = CcbGetTwPendingFlag();
            
            if (iPendingTwFlag )
            {
               if (iPendingTwFlag & (1 << iIdx))
               {
                   DISPHANDLE hdl ;
                    
                   hdl = SearchWork(work_start_qtw);
                    
                   if (hdl)
                   {
                       CcbCancelWork(hdl);

                       return hdl;
                   }
               }
               return DISP_INVALID_HANDLE;
            }

        }

        if (!CcbGetHandlerStatus(pCmd->iDevId))
        {
            LOG(VOS_LOG_DEBUG,"CTW: hs %d : invalid state",pCmd->iDevId);    
            return DISP_INVALID_HANDLE;
        }

        /* try to take water */
        if (CcbGetHandlerType(pCmd->iDevId) == pCmd->iType)
        {
            int iIndex   = (pCmd->iDevId - APP_HAND_SET_BEGIN_ADDRESS);

            if (aHandler[iIndex].iTrxMap & (1 << APP_TRX_CAN))
            {
                CanCcbSaveQtw2(APP_TRX_CAN,pCmd->iDevId,pCmd->iVolume);
            }
            else if (aHandler[iIndex].iTrxMap & (1 << APP_TRX_ZIGBEE))
            {
                CanCcbSaveQtw2(APP_TRX_ZIGBEE,pCmd->iDevId,pCmd->iVolume);
            }
            else
            {
                CanCcbSaveQtw2(APP_TRX_CAN,pCmd->iDevId,pCmd->iVolume);
            }
            
            /* remove cir work if any */
            CcbRmvWork(work_start_cir);
             
            /* construct work */
            return CcbInnerWorkStartQtw(CcbGetHandlerId2Index(pCmd->iDevId));
            
        }
        break;
    default:
        return DISP_INVALID_HANDLE;
    }

   return DISP_INVALID_HANDLE;
}

DISPHANDLE CCB::DispCmdCir(unsigned char *pucData, int iLength)
{
    DISP_CMD_CIR_STRU *pCmd = (DISP_CMD_CIR_STRU *)pucData;

    (void)iLength;

    LOG(VOS_LOG_DEBUG,"Cir: %d & %d",curWorkState.iMainWorkState4Pw ,curWorkState.iSubWorkState4Pw);    
    
    if (DISP_WORK_STATE_RUN != curWorkState.iMainWorkState4Pw)
    {
        return DISP_INVALID_HANDLE;
    }

   switch(pCmd->iType)
   {
   case CIR_TYPE_UP:
        if ((DISP_WORK_STATE_RUN == curWorkState.iMainWorkState4Pw)
            && (DISP_WORK_SUB_IDLE == curWorkState.iSubWorkState4Pw))
        {
           if (!SearchWork(work_start_cir))
           {
               return CcbInnerWorkStartCir(CIR_TYPE_UP);
           }
        }
        if ((DISP_WORK_STATE_RUN == curWorkState.iMainWorkState4Pw)
            && (DISP_WORK_SUB_RUN_CIR == curWorkState.iSubWorkState4Pw))
        {
           if (!SearchWork(work_stop_cir))
           {
               return CcbInnerWorkStopCir();
           }   
        }
        break;
   case CIR_TYPE_HP:
       switch(gGlobalParam.iMachineType)
       {
       case MACHINE_PURIST:
       case MACHINE_C_D:
           break;
       default:
           if ((DISP_WORK_STATE_RUN == curWorkState.iMainWorkState4Pw)
               && (DISP_WORK_SUB_IDLE == curWorkState.iSubWorkState4Pw))
           {
              if (haveHPCir())
              {
                  if (!SearchWork(work_start_cir))
                  {
                      return CcbInnerWorkStartCir(CIR_TYPE_HP);
                  }
              }
           }
           if ((DISP_WORK_STATE_RUN == curWorkState.iMainWorkState4Pw)
               && (DISP_WORK_SUB_RUN_CIR == curWorkState.iSubWorkState4Pw)
               && (CIR_TYPE_HP == iCirType))
           {
              if (!SearchWork(work_stop_cir))
              {
                  return CcbInnerWorkStopCir();
              }   
           }
           break;
        }
        break;
   }
   return DISP_INVALID_HANDLE;
}



DISPHANDLE CCB::DispCmdEngCmdProc(unsigned char *pucData, int iLength)
{
   int iRet = -1;
   (void)iLength;

   if (0 == (ulActiveMask & (1 << APP_DEV_TYPE_EXE_BOARD)))
   {
       return DISP_INVALID_HANDLE;
   }
   
   LOG(VOS_LOG_WARNING,"DispCmdEngCmdProc %d&%d&%d",pucData[0],pucData[1],pucData[2]);    
   
   /* do something here */
   switch(pucData[0])
   {
   case ENG_CMD_TYPE_SWITCH:
       iRet = CcbUpdateSwitch(WORK_LIST_NUM,0,(1 << pucData[1]),pucData[2] ? (1 << pucData[1]) : 0);
       break;
   case ENG_CMD_TYPE_IB:
       iRet = CcbUpdateIAndBs(WORK_LIST_NUM,0,(1 << pucData[1]),pucData[2] ? (1 << pucData[1]) : 0);
       break;
   case ENG_CMD_TYPE_FM:
       iRet = CcbUpdateFms(WORK_LIST_NUM,0,(1 << pucData[1]),pucData[2] ? (1 << pucData[1]) : 0);
       break;
   case ENG_CMD_TYPE_RPUMP:
       iRet = CcbSetExeHoldRegs(WORK_LIST_NUM,((pucData[1]<<8)|pucData[2]),((pucData[3]<<8)|pucData[4]));
       break;
   }
   
   return (-1 == iRet) ?  DISP_INVALID_HANDLE : DISP_SPECIAL_HANDLE;
}


DISPHANDLE CCB::DispCmdSwitchReport(unsigned char *pucData, int iLength)
{
    DISP_CMD_SWITCH_STATE_REPORT_STRU *pCmd = (DISP_CMD_SWITCH_STATE_REPORT_STRU *)pucData;
    (void)iLength;
    
   bit1SysMonitorStateRpt = !!pCmd->iRptFlag;

   return DISP_SPECIAL_HANDLE;
}

/**
 * @Author: dcj
 * @Date: 2021-01-26 12:44:28
 * @Description: 执行打开所有电磁阀的命令，请确保在待机状态下调用此函数
 * @param {unsignedchar} *pucData: 命令的附加参数
 * @param {int} iLength: 附件参数的长度
 */
DISPHANDLE CCB::DispCmdOpenValvesCmdProc(unsigned char *pucData, int iLength)
{
    int iRet = -1;
    int iTmp;
    (void)iLength;

    if (0 == (ulActiveMask & (1 << APP_DEV_TYPE_EXE_BOARD)))
    {
        return DISP_INVALID_HANDLE;
    }  

    if ( !(DISP_WORK_STATE_IDLE == curWorkState.iMainWorkState
        && DISP_WORK_SUB_IDLE   == curWorkState.iSubWorkState ))
    {
        return DISP_INVALID_HANDLE;
    }

    iTmp = (1 << APP_EXE_E1_NO)|(1<<APP_EXE_E2_NO)|(1<<APP_EXE_E3_NO)|(1<<APP_EXE_E4_NO)
          |(1 << APP_EXE_E5_NO)|(1<<APP_EXE_E6_NO)|(1<<APP_EXE_E9_NO)|(1<<APP_EXE_E10_NO);
    iRet = CcbUpdateSwitch(WORK_LIST_NUM,0, iTmp, pucData[0] ? iTmp : 0);
   
    return (-1 == iRet) ?  DISP_INVALID_HANDLE : DISP_SPECIAL_HANDLE;
}


DISPHANDLE CCB::DispCmdEntry(int iCmdId,unsigned char *pucData, int iLength)
{
    LOG(VOS_LOG_WARNING,"DispCmdEntry %s",m_WorkName[iCmdId]);    
    
    switch(iCmdId)
    {
    case DISP_CMD_IDLE:
        return bit1EngineerMode ? NULL : DispCmdIdleProc();
    case DISP_CMD_RUN:
        return  bit1EngineerMode ? NULL : DispCmdRunProc();
    case DISP_CMD_WASH:
        return  bit1EngineerMode ? NULL : DispCmdWashProc(pucData,iLength);
    case DISP_CMD_CANCEL_WORK:
        return  bit1EngineerMode ? NULL : DispCmdCancelWork(pucData,iLength);
    case DISP_CMD_RESET:
        return  bit1EngineerMode ? NULL : DispCmdResetProc();
    case DISP_CMD_LEAK_RESET:
        return bit1EngineerMode ? NULL : DispCmdLeakResetProc();
    case DISP_CMD_TUBE_CIR:
        return bit1EngineerMode ? NULL : DispCmdTubeCirProc(pucData,iLength);
    case DISP_CMD_HALT:
        return  bit1EngineerMode ? NULL : DispCmdHaltProc();
    case DISP_CMD_ENG_MODE:
        return DispCmdEngProc(pucData, iLength);
    case DISP_CMD_ENG_CMD:
        return /*!bit1EngineerMode ? NULL :*/ DispCmdEngCmdProc(pucData,iLength);
    case DISP_CMD_TW:
        return bit1EngineerMode ? NULL : DispCmdTw(pucData,iLength);
    case DISP_CMD_CIR:
        return bit1EngineerMode ? NULL : DispCmdCir(pucData,iLength);
    case DISP_CMD_SWITCH_REPORT:
        return bit1EngineerMode ? NULL : DispCmdSwitchReport(pucData,iLength);
    case DISP_CMD_OPENALLVALVES:
        return DispCmdOpenValvesCmdProc(pucData,iLength);
    }
    return DISP_INVALID_HANDLE;
}

int CCB::DispIapEntry(IAP_CAN_CMD_STRU *pIapCmd)
{
    CAN_BUILD_ADDRESS_IDENTIFIER(pIapCmd->ulCanId,APP_DEV_TYPE_MAIN_CTRL,pIapCmd->ulCanId);

    return CcbSndIapCanCmd(pIapCmd->iCanChl,pIapCmd->ulCanId,pIapCmd->ucCmd,(unsigned char *)pIapCmd->data,pIapCmd->iPayLoadLen);
}

int CCB::DispAfEntry(IAP_CAN_CMD_STRU *pIapCmd)
{
    CAN_BUILD_ADDRESS_IDENTIFIER(pIapCmd->ulCanId,APP_DEV_TYPE_MAIN_CTRL,pIapCmd->ulCanId);

    return CcbSndAfCanCmd(pIapCmd->iCanChl,pIapCmd->ulCanId,pIapCmd->ucCmd,(unsigned char *)pIapCmd->data,pIapCmd->iPayLoadLen);
}

int CCB::DispAfRfEntry(IAP_CAN_CMD_STRU *pIapCmd)
{
    if (aRfReader[pIapCmd->ulCanId].bit1RfDetected)
    {
        CAN_BUILD_ADDRESS_IDENTIFIER(pIapCmd->ulCanId,APP_DEV_TYPE_MAIN_CTRL,(APP_RF_READER_BEGIN_ADDRESS + pIapCmd->ulCanId));
        
        return CcbSndAfCanCmd(pIapCmd->iCanChl,pIapCmd->ulCanId,pIapCmd->ucCmd,(unsigned char *)pIapCmd->data,pIapCmd->iPayLoadLen);
    }

    return -1;
}

int CCB::DispGetWorkState()
{
    return curWorkState.iMainWorkState; 
}

int CCB::DispGetWorkState4Pw()
{
    return curWorkState.iMainWorkState4Pw; 
}

int CCB::DispGetSubWorkState4Pw()
{
    return curWorkState.iSubWorkState4Pw; 
}

void CCB::DispSetSubWorkState4Pw(int SubWorkState4Pw)
{
    curWorkState.iSubWorkState4Pw = SubWorkState4Pw; 
}


char *CCB::DispGetAscInfo(int idx)
{
    if (idx >= 2*WORK_MSG_TYPE_EFW + 1)
    {
        return "none";
    }

    return (char *)notify_info[idx];
}

int CCB::DispConvertVoltage2RPumpSpeed(float fv)
{
     int speed =(int) (((41.2/(fv/1.23 - 1))/10)*256);  /* 2018/03/07 */

     if (speed >= 255) speed = 255;
     if (speed < 0)    speed = 0;

     return speed;
}

int CCB::DispConvertRPumpSpeed2Voltage(int speed)
{
     int iVoltage;

     speed &= 0xff;

     if (0 == speed) return 24000;

     printf("speed = %d\r\n",speed);

     iVoltage =  (int)((1230*(1 + (41.2*256)/(speed*10)))); /* 2018/03/07 */

     if (iVoltage > 24000) return 24000;
     return iVoltage;
}


int CCB::DispConvertRPumpSpeed2Scale(int speed)
{
     int iIndex = PUMP_SPEED_NUM - 1;

     speed &= 0XFF;

     for (iIndex = 0; iIndex < PUMP_SPEED_NUM - 1; iIndex++)
     {
        if (speed >= aiSpeed2ScaleMap[iIndex])
        {
            return iIndex;
        }
     }

     return iIndex;
}

int CCB::DispReadRPumpSpeed(void)
{
    return CcbGetExeHoldRegs(WORK_LIST_NUM,APP_EXE_HOLD_REG_RPUMP_OFFSET,APP_EXE_HOLD_REG_RPUMP_NUM);
}

void CCB::DispGetRPumpSpeed(int iIndex ,int *pValue)
{
    ///MODBUS_PACKET_COMM_STRU *pmg = (MODBUS_PACKET_COMM_STRU *)aucModbusBuffer;

    *pValue = (ExeBrd.ausHoldRegister[APP_EXE_HOLD_REG_RPUMP_OFFSET + iIndex] & 0xff);
}

void CCB::DispSetHanlerConfig(DB_HANDLER_STRU *pHdl)
{
   int iIdx = pHdl->address - APP_HAND_SET_BEGIN_ADDRESS;

   if (iIdx >= 0 && iIdx < APP_HAND_SET_MAX_NUMBER)
   {
       aHandler[iIdx].iDevType = pHdl->type;
   }
}

void CCB::emitNotify(unsigned char *pucData,int iLength)
{
   // do something here
   unsigned char *pdi = (unsigned char *)mem_malloc(iLength);

   if (pdi)
   {
       memcpy(pdi,pucData,iLength);
       
       emit dispIndication(pdi,iLength);
   }
}


void CCB::emitIapNotify(IAP_NOTIFY_STRU *pIapNotify)
{
    // do something here
    IAP_NOTIFY_STRU *pdi = (IAP_NOTIFY_STRU *)mem_malloc(IAP_NOTIFY_SIZE + pIapNotify->iMsgLen);
    
    if (pdi)
    {
        memcpy(pdi,pIapNotify,IAP_NOTIFY_SIZE + pIapNotify->iMsgLen);
        
        emit iapIndication(pdi);
    }
}

void CCB::emitCmReset(uint32_t ulMap)
{
   LOG(VOS_LOG_DEBUG,"cmresetindication");
   emit cmresetindication(ulMap);
}



void CCB::DispGetRPumpSwitchState(int iIndex ,int *pState)
{
   *pState = !!(ExeBrd.aRPumpObjs[iIndex].iActive & DISP_ACT_TYPE_SWITCH);
}


void CCB::MainResetModulers()
{
     APP_PACKET_HOST_RESET_STRU Rst;

     unsigned int ulIdentifier;

     memset(&Rst,0,sizeof(APP_PACKET_HOST_RESET_STRU));

     Rst.hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL ;
     Rst.hdr.ucMsgType = APP_PACKET_COMM_HOST_RESET;

     CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_DEV_TYPE_MAIN_CTRL,CAN_BROAD_CAST_ADDRESS);

     CcbSndAfCanCmd(APP_CAN_CHL_OUTER,ulIdentifier,SAPP_CMD_DATA,(unsigned char *)&Rst,sizeof(Rst));
     CcbSndAfCanCmd(APP_CAN_CHL_INNER,ulIdentifier,SAPP_CMD_DATA,(unsigned char *)&Rst,sizeof(Rst));
}



void CCB::MainSndHeartBeat(void)
{
     unsigned char buf[32];
     
     unsigned int ulIdentifier;
     
     APP_PACKET_HEART_BEAT_REQ_STRU *pHbt  = (APP_PACKET_HEART_BEAT_REQ_STRU *)buf;
     APP_PACKET_HO_STATE_STRU       *pLoad = (APP_PACKET_HO_STATE_STRU *)pHbt->aucData;

     //LOG(VOS_LOG_WARNING,"MainSndHeartBeat ");    

     memset(pHbt,0,sizeof(APP_PACKET_HOST_RESET_STRU));

     pHbt->hdr.ucLen     = sizeof(APP_PACKET_HO_STATE_STRU) ;
     pHbt->hdr.ucDevType = APP_DEV_TYPE_MAIN_CTRL ;
     pHbt->hdr.ucMsgType = APP_PACKET_COMM_HEART_BEAT;

     pLoad->ucState      = CanCcbGetHoState();
     pLoad->ucResult     = 0;
     pLoad->ucAlarmState = getAlarmState();

     switch(pLoad->ucState)
     {
     case APP_PACKET_HS_STATE_QTW:
         pLoad->ucAddData    = CcbGetTwFlag();
         pLoad->ucResult     = CcbGetTwPendingFlag();
         break;
     case APP_PACKET_HS_STATE_CIR:
         pLoad->ucAddData    = iCirType;
         break;
     default :
         pLoad->ucAddData    = 0;
         break;
     }

#if 0
     /* 2018/04/09 Add begin*/
     if ((CIR_TYPE_HP == iCirType)
        && (APP_PACKET_HS_STATE_CIR == CanCcbGetHoState()))
     {
        pLoad->ucState = APP_PACKET_HS_STATE_RUN;
     }
     /* 2018/04/09 Add end */
#endif
     pLoad->ucMainCtrlState = CanCcbGetMainCtrlState();
     
     CAN_BUILD_ADDRESS_IDENTIFIER(ulIdentifier,APP_DEV_TYPE_MAIN_CTRL,CAN_BROAD_CAST_ADDRESS);

     if (ulRegisterMask & (1 << APP_PAKCET_ADDRESS_EXE))
     {
          CcbSndAfCanCmd(APP_CAN_CHL_INNER,ulIdentifier,SAPP_CMD_DATA,buf,pHbt->hdr.ucLen + APP_PROTOL_HEADER_LEN);
     }

     if (ulRegisterMask & (0X3 << APP_HAND_SET_BEGIN_ADDRESS))
     {
         CcbSndAfCanCmd(APP_CAN_CHL_OUTER,ulIdentifier,SAPP_CMD_DATA,buf,pHbt->hdr.ucLen + APP_PROTOL_HEADER_LEN);
     }

}

void CCB::MainInitInnerIpc()
{
    int iLoop;
    
    sp_thread_mutex_init(&Ipc.mutex,NULL);
    sp_thread_cond_init( &Ipc.cond, NULL );
    sp_thread_mutex_init(&Ipc4Rfid.mutex,NULL);
    sp_thread_cond_init( &Ipc4Rfid.cond, NULL );

    sp_thread_mutex_init(&m_Ipc4DevMgr.mutex,NULL);
    sp_thread_cond_init( &m_Ipc4DevMgr.cond, NULL );

    for (iLoop = 0; iLoop < MAX_RFREADER_NUM; iLoop++)
    {
        sp_thread_mutex_init(&aRfReader[iLoop].Ipc.mutex,NULL);
        sp_thread_cond_init( &aRfReader[iLoop].Ipc.cond, NULL );
    }   

}

void CCB::MainDeinitInnerIpc(void)
{
    int iLoop;

    sp_thread_mutex_destroy(&Ipc.mutex);
    sp_thread_cond_destroy( &Ipc.cond );

    sp_thread_mutex_destroy(&Ipc4Rfid.mutex);
    sp_thread_cond_destroy( &Ipc4Rfid.cond );

    sp_thread_mutex_destroy(&m_Ipc4DevMgr.mutex);
    sp_thread_cond_destroy( &m_Ipc4DevMgr.cond );

    for (iLoop = 0; iLoop < MAX_RFREADER_NUM; iLoop++)
    {
        sp_thread_mutex_destroy(&aRfReader[iLoop].Ipc.mutex);
        sp_thread_cond_destroy( &aRfReader[iLoop].Ipc.cond );
    }   
    
}

void CCB::CcbInitMachineType()
{
    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
        ulHyperTwMask  = (1<<APP_EXE_E4_NO)|(1<<APP_EXE_E5_NO)|(1<<APP_EXE_E6_NO)|(1<<APP_EXE_C2_NO)|(1<<APP_EXE_N2_NO);
        ulNormalTwMask = (1<<APP_EXE_E4_NO)|(1<<APP_EXE_E5_NO)|(1<<APP_EXE_E6_NO)|(1<<APP_EXE_C2_NO);
        ulCirMask      = (1<<APP_EXE_E4_NO)|(1<<APP_EXE_E5_NO)|(1<<APP_EXE_E6_NO)|(1<<APP_EXE_C2_NO)|(1<<APP_EXE_N2_NO);

        ulRunMask      = APP_EXE_INNER_SWITCHS & (~((1<<APP_EXE_E4_NO)|(1<<APP_EXE_E5_NO)|(1<<APP_EXE_E6_NO)|(1<<APP_EXE_C2_NO)|(1<<APP_EXE_N2_NO)));

        ulPMMask       = (((1 << APP_EXE_PM1_NO)|(1 << APP_EXE_PM2_NO)) << APP_EXE_MAX_ECO_NUMBER);
        bit1CirSpeedAdjust = TRUE;
        
        break;
    case MACHINE_C:
        ulHyperTwMask  = 0;
        ulNormalTwMask = (1<<APP_EXE_E4_NO)|(1<<APP_EXE_E5_NO)|(1<<APP_EXE_E6_NO)|(1<<APP_EXE_C2_NO)|(1<<APP_EXE_N2_NO);
        ulCirMask      = (1<<APP_EXE_E4_NO)|(1<<APP_EXE_E5_NO)|(1<<APP_EXE_E6_NO)|(1<<APP_EXE_C2_NO)|(1<<APP_EXE_N2_NO);

        ulRunMask      = APP_EXE_INNER_SWITCHS & (~((1<<APP_EXE_E4_NO)|(1<<APP_EXE_E5_NO)|(1<<APP_EXE_E6_NO)|(1<<APP_EXE_C2_NO)|(1<<APP_EXE_N2_NO)));

        ulPMMask       = (((1 << APP_EXE_PM1_NO)|(1 << APP_EXE_PM2_NO)) << APP_EXE_MAX_ECO_NUMBER);
        bit1CirSpeedAdjust = TRUE;
        break;
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_RO_H:
        ulHyperTwMask  = 0;
        ulNormalTwMask = (1<<APP_EXE_E4_NO)|(1<<APP_EXE_E6_NO)|(1<<APP_EXE_C2_NO);
        ulCirMask      = (1<<APP_EXE_E4_NO)|(1<<APP_EXE_E6_NO)|(1<<APP_EXE_C2_NO);

        ulRunMask      = APP_EXE_INNER_SWITCHS & (~((1<<APP_EXE_E4_NO)|(1<<APP_EXE_E6_NO)|(1<<APP_EXE_C2_NO)));

        ulPMMask       = (((1 << APP_EXE_PM1_NO)|(1 << APP_EXE_PM2_NO)) << APP_EXE_MAX_ECO_NUMBER);
        bit1CirSpeedAdjust = TRUE;
        
        break;
    case MACHINE_PURIST:
    case MACHINE_C_D:
        ulHyperTwMask  = (1<<APP_EXE_E4_NO)|(1<<APP_EXE_E5_NO)|(1<<APP_EXE_C2_NO)|(1<<APP_EXE_N2_NO)|(1<<APP_EXE_E6_NO)|(1<<APP_EXE_E10_NO);
        ulNormalTwMask = 0;
        ulCirMask      = (1<<APP_EXE_E4_NO)|(1<<APP_EXE_E5_NO)|(1<<APP_EXE_C2_NO)|(1<<APP_EXE_N2_NO)|(1<<APP_EXE_E6_NO);

        ulRunMask      = 0;

        ulPMMask       = ((1 << APP_EXE_PM2_NO) << APP_EXE_MAX_ECO_NUMBER);
        
        bit1CirSpeedAdjust = TRUE;
        break;
    default:  /* late implement */
        ulHyperTwMask  = (1<<APP_EXE_E4_NO)|(1<<APP_EXE_E5_NO)|(1<<APP_EXE_E6_NO)|(1<<APP_EXE_C2_NO)|(1<<APP_EXE_N2_NO);
        ulNormalTwMask = (1<<APP_EXE_E4_NO)|(1<<APP_EXE_E5_NO)|(1<<APP_EXE_E6_NO)|(1<<APP_EXE_C2_NO);
        ulCirMask      = (1<<APP_EXE_E4_NO)|(1<<APP_EXE_E5_NO)|(1<<APP_EXE_E6_NO)|(1<<APP_EXE_C2_NO);

        ulRunMask      = APP_EXE_INNER_SWITCHS & (~((1<<APP_EXE_E4_NO)|(1<<APP_EXE_E5_NO)|(1<<APP_EXE_E6_NO)|(1<<APP_EXE_C2_NO)|(1<<APP_EXE_N2_NO)));

        ulPMMask       = APP_EXE_PM_REPORT_MASK;
        
        break;
    }

    /* for test purpose  */
}

void CCB::MainInitMsg()
{
   int iLoop;
   float fv; //2019.1.7

   for (iLoop = 0; iLoop < WORK_LIST_NUM; iLoop++)
   {
       INIT_LIST_HEAD(&WorkList[iLoop]);
   }
   
   for (iLoop = 0; iLoop < APP_EXE_VALVE_NUM; iLoop++)
   {
       ExeBrd.aValveObjs[iLoop].emDispObjType = APP_OBJ_VALVE;
       ExeBrd.aValveObjs[iLoop].iDispObjId    = iLoop;
       ExeBrd.aValveObjs[iLoop].iVChoice      = APP_OBJ_VALUE_U32;
   }

   for (iLoop = 0; iLoop < APP_EXE_PRESSURE_METER; iLoop++)
   {
       ExeBrd.aPMObjs[iLoop].emDispObjType = APP_OBJ_B;
       ExeBrd.aPMObjs[iLoop].iDispObjId    = iLoop;
       ExeBrd.aPMObjs[iLoop].iVChoice      = APP_OBJ_VALUE_U32;    /* ua */
   }

   for (iLoop = 0; iLoop < APP_EXE_G_PUMP_NUM; iLoop++)
   {
       ExeBrd.aGPumpObjs[iLoop].emDispObjType = APP_OBJ_N_PUMP;
       ExeBrd.aGPumpObjs[iLoop].iDispObjId    = iLoop;
       ExeBrd.aGPumpObjs[iLoop].iVChoice      = APP_OBJ_VALUE_U32; /* ua */
   }

   for (iLoop = 0; iLoop < APP_EXE_R_PUMP_NUM; iLoop++)
   {
       ExeBrd.aRPumpObjs[iLoop].emDispObjType = APP_OBJ_R_PUMP;
       ExeBrd.aRPumpObjs[iLoop].iDispObjId    = iLoop;
       ExeBrd.aRPumpObjs[iLoop].iVChoice      = APP_OBJ_VALUE_U32; 
   }

   for (iLoop = 0; iLoop < APP_EXE_RECT_NUM; iLoop++)
   {
       ExeBrd.aRectObjs[iLoop].emDispObjType = APP_OBJ_RECT;
       ExeBrd.aRectObjs[iLoop].iDispObjId    = iLoop;
       ExeBrd.aRectObjs[iLoop].iVChoice      = APP_OBJ_VALUE_U32;
   }

   for (iLoop = 0; iLoop < APP_EXE_EDI_NUM; iLoop++)
   {
       ExeBrd.aEDIObjs[iLoop].emDispObjType = APP_OBJ_EDI;
       ExeBrd.aEDIObjs[iLoop].iDispObjId    = iLoop;
       ExeBrd.aEDIObjs[iLoop].iVChoice      = APP_OBJ_VALUE_U32;
   }

   for (iLoop = 0; iLoop < APP_EXE_ECO_NUM; iLoop++)
   {
       ExeBrd.aEcoObjs[iLoop].emDispObjType = APP_OBJ_I;
       ExeBrd.aEcoObjs[iLoop].iDispObjId    = iLoop;
       ExeBrd.aEcoObjs[iLoop].iVChoice      = APP_OBJ_VALUE_CUST;
   }

   for (iLoop = 0; iLoop < APP_FM_FLOW_METER_NUM; iLoop++)
   {
       FlowMeter.aFmObjs[iLoop].emDispObjType = APP_OBJ_S;
       FlowMeter.aFmObjs[iLoop].iDispObjId    = iLoop;
       FlowMeter.aFmObjs[iLoop].iVChoice      = APP_OBJ_VALUE_U32;

       FlowMeter.aHist[iLoop].ulSec           = gulSecond;
       FlowMeter.aHist[iLoop].iVChoice        = APP_OBJ_VALUE_U32;
       FlowMeter.aHist[iLoop].curValue.ulV    = INVALID_FM_VALUE;
       FlowMeter.aHist[iLoop].lstValue.ulV    = INVALID_FM_VALUE;

       FlowMeter.aulHistTotal[iLoop]          = 0;
   }

   memset(aHandler,0,sizeof(HANDLER_STRU)*MAX_HANDLER_NUM);

   memset(aRfReader,0,sizeof(RFREADER_STRU)*MAX_RFREADER_NUM);

   for (iLoop = 0; iLoop < PUMP_SPEED_NUM; iLoop++)
   {
       //2019.1.7
       if(10 <= iLoop)
       {
           fv = 24.0;
       }
       else if(iLoop == 9)
       {
           fv = 18.0;
       }
       else if(iLoop == 8)
       {
           fv = 15.0;
       }
       else
       {
           fv = iLoop + 6.0;
       }
       aiSpeed2ScaleMap[iLoop] = DispConvertVoltage2RPumpSpeed(fv);
       //end
       //aiSpeed2ScaleMap[iLoop] = DispConvertVoltage2RPumpSpeed(((24.0 - 5.0)*iLoop)/PUMP_SPEED_NUM + 5);
   }
   
   // for ccb init
   curWorkState.iMainWorkState = DISP_WORK_STATE_IDLE;
   curWorkState.iSubWorkState  = DISP_WORK_SUB_IDLE;

   curWorkState.iMainWorkState4Pw = DISP_WORK_STATE_IDLE;
   curWorkState.iSubWorkState4Pw  = DISP_WORK_SUB_IDLE;

   ExeBrd.ucDinState = 0XFF;

   ulAdapterAgingCount = 0XFFFFFF00;
   
   switch(gGlobalParam.iMachineType)
   {
   case MACHINE_UP:
        m_aWaterType[APP_DEV_HS_SUB_UP] = WATER_TYPE_UP;
        m_aWaterType[APP_DEV_HS_SUB_HP] = WATER_TYPE_HP_RO;
        break;
   case MACHINE_EDI:
         m_aWaterType[APP_DEV_HS_SUB_UP] = WATER_TYPE_NUM;
         m_aWaterType[APP_DEV_HS_SUB_HP] = WATER_TYPE_HP_EDI;
         break;
   case MACHINE_RO:
   case MACHINE_RO_H:
   case MACHINE_C:
        m_aWaterType[APP_DEV_HS_SUB_UP] = WATER_TYPE_NUM;
        m_aWaterType[APP_DEV_HS_SUB_HP] = WATER_TYPE_HP_RO;
        break;
   case MACHINE_PURIST:
   case MACHINE_C_D:
        m_aWaterType[APP_DEV_HS_SUB_UP] = WATER_TYPE_UP;
        m_aWaterType[APP_DEV_HS_SUB_HP] = WATER_TYPE_NUM;
        break;
   default:
        m_aWaterType[APP_DEV_HS_SUB_UP] = WATER_TYPE_NUM;
        m_aWaterType[APP_DEV_HS_SUB_HP] = WATER_TYPE_NUM;
        break;
   }

   CcbInitMachineType();

   /* for other init */
   MainInitInnerIpc();

   for (iLoop = 0; iLoop < WORK_LIST_NUM + 1; iLoop++)
   {
       sp_thread_sem_init(&Sem4Delay[iLoop],0,1); /* binary semphone */
   }

   sp_thread_mutex_init(&WorkMutex,NULL);
   
   sp_thread_mutex_init(&ModbusMutex,NULL);

   sp_thread_mutex_init(&C12Mutex,NULL);

   TASK_HANDLE_CANITF    = Task_CreateMessageTask(CanItfMsgProc,NULL,NULL,&gCanItf[0]);
   TASK_HANDLE_MOCAN     = Task_CreateMessageTask(CanMsgMsgProc,NULL,NULL,this);
   
   // start period check timer
   timer_add(1000   ,TIMER_PERIOD ,Main_timer_handler,(void *)TIMER_SECOND);
   
   timer_add(1000*60,TIMER_PERIOD ,Main_timer_handler,(void *)TIMER_MINUTE);
   
   Task_DispatchWorkTask(TimerProc,&gTps);
   
   // send init message to Main Proc
   VOS_SndMsg2(TASK_HANDLE_CANITF  ,INIT_ALL_THREAD_EVENT,0,NULL);
   VOS_SndMsg2(TASK_HANDLE_MOCAN   ,INIT_ALL_THREAD_EVENT,0,NULL);

   // set reset to all sub modulers
   MainResetModulers();
}

void CCB::MainDeInitMsg()
{
    int iLoop;
    MainDeinitInnerIpc();

    for (iLoop = 0; iLoop < WORK_LIST_NUM + 1; iLoop++)
    {
        sp_thread_sem_destroy(&Sem4Delay[iLoop]);
    }

    sp_thread_mutex_destroy(&WorkMutex);

    sp_thread_mutex_destroy(&ModbusMutex);

    sp_thread_mutex_destroy(&C12Mutex);

}

void CCB::Main_timer_handler(void *arg)
{
    int id = (int )arg;
    int ret;
    TIMER_MSG_STRU *tm = (TIMER_MSG_STRU *)SatAllocMsg(TIMER_MSG_MSG_SIZE);//malloc(sizeof(TIMER_MSG_STRU));
    if (tm)
    {
        tm->id = id;
        tm->msgHead.event = TIMER_MSG;
        ret = VOS_SndMsg(TASK_HANDLE_MAIN,(SAT_MSG_HEAD *)tm);
        if (0 != ret)
        {
        }
    }
}

void CCB::MainHeartBeatProc()
{
    int iLoop;
    if (ulRegisterMask)
    {
       iHbtCnt = (iHbtCnt + 1) % (HEART_BEAT_PERIOD - HEART_BEAT_MARGIN);

       ulActiveShadowMask = ulActiveMask;

       if (0 == iHbtCnt)
       {
          for (iLoop = 0; iLoop < 32; iLoop++)
          {
              if ((1 << iLoop) & (ulRegisterMask))
              {
                  if ((1 << iLoop) & (ulActiveMask))
                  {
                      if (aucHbCounts[iLoop] > 0)
                      {
                          aucHbCounts[iLoop]-- ;
                      }

                      if (!aucHbCounts[iLoop])
                      {
                          ulActiveMask &= ~(1 << iLoop);
                          aulActMask4Trx[APP_TRX_CAN] &= ~(1 << iLoop);
                          aulActMask4Trx[APP_TRX_ZIGBEE] &= ~(1 << iLoop);
                          
                          ulActiveMask4HbCheck |= (1 << iLoop);
                      }
                      continue;
                  }
                      /* lost heart beat , raise alarm !*/
                  switch(iLoop)
                  {
                  case APP_PAKCET_ADDRESS_EXE:
                       if (ulActiveMask4HbCheck & (1 << iLoop))
                       {
                           ulActiveMask4HbCheck &= ~(1 << iLoop);
                           CcbExeBrdNotify();
                       }
                       break;
                   case APP_PAKCET_ADDRESS_FM:
                        if (ulActiveMask4HbCheck & (1 << iLoop))
                        {
                            ulActiveMask4HbCheck &= ~(1 << iLoop);
                            CcbFmBrdNotify();
                        }
                        break;
                    case APP_HAND_SET_BEGIN_ADDRESS ... APP_HAND_SET_END_ADDRESS:
                        if (ulActiveMask4HbCheck & (1 << iLoop))
                        {
                            ulActiveMask4HbCheck &= ~(1 << iLoop);
                            aHandler[iLoop - APP_HAND_SET_BEGIN_ADDRESS].bit1Active = 0;
                            aHandler[iLoop - APP_HAND_SET_BEGIN_ADDRESS].iTrxMap  = 0;                           
                            CcbHandlerNotify(iLoop - APP_HAND_SET_BEGIN_ADDRESS);
                        }
                        break;
                    case APP_RF_READER_BEGIN_ADDRESS ... APP_RF_READER_END_ADDRESS:
                        if (ulActiveMask4HbCheck & (1 << iLoop))
                        {
                            ulActiveMask4HbCheck &= ~(1 << iLoop);
                            aRfReader[iLoop - APP_RF_READER_BEGIN_ADDRESS].bit1Active = 0;
                            CcbRfidNotify(iLoop - APP_RF_READER_BEGIN_ADDRESS);
                        }
                        break;
                        
                        
                  }
              }
          }
          MainSndHeartBeat();
       }
    } 
}

void CCB::CcbAddExeSwitchWork(WORK_SETUP_REPORT_STRU *pRpt )
{
    if (!SearchWork(work_switch_setup_exe))
    {
        WorkSwitchSetParam4Exe.ulMask = pRpt->ulMask;
        WorkSwitchSetParam4Exe.ulValue = pRpt->ulValue;
        CcbInnerWorkSetupExeSwitch();
    }                

}


void CCB::CcbAddExeReportWork(WORK_SETUP_REPORT_STRU *pRpt )
{
    if (!SearchWork(work_rpt_setup_exe))
    {
        WorkRptSetParam4Exe.ulMask = pRpt->ulMask;
        WorkRptSetParam4Exe.ulValue = pRpt->ulValue;
        CcbInnerWorkSetupExeReport();
    }                

}

void CCB::CcbAddFmReportWork(WORK_SETUP_REPORT_STRU *pRpt )
{
    if (!SearchWork(work_rpt_setup_fm))
    {
        WorkRptSetParam4Fm.ulMask = pRpt->ulMask;
        WorkRptSetParam4Fm.ulValue = pRpt->ulValue;
        CcbInnerWorkSetupFmReport();
    }                

}

void CCB::MainSecondTask4MainState()
{
    int iLoop;

    switch(curWorkState.iMainWorkState)
    {
    case DISP_WORK_STATE_PREPARE:
        iInitRunTimer++;
        break;
    case DISP_WORK_STATE_IDLE:
        {
            switch(curWorkState.iSubWorkState)
            {
            case DISP_WORK_SUB_IDLE:
            case DISP_WORK_SUB_WASH:
                break;
            default:
                break;
            }
        }
        break;
    case DISP_WORK_STATE_LPP:
        if (gulSecond - ulLPPTick >= (gGlobalParam.TMParam.aulTime[TIME_PARAM_LPP] / 1000))
        {
            /* move to init run */
            if (!SearchWork(work_init_run))
            {
                CcbInnerWorkInitRun();
            }                
            
        }
        break;
    case DISP_WORK_STATE_RUN:
        switch(gGlobalParam.iMachineType)
        {
        case MACHINE_UP: 
        case MACHINE_RO: 
        case MACHINE_RO_H:
        case MACHINE_C:
        case MACHINE_EDI:
            if (bit1B2Full)
            {
                if (!CcbGetPmObjState(1 << APP_EXE_PM2_NO))
                {
                    /* Active Report Flag */
                    WORK_SETUP_REPORT_STRU Rpt;
                    Rpt.ulMask  = MAKE_B_MASK((1 << APP_EXE_PM2_NO));
                    Rpt.ulValue = MAKE_B_MASK((1 << APP_EXE_PM2_NO));
                    
                    LOG(VOS_LOG_WARNING,"CcbAddExeReportWork 3");    
                    
                    CcbAddExeReportWork(&Rpt);
                }
                else
                {
                    if (CcbConvert2Pm2SP(ExeBrd.aPMObjs[APP_EXE_PM2_NO].Value.ulV) < B2_FULL)
                    {
                        bit1B2Full = FALSE;
                    }
                }
    
                if (bit1B2Full)
                {
                    if (!bit1PeriodFlushing)
                    {
                        if (gulSecond - ulB2FullTick >= gGlobalParam.TMParam.aulTime[TIME_PARAM_NormRunT5]/1000)
                        {
                            if (!SearchWork(work_tank_start_full_loop))
                            {
                                CcbInnerWorkStartTankFullLoop();
                            }                
                        }
                    }
                    else
                    {
                        LOG(VOS_LOG_WARNING,"TF %d:%d",gGlobalParam.TMParam.aulTime[TIME_PARAM_NormRunT4]/1000,gulSecond - ulB2FullTick);    
                    }
                }
            }
            else
            {
                if (!bit1ProduceWater)
                {
                    if(haveB3())
                    {
                        if ((CcbConvert2Pm2SP(ExeBrd.aPMObjs[APP_EXE_PM2_NO].Value.ulV) < CcbGetSp5())
                             && (CcbConvert2Pm3SP(ExeBrd.aPMObjs[APP_EXE_PM3_NO].Value.ulV) > 50.0))
                        {
                            /* start Nomal Run */
                            if (DISP_WORK_SUB_IDLE == curWorkState.iSubWorkState)
                            {
                                if (!SearchWork(work_normal_run))
                                {
                                    CcbInnerWorkRun();
                                }
                            }
                        }
                    }
                    else
                    {
                        if (CcbConvert2Pm2SP(ExeBrd.aPMObjs[APP_EXE_PM2_NO].Value.ulV) < CcbGetSp5())
                        {
                            if (DISP_WORK_SUB_IDLE == curWorkState.iSubWorkState)
                            {
                                if (!SearchWork(work_normal_run))
                                {
                                    CcbInnerWorkRun();
                                }
                            }
                        }
                    }
                }
            }   

            if (bit1PeriodFlushing)
            {
                if (gulSecond - ulB2FullTick >= gGlobalParam.TMParam.aulTime[TIME_PARAM_NormRunT4]/1000)
                {
                    if (!SearchWork(work_tank_stop_full_loop))
                    {
                        CcbInnerWorkStopTankFullLoop();
                    }                
                }
            }
            break;
        default:
            break;
        }

        /*Check B3 */
        if (!CcbGetPmObjState(1 << APP_EXE_PM3_NO)
            && haveB3())
        {
           /* Active Report Flag */
           WORK_SETUP_REPORT_STRU Rpt;
           Rpt.ulMask  = MAKE_B_MASK((1 << APP_EXE_PM3_NO));
           Rpt.ulValue = MAKE_B_MASK((1 << APP_EXE_PM3_NO));
           
           CcbAddExeReportWork(&Rpt);
        }

        if (bit1B1Check4RuningState 
            && bit1B1UnderPressureDetected)
        {
            //if (gulSecond - ulB1UnderPressureTick >= 60)
            if (gulSecond - ulB1UnderPressureTick >= DEFAULT_LPP_CHECK_NUMBER)
            {
                /* move to LPP state */
                if (!SearchWork(work_start_lpp))
                {
                    CcbInnerWorkLpp();
                }   

               /* Cancel Running task */
               {
                    DISPHANDLE hdl ;        
                    hdl = SearchWork(work_init_run);

                    if (hdl)
                    {
                        CcbCancelWork(hdl);
                    }

                    hdl = SearchWork(work_normal_run);

                    if (hdl)
                    {
                        CcbCancelWork(hdl);
                    }                   
               }
            }
        }
        break;
    }

    /* 2018/01/11 begin : always enable B1 report according to DU */
    if (gGlobalParam.SubModSetting.ulFlags & (1 << DISP_SM_HaveB2))
    {
        if (ulActiveMask   & (1 << APP_PAKCET_ADDRESS_EXE))
        {
            if (!CcbGetPmObjState((1 << APP_EXE_PM2_NO)))
            {
                WORK_SETUP_REPORT_STRU Rpt;
                Rpt.ulMask  = MAKE_B_MASK((1 << APP_EXE_PM2_NO));
                Rpt.ulValue = MAKE_B_MASK((1 << APP_EXE_PM2_NO));
                CcbAddExeReportWork(&Rpt);
            }
        }
    }
    /* 2018/01/11 end */
    
    /* independent work to fill tank */
    if (bit1FillingTank)
    {
        /* check valve state */
        if (!CcbGetSwitchObjState((1 << APP_EXE_E10_NO)))
        {
            WORK_SETUP_SWITCH_STRU Rpt;
            Rpt.ulMask  = (1 << APP_EXE_E10_NO);
            Rpt.ulValue = (1 << APP_EXE_E10_NO);
            
            CcbAddExeSwitchWork(&Rpt);
        }
        
        /* check pm state */
        if (!CcbGetPmObjState((1 << APP_EXE_PM3_NO)))
        {
            WORK_SETUP_REPORT_STRU Rpt;
            Rpt.ulMask  = MAKE_B_MASK((1 << APP_EXE_PM3_NO));
            Rpt.ulValue = MAKE_B_MASK((1 << APP_EXE_PM3_NO));
            
            CcbAddExeReportWork(&Rpt);
        }

    }

}

/**
 * 检查B3液位是否可以重新初始化运行
 */
void CCB::checkB3ToReInitRun()
{
    if(haveB3())
    {
        switch(curWorkState.iMainWorkState)
        {
        case DISP_WORK_STATE_IDLE:
            {
                switch(curWorkState.iSubWorkState)
                {
                case DISP_WORK_SUB_IDLE:
                    if(CcbConvert2Pm3SP(ExeBrd.aPMObjs[APP_EXE_PM3_NO].Value.ulV) > 50.0)
                    {
                        if (!SearchWork(work_init_run))
                        {
                            CcbInnerWorkInitRun();
                        }
                    }
                    break;
                default:
                    break;
                }
            }
            break;
        default:
            break;
        }
    }
}

void CCB::MainSecondTask4Pw()
{
    switch(curWorkState.iMainWorkState4Pw)
    {
    case DISP_WORK_STATE_RUN:
        // WARNING: 2019.12.24 增加checkB3ToReInitRun(),以尝试在原水箱液位低不能启动后，自动恢复冲洗运行
        checkB3ToReInitRun(); 
        switch(curWorkState.iSubWorkState4Pw)
        {
        case DISP_WORK_SUB_IDLE:
            if (gulSecond - ulLstCirTick >= (DEFAULT_IDLECIRTIME / 1000))
            {
                if (haveHPCir())
                {
                    if (!SearchWork(work_start_cir))
                    {
                        CcbInnerWorkStartCir(CIR_TYPE_HP);
                    } 
                }
            }
               
            break;
        case DISP_WORK_SUB_RUN_QTW:
            if (!CcbGetFmObjState((1 << APP_FM_FM3_NO)))
            {
                WORK_SETUP_REPORT_STRU Rpt;
                Rpt.ulMask  = (1 << APP_FM_FM3_NO);
                Rpt.ulValue = (1 << APP_FM_FM3_NO);
                CcbAddFmReportWork(&Rpt);
            }
            if (gGlobalParam.SubModSetting.ulFlags & (1 << DISP_SM_HaveB2))
            {
                if (!CcbGetPmObjState((1 << APP_EXE_PM2_NO)))
                {
                    WORK_SETUP_REPORT_STRU Rpt;
                    Rpt.ulMask  = (1 << APP_EXE_PM2_NO);
                    Rpt.ulValue = (1 << APP_EXE_PM2_NO);
                    CcbAddExeReportWork(&Rpt);
                } 
            }
            break;
        case DISP_WORK_SUB_RUN_CIR:
            {
                int ulDuration = 0;

                switch(iCirType)
                {
                case CIR_TYPE_UP:
                    /* toc here if toc is configured */
                    if (haveTOC())
                    {
                        iTocStageTimer++;
                        switch(iTocStage)
                        {
                        case APP_PACKET_EXE_TOC_STAGE_FLUSH1:
                        {
                            if(iTocStageTimer == 150)
                            {
                                bit1TocAlarmNeedCheck = TRUE;
                            }
                            if (iTocStageTimer >= 160)
                            {
                                bit1TocAlarmNeedCheck = FALSE;
                                if (!SearchWork(work_start_toc_cir))
                                {
                                    CcbInnerWorkStartCirToc(APP_PACKET_EXE_TOC_STAGE_OXDIZATION);
                                }   
                            }
                            break;
                        }
                        case APP_PACKET_EXE_TOC_STAGE_OXDIZATION:
                            if (iTocStageTimer >= 180)
                            {
                                if (!SearchWork(work_start_toc_cir))
                                {
                                    CcbInnerWorkStartCirToc(APP_PACKET_EXE_TOC_STAGE_FLUSH2);
                                }   
                            }
                            break;
                        case APP_PACKET_EXE_TOC_STAGE_FLUSH2:
                            if (iTocStageTimer >= 20)
                            {
                                if (!SearchWork(work_stop_cir))
                                {
                                    CcbInnerWorkStopCir();
                                }   
                            }
                            break;
                            
                        }

                        /* check toc alarm */
                    }
                    else
                    {
                        ulDuration = (gGlobalParam.TMParam.aulTime[TIME_PARAM_InterCirDuration]/1000);                    
                        if (gulSecond - ulCirTick >= (unsigned int)ulDuration)
                        {
                            if (!SearchWork(work_stop_cir))
                            {
                                CcbInnerWorkStopCir();
                            }   
                        }     
                    }
                    break;
                case CIR_TYPE_HP:
                    ulDuration = 60*60; 
                    if(gGlobalParam.iMachineType == MACHINE_C)
                    {
                        ulDuration = 6*60; 
                    }
                    if (gulSecond - ulCirTick >= (unsigned int)ulDuration)
                    {
                        if (!SearchWork(work_stop_cir))
                        {
                            CcbInnerWorkStopCir();
                        }   
                    }                    
                    break;

                }

                /* check I5 & SP7*/
                if (!CcbGetIObjState((1 << APP_EXE_I5_NO)))
                {
                    WORK_SETUP_REPORT_STRU Rpt;
                    
                    Rpt.ulMask  = (1 << APP_EXE_I5_NO);
                    Rpt.ulValue = (1 << APP_EXE_I5_NO);
                
                    CcbAddExeReportWork(&Rpt);
                }
                LOG(VOS_LOG_WARNING,"DISP_WORK_SUB_RUN_CIR %d:%d:%d",iCirType,ulDuration,gulSecond - ulCirTick);    
                
            }
            break;
        default:
            break;
        } 
        
        ulN3PeriodTimer++;

        if (ulN3PeriodTimer % (60*60*2) == 0)
        {
            /* start N3 */
            if (!SearchWork(work_start_N3))
            {
                CcbInnerWorkStartN3();
            }                
        }

        if (bit1N3Active)
        {
            ulN3Duration++;
            if (ulN3Duration >= (gGlobalParam.MiscParam.iTankUvOnTime * 60))
            {
                if (!SearchWork(work_stop_N3))
                {
                    CcbInnerWorkStopN3();
                }              
            }
        }        
        break;
    }

    // NOTE: 纯水分配控制
    checkForTubeCir();
}

//ex
void CCB::Ex_DispDecPressure()
{
    if((curWorkState.iMainWorkState4Pw == DISP_WORK_STATE_IDLE)
        &&(curWorkState.iSubWorkState4Pw ==DISP_WORK_SUB_IDLE_DEPRESSURE)
        &&(gEx_Ccb.EX_Check_State.bit1CheckDecPressure == 1))
    {
        if((ex_gulSecond - gEx_Ccb.Ex_Delay_Tick.ulDecPressure) > 30)
        {
            CcbNotDecPressure(gEx_Ccb.Ex_Delay_Tick.iHandleType, 0);
        }
    }
}
//end


void CCB::MainSecondTask()
{
    gulSecond++;
    ex_gulSecond++; //ex

    mem_free_auto();
    
    //printf("MainSecondTask \r\n");
    MainHeartBeatProc();

    MainSecondTask4MainState();

    MainSecondTask4Pw();

    Ex_DispDecPressure(); //ex

    DispKeyQtw();
}

void CCB::MainMinuteTask()
{
    CcbNvStaticsNotify();
    CcbPumpStaticsNotify();
    CcbFmStaticsNotify();
}

void CCB::MainProcTimerMsg(SAT_MSG_HEAD *pMsg)
{
    TIMER_MSG_STRU *pTimerMsg = (TIMER_MSG_STRU *)(pMsg);

    switch(pTimerMsg->id)
    {
    case TIMER_SECOND:
        MainSecondTask();
        break;
    case TIMER_MINUTE:
        MainMinuteTask();
        break;
    default:
        break;
    }
}

void CCB::MainMsgProc(void *para, SAT_MSG_HEAD *pMsg)
{
    CCB *pCcb = (CCB *)(para);

    switch(pMsg->event)
    {
    case INIT_ALL_THREAD_EVENT:
        pCcb->MainInitMsg();
        break;
    case TIMER_MSG:
        pCcb->MainProcTimerMsg(pMsg);
        break;
    case CANITF_MAIN_MSG:
        pCcb->CanCcbCanItfMsg(pMsg);
        break;
    case QUIT_ALL_THREAD_EVENT:
        pCcb->MainDeInitMsg();
        break;
    case MAIN_WORK_MSG:
        pCcb->CcbWorMsgProc(pMsg);
        break;
    default:
        break;
   }
}

void CCB::CanMsgMsgProc(void *para,SAT_MSG_HEAD *pMsg)
{
    CCB *pCcb = (CCB *)para;
    
    switch(pMsg->event)
    {
    case CANITF_MAIN_MSG:
        pCcb->CanCcbCanItfMsg(pMsg);
        break;
    default:
        break;
   }
}

void CCB::CcbInit()
{
    int iLoop;

    for (iLoop = 0; iLoop < APP_RFID_SUB_TYPE_NUM ; iLoop++)
    {
        m_pConsumableInstaller[iLoop] = new ConsumableInstaller(iLoop);
    }

    memset(m_aiAlarmRcdMask, 0, sizeof(m_aiAlarmRcdMask));
    memset(m_aMas, 0, sizeof(m_aMas));
    memset(m_cMas, 0, sizeof(m_cMas));

    bit1InitRuned       = 0;
    bit1AlarmRej        = 0; 
    bit1AlarmRoPw       = 0;  
    bit1AlarmEDI        = 0; 
    bit1AlarmUP         = 0;
    bit1AlarmTOC        = 0;
    bit1AlarmTapInPress = 0;
    bit1NeedFillTank    = 0;  
    bit1B2Full          = 0;  
    bit1FillingTank     = 0;  
    bit1N3Active        = 0;  
    bit1ProduceWater    = 0;  
    bit1B2Empty         = 0; 
    bit1I54Cir          = 0;  
    bit1I44Nw           = 0;  
    bit1LeakKey4Reset   = 0;  
    bit1EngineerMode    = 0;
    bit1NeedTubeCir     = 0;
    bit1TubeCirOngoing  = 0;
    bit1TocOngoing      = 0;
    bit1SysMonitorStateRpt  = 0;
    bit1PeriodFlushing      = 0;
    bit1B1Check4RuningState = 0;
    bit1B1UnderPressureDetected  = 0;
    bit3RuningState = 0;    
    bit1AlarmROPLV        = 0;  
    bit1AlarmROPHV        = 0;  
    bit1AlarmRODV         = 0;   
    bit1CirSpeedAdjust    = 0;   
    bit1ROWashPause       = 0;  
    bit1TocAlarmNeedCheck = 0;  
    bit1AlarmWorkPressHigh= 0;  

    ulRegisterMask        = 0;
    ulActiveMask          = 0;
    ulActiveShadowMask    = 0;
    ulActiveMask4HbCheck  = 0;
    iBusyWork             = 0;

    memset(aucHbCounts,0,sizeof(aucHbCounts));

    ulHyperTwMask  = 0;       
    ulNormalTwMask = 0;       
    ulCirMask      = 0;            
    ulRunMask      = 0;            
    ulPMMask       = 0;            
    ulAlarmEDITick   = 0;
    ulAlarmRejTick   = 0;
    ulAlarmRoPwTick  = 0;
    ulAlarmUPTick    = 0;
    ulB2FullTick     = 0;
    ulLPPTick        = 0;
    ulB1UnderPressureTick = 0;
    ulTubeIdleCirTick     = 0;
    ulTubeIdleCirIntervalTick = 0;
    ulTubeCirStartDelayTick   = 0;
    ulProduceWaterBgnTime     = 0;
    ulCirTick         = 0;
    ulLstCirTick      = 0;
    iCirType          = 0;           
    iTocStage         = 0;          
    iTocStageTimer    = 0;
    iInitRunTimer     = 0;
    ulN3Duration      = 0;
    ulN3PeriodTimer   = 0;
    ulFiredAlarmFlags = 0;
    ulKeyWorkStates   = 0;
    ucLeakWorkStates  = 0; 

    ulWorkThdState   = 0; 
    ulWorkThdAbort   = 0;
    iWashType        = 0;  
    ulCanCheck       = 0;
    ulRejCheckCount  = 0;
 
    ulRopVelocity = 0;
    ulLstRopFlow  = 0;
    ulLstRopTick  = 0;
    iRopVCheckLowEventCount   = 0;
    iRopVCheckLowRestoreCount = 0;
  
    iRopVCheckHighEventCount   = 0;
    iRopVCheckHighRestoreCount = 0;
    
    ulRodVelocity = 0;
    ulLstRodFlow  = 0;
    ulLstRodTick  = 0;
    iRoDVCheckEventCount   = 0;
    iRoDVCheckRestoreCount = 0;
    ulAdapterAgingCount    = 0;

    m_iInstallHdlIdx       = -1;
 
    memset(aiSpeed2ScaleMap,0,sizeof(aiSpeed2ScaleMap));
    memset(aulActMask4Trx,0,sizeof(aulActMask4Trx));

     // for debug purpose
    m_WorkName.insert(DISP_CMD_IDLE,"DISP_CMD_IDLE");
    m_WorkName.insert(DISP_CMD_RUN,"DISP_CMD_RUN");
    m_WorkName.insert(DISP_CMD_WASH,"DISP_CMD_WASH");
    m_WorkName.insert(DISP_CMD_RESET,"DISP_CMD_RESET");
    m_WorkName.insert(DISP_CMD_LEAK_RESET,"DISP_CMD_LEAK_RESET");
    m_WorkName.insert(DISP_CMD_ENG_MODE,"DISP_CMD_ENG_MODE");
    m_WorkName.insert(DISP_CMD_EXIT_ENG_MODE,"DISP_CMD_EXIT_ENG_MODE");
    m_WorkName.insert(DISP_CMD_LEAK_RESET,"DISP_CMD_LEAK_RESET");
    m_WorkName.insert(DISP_CMD_TUBE_CIR,"DISP_CMD_TUBE_CIR");
    m_WorkName.insert(DISP_CMD_TW,"DISP_CMD_TW");
    m_WorkName.insert(DISP_CMD_CIR,"DISP_CMD_CIR");
    m_WorkName.insert(DISP_CMD_SWITCH_REPORT,"DISP_CMD_SWITCH_REPORT");
    m_WorkName.insert(DISP_CMD_HALT,"DISP_CMD_HALT");
    m_WorkName.insert(DISP_CMD_CANCEL_WORK,"DISP_CMD_CANCEL_WORK");
   
    //int err = 0;
    VOS_SetLogLevel(VOS_LOG_DEBUG);

    VOS_ResetLogNo();

    Task_init(15);
    
    TimerInit();

}

//质量检验主板用
int CCB::Ex_FactoryTest(int select)
{
    int iTmp, iRet;
    switch(select)
    {
    case 0:
    {
        iTmp  = (1 << APP_FM_FM1_NO)|(1 << APP_FM_FM2_NO)|(1<<APP_FM_FM3_NO)|(1<<APP_FM_FM4_NO);
        iRet = CcbUpdateFms(WORK_LIST_LP, 0, iTmp, iTmp);
        if (iRet )
        {
            return 1;
        }
        break;
    }
    case 1:
    {
        iTmp  = (GET_B_MASK(APP_EXE_PM1_NO))|(GET_B_MASK(APP_EXE_PM2_NO))|(GET_B_MASK(APP_EXE_PM3_NO));
        iRet = CcbUpdateIAndBs(WORK_LIST_LP, 0, iTmp, iTmp);
        if (iRet )
        {
            return 2;
        }
        break;
    }
    default:
        break;
    }

    return 0;
}

//2019.9.16 add
unsigned int  CCB::getKeyState()
{
    return ulKeyWorkStates;
}

unsigned char  CCB::getLeakState()
{
    return ucLeakWorkStates;
}

// 2020/08/19
int CCB::getWaterTankCfgMask()
{
    int iMask = 0;

    switch(gGlobalParam.iMachineType)
    {
    default:
        iMask |= 1 << DISP_SM_HaveB2;
        break;
    }
}

// dcj: TODO 
void CCB::getInitInstallMask(int &iType0Mask,int &iType1Mask)
{
    iType0Mask = 0;
    iType1Mask = 0;

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_RO_H:
    case MACHINE_C:
        iType0Mask |= (1 << DISP_P_PACK);
        break;
    default:
        break;
    }
    
    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_PURIST:
    case MACHINE_C_D:
    case MACHINE_RO_H:
    case MACHINE_C:
        iType0Mask |= (1 << DISP_H_PACK);
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_PURIST:
    case MACHINE_C_D:
        iType0Mask |= (1 << DISP_U_PACK);
        break;
    default:
        break;
    }
    
    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_EDI:
        iType0Mask |= (1 << DISP_AC_PACK);
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_C_D:
        iType0Mask |= 1 << DISP_N1_UV;  //254
        break;
    }
        
    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_PURIST:
    case MACHINE_C_D:
        iType0Mask |= 1 << DISP_N2_UV;  //185
        break;
    }
    
    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_RO_H:
    case MACHINE_C:
    case MACHINE_PURIST:
    case MACHINE_C_D:
        iType0Mask |= 1 << DISP_N3_UV; //Tank UV
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_RO_H:
    case MACHINE_C:
        iType1Mask |= 1 << DISP_MACHINERY_RO_MEMBRANE;
        break;
    default:
        break;
    }

    switch (gGlobalParam.iMachineType)  
    {
    case MACHINE_EDI:
        iType1Mask |= 1 << DISP_MACHINERY_EDI;
        break;
    default:
        break;
    }
    
    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_RO_H:
    case MACHINE_C:
    case MACHINE_PURIST:
    case MACHINE_C_D:
        iType0Mask |= 1 << DISP_W_FILTER;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_RO_H:
    case MACHINE_C:
    case MACHINE_PURIST:
    case MACHINE_C_D:
        iType1Mask |= 1 << DISP_MACHINERY_CIR_PUMP;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_RO_H:
    case MACHINE_C:
        iType1Mask |= 1 << DISP_MACHINERY_RO_BOOSTER_PUMP;
        break;
    default:
        break;
    }
        
    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_RO_H:
    case MACHINE_C:
    case MACHINE_PURIST:
    case MACHINE_C_D:
        iType0Mask |= 1 << DISP_T_A_FILTER;
        break;
    default:
        break;
    }
}

void CCB::start()
{
    TASK_HANDLE_MAIN = instance->m_taskHdl   = Task_CreateMessageTask(CCB::MainMsgProc ,NULL,NULL,instance);
    
    if (instance->m_taskHdl)
    {
        VOS_SndMsg2(instance->m_taskHdl ,INIT_ALL_THREAD_EVENT,0,NULL);
    }  
}

CCB::CCB() 
{

    CcbInit();
}

CCB *CCB::instance = NULL;

CCB * CCB::getInstance()
{
    if (NULL == instance)
    {
        instance = new CCB();

      
    }
    return instance;
}


void CCB::onQtwTimerOut()
{
    if (DISP_WORK_STATE_RUN == curWorkState.iMainWorkState4Pw
        && DISP_WORK_SUB_RUN_QTW == curWorkState.iSubWorkState4Pw)
    {
        if (QtwMeas.ulTotalFm != INVALID_FM_VALUE)
        {
            CcbStopQtw();
        }
    }
}

