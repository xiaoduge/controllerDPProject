#include "crecordparams.h"
#include "mainwindow.h"
#include "exconfig.h"

CRecordParams::CRecordParams(QObject *parent) : QObject(parent)
{
    m_pCcb = CCB::getInstance();

    m_historyInfo = m_pCcb->DispGetHistInfo();

    for(int i = 0; i < MSG_NUM; ++i)
    {
        m_historyInfo[i].value1 = 0;
        m_historyInfo[i].value2 = 0;
    } 
}

void CRecordParams::updEcoInfo(int iIndex, ECO_INFO_STRU *info)
{
    switch(iIndex)
    {
    case APP_EXE_I5_NO:
        if (m_pCcb->DispGetUpQtwFlag() || m_pCcb->DispGetUpCirFlag())
        {
            m_historyInfo[UP_Resis].value1 = info->fQuality;
            m_historyInfo[UP_Resis].value2 = info->fTemperature;
        }
        break;
    case APP_EXE_I4_NO:
        if (m_pCcb->DispGetTocCirFlag())
        {
            m_historyInfo[HP_Resis].value1 = info->fQuality;
            m_historyInfo[HP_Resis].value2 = info->fTemperature;
        }
        else if (m_pCcb->DispGetEdiQtwFlag() || m_pCcb->DispGetTankCirFlag())
        {
            switch(gGlobalParam.iMachineType)
            {
            case MACHINE_PURIST:
            case MACHINE_C_D:
            case MACHINE_RO:
            case MACHINE_RO_H:
            case MACHINE_C:
            case MACHINE_UP:
                break;
            default:
                m_historyInfo[HP_Resis].value1 = info->fQuality;
                m_historyInfo[HP_Resis].value2 = info->fTemperature;
                break;
            }

        }
        break;
    case APP_EXE_I3_NO:
        m_historyInfo[EDI_Product].value1 = info->fQuality;
        m_historyInfo[EDI_Product].value2 = info->fTemperature;

        if(gGlobalParam.MiscParam.ulMisFlags & (1 << DISP_SM_HP_Water_Cir))
        {
            switch(gGlobalParam.iMachineType)
            {
            case MACHINE_RO:
            case MACHINE_RO_H:
            case MACHINE_C:
            case MACHINE_UP:
                if (m_pCcb->DispGetEdiQtwFlag() || m_pCcb->DispGetTankCirFlag())
                {
                    m_historyInfo[HP_Resis].value1 = info->fQuality;
                    m_historyInfo[HP_Resis].value2 = info->fTemperature;
                }
                break;
            default:
                break;
            }
        }
        break;
    case APP_EXE_I2_NO: // RO Out
    {
        float fResidue;

        if (m_pCcb->DispGetREJ(&fResidue)
            && (MACHINE_PURIST != gGlobalParam.iMachineType)
            && (MACHINE_C_D    != gGlobalParam.iMachineType))
        {
            m_historyInfo[RO_Rejection].value1 = fResidue;
        }

        if((MACHINE_PURIST != gGlobalParam.iMachineType) && (MACHINE_C_D != gGlobalParam.iMachineType))
        {
            m_historyInfo[RO_Product].value1 = info->fQuality;
            m_historyInfo[RO_Product].value2 = info->fTemperature;

            switch(gGlobalParam.iMachineType)
            {
            case MACHINE_RO:
            case MACHINE_RO_H:
            case MACHINE_C:
            case MACHINE_UP:
                if(!(gGlobalParam.MiscParam.ulMisFlags & (1 << DISP_SM_HP_Water_Cir)))
                {
                    if (m_pCcb->DispGetEdiQtwFlag())
                    {
                        m_historyInfo[HP_Resis].value1 = info->fQuality;
                        m_historyInfo[HP_Resis].value2 = info->fTemperature;
                    }
                }
                break;
            default:
                break;
            }
        }
        else
        {
            m_historyInfo[UP_IN].value1 = info->fQuality;
            m_historyInfo[UP_IN].value2 = info->fTemperature;
        }
        break;
    }
    case APP_EXE_I1_NO: // RO In
        if (m_pCcb->DispGetInitRunFlag())
        {
            m_historyInfo[Tap_Cond].value1 = info->fQuality;
            m_historyInfo[Tap_Cond].value2 = info->fTemperature;
        }
        else
        {
            m_historyInfo[RO_Feed_Cond].value1 = info->fQuality;
            m_historyInfo[RO_Feed_Cond].value2 = info->fTemperature;

            m_historyInfo[Tap_Cond].value2 = info->fTemperature;
        }
        break;
    }
}

void CRecordParams::updPressure(int iIndex,float fvalue)
{
    if (APP_EXE_PM1_NO == iIndex)
    {
        m_historyInfo[RO_Pressure].value1 = fvalue;
    }
}

void CRecordParams::updSwPressure(float fvalue)
{
    m_historyInfo[RO_Feed_Pressure].value1 = fvalue;
}

void CRecordParams::updTOC(float fToc)
{
    m_historyInfo[TOC_Value].value1 = fToc;
}

void CRecordParams::updFlowInfo(int iIndex,int iValue)
{
    switch(iIndex)
    {
    case APP_FM_FM1_NO:
        if (m_pCcb->DispGetUpQtwFlag()
            || m_pCcb->DispGetUpCirFlag())
        {
            if (0 != m_historyInfo[HP_Disp_Rate].value1)
            {
                m_historyInfo[HP_Disp_Rate].value1 = 0;
            }
            m_historyInfo[UP_Disp_Rate].value1 = ((iValue*1.0)/1000);
        }
        else if(m_pCcb->DispGetEdiQtwFlag() 
               || m_pCcb->DispGetTankCirFlag()) //0629
        {
            if (0 != m_historyInfo[UP_Disp_Rate].value1)
            {
                m_historyInfo[UP_Disp_Rate].value1 = 0;
            }
            m_historyInfo[HP_Disp_Rate].value1 = ((iValue*1.0)/1000);
        }
        else
        {
            if (0 != m_historyInfo[UP_Disp_Rate].value1)
            {
                m_historyInfo[UP_Disp_Rate].value1 = 0;
            }
            if (0 != m_historyInfo[HP_Disp_Rate].value1)
            {
                m_historyInfo[HP_Disp_Rate].value1 = 0;
            }
        }
        break;
    default:
        break;
   }
}

void CRecordParams::updTank(int iLevel,float fVolume)
{
    m_historyInfo[Pure_Tank_Level].value1 = iLevel;
    m_historyInfo[Pure_Tank_Level].value2 = fVolume;
}

void CRecordParams::updSourceTank(int iLevel, float fVolume)
{
    m_historyInfo[Source_Tank_Level].value1 = iLevel;
    m_historyInfo[Source_Tank_Level].value2 = fVolume;
}


