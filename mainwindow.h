#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAudioOutput>
#include <QAudioInput>
#include <QEvent>
#include <QElapsedTimer>
#include <QFile>
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
private:
	void switchAudioSample(int target);
	void switchNetwork(P2PNetwork::protocol);
	Ui::MainWindow* ui;

	QIODevice* device = nullptr;

	QAudioOutput* output = nullptr;
	QAudioInput* input = nullptr;
	QAudioFormat format;
	bool isconnected = false;
	bool isudpplaying = false;
	bool ismsgconnected = false;
	P2PNetwork* p2p;
	P2PNetwork* p2p_msg;

	enum RecvMode
	{
		NotStarted,
		Transforming,
		EndFile
	} RM = NotStarted;

	QFile* file = nullptr;
	QElapsedTimer startrecv;

	enum SendMode
	{
		SNotStarted,
		SWaitRepsone,
		STransforming
	} SM = SNotStarted;

	QFile* sf = nullptr;
	QElapsedTimer startsend;
	QByteArray readbuf;
	qint64 cur = 0;
	qint64 size = 0;
	qint64 rcur = 0;
	qint64 rsize = 0;
};
#endif // MAINWINDOW_H
