#include "setpoint.h"
#include "mainwindow.h"
#include "setplistwidgtitem.h"
#include "exconfig.h"
#include <QListWidget>
#include <QListWidgetItem>

#include "cbitmapbutton.h"
#include <QRect>
#include "notify.h"
#include "config.h"


SetPoint::SetPoint(QObject *parent,CBaseWidget *widget ,MainWindow *wndMain) : CSubPage(parent,widget,wndMain)
{
    int iIdx = 0;

    switch(gGlobalParam.iMachineType)/*B1自来水压力下限*/
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
    case MACHINE_Genie:
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_ADAPT:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT1;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP1;
        iIdx++;     
        break;
    default:
        break;
    } 

    switch(gGlobalParam.iMachineType)/*工作压力上限*/
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
    case MACHINE_Genie:
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_ADAPT:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT1;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP33;
        iIdx++;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)/*自来水电导率上限 I1a*/
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
    case MACHINE_Genie:
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_ADAPT:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT1;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP13;
        iIdx++;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)/* 进水温度上限？℃ 下限？℃ */
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
    case MACHINE_Genie:
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_ADAPT:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT2;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP18;
        aIds[iIdx].iParamId[1] = MACHINE_PARAM_SP19;
        iIdx++;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)/* RO产水上限？μS/cm */
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
    case MACHINE_Genie:
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_ADAPT:
    case MACHINE_PURIST: 
        aIds[iIdx].iDspType    = SET_POINT_FORMAT1;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP3;
        iIdx++;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)/* RO产水温度上限？℃ 下限？℃ */
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
    case MACHINE_Genie:
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_ADAPT:
    case MACHINE_PURIST:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT2;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP20;
        aIds[iIdx].iParamId[1] = MACHINE_PARAM_SP21;
        iIdx++;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)/* RO截留率 下限？% */
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
    case MACHINE_Genie:
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_ADAPT:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT1;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP2;
        iIdx++;
        break;
    default:
        break;
    } 

    switch(gGlobalParam.iMachineType)/* EDI产水下限？MΩ.cm */
    {
    case MACHINE_L_Genie:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_Genie:
    case MACHINE_EDI:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT1;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP4;
        iIdx++;
        break;
    default:
        break;
    } 

    switch(gGlobalParam.iMachineType)/* EDI产水温度上限？℃ 下限？℃ */
    {
    case MACHINE_L_Genie:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_Genie:
    case MACHINE_EDI:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT2;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP22;
        aIds[iIdx].iParamId[1] = MACHINE_PARAM_SP23;
        iIdx++;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)/* UP取水下限？MΩ.cm */
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_Genie:
    case MACHINE_UP:
    case MACHINE_PURIST:
    case MACHINE_ADAPT:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT1;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP7;
        iIdx++;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)/* UP产水温度 上限？℃ 下限？℃ */
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_Genie:
    case MACHINE_UP:
    case MACHINE_PURIST:
    case MACHINE_ADAPT:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT2;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP24;
        aIds[iIdx].iParamId[1] = MACHINE_PARAM_SP25;
        iIdx++;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)/*HP产水水质下限 ? MΩ.cm */
    {
    case MACHINE_PURIST:
    case MACHINE_ADAPT:
        break;
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
        if(gAdditionalCfgParam.machineInfo.iMachineFlow < 500)
        {
            aIds[iIdx].iDspType    = SET_POINT_FORMAT1;
            aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP32;
            iIdx++;
        }
        break;
    default:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT1;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP32;
        iIdx++;
        break;
    }

    switch(gGlobalParam.iMachineType)// 循环水质下限 ? MΩ.cm, 用于T Pack耗材更换提醒  
    {
    case MACHINE_Genie:
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT1;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP31;
        iIdx++;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)//水箱水质上限?MΩ.cm 下限?MΩ.cm, 用于判断是否自动开启HP循环
    {
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
        if(gAdditionalCfgParam.machineInfo.iMachineFlow < 500)
        {
            aIds[iIdx].iDspType    = SET_POINT_FORMAT2;
            aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP10;
            aIds[iIdx].iParamId[1] = MACHINE_PARAM_SP11;
            iIdx++;
        }
        break;
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_Genie:
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT2;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP10;
        aIds[iIdx].iParamId[1] = MACHINE_PARAM_SP11;
        iIdx++;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)/* TOC传感器温度上限？℃ 下限？℃ */
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_Genie:
    case MACHINE_UP:
    case MACHINE_PURIST:
    case MACHINE_ADAPT:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT2;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP28;
        aIds[iIdx].iParamId[1] = MACHINE_PARAM_SP29;
        iIdx++;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)/* TOC进水水质下限？MΩ.cm */
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_Genie:
    case MACHINE_UP:
    case MACHINE_PURIST:
    case MACHINE_ADAPT:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT1;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP30;
        iIdx++;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)// 管路水质下限？MΩ.cm, 暂时未启用
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT1;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP17;
        iIdx++;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)// 管路温度上限？℃  下限？℃ , 暂时未启用
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT2;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP26;
        aIds[iIdx].iParamId[1] = MACHINE_PARAM_SP27;
        iIdx++;
        break;
    default:
        break;
    }

    switch(gGlobalParam.iMachineType)/*纯水箱液位水箱空？%  恢复注水？%*/
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
    case MACHINE_Genie:
    case MACHINE_UP:
    case MACHINE_EDI:
    case MACHINE_RO:
    case MACHINE_PURIST:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT2;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP5;
        aIds[iIdx].iParamId[1] = MACHINE_PARAM_SP6;
        iIdx++;
        break;
    default:
        break;
    } 

    switch(gGlobalParam.iMachineType)/* 源水箱液位水箱空？%  恢复注水？% */
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT2;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP8;
        aIds[iIdx].iParamId[1] = MACHINE_PARAM_SP9;
        iIdx++;
        break;
    default:
        break;
    } 

    switch(gGlobalParam.iMachineType)/* RO产水流速上限100.0L/min 下限20.0L/min */
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT2;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP14;
        aIds[iIdx].iParamId[1] = MACHINE_PARAM_SP15;
        iIdx++;
        break;
    default:
        break;
    } 

    switch(gGlobalParam.iMachineType)/* RO弃水流速 下限20.0L/min */
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
        aIds[iIdx].iDspType    = SET_POINT_FORMAT1;
        aIds[iIdx].iParamId[0] = MACHINE_PARAM_SP16;
        iIdx++;
        break;
    default:
        break;
    } 

    m_iRealNum = iIdx;
    
    creatTitle();

    initUi();

    buildTranslation();

    // 20200828: puncture for web system configuration
    gCConfig.m_SetPointCfgInfo.ulPart1 = 0;
    gCConfig.m_SetPointCfgInfo.ulPart2 = 0;
    for (iIdx = 0; iIdx < m_iRealNum; iIdx++)
    {
        switch(aIds[iIdx].iDspType )
        {
        case SET_POINT_FORMAT2:
            if (aIds[iIdx].iParamId[1] < 32)
            {
                gCConfig.m_SetPointCfgInfo.ulPart1 |= 1 << aIds[iIdx].iParamId[1];
            }
            else
            {
                gCConfig.m_SetPointCfgInfo.ulPart1 |= 1 << (aIds[iIdx].iParamId[1] - 32);
            }
        case SET_POINT_FORMAT1:
            if (aIds[iIdx].iParamId[0] < 32)
            {
                gCConfig.m_SetPointCfgInfo.ulPart1 |= 1 << aIds[iIdx].iParamId[0];
            }
            else
            {
                gCConfig.m_SetPointCfgInfo.ulPart1 |= 1 << (aIds[iIdx].iParamId[0] - 32);
            }
            break;
        }

    }

}

void SetPoint::creatTitle()
{
    CSubPage::creatTitle();

    buildTitles();

    selectTitle(0);
}

void SetPoint::buildTitles()
{
    QStringList stringList;

    stringList << tr("Alarm S.P");

    setTitles(stringList);

}

void SetPoint::buildTranslation()
{
    for(int iLoop = 0; iLoop < m_iRealNum; iLoop++)
    {
        switch(aIds[iLoop].iParamId[0])
        {
        case MACHINE_PARAM_SP1:
            /* 自来水压力下限1.0bar */
            pSetPlistItem[iLoop]->setName(tr("Tap Pressure"));
            pSetPlistItem[iLoop]->setP1Name(tr("Lower Limit"));
            pSetPlistItem[iLoop]->setP1Unit(tr("bar"));
            break;
        case MACHINE_PARAM_SP2:
            /* RO截留率 下限92.0% */
            pSetPlistItem[iLoop]->setName(tr("RO Rejection"));
            pSetPlistItem[iLoop]->setP1Name(tr("Lower Limit"));
            pSetPlistItem[iLoop]->setP1Unit(tr("%"));
            break;
        case MACHINE_PARAM_SP3:
            /* RO产水  上限50.0μS/cm */
            if (MACHINE_PURIST == gGlobalParam.iMachineType)
            {
                pSetPlistItem[iLoop]->setName(tr("UP Feed Cond."));
            }
            else
            {
                pSetPlistItem[iLoop]->setName(tr("RO Cond."));
            }
            pSetPlistItem[iLoop]->setP1Name(tr("Upper Limit"));
            pSetPlistItem[iLoop]->setP1Unit(tr("us/cm"));
            break;
        case MACHINE_PARAM_SP4:
            /* EDI产水 下限1.0MΩ.cm */
            pSetPlistItem[iLoop]->setName(tr("EDI Resis."));
            pSetPlistItem[iLoop]->setP1Name(tr("Lower Limit"));
            pSetPlistItem[iLoop]->setP1Unit(tr("omg"));
            break;
        case MACHINE_PARAM_SP5:
        case MACHINE_PARAM_SP6:
            /* 纯水箱液位     水箱空10%  恢复注水 80% */
            pSetPlistItem[iLoop]->setName(tr("Pure Tank Level"));
            pSetPlistItem[iLoop]->setP1Name(tr("Empty"));
            pSetPlistItem[iLoop]->setP2Name(tr("Refill"));
            pSetPlistItem[iLoop]->setP1Unit(tr("%"));
            pSetPlistItem[iLoop]->setP2Unit(tr("%"));
            break;
        case MACHINE_PARAM_SP7:
            /* UP取水下限16.0MΩ.cm */
             switch(gGlobalParam.iMachineType)
             {
             case MACHINE_L_Genie:
             case MACHINE_L_UP:
             case MACHINE_Genie:
             case MACHINE_UP:
             case MACHINE_PURIST:
             case MACHINE_ADAPT:
                 pSetPlistItem[iLoop]->setName(tr("UP Resis."));
                 break;
             }
            pSetPlistItem[iLoop]->setP1Name(tr("Lower Limit"));
            pSetPlistItem[iLoop]->setP1Unit(tr("omg"));
            break;
        case MACHINE_PARAM_SP8:
        case MACHINE_PARAM_SP9:
            /* 源水箱液位     水箱空 10%  恢复注水 80% */
            pSetPlistItem[iLoop]->setName(tr("Feed Tank Level"));
            pSetPlistItem[iLoop]->setP1Name(tr("Empty"));
            pSetPlistItem[iLoop]->setP2Name(tr("Refill"));
            pSetPlistItem[iLoop]->setP1Unit(tr("%"));
            pSetPlistItem[iLoop]->setP2Unit(tr("%"));
            break;
        case MACHINE_PARAM_SP10:
        case MACHINE_PARAM_SP11:
            /* 水箱水质  上限15.0MΩ.cm下限 1.0MΩ.cm */
            pSetPlistItem[iLoop]->setName(tr("Tank Resis."));
            pSetPlistItem[iLoop]->setP2Name(tr("Max."));
            pSetPlistItem[iLoop]->setP1Name(tr("Min."));
            pSetPlistItem[iLoop]->setP1Unit(tr("omg"));
            pSetPlistItem[iLoop]->setP2Unit(tr("omg"));
            break;
        case MACHINE_PARAM_SP12:
            /* 纯水取水  下限1.0MΩ.cm */
            pSetPlistItem[iLoop]->setName(tr("HP Resis."));
            pSetPlistItem[iLoop]->setP1Name(tr("Lower Limit"));
            pSetPlistItem[iLoop]->setP1Unit(tr("omg"));
            break;
        case MACHINE_PARAM_SP13:
            /* 自来水电导率 上限20000μS/cm */
            pSetPlistItem[iLoop]->setName(tr("Tap Cond."));
            pSetPlistItem[iLoop]->setP1Name(tr("Upper Limit"));
            pSetPlistItem[iLoop]->setP1Unit(tr("us/cm"));
            break;
        case MACHINE_PARAM_SP14:
        case MACHINE_PARAM_SP15:
            /* RO产水流速 上限100.0L/min下限20.0L/min */
            pSetPlistItem[iLoop]->setName(tr("RO Product Rate"));
            pSetPlistItem[iLoop]->setP2Name(tr("Max."));
            pSetPlistItem[iLoop]->setP1Name(tr("Min."));
            pSetPlistItem[iLoop]->setP1Unit(tr("L/h"));
            pSetPlistItem[iLoop]->setP2Unit(tr("L/h"));
            break;
        case MACHINE_PARAM_SP16:
            /*     RO弃水流速 下限20.0L/min    */
            pSetPlistItem[iLoop]->setName(tr("RO Reject Rate"));
            pSetPlistItem[iLoop]->setP1Name(tr("Lower Limit"));
            pSetPlistItem[iLoop]->setP1Unit(tr("L/h"));
            break;
        case MACHINE_PARAM_SP17:
            /*     管路水质   下限 1MΩ.cm    */
            pSetPlistItem[iLoop]->setName(tr("Loop Resis."));
            pSetPlistItem[iLoop]->setP1Name(tr("Lower Limit"));
            pSetPlistItem[iLoop]->setP1Unit(tr("omg"));
            break;
        case MACHINE_PARAM_SP18:
        case MACHINE_PARAM_SP19:
            /*     
            进水温度 上限45℃ 下限5℃
            */
            pSetPlistItem[iLoop]->setName(tr("Tap Temp."));
            pSetPlistItem[iLoop]->setP2Name(tr("Max."));
            pSetPlistItem[iLoop]->setP1Name(tr("Min."));
            pSetPlistItem[iLoop]->setP1Unit(tr("Celsius"));
            pSetPlistItem[iLoop]->setP2Unit(tr("Celsius"));
            break;
        case MACHINE_PARAM_SP20:
        case MACHINE_PARAM_SP21:
            /*     
            RO产水温度 上限45℃ 下限5℃
            */
            if (MACHINE_PURIST == gGlobalParam.iMachineType)
            {
                pSetPlistItem[iLoop]->setName(tr("UP Feed Temp."));
            }
            else
            {
                pSetPlistItem[iLoop]->setName(tr("RO Temp."));
            }
            pSetPlistItem[iLoop]->setP2Name(tr("Max."));
            pSetPlistItem[iLoop]->setP1Name(tr("Min."));
            pSetPlistItem[iLoop]->setP1Unit(tr("Celsius"));
            pSetPlistItem[iLoop]->setP2Unit(tr("Celsius"));
            break;
        case MACHINE_PARAM_SP22:
        case MACHINE_PARAM_SP23:
            /*     
            EDI产水温度 上限45℃ 下限5℃
            */
            pSetPlistItem[iLoop]->setName(tr("EDI Temp."));
            pSetPlistItem[iLoop]->setP2Name(tr("Max."));
            pSetPlistItem[iLoop]->setP1Name(tr("Min."));
            pSetPlistItem[iLoop]->setP1Unit(tr("Celsius"));
            pSetPlistItem[iLoop]->setP2Unit(tr("Celsius"));
            break;
        case MACHINE_PARAM_SP24:
        case MACHINE_PARAM_SP25:
            /*     
            UP产水温度 上限45℃ 下限5℃
            */
            switch(gGlobalParam.iMachineType)
             {
             case MACHINE_L_Genie:
             case MACHINE_L_UP:
             case MACHINE_Genie:
             case MACHINE_UP:
             case MACHINE_PURIST:
             case MACHINE_ADAPT:
                 pSetPlistItem[iLoop]->setName(tr("UP Temp."));
                 break;
             case MACHINE_L_EDI_LOOP: 
             case MACHINE_L_RO_LOOP:
             case MACHINE_EDI:
             case MACHINE_RO:
                 pSetPlistItem[iLoop]->setName(tr("HP Temp."));                     
                 break;
             }
            pSetPlistItem[iLoop]->setP2Name(tr("Max."));
            pSetPlistItem[iLoop]->setP1Name(tr("Min."));            
            pSetPlistItem[iLoop]->setP1Unit(tr("Celsius"));
            pSetPlistItem[iLoop]->setP2Unit(tr("Celsius"));
            break;
        case MACHINE_PARAM_SP26:
        case MACHINE_PARAM_SP27:
            /*     
            管路温度 上限45℃  下限5℃
            */
            pSetPlistItem[iLoop]->setName(tr("Loop Temp."));
            pSetPlistItem[iLoop]->setP2Name(tr("Max."));
            pSetPlistItem[iLoop]->setP1Name(tr("Min."));
            pSetPlistItem[iLoop]->setP1Unit(tr("Celsius"));
            pSetPlistItem[iLoop]->setP2Unit(tr("Celsius"));
            break;
        case MACHINE_PARAM_SP28:
        case MACHINE_PARAM_SP29:
            /*     
            TOC传感器温度 上限45℃ 下限5℃
            */
            pSetPlistItem[iLoop]->setName(tr("TOC Temp."));
            pSetPlistItem[iLoop]->setP2Name(tr("Max."));
            pSetPlistItem[iLoop]->setP1Name(tr("Min."));
            pSetPlistItem[iLoop]->setP1Unit(tr("Celsius"));
            pSetPlistItem[iLoop]->setP2Unit(tr("Celsius"));
            break;
        case MACHINE_PARAM_SP30:
            /*     
            TOC进水水质下限15.0MΩ.cm
            */
            pSetPlistItem[iLoop]->setName(tr("TOC Feed Resis."));
            pSetPlistItem[iLoop]->setP1Name(tr("Lower Limit"));            
            pSetPlistItem[iLoop]->setP1Unit(tr("omg"));
            break;
        case MACHINE_PARAM_SP31:
            /*     
            循环水质下限 ? MΩ.cm
            */
            pSetPlistItem[iLoop]->setName(tr("Cir Water Quality"));
            pSetPlistItem[iLoop]->setP1Name(tr("Lower threshold"));            
            pSetPlistItem[iLoop]->setP1Unit(tr("omg"));
            break;
        case MACHINE_PARAM_SP32:
            /*     
            HP产水水质下限 ? MΩ.cm
            */
            pSetPlistItem[iLoop]->setName(tr("HP Produce Water Quality"));
            pSetPlistItem[iLoop]->setP1Name(tr("Lower threshold"));            
            pSetPlistItem[iLoop]->setP1Unit(tr("omg"));
            break;
        case MACHINE_PARAM_SP33:
            /* work Pressure max 12bar */
            pSetPlistItem[iLoop]->setName(tr("Work Pressure"));
            pSetPlistItem[iLoop]->setP1Name(tr("Max."));
            pSetPlistItem[iLoop]->setP1Unit(tr("bar"));
            break;
        }
    }

    m_pBtnSave->setTip(tr("Save"), QColor(228, 231, 240), BITMAPBUTTON_TIP_CENTER);
}

void SetPoint::switchLanguage()
{
    buildTranslation();

    buildTitles();

    selectTitle(titleIndex());
}

void SetPoint::createList()
{
    //int aItems[SETPNUM] = {1,1,1,1,2,1,2,2,1,1,2,1,1,2,2,2,2,2,2,1};
    int iLoop;

    QColor colors[] = {QColor(200,200,188),QColor(228, 231, 240)};
    
    listWidget = new QListWidget((QWidget *)m_widget);

    listWidget->setStyleSheet("background-color:transparent");
    listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listWidget->setFrameShape(QListWidget::NoFrame);

    listWidget->setGeometry(QRect(10,60 ,780,490));

    for(iLoop = 0 ; iLoop < m_iRealNum ; iLoop++)
    {
        listWidgetIem[iLoop] = new QListWidgetItem;
        listWidgetIem[iLoop]->setSizeHint(QSize(600 , SET_P_LIST_WIDGET_HEIGHT));
        listWidgetIem[iLoop]->setBackground(colors[iLoop % 2]);

        pSetPlistItem[iLoop] = new SetPlistwidgtitem(NULL,aIds[iLoop].iDspType);

        listWidget->insertItem(iLoop,listWidgetIem[iLoop]);
        listWidget->setItemWidget(listWidgetIem[iLoop] , pSetPlistItem[iLoop]);

        
    }

    listWidget->setStyleSheet(
                 "QListWidget{background-color:transparent;}"  
                 "QListWidget::item:!selected{}"  
                 "QListWidget::item:selected:active{background:#F2F2F2;color:#19649F;}"  
                 "QListWidget::item:selected{background:#F2F2F2;color:#19649F;}" 
                 );
    
    connect(listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(ItemClicked(QListWidgetItem*)));
}


void SetPoint::setBackColor()
{
    QSize size(width(),height());

    QImage image_bg = QImage(size, QImage::Format_ARGB32);

    QPainter p(&image_bg);

    p.fillRect(image_bg.rect(), QColor(228, 231, 240));

    QPalette pal(m_widget->palette());

    pal.setBrush(m_widget->backgroundRole(),QBrush(image_bg));

    m_widget->setAutoFillBackground(true);
    m_widget->setPalette(pal);
}



void SetPoint::initUi()
{

    setBackColor();

    createList();

    
    m_pBtnSave = new CBitmapButton(m_widget,BITMAPBUTTON_STYLE_PUSH,BITMAPBUTTON_PIC_STYLE_NORMAL,0);

    m_pBtnSave->setButtonPicture(gpGlobalPixmaps[GLOBAL_BMP_PAGE_NAVI_NORMAL]);

    m_pBtnSave->setPressPicture(gpGlobalPixmaps[GLOBAL_BMP_PAGE_NAVI_ACTIVE]);

    m_pBtnSave->setGeometry(700,560,m_pBtnSave->getPicWidth(),m_pBtnSave->getPicHeight());

    m_pBtnSave->setStyleSheet("background-color:transparent");
    
    connect(m_pBtnSave, SIGNAL(clicked(int)), this, SLOT(on_btn_clicked(int)));

    m_pBtnSave->show();

}

void SetPoint::ItemClicked(QListWidgetItem * pItem)
{
    
}

void SetPoint::save()
{
    float fTemp;

    DISP_MACHINE_PARAM_STRU  MMParam = gGlobalParam.MMParam;  

    int iLoop;

    for(iLoop = 0 ; iLoop < m_iRealNum ; iLoop++)
    {
        switch(aIds[iLoop].iParamId[0])
        {
        case MACHINE_PARAM_SP1:
            /* 自来水压力下限1.0bar */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP1] = fTemp;
            break;
        case MACHINE_PARAM_SP2:
            /* RO截留率 下限92.0% */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP2] = fTemp;
            break;
        case MACHINE_PARAM_SP3:
            /* RO产水  上限50.0μS/cm */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP3] = fTemp;
            break;
        case MACHINE_PARAM_SP4:
            /* EDI产水 下限1.0MΩ.cm */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP4] = fTemp;
            break;
        case MACHINE_PARAM_SP6:
        case MACHINE_PARAM_SP5:
            /* 纯水箱液位     水箱空10%  恢复注水 80% */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP6] = fTemp;
            fTemp = pSetPlistItem[iLoop]->getP2().toFloat();
            MMParam.SP[MACHINE_PARAM_SP5] = fTemp;
            break;
        case MACHINE_PARAM_SP7:
            /* UP取水下限16.0MΩ.cm */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP7] = fTemp;
            break;
        case MACHINE_PARAM_SP8:
        case MACHINE_PARAM_SP9:
            /* 源水箱液位     水箱空 10%  恢复注水 80% */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP9] = fTemp;
            fTemp = pSetPlistItem[iLoop]->getP2().toFloat();
            MMParam.SP[MACHINE_PARAM_SP8] = fTemp;
            break;
        case MACHINE_PARAM_SP10:
        case MACHINE_PARAM_SP11:
            /* 水箱水质  上限15.0MΩ.cm下限 1.0MΩ.cm */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP10] = fTemp;
            fTemp = pSetPlistItem[iLoop]->getP2().toFloat();
            MMParam.SP[MACHINE_PARAM_SP11] = fTemp;
            break;
        case MACHINE_PARAM_SP12:
            /* 纯水取水  下限1.0MΩ.cm */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP12] = fTemp;
            break;
        case MACHINE_PARAM_SP13:
            /* 自来水电导率 上限20000μS/cm */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP13] = fTemp;
            break;
        case MACHINE_PARAM_SP14:
        case MACHINE_PARAM_SP15:
            /* RO产水流速 上限100.0L/min下限20.0L/min */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP15] = fTemp;
            fTemp = pSetPlistItem[iLoop]->getP2().toFloat();
            MMParam.SP[MACHINE_PARAM_SP14] = fTemp;
            break;
        case MACHINE_PARAM_SP16:
            /*     RO弃水流速 下限20.0L/min    */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP16] = fTemp;
            break;
        case MACHINE_PARAM_SP17:
            /*     管路水质   下限 1MΩ.cm    */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP17] = fTemp;
            break;
        case MACHINE_PARAM_SP18:
        case MACHINE_PARAM_SP19:
            /*     
            进水温度 上限45℃ 下限5℃
            */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP19] = fTemp;
            fTemp = pSetPlistItem[iLoop]->getP2().toFloat();
            MMParam.SP[MACHINE_PARAM_SP18] = fTemp;
            break;
        case MACHINE_PARAM_SP20:
        case MACHINE_PARAM_SP21:
            /*     
            RO产水温度 上限45℃ 下限5℃
            */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP21] = fTemp;
            fTemp = pSetPlistItem[iLoop]->getP2().toFloat();
            MMParam.SP[MACHINE_PARAM_SP20] = fTemp;
            break;
        case MACHINE_PARAM_SP22:
        case MACHINE_PARAM_SP23:
            /*     
            EDI产水温度 上限45℃ 下限5℃
            */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP23] = fTemp;
            fTemp = pSetPlistItem[iLoop]->getP2().toFloat();
            MMParam.SP[MACHINE_PARAM_SP22] = fTemp;
            break;
        case MACHINE_PARAM_SP24:
        case MACHINE_PARAM_SP25:
            /*     
            UP产水温度 上限45℃ 下限5℃
            */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP25] = fTemp;
            fTemp = pSetPlistItem[iLoop]->getP2().toFloat();
            MMParam.SP[MACHINE_PARAM_SP24] = fTemp;
            break;
        case MACHINE_PARAM_SP26:
        case MACHINE_PARAM_SP27:
            /*     
            管路温度 上限45℃  下限5℃
            */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP27] = fTemp;
            fTemp = pSetPlistItem[iLoop]->getP2().toFloat();
            MMParam.SP[MACHINE_PARAM_SP26] = fTemp;
            break;
        case MACHINE_PARAM_SP28:
        case MACHINE_PARAM_SP29:
            /*     
            TOC传感器温度 上限45℃ 下限5℃
            */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP29] = fTemp;
            fTemp = pSetPlistItem[iLoop]->getP2().toFloat();
            MMParam.SP[MACHINE_PARAM_SP28] = fTemp;
            break;
        case MACHINE_PARAM_SP30:
            /*     
            TOC进水水质下限15.0MΩ.cm
            */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP30] = fTemp;
            break;
        case MACHINE_PARAM_SP31:
            /*     
            循环水质下限 ? MΩ.cm
            */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP31] = fTemp;
            break;
        case MACHINE_PARAM_SP32:
            /*     
            HP产水水质下限 ? MΩ.cm
            */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP32] = fTemp;
            break;
        case MACHINE_PARAM_SP33:
            /* 自来水压力下限1.0bar */
            fTemp = pSetPlistItem[iLoop]->getP1().toFloat();
            MMParam.SP[MACHINE_PARAM_SP33] = fTemp;
            break;
        }
    }

    MainSaveMachineParam(gGlobalParam.iMachineType,MMParam);
    MainUpdateSpecificParam(NOT_PARAM_MACHINE_PARAM);

    m_wndMain->saveAlarmSP();
    
    m_wndMain->MainWriteLoginOperationInfo2Db(SETPAGE_SYSTEM_PARAM_CONFIG);

    show(false);
    m_parent->show(true);
}


void SetPoint::on_btn_clicked(int index)
{
    switch(index)
    {
    case 0:
        {
             /* save parameter */
             save();
        }
        break;
    }
    
    m_wndMain->prepareKeyStroke();
}

void SetPoint::update()
{
    // {1,1,1,1,2,1,2,2,1,1,2,1,1,2,2,2,2,2,2,1}

    int iLoop;
    
    for(iLoop = 0 ; iLoop < m_iRealNum ; iLoop++)
    {
        switch(aIds[iLoop].iParamId[0])
        {
        case MACHINE_PARAM_SP1:
            /* 自来水压力下限1.0bar */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP1],'f',1));
            
            break;
        case MACHINE_PARAM_SP2:
            /* RO截留率 下限92.0% */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP2],'f',1));
            break;
        case MACHINE_PARAM_SP3:
            /* RO产水  上限50.0μS/cm */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP3],'f',1));
            break;
        case MACHINE_PARAM_SP4:
            /* EDI产水 下限1.0MΩ.cm */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP4],'f',1));
            break;
        case MACHINE_PARAM_SP5:
        case MACHINE_PARAM_SP6:
            /* 纯水箱液位     水箱空10%  恢复注水 80% */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP6],'f',1));
            pSetPlistItem[iLoop]->setP2(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP5],'f',1));
            break;
        case MACHINE_PARAM_SP7:
            /* UP取水下限16.0MΩ.cm */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP7],'f',1));
            break;
        case MACHINE_PARAM_SP8:
        case MACHINE_PARAM_SP9:
            /* 源水箱液位     水箱空 10%  恢复注水 80% */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP9],'f',1));
            pSetPlistItem[iLoop]->setP2(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP8],'f',1));
            break;
        case MACHINE_PARAM_SP10:
        case MACHINE_PARAM_SP11:
            /* 水箱水质  上限15.0MΩ.cm下限 1.0MΩ.cm */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP10],'f',1));
            pSetPlistItem[iLoop]->setP2(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP11],'f',1));
            break;
        case MACHINE_PARAM_SP12:
            /* 纯水取水  下限1.0MΩ.cm */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP12],'f',1));
            break;
        case MACHINE_PARAM_SP13:
            /* 自来水电导率 上限20000μS/cm */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP13],'f',1));
            break;
        case MACHINE_PARAM_SP14:
        case MACHINE_PARAM_SP15:
            /* RO产水流速 上限100.0L/min下限20.0L/min */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP15],'f',1));
            pSetPlistItem[iLoop]->setP2(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP14],'f',1));
            
            break;
        case MACHINE_PARAM_SP16:
            /*     RO弃水流速 下限20.0L/min    */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP16],'f',1));
            
            break;
        case MACHINE_PARAM_SP17:
            /*     管路水质   下限 1MΩ.cm    */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP17],'f',1));
            break;
        case MACHINE_PARAM_SP18:
        case MACHINE_PARAM_SP19:
            /*     
            进水温度 上限45℃ 下限5℃
            */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP19],'f',1));
            pSetPlistItem[iLoop]->setP2(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP18],'f',1));
            break;
        case MACHINE_PARAM_SP20:
        case MACHINE_PARAM_SP21:
            /*     
            RO产水温度 上限45℃ 下限5℃
            */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP21],'f',1));
            pSetPlistItem[iLoop]->setP2(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP20],'f',1));
            break;
        case MACHINE_PARAM_SP22:
        case MACHINE_PARAM_SP23:
            /*     
            EDI产水温度 上限45℃ 下限5℃
            */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP23],'f',1));
            pSetPlistItem[iLoop]->setP2(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP22],'f',1));
            break;
        case MACHINE_PARAM_SP24:
        case MACHINE_PARAM_SP25:
            /*     
            UP产水温度 上限45℃ 下限5℃
            */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP25],'f',1));
            pSetPlistItem[iLoop]->setP2(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP24],'f',1));
            break;
        case MACHINE_PARAM_SP26:
        case MACHINE_PARAM_SP27:
            /*     
            管路温度 上限45℃  下限5℃
            */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP27],'f',1));
            pSetPlistItem[iLoop]->setP2(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP26],'f',1));
            break;
        case MACHINE_PARAM_SP28:
        case MACHINE_PARAM_SP29:
            /*     
            TOC传感器温度 上限45℃ 下限5℃
            */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP29],'f',1));
            pSetPlistItem[iLoop]->setP2(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP28],'f',1));
            break;
        case MACHINE_PARAM_SP30:
            /*     
            TOC进水水质下限15.0MΩ.cm
            */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP30],'f',1));
            break;
        case MACHINE_PARAM_SP31:
            /*     
            循环水质下限 ? MΩ.cm
            */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP31],'f',1));
            break;
        case MACHINE_PARAM_SP32:
            /*     
            HP产水水质下限 ? MΩ.cm
            */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP32],'f',1));
            break;
        case MACHINE_PARAM_SP33:
            /* 自来水压力下限1.0bar */
            pSetPlistItem[iLoop]->setP1(QString::number(gGlobalParam.MMParam.SP[MACHINE_PARAM_SP33],'f',1));

            break;
        }
    }
    
}

