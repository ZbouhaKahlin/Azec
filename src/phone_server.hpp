#pragma once
#include <QtCore>
#include <QtNetwork>
#include <functional>
#include <QSvgRenderer>
#include <QPainter>
#include "diagnostics.hpp"
#include "lan_address.hpp"
#include "theme.hpp"

class PhoneServer final : public QObject {
    QTcpServer server_;
    QString token_=QUuid::createUuid().toString(QUuid::Id128),code_=QString::number(QRandomGenerator::system()->bounded(100000,1000000));
    std::function<QJsonObject()> state_;
    std::function<QJsonObject(const QJsonObject&)> update_;
    QHash<QString,QPair<int,qint64>> failures_;
    QHash<QString,QByteArray> icon_cache_;
    QStringList hosts_;
    QTimer network_timer_;
    QElapsedTimer elapsed_;
    QString advertised_;
    std::function<void()> network_changed_;
    int connections_=0;
    static bool secretEqual(const QByteArray& actual,const QByteArray& expected){
        if(actual.size()!=expected.size())return false;
        volatile unsigned int difference=0;
        for(qsizetype i=0;i<expected.size();++i)difference=difference|unsigned(static_cast<unsigned char>(actual[i])^static_cast<unsigned char>(expected[i]));
        return difference==0;
    }
    void refreshNetwork(){
        QStringList current={"localhost","127.0.0.1"};
        for(const auto& address:QNetworkInterface::allAddresses())if(address.protocol()==QAbstractSocket::IPv4Protocol&&!current.contains(address.toString()))current<<address.toString();
        current.sort();const auto nextAddress=currentAddress();
        if(current!=hosts_||nextAddress!=advertised_){hosts_=current;advertised_=nextAddress;if(network_changed_)network_changed_();}
    }
    void pruneFailures(){
        const auto now=elapsed_.elapsed();
        for(auto it=failures_.begin();it!=failures_.end();)if(now-it.value().second>=60000)it=failures_.erase(it);else ++it;
    }
    void recordFailure(const QString& peer){
        pruneFailures();
        if(!failures_.contains(peer)&&failures_.size()>=256){
            auto oldest=failures_.begin();for(auto it=failures_.begin();it!=failures_.end();++it)if(it.value().second<oldest.value().second)oldest=it;
            failures_.erase(oldest);
        }
        if(!failures_.contains(peer))failures_.insert(peer,{0,elapsed_.elapsed()});
        ++failures_[peer].first;
    }
    static void reply(QTcpSocket* socket,int status,const QByteArray& body,const QByteArray& type="application/json; charset=utf-8"){
        const QByteArray reason=status==200?"OK":status==400?"Bad Request":status==401?"Unauthorized":status==403?"Forbidden":status==404?"Not Found":status==429?"Too Many Requests":"Error";
        socket->write("HTTP/1.1 "+QByteArray::number(status)+" "+reason+"\r\nContent-Type: "+type+"\r\nContent-Length: "+QByteArray::number(body.size())+"\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nReferrer-Policy: no-referrer\r\nContent-Security-Policy: default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; img-src 'self' data:; font-src 'self' data:; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'\r\nConnection: close\r\n\r\n"+body);socket->disconnectFromHost();
    }
    static QByteArray json(const QJsonObject& value){return QJsonDocument(value).toJson(QJsonDocument::Compact);}
    void handle(QTcpSocket* socket,const QByteArray& request,int headerEnd,int length){
        const auto lines=request.left(headerEnd).split('\n');const auto first=lines.value(0).trimmed().split(' ');if(first.size()!=3){reply(socket,400,"{}");return;}
        const auto method=first[0];const auto target=QUrl::fromEncoded(first[1]);const auto path=target.path();QHash<QByteArray,QByteArray> headers;
        for(int i=1;i<lines.size();++i){const auto line=lines[i].trimmed();const int colon=line.indexOf(':');if(colon>0)headers[line.left(colon).toLower()]=line.mid(colon+1).trimmed();}
        const auto host=QString::fromUtf8(headers.value("host")).toLower();const auto hostname=host.section(':',0,0);const auto expectedHost=hostname+":"+QString::number(port());
        if(!hosts_.contains(hostname)||host!=expectedHost){reply(socket,403,"{\"error\":\"Invalid host\"}");return;}
        const auto origin=headers.value("origin");if(!origin.isEmpty()&&origin!="http://"+host.toUtf8()){reply(socket,403,"{\"error\":\"Invalid origin\"}");return;}
        if(method=="GET"&&path=="/"){QFile file(":/ui/phone.html");if(!file.open(QIODevice::ReadOnly)){reply(socket,404,"{}");return;}auto html=file.readAll();reply(socket,200,html,"text/html; charset=utf-8");return;}
        if(method=="GET"&&(path=="/manifest.webmanifest"||path=="/app-icon.svg")){
            QFile file(path=="/manifest.webmanifest"?":/ui/manifest.webmanifest":":/ui/app-icon.svg");
            if(!file.open(QIODevice::ReadOnly)){reply(socket,404,"{}");return;}
            if(path=="/manifest.webmanifest"){
                // The installed app paints its splash and status bar from these
                // two colours, so they follow the active desktop palette.
                auto manifest=QJsonDocument::fromJson(file.readAll()).object();
                manifest["background_color"]=kbtheme::paper.name();manifest["theme_color"]=kbtheme::defaultAccent.name();
                reply(socket,200,json(manifest),"application/manifest+json");return;
            }
            reply(socket,200,file.readAll(),"image/svg+xml");return;
        }
        if(method=="GET"&&(path=="/app-icon-192.png"||path=="/app-icon-512.png"||path=="/apple-touch-icon.png")){
            if(!icon_cache_.contains(path)){
                const int size=path=="/app-icon-512.png"?512:path=="/app-icon-192.png"?192:180;
                QImage image(size,size,QImage::Format_ARGB32_Premultiplied);image.fill(Qt::transparent);
                QPainter painter(&image);QSvgRenderer renderer(QStringLiteral(":/ui/app-icon.svg"));renderer.render(&painter);painter.end();
                QByteArray bytes;QBuffer buffer(&bytes);buffer.open(QIODevice::WriteOnly);image.save(&buffer,"PNG");icon_cache_.insert(path,bytes);
            }
            reply(socket,200,icon_cache_.value(path),"image/png");return;
        }
        if(method=="GET"&&path.startsWith("/icons/")){
            const auto name=path.mid(7);if(!QRegularExpression("^[a-z0-9]+\\.svg$").match(name).hasMatch()){reply(socket,404,"{}");return;}
            QFile file(":/operators/"+name);if(!file.open(QIODevice::ReadOnly)){reply(socket,404,"{}");return;}reply(socket,200,file.readAll(),"image/svg+xml");return;
        }
        QJsonParseError parse;const auto body=QJsonDocument::fromJson(request.mid(headerEnd+4,length),&parse);
        if(method=="POST"&&path=="/api/pair"){
            pruneFailures();const auto peer=socket->peerAddress().toString();
            if(failures_.value(peer).first>=5){reply(socket,429,"{\"error\":\"Wait a minute before pairing again\"}");return;}
            if(parse.error!=QJsonParseError::NoError||!body.isObject()||!secretEqual(body.object()["code"].toString().toUtf8(),code_.toUtf8())){recordFailure(peer);reply(socket,401,"{\"error\":\"Incorrect pairing code\"}");return;}
            failures_.remove(peer);reply(socket,200,json({{"token",token_}}));return;
        }
        if(!secretEqual(headers.value("authorization"),"Bearer "+token_.toUtf8())){reply(socket,401,"{\"error\":\"Pair with the PC first\"}");return;}
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
    PhoneServer(std::function<QJsonObject()> state,std::function<QJsonObject(const QJsonObject&)> update,QObject* parent=nullptr):QObject(parent),state_(std::move(state)),update_(std::move(update)){
        elapsed_.start();connect(&server_,&QTcpServer::newConnection,this,[this]{accept();});
        network_timer_.setInterval(2500);connect(&network_timer_,&QTimer::timeout,this,[this]{refreshNetwork();pruneFailures();});
    }
    void onNetworkChanged(std::function<void()> callback){network_changed_=std::move(callback);}
    bool start(){
        if(server_.isListening())return true;
        if(!server_.listen(QHostAddress::AnyIPv4,18764)&&!server_.listen(QHostAddress::AnyIPv4,0)){diagnostic("phone start",server_.errorString());return false;}
        refreshNetwork();network_timer_.start();return true;
    }
    void stop(){network_timer_.stop();server_.close();for(auto* socket:server_.findChildren<QTcpSocket*>())socket->abort();if(network_changed_)network_changed_();}
    bool listening() const{return server_.isListening();}
    quint16 port() const{return server_.serverPort();}
    QString code() const{return code_;}
    QString token() const{return token_;}
    // Rotate the token and code, and drop current connections. The server keeps
    // listening; already-paired phones receive 401 on their next request.
    void rotate(){
        token_=QUuid::createUuid().toString(QUuid::Id128);
        code_=QString::number(QRandomGenerator::system()->bounded(100000,1000000));
        const auto sockets=findChildren<QTcpSocket*>();
        for(auto* socket:sockets)socket->abort();
        if(network_changed_)network_changed_();
    }
    QString currentAddress() const{
        return "http://"+lan::currentAddress()+":"+QString::number(port());
    }
    // refreshNetwork owns adapter enumeration; repeated UI reads in one toggle
    // must not rescan every interface for the address, link, and QR separately.
    QString address() const{return server_.isListening()&&!advertised_.isEmpty()?advertised_:currentAddress();}
    QString pairingLink() const{return address()+"/#token="+token_;}
};
