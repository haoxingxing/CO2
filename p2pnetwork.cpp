#include "p2pnetwork.h"
const char* name_protocol[] = { "UDP", "TCP", "None" };
const char* name_curtcpst[] = { "TCONNECTED", "TCONNECTING", "TLISTENING", "TBLANK" };
const char* name_curudpst[] = { "UCONNECTED","ULISTENING","UBLANK" };
P2PNetwork::P2PNetwork(QObject* parent, int port) : QObject(parent), port(port)
{
	connect(this, &P2PNetwork::protocolSwitched, this, [&](protocol p)
		{
			qDebug() << "Protocol Switched" << name_protocol[p] << tcpS << tcpC << name_curtcpst[CurTCPSTATUS] << udp << name_curudpst[CurUDPSTATUS] << name_protocol[_protocol];
		});
	connect(this, &P2PNetwork::connected, this, [&]
		{
			qDebug() << "connected" << tcpS << tcpC << name_curtcpst[CurTCPSTATUS] << udp << name_curudpst[CurUDPSTATUS] << name_protocol[_protocol];
		});
	connect(this, &P2PNetwork::disconnected, this, [&]
		{
			qDebug() << "disconnected" << tcpS << tcpC << name_curtcpst[CurTCPSTATUS] << udp << name_curudpst[CurUDPSTATUS] << name_protocol[_protocol];
		});
}

void P2PNetwork::switchProtocol(protocol target)
{
	switch (_protocol)
	{
	case UDP:
		destoryUDP();
		break;
	case TCP:
		destoryTCP();
		break;
	case None:
		break;
	}
	_protocol = target;
	switch (_protocol)
	{
	case UDP:
		initUDP();
		emit protocolSwitched(UDP);
		break;
	case TCP:
		initTCP();
		emit protocolSwitched(TCP);
		break;
	case None:
		emit protocolSwitched(None);
		break;
	}
}

QAbstractSocket* P2PNetwork::getSocket() const
{
	switch (_protocol)
	{
	case UDP:
		return udp;
	case TCP:
		return tcpC;
	default:
		return nullptr;
	}
}

void P2PNetwork::connectToHost(const QString& host, int port)
{
	qDebug() << "connect to" << host << port << "via" << name_protocol[_protocol];
	if (_protocol == None)
	{
		qDebug() << "no protocol selected" << tcpS << tcpC << name_curtcpst[CurTCPSTATUS] << udp << name_curudpst[CurUDPSTATUS] << name_protocol[_protocol];
		emit disconnected();
		return;
	}
	if ((_protocol == TCP && (CurTCPSTATUS == TCONNECTED || CurTCPSTATUS == TCONNECTING)) || (_protocol == UDP &&
		CurUDPSTATUS == UCONNECTED))
	{
		qDebug() << "already connecting or connected" << tcpS << tcpC << name_curtcpst[CurTCPSTATUS] << udp << name_curudpst[CurUDPSTATUS] << name_protocol[_protocol];
		return;
	}

	if (_protocol == TCP && CurTCPSTATUS == TLISTENING)
	{
		tcpS->close();
		qDebug() << "Stopped TCP Server" << tcpS << tcpC << name_curtcpst[CurTCPSTATUS] << udp << name_curudpst[CurUDPSTATUS] << name_protocol[_protocol];
		CurTCPSTATUS = TCONNECTING;
		tcpC = new QTcpSocket(this);
		waitTcpConnected = new QTimer(this);
		connect(waitTcpConnected, &QTimer::timeout, this, [&]
			{
				waitTcpConnected->stop();
				waitTcpConnected->deleteLater();
				waitTcpConnected = nullptr;
				tcpC->abort();
				//emit disconnected();
				//switchProtocol(None);
				//CurTCPSTATUS = TBLANK;   //The timeout may not work
			});
		connect(tcpC, &QTcpSocket::connected, this, [&]
			{
				waitTcpConnected->stop();
				waitTcpConnected->deleteLater();
				waitTcpConnected = nullptr;
				CurTCPSTATUS = TCONNECTED;
				emit connected();
			});
		connect(tcpC, &QTcpSocket::disconnected, this, [&]
			{
				if (CurTCPSTATUS == TCONNECTED) {
					emit disconnected(); //CleanUp Or Crash
					CurTCPSTATUS = TBLANK;
				}
				destoryTcpSocket();
				restartTCP();
			});
		waitTcpConnected->start(5000);
	}
	getSocket()->connectToHost(host, port);
}

void P2PNetwork::disconnectFromHost()
{
	switch (_protocol)
	{
	case UDP:
		if (CurUDPSTATUS == UCONNECTED)
		{
			udp->disconnectFromHost();
		}
		else
		{
			qDebug() << "not connected" << tcpS << tcpC << name_curtcpst[CurTCPSTATUS] << udp << name_curudpst[CurUDPSTATUS] << name_protocol[_protocol];
		}
		break;
	case TCP:
		switch (CurTCPSTATUS)
		{
		case TLISTENING:
			qDebug() << "not connected" << tcpS << tcpC << name_curtcpst[CurTCPSTATUS] << udp << name_curudpst[CurUDPSTATUS] << name_protocol[_protocol];
			break;
		case TCONNECTING:
		case TCONNECTED:
			tcpC->disconnectFromHost();
			break;
		default:
			assert(false);
		}
	case None:
		qDebug() << "not connected" << tcpS << tcpC << name_curtcpst[CurTCPSTATUS] << udp << name_curudpst[CurUDPSTATUS] << name_protocol[_protocol];
		break;
	}
}

void P2PNetwork::destoryTCP()
{
	switch (CurTCPSTATUS)
	{
	case TLISTENING:
		destoryTcpServer();
		CurTCPSTATUS = TBLANK;
		break;
	case TCONNECTED:
	case TCONNECTING:
		destoryTcpSocket();
		destoryTcpServer();
		CurTCPSTATUS = TBLANK;
		break;
	case TBLANK:
		break;
	}
}

void P2PNetwork::initUDP()
{
	udp = new QUdpSocket(this);
	udp->bind(QHostAddress::Any, port);
	CurUDPSTATUS = ULISTENING;
	connect(udp, &QUdpSocket::connected, this, [&]
		{
			CurUDPSTATUS = UCONNECTED;
			emit connected();
		});
	connect(udp, &QUdpSocket::disconnected, this, [&]
		{
			emit disconnected(); //CleanUp Or Crash
			CurUDPSTATUS = ULISTENING;
			udp->bind(QHostAddress::Any, port);
		});
	qDebug() << "Initialize UDP" << tcpS << tcpC << name_curtcpst[CurTCPSTATUS] << udp << name_curudpst[CurUDPSTATUS] << name_protocol[_protocol];
}

void P2PNetwork::restartTCP()
{
	CurTCPSTATUS = TLISTENING;
	tcpS->listen(QHostAddress::Any, port);
	qDebug() << "Restarted TCP Server" << tcpS << tcpC << name_curtcpst[CurTCPSTATUS] << udp << name_curudpst[CurUDPSTATUS] << name_protocol[_protocol];
}

void P2PNetwork::destoryUDP()
{
	if (CurUDPSTATUS == UCONNECTED) {
		emit disconnected();
	}
	udp->disconnect();
	udp->deleteLater();
	udp = nullptr;
	CurUDPSTATUS = UBLANK;
	qDebug() << "UDP Destoryed" << tcpS << tcpC << name_curtcpst[CurTCPSTATUS] << udp << name_curudpst[CurUDPSTATUS] << name_protocol[_protocol];
}
void P2PNetwork::initTCP()
{
	tcpS = new QTcpServer(this);
	tcpS->setMaxPendingConnections(1);
	connect(tcpS, &QTcpServer::newConnection, this, [&]
		{
			tcpC = tcpS->nextPendingConnection();
			tcpS->close();
			qDebug() << "Stopped TCP Server" << tcpS << tcpC << name_curtcpst[CurTCPSTATUS] << udp << name_curudpst[CurUDPSTATUS] << name_protocol[_protocol];
			tcpC->setParent(this);
			CurTCPSTATUS = TCONNECTED;
			connect(tcpC, &QTcpSocket::disconnected, this, [&]
				{
					if (CurTCPSTATUS == TCONNECTED) {
						emit disconnected(); //CleanUp Or Crash
						CurTCPSTATUS = TBLANK;
					}
					destoryTcpSocket();
					restartTCP();
				});
			emit connected();
		});
	CurTCPSTATUS = TLISTENING;
	tcpS->listen(QHostAddress::Any, port);
	qDebug() << "Initialize TCP" << tcpS << tcpC << name_curtcpst[CurTCPSTATUS] << udp << name_curudpst[CurUDPSTATUS] << name_protocol[_protocol];
}
void P2PNetwork::destoryTcpServer()
{
	tcpS->disconnect();
	tcpS->deleteLater();
	tcpS = nullptr;
	qDebug() << "TCP Server Destoryed" << tcpS << tcpC << name_curtcpst[CurTCPSTATUS] << udp << name_curudpst[CurUDPSTATUS] << name_protocol[_protocol];
}

void P2PNetwork::destoryTcpSocket()
{
	if (CurTCPSTATUS == TCONNECTED) {
		emit disconnected();
		tcpC->disconnectFromHost();
	}
	tcpC->disconnect();
	tcpC->deleteLater();
	tcpC = nullptr;
	CurTCPSTATUS = TBLANK;
	qDebug() << "TCP Socket Destoryed" << tcpS << tcpC << name_curtcpst[CurTCPSTATUS] << udp << name_curudpst[CurUDPSTATUS] << name_protocol[_protocol];
}