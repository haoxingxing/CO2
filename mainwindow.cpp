#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QAudioInput>
#include <QScrollBar>
#include <QMessageBox>
#include <QDebug>
#include <QKeyEvent>
#include <QFileDialog>
#include <QTime>
#include "tools.h"
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow),
      ch_(QCryptographicHash::Md5),
      p2p(new P2PNetwork(this, QHostAddress::AnyIPv4,1002)),
      p2p_msg(new P2PNetwork(this, QHostAddress::AnyIPv4,1003))
{
    ui->setupUi(this);

    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/wav");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);
    ui->comboBox->addItem("UDP");
    ui->comboBox->addItem("TCP");
    ui->label_2->setDisabled(true);
    ui->isaecon->setChecked(true);
    log("Init Audio Format");
    connect(p2p, &P2PNetwork::disconnected, this, [&]
    {
        ui->comboBox->setDisabled(false);
        ui->lineEdit->setDisabled(false);
        ui->isaecon->setDisabled(false);
        ui->hz->setDisabled(false);
        ui->pushButton->setText("Connect");
        isconnected = false;
        ui->status->setText("Disconnected");
        input->stop();
        log("Voice Disconnected");
        DEBUG << device_input->disconnect();
    });

    connect(p2p, &P2PNetwork::connected, this, [&]
    {
        ui->comboBox->setDisabled(true);
        ui->lineEdit->setDisabled(true);
        ui->isaecon->setDisabled(true);
        ui->hz->setDisabled(true);
        connect(p2p->getSocket(), &QAbstractSocket::readyRead, this, &MainWindow::playData);
        ui->pushButton->setText("Disconnect");
        ui->status->setText("Connected");
        isconnected = true;
        device_input = input->start();
        log("Voice Cconnected");
        connect(device_input, &QIODevice::readyRead, this, &MainWindow::send_voice);
        if (!ismsgconnected)
            ui->msg_connect->setText(p2p->getSocket()->peerAddress().toString());
        if (p2p->GetProtocol() == P2PNetwork::protocol::TCP)
            ui->lineEdit->setText(p2p->getSocket()->peerAddress().toString());
    });
    connect(p2p_msg, &P2PNetwork::disconnected, this, [&]
    {
        ui->ismd5->setDisabled(false);
        ui->msg_send->setDisabled(true);
        ui->pushButton_2->setDisabled(true);
        ui->pushButton_2->setText("Send File");
        ui->msg_connect->setDisabled(false);
        ismsgconnected = false;
        ui->msg_status->setText("Disconnected");
        ui->msg_2->append("Disconnected");
        ui->msg_conn->setText("Connect");
        log("Message Disconnected");
        ui->msg_send->setDefault(false);
        if (SM > SendMode::SNotStarted)
        {
            sf->deleteLater();
            SM = SendMode::SNotStarted;
        }
        if (RM > RecvMode::NotStarted)
        {
            file->remove();
            file->flush();
            file->deleteLater();
            RM = RecvMode::NotStarted;
        }
    });
    connect(p2p_msg, &P2PNetwork::connected, this, [&]
    {
        ui->msg_send->setDisabled(false);
        ui->pushButton_2->setDisabled(false);
        ui->msg_connect->setDisabled(true);
        connect(p2p_msg->getSocket(), &QAbstractSocket::readyRead, this, &MainWindow::readMsg);
        ui->msg_status->setText("Connected");
        ui->msg_2->append("Connected " + p2p_msg->getSocket()->peerAddress().toString());
        ui->msg_conn->setText("Disconnect");
        ismsgconnected = true;
        log("Message Connected");
        ui->msg_send->setDefault(true);
        if (!isconnected)
            ui->lineEdit->setText(p2p_msg->getSocket()->peerAddress().toString());
        if (p2p_msg->GetProtocol() == P2PNetwork::protocol::TCP)
            ui->msg_connect->setText(p2p_msg->getSocket()->peerAddress().toString());
    });
    connect(ui->msg, &QLineEdit::returnPressed, this, [&]
    {
        if (ismsgconnected)
            on_msg_send_clicked();
    });
    connect(ui->lineEdit, &QLineEdit::returnPressed, this, [&]
        {
            if (!isconnected)
                on_pushButton_clicked();
        });
    connect(ui->msg_connect, &QLineEdit::returnPressed, this, [&]
        {
            if (!ismsgconnected)
                on_msg_conn_clicked();
        });
    connect(p2p, &P2PNetwork::error, this, [&](QAbstractSocket::SocketError, const QString& str)
    {
            log("Voice Connection Error: " + str);
            ui->status->setText(str);
    });
    connect(p2p_msg, &P2PNetwork::error, this, [&](QAbstractSocket::SocketError,const QString& str)
        {
            log("Message Connection Error: " + str);
            ui->msg_status->setText(str);
        });
    connect(ui->commandLinkButton, &QAbstractButton::clicked, this, [&]
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("About");
        msgBox.setText("About the Program CO2");
        msgBox.setInformativeText(
            "The open-source application is designed for LAN voice and message chat\n"
            "The Voice AEC algorithm is provided by SpeexDSP\n" 
            "The GUI Framework is provided by Qt " QT_VERSION_STR "\n"
            "                               Powered by haoxingxing 2020\n"
            "\n"
            "Compiled " __TIMESTAMP__ "\n"
            "Thanks for using");
        auto* a = new QPushButton("AboutQt", &msgBox);
        connect(a, &QPushButton::clicked, &msgBox, &QApplication::aboutQt);
        msgBox.addButton(a, QMessageBox::NoRole);
        msgBox.addButton("OK", QMessageBox::AcceptRole);
        int ret = msgBox.exec();

    });
	p2p_msg->switchProtocol(P2PNetwork::protocol::TCP);
    ui->pushButton_2->setDisabled(true);
    log("Main Menu Init");

}

MainWindow::~MainWindow()
{
    if (m_echo_state != nullptr) {
        speex_echo_state_destroy(m_echo_state);
        speex_preprocess_state_destroy(m_preprocess_state);
        m_echo_state = nullptr;
        m_preprocess_state = nullptr;
        delete m_AECBufferOut;
    }
    delete ui;
}

void MainWindow::SwitchedNetwork(P2PNetwork::protocol pro) const
{
    ui->status->setText("Ready");
}


void MainWindow::on_pushButton_clicked() const
{
    ui->comboBox->setDisabled(true);
    ui->lineEdit->setDisabled(true);
    ui->hz->setDisabled(true);
    if (!isconnected)
    {
        log("Voice Connecting");;
        ui->pushButton->setText("Connecting");
        p2p->connectToHost(ui->lineEdit->text(), 1002);
    }
    else
    {
        log("Voice Disconnecting");;
        p2p->disconnectFromHost();
    }
}

void MainWindow::on_InVol_valueChanged(int value) const
{
    DEBUG << value << value / 99.0;
    input->setVolume(value / 99.0);
}

void MainWindow::on_comboBox_currentIndexChanged(const QString& arg1) const
{
    log("Voice Switch Network Protocol to " + arg1);;
    if (arg1 == "UDP")
        p2p->switchProtocol(P2PNetwork::protocol::UDP);
    else if (arg1 == "TCP")
        p2p->switchProtocol(P2PNetwork::protocol::TCP);
    else
        assert(false);
}

void MainWindow::on_hz_currentTextChanged(const QString& arg1)
{
    log("Switch Voice Sample Rate to " + arg1);;
    switchAudioSample(arg1.toInt());
}


void MainWindow::switchNetwork(P2PNetwork::protocol p)
{
    isudpplaying = false;
    p2p->switchProtocol(p);
}

void MainWindow::on_msg_send_clicked() const
{
    p2p_msg->getSocket()->write("0" + ui->msg->text().toUtf8().toBase64() + "\n");
    ui->msg_2->append("You: " + ui->msg->text());
    ui->msg->clear();
}

void MainWindow::on_msg_conn_clicked() const
{
    ui->msg_connect->setDisabled(true);
    if (!ismsgconnected)
    {
        log("Message Connecting");;
        ui->msg_conn->setText("Connecting");
        p2p_msg->connectToHost(ui->msg_connect->text(), 1003);
    }
    else
    {
        log("Message Disconnecting");;
        p2p_msg->disconnectFromHost();
    }
}

void MainWindow::on_pushButton_2_clicked()
{
    const auto x = QFileDialog::getOpenFileName(this, "Select one  or more files to transfer");
    sf = new QFile(x);
    if (!sf->exists())
    {
        log("File doesn't exist");;
        QMessageBox::critical(this, "File doesn't exist", "Unable to send file");
        return;
    }
    ch_.reset();
    ui->pushButton_2->setDisabled(true);
    ui->pushButton_2->setText("Sending");
    ui->ismd5->setDisabled(true);
    sf->open(QIODevice::ReadOnly);
    size = sf->size();
    cur = 0;
    log("File Transfer Started Default File Name: " + x);
    DEBUG << "File Transfer Started Default File Name: " + x;
    p2p_msg->getSocket()->write(
        "1" + x.toUtf8().toBase64() + " " + QString::number(size).toUtf8() +" " + (ui->ismd5->isChecked()? "1" : "0") + "\n");
    SM = SendMode::SWaitRepsone;
}

void MainWindow::readMsg()
{
    readbuf.append(p2p_msg->getSocket()->readLine());

    if ((readbuf.size() < 1)||(readbuf.back() != '\n'))
        return;
    QByteArray data = readbuf.chopped(1);
    readbuf.clear();
    switch (data.front())
    {
    case '0':
        ui->msg_2->append("[" + p2p_msg->getSocket()->peerAddress().toString() + "]: " + QByteArray::fromBase64(data.mid(1)));
        break;
    case '1':
        {
            auto n = data.mid(1).split(' ');
            log(
                "[" + p2p_msg->getSocket()->peerAddress().toString() + "] File Transfer Started Default File Name: "
                + QByteArray::fromBase64(n[0]));
            file = new QFile(QFileDialog::getSaveFileName(this, "Find place to drop", QByteArray::fromBase64(n[0])),
                             this);
            rsize = n[1].toULongLong();
            ui->ismd5->setChecked(n[2] == "1");
            rcur = 0;
            RM = RecvMode::Transforming;
            if (ui->ismd5->isChecked())
                ch_.reset();
            file->open(QIODevice::WriteOnly);
            p2p_msg->getSocket()->write("4\n");
            startrecv.start();
            ui->pushButton_2->setText("Recving");
            ui->pushButton_2->setDisabled(true);
            ui->ismd5->setDisabled(true);
            // Start File Trans With Filename
            break;
        }
    case '2':
        // Write to File Selected
        assert(RM != Transforming);
        data = QByteArray::fromBase64(data.mid(1));
        DEBUG << p2p_msg->getSocket()->peerAddress().toString() << "W" << data.length();
        rcur += data.length();
        if (ui->ismd5->isChecked())
            ch_.addData(data);
        ui->msg_status->setText(
            "Recving " + QString::number(100.0 * rcur / rsize , 'f', 2) + "% " +
            QString::number(0.001 * rcur / startrecv.elapsed(), 'f', 2) + " MB/s " +
            QString::number(rcur / 1000000.0, 'f', 3) + "/" + QString::number(rsize / 1000000.0, 'f', 3) + " MB");
        file->write(data);
        p2p_msg->getSocket()->write("4\n");
        break;
    case '3':
        assert(RM != Transforming);
        data = data.mid(1);
        file->flush();
        file->close();
        file->deleteLater();
        log(
            "Recvd " + QString::number(100.0 * rcur / rsize, 'f', 2) + "% " +
            QString::number(0.001 * rcur / startrecv.elapsed(), 'f', 2) + " MB/s " +
            QString::number(rcur / 1000000.0, 'f', 3) + "/" + QString::number(rsize / 1000000.0, 'f', 3) + " MB");
        rsize = 0;
        rcur = 0;
        if (ui->ismd5->isChecked()) {
            log("[" + p2p_msg->getSocket()->peerAddress().toString() + "] Remote MD5 " + data);
            log("[" + p2p_msg->getSocket()->peerAddress().toString() + "] Local  MD5 " + ch_.result().toHex());
            p2p_msg->getSocket()->write("5" + ch_.result().toHex() + "\n");
        }
        ui->ismd5->setDisabled(false);
        ui->msg_status->setText("Recv Finished");
        log ("[" + p2p_msg->getSocket()->peerAddress().toString() + "] File Transfer Ended");
        ui->pushButton_2->setText("Send File");
        ui->pushButton_2->setDisabled(false);
        RM = RecvMode::NotStarted;
        break;
    case '4':
        assert(SM < SWaitRepsone);
        if (SM == SendMode::SWaitRepsone)
            startsend.start();
        if (!sf->atEnd())
        {
            SM = SendMode::STransforming;
            const auto d = sf->read(1024 * 1024 * 2);
            DEBUG << "R" << d.length();
            if (ui->ismd5->isChecked())
                ch_.addData(d);
            cur += d.length();
            ui->msg_status->setText(
                "Sending " + QString::number(cur * 1.0 / size * 100, 'f', 2) + "% " + (startsend.elapsed() > 0
                                                                                           ? QString::number(
                                                                                               0.001 * cur
                                                                                                   / startsend.
                                                                                                   elapsed(), 'f',
                                                                                               2)
                                                                                           : "NaN Speed") + " MB/s "
                + QString::number(cur / 1000000.0, 'f', 3) + "/" + QString::number(size / 1000000.0, 'f', 3) +
                " MB");
            p2p_msg->getSocket()->write("2" + d.toBase64() + "\n");
        }
        else
        {
            SM = SendMode::SWaitMD5;
            log(
                "Sent " + QString::number(cur * 1.0 / size * 100, 'f', 2) + "% " + (startsend.elapsed() > 0
                    ? QString::number(
                        0.001 * cur
                        / startsend.
                        elapsed(), 'f',
                        2)
                    : "NaN Speed") + " MB/s "
                + QString::number(cur / 1000000.0, 'f', 3) + "/" + QString::number(size / 1000000.0, 'f', 3) +
                " MB");
            if (ui->ismd5->isChecked()) {
                log("[" + p2p_msg->getSocket()->peerAddress().toString() + "] Local  MD5 " + ch_.result().toHex());
                ui->msg_status->setText("Waiting for MD5 response");
            }
            else
            {
                ui->ismd5->setDisabled(false);
                log ("File Transfer Ended");
                ui->msg_status->setText("Send Finished");
                ui->pushButton_2->setText("Send File");
                ui->pushButton_2->setDisabled(false);
                SM = SendMode::SNotStarted;
            }
            p2p_msg->getSocket()->write("3" + (ui->ismd5->isChecked() ? ch_.result().toHex() : "" )+ "\n");
            sf->deleteLater();
            cur = 0;
            size = 0;
        }
        break;
    case '5':
        assert(SM != SWaitMD5);
        SM = SendMode::SNotStarted;
        data = data.mid(1);
        ui->msg_status->setText("Send Finished");
        log("[" + p2p_msg->getSocket()->peerAddress().toString() + "] Remote MD5 " + data);
        log("File Transfer Ended");
        ui->ismd5->setDisabled(false);
        ui->pushButton_2->setText("Send File");
        ui->pushButton_2->setDisabled(false);
        break;
    default:
        assert(false);
    }
}


void MainWindow::send_voice()
{
    QByteArray last_mic_capd = device_input->readAll();
    if (ui->isaecon->isChecked()) {
        // get most recent sample, remove echo and repush it on the list to be played    later
        speex_echo_capture(m_echo_state, reinterpret_cast<spx_int16_t*>(last_mic_capd.data()), m_AECBufferOut);
        speex_preprocess_run(m_preprocess_state, m_AECBufferOut);
        p2p->getSocket()->write(QByteArray(reinterpret_cast<const char*>(m_AECBufferOut), m_AECBufferOut_size * static_cast<int>(sizeof(spx_int16_t))));
    }
    else
    {
        p2p->getSocket()->write(last_mic_capd);
    }
    p2p->getSocket()->flush();

}

void MainWindow::playData() const
{
    ui->status->setText("Playing");
    auto x = p2p->getSocket()->readAll();
    if (ui->isaecon->isChecked()) {
        speex_echo_playback(m_echo_state, reinterpret_cast<const spx_int16_t*>(x.constData()));
    }
    device_output->write(x.data(), x.size());
}

void MainWindow::on_isaecon_stateChanged(int arg1) const
{
    ui->hz->clear();
    if (arg1)
    {
        ui->hz->addItems({ "8000","16000","32000" });
        log("Speex AEC ON");
    }
    else
    {
        ui->hz->addItems({ "8000","16000","32000" ,"48000","96000","128000","240000","320000"});
        log("Speex AEC OFF");
    }
}

inline void MainWindow::log(const QString& str) const
{
    ui->msg_2->append(QTime::currentTime().toString() + " " + str);
}

void MainWindow::switchAudioSample(int target)
{
    if (target == 0)
        return;
    if (m_echo_state != nullptr) {
        log("Destory SpeexDSP");
        DEBUG << "Destory Speex" << m_echo_state << m_preprocess_state;
        speex_echo_state_destroy(m_echo_state);
        speex_preprocess_state_destroy(m_preprocess_state);
        delete[] m_AECBufferOut;
        m_echo_state = nullptr;
        m_preprocess_state = nullptr;
    }
    if (input != nullptr) {
        log("Destory Qt Audio");
        // ReSharper disable once CppExpressionWithoutSideEffects
        input->disconnect();
        // ReSharper disable once CppExpressionWithoutSideEffects
        output->disconnect();
        // ReSharper disable once CppExpressionWithoutSideEffects
        device_output->disconnect();
        if (device_input != nullptr)
            DEBUG << device_input->disconnect();
        delete input;
        delete output;
        delete device_output;
        delete device_input;
        device_input = nullptr;
    }
    format.setSampleRate(target);
    log("Reset format");
    const QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format))
        format = info.nearestFormat(format);
    ui->label_2->setText(QString::number(format.sampleRate() / 1000.0) + "kHz " + QString(ui->isaecon->isChecked() ? "AEC ON" : "AEC OFF"));
    log("Init Qt Audio");
    input = new QAudioInput(format);
    output = new QAudioOutput(format);
    input->setVolume(ui->InVol->value() / 99.0);
    device_output = output->start();
    if (ui->isaecon->isChecked()) {
        log("Init SpeexDSP");
        m_echo_state = speex_echo_state_init(target / 100, target / 10);
        DEBUG << "Initialize Speex AEC" << m_echo_state;
        m_AECBufferOut_size = target / 100;
        DEBUG << "Speex AEC Set SAMPLING_RATE" << speex_echo_ctl(m_echo_state, SPEEX_ECHO_SET_SAMPLING_RATE, &target);
        m_preprocess_state = speex_preprocess_state_init(m_AECBufferOut_size, target);
        DEBUG << "Initialize Speex Preprocess" << m_preprocess_state;
        m_AECBufferOut = new spx_int16_t[m_AECBufferOut_size];
    }

    ui->status->setText("Ready");
}
