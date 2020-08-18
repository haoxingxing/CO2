#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAudioOutput>
#include <QAudioInput>
#include <QEvent>
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
	void playData();
	void SwitchedNetwork(P2PNetwork::protocol);
private slots:
	void on_pushButton_clicked();

	void on_InVol_valueChanged(int value);

	void on_comboBox_currentIndexChanged(const QString& arg1);

	void on_hz_currentTextChanged(const QString& arg1);

	void on_msg_send_clicked();

	void on_msg_conn_clicked();

	void on_pushButton_2_clicked();

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
};
#endif // MAINWINDOW_H
