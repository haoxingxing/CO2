#include "p2pnetwork.h"
const char* h[] = { "UDP", "TCP", "None" };

P2PNetwork::P2PNetwork(QObject* parent,int port) : QObject(parent), port(port)
{
    connect(this, &P2PNetwork::protocolSwitched, this, [&](protocol p) {
        qDebug() << "Protocol Switched" << h[p];
    	});
    connect(this, &P2PNetwork::connected, this, [&] {
        qDebug() << "connected";
        });
    connect(this, &P2PNetwork::disconnected, this, [&] {
        qDebug() << "disconnected";
        });
}

void P2PNetwork::switchProtocol(P2PNetwork::protocol target)
{
    switch (_protocol) {
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
    switch (_protocol) {
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

QAbstractSocket* P2PNetwork::getSocket()
{
    if (_protocol == UDP)
        return udp;
    else
        if (_protocol == TCP)
            return tcpC;
    return nullptr;
}

void P2PNetwork::connectToHost(const QString& host, int port)
{
    qDebug() << "connect to" << host << port << "via" << h[_protocol];
	if (_protocol == None)
	{
        qDebug() << "no protocol selected";
        return;
	}
    if ((_protocol == TCP && CurTCPSTATUS == TCONNECTED) || (_protocol == UDP && CurUDPSTATUS == UCONNECTED))
    {
        qDebug() << "already connected";
        return;
    }
    if (_protocol == TCP)
    {
        destoryTCP();
        _protocol = TCP;
        CurTCPSTATUS = TCONNECTING;
        tcpC = new QTcpSocket(this);
        connect(tcpC, &QTcpSocket::connected, this, [&]
        {
            CurTCPSTATUS = TCONNECTED;
            emit connected();
        });
        connect(tcpC, &QTcpSocket::disconnected, this, [&] {
            emit disconnected(); //CleanUp Or Crash
            destoryTcpSocket();
            CurTCPSTATUS = TBLANK;
            switchProtocol(None);
            });
    }
    getSocket()->connectToHost(host,port);
}

void P2PNetwork::disconnectFromHost()
{
    switchProtocol(None);
}

void P2PNetwork::destoryTCP()
{
    switch (CurTCPSTATUS) {
    case TLISTENING:
        destoryTcpServer();
        CurTCPSTATUS = TBLANK;
        break;
    case TCONNECTED:
        destoryTcpSocket();
        break;
    case TCONNECTING:
        destoryTcpSocket();
        break;
    case TBLANK:
        break;
    }
    _protocol = None;
}

void P2PNetwork::destoryUDP()
{
    if (udp != nullptr){
        if (CurUDPSTATUS == UCONNECTED)
            emit disconnected();
        qDebug() << "UDP Destoryed";
        udp->close();
        udp->disconnect();
        udp->deleteLater();
        udp = nullptr;
        CurUDPSTATUS = UBLANK;
    }
    _protocol = None;
}

void P2PNetwork::initTCP()
{
    qDebug() << "Initialize TCP";
    tcpS = new QTcpServer(this);
    tcpS->setMaxPendingConnections(1);
    connect(tcpS, &QTcpServer::newConnection, this,[&]{
        tcpC = tcpS->nextPendingConnection();
        tcpC->setParent(this);
        CurTCPSTATUS = TCONNECTED;
        destoryTcpServer();
        connect(tcpC, &QTcpSocket::disconnected,this,[&]{
            emit disconnected(); //CleanUp Or Crash
            destoryTcpSocket();
            CurTCPSTATUS = TBLANK;
            switchProtocol(None);
        });
        emit connected();
    });
    CurTCPSTATUS = TLISTENING;
    tcpS->listen(QHostAddress::Any,port);

}

void P2PNetwork::initUDP()
{
    qDebug() << "Initialize UDP";
    udp = new QUdpSocket(this);
    udp->bind(QHostAddress::Any, port);
    CurUDPSTATUS = ULISTENING;
    connect(udp,&QUdpSocket::connected,this,[&]{
        CurUDPSTATUS = UCONNECTED;
        emit connected();
    });
}

void P2PNetwork::destoryTcpServer()
{
    if (tcpS != nullptr){
        qDebug() << "TCP Server Destoryed";
        tcpS->close();
        tcpS->disconnect();
        tcpS->deleteLater();
        tcpS = nullptr;
    }
}

void P2PNetwork::destoryTcpSocket()
{
    if (tcpC != nullptr){
        qDebug() << "TCP Socket Destoryed";
        tcpC->close();
        tcpC->disconnect();
        tcpC->deleteLater();
        tcpC = nullptr;
    }
}
