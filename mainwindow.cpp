#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QAudioInput>
#include <QScrollBar>
#include <QDebug>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),
      p2p(new P2PNetwork(this,1002))
{
    ui->setupUi(this);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);
    ui->comboBox->addItem("UDP");
    ui->comboBox->addItem("TCP");
    ui->label_2->setDisabled(true);
    switchAudioSample(44100);
    connect(p2p, &P2PNetwork::disconnected, this, [&]
    {
            switchAudioSample(ui->hz->currentText().toInt());
            ui->comboBox->setDisabled(false);
            ui->lineEdit->setDisabled(false);
            ui->hz->setDisabled(false);
            ui->pushButton->setText("Connect");
            isconnected = false;
    });

    connect(p2p, &P2PNetwork::connected, this, [&] {
        ui->comboBox->setDisabled(true);
        ui->lineEdit->setDisabled(true);
        ui->hz->setDisabled(true);
        connect(p2p->getSocket(), &QAbstractSocket::readyRead, this, &MainWindow::playData);
        input->start(p2p->getSocket());
        ui->pushButton->setText("Disconnect");
        isconnected = true;

        });

    connect(p2p, &P2PNetwork::protocolSwitched, this, &MainWindow::SwitchedNetwork);
    //ui->textBrowser->setDisabled(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::SwitchedNetwork(P2PNetwork::protocol pro)
{
	if (pro == P2PNetwork::None)
	{
        if (ui->comboBox->currentText() == "UDP")
            switchNetwork(P2PNetwork::UDP);
        else if (ui->comboBox->currentText() == "TCP")
            switchNetwork(P2PNetwork::TCP);
        else
            assert(false);
	}
}
void MainWindow::playData()
{
    QByteArray data;
    data = p2p->getSocket()->readAll();
    device->write(data.data(), data.size());
}

void MainWindow::on_pushButton_clicked()
{
    ui->comboBox->setDisabled(true);
    ui->lineEdit->setDisabled(true);
    ui->hz->setDisabled(true);
    if (!isconnected) {
        ui->pushButton->setText("Connecting");
        p2p->connectToHost(ui->lineEdit->text(), 1002);
    }
    else
    {
        p2p->disconnectFromHost();
    }   

}

void MainWindow::on_InVol_valueChanged(int value)
{
    input->setVolume(value/99.0);
}


void MainWindow::on_comboBox_currentIndexChanged(const QString &arg1)
{
    if (arg1 == "UDP"){
        p2p->switchProtocol(P2PNetwork::UDP);
    } else if (arg1 == "TCP")
    {
        p2p->switchProtocol(P2PNetwork::TCP);
    } else
        assert(false);

}

void MainWindow::on_hz_currentTextChanged(const QString &arg1)
{
    switchAudioSample(arg1.toInt());
}

void MainWindow::switchAudioSample(int target)
{
    format.setSampleRate(target);
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format))
        format = info.nearestFormat(format);
    ui->label_2->setText(QString::number(format.sampleRate()/1000.0)  + "kHz " +
                         QString::number(format.channelCount())       + " Channel " +
                         format.codec()                               + " codec");
    delete input;
    delete output;
    delete device;
    input = new QAudioInput(format);
    output = new QAudioOutput(format);
    device = output->start();
    ui->status->setText("Ready");
}

void MainWindow::switchNetwork(P2PNetwork::protocol p)
{
    isudpplaying = false;
    p2p->switchProtocol(p);
    ui->status->setText("Ready");

}
