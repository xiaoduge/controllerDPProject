#ifndef NETWORKPAGE_H
#define NETWORKPAGE_H

#include "subpage.h"
#include "Display.h"

class MainWindow;
class QProcess;
class DLineEdit;
class QCheckBox;
class CBitmapButton;
class QListWidget;
class QListWidgetItem;

enum NETWORKPAGE_BTN_ENUM
{
   NETWORKPAGE_BTN_SAVE = 0,
   NETWORKPAGE_BTN_NUM
};

class NetworkPage : public CSubPage
{
    Q_OBJECT
public:
    NetworkPage(QObject *parent = 0,CBaseWidget *widget = 0 , MainWindow *wndMain = 0);

    virtual void creatTitle();

    virtual void switchLanguage();

    virtual void buildTranslation();

    virtual void initUi();

    void leaveSubPage();
    void update();

private:

    void buildTitles();
    
    void save();

    void setBackColor();

    QLabel        *m_laName[DISPLAY_NETWORK_NUM];
    QCheckBox     *m_chkSwitchs[DISPLAY_NETWORK_NUM];
    QWidget       *m_pBackWidget[DISPLAY_NETWORK_NUM];
    QString        m_astrNetName[DISPLAY_NETWORK_NUM];
    QString       m_strQss4Chk;
    int           m_iNetworkMask;
    
    CBitmapButton     *m_pBtnSave;   

    //add for wifi config
    QWidget *m_pWifiConfigWidget;
    QPushButton *m_pRefreshWifiBtn;
	QPushButton *m_pGetNetInfoBtn;
    QCheckBox *m_pAddCheckBox;

    QPushButton *m_pAddSSIDBtn;
    QLabel *m_pSSIDLab;
    DLineEdit *m_pSSIDEdit;

    QListWidget *m_pWifiMsgListWidget;
    QWidget *m_pWifiSSIDAddWidget;

    QProcess *m_pProcess;
	QProcess *m_pGetInfoProcess;
	bool m_bRefreshed;

public slots:
    void on_btn_clicked(int);
    void on_checkBox_changeState(int state);

    void on_addSSIDBtn_clicked();
    void on_wifiRefreshBtn_clicked();
    void on_refreshWifiMsg();
    void on_wifiListWidget_itemClicked(QListWidgetItem *item);
    void on_addCheckBox_stateChanged(int state);

	void on_getNetInfoBtn_clicked();
	void on_getNetInfo();
};

#endif // NETWORKPAGE_H
