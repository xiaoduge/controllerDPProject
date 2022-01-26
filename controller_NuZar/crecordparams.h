#ifndef CRECORDPARAMS_H
#define CRECORDPARAMS_H

#include "Display.h"
#include <QObject>

class CCB;

class CRecordParams : public QObject
{
    Q_OBJECT
public:
    CRecordParams(QObject *parent = 0);

    void updEcoInfo(int iIndex,ECO_INFO_STRU *info);
    void updPressure(int iIndex,float fvalue);
    void updFlowInfo(int iIndex,int Value);
    void updTank(int iIndex,float fVolume);
    void updSourceTank(int iIndex, float fVolume);
    void updSwPressure(float fvalue);
    void updTOC(float fToc); //2018.11.13

private:
    struct Show_History_Info *m_historyInfo;

    CCB  *m_pCcb;
};

#endif
