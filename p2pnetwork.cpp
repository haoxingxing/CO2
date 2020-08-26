#include "p2pnetwork.h"
#include "tools.h"
const char* name_protocol[] = {"None", "UDP", "TCP",};
const char* name_curtcpst[] = {"BLANK", "LISTENING", "CONNECTING", "CONNECTED",};
const char* name_curudpst[] = {"BLANK", "LISTENING", "CONNECTED",};
//#define P2PNETWORK_STATUS "status:" << tcpS << tcpC << name_curtcpst[static_cast<int>(CurTCPSTATUS)] << udp << name_curudpst[static_cast<int>(CurUDPSTATUS)] << name_protocol[static_cast<int>(_protocol)]  // NOLINT(cppcoreguidelines-macro-usage)
#define P2PNETWORK_STATUS ""

P2PNetwork::P2PNetwork(QObject* parent, QHostAddress bind_address, int port) : QObject(parent),
                                                                               bindaddress(bind_address), port(port)
{
	connect(this, &P2PNetwork::protocolSwitched, this, [&](protocol p)
	{
		DEBUG << "Protocol Switched" << name_protocol[static_cast<int>(p)] << P2PNETWORK_STATUS;
	});
	connect(this, &P2PNetwork::connected, this, [&]
	{
		DEBUG << "connected" << P2PNETWORK_STATUS;
	});
	connect(this, &P2PNetwork::disconnected, this, [&]
	{
		DEBUG << "disconnected" << P2PNETWORK_STATUS;
	});
	connect(this, &P2PNetwork::error, this, [&](QAbstractSocket::SocketError e, const QString& str)
	{
		DEBUG << e << str << P2PNETWORK_STATUS;
	});
}

void P2PNetwork::switchProtocol(protocol target)
{
	switch (_protocol)
	{
	case protocol::UDP:
		destoryUDP();
		break;
	case protocol::TCP:
		destoryTCP();
		break;
	case protocol::None:
		break;
	}
	_protocol = target;
	switch (_protocol)
	{
	case protocol::UDP:
		initUDP();
		break;
	case protocol::TCP:
		initTCP();
		break;
	case protocol::None:
		break;
	}
	emit protocolSwitched(_protocol);
}

QAbstractSocket* P2PNetwork::getSocket() const
{
	switch (_protocol)
	{
	case protocol::UDP:
		return udp;
	case protocol::TCP:
		return tcpC;
	default:
		return nullptr;
	}
}

void P2PNetwork::connectToHost(const QString& host, int port)
{
	DEBUG << "connect to" << host << port << "via" << name_protocol[static_cast<int>(_protocol)];
	if (_protocol == protocol::None)
	{
		DEBUG << "no protocol selected" << P2PNETWORK_STATUS;
		emit disconnected();
		return;
	}

	if (_protocol == protocol::TCP && CurTCPSTATUS == TCP_status::LISTENING)
	{
		stopTCP();
		CurTCPSTATUS = TCP_status::CONNECTING;
		tcpC = new QTcpSocket(this);
		connect(tcpC, &QTcpSocket::connected, this, [&]
		{
			CurTCPSTATUS = TCP_status::CONNECTED;
			emit connected();
		});

		connect(tcpC, &QTcpSocket::disconnected, this, [&]
		{
			if (CurTCPSTATUS == TCP_status::CONNECTED)
			{
				emit disconnected(); //CleanUp Or Crash
				CurTCPSTATUS = TCP_status::BLANK;
			}
			destoryTcpSocket();
			restartTCP();
		});
		connect(tcpC, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this,
		        [&](QAbstractSocket::SocketError e)
		        {
			        emit error(e, tcpC->errorString());
			        if (CurTCPSTATUS == TCP_status::CONNECTING)
			        {
				        emit disconnected();
				        destoryTcpSocket();
				        restartTCP();
			        }
		        });
	}
	getSocket()->connectToHost(host, port);
}

void P2PNetwork::disconnectFromHost() const
{
	switch (_protocol)
	{
	case protocol::UDP:
		if (CurUDPSTATUS == UDP_status::CONNECTED)
		{
			udp->disconnectFromHost();
		}
		else
		{
			DEBUG << "not connected" << P2PNETWORK_STATUS;
		}
		break;
	case protocol::TCP:
		switch (CurTCPSTATUS)
		{
		case TCP_status::LISTENING:
			DEBUG << "not connected" << P2PNETWORK_STATUS;
			break;
		case TCP_status::CONNECTING:
		case TCP_status::CONNECTED:
			tcpC->disconnectFromHost();
			break;
		default:
			assert(false);
		}
	case protocol::None:
		DEBUG << "not connected" << P2PNETWORK_STATUS;
		break;
	}
}

void P2PNetwork::destoryTCP()
{
	switch (CurTCPSTATUS)
	{
	case TCP_status::LISTENING:
		destoryTcpServer();
		CurTCPSTATUS = TCP_status::BLANK;
		break;
	case TCP_status::CONNECTED:
	case TCP_status::CONNECTING:
		destoryTcpSocket();
		destoryTcpServer();
		CurTCPSTATUS = TCP_status::BLANK;
		break;
	case TCP_status::BLANK:
		break;
	}
}

void P2PNetwork::initUDP()
{
	udp = new QUdpSocket(this);
	udp->bind(bindaddress, port);
	CurUDPSTATUS = UDP_status::LISTENING;
	connect(udp, &QUdpSocket::connected, this, [&]
	{
		CurUDPSTATUS = UDP_status::CONNECTED;
		emit connected();
	});
	connect(udp, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this,
	        [&](QAbstractSocket::SocketError e)
	        {
		        emit error(e, udp->errorString());
		        emit disconnected();
	        });
	connect(udp, &QUdpSocket::disconnected, this, [&]
	{
		emit disconnected(); //CleanUp Or Crash
		restartUDP();
	});
	DEBUG << "Initialize UDP" << P2PNETWORK_STATUS;
}

void P2PNetwork::stopTCP() const
{
	tcpS->close();
	DEBUG << "Stopped TCP Server" << P2PNETWORK_STATUS;
}

void P2PNetwork::restartTCP()
{
	CurTCPSTATUS = TCP_status::LISTENING;
	tcpS->listen(bindaddress, port);
	DEBUG << "Restarted TCP Server" << P2PNETWORK_STATUS;
}

void P2PNetwork::restartUDP()
{
	CurUDPSTATUS = UDP_status::LISTENING;
	udp->bind(bindaddress, port);
	DEBUG << "Restarted UDP Server" << P2PNETWORK_STATUS;
}

void P2PNetwork::destoryUDP()
{
	if (CurUDPSTATUS == UDP_status::CONNECTED)
	{
		emit disconnected();
	}
	DEBUG << "Disconnect SIGNAL/SLOT" << udp->disconnect() << P2PNETWORK_STATUS;
	udp->deleteLater();
	udp = nullptr;
	CurUDPSTATUS = UDP_status::BLANK;
	DEBUG << "UDP Destoryed" << P2PNETWORK_STATUS;
}

void P2PNetwork::initTCP()
{
	tcpS = new QTcpServer(this);
	tcpS->setMaxPendingConnections(1);
	connect(tcpS, &QTcpServer::acceptError, this, [&](QAbstractSocket::SocketError e)
	{
		emit error(e, tcpS->errorString());
	});

	connect(tcpS, &QTcpServer::newConnection, this, [&]
	{
		tcpC = tcpS->nextPendingConnection();
		stopTCP();
		tcpC->setParent(this);
		CurTCPSTATUS = TCP_status::CONNECTED;
		connect(tcpC, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this,
		        [&](QAbstractSocket::SocketError e)
		        {
			        emit error(e, tcpC->errorString());
		        });
		connect(tcpC, &QTcpSocket::disconnected, this, [&]
		{
			if (CurTCPSTATUS == TCP_status::CONNECTED)
			{
				emit disconnected(); //CleanUp Or Crash
				CurTCPSTATUS = TCP_status::BLANK;
			}
			destoryTcpSocket();
			restartTCP();
		});
		emit connected();
	});
	CurTCPSTATUS = TCP_status::LISTENING;
	tcpS->listen(bindaddress, port);
	DEBUG << "Initialize TCP" << P2PNETWORK_STATUS;
}

void P2PNetwork::destoryTcpServer()
{
	DEBUG << "Disconnect SIGNAL/SLOT" << tcpS->disconnect() << P2PNETWORK_STATUS;
	tcpS->deleteLater();
	tcpS = nullptr;
	DEBUG << "TCP Server Destoryed" << P2PNETWORK_STATUS;
}

void P2PNetwork::destoryTcpSocket()
{
	if (CurTCPSTATUS == TCP_status::CONNECTED)
	{
		emit disconnected();
		tcpC->disconnectFromHost();
	}
	DEBUG << "Disconnect SIGNAL/SLOT" << tcpC->disconnect() << P2PNETWORK_STATUS;
	tcpC->deleteLater();
	tcpC = nullptr;
	CurTCPSTATUS = TCP_status::BLANK;
	DEBUG << "TCP Socket Destoryed" << P2PNETWORK_STATUS;
}
