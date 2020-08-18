#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QAudioInput>
#include <QScrollBar>
#include <QDebug>
#include <QKeyEvent>
#include "tools.h"

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	  , ui(new Ui::MainWindow),
	  p2p(new P2PNetwork(this, 1002)),
	  p2p_msg(new P2PNetwork(this, 1003))
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
		ui->status->setText("Disconnected");
	});

	connect(p2p, &P2PNetwork::connected, this, [&]
	{
		ui->comboBox->setDisabled(true);
		ui->lineEdit->setDisabled(true);
		ui->hz->setDisabled(true);
		connect(p2p->getSocket(), &QAbstractSocket::readyRead, this, &MainWindow::playData);
		input->start(p2p->getSocket());
		ui->pushButton->setText("Disconnect");
		ui->status->setText("Connected");
		isconnected = true;
		if (!ismsgconnected)
			ui->msg_connect->setText(p2p->getSocket()->peerAddress().toString());
		if (p2p->_protocol == P2PNetwork::TCP)
			ui->lineEdit->setText(p2p->getSocket()->peerAddress().toString());
	});
	connect(p2p_msg, &P2PNetwork::disconnected, this, [&]
	{
		ui->msg_send->setDisabled(true);
		ui->msg_connect->setDisabled(false);
		ismsgconnected = false;
		ui->msg_status->setText("Disconnected");
		ui->msg_2->append("Disconnected");
		ui->msg_conn->setText("Connect");
		ui->msg_send->setDefault(false);
	});
	connect(p2p_msg, &P2PNetwork::connected, this, [&]
	{
		ui->msg_send->setDisabled(false);
		ui->msg_connect->setDisabled(true);
		connect(p2p_msg->getSocket(), &QAbstractSocket::readyRead, this, [&]
		{
			ui->msg_2->append(
				"[" + p2p_msg->getSocket()->peerAddress().toString() + "]: " + p2p_msg->getSocket()->readAll());
		});
		ui->msg_status->setText("Connected");
		ui->msg_2->append("Connected " + p2p_msg->getSocket()->peerAddress().toString());
		ui->msg_conn->setText("Disconnect");
		ismsgconnected = true;
		ui->msg_send->setDefault(true);
		if (!isconnected)
			ui->lineEdit->setText(p2p_msg->getSocket()->peerAddress().toString());
		if (p2p_msg->_protocol == P2PNetwork::TCP)
			ui->msg_connect->setText(p2p_msg->getSocket()->peerAddress().toString());
	});
	connect(ui->msg, &QLineEdit::returnPressed, this, [&]
	{
			if (ismsgconnected)
				on_msg_send_clicked();
	});
	connect(p2p, &P2PNetwork::protocolSwitched, this, &MainWindow::SwitchedNetwork);
	//ui->textBrowser->setDisabled(true);
	p2p_msg->switchProtocol(P2PNetwork::TCP);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::SwitchedNetwork(P2PNetwork::protocol pro)
{
	DEBUG << pro;
	ui->status->setText("Ready");
}

void MainWindow::playData()
{
	ui->status->setText("Playing");
	QByteArray data;
	data = p2p->getSocket()->readAll();
	device->write(data.data(), data.size());
}

void MainWindow::on_pushButton_clicked()
{
	ui->comboBox->setDisabled(true);
	ui->lineEdit->setDisabled(true);
	ui->hz->setDisabled(true);
	if (!isconnected)
	{
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
	DEBUG << value << value / 99.0;
	input->setVolume(value / 99.0);
}

void MainWindow::on_comboBox_currentIndexChanged(const QString& arg1)
{
	if (arg1 == "UDP")
	{
		p2p->switchProtocol(P2PNetwork::UDP);
	}
	else if (arg1 == "TCP")
	{
		p2p->switchProtocol(P2PNetwork::TCP);
	}
	else
		assert(false);
}

void MainWindow::on_hz_currentTextChanged(const QString& arg1)
{
	switchAudioSample(arg1.toInt());
}

void MainWindow::switchAudioSample(int target)
{
	format.setSampleRate(target);
	QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
	if (!info.isFormatSupported(format))
		format = info.nearestFormat(format);
	ui->label_2->setText(QString::number(format.sampleRate() / 1000.0) + "kHz " +
		QString::number(format.channelCount()) + " Channel " +
		format.codec() + " codec");
	delete input;
	delete output;
	delete device;
	input = new QAudioInput(format);
	output = new QAudioOutput(format);
	input->setVolume(ui->InVol->value() / 99.0);
	device = output->start();
	ui->status->setText("Ready");
}

void MainWindow::switchNetwork(P2PNetwork::protocol p)
{
	isudpplaying = false;
	p2p->switchProtocol(p);
}

void MainWindow::on_msg_send_clicked()
{
	p2p_msg->getSocket()->write(ui->msg->text().toUtf8());
	ui->msg_2->append("You: " + ui->msg->text());
	ui->msg->clear();
}

void MainWindow::on_msg_conn_clicked()
{
	ui->msg_connect->setDisabled(true);
	if (!ismsgconnected)
	{
		ui->msg_conn->setText("Connecting");
		p2p_msg->connectToHost(ui->msg_connect->text(), 1003);
	}
	else
	{
		p2p_msg->disconnectFromHost();
	}
}

void MainWindow::on_pushButton_2_clicked()
{
}
