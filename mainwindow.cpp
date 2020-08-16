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
    format.setSampleRate(384000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format))
        format = info.nearestFormat(format);
    ui->textBrowser->setText(ui->textBrowser->document()->toHtml() + "<p align=\"center\">" +
                             QString::number(format.sampleRate()/1000) + " kHz " +
                             QString::number(format.channelCount()) + " Channel " +
                             format.codec() + " codec </p>");
    input = new QAudioInput(format);
    output = new QAudioOutput(format);
    device = output->start();

    ui->textBrowser->setDisabled(true);
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
            frames_sks++;
            ui->label->setText("Tx:"+ udp->peerAddress().toString()+ " Rx Frames: " + QString::number(frames_sks));
      //      ui->textBrowser->document()->setPlainText(data.left(ui->textBrowser->widthMM()*ui->textBrowser->heightMM() / 12).toHex(' '));
        }
    }
    else if (mode == TCP)
    {
        QByteArray data;
        data = tcpC->readAll();
        device->write(data.data(), data.size());
        frames_sks++;
        ui->label->setText("Tx:"+ tcpC->peerAddress().toString()+ " Rx Frames: " + QString::number(frames_sks));
       // ui->textBrowser->document()->setPlainText(data.left(ui->textBrowser->widthMM()*ui->textBrowser->heightMM() / 12).toHex(' '));
    }
}

void MainWindow::on_pushButton_clicked()
{
    ui->comboBox->setDisabled(true);
    ui->lineEdit->setReadOnly(true);
    ui->lineEdit->setDisabled(true);
    ui->pushButton->setDisabled(true);
    ui->pushButton->setFlat(true);

    if (mode == UDP){
        udp->connectToHost(ui->lineEdit->text(), 1002);
        ui->label->setText("Connect to " + ui->lineEdit->text() +":1002");
        ui->pushButton->setText("Connected");
    }
    else if (mode == TCP){
        tcpS->close();
        tcpS->disconnect();
        tcpS->deleteLater();
        tcpC=new QTcpSocket;
        connect(tcpC,&QTcpSocket::connected,this,[&]{
            ui->label->setText("Connect to " + ui->lineEdit->text() +":1002");
            ui->pushButton->setText("Connected");
            connect(tcpC, &QTcpSocket::readyRead, this, &MainWindow::playData);
            input->start(tcpC);

        });
        ui->pushButton->setText("Connecting");
        tcpC->connectToHost(ui->lineEdit->text(),1002);
    }
}

void MainWindow::on_InVol_valueChanged(int value)
{
    input->setVolume(value/99.0);
}


void MainWindow::on_comboBox_currentIndexChanged(const QString &arg1)
{
    if (arg1 == "UDP"){
        if (mode != UDP){
            if (mode == TCP){
                tcpS->disconnect();
                tcpS->deleteLater();
                tcpS = nullptr;
                input->stop();
            }
            mode = UDP;
            udp = new QUdpSocket(this);
            udp->bind(QHostAddress::Any, 1002);
            connect(udp, &QUdpSocket::readyRead, this, &MainWindow::playData);
            connect(udp,&QUdpSocket::connected,this,[&]{
                input->start(udp);
            });
        }
    } else
        if (arg1 == "TCP") {
            if (mode != TCP){
                if (mode == UDP){
                    udp->disconnect();
                    input->stop();
                    udp->deleteLater();
                    udp = nullptr;
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
                        ui->comboBox->setDisabled(true);
                        ui->lineEdit->setReadOnly(true);
                        ui->lineEdit->setDisabled(true);
                        ui->pushButton->setDisabled(true);
                        ui->pushButton->setFlat(true);
                        ui->label->setText("Accepted Connection From " + tcpC->peerAddress().toString());
                        input->start(tcpC);
                        ui->pushButton->setText("Passive Connected");
                        ui->lineEdit->setText(tcpC->peerAddress().toString());
                    }
                });
            }
        } else
            assert(false);
}
