
#include <qwidget.h>
#include <qscrollview.h>

class QMyScrollView: public QScrollView
{
  Q_OBJECT
  public:
    void keyPressEvent(QKeyEvent*);
    void keyReleaseEvent(QKeyEvent*);
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
    void keyPressEvent(QKeyEvent*);
    void keyReleaseEvent(QKeyEvent*);
    void closeEvent(QCloseEvent*);
    bool event(QEvent*);
  public slots:
    void dataReceived();
    void soundSend();
};

