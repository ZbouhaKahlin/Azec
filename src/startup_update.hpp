#pragma once
#include "ui_widgets.hpp"
#include "app_style.hpp"
#include "strings.hpp"
#include "update_check.hpp"
#include <windows.h>

// Runs before the Engine workers, phone server, and editable desktop exist.
class StartupUpdateWindow final : public QWidget {
public:
    enum class Phase {Checking,Resetting,Downloading,Restarting,Offline,Retry,Complete};
private:
    int language_;Phase phase_=Phase::Checking;QNetworkAccessManager* network_;
    QString target_,message_,previous_;qreal progress_=0,text_=1;bool glass_=false,motion_=true;
    QVariantAnimation progress_motion_,text_motion_;std::function<void()> after_progress_,ready_;
    std::function<bool(const QString&)> install_;QPushButton* retry_;QElapsedTimer checking_;
    azec_update::Release release_;int attempt_=0;bool required_=false;
    QString text(UiText id) const{return uiText(id,language_);}
    void label(UiText id){
        previous_=message_;message_=text(id);setAccessibleDescription(message_);
        if(motion_)retargetAnimation(text_motion_,0,1,180);else{text_=1;update();}
    }
    void progressTo(qreal value,int duration=180,std::function<void()> next={}){
        after_progress_=std::move(next);
        if(motion_)retargetAnimation(progress_motion_,progress_,value,duration);
        else{progress_=value;update();QTimer::singleShot(0,this,[this]{auto next=std::move(after_progress_);if(next)next();});}
    }
    void proceed(){phase_=Phase::Complete;progressTo(.25,180,[this]{if(ready_)ready_();});}
    void failed(){phase_=Phase::Retry;after_progress_={};progress_motion_.stop();label(UiText::StartupDownloadFailed);retry_->show();retry_->setFocus(Qt::OtherFocusReason);}
    void download(){
        phase_=Phase::Downloading;
        azec_update::downloadVerified(network_,this,release_.asset,release_.digest,target_,[this](bool ok){
            if(!ok){failed();return;}
            progressTo(1,240,[this]{
                phase_=Phase::Restarting;label(UiText::StartupRestarting);
                QTimer::singleShot(300,this,[this]{if(!install_||!install_(target_))failed();});
            });
        },[this](qint64 received,qint64 total){
            if(phase_!=Phase::Downloading||total<=0)return;
            const qreal target=qMin(qreal(.95),qreal(received)/total*.95);
            if(target>progress_)progressTo(target);
        });
    }
    void checked(const azec_update::Release& result){
        release_=result;
        if(required_&&(!result.checked||!result.required)){failed();return;}
        required_|=result.required;
        if(!result.checked){phase_=Phase::Offline;label(UiText::StartupCheckUnavailable);QTimer::singleShot(1200,this,[this]{proceed();});return;}
        if(!result.required){proceed();return;}
        if(!result.downloadable()||target_.isEmpty()){failed();return;}
        phase_=Phase::Resetting;label(UiText::StartupDownloading);
        progressTo(0,240,[this]{download();});
    }
public:
    StartupUpdateWindow(int language,const QString& target,std::function<void()> ready,std::function<bool(const QString&)> install,
                        QNetworkAccessManager* network=nullptr,QWidget* parent=nullptr)
        :QWidget(parent,Qt::Window|Qt::FramelessWindowHint),language_(language),network_(network?network:new QNetworkAccessManager(this)),
         target_(target),ready_(std::move(ready)),install_(std::move(install)){
        setObjectName("startupUpdate");setWindowTitle("Azec");setAccessibleName("Azec");setAttribute(Qt::WA_TranslucentBackground);
        setWindowIcon(QIcon(":/ui/app-icon.svg"));setLayoutDirection(language==5||language==6?Qt::RightToLeft:Qt::LeftToRight);
        const auto area=QGuiApplication::primaryScreen()->availableGeometry();resize(qMin(400,area.width()-32),qMin(232,area.height()-32));move(area.center()-rect().center());
        setStyleSheet(applicationStyle(kbtheme::defaultAccent));
        BOOL enabled=TRUE;SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,0,&enabled,0);motion_=enabled;
        retry_=new MonoButton(text(UiText::Retry));retry_->setParent(this);retry_->setObjectName("secondary");retry_->setGeometry(width()/2-45,height()-35,90,28);retry_->hide();
        connect(retry_,&QPushButton::clicked,this,[this]{start();});
        auto* close=new IconButton(":/ui/close.svg",[]{return kbtheme::defaultAccent;});close->setParent(this);close->setObjectName("closeControl");close->setAccessibleName(text(UiText::Close));close->setToolTip(text(UiText::Close));close->setGeometry(width()-38,6,32,32);
        connect(close,&QPushButton::clicked,this,&QWidget::close);
        progress_motion_.setEasingCurve(QEasingCurve::InOutCubic);text_motion_.setEasingCurve(monoEase());
        connect(&progress_motion_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){progress_=value.toReal();update();});
        connect(&progress_motion_,&QVariantAnimation::finished,this,[this]{auto next=std::move(after_progress_);if(next)next();});
        connect(&text_motion_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){text_=value.toReal();update();});
        message_=text(UiText::StartupCheckingUpdates);setAccessibleDescription(message_);
    }
    void start(){
        const int attempt=++attempt_;retry_->hide();phase_=Phase::Checking;label(UiText::StartupCheckingUpdates);checking_.start();progressTo(.25,800);
        azec_update::checkLatest(network_,this,QCoreApplication::applicationVersion(),[this,attempt](azec_update::Release result){
            const int wait=qMax(0,800-int(checking_.elapsed()));
            QTimer::singleShot(wait,this,[this,attempt,result]{if(attempt==attempt_)checked(result);});
        });
    }
    Phase phase() const{return phase_;}
    qreal progress() const{return progress_;}
    QString message() const{return message_;}
protected:
    void showEvent(QShowEvent* event) override{
        QWidget::showEvent(event);QPainterPath shape;shape.addRoundedRect(QRectF(rect()),24,24);const QRegion region(shape.toFillPolygon().toPolygon());setMask(region);
        struct Policy{DWORD state,flags,color,animation;};struct Data{DWORD attribute;PVOID data;SIZE_T size;};
        static const auto apply=reinterpret_cast<BOOL(WINAPI*)(HWND,Data*)>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"SetWindowCompositionAttribute"));
        if(apply){Policy policy{4,0,0x01000000,0};Data data{19,&policy,sizeof(policy)};glass_=apply(HWND(winId()),&data);}
        if(auto* native=windowHandle())native->setMask(region);
    }
    void paintEvent(QPaintEvent*) override{
        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);painter.setRenderHint(QPainter::SmoothPixmapTransform);
        QPainterPath shape;shape.addRoundedRect(QRectF(rect()).adjusted(.5,.5,-.5,-.5),24,24);
        painter.fillPath(shape,glass_?kbtheme::windowGlass:kbtheme::paper);if(glass_)painter.fillPath(shape,kbtheme::windowWash);
        BrandLabel::draw(painter,QRectF(width()/2.0-94,43,188,66),devicePixelRatioF());
        const QRectF bar(width()/2.0-124,height()*.64,248,3);painter.setPen(Qt::NoPen);painter.setBrush(kbtheme::content(.09));painter.drawRoundedRect(bar,1.5,1.5);
        auto fill=bar;fill.setWidth(bar.width()*progress_);painter.setBrush(kbtheme::defaultAccent);painter.drawRoundedRect(fill,1.5,1.5);
        QFont font("Segoe UI");font.setPixelSize(12);painter.setFont(font);painter.setPen(kbtheme::muted);
        const QRectF label(width()/2.0-174,bar.y()+17,348,28);
        if(text_<1&&!previous_.isEmpty()){painter.setOpacity(1-text_);painter.drawText(label.translated(0,-2*text_),Qt::AlignHCenter|Qt::AlignTop|Qt::TextWordWrap,previous_);}
        painter.setOpacity(text_);painter.drawText(label.translated(0,2*(1-text_)),Qt::AlignHCenter|Qt::AlignTop|Qt::TextWordWrap,message_);
    }
};
