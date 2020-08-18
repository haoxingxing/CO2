#ifndef P2PNETWORK_H
#define P2PNETWORK_H
#include <QObject>
#include <QTimer>
#include <QUdpSocket>
#include <QTcpServer>
#include <QTcpSocket>

class P2PNetwork : public QObject
{
Q_OBJECT
public:
	enum protocol
	{
		UDP,
		TCP,
		None
	} _protocol = None;

	P2PNetwork(QObject* parent, int port);
	void switchProtocol(protocol target);
	QAbstractSocket* getSocket() const;
	void connectToHost(const QString& host, int port);
	void disconnectFromHost();
signals:
	void disconnected();
	void connected();
	void protocolSwitched(protocol);
private:
	void destoryTCP();
	void destoryUDP();
	void initTCP();
	void initUDP();

	void restartTCP();
	void restartUDP();
	QTimer* waitTcpConnected = nullptr;
	QUdpSocket* udp = nullptr;
	QTcpSocket* tcpC = nullptr;
	QTcpServer* tcpS = nullptr;

	enum
	{
		TCONNECTED,
		TCONNECTING,
		TLISTENING,
		TBLANK
	} CurTCPSTATUS = TBLANK;

	enum
	{
		UCONNECTED,
		ULISTENING,
		UBLANK
	} CurUDPSTATUS = UBLANK;

	void destoryTcpServer();
	void destoryTcpSocket();
	int port;
};

#endif // P2PNETWORK_H
