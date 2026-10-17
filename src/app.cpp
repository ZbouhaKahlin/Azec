// Qt presentation; recovered input behavior stays in engine.cpp and core.cpp.
#include "engine.hpp"
#include "ui_widgets.hpp"
#include "phone_server.hpp"
#include <windows.h>
#include <dwmapi.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSaveFile>
#include <QSet>
#include <algorithm>
#include <cmath>

static bool keyDown(int key){return key&&(GetAsyncKeyState(key)&0x8000);}
static QString keyName(int key){
    if((key>='0'&&key<='9')||(key>='A'&&key<='Z'))return QString(QChar(key));
    switch(key){case 0:return "None";case 1:return "M1";case 2:return "M2";case 4:return "M3";case 5:return "M4";case 6:return "M5";
    case VK_LEFT:return "Left";case VK_RIGHT:return "Right";case VK_UP:return "Up";case VK_DOWN:return "Down";case VK_INSERT:return "Insert";case VK_END:return "End";}
    wchar_t name[128]{};UINT scan=MapVirtualKeyW(key,MAPVK_VK_TO_VSC);if(key==VK_DELETE||key==VK_BACK)scan|=0x100;
    if(GetKeyNameTextW(scan<<16,name,128))return QString::fromWCharArray(name);return QString::number(key);
}
static void roundWindow(QWidget* window){
    const HWND handle=HWND(window->winId());const DWORD preference=2;
    if(SUCCEEDED(DwmSetWindowAttribute(handle,33,&preference,sizeof(preference))))return;
    // Windows 10 lacks the DWM corner-preference API. Clip the actual HWND.
    RECT rectangle{};GetWindowRect(handle,&rectangle);const int radius=qRound(24*window->devicePixelRatioF());
    HRGN shape=CreateRoundRectRgn(0,0,rectangle.right-rectangle.left+1,rectangle.bottom-rectangle.top+1,radius,radius);
    if(shape&&!SetWindowRgn(handle,shape,TRUE))DeleteObject(shape);
}

class MainWindow final : public QWidget {
    r6::Engine& engine_;
    std::filesystem::path config_path_;
    QListWidget* operators_=nullptr;QListWidget* favorites_list_=nullptr;QLineEdit* search_=nullptr;
    QLabel *favorites_title_=nullptr,*preferences_title_=nullptr;QWidget* favorites_group_=nullptr;
    QPushButton* back_=nullptr;QSet<QString> favorites_;
    PageStack* pages_=nullptr;QWidget* operator_page_=nullptr;
    RaisedPanel* panel_=nullptr;QScrollArea* sidebar_scroll_=nullptr;
    QVariantAnimation operator_transition_;
    FadeLabel *name_opacity_=nullptr,*icon_opacity_=nullptr;
    QLabel *operators_title_=nullptr,*operator_name_=nullptr,*operator_icon_=nullptr,*subtitle_=nullptr;
    QPushButton *status_=nullptr,*save_=nullptr,*close_=nullptr,*gear_=nullptr;
    QWidget* preferences_=nullptr;QScrollArea* preferences_scroll_=nullptr;
    QScrollArea* scroll_=nullptr;QWidget* settings_=nullptr;QVBoxLayout* cards_=nullptr;
    Section *appearance_=nullptr,*system_=nullptr;
    Toggle *recoil_master_=nullptr,*rapid_master_=nullptr,*humanize_=nullptr,*operator_rapid_=nullptr,*dual_=nullptr;
    QWidget* weapon_selector_=nullptr;QPushButton *primary_=nullptr,*secondary_=nullptr;
    PhoneServer* phone_=nullptr;
    QDoubleSpinBox *vertical_=nullptr,*horizontal_=nullptr;QSpinBox* delay_=nullptr;
    QSlider *vertical_slider_=nullptr,*horizontal_slider_=nullptr;QComboBox* language_=nullptr;
    std::array<QPushButton*,6> key_buttons_{};
    QTimer poll_,paint_,save_feedback_;QElapsedTimer clock_;
    bool was_menu_=false,was_recoil_=false,was_rapid_=false,was_primary_=false,was_secondary_=false,refreshing_=false,native_ready_=false;
    int binding_=0,release_key_=0,paint_count_=0;qint64 blocked_until_=0,last_tune_=0;
    QStringList icon_failures_;
    QColor accent() const {const auto c=engine_.snapshot();return QColor::fromRgbF(c.neon[0],c.neon[1],c.neon[2]);}
    QString words(const char* en,const char* pt) const{return QString::fromUtf8(engine_.snapshot().language?en:pt);}
    static QJsonObject profileJson(const r6::Operator& op){return {{"vertical",std::round(op.vertical*10.0)/10.0},{"horizontal",std::round(op.horizontal*10.0)/10.0},{"humanize",bool(op.humanize)},{"rapid",bool(op.rapid)},{"delay",op.delay}};}
    QJsonObject mobileState() const {
        const auto c=engine_.snapshot();QJsonArray operators;
        for(int id=0;id<r6::operator_count;++id){const auto name=QString::fromUtf8(c.operators[id].name);operators.append(QJsonObject{{"id",id},{"name",name},{"favorite",favorites_.contains(name)},{"dual",c.secondary_enabled[id]}});}
        QJsonArray keys;const int values[]={c.menu_key,c.recoil_key,c.rapid_key,c.panic_key,c.primary_key,c.secondary_key};const char* labels[]={"Open / hide window","Recoil toggle","Rapid fire toggle","Exit key","Primary weapon","Secondary weapon"};
        for(int i=0;i<6;++i)keys.append(QJsonObject{{"label",labels[i]},{"value",values[i]},{"name",keyName(values[i])}});
        return {{"operators",operators},{"operatorId",c.selected},{"weaponSlot",c.active_slot()},{"dual",c.secondary_enabled[c.selected]},{"profile",profileJson(c.active_operator())},{"dirty",save_->property("saveState").toString()=="dirty"},{"keys",keys},
            {"globals",QJsonObject{{"recoilMaster",engine_.recoil_master.load()},{"rapidMaster",engine_.rapid_master.load()},{"recoilActive",engine_.recoil_active.load()},{"sound",bool(c.sound)},{"language",c.language},{"accent",accent().name()},{"radius",c.radius}}}};
    }
    QJsonObject mobileUpdate(const QJsonObject& request){
        const auto fail=[](const QString& message){return QJsonObject{{"error",message}};};
        const auto action=request["action"].toString();auto next=engine_.snapshot();
        const auto validNumber=[](const QJsonValue& v,double low,double high){return v.isDouble()&&std::isfinite(v.toDouble())&&v.toDouble()>=low&&v.toDouble()<=high;};
        const auto validInteger=[&](const QJsonValue& v,int low,int high){return validNumber(v,low,high)&&std::floor(v.toDouble())==v.toDouble();};
        if(action=="save"){save();if(save_->property("saveState").toString()=="error")return fail("Could not save configuration");return mobileState();}
        if(action=="selectOperator"||action=="setProfile"||action=="setDual"||action=="setFavorite"){
            if(!validInteger(request["operator"],0,r6::operator_count-1))return fail("Invalid operator");
            const int id=request["operator"].toInt();
            if(action=="selectOperator"){selectOperator(id);return mobileState();}
            if(action=="setFavorite"){if(!request["enabled"].isBool())return fail("Invalid favorite value");setFavorite(id,request["enabled"].toBool());if(favorites_.contains(QString::fromUtf8(next.operators[id].name))!=request["enabled"].toBool())return fail("Could not save favorite");return mobileState();}
            if(action=="setDual"){
                if(!request["enabled"].isBool())return fail("Invalid secondary profile value");
                engine_.edit([&](r6::Config& c){c.enable_secondary(id,request["enabled"].toBool());if(c.selected==id)c.weapon_slot=request["enabled"].toBool()?1:0;});dirty();syncSelection();return mobileState();
            }
            if(!validInteger(request["slot"],0,1))return fail("Invalid weapon slot");const int slot=request["slot"].toInt();if(slot==1&&!next.secondary_enabled[id])return fail("Enable the secondary profile first");
            auto op=slot?next.secondary[id]:next.operators[id];const auto values=request["values"].toObject();if(values.isEmpty())return fail("No profile values");
            for(auto it=values.begin();it!=values.end();++it){
                const auto key=it.key();const auto value=it.value();
                if(key=="vertical"&&validNumber(value,0,100))op.vertical=float(value.toDouble());
                else if(key=="horizontal"&&validNumber(value,-50,50))op.horizontal=float(value.toDouble());
                else if(key=="delay"&&validInteger(value,30,150))op.delay=value.toInt();
                else if(key=="humanize"&&value.isBool())op.humanize=value.toBool();
                else if(key=="rapid"&&value.isBool())op.rapid=value.toBool();else return fail("Invalid profile value: "+key);
            }
            engine_.edit([&](r6::Config& c){if(slot)c.secondary[id]=op;else c.operators[id]=op;});dirty();syncSelection();return mobileState();
        }
        if(action=="selectWeapon"){
            if(!validInteger(request["slot"],0,1)||!engine_.select_weapon(request["slot"].toInt()))return fail("Weapon profile is unavailable");syncSelection();return mobileState();
        }
        if(action=="setGlobal"){
            const auto values=request["values"].toObject();if(values.isEmpty())return fail("No settings values");
            bool master=engine_.recoil_master,rapid=engine_.rapid_master,active=engine_.recoil_active;bool rebuild=false,keysChanged=false;
            for(auto it=values.begin();it!=values.end();++it){const auto key=it.key();const auto value=it.value();
                if((key=="recoilMaster"||key=="rapidMaster"||key=="recoilActive"||key=="sound")&&value.isBool()){
                    if(key=="recoilMaster")master=value.toBool();else if(key=="rapidMaster")rapid=value.toBool();else if(key=="recoilActive")active=value.toBool();else {next.sound=value.toBool();rebuild=true;}
                }else if(key=="language"&&validInteger(value,0,1)){next.language=value.toInt();rebuild=true;}
                else if(key=="radius"&&validNumber(value,50,400)){next.radius=float(value.toDouble());rebuild=true;}
                else if(key=="accent"&&value.isString()&&QRegularExpression("^#[0-9a-fA-F]{6}$").match(value.toString()).hasMatch()){const QColor color(value.toString());next.neon[0]=color.redF();next.neon[1]=color.greenF();next.neon[2]=color.blueF();rebuild=true;}
                else if(key.startsWith("key")&&key.size()==4&&key[3]>='0'&&key[3]<='5'&&validInteger(value,0,255)){int* keys[]={&next.menu_key,&next.recoil_key,&next.rapid_key,&next.panic_key,&next.primary_key,&next.secondary_key};*keys[key[3].digitValue()]=value.toInt();rebuild=keysChanged=true;}
                else return fail("Invalid setting: "+key);
            }
            if(next.primary_key&&next.primary_key==next.secondary_key)return fail("Use different keys for primary and secondary");
            next.rapid_master=rapid;engine_.edit([&](r6::Config& c){c=next;});engine_.recoil_master=master;engine_.rapid_master=rapid;engine_.recoil_active=active;dirty();applyStyle();syncSelection();updateStatus();
            if(rebuild)QTimer::singleShot(0,this,[this]{buildPreferences();buildSettings();updateLanguage();});if(keysChanged)save();return mobileState();
        }
        return fail("Unknown action");
    }
    void updateSavePresentation(const QString& state){
        save_->setProperty("saveState",state);
        save_->setIcon(QIcon(state=="saved"?":/ui/saved.svg":":/ui/save.svg"));
        save_->setToolTip(state=="dirty"?words("Save configuration · Unsaved changes","Salvar configuracoes · Alteracoes nao salvas"):state=="saved"?words("Configuration saved","Configuracao salva"):state=="error"?words("Could not save configuration","Nao foi possivel salvar"):words("Save configuration","Salvar configuracoes"));
        save_->style()->unpolish(save_);save_->style()->polish(save_);save_->update();
    }
    void dirty(){if(!refreshing_){save_feedback_.stop();updateSavePresentation("dirty");}}
    void editOperator(const std::function<void(r6::Operator&)>& change){engine_.edit([&](r6::Config& c){change(c.active_operator());});dirty();}
    std::filesystem::path favoritesPath() const{return config_path_.parent_path()/L"kingboss_ui.json";}
    QSet<QString> readFavorites() const {
        QFile file(QString::fromStdWString(favoritesPath().wstring()));QSet<QString> result;
        if(!file.open(QIODevice::ReadOnly))return result;
        const auto array=QJsonDocument::fromJson(file.readAll()).object()["favorites"].toArray();const auto c=engine_.snapshot();
        for(const auto& entry:array)for(const auto& op:c.operators)if(entry.toString()==QString::fromUtf8(op.name))result.insert(entry.toString());
        return result;
    }
    bool persistFavorites() const {
        QStringList names(favorites_.begin(),favorites_.end());names.sort(Qt::CaseInsensitive);
        QSaveFile file(QString::fromStdWString(favoritesPath().wstring()));
        if(!file.open(QIODevice::WriteOnly))return false;
        const auto bytes=QJsonDocument(QJsonObject{{"favorites",QJsonArray::fromStringList(names)}}).toJson();
        return file.write(bytes)==bytes.size()&&file.commit();
    }
    void filterOperators(){
        const auto term=search_->text();int shown=0;
        for(auto* list:{operators_,favorites_list_}){int count=0;for(int row=0;row<list->count();++row){auto* item=list->item(row);const bool match=item->text().contains(term,Qt::CaseInsensitive)&&(list==favorites_list_||!favorites_.contains(item->text()));item->setHidden(!match);if(match)++count;}list->setFixedHeight(count*56);if(list==favorites_list_)shown=count;}
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
        if(!persistFavorites()){favorites_=previous;updateSavePresentation("error");save_->setToolTip(words("Could not save favorites","Nao foi possivel salvar favoritos"));}
        refreshFavorites();
    }
    void showOperatorMenu(QListWidget* list,const QPoint& position){
        auto* item=list->itemAt(position);if(!item)return;const int id=item->data(Qt::UserRole).toInt();const bool favorite=favorites_.contains(item->text());
        FavoritePopup popup(favorite?words("Remove favorite","Remover favorito"):words("Favorite","Favoritar"),this);
        if(popup.execAt(list->viewport()->mapToGlobal(position)))setFavorite(id,!favorite);
    }
    void stopTransition(){operator_transition_.stop();name_opacity_->setOpacity(1);icon_opacity_->setOpacity(1);}
    void showOperators(){stopTransition();pages_->switchTo(operator_page_);gear_->setProperty("activePage",false);gear_->style()->unpolish(gear_);gear_->style()->polish(gear_);}
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
        auto* widget=new QWidget;widget->setObjectName("settingRow");widget->setMinimumHeight(height);auto* row=new QHBoxLayout(widget);row->setContentsMargins(14,5,12,5);row->setSpacing(12);
        auto* text=new QWidget;auto* column=new QVBoxLayout(text);column->setContentsMargins(0,0,0,0);column->setSpacing(3);column->addWidget(label(title));
        if(!description.isEmpty())widget->setToolTip(description);
        row->addWidget(text,1);section->body->addWidget(widget);return row;
    }
    Toggle* toggle(QHBoxLayout* row,const QString& title,bool checked,const std::function<void(bool)>& callback){
        auto* result=new Toggle([this]{return accent();});result->setAccessibleName(title);result->setChecked(checked);row->addWidget(result,0,Qt::AlignVCenter);
        auto* surface=row->parentWidget();surface->setProperty("enabledSetting",checked);connect(result,&QAbstractButton::toggled,surface,[surface](bool on){surface->setProperty("enabledSetting",on);surface->style()->unpolish(surface);surface->style()->polish(surface);});
        connect(result,&QAbstractButton::toggled,this,[this,callback](bool on){if(!refreshing_)callback(on);});return result;
    }
    void amount(Section* section,const QString& title,bool vertical){
        auto* row=settingRow(section,title,{},44);row->itemAt(0)->widget()->setFixedWidth(144);
        auto* slider=new QSlider(Qt::Horizontal);slider->setRange(vertical?0:-500,vertical?1000:500);slider->setAccessibleName(title);
        auto* value=new QDoubleSpinBox;value->setDecimals(1);value->setSingleStep(.1);value->setRange(vertical?0:-50,vertical?100:50);value->setButtonSymbols(QAbstractSpinBox::NoButtons);value->setFixedWidth(65);value->setAlignment(Qt::AlignCenter);value->setAccessibleName(title+" value");
        row->addWidget(slider,1);row->addWidget(value);if(vertical){vertical_=value;vertical_slider_=slider;}else{horizontal_=value;horizontal_slider_=slider;}
        connect(slider,&QSlider::valueChanged,value,[value](int n){value->setValue(n/10.0);});
        connect(value,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[=](double n){QSignalBlocker blocked(slider);slider->setValue(qRound(n*10));if(!refreshing_)editOperator([=](r6::Operator& op){if(vertical)op.vertical=float(n);else op.horizontal=float(n);});});
    }
    void applyStyle(){
        setStyleSheet(QString::fromUtf8(R"CSS(
            QWidget { color:#d0cfd0; font-family:'Segoe UI'; font-size:13px; font-weight:400; }
            QWidget#mainWindow { background:#1a191a; }
            QFrame#titleBar { background:#1a191a; border:0; }
            QLabel#brand { font-family:'Consolas'; font-size:19px; font-weight:400; color:#dfdddf; }
            QLabel#pageTitle,QLabel#preferencesTitle { font-size:20px; font-weight:500; color:#eeedee; }
            QLabel#sidebarTitle { font-size:15px; font-weight:500; color:#d0cfd0; }
            QLabel#hint,QLabel#subtitle { color:#8e8b91; font-size:12px; }
            QLabel#favoritesTitle { color:#b3a16a; font-size:12px; }
            QFrame#favoritesDivider { background:#494332; border:0; max-height:1px; }
            QFrame#operatorsDivider { background:#353236; border:0; max-height:1px; }
            QLabel#rowTitle { color:#cecccf; font-size:13px; }
            QLabel#cardTitle { color:#a9a7ab; font-size:13px; font-weight:500; }
            QLabel#sectionArrow { color:#88858c; font-size:20px; }
            QLineEdit#search { background:#252426; color:#dedcdf; border:1px solid transparent; border-radius:7px; padding:7px 9px; }
            QLineEdit#search:focus { border-color:%1; }
            QMenu { background:#252426; color:#d0cfd0; border:0; padding:5px; }
            QMenu::item { padding:8px 18px 8px 10px; border-radius:4px; }
            QMenu::item:selected { background:#353236; }
            QFrame#favoritePopup { background:#282729; border:1px solid #373538; border-radius:8px; }
            QPushButton#toggleFavorite { text-align:left; background:transparent; border:0; border-radius:5px; padding:5px 9px; color:#dddadd; font-size:13px; }
            QPushButton#toggleFavorite:hover { background:#373437; }
            QPushButton#toggleFavorite:pressed { background:#403b40; }
            QPushButton#weaponChoice { background:transparent; color:#a7a3a9; border:0; border-radius:5px; padding:7px 14px; font-size:13px; }
            QPushButton#weaponChoice:hover { background:#2c2a2d; }
            QPushButton#weaponChoice[chosen="true"] { background:#183c32; color:#b8e2d3; }
            QLabel#phoneAddress { font-family:'Consolas'; font-size:12px; color:#a7c6bb; }
            QPushButton#windowControl[activePage="true"] { background:#29272b; }
            QListWidget { background:transparent; border:0; outline:0; }
            QFrame#settingCard { background:transparent; border:0; }
            QWidget#settingRow { background:#252426; border-radius:6px; border-left:3px solid transparent; }
            QWidget#settingRow[enabledSetting="true"] { border-left-color:%1; }
            QPushButton#sectionHeading { text-align:left; padding:12px 0; border:0; background:transparent; color:#dedcdf; font-weight:500; }
            QPushButton#sectionHeading:hover { color:white; }
            QPushButton#windowControl,QPushButton#saveControl { background:transparent; border:0; border-radius:6px; color:#a6a2aa; font-size:19px; }
            QPushButton#windowControl:hover,QPushButton#saveControl:hover { background:#29272b; color:#f1eef3; }
            QPushButton#saveControl[saveState="dirty"],QPushButton#saveControl[saveState="saved"] { background:#17362e; }
            QPushButton#saveControl[saveState="error"] { background:#4c2a30; }
            QPushButton#closeControl { background:transparent; border:0; border-radius:6px; color:#a6a2aa; font-size:24px; }
            QPushButton#closeControl:hover { background:#4c2a30; color:#ffb5bd; }
            QPushButton#status { font-family:'Segoe UI'; font-weight:400; background:#222122; border:0; border-radius:6px; padding:7px 12px; color:#aaa6ad; font-size:13px; }
            QPushButton#status[active="true"] { color:%1; }
            QPushButton#preferencesSave { font-family:'Segoe UI'; font-size:14px; background:%1; color:#f0f5f3; border:0; border-radius:7px; padding:10px 18px; font-weight:500; }
            QPushButton#preferencesSave:hover { background:#079d7c; }
            QPushButton#keyButton,QPushButton#secondary { background:#2e2c2f; border:0; border-radius:6px; padding:7px 12px; color:#d7d3da; }
            QPushButton#keyButton:hover,QPushButton#secondary:hover { background:#373438; }
            QPushButton#keyButton:pressed,QPushButton#secondary:pressed { background:#3e3a3f; }
            QDoubleSpinBox,QSpinBox { background:#302e31; border:1px solid transparent; border-radius:5px; padding:5px; color:#e0dce3; }
            QDoubleSpinBox:focus,QSpinBox:focus { border-color:%1; }
            QDoubleSpinBox:disabled,QSpinBox:disabled { color:#6f6a74; background:#2b292c; }
            QSlider::groove:horizontal { height:4px; background:#444045; border-radius:2px; }
            QSlider::sub-page:horizontal { background:%1; border-radius:2px; }
            QSlider::handle:horizontal { width:12px; margin:-4px 0; background:#d2d0d4; border:0; border-radius:6px; }
            QComboBox { background:#353236; color:#d7d3da; border:0; border-radius:5px; padding:7px 10px; min-width:120px; }
            QComboBox::drop-down { border:0; width:22px; }
            QComboBox QAbstractItemView { background:#2b282d; selection-background-color:%1; border:0; padding:4px; }
            QScrollArea { background:transparent; border:0; }
            QScrollArea>QWidget>QWidget { background:transparent; }
            QScrollBar:vertical { background:transparent; width:6px; margin:3px 0; }
            QScrollBar::handle:vertical { background:#454247; min-height:36px; border-radius:3px; }
            QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
            QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical { background:transparent; }
            QDialog { background:#1a191a; }
            QToolTip { background:#302c32; color:#e0dce3; border:0; padding:7px; }
            QPushButton:focus { outline:0; }
        )CSS").arg(accent().name()));operators_->viewport()->update();
    }
    void buildSettings(){
        refreshing_=true;const auto c=engine_.snapshot();if(settings_){scroll_->takeWidget();delete settings_;}
        settings_=new QWidget;settings_->setObjectName("settingsContent");cards_=new QVBoxLayout(settings_);cards_->setContentsMargins(0,0,8,10);cards_->setSpacing(10);
        auto* weapons=new Section(words("Weapon profiles","Perfis de armas"),false,true);cards_->addWidget(weapons);
        dual_=toggle(settingRow(weapons,words("Separate secondary profile","Perfil secundario separado"),words("Use 1 and 2 to switch weapon profiles. Change the keys in Settings.","Use 1 e 2 para trocar perfis. Altere as teclas nas Configuracoes.")),"Separate secondary profile",c.secondary_enabled[c.selected],[this](bool on){engine_.edit([&](r6::Config& c){c.enable_secondary(c.selected,on);c.weapon_slot=on?1:0;});dirty();syncSelection();});
        weapon_selector_=new QWidget;auto* choices=new QHBoxLayout(weapon_selector_);choices->setContentsMargins(0,2,0,2);choices->setSpacing(6);
        primary_=new QPushButton;secondary_=new QPushButton;for(auto* button:{primary_,secondary_}){button->setObjectName("weaponChoice");button->setCursor(Qt::PointingHandCursor);choices->addWidget(button);}choices->addStretch();weapons->body->addWidget(weapon_selector_);
        connect(primary_,&QPushButton::clicked,this,[this]{chooseWeapon(0);});connect(secondary_,&QPushButton::clicked,this,[this]{chooseWeapon(1);});
        auto* recoil=new Section(words("Recoil control","Controle de recoil"),false,true);cards_->addWidget(recoil);
        recoil_master_=toggle(settingRow(recoil,words("Master enable","Habilitar global"),words("Apply this operator's recoil settings.","Aplicar os ajustes deste operador.")),"Master recoil enable",engine_.recoil_master,[this](bool on){engine_.recoil_master=on;});
        amount(recoil,words("Vertical force","Forca vertical"),true);amount(recoil,words("Horizontal adjust","Ajuste horizontal"),false);
        humanize_=toggle(settingRow(recoil,words("Humanize movement","Humanizar movimento"),words("Vary movement slightly on both axes.","Variar o movimento nos dois eixos.")),"Humanize movement",false,[this](bool on){editOperator([=](r6::Operator& op){op.humanize=on;});});
        auto* rapid=new Section("Rapid fire",false,true);cards_->addWidget(rapid);
        rapid_master_=toggle(settingRow(rapid,words("Global rapid fire","Rapid fire global"),words("Master control for every operator.","Controle global para todos os operadores.")),"Global rapid fire",engine_.rapid_master,[this](bool on){engine_.set_rapid_master(on);dirty();});
        operator_rapid_=toggle(settingRow(rapid,words("Enable for this operator","Habilitar neste operador")),"Operator rapid fire",false,[this](bool on){editOperator([=](r6::Operator& op){op.rapid=on;});delay_->setEnabled(on);});
        auto* delayRow=settingRow(rapid,words("Shot delay","Intervalo entre tiros"),words("Delay between repeated shots.","Tempo entre os disparos repetidos."));
        delay_=new QSpinBox;delay_->setRange(30,150);delay_->setSuffix(" ms");delay_->setButtonSymbols(QAbstractSpinBox::NoButtons);delay_->setAlignment(Qt::AlignCenter);delay_->setFixedWidth(85);delay_->setAccessibleName("Shot delay");delayRow->addWidget(delay_);
        connect(delay_,qOverload<int>(&QSpinBox::valueChanged),this,[this](int n){if(!refreshing_)editOperator([=](r6::Operator& op){op.delay=n;});});
        cards_->addStretch();scroll_->setWidget(settings_);refreshing_=false;syncSelection();
    }
    void buildPreferences(){
        if(!preferences_)return;
        refreshing_=true;const auto c=engine_.snapshot();
        delete preferences_scroll_->takeWidget();auto* content=new QWidget;
        auto* preferencesLayout=new QVBoxLayout(content);preferencesLayout->setContentsMargins(0,0,8,0);preferencesLayout->setSpacing(16);
        auto* system=new Section(words("System settings","Configuracoes"),false,true);system_=system;preferencesLayout->addWidget(system);
        toggle(settingRow(system,words("Feedback sound","Som de feedback"),words("Beep when hotkey states change.","Sinal sonoro ao alternar atalhos.")),"Feedback sound",c.sound,[this](bool on){engine_.edit([=](r6::Config& c){c.sound=on;});dirty();});
        auto* langRow=settingRow(system,words("Language","Idioma"));language_=new QComboBox;language_->setAccessibleName("Language");language_->addItem("English (US)",1);language_->addItem("Portugues (BR)",0);language_->setCurrentIndex(c.language?0:1);langRow->addWidget(language_);
        connect(language_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int index){engine_.edit([=](r6::Config& c){c.language=index==0?1:0;});dirty();QTimer::singleShot(0,this,[this]{buildPreferences();buildSettings();updateLanguage();});});
        const QString titles[]={words("Open / hide window","Abrir / ocultar janela"),words("Recoil toggle","Alternar recoil"),words("Rapid fire toggle","Alternar rapid fire"),words("Panic / exit key","Tecla de saida"),words("Primary weapon","Arma primaria"),words("Secondary weapon","Arma secundaria")};const int keys[]={c.menu_key,c.recoil_key,c.rapid_key,c.panic_key,c.primary_key,c.secondary_key};
        for(int i=0;i<6;++i){auto* row=settingRow(system,titles[i],{},44);auto* button=new QPushButton(keyName(keys[i]));button->setObjectName("keyButton");button->setFixedWidth(108);button->setAccessibleName(titles[i]);row->addWidget(button);key_buttons_[i]=button;connect(button,&QPushButton::clicked,this,[this,i]{bindKey(i);});}
        auto* phoneSection=new Section(words("Phone control","Controle pelo celular"),false,true);preferencesLayout->addWidget(phoneSection);
        toggle(settingRow(phoneSection,words("Enable phone access","Habilitar acesso pelo celular")),"Enable phone access",c.phone_enabled,[this](bool enabled){engine_.edit([&](r6::Config& c){c.phone_enabled=enabled;});if(enabled)phone_->start();else phone_->stop();dirty();QTimer::singleShot(0,this,[this]{buildPreferences();});});
        if(c.phone_enabled){
            auto* connection=new QWidget;auto* connectionLayout=new QVBoxLayout(connection);connectionLayout->setContentsMargins(15,7,12,8);connectionLayout->setSpacing(7);
            auto* address=label(phone_->listening()?phone_->address():words("Could not start phone server","Nao foi possivel iniciar o servidor"),"phoneAddress");address->setTextInteractionFlags(Qt::TextSelectableByMouse);connectionLayout->addWidget(address);
            auto* pair=new QHBoxLayout;pair->addWidget(label(words("Pairing code: ","Codigo: ")+phone_->code(),"hint"));pair->addStretch();auto* copy=new QPushButton(words("Copy link","Copiar link"));copy->setObjectName("secondary");pair->addWidget(copy);connect(copy,&QPushButton::clicked,this,[this,copy]{QApplication::clipboard()->setText(phone_->pairingLink());copy->setText(words("Copied","Copiado"));});connectionLayout->addLayout(pair);auto* note=label(words("Open this address in Safari on the same Wi-Fi.","Abra o endereco no Safari usando o mesmo Wi-Fi."),"hint");note->setWordWrap(true);connectionLayout->addWidget(note);phoneSection->body->addWidget(connection);
        }
        auto* appearance=new Section(words("Appearance","Aparencia"),false,true);appearance_=appearance;preferencesLayout->addWidget(appearance);
        auto* colorRow=settingRow(appearance,words("Accent color","Cor de destaque"),words("Used for switches and selected states.","Usada nos controles e na selecao."));auto* color=new QPushButton(accent().name().toUpper());color->setObjectName("secondary");color->setAccessibleName("Accent color");colorRow->addWidget(color);
        connect(color,&QPushButton::clicked,this,[this,color]{const auto next=chooseColor();if(next.isValid()){engine_.edit([=](r6::Config& c){c.neon[0]=next.redF();c.neon[1]=next.greenF();c.neon[2]=next.blueF();});color->setText(next.name().toUpper());applyStyle();dirty();}});
        auto* radiusRow=settingRow(appearance,words("Reactive radius","Raio reativo"),words("Mouse response in the background.","Resposta ao mouse no fundo."));auto* radius=new QSlider(Qt::Horizontal);radius->setRange(50,400);radius->setValue(qRound(c.radius));radius->setAccessibleName("Reactive radius");radiusRow->addWidget(radius,1);connect(radius,&QSlider::valueChanged,this,[this](int n){engine_.edit([=](r6::Config& c){c.radius=float(n);});dirty();});
        auto* reset=new QPushButton(words("Reset appearance","Redefinir aparencia"));reset->setObjectName("secondary");appearance->body->addWidget(reset,0,Qt::AlignLeft);connect(reset,&QPushButton::clicked,this,[this]{engine_.edit([](r6::Config& c){c.neon[0]=5.f/255;c.neon[1]=134.f/255;c.neon[2]=105.f/255;c.radius=130;});applyStyle();dirty();QTimer::singleShot(0,this,[this]{buildPreferences();});});
        auto* guide=new QPushButton(words("Quick guide","Guia rapido"));guide->setObjectName("secondary");preferencesLayout->addWidget(guide,0,Qt::AlignLeft);connect(guide,&QPushButton::clicked,this,[this]{help();});
        preferencesLayout->addStretch();preferences_scroll_->setWidget(content);refreshing_=false;
    }
    void showPreferences(){
        if(pages_->currentWidget()==preferences_){showOperators();return;}
        stopTransition();pages_->switchTo(preferences_);gear_->setProperty("activePage",true);gear_->style()->unpolish(gear_);gear_->style()->polish(gear_);
    }
    void updateLanguage(){
        operators_title_->setText(words("All operators","Todos os operadores"));favorites_title_->setText(words("Favorites","Favoritos"));search_->setPlaceholderText(words("Search operators...","Pesquisar operadores..."));subtitle_->setText(words("Operator settings","Ajustes do operador"));
        updateSavePresentation(save_->property("saveState").toString());gear_->setToolTip(words("Settings","Configuracoes"));preferences_title_->setText(words("Settings","Configuracoes"));back_->setToolTip(words("Back to operators","Voltar aos operadores"));filterOperators();syncSelection();updateStatus();
    }
    void syncSelection(){
        const auto c=engine_.snapshot();const auto& op=c.active_operator();refreshing_=true;operator_name_->setText(QString::fromUtf8(op.name));operator_icon_->setPixmap(operatorPixmap(QString::fromUtf8(op.name),44,devicePixelRatioF()));
        dual_->syncChecked(c.secondary_enabled[c.selected]);weapon_selector_->setVisible(c.secondary_enabled[c.selected]);
        primary_->setText(words("Primary","Primaria")+"  ·  "+keyName(c.primary_key));secondary_->setText(words("Secondary","Secundaria")+"  ·  "+keyName(c.secondary_key));
        primary_->setProperty("chosen",c.active_slot()==0);secondary_->setProperty("chosen",c.active_slot()==1);for(auto* button:{primary_,secondary_}){button->style()->unpolish(button);button->style()->polish(button);}
        subtitle_->setText(c.secondary_enabled[c.selected]?(c.active_slot()?words("Secondary weapon","Arma secundaria"):words("Primary weapon","Arma primaria")):words("Operator settings","Ajustes do operador"));
        vertical_->setValue(op.vertical);horizontal_->setValue(op.horizontal);vertical_slider_->setValue(qRound(op.vertical*10));horizontal_slider_->setValue(qRound(op.horizontal*10));humanize_->syncChecked(op.humanize);operator_rapid_->syncChecked(op.rapid);delay_->setValue(std::clamp(op.delay,30,150));delay_->setEnabled(op.rapid);for(auto* control:{humanize_,operator_rapid_,dual_}){auto* surface=control->parentWidget();surface->setProperty("enabledSetting",control->isChecked());surface->style()->unpolish(surface);surface->style()->polish(surface);}refreshing_=false;
    }
    void chooseWeapon(int slot){if(engine_.select_weapon(slot)){stopTransition();syncSelection();}}
    void handleWeaponKeys(bool primary,bool secondary,bool editing,bool modified){
        if(!editing&&!modified){if(primary&&!was_primary_)chooseWeapon(0);if(secondary&&!was_secondary_)chooseWeapon(1);}was_primary_=primary;was_secondary_=secondary;
    }
    void updateStatus(){status_->setText(engine_.recoil_active?words("Recoil on","Recoil ligado"):words("Recoil off","Recoil desligado"));status_->setProperty("active",engine_.recoil_active.load());status_->style()->unpolish(status_);status_->style()->polish(status_);recoil_master_->syncChecked(engine_.recoil_master);rapid_master_->syncChecked(engine_.rapid_master);for(auto* control:{recoil_master_,rapid_master_}){auto* surface=control->parentWidget();surface->setProperty("enabledSetting",control->isChecked());surface->style()->unpolish(surface);surface->style()->polish(surface);}}
    void feedback(bool on,int onFrequency,int offFrequency){if(!engine_.dry_run()&&engine_.snapshot().sound)Beep(on?onFrequency:offFrequency,50);}
    void toggleRecoil(){engine_.recoil_active=!engine_.recoil_active;feedback(engine_.recoil_active,600,300);updateStatus();}
    void pollKeys(){
        engine_.menu_visible=isVisible()&&!isMinimized();if(binding_||QApplication::activeModalWidget()||QApplication::activePopupWidget())return;
        if(release_key_){if(keyDown(release_key_)||clock_.elapsed()<blocked_until_)return;release_key_=0;was_menu_=was_recoil_=was_rapid_=false;}
        const auto c=engine_.snapshot();if(keyDown(c.panic_key)){close();return;}
        const bool menu=keyDown(c.menu_key),recoil=keyDown(c.recoil_key),rapid=keyDown(c.rapid_key);
        if(menu&&!was_menu_){if(isVisible())hide();else{showNormal();raise();activateWindow();}engine_.menu_visible=isVisible();}
        if(recoil&&!was_recoil_)toggleRecoil();if(rapid&&!was_rapid_){engine_.set_rapid_master(!engine_.rapid_master);feedback(engine_.rapid_master,1000,500);updateStatus();dirty();}
        was_menu_=menu;was_recoil_=recoil;was_rapid_=rapid;
        auto* focused=QApplication::focusWidget();const bool editing=isActiveWindow()&&(qobject_cast<QLineEdit*>(focused)||qobject_cast<QAbstractSpinBox*>(focused)||qobject_cast<QComboBox*>(focused));
        handleWeaponKeys(keyDown(c.primary_key),keyDown(c.secondary_key),editing,keyDown(VK_CONTROL)||keyDown(VK_MENU)||keyDown(VK_LWIN)||keyDown(VK_RWIN));
        if(isVisible()&&isActiveWindow()&&pages_->currentWidget()==operator_page_&&!editing&&clock_.elapsed()-last_tune_>80){const float y=(keyDown(VK_UP)? .1f:0)-(keyDown(VK_DOWN)? .1f:0),x=(keyDown(VK_RIGHT)? .1f:0)-(keyDown(VK_LEFT)? .1f:0);if(x||y){editOperator([=](r6::Operator& op){op.vertical=std::clamp(op.vertical+y,0.f,100.f);op.horizontal=std::clamp(op.horizontal+x,-50.f,50.f);});syncSelection();last_tune_=clock_.elapsed();}}
    }
    void save(){
        const bool saved=engine_.save(config_path_);updateSavePresentation(saved?"saved":"error");
        if(saved)save_feedback_.start();else save_feedback_.stop();
    }
    QColor chooseColor(){
        QDialog dialog(this,Qt::Dialog|Qt::FramelessWindowHint);dialog.setWindowTitle("Accent color");auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(20,14,20,18);
        auto* header=new QHBoxLayout;header->addWidget(label(words("Accent color","Cor de destaque"),"cardTitle"),1);auto* close=new QPushButton(QString::fromUtf8("×"));close->setObjectName("closeControl");close->setFixedSize(30,30);header->addWidget(close);layout->addLayout(header);connect(close,&QPushButton::clicked,&dialog,&QDialog::reject);
        auto* picker=new QColorDialog(accent(),&dialog);picker->setOption(QColorDialog::DontUseNativeDialog);picker->setOption(QColorDialog::NoButtons);picker->setWindowFlags(Qt::Widget);layout->addWidget(picker);
        auto* buttons=new QHBoxLayout;buttons->addStretch();auto* cancel=new QPushButton(words("Cancel","Cancelar"));cancel->setObjectName("secondary");auto* apply=new QPushButton(words("Apply color","Aplicar cor"));apply->setObjectName("save");buttons->addWidget(cancel);buttons->addWidget(apply);layout->addLayout(buttons);connect(cancel,&QPushButton::clicked,&dialog,&QDialog::reject);connect(apply,&QPushButton::clicked,&dialog,&QDialog::accept);
        dialog.adjustSize();roundWindow(&dialog);return dialog.exec()==QDialog::Accepted?picker->currentColor():QColor();
    }
    void bindKey(int index){
        binding_=index+1;QDialog dialog(this,Qt::Dialog|Qt::FramelessWindowHint);dialog.setFixedSize(360,205);dialog.setWindowTitle("Key binding");auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(22,18,22,20);
        auto* header=new QHBoxLayout;header->addWidget(label(words("Set a hotkey","Definir atalho"),"cardTitle"),1);auto* cancel=new QPushButton(QString::fromUtf8("×"));cancel->setObjectName("closeControl");cancel->setFixedSize(30,30);header->addWidget(cancel);layout->addLayout(header);connect(cancel,&QPushButton::clicked,&dialog,&QDialog::reject);
        auto* hint=label(words("Release your keys. Waiting...","Solte as teclas. Aguarde..."));hint->setWordWrap(true);layout->addWidget(hint);layout->addWidget(label(words("Esc cancels · Backspace clears","Esc cancela · Backspace remove"),"hint"));layout->addStretch();
        QElapsedTimer started;started.start();QTimer timer;timer.setInterval(16);connect(&timer,&QTimer::timeout,&dialog,[&]{if(started.elapsed()<1200)return;hint->setText(words("Press a key now","Pressione uma tecla agora"));for(int key=3;key<256;++key)if(keyDown(key)){if(key!=VK_ESCAPE){const auto existing=engine_.snapshot();if(key!=VK_BACK&&((index==4&&key==existing.secondary_key)||(index==5&&key==existing.primary_key))){hint->setText(words("Choose a different weapon key","Escolha outra tecla de arma"));return;}engine_.edit([=](r6::Config& c){int* keys[]={&c.menu_key,&c.recoil_key,&c.rapid_key,&c.panic_key,&c.primary_key,&c.secondary_key};*keys[index]=key==VK_BACK?0:key;});dirty();save();}release_key_=key;blocked_until_=clock_.elapsed()+250;dialog.accept();break;}});
        timer.start();roundWindow(&dialog);dialog.exec();binding_=0;const auto c=engine_.snapshot();const int keys[]={c.menu_key,c.recoil_key,c.rapid_key,c.panic_key,c.primary_key,c.secondary_key};if(key_buttons_[index])key_buttons_[index]->setText(keyName(keys[index]));syncSelection();updateStatus();
    }
    void help(){
        QDialog dialog(this,Qt::Dialog|Qt::FramelessWindowHint);dialog.setFixedSize(410,230);dialog.setWindowTitle("KingBoss2004 help");auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(24,18,24,22);
        auto* header=new QHBoxLayout;auto* title=label("","cardTitle");header->addWidget(title,1);auto* close=new QPushButton(QString::fromUtf8("×"));close->setObjectName("closeControl");close->setFixedSize(30,30);header->addWidget(close);layout->addLayout(header);connect(close,&QPushButton::clicked,&dialog,&QDialog::accept);
        auto* description=label("");description->setWordWrap(true);layout->addWidget(description,1);auto* row=new QHBoxLayout;auto* back=new QPushButton(words("Back","Voltar"));back->setObjectName("secondary");auto* next=new QPushButton(words("Next","Proximo"));next->setObjectName("save");auto* count=label("","hint");row->addWidget(back);row->addWidget(count,1,Qt::AlignCenter);row->addWidget(next);layout->addLayout(row);
        const QStringList titles={words("Welcome","Bem-vindo"),words("Choose an operator","Selecione um operador"),words("Adjust recoil","Ajuste o recoil"),"Rapid fire",words("Appearance","Aparencia"),words("Hotkeys and saving","Atalhos e configuracoes")};
        const QStringList descriptions={words("Your operator profiles and controls, in one place.","Seus perfis e controles em um so lugar."),words("Select a name in the sidebar, or search for an operator.","Selecione um nome na barra lateral ou pesquise um operador."),words("Set vertical and horizontal force. Use arrow keys for fine adjustments.","Ajuste a forca vertical e horizontal. Use as setas para ajustes finos."),words("Enable the global control, then enable rapid fire for the selected operator.","Habilite o controle global e depois o rapid fire do operador."),words("Open the settings gear to change the accent color and reactive radius.","Abra a engrenagem para alterar a cor e o raio reativo."),words("Set your hotkeys in System settings. Save configuration to keep your changes.","Defina os atalhos em Configuracoes. Salve para manter suas alteracoes.")};
        int step=0;const auto refresh=[&]{title->setText(titles[step]);description->setText(descriptions[step]);count->setText(QString::number(step+1)+" / 6");back->setEnabled(step>0);next->setText(step==5?words("Done","Concluir"):words("Next","Proximo"));};
        connect(back,&QPushButton::clicked,&dialog,[&]{if(step)--step;refresh();});connect(next,&QPushButton::clicked,&dialog,[&]{if(step==5)dialog.accept();else{++step;refresh();}});refresh();roundWindow(&dialog);dialog.exec();
    }
protected:
    void paintEvent(QPaintEvent*) override {
        ++paint_count_;QPainter p(this);p.fillRect(rect(),QColor("#1a191a"));const auto c=engine_.snapshot();const QPoint mouse=mapFromGlobal(QCursor::pos());QColor color=accent();p.setRenderHint(QPainter::Antialiasing);p.setPen(Qt::NoPen);
        static const QPixmap image("titio.png");if(!image.isNull()){p.setOpacity(.035);p.drawPixmap(rect(),image);p.setOpacity(1);}
        for(int x=18;x<width();x+=45)for(int y=90;y<height()-12;y+=45){const auto delta=QPoint(x,y)-mouse;const qreal distance=std::hypot(delta.x(),delta.y());QColor dot=color;dot.setAlpha(8);p.setBrush(dot);p.drawEllipse(QPointF(x,y),1.1,1.1);if(distance<c.radius){const qreal strength=1-distance/c.radius;dot.setAlpha(qRound(strength*15));p.setBrush(dot);p.drawEllipse(QPointF(x,y),1.5+strength*1.4,1.5+strength*1.4);}}
    }
    void showEvent(QShowEvent* event) override {
        QWidget::showEvent(event);engine_.menu_visible=true;if(!native_ready_){native_ready_=true;const HWND handle=HWND(winId());const auto style=GetWindowLongPtrW(handle,GWL_STYLE);SetWindowLongPtrW(handle,GWL_STYLE,style|WS_THICKFRAME|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX);SetWindowPos(handle,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);roundWindow(this);const BOOL dark=TRUE;DwmSetWindowAttribute(handle,20,&dark,sizeof(dark));}
    }
    void hideEvent(QHideEvent* event) override {engine_.menu_visible=false;QWidget::hideEvent(event);}
    void closeEvent(QCloseEvent* event) override {engine_.stop();QWidget::closeEvent(event);}
    void resizeEvent(QResizeEvent* event) override {QWidget::resizeEvent(event);if(native_ready_)roundWindow(this);}
    bool eventFilter(QObject* object,QEvent* event) override {
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
        setObjectName("mainWindow");setWindowTitle("KingBoss2004");setWindowFlags(Qt::Window|Qt::FramelessWindowHint);setWindowIcon(operatorIcon("Ash"));const auto area=QGuiApplication::primaryScreen()->availableGeometry();setMinimumSize(std::min(760,area.width()-32),std::min(560,area.height()-32));resize(std::min(864,area.width()-48),std::min(650,area.height()-48));move(area.center()-rect().center());
        auto* layout=new QVBoxLayout(this);layout->setContentsMargins(0,0,0,0);layout->setSpacing(0);auto* header=new DragHeader;header->setObjectName("titleBar");header->setFixedHeight(46);auto* top=new QHBoxLayout(header);top->setContentsMargins(20,0,10,0);top->setSpacing(5);
        top->addWidget(new BrandLabel);top->addStretch();status_=new QPushButton;status_->setObjectName("status");status_->setToolTip("Toggle recoil activity");top->addWidget(status_);connect(status_,&QPushButton::clicked,this,[this]{toggleRecoil();});
        save_=new QPushButton;save_->setObjectName("saveControl");save_->setAccessibleName("Save configuration");save_->setIcon(QIcon(":/ui/save.svg"));save_->setIconSize(QSize(21,21));save_->setFixedSize(34,34);save_->setCursor(Qt::PointingHandCursor);top->addWidget(save_);connect(save_,&QPushButton::clicked,this,[this]{save();});
        save_feedback_.setSingleShot(true);save_feedback_.setInterval(1600);connect(&save_feedback_,&QTimer::timeout,this,[this]{updateSavePresentation("clean");});
        gear_=new QPushButton;gear_->setIcon(QIcon(":/ui/settings.svg"));gear_->setIconSize(QSize(21,21));gear_->setObjectName("windowControl");gear_->setFixedSize(34,34);gear_->setCursor(Qt::PointingHandCursor);gear_->setAccessibleName("Settings");gear_->setToolTip(words("Settings","Configuracoes"));top->addWidget(gear_);connect(gear_,&QPushButton::clicked,this,[this]{showPreferences();});auto* minimize=new QPushButton(QString::fromUtf8("−"));minimize->setObjectName("windowControl");minimize->setFixedSize(32,32);minimize->setAccessibleName("Minimize");top->addWidget(minimize);connect(minimize,&QPushButton::clicked,this,&QWidget::showMinimized);
        close_=new QPushButton(QString::fromUtf8("×"));close_->setObjectName("closeControl");close_->setFixedSize(34,34);close_->setAccessibleName("Close");top->addWidget(close_);connect(close_,&QPushButton::clicked,this,&QWidget::close);layout->addWidget(header);
        auto* body=new QWidget;auto* columns=new QHBoxLayout(body);columns->setContentsMargins(0,0,0,0);columns->setSpacing(0);
        auto* sidebar=new QWidget;sidebar->setObjectName("operatorSidebar");sidebar->setFixedWidth(200);auto* sidebarLayout=new QVBoxLayout(sidebar);sidebarLayout->setContentsMargins(10,18,2,12);sidebarLayout->setSpacing(8);
        search_=new QLineEdit;search_->setObjectName("search");search_->setClearButtonEnabled(true);search_->setAccessibleName("Search operators");sidebarLayout->addWidget(search_);
        sidebar_scroll_=new QScrollArea;sidebar_scroll_->setObjectName("sidebarScroll");sidebar_scroll_->setWidgetResizable(true);sidebar_scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);sidebarLayout->addWidget(sidebar_scroll_,1);
        auto* sidebarContent=new QWidget;auto* groups=new QVBoxLayout(sidebarContent);groups->setContentsMargins(0,0,0,0);groups->setSpacing(8);
        const auto sectionHeader=[&](QVBoxLayout* column,QLabel* title,const char* dividerName){auto* row=new QHBoxLayout;row->setContentsMargins(10,8,8,0);row->setSpacing(8);row->addWidget(title);auto* line=new QFrame;line->setObjectName(dividerName);line->setFixedHeight(1);row->addWidget(line,1);column->addLayout(row);};
        favorites_group_=new QWidget;favorites_group_->setObjectName("favoritesGroup");auto* favoriteRows=new QVBoxLayout(favorites_group_);favoriteRows->setContentsMargins(0,0,0,0);favoriteRows->setSpacing(8);groups->addWidget(favorites_group_);
        favorites_title_=label("Favorites","favoritesTitle");sectionHeader(favoriteRows,favorites_title_,"favoritesDivider");
        favorites_list_=new OperatorList(sidebar_scroll_);favorites_list_->setObjectName("favorites");favorites_list_->setAccessibleName("Favorite operators");favoriteRows->addWidget(favorites_list_);
        operators_title_=label("All operators","hint");sectionHeader(groups,operators_title_,"operatorsDivider");
        operators_=new OperatorList(sidebar_scroll_);operators_->setObjectName("operators");operators_->setAccessibleName("All operators");groups->addWidget(operators_);groups->addStretch();sidebar_scroll_->setWidget(sidebarContent);FadingScrollBar::install(sidebar_scroll_);
        for(auto* list:{operators_,favorites_list_}){list->setMouseTracking(true);list->setUniformItemSizes(true);list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);list->setItemDelegate(new OperatorDelegate([this]{return accent();},list));list->setContextMenuPolicy(Qt::CustomContextMenu);connect(list,&QWidget::customContextMenuRequested,this,[this,list](const QPoint& point){showOperatorMenu(list,point);});}
        const auto c=engine_.snapshot();for(int i=0;i<r6::operator_count;++i){const QString name=QString::fromUtf8(c.operators[i].name);auto* item=new QListWidgetItem(operatorIcon(name),name);item->setData(Qt::UserRole,i);operators_->addItem(item);QSvgRenderer renderer(":/operators/"+name.toLower()+".svg");if(!renderer.isValid())icon_failures_<<name;}
        operators_->setCurrentRow(c.selected);columns->addWidget(sidebar);
        pages_=new PageStack;pages_->setObjectName("contentPages");operator_page_=new QWidget;operator_page_->setObjectName("operatorPage");auto* rightLayout=new QVBoxLayout(operator_page_);rightLayout->setContentsMargins(20,20,16,14);rightLayout->setSpacing(10);
        auto* selected=new QHBoxLayout;selected->setContentsMargins(2,0,0,0);selected->setSpacing(10);operator_icon_=new FadeLabel;operator_icon_->setFixedSize(46,46);selected->addWidget(operator_icon_);auto* titles=new QVBoxLayout;titles->setSpacing(2);operator_name_=new FadeLabel;operator_name_->setObjectName("pageTitle");subtitle_=label("","subtitle");titles->addWidget(operator_name_);titles->addWidget(subtitle_);selected->addLayout(titles,1);rightLayout->addLayout(selected);
        name_opacity_=static_cast<FadeLabel*>(operator_name_);icon_opacity_=static_cast<FadeLabel*>(operator_icon_);
        scroll_=new QScrollArea;scroll_->setWidgetResizable(true);scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);FadingScrollBar::install(scroll_);rightLayout->addWidget(scroll_,1);pages_->addWidget(operator_page_);
        preferences_=new QWidget;preferences_->setObjectName("preferencesPage");auto* preferenceLayout=new QVBoxLayout(preferences_);preferenceLayout->setContentsMargins(20,20,16,14);preferenceLayout->setSpacing(10);auto* preferenceHeader=new QHBoxLayout;preferenceHeader->setSpacing(10);
        back_=new QPushButton;back_->setObjectName("windowControl");back_->setIcon(QIcon(":/ui/back.svg"));back_->setIconSize(QSize(20,20));back_->setFixedSize(30,30);back_->setAccessibleName("Back to operators");connect(back_,&QPushButton::clicked,this,[this]{showOperators();});preferenceHeader->addWidget(back_);preferences_title_=label("Settings","pageTitle");preferenceHeader->addWidget(preferences_title_,1);preferenceLayout->addLayout(preferenceHeader);
        preferences_scroll_=new QScrollArea;preferences_scroll_->setWidgetResizable(true);preferences_scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);FadingScrollBar::install(preferences_scroll_);preferenceLayout->addWidget(preferences_scroll_,1);pages_->addWidget(preferences_);
        panel_=new RaisedPanel;panel_->setObjectName("raisedPanel");auto* panelLayout=new QVBoxLayout(panel_);panelLayout->setContentsMargins(8,3,12,13);panelLayout->setSpacing(0);panelLayout->addWidget(pages_);columns->addWidget(panel_,1);layout->addWidget(body,1);
        favorites_=readFavorites();refreshFavorites();
        connect(operators_,&QListWidget::currentItemChanged,this,[this](QListWidgetItem* item,QListWidgetItem*){if(item)selectOperator(item->data(Qt::UserRole).toInt());});connect(favorites_list_,&QListWidget::currentItemChanged,this,[this](QListWidgetItem* item,QListWidgetItem*){if(item)selectOperator(item->data(Qt::UserRole).toInt());});
        for(auto* list:{operators_,favorites_list_})connect(list,&QListWidget::itemClicked,this,[this](QListWidgetItem* item){if(item)selectOperator(item->data(Qt::UserRole).toInt());});
        connect(search_,&QLineEdit::textChanged,this,[this]{filterOperators();});
        operator_transition_.setDuration(120);operator_transition_.setStartValue(.35);operator_transition_.setEndValue(1.0);operator_transition_.setEasingCurve(QEasingCurve::InOutQuad);
        connect(&operator_transition_,&QVariantAnimation::valueChanged,this,[this](const QVariant& value){name_opacity_->setOpacity(value.toReal());icon_opacity_->setOpacity(value.toReal());});
        phone_=new PhoneServer([this]{return mobileState();},[this](const QJsonObject& request){return mobileUpdate(request);},this);if(c.phone_enabled)phone_->start();
        buildPreferences();
        buildSettings();updateLanguage();applyStyle();qApp->installEventFilter(this);clock_.start();poll_.setInterval(8);connect(&poll_,&QTimer::timeout,this,[this]{pollKeys();});poll_.start();paint_.setInterval(50);connect(&paint_,&QTimer::timeout,this,[this]{if(isVisible()&&!isMinimized())update();});paint_.start();
    }
    void capture(const QString& file){grab().save(file);}
    void writePhoneInfo(const QString& path) const {if(!engine_.dry_run())return;QFile file(path);if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(QJsonObject{{"port",int(phone_->port())},{"address",phone_->address()},{"token",phone_->token()},{"code",phone_->code()},{"pid",int(QCoreApplication::applicationPid())},{"dryRun",true}}).toJson());}
    void closeUsingControl(){close_->click();}
    QJsonObject selfTest(const QString& output){
        const auto wait=[](int milliseconds){QEventLoop loop;QTimer::singleShot(milliseconds,&loop,&QEventLoop::quit);loop.exec();};
        QJsonObject checks;const auto available=QGuiApplication::primaryScreen()->availableGeometry();checks["window_fits_available_screen"]=width()<=available.width()&&height()<=available.height();checks["compact_default_when_screen_allows"]=available.width()<912||available.height()<698||size()==QSize(864,650);checks["sidebar_is_narrower"]=findChild<QWidget*>("operatorSidebar")->width()==200;checks["kingboss_branding"]=windowTitle()=="KingBoss2004"&&findChild<QLabel*>("brand")->text()=="[ KING BOSS ]";checks["all_71_operators"]=operators_->count()==71;checks["all_71_svg_icons_valid"]=icon_failures_.empty();checks["actual_native_window_not_fullscreen"]=!isFullScreen();
        auto* sidebarBar=static_cast<FadingScrollBar*>(sidebar_scroll_->verticalScrollBar());checks["scrollbars_start_invisible"]=sidebarBar->opacity()==0&&static_cast<FadingScrollBar*>(scroll_->verticalScrollBar())->opacity()==0&&static_cast<FadingScrollBar*>(preferences_scroll_->verticalScrollBar())->opacity()==0;
        checks["one_shared_sidebar_scroll"]=sidebar_scroll_->widget()->isAncestorOf(operators_)&&sidebar_scroll_->widget()->isAncestorOf(favorites_list_)&&operators_->verticalScrollBarPolicy()==Qt::ScrollBarAlwaysOff&&favorites_list_->verticalScrollBarPolicy()==Qt::ScrollBarAlwaysOff;
        checks["raised_panel_contains_both_pages"]=panel_->isAncestorOf(operator_page_)&&panel_->isAncestorOf(preferences_)&&!panel_->graphicsEffect()&&pages_->width()>findChild<QWidget*>("operatorSidebar")->width()*3;
        search_->setText("Blackbeard");int visible=0;for(int i=0;i<operators_->count();++i)if(!operators_->item(i)->isHidden())++visible;checks["search_filters_sidebar"]=visible==1;search_->clear();
        operators_->setCurrentRow(4);vertical_->setValue(12.3);horizontal_->setValue(-2.4);humanize_->setChecked(false);operator_rapid_->setChecked(true);delay_->setValue(75);auto c=engine_.snapshot();checks["controls_update_selected_operator"]=c.selected==4&&std::abs(c.operators[4].vertical-12.3f)<.001f&&std::abs(c.operators[4].horizontal+2.4f)<.001f&&!c.operators[4].humanize&&c.operators[4].rapid&&c.operators[4].delay==75;
        operators_->setCurrentRow(0);checks["other_profile_preserved"]=vertical_->value()==10&&horizontal_->value()==0;operators_->setCurrentRow(4);const auto fixture=std::filesystem::path(output.toStdWString())/L"ui-test-config.bin";r6::Config readback;checks["3734_byte_compatible_settings"]=engine_.save(fixture)&&readback.load(fixture)&&readback.bytes()==engine_.snapshot().bytes();
        save_->click();r6::Config saved;checks["save_button_writes_settings"]=saved.load(config_path_)&&saved.bytes()==engine_.snapshot().bytes();checks["save_icon_feedback"]=save_->property("saveState").toString()=="saved";
        const bool masterBefore=engine_.recoil_master;recoil_master_->click();checks["master_recoil_control"]=engine_.recoil_master.load()!=masterBefore;recoil_master_->click();const bool rapidBefore=engine_.rapid_master;rapid_master_->click();checks["global_rapid_control"]=engine_.rapid_master.load()!=rapidBefore&&bool(engine_.snapshot().rapid_master)==engine_.rapid_master.load();rapid_master_->click();
        const auto before=engine_.recoil_active.load();checks["initial_recoil_off"]=!before;status_->click();checks["activation_control"]=engine_.recoil_active.load()!=before;status_->click();checks["second_click_restores_off"]=!engine_.recoil_active.load();
        checks["consistent_system_typography"]=QFontInfo(operator_name_->font()).family()=="Segoe UI"&&QFontInfo(save_->font()).family()=="Segoe UI"&&QFontInfo(status_->font()).family()=="Segoe UI"&&QFontInfo(findChild<QLabel*>("brand")->font()).family()=="Consolas";
        checks["compact_operator_rows"]=operators_->sizeHintForRow(0)==56&&operators_->item(0)->icon().availableSizes().contains(QSize(40,40));
        checks["status_has_no_hotkey"]=status_->text()=="Recoil off";
        checks["save_icon_next_to_settings"]=save_->text().isEmpty()&&!save_->icon().isNull()&&save_->parentWidget()==gear_->parentWidget()&&save_->geometry().right()<gear_->geometry().left();
        checks["footer_removed"]=!findChild<QFrame*>("footer");checks["small_r6_label_removed"]=!findChild<QLabel*>("r6Badge");
        bool question=false;for(auto* button:findChildren<QPushButton*>())question|=button->text()=="?";checks["no_question_mark_control"]=!question;

        DWORD preference=0;const auto hr=DwmGetWindowAttribute(HWND(winId()),33,&preference,sizeof(preference));HRGN region=CreateRectRgn(0,0,0,0);const int regionType=GetWindowRgn(HWND(winId()),region);const bool clipped=regionType!=ERROR&&!PtInRegion(region,0,0)&&PtInRegion(region,qRound(width()*devicePixelRatioF()/2),qRound(height()*devicePixelRatioF()/2));DeleteObject(region);
        checks["actual_rounded_window_shape"]=(SUCCEEDED(hr)&&preference==2)||clipped;checks["sendinput_suppressed_in_dry_run"]=engine_.dry_run()&&engine_.injected_events==0;
        scroll_->verticalScrollBar()->setValue(0);QApplication::processEvents();capture(output+"/window.png");
        const auto stationaryPanel=panel_->geometry();auto* preferenceContent=preferences_scroll_->widget();
        gear_->click();checks["gear_opens_inline_settings"]=pages_->currentWidget()==preferences_&&preferences_->window()==this&&!QApplication::activeModalWidget();
        checks["settings_navigation_starts_crossfade"]=pages_->transitioning()&&preferences_scroll_->widget()==preferenceContent;
        wait(80);capture(output+"/settings-transition.png");wait(140);checks["settings_crossfade_finishes_without_moving_panel"]=!pages_->transitioning()&&panel_->geometry()==stationaryPanel;
        back_->click();wait(35);gear_->click();wait(35);back_->click();wait(220);checks["rapid_settings_navigation_finishes_on_last_page"]=pages_->currentWidget()==operator_page_&&!pages_->transitioning()&&panel_->geometry()==stationaryPanel;gear_->click();wait(220);
        checks["settings_exposes_hotkeys"]=language_&&key_buttons_[0]&&key_buttons_[0]->accessibleName()=="Open / hide window";
        language_->setCurrentIndex(1);QApplication::processEvents();checks["portuguese_labels"]=operators_title_->text()=="Todos os operadores"&&save_->toolTip().startsWith("Salvar configuracoes");language_->setCurrentIndex(0);QApplication::processEvents();
        preferences_scroll_->verticalScrollBar()->setValue(0);QApplication::processEvents();capture(output+"/system.png");back_->click();checks["settings_back_preserves_profile"]=pages_->currentWidget()==operator_page_&&engine_.snapshot().selected==4&&vertical_->value()==12.3;
        const auto originalFavorites=favorites_;favorites_.clear();persistFavorites();refreshFavorites();wait(220);checks["empty_favorites_hide_entire_group"]=favorites_group_->isHidden()&&!favorites_title_->isVisible()&&favorites_list_->isHidden();capture(output+"/empty-favorites.png");
        setFavorite(4,false);operators_->scrollToItem(operators_->item(4));QApplication::processEvents();
        bool favoriteAction=false,compactMenu=false,insetHover=false;QTimer::singleShot(50,this,[&]{auto* popup=QApplication::activePopupWidget();auto* button=popup?popup->findChild<QPushButton*>("toggleFavorite"):nullptr;if(button){favoriteAction=!button->icon().isNull();compactMenu=popup->width()==178&&popup->height()==52;const auto before=popup->grab().toImage();button->setAttribute(Qt::WA_UnderMouse,true);button->update();const auto after=popup->grab().toImage();insetHover=button->geometry().left()>=6&&button->geometry().top()>=6&&before.pixelColor(3,3)==after.pixelColor(3,3);popup->grab().save(output+"/favorite-menu.png");button->click();}});
        emit operators_->customContextMenuRequested(operators_->visualItemRect(operators_->item(4)).center());
        checks["right_click_adds_favorite"]=favoriteAction&&favorites_.contains("Ash")&&favorites_list_->count()>0&&favorites_group_->isVisible();checks["favorite_menu_is_compact"]=compactMenu;checks["favorite_hover_stays_inset"]=insetHover;checks["favorite_appears_only_once"]=operators_->item(4)->isHidden()&&!favorites_list_->isHidden();checks["favorites_persist_to_disk"]=readFavorites()==favorites_;
        {MainWindow reopened(engine_,config_path_);checks["favorites_reload_on_fresh_window"]=reopened.favorites_.contains("Ash")&&reopened.favorites_list_->count()==favorites_list_->count();}
        int favoriteRow=-1;for(int i=0;i<favorites_list_->count();++i)if(favorites_list_->item(i)->data(Qt::UserRole).toInt()==4)favoriteRow=i;
        operators_->setCurrentRow(0);favorites_list_->setCurrentRow(favoriteRow);checks["favorite_selects_correct_profile"]=engine_.snapshot().selected==4&&vertical_->value()==12.3;
        const auto panelRect=panel_->geometry(),controlsRect=settings_->geometry();
        operators_->setCurrentRow(5);emit operators_->itemClicked(operators_->item(5));checks["operator_transition_starts"]=operator_transition_.state()==QAbstractAnimation::Running&&name_opacity_->opacity()<1&&icon_opacity_->opacity()<1&&operator_transition_.duration()==120;
        checks["controls_update_during_fade"]=vertical_->value()==engine_.snapshot().operators[5].vertical&&settings_->geometry()==controlsRect&&panel_->geometry()==panelRect&&!operator_page_->graphicsEffect()&&!settings_->graphicsEffect();
        operators_->setCurrentRow(1);wait(30);operators_->setCurrentRow(4);wait(170);checks["rapid_switching_finishes_on_last_profile"]=operator_transition_.state()==QAbstractAnimation::Stopped&&name_opacity_->opacity()==1&&icon_opacity_->opacity()==1&&engine_.snapshot().selected==4&&operator_name_->text()=="Ash"&&vertical_->value()==12.3;
        operators_->setCurrentRow(5);gear_->click();checks["settings_interrupts_fade_cleanly"]=name_opacity_->opacity()==1&&panel_->geometry()==panelRect;operators_->setCurrentRow(1);checks["all_list_selection_exits_settings"]=pages_->currentWidget()==operator_page_&&engine_.snapshot().selected==1;gear_->click();favorites_list_->setCurrentRow(favoriteRow);checks["favorite_selection_exits_settings"]=pages_->currentWidget()==operator_page_&&engine_.snapshot().selected==4;
        search_->clear();sidebar_scroll_->verticalScrollBar()->setValue(0);QApplication::processEvents();capture(output+"/favorites.png");
        search_->setText("Ash");int favoriteMatches=0;for(int row=0;row<favorites_list_->count();++row)if(!favorites_list_->item(row)->isHidden())++favoriteMatches;checks["search_filters_without_duplicates"]=!favorites_list_->isHidden()&&operators_->item(4)->isHidden()&&operators_->height()==56&&favorites_list_->height()==favoriteMatches*56;search_->clear();
        search_->setText("Blackbeard");checks["search_filters_favorites"]=favorites_list_->isHidden()&&favorites_group_->isHidden();search_->clear();
        QTimer::singleShot(50,this,[&]{auto* popup=QApplication::activePopupWidget();auto* button=popup?popup->findChild<QPushButton*>("toggleFavorite"):nullptr;if(button)button->click();});emit favorites_list_->customContextMenuRequested(favorites_list_->visualItemRect(favorites_list_->item(favoriteRow)).center());
        bool removed=true;for(int i=0;i<favorites_list_->count();++i)removed&=favorites_list_->item(i)->data(Qt::UserRole).toInt()!=4;checks["favorite_removal_updates_sidebar"]=removed&&!favorites_.contains("Ash")&&readFavorites()==favorites_&&favorites_group_->isHidden()&&!operators_->item(4)->isHidden();favorites_=originalFavorites;persistFavorites();refreshFavorites();
        for(int i=0;i<6;++i)setFavorite(i,true);QApplication::processEvents();checks["favorites_have_no_separate_scroll_or_cap"]=favorites_list_->height()==favorites_list_->count()*56&&favorites_list_->height()>3*56&&favorites_list_->verticalScrollBar()->maximum()==0;
        // Real viewport wheel input must reach the common sidebar, even over a list.
        sidebarBar->setValue(0);const auto wheelPoint=operators_->viewport()->rect().center();QWheelEvent wheel(QPointF(wheelPoint),QPointF(operators_->viewport()->mapToGlobal(wheelPoint)),QPoint(),QPoint(0,-120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);QApplication::sendEvent(operators_->viewport(),&wheel);wait(130);checks["wheel_over_list_scrolls_shared_sidebar"]=sidebarBar->value()>0&&sidebarBar->opacity()>.99;
        favorites_=originalFavorites;persistFavorites();refreshFavorites();QApplication::processEvents();
        selectOperator(4);dual_->click();checks["secondary_mode_reveals_selector"]=weapon_selector_->isVisible()&&engine_.snapshot().active_slot()==1&&vertical_->value()==12.3;
        vertical_->setValue(28.7);horizontal_->setValue(3.2);humanize_->setChecked(true);operator_rapid_->setChecked(false);
        handleWeaponKeys(false,false,false,false);handleWeaponKeys(true,false,false,false);checks["primary_key_restores_primary_profile"]=engine_.snapshot().active_slot()==0&&vertical_->value()==12.3&&horizontal_->value()==-2.4&&operator_rapid_->isChecked();
        handleWeaponKeys(false,false,false,false);handleWeaponKeys(false,true,false,false);checks["secondary_key_restores_secondary_profile"]=engine_.snapshot().active_slot()==1&&vertical_->value()==28.7&&horizontal_->value()==3.2&&humanize_->isChecked()&&!operator_rapid_->isChecked();
        handleWeaponKeys(false,false,false,false);handleWeaponKeys(true,false,true,false);checks["weapon_keys_do_not_interrupt_number_entry"]=engine_.snapshot().active_slot()==1;
        handleWeaponKeys(false,false,false,false);handleWeaponKeys(true,false,false,true);checks["modified_shortcuts_do_not_switch_weapons"]=engine_.snapshot().active_slot()==1;handleWeaponKeys(false,false,false,false);
        scroll_->verticalScrollBar()->setValue(0);wait(220);capture(output+"/weapons.png");
        dual_->click();checks["disabling_secondary_restores_primary"]=engine_.snapshot().active_slot()==0&&vertical_->value()==12.3&&weapon_selector_->isHidden();dual_->click();checks["reenabling_secondary_preserves_edits"]=engine_.snapshot().active_slot()==1&&vertical_->value()==28.7;
        save_->click();{r6::Engine restored(true);const bool loaded=restored.load(config_path_);const auto saved=restored.snapshot();checks["both_weapon_profiles_reload_from_disk"]=loaded&&saved.secondary_enabled[4]&&std::abs(saved.secondary[4].vertical-28.7f)<.001&&std::abs(saved.operators[4].vertical-12.3f)<.001&&saved.primary_key=='1'&&saved.secondary_key=='2';}
        selectOperator(0);checks["secondary_is_optional_for_each_operator"]=!engine_.snapshot().secondary_enabled[0]&&!engine_.select_weapon(1)&&engine_.snapshot().active_slot()==0;
        selectOperator(4);checks["character_selection_starts_on_primary"]=engine_.snapshot().active_slot()==0&&vertical_->value()==12.3;dual_->click();save_->click();
        const auto unmodified=engine_.snapshot().bytes();checks["phone_rejects_invalid_values"]=mobileUpdate({{"action","setProfile"},{"operator",4},{"slot",0},{"values",QJsonObject{{"vertical",-3}}}}).contains("error")&&engine_.snapshot().bytes()==unmodified;
        checks["phone_server_listens_and_provides_pairing"]=phone_->listening()&&phone_->port()>0&&phone_->code().size()==6&&phone_->token().size()==32;
        checks["phone_state_matches_desktop"]=mobileState()["operatorId"].toInt()==4&&mobileState()["profile"].toObject()["vertical"].toDouble()==12.3;
        // Exercise timing and drag behavior on all three production scrollbars.
        settings_->setMinimumHeight(scroll_->viewport()->height()+300);QApplication::processEvents();gear_->click();QApplication::processEvents();
        auto* operatorBar=static_cast<FadingScrollBar*>(scroll_->verticalScrollBar());auto* preferenceBar=static_cast<FadingScrollBar*>(preferences_scroll_->verticalScrollBar());
        const auto sidebarWidth=sidebar_scroll_->viewport()->width(),preferenceWidth=preferences_scroll_->viewport()->width();
        for(auto* bar:{sidebarBar,operatorBar,preferenceBar}){bar->setValue(1);bar->reveal();}wait(130);
        checks["all_scrollbars_fade_in"]=sidebarBar->opacity()>.99&&operatorBar->opacity()>.99&&preferenceBar->opacity()>.99;
        wait(2100);for(auto* bar:{sidebarBar,operatorBar,preferenceBar})bar->setValue(2);wait(1100);
        checks["scrolling_resets_three_second_timer"]=sidebarBar->opacity()>.99&&operatorBar->opacity()>.99&&preferenceBar->opacity()>.99;
        wait(2250);checks["all_scrollbars_fade_out_after_idle"]=sidebarBar->opacity()==0&&operatorBar->opacity()==0&&preferenceBar->opacity()==0;
        checks["scrollbar_fades_do_not_shift_content"]=sidebarWidth==sidebar_scroll_->viewport()->width()&&preferenceWidth==preferences_scroll_->viewport()->width();
        for(auto* bar:{sidebarBar,operatorBar,preferenceBar})bar->setSliderDown(true);wait(3350);
        checks["drag_keeps_all_scrollbars_visible"]=sidebarBar->opacity()>.99&&operatorBar->opacity()>.99&&preferenceBar->opacity()>.99;
        for(auto* bar:{sidebarBar,operatorBar,preferenceBar})bar->setSliderDown(false);wait(3350);
        checks["scrollbars_fade_after_drag_release"]=sidebarBar->opacity()==0&&operatorBar->opacity()==0&&preferenceBar->opacity()==0;
        settings_->setMinimumHeight(0);back_->click();sidebarBar->setValue(0);scroll_->verticalScrollBar()->setValue(0);showNormal();raise();activateWindow();wait(3400);capture(output+"/finished.png");
        const auto unchanged=engine_.snapshot().bytes();QTimer::singleShot(80,this,[=]{auto* modal=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(modal){modal->grab().save(output+"/color.png");modal->reject();}});chooseColor();checks["cancel_color_preserves_settings"]=engine_.snapshot().bytes()==unchanged;
        QTimer::singleShot(80,this,[=]{auto* modal=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(modal){modal->grab().save(output+"/binding.png");modal->reject();}});bindKey(0);checks["cancel_key_binding_preserves_settings"]=engine_.snapshot().bytes()==unchanged;
        QTimer::singleShot(80,this,[=]{auto* modal=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(modal){modal->grab().save(output+"/help.png");modal->reject();}});help();checks["help_dialog_preserves_settings"]=engine_.snapshot().bytes()==unchanged;
        QJsonObject report;report["checks"]=checks;report["rounding_method"]=clipped?"native rounded window region (Windows 10)":"DWM rounded corners";report["icon_failures"]=QJsonArray::fromStringList(icon_failures_);report["font_family"]=QFontInfo(operator_name_->font()).family();report["heading_font_style"]=QFontInfo(operator_name_->font()).styleName();report["body_font_style"]=QFontInfo(humanize_->parentWidget()->findChild<QLabel*>()->font()).styleName();report["save_font_family"]=QFontInfo(save_->font()).family();report["paint_events"]=paint_count_;report["mouse_api"]="SendInput";report["input_injection"]="disabled";report["width"]=width();report["height"]=height();report["status_text"]=status_->text();report["display_scale"]=devicePixelRatioF();bool passed=true;for(const auto& check:checks)passed&=check.toBool();report["passed"]=passed;return report;
    }
};

int main(int argc,char** argv){
    QApplication app(argc,argv);QApplication::setApplicationName("KingBoss2004");QApplication::setOrganizationName("KingBoss2004");QApplication::setStyle("Fusion");QFont font("Segoe UI");font.setPixelSize(13);font.setWeight(QFont::Normal);font.setHintingPreference(QFont::PreferNoHinting);font.setStyleStrategy(QFont::PreferAntialias);app.setFont(font);
    QPalette palette;palette.setColor(QPalette::Window,QColor("#1a191a"));palette.setColor(QPalette::WindowText,QColor("#d0cfd0"));palette.setColor(QPalette::Base,QColor("#252426"));palette.setColor(QPalette::AlternateBase,QColor("#302d32"));palette.setColor(QPalette::Text,QColor("#d0cfd0"));palette.setColor(QPalette::Button,QColor("#343135"));palette.setColor(QPalette::ButtonText,QColor("#d0cfd0"));palette.setColor(QPalette::Highlight,QColor("#058669"));palette.setColor(QPalette::HighlightedText,Qt::white);palette.setColor(QPalette::Disabled,QPalette::Text,QColor("#77727c"));app.setPalette(palette);
    const auto arguments=app.arguments();const bool selftest=arguments.contains("--ui-self-test"),dryrun=selftest||arguments.contains("--dry-run");QString output,phoneInfo;int duration=0;for(const auto& argument:arguments){if(argument.startsWith("--phone-info="))phoneInfo=argument.mid(13);if(argument.startsWith("--output="))output=argument.mid(9);if(argument.startsWith("--exit-after-ms="))duration=argument.mid(16).toInt();}if(output.isEmpty())output=QDir::currentPath();QDir().mkpath(output);
    r6::Engine engine(dryrun);const auto configPath=std::filesystem::current_path()/L"r6_config.bin";if(!engine.load(configPath))engine.edit([](r6::Config& c){c.language=1;});engine.start();MainWindow window(engine,configPath);window.show();if(!phoneInfo.isEmpty())window.writePhoneInfo(phoneInfo);int testResult=0;
    if(selftest)QTimer::singleShot(700,&window,[&]{const auto report=window.selfTest(output);QFile file(output+"/ui-verification.json");file.open(QIODevice::WriteOnly);file.write(QJsonDocument(report).toJson());testResult=report["passed"].toBool()?0:2;QTimer::singleShot(150,&window,[&]{window.closeUsingControl();});});else if(duration>0)QTimer::singleShot(duration,&window,&QWidget::close);
    const int result=app.exec();engine.stop();return selftest?testResult:result;
}

