#ifndef P2PNETWORK_H
#define P2PNETWORK_H
#include <QObject>
#include <QUdpSocket>
#include <QTcpServer>
#include <QTcpSocket>

class P2PNetwork : public QObject
{
Q_OBJECT
public:
	enum class protocol
	{
		None,
		UDP,
		TCP,
	};

	P2PNetwork(QObject* parent, QHostAddress bind_address, int port);
	void switchProtocol(protocol target);
	QAbstractSocket* getSocket() const;
	void connectToHost(const QString& host, int port);
	void disconnectFromHost() const;
	protocol GetProtocol() const { return _protocol; }
signals:
	void disconnected();
	void connected();
	void protocolSwitched(protocol);
	void error(QAbstractSocket::SocketError, QString errstr);
private:
	void destoryTCP();
	void destoryUDP();
	void initTCP();
	void initUDP();
	void stopTCP() const;
	void restartTCP();
	void restartUDP();
	QUdpSocket* udp = nullptr;
	QTcpSocket* tcpC = nullptr;
	QTcpServer* tcpS = nullptr;
	QHostAddress bindaddress;
	protocol _protocol = protocol::None;

	enum class TCP_status
	{
		BLANK,
		LISTENING,
		CONNECTING,
		CONNECTED,
	} CurTCPSTATUS = TCP_status::BLANK;

	enum class UDP_status
	{
		BLANK,
		LISTENING,
		CONNECTED,
	} CurUDPSTATUS = UDP_status::BLANK;

	void destoryTcpServer();
	void destoryTcpSocket();
	int port;
};

#endif // P2PNETWORK_H
