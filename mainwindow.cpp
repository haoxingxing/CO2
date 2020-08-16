#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QAudioInput>
#include <QScrollBar>
#include <QDebug>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->comboBox->addItem("UDP");
    ui->comboBox->addItem("TCP");
    QAudioFormat format;
    format.setSampleRate(64000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format))
        format = info.nearestFormat(format);
    ui->label_2->setText(    QString::number(format.sampleRate()/1000.0) + "kHz " +
                             QString::number(format.channelCount()) + " Channel " +
                             format.codec() + " codec");
    input = new QAudioInput(format);
    output = new QAudioOutput(format);
    device = output->start();
    ui->label_2->setDisabled(true);
    //ui->textBrowser->setDisabled(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::playData()
{
    if (mode == UDP){
        while (udp->hasPendingDatagrams())
        {
            QByteArray data;
            data.resize(udp->pendingDatagramSize());
            udp->readDatagram(data.data(), data.size());
            device->write(data.data(), data.size());
            if (!isudpplaying){
                ui->status->setText("UDP Playing");
                isudpplaying= true;
            }
            // ui->label->setText("Tx:"+ udp->peerAddress().toString()+ " Rx Frames: " + QString::number(frames_sks));
            //      ui->textBrowser->document()->setPlainText(data.left(ui->textBrowser->widthMM()*ui->textBrowser->heightMM() / 12).toHex(' '));
        }
    }
    else if (mode == TCP)
    {
        QByteArray data;
        data = tcpC->readAll();
        device->write(data.data(), data.size());
        // ui->label->setText("Tx:"+ tcpC->peerAddress().toString()+ " Rx Frames: " + QString::number(frames_sks));
        // ui->textBrowser->document()->setPlainText(data.left(ui->textBrowser->widthMM()*ui->textBrowser->heightMM() / 12).toHex(' '));
    }
}

void MainWindow::on_pushButton_clicked()
{
    if (!isconnected){
        ui->comboBox->setDisabled(true);
        ui->lineEdit->setReadOnly(true);
        ui->lineEdit->setDisabled(true);
        ui->hz->setDisabled(true);
        isconnected = true;

        if (mode == UDP){
            udp->connectToHost(ui->lineEdit->text(), 1002);
            //  ui->label->setText("Connect to " + ui->lineEdit->text() +":1002");
            ui->pushButton->setText("Disconnect");
        }
        else if (mode == TCP){
            tcpS->close();
            tcpS->disconnect();
            tcpS->deleteLater();
            tcpC=new QTcpSocket;
            connect(tcpC,&QTcpSocket::connected,this,[&]{
                // ui->label->setText("Connect to " + ui->lineEdit->text() +":1002");
                ui->pushButton->setText("Disconnect");
                isconnected = true;
                connect(tcpC, &QTcpSocket::readyRead, this, &MainWindow::playData);
                input->start(tcpC);


            });
            ui->pushButton->setText("Connecting");
            tcpC->connectToHost(ui->lineEdit->text(),1002);
        }
    }
    else
    {
        ui->pushButton->setText("Connect");
        ui->comboBox->setDisabled(false);
        ui->lineEdit->setReadOnly(false);
        ui->lineEdit->setDisabled(false);
        ui->hz->setDisabled(false);
        on_hz_currentTextChanged(ui->hz->currentText());
        on_comboBox_currentIndexChanged(ui->comboBox->currentText());
        isconnected = false;
    }
}

void MainWindow::on_InVol_valueChanged(int value)
{
    input->setVolume(value/99.0);
}


void MainWindow::on_comboBox_currentIndexChanged(const QString &arg1)
{
    isudpplaying = false;
    ui->status->setText("Switching");
    if (arg1 == "UDP"){
        if (mode == TCP){
            tcpS->disconnect();
            tcpS->deleteLater();
            tcpS = nullptr;
            input->stop();
        }
        else if (mode == UDP){
            udp->disconnect();
            input->stop();
            udp->deleteLater();
            udp = nullptr;
        }
        mode = UDP;
        udp = new QUdpSocket(this);
        udp->bind(QHostAddress::Any, 1002);
        connect(udp, &QUdpSocket::readyRead, this, &MainWindow::playData);
        connect(udp,&QUdpSocket::connected,this,[&]{
            input->start(udp);
        });

    } else
        if (arg1 == "TCP") {
            if (mode == UDP){
                udp->disconnect();
                input->stop();
                udp->deleteLater();
                udp = nullptr;
            }
            else
                if (mode == TCP){
                    if (!isconnected){
                        tcpS->disconnect();
                        tcpS->deleteLater();
                        tcpS = nullptr;
                    }
                    else{
                        input->stop();
                        tcpC->disconnectFromHost();
                        tcpC->disconnect();
                        input->stop();
                        output->stop();
                        tcpC->deleteLater();
                        tcpS->deleteLater();
                    }
                }
            mode = TCP;
            tcpS = new QTcpServer;
            tcpS->setMaxPendingConnections(1);
            tcpS->listen(QHostAddress::Any,1002);
            connect(tcpS,&QTcpServer::newConnection,this,[&]{
                if (tcpS->hasPendingConnections())
                {
                    tcpC=tcpS->nextPendingConnection();
                    tcpS->close();
                    connect(tcpC, &QTcpSocket::readyRead, this, &MainWindow::playData);
                    connect(tcpC, &QTcpSocket::disconnected,this,[&]{
                        on_pushButton_clicked();
                    });
                    isconnected = true;
                    ui->comboBox->setDisabled(true);
                    ui->lineEdit->setReadOnly(true);
                    ui->lineEdit->setDisabled(true);

                    ui->hz->setDisabled(true);
                    //ui->label->setText("Accepted Connection From " + tcpC->peerAddress().toString());
                    input->start(tcpC);
                    ui->pushButton->setText("Passive Connected");
                    ui->lineEdit->setText(tcpC->peerAddress().toString());
                }
            });
        } else
            assert(false);
    ui->status->setText("Ready");
}

void MainWindow::on_hz_currentTextChanged(const QString &arg1)
{
    ui->status->setText("Switching");
    QAudioFormat format;
    format.setSampleRate(arg1.toInt());
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format))
        format = info.nearestFormat(format);
    ui->label_2->setText(    QString::number(format.sampleRate()/1000.0) + "kHz " +
                             QString::number(format.channelCount()) + " Channel " +
                             format.codec() + " codec");
    input->deleteLater();
    output->deleteLater();
    input = new QAudioInput(format);
    output = new QAudioOutput(format);
    device = output->start();
    ui->status->setText("Ready");
}
