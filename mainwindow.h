#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAudioOutput>
#include <QAudioInput>
#include <QEvent>
#include <QElapsedTimer>
#include <QCryptographicHash>
#include <QFile>

#include "speex/speex_echo.h"
#include "speex/speex_preprocess.h"
#include "p2pnetwork.h"
QT_BEGIN_NAMESPACE

namespace Ui
{
    class MainWindow;
}

QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
public slots:
    void playData() const;
    void SwitchedNetwork(P2PNetwork::protocol) const;
private slots:
    void on_pushButton_clicked() const;

    void on_InVol_valueChanged(int value) const;

    void on_comboBox_currentIndexChanged(const QString& arg1) const;

    void on_hz_currentTextChanged(const QString& arg1);

    void on_msg_send_clicked() const;

    void on_msg_conn_clicked() const;

    void on_pushButton_2_clicked();

    void readMsg();

    void send_voice();
    void on_isaecon_stateChanged(int arg1) const;

private:
    void switchAudioSample(int target);
    void switchNetwork(P2PNetwork::protocol);
    Ui::MainWindow* ui;
  
    QIODevice* device_output = nullptr;
    QIODevice* device_input = nullptr;
    QCryptographicHash ch_;
    QAudioOutput* output = nullptr;
    QAudioInput* input = nullptr;
    QAudioFormat format;
    bool isconnected = false;
    bool isudpplaying = false;
    bool ismsgconnected = false;
    P2PNetwork* p2p;
    P2PNetwork* p2p_msg;

    enum class RecvMode
    {
        NotStarted,
        Transforming,
        EndFile
    } RM = RecvMode::NotStarted;

    QFile* file = nullptr;
    QElapsedTimer startrecv;

    enum class SendMode 
    {
        SNotStarted,
        SWaitRepsone,
        STransforming,
        SWaitMD5
    } SM = SendMode::SNotStarted;

    QFile* sf = nullptr;
    QElapsedTimer startsend;
    QByteArray readbuf;
    qint64 cur = 0;
    qint64 size = 0;
    qint64 rcur = 0;
    qint64 rsize = 0;
    spx_int16_t* m_AECBufferOut{};
    int m_AECBufferOut_size = 0;
    int data_size = 0; 
    SpeexEchoState* m_echo_state = nullptr;
    SpeexPreprocessState* m_preprocess_state = nullptr;

};
#endif // MAINWINDOW_H
