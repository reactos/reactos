
#include <qwidget.h>
#include <qscrollview.h>
#include <qdialog.h>
#include <qlistbox.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qpopupmenu.h>

class QMyConnectionItem
{
  public:
    QString ServerName;
    QString UserName;
    QString ServerIP;
    int Width;
    int Height;
    int FullScreen;
};

class QMyDialog: public QDialog
{
  Q_OBJECT
  public:
    QMyDialog(QWidget*);
    ~QMyDialog();
  public:
    QListBox* ListBox;
    QPushButton* OKButton;
    QPushButton* CancelButton;
    QLabel* Label1;
    QLineEdit* ServerNameEdit;
    QLabel* Label2;
    QLineEdit* UserNameEdit;
    QLabel* Label3;
    QLineEdit* IPEdit;
    QLineEdit* WidthEdit;
    QLineEdit* HeightEdit;
    QComboBox* WidthHeightBox;
    QPushButton* AddButton;
    QPushButton* EditButton;
    QPushButton* SaveButton;
    QPushButton* RemoveButton;
    QCheckBox* FullScreenCheckBox;
  public slots:
    void ComboChanged(int);
    void OKClicked();
    void CancelClicked();
    void AddClicked();
    void EditClicked();
    void SaveClicked();
    void RemoveClicked();
    void ListBoxChanged();
    void ListBoxSelected(int);
  public:
    QString ServerName;
    QString UserName;
    QString ServerIP;
    int Width;
    int Height;
    int FullScreen;
    QMyConnectionItem* ConnectionList[10];
};

class QMyScrollView: public QScrollView
{
  Q_OBJECT
  public:
    QMyScrollView();
    ~QMyScrollView();
    void keyPressEvent(QKeyEvent*);
    void keyReleaseEvent(QKeyEvent*);
    void showEvent(QShowEvent*);
    void show();
    void polish();
    void timerEvent(QTimerEvent*);
  public:
    int timer_id;
    int sound_timer_id;
};

class QMyMainWindow: public QWidget
{
  Q_OBJECT
  public:
    QMyMainWindow();
    ~QMyMainWindow();
    void paintEvent(QPaintEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void wheelEvent(QWheelEvent*);
    void closeEvent(QCloseEvent*);
    void timerEvent(QTimerEvent*);
  public slots:
    void dataReceived();
    void soundSend();
    void MemuClicked(int);
  public:
    QPopupMenu* PopupMenu;
    int timer_id;
    int mx;
    int my;
};

