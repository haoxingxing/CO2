#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QAudioOutput>
#include <QAudioInput>
#include <QTcpServer>
#include <QTcpSocket>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
public slots:
    void playData();
private slots:
    void on_pushButton_clicked();

    void on_InVol_valueChanged(int value);

    void on_comboBox_currentIndexChanged(const QString &arg1);

    void on_hz_currentTextChanged(const QString &arg1);

private:
    Ui::MainWindow *ui;

    QUdpSocket *udp;
    QTcpSocket *tcpC;
    QTcpServer *tcpS;

    enum _mode{
        UDP,TCP,None
    }mode = None;

    QIODevice *device;

    QAudioOutput* output;
    QAudioInput* input;

    bool isconnected = false;
    bool isudpplaying = false;
};
#endif // MAINWINDOW_H
