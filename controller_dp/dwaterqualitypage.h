#ifndef DWATERQUALITYPAGE_H
#define DWATERQUALITYPAGE_H

#include "Display.h"
#include "subpage.h"
#include "dwaterqualitywidget.h"
class CCB;

class DWaterQualityPage : public CSubPage
{
    Q_OBJECT ;

public:
    DWaterQualityPage(QObject *parent = 0,CBaseWidget *widget = 0 , MainWindow *wndMain = 0);
    virtual void creatTitle();

    virtual void switchLanguage();

    virtual void buildTranslation();

    virtual void initUi();

    virtual void update();

    void updEcoInfo(int iIndex,ECO_INFO_STRU *info);

    void updAllInfo(void);

    void updPressure(int iIndex,float fvalue);

    void updFlowInfo(int iIndex,int Value);

    void updTank(int iIndex,float fVolume);
    void updSourceTank(int iIndex, float fVolume);

    void updSwPressure(float fvalue);

    void updTOC(float fToc); //2018.11.13

private:
    void initAllValue();
    void initTagsArray();
    void initConfigList();
    void updateValue(const DTags& t, const QString& value1, const QString& value2 = "--");

    void updHistoryEcoInfo();
    void updHistoryPressure();
    void updHistoryFlowInfo();
    void updHistoryTank();
    void updHistoryTOC();

    void setBackColor();
    void buildTitles();

private:
    DWaterQualityWidget* m_pQualityWidget;
    QList<DTags> m_cfglist;
    QString strMsg[MSG_NUM];
    QString strUnitMsg[UNIT_MSG_NUM];

    DTags m_tags[MSG_NUM];

    unsigned int    m_aulFlowMeter[APP_FM_FLOW_METER_NUM];
    struct Show_History_Info *m_historyInfo;

    bool m_showFirst;
    CCB  *m_pCcb;
};

#endif // EX_WATERQUALITYPAGE_H
