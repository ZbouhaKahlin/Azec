#include "main_window.hpp"

r6::SensitivitySetup MainWindow::mouseSetupInput() const {
    r6::SensitivitySetup setup;
    if(mouse_dpi_)setup.dpi=mouse_dpi_->value();
    if(mouse_horizontal_)setup.horizontal=mouse_horizontal_->value();
    if(mouse_vertical_)setup.vertical=mouse_vertical_->value();
    if(mouse_multiplier_)setup.multiplier=mouse_multiplier_->value();
    setup.source=mouse_source_.toStdWString();return setup;
}

void MainWindow::buildMouseSetup(QVBoxLayout* column){
    mouse_setup_=new Section(text(UiText::MouseSetup),false,true,kbtheme::yellow);column->addWidget(mouse_setup_);
    auto* section=mouse_setup_.data();
    const auto baseline=engine_.snapshot().sensitivity;
    auto initial=mouse_draft_.value_or(baseline);QString status;
    if(!initial.validGame()&&!engine_.dry_run()){
        const auto files=sensitivity::candidates();
        if(files.size()==1){if(const auto found=sensitivity::readFile(files.first())){const int dpi=initial.dpi;initial=*found;initial.dpi=dpi;status=text(UiText::GameSettingsRead);}}
        else status=text(files.isEmpty()?UiText::GameSettingsMissing:UiText::GameSettingsMultiple);
    }
    mouse_source_=QString::fromStdWString(initial.source);
    auto* dpiRow=settingRow(section,text(UiText::MouseDpi));mouse_dpi_=new QSpinBox;mouse_dpi_->setRange(0,100000);mouse_dpi_->setSingleStep(50);mouse_dpi_->setSpecialValueText(QString::fromUtf8("—"));mouse_dpi_->setButtonSymbols(QAbstractSpinBox::NoButtons);mouse_dpi_->setFixedWidth(110);mouse_dpi_->setAlignment(Qt::AlignCenter);mouse_dpi_->setAccessibleName(text(UiText::MouseDpi));mouse_dpi_->setValue(initial.dpi);dpiRow->addWidget(mouse_dpi_);
    const auto number=[&](UiText title,int decimals,double maximum,double step,double value){
        auto* row=settingRow(section,text(title));auto* field=new QDoubleSpinBox;field->setRange(0,maximum);field->setDecimals(decimals);field->setSingleStep(step);field->setSpecialValueText(QString::fromUtf8("—"));field->setButtonSymbols(QAbstractSpinBox::NoButtons);field->setFixedWidth(110);field->setAlignment(Qt::AlignCenter);field->setAccessibleName(text(title));field->setValue(value);row->addWidget(field);return field;
    };
    mouse_horizontal_=number(UiText::SensitivityHorizontal,3,100,1,initial.horizontal);
    mouse_vertical_=number(UiText::SensitivityVertical,3,100,1,initial.vertical);
    mouse_multiplier_=number(UiText::SensitivityMultiplier,6,1,.001,initial.multiplier);
    auto* fileActions=new QHBoxLayout;fileActions->setContentsMargins(14,8,12,0);fileActions->setSpacing(8);
    auto* read=new QPushButton(text(UiText::ReadGameSettings));auto* choose=new QPushButton(text(UiText::ChooseGameSettings));
    for(auto* button:{read,choose}){button->setObjectName("secondary");fileActions->addWidget(button);}fileActions->addStretch();section->body->addLayout(fileActions);
    connect(read,&QPushButton::clicked,this,[this]{readGameSensitivity();});connect(choose,&QPushButton::clicked,this,[this]{readGameSensitivity(true);});
    mouse_setup_status_=label(status,"rowHint");mouse_setup_status_->setWordWrap(true);mouse_setup_status_->setContentsMargins(14,4,12,4);mouse_setup_status_->setToolTip(mouse_source_);section->body->addWidget(mouse_setup_status_);
    auto* hint=label(text(UiText::MouseSetupHint),"rowHint");hint->setWordWrap(true);hint->setContentsMargins(14,0,12,8);section->body->addWidget(hint);
    auto* actions=new QHBoxLayout;actions->setContentsMargins(14,0,12,8);actions->setSpacing(8);
    mouse_save_=new QPushButton(text(UiText::SaveMouseSetup));mouse_save_->setObjectName("secondary");mouse_rescale_=new QPushButton(text(UiText::RescaleProfiles));mouse_rescale_->setObjectName("save");
    actions->addWidget(mouse_save_);actions->addWidget(mouse_rescale_);actions->addStretch();section->body->addLayout(actions);
    connect(mouse_save_,&QPushButton::clicked,this,[this]{saveMouseSetup();});connect(mouse_rescale_,&QPushButton::clicked,this,[this]{rescaleMouseProfiles();});
    connect(mouse_dpi_,qOverload<int>(&QSpinBox::valueChanged),this,[this]{updateMouseSetupControls();});
    for(auto* field:{mouse_horizontal_.data(),mouse_vertical_.data(),mouse_multiplier_.data()})connect(field,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this]{updateMouseSetupControls();});
    updateMouseSetupControls();
}

void MainWindow::updateMouseSetupControls(){
    if(!mouse_save_||!mouse_rescale_)return;
    const auto input=mouseSetupInput();mouse_draft_=input;const auto baseline=engine_.snapshot().sensitivity;
    const bool changed=baseline.valid()&&input.valid()&&!sensitivity::sameGame(baseline,input);
    mouse_save_->setEnabled(input.valid()&&!save_blocked_);mouse_rescale_->setEnabled(changed&&!save_blocked_);
    mouse_save_->setText(text(changed?UiText::SaveWithoutRescaling:UiText::SaveMouseSetup));
}

void MainWindow::readGameSensitivity(bool chooseFile){
    QString path;
    if(!chooseFile){
        if(!mouse_source_.isEmpty()&&sensitivity::readFile(mouse_source_))path=mouse_source_;
        else {const auto files=sensitivity::candidates();if(files.size()==1)path=files.first();else if(files.size()>1)chooseFile=true;}
    }
    if(chooseFile)path=QFileDialog::getOpenFileName(this,text(UiText::ChooseGameSettings),mouse_source_,"GameSettings.ini (*.ini)",nullptr,QFileDialog::DontUseNativeDialog);
    if(path.isEmpty()){if(!chooseFile)mouse_setup_status_->setText(text(UiText::GameSettingsMissing));return;}
    const auto found=sensitivity::readFile(path);
    if(!found){mouse_setup_status_->setText(text(UiText::GameSettingsMissing));return;}
    const QSignalBlocker h(mouse_horizontal_.data()),v(mouse_vertical_.data()),m(mouse_multiplier_.data());
    mouse_horizontal_->setValue(found->horizontal);mouse_vertical_->setValue(found->vertical);mouse_multiplier_->setValue(found->multiplier);
    mouse_source_=QString::fromStdWString(found->source);mouse_setup_status_->setToolTip(mouse_source_);mouse_setup_status_->setText(text(UiText::GameSettingsRead));updateMouseSetupControls();
}

bool MainWindow::saveMouseSetup(){
    const auto input=mouseSetupInput();if(!input.valid()){showNotice(text(UiText::MouseSetupRequired),true);return false;}
    if(save_blocked_){showNotice(text(UiText::CouldNotSaveConfiguration),true,true);return false;}
    engine_.edit([&](r6::Config& config){config.sensitivity=input;});mouse_setup_dirty_=true;dirty();
    const bool saved=save();if(saved){mouse_setup_status_->setText(text(UiText::MouseSetupSaved));updateMouseSetupControls();}return saved;
}

void MainWindow::rescaleMouseProfiles(){
    const auto input=mouseSetupInput();const auto before=engine_.snapshot();
    if(save_blocked_||!before.sensitivity.valid()||!input.valid()||sensitivity::sameGame(before.sensitivity,input))return;
    const auto factors=sensitivity::scale(before,input);
    if(!factors.fits){showNotice(text(UiText::RescaleOutOfRange),true,true);return;}
    QDialog dialog(this,Qt::Dialog|Qt::FramelessWindowHint);dialog.setObjectName("helpDialog");dialog.setAttribute(Qt::WA_StyledBackground);dialog.setWindowTitle(text(UiText::RescaleProfiles));dialog.setFixedWidth(440);
    auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(24,20,24,22);layout->setSpacing(16);
    auto* title=label(text(UiText::RescaleProfiles),"cardTitle");title->setWordWrap(true);layout->addWidget(title);
    QString preview=text(UiText::RescalePreview);if(preview.contains("%1"))preview=preview.arg(QString::number(factors.horizontal,'g',5),QString::number(factors.vertical,'g',5));
    auto* description=label(preview);description->setWordWrap(true);layout->addWidget(description);
    auto* buttons=new QHBoxLayout;auto* cancel=new QPushButton(text(UiText::Cancel));cancel->setObjectName("secondary");auto* apply=new QPushButton(text(UiText::RescaleProfiles));apply->setObjectName("save");buttons->addWidget(cancel);buttons->addWidget(apply);layout->addLayout(buttons);
    connect(cancel,&QPushButton::clicked,&dialog,&QDialog::reject);connect(apply,&QPushButton::clicked,&dialog,&QDialog::accept);
    dialog.adjustSize();roundWindow(&dialog);dialog.move(frameGeometry().center()-dialog.rect().center());if(dialog.exec()!=QDialog::Accepted)return;
    bool applied=false;
    engine_.edit([&](r6::Config& config){
        // Phone edits may have arrived while the preview was open. Revalidate
        // the current profiles under the Engine lock; never clamp silently.
        if(sensitivity::sameGame(config.sensitivity,before.sensitivity)&&sensitivity::scale(config,input).fits){sensitivity::apply(config,input);applied=true;}
    });
    if(!applied){showNotice(text(UiText::RescaleOutOfRange),true,true);return;}
    mouse_setup_dirty_=true;dirty();syncSelection();updateMouseSetupControls();mouse_setup_status_->setText(text(UiText::ProfilesRescaled));showNotice(text(UiText::ProfilesRescaled));
}

void MainWindow::checkMouseSensitivityChange(){
    if(engine_.dry_run()||save_blocked_)return;
    const auto setup=engine_.snapshot().sensitivity;
    if(setup.valid()&&!setup.source.empty())if(const auto found=sensitivity::readFile(QString::fromStdWString(setup.source));found&&!sensitivity::sameGame(setup,*found))showNotice(text(UiText::SensitivityChanged),false,true);
}
