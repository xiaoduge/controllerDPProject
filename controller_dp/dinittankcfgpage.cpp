#include "dinittankcfgpage.h"
#include <QMouseEvent>
#include "mainwindow.h"
#include "dlineedit.h"
#include "exdisplay.h"
#include <QLabel>
#include <QComboBox>

#define BACKWIDGET_START_X      10
#define BACKWIDGET_START_Y      80
#define BACKWIDGET_START_HIATUS 80
#define BACKWIDGET_HEIGHT       60
#define BACKWIDGET_WIDTH        (800 - BACKWIDGET_START_X*2)
#define BACKWIDGET_ITEM_LENGTH  120
#define BACKWIDGET_ITEM_HEIGHT  30

static QRect sQrectAry[2] = 
{
    QRect(5,  BACKWIDGET_HEIGHT/2 - BACKWIDGET_ITEM_HEIGHT/2, BACKWIDGET_ITEM_LENGTH + 75, BACKWIDGET_ITEM_HEIGHT) ,
    QRect(60, 2, 110 , 18) ,
};

DInitTankcfgpage::DInitTankcfgpage(QObject *parent,CBaseWidget *widget ,MainWindow *wndMain) : CSubPage(parent,widget,wndMain)
{
    creatTitle();
    initUi();
    buildTranslation();
    this->hideTitleBar();
}

void DInitTankcfgpage::creatTitle()
{
    CSubPage::creatTitle();

    buildTitles();

    selectTitle(0);
}

void DInitTankcfgpage::buildTitles()
{
    QStringList stringList;

    stringList << tr("System Config");

    setTitles(stringList);
}

void DInitTankcfgpage::buildTranslation()
{
    m_pExLbTitle->setText(tr("Tank Set"));

    QFont font = m_lbPWTankName->font();
    font.setPointSize(14);
    m_lbPWTankName->setText(tr("Pure Water Tank"));
    m_lbPWTankName->setFont(font);
    m_cmbPWTankVolume->setItemText(5,tr("UDF"));
    m_cmbPWTankVolume->setItemText(6,tr("NO"));

    m_lbSWTankName->setText(tr("Feed Tank"));
    m_lbSWTankName->setFont(font);
    m_cmbSWTankVolume->setItemText(5,tr("UDF"));
    m_cmbSWTankVolume->setItemText(6,tr("NO"));

    m_pExNextBtn->setText(tr("Next"));
    m_pExBackBtn->setText(tr("Back"));

    m_lbPWHUnit->setText(tr("Height(M)"));
    m_lbPWCUnit->setText(tr("Volume(L)"));
    m_lbSWHUnit->setText(tr("Height(M)"));
    m_lbSWCUnit->setText(tr("Volume(L)"));

    m_pPureRangeLab->setText(tr("Pure Tank Level Sensor Range"));
    m_pFeedRangeLab->setText(tr("Feed Tank Level Sensor Range"));
}

void DInitTankcfgpage::switchLanguage()
{
    buildTranslation();

    buildTitles();

    selectTitle(titleIndex());
}

void DInitTankcfgpage::setBackColor()
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

#define X_MARGIN (5)
#define X_ITEM_WIDTH (60)
#define X_VALUE_WIDTH (60)

void DInitTankcfgpage::createControl()
{
    QRegExp sensorRangerx("([0-9]{1}[\.][0-9]{0,3})");
    QRegExp double_rx("([0-9]{0,1}[\.][0-9]{0,2})");

    QWidget *tmpWidget = NULL;
    QRect    rectTmp;
    int iLine = 0;    

    /* line 1*/
    tmpWidget = new QWidget(m_widget);
    QPalette pal(tmpWidget->palette());
    pal.setColor(QPalette::Background, QColor(255,255,255));

    tmpWidget->setAutoFillBackground(true);
    tmpWidget->setPalette(pal);

    tmpWidget->setGeometry(QRect(50 , 150 + 80 * iLine, 640 ,60));

    rectTmp = sQrectAry[0];
    rectTmp.setX(10);
    m_lbPWTankName = new QLabel(tmpWidget);
    m_lbPWTankName->setGeometry(rectTmp);
    m_lbPWTankName->hide();

    rectTmp.setX(210);
    rectTmp.setWidth(80);
    m_cmbPWTankVolume = new QComboBox(tmpWidget);
    m_cmbPWTankVolume->setGeometry(rectTmp);

    m_cmbPWTankVolume->addItem(tr("30"));
    m_cmbPWTankVolume->addItem(tr("60"));
    m_cmbPWTankVolume->addItem(tr("100"));
    m_cmbPWTankVolume->addItem(tr("200"));
    m_cmbPWTankVolume->addItem(tr("350"));
    m_cmbPWTankVolume->addItem(tr("UDF"));

    if(MACHINE_PURIST== gGlobalParam.iMachineType) m_cmbPWTankVolume->addItem(tr("NO"));

    connect(m_cmbPWTankVolume, SIGNAL(currentIndexChanged(int)),
            this, SLOT(on_CmbIndexChange_pw(int)));

    m_cmbPWTankVolume->hide();

    rectTmp.setX(300);
    rectTmp.setWidth(60);
    m_lePWTankHeight = new DLineEdit(tmpWidget);
    m_lePWTankHeight->setGeometry(rectTmp);
    m_lePWTankHeight->setValidator(new QRegExpValidator(double_rx,this));
    m_lePWTankHeight->hide();

    rectTmp.setX(365);
    rectTmp.setWidth(90);
    m_lbPWHUnit = new QLabel(tmpWidget);
    m_lbPWHUnit->setGeometry(rectTmp);
    m_lbPWHUnit->setText(tr("Height(M)"));
    m_lbPWHUnit->hide();

    rectTmp.setX(465);
    rectTmp.setWidth(60);
    m_lePWTankCap = new DLineEdit(tmpWidget);
    m_lePWTankCap->setGeometry(rectTmp);
    m_lePWTankCap->setValidator(new QIntValidator(0, 9999, this));
    m_lePWTankCap->hide();

    rectTmp.setX(530);
    rectTmp.setWidth(90);
    m_lbPWCUnit = new QLabel(tmpWidget);
    m_lbPWCUnit->setGeometry(rectTmp);
    m_lbPWCUnit->setText(tr("Volume(L)"));
    m_lbPWCUnit->hide();

#if 1
    if (m_wndMain->m_pCcb->getWaterTankCfgMask() & (1 << DISP_SM_HaveB2))
    {
        m_lbPWTankName->show();
        m_cmbPWTankVolume->show();
        m_lePWTankHeight->show();
        m_lbPWHUnit->show();
        m_lePWTankCap->show();
        m_lbPWCUnit->show();
    }
    else
    {
        tmpWidget->setAutoFillBackground(false);
    }
#else
    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_ADAPT:
        tmpWidget->setAutoFillBackground(false);
        break;
    default:
        m_lbPWTankName->show();
        m_cmbPWTankVolume->show();
        m_lePWTankHeight->show();
        m_lbPWHUnit->show();
        m_lePWTankCap->show();
        m_lbPWCUnit->show();
        break;
    }
#endif
    /* line 2*/
    ++iLine;
    tmpWidget = new QWidget(m_widget);
    tmpWidget->setGeometry(QRect(50 , 150 + 80 * iLine, 640 ,60));
    tmpWidget->setAutoFillBackground(true);
    tmpWidget->setPalette(pal);

    rectTmp.setX(10);
    rectTmp.setWidth(sQrectAry[0].width() + 60);

    m_pPureRangeLab = new QLabel(tmpWidget);
    m_pPureRangeLab->setGeometry(rectTmp);
    m_pPureRangeLab->hide();

    rectTmp.setX(300);
    rectTmp.setWidth(80);
    m_pPureRangeEdit = new DLineEdit(tmpWidget);
    m_pPureRangeEdit->setValidator(new QRegExpValidator(sensorRangerx, this));
    m_pPureRangeEdit->setGeometry(rectTmp);
    m_pPureRangeEdit->setText(QString("%1").arg(gSensorRange.fPureSRange));
    m_pPureRangeEdit->hide();

    rectTmp.setX(390);
    rectTmp.setWidth(60);
    m_pPureRangeUnit = new QLabel(tmpWidget);
    m_pPureRangeUnit->setText(tr("bar"));
    m_pPureRangeUnit->setGeometry(rectTmp);
    m_pPureRangeUnit->hide();

#if 1
    if (m_wndMain->m_pCcb->getWaterTankCfgMask() & (1 << DISP_SM_HaveB2))
    {
        m_pPureRangeLab->show();
        m_pPureRangeEdit->show();
        m_pPureRangeUnit->show();
    }
    else
    {
        tmpWidget->setAutoFillBackground(false);
    }
#else

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_ADAPT:
        tmpWidget->setAutoFillBackground(false);
        break;
    default:
        m_pPureRangeLab->show();
        m_pPureRangeEdit->show();
        m_pPureRangeUnit->show();
        break;
    }
#endif
    /* line 3*/
    ++iLine;
    tmpWidget = new QWidget(m_widget);
    tmpWidget->setGeometry(QRect(50 , 150 + 80 * iLine, 640 ,60));

    tmpWidget->setAutoFillBackground(true);
    tmpWidget->setPalette(pal);

    rectTmp.setX(10);
    rectTmp.setWidth(sQrectAry[0].width());
    m_lbSWTankName = new QLabel(tmpWidget);
    m_lbSWTankName->setGeometry(rectTmp);
    m_lbSWTankName->hide();

    rectTmp.setX(210);
    rectTmp.setWidth(60+20);
    m_cmbSWTankVolume = new QComboBox(tmpWidget);
    m_cmbSWTankVolume->setGeometry(rectTmp);

    m_cmbSWTankVolume->addItem(tr("30"));
    m_cmbSWTankVolume->addItem(tr("60"));
    m_cmbSWTankVolume->addItem(tr("100"));
    m_cmbSWTankVolume->addItem(tr("200"));
    m_cmbSWTankVolume->addItem(tr("350"));
    m_cmbSWTankVolume->addItem(tr("UDF"));
    m_cmbSWTankVolume->addItem(tr("NO"));
    connect(m_cmbSWTankVolume, SIGNAL(currentIndexChanged(int)),
            this, SLOT(on_CmbIndexChange_sw(int)));

    m_cmbSWTankVolume->hide();

    rectTmp.setX(300);
    rectTmp.setWidth(60);
    m_leSWTankHeight = new DLineEdit(tmpWidget);
    m_leSWTankHeight->setGeometry(rectTmp);
    m_leSWTankHeight->setValidator(new QRegExpValidator(double_rx,this));
    m_leSWTankHeight->hide();

    rectTmp.setX(365);
    rectTmp.setWidth(90);
    m_lbSWHUnit = new QLabel(tmpWidget);
    m_lbSWHUnit->setGeometry(rectTmp);
    m_lbSWHUnit->setText(tr("Height(M)"));
    m_lbSWHUnit->hide();

    rectTmp.setX(465);
    rectTmp.setWidth(60);
    m_leSWTankCap = new DLineEdit(tmpWidget);
    m_leSWTankCap->setGeometry(rectTmp);
    m_leSWTankCap->setValidator(new QIntValidator(0, 9999, this));
    m_leSWTankCap->hide();

    rectTmp.setX(530);
    rectTmp.setWidth(90);
    m_lbSWCUnit = new QLabel(tmpWidget);
    m_lbSWCUnit->setGeometry(rectTmp);
    m_lbSWCUnit->setText(tr("Volume(L)"));
    m_lbSWCUnit->hide();

#if 1
    if (m_wndMain->m_pCcb->getWaterTankCfgMask() & (1 << DISP_SM_HaveB3))
    {
        m_lbSWTankName->show();
        m_cmbSWTankVolume->show();
        m_leSWTankHeight->show();
        m_lbSWHUnit->show();
        m_leSWTankCap->show();
        m_lbSWCUnit->show();
    }
    else
    {
        tmpWidget->setAutoFillBackground(false);
    }
#else
    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
        m_lbSWTankName->show();
        m_cmbSWTankVolume->show();
        m_leSWTankHeight->show();
        m_lbSWHUnit->show();
        m_leSWTankCap->show();
        m_lbSWCUnit->show();
        break;
    default:
        tmpWidget->setAutoFillBackground(false);
        break;
    }
#endif
    /* line 4*/
    ++iLine;
    tmpWidget = new QWidget(m_widget);
    tmpWidget->setGeometry(QRect(50 , 150 + 80 * iLine, 640 ,60));

    tmpWidget->setAutoFillBackground(true);
    tmpWidget->setPalette(pal);

    rectTmp.setX(10);
    rectTmp.setWidth(sQrectAry[0].width() + 60);

    m_pFeedRangeLab = new QLabel(tmpWidget);
    m_pFeedRangeLab->setGeometry(rectTmp);

    rectTmp.setX(300);
    rectTmp.setWidth(80);
    m_pFeedRangeEdit = new DLineEdit(tmpWidget);
    m_pFeedRangeEdit->setValidator(new QRegExpValidator(sensorRangerx, this));
    m_pFeedRangeEdit->setGeometry(rectTmp);
    m_pFeedRangeEdit->setText(QString("%1").arg(gSensorRange.fFeedSRange));

    rectTmp.setX(390);
    rectTmp.setWidth(60);
    m_pFeedRangeUnit = new QLabel(tmpWidget);
    m_pFeedRangeUnit->setText(tr("bar"));
    m_pFeedRangeUnit->setGeometry(rectTmp);

    tmpWidget->hide();

#if 1
    if (m_wndMain->m_pCcb->getWaterTankCfgMask() & (1 << DISP_SM_HaveB3))
    {
        tmpWidget->show();
     }
 #else
    
    switch(gGlobalParam.iMachineType)
    {
     case MACHINE_L_Genie:
     case MACHINE_L_UP:
     case MACHINE_L_EDI_LOOP:
     case MACHINE_L_RO_LOOP:
        tmpWidget->show();
        break;
    }
#endif
    //
    m_pExNextBtn = new QPushButton(m_widget);
    m_pExBackBtn = new QPushButton(m_widget);

    connect(m_pExNextBtn, SIGNAL(clicked()), this, SLOT(on_ExNextBtn_clicked()));
    connect(m_pExBackBtn, SIGNAL(clicked()), this, SLOT(on_ExBackBtn_clicked()));

    m_pExBackBtn->move(200, 470);
    m_pExNextBtn->move(500, 470);
}


void DInitTankcfgpage::initUi()
{
    setBackColor();
    createHeader();
    createControl();
    connectData();
}

void DInitTankcfgpage::update()
{
    connectData();
}

void DInitTankcfgpage::createHeader()
{
    m_pExLbTitle = new QLabel(m_widget);
    m_pExLbTitle->setGeometry(QRect(50, 100, 300, 28));
    m_pExLbTitle->setStyleSheet(" font-size:24pt;color:#000000;font-family:Arial;QFont::Bold");
}

void DInitTankcfgpage::mousePressEvent(QMouseEvent *e)
{
    if (!m_lstFlag)
    {
        m_lstX = e->x();
        m_lstY = e->y();
        m_curX = e->x();
        m_curY = e->y();
        m_lstFlag = 1;
    }
}

void DInitTankcfgpage::mouseMoveEvent(QMouseEvent *e)
{
    if (0 == e->x()
        && 0 == e->y())
    {
       return;
    }

    m_curX = e->x();
    m_curY = e->y();
}

void DInitTankcfgpage::mouseReleaseEvent(QMouseEvent *e)
{
    if (abs(m_curX - m_lstX) >= PAGE_X_DIMENSION
        && abs(m_curY - m_lstY) <= PAGE_Y_DIMENSION)
    {
        save();
        m_wndMain->naviInitPage(Ex_Init_Tankcfg, m_curX - m_lstX > 0 ? 1 : 0);
    }
    m_lstFlag = 0;
}

void DInitTankcfgpage::connectData()
{
    int iIdx = gGlobalParam.PmParam.aiBuckType[DISP_PM_PM2];

    if (iIdx > DISP_WATER_BARREL_TYPE_NUM )
    {
        iIdx = DISP_WATER_BARREL_TYPE_030L;
        m_pPureRangeEdit->setEnabled(false);
    }

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_ADAPT:
        break;
    default:
    {
        if (m_cmbPWTankVolume->isVisible())
        {
            if (iIdx <= DISP_WATER_BARREL_TYPE_NO)
            {
                m_cmbPWTankVolume->setCurrentIndex(iIdx);

                if (DISP_WATER_BARREL_TYPE_UDF != iIdx)
                {
                    m_lePWTankHeight->hide();
                    m_lbPWHUnit->hide();
                    m_lePWTankCap->hide();
                    m_lbPWCUnit->hide();
                    m_pPureRangeEdit->setEnabled(false);
                }
                else
                {
                    m_lePWTankHeight->show();
                    m_lbPWHUnit->show();
                    m_lePWTankCap->show();
                    m_lbPWCUnit->show();
                    m_lePWTankHeight->setText(QString::number(gGlobalParam.PmParam.afDepth[DISP_PM_PM2],'f',2));
                    m_lePWTankCap->setText(QString::number(gGlobalParam.PmParam.afCap[DISP_PM_PM2]));
                    m_pPureRangeEdit->setEnabled(true);
                }
            }   
        }
        break;
    }
    }

    iIdx = gGlobalParam.PmParam.aiBuckType[DISP_PM_PM3];

    if (iIdx > DISP_WATER_BARREL_TYPE_NUM )
    {
        iIdx = DISP_WATER_BARREL_TYPE_030L;
        m_pFeedRangeEdit->setEnabled(false);
    }

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
    {
        if (m_cmbSWTankVolume->isVisible())
        {
            if (iIdx <= DISP_WATER_BARREL_TYPE_NO)
            {
                m_cmbSWTankVolume->setCurrentIndex(iIdx);

                if (DISP_WATER_BARREL_TYPE_UDF != iIdx)
                {
                    m_leSWTankHeight->hide();
                    m_lbSWHUnit->hide();
                    m_leSWTankCap->hide();
                    m_lbSWCUnit->hide();
                    m_pFeedRangeEdit->setEnabled(false);
                }
                else
                {
                    m_leSWTankHeight->show();
                    m_lbSWHUnit->show();
                    m_leSWTankCap->show();
                    m_lbSWCUnit->show();
                    m_leSWTankHeight->setText(QString::number(gGlobalParam.PmParam.afDepth[DISP_PM_PM3],'f',2));
                    m_leSWTankCap->setText(QString::number(gGlobalParam.PmParam.afCap[DISP_PM_PM3]));
                    m_pFeedRangeEdit->setEnabled(true);
                }
            }
        }
    }
        break;
    }
    m_pPureRangeEdit->setText(QString("%1").arg(gSensorRange.fPureSRange));
    m_pFeedRangeEdit->setText(QString("%1").arg(gSensorRange.fFeedSRange));
}

void DInitTankcfgpage::save()
{
    DISP_PM_SETTING_STRU          pmParam = gGlobalParam.PmParam;
    DISP_SUB_MODULE_SETTING_STRU  smParam = gGlobalParam.SubModSetting;

    pmParam.aiBuckType[DISP_PM_PM2] = m_cmbPWTankVolume->currentIndex();

    if (m_cmbPWTankVolume->isVisible())
    {
        smParam.ulFlags |= 1 << DISP_SM_HaveB2;

        switch (pmParam.aiBuckType[DISP_PM_PM2])
        {
        case DISP_WATER_BARREL_TYPE_030L:
            pmParam.afCap[DISP_PM_PM2] = 30;
            pmParam.afDepth[DISP_PM_PM2] = 0.30;
            break;
        case DISP_WATER_BARREL_TYPE_060L:
            pmParam.afCap[DISP_PM_PM2] = 60;
            pmParam.afDepth[DISP_PM_PM2] = 0.60;
            break;
        case DISP_WATER_BARREL_TYPE_100L:
            pmParam.afCap[DISP_PM_PM2] = 100;
            pmParam.afDepth[DISP_PM_PM2] = 1.00;
            break;
        case DISP_WATER_BARREL_TYPE_200L:
            pmParam.afCap[DISP_PM_PM2] = 200;
            pmParam.afDepth[DISP_PM_PM2] = 1.00;
            break;
        case DISP_WATER_BARREL_TYPE_350L:
            pmParam.afCap[DISP_PM_PM2] = 350;
            pmParam.afDepth[DISP_PM_PM2] = 1.00;
            break;
        case DISP_WATER_BARREL_TYPE_UDF:
            pmParam.afCap[DISP_PM_PM2] = m_lePWTankCap->text().toInt();
            pmParam.afDepth[DISP_PM_PM2] = m_lePWTankHeight->text().toFloat();
            break;
        case DISP_WATER_BARREL_TYPE_NO:
            smParam.ulFlags &= ~(1 << DISP_SM_HaveB2);
            break;
        }
    }
    else
    {
        smParam.ulFlags &= ~(1 << DISP_SM_HaveB2);
    }

    pmParam.aiBuckType[DISP_PM_PM3] = m_cmbSWTankVolume->currentIndex();

    if (m_cmbSWTankVolume->isVisible())
    {
        smParam.ulFlags |= 1 << DISP_SM_HaveB3;

        switch (pmParam.aiBuckType[DISP_PM_PM3])
        {
        case DISP_WATER_BARREL_TYPE_030L:
            pmParam.afCap[DISP_PM_PM3] = 30;
            pmParam.afDepth[DISP_PM_PM3] = 0.30;
            break;
        case DISP_WATER_BARREL_TYPE_060L:
            pmParam.afCap[DISP_PM_PM3] = 60;
            pmParam.afDepth[DISP_PM_PM3] = 0.60;
            break;
        case DISP_WATER_BARREL_TYPE_100L:
            pmParam.afCap[DISP_PM_PM3] = 100;
            pmParam.afDepth[DISP_PM_PM3] = 1.00;
            break;
        case DISP_WATER_BARREL_TYPE_200L:
            pmParam.afCap[DISP_PM_PM3] = 200;
            pmParam.afDepth[DISP_PM_PM3] = 1.00;
            break;
        case DISP_WATER_BARREL_TYPE_350L:
            pmParam.afCap[DISP_PM_PM3] = 350;
            pmParam.afDepth[DISP_PM_PM3] = 1.00;
            break;
        case DISP_WATER_BARREL_TYPE_UDF:
            pmParam.afCap[DISP_PM_PM3] = m_leSWTankCap->text().toInt();
            pmParam.afDepth[DISP_PM_PM3] = m_leSWTankHeight->text().toFloat();
            break;
        case DISP_WATER_BARREL_TYPE_NO:
            smParam.ulFlags &= ~(1 << DISP_SM_HaveB3);
            break;
        }
    }
    else
    {
        smParam.ulFlags &= ~(1 << DISP_SM_HaveB3);
    }

    if(DISP_WATER_BARREL_TYPE_UDF == pmParam.aiBuckType[DISP_PM_PM2])
    {
        gSensorRange.fPureSRange = m_pPureRangeEdit->text().toFloat();
    }
    else
    {
        gSensorRange.fPureSRange = 0.2;
    }

    if(DISP_WATER_BARREL_TYPE_UDF == pmParam.aiBuckType[DISP_PM_PM3])
    {
        gSensorRange.fFeedSRange = m_pFeedRangeEdit->text().toFloat();
    }
    else
    {
        gSensorRange.fFeedSRange = 0.2;
    }
    MainSaveSensorRange(gGlobalParam.iMachineType);

    MainSavePMParam(gGlobalParam.iMachineType,pmParam);
    MainSaveSubModuleSetting(gGlobalParam.iMachineType,smParam);
    MainUpdateGlobalParam();
}

void DInitTankcfgpage::on_CmbIndexChange_pw(int index)
{
    int iIdx = m_cmbPWTankVolume->currentIndex();

    (void)index;

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_ADAPT:
        break;
    default:
    {
        if (m_cmbPWTankVolume->isVisible())
        {
            if (DISP_WATER_BARREL_TYPE_UDF == iIdx)
            {
                m_lePWTankHeight->show();
                m_lbPWHUnit->show();
                m_lePWTankCap->show();
                m_lbPWCUnit->show();
                m_lePWTankHeight->setText(QString::number(gGlobalParam.PmParam.afDepth[DISP_PM_PM2],'f',2));
                m_lePWTankCap->setText(QString::number(gGlobalParam.PmParam.afCap[DISP_PM_PM2]));
                m_pPureRangeEdit->setEnabled(true);
            }
            else
            {
                m_lePWTankHeight->hide();
                m_lbPWHUnit->hide();
                m_lePWTankCap->hide();
                m_lbPWCUnit->hide();
                m_pPureRangeEdit->setEnabled(false);
            }
        }
        break;
    }
    }
}

void DInitTankcfgpage::on_CmbIndexChange_sw(int index)
{
    int iIdx = m_cmbSWTankVolume->currentIndex();
    (void)index;

    switch(gGlobalParam.iMachineType)
    {
    case MACHINE_L_Genie:
    case MACHINE_L_UP:
    case MACHINE_L_EDI_LOOP:
    case MACHINE_L_RO_LOOP:
    {
        if (m_cmbSWTankVolume->isVisible())
        {
            if (DISP_WATER_BARREL_TYPE_UDF == iIdx)
            {
                m_leSWTankHeight->show();
                m_lbSWHUnit->show();
                m_leSWTankCap->show();
                m_lbSWCUnit->show();

                m_leSWTankHeight->setText(QString::number(gGlobalParam.PmParam.afDepth[DISP_PM_PM3],'f',2));
                m_leSWTankCap->setText(QString::number(gGlobalParam.PmParam.afCap[DISP_PM_PM3]));
                m_pFeedRangeEdit->setEnabled(true);
            }
            else
            {
                m_leSWTankHeight->hide();
                m_lbSWHUnit->hide();
                m_leSWTankCap->hide();
                m_lbSWCUnit->hide();
                m_pFeedRangeEdit->setEnabled(false);
            }
        }
    }
        break;
    }
}

void DInitTankcfgpage::on_ExNextBtn_clicked()
{
    save();
    m_wndMain->naviInitPage(Ex_Init_Tankcfg, 0);
    m_wndMain->prepareKeyStroke();
}

void DInitTankcfgpage::on_ExBackBtn_clicked()
{
    m_wndMain->naviInitPage(Ex_Init_Tankcfg, 1);
    m_wndMain->prepareKeyStroke();
}

 
