#pragma once
// Qt presentation; recovered input behavior stays in engine.cpp and core.cpp.
#include "engine.hpp"
#include "ui_widgets.hpp"
#include "phone_server.hpp"
#include "app_style.hpp"
#include "pairing_qr.hpp"
#include "hotkeys.hpp"
#include "strings.hpp"
#include "flags.hpp"
#include "diagnostics.hpp"
#include <windows.h>
#include <dwmapi.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSaveFile>
#include <QSet>
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
    QListWidget* operators_=nullptr;QListWidget* favorites_list_=nullptr;QLineEdit* search_=nullptr;
    QLabel *favorites_title_=nullptr,*preferences_title_=nullptr;QWidget* favorites_group_=nullptr;
    IconButton* back_=nullptr;QSet<QString> favorites_;
    PageStack* pages_=nullptr;QWidget* operator_page_=nullptr;
    RaisedPanel* panel_=nullptr;QScrollArea* sidebar_scroll_=nullptr;
    QVariantAnimation operator_transition_;
    FadeLabel *name_opacity_=nullptr,*icon_opacity_=nullptr;
    QLabel *operators_title_=nullptr,*operator_name_=nullptr,*operator_icon_=nullptr,*subtitle_=nullptr;
    QPushButton* status_=nullptr;IconButton *save_=nullptr,*close_=nullptr,*gear_=nullptr;
    QWidget* preferences_=nullptr;QScrollArea* preferences_scroll_=nullptr;
    QScrollArea* scroll_=nullptr;QWidget* settings_=nullptr;QVBoxLayout* cards_=nullptr;
    Section *appearance_=nullptr,*system_=nullptr;
    Toggle *recoil_master_=nullptr,*rapid_master_=nullptr,*humanize_=nullptr,*operator_rapid_=nullptr,*dual_=nullptr;
    ExpandingContent* weapon_selector_=nullptr;WeaponButton *primary_=nullptr,*secondary_=nullptr;
    PhoneServer* phone_=nullptr;
    QPointer<PairingQr> pairing_qr_;
    QPointer<QLabel> phone_address_;
    QPointer<ExpandingContent> phone_details_;
    QPointer<Toggle> phone_enabled_;
    QLabel* notice_=nullptr;
    QSystemTrayIcon* tray_=nullptr;
    QMenu* tray_menu_=nullptr;
    QAction *tray_window_=nullptr,*tray_recoil_=nullptr,*tray_rapid_=nullptr,*tray_exit_=nullptr;
    bool unsaved_=false,force_close_=false,close_prompt_=false,save_blocked_=false;
    QPoint last_cursor_{-10000,-10000};
    QDoubleSpinBox *vertical_=nullptr,*horizontal_=nullptr;QSpinBox* delay_=nullptr;
    QSlider *vertical_slider_=nullptr,*horizontal_slider_=nullptr;QComboBox* language_=nullptr;QPointer<QDialog> palestine_popup_;
    std::array<QPushButton*,6> key_buttons_{};
    QTimer poll_,paint_,save_feedback_;QElapsedTimer clock_;
    bool was_menu_=false,was_recoil_=false,was_rapid_=false,was_primary_=false,was_secondary_=false,refreshing_=false,native_ready_=false;
    int binding_=0,release_key_=0,paint_count_=0;qint64 blocked_until_=0,last_tune_=0;
    QStringList icon_failures_;
    QColor accent() const {const auto c=engine_.snapshot();return QColor::fromRgbF(c.neon[0],c.neon[1],c.neon[2]);}
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
        tray_rapid_->setText(text(UiText::GlobalRapidFire));tray_rapid_->setChecked(engine_.rapid_master);
        tray_exit_->setText(text(UiText::Exit));tray_->setToolTip("Azec");
    }
    void buildTray(){
        tray_=new QSystemTrayIcon(QIcon(":/ui/app-icon.svg"),this);tray_menu_=new QMenu(this);
        tray_window_=tray_menu_->addAction(QString());tray_recoil_=tray_menu_->addAction(QString());tray_recoil_->setCheckable(true);
        tray_rapid_=tray_menu_->addAction(QString());tray_rapid_->setCheckable(true);tray_menu_->addSeparator();tray_exit_=tray_menu_->addAction(QString());
        tray_->setContextMenu(tray_menu_);refreshTray();
        connect(tray_menu_,&QMenu::aboutToShow,this,[this]{refreshTray();});
        connect(tray_window_,&QAction::triggered,this,[this]{if(isVisible())hide();else{showNormal();raise();activateWindow();}});
        connect(tray_recoil_,&QAction::triggered,this,[this]{toggleRecoil();});
        connect(tray_rapid_,&QAction::triggered,this,[this]{engine_.set_rapid_master(!engine_.rapid_master);dirty();updateStatus();});
        connect(tray_exit_,&QAction::triggered,this,[this]{close();});
        connect(tray_,&QSystemTrayIcon::activated,this,[this](QSystemTrayIcon::ActivationReason reason){if(reason==QSystemTrayIcon::Trigger||reason==QSystemTrayIcon::DoubleClick){showNormal();raise();activateWindow();}});
        if(!engine_.dry_run()&&QSystemTrayIcon::isSystemTrayAvailable())tray_->show();
    }
    void updateSavePresentation(const QString& state){
        if(state=="saved")unsaved_=false;
        save_->setProperty("unsaved",unsaved_);
        save_->setProperty("saveState",state);
        save_->setToolTip(state=="dirty"?text(UiText::SaveConfigurationUnsavedChanges):state=="saved"?text(UiText::ConfigurationSaved):state=="error"?text(UiText::CouldNotSaveConfiguration):text(UiText::SaveConfiguration));
        save_->update();
    }
    void dirty(){if(!refreshing_){unsaved_=true;save_feedback_.stop();updateSavePresentation("dirty");update();}}
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
        QStringList names(favorites_.begin(),favorites_.end());names.sort(Qt::CaseInsensitive);
        QSaveFile file(QString::fromStdWString(favoritesPath().wstring()));
        if(!file.open(QIODevice::WriteOnly)){diagnostic("save favorites",file.errorString());return false;}
        const auto bytes=QJsonDocument(QJsonObject{{"version",1},{"favorites",QJsonArray::fromStringList(names)}}).toJson();
        const bool ok=file.write(bytes)==bytes.size()&&file.commit();if(!ok)diagnostic("save favorites",file.errorString());return ok;
    }
    void filterOperators(){
        const auto term=search_->text();int shown=0;
        for(auto* list:{operators_,favorites_list_}){int count=0;for(int row=0;row<list->count();++row){auto* item=list->item(row);const bool match=item->text().contains(term,Qt::CaseInsensitive)&&(list==favorites_list_||!favorites_.contains(item->text()));item->setHidden(!match);if(match)++count;}list->setFixedHeight(count*operatorRowHeight);if(list==favorites_list_)shown=count;}
        favorites_group_->setVisible(shown>0);favorites_list_->setVisible(shown>0);
    }
    void refreshFavorites(){
        QSignalBlocker blocked(favorites_list_);favorites_list_->clear();const auto c=engine_.snapshot();
        for(int i=0;i<operators_->count();++i){auto* original=operators_->item(i);const bool favorite=favorites_.contains(original->text());original->setData(Qt::UserRole+1,favorite);
            if(favorite){auto* item=new QListWidgetItem(original->icon(),original->text());item->setData(Qt::UserRole,i);item->setData(Qt::UserRole+1,true);favorites_list_->addItem(item);if(i==c.selected)favorites_list_->setCurrentItem(item);}}
        operators_->viewport()->update();filterOperators();
    }
    void setFavorite(int id,bool enabled){
        if(id<0||id>=r6::operator_count)return;const auto name=QString::fromUtf8(engine_.snapshot().operators[id].name);const auto previous=favorites_;
        if(enabled)favorites_.insert(name);else favorites_.remove(name);
        if(!persistFavorites()){favorites_=previous;updateSavePresentation("error");save_->setToolTip(text(UiText::CouldNotSaveFavorites));}
        refreshFavorites();
    }
    void showOperatorMenu(QListWidget* list,const QPoint& position){
        auto* item=list->itemAt(position);if(!item)return;const int id=item->data(Qt::UserRole).toInt();const bool favorite=favorites_.contains(item->text());
        FavoritePopup popup(favorite?text(UiText::RemoveFavorite):text(UiText::Favorite),this);
        if(popup.execAt(list->viewport()->mapToGlobal(position)))setFavorite(id,!favorite);
    }
    void stopTransition(){operator_transition_.stop();name_opacity_->setOpacity(1);icon_opacity_->setOpacity(1);}
    void showOperators(){if(pairing_qr_)pairing_qr_->conceal();stopTransition();pages_->switchTo(operator_page_);gear_->setProperty("activePage",false);gear_->style()->unpolish(gear_);gear_->style()->polish(gear_);}
    void selectOperator(int id){
        if(id<0||id>=r6::operator_count)return;const bool changed=engine_.snapshot().selected!=id;
        if(!changed&&pages_->currentWidget()==operator_page_)return;
        const bool animate=changed&&operator_page_->isVisible();stopTransition();
        engine_.edit([=](r6::Config& c){c.selected=id;if(changed)c.weapon_slot=0;});syncSelection();
        {QSignalBlocker blocked(operators_);operators_->setCurrentRow(id);}
        {QSignalBlocker blocked(favorites_list_);favorites_list_->setCurrentRow(-1);for(int i=0;i<favorites_list_->count();++i)if(favorites_list_->item(i)->data(Qt::UserRole).toInt()==id)favorites_list_->setCurrentRow(i);}
        if(pages_->currentWidget()!=operator_page_)showOperators();
        if(changed)dirty();
        if(animate){name_opacity_->setOpacity(.35);icon_opacity_->setOpacity(.35);operator_transition_.start();}
    }
    QLabel* label(const QString& text,const char* role="rowTitle"){auto* result=new QLabel(text);result->setObjectName(role);return result;}
    QHBoxLayout* settingRow(Section* section,const QString& title,const QString& description={},int height=44){
        auto* widget=new QWidget;widget->setObjectName("settingRow");widget->setMinimumHeight(description.isEmpty()?height:56);auto* row=new QHBoxLayout(widget);row->setContentsMargins(14,8,12,8);row->setSpacing(12);
        auto* text=new QWidget;auto* column=new QVBoxLayout(text);column->setContentsMargins(0,0,0,0);column->setSpacing(2);column->addWidget(label(title));
        if(!description.isEmpty()){auto* hint=label(description,"rowHint");hint->setWordWrap(true);column->addWidget(hint);widget->setToolTip(description);}
        row->addWidget(text,1);section->body->addWidget(widget);return row;
    }
    Toggle* toggle(QHBoxLayout* row,const QString& title,bool checked,const std::function<void(bool)>& callback){
        auto* result=new Toggle([this]{return accent();});const auto* rowLabel=row->parentWidget()->findChild<QLabel*>();result->setAccessibleName(rowLabel?rowLabel->text():title);result->setChecked(checked);row->addWidget(result,0,Qt::AlignVCenter);
        auto* surface=row->parentWidget();surface->setProperty("enabledSetting",checked);connect(result,&QAbstractButton::toggled,surface,[surface](bool on){surface->setProperty("enabledSetting",on);surface->style()->unpolish(surface);surface->style()->polish(surface);});
        connect(result,&QAbstractButton::toggled,this,[this,callback](bool on){if(!refreshing_)callback(on);});return result;
    }
    void amount(Section* section,const QString& title,bool vertical){
        auto* row=settingRow(section,title,{},44);row->itemAt(0)->widget()->setFixedWidth(144);
        auto* slider=new QSlider(Qt::Horizontal);slider->setRange(vertical?0:-500,vertical?1000:500);slider->setAccessibleName(title);
        auto* value=new QDoubleSpinBox;value->setDecimals(1);value->setSingleStep(.1);value->setRange(vertical?0:-50,vertical?100:50);value->setButtonSymbols(QAbstractSpinBox::NoButtons);value->setFixedWidth(65);value->setAlignment(Qt::AlignCenter);value->setAccessibleName(title+text(UiText::Value));
        row->addWidget(slider,1);row->addWidget(value);if(vertical){vertical_=value;vertical_slider_=slider;}else{horizontal_=value;horizontal_slider_=slider;}
        connect(slider,&QSlider::valueChanged,value,[value](int n){value->setValue(n/10.0);});
        connect(value,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[=](double n){QSignalBlocker blocked(slider);slider->setValue(qRound(n*10));if(!refreshing_)editOperator([=](r6::Operator& op){if(vertical)op.vertical=float(n);else op.horizontal=float(n);});});
    }
    void applyStyle(){setStyleSheet(applicationStyle(accent()));operators_->viewport()->update();favorites_list_->viewport()->update();update();}
    void buildSettings(){
        refreshing_=true;const auto c=engine_.snapshot();if(settings_){scroll_->takeWidget();delete settings_;}
        settings_=new QWidget;settings_->setObjectName("settingsContent");cards_=new QVBoxLayout(settings_);cards_->setContentsMargins(2,0,10,12);cards_->setSpacing(10);
        auto* weapons=new Section(text(UiText::WeaponProfiles),false,true,kbtheme::yellow);weapons->body->setSpacing(0);cards_->addWidget(weapons);
        dual_=toggle(settingRow(weapons,text(UiText::SeparateSecondaryProfile),text(UiText::Use1And2ToSwitchWeaponProfilesChangeTheKeysInSettings)),"Separate secondary profile",c.secondary_enabled[c.selected],[this](bool on){engine_.edit([&](r6::Config& c){c.enable_secondary(c.selected,on);c.weapon_slot=on?1:0;});dirty();syncSelection();});
        auto* weaponChoices=new QWidget;auto* choices=new QHBoxLayout(weaponChoices);choices->setContentsMargins(0,0,0,0);choices->setSpacing(6);
        primary_=new WeaponButton([this]{return accent();});secondary_=new WeaponButton([this]{return accent();});for(auto* button:{primary_,secondary_})choices->addWidget(button);choices->addStretch();auto* weaponBox=new QWidget;auto* weaponBoxLayout=new QVBoxLayout(weaponBox);weaponBoxLayout->setContentsMargins(0,8,0,0);weaponBoxLayout->setSpacing(0);weaponBoxLayout->addWidget(weaponChoices);weapon_selector_=new ExpandingContent(weaponBox);weapons->body->addWidget(weapon_selector_);
        connect(primary_,&QPushButton::clicked,this,[this]{chooseWeapon(0);});connect(secondary_,&QPushButton::clicked,this,[this]{chooseWeapon(1);});
        auto* recoil=new Section(text(UiText::RecoilControl),false,true,kbtheme::blue);cards_->addWidget(recoil);
        recoil_master_=toggle(settingRow(recoil,text(UiText::MasterEnable),text(UiText::ApplyThisOperatorSRecoilSettings)),"Master recoil enable",engine_.recoil_master,[this](bool on){engine_.recoil_master=on;});
        amount(recoil,text(UiText::VerticalForce),true);amount(recoil,text(UiText::HorizontalAdjust),false);
        humanize_=toggle(settingRow(recoil,text(UiText::HumanizeMovement),text(UiText::VaryMovementSlightlyOnBothAxes)),"Humanize movement",false,[this](bool on){editOperator([=](r6::Operator& op){op.humanize=on;});});
        auto* rapid=new Section(text(UiText::RapidFire),false,true,kbtheme::pink);cards_->addWidget(rapid);
        rapid_master_=toggle(settingRow(rapid,text(UiText::GlobalRapidFire),text(UiText::MasterControlForEveryOperator)),"Global rapid fire",engine_.rapid_master,[this](bool on){engine_.set_rapid_master(on);dirty();});
        operator_rapid_=toggle(settingRow(rapid,text(UiText::EnableForThisOperator)),"Operator rapid fire",false,[this](bool on){editOperator([=](r6::Operator& op){op.rapid=on;});delay_->setEnabled(on);});
        auto* delayRow=settingRow(rapid,text(UiText::ShotDelay),text(UiText::DelayBetweenRepeatedShots));
        delay_=new QSpinBox;delay_->setRange(30,150);delay_->setSuffix(text(UiText::Ms));delay_->setButtonSymbols(QAbstractSpinBox::NoButtons);delay_->setAlignment(Qt::AlignCenter);delay_->setFixedWidth(85);delay_->setAccessibleName(text(UiText::ShotDelay));delayRow->addWidget(delay_);
        connect(delay_,qOverload<int>(&QSpinBox::valueChanged),this,[this](int n){if(!refreshing_)editOperator([=](r6::Operator& op){op.delay=n;});});
        cards_->addStretch();scroll_->setWidget(settings_);refreshing_=false;syncSelection();
    }
    void buildPreferences(){
        if(!preferences_)return;
        refreshing_=true;const auto c=engine_.snapshot();
        delete preferences_scroll_->takeWidget();auto* content=new QWidget;
        auto* preferencesLayout=new QVBoxLayout(content);preferencesLayout->setContentsMargins(2,0,10,0);preferencesLayout->setSpacing(14);
        auto* system=new Section(text(UiText::SystemSettings),false,true,kbtheme::lime);system_=system;preferencesLayout->addWidget(system);
        toggle(settingRow(system,text(UiText::FeedbackSound),text(UiText::BeepWhenHotkeyStatesChange)),"Feedback sound",c.sound,[this](bool on){engine_.edit([=](r6::Config& c){c.sound=on;});dirty();});
        auto* langRow=settingRow(system,text(UiText::Language));language_=new QComboBox;language_->setAccessibleName(text(UiText::Language));language_->setIconSize(QSize(22,15));
        for(const auto& entry:languageEntries())language_->addItem(languageFlag(entry.value),QString::fromUtf8(entry.label),entry.value);
        language_->setCurrentIndex(qMax(0,language_->findData(c.language)));langRow->addWidget(language_);
        connect(language_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int index){const int value=language_->itemData(index).toInt();engine_.edit([=](r6::Config& c){c.language=value;});dirty();if(value==6)QTimer::singleShot(0,this,[this]{showPalestineSurprise();});QTimer::singleShot(0,this,[this]{buildPreferences();buildSettings();updateLanguage();});});
        const QString titles[]={text(UiText::OpenHideWindow),text(UiText::RecoilToggle),text(UiText::RapidFireToggle),text(UiText::PanicExitKey),text(UiText::PrimaryWeapon),text(UiText::SecondaryWeapon)};const int keys[]={c.menu_key,c.recoil_key,c.rapid_key,c.panic_key,c.primary_key,c.secondary_key};
        for(int i=0;i<6;++i){auto* row=settingRow(system,titles[i],{},44);auto* button=new QPushButton(bindingName(keys[i]));button->setObjectName("keyButton");button->setFixedWidth(108);button->setAccessibleName(titles[i]);row->addWidget(button);key_buttons_[i]=button;connect(button,&QPushButton::clicked,this,[this,i]{bindKey(i);});}
        auto* phoneSection=new Section(text(UiText::PhoneControl),false,true,kbtheme::yellow);phoneSection->body->setSpacing(0);preferencesLayout->addWidget(phoneSection);
        phone_enabled_=toggle(settingRow(phoneSection,text(UiText::EnablePhoneAccess)),"Enable phone access",c.phone_enabled,[this](bool enabled){
            engine_.edit([&](r6::Config& c){c.phone_enabled=enabled;});
            if(pairing_qr_)pairing_qr_->conceal();
            if(enabled)phone_->start();else phone_->stop();
            updatePhoneDetails();phone_details_->setExpanded(enabled);dirty();
        });
        phone_enabled_->parentWidget()->setFixedHeight(44);
        {
            auto* connection=new QWidget;auto* connectionLayout=new QVBoxLayout(connection);connectionLayout->setContentsMargins(15,13,12,8);connectionLayout->setSpacing(7);
            auto* address=label(phone_->listening()?phone_->address():text(UiText::CouldNotStartPhoneServer),"phoneAddress");phone_address_=address;address->setTextInteractionFlags(Qt::TextSelectableByMouse);connectionLayout->addWidget(address);
            auto* pair=new QHBoxLayout;pair->addWidget(label(text(UiText::PairingCode)+phone_->code(),"hint"));pair->addStretch();auto* copy=new QPushButton(text(UiText::CopyLink));copy->setObjectName("secondary");pair->addWidget(copy);connect(copy,&QPushButton::clicked,this,[this,copy]{QApplication::clipboard()->setText(phone_->pairingLink());copy->setText(text(UiText::Copied));});connectionLayout->addLayout(pair);auto* note=label(text(UiText::OpenThisAddressInSafariOnTheSameWiFi),"hint");note->setWordWrap(true);connectionLayout->addWidget(note);
            pairing_qr_=new PairingQr(text(UiText::ClickToRevealQR),text(UiText::ClickToHideQR));
            pairing_qr_->setEnabled(phone_->listening());pairing_qr_->setPairingLink(phone_->listening()?phone_->pairingLink():QString());
            connectionLayout->addWidget(pairing_qr_,0,Qt::AlignLeft);
            phone_details_=new ExpandingContent(connection,300);phoneSection->body->addWidget(phone_details_);phone_details_->setExpanded(c.phone_enabled,false);
        }
        auto* appearance=new Section(text(UiText::Appearance),false,true,kbtheme::blue);appearance_=appearance;preferencesLayout->addWidget(appearance);
        auto* colorRow=settingRow(appearance,text(UiText::AccentColor),text(UiText::UsedForSwitchesAndSelectedStates));auto* color=new QPushButton(accent().name().toUpper());color->setObjectName("secondary");color->setAccessibleName(text(UiText::AccentColor));colorRow->addWidget(color);
        connect(color,&QPushButton::clicked,this,[this,color]{const auto next=chooseColor();if(next.isValid()){engine_.edit([=](r6::Config& c){c.neon[0]=next.redF();c.neon[1]=next.greenF();c.neon[2]=next.blueF();});color->setText(next.name().toUpper());applyStyle();dirty();}});
        auto* radiusRow=settingRow(appearance,text(UiText::ReactiveRadius),text(UiText::MouseResponseInTheBackground));auto* radius=new QSlider(Qt::Horizontal);radius->setRange(50,400);radius->setValue(qRound(c.radius));radius->setAccessibleName(text(UiText::ReactiveRadius));radiusRow->addWidget(radius,1);connect(radius,&QSlider::valueChanged,this,[this](int n){engine_.edit([=](r6::Config& c){c.radius=float(n);});dirty();});
        auto* reset=new QPushButton(text(UiText::ResetAppearance));reset->setObjectName("secondary");appearance->body->addWidget(reset,0,Qt::AlignLeft);connect(reset,&QPushButton::clicked,this,[this]{engine_.edit([](r6::Config& c){c.neon[0]=114.f/255;c.neon[1]=72.f/255;c.neon[2]=215.f/255;c.radius=130;});applyStyle();dirty();QTimer::singleShot(0,this,[this]{buildPreferences();});});
        auto* guide=new QPushButton(text(UiText::QuickGuide));guide->setObjectName("secondary");preferencesLayout->addWidget(guide,0,Qt::AlignLeft);connect(guide,&QPushButton::clicked,this,[this]{help();});
        auto* transfer=new Section(text(UiText::ProfileBackup),false,true,kbtheme::pink);preferencesLayout->addWidget(transfer);
        auto* transfers=new QHBoxLayout;auto* exportButton=new QPushButton(text(UiText::ExportProfiles));auto* importButton=new QPushButton(text(UiText::ImportProfiles));
        for(auto* button:{exportButton,importButton}){button->setObjectName("secondary");transfers->addWidget(button);}transfer->body->addLayout(transfers);
        connect(exportButton,&QPushButton::clicked,this,[this]{exportProfiles();});connect(importButton,&QPushButton::clicked,this,[this]{importProfiles();});
        preferencesLayout->addStretch();preferences_scroll_->setWidget(content);refreshing_=false;
    }
    void updatePhoneDetails(){
        if(!engine_.snapshot().phone_enabled){if(pairing_qr_){pairing_qr_->conceal();pairing_qr_->setEnabled(false);}return;}
        if(phone_address_)phone_address_->setText(phone_->listening()?phone_->address():text(UiText::CouldNotStartPhoneServer));
        if(pairing_qr_){pairing_qr_->setPairingLink(phone_->listening()?phone_->pairingLink():QString());pairing_qr_->setEnabled(phone_->listening());}
    }
    void showPreferences(){
        if(pages_->currentWidget()==preferences_){showOperators();return;}
        stopTransition();pages_->switchTo(preferences_);gear_->setProperty("activePage",true);gear_->style()->unpolish(gear_);gear_->style()->polish(gear_);
    }
    void updateLanguage(){
        operators_title_->setText(text(UiText::AllOperators));favorites_title_->setText(text(UiText::Favorites));search_->setPlaceholderText(text(UiText::SearchOperators));subtitle_->setText(text(UiText::OperatorSettings));
        updateSavePresentation(save_->property("saveState").toString());gear_->setToolTip(text(UiText::Settings));preferences_title_->setText(text(UiText::Settings));back_->setToolTip(text(UiText::BackToOperators));filterOperators();syncSelection();updateStatus();
    }
    void syncSelection(){
        const auto c=engine_.snapshot();const auto& op=c.active_operator();refreshing_=true;operator_name_->setText(QString::fromUtf8(op.name));operator_icon_->setPixmap(operatorPixmap(QString::fromUtf8(op.name),44,devicePixelRatioF()));
        dual_->syncChecked(c.secondary_enabled[c.selected]);weapon_selector_->setExpanded(c.secondary_enabled[c.selected]);
        primary_->setText(text(UiText::Primary));secondary_->setText(text(UiText::Secondary));
        primary_->setChosen(c.active_slot()==0);secondary_->setChosen(c.active_slot()==1);
        subtitle_->setText(c.secondary_enabled[c.selected]?(c.active_slot()?text(UiText::SecondaryWeapon):text(UiText::PrimaryWeapon)):text(UiText::OperatorSettings));
        vertical_->setValue(op.vertical);horizontal_->setValue(op.horizontal);vertical_slider_->setValue(qRound(op.vertical*10));horizontal_slider_->setValue(qRound(op.horizontal*10));humanize_->syncChecked(op.humanize);operator_rapid_->syncChecked(op.rapid);delay_->setValue(std::clamp(op.delay,30,150));delay_->setEnabled(op.rapid);for(auto* control:{humanize_,operator_rapid_,dual_}){auto* surface=control->parentWidget();surface->setProperty("enabledSetting",control->isChecked());surface->style()->unpolish(surface);surface->style()->polish(surface);}refreshing_=false;
    }
    void chooseWeapon(int slot){if(engine_.select_weapon(slot)){stopTransition();syncSelection();}}
    void handleWeaponKeys(bool primary,bool secondary,bool editing,bool modified){
        if(!editing&&!modified){if(primary&&!was_primary_)chooseWeapon(0);if(secondary&&!was_secondary_)chooseWeapon(1);}was_primary_=primary;was_secondary_=secondary;
    }
    void updateStatus(){status_->setText(engine_.recoil_active?text(UiText::RecoilOn):text(UiText::RecoilOff));status_->setProperty("active",engine_.recoil_active.load());status_->style()->unpolish(status_);status_->style()->polish(status_);recoil_master_->syncChecked(engine_.recoil_master);rapid_master_->syncChecked(engine_.rapid_master);for(auto* control:{recoil_master_,rapid_master_}){auto* surface=control->parentWidget();surface->setProperty("enabledSetting",control->isChecked());surface->style()->unpolish(surface);surface->style()->polish(surface);}}
    void feedback(bool on,int onFrequency,int offFrequency){if(!engine_.dry_run()&&engine_.snapshot().sound)Beep(on?onFrequency:offFrequency,50);}
    void toggleRecoil(){engine_.recoil_active=!engine_.recoil_active;feedback(engine_.recoil_active,600,300);updateStatus();}
    void pollKeys(){
        if(!binding_&&keyDown(engine_.snapshot().panic_key)){force_close_=true;engine_.stop();qApp->quit();return;}
        engine_.menu_visible=isVisible()&&!isMinimized();if(binding_||QApplication::activeModalWidget()||QApplication::activePopupWidget())return;
        if(release_key_){if(keyDown(release_key_)||clock_.elapsed()<blocked_until_)return;release_key_=0;was_menu_=was_recoil_=was_rapid_=false;}
        const auto c=engine_.snapshot();if(keyDown(c.panic_key)){force_close_=true;close();return;}
        const bool menu=keyDown(c.menu_key),recoil=keyDown(c.recoil_key),rapid=keyDown(c.rapid_key);
        if(menu&&!was_menu_){if(isVisible())hide();else{showNormal();raise();activateWindow();}engine_.menu_visible=isVisible();}
        if(recoil&&!was_recoil_)toggleRecoil();if(rapid&&!was_rapid_){engine_.set_rapid_master(!engine_.rapid_master);feedback(engine_.rapid_master,1000,500);updateStatus();dirty();}
        was_menu_=menu;was_recoil_=recoil;was_rapid_=rapid;
        auto* focused=QApplication::focusWidget();const bool editing=isActiveWindow()&&(qobject_cast<QLineEdit*>(focused)||qobject_cast<QAbstractSpinBox*>(focused)||qobject_cast<QComboBox*>(focused));
        handleWeaponKeys(keyDown(c.primary_key),keyDown(c.secondary_key),editing,keyDown(VK_CONTROL)||keyDown(VK_MENU)||keyDown(VK_LWIN)||keyDown(VK_RWIN));
        if(isVisible()&&isActiveWindow()&&pages_->currentWidget()==operator_page_&&!editing&&clock_.elapsed()-last_tune_>80){const float y=(keyDown(VK_UP)? .1f:0)-(keyDown(VK_DOWN)? .1f:0),x=(keyDown(VK_RIGHT)? .1f:0)-(keyDown(VK_LEFT)? .1f:0);if(x||y){editOperator([=](r6::Operator& op){op.vertical=std::clamp(op.vertical+y,0.f,100.f);op.horizontal=std::clamp(op.horizontal+x,-50.f,50.f);});syncSelection();last_tune_=clock_.elapsed();}}
    }
    bool save(){
        const bool saved=!save_blocked_&&engine_.save(config_path_);updateSavePresentation(saved?"saved":"error");
        if(saved)save_feedback_.start();else save_feedback_.stop();
        return saved;
    }
    QColor chooseColor(){
        QDialog dialog(this,Qt::Dialog|Qt::FramelessWindowHint);dialog.setWindowTitle(text(UiText::AccentColor));auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(20,14,20,18);
        auto* header=new QHBoxLayout;header->addWidget(label(text(UiText::AccentColor),"cardTitle"),1);auto* close=new QPushButton(QString::fromUtf8("×"));close->setObjectName("closeControl");close->setFixedSize(30,30);header->addWidget(close);layout->addLayout(header);connect(close,&QPushButton::clicked,&dialog,&QDialog::reject);
        auto* picker=new QColorDialog(accent(),&dialog);picker->setOption(QColorDialog::DontUseNativeDialog);picker->setOption(QColorDialog::NoButtons);picker->setWindowFlags(Qt::Widget);layout->addWidget(picker);
        auto* buttons=new QHBoxLayout;buttons->addStretch();auto* cancel=new QPushButton(text(UiText::Cancel));cancel->setObjectName("secondary");auto* apply=new QPushButton(text(UiText::ApplyColor));apply->setObjectName("save");buttons->addWidget(cancel);buttons->addWidget(apply);layout->addLayout(buttons);connect(cancel,&QPushButton::clicked,&dialog,&QDialog::reject);connect(apply,&QPushButton::clicked,&dialog,&QDialog::accept);
        dialog.adjustSize();roundWindow(&dialog);return dialog.exec()==QDialog::Accepted?picker->currentColor():QColor();
    }
    void bindKey(int index){
        binding_=index+1;QDialog dialog(this,Qt::Dialog|Qt::FramelessWindowHint);dialog.setFixedSize(360,205);dialog.setWindowTitle(text(UiText::KeyBinding));auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(22,18,22,20);
        auto* header=new QHBoxLayout;header->addWidget(label(text(UiText::SetAHotkey),"cardTitle"),1);auto* cancel=new IconButton(":/ui/close.svg",[this]{return accent();});cancel->setObjectName("closeControl");cancel->setAccessibleName(text(UiText::CancelHotkeyBinding));header->addWidget(cancel);layout->addLayout(header);connect(cancel,&QPushButton::clicked,&dialog,&QDialog::reject);
        auto* hint=label(text(UiText::ReleaseYourKeysWaiting));hint->setWordWrap(true);layout->addWidget(hint);layout->addWidget(label(text(UiText::EscCancelsBackspaceClears),"hint"));layout->addStretch();
        QElapsedTimer started;started.start();QTimer timer;timer.setInterval(16);connect(&timer,&QTimer::timeout,&dialog,[&]{if(started.elapsed()<1200)return;hint->setText(text(UiText::PressAKeyNow));for(int key=3;key<256;++key)if(keyDown(key)){if(key!=VK_ESCAPE){const auto existing=engine_.snapshot();if(key!=VK_BACK&&hotkeyConflict(existing,index,key)>=0){hint->setText(text(UiText::ThisKeyIsAlreadyAssignedChooseAnother));return;}engine_.edit([=](r6::Config& c){int* keys[]={&c.menu_key,&c.recoil_key,&c.rapid_key,&c.panic_key,&c.primary_key,&c.secondary_key};*keys[index]=key==VK_BACK?0:key;});dirty();save();}release_key_=key;blocked_until_=clock_.elapsed()+250;dialog.accept();break;}});
        timer.start();roundWindow(&dialog);dialog.exec();binding_=0;const auto c=engine_.snapshot();const int keys[]={c.menu_key,c.recoil_key,c.rapid_key,c.panic_key,c.primary_key,c.secondary_key};if(key_buttons_[index])key_buttons_[index]->setText(bindingName(keys[index]));syncSelection();updateStatus();
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
        auto* close=new QPushButton("Free Palestine");close->setObjectName("save");layout->addWidget(close,0,Qt::AlignCenter);connect(close,&QPushButton::clicked,popup,&QDialog::close);
        popup->setFixedWidth(340);popup->adjustSize();popup->move(mapToGlobal(rect().center())-popup->rect().center());roundWindow(popup);
        palestine_popup_=popup;popup->show();popup->raise();popup->activateWindow();
    }
    void help(){
        QDialog dialog(this,Qt::Dialog|Qt::FramelessWindowHint);dialog.setFixedSize(410,230);dialog.setWindowTitle(text(UiText::AzecHelp));auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(24,18,24,22);
        auto* header=new QHBoxLayout;auto* title=label("","cardTitle");header->addWidget(title,1);auto* close=new IconButton(":/ui/close.svg",[this]{return accent();});close->setObjectName("closeControl");close->setAccessibleName(text(UiText::CloseQuickGuide));header->addWidget(close);layout->addLayout(header);connect(close,&QPushButton::clicked,&dialog,&QDialog::accept);
        auto* description=label("");description->setWordWrap(true);layout->addWidget(description,1);auto* row=new QHBoxLayout;auto* back=new QPushButton(text(UiText::Back));back->setObjectName("secondary");auto* next=new QPushButton(text(UiText::Next));next->setObjectName("save");auto* count=label("","hint");row->addWidget(back);row->addWidget(count,1,Qt::AlignCenter);row->addWidget(next);layout->addLayout(row);
        const QStringList titles={text(UiText::Welcome),text(UiText::ChooseAnOperator),text(UiText::AdjustRecoil),text(UiText::RapidFire),text(UiText::Appearance),text(UiText::HotkeysAndSaving)};
        const QStringList descriptions={text(UiText::YourOperatorProfilesAndControlsInOnePlace),text(UiText::SelectANameInTheSidebarOrSearchForAnOperator),text(UiText::SetVerticalAndHorizontalForceUseArrowKeysForFineAdjustments),text(UiText::EnableTheGlobalControlThenEnableRapidFireForTheSelectedOperator),text(UiText::OpenTheSettingsGearToChangeTheAccentColorAndReactiveRadius),text(UiText::SetYourHotkeysInSystemSettingsSaveConfigurationToKeepYourChanges)};
        int step=0;const auto refresh=[&]{title->setText(titles[step]);description->setText(descriptions[step]);count->setText(QString::number(step+1)+" / 6");back->setEnabled(step>0);next->setText(step==5?text(UiText::Done):text(UiText::Next));};
        connect(back,&QPushButton::clicked,&dialog,[&]{if(step)--step;refresh();});connect(next,&QPushButton::clicked,&dialog,[&]{if(step==5)dialog.accept();else{++step;refresh();}});refresh();roundWindow(&dialog);dialog.exec();
    }
protected:
    void paintEvent(QPaintEvent*) override {
        ++paint_count_;QPainter p(this);p.fillRect(rect(),kbtheme::paper);const auto c=engine_.snapshot();const QPoint mouse=mapFromGlobal(QCursor::pos());QColor color=accent();p.setRenderHint(QPainter::Antialiasing);p.setPen(Qt::NoPen);
        static const QPixmap image("titio.png");if(!image.isNull()){p.setOpacity(.02);p.drawPixmap(rect(),image);p.setOpacity(1);}
        for(int x=18;x<width();x+=45)for(int y=90;y<height()-12;y+=45){const auto delta=QPoint(x,y)-mouse;const qreal distance=std::hypot(delta.x(),delta.y());QColor dot=color;dot.setAlpha(8);p.setBrush(dot);p.drawEllipse(QPointF(x,y),1.1,1.1);if(distance<c.radius){const qreal strength=1-distance/c.radius;dot.setAlpha(qRound(strength*15));p.setBrush(dot);p.drawEllipse(QPointF(x,y),1.5+strength*1.4,1.5+strength*1.4);}}
    }
    void showEvent(QShowEvent* event) override {
        QWidget::showEvent(event);engine_.menu_visible=true;if(!native_ready_){native_ready_=true;const HWND handle=HWND(winId());const auto style=GetWindowLongPtrW(handle,GWL_STYLE);SetWindowLongPtrW(handle,GWL_STYLE,style|WS_THICKFRAME|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX);SetWindowPos(handle,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);roundWindow(this);const BOOL dark=FALSE;DwmSetWindowAttribute(handle,20,&dark,sizeof(dark));}
    }
    void hideEvent(QHideEvent* event) override {if(pairing_qr_)pairing_qr_->conceal();engine_.menu_visible=false;QWidget::hideEvent(event);}
    void closeEvent(QCloseEvent* event) override {
        if(close_prompt_){event->ignore();return;}
        if(unsaved_&&!force_close_){
            close_prompt_=true;const bool recoil=engine_.recoil_active.exchange(false),rapid=engine_.rapid_master.exchange(false);
            QDialog dialog(this,Qt::Dialog|Qt::FramelessWindowHint);dialog.setObjectName("unsavedDialog");dialog.setWindowTitle(text(UiText::UnsavedChanges));dialog.setFixedWidth(390);
            auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(22,20,22,20);layout->setSpacing(18);
            layout->addWidget(label(text(UiText::SaveYourChangesBeforeClosing),"rowTitle"));
            auto* row=new QHBoxLayout;int choice=0;
            for(int i=0;i<3;++i){const QString titles[]={text(UiText::Cancel),text(UiText::Discard),text(UiText::Save)};auto* button=new QPushButton(titles[i]);button->setObjectName(i==2?"save":i==1?"discardChanges":"cancelClose");row->addWidget(button);connect(button,&QPushButton::clicked,&dialog,[&,i]{choice=i;dialog.accept();});}
            layout->addLayout(row);dialog.adjustSize();roundWindow(&dialog);dialog.exec();
            engine_.recoil_active=recoil;engine_.rapid_master=rapid;close_prompt_=false;
            if(choice==0||(choice==2&&!save())){event->ignore();return;}
        }
        if(tray_)tray_->hide();phone_->stop();engine_.stop();QWidget::closeEvent(event);
    }
    void resizeEvent(QResizeEvent* event) override {QWidget::resizeEvent(event);if(native_ready_)roundWindow(this);}
    bool eventFilter(QObject* object,QEvent* event) override {
        if(event->type()==QEvent::FocusIn||event->type()==QEvent::FocusOut||event->type()==QEvent::MouseButtonPress){
            if(auto* button=qobject_cast<QPushButton*>(object);button&&isAncestorOf(button)){
                const auto reason=event->type()==QEvent::MouseButtonPress?Qt::MouseFocusReason:static_cast<QFocusEvent*>(event)->reason();const bool keyboard=event->type()==QEvent::FocusIn&&(reason==Qt::TabFocusReason||reason==Qt::BacktabFocusReason||reason==Qt::ShortcutFocusReason);
                button->setProperty("keyboardFocus",keyboard);button->style()->unpolish(button);button->style()->polish(button);button->update();
            }
        }
        if(event->type()==QEvent::KeyPress){auto* key=static_cast<QKeyEvent*>(event);auto* widget=qobject_cast<QWidget*>(object);
            if(widget&&widget->window()==this&&pages_->currentWidget()==operator_page_&&(key->key()==Qt::Key_Up||key->key()==Qt::Key_Down||key->key()==Qt::Key_Left||key->key()==Qt::Key_Right)&&!qobject_cast<QLineEdit*>(widget)&&!qobject_cast<QAbstractSpinBox*>(widget))return true;}
        return QWidget::eventFilter(object,event);
    }
    bool nativeEvent(const QByteArray& type,void* message,qintptr* result) override {
        auto* msg=static_cast<MSG*>(message);if(msg->message==WM_NCCALCSIZE&&msg->wParam){*result=0;return true;}
        if(msg->message==WM_NCHITTEST){RECT r;GetWindowRect(HWND(winId()),&r);const int x=short(LOWORD(msg->lParam)),y=short(HIWORD(msg->lParam)),edge=qRound(6*devicePixelRatioF());const bool left=x<r.left+edge,right=x>=r.right-edge,top=y<r.top+edge,bottom=y>=r.bottom-edge;
            if(top&&left)*result=HTTOPLEFT;else if(top&&right)*result=HTTOPRIGHT;else if(bottom&&left)*result=HTBOTTOMLEFT;else if(bottom&&right)*result=HTBOTTOMRIGHT;else if(left)*result=HTLEFT;else if(right)*result=HTRIGHT;else if(top)*result=HTTOP;else if(bottom)*result=HTBOTTOM;else return QWidget::nativeEvent(type,message,result);return true;}return QWidget::nativeEvent(type,message,result);
    }
public:
    explicit MainWindow(r6::Engine& engine,std::filesystem::path config_path):engine_(engine),config_path_(std::move(config_path)){
        setObjectName("mainWindow");setWindowTitle("Azec");setWindowFlags(Qt::Window|Qt::FramelessWindowHint);setWindowIcon(operatorIcon("Ash"));const auto area=QGuiApplication::primaryScreen()->availableGeometry();setMinimumSize(std::min(760,area.width()-32),std::min(560,area.height()-32));resize(std::min(864,area.width()-48),std::min(650,area.height()-48));move(area.center()-rect().center());
        auto* layout=new QVBoxLayout(this);layout->setContentsMargins(0,0,0,0);layout->setSpacing(0);auto* header=new DragHeader;header->setObjectName("titleBar");header->setFixedHeight(46);auto* top=new QHBoxLayout(header);top->setContentsMargins(20,0,10,0);top->setSpacing(5);
        top->addWidget(new BrandLabel);top->addStretch();status_=new QPushButton;status_->setObjectName("status");status_->setToolTip(text(UiText::ToggleRecoilActivity));top->addWidget(status_);connect(status_,&QPushButton::clicked,this,[this]{toggleRecoil();});
        save_=new IconButton(":/ui/save.svg",[this]{return accent();});save_->setObjectName("saveControl");save_->setAccessibleName(text(UiText::SaveConfiguration));top->addWidget(save_);connect(save_,&QPushButton::clicked,this,[this]{save();});
        save_feedback_.setSingleShot(true);save_feedback_.setInterval(1600);connect(&save_feedback_,&QTimer::timeout,this,[this]{updateSavePresentation("clean");});
        gear_=new IconButton(":/ui/settings.svg",[this]{return accent();});gear_->setObjectName("windowControl");gear_->setAccessibleName(text(UiText::Settings));gear_->setToolTip(text(UiText::Settings));top->addWidget(gear_);connect(gear_,&QPushButton::clicked,this,[this]{showPreferences();});auto* minimize=new IconButton(":/ui/minimize.svg",[this]{return accent();});minimize->setObjectName("windowControl");minimize->setAccessibleName(text(UiText::Minimize));minimize->setToolTip(text(UiText::Minimize));top->addWidget(minimize);connect(minimize,&QPushButton::clicked,this,&QWidget::showMinimized);
        close_=new IconButton(":/ui/close.svg",[this]{return accent();});close_->setObjectName("closeControl");close_->setAccessibleName(text(UiText::Close));close_->setToolTip(text(UiText::Close));top->addWidget(close_);connect(close_,&QPushButton::clicked,this,&QWidget::close);layout->addWidget(header);
        notice_=new QLabel;notice_->setObjectName("startupNotice");notice_->setWordWrap(true);notice_->setContentsMargins(18,8,18,8);notice_->setStyleSheet("color:#31204C;background:#FFA8C9;font-weight:600;");notice_->hide();layout->addWidget(notice_);
        auto* body=new QWidget;auto* columns=new QHBoxLayout(body);columns->setContentsMargins(0,0,0,0);columns->setSpacing(0);
        auto* sidebar=new QWidget;sidebar->setObjectName("operatorSidebar");sidebar->setFixedWidth(200);auto* sidebarLayout=new QVBoxLayout(sidebar);sidebarLayout->setContentsMargins(10,18,2,12);sidebarLayout->setSpacing(8);
        search_=new QLineEdit;search_->setObjectName("search");search_->setClearButtonEnabled(true);search_->setAccessibleName(text(UiText::SearchOperators4c533));sidebarLayout->addWidget(search_);
        sidebar_scroll_=new QScrollArea;sidebar_scroll_->setObjectName("sidebarScroll");sidebar_scroll_->setWidgetResizable(true);sidebar_scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);sidebarLayout->addWidget(sidebar_scroll_,1);
        auto* sidebarContent=new QWidget;auto* groups=new QVBoxLayout(sidebarContent);groups->setContentsMargins(0,0,0,0);groups->setSpacing(8);
        const auto sectionHeader=[&](QVBoxLayout* column,QLabel* title,const char* dividerName){auto* row=new QHBoxLayout;row->setContentsMargins(10,8,8,0);row->setSpacing(8);row->addWidget(title);auto* line=new QFrame;line->setObjectName(dividerName);line->setFixedHeight(2);row->addWidget(line,1);column->addLayout(row);};
        favorites_group_=new QWidget;favorites_group_->setObjectName("favoritesGroup");auto* favoriteRows=new QVBoxLayout(favorites_group_);favoriteRows->setContentsMargins(0,0,0,0);favoriteRows->setSpacing(8);groups->addWidget(favorites_group_);
        favorites_title_=new StickerLabel("Favorites",kbtheme::yellow);sectionHeader(favoriteRows,favorites_title_,"favoritesDivider");
        favorites_list_=new OperatorList(sidebar_scroll_);favorites_list_->setObjectName("favorites");favorites_list_->setAccessibleName(text(UiText::FavoriteOperators));favoriteRows->addWidget(favorites_list_);
        operators_title_=new StickerLabel("All operators",kbtheme::blue);sectionHeader(groups,operators_title_,"operatorsDivider");
        operators_=new OperatorList(sidebar_scroll_);operators_->setObjectName("operators");operators_->setAccessibleName(text(UiText::AllOperators));groups->addWidget(operators_);groups->addStretch();sidebar_scroll_->setWidget(sidebarContent);FadingScrollBar::install(sidebar_scroll_);new ScrollFade(sidebar_scroll_,kbtheme::paper);new ScrollFade(sidebar_scroll_,kbtheme::paper,true);
        for(auto* list:{operators_,favorites_list_}){list->setMouseTracking(true);list->setUniformItemSizes(true);list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);list->setItemDelegate(new OperatorDelegate([this]{return accent();},list));list->setContextMenuPolicy(Qt::CustomContextMenu);connect(list,&QWidget::customContextMenuRequested,this,[this,list](const QPoint& point){showOperatorMenu(list,point);});}
        const auto c=engine_.snapshot();for(int i=0;i<r6::operator_count;++i){const QString name=QString::fromUtf8(c.operators[i].name);auto* item=new QListWidgetItem(operatorIcon(name),name);item->setData(Qt::UserRole,i);operators_->addItem(item);QSvgRenderer renderer(":/operators/"+name.toLower()+".svg");if(!renderer.isValid())icon_failures_<<name;}
        operators_->setCurrentRow(c.selected);columns->addWidget(sidebar);
        pages_=new PageStack;pages_->setObjectName("contentPages");operator_page_=new QWidget;operator_page_->setObjectName("operatorPage");auto* rightLayout=new QVBoxLayout(operator_page_);rightLayout->setContentsMargins(20,20,16,14);rightLayout->setSpacing(10);
        auto* selected=new QHBoxLayout;selected->setContentsMargins(2,0,0,0);selected->setSpacing(10);operator_icon_=new FadeLabel;operator_icon_->setFixedSize(46,46);selected->addWidget(operator_icon_);auto* titles=new QVBoxLayout;titles->setSpacing(2);operator_name_=new FadeLabel;operator_name_->setObjectName("pageTitle");subtitle_=label("","subtitle");titles->addWidget(operator_name_);titles->addWidget(subtitle_);selected->addLayout(titles,1);rightLayout->addLayout(selected);
        name_opacity_=static_cast<FadeLabel*>(operator_name_);icon_opacity_=static_cast<FadeLabel*>(operator_icon_);
        scroll_=new QScrollArea;scroll_->setWidgetResizable(true);scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);FadingScrollBar::install(scroll_);new ScrollFade(scroll_,kbtheme::surface);new ScrollFade(scroll_,kbtheme::surface,true);rightLayout->addWidget(scroll_,1);pages_->addWidget(operator_page_);
        preferences_=new QWidget;preferences_->setObjectName("preferencesPage");auto* preferenceLayout=new QVBoxLayout(preferences_);preferenceLayout->setContentsMargins(20,20,16,14);preferenceLayout->setSpacing(10);auto* preferenceHeader=new QHBoxLayout;preferenceHeader->setSpacing(10);
        back_=new IconButton(":/ui/back.svg",[this]{return accent();});back_->setObjectName("windowControl");back_->setAccessibleName(text(UiText::BackToOperators));connect(back_,&QPushButton::clicked,this,[this]{showOperators();});preferenceHeader->addWidget(back_);preferences_title_=label("Settings","pageTitle");preferenceHeader->addWidget(preferences_title_,1);preferenceLayout->addLayout(preferenceHeader);
        preferences_scroll_=new QScrollArea;preferences_scroll_->setWidgetResizable(true);preferences_scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);FadingScrollBar::install(preferences_scroll_);preferenceLayout->addWidget(preferences_scroll_,1);pages_->addWidget(preferences_);
        panel_=new RaisedPanel;panel_->setObjectName("raisedPanel");auto* panelLayout=new QVBoxLayout(panel_);panelLayout->setContentsMargins(8,3,12,13);panelLayout->setSpacing(0);panelLayout->addWidget(pages_);columns->addWidget(panel_,1);layout->addWidget(body,1);
        favorites_=readFavorites();refreshFavorites();
        connect(operators_,&QListWidget::currentItemChanged,this,[this](QListWidgetItem* item,QListWidgetItem*){if(item)selectOperator(item->data(Qt::UserRole).toInt());});connect(favorites_list_,&QListWidget::currentItemChanged,this,[this](QListWidgetItem* item,QListWidgetItem*){if(item)selectOperator(item->data(Qt::UserRole).toInt());});
        for(auto* list:{operators_,favorites_list_})connect(list,&QListWidget::itemClicked,this,[this](QListWidgetItem* item){if(item)selectOperator(item->data(Qt::UserRole).toInt());});
        connect(search_,&QLineEdit::textChanged,this,[this]{filterOperators();});
        operator_transition_.setDuration(120);operator_transition_.setStartValue(.35);operator_transition_.setEndValue(1.0);operator_transition_.setEasingCurve(QEasingCurve::InOutQuad);
        connect(&operator_transition_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){name_opacity_->setOpacity(value.toReal());icon_opacity_->setOpacity(value.toReal());});
        phone_=new PhoneServer([this]{return mobileState();},[this](const QJsonObject& request){return mobileUpdate(request);},this);if(c.phone_enabled)phone_->start();
        phone_->onNetworkChanged([this]{updatePhoneDetails();});
        buildPreferences();
        buildSettings();updateLanguage();applyStyle();buildTray();qApp->installEventFilter(this);clock_.start();poll_.setInterval(8);connect(&poll_,&QTimer::timeout,this,[this]{pollKeys();});poll_.start();paint_.setInterval(50);connect(&paint_,&QTimer::timeout,this,[this]{if(isVisible()&&!isMinimized()){const auto current=mapFromGlobal(QCursor::pos());if(current!=last_cursor_){last_cursor_=current;update();}}});paint_.start();
    }
    void capture(const QString& file){grab().save(file);}
    void writePhoneInfo(const QString& path) const {if(!engine_.dry_run())return;QFile file(path);if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(QJsonObject{{"port",int(phone_->port())},{"address",phone_->address()},{"token",phone_->token()},{"code",phone_->code()},{"pid",int(QCoreApplication::applicationPid())},{"dryRun",true}}).toJson());}
    void closeUsingControl(){close_->click();}
    void setStartupNotice(const QString& message,bool blockSaving){save_blocked_=blockSaving;if(!message.isEmpty()){notice_->setText(message);notice_->show();}else if(!uniqueHotkeys(engine_.snapshot())){notice_->setText(text(UiText::ConflictingHotkeys));notice_->show();}}
    void closeForTimeout(){if(engine_.dry_run())force_close_=true;close();}
#ifdef AZEC_UI_TESTS
    void finishVerification(){force_close_=true;close();}
    QJsonObject selfTest(const QString& output);
#endif

};
