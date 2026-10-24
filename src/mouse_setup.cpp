#include "main_window.hpp"
#include "profile_transfer.hpp"

namespace {
QString uniqueSetName(const r6::Config& config,const QString& proposed){
    const auto base=proposed.left(70).trimmed();QString name=base;int suffix=2;
    const auto exists=[&](const QString& candidate){return std::any_of(config.profile_sets.begin(),config.profile_sets.end(),[&](const auto& set){return QString::fromStdString(set.name).compare(candidate,Qt::CaseInsensitive)==0;});};
    while(exists(name))name=base+" ("+QString::number(suffix++)+")";return name;
}
bool validSetName(const r6::Config& config,const QString& name,const std::string& except={}){
    return !name.trimmed().isEmpty()&&name.size()<=80&&std::none_of(config.profile_sets.begin(),config.profile_sets.end(),[&](const auto& set){return set.id!=except&&QString::fromStdString(set.name).toCaseFolded()==name.toCaseFolded();});
}
std::string newSetId(){return QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();}
void preserveCurrentSet(r6::Config& config,const QString& fallback,const QString& reserved={}){
    profilesets::remember(config);
    if(config.active_profile_set.empty()){
        const auto id=newSetId();auto name=uniqueSetName(config,fallback);if(name.compare(reserved,Qt::CaseInsensitive)==0)name=uniqueSetName(config,name+" (1)");config.profile_sets.push_back(profilesets::capture(config,id,name.toStdString()));config.active_profile_set=id;
    }
}
}

r6::SensitivitySetup MainWindow::mouseSetupInput() const {
    r6::SensitivitySetup setup;
    if(mouse_dpi_)setup.dpi=mouse_dpi_->value();
    if(mouse_horizontal_)setup.horizontal=mouse_horizontal_->value();
    if(mouse_vertical_)setup.vertical=mouse_vertical_->value();
    if(mouse_multiplier_)setup.multiplier=mouse_multiplier_->value();
    setup.source=mouse_source_.toStdWString();return setup;
}
r6::SensitivitySetup MainWindow::converterSourceInput() const {
    r6::SensitivitySetup setup;
    if(converter_dpi_)setup.dpi=converter_dpi_->value();
    if(converter_horizontal_)setup.horizontal=converter_horizontal_->value();
    if(converter_vertical_)setup.vertical=converter_vertical_->value();
    if(converter_multiplier_)setup.multiplier=converter_multiplier_->value();return setup;
}

void MainWindow::buildMouseSetup(QVBoxLayout* column){
    const auto config=engine_.snapshot();
    auto* library=new Section(text(UiText::ProfileSets),false,true,kbtheme::pink);column->addWidget(library);
    auto* libraryHint=label(text(UiText::ProfileSetsHint),"rowHint");libraryHint->setWordWrap(true);libraryHint->setContentsMargins(14,4,12,4);library->body->addWidget(libraryHint);
    auto* row=new QHBoxLayout;row->setContentsMargins(14,0,12,8);row->setSpacing(8);library->body->addLayout(row);profile_sets_=new LanguageComboBox;
    profile_sets_->setMinimumWidth(0);profile_sets_->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);profile_sets_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);profile_sets_->setMinimumContentsLength(8);profile_sets_->setAccessibleName(text(UiText::ActiveProfileSet));
    if(config.active_profile_set.empty())profile_sets_->addItem(text(UiText::CurrentProfiles),QString());
    for(const auto& set:config.profile_sets)profile_sets_->addItem(QString::fromStdString(set.name),QString::fromStdString(set.id));
    profile_sets_->setCurrentIndex(profile_sets_->findData(QString::fromStdString(config.active_profile_set)));row->addWidget(profile_sets_,1);
    auto* options=new QPushButton(QString::fromUtf8("⋯"));options->setObjectName("secondary");options->setFixedSize(44,38);options->setAccessibleName(text(UiText::ProfileSetOptions));options->setToolTip(text(UiText::ProfileSetOptions));options->setEnabled(!save_blocked_);row->addWidget(options);
    connect(options,&QPushButton::clicked,this,[this,options]{
        QMenu menu(this);const auto config=engine_.snapshot();const UiText actions[]={UiText::DuplicateSet,UiText::RenameSet,UiText::DeleteSet};
        for(int i=0;i<3;++i){if(i==2)menu.addSeparator();auto* action=menu.addAction(text(actions[i]));action->setData(i);action->setEnabled(i==0||!config.active_profile_set.empty());}
        // Rebuilding Settings deletes the menu's button, so close the menu first.
        auto* chosen=menu.exec(options->mapToGlobal(QPoint(0,options->height())));if(chosen)manageProfileSet(chosen->data().toInt());
    });
    profile_sets_->setEnabled(!save_blocked_);connect(profile_sets_,qOverload<int>(&QComboBox::activated),this,[this]{manageProfileSet(3);});

    mouse_setup_=new Section(text(UiText::ConvertSensitivity),false,true,kbtheme::yellow);column->addWidget(mouse_setup_);auto* section=mouse_setup_.data();
    const auto step=[&](UiText title){auto* heading=label(text(title),"rowTitle");heading->setWordWrap(true);heading->setContentsMargins(14,8,12,0);section->body->addWidget(heading);};
    step(UiText::ChooseConversionSource);
    auto* sourceStatus=label(converter_import_?text(UiText::ImportedSource).arg(converter_import_name_):text(UiText::CurrentProfiles),"rowHint");sourceStatus->setTextFormat(Qt::PlainText);sourceStatus->setWordWrap(true);sourceStatus->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);sourceStatus->setContentsMargins(14,0,12,0);section->body->addWidget(sourceStatus);
    auto* sourceActions=new QVBoxLayout;sourceActions->setContentsMargins(14,6,12,8);sourceActions->setSpacing(8);
    auto* import=new QPushButton(text(UiText::ChooseBackup));auto* current=new QPushButton(text(UiText::UseCurrentProfiles));if(!converter_import_)current->hide();
    for(auto* button:{import,current}){button->setObjectName("secondary");sourceActions->addWidget(button);}section->body->addLayout(sourceActions);
    connect(import,&QPushButton::clicked,this,[this]{importForConversion();});connect(current,&QPushButton::clicked,this,[this]{resetConverter();buildPreferences();});
    step(UiText::ConversionValues);

    const auto from=converter_draft_.value_or(converter_import_?converter_import_->sensitivity:config.sensitivity);
    const auto to=mouse_draft_.value_or(config.sensitivity);mouse_source_=QString::fromStdWString(to.source);
    auto* grid=new QGridLayout;grid->setContentsMargins(14,4,12,8);grid->setHorizontalSpacing(10);grid->setVerticalSpacing(10);grid->setColumnStretch(0,1);
    for(int i=0;i<2;++i){auto* heading=label(text(i?UiText::ToSensitivity:UiText::FromSensitivity),"rowHint");heading->setAlignment(Qt::AlignCenter);grid->addWidget(heading,0,i+1);}
    const auto dpi=[&](int col,int value,const QString& prefix){auto* field=new QSpinBox;field->setRange(0,100000);field->setSingleStep(50);field->setSpecialValueText(QString::fromUtf8("—"));field->setButtonSymbols(QAbstractSpinBox::NoButtons);field->setFixedWidth(100);field->setAlignment(Qt::AlignCenter);field->setAccessibleName(prefix+" · "+text(UiText::MouseDpi));field->setValue(value);grid->addWidget(field,1,col);return field;};
    grid->addWidget(label(text(UiText::MouseDpi)),1,0);converter_dpi_=dpi(1,from.dpi,text(UiText::FromSensitivity));mouse_dpi_=dpi(2,to.dpi,text(UiText::ToSensitivity));
    const auto number=[&](UiText title,int r,int col,int decimals,double maximum,double step,double value){
        if(col==1){auto* titleLabel=label(text(title));titleLabel->setWordWrap(true);grid->addWidget(titleLabel,r,0);}
        auto* field=new QDoubleSpinBox;field->setRange(0,maximum);field->setDecimals(decimals);field->setSingleStep(step);field->setSpecialValueText(QString::fromUtf8("—"));field->setButtonSymbols(QAbstractSpinBox::NoButtons);field->setFixedWidth(100);field->setAlignment(Qt::AlignCenter);field->setAccessibleName(text(col==1?UiText::FromSensitivity:UiText::ToSensitivity)+" · "+text(title));field->setValue(value);grid->addWidget(field,r,col);return field;
    };
    converter_horizontal_=number(UiText::SensitivityHorizontal,2,1,3,100,1,from.horizontal);mouse_horizontal_=number(UiText::SensitivityHorizontal,2,2,3,100,1,to.horizontal);
    converter_vertical_=number(UiText::SensitivityVertical,3,1,3,100,1,from.vertical);mouse_vertical_=number(UiText::SensitivityVertical,3,2,3,100,1,to.vertical);
    converter_multiplier_=number(UiText::SensitivityMultiplier,4,1,6,1,.001,from.multiplier);mouse_multiplier_=number(UiText::SensitivityMultiplier,4,2,6,1,.001,to.multiplier);section->body->addLayout(grid);
    auto* fileActions=new QHBoxLayout;fileActions->setContentsMargins(14,0,12,8);fileActions->setSpacing(8);
    auto* read=new QPushButton(text(UiText::ReadGameSettings));auto* choose=new QPushButton(text(UiText::ChooseGameSettings));
    for(auto* button:{read,choose}){button->setObjectName("secondary");fileActions->addWidget(button);}section->body->addLayout(fileActions);
    connect(read,&QPushButton::clicked,this,[this]{readGameSensitivity();});connect(choose,&QPushButton::clicked,this,[this]{readGameSensitivity(true);});
    mouse_setup_status_=label({},"rowHint");mouse_setup_status_->setWordWrap(true);mouse_setup_status_->setContentsMargins(14,0,12,0);section->body->addWidget(mouse_setup_status_);mouse_setup_status_->hide();
    step(UiText::ConversionResult);
    converter_name_=new QLineEdit;converter_name_->setMaxLength(80);converter_name_->setAccessibleName(text(UiText::ProfileSetName));converter_name_->setText(converter_name_draft_.isEmpty()?uniqueSetName(config,text(UiText::ConvertedProfiles)):converter_name_draft_);section->body->addWidget(converter_name_);
    auto* hint=label(text(UiText::MouseSetupHint),"rowHint");hint->setWordWrap(true);hint->setContentsMargins(14,0,12,8);section->body->addWidget(hint);
    converter_validation_=label({},"rowHint");converter_validation_->setWordWrap(true);converter_validation_->setContentsMargins(14,0,12,4);section->body->addWidget(converter_validation_);
    auto* actionsRow=new QHBoxLayout;actionsRow->setContentsMargins(14,0,12,8);mouse_rescale_=new QPushButton(text(UiText::ConvertAsNewSet));mouse_rescale_->setObjectName("save");actionsRow->addWidget(mouse_rescale_);section->body->addLayout(actionsRow);mouse_save_=nullptr;
    connect(mouse_rescale_,&QPushButton::clicked,this,[this]{rescaleMouseProfiles();});
    for(auto* field:{mouse_dpi_.data(),converter_dpi_.data()})connect(field,qOverload<int>(&QSpinBox::valueChanged),this,[this]{updateMouseSetupControls();});
    for(auto* field:{mouse_horizontal_.data(),mouse_vertical_.data(),mouse_multiplier_.data(),converter_horizontal_.data(),converter_vertical_.data(),converter_multiplier_.data()})connect(field,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this]{updateMouseSetupControls();});
    connect(converter_name_,&QLineEdit::textChanged,this,[this]{updateMouseSetupControls();});updateMouseSetupControls();
}

void MainWindow::updateMouseSetupControls(){
    if(!mouse_rescale_||!converter_name_)return;
    mouse_draft_=mouseSetupInput();converter_draft_=converterSourceInput();converter_name_draft_=converter_name_->text();
    const bool valuesValid=mouse_draft_->valid()&&converter_draft_->valid();
    const bool nameValid=validSetName(engine_.snapshot(),converter_name_draft_.trimmed());
    const auto explanation=save_blocked_?text(UiText::CouldNotSaveConfiguration):!valuesValid?text(UiText::MouseSetupRequired):!nameValid?text(UiText::InvalidSetName):QString();
    converter_validation_->setText(explanation);converter_validation_->setVisible(!explanation.isEmpty());mouse_rescale_->setToolTip(explanation);
    mouse_rescale_->setEnabled(!save_blocked_&&valuesValid&&nameValid);
}
void MainWindow::resetConverter(){
    mouse_draft_.reset();converter_draft_.reset();converter_import_.reset();converter_name_draft_.clear();converter_import_name_.clear();mouse_source_.clear();
}
bool MainWindow::confirmProfileAction(const QString& title,const QString& body,const QString& action){
    QDialog dialog(this,Qt::Dialog|Qt::FramelessWindowHint);dialog.setObjectName("helpDialog");dialog.setAttribute(Qt::WA_StyledBackground);dialog.setWindowTitle(title);dialog.setFixedWidth(420);
    auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(24,20,24,22);layout->setSpacing(16);auto* heading=label(title,"cardTitle");heading->setWordWrap(true);layout->addWidget(heading);auto* description=label(body);description->setTextFormat(Qt::PlainText);description->setWordWrap(true);layout->addWidget(description);
    auto* row=new QHBoxLayout;auto* cancel=new QPushButton(text(UiText::Cancel));cancel->setObjectName("secondary");auto* apply=new QPushButton(action);apply->setObjectName("save");row->addWidget(cancel);row->addWidget(apply);layout->addLayout(row);
    connect(cancel,&QPushButton::clicked,&dialog,&QDialog::reject);connect(apply,&QPushButton::clicked,&dialog,&QDialog::accept);dialog.adjustSize();roundWindow(&dialog);dialog.move(frameGeometry().center()-dialog.rect().center());return dialog.exec()==QDialog::Accepted;
}
QString MainWindow::askProfileSetName(const QString& initial,const QString& action){
    QDialog dialog(this,Qt::Dialog|Qt::FramelessWindowHint);dialog.setObjectName("helpDialog");dialog.setAttribute(Qt::WA_StyledBackground);dialog.setWindowTitle(text(UiText::ProfileSetName));dialog.setFixedWidth(380);
    auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(24,20,24,22);layout->setSpacing(16);layout->addWidget(label(text(UiText::ProfileSetName),"cardTitle"));auto* field=new QLineEdit(initial);field->setMaxLength(80);field->setAccessibleName(text(UiText::ProfileSetName));layout->addWidget(field);
    auto* row=new QHBoxLayout;auto* cancel=new QPushButton(text(UiText::Cancel));cancel->setObjectName("secondary");auto* apply=new QPushButton(action);apply->setObjectName("save");row->addWidget(cancel);row->addWidget(apply);layout->addLayout(row);
    connect(field,&QLineEdit::textChanged,apply,[apply](const QString& name){apply->setEnabled(!name.trimmed().isEmpty());});connect(cancel,&QPushButton::clicked,&dialog,&QDialog::reject);connect(apply,&QPushButton::clicked,&dialog,&QDialog::accept);field->selectAll();field->setFocus();dialog.adjustSize();roundWindow(&dialog);dialog.move(frameGeometry().center()-dialog.rect().center());return dialog.exec()==QDialog::Accepted?field->text().trimmed():QString();
}
void MainWindow::manageProfileSet(int action){
    if(save_blocked_)return;const auto before=engine_.snapshot();const auto active=std::find_if(before.profile_sets.begin(),before.profile_sets.end(),[&](const auto& set){return set.id==before.active_profile_set;});
    const auto currentLabel=text(UiText::CurrentProfiles);
    QString name;const auto id=profile_sets_?profile_sets_->currentData().toString().toStdString():std::string();
    if(action==0||action==1){
        if(action==1&&active==before.profile_sets.end())return;
        name=askProfileSetName(action==1?QString::fromStdString(active->name):uniqueSetName(before,text(UiText::CurrentProfiles)),text(action==1?UiText::RenameSet:UiText::CreateCopy));if(name.isEmpty())return;
        if(!validSetName(before,name,action==1?before.active_profile_set:std::string())){showNotice(text(UiText::InvalidSetName),true,true);return;}
    }
    if(action==2&&(active==before.profile_sets.end()||!confirmProfileAction(text(UiText::DeleteSet),text(UiText::DeleteSetPrompt).arg(QString::fromStdString(active->name)),text(UiText::DeleteSet))))return;
    if(action==3&&(id.empty()||id==before.active_profile_set))return;
    const size_t needed=action==0?1:action==3&&before.active_profile_set.empty()?1:0;
    if(before.profile_sets.size()+needed>profilesets::limit){showNotice(text(UiText::ProfileSetLimit),true,true);buildPreferences();return;}
    engine_.edit([&](r6::Config& config){
        if(action==0){profilesets::remember(config);const auto newId=newSetId();config.profile_sets.push_back(profilesets::capture(config,newId,name.toStdString()));config.active_profile_set=newId;}
        else if(action==1){for(auto& set:config.profile_sets)if(set.id==config.active_profile_set)set.name=name.toStdString();}
        else if(action==2){const auto removed=config.active_profile_set;config.profile_sets.erase(std::remove_if(config.profile_sets.begin(),config.profile_sets.end(),[&](const auto& set){return set.id==removed;}),config.profile_sets.end());config.active_profile_set.clear();}
        else {preserveCurrentSet(config,currentLabel);for(const auto& set:config.profile_sets)if(set.id==id){profilesets::activate(config,set);break;}}
    });
    resetConverter();dirty();syncSelection();updateStatus();buildPreferences();
}

void MainWindow::importForConversion(){
    const auto path=QFileDialog::getOpenFileName(this,text(UiText::ImportProfiles),{},"JSON (*.json)",nullptr,QFileDialog::DontUseNativeDialog);if(path.isEmpty())return;
    QFile file(path);r6::Config imported;QSet<QString> favorites;BackupOptions options;const auto existing=engine_.snapshot();
    if(!file.open(QIODevice::ReadOnly)||file.size()>1024*1024){showNotice(text(UiText::ImportFailedCheckTheBackupFormatAndFilePermissionsExistingProfile),true,true);return;}
    QJsonParseError error;const auto document=QJsonDocument::fromJson(file.readAll(),&error);
    if(file.error()!=QFile::NoError||error.error!=QJsonParseError::NoError||!document.isObject()||!parseBackup(document.object(),imported,favorites,options,&existing)){showNotice(text(UiText::ImportFailedCheckTheBackupFormatAndFilePermissionsExistingProfile),true,true);return;}
    imported.profile_sets.clear();imported.active_profile_set.clear();converter_import_=std::move(imported);converter_draft_=converter_import_->sensitivity;converter_import_name_=QFileInfo(path).completeBaseName().left(70).trimmed();if(converter_import_name_.isEmpty())converter_import_name_=text(UiText::CurrentProfiles);converter_name_draft_=uniqueSetName(existing,converter_import_name_+" · "+text(UiText::ConvertedProfiles));buildPreferences();
}

void MainWindow::readGameSensitivity(bool chooseFile){
    QString path;
    if(!chooseFile){if(!mouse_source_.isEmpty()&&sensitivity::readFile(mouse_source_))path=mouse_source_;else {const auto files=sensitivity::candidates();if(files.size()==1)path=files.first();else if(files.size()>1)chooseFile=true;}}
    if(chooseFile)path=QFileDialog::getOpenFileName(this,text(UiText::ChooseGameSettings),mouse_source_,"GameSettings.ini (*.ini)",nullptr,QFileDialog::DontUseNativeDialog);
    if(path.isEmpty()){if(!chooseFile){mouse_setup_status_->setText(text(UiText::GameSettingsMissing));mouse_setup_status_->show();}return;}
    mouse_setup_status_->show();const auto found=sensitivity::readFile(path);if(!found){mouse_setup_status_->setText(text(UiText::GameSettingsMissing));return;}
    const QSignalBlocker h(mouse_horizontal_.data()),v(mouse_vertical_.data()),m(mouse_multiplier_.data());mouse_horizontal_->setValue(found->horizontal);mouse_vertical_->setValue(found->vertical);mouse_multiplier_->setValue(found->multiplier);mouse_source_=QString::fromStdWString(found->source);mouse_setup_status_->setToolTip(mouse_source_);mouse_setup_status_->setText(text(UiText::GameSettingsRead));updateMouseSetupControls();
}
// Kept as the starting-value handler for existing integrations; conversion never calls it.
bool MainWindow::saveMouseSetup(){
    const auto input=mouseSetupInput();if(save_blocked_||!input.valid())return false;engine_.edit([&](r6::Config& config){config.sensitivity=input;});mouse_setup_dirty_=true;dirty();return save();
}
void MainWindow::rescaleMouseProfiles(){
    const auto from=converterSourceInput(),to=mouseSetupInput();const auto name=converter_name_->text().trimmed();const auto before=engine_.snapshot();
    const auto currentLabel=text(UiText::CurrentProfiles);
    if(save_blocked_||!from.valid()||!to.valid())return;
    if(!validSetName(before,name)){showNotice(text(UiText::InvalidSetName),true,true);return;}
    const size_t needed=1+(before.active_profile_set.empty()?1:0)+(converter_import_?1:0);
    if(before.profile_sets.size()+needed>profilesets::limit){showNotice(text(UiText::ProfileSetLimit),true,true);return;}
    auto source=converter_import_.value_or(before);source.sensitivity=from;const auto factors=sensitivity::scale(source,to);if(!factors.fits){showNotice(text(UiText::RescaleOutOfRange),true,true);return;}
    if(!confirmProfileAction(text(UiText::ConvertAsNewSet),text(UiText::ConversionPreview).arg(QString::number(factors.horizontal,'g',5),QString::number(factors.vertical,'g',5),name),text(UiText::CreateCopy)))return;
    bool applied=false;
    engine_.edit([&](r6::Config& config){
        auto converted=converter_import_.value_or(config);converted.sensitivity=from;if(!sensitivity::scale(converted,to).fits||!validSetName(config,name)||config.profile_sets.size()+(config.active_profile_set.empty()?1:0)+1+(converter_import_?1:0)>profilesets::limit)return;
        preserveCurrentSet(config,currentLabel,name);
        if(converter_import_){const auto originalId=newSetId();auto originalName=uniqueSetName(config,converter_import_name_);if(originalName.compare(name,Qt::CaseInsensitive)==0)originalName=uniqueSetName(config,originalName+" (1)");config.profile_sets.push_back(profilesets::capture(converted,originalId,originalName.toStdString()));}
        sensitivity::apply(converted,to);const auto newId=newSetId();const auto set=profilesets::capture(converted,newId,name.toStdString());config.profile_sets.push_back(set);profilesets::activate(config,set);applied=true;
    });
    if(!applied){showNotice(text(UiText::RescaleOutOfRange),true,true);return;}
    resetConverter();mouse_setup_dirty_=true;dirty();syncSelection();updateStatus();buildPreferences();showNotice(text(UiText::ProfilesRescaled));
}
void MainWindow::checkMouseSensitivityChange(){
    if(engine_.dry_run()||save_blocked_)return;const auto setup=engine_.snapshot().sensitivity;
    if(setup.valid()&&!setup.source.empty())if(const auto found=sensitivity::readFile(QString::fromStdWString(setup.source));found&&!sensitivity::sameGame(setup,*found))showNotice(text(UiText::SensitivityChanged),false,true);
}
