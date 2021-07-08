#ifndef DIAPLAYPAGE_H
#define DIAPLAYPAGE_H

#include <QObject>
#include <QPixmap>
#include <QPalette>
#include <QLabel>
#include <QInputDialog>

#include "basewidget.h"
#include "subpage.h"
#include "cbitmapbutton.h"
#include <QLineEdit>
#include <QLabel>
#include <QTextEdit>
#include <QListWidgetItem>
#include <QHBoxLayout>
#include <QPushButton>

#include <QSlider>

#include <QComboBox>
#include "cbitmapbutton.h"

class MainWindow;

enum DISPLAY_BTN_NAME
{
    DISPLAY_BTN_ENERGY_SAVE = 0, 
    DISPLAY_BTN_SAVE,
    DISPLAY_BTN_NUMBER,
};


class DisplayPage : public CSubPage
{
    Q_OBJECT
public:
    DisplayPage(QObject *parent = 0,CBaseWidget *widget = 0 , MainWindow *wndMain = 0);
    virtual void creatTitle();

    virtual void switchLanguage();

    virtual void buildTranslation();

    virtual void initUi();

private:

    void Set_Back_Color();

    void buildTitles();
    
    void save();

    QLabel        *laName[2];

    CBitmapButton *m_pBtnEnergySave;

    QWidget       *m_pBackWidget[2];

    QSlider       *pSlider[1];

    QString       m_DispNames[2];
    
    CBitmapButton *m_pBtnSave;

    int            m_iEnergySave;
    int            m_iBrightness;

public slots:

    void on_btn_clicked(int tmp);

    void setValue(int);
};


#endif // DIAPLAYPAGE_H
