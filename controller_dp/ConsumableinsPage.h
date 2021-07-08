#ifndef CONSUMABLEINSPAGE_H
#define CONSUMABLEINSPAGE_H

#include "subpage.h"
#include "Display.h"

#define MAX_ConsumableNum (DISP_CM_NAME_NUM  + DISP_MACHINERY_NAME_NUM)

class MainWindow;
class QListWidgetItem;
class titleBar;
class QListWidget;
class CosumableInsListWidgtItem;

class ConsumableInsPage : public CSubPage
{
    Q_OBJECT ;

public:
    ConsumableInsPage(QObject *parent = 0,CBaseWidget *widget = 0 , MainWindow *wndMain = 0, short instanceType = 0);

    virtual void creatTitle();

    virtual void switchLanguage();

    virtual void buildTranslation();

    virtual void initUi();
    
    virtual void update();

    virtual void fade();

    void updateRfidInfo(int iRfId);

private:
    void createList();

    void setBackColor();

    void buildTitles();

    void toCurrentItem(int index);

    void initTypeMap();

    //initNormalItem和initScanItem 只能单独调用一个
    void initNormalItem();
    void initManualItem();
	void installFeedback(bool result);

private:
    QListWidget *listWidget;

    QListWidgetItem *listWidgetIem[MAX_ConsumableNum];

    CosumableInsListWidgtItem *m_aInsListItem[MAX_ConsumableNum];

    QString m_strUserName;

    int     m_iRealItemNum;

    int     m_iViewItemNum;

    int     m_iCurrentItem;

    DISP_ACCESSORY_STRU aIds[MAX_ConsumableNum];

    QMap<short int, short int> m_typeMap;

	bool m_bRfidWork;

public slots:
    void on_btn_clicked(int index);

    void ItemClicked(QListWidgetItem *item);
};

#endif // CONSUMABLEINSPAGE_H
