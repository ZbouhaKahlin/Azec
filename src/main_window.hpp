#pragma once
// Qt presentation; recovered input behavior stays in engine.cpp and core.cpp.
#include "engine.hpp"
#include "profile_store.hpp"
#include "ui_widgets.hpp"
#include "phone_server.hpp"
#include "app_style.hpp"
#include "pairing_qr.hpp"
#include "hotkeys.hpp"
#include "logitech_script.hpp"
#include "driver_input.hpp"
#include "strings.hpp"
#include "flags.hpp"
#include "diagnostics.hpp"
#include "game_focus.hpp"
#include "sound.hpp"
#include "version.hpp"
#include "update_check.hpp"
#include "app_paths.hpp"
#include "onboarding_widgets.hpp"
#include <windows.h>
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
    QPointer<OnboardingPill> tour_pill_;QPointer<OnboardingArrow> tour_arrow_;
    QTimer tour_refresh_;int tour_step_=-1;bool tour_checked_=false;
    QPointer<Section> mouse_setup_;QPointer<QSpinBox> mouse_dpi_;
    QPointer<QDoubleSpinBox> mouse_horizontal_,mouse_vertical_,mouse_multiplier_;
    QPointer<QLabel> mouse_setup_status_;QPointer<QPushButton> mouse_rescale_,mouse_save_;
    std::optional<r6::SensitivitySetup> mouse_draft_;QString mouse_source_;bool mouse_setup_dirty_=false;
    void buildMouseSetup(QVBoxLayout* column);
    r6::SensitivitySetup mouseSetupInput() const;
    void updateMouseSetupControls();
    void readGameSensitivity(bool chooseFile=false);
    bool saveMouseSetup();
    void rescaleMouseProfiles();
    void maybeStartTour();
    void startTour();
    void setTourStep(int step);
    void advanceTour();
    void finishTour();
    void positionTour();
    void buildGoofyLayout(QVBoxLayout* layout);
    QListWidget* operators_=nullptr;QListWidget* favorites_list_=nullptr;QLineEdit* search_=nullptr;
    QLabel *favorites_title_=nullptr,*preferences_title_=nullptr;QWidget* favorites_group_=nullptr;
    IconButton* back_=nullptr;QSet<QString> favorites_;
    PageStack* pages_=nullptr;QWidget* operator_page_=nullptr;
    RaisedPanel* panel_=nullptr;QScrollArea* sidebar_scroll_=nullptr;
    QWidget *sidebar_=nullptr,*favorites_header_=nullptr,*favorites_heading_=nullptr,*operators_header_=nullptr,*sidebar_search_box_=nullptr,*settings_nav_=nullptr;
    QVector<SettingsNavButton*> settings_nav_buttons_;bool settings_sidebar_=false;
    IconButton *sidebar_toggle_=nullptr,*sidebar_search_=nullptr;StickerStarButton* sidebar_favorites_=nullptr;
    QVariantAnimation sidebar_animation_;bool sidebar_collapsed_=false,focus_sidebar_search_=false,sidebar_animating_=false,favorites_shown_=false;
    QVector<ThemeCard*> theme_cards_;QVector<QWidget*> preference_categories_;int preference_category_=0;
    static constexpr int mouse_setup_category_=5;
    QVariantAnimation operator_transition_;
    FadeLabel *name_opacity_=nullptr,*icon_opacity_=nullptr;
    QLabel *operators_title_=nullptr,*operator_name_=nullptr,*operator_icon_=nullptr,*subtitle_=nullptr;
    QPushButton* status_=nullptr;QPushButton* rapid_status_=nullptr;IconButton *save_=nullptr,*close_=nullptr,*gear_=nullptr;
    QWidget* preferences_=nullptr;QScrollArea* preferences_scroll_=nullptr;
    QScrollArea* scroll_=nullptr;QWidget* settings_=nullptr;QVBoxLayout* cards_=nullptr;
    Section *system_=nullptr;
    Toggle *rapid_active_=nullptr,*game_focus_only_=nullptr,*ads_only_=nullptr,*humanize_=nullptr,*dual_=nullptr;
    ExpandingContent* weapon_selector_=nullptr;WeaponButton *primary_=nullptr,*secondary_=nullptr;
    PhoneServer* phone_=nullptr;
    QPointer<PairingQr> pairing_qr_;
    QPointer<Toggle> logitech_output_;
    QPointer<Toggle> driver_output_;
    QPointer<QPushButton> logitech_export_;
    QPointer<QLabel> logitech_status_;
    QPointer<QLabel> phone_address_;
    QPointer<QLabel> phone_code_;
    QPointer<ExpandingContent> phone_details_;
    QPointer<Toggle> phone_enabled_;
    QPointer<QPushButton> copy_link_,copy_code_,phone_new_code_,phone_retry_;
    QPointer<QPushButton> update_button_;QNetworkAccessManager* network_=nullptr;bool update_in_progress_=false;
    QWidget* notice_bar_=nullptr;QLabel* notice_=nullptr;QPushButton* notice_close_=nullptr;
    QSystemTrayIcon* tray_=nullptr;
    QMenu* tray_menu_=nullptr;
    QAction *tray_window_=nullptr,*tray_recoil_=nullptr,*tray_rapid_=nullptr,*tray_exit_=nullptr;
    QDialog* close_alert_=nullptr;
    bool unsaved_=false,force_close_=false,close_prompt_=false,save_blocked_=false;
    QPoint last_cursor_{-10000,-10000};
    QDoubleSpinBox *vertical_=nullptr,*horizontal_=nullptr;QSpinBox* delay_=nullptr;
    QSlider *vertical_slider_=nullptr,*horizontal_slider_=nullptr,*delay_slider_=nullptr;QWidget* delay_controls_=nullptr;StepButton *delay_minus_=nullptr,*delay_plus_=nullptr;QComboBox* language_=nullptr;QPointer<QDialog> palestine_popup_;
    std::array<QPushButton*,5> key_buttons_{};
    QTimer poll_,paint_,save_feedback_,notice_timer_;QElapsedTimer clock_;
    bool was_menu_=false,was_recoil_=false,was_primary_=false,was_secondary_=false,refreshing_=false,native_ready_=false;
    int binding_=0,release_key_=0,paint_count_=0,cursor_repaints_=0;qint64 blocked_until_=0,last_tune_=0;
    QStringList icon_failures_;
    void trackCursor(const QPoint& current){if(current!=last_cursor_){last_cursor_=current;++cursor_repaints_;update();}}
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
        if(!persistFavorites()){favorites_=previous;updateSavePresentation("error");save_->setToolTip(text(UiText::CouldNotSaveFavorites));showNotice(text(UiText::CouldNotSaveFavorites),true,true);}
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
    QSlider* makeSlider(){return new QSlider(Qt::Horizontal);}
    void amount(Section* section,const QString& title,bool vertical){
        auto* slider=makeSlider();slider->setRange(vertical?0:-500,vertical?1000:500);slider->setSingleStep(1);slider->setPageStep(5);slider->setAccessibleName(title);
        auto* value=new QDoubleSpinBox;value->setDecimals(1);value->setSingleStep(.1);value->setRange(vertical?0:-50,vertical?100:50);value->setButtonSymbols(QAbstractSpinBox::NoButtons);value->setAccessibleName(title+text(UiText::Value));
        auto* row=settingRow(section,title,{},44);row->itemAt(0)->widget()->setFixedWidth(144);value->setFixedWidth(65);value->setAlignment(Qt::AlignCenter);
        // One step is 0.1; the arrows flank the slider and holding repeats.
        const auto nudge=[slider](int direction){slider->setValue(std::clamp(slider->value()+direction,slider->minimum(),slider->maximum()));};
        auto* minus=new StepButton(QString::fromUtf8("−"),-1,nudge);minus->setAccessibleName(title+QStringLiteral(" -0.1"));minus->setToolTip(QStringLiteral("-0.1"));
        auto* plus=new StepButton("+",1,nudge);plus->setAccessibleName(title+QStringLiteral(" +0.1"));plus->setToolTip(QStringLiteral("+0.1"));
        row->addWidget(minus);row->addWidget(slider,1);row->addWidget(plus);row->addWidget(value);
        if(vertical){vertical_=value;vertical_slider_=slider;}else{horizontal_=value;horizontal_slider_=slider;}
        const auto syncSteps=[=]{minus->setEnabled(slider->value()>slider->minimum());plus->setEnabled(slider->value()<slider->maximum());};
        connect(slider,&QSlider::valueChanged,value,[value,syncSteps](int n){value->setValue(n/10.0);syncSteps();});
        syncSteps();
        connect(value,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[=](double n){QSignalBlocker blocked(slider);slider->setValue(qRound(n*10));syncSteps();if(!refreshing_)editOperator([=](r6::Operator& op){if(vertical)op.vertical=float(n);else op.horizontal=float(n);});});
    }
    void styleNotice(){
        if(!notice_bar_)return;
        const bool error=notice_bar_->property("error").toBool();
        const QColor background=error?kbtheme::pink:kbtheme::lavender;
        notice_bar_->setStyleSheet(QString("QWidget#noticeBar{background:%1;border:2px solid %2;border-radius:12px;}"
            "QLabel#startupNotice{color:%3;font-weight:600;}"
            "QPushButton#noticeClose{background:transparent;border:0;color:%3;font-size:18px;font-weight:800;padding:0;}")
            .arg(background.name(),kbtheme::outline.name(),kbtheme::readableOn(background).name()));
    }
    void applyStyle(){
        pages_->settle();qApp->setPalette(kbtheme::palette());setStyleSheet(applicationStyle(accent()));setPalette(kbtheme::palette());
        styleNotice();
        if(tour_step_>=0)setTourStep(tour_step_);
        for(auto* child:findChildren<QWidget*>())child->update();update();
    }
    void syncThemeCards(){
        for(auto* card:theme_cards_)if(card->themeId()==kbtheme::activeColorTheme)card->setChecked(true);
        for(auto* card:theme_cards_){if(card->themeId()!=kbtheme::activeColorTheme&&card->isChecked())card->setChecked(false);card->update();}
    }
    QString selectedThemeId() const{
        for(auto* card:theme_cards_)if(card&&card->isChecked())return card->themeId();
        return {};
    }
    bool chooseColorTheme(const QString& id){
        if(!kbtheme::writeColorTheme(config_path_,id)){
            syncThemeCards();showNotice(text(UiText::ThemeSaveFailed),true,true);return false;
        }
        kbtheme::initialize(id);
        if(pairing_qr_)pairing_qr_->refreshTheme();
        applyStyle();syncThemeCards();return true;
    }
    void chooseSettingsCategory(int index){
        if(index<0||index>=preference_categories_.size())return;
        if(pairing_qr_&&index!=3)pairing_qr_->conceal();preference_category_=index;
        for(int i=0;i<preference_categories_.size();++i)preference_categories_[i]->setVisible(i==index);
        for(int i=0;i<settings_nav_buttons_.size();++i)settings_nav_buttons_[i]->setChecked(i==index);
        preferences_scroll_->verticalScrollBar()->setValue(0);
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
        const QString names[]={text(UiText::General),text(UiText::Appearance),text(UiText::Shortcuts),text(UiText::PhoneControl),text(UiText::ProfileBackup),text(UiText::MouseSetup)};
        const QString hints[]={text(UiText::SettingsGeneralHint),text(UiText::SettingsAppearanceHint),text(UiText::SettingsShortcutsHint),text(UiText::SettingsPhoneHint),text(UiText::SettingsBackupHint),text(UiText::MouseSetupTabHint)};
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
        if(collapsed!=sidebar_collapsed_&&!uiprefs::update(config_path_,[=](QJsonObject& root){root["sidebarCollapsed"]=collapsed;})){
            updateSidebarControls();showNotice(text(UiText::SidebarSaveFailed),true,true);return;
        }
        sidebar_collapsed_=collapsed;focus_sidebar_search_=focusSearch&&!collapsed;
        const int target=collapsed?76:200;sidebar_animating_=animate&&sidebar_->width()!=target;updateSidebarControls();
        if(!animate||sidebar_->width()==target){sidebar_animation_.stop();sidebar_animating_=false;sidebar_->setFixedWidth(target);updateSidebarControls();if(focus_sidebar_search_){search_->setFocus(Qt::ShortcutFocusReason);focus_sidebar_search_=false;}return;}
        retargetAnimation(sidebar_animation_,sidebar_->width(),target,240);
    }
    void buildSettings(){
        refreshing_=true;const auto c=engine_.snapshot();const auto hint=[this](UiText id){return text(id);};if(settings_){scroll_->takeWidget();delete settings_;}
        settings_=new QWidget;settings_->setObjectName("settingsContent");cards_=new QVBoxLayout(settings_);cards_->setContentsMargins(2,0,10,12);cards_->setSpacing(10);
        auto* weapons=new Section(text(UiText::WeaponProfiles),false,true,kbtheme::yellow);weapons->body->setSpacing(0);cards_->addWidget(weapons);
        dual_=toggle(settingRow(weapons,text(UiText::SeparateSecondaryProfile),hint(UiText::Use1And2ToSwitchWeaponProfilesChangeTheKeysInSettings)),"Separate secondary profile",c.secondary_enabled[c.selected],[this](bool on){engine_.edit([&](r6::Config& c){c.enable_secondary(c.selected,on);c.weapon_slot=on?1:0;});dirty();syncSelection();});
        auto* weaponChoices=new QWidget;auto* choices=new QHBoxLayout(weaponChoices);choices->setContentsMargins(0,0,0,0);choices->setSpacing(6);
        primary_=new WeaponButton([this]{return accent();});secondary_=new WeaponButton([this]{return accent();});for(auto* button:{primary_,secondary_})choices->addWidget(button);choices->addStretch();auto* weaponBox=new QWidget;auto* weaponBoxLayout=new QVBoxLayout(weaponBox);weaponBoxLayout->setContentsMargins(0,8,0,0);weaponBoxLayout->setSpacing(0);weaponBoxLayout->addWidget(weaponChoices);weapon_selector_=new ExpandingContent(weaponBox);weapons->body->addWidget(weapon_selector_);
        connect(primary_,&QPushButton::clicked,this,[this]{chooseWeapon(0);});connect(secondary_,&QPushButton::clicked,this,[this]{chooseWeapon(1);});
        auto* recoil=new Section(text(UiText::RecoilControl),false,true,kbtheme::blue);cards_->addWidget(recoil);
        amount(recoil,text(UiText::VerticalForce),true);amount(recoil,text(UiText::HorizontalAdjust),false);
        humanize_=toggle(settingRow(recoil,text(UiText::HumanizeMovement),hint(UiText::VaryMovementSlightlyOnBothAxes)),"Humanize movement",false,[this](bool on){editOperator([=](r6::Operator& op){op.humanize=on;});});
        auto* rapid=new Section(text(UiText::RapidFire),false,true,kbtheme::pink);cards_->addWidget(rapid);
        rapid_active_=toggle(settingRow(rapid,text(UiText::RapidFire),hint(UiText::RapidFireGlobalHint)),"Rapid fire",c.active_operator().rapid!=0,[this](bool on){editOperator([=](r6::Operator& op){op.rapid=on;});updateStatus();});
        auto* delayRow=settingRow(rapid,text(UiText::ShotDelay),hint(UiText::DelayBetweenRepeatedShots));
        delay_=new QSpinBox;delay_->setRange(30,150);delay_->setSuffix(text(UiText::Ms));delay_->setButtonSymbols(QAbstractSpinBox::NoButtons);delay_->setReadOnly(true);delay_->setFocusPolicy(Qt::NoFocus);delay_->setAlignment(Qt::AlignCenter);delay_->setFixedWidth(85);delay_->setAccessibleName(text(UiText::ShotDelay));delayRow->addWidget(delay_);
        delay_controls_=new QWidget;auto* delayControls=new QHBoxLayout(delay_controls_);delayControls->setContentsMargins(14,0,12,10);delayControls->setSpacing(10);
        delay_slider_=makeSlider();delay_slider_->setObjectName("shotDelaySlider");delay_slider_->setRange(30,150);delay_slider_->setSingleStep(1);delay_slider_->setPageStep(5);delay_slider_->setAccessibleName(text(UiText::ShotDelay));
        const auto nudgeDelay=[this](int direction){delay_slider_->setValue(std::clamp(delay_slider_->value()+direction*5,30,150));};
        delay_minus_=new StepButton(QString::fromUtf8("−"),-1,nudgeDelay);delay_plus_=new StepButton("+",1,nudgeDelay);
        delay_minus_->setAccessibleName(text(UiText::ShotDelay)+QStringLiteral(" -5")+text(UiText::Ms));delay_plus_->setAccessibleName(text(UiText::ShotDelay)+QStringLiteral(" +5")+text(UiText::Ms));
        delay_minus_->setToolTip(QStringLiteral("-5")+text(UiText::Ms));delay_plus_->setToolTip(QStringLiteral("+5")+text(UiText::Ms));
        delayControls->addWidget(delay_minus_);delayControls->addWidget(delay_slider_,1);delayControls->addWidget(delay_plus_);rapid->body->addWidget(delay_controls_);
        connect(delay_slider_,&QSlider::valueChanged,delay_,&QSpinBox::setValue);
        connect(delay_,qOverload<int>(&QSpinBox::valueChanged),this,[this](int n){QSignalBlocker blocked(delay_slider_);delay_slider_->setValue(n);syncDelayControls();if(!refreshing_)editOperator([=](r6::Operator& op){op.delay=n;});});

        cards_->addStretch();scroll_->setWidget(settings_);refreshing_=false;syncSelection();
    }
    void buildLogitechOutput(QVBoxLayout* column){
        auto* output=new Section(text(UiText::OutputMethod),false,true,kbtheme::blue);column->addWidget(output);
        const int method=engine_.snapshot().output_method;
        logitech_output_=toggle(settingRow(output,text(UiText::LogitechOutput),text(UiText::LogitechOutputHint)),"Logitech G HUB output",method==1,[this](bool on){
            // One pull, one owner: exactly one backend emits movement.
            engine_.edit([&](r6::Config& config){config.output_method=on?1:0;});
            if(driver_output_)driver_output_->syncChecked(false);
            engine_.output_method=on?1:0;dirty();refreshLogitechStatus();
        });
        driver_output_=toggle(settingRow(output,text(UiText::DriverOutput),text(UiText::DriverOutputHint)),"Interception driver",method==2,[this](bool on){
            engine_.edit([&](r6::Config& config){config.output_method=on?2:0;});
            if(logitech_output_)logitech_output_->syncChecked(false);
            engine_.output_method=on?2:0;dirty();refreshLogitechStatus();
        });
        auto* hint=label(text(UiText::LogitechScriptHint),"rowHint");hint->setWordWrap(true);hint->setContentsMargins(14,4,12,4);output->body->addWidget(hint);
        logitech_status_=label(QString(),"rowHint");logitech_status_->setWordWrap(true);logitech_status_->setContentsMargins(14,0,12,4);output->body->addWidget(logitech_status_);
        auto* actions=new QHBoxLayout;actions->setContentsMargins(14,2,12,10);actions->setSpacing(8);
        logitech_export_=new QPushButton(text(UiText::ExportLogitechScript));logitech_export_->setObjectName("secondary");
        logitech_export_->setAccessibleName(text(UiText::ExportLogitechScript));
        actions->addWidget(logitech_export_);actions->addStretch();output->body->addLayout(actions);
        connect(logitech_export_,&QPushButton::clicked,this,[this]{exportLogitechScript();});
        refreshLogitechStatus();
    }
    void refreshLogitechStatus(){
        if(!logitech_status_)return;
        const auto config=engine_.snapshot();
        QString state=text(logitech::installed()?UiText::LogitechHubFound:UiText::LogitechHubMissing);
        state+=QStringLiteral(" · ")+text(driver_input::available()?UiText::DriverFound:UiText::DriverMissing);
        if(config.output_method==1){
            const auto path=logitech::scriptPath();
            // Profiles edited after the last export would otherwise silently disagree.
            if(QFileInfo::exists(path)&&logitech::read(path)!=logitech::script(config,engine_.ads_only.load()))
                state+=QStringLiteral(" · ")+text(UiText::LogitechScriptStale);
            logitech_status_->setToolTip(path);
        }else logitech_status_->setToolTip(QString());
        logitech_status_->setText(state);
        if(logitech_export_)logitech_export_->setEnabled(!save_blocked_&&config.output_method==1);
    }
    void exportLogitechScript(){
        const auto bytes=logitech::script(engine_.snapshot(),engine_.ads_only.load());
        QString path,error;
        if(!logitech::write(bytes,logitech::scriptPath(),&path,&error)){
            diagnostic("write logitech script",error);
            showNotice(text(UiText::LogitechScriptFailed),true,true);return;
        }
        if(logitech_status_)logitech_status_->setToolTip(path);
        showNotice(text(UiText::LogitechScriptExported));
        refreshLogitechStatus();
    }
    void buildPreferences(){
        if(!preferences_)return;
        refreshing_=true;const auto c=engine_.snapshot();
        theme_cards_.clear();delete preferences_scroll_->takeWidget();preference_categories_.clear();auto* content=new QWidget;
        auto* outer=new QVBoxLayout(content);outer->setContentsMargins(2,0,10,0);outer->setSpacing(14);
        const auto categoryLayout=[&]{auto* page=new QWidget;page->setMinimumWidth(0);auto* layout=new QVBoxLayout(page);layout->setContentsMargins(0,0,0,0);layout->setSpacing(14);outer->addWidget(page);preference_categories_.append(page);return layout;};
        auto* generalLayout=categoryLayout();auto* preferencesLayout=categoryLayout();auto* shortcutsLayout=categoryLayout();auto* phoneLayout=categoryLayout();auto* backupLayout=categoryLayout();auto* mouseLayout=categoryLayout();
        auto* colorTitle=new StickerLabel(text(UiText::ColorTheme),kbtheme::blue);preferencesLayout->addWidget(colorTitle,0,Qt::AlignLeft);
        auto* themeGrid=new QWidget;themeGrid->setObjectName("themeGrid");auto* grid=new QGridLayout(themeGrid);
        themeGrid->setToolTip(text(UiText::ColorThemeHint));
        grid->setContentsMargins(0,0,0,0);grid->setHorizontalSpacing(14);grid->setVerticalSpacing(10);
        for(int i=0;i<kbtheme::colorThemes.size();++i){
            const auto& theme=kbtheme::colorThemes[i];auto* card=new ThemeCard(QLatin1String(theme.id));
            theme_cards_.append(card);grid->addWidget(card,i/3,i%3);
            const QString id=QLatin1String(theme.id);
            connect(card,&QAbstractButton::clicked,this,[this,id]{if(id!=kbtheme::activeColorTheme)chooseColorTheme(id);else syncThemeCards();});
        }
        for(int column=0;column<3;++column)grid->setColumnStretch(column,1);
        preferencesLayout->addWidget(themeGrid);syncThemeCards();
        auto* system=new Section(text(UiText::SystemSettings),false,true,kbtheme::lime);system_=system;generalLayout->addWidget(system);
        toggle(settingRow(system,text(UiText::FeedbackSound),text(UiText::SoundWhenClosingUnsaved)),"Feedback sound",c.sound,[this](bool on){engine_.edit([=](r6::Config& c){c.sound=on;});dirty();});
        game_focus_only_=toggle(settingRow(system,text(UiText::OnlyWhileGameIsActive),text(UiText::OnlyWhileGameIsActiveHint)),"Only in Rainbow Six",engine_.game_focus_only,[this](bool on){engine_.game_focus_only=on;dirty();});
        ads_only_=toggle(settingRow(system,text(UiText::OnlyWhileAiming),text(UiText::OnlyWhileAimingHint)),"Only while aiming",engine_.ads_only,[this](bool on){engine_.ads_only=on;dirty();});
        auto* langRow=settingRow(system,text(UiText::Language));language_=new LanguageComboBox;language_->setAccessibleName(text(UiText::Language));language_->setIconSize(QSize(22,15));
        for(const auto& entry:languageEntries())language_->addItem(languageFlag(entry.value),QString::fromUtf8(entry.label),entry.value);
        language_->setCurrentIndex(qMax(0,language_->findData(c.language)));langRow->addWidget(language_);
        connect(language_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int index){const int value=language_->itemData(index).toInt();engine_.edit([=](r6::Config& c){c.language=value;});dirty();if(value==6)QTimer::singleShot(0,this,[this]{showPalestineSurprise();});QTimer::singleShot(0,this,[this]{buildPreferences();buildSettings();updateLanguage();});});
        buildLogitechOutput(generalLayout);
        auto* updates=new Section(text(UiText::Updates),false,true,kbtheme::blue);generalLayout->addWidget(updates);
        auto* versionRow=settingRow(updates,text(UiText::InstalledVersion),QString("Azec ")+QCoreApplication::applicationVersion());
        update_button_=new QPushButton(text(UiText::CheckForUpdates));update_button_->setObjectName("secondary");update_button_->setAccessibleName(text(UiText::CheckForUpdates));update_button_->setEnabled(!update_in_progress_);
        versionRow->addWidget(update_button_);connect(update_button_,&QPushButton::clicked,this,[this]{checkForUpdates();});
        auto* shortcuts=new Section(text(UiText::Shortcuts),false,true,kbtheme::blue);shortcutsLayout->addWidget(shortcuts);
        const QString titles[]={text(UiText::OpenHideWindow),text(UiText::RecoilToggle),text(UiText::PanicExitKey),text(UiText::PrimaryWeapon),text(UiText::SecondaryWeapon)};const int keys[]={c.menu_key,c.recoil_key,c.panic_key,c.primary_key,c.secondary_key};
        for(int i=0;i<5;++i){auto* row=settingRow(shortcuts,titles[i],{},44);auto* button=new QPushButton(bindingName(keys[i]));button->setObjectName("keyButton");button->setFixedWidth(108);button->setAccessibleName(titles[i]);row->addWidget(button);key_buttons_[i]=button;connect(button,&QPushButton::clicked,this,[this,i]{bindKey(i);});}
        auto* phoneSection=new Section(text(UiText::PhoneControl),false,true,kbtheme::yellow);phoneSection->body->setSpacing(0);phoneLayout->addWidget(phoneSection);
        phone_enabled_=toggle(settingRow(phoneSection,text(UiText::EnablePhoneAccess)),"Enable phone access",c.phone_enabled,[this](bool enabled){
            engine_.edit([&](r6::Config& c){c.phone_enabled=enabled;});
            if(pairing_qr_)pairing_qr_->conceal();
            if(enabled){if(!phone_->start()){showNotice(text(UiText::CouldNotStartPhoneServer),true,true);if(notice_bar_)notice_bar_->setProperty("phoneError",true);}}
            else phone_->stop();
            updatePhoneDetails();phone_details_->setExpanded(enabled);dirty();
        });
        phone_enabled_->parentWidget()->setFixedHeight(44);
        {
            auto* connection=new QWidget;auto* connectionLayout=new QVBoxLayout(connection);connectionLayout->setContentsMargins(15,13,12,8);connectionLayout->setSpacing(7);
            auto* addressRow=new QHBoxLayout;addressRow->setSpacing(8);
            auto* address=label(phone_->listening()?phone_->address():text(UiText::CouldNotStartPhoneServer),"phoneAddress");phone_address_=address;address->setTextInteractionFlags(Qt::TextSelectableByMouse);addressRow->addWidget(address,1);
            phone_retry_=new QPushButton(text(UiText::Retry));phone_retry_->setObjectName("secondary");phone_retry_->setAccessibleName(text(UiText::Retry));addressRow->addWidget(phone_retry_);
            connect(phone_retry_,&QPushButton::clicked,this,[this]{if(!phone_->start()){showNotice(text(UiText::CouldNotStartPhoneServer),true,true);if(notice_bar_)notice_bar_->setProperty("phoneError",true);}else if(notice_bar_&&notice_bar_->property("phoneError").toBool()){notice_bar_->setProperty("phoneError",false);notice_timer_.stop();notice_bar_->hide();}updatePhoneDetails();});
            connectionLayout->addLayout(addressRow);
            auto* pair=new QHBoxLayout;pair->setSpacing(7);pair->addWidget(label(text(UiText::PairingCode),"hint"));
            phone_code_=new QLabel(phone_->code());phone_code_->setObjectName("pairingCode");phone_code_->setTextInteractionFlags(Qt::TextSelectableByMouse);pair->addWidget(phone_code_);pair->addStretch();
            copy_code_=new QPushButton(text(UiText::CopyCode));copy_code_->setObjectName("secondary");pair->addWidget(copy_code_);
            connect(copy_code_,&QPushButton::clicked,this,[this]{QApplication::clipboard()->setText(phone_->code());copy_code_->setText(text(UiText::Copied));QTimer::singleShot(1400,copy_code_.data(),[this]{if(copy_code_)copy_code_->setText(text(UiText::CopyCode));});});
            copy_link_=new QPushButton(text(UiText::CopyLink));copy_link_->setObjectName("secondary");pair->addWidget(copy_link_);
            connect(copy_link_,&QPushButton::clicked,this,[this]{QApplication::clipboard()->setText(phone_->pairingLink());copy_link_->setText(text(UiText::Copied));QTimer::singleShot(1400,copy_link_.data(),[this]{if(copy_link_)copy_link_->setText(text(UiText::CopyLink));});});
            phone_new_code_=new QPushButton(text(UiText::NewCode));phone_new_code_->setObjectName("secondary");pair->addWidget(phone_new_code_);
            connect(phone_new_code_,&QPushButton::clicked,this,[this]{phone_->rotate();if(pairing_qr_)pairing_qr_->conceal();updatePhoneDetails();showNotice(text(UiText::NewCodeCreated));});
            connectionLayout->addLayout(pair);
            auto* note=label(text(UiText::OpenThisAddressInSafariOnTheSameWiFi),"hint");note->setWordWrap(true);connectionLayout->addWidget(note);
            pairing_qr_=new PairingQr(text(UiText::ClickToRevealQR),text(UiText::ClickToHideQR));
            pairing_qr_->setEnabled(phone_->listening());pairing_qr_->setPairingLink(phone_->listening()?phone_->pairingLink():QString());
            connectionLayout->addWidget(pairing_qr_,0,Qt::AlignLeft);
            phone_details_=new ExpandingContent(connection,300);phoneSection->body->addWidget(phone_details_);phone_details_->setExpanded(c.phone_enabled,false);
            updatePhoneDetails();
        }
        buildMouseSetup(mouseLayout);
        auto* guideRow=new QHBoxLayout;
        auto* guide=new QPushButton(text(UiText::QuickGuide));guide->setObjectName("secondary");guideRow->addWidget(guide);connect(guide,&QPushButton::clicked,this,[this]{help();});
        auto* replay=new QPushButton(text(UiText::ReplayTour));replay->setObjectName("secondary");guideRow->addWidget(replay);connect(replay,&QPushButton::clicked,this,[this]{startTour();});guideRow->addStretch();generalLayout->addLayout(guideRow);
        auto* transfer=new Section(text(UiText::ProfileBackup),false,true,kbtheme::pink);backupLayout->addWidget(transfer);
        auto* transfers=new QHBoxLayout;auto* exportButton=new QPushButton(text(UiText::ExportProfiles));auto* importButton=new QPushButton(text(UiText::ImportProfiles));
        for(auto* button:{exportButton,importButton}){button->setObjectName("secondary");transfers->addWidget(button);}transfer->body->addLayout(transfers);
        connect(exportButton,&QPushButton::clicked,this,[this]{exportProfiles();});connect(importButton,&QPushButton::clicked,this,[this]{importProfiles();});
        outer->addStretch();preferences_scroll_->setWidget(content);updateSettingsNavLabels();chooseSettingsCategory(preference_category_);refreshing_=false;
    }
    // Both installed and older portable shared builds have a payload marker.
    bool packagedBuild() const{return QFileInfo(QCoreApplication::applicationDirPath()+"/version.txt").exists();}
    void setUpdating(bool busy){update_in_progress_=busy;if(update_button_)update_button_->setEnabled(!busy);}
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
            QString asset,checksums;QByteArray digest;
            for(const auto& entry:root["assets"].toArray()){
                const auto object=entry.toObject();const auto name=object["name"].toString();
                if(name==azec_update::assetName){asset=object["browser_download_url"].toString();digest=azec_update::validHash(object["digest"].toString().toLatin1());}
                if(name=="SHA256SUMS.txt")checksums=object["browser_download_url"].toString();
            }
            if(!packagedBuild()||asset.isEmpty()){QDesktopServices::openUrl(QUrl(azec_update::releasePage));return;}
            if(unsaved_){showNotice(text(UiText::SaveConfigurationUnsavedChanges));return;}
            if(QMessageBox::question(this,"Azec",text(UiText::UpdateAvailable)+"\n\nAzec "+current+"  ->  "+latest,QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes)!=QMessageBox::Yes)return;
            downloadUpdate(asset,digest,checksums);
        });
    }
    void downloadUpdate(const QString& url,const QByteArray& digest,const QString& checksums){
        if(!network_)network_=new QNetworkAccessManager(this);
        if(!url.startsWith("https://")||(!checksums.isEmpty()&&!checksums.startsWith("https://"))){showNotice(text(UiText::UpdateCheckFailed),true,true);return;}
        setUpdating(true);
        if(digest.isEmpty()){
            if(checksums.isEmpty()){setUpdating(false);showNotice(text(UiText::UpdateCheckFailed),true,true);return;}
            auto* reply=network_->get(azec_update::request(QUrl(checksums)));
            connect(reply,&QNetworkReply::readyRead,this,[reply]{if(reply->bytesAvailable()>65536)reply->abort();});
            connect(reply,&QNetworkReply::finished,this,[this,reply,url]{
                const auto hash=azec_update::checksum(reply->readAll());const bool ok=reply->error()==QNetworkReply::NoError&&!hash.isEmpty();reply->deleteLater();
                if(!ok){setUpdating(false);showNotice(text(UiText::UpdateCheckFailed),true,true);return;}
                downloadUpdate(url,hash,{});
            });return;
        }
        const auto target=azec_paths::pendingUpdate();
        azec_update::downloadVerified(network_,this,QUrl(url),digest,target,[this,target](bool ok){
            setUpdating(false);
            if(!ok){showNotice(text(UiText::UpdateCheckFailed),true,true);return;}
            applyDownloadedUpdate(target);
        });
    }
    void applyDownloadedUpdate(const QString& file){
        // Edits may have arrived from the desktop or phone during the download.
        if(unsaved_){showNotice(text(UiText::SaveConfigurationUnsavedChanges));return;}
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
        const bool listening=phone_->listening();
        if(phone_address_)phone_address_->setText(listening?phone_->address():text(UiText::CouldNotStartPhoneServer));
        if(phone_code_)phone_code_->setText(phone_->code());
        if(copy_link_)copy_link_->setEnabled(listening);if(copy_code_)copy_code_->setEnabled(listening);
        if(phone_new_code_)phone_new_code_->setEnabled(listening);if(phone_retry_)phone_retry_->setVisible(!listening);
        if(pairing_qr_){pairing_qr_->setPairingLink(listening?phone_->pairingLink():QString());pairing_qr_->setEnabled(listening);}
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
        operators_title_->setText(text(UiText::AllOperators));favorites_title_->setText(text(UiText::Favorites));search_->setPlaceholderText(text(UiText::SearchOperators));subtitle_->setText(text(UiText::OperatorSettings));
        updateSettingsNavLabels();
        updateSidebarControls();
        updateSavePresentation(save_->property("saveState").toString());gear_->setToolTip(text(UiText::Settings));preferences_title_->setText(text(UiText::Settings));back_->setToolTip(text(UiText::BackToOperators));status_->setToolTip(text(UiText::ToggleRecoilActivity));rapid_status_->setToolTip(text(UiText::RapidFire));filterOperators();syncSelection();updateStatus();
    }
    void syncSelection(){
        const auto c=engine_.snapshot();const auto& op=c.active_operator();refreshing_=true;operator_name_->setText(QString::fromUtf8(op.name));operator_icon_->setPixmap(operatorPixmap(QString::fromUtf8(op.name),44,devicePixelRatioF()));
        dual_->syncChecked(c.secondary_enabled[c.selected]);weapon_selector_->setExpanded(c.secondary_enabled[c.selected]);
        primary_->setText(text(UiText::Primary));secondary_->setText(text(UiText::Secondary));
        primary_->setChosen(c.active_slot()==0);secondary_->setChosen(c.active_slot()==1);
        subtitle_->setText(c.secondary_enabled[c.selected]?(c.active_slot()?text(UiText::SecondaryWeapon):text(UiText::PrimaryWeapon)):text(UiText::OperatorSettings));
        vertical_->setValue(op.vertical);horizontal_->setValue(op.horizontal);vertical_slider_->setValue(qRound(op.vertical*10));horizontal_slider_->setValue(qRound(op.horizontal*10));humanize_->syncChecked(op.humanize);delay_->setValue(std::clamp(op.delay,30,150));delay_->setEnabled(op.rapid!=0);for(auto* control:{humanize_,dual_}){auto* surface=control->parentWidget();surface->setProperty("enabledSetting",control->isChecked());surface->style()->unpolish(surface);surface->style()->polish(surface);}refreshing_=false;updateStatus();
    }
    void chooseWeapon(int slot){if(engine_.select_weapon(slot)){stopTransition();syncSelection();}}
    void handleWeaponKeys(bool primary,bool secondary,bool editing,bool modified){
        if(!editing&&!modified){if(primary&&!was_primary_)chooseWeapon(0);if(secondary&&!was_secondary_)chooseWeapon(1);}was_primary_=primary;was_secondary_=secondary;
    }
    void syncDelayControls(){if(!delay_slider_)return;const bool enabled=engine_.rapid_active();delay_->setEnabled(enabled);delay_controls_->setEnabled(enabled);delay_minus_->setEnabled(delay_slider_->value()>30);delay_plus_->setEnabled(delay_slider_->value()<150);}
    void updateStatus(){const bool active=engine_.recoil_active;status_->setText(active?text(UiText::RecoilOn):text(UiText::RecoilOff));status_->setProperty("active",active);status_->style()->unpolish(status_);status_->style()->polish(status_);
        const bool rapid=engine_.rapid_active();rapid_status_->setText(rapid?text(UiText::RapidFireOn):text(UiText::RapidFireOff));rapid_status_->setProperty("active",rapid);rapid_status_->style()->unpolish(rapid_status_);rapid_status_->style()->polish(rapid_status_);
        if(rapid_active_){rapid_active_->syncChecked(rapid);auto* surface=rapid_active_->parentWidget();surface->setProperty("enabledSetting",rapid_active_->isChecked());surface->style()->unpolish(surface);surface->style()->polish(surface);}if(game_focus_only_)game_focus_only_->syncChecked(engine_.game_focus_only);if(ads_only_)ads_only_->syncChecked(engine_.ads_only);syncDelayControls();if(tray_rapid_)tray_rapid_->setChecked(rapid);}
    void toggleRecoil(){engine_.recoil_active=!engine_.recoil_active;updateStatus();}
    void toggleRapid(){const bool on=!engine_.rapid_active();engine_.set_rapid_active(on);dirty();updateStatus();}
    bool textEditingActive() const {auto* focused=QApplication::focusWidget();return focused&&(qobject_cast<QLineEdit*>(focused)||qobject_cast<QAbstractSpinBox*>(focused)||qobject_cast<QComboBox*>(focused));}
    void pollKeys(){
        const bool editing=isActiveWindow()&&textEditingActive();
        const bool game=r6::gameIsActive();
        const bool focusOnly=engine_.game_focus_only.load();
        const bool azecActive=isActiveWindow()||(tour_pill_&&tour_pill_->isActiveWindow())||QApplication::activeModalWidget()||QApplication::activePopupWidget();
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
    bool save(){
        const bool saved=!save_blocked_&&engine_.save(config_path_);
        if(saved){
            mouse_setup_dirty_=false;
            const bool themeSaved=kbtheme::writeColorTheme(config_path_,kbtheme::activeColorTheme);
            updateSavePresentation("saved");save_feedback_.start();
            refreshLogitechStatus();
            if(notice_bar_&&notice_bar_->property("saveError").toBool()){notice_bar_->setProperty("saveError",false);notice_timer_.stop();notice_bar_->hide();}
            if(!themeSaved)showNotice(text(UiText::ThemeSaveFailed),true,true);
        }else{
            updateSavePresentation("error");save_feedback_.stop();
            showNotice(text(UiText::CouldNotSaveConfiguration),true,true);
            if(notice_bar_)notice_bar_->setProperty("saveError",true);
        }
        return saved;
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
                engine_.edit([=](r6::Config& c){int* keys[]={&c.menu_key,&c.recoil_key,&c.panic_key,&c.primary_key,&c.secondary_key};*keys[index]=key==VK_BACK?0:key;});dirty();save();release_key_=key;blocked_until_=clock_.elapsed()+250;dialog.accept();return;
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
        auto* close=new QPushButton("Free Palestine");close->setObjectName("save");layout->addWidget(close,0,Qt::AlignCenter);connect(close,&QPushButton::clicked,popup,&QDialog::close);
        popup->setFixedWidth(340);popup->adjustSize();popup->move(mapToGlobal(rect().center())-popup->rect().center());roundWindow(popup);
        palestine_popup_=popup;popup->show();popup->raise();popup->activateWindow();
    }
    void help(){
        QDialog dialog(this,Qt::Dialog|Qt::FramelessWindowHint);dialog.setObjectName("helpDialog");dialog.setAttribute(Qt::WA_StyledBackground);dialog.setFixedSize(410,230);dialog.setWindowTitle(text(UiText::AzecHelp));auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(24,18,24,22);
        auto* header=new QHBoxLayout;auto* title=label("","cardTitle");header->addWidget(title,1);auto* close=new IconButton(":/ui/close.svg",[this]{return accent();});close->setObjectName("closeControl");close->setAccessibleName(text(UiText::CloseQuickGuide));header->addWidget(close);layout->addLayout(header);connect(close,&QPushButton::clicked,&dialog,&QDialog::accept);
        auto* description=label("");description->setWordWrap(true);layout->addWidget(description,1);auto* row=new QHBoxLayout;auto* back=new QPushButton(text(UiText::Back));back->setObjectName("secondary");auto* next=new QPushButton(text(UiText::Next));next->setObjectName("save");auto* count=label("","hint");row->addWidget(back);row->addWidget(count,1,Qt::AlignCenter);row->addWidget(next);layout->addLayout(row);
        const QStringList titles={text(UiText::Welcome),text(UiText::ChooseAnOperator),text(UiText::AdjustRecoil),text(UiText::RapidFire),text(UiText::HotkeysAndSaving)};
        const QStringList descriptions={text(UiText::YourOperatorProfilesAndControlsInOnePlace),text(UiText::SelectANameInTheSidebarOrSearchForAnOperator),text(UiText::SetVerticalAndHorizontalForceUseArrowKeysForFineAdjustments),text(UiText::EnableRapidFireForTheSelectedWeapon),text(UiText::SetYourHotkeysInSystemSettingsSaveConfigurationToKeepYourChanges)};
        int step=0;const auto refresh=[&]{title->setText(titles[step]);description->setText(descriptions[step]);count->setText(QString::number(step+1)+" / 5");back->setEnabled(step>0);next->setText(step==4?text(UiText::Done):text(UiText::Next));};
        connect(back,&QPushButton::clicked,&dialog,[&]{if(step)--step;refresh();});connect(next,&QPushButton::clicked,&dialog,[&]{if(step==4)dialog.accept();else{++step;refresh();}});refresh();roundWindow(&dialog);dialog.exec();
    }
protected:
    void paintEvent(QPaintEvent*) override {

        ++paint_count_;QPainter p(this);p.fillRect(rect(),kbtheme::paper);QColor color=accent();p.setRenderHint(QPainter::Antialiasing);p.setPen(Qt::NoPen);
        static const QPixmap image("titio.png");if(!image.isNull()){p.setOpacity(.02);p.drawPixmap(rect(),image);p.setOpacity(1);}
        color.setAlpha(8);p.setBrush(color);
        for(int x=18;x<width();x+=45)for(int y=90;y<height()-12;y+=45)p.drawEllipse(QPointF(x,y),1.1,1.1);
    }
    void showEvent(QShowEvent* event) override {
        QWidget::showEvent(event);engine_.menu_visible=true;if(!native_ready_){native_ready_=true;const HWND handle=HWND(winId());const auto style=GetWindowLongPtrW(handle,GWL_STYLE);SetWindowLongPtrW(handle,GWL_STYLE,style|WS_THICKFRAME|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX);SetWindowPos(handle,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);roundWindow(this);const BOOL dark=FALSE;DwmSetWindowAttribute(handle,20,&dark,sizeof(dark));}
    }
    void hideEvent(QHideEvent* event) override {if(pairing_qr_)pairing_qr_->conceal();if(tour_pill_)tour_pill_->hide();if(tour_arrow_)tour_arrow_->hide();engine_.menu_visible=false;QWidget::hideEvent(event);}
    void closeEvent(QCloseEvent* event) override {
        if(close_prompt_){if(!isVisible())showNormal();raise();activateWindow();QApplication::alert(this,0);if(close_alert_&&close_alert_->isVisible()){close_alert_->raise();close_alert_->activateWindow();}event->ignore();return;}
        if(unsaved_&&!force_close_){
            close_prompt_=true;const bool recoil=engine_.recoil_active.exchange(false);
            if(!engine_.dry_run()&&engine_.snapshot().sound)sound::playUnsaved();
            if(!isVisible())showNormal();
            raise();activateWindow();QApplication::alert(this,0);
            QDialog dialog(this,Qt::Dialog|Qt::FramelessWindowHint);dialog.setObjectName("unsavedDialog");dialog.setAttribute(Qt::WA_StyledBackground);dialog.setWindowTitle(text(UiText::UnsavedChanges));dialog.setFixedWidth(390);
            auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(22,20,22,20);layout->setSpacing(18);
            layout->addWidget(label(text(UiText::SaveYourChangesBeforeClosing),"rowTitle"));
            auto* row=new QHBoxLayout;int choice=0;
            for(int i=0;i<3;++i){const QString titles[]={text(UiText::Cancel),text(UiText::Discard),text(UiText::Save)};auto* button=new QPushButton(titles[i]);button->setObjectName(i==2?"save":i==1?"discardChanges":"cancelClose");row->addWidget(button);connect(button,&QPushButton::clicked,&dialog,[&,i]{choice=i;dialog.accept();});}
            layout->addLayout(row);dialog.adjustSize();roundWindow(&dialog);close_alert_=&dialog;dialog.exec();close_alert_=nullptr;
            engine_.recoil_active=recoil;close_prompt_=false;
            if(choice==0||(choice==2&&!save())){event->ignore();return;}
        }
        if(tour_pill_)tour_pill_->hide();if(tour_arrow_)tour_arrow_->hide();tour_refresh_.stop();
        if(tray_)tray_->hide();phone_->stop();engine_.stop();QWidget::closeEvent(event);
    }
    void resizeEvent(QResizeEvent* event) override {QWidget::resizeEvent(event);if(native_ready_)roundWindow(this);}
    bool eventFilter(QObject* object,QEvent* event) override {
        if(event->type()==QEvent::FocusIn||event->type()==QEvent::FocusOut||event->type()==QEvent::MouseButtonPress){
            if(auto* button=qobject_cast<QPushButton*>(object);button&&(isAncestorOf(button)||button->window()->parentWidget()==this)){
                const auto reason=event->type()==QEvent::MouseButtonPress?Qt::MouseFocusReason:static_cast<QFocusEvent*>(event)->reason();const bool keyboard=event->type()==QEvent::FocusIn&&(reason==Qt::TabFocusReason||reason==Qt::BacktabFocusReason||reason==Qt::ShortcutFocusReason);
                button->setProperty("keyboardFocus",keyboard);button->style()->unpolish(button);button->style()->polish(button);button->update();
            }
        }
        if(event->type()==QEvent::Wheel){
            auto* widget=qobject_cast<QWidget*>(object);
            if(widget&&!widget->hasFocus()&&(qobject_cast<QAbstractSlider*>(widget)||qobject_cast<QAbstractSpinBox*>(widget)||qobject_cast<QComboBox*>(widget))){
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
        auto* msg=static_cast<MSG*>(message);if(msg->message==WM_NCCALCSIZE&&msg->wParam){*result=0;return true;}
        if(msg->message==WM_NCHITTEST){RECT r;GetWindowRect(HWND(winId()),&r);const int x=short(LOWORD(msg->lParam)),y=short(HIWORD(msg->lParam)),edge=qRound(6*devicePixelRatioF());const bool left=x<r.left+edge,right=x>=r.right-edge,top=y<r.top+edge,bottom=y>=r.bottom-edge;
            if(top&&left)*result=HTTOPLEFT;else if(top&&right)*result=HTTOPRIGHT;else if(bottom&&left)*result=HTBOTTOMLEFT;else if(bottom&&right)*result=HTBOTTOMRIGHT;else if(left)*result=HTLEFT;else if(right)*result=HTRIGHT;else if(top)*result=HTTOP;else if(bottom)*result=HTBOTTOM;else return QWidget::nativeEvent(type,message,result);return true;}return QWidget::nativeEvent(type,message,result);
    }
public:
    explicit MainWindow(r6::Engine& engine,std::filesystem::path config_path):engine_(engine),config_path_(std::move(config_path)){
        kbtheme::initialize(kbtheme::readColorTheme(config_path_));
        setObjectName("mainWindow");setWindowTitle("Azec");setWindowFlags(Qt::Window|Qt::FramelessWindowHint);setWindowIcon(QIcon(":/ui/app-icon.svg"));const auto area=QGuiApplication::primaryScreen()->availableGeometry();setMinimumSize(std::min(760,area.width()-32),std::min(560,area.height()-32));resize(std::min(864,area.width()-48),std::min(664,area.height()-48));move(area.center()-rect().center());
        auto* layout=new QVBoxLayout(this);layout->setContentsMargins(0,0,0,0);layout->setSpacing(0);auto* header=new DragHeader;header->setObjectName("titleBar");header->setFixedHeight(60);auto* top=new QHBoxLayout(header);top->setContentsMargins(20,0,20,0);top->setSpacing(5);
        auto* logo=new QLabel;logo->setObjectName("brandLogo");logo->setFixedSize(32,32);logo->setPixmap(QIcon(":/ui/logo.svg").pixmap(QSize(32,32),devicePixelRatioF()));top->addWidget(logo);top->addSpacing(5);
        top->addWidget(new BrandLabel);
        top->addStretch();status_=new QPushButton;status_->setObjectName("status");status_->setToolTip(text(UiText::ToggleRecoilActivity));top->addWidget(status_);connect(status_,&QPushButton::clicked,this,[this]{toggleRecoil();});
        rapid_status_=new QPushButton;rapid_status_->setObjectName("statusRapid");rapid_status_->setToolTip(text(UiText::RapidFire));top->addWidget(rapid_status_);connect(rapid_status_,&QPushButton::clicked,this,[this]{toggleRapid();});
        save_=new IconButton(":/ui/save.svg",[this]{return accent();});save_->setObjectName("saveControl");save_->setAccessibleName(text(UiText::SaveConfiguration));top->addSpacing(7);top->addWidget(save_);connect(save_,&QPushButton::clicked,this,[this]{save();});
        save_feedback_.setSingleShot(true);save_feedback_.setInterval(1600);connect(&save_feedback_,&QTimer::timeout,this,[this]{updateSavePresentation("clean");});
        gear_=new IconButton(":/ui/settings.svg",[this]{return accent();});gear_->setObjectName("windowControl");gear_->setAccessibleName(text(UiText::Settings));gear_->setToolTip(text(UiText::Settings));top->addWidget(gear_);connect(gear_,&QPushButton::clicked,this,[this]{showPreferences();});top->addSpacing(7);auto* minimize=new IconButton(":/ui/minimize.svg",[this]{return accent();});minimize->setObjectName("windowControl");minimize->setAccessibleName(text(UiText::Minimize));minimize->setToolTip(text(UiText::Minimize));top->addWidget(minimize);connect(minimize,&QPushButton::clicked,this,&QWidget::showMinimized);
        close_=new IconButton(":/ui/close.svg",[this]{return accent();});close_->setObjectName("closeControl");close_->setAccessibleName(text(UiText::Close));close_->setToolTip(text(UiText::Close));top->addWidget(close_);connect(close_,&QPushButton::clicked,this,&QWidget::close);layout->addWidget(header);
        notice_bar_=new QWidget;notice_bar_->setObjectName("noticeBar");auto* noticeLayout=new QHBoxLayout(notice_bar_);noticeLayout->setContentsMargins(14,6,8,6);noticeLayout->setSpacing(8);
        notice_=new QLabel;notice_->setObjectName("startupNotice");notice_->setWordWrap(true);
        notice_close_=new QPushButton(QString::fromUtf8("×"));notice_close_->setObjectName("noticeClose");notice_close_->setFixedSize(24,24);notice_close_->setToolTip(text(UiText::Close));notice_close_->setAccessibleName(text(UiText::Close));
        connect(notice_close_,&QPushButton::clicked,this,[this]{notice_timer_.stop();notice_bar_->hide();});
        noticeLayout->addWidget(notice_,1);noticeLayout->addWidget(notice_close_,0,Qt::AlignTop);notice_bar_->hide();layout->addWidget(notice_bar_);
        buildGoofyLayout(layout);
        const auto c=engine_.snapshot();for(int i=0;i<r6::operator_count;++i){const QString name=QString::fromUtf8(c.operators[i].name);auto* item=new QListWidgetItem(operatorIcon(name),name);item->setData(Qt::UserRole,i);item->setToolTip(name);operators_->addItem(item);QSvgRenderer renderer(":/operators/"+operatorAssetName(name)+".svg");if(!renderer.isValid())icon_failures_<<name;}
        operators_->setCurrentRow(c.selected);
        favorites_=readFavorites();refreshFavorites();
        connect(operators_,&QListWidget::currentItemChanged,this,[this](QListWidgetItem* item,QListWidgetItem*){if(item)selectOperator(item->data(Qt::UserRole).toInt());});connect(favorites_list_,&QListWidget::currentItemChanged,this,[this](QListWidgetItem* item,QListWidgetItem*){if(item)selectOperator(item->data(Qt::UserRole).toInt());});
        for(auto* list:{operators_,favorites_list_})connect(list,&QListWidget::itemClicked,this,[this](QListWidgetItem* item){if(!item)return;const int id=item->data(Qt::UserRole).toInt();selectOperator(id);if(tour_step_==0&&id==0)QTimer::singleShot(0,this,[this]{if(tour_step_==0)advanceTour();});});
        connect(search_,&QLineEdit::textChanged,this,[this]{filterOperators();});
        operator_transition_.setDuration(120);operator_transition_.setStartValue(.35);operator_transition_.setEndValue(1.0);operator_transition_.setEasingCurve(QEasingCurve::InOutQuad);
        connect(&operator_transition_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){name_opacity_->setOpacity(value.toReal());icon_opacity_->setOpacity(value.toReal());});
        phone_=new PhoneServer([this]{return mobileState();},[this](const QJsonObject& request){return mobileUpdate(request);},this);if(c.phone_enabled)phone_->start();
        phone_->onNetworkChanged([this]{updatePhoneDetails();});
        notice_timer_.setSingleShot(true);notice_timer_.setInterval(8000);connect(&notice_timer_,&QTimer::timeout,this,[this]{notice_bar_->hide();});
        buildPreferences();
        if(c.phone_enabled&&!phone_->listening()){showNotice(text(UiText::CouldNotStartPhoneServer),true,true);if(notice_bar_)notice_bar_->setProperty("phoneError",true);}
        buildSettings();updateLanguage();applyStyle();buildTray();qApp->installEventFilter(this);
        auto* saveShortcut=new QShortcut(QKeySequence::Save,this);connect(saveShortcut,&QShortcut::activated,this,[this]{save();});
        auto* findShortcut=new QShortcut(QKeySequence::Find,this);connect(findShortcut,&QShortcut::activated,this,[this]{if(sidebar_collapsed_)setSidebarCollapsed(false,true,true);else{search_->setFocus(Qt::ShortcutFocusReason);search_->selectAll();}});
        auto* escapeShortcut=new QShortcut(QKeySequence(Qt::Key_Escape),this);connect(escapeShortcut,&QShortcut::activated,this,[this]{if(pages_->currentWidget()==preferences_)showOperators();else if(!search_->text().isEmpty())search_->clear();});
        clock_.start();poll_.setInterval(8);connect(&poll_,&QTimer::timeout,this,[this]{pollKeys();});poll_.start();paint_.setInterval(50);connect(&paint_,&QTimer::timeout,this,[this]{if(isVisible()&&!isMinimized()){const auto current=mapFromGlobal(QCursor::pos());trackCursor(current);}});paint_.start();
        tour_refresh_.setInterval(60);connect(&tour_refresh_,&QTimer::timeout,this,[this]{positionTour();});
        QTimer::singleShot(350,this,[this]{maybeStartTour();});
    }

    void capture(const QString& file){grab().save(file);}
    void writePhoneInfo(const QString& path) const {if(!engine_.dry_run())return;QFile file(path);if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(QJsonObject{{"port",int(phone_->port())},{"address",phone_->address()},{"token",phone_->token()},{"code",phone_->code()},{"pid",int(QCoreApplication::applicationPid())},{"dryRun",true},{"interface",QStringLiteral("goofy")}}).toJson());}
    void closeUsingControl(){close_->click();}
    void showNotice(const QString& message,bool error=false,bool persistent=false){
        if(!notice_bar_||!notice_)return;
        notice_bar_->setProperty("saveError",false);notice_bar_->setProperty("phoneError",false);
        notice_bar_->setProperty("error",error);notice_->setText(message);styleNotice();notice_bar_->show();notice_timer_.stop();
        if(!persistent){notice_timer_.setInterval(error?6000:3500);notice_timer_.start();}
    }
    void setStartupNotice(const QString& message,bool blockSaving){
        save_blocked_=blockSaving;
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
