#pragma once
#include <QtNetwork>
#include <QTimer>
#include <functional>
#include <memory>

namespace driver_setup {
struct DownloadLimits {
    qint64 bytes=16*1024*1024;
    int inactivityMs=30000;
    int totalMs=120000;
};
// Bound both the accumulated archive and Qt's incoming buffer. The caller
// still verifies the pinned archive hash before writing or extracting it.
inline void downloadArchive(QNetworkAccessManager* network,QObject* owner,const QUrl& url,
                            std::function<void(QByteArray,QString)> finished,DownloadLimits limits={}){
    struct Download {QByteArray bytes;QString error;};
    const auto state=std::make_shared<Download>();
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::UserAgentHeader,QStringLiteral("Azec/")+QCoreApplication::applicationVersion());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(limits.inactivityMs);
    auto* reply=network->get(request);reply->setReadBufferSize(64*1024);
    auto* deadline=new QTimer(reply);deadline->setSingleShot(true);
    QObject::connect(deadline,&QTimer::timeout,reply,[reply,state]{
        state->error=QStringLiteral("Driver download timed out");reply->abort();
    });
    deadline->start(limits.totalMs);
    QObject::connect(reply,&QNetworkReply::metaDataChanged,owner,[reply,state,limits]{
        if(reply->header(QNetworkRequest::ContentLengthHeader).toLongLong()>limits.bytes){
            state->error=QStringLiteral("Driver download exceeds the size limit");reply->abort();
        }
    });
    const auto consume=[reply,state,limits]{
        if(!state->error.isEmpty())return;
        const auto bytes=reply->read(limits.bytes-state->bytes.size()+1);
        if(state->bytes.size()+bytes.size()>limits.bytes){
            state->error=QStringLiteral("Driver download exceeds the size limit");state->bytes.clear();reply->abort();return;
        }
        state->bytes.append(bytes);
    };
    QObject::connect(reply,&QNetworkReply::readyRead,owner,consume);
    QObject::connect(reply,&QNetworkReply::finished,owner,[reply,state,deadline,consume,finished]{
        deadline->stop();if(reply->error()==QNetworkReply::NoError)consume();
        if(state->error.isEmpty()&&reply->error()!=QNetworkReply::NoError)state->error=reply->errorString();
        if(state->error.isEmpty()&&reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()!=200)
            state->error=QStringLiteral("Driver download returned an unexpected HTTP status");
        reply->deleteLater();
        finished(state->error.isEmpty()?std::move(state->bytes):QByteArray(),state->error);
    });
}
}
