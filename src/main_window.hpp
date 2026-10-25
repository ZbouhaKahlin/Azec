#pragma once
// Qt presentation; recovered input behavior stays in engine.cpp and core.cpp.
#include "engine.hpp"
#include "profile_store.hpp"
#include "ui_widgets.hpp"
#include "phone_server.hpp"
#include "app_style.hpp"
#include "pairing_qr.hpp"
#include "hotkeys.hpp"
#include "driver_input.hpp"
#include "driver_setup.hpp"
#include "driver_download.hpp"
#include "strings.hpp"
#include "flags.hpp"
#include "diagnostics.hpp"
#include "game_focus.hpp"
#include "sound.hpp"
#include "version.hpp"
#include "update_check.hpp"
#include "app_paths.hpp"
#include <windows.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSaveFile>
#include <QSet>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDesktopServices>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QOperatingSystemVersion>
#include <algorithm>
#include <cmath>

inline bool keyDown(int key){return key&&(GetAsyncKeyState(key)&0x8000);}
inline QString keyName(int key){
    if((key>='0'&&key<='9')||(key>='A'&&key<='Z'))return QString(QChar(key));
    switch(key){case 0:return "None";case 1:return "M1";case 2:return "M2";case 4:return "M3";case 5:return "M4";case 6:return "M5";
    case VK_LEFT:return "Left";case VK_RIGHT:return "Right";case VK_UP:return "Up";case VK_DOWN:return "Down";case VK_INSERT:return "Insert";case VK_END:return "End";}
    wchar_t name[128]{};UINT scan=MapVirtualKeyW(key,MAPVK_VK_TO_VSC);if(key==VK_DELETE||key==VK_BACK)scan|=0x100;
    if(GetKeyNameTextW(scan<<16,name,128))return QString::fromWCharArray(name);return QString::number(key);
}
inline void roundWindow(QWidget* window){
    const HWND handle=HWND(window->winId());const DWORD preference=2;
    DwmSetWindowAttribute(handle,33,&preference,sizeof(preference));
    // A region gives both Windows versions the same soft, 24 px outer corners.
    RECT rectangle{};GetWindowRect(handle,&rectangle);const int diameter=qRound(48*window->devicePixelRatioF());
    HRGN shape=CreateRoundRectRgn(0,0,rectangle.right-rectangle.left+1,rectangle.bottom-rectangle.top+1,diameter,diameter);
    if(shape&&!SetWindowRgn(handle,shape,TRUE))DeleteObject(shape);
}

class MainWindow final : public QWidget {
    r6::Engine& engine_;
    std::filesystem::path config_path_;
    void buildDesktopLayout(QVBoxLayout* layout);
    bool confirmProfileAction(const QString& title,const QString& body,const QString& action);
    QListWidget* operators_=nullptr;QListWidget* favorites_list_=nullptr;QLineEdit* search_=nullptr;
    QLabel *favorites_title_=nullptr,*preferences_title_=nullptr;QWidget* favorites_group_=nullptr;
    IconButton* back_=nullptr;QSet<QString> favorites_;bool imported_ui_pending_=false;
    PageStack* pages_=nullptr;QWidget* operator_page_=nullptr;
    RaisedPanel* panel_=nullptr;QScrollArea* sidebar_scroll_=nullptr;
    QWidget *sidebar_=nullptr,*favorites_header_=nullptr,*favorites_heading_=nullptr,*operators_header_=nullptr,*sidebar_search_box_=nullptr,*settings_nav_=nullptr;
    QVector<SettingsNavButton*> settings_nav_buttons_;bool settings_sidebar_=false;
    IconButton *sidebar_toggle_=nullptr,*sidebar_search_=nullptr;FavoriteStarButton* sidebar_favorites_=nullptr;
    QVariantAnimation sidebar_animation_;bool sidebar_collapsed_=false,focus_sidebar_search_=false,sidebar_animating_=false,favorites_shown_=false;
    QVector<QWidget*> preference_categories_;int preference_category_=0;
    bool glass_active_=false,glass_applied_=false,glass_lightweight_=false;
    QVariantAnimation theme_motion_;
    QPointer<PageSnapshot> theme_snapshot_;
    QVariantAnimation operator_transition_;
    FadeLabel *name_opacity_=nullptr,*icon_opacity_=nullptr;
    QLabel *operators_title_=nullptr,*operator_name_=nullptr,*operator_icon_=nullptr;
    MonoButton* status_=nullptr;MonoButton* rapid_status_=nullptr;IconButton *save_=nullptr,*close_=nullptr,*gear_=nullptr,*minimize_=nullptr;
    QWidget* preferences_=nullptr;QScrollArea* preferences_scroll_=nullptr;
    QWidget* profile_save_dock_=nullptr;QLabel* profile_save_hint_=nullptr;QPushButton* profile_save_button_=nullptr;
    QScrollArea* scroll_=nullptr;QWidget* settings_=nullptr;QVBoxLayout* cards_=nullptr;
    Section *system_=nullptr;
    Toggle *rapid_active_=nullptr,*game_focus_only_=nullptr,*ads_only_=nullptr,*humanize_=nullptr,*dual_=nullptr;
    WeaponButton *primary_=nullptr,*secondary_=nullptr;
    PhoneServer* phone_=nullptr;
    QPointer<PairingQr> pairing_qr_;
    QPointer<QPushButton> driver_test_;
    QPointer<QPushButton> driver_check_;
    QPointer<QPushButton> driver_download_;
    QPointer<QPushButton> driver_page_;
    QPointer<QPushButton> restart_admin_;
    QPointer<CodeBlock> driver_code_;
    QString driver_installer_;
    QPointer<QLabel> driver_status_;
    QPointer<QLabel> phone_address_;
    QPointer<QLabel> phone_code_;
    QPointer<ExpandingContent> phone_details_;
    QPointer<Toggle> phone_enabled_;
    QPointer<MonoButton> copy_link_,copy_code_;
    QPointer<QPushButton> phone_new_code_,phone_retry_;
    QPointer<QPushButton> update_button_;QNetworkAccessManager* network_=nullptr;bool update_in_progress_=false;
    QWidget* notice_bar_=nullptr;QLabel *notice_=nullptr,*notice_title_=nullptr,*notice_badge_=nullptr;QPushButton* notice_close_=nullptr;QScrollArea* notice_scroll_=nullptr;
    QPointer<QComboBox> theme_choice_;
    QLabel* brand_logo_=nullptr;
    QSystemTrayIcon* tray_=nullptr;
    QMenu* tray_menu_=nullptr;
    QAction *tray_window_=nullptr,*tray_recoil_=nullptr,*tray_rapid_=nullptr,*tray_exit_=nullptr;
    QDialog* close_alert_=nullptr;
    bool unsaved_=false,force_close_=false,close_prompt_=false,save_blocked_=false,close_recoil_changed_=false;
    QPoint last_cursor_{-10000,-10000};
    QDoubleSpinBox *vertical_=nullptr,*horizontal_=nullptr;QSpinBox* delay_=nullptr;
    QSlider *vertical_slider_=nullptr,*horizontal_slider_=nullptr,*delay_slider_=nullptr;QWidget* delay_controls_=nullptr;StepButton *delay_minus_=nullptr,*delay_plus_=nullptr;QComboBox* language_=nullptr;QPointer<QDialog> palestine_popup_;
    std::array<QPushButton*,5> key_buttons_{};
    QTimer poll_,paint_,save_feedback_,notice_timer_;QElapsedTimer clock_;
    bool was_menu_=false,was_recoil_=false,was_primary_=false,was_secondary_=false,refreshing_=false,native_ready_=false;
    int binding_=0,release_key_=0,paint_count_=0,cursor_repaints_=0;qint64 blocked_until_=0,last_tune_=0;
    QStringList icon_failures_;
    // Counts cursor moves without repainting: through the acrylic glass every
    // full-window repaint forces the system to recompose the blur, so motion
    // must stay paint-free (hover feedback is handled by the controls themselves).
    void trackCursor(const QPoint& current){if(current!=last_cursor_){last_cursor_=current;++cursor_repaints_;}}
    QColor accent() const {return kbtheme::defaultAccent;}
    QString text(UiText id) const{return uiText(id,engine_.snapshot().language);}
    QString bindingName(int key) const{
        switch(key){case 0:return text(UiText::NoneKey);case VK_LEFT:return text(UiText::LeftKey);case VK_RIGHT:return text(UiText::RightKey);case VK_UP:return text(UiText::UpKey);case VK_DOWN:return text(UiText::DownKey);default:return keyName(key);}
    }
    static QJsonObject profileJson(const r6::Operator& op){return {{"vertical",std::round(op.vertical*10.0)/10.0},{"horizontal",std::round(op.horizontal*10.0)/10.0},{"humanize",bool(op.humanize)},{"rapid",bool(op.rapid)},{"delay",op.delay}};}
    QJsonObject mobileState() const;
    QJsonObject mobileUpdate(const QJsonObject& request);
    void exportProfiles();
    void importProfiles();
    bool applyBackup(const QByteArray& bytes);
    void refreshTray(){
        if(!tray_)return;
        tray_window_->setText(text(UiText::OpenHideWindow));
        tray_recoil_->setText(text(UiText::RecoilActive));tray_recoil_->setChecked(engine_.recoil_active);
        tray_rapid_->setText(text(UiText::RapidFire));tray_rapid_->setChecked(engine_.rapid_active());
        tray_exit_->setText(text(UiText::Exit));tray_->setToolTip("Azec");
    }
    void buildTray(){
        tray_=new QSystemTrayIcon(QIcon(":/ui/app-icon.svg"),this);tray_menu_=new QMenu(this);
        tray_window_=tray_menu_->addAction(QString());tray_recoil_=tray_menu_->addAction(QString());tray_recoil_->setCheckable(true);
        tray_rapid_=tray_menu_->addAction(QString());tray_rapid_->setCheckable(true);
        tray_menu_->addSeparator();tray_exit_=tray_menu_->addAction(QString());
        tray_->setContextMenu(tray_menu_);refreshTray();
        connect(tray_menu_,&QMenu::aboutToShow,this,[this]{refreshTray();});
        connect(tray_window_,&QAction::triggered,this,[this]{if(isVisible()&&!isMinimized())hide();else{showNormal();raise();activateWindow();}});
        connect(tray_recoil_,&QAction::triggered,this,[this]{toggleRecoil();});
        connect(tray_rapid_,&QAction::triggered,this,[this]{toggleRapid();});
        connect(tray_exit_,&QAction::triggered,this,[this]{close();});
        connect(tray_,&QSystemTrayIcon::activated,this,[this](QSystemTrayIcon::ActivationReason reason){if(reason==QSystemTrayIcon::Trigger||reason==QSystemTrayIcon::DoubleClick){showNormal();raise();activateWindow();}});
        if(!engine_.dry_run()&&QSystemTrayIcon::isSystemTrayAvailable())tray_->show();
    }
    void updateSavePresentation(const QString& state,bool animate=false){
        if(state=="saved")unsaved_=false;
        save_->setProperty("unsaved",unsaved_);
        save_->setSaveState(state,animate);
        save_->setEnabled(!save_blocked_);
        save_->setToolTip(state=="dirty"?text(UiText::SaveConfigurationUnsavedChanges):state=="saved"?text(UiText::ConfigurationSaved):state=="error"?text(UiText::CouldNotSaveConfiguration):text(UiText::SaveConfiguration));
        save_->update();
        if(profile_save_button_){
            profile_save_hint_->setText(text(state=="error"?UiText::CouldNotSaveConfiguration:unsaved_?UiText::UnsavedChanges:UiText::ConfigurationSaved));
            profile_save_button_->setText(text(UiText::SaveChanges));profile_save_button_->setEnabled(unsaved_&&!save_blocked_);
        }
    }
    void dirty(){if(!refreshing_){unsaved_=true;save_feedback_.stop();updateSavePresentation("dirty");}}
    void editOperator(const std::function<void(r6::Operator&)>& change){engine_.edit([&](r6::Config& c){change(c.active_operator());});dirty();}
    std::filesystem::path favoritesPath() const{return config_path_.parent_path()/L"azec_ui.json";}
    QSet<QString> readFavorites() const {
        QFile file(QString::fromStdWString(favoritesPath().wstring()));QSet<QString> result;
        if(!file.open(QIODevice::ReadOnly))return result;
        if(file.size()>1024*1024){diagnostic("load favorites","File too large; ignored");return result;}
        QJsonParseError error;const auto document=QJsonDocument::fromJson(file.readAll(),&error);const auto root=document.object();
        if(error.error!=QJsonParseError::NoError||!document.isObject()||(root.contains("version")&&root["version"].toInt()!=1)){diagnostic("load favorites","Invalid or unsupported file; ignored");return result;}
        const auto array=root["favorites"].toArray();const auto c=engine_.snapshot();
        for(const auto& entry:array)for(const auto& op:c.operators)if(entry.toString()==QString::fromUtf8(op.name))result.insert(entry.toString());
        return result;
    }
    bool persistFavorites() const {
        if(imported_ui_pending_)return true;
        QStringList names(favorites_.begin(),favorites_.end());names.sort(Qt::CaseInsensitive);
        return uiprefs::update(config_path_,[&](QJsonObject& root){root["favorites"]=QJsonArray::fromStringList(names);});
    }

    void filterOperators(){

        const auto term=search_->text();int shown=0;
        for(auto* list:{operators_,favorites_list_}){int count=0;for(int row=0;row<list->count();++row){auto* item=list->item(row);const bool match=item->text().contains(term,Qt::CaseInsensitive)&&(list==favorites_list_||!favorites_.contains(item->text()));item->setHidden(!match);if(match)++count;}list->setFixedHeight(count*operatorRowHeight);if(list==favorites_list_)shown=count;}
        favorites_shown_=shown>0;
        favorites_group_->setVisible(favorites_shown_);favorites_list_->setVisible(favorites_shown_);
        updateSidebarControls();

    }

    void refreshFavorites(){
        QSignalBlocker blocked(favorites_list_);favorites_list_->clear();const auto c=engine_.snapshot();
        for(int i=0;i<operators_->count();++i){auto* original=operators_->item(i);const bool favorite=favorites_.contains(original->text());original->setData(Qt::UserRole+1,favorite);
            if(favorite){auto* item=new QListWidgetItem(original->icon(),original->text());item->setToolTip(original->text());item->setData(Qt::UserRole,i);item->setData(Qt::UserRole+1,true);favorites_list_->addItem(item);if(i==c.selected)favorites_list_->setCurrentItem(item);}}
        operators_->viewport()->update();filterOperators();updateSidebarControls();syncSidebarSelection();
    }
    void setFavorite(int id,bool enabled){
        if(id<0||id>=r6::operator_count)return;const auto name=QString::fromUtf8(engine_.snapshot().operators[id].name);const auto previous=favorites_;
        if(enabled)favorites_.insert(name);else favorites_.remove(name);
        if(!persistFavorites()){favorites_=previous;showNotice(text(UiText::CouldNotSaveFavorites),true,true);}
        refreshFavorites();
    }
    void showOperatorMenu(QListWidget* list,const QPoint& position){
        auto* item=list->itemAt(position);if(!item)return;const int id=item->data(Qt::UserRole).toInt();const bool favorite=favorites_.contains(item->text());
        FavoritePopup popup(favorite?text(UiText::RemoveFavorite):text(UiText::Favorite),this);
        if(popup.execAt(list->viewport()->mapToGlobal(position)))setFavorite(id,!favorite);
    }
    void stopTransition(){operator_transition_.stop();name_opacity_->setOpacity(1);icon_opacity_->setOpacity(1);}
    void showOperators(){if(pairing_qr_)pairing_qr_->conceal();stopTransition();settings_sidebar_=false;updateSidebarControls();pages_->switchTo(operator_page_);gear_->setProperty("activePage",false);gear_->style()->unpolish(gear_);gear_->style()->polish(gear_);}
    void syncSidebarSelection(){
        const int id=engine_.snapshot().selected;
        {QSignalBlocker blocked(operators_);operators_->setCurrentRow(id);}
        {QSignalBlocker blocked(favorites_list_);favorites_list_->setCurrentRow(-1);for(int i=0;i<favorites_list_->count();++i)if(favorites_list_->item(i)->data(Qt::UserRole).toInt()==id){favorites_list_->setCurrentRow(i);break;}}
    }
    void selectOperator(int id){
        if(id<0||id>=r6::operator_count)return;const bool changed=engine_.snapshot().selected!=id;
        if(!changed&&pages_->currentWidget()==operator_page_)return;
        const bool animate=changed&&operator_page_->isVisible();stopTransition();
        engine_.edit([=](r6::Config& c){c.selected=id;if(changed)c.weapon_slot=0;});
        if(changed&&!persistSelection(config_path_,id))diagnostic("save selection","Could not update the selected operator");
        syncSelection();syncSidebarSelection();
        if(pages_->currentWidget()!=operator_page_)showOperators();
        if(animate){name_opacity_->setOpacity(.35);icon_opacity_->setOpacity(.35);operator_transition_.start();}
    }
    QLabel* label(const QString& text,const char* role="rowTitle"){
        const bool description=qstrcmp(role,"rowHint")==0||qstrcmp(role,"hint")==0||qstrcmp(role,"subtitle")==0;
        QLabel* result=description?static_cast<QLabel*>(new MonoDescription(text)):new QLabel(text);
        result->setObjectName(role);return result;
    }
    QHBoxLayout* settingRow(Section* section,const QString& title,const QString& description={},int height=44){
        auto* widget=new QWidget;widget->setObjectName("settingRow");widget->setMinimumHeight(description.isEmpty()?height:56);auto* row=new QHBoxLayout(widget);const bool open=section->property("openProfile").toBool()||section->property("openSettings").toBool();row->setContentsMargins(open?0:16,open?8:12,open?0:16,open?8:12);row->setSpacing(12);
        auto* text=new QWidget;auto* column=new QVBoxLayout(text);column->setContentsMargins(0,0,0,0);column->setSpacing(4);auto* titleLabel=label(title);titleLabel->setWordWrap(true);column->addWidget(titleLabel);
        if(!description.isEmpty()){auto* hint=label(description,"rowHint");hint->setWordWrap(true);column->addWidget(hint);widget->setToolTip(description);}
        row->addWidget(text,1);section->body->addWidget(widget);return row;
    }
    Toggle* toggle(QHBoxLayout* row,const QString& title,bool checked,const std::function<void(bool)>& callback){
        auto* result=new Toggle([this]{return accent();});const auto* rowLabel=row->parentWidget()->findChild<QLabel*>();result->setAccessibleName(rowLabel?rowLabel->text():title);result->setChecked(checked);row->addWidget(result,0,Qt::AlignVCenter);
        connect(result,&QAbstractButton::toggled,this,[this,callback](bool on){if(!refreshing_){if(engine_.snapshot().sound)sound::playToggle();callback(on);}});return result;
    }
    ResetSlider* makeSlider(){return new ResetSlider(Qt::Horizontal);}
    QWidget* amount(const QString& title,bool vertical){
        auto* slider=makeSlider();slider->setRange(vertical?0:-500,vertical?1000:500);slider->setSingleStep(1);slider->setPageStep(5);slider->setAccessibleName(title);
        // A double-click returns the value to the Operator defaults (10.0 / 0.0).
        slider->setResetValue(vertical?100:0);
        auto* value=new QDoubleSpinBox;value->setDecimals(1);value->setSingleStep(.1);value->setRange(vertical?0:-50,vertical?100:50);value->setButtonSymbols(QAbstractSpinBox::NoButtons);value->setAccessibleName(title+text(UiText::Value));
        auto* field=new QWidget;field->setObjectName("recoilAxis");auto* column=new QVBoxLayout(field);column->setContentsMargins(0,0,0,0);column->setSpacing(4);
        auto* titleLabel=label(title);titleLabel->setWordWrap(true);titleLabel->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);titleLabel->setBuddy(value);column->addWidget(titleLabel);
        value->setObjectName("recoilValue");value->setFixedWidth(110);value->setAlignment(Qt::AlignLeading);column->addWidget(value,0,Qt::AlignLeading);
        auto* row=new QHBoxLayout;row->setContentsMargins(0,0,0,0);row->setSpacing(6);column->addLayout(row);
        // One step is 0.1; the arrows flank the slider and holding repeats.
        const auto nudge=[slider](int direction){slider->setValue(std::clamp(slider->value()+direction,slider->minimum(),slider->maximum()));};
        auto* minus=new StepButton(QString::fromUtf8("−"),-1,nudge);minus->setAccessibleName(title+QStringLiteral(" -0.1"));minus->setToolTip(QStringLiteral("-0.1"));
        auto* plus=new StepButton("+",1,nudge);plus->setAccessibleName(title+QStringLiteral(" +0.1"));plus->setToolTip(QStringLiteral("+0.1"));
        row->addWidget(minus);row->addWidget(slider,1);row->addWidget(plus);
        if(vertical){vertical_=value;vertical_slider_=slider;}else{horizontal_=value;horizontal_slider_=slider;}
        const auto syncSteps=[=]{minus->setEnabled(slider->value()>slider->minimum());plus->setEnabled(slider->value()<slider->maximum());};
        connect(slider,&QSlider::valueChanged,value,[value,syncSteps](int n){value->setValue(n/10.0);syncSteps();});
        syncSteps();
        connect(value,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[=](double n){QSignalBlocker blocked(slider);slider->setValue(qRound(n*10));syncSteps();if(!refreshing_)editOperator([=](r6::Operator& op){if(vertical)op.vertical=float(n);else op.horizontal=float(n);});});
        return field;
    }
    void styleNotice(){
        if(!notice_bar_)return;
        const bool error=notice_bar_->property("error").toBool();
        const QColor badge=error?QColor(244,63,94,38):QColor(69,155,247,32);
        const QColor badgeInk=error?kbtheme::danger:kbtheme::purple;
        notice_title_->setText(text(error?UiText::NoticeErrorTitle:UiText::NoticeTitle));notice_close_->setText(text(UiText::Understood));
        notice_badge_->setText(error?QStringLiteral("!"):QStringLiteral("i"));
        notice_bar_->setStyleSheet(QString("QWidget#noticeBar{background:%1;border:1px solid %2;border-radius:14px;}"
            "QLabel#noticeBadge{background:%3;color:%4;border:0;border-radius:17px;font-size:17px;font-weight:600;}"
            "QLabel#startupNotice{color:%5;font-size:13px;font-weight:400;}")
            .arg(kbtheme::css(kbtheme::dialog),kbtheme::css(kbtheme::outline),kbtheme::css(badge),badgeInk.name(),kbtheme::ink.name()));
        if(auto* shadow=qobject_cast<QGraphicsDropShadowEffect*>(notice_bar_->graphicsEffect())){shadow->setBlurRadius(28);shadow->setOffset(0,6);shadow->setColor(QColor(0,0,0,110));}
    }
    void applyStyle(){
        pages_->settle();qApp->setPalette(kbtheme::palette());setStyleSheet(applicationStyle(accent()));setPalette(kbtheme::palette());
        if(brand_logo_){
            QPixmap mark=QIcon(":/ui/logo.svg").pixmap(QSize(32,32),devicePixelRatioF());
            QPainter tint(&mark);tint.setCompositionMode(QPainter::CompositionMode_SourceIn);tint.fillRect(mark.rect(),kbtheme::ink);tint.end();
            brand_logo_->setPixmap(mark);
        }
        styleNotice();
        layoutNotice();
        for(auto* child:findChildren<QWidget*>())child->update();update();
    }
    void setLightTheme(bool light){
        if(light==kbtheme::light)return;
        if(!uiprefs::update(config_path_,[=](QJsonObject& root){root["theme"]=light?"light":"dark";})){
            if(theme_choice_){QSignalBlocker blocked(theme_choice_.data());theme_choice_->setCurrentIndex(kbtheme::light?1:0);}
            showNotice(text(UiText::CouldNotSaveTheme),true,true);return;
        }
        pages_->settle();
        const QPixmap previous=isVisible()?(theme_snapshot_?theme_snapshot_->frame():grab()):QPixmap();
        settleThemeTransition();
        setUpdatesEnabled(false);
        kbtheme::initialize(light);
        const BOOL dark=!light;DwmSetWindowAttribute(HWND(winId()),20,&dark,sizeof(dark));
        applyStyle();
        if(layout())layout()->activate();
        setUpdatesEnabled(true);
        if(!previous.isNull()){
            // Capture each foreground once; never repolish every animation frame.
            const QPixmap next=grab();
            theme_snapshot_=new PageSnapshot(previous,next,this,true);theme_snapshot_->setGeometry(rect());theme_snapshot_->show();theme_snapshot_->raise();
            theme_motion_.start();
        }
    }
    void settleThemeTransition(){
        theme_motion_.stop();delete theme_snapshot_.data();
    }
    void chooseSettingsCategory(int index){
        if(index<0||index>=preference_categories_.size())return;
        if(pairing_qr_&&index!=2)pairing_qr_->conceal();
        const bool animate=pages_->currentWidget()==preferences_&&index!=preference_category_;
        const auto change=[this,index]{
            preference_category_=index;
            for(int i=0;i<preference_categories_.size();++i)preference_categories_[i]->setVisible(i==index);
            for(int i=0;i<settings_nav_buttons_.size();++i)settings_nav_buttons_[i]->setChecked(i==index);
            if(profile_save_dock_)profile_save_dock_->setVisible(index==3);
            static_cast<SmoothScrollArea*>(preferences_scroll_)->stopScrolling();
            if(auto* content=preferences_scroll_->widget();content&&content->layout())content->layout()->activate();
            preferences_scroll_->verticalScrollBar()->setValue(0);
        };
        if(animate)pages_->transition(change);else{pages_->settle();change();}
    }
    static void setWidgetOpacity(QWidget* widget,qreal opacity){
        if(!widget)return;
        auto* effect=qobject_cast<QGraphicsOpacityEffect*>(widget->graphicsEffect());
        if(!effect){effect=new QGraphicsOpacityEffect(widget);widget->setGraphicsEffect(effect);}
        effect->setOpacity(std::clamp(opacity,0.0,1.0));
    }
    void updateSidebarFades(){
        if(!sidebar_||settings_sidebar_)return;
        const qreal expanded=std::clamp((sidebar_->width()-88.0)/72.0,0.0,1.0);
        setWidgetOpacity(favorites_heading_,expanded);
        setWidgetOpacity(operators_header_,expanded);
        setWidgetOpacity(sidebar_favorites_,1.0-expanded);
    }
    void updateSettingsNavLabels(){
        const QString names[]={text(UiText::General),text(UiText::Shortcuts),text(UiText::PhoneControl),text(UiText::ProfileBackup)};
        const QString hints[]={text(UiText::SettingsGeneralHint),text(UiText::SettingsShortcutsHint),text(UiText::SettingsPhoneHint),text(UiText::SettingsBackupHint)};
        for(int i=0;i<settings_nav_buttons_.size()&&i<int(std::size(names));++i){settings_nav_buttons_[i]->setText(names[i]);settings_nav_buttons_[i]->setDescription(hints[i]);settings_nav_buttons_[i]->setToolTip(names[i]+QStringLiteral(" — ")+hints[i]);settings_nav_buttons_[i]->setAccessibleName(names[i]+QStringLiteral(". ")+hints[i]);}
    }
    void updateSidebarControls(){
        if(!sidebar_)return;
        const bool compact=sidebar_collapsed_;
        const bool labelsVisible=sidebar_animating_||(!compact&&sidebar_->width()>=170);
        if(settings_sidebar_){
            search_->setVisible(false);sidebar_search_->setVisible(false);sidebar_search_box_->setVisible(false);
            sidebar_scroll_->setVisible(false);if(sidebar_favorites_)sidebar_favorites_->setVisible(false);
            settings_nav_->setVisible(true);
        }else{
            settings_nav_->setVisible(false);
            sidebar_scroll_->setVisible(true);sidebar_search_box_->setVisible(true);
            search_->setVisible(!compact);sidebar_search_->setVisible(compact);
            if(sidebar_favorites_){sidebar_favorites_->setVisible(favorites_shown_&&(compact||sidebar_animating_));sidebar_favorites_->setToolTip(text(UiText::Favorites));sidebar_favorites_->setAccessibleName(text(UiText::Favorites));}
            favorites_header_->setVisible(true);favorites_heading_->setVisible(labelsVisible);
            // Both group headers retain their height, including while their
            // labels fade, so collapsing never shifts either list vertically.
            operators_header_->setVisible(true);operators_title_->setVisible(labelsVisible);
        }
        const QString title=text(compact?UiText::ExpandSidebar:UiText::CollapseSidebar);
        sidebar_toggle_->setToolTip(title);sidebar_toggle_->setAccessibleName(title);sidebar_toggle_->setChecked(compact);sidebar_toggle_->setText(text(UiText::Collapse));
        sidebar_search_->setToolTip(text(UiText::SearchOperators));sidebar_search_->setAccessibleName(text(UiText::SearchOperators));
        updateSidebarFades();
    }
    void setSidebarCollapsed(bool collapsed,bool animate=true,bool focusSearch=false){
        if(!sidebar_)return;
        if(!imported_ui_pending_&&collapsed!=sidebar_collapsed_&&!uiprefs::update(config_path_,[=](QJsonObject& root){root["sidebarCollapsed"]=collapsed;})){
            updateSidebarControls();showNotice(text(UiText::SidebarSaveFailed),true,true);return;
        }
        sidebar_collapsed_=collapsed;focus_sidebar_search_=focusSearch&&!collapsed;
        const int target=collapsed?76:200;sidebar_animating_=animate&&sidebar_->width()!=target;updateSidebarControls();
        if(!animate||sidebar_->width()==target){sidebar_animation_.stop();sidebar_animating_=false;sidebar_->setFixedWidth(target);updateSidebarControls();if(focus_sidebar_search_){search_->setFocus(Qt::ShortcutFocusReason);focus_sidebar_search_=false;}return;}
        retargetAnimation(sidebar_animation_,sidebar_->width(),target,180);
    }
    void buildSettings(){
        refreshing_=true;const auto c=engine_.snapshot();const auto hint=[this](UiText id){return text(id);};if(settings_){scroll_->takeWidget();delete settings_;}
        settings_=new QWidget;settings_->setObjectName("settingsContent");cards_=new QVBoxLayout(settings_);cards_->setContentsMargins(2,0,10,12);cards_->setSpacing(24);
        auto* weaponToolbar=new QWidget;weaponToolbar->setObjectName("weaponToolbar");weaponToolbar->setToolTip(hint(UiText::Use1And2ToSwitchWeaponProfilesChangeTheKeysInSettings));
        auto* weaponRow=new QHBoxLayout(weaponToolbar);weaponRow->setContentsMargins(0,0,0,0);weaponRow->setSpacing(20);cards_->addWidget(weaponToolbar);
        auto* weaponChoices=new WeaponTabs;weaponChoices->setObjectName("weaponChoices");weaponChoices->setAttribute(Qt::WA_StyledBackground);weaponChoices->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Fixed);
        auto* choices=new QHBoxLayout(weaponChoices);choices->setContentsMargins(0,0,0,0);choices->setSpacing(4);
        primary_=new WeaponButton([this]{return accent();});secondary_=new WeaponButton([this]{return accent();});for(auto* button:{primary_,secondary_})choices->addWidget(button);
        weaponRow->addWidget(weaponChoices);weaponRow->addStretch(1);
        auto* secondaryOption=new QWidget;auto* secondaryRow=new QHBoxLayout(secondaryOption);secondaryRow->setContentsMargins(0,0,0,0);secondaryRow->setSpacing(6);
        auto* secondaryLabel=new QLabel(text(UiText::SeparateSecondaryProfile));secondaryLabel->setObjectName("rowHint");secondaryLabel->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Preferred);secondaryRow->addWidget(secondaryLabel,1);
        dual_=toggle(secondaryRow,text(UiText::SeparateSecondaryProfile),c.secondary_enabled[c.selected],[this](bool on){engine_.edit([&](r6::Config& c){c.enable_secondary(c.selected,on);c.weapon_slot=on?1:0;});dirty();syncSelection();});
        dual_->setAccessibleDescription(weaponToolbar->toolTip());secondaryLabel->setBuddy(dual_);weaponRow->addWidget(secondaryOption);
        QWidget::setTabOrder(primary_,secondary_);QWidget::setTabOrder(secondary_,dual_);
        connect(primary_,&QPushButton::clicked,this,[this]{chooseWeapon(0);});connect(secondary_,&QPushButton::clicked,this,[this]{chooseWeapon(1);});
        auto* recoil=new Section(text(UiText::RecoilControl));recoil->setOpenProfile(true);cards_->addWidget(recoil);
        auto* axes=new QWidget;axes->setObjectName("recoilAxes");auto* axisColumns=new QHBoxLayout(axes);axisColumns->setContentsMargins(0,0,0,8);axisColumns->setSpacing(32);
        axisColumns->addWidget(amount(text(UiText::VerticalForce),true),1);axisColumns->addWidget(amount(text(UiText::HorizontalAdjust),false),1);recoil->body->addWidget(axes);
        humanize_=toggle(settingRow(recoil,text(UiText::HumanizeMovement),hint(UiText::VaryMovementSlightlyOnBothAxes)),"Humanize movement",false,[this](bool on){editOperator([=](r6::Operator& op){op.humanize=on;});});
        auto* rapid=new Section(text(UiText::RapidFire));rapid->setOpenProfile(true);cards_->addWidget(rapid);
        rapid_active_=toggle(settingRow(rapid,text(UiText::RapidFire),hint(UiText::RapidFireGlobalHint)),"Rapid fire",c.active_operator().rapid!=0,[this](bool on){editOperator([=](r6::Operator& op){op.rapid=on;});updateStatus();});
        delay_=new QSpinBox;delay_->setObjectName("delayValue");delay_->setRange(30,150);delay_->setSuffix(text(UiText::Ms));delay_->setButtonSymbols(QAbstractSpinBox::NoButtons);delay_->setReadOnly(true);delay_->setFocusPolicy(Qt::NoFocus);delay_->setAlignment(Qt::AlignTrailing);delay_->setFixedWidth(75);delay_->setAccessibleName(text(UiText::ShotDelay));
        delay_controls_=new QWidget;delay_controls_->setToolTip(hint(UiText::DelayBetweenRepeatedShots));auto* delayControls=new QHBoxLayout(delay_controls_);delayControls->setContentsMargins(0,8,0,8);delayControls->setSpacing(8);
        auto* delayLabel=label(text(UiText::ShotDelay));delayLabel->setWordWrap(true);delayControls->addWidget(delayLabel);
        delay_slider_=makeSlider();delay_slider_->setObjectName("shotDelaySlider");delay_slider_->setRange(30,150);delay_slider_->setSingleStep(1);delay_slider_->setPageStep(5);delay_slider_->setAccessibleName(text(UiText::ShotDelay));
        static_cast<ResetSlider*>(delay_slider_)->setResetValue(50);
        const auto nudgeDelay=[this](int direction){delay_slider_->setValue(std::clamp(delay_slider_->value()+direction*5,30,150));};
        delay_minus_=new StepButton(QString::fromUtf8("−"),-1,nudgeDelay);delay_plus_=new StepButton("+",1,nudgeDelay);
        delay_minus_->setAccessibleName(text(UiText::ShotDelay)+QStringLiteral(" -5")+text(UiText::Ms));delay_plus_->setAccessibleName(text(UiText::ShotDelay)+QStringLiteral(" +5")+text(UiText::Ms));
        delay_minus_->setToolTip(QStringLiteral("-5")+text(UiText::Ms));delay_plus_->setToolTip(QStringLiteral("+5")+text(UiText::Ms));
        delayControls->addWidget(delay_minus_);delayControls->addWidget(delay_slider_,1);delayControls->addWidget(delay_plus_);delayControls->addWidget(delay_);rapid->body->addWidget(delay_controls_);
        connect(delay_slider_,&QSlider::valueChanged,delay_,&QSpinBox::setValue);
        connect(delay_,qOverload<int>(&QSpinBox::valueChanged),this,[this](int n){QSignalBlocker blocked(delay_slider_);delay_slider_->setValue(n);syncDelayControls();if(!refreshing_)editOperator([=](r6::Operator& op){op.delay=n;});});

        cards_->addStretch();scroll_->setWidget(settings_);refreshing_=false;syncSelection();
    }
    void buildOutputMethod(QVBoxLayout* column){
        auto* output=new Section(text(UiText::DriverOutput));output->setOpenSettings(true);
        output->body->setSpacing(14);column->addWidget(output);
        driver_status_=new QLabel(QString());driver_status_->setObjectName("driverState");
        driver_status_->setAlignment(Qt::AlignLeading|Qt::AlignVCenter);
        output->headerRow->addWidget(driver_status_,0,Qt::AlignVCenter);

        // One visible walkthrough: every action belongs to the step it explains.
        auto* guide=new QFrame;guide->setObjectName("driverGuide");
        auto* steps=new QVBoxLayout(guide);steps->setContentsMargins(0,6,0,4);steps->setSpacing(24);
        const auto step=[&](int number,UiText title,UiText description){
            auto* row=new QHBoxLayout;row->setContentsMargins(0,0,0,0);row->setSpacing(12);
            auto* chip=new QLabel(QString::number(number));chip->setObjectName("driverStepNumber");
            chip->setAlignment(Qt::AlignCenter);chip->setFixedSize(18,22);row->addWidget(chip,0,Qt::AlignTop);
            auto* content=new QVBoxLayout;content->setContentsMargins(0,0,0,0);content->setSpacing(9);
            auto* heading=label(text(title),"driverStepTitle");heading->setWordWrap(true);content->addWidget(heading);
            auto* explanation=label(text(description),"rowHint");explanation->setWordWrap(true);content->addWidget(explanation);
            row->addLayout(content,1);steps->addLayout(row);return content;
        };
        const auto button=[&](UiText title,const char* objectName="secondary"){
            auto* result=new MonoButton(text(title));result->setObjectName(objectName);
            result->setAccessibleName(text(title));result->setCursor(Qt::PointingHandCursor);return result;
        };

        auto* files=step(1,UiText::DriverStepFiles,UiText::DriverStepFilesBody);
        auto* downloads=new QHBoxLayout;downloads->setSpacing(10);
        driver_download_=button(UiText::DownloadAndPlace,"driverDownload");
        driver_page_=button(UiText::DriverDownload,"driverManualDownload");
        downloads->addWidget(driver_download_);downloads->addWidget(driver_page_);downloads->addStretch();
        files->addLayout(downloads);

        auto* install=step(2,UiText::DriverStepInstall,UiText::DriverStepInstallBody);
        driver_code_=new CodeBlock(text(UiText::CopyCommand),text(UiText::Copied));
        driver_code_->setCopyFeedback([this]{if(engine_.snapshot().sound)sound::playCopy();});
        driver_code_->layout()->setContentsMargins(0,8,0,8);
        install->addWidget(driver_code_);

        auto* admin=step(3,UiText::DriverStepAdmin,UiText::DriverStepAdminBody);
        restart_admin_=button(UiText::RestartAsAdmin);admin->addWidget(restart_admin_,0,Qt::AlignLeading);

        auto* verify=step(4,UiText::DriverStepTest,UiText::DriverStepTestBody);
        auto* checks=new QHBoxLayout;checks->setSpacing(10);
        driver_check_=button(UiText::CheckDriver);checks->addWidget(driver_check_);
        driver_test_=button(UiText::TestDriver);checks->addWidget(driver_test_);checks->addStretch();verify->addLayout(checks);

        auto* help=new QWidget;auto* helpLayout=new QVBoxLayout(help);helpLayout->setContentsMargins(0,0,0,0);helpLayout->setSpacing(12);
        auto* divider=new QFrame;divider->setObjectName("driverDivider");helpLayout->addWidget(divider);
        auto* troubleButton=button(UiText::DriverTroubleshooting,"driverTroubleshooting");
        troubleButton->setText(QString::fromUtf8("▸ ")+text(UiText::DriverTroubleshooting));
        troubleButton->setCheckable(true);helpLayout->addWidget(troubleButton,0,Qt::AlignLeading);
        auto* trouble=label(text(UiText::DriverTroubleshootingBody),"rowHint");trouble->setWordWrap(true);
        auto* troubleContent=new ExpandingContent(trouble,180);troubleContent->setExpanded(false,false);
        helpLayout->addWidget(troubleContent);connect(troubleButton,&QPushButton::toggled,troubleContent,[troubleContent](bool open){troubleContent->setExpanded(open);});
        connect(troubleButton,&QPushButton::toggled,this,[this,troubleButton](bool open){
            troubleButton->setText(QString::fromUtf8(open?"▾ ":"▸ ")+text(UiText::DriverTroubleshooting));
        });
        steps->addWidget(help);output->body->addWidget(guide);

        connect(driver_test_,&QPushButton::clicked,this,[this]{testDriver();});
        connect(driver_check_,&QPushButton::clicked,this,[this]{checkDriver();});
        connect(driver_download_,&QPushButton::clicked,this,[this]{downloadDriver();});
        connect(driver_page_,&QPushButton::clicked,this,[]{
            QDesktopServices::openUrl(QUrl(driver_setup::releasePage()));
        });
        connect(restart_admin_,&QPushButton::clicked,this,[this]{restartElevated();});
        resolveInstaller();refreshOutputStatus();
    }
    void testDriver(){
        if(engine_.dry_run()){showNotice(text(UiText::DriverTestFailed),true);return;}
        // A visible 40 px sweep proves the driver path without the game.
        if(driver_input::test())showNotice(text(UiText::DriverTestSent));
        else{diagnostic("driver test","interception_send rejected the test move");showNotice(text(UiText::DriverTestFailed),true,true);}
    }
    // The Interception device only answers an elevated caller, so a refused
    // context offers a one-click relaunch that keeps the current arguments.
    // Pending edits are only written when the prompt's Save is chosen, so a
    // cancelled restart never commits them.
    void restartElevated(){
        if(unsaved_){
            const int choice=unsavedChoice(text(UiText::SaveYourChangesBeforeClosing));
            if(choice==0)return;
            if(choice==2&&!save(true))return;
        }
        QStringList arguments=QCoreApplication::arguments();arguments.removeFirst();
        std::wstring parameters;
        for(const auto& argument:arguments){
            if(!parameters.empty())parameters+=L' ';
            parameters+=L'"'+argument.toStdWString()+L'"';
        }
        const auto executable=QCoreApplication::applicationFilePath().toStdWString();
        SHELLEXECUTEINFOW launch{};launch.cbSize=sizeof(launch);launch.fMask=SEE_MASK_FLAG_NO_UI;
        launch.lpVerb=L"runas";launch.lpFile=executable.c_str();
        launch.lpParameters=parameters.empty()?nullptr:parameters.c_str();launch.nShow=SW_SHOWNORMAL;
        if(!ShellExecuteExW(&launch)){
            const auto error=GetLastError();
            if(error==ERROR_CANCELLED)showNotice(text(UiText::AdministratorRestartCancelled));
            else{diagnostic("restart as administrator","Windows error "+QString::number(error));showNotice(text(UiText::AdministratorRestartFailed),true,true);}
            return;
        }
        force_close_=true;close();
    }
    // Finds an extracted Interception folder anywhere obvious and keeps the
    // display command pointing at its installer.
    void resolveInstaller(){
        const auto found=engine_.dry_run()?QString():driver_setup::locate();
        driver_installer_=driver_setup::installerFolder(found);
        if(driver_code_){
            driver_code_->setText(driver_installer_.isEmpty()?text(UiText::DriverStepFilesBody):driver_setup::installCommand(driver_installer_));
            driver_code_->setEnabled(!driver_installer_.isEmpty());
        }
    }
    void checkDriver(){
        driver_input::retryInitialization();
        auto current=driver_input::state();
        if(current==driver_input::State::DllMissing){
            const auto found=driver_setup::locate();
            if(!found.isEmpty()){
                QString error;
                if(driver_setup::place(found,&error)){
                    showNotice(text(UiText::DriverPlaced));
                    driver_input::retryInitialization();current=driver_input::state();
                }else{
                    diagnostic("driver place",error);
                    showNotice(text(UiText::DriverPlaceFailed),true,true);
                }
            }
        }
        resolveInstaller();
        refreshOutputStatus();
        if(current==driver_input::State::Ready){showNotice(text(UiText::DriverFound));return;}
        showNotice(text(current==driver_input::State::Refused?UiText::DriverBlocked:UiText::DriverMissing),true);
    }
    void downloadDriver(){
        if(!network_)network_=new QNetworkAccessManager(this);
        if(driver_download_)driver_download_->setEnabled(false);
        driver_setup::downloadArchive(network_,this,QUrl(driver_setup::downloadUrl()),[this](QByteArray payload,QString error){
            if(driver_download_)driver_download_->setEnabled(true);
            // The pinned hash is the only thing trusted here; a rewritten URL or
            // a captive portal fails before anything touches the disk.
            if(!error.isEmpty()||!driver_setup::verify(payload)){
                diagnostic("driver download",error.isEmpty()?QStringLiteral("Archive checksum mismatch"):error);
                showNotice(text(UiText::DriverDownloadFailed),true,true);return;
            }
            const auto root=driver_setup::workRoot();
            const auto archive=root+QStringLiteral("/Interception.zip");
            QDir().mkpath(root);
            QFile file(archive);
            if(!file.open(QIODevice::WriteOnly)||file.write(payload)!=payload.size()){
                diagnostic("driver download","could not write "+archive);
                showNotice(text(UiText::DriverDownloadFailed),true,true);return;
            }
            file.close();
            driver_setup::VerifiedFile verifiedArchive;
            if(!verifiedArchive.open(archive,driver_setup::sha256())||!driver_setup::extract(archive,root,&error)){
                diagnostic("driver extract",error);
                showNotice(text(UiText::DriverDownloadFailed),true,true);return;
            }
            const auto found=driver_setup::findDll(root);
            if(found.isEmpty()){diagnostic("driver extract","library/x64/interception.dll not found");showNotice(text(UiText::DriverDownloadFailed),true,true);return;}
            if(!driver_setup::place(found,&error)){
                diagnostic("driver place",error);
                showNotice(text(UiText::DriverPlaceFailed),true,true);return;
            }
            driver_input::retryInitialization();resolveInstaller();refreshOutputStatus();
            showNotice(text(UiText::DriverFilesReady));
        });
    }
    // The driver is the only output backend, so the card reports its state
    // instead of offering a choice.
    void refreshOutputStatus(){
        if(!driver_status_)return;
        const auto driver_state=engine_.dry_run()?driver_input::State::DllMissing:driver_input::state();
        driver_status_->setText(text(driver_state==driver_input::State::Ready?UiText::DriverStateReady
                                    :driver_state==driver_input::State::Refused?UiText::DriverStateBlocked
                                    :UiText::DriverStateMissing));
        driver_status_->setProperty("state",driver_state==driver_input::State::Ready?QStringLiteral("ready")
                                         :driver_state==driver_input::State::Refused?QStringLiteral("blocked")
                                         :QStringLiteral("missing"));
        driver_status_->style()->unpolish(driver_status_);driver_status_->style()->polish(driver_status_);
        if(driver_test_)driver_test_->setEnabled(driver_state==driver_input::State::Ready);
    }
    void buildPreferences(){
        if(!preferences_)return;
        refreshing_=true;const auto c=engine_.snapshot();
        delete preferences_scroll_->takeWidget();preference_categories_.clear();auto* content=new QWidget;content->setObjectName("settingsSurface");
        auto* outer=new QVBoxLayout(content);outer->setContentsMargins(2,0,10,0);outer->setSpacing(14);
        const auto categoryLayout=[&]{auto* page=new QWidget;page->setMinimumWidth(0);auto* layout=new QVBoxLayout(page);layout->setContentsMargins(0,0,0,0);layout->setSpacing(24);outer->addWidget(page);preference_categories_.append(page);return layout;};
        auto* generalLayout=categoryLayout();auto* shortcutsLayout=categoryLayout();auto* phoneLayout=categoryLayout();auto* backupLayout=categoryLayout();
        auto* system=new Section(text(UiText::SystemSettings));system->setOpenSettings();system_=system;generalLayout->addWidget(system);
        auto* themeRow=settingRow(system,text(UiText::Theme),text(UiText::ThemeHint));
        theme_choice_=new LanguageComboBox(nullptr,QStringLiteral("themePopup"));theme_choice_->setObjectName("themeChoice");theme_choice_->setAccessibleName(text(UiText::Theme));
        theme_choice_->addItem(text(UiText::DarkTheme));theme_choice_->addItem(text(UiText::LightTheme));theme_choice_->setCurrentIndex(kbtheme::light?1:0);themeRow->addWidget(theme_choice_);
        connect(theme_choice_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int index){setLightTheme(index==1);});
        toggle(settingRow(system,text(UiText::FeedbackSound),text(UiText::SoundWhenClosingUnsaved)),"Feedback sound",c.sound,[this](bool on){engine_.edit([=](r6::Config& c){c.sound=on;});dirty();});
        game_focus_only_=toggle(settingRow(system,text(UiText::OnlyWhileGameIsActive),text(UiText::OnlyWhileGameIsActiveHint)),"Only in Rainbow Six",engine_.game_focus_only,[this](bool on){engine_.game_focus_only=on;dirty();});
        ads_only_=toggle(settingRow(system,text(UiText::OnlyWhileAiming),text(UiText::OnlyWhileAimingHint)),"Only while aiming",engine_.ads_only,[this](bool on){engine_.ads_only=on;dirty();});
        auto* langRow=settingRow(system,text(UiText::Language));language_=new LanguageComboBox;language_->setAccessibleName(text(UiText::Language));language_->setIconSize(QSize(22,15));
        for(const auto& entry:languageEntries())language_->addItem(languageFlag(entry.value),QString::fromUtf8(entry.label),entry.value);
        language_->setCurrentIndex(qMax(0,language_->findData(c.language)));langRow->addWidget(language_);
        connect(language_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int index){const int value=language_->itemData(index).toInt();engine_.edit([=](r6::Config& c){c.language=value;});dirty();if(value==6)QTimer::singleShot(0,this,[this]{showPalestineSurprise();});QTimer::singleShot(0,this,[this]{buildPreferences();buildSettings();updateLanguage();});});
        buildOutputMethod(generalLayout);
        auto* updates=new Section(text(UiText::Updates));updates->setOpenSettings(true);generalLayout->addWidget(updates);
        auto* versionRow=settingRow(updates,text(UiText::InstalledVersion),QString("Azec ")+QCoreApplication::applicationVersion());
        update_button_=new MonoButton(text(UiText::CheckForUpdates));update_button_->setObjectName("secondary");update_button_->setAccessibleName(text(UiText::CheckForUpdates));update_button_->setEnabled(!update_in_progress_);
        versionRow->addWidget(update_button_);connect(update_button_,&QPushButton::clicked,this,[this]{checkForUpdates();});
        auto* shortcutPage=new QWidget;shortcutPage->setObjectName("shortcutPage");shortcutPage->setMaximumWidth(620);shortcutPage->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);shortcutsLayout->addWidget(shortcutPage,0,Qt::AlignLeading);
        auto* shortcutLayout=new QVBoxLayout(shortcutPage);shortcutLayout->setContentsMargins(0,0,0,0);shortcutLayout->setSpacing(20);
        auto* shortcutIntro=new QVBoxLayout;shortcutIntro->setSpacing(6);shortcutIntro->addWidget(label(text(UiText::Shortcuts),"shortcutTitle"));
        auto* shortcutHint=label(text(UiText::ShortcutIntro),"rowHint");shortcutHint->setWordWrap(true);shortcutIntro->addWidget(shortcutHint);shortcutLayout->addLayout(shortcutIntro);
        auto* appShortcuts=new Section(text(UiText::ShortcutApplicationGroup));appShortcuts->setOpenSettings();shortcutLayout->addWidget(appShortcuts);
        auto* weaponShortcuts=new Section(text(UiText::ShortcutWeaponGroup));weaponShortcuts->setOpenSettings(true);shortcutLayout->addWidget(weaponShortcuts);
        const QString titles[]={text(UiText::OpenHideWindow),text(UiText::RecoilToggle),text(UiText::PanicExitKey),text(UiText::PrimaryWeapon),text(UiText::SecondaryWeapon)};const int keys[]={c.menu_key,c.recoil_key,c.panic_key,c.primary_key,c.secondary_key};
        const UiText shortcutHints[]={UiText::ShortcutWindowHint,UiText::ShortcutRecoilHint,UiText::ShortcutPanicHint,UiText::ShortcutPrimaryHint,UiText::ShortcutSecondaryHint};
        for(int i=0;i<5;++i){
            auto* section=i<3?appShortcuts:weaponShortcuts;
            if(i!=0&&i!=3){auto* divider=new QFrame;divider->setObjectName("profileDivider");divider->setFixedHeight(1);section->body->addWidget(divider);}
            auto* row=settingRow(section,titles[i],text(shortcutHints[i]));row->parentWidget()->setMinimumHeight(66);
            auto* button=new MonoButton(bindingName(keys[i]));button->setObjectName("keyButton");button->setFixedSize(96,36);button->setAccessibleName(titles[i]);button->setAccessibleDescription(text(shortcutHints[i]));row->addWidget(button,0,Qt::AlignVCenter);key_buttons_[i]=button;connect(button,&QPushButton::clicked,this,[this,i]{bindKey(i);});
        }
        auto* phoneSection=new Section(text(UiText::PhoneControl));phoneSection->setOpenSettings();phoneSection->body->setSpacing(0);phoneLayout->addWidget(phoneSection);
        phone_enabled_=toggle(settingRow(phoneSection,text(UiText::EnablePhoneAccess)),"Enable phone access",c.phone_enabled,[this](bool enabled){
            engine_.edit([&](r6::Config& c){c.phone_enabled=enabled;});
            if(pairing_qr_)pairing_qr_->conceal();
            if(enabled){if(!phone_->start()){showNotice(text(UiText::CouldNotStartPhoneServer),true,true);if(notice_bar_)notice_bar_->setProperty("phoneError",true);}}
            else phone_->stop();
            updatePhoneDetails();phone_details_->setExpanded(enabled);dirty();
        });
        phone_enabled_->parentWidget()->setFixedHeight(44);
        {
            auto* connection=new QWidget;auto* connectionLayout=new QVBoxLayout(connection);connectionLayout->setContentsMargins(0,13,0,8);connectionLayout->setSpacing(10);
            auto* addressRow=new QHBoxLayout;addressRow->setSpacing(8);
            auto* address=label({},"phoneAddress");phone_address_=address;address->setWordWrap(true);address->setMinimumWidth(0);address->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);address->setTextInteractionFlags(Qt::TextSelectableByMouse);addressRow->addWidget(address,1);
            phone_retry_=new MonoButton(text(UiText::Retry));phone_retry_->setObjectName("secondary");phone_retry_->setAccessibleName(text(UiText::Retry));addressRow->addWidget(phone_retry_);
            connect(phone_retry_,&QPushButton::clicked,this,[this]{if(!phone_->start()){showNotice(text(UiText::CouldNotStartPhoneServer),true,true);if(notice_bar_)notice_bar_->setProperty("phoneError",true);}else if(notice_bar_&&notice_bar_->property("phoneError").toBool()){notice_bar_->setProperty("phoneError",false);notice_timer_.stop();notice_bar_->hide();}updatePhoneDetails();});
            connectionLayout->addLayout(addressRow);
            auto* pair=new QHBoxLayout;pair->setSpacing(7);pair->addWidget(label(text(UiText::PairingCode),"hint"));
            phone_code_=new QLabel(phone_->code());phone_code_->setObjectName("pairingCode");phone_code_->setTextInteractionFlags(Qt::TextSelectableByMouse);pair->addWidget(phone_code_);pair->addStretch();
            connectionLayout->addLayout(pair);auto* pairActions=new QHBoxLayout;pairActions->setSpacing(12);
            copy_code_=new MonoButton(text(UiText::CopyCode));copy_code_->setObjectName("secondary");copy_code_->setInteractionIcon(InteractionIcon::Copy);pairActions->addWidget(copy_code_);
            connect(copy_code_,&QPushButton::clicked,this,[this]{QApplication::clipboard()->setText(phone_->code());if(engine_.snapshot().sound)sound::playCopy();copy_code_->setToolTip(text(UiText::Copied));copy_code_->confirmCopy();});
            copy_link_=new MonoButton(text(UiText::CopyLink));copy_link_->setObjectName("secondary");copy_link_->setInteractionIcon(InteractionIcon::Copy);pairActions->addWidget(copy_link_);
            connect(copy_link_,&QPushButton::clicked,this,[this]{QApplication::clipboard()->setText(phone_->pairingLink());if(engine_.snapshot().sound)sound::playCopy();copy_link_->setToolTip(text(UiText::Copied));copy_link_->confirmCopy();});
            phone_new_code_=new MonoButton(text(UiText::NewCode));phone_new_code_->setObjectName("secondary");pairActions->addWidget(phone_new_code_);
            connect(phone_new_code_,&QPushButton::clicked,this,[this]{phone_->rotate();if(pairing_qr_)pairing_qr_->conceal();updatePhoneDetails();showNotice(text(UiText::NewCodeCreated));});
            pairActions->addStretch();connectionLayout->addLayout(pairActions);
            auto* note=label(text(UiText::OpenThisAddressInSafariOnTheSameWiFi),"hint");note->setWordWrap(true);connectionLayout->addWidget(note);
            pairing_qr_=new PairingQr(text(UiText::ClickToRevealQR),text(UiText::ClickToHideQR));
            pairing_qr_->setEnabled(phone_->listening());pairing_qr_->setPairingLink(phone_->listening()?phone_->pairingLink():QString());
            connectionLayout->addWidget(pairing_qr_,0,Qt::AlignLeft);
            phone_details_=new ExpandingContent(connection,180);phoneSection->body->addWidget(phone_details_);phone_details_->setExpanded(c.phone_enabled,false);
            updatePhoneDetails();
        }
        auto* guideRow=new QHBoxLayout;
        auto* guide=new MonoButton(text(UiText::QuickGuide));guide->setObjectName("secondary");guideRow->addWidget(guide);connect(guide,&QPushButton::clicked,this,[this]{help();});
        guideRow->addStretch();generalLayout->addLayout(guideRow);
        auto* exportCard=new Section(text(UiText::ExportBackup));exportCard->setOpenSettings();backupLayout->addWidget(exportCard);
        auto* exportRow=new QHBoxLayout;exportRow->setSpacing(16);exportCard->body->addLayout(exportRow);
        auto* exportHint=label(text(UiText::ExportBackupHint),"rowHint");exportHint->setWordWrap(true);exportRow->addWidget(exportHint,1);
        auto* exportButton=new MonoButton(text(UiText::ExportBackup));exportButton->setObjectName("secondary");exportRow->addWidget(exportButton,0,Qt::AlignVCenter);
        auto* importCard=new Section(text(UiText::ImportBackup));importCard->setOpenSettings(true);backupLayout->addWidget(importCard);
        auto* importRow=new QHBoxLayout;importRow->setSpacing(16);importCard->body->addLayout(importRow);
        auto* importHint=label(text(UiText::ImportBackupHint),"rowHint");importHint->setWordWrap(true);importRow->addWidget(importHint,1);
        auto* importButton=new MonoButton(text(UiText::ChooseBackup));importButton->setObjectName("secondary");importButton->setEnabled(!save_blocked_);importRow->addWidget(importButton,0,Qt::AlignVCenter);
        connect(exportButton,&QPushButton::clicked,this,[this]{exportProfiles();});connect(importButton,&QPushButton::clicked,this,[this]{importProfiles();});
        for(auto* action:content->findChildren<QPushButton*>())action->setProperty("plainAction",true);
        outer->addStretch();preferences_scroll_->setWidget(content);updateSettingsNavLabels();chooseSettingsCategory(std::clamp(preference_category_,0,int(preference_categories_.size())-1));refreshing_=false;
    }
    // Both installed and older portable shared builds have a payload marker.
    bool packagedBuild() const{return QFileInfo(QCoreApplication::applicationDirPath()+"/version.txt").exists();}
    void setUpdating(bool busy){update_in_progress_=busy;if(update_button_)update_button_->setEnabled(!busy);}
    void showSaveBeforeUpdate(){
        auto message=text(UiText::SaveConfigurationUnsavedChanges);
        if(notice_bar_->isVisible()&&notice_bar_->property("persistent").toBool()){
            if(notice_->text().contains(message))return;
            message=notice_->text()+"\n\n"+message;
        }
        showNotice(message,true,true);notice_bar_->setProperty("saveError",true);
    }
    void checkForUpdates(){
        if(!update_button_||update_in_progress_)return;
        setUpdating(true);
        if(!network_)network_=new QNetworkAccessManager(this);
        QNetworkRequest request=azec_update::request(QUrl(azec_update::latestApi));
        request.setHeader(QNetworkRequest::UserAgentHeader,QString("Azec/")+AZEC_VERSION);
        request.setRawHeader("Accept","application/vnd.github+json");
        auto* reply=network_->get(request);
        connect(reply,&QNetworkReply::finished,this,[this,reply]{
            reply->deleteLater();
            setUpdating(false);
            const auto payload=reply->readAll();
            if(reply->error()!=QNetworkReply::NoError){showNotice(text(UiText::UpdateCheckFailed),true,true);return;}
            const auto root=QJsonDocument::fromJson(payload).object();
            QString latest=root["tag_name"].toString();
            if(latest.startsWith('v')||latest.startsWith('V'))latest.remove(0,1);
            if(latest.isEmpty()){showNotice(text(UiText::UpdateCheckFailed),true,true);return;}
            const QString current=QCoreApplication::applicationVersion();
            if(!azec_update::newer(latest,current)){QMessageBox::information(this,"Azec",text(UiText::UpToDate)+"\n\nAzec "+current);return;}
            if(engine_.snapshot().sound)sound::playArrival();
            QString asset;QByteArray digest;
            for(const auto& entry:root["assets"].toArray()){
                const auto object=entry.toObject();const auto name=object["name"].toString();
                if(name==azec_update::assetName){asset=object["browser_download_url"].toString();digest=azec_update::validHash(object["digest"].toString().toLatin1());}
            }
            if(!packagedBuild()||asset.isEmpty()){QDesktopServices::openUrl(QUrl(azec_update::releasePage));return;}
            if(unsaved_){showSaveBeforeUpdate();return;}
            if(QMessageBox::question(this,"Azec",text(UiText::UpdateAvailable)+"\n\nAzec "+current+"  ->  "+latest,QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes)!=QMessageBox::Yes)return;
            downloadUpdate(asset,digest);
        });
    }
    void downloadUpdate(const QString& url,const QByteArray& digest){
        if(!network_)network_=new QNetworkAccessManager(this);
        if(!url.startsWith("https://")||digest.isEmpty()){showNotice(text(UiText::UpdateCheckFailed),true,true);return;}
        setUpdating(true);
        const auto target=azec_paths::pendingUpdate();
        azec_update::downloadVerified(network_,this,QUrl(url),digest,target,[this,target](bool ok){
            setUpdating(false);
            if(!ok){showNotice(text(UiText::UpdateCheckFailed),true,true);return;}
            applyDownloadedUpdate(target);
        });
    }
    void applyDownloadedUpdate(const QString& file){
        // Edits may have arrived from the desktop or phone during the download.
        if(unsaved_){showSaveBeforeUpdate();return;}
        const auto loader=azec_paths::launcher();
        auto* process=new QProcess(this);
        process->setProgram(file);
        process->setArguments(azec_paths::updateArguments());
        process->setWorkingDirectory(QFileInfo(loader).absolutePath());
        const bool started=process->startDetached();
        process->deleteLater();
        if(!started){showNotice(text(UiText::UpdateCheckFailed),true,true);return;}
        force_close_=true;close();
    }
    void updatePhoneDetails(){
        const bool enabled=engine_.snapshot().phone_enabled;
        if(!enabled){
            if(pairing_qr_){pairing_qr_->conceal();pairing_qr_->setEnabled(false);}
            if(copy_link_)copy_link_->setEnabled(false);if(copy_code_)copy_code_->setEnabled(false);
            if(phone_new_code_)phone_new_code_->setEnabled(false);if(phone_retry_)phone_retry_->setVisible(false);
            return;
        }
        const bool listening=phone_->listening();const auto address=phone_->address();const bool reachable=listening&&!address.isEmpty();
        if(phone_address_){const QString label=!listening?text(UiText::CouldNotStartPhoneServer):reachable?address:text(UiText::PhoneNetworkUnavailable);if(phone_address_->text()!=label&&copy_link_)copy_link_->clearCopyFeedback();phone_address_->setText(label);}
        if(phone_code_){if(phone_code_->text()!=phone_->code()){if(copy_code_)copy_code_->clearCopyFeedback();if(copy_link_)copy_link_->clearCopyFeedback();}phone_code_->setText(phone_->code());}
        if(copy_link_)copy_link_->setEnabled(reachable);if(copy_code_)copy_code_->setEnabled(listening);
        if(phone_new_code_)phone_new_code_->setEnabled(listening);if(phone_retry_)phone_retry_->setVisible(!listening);
        if(pairing_qr_){pairing_qr_->setPairingLink(reachable?phone_->pairingLink():QString());pairing_qr_->setEnabled(reachable);}
    }
    void showPreferences(){
        if(pages_->currentWidget()==preferences_){showOperators();return;}
        settings_sidebar_=true;updateSidebarControls();stopTransition();pages_->switchTo(preferences_);gear_->setProperty("activePage",true);gear_->style()->unpolish(gear_);gear_->style()->polish(gear_);
    }
    void updateLanguage(){
        // Arabic (5) and the Hebrew "Free Palestine" prank (6) both render
        // right-to-left, in both the app window and every dialog.
        const int language=engine_.snapshot().language;const auto direction=(language==5||language==6)?Qt::RightToLeft:Qt::LeftToRight;
        if(qApp->layoutDirection()!=direction){qApp->setLayoutDirection(direction);applyStyle();}
        operators_title_->setText(text(UiText::AllOperators));favorites_title_->setText(text(UiText::Favorites));search_->setPlaceholderText(text(UiText::SearchOperators));
        updateSettingsNavLabels();
        if(minimize_){minimize_->setToolTip(text(UiText::Minimize));minimize_->setAccessibleName(text(UiText::Minimize));}
        close_->setToolTip(text(UiText::Close));close_->setAccessibleName(text(UiText::Close));
        styleNotice();layoutNotice();
        updateSidebarControls();
        updateSavePresentation(save_->property("saveState").toString());gear_->setToolTip(text(UiText::Settings));preferences_title_->setText(text(UiText::Settings));back_->setToolTip(text(UiText::BackToOperators));status_->setToolTip(text(UiText::ToggleRecoilActivity));rapid_status_->setToolTip(text(UiText::RapidFire));filterOperators();syncSelection();updateStatus();
    }
    void syncSelection(){
        const auto c=engine_.snapshot();const auto& op=c.active_operator();refreshing_=true;operator_name_->setText(QString::fromUtf8(op.name));operator_icon_->setPixmap(operatorPixmap(QString::fromUtf8(op.name),44,devicePixelRatioF()));
        dual_->syncChecked(c.secondary_enabled[c.selected]);secondary_->setEnabled(c.secondary_enabled[c.selected]);
        primary_->setText(text(UiText::Primary));secondary_->setText(text(UiText::Secondary));
        primary_->setChosen(c.active_slot()==0);secondary_->setChosen(c.active_slot()==1);
        vertical_->setValue(op.vertical);horizontal_->setValue(op.horizontal);vertical_slider_->setValue(qRound(op.vertical*10));horizontal_slider_->setValue(qRound(op.horizontal*10));humanize_->syncChecked(op.humanize);delay_->setValue(std::clamp(op.delay,30,150));delay_->setEnabled(op.rapid!=0);refreshing_=false;updateStatus();
    }
    void chooseWeapon(int slot){if(engine_.select_weapon(slot)){stopTransition();syncSelection();}}
    void handleWeaponKeys(bool primary,bool secondary,bool editing,bool modified){
        if(!editing&&!modified){if(primary&&!was_primary_)chooseWeapon(0);if(secondary&&!was_secondary_)chooseWeapon(1);}was_primary_=primary;was_secondary_=secondary;
    }
    void syncDelayControls(){if(!delay_slider_)return;const bool enabled=engine_.rapid_active();delay_->setEnabled(enabled);delay_controls_->setEnabled(enabled);delay_minus_->setEnabled(delay_slider_->value()>30);delay_plus_->setEnabled(delay_slider_->value()<150);}
    void updateStatus(){const bool active=engine_.recoil_active;status_->setText(active?text(UiText::RecoilOn):text(UiText::RecoilOff));status_->setStatusActive(active);status_->style()->unpolish(status_);status_->style()->polish(status_);
        const bool rapid=engine_.rapid_active();rapid_status_->setText(rapid?text(UiText::RapidFireOn):text(UiText::RapidFireOff));rapid_status_->setStatusActive(rapid);rapid_status_->style()->unpolish(rapid_status_);rapid_status_->style()->polish(rapid_status_);
        if(rapid_active_)rapid_active_->syncChecked(rapid);if(game_focus_only_)game_focus_only_->syncChecked(engine_.game_focus_only);if(ads_only_)ads_only_->syncChecked(engine_.ads_only);syncDelayControls();if(tray_rapid_)tray_rapid_->setChecked(rapid);}
    void toggleRecoil(){engine_.recoil_active=!engine_.recoil_active;if(engine_.snapshot().sound)sound::playToggle();updateStatus();}
    void toggleRapid(){const bool on=!engine_.rapid_active();engine_.set_rapid_active(on);dirty();if(engine_.snapshot().sound)sound::playToggle();updateStatus();}
    bool textEditingActive() const {auto* focused=QApplication::focusWidget();return focused&&(qobject_cast<QLineEdit*>(focused)||qobject_cast<QAbstractSpinBox*>(focused)||qobject_cast<QComboBox*>(focused));}
    void pollKeys(){
        const bool editing=isActiveWindow()&&textEditingActive();
        const bool game=r6::gameIsActive();
        const bool focusOnly=engine_.game_focus_only.load();
        const bool azecActive=isActiveWindow()||QApplication::activeModalWidget()||QApplication::activePopupWidget();
        engine_.input_allowed=!focusOnly||game;
        engine_.menu_visible=azecActive;
        if(binding_||QApplication::activeModalWidget()||QApplication::activePopupWidget())return;
        if(release_key_){if(keyDown(release_key_)||clock_.elapsed()<blocked_until_)return;release_key_=0;was_menu_=was_recoil_=false;}
        const auto k=engine_.hotkeys();
        if(!azecActive&&!editing&&(!focusOnly||game)&&keyDown(k.panic)){force_close_=true;engine_.stop();qApp->quit();return;}
        if(!(!focusOnly||game||azecActive)){was_menu_=was_recoil_=was_primary_=was_secondary_=false;return;}
        const bool menu=keyDown(k.menu),recoil=keyDown(k.recoil);
        if(menu&&!was_menu_){if(isVisible()&&!isMinimized())hide();else{showNormal();raise();activateWindow();}engine_.menu_visible=isVisible();}
        if(recoil&&!was_recoil_)toggleRecoil();
        was_menu_=menu;was_recoil_=recoil;
        auto* focused=QApplication::focusWidget();
        const auto modifierKey=[](int key){return key==VK_CONTROL||key==VK_MENU||key==VK_LWIN||key==VK_RWIN||key==VK_LCONTROL||key==VK_RCONTROL||key==VK_LMENU||key==VK_RMENU;};
        const bool modifierHeld=keyDown(VK_CONTROL)||keyDown(VK_MENU)||keyDown(VK_LWIN)||keyDown(VK_RWIN);
        const bool modified=modifierHeld&&!(modifierKey(k.primary)&&keyDown(k.primary))&&!(modifierKey(k.secondary)&&keyDown(k.secondary));
        handleWeaponKeys(keyDown(k.primary),keyDown(k.secondary),editing,modified);
        if(isVisible()&&isActiveWindow()&&pages_->currentWidget()==operator_page_&&!editing&&!qobject_cast<QSlider*>(focused)&&!qobject_cast<QAbstractItemView*>(focused)&&clock_.elapsed()-last_tune_>80){const float y=(keyDown(VK_UP)? .1f:0)-(keyDown(VK_DOWN)? .1f:0),x=(keyDown(VK_RIGHT)? .1f:0)-(keyDown(VK_LEFT)? .1f:0);if(x||y){editOperator([=](r6::Operator& op){op.vertical=std::clamp(op.vertical+y,0.f,100.f);op.horizontal=std::clamp(op.horizontal+x,-50.f,50.f);});syncSelection();last_tune_=clock_.elapsed();}}
    }
    bool save(bool celebrate=false){
        if(save_blocked_)return false;
        QByteArray uiBytes;
        if(imported_ui_pending_){
            QJsonObject root;
            if(!uiprefs::readDocument(uiprefs::filePath(config_path_),root)){
                updateSavePresentation("error");save_feedback_.stop();showNotice(text(UiText::CouldNotSaveConfiguration),true,true);notice_bar_->setProperty("saveError",true);return false;
            }
            QStringList names(favorites_.begin(),favorites_.end());names.sort(Qt::CaseInsensitive);
            root["favorites"]=QJsonArray::fromStringList(names);
            root["sidebarCollapsed"]=sidebar_collapsed_;root["version"]=1;root.remove("interface");root.remove("colorTheme");
            uiBytes=QJsonDocument(root).toJson();
        }
        const bool saved=!save_blocked_&&engine_.save(config_path_,uiBytes);
        if(saved){
            imported_ui_pending_=false;
            updateSavePresentation("saved",true);save_feedback_.start();
            if(celebrate&&engine_.snapshot().sound)sound::playSuccess();
            refreshOutputStatus();
            if(notice_bar_&&notice_bar_->property("saveError").toBool()){notice_bar_->setProperty("saveError",false);notice_timer_.stop();notice_bar_->hide();}
        }else{
            updateSavePresentation("error");save_feedback_.stop();
            showNotice(text(UiText::CouldNotSaveConfiguration),true,true);
            if(notice_bar_)notice_bar_->setProperty("saveError",true);
        }
        return saved;
    }
    // Bindings persist on their own. A shortcut change must never commit the
    // profile edits that are still waiting for Save.
    bool saveShortcuts(){
        const auto c=engine_.snapshot();
        if(!save_blocked_&&persistHotkeys(config_path_,c.menu_key,c.recoil_key,c.panic_key,c.primary_key,c.secondary_key))return true;
        diagnostic("save shortcuts",save_blocked_?"Saving is blocked":"Could not update the binding files");
        unsaved_=true;updateSavePresentation("error");save_feedback_.stop();showNotice(text(UiText::CouldNotSaveConfiguration),true,true);
        if(notice_bar_)notice_bar_->setProperty("saveError",true);
        return false;
    }
    void bindKey(int index){
        binding_=index+1;QDialog dialog(this,Qt::Dialog|Qt::FramelessWindowHint);dialog.setObjectName("hotkeyDialog");dialog.setAttribute(Qt::WA_StyledBackground);dialog.setWindowTitle(text(UiText::KeyBinding));dialog.setFixedWidth(336);auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(20,16,20,18);layout->setSpacing(12);
        const QString actions[]={text(UiText::OpenHideWindow),text(UiText::RecoilToggle),text(UiText::PanicExitKey),text(UiText::PrimaryWeapon),text(UiText::SecondaryWeapon)};
        auto* header=new QHBoxLayout;header->setSpacing(8);auto* action=label(actions[index],"rowTitle");action->setWordWrap(true);header->addWidget(action,1);auto* cancel=new IconButton(":/ui/close.svg",[this]{return accent();});cancel->setObjectName("closeControl");cancel->setAccessibleName(text(UiText::CancelHotkeyBinding));cancel->setToolTip(text(UiText::CancelHotkeyBinding));header->addWidget(cancel,0,Qt::AlignTop);layout->addLayout(header);connect(cancel,&QPushButton::clicked,&dialog,&QDialog::reject);
        auto* hint=label(text(UiText::ReleaseYourKeysWaiting),"bindingHint");hint->setWordWrap(true);hint->setAlignment(Qt::AlignCenter);hint->setAccessibleName(text(UiText::SetAHotkey));layout->addWidget(hint);
        auto* instructions=label(text(UiText::EscCancelsBackspaceClears),"hint");instructions->setWordWrap(true);instructions->setAlignment(Qt::AlignCenter);layout->addWidget(instructions);
        // Reserve only the space the localized messages need, so rejected keys
        // never resize the prompt while the user is typing.
        dialog.ensurePolished();hint->ensurePolished();const auto margins=layout->contentsMargins();const int hintWidth=dialog.contentsRect().width()-margins.left()-margins.right();int hintHeight=52;
        for(const auto message:{UiText::ReleaseYourKeysWaiting,UiText::PressAKeyNow,UiText::UnsafeGlobalHotkey,UiText::ThisKeyIsAlreadyAssignedChooseAnother}){hint->setText(text(message));hintHeight=qMax(hintHeight,hint->heightForWidth(hintWidth));}
        hint->setFixedHeight(hintHeight);hint->setText(text(UiText::ReleaseYourKeysWaiting));
        const auto showBindingError=[&](UiText message){hint->setText(text(message));hint->setProperty("invalid",true);hint->style()->unpolish(hint);hint->style()->polish(hint);hint->update();};
        bool released=false;int rejectedKey=0;QTimer timer;timer.setInterval(16);connect(&timer,&QTimer::timeout,&dialog,[&]{
            if(!dialog.isActiveWindow())return;
            if(!released){for(int key=1;key<256;++key)if(keyDown(key))return;released=true;hint->setText(text(UiText::PressAKeyNow));}
            if(rejectedKey){if(keyDown(rejectedKey))return;rejectedKey=0;}
            for(int key=3;key<256;++key){if(!r6::validHotkey(key))continue;if(keyDown(key)){
                if(key==VK_ESCAPE){release_key_=key;blocked_until_=clock_.elapsed()+250;dialog.reject();return;}
                if(key!=VK_BACK&&index==2&&!r6::validGlobalHotkey(key)){showBindingError(UiText::UnsafeGlobalHotkey);rejectedKey=key;return;}
                const auto existing=engine_.snapshot();if(key!=VK_BACK&&hotkeyConflict(existing,index,key)>=0){showBindingError(UiText::ThisKeyIsAlreadyAssignedChooseAnother);rejectedKey=key;return;}
                engine_.edit([=](r6::Config& c){int* keys[]={&c.menu_key,&c.recoil_key,&c.panic_key,&c.primary_key,&c.secondary_key};*keys[index]=key==VK_BACK?0:key;});saveShortcuts();release_key_=key;blocked_until_=clock_.elapsed()+250;dialog.accept();return;
            }}
        });
        dialog.adjustSize();dialog.setFixedHeight(dialog.height());roundWindow(&dialog);dialog.move(frameGeometry().center()-dialog.rect().center());timer.start();dialog.exec();binding_=0;const auto c=engine_.snapshot();const int keys[]={c.menu_key,c.recoil_key,c.panic_key,c.primary_key,c.secondary_key};if(key_buttons_[index])key_buttons_[index]->setText(bindingName(keys[index]));syncSelection();updateStatus();
    }
    void showPalestineSurprise(){
        // The Hebrew picker entry is bait: the reveal is a Free Palestine popup.
        if(palestine_popup_&&palestine_popup_->isVisible()){palestine_popup_->raise();palestine_popup_->activateWindow();return;}
        auto* popup=new QDialog(this,Qt::Dialog|Qt::FramelessWindowHint);popup->setObjectName("palestinePopup");popup->setAttribute(Qt::WA_DeleteOnClose);popup->setWindowTitle("Free Palestine");
        auto* layout=new QVBoxLayout(popup);layout->setContentsMargins(22,16,22,18);layout->setSpacing(6);
        auto* flag=new QLabel;flag->setPixmap(palestineFlagPixmap(120,80));flag->setAlignment(Qt::AlignCenter);layout->addWidget(flag,0,Qt::AlignCenter);
        auto* title=label("Free Palestine","cardTitle");title->setAlignment(Qt::AlignCenter);layout->addWidget(title);
        auto* arabic=label(QString::fromUtf8("فلسطين حرة"),"palestineArabic");arabic->setAlignment(Qt::AlignCenter);layout->addWidget(arabic);
        auto* hint=label(QString::fromUtf8("Palestine will be free · فلسطين حرة"),"hint");hint->setAlignment(Qt::AlignCenter);layout->addWidget(hint);
        auto* close=new MonoButton("Free Palestine");close->setObjectName("save");layout->addWidget(close,0,Qt::AlignCenter);connect(close,&QPushButton::clicked,popup,&QDialog::close);
        popup->setFixedWidth(340);popup->adjustSize();popup->move(mapToGlobal(rect().center())-popup->rect().center());roundWindow(popup);
        palestine_popup_=popup;popup->show();popup->raise();popup->activateWindow();
    }
    void help(){
        QDialog dialog(this,Qt::Dialog|Qt::FramelessWindowHint);dialog.setObjectName("helpDialog");dialog.setAttribute(Qt::WA_StyledBackground);dialog.setFixedSize(410,230);dialog.setWindowTitle(text(UiText::AzecHelp));auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(24,18,24,22);
        auto* header=new QHBoxLayout;auto* title=label("","cardTitle");header->addWidget(title,1);auto* close=new IconButton(":/ui/close.svg",[this]{return accent();});close->setObjectName("closeControl");close->setAccessibleName(text(UiText::CloseQuickGuide));header->addWidget(close);layout->addLayout(header);connect(close,&QPushButton::clicked,&dialog,&QDialog::accept);
        auto* description=label("");description->setWordWrap(true);layout->addWidget(description,1);auto* row=new QHBoxLayout;auto* back=new MonoButton(text(UiText::Back));back->setObjectName("secondary");auto* next=new MonoButton(text(UiText::Next));next->setObjectName("save");auto* count=label("","hint");row->addWidget(back);row->addWidget(count,1,Qt::AlignCenter);row->addWidget(next);layout->addLayout(row);
        const QStringList titles={text(UiText::Welcome),text(UiText::ChooseAnOperator),text(UiText::AdjustRecoil),text(UiText::RapidFire),text(UiText::HotkeysAndSaving)};
        const QStringList descriptions={text(UiText::YourOperatorProfilesAndControlsInOnePlace),text(UiText::SelectANameInTheSidebarOrSearchForAnOperator),text(UiText::SetVerticalAndHorizontalForceUseArrowKeysForFineAdjustments),text(UiText::EnableRapidFireForTheSelectedWeapon),text(UiText::SetYourHotkeysInSystemSettingsSaveConfigurationToKeepYourChanges)};
        int step=0;const auto refresh=[&]{title->setText(titles[step]);description->setText(descriptions[step]);count->setText(QString::number(step+1)+" / 5");back->setEnabled(step>0);next->setText(step==4?text(UiText::Done):text(UiText::Next));};
        connect(back,&QPushButton::clicked,&dialog,[&]{if(step)--step;refresh();});connect(next,&QPushButton::clicked,&dialog,[&]{if(step==4)dialog.accept();else{++step;refresh();}});refresh();roundWindow(&dialog);dialog.exec();
    }
protected:
    void updateWindowShape(){
        if(isMaximized()||isFullScreen()){clearMask();return;}
        // Painting a curve alone leaves the native acrylic rectangle behind it.
        // A Qt mask also clips the HWND's blur and child surfaces on Windows 10.
        QPainterPath shape;shape.addRoundedRect(QRectF(rect()),16,16);
        setMask(QRegion(shape.toFillPolygon().toPolygon()));
    }
    void paintEvent(QPaintEvent*) override {

        ++paint_count_;QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
        // Match the native window mask; no acrylic may extend into the notches.
        const qreal radius=isMaximized()?0:16;
        QPainterPath shape;shape.addRoundedRect(QRectF(rect()).adjusted(.5,.5,-.5,-.5),radius,radius);
        p.fillPath(shape,glass_active_?kbtheme::windowGlass:kbtheme::paper);
        if(glass_active_){
            p.fillPath(shape,kbtheme::windowWash);
        }
    }
    // Keep the same translucent shell over either effect. Change the native
    // material only at move/resize boundaries, never for each mouse position.
    bool applyWindowGlass(bool lightweight){
        struct AccentPolicy{DWORD state;DWORD flags;DWORD color;DWORD animationId;};
        struct AttributeData{DWORD attribute;PVOID data;SIZE_T size;};
        static const auto setComposition=reinterpret_cast<BOOL(WINAPI*)(HWND,AttributeData*)>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"SetWindowCompositionAttribute"));
        if(!setComposition)return false;
        // ACCENT_ENABLE_BLURBEHIND (3) uses flag 2; acrylic (4) uses flag 0.
        // Leave the system tint nearly empty; Qt paints the inset shell.
        // Both themes keep glass active. Theme changes only repaint the tint,
        // avoiding the clear frame caused by disabling/re-enabling acrylic.
        AccentPolicy policy{lightweight?3u:4u,lightweight?2u:0u,0x01000000u,0};
        AttributeData helper{19,reinterpret_cast<PVOID>(&policy),sizeof(policy)}; // WCA_ACCENT_POLICY
        if(!setComposition(HWND(winId()),&helper))return false;
        const bool wasActive=glass_active_;
        glass_active_=true;glass_lightweight_=lightweight;
        if(wasActive!=glass_active_)update();
        return true;
    }
    void enableWindowGlass(){
        if(glass_applied_)return;glass_applied_=true;
        // Attach after the first paint, retaining the opaque fallback on failure.
        applyWindowGlass(false);
    }
    void showEvent(QShowEvent* event) override {
        QWidget::showEvent(event);engine_.menu_visible=true;
        if(!native_ready_){native_ready_=true;const HWND handle=HWND(winId());const auto style=GetWindowLongPtrW(handle,GWL_STYLE);SetWindowLongPtrW(handle,GWL_STYLE,style|WS_THICKFRAME|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX);SetWindowPos(handle,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);const BOOL dark=!kbtheme::light;DwmSetWindowAttribute(handle,20,&dark,sizeof(dark));
            // The blur attaches after the first paint, like the reference.
            QTimer::singleShot(0,this,[this]{enableWindowGlass();});}
        updateWindowShape();
    }
    void hideEvent(QHideEvent* event) override {
        settleThemeTransition();
        if(glass_lightweight_)applyWindowGlass(false);
        if(pairing_qr_)pairing_qr_->conceal();engine_.menu_visible=false;QWidget::hideEvent(event);
    }
    // The one Save / Discard / Cancel prompt for leaving with unsaved edits,
    // whether the app closes or relaunches with administrator rights.
    int unsavedChoice(const QString& heading){
        QDialog dialog(this,Qt::Dialog|Qt::FramelessWindowHint);dialog.setObjectName("unsavedDialog");dialog.setAttribute(Qt::WA_StyledBackground);dialog.setWindowTitle(text(UiText::UnsavedChanges));dialog.setFixedWidth(390);
        auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(22,20,22,20);layout->setSpacing(18);
        layout->addWidget(label(heading,"rowTitle"));
        auto* row=new QHBoxLayout;int choice=0;
        for(int i=0;i<3;++i){const QString titles[]={text(UiText::Cancel),text(UiText::Discard),text(UiText::Save)};auto* button=new MonoButton(titles[i]);button->setObjectName(i==2?"save":i==1?"discardChanges":"cancelClose");row->addWidget(button);connect(button,&QPushButton::clicked,&dialog,[&,i]{choice=i;dialog.accept();});}
        layout->addLayout(row);dialog.adjustSize();roundWindow(&dialog);close_alert_=&dialog;dialog.exec();close_alert_=nullptr;
        return choice;
    }
    void closeEvent(QCloseEvent* event) override {
        if(close_prompt_){if(!isVisible())showNormal();raise();activateWindow();QApplication::alert(this,0);if(close_alert_&&close_alert_->isVisible()){close_alert_->raise();close_alert_->activateWindow();}event->ignore();return;}
        if(unsaved_&&!force_close_){
            close_prompt_=true;close_recoil_changed_=false;const bool recoil=engine_.recoil_active.exchange(false);
            if(!engine_.dry_run()&&engine_.snapshot().sound)sound::playUnsaved();
            if(!isVisible())showNormal();
            raise();activateWindow();QApplication::alert(this,0);
            const int choice=unsavedChoice(text(UiText::SaveYourChangesBeforeClosing));
            if(!close_recoil_changed_)engine_.recoil_active=recoil;close_prompt_=false;
            if(choice==0||(choice==2&&!save(true))){event->ignore();return;}
        }
        if(tray_)tray_->hide();phone_->stop();engine_.stop();QWidget::closeEvent(event);
    }
    void resizeEvent(QResizeEvent* event) override {settleThemeTransition();QWidget::resizeEvent(event);updateWindowShape();layoutNotice();}
    void changeEvent(QEvent* event) override {
        QWidget::changeEvent(event);
        if(event->type()==QEvent::WindowStateChange)updateWindowShape();
    }
    bool eventFilter(QObject* object,QEvent* event) override {
        if(object==sidebar_search_box_&&event->type()==QEvent::Resize&&sidebar_search_)sidebar_search_->setFixedWidth(static_cast<QResizeEvent*>(event)->size().width());
        if(event->type()==QEvent::FocusIn||event->type()==QEvent::FocusOut||event->type()==QEvent::MouseButtonPress){
            if(auto* button=qobject_cast<QPushButton*>(object);button&&(isAncestorOf(button)||button->window()->parentWidget()==this)){
                const auto reason=event->type()==QEvent::MouseButtonPress?Qt::MouseFocusReason:static_cast<QFocusEvent*>(event)->reason();const bool keyboard=event->type()==QEvent::FocusIn&&(reason==Qt::TabFocusReason||reason==Qt::BacktabFocusReason||reason==Qt::ShortcutFocusReason);
                button->setProperty("keyboardFocus",keyboard);button->style()->unpolish(button);button->style()->polish(button);button->update();
            }
        }
        if(event->type()==QEvent::Wheel){
            auto* widget=qobject_cast<QWidget*>(object);
            // A dropdown never takes the wheel, even when it still holds focus
            // after being clicked: scrolling the page must not change a setting.
            const bool dropdown=qobject_cast<QComboBox*>(widget)!=nullptr;
            const bool valueControl=dropdown||qobject_cast<QAbstractSlider*>(widget)||qobject_cast<QAbstractSpinBox*>(widget);
            if(widget&&valueControl&&(dropdown||!widget->hasFocus())){
                for(auto* parent=widget->parentWidget();parent;parent=parent->parentWidget()){
                    if(auto* area=qobject_cast<QScrollArea*>(parent)){
                        auto* wheel=static_cast<QWheelEvent*>(event);
                        const int angle=wheel->angleDelta().y();
                        const int pixels=wheel->pixelDelta().y();
                        if(auto* smooth=dynamic_cast<SmoothScrollArea*>(area))smooth->scrollByWheel(angle,pixels);
                        else {
                            auto* bar=area->verticalScrollBar();
                            if(angle)bar->setValue(bar->value()-angle/120*bar->singleStep()*3);
                            else if(pixels)bar->setValue(bar->value()-pixels);
                        }
                        return true;
                    }
                }
            }
        }
        if(event->type()==QEvent::KeyPress){auto* key=static_cast<QKeyEvent*>(event);auto* widget=qobject_cast<QWidget*>(object);
            if(widget&&widget->window()==this&&pages_->currentWidget()==operator_page_&&(key->key()==Qt::Key_Up||key->key()==Qt::Key_Down||key->key()==Qt::Key_Left||key->key()==Qt::Key_Right)&&!qobject_cast<QLineEdit*>(widget)&&!qobject_cast<QAbstractSpinBox*>(widget)&&!qobject_cast<QSlider*>(widget)&&!qobject_cast<QAbstractItemView*>(widget))return true;}
        return QWidget::eventFilter(object,event);
    }
    bool nativeEvent(const QByteArray& type,void* message,qintptr* result) override {
        auto* msg=static_cast<MSG*>(message);
        if(msg->message==WM_ENTERSIZEMOVE&&glass_active_){
            // Windows 10 acrylic stalls native dragging. Plain blur keeps the
            // background translucent without that material's movement cost.
            // Do not apply this workaround to Windows 11, where plain blur has
            // its own compositor performance problems.
            const auto windows=QOperatingSystemVersion::current();
            if(windows.majorVersion()==10&&windows.microVersion()<22000&&!glass_lightweight_){settleThemeTransition();applyWindowGlass(true);}
        }else if(msg->message==WM_EXITSIZEMOVE&&glass_lightweight_){
            // Also sent for a cancelled move/resize. Failed changes retain the
            // last successful material instead of losing the glass background.
            applyWindowGlass(false);
        }
        if(msg->message==WM_NCCALCSIZE&&msg->wParam){*result=0;return true;}
        if(msg->message==WM_NCHITTEST){RECT r;GetWindowRect(HWND(winId()),&r);const int x=short(LOWORD(msg->lParam)),y=short(HIWORD(msg->lParam)),edge=qRound(6*devicePixelRatioF());const bool left=x<r.left+edge,right=x>=r.right-edge,top=y<r.top+edge,bottom=y>=r.bottom-edge;
            if(top&&left)*result=HTTOPLEFT;else if(top&&right)*result=HTTOPRIGHT;else if(bottom&&left)*result=HTBOTTOMLEFT;else if(bottom&&right)*result=HTBOTTOMRIGHT;else if(left)*result=HTLEFT;else if(right)*result=HTRIGHT;else if(top)*result=HTTOP;else if(bottom)*result=HTBOTTOM;else return QWidget::nativeEvent(type,message,result);return true;}return QWidget::nativeEvent(type,message,result);
    }
public:
    explicit MainWindow(r6::Engine& engine,std::filesystem::path config_path):engine_(engine),config_path_(std::move(config_path)){
        kbtheme::initialize(uiprefs::lightTheme(config_path_));
        theme_motion_.setDuration(160);theme_motion_.setStartValue(0.0);theme_motion_.setEndValue(1.0);theme_motion_.setEasingCurve(monoEase());
        connect(&theme_motion_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){if(theme_snapshot_)theme_snapshot_->setProgress(value.toReal());});
        connect(&theme_motion_,&QVariantAnimation::finished,this,[this]{settleThemeTransition();});
        setObjectName("mainWindow");setWindowTitle("Azec");setWindowFlags(Qt::Window|Qt::FramelessWindowHint);setAttribute(Qt::WA_TranslucentBackground);setWindowIcon(QIcon(":/ui/app-icon.svg"));const auto area=QGuiApplication::primaryScreen()->availableGeometry();setMinimumSize(std::min(760,area.width()-32),std::min(560,area.height()-32));resize(std::min(864,area.width()-48),std::min(664,area.height()-48));move(area.center()-rect().center());
        auto* layout=new QVBoxLayout(this);layout->setContentsMargins(0,0,0,0);layout->setSpacing(0);auto* header=new DragHeader;header->setObjectName("titleBar");header->setFixedHeight(52);auto* top=new QHBoxLayout(header);top->setContentsMargins(20,0,20,0);top->setSpacing(5);
        auto* logo=new QLabel;brand_logo_=logo;logo->setObjectName("brandLogo");logo->setFixedSize(32,32);logo->setPixmap(QIcon(":/ui/logo.svg").pixmap(QSize(32,32),devicePixelRatioF()));top->addWidget(logo);top->addSpacing(5);
        top->addWidget(new BrandLabel);
        top->addStretch();status_=new MonoButton;status_->setObjectName("status");status_->setInteractionIcon(InteractionIcon::Recoil);status_->setFixedHeight(32);status_->setCursor(Qt::PointingHandCursor);status_->setToolTip(text(UiText::ToggleRecoilActivity));top->addWidget(status_);connect(status_,&QPushButton::clicked,this,[this]{toggleRecoil();});
        rapid_status_=new MonoButton;rapid_status_->setObjectName("statusRapid");rapid_status_->setInteractionIcon(InteractionIcon::Rapid);rapid_status_->setFixedHeight(32);rapid_status_->setCursor(Qt::PointingHandCursor);rapid_status_->setToolTip(text(UiText::RapidFire));top->addWidget(rapid_status_);connect(rapid_status_,&QPushButton::clicked,this,[this]{toggleRapid();});
        save_=new IconButton(":/ui/save.svg",[this]{return accent();});save_->setObjectName("saveControl");save_->setAccessibleName(text(UiText::SaveConfiguration));top->addSpacing(7);top->addWidget(save_);connect(save_,&QPushButton::clicked,this,[this]{save(true);});
        save_feedback_.setSingleShot(true);save_feedback_.setInterval(1600);connect(&save_feedback_,&QTimer::timeout,this,[this]{updateSavePresentation("clean");});
        gear_=new IconButton(":/ui/settings.svg",[this]{return accent();});gear_->setObjectName("windowControl");gear_->setAccessibleName(text(UiText::Settings));gear_->setToolTip(text(UiText::Settings));top->addWidget(gear_);connect(gear_,&QPushButton::clicked,this,[this]{showPreferences();});top->addSpacing(7);minimize_=new IconButton(":/ui/minimize.svg",[this]{return accent();});minimize_->setObjectName("windowControl");minimize_->setAccessibleName(text(UiText::Minimize));minimize_->setToolTip(text(UiText::Minimize));top->addWidget(minimize_);connect(minimize_,&QPushButton::clicked,this,&QWidget::showMinimized);
        close_=new IconButton(":/ui/close.svg",[this]{return accent();});close_->setObjectName("closeControl");close_->setAccessibleName(text(UiText::Close));close_->setToolTip(text(UiText::Close));top->addWidget(close_);connect(close_,&QPushButton::clicked,this,&QWidget::close);layout->addWidget(header);
        notice_bar_=new QWidget(this);notice_bar_->setObjectName("noticeBar");notice_bar_->setAttribute(Qt::WA_StyledBackground);auto* noticeLayout=new QVBoxLayout(notice_bar_);noticeLayout->setContentsMargins(24,22,24,22);noticeLayout->setSpacing(16);
        auto* noticeHeader=new QHBoxLayout;noticeHeader->setSpacing(12);notice_badge_=label({},"noticeBadge");notice_badge_->setFixedSize(34,34);notice_badge_->setAlignment(Qt::AlignCenter);noticeHeader->addWidget(notice_badge_);
        notice_title_=label({},"cardTitle");notice_title_->setWordWrap(true);noticeHeader->addWidget(notice_title_,1);noticeLayout->addLayout(noticeHeader);
        notice_=new QLabel;notice_->setObjectName("startupNotice");notice_->setWordWrap(true);notice_->setTextFormat(Qt::PlainText);notice_->setTextInteractionFlags(Qt::TextSelectableByMouse);notice_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
        notice_scroll_=new QScrollArea;notice_scroll_->setWidgetResizable(true);notice_scroll_->setFrameShape(QFrame::NoFrame);notice_scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);notice_scroll_->setWidget(notice_);noticeLayout->addWidget(notice_scroll_);FadingScrollBar::install(notice_scroll_);
        notice_close_=new MonoButton;notice_close_->setObjectName("secondary");notice_close_->setMinimumHeight(40);
        connect(notice_close_,&QPushButton::clicked,this,[this]{notice_timer_.stop();notice_bar_->hide();});
        noticeLayout->addWidget(notice_close_);auto* noticeShadow=new QGraphicsDropShadowEffect(notice_bar_);noticeShadow->setBlurRadius(0);noticeShadow->setOffset(3,4);notice_bar_->setGraphicsEffect(noticeShadow);notice_bar_->hide();
        buildDesktopLayout(layout);
        const auto c=engine_.snapshot();for(int i=0;i<r6::operator_count;++i){const QString name=QString::fromUtf8(c.operators[i].name);auto* item=new QListWidgetItem(operatorIcon(name),name);item->setData(Qt::UserRole,i);item->setToolTip(name);operators_->addItem(item);QSvgRenderer renderer(":/operators/"+operatorAssetName(name)+".svg");if(!renderer.isValid())icon_failures_<<name;}
        operators_->setCurrentRow(c.selected);
        favorites_=readFavorites();refreshFavorites();
        connect(operators_,&QListWidget::currentItemChanged,this,[this](QListWidgetItem* item,QListWidgetItem*){if(item)selectOperator(item->data(Qt::UserRole).toInt());});connect(favorites_list_,&QListWidget::currentItemChanged,this,[this](QListWidgetItem* item,QListWidgetItem*){if(item)selectOperator(item->data(Qt::UserRole).toInt());});
        for(auto* list:{operators_,favorites_list_})connect(list,&QListWidget::itemClicked,this,[this](QListWidgetItem* item){if(!item)return;selectOperator(item->data(Qt::UserRole).toInt());});
        connect(search_,&QLineEdit::textChanged,this,[this]{filterOperators();});
        operator_transition_.setDuration(120);operator_transition_.setStartValue(.35);operator_transition_.setEndValue(1.0);operator_transition_.setEasingCurve(QEasingCurve::InOutQuad);
        connect(&operator_transition_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){name_opacity_->setOpacity(value.toReal());icon_opacity_->setOpacity(value.toReal());});
        phone_=new PhoneServer([this]{return mobileState();},[this](const QJsonObject& request){return mobileUpdate(request);},this);if(c.phone_enabled)phone_->start();
        phone_->onNetworkChanged([this]{updatePhoneDetails();});
        notice_timer_.setSingleShot(true);notice_timer_.setInterval(8000);connect(&notice_timer_,&QTimer::timeout,this,[this]{notice_bar_->hide();});
        buildPreferences();
        if(c.phone_enabled&&!phone_->listening()){showNotice(text(UiText::CouldNotStartPhoneServer),true,true);if(notice_bar_)notice_bar_->setProperty("phoneError",true);}
        buildSettings();updateLanguage();applyStyle();buildTray();qApp->installEventFilter(this);
        auto* saveShortcut=new QShortcut(QKeySequence::Save,this);connect(saveShortcut,&QShortcut::activated,this,[this]{save(true);});
        auto* findShortcut=new QShortcut(QKeySequence::Find,this);connect(findShortcut,&QShortcut::activated,this,[this]{if(sidebar_collapsed_)setSidebarCollapsed(false,true,true);else{search_->setFocus(Qt::ShortcutFocusReason);search_->selectAll();}});
        auto* escapeShortcut=new QShortcut(QKeySequence(Qt::Key_Escape),this);connect(escapeShortcut,&QShortcut::activated,this,[this]{if(notice_bar_->isVisible()){notice_timer_.stop();notice_bar_->hide();}else if(pages_->currentWidget()==preferences_)showOperators();else if(!search_->text().isEmpty())search_->clear();});
        clock_.start();poll_.setInterval(8);connect(&poll_,&QTimer::timeout,this,[this]{pollKeys();});poll_.start();paint_.setInterval(50);connect(&paint_,&QTimer::timeout,this,[this]{if(isVisible()&&!isMinimized()){const auto current=mapFromGlobal(QCursor::pos());trackCursor(current);}});paint_.start();
    }

    void capture(const QString& file){grab().save(file);}
    void captureDocumentation(const QString& folder){
        if(!engine_.dry_run()||!QDir().mkpath(folder))return;
        capture(folder+"/desktop.png");showPreferences();chooseSettingsCategory(0);
        QTimer::singleShot(300,this,[this,folder]{capture(folder+"/settings.png");closeForTimeout();});
    }
    void writePhoneInfo(const QString& path) const {if(!engine_.dry_run())return;QFile file(path);if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(QJsonObject{{"port",int(phone_->port())},{"address",phone_->address()},{"token",phone_->token()},{"code",phone_->code()},{"pid",int(QCoreApplication::applicationPid())},{"dryRun",true},{"interface",QStringLiteral("monocode")}}).toJson());}
    void closeUsingControl(){close_->click();}
    void showNotice(const QString& message,bool error=false,bool persistent=false){
        if(!notice_bar_||!notice_)return;
        // Routine feedback must not erase a recovery notice or unresolved error.
        if(!error&&notice_bar_->isVisible()&&notice_bar_->property("persistent").toBool())return;
        notice_bar_->setProperty("saveError",false);notice_bar_->setProperty("phoneError",false);
        notice_bar_->setProperty("persistent",persistent||error);
        notice_bar_->setProperty("error",error);notice_->setText(message);styleNotice();
        notice_timer_.stop();layoutNotice();notice_scroll_->verticalScrollBar()->setValue(0);notice_bar_->show();notice_bar_->raise();
        if(!persistent&&!error){notice_timer_.setInterval(5000);notice_timer_.start();}
    }
    // A nonmodal card stays centered inside Azec without taking keyboard focus.
    void layoutNotice(){
        if(!notice_bar_)return;
        const int width=qMin(400,this->width()-40);
        const int bodyHeight=qMax(40,notice_->heightForWidth(width-64));
        notice_scroll_->setFixedHeight(qMin(bodyHeight,qMax(60,this->height()-280)));
        notice_bar_->setFixedWidth(width);notice_bar_->adjustSize();
        notice_bar_->move((this->width()-width)/2,(this->height()-notice_bar_->height())/2);
        notice_bar_->raise();
    }
    void setStartupNotice(const QString& message,bool blockSaving){
        save_blocked_=blockSaving;
        updateSavePresentation(save_->property("saveState").toString());
        if(!message.isEmpty())showNotice(message,blockSaving,true);
        else if(!uniqueHotkeys(engine_.snapshot()))showNotice(text(UiText::ConflictingHotkeys),true,false);
    }
    void closeForTimeout(){if(engine_.dry_run())force_close_=true;close();}
#ifdef AZEC_UI_TESTS

    void finishVerification(){force_close_=true;close();}
    QJsonObject selfTest(const QString& output);
#endif

};

#include "desktop_layout.hpp"
