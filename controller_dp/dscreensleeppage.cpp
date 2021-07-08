#include "dscreensleeppage.h"
#include "exconfig.h"
#include "mainwindow.h"
#include <QLabel>
#include <QPalette>
#include <QTimer>

#include "dsleepwidget.h"


DScreenSleepPage::DScreenSleepPage(QObject *parent, CBaseWidget *widget, MainWindow  *wndMain)
    :CPage(parent, widget, wndMain)
{
    buildTranslation();
    initUi();    
}

void DScreenSleepPage::buildTranslation()
{
    m_titleMsg = m_wndMain->machineName();
}

void DScreenSleepPage::update()
{
    buildTranslation();

    if(m_pdwidget)
    {
        m_pdwidget->setMsg(m_titleMsg);
    }
}

void DScreenSleepPage::initUi()
{
    m_pdwidget = new DSleepWidget(m_titleMsg, 800, 600, m_widget);
    m_pdwidget->setGeometry(0, 0, 800, 600);
}


void DScreenSleepPage::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    emit pageHide();
}

void DScreenSleepPage::timerEvent(QTimerEvent *event)
{
    if(event->timerId() == m_timeID)
    {
        int x = randomPos(800);
        int y = randomPos(600) + 30;
        m_pdwidget->setPos(x, y);
        m_pdwidget->update();
    }
}

void DScreenSleepPage::on_SleepPageShow(bool isShow)
{
    if(isShow)
    {
        m_timeID = startTimer(3000);
    }
    else
    {
        killTimer(m_timeID);
    }
}

int DScreenSleepPage::randomPos(int max)
{
    qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
    int temp = qrand()%max;
    if(temp < 5)
    {
        temp += 5;
    }
    return temp;
}




