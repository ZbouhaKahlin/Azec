#include "main_window.hpp"

QJsonObject MainWindow::mobileState() const {
        const auto c=engine_.snapshot();QJsonArray operators;
        for(int id=0;id<r6::operator_count;++id){const auto name=QString::fromUtf8(c.operators[id].name);operators.append(QJsonObject{{"id",id},{"name",name},{"favorite",favorites_.contains(name)},{"dual",c.secondary_enabled[id]}});}
        QJsonArray keys;const int values[]={c.menu_key,c.recoil_key,c.panic_key,c.primary_key,c.secondary_key};const char* labels[]={"Open / hide window","Recoil toggle","Exit key","Primary weapon","Secondary weapon"};
        for(int i=0;i<5;++i)keys.append(QJsonObject{{"label",labels[i]},{"value",values[i]},{"name",bindingName(values[i])}});
        QJsonArray themes;for(const auto& theme:kbtheme::colorThemes)themes.append(QJsonObject{{"id",theme.id},{"name",theme.name}});
        const QJsonObject colors{{"paper",kbtheme::paper.name()},{"surface",kbtheme::surface.name()},{"card",kbtheme::card.name()},{"ink",kbtheme::ink.name()},{"muted",kbtheme::muted.name()},{"accent",accent().name()},{"accent-ink",kbtheme::readableOn(accent()).name()},{"lavender",kbtheme::lavender.name()},{"outline",kbtheme::outline.name()},{"chip-ink",kbtheme::chipInk.name()}};
        const QJsonObject appearance{{"id",kbtheme::activeColorTheme},{"scheme",kbtheme::paper.lightness()<128?"dark":"light"},{"colors",colors},{"themes",themes}};
        return {{"operators",operators},{"operatorId",c.selected},{"weaponSlot",c.active_slot()},{"dual",c.secondary_enabled[c.selected]},{"profile",profileJson(c.active_operator())},{"dirty",unsaved_},{"keys",keys},{"appearance",appearance},
            {"globals",QJsonObject{{"recoilMaster",engine_.recoil_master.load()},{"recoilActive",engine_.recoil_active.load()},{"sound",bool(c.sound)},{"language",c.language},{"accent",accent().name()}}}};
    }
QJsonObject MainWindow::mobileUpdate(const QJsonObject& request){
        const auto fail=[](const QString& message){return QJsonObject{{"error",message}};};
        const auto action=request["action"].toString();auto next=engine_.snapshot();
        const auto validNumber=[](const QJsonValue& v,double low,double high){return v.isDouble()&&std::isfinite(v.toDouble())&&v.toDouble()>=low&&v.toDouble()<=high;};
        const auto validInteger=[&](const QJsonValue& v,int low,int high){return validNumber(v,low,high)&&std::floor(v.toDouble())==v.toDouble();};
        if(action=="save"){save();if(save_->property("saveState").toString()=="error")return fail("Could not save configuration");return mobileState();}
        if(action=="setTheme"){
            const auto id=request["theme"].toString();
            if(!request["theme"].isString()||id!=QLatin1String(kbtheme::colorTheme(id).id))return fail("Invalid color theme");
            if(!chooseColorTheme(id))return fail(text(UiText::ThemeSaveFailed));return mobileState();
        }
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
            const auto before=next;
            const auto values=request["values"].toObject();if(values.isEmpty())return fail("No settings values");
            bool master=engine_.recoil_master,active=engine_.recoil_active;bool rebuild=false,keysChanged=false;
            for(auto it=values.begin();it!=values.end();++it){const auto key=it.key();const auto value=it.value();
                if((key=="recoilMaster"||key=="recoilActive"||key=="sound")&&value.isBool()){
                    if(key=="recoilMaster")master=value.toBool();else if(key=="recoilActive")active=value.toBool();else {next.sound=value.toBool();rebuild=true;}
                }else if(key=="language"&&validInteger(value,0,8)){next.language=value.toInt();rebuild=true;}
                else if(key.startsWith("key")&&key.size()==4&&key[3]>='0'&&key[3]<='4'&&validInteger(value,0,255)&&r6::validHotkey(value.toInt())){int* keys[]={&next.menu_key,&next.recoil_key,&next.panic_key,&next.primary_key,&next.secondary_key};*keys[key[3].digitValue()]=value.toInt();rebuild=keysChanged=true;}
                else return fail("Invalid setting: "+key);
            }
            if(!uniqueHotkeys(next))return fail("Each action must use a different hotkey");
            const bool persistenceChanged=next.bytes()!=before.bytes()||next.primary_key!=before.primary_key||next.secondary_key!=before.secondary_key||master!=engine_.recoil_master;
            engine_.edit([&](r6::Config& c){c=next;});engine_.recoil_master=master;engine_.recoil_active=active;if(persistenceChanged)dirty();applyStyle();syncSelection();updateStatus();
            if(rebuild)QTimer::singleShot(0,this,[this]{buildPreferences();buildSettings();updateLanguage();});if(keysChanged)save();return mobileState();
        }
        return fail("Unknown action");
    }
