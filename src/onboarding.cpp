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
    if(tour_step_==5&&tour_pill_)tour_pill_->setNextEnabled(input.valid()&&!save_blocked_&&
        (!baseline.valid()||(sensitivity::sameGame(baseline,input)&&baseline.dpi==input.dpi)));
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

void MainWindow::maybeStartTour(){
    if(tour_checked_||engine_.dry_run())return;tour_checked_=true;
    if(save_blocked_)return;
    QJsonObject preferences;if(!uiprefs::readDocument(uiprefs::filePath(config_path_),preferences))return;
    if(preferences["onboardingVersion"].toInt()<1){startTour();return;}
    const auto setup=engine_.snapshot().sensitivity;
    if(setup.valid()&&!setup.source.empty())if(const auto found=sensitivity::readFile(QString::fromStdWString(setup.source));found&&!sensitivity::sameGame(setup,*found))showNotice(text(UiText::SensitivityChanged),false,true);
}

void MainWindow::startTour(){
    if(tour_step_>=0){positionTour();return;}
    if(!tour_pill_){tour_pill_=new OnboardingPill(this);tour_pill_->onNext=[this]{advanceTour();};tour_pill_->onSkip=[this]{finishTour();};}
    if(!tour_arrow_)tour_arrow_=new OnboardingArrow(this);
    setSidebarCollapsed(false,false);search_->clear();setTourStep(0);tour_refresh_.start();
    // Keep the instruction pill clear of the app on shorter laptop displays.
    auto* screen=QGuiApplication::screenAt(frameGeometry().center());if(!screen)screen=QGuiApplication::primaryScreen();
    const auto available=screen->availableGeometry();const int freeHeight=tour_pill_->y()-available.top()-32;
    if(frameGeometry().bottom()+16>tour_pill_->y()&&freeHeight>=minimumHeight()){
        resize(width(),std::min(height(),freeHeight));move(available.center().x()-width()/2,available.top()+16);
    }
    positionTour();
}

void MainWindow::setTourStep(int step){
    if(!tour_pill_||step<0||step>=6)return;tour_step_=step;
    if(step<4){
        showOperators();
    }else {
        if(pages_->currentWidget()!=preferences_)showPreferences();chooseSettingsCategory(step==4?2:mouse_setup_category_);
    }
    const UiText titles[]={UiText::TourChooseTitle,UiText::WeaponProfiles,UiText::RecoilControl,UiText::RapidFire,UiText::Shortcuts,UiText::MouseSetup};
    const UiText descriptions[]={UiText::TourChooseHint,UiText::TourProfilesHint,UiText::TourRecoilHint,UiText::TourRapidHint,UiText::TourShortcutsHint,UiText::TourSetupHint};
    QString description=text(descriptions[step]);
    if(step==4&&description.contains("%1")){const auto config=engine_.snapshot();description=description.arg(bindingName(config.primary_key),bindingName(config.secondary_key));}
    auto* screen=QGuiApplication::screenAt(frameGeometry().center());if(!screen)screen=QGuiApplication::primaryScreen();
    tour_pill_->present(step,6,text(titles[step]),description,engine_.snapshot().language,screen->availableGeometry());
    tour_pill_->setNextEnabled(true);if(step==5)updateMouseSetupControls();
    QTimer::singleShot(50,this,[this,step]{
        if(tour_step_!=step)return;
        if(step==0){auto* list=favorites_.contains("Ace")?favorites_list_:operators_;
            for(int i=0;i<list->count();++i)if(list->item(i)->data(Qt::UserRole).toInt()==0){
                const auto row=list->visualItemRect(list->item(i)).intersected(list->viewport()->rect());
                const QRect target(list->viewport()->mapToGlobal(row.topLeft()),row.size());auto* viewport=sidebar_scroll_->viewport();
                if(!QRect(viewport->mapToGlobal(QPoint()),viewport->size()).contains(target)){
                    const int y=list->viewport()->mapTo(sidebar_scroll_->widget(),row.topLeft()).y();
                    sidebar_scroll_->verticalScrollBar()->setValue(std::max(0,y-48));
                }break;
            }
        }
        QWidget* target=step==1?static_cast<QWidget*>(dual_):step==2?static_cast<QWidget*>(vertical_):step==3?static_cast<QWidget*>(rapid_active_):nullptr;
        if(target)scroll_->ensureWidgetVisible(target,20,40);
        if(step==5&&mouse_setup_)preferences_scroll_->ensureWidgetVisible(mouse_setup_,0,12);
        positionTour();
    });positionTour();
}

void MainWindow::advanceTour(){
    if(tour_step_<0)return;
    if(tour_step_==0)selectOperator(0);
    if(tour_step_==5){
        const auto input=mouseSetupInput(),baseline=engine_.snapshot().sensitivity;
        if(!input.valid()||(baseline.valid()&&(!sensitivity::sameGame(baseline,input)||baseline.dpi!=input.dpi)))return;
        if((!baseline.valid()||mouse_setup_dirty_)&&!saveMouseSetup())return;
        finishTour();return;
    }setTourStep(tour_step_+1);
}

void MainWindow::finishTour(){
    if(tour_step_<0)return;
    if(!uiprefs::update(config_path_,[](QJsonObject& root){root["onboardingVersion"]=1;}))showNotice(text(UiText::CouldNotSaveConfiguration),true,true);
    tour_step_=-1;tour_refresh_.stop();if(tour_arrow_)tour_arrow_->hide();if(tour_pill_)tour_pill_->hide();
}

void MainWindow::positionTour(){
    if(tour_step_<0||!tour_pill_||!tour_arrow_)return;
    auto* active=QApplication::activeWindow();
    const bool ours=active&&(active==this||active==tour_pill_);
    if(!isVisible()||isMinimized()||!ours||QApplication::activeModalWidget()||QApplication::activePopupWidget()){
        tour_pill_->hide();tour_arrow_->hide();return;
    }
    auto* screen=QGuiApplication::screenAt(frameGeometry().center());if(!screen)screen=QGuiApplication::primaryScreen();
    const auto available=screen->availableGeometry();tour_pill_->place(available);
    if(!tour_pill_->isVisible())tour_pill_->show();
    QRect target;QWidget* widget=nullptr;
    if(tour_step_==0&&pages_->currentWidget()==operator_page_){
        auto* list=favorites_.contains("Ace")?favorites_list_:operators_;
        for(int i=0;i<list->count();++i)if(list->item(i)->data(Qt::UserRole).toInt()==0){const auto rect=list->visualItemRect(list->item(i)).intersected(list->viewport()->rect());target=QRect(list->viewport()->mapToGlobal(rect.topLeft()),rect.size());break;}
    }else if(tour_step_<4&&pages_->currentWidget()==operator_page_){
        widget=tour_step_==1?static_cast<QWidget*>(dual_):tour_step_==2?static_cast<QWidget*>(vertical_):static_cast<QWidget*>(rapid_active_);
    }else if(tour_step_==4&&pages_->currentWidget()==preferences_&&preference_category_==2)widget=key_buttons_[0];
    else if(tour_step_==5&&pages_->currentWidget()==preferences_&&preference_category_==mouse_setup_category_)widget=mouse_dpi_;
    if(widget&&widget->isVisible())target=QRect(widget->mapToGlobal(QPoint()),widget->size());
    const QRect windowArea(mapToGlobal(QPoint()),size());
    auto* viewport=tour_step_==0?sidebar_scroll_->viewport():tour_step_<4?scroll_->viewport():preferences_scroll_->viewport();
    const QRect visibleArea(viewport->mapToGlobal(QPoint()),viewport->size());
    if(target.isEmpty()||!windowArea.contains(target.center())||!visibleArea.contains(target)){tour_arrow_->hide();return;}
    tour_arrow_->pointTo(target);if(!tour_arrow_->isVisible())tour_arrow_->show();
}
