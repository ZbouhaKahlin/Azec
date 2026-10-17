#pragma once
#include <QtCore>
#include <QtNetwork>
#include <functional>

class PhoneServer final : public QObject {
    QTcpServer server_;
    QString token_=QUuid::createUuid().toString(QUuid::Id128),code_=QString::number(QRandomGenerator::global()->bounded(100000,1000000));
    std::function<QJsonObject()> state_;
    std::function<QJsonObject(const QJsonObject&)> update_;
    QHash<QString,QPair<int,qint64>> failures_;
    QStringList hosts_;
    int connections_=0;
    static void reply(QTcpSocket* socket,int status,const QByteArray& body,const QByteArray& type="application/json; charset=utf-8"){
        const QByteArray reason=status==200?"OK":status==400?"Bad Request":status==401?"Unauthorized":status==403?"Forbidden":status==404?"Not Found":status==429?"Too Many Requests":"Error";
        socket->write("HTTP/1.1 "+QByteArray::number(status)+" "+reason+"\r\nContent-Type: "+type+"\r\nContent-Length: "+QByteArray::number(body.size())+"\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nReferrer-Policy: no-referrer\r\nContent-Security-Policy: default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; img-src 'self' data:; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'\r\nConnection: close\r\n\r\n"+body);socket->disconnectFromHost();
    }
    static QByteArray json(const QJsonObject& value){return QJsonDocument(value).toJson(QJsonDocument::Compact);}
    void handle(QTcpSocket* socket,const QByteArray& request,int headerEnd,int length){
        const auto lines=request.left(headerEnd).split('\n');const auto first=lines.value(0).trimmed().split(' ');if(first.size()!=3){reply(socket,400,"{}");return;}
        const auto method=first[0];const auto target=QUrl::fromEncoded(first[1]);const auto path=target.path();QHash<QByteArray,QByteArray> headers;
        for(int i=1;i<lines.size();++i){const auto line=lines[i].trimmed();const int colon=line.indexOf(':');if(colon>0)headers[line.left(colon).toLower()]=line.mid(colon+1).trimmed();}
        const auto host=QString::fromUtf8(headers.value("host")).toLower();const auto hostname=host.section(':',0,0);const auto expectedHost=hostname+":"+QString::number(port());
        if(!hosts_.contains(hostname)||host!=expectedHost){reply(socket,403,"{\"error\":\"Invalid host\"}");return;}
        const auto origin=headers.value("origin");if(!origin.isEmpty()&&origin!="http://"+host.toUtf8()){reply(socket,403,"{\"error\":\"Invalid origin\"}");return;}
        if(method=="GET"&&path=="/"){QFile file(":/ui/phone.html");if(!file.open(QIODevice::ReadOnly)){reply(socket,404,"{}");return;}reply(socket,200,file.readAll(),"text/html; charset=utf-8");return;}
        if(method=="GET"&&path.startsWith("/icons/")){
            const auto name=path.mid(7);if(!QRegularExpression("^[a-z0-9]+\\.svg$").match(name).hasMatch()){reply(socket,404,"{}");return;}
            QFile file(":/operators/"+name);if(!file.open(QIODevice::ReadOnly)){reply(socket,404,"{}");return;}reply(socket,200,file.readAll(),"image/svg+xml");return;
        }
        const auto peer=socket->peerAddress().toString();auto& attempts=failures_[peer];const auto now=QDateTime::currentMSecsSinceEpoch();if(now-attempts.second>60000)attempts={0,now};
        if(attempts.first>=5){reply(socket,429,"{\"error\":\"Wait a minute before pairing again\"}");return;}
        QJsonParseError parse;const auto body=QJsonDocument::fromJson(request.mid(headerEnd+4,length),&parse);
        if(method=="POST"&&path=="/api/pair"){
            if(parse.error!=QJsonParseError::NoError||!body.isObject()||body.object()["code"].toString()!=code_){++attempts.first;reply(socket,401,"{\"error\":\"Incorrect pairing code\"}");return;}
            attempts.first=0;reply(socket,200,json({{"token",token_}}));return;
        }
        if(headers.value("authorization")!="Bearer "+token_.toUtf8()){++attempts.first;reply(socket,401,"{\"error\":\"Pair with the PC first\"}");return;}
        if(method=="GET"&&path=="/api/state"){reply(socket,200,json(state_()));return;}
        if(method=="POST"&&path=="/api/update"){
            if(parse.error!=QJsonParseError::NoError||!body.isObject()){reply(socket,400,"{\"error\":\"Invalid JSON\"}");return;}
            const auto result=update_(body.object());reply(socket,result.contains("error")?400:200,json(result));return;
        }
        reply(socket,404,"{\"error\":\"Not found\"}");
    }
    void accept(){
        while(server_.hasPendingConnections()){
            auto* socket=server_.nextPendingConnection();if(connections_>=24){socket->abort();socket->deleteLater();continue;}++connections_;socket->setReadBufferSize(32768);
            auto buffer=QSharedPointer<QByteArray>::create();auto completed=QSharedPointer<bool>::create(false);
            auto* timeout=new QTimer(socket);timeout->setSingleShot(true);timeout->setInterval(5000);connect(timeout,&QTimer::timeout,socket,&QTcpSocket::abort);timeout->start();
            connect(socket,&QTcpSocket::disconnected,this,[this,socket]{--connections_;socket->deleteLater();});
            connect(socket,&QTcpSocket::readyRead,this,[this,socket,buffer,completed,timeout]{
                if(*completed)return;buffer->append(socket->readAll());const int end=buffer->indexOf("\r\n\r\n");
                if(buffer->size()>32768||(end<0&&buffer->size()>8192)){*completed=true;reply(socket,400,"{}");return;}if(end<0)return;
                int length=0;bool hasLength=false;
                for(const auto& line:buffer->left(end).split('\n')){if(line.trimmed().toLower().startsWith("transfer-encoding:")){*completed=true;reply(socket,400,"{}");return;}if(line.trimmed().toLower().startsWith("content-length:")){bool ok=false;const auto parsed=line.mid(line.indexOf(':')+1).trimmed().toInt(&ok);if(hasLength||!ok||parsed<0||parsed>16384){*completed=true;reply(socket,400,"{}");return;}hasLength=true;length=parsed;}}
                if(end>8192){*completed=true;reply(socket,400,"{}");return;}if(buffer->size()<end+4+length)return;
                *completed=true;timeout->stop();handle(socket,*buffer,end,length);
            });
        }
    }
public:
    PhoneServer(std::function<QJsonObject()> state,std::function<QJsonObject(const QJsonObject&)> update,QObject* parent=nullptr):QObject(parent),state_(std::move(state)),update_(std::move(update)){connect(&server_,&QTcpServer::newConnection,this,[this]{accept();});}
    bool start(){
        if(server_.isListening())return true;
        if(!server_.listen(QHostAddress::AnyIPv4,18764)&&!server_.listen(QHostAddress::AnyIPv4,0))return false;
        hosts_={"localhost","127.0.0.1"};for(const auto& address:QNetworkInterface::allAddresses())if(address.protocol()==QAbstractSocket::IPv4Protocol&&!hosts_.contains(address.toString()))hosts_<<address.toString();return true;
    }
    void stop(){server_.close();for(auto* socket:server_.findChildren<QTcpSocket*>())socket->abort();}
    bool listening() const{return server_.isListening();}
    quint16 port() const{return server_.serverPort();}
    QString code() const{return code_;}
    QString token() const{return token_;}
    QString address() const{
        QString best="127.0.0.1";
        for(const auto& interface:QNetworkInterface::allInterfaces()){
            const auto flags=interface.flags();if(!(flags&QNetworkInterface::IsUp)||!(flags&QNetworkInterface::IsRunning)||(flags&QNetworkInterface::IsLoopBack))continue;
            for(const auto& entry:interface.addressEntries()){const auto ip=entry.ip();if(ip.protocol()!=QAbstractSocket::IPv4Protocol||ip.isLinkLocal())continue;
                if(best=="127.0.0.1"||interface.type()==QNetworkInterface::Wifi){best=ip.toString();if(interface.type()==QNetworkInterface::Wifi)return "http://"+best+":"+QString::number(port());}
            }
        }
        return "http://"+best+":"+QString::number(port());
    }
    QString pairingLink() const{return address()+"/#token="+token_;}
};
