#include "WebsocketAPI.h"

#include "QtWebSockets/qwebsocketserver.h"
#include "QtWebSockets/qwebsocket.h"
#include <QtCore/QDebug>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonarray.h>
#include <qfile.h>
#include <qimage.h>

QT_USE_NAMESPACE

//! [constructor]
WebsocketAPI::WebsocketAPI(quint16 port, bool debug, QObject* parent) :
	QObject(parent),
	m_pWebSocketServer(new QWebSocketServer(QStringLiteral("Echo Server"), QWebSocketServer::NonSecureMode, this)),
	m_debug(debug)
{
	if (m_pWebSocketServer->listen(QHostAddress::Any, port))
	{
		if (m_debug)
			qDebug() << "Echoserver listening on port" << port;
		connect(m_pWebSocketServer, &QWebSocketServer::newConnection, this, &WebsocketAPI::onNewConnection);
		connect(m_pWebSocketServer, &QWebSocketServer::closed, this, &WebsocketAPI::closed);
	}
}
//! [constructor]




WebsocketAPI::~WebsocketAPI()
{
	m_pWebSocketServer->close();
	qDeleteAll(m_clients.begin(), m_clients.end());
}

//! [onNewConnection]
void WebsocketAPI::onNewConnection()
{

	m_count = 0;
	QWebSocket* pSocket = m_pWebSocketServer->nextPendingConnection();

	connect(pSocket, &QWebSocket::textMessageReceived, this, &WebsocketAPI::processTextMessage);
	connect(pSocket, &QWebSocket::binaryMessageReceived, this, &WebsocketAPI::processBinaryMessage);
	connect(pSocket, &QWebSocket::disconnected, this, &WebsocketAPI::socketDisconnected);

	m_clients << pSocket;
}
//! [onNewConnection]

//! [processTextMessage]
void WebsocketAPI::processTextMessage(QString message)
{
	QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());

	auto Request = QJsonDocument::fromJson(message.toLatin1());


	if (pClient && Request["method"].toString() == "wslink.hello")
	{

		const auto ClientID = QJsonObject{{"clientID", "123456789"}};
		const auto resultArray = QJsonArray{ClientID};


		QJsonObject ResponseArray;

		ResponseArray["wslink"] = "1.0";
		ResponseArray["id"] = Request["id"].toString();
		ResponseArray["result"] = ClientID;

		const QJsonDocument Response{ResponseArray};




		pClient->sendTextMessage(Response.toJson());
	}
	else
	{
		



		QJsonObject ResponseArray;

		ResponseArray["wslink"] = "1.0";
		ResponseArray["id"] = Request["id"].toString();
		
		const auto success = QJsonObject{{"result", "success"}};
		if (Request["method"].toString() == "viewport.image.push.observer.add")
		{
			const auto viewID = QJsonObject{{"result", "success"} ,{"viewId", "1"}};
			ResponseArray["result"] = viewID;
		}
		else
		{
			ResponseArray["result"] = success;
		}

		const QJsonDocument Response{ResponseArray};

		pClient->sendTextMessage(Response.toJson());

		if (Request["method"].toString() == "viewport.image.push")
		{
			sendImage(pClient);
		}

	}


}

void WebsocketAPI::sendImage(QWebSocket* pClient)
{

	QString imageString("wslink_bin");
	
	imageString.append(QString::number(m_count));

	const auto resultArray1 = QJsonArray{imageString};
	//const auto resultArray = QJsonArray{ClientID};

	QJsonObject ResponseArray;

	ResponseArray["wslink"] = "1.0";
	ResponseArray["method"] = "wslink.binary.attachment";
	ResponseArray["args"] = resultArray1;

	const QJsonDocument Response{ResponseArray};

	pClient->sendTextMessage(Response.toJson());


	QImage img("C:\\Users\\P41877\\Downloads\\catImage.jpg");
	//QImage img2 = img.scaled(1920, 872);

	//img2.save("C:\\Users\\P41877\\Downloads\\catImage2.jpg");

	QByteArray ba;

	QFile file("C:\\Users\\P41877\\Downloads\\catImage.jpg");
	QDataStream in(&file);
	file.open(QIODevice::ReadOnly);

	ba.resize(file.size());
	in.readRawData(ba.data(), file.size());
	pClient->sendBinaryMessage(ba);

	auto imageSize = ba.size();

	const auto resultArray2 = QJsonArray{img.size().width(), img.size().height()};
	const auto result = QJsonObject{{"format", "jpeg"}, {"global_id", 1}, {"global_id", "1"}, {"id", "1"},
		{"image", imageString}, {"localTime", 0}, {"memsize", imageSize}, {"mtime", 2125+m_count*5}, {"size", resultArray2},
		{"stale", m_count%2==0}, {"workTime", 77}};
	

	QJsonObject ResponseArray2;

	ResponseArray2["wslink"] = "1.0";
	ResponseArray2["id"] = "publish:viewport.image.push.subscription:0";
	ResponseArray2["result"] = result;

	const QJsonDocument Response2{ResponseArray2};

	pClient->sendTextMessage(Response2.toJson());

	m_count++;

}



//! [processTextMessage]

//! [processBinaryMessage]
void WebsocketAPI::processBinaryMessage(QByteArray message)
{
	QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());
	if (m_debug)
		qDebug() << "Binary Message received:" << message;
	if (pClient)
	{
		pClient->sendBinaryMessage(message);
	}
}
//! [processBinaryMessage]

//! [socketDisconnected]
void WebsocketAPI::socketDisconnected()
{
	QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());
	if (m_debug)
		qDebug() << "socketDisconnected:" << pClient;
	if (pClient)
	{
		m_clients.removeAll(pClient);
		pClient->deleteLater();
	}
}
//! [socketDisconnected]