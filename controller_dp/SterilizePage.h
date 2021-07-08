#ifndef STERILIZEPAGE_H
#define STERILIZEPAGE_H


#include "subpage.h"
#include "Display.h"

class MainWindow;
class CBitmapButton;
class QProgressBar;

class Sterilize: public QObject
{
public:
    QWidget       *BackWidget;
    QLabel        *laBackNo;
    QLabel        *laName;
    
    QLabel        *laPromptFrame;
    QLabel        *laPrompt;

    QLabel        *laMtTime;
    QLabel        *laNextMtTime;

    CBitmapButton *btnClean;

};

class SterilizePage : public CSubPage
{
    Q_OBJECT

public:
    SterilizePage(QObject *parent = 0,CBaseWidget *widget = 0 , MainWindow *wndMain = 0);

    virtual void creatTitle();

    virtual void switchLanguage();

    virtual void buildTranslation();

    virtual void initUi();

    virtual void update();

    virtual void fade();
	virtual void leaveSubPage();

    void updateRunInfo(int index,bool bRun);

    void updateRfidInfo(int iRfId);
    
public slots:
    void on_btn_clicked(int);

protected:
    void timerEvent(QTimerEvent *event );
    
private:
    void setBackColor();
    
    void createControl();

    void buildTitles();

    void startCleanProgress(int iType);

private:
    Sterilize    m_aSterilize[APP_CLEAN_NUM];

    QProgressBar *m_pCleanProBar;
    int m_cleanTimerId;

    int m_iWashDuration[APP_CLEAN_NUM - 1];
	int m_curProgreeValue;
};

#endif // STERILIZEPAGE_H
