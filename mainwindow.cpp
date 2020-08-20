#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QAudioInput>
#include <QScrollBar>
#include <QMessageBox>
#include <QDebug>
#include <QKeyEvent>
#include <QFileDialog>
#include "tools.h"

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	  , ui(new Ui::MainWindow),
	  p2p(new P2PNetwork(this, QHostAddress::AnyIPv4,1002)),
	  p2p_msg(new P2PNetwork(this, QHostAddress::AnyIPv4,1003))
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
		if (p2p->GetProtocol() == P2PNetwork::protocol::TCP)
			ui->lineEdit->setText(p2p->getSocket()->peerAddress().toString());
	});
	connect(p2p_msg, &P2PNetwork::disconnected, this, [&]
	{
		ui->msg_send->setDisabled(true);
		ui->pushButton_2->setDisabled(true);
		ui->pushButton_2->setText("Send File");
		ui->msg_connect->setDisabled(false);
		ismsgconnected = false;
		ui->msg_status->setText("Disconnected");
		ui->msg_2->append("Disconnected");
		ui->msg_conn->setText("Connect");
		ui->msg_send->setDefault(false);
		if (SM > SNotStarted)
		{
			sf->deleteLater();
			SM = SNotStarted;
		}
		if (RM > NotStarted)
		{
			file->remove();
			file->flush();
			file->deleteLater();
			RM = NotStarted;
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
	connect(p2p, &P2PNetwork::error, this, [&](QAbstractSocket::SocketError,QString str)
	{
			ui->msg_2->append("Voice Connection Error: " + str);
			ui->status->setText(str);
	});
	connect(p2p_msg, &P2PNetwork::error, this, [&](QAbstractSocket::SocketError, QString str)
		{
			ui->msg_2->append("Message Connection Error: " + str);
			ui->msg_status->setText(str);
		});
	//ui->textBrowser->setDisabled(true);
	p2p_msg->switchProtocol(P2PNetwork::protocol::TCP);
	ui->pushButton_2->setDisabled(true);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::SwitchedNetwork(P2PNetwork::protocol pro) const
{
	DEBUG << static_cast<int>(pro);
	ui->status->setText("Ready");
}

void MainWindow::playData() const
{
	ui->status->setText("Playing");
	QByteArray data;
	data = p2p->getSocket()->readAll();
	device->write(data.data(), data.size());
}

void MainWindow::on_pushButton_clicked() const
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

void MainWindow::on_InVol_valueChanged(int value) const
{
	DEBUG << value << value / 99.0;
	input->setVolume(value / 99.0);
}

void MainWindow::on_comboBox_currentIndexChanged(const QString& arg1) const
{
	if (arg1 == "UDP")
		p2p->switchProtocol(P2PNetwork::protocol::UDP);
	else if (arg1 == "TCP")
		p2p->switchProtocol(P2PNetwork::protocol::TCP);
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
	const QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
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
	const auto x = QFileDialog::getOpenFileName(this, "Select one  or more files to transfer");
	sf = new QFile(x);
	if (!sf->exists())
	{
		QMessageBox::critical(this, "File doesn't exist", "Unable to send file");
		return;
	}
	ui->pushButton_2->setDisabled(true);
	ui->pushButton_2->setText("Sending");
	sf->open(QIODevice::ReadOnly);
	size = sf->size();
	cur = 0;
	ui->msg_2->append("File Transfer Started Default File Name: " + x);
	DEBUG << "File Transfer Started Default File Name: " + x;
	p2p_msg->getSocket()->write(
		"1" + x.toUtf8().toBase64() + " " + QString::number(size).toUtf8() + "\n");
	SM = SWaitRepsone;
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
			ui->msg_2->append(
				"[" + p2p_msg->getSocket()->peerAddress().toString() + "] File Transfer Started Default File Name: "
				+ QByteArray::fromBase64(n[0]));
			file = new QFile(QFileDialog::getSaveFileName(this, "Find place to drop", QByteArray::fromBase64(n[0])),
			                 this);
			rsize = n[1].toULongLong();
			rcur = 0;
			RM = Transforming;
			file->open(QIODevice::WriteOnly);
			p2p_msg->getSocket()->write("4\n");
			startrecv.start();
			ui->pushButton_2->setText("Recving");
			ui->pushButton_2->setDisabled(true);
			// Start File Trans With Filename 
			break;
		}
	case '2':
		// Write to File Selected
		assert(RM != Transforming);
		data = QByteArray::fromBase64(data.mid(1));
		DEBUG << p2p_msg->getSocket()->peerAddress().toString() << "W" << data.length();
		rcur += data.length();
		ui->msg_status->setText(
			"Recving " + QString::number(100.0 * rcur / rsize , 'f', 2) + "% " +
			QString::number(0.001 * rcur / startrecv.elapsed(), 'f', 2) + " MB/s " +
			QString::number(rcur / 1000000.0, 'f', 2) + "/" + QString::number(rsize / 1000000.0, 'f', 2) + " MB");
		file->write(data);
		p2p_msg->getSocket()->write("4\n");
		break;
	case '3':
		assert(RM != Transforming);
		file->flush();
		file->close();
		file->deleteLater();
		rsize = 0;
		rcur = 0;
		ui->msg_2->append("[" + p2p_msg->getSocket()->peerAddress().toString() + "] File Transfer Ended");
		ui->pushButton_2->setText("Send File");
		ui->pushButton_2->setDisabled(false);
		RM = NotStarted;
		break;
	case '4':
		assert(SM < SWaitRepsone);
		if (SM == SWaitRepsone)
			startsend.start();
		if (!sf->atEnd())
		{
			SM = STransforming;
			const auto d = sf->read(1024 * 1024 * 2);
			DEBUG << "R" << d.length();
			cur += d.length();
			ui->msg_status->setText(
				"Sending " + QString::number(cur * 1.0 / size * 100, 'f', 2) + "% " + (startsend.elapsed() > 0
					                                                                       ? QString::number(
						                                                                       0.001 * cur
							                                                                       / startsend.
							                                                                       elapsed(), 'f',
						                                                                       2)
					                                                                       : "NaN Speed") + " MB/s "
				+ QString::number(cur / 1000000.0, 'f', 2) + "/" + QString::number(size / 1000000.0, 'f', 2) +
				" MB");
			p2p_msg->getSocket()->write("2" + d.toBase64() + "\n");
		}
		else
		{
			SM = SNotStarted;
			ui->msg_2->append("File Transfer Ended");
			p2p_msg->getSocket()->write("3\n");
			sf->deleteLater();
			cur = 0;
			size = 0;
			ui->pushButton_2->setText("Send File");
			ui->pushButton_2->setDisabled(false);
		}
	default:
		assert(false);
	}
}
