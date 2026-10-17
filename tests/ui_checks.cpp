#include "../src/main_window.hpp"
#include "../src/profile_transfer.hpp"

QJsonObject MainWindow::selfTest(const QString& output){
        const auto wait=[](int milliseconds){QEventLoop loop;QTimer::singleShot(milliseconds,&loop,&QEventLoop::quit);loop.exec();};
        if(QCoreApplication::arguments().contains("--motion-only")){
            const bool captureFrames=!QCoreApplication::arguments().contains("--motion-timing-only");
            QJsonObject sequences,motionChecks;bool continuous=true,stableControls=true,stableBackground=true;
            showPreferences();wait(250);phone_enabled_->setFocus(Qt::MouseFocusReason);preferences_scroll_->ensureWidgetVisible(phone_enabled_);wait(80);
            const auto record=[&](const QString& name,const std::function<void()>& action){
                QDir().mkpath(output+"/"+name);QVector<QImage> frames;QJsonArray samples;QElapsedTimer clock;clock.start();
                QTimer sample;sample.setTimerType(Qt::PreciseTimer);sample.setInterval(16);QEventLoop loop;
                connect(&sample,&QTimer::timeout,&loop,[&]{
                    const qint64 time=clock.nsecsElapsed();QElapsedTimer captureTime;captureTime.start();
                    if(captureFrames)frames.append(grab().toImage());
                    stableControls&=phone_enabled_->parentWidget()->height()==44;
                    QString background;QPoint backgroundPoint;
                    // Stay inside the body, away from the moving row/clip edge:
                    // a native scroll clamp can be one layout pass ahead of a grab.
                    if(captureFrames&&phone_details_->height()>64){const QPoint local=phone_details_->mapTo(preferences_scroll_->viewport(),QPoint(4,32));if(preferences_scroll_->viewport()->rect().contains(local)){backgroundPoint=preferences_scroll_->viewport()->mapTo(this,local);const auto color=frames.last().pixelColor(qRound(backgroundPoint.x()*devicePixelRatioF()),qRound(backgroundPoint.y()*devicePixelRatioF()));background=color.name();stableBackground&=presentation::isRally()?(std::abs(color.red()-rally::panel.red())<=2&&std::abs(color.green()-rally::panel.green())<=2&&std::abs(color.blue()-rally::panel.blue())<=2):(color.red()>=230&&color.green()>=230&&color.blue()>=230);}}
                    samples.append(QJsonObject{{"ms",time/1000000.0},{"captureMs",captureTime.nsecsElapsed()/1000000.0},{"height",phone_details_->height()},{"progress",phone_details_->progress()},{"scroll",preferences_scroll_->verticalScrollBar()->value()},{"maximum",preferences_scroll_->verticalScrollBar()->maximum()},{"toggleHeight",phone_enabled_->parentWidget()->height()},{"background",background},{"backgroundX",backgroundPoint.x()},{"backgroundY",backgroundPoint.y()},{"qr",pairing_qr_->revealProgress()}});
                });
                sample.start();QElapsedTimer actionTime;actionTime.start();action();const double actionMs=actionTime.nsecsElapsed()/1000000.0;
                QTimer::singleShot(700,&loop,&QEventLoop::quit);loop.exec();sample.stop();
                for(int i=0;i<frames.size();++i)frames[i].save(output+"/"+name+QString("/frame-%1.png").arg(i,3,10,QChar('0')));
                sequences[name]=QJsonObject{{"actionMs",actionMs},{"samples",samples}};
            };
            const auto togglePhone=[&]{phone_enabled_->setFocus(Qt::MouseFocusReason);const int before=phone_details_->height();phone_enabled_->click();continuous&=before==phone_details_->height();};
            record("phone-off",togglePhone);
            preferences_scroll_->verticalScrollBar()->setValue(preferences_scroll_->verticalScrollBar()->maximum());
            record("phone-on",togglePhone);
            preferences_scroll_->ensureWidgetVisible(pairing_qr_);wait(100);
            record("qr-reveal",[&]{pairing_qr_->click();});
            record("qr-conceal",[&]{pairing_qr_->click();});
            record("phone-reversal",[&]{togglePhone();QTimer::singleShot(80,this,togglePhone);QTimer::singleShot(170,this,togglePhone);QTimer::singleShot(220,this,togglePhone);});
            motionChecks["phone_retarget_preserves_height"]=continuous;motionChecks["phone_switch_keeps_44px_row"]=stableControls;if(captureFrames)motionChecks["phone_background_stays_panel_color"]=stableBackground;
            return {{"passed",continuous&&stableControls&&stableBackground},{"checks",motionChecks},{"sequences",sequences}};
        }
        const bool rally=presentation::isRally();QJsonObject checks;const auto available=QGuiApplication::primaryScreen()->availableGeometry();checks["window_fits_available_screen"]=width()<=available.width()&&height()<=available.height();checks["compact_default_when_screen_allows"]=available.width()<912||available.height()<698||size()==QSize(864,650);checks["layout_matches_interface"]=rally?(!findChild<QWidget*>("operatorSidebar")&&operator_page_->width()==pages_->width()):findChild<QWidget*>("operatorSidebar")->width()==200;auto* brand=findChild<QLabel*>("brand");checks["azec_branding"]=windowTitle()=="Azec"&&brand->accessibleName()=="Azec"&&brand->text()=="Azec";checks["all_72_operators"]=operators_->count()==r6::operator_count;checks["extension_operator_row"]=operators_->item(operators_->count()-1)->text()=="Noor";checks["all_72_svg_icons_valid"]=icon_failures_.empty();checks["actual_native_window_not_fullscreen"]=!isFullScreen();
        if(rally){operator_picker_->click();wait(230);filterOperators();capture(output+"/roster.png");}
        checks["only_selected_interface_constructed"]=rally?roster_page_&&operator_picker_&&search_->isVisible():!roster_page_&&!operator_picker_;
        auto* sidebarBar=static_cast<FadingScrollBar*>(sidebar_scroll_->verticalScrollBar());checks["scrollbars_start_invisible"]=sidebarBar->opacity()==0&&static_cast<FadingScrollBar*>(scroll_->verticalScrollBar())->opacity()==0&&static_cast<FadingScrollBar*>(preferences_scroll_->verticalScrollBar())->opacity()==0;
        checks["one_shared_sidebar_scroll"]=sidebar_scroll_->widget()->isAncestorOf(operators_)&&sidebar_scroll_->widget()->isAncestorOf(favorites_list_)&&operators_->verticalScrollBarPolicy()==Qt::ScrollBarAlwaysOff&&favorites_list_->verticalScrollBarPolicy()==Qt::ScrollBarAlwaysOff;
        checks["raised_panel_contains_both_pages"]=panel_->isAncestorOf(operator_page_)&&panel_->isAncestorOf(preferences_)&&!panel_->graphicsEffect()&&(rally?pages_->width()==width():pages_->width()>findChild<QWidget*>("operatorSidebar")->width()*3);
        search_->setText("Blackbeard");int visible=0;for(int i=0;i<operators_->count();++i)if(!operators_->item(i)->isHidden())++visible;checks["search_filters_sidebar"]=visible==1;search_->clear();
        operators_->setCurrentRow(4);vertical_->setValue(12.3);horizontal_->setValue(-2.4);humanize_->setChecked(false);operator_rapid_->setChecked(true);delay_->setValue(75);auto c=engine_.snapshot();checks["controls_update_selected_operator"]=c.selected==4&&std::abs(c.operators[4].vertical-12.3f)<.001f&&std::abs(c.operators[4].horizontal+2.4f)<.001f&&!c.operators[4].humanize&&c.operators[4].rapid&&c.operators[4].delay==75;
        operators_->setCurrentRow(0);checks["other_profile_preserved"]=vertical_->value()==10&&horizontal_->value()==0;operators_->setCurrentRow(4);const auto fixture=std::filesystem::path(output.toStdWString())/L"ui-test-config.bin";r6::Config readback;checks["3734_byte_compatible_settings"]=engine_.save(fixture)&&readback.load(fixture)&&readback.bytes()==engine_.snapshot().bytes();
        const auto saveIcon=save_->icon().cacheKey();save_->click();r6::Config saved;checks["save_button_writes_settings"]=saved.load(config_path_)&&saved.bytes()==engine_.snapshot().bytes();checks["save_icon_feedback"]=save_->property("saveState").toString()=="saved";
        wait(180);const auto saveAfterClick=save_->grab().toImage();capture(output+"/save-start.png");wait(1100);capture(output+"/save-feedback.png");
        checks["save_has_no_glow_or_icon_swap"]=save_->grab().toImage()==saveAfterClick&&save_->icon().cacheKey()==saveIcon;
        const bool masterBefore=engine_.recoil_master;recoil_master_->click();checks["master_recoil_control"]=engine_.recoil_master.load()!=masterBefore;recoil_master_->click();const bool rapidBefore=engine_.rapid_master;rapid_master_->click();checks["global_rapid_control"]=engine_.rapid_master.load()!=rapidBefore&&bool(engine_.snapshot().rapid_master)==engine_.rapid_master.load();rapid_master_->click();
        const auto before=engine_.recoil_active.load();checks["initial_recoil_off"]=!before;status_->click();checks["activation_control"]=engine_.recoil_active.load()!=before;status_->click();checks["second_click_restores_off"]=!engine_.recoil_active.load();
        // Hidden harness windows need an active Qt focus context for Tab events.
        QApplication::setActiveWindow(this);status_->clearFocus();status_->setFocus(Qt::TabFocusReason);QApplication::processEvents();const bool keyboardIndicator=status_->property("keyboardFocus").toBool();
        const QPointF statusPoint=status_->rect().center();QMouseEvent press(QEvent::MouseButtonPress,statusPoint,QPointF(status_->mapToGlobal(statusPoint.toPoint())),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(status_,&press);QMouseEvent release(QEvent::MouseButtonRelease,statusPoint,QPointF(status_->mapToGlobal(statusPoint.toPoint())),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(status_,&release);status_->click();checks["keyboard_focus_is_visible_and_mouse_click_clears_it"]=keyboardIndicator&&!status_->property("keyboardFocus").toBool()&&!engine_.recoil_active.load();
        checks["consistent_bundled_typography"]=QFontInfo(operator_name_->font()).family()==(rally?"Geist":"Fredoka")&&QFontInfo(save_->font()).family()=="Geist"&&QFontInfo(status_->font()).family()=="Geist";
        checks["compact_operator_rows"]=operators_->sizeHintForRow(0)==operatorRowHeight&&operators_->item(0)->icon().availableSizes().contains(QSize(operatorIconSize,operatorIconSize));
        QFont sidebarFont("Geist");sidebarFont.setPixelSize(rally?12:14);sidebarFont.setWeight(QFont::Medium);bool namesFit=true;for(int row=0;row<operators_->count();++row)namesFit&=QFontMetrics(sidebarFont).horizontalAdvance(operators_->item(row)->text())<=(rally?operators_->gridSize().width()-16:operators_->viewport()->width()-73);checks["larger_operator_names_fit_sidebar"]=namesFit;
        bool aligned=true;for(auto* button:save_->parentWidget()->findChildren<QPushButton*>(QString(),Qt::FindDirectChildrenOnly))if(dynamic_cast<IconButton*>(button))aligned&=button->size()==QSize(34,34)&&button->geometry().center().y()==save_->geometry().center().y()&&button->text().isEmpty()&&button->iconSize()==QSize(20,20);checks["header_icons_share_geometry"]=aligned;
        const auto gearBefore=gear_->grab().toImage();QEnterEvent enter(QPointF(17,17),QPointF(gear_->pos()+QPoint(17,17)),QPointF(gear_->mapToGlobal(QPoint(17,17))));QApplication::sendEvent(gear_,&enter);wait(170);const auto gearHovered=gear_->grab().toImage();capture(output+"/header-hover.png");
        bool transparentEdges=true;for(int y=0;y<gearBefore.height();++y)for(int x=0;x<gearBefore.width();++x)if(x<3||y<3||x>=gearBefore.width()-3||y>=gearBefore.height()-3)transparentEdges&=gearBefore.pixelColor(x,y)==gearHovered.pixelColor(x,y);QEvent leave(QEvent::Leave);QApplication::sendEvent(gear_,&leave);wait(170);
        bool hoverFlat=gearBefore==gearHovered;{const auto statusBefore=status_->grab().toImage();QEnterEvent statusEnter(QPointF(5,5),QPointF(5,5),status_->mapToGlobal(QPoint(5,5)));QApplication::sendEvent(status_,&statusEnter);wait(30);hoverFlat&=statusBefore==status_->grab().toImage();QEvent statusLeave(QEvent::Leave);QApplication::sendEvent(status_,&statusLeave);}
        checks["no_hover_effects"]=transparentEdges&&hoverFlat;checks["slider_handles_fit_widget"]=vertical_slider_->height()>=18&&horizontal_slider_->height()>=18;
        checks["status_has_no_hotkey"]=status_->text()=="Recoil off";
        checks["save_icon_next_to_settings"]=save_->text().isEmpty()&&!save_->icon().isNull()&&save_->parentWidget()==gear_->parentWidget()&&save_->geometry().right()<gear_->geometry().left();
        checks["footer_removed"]=!findChild<QFrame*>("footer");checks["small_r6_label_removed"]=!findChild<QLabel*>("r6Badge");
        bool question=false;for(auto* button:findChildren<QPushButton*>())question|=button->text()=="?";checks["no_question_mark_control"]=!question;

        DWORD preference=0;const auto hr=DwmGetWindowAttribute(HWND(winId()),33,&preference,sizeof(preference));HRGN region=CreateRectRgn(0,0,0,0);const int regionType=GetWindowRgn(HWND(winId()),region);const bool clipped=regionType!=ERROR&&!PtInRegion(region,0,0)&&PtInRegion(region,qRound(width()*devicePixelRatioF()/2),qRound(height()*devicePixelRatioF()/2));DeleteObject(region);
        checks["actual_rounded_window_shape"]=(SUCCEEDED(hr)&&preference==2)||clipped;checks["sendinput_suppressed_in_dry_run"]=engine_.dry_run()&&engine_.injected_events==0;
        status_->setFocus(Qt::OtherFocusReason);scroll_->verticalScrollBar()->setValue(0);QApplication::processEvents();capture(output+"/window.png");
        const auto stationaryPanel=panel_->geometry();auto* preferenceContent=preferences_scroll_->widget();
        gear_->click();checks["gear_opens_inline_settings"]=pages_->currentWidget()==preferences_&&preferences_->window()==this&&!QApplication::activeModalWidget();
        checks["settings_navigation_starts_crossfade"]=pages_->transitioning()&&preferences_scroll_->widget()==preferenceContent;
        wait(80);capture(output+"/settings-transition.png");wait(140);checks["settings_crossfade_finishes_without_moving_panel"]=!pages_->transitioning()&&panel_->geometry()==stationaryPanel;
        back_->click();wait(35);gear_->click();wait(35);back_->click();wait(220);checks["rapid_settings_navigation_finishes_on_last_page"]=pages_->currentWidget()==operator_page_&&!pages_->transitioning()&&panel_->geometry()==stationaryPanel;gear_->click();wait(220);
        checks["settings_exposes_hotkeys"]=language_&&key_buttons_[0]&&key_buttons_[0]->accessibleName()=="Open / hide window";
        language_->showPopup();wait(40);auto* languagePopup=findChild<QFrame*>("languagePopup");
        checks["language_popup_uses_ink_frame"]=languagePopup&&languagePopup->isVisible()&&(languagePopup->windowFlags()&Qt::FramelessWindowHint)&&languagePopup->findChild<QListWidget*>()->count()==9;
        if(languagePopup){languagePopup->grab().save(output+"/language-popup.png");auto* list=languagePopup->findChild<QListWidget*>();const auto before=list->grab().toImage();QEnterEvent enterChoice(QPointF(5,5),QPointF(5,5),list->mapToGlobal(QPoint(5,5)));QApplication::sendEvent(list,&enterChoice);checks["language_popup_hover_stays_flat"]=before==list->grab().toImage();}language_->hidePopup();
        language_->setCurrentIndex(1);QApplication::processEvents();checks["portuguese_labels"]=operators_title_->text()=="Todos os operadores"&&save_->toolTip().startsWith("Salvar configurações");language_->setCurrentIndex(0);QApplication::processEvents();
        bool localizedSidebarFits=true,hebrewPrankVisible=true;int frenchIndex=-1,prankIndex=-1;
        for(int i=0;i<language_->count();++i){language_->setCurrentIndex(i);QApplication::processEvents();if(auto* popup=findChild<QDialog*>("palestinePopup"))popup->close();const int id=language_->itemData(i).toInt();localizedSidebarFits&=sidebar_scroll_->widget()->width()<=sidebar_scroll_->viewport()->width();localizedSidebarFits&=operators_title_->width()<=sidebar_scroll_->viewport()->width();if(id==2)frenchIndex=i;if(id==6)prankIndex=i;}
        checks["localized_sidebar_fits_column"]=localizedSidebarFits;
        if(frenchIndex>=0){language_->setCurrentIndex(frenchIndex);QApplication::processEvents();capture(output+"/sidebar-french.png");}
        language_->setCurrentIndex(1);QApplication::processEvents();capture(output+"/sidebar-portuguese.png");
        hebrewPrankVisible=prankIndex>=0;if(prankIndex>=0){language_->setCurrentIndex(prankIndex);QApplication::processEvents();auto* popup=findChild<QDialog*>("palestinePopup");hebrewPrankVisible=popup&&popup->isVisible()&&!popup->isModal()&&!QApplication::activeModalWidget()&&operators_title_->text()==QString::fromUtf8(lang_he[static_cast<int>(UiText::AllOperators)]);if(popup){popup->grab().save(output+"/palestine-popup.png");popup->close();}capture(output+"/sidebar-prank.png");}
        checks["hebrew_prank_shows_palestine"]=hebrewPrankVisible;
        language_->setCurrentIndex(0);QApplication::processEvents();
        preferences_scroll_->verticalScrollBar()->setValue(0);QApplication::processEvents();capture(output+"/system.png");back_->click();checks["settings_back_preserves_profile"]=pages_->currentWidget()==operator_page_&&engine_.snapshot().selected==4&&vertical_->value()==12.3;
        if(rally){showRoster();wait(230);}const auto originalFavorites=favorites_;favorites_.clear();persistFavorites();refreshFavorites();wait(220);checks["empty_favorites_hide_entire_group"]=favorites_group_->isHidden()&&!favorites_title_->isVisible()&&favorites_list_->isHidden();capture(output+"/empty-favorites.png");
        setFavorite(4,false);operators_->scrollToItem(operators_->item(4));QApplication::processEvents();
        bool favoriteAction=false,compactMenu=false,insetHover=false;QTimer::singleShot(50,this,[&]{auto* popup=qobject_cast<QMenu*>(QApplication::activePopupWidget());auto* action=popup?popup->findChild<QAction*>("toggleFavorite"):nullptr;if(action){favoriteAction=!action->icon().isNull()&&popup->findChildren<QPushButton*>().isEmpty();compactMenu=popup->width()<=190&&popup->height()<=42;const auto before=popup->grab().toImage();popup->setActiveAction(action);const auto after=popup->grab().toImage();const auto actionRect=popup->actionGeometry(action);insetHover=actionRect.left()>=4&&actionRect.top()>=4&&before.pixelColor(2,2)==after.pixelColor(2,2);popup->grab().save(output+"/favorite-menu.png");action->trigger();}});
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
        if(rally){showRoster();wait(230);}search_->clear();sidebar_scroll_->verticalScrollBar()->setValue(0);QApplication::processEvents();capture(output+"/favorites.png");
        search_->setText("Ash");int favoriteMatches=0;for(int row=0;row<favorites_list_->count();++row)if(!favorites_list_->item(row)->isHidden())++favoriteMatches;checks["search_filters_without_duplicates"]=!favorites_list_->isHidden()&&operators_->item(4)->isHidden()&&(rally?(operators_->height()<=operatorRowHeight+2&&favorites_list_->height()<=operatorRowHeight+2&&favoriteMatches>0):(operators_->height()==operatorRowHeight&&favorites_list_->height()==favoriteMatches*operatorRowHeight));search_->clear();
        search_->setText("Blackbeard");checks["search_filters_favorites"]=favorites_list_->isHidden()&&favorites_group_->isHidden();search_->clear();
        QTimer::singleShot(50,this,[&]{auto* popup=QApplication::activePopupWidget();auto* action=popup?popup->findChild<QAction*>("toggleFavorite"):nullptr;if(action)action->trigger();});emit favorites_list_->customContextMenuRequested(favorites_list_->visualItemRect(favorites_list_->item(favoriteRow)).center());
        bool removed=true;for(int i=0;i<favorites_list_->count();++i)removed&=favorites_list_->item(i)->data(Qt::UserRole).toInt()!=4;checks["favorite_removal_updates_sidebar"]=removed&&!favorites_.contains("Ash")&&readFavorites()==favorites_&&favorites_group_->isHidden()&&!operators_->item(4)->isHidden();favorites_=originalFavorites;persistFavorites();refreshFavorites();
        for(int i=0;i<6;++i)setFavorite(i,true);QApplication::processEvents();checks["favorites_have_no_separate_scroll_or_cap"]=(rally?(favorites_list_->height()==((favorites_list_->count()+std::max(1,sidebar_scroll_->viewport()->width()/operators_->gridSize().width())-1)/std::max(1,sidebar_scroll_->viewport()->width()/operators_->gridSize().width()))*operatorRowHeight+2):(favorites_list_->height()==favorites_list_->count()*operatorRowHeight&&favorites_list_->height()>3*operatorRowHeight))&&favorites_list_->verticalScrollBar()->maximum()==0;
        // Real viewport wheel input must reach the common sidebar, even over a list.
        sidebarBar->setValue(0);const auto wheelPoint=operators_->viewport()->rect().center();QWheelEvent wheel(QPointF(wheelPoint),QPointF(operators_->viewport()->mapToGlobal(wheelPoint)),QPoint(),QPoint(0,-120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);QApplication::sendEvent(operators_->viewport(),&wheel);wait(130);checks["wheel_over_list_scrolls_shared_sidebar"]=sidebarBar->value()>0&&sidebarBar->opacity()>.99;
        favorites_=originalFavorites;persistFavorites();refreshFavorites();QApplication::processEvents();
        selectOperator(4);dual_->click();checks["secondary_mode_reveals_selector"]=weapon_selector_->expanded()&&weapon_selector_->transitioning()&&engine_.snapshot().active_slot()==1&&vertical_->value()==12.3;
        wait(90);checks["weapon_tabs_expand_between_endpoints"]=weapon_selector_->progress()>0&&weapon_selector_->progress()<1&&weapon_selector_->height()>0;capture(output+"/weapons-expanding.png");
        dual_->click();wait(25);dual_->click();wait(270);checks["rapid_weapon_tab_reversal_settles"]=weapon_selector_->expanded()&&!weapon_selector_->transitioning()&&weapon_selector_->progress()==1&&engine_.snapshot().active_slot()==1;
        vertical_->setValue(28.7);horizontal_->setValue(3.2);humanize_->setChecked(true);operator_rapid_->setChecked(false);
        handleWeaponKeys(false,false,false,false);handleWeaponKeys(true,false,false,false);checks["primary_key_restores_primary_profile"]=engine_.snapshot().active_slot()==0&&vertical_->value()==12.3&&horizontal_->value()==-2.4&&operator_rapid_->isChecked();
        handleWeaponKeys(false,false,false,false);handleWeaponKeys(false,true,false,false);checks["secondary_key_restores_secondary_profile"]=engine_.snapshot().active_slot()==1&&vertical_->value()==28.7&&horizontal_->value()==3.2&&humanize_->isChecked()&&!operator_rapid_->isChecked();
        handleWeaponKeys(false,false,false,false);handleWeaponKeys(true,false,true,false);checks["weapon_keys_do_not_interrupt_number_entry"]=engine_.snapshot().active_slot()==1;
        handleWeaponKeys(false,false,false,false);handleWeaponKeys(true,false,false,true);checks["modified_shortcuts_do_not_switch_weapons"]=engine_.snapshot().active_slot()==1;handleWeaponKeys(false,false,false,false);
        scroll_->verticalScrollBar()->setValue(0);wait(220);capture(output+"/weapons.png");
        dual_->click();checks["disabling_secondary_restores_primary"]=engine_.snapshot().active_slot()==0&&vertical_->value()==12.3&&!weapon_selector_->expanded();wait(260);checks["disabled_weapon_tabs_reserve_no_height"]=weapon_selector_->height()==0;dual_->click();checks["reenabling_secondary_preserves_edits"]=engine_.snapshot().active_slot()==1&&vertical_->value()==28.7;
        save_->click();{r6::Engine restored(true);const bool loaded=restored.load(config_path_);const auto saved=restored.snapshot();checks["both_weapon_profiles_reload_from_disk"]=loaded&&saved.secondary_enabled[4]&&std::abs(saved.secondary[4].vertical-28.7f)<.001&&std::abs(saved.operators[4].vertical-12.3f)<.001&&saved.primary_key=='1'&&saved.secondary_key=='2';}
        selectOperator(0);checks["secondary_is_optional_for_each_operator"]=!engine_.snapshot().secondary_enabled[0]&&!engine_.select_weapon(1)&&engine_.snapshot().active_slot()==0;
        selectOperator(4);checks["character_selection_starts_on_primary"]=engine_.snapshot().active_slot()==0&&vertical_->value()==12.3;dual_->click();save_->click();
        const auto unmodified=engine_.snapshot().bytes();checks["phone_rejects_invalid_values"]=mobileUpdate({{"action","setProfile"},{"operator",4},{"slot",0},{"values",QJsonObject{{"vertical",-3}}}}).contains("error")&&engine_.snapshot().bytes()==unmodified;
        checks["phone_server_listens_and_provides_pairing"]=phone_->listening()&&phone_->port()>0&&phone_->code().size()==6&&phone_->token().size()==32;
        const lan::Adapter virtualSwitch{QHostAddress("172.28.176.1"),false,false,false,1},ethernet{QHostAddress("192.168.11.103"),true,true,false,25},wifi{QHostAddress("10.0.0.8"),true,true,true,50},vpn{QHostAddress("10.9.0.2"),false,true,false,1};
        checks["phone_prefers_lan_over_virtual_adapters"]=lan::selectAddress({virtualSwitch,ethernet})=="192.168.11.103"&&lan::selectAddress({ethernet,virtualSwitch})=="192.168.11.103"&&lan::selectAddress({vpn,ethernet})=="192.168.11.103"&&lan::selectAddress({ethernet,wifi})=="10.0.0.8";
        checks["phone_handles_isolated_or_missing_lan"]=lan::selectAddress({virtualSwitch})=="127.0.0.1"&&lan::selectAddress({virtualSwitch,{QHostAddress("192.168.8.1"),true,false,false,10}})=="192.168.8.1"&&lan::selectAddress({{QHostAddress("169.254.1.2"),true,true,false,1}})=="127.0.0.1";
        checks["phone_state_matches_desktop"]=mobileState()["operatorId"].toInt()==4&&mobileState()["profile"].toObject()["vertical"].toDouble()==12.3;
        // Exercise timing and drag behavior on all three production scrollbars.
        settings_->setMinimumHeight(scroll_->viewport()->height()+300);QApplication::processEvents();gear_->click();QApplication::processEvents();
        auto* operatorBar=static_cast<FadingScrollBar*>(scroll_->verticalScrollBar());auto* preferenceBar=static_cast<FadingScrollBar*>(preferences_scroll_->verticalScrollBar());
        const auto sidebarWidth=sidebar_scroll_->viewport()->width(),preferenceWidth=preferences_scroll_->viewport()->width();
        for(auto* bar:{sidebarBar,operatorBar,preferenceBar}){bar->setValue(1);bar->reveal();}wait(130);
        checks["all_scrollbars_fade_in"]=sidebarBar->opacity()>.99&&operatorBar->opacity()>.99&&preferenceBar->opacity()>.99;
        wait(2100);for(auto* bar:{sidebarBar,operatorBar,preferenceBar})bar->setValue(2);wait(1100);
        checks["scrolling_resets_three_second_timer"]=sidebarBar->opacity()>.99&&operatorBar->opacity()>.99&&preferenceBar->opacity()>.99;
        wait(2250);for(int attempt=0;attempt<80&&(sidebarBar->opacity()>0||operatorBar->opacity()>0||preferenceBar->opacity()>0);++attempt)wait(50);checks["all_scrollbars_fade_out_after_idle"]=sidebarBar->opacity()==0&&operatorBar->opacity()==0&&preferenceBar->opacity()==0;
        checks["scrollbar_fades_do_not_shift_content"]=sidebarWidth==sidebar_scroll_->viewport()->width()&&preferenceWidth==preferences_scroll_->viewport()->width();
        for(auto* bar:{sidebarBar,operatorBar,preferenceBar})bar->setSliderDown(true);wait(3350);
        checks["drag_keeps_all_scrollbars_visible"]=sidebarBar->opacity()>.99&&operatorBar->opacity()>.99&&preferenceBar->opacity()>.99;
        for(auto* bar:{sidebarBar,operatorBar,preferenceBar})bar->setSliderDown(false);wait(3350);for(int attempt=0;attempt<80&&(sidebarBar->opacity()>0||operatorBar->opacity()>0||preferenceBar->opacity()>0);++attempt)wait(50);
        checks["scrollbars_fade_after_drag_release"]=sidebarBar->opacity()==0&&operatorBar->opacity()==0&&preferenceBar->opacity()==0;
        settings_->setMinimumHeight(0);back_->click();sidebarBar->setValue(0);scroll_->verticalScrollBar()->setValue(0);showNormal();raise();activateWindow();wait(3400);capture(output+"/finished.png");
        const auto unchanged=engine_.snapshot().bytes();QTimer::singleShot(80,this,[=]{auto* modal=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(modal){modal->grab().save(output+"/color.png");modal->reject();}});chooseColor();checks["cancel_color_preserves_settings"]=engine_.snapshot().bytes()==unchanged;
        bool bindingFits=true;
        QTimer::singleShot(80,this,[&]{auto* modal=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(modal){modal->grab().save(output+"/binding.png");bindingFits&=modal->objectName()=="hotkeyDialog"&&modal->width()>=380&&modal->findChild<QLabel*>("bindingHint")->wordWrap();for(auto* label:modal->findChildren<QLabel*>())bindingFits&=modal->rect().contains(QRect(label->mapTo(modal,QPoint()),label->size()));modal->reject();}});bindKey(0);checks["cancel_key_binding_preserves_settings"]=engine_.snapshot().bytes()==unchanged;
        for(int language=0;language<9;++language){engine_.edit([=](r6::Config& c){c.language=language;});QTimer::singleShot(40,this,[&,language]{auto* modal=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!modal){bindingFits=false;return;}auto* hint=modal->findChild<QLabel*>("bindingHint");hint->setText(text(UiText::ThisKeyIsAlreadyAssignedChooseAnother));modal->adjustSize();QApplication::processEvents();for(auto* label:modal->findChildren<QLabel*>()){bindingFits&=modal->rect().contains(QRect(label->mapTo(modal,QPoint()),label->size()));if(label->wordWrap())bindingFits&=label->height()>=label->heightForWidth(label->width());}if(language==0)modal->grab().save(output+"/binding-portuguese-conflict.png");modal->reject();});bindKey(0);}engine_.edit([](r6::Config& c){c.language=1;});checks["binding_instructions_fit_all_languages"]=bindingFits;
        QTimer::singleShot(80,this,[=]{auto* modal=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(modal){modal->grab().save(output+"/help.png");modal->reject();}});help();checks["help_dialog_preserves_settings"]=engine_.snapshot().bytes()==unchanged;
        // New reliability/UI behavior uses the same production paths as the real window.
        showPreferences();wait(220);buildPreferences();
        checks["brand_logo_asset_valid"]=QSvgRenderer(QStringLiteral(":/ui/logo.svg")).isValid()&&(rally?(!findChild<QLabel*>("brandLogo")&&findChild<QLabel*>("brand")->width()==118):bool(findChild<QLabel*>("brandLogo")));
        checks["qr_is_obscured_by_default"]=pairing_qr_&&!pairing_qr_->revealed();
        if(pairing_qr_){
            checks["qr_keeps_full_scannable_size"]=pairing_qr_->size()==QSize(184,208);
            preferences_scroll_->ensureWidgetVisible(pairing_qr_);for(int attempt=0;attempt<200&&!pairing_qr_->framesReady();++attempt)wait(10);wait(120);checks["qr_frames_ready_before_reveal"]=pairing_qr_->framesReady();pairing_qr_->grab().save(output+"/qr-hidden.png");
            pairing_qr_->click();checks["qr_click_reveals"]=pairing_qr_->revealed()&&pairing_qr_->transitioning();wait(120);checks["qr_reveal_has_intermediate_frames"]=pairing_qr_->revealProgress()>0&&pairing_qr_->revealProgress()<1;pairing_qr_->grab().save(output+"/qr-revealing.png");
            const qreal beforeQrReverse=pairing_qr_->revealProgress();pairing_qr_->click();checks["qr_retarget_preserves_displayed_progress"]=pairing_qr_->revealProgress()==beforeQrReverse;wait(30);pairing_qr_->click();wait(460);checks["rapid_qr_reversal_settles_revealed"]=pairing_qr_->revealed()&&pairing_qr_->revealProgress()==1&&!pairing_qr_->transitioning();pairing_qr_->grab().save(output+"/qr-revealed.png");
            capture(output+"/phone-branded.png");
            // Decode these production-rendered images independently with qr_scan.py.
            // Only the current session's hash is retained; sample URLs are fixtures.
            const auto scanCase=[](const QString& file,const QString& link){return QJsonObject{{"file",file},{"sha256",QString::fromLatin1(QCryptographicHash::hash(link.toUtf8(),QCryptographicHash::Sha256).toHex())}};};
            QJsonArray qrCases;qrCases.append(scanCase("qr-revealed.png",phone_->pairingLink()));
            pairing_qr_->click();wait(100);checks["qr_conceal_has_intermediate_frames"]=!pairing_qr_->revealed()&&pairing_qr_->revealProgress()>0&&pairing_qr_->revealProgress()<1;pairing_qr_->grab().save(output+"/qr-concealing.png");wait(250);
            pairing_qr_->setPairingLink(phone_->pairingLink()+"&refresh=1");checks["changed_qr_link_conceals"]=!pairing_qr_->revealed();
            pairing_qr_->setPairingLink(phone_->pairingLink());pairing_qr_->click();showOperators();wait(220);checks["leaving_settings_conceals_qr"]=!pairing_qr_->revealed();
            const QStringList sampleLinks={
                "http://127.0.0.1:18764/#token=0123456789abcdef0123456789abcdef",
                "http://192.168.1.23:18764/#token=ffeeddccbbaa99887766554433221100",
                "http://10.0.0.8:65535/#token=6be3872124af49ccac9c76bdbccdae94",
                "http://172.31.255.254:18764/#token=4bbdc4dca77d42e0a61f8da225ac028e",
                "https://github.com/Azec-Zuckerberg/Azec"};
            int sampleIndex=0;bool samplesReady=true;
            for(const auto& link:sampleLinks){
                PairingQr sample(QStringLiteral("Reveal QR"),QStringLiteral("Hide QR"));sample.setPairingLink(link);
                for(int attempt=0;attempt<200&&!sample.framesReady();++attempt)wait(10);
                samplesReady&=sample.framesReady();sample.click();wait(460);
                const auto name=QString("qr-sample-%1.png").arg(++sampleIndex);samplesReady&=sample.grab().save(output+"/"+name);qrCases.append(scanCase(name,link));
            }
            checks["branded_qr_samples_rendered"]=samplesReady;
            QFile scanManifest(output+"/qr-scan-cases.json");scanManifest.open(QIODevice::WriteOnly);scanManifest.write(QJsonDocument(qrCases).toJson());
        }
        showPreferences();wait(220);phone_enabled_->setFocus(Qt::MouseFocusReason);preferences_scroll_->ensureWidgetVisible(phone_enabled_);wait(80);
        auto* stablePreferences=preferences_scroll_->widget();auto* stablePhoneSwitch=phone_enabled_.data();auto* stableBar=preferences_scroll_->verticalScrollBar();
        // Anchor away from the bottom limit: exact position is possible only
        // while the old offset remains inside the collapsed scroll range.
        stableBar->setValue(std::min(stableBar->value(),std::max(0,stableBar->maximum()-phone_details_->height())/2));
        const int phoneScroll=stableBar->value();const QPoint togglePosition=phone_enabled_->mapTo(preferences_scroll_->viewport(),QPoint());
        pairing_qr_->click();wait(460);phone_enabled_->click();
        checks["phone_disable_stops_server_and_conceals_immediately"]=!phone_->listening()&&!pairing_qr_->revealed()&&pairing_qr_->revealProgress()==0;
        wait(90);checks["phone_details_collapse_between_endpoints"]=phone_details_->transitioning()&&phone_details_->progress()>0&&phone_details_->progress()<1;capture(output+"/phone-collapsing.png");wait(250);
        checks["phone_toggle_preserves_page_switch_and_scrollbar"]=preferences_scroll_->widget()==stablePreferences&&phone_enabled_==stablePhoneSwitch&&preferences_scroll_->verticalScrollBar()==stableBar;
        checks["phone_disable_keeps_toggle_anchored"]=phone_enabled_->mapTo(preferences_scroll_->viewport(),QPoint())==togglePosition&&stableBar->value()==phoneScroll&&phone_details_->height()==0;
        capture(output+"/phone-disabled.png");save();{r6::Engine restored(true);checks["disabled_phone_preference_persists"]=restored.load(config_path_)&&!restored.snapshot().phone_enabled;}
        phone_enabled_->click();wait(70);const int beforePhoneReverse=phone_details_->height();phone_enabled_->click();checks["phone_retarget_preserves_displayed_height"]=phone_details_->height()==beforePhoneReverse;wait(25);phone_enabled_->click();wait(360);
        checks["rapid_phone_toggle_settles_enabled"]=phone_->listening()&&phone_details_->expanded()&&!phone_details_->transitioning()&&phone_details_->progress()==1&&pairing_qr_->isEnabled()&&!pairing_qr_->revealed()&&preferences_scroll_->widget()==stablePreferences;
        stableBar->setValue(stableBar->maximum());const int expandedBottom=stableBar->maximum();phone_enabled_->click();wait(90);
        checks["phone_bottom_scroll_adjusts_during_collapse"]=stableBar->maximum()>0&&stableBar->maximum()<expandedBottom&&stableBar->value()==stableBar->maximum()&&phone_details_->transitioning();wait(250);
        checks["phone_bottom_scroll_stays_in_range_without_reset"]=stableBar->value()==stableBar->maximum()&&stableBar->value()>0&&preferences_scroll_->widget()==stablePreferences;
        phone_enabled_->click();wait(340);
        preferences_scroll_->ensureWidgetVisible(pairing_qr_);wait(80);capture(output+"/phone-enabled.png");
        pairing_qr_->click();wait(80);hide();show();wait(350);checks["hiding_window_cancels_qr_reveal"]=!pairing_qr_->revealed()&&pairing_qr_->revealProgress()==0&&!pairing_qr_->transitioning();showOperators();wait(220);
        checks["tray_has_window_recoil_rapid_exit"]=tray_&&tray_menu_&&tray_recoil_->isCheckable()&&tray_rapid_->isCheckable()&&tray_exit_;
        const auto baseline=engine_.snapshot();const auto backup=makeBackup(baseline,favorites_);r6::Config imported;QSet<QString> importedFavorites;
        checks["complete_profile_backup_roundtrip"]=parseBackup(backup,imported,importedFavorites)&&imported.bytes()==baseline.bytes()&&importedFavorites==favorites_&&imported.secondary_enabled==baseline.secondary_enabled;
        auto malformedBackup=backup;auto entries=backup["operators"].toArray();entries.removeLast();entries.removeLast();malformedBackup["operators"]=entries;
        const auto unchangedBeforeImport=engine_.snapshot().bytes();
        checks["invalid_import_preserves_profiles"]=!applyBackup(QJsonDocument(malformedBackup).toJson())&&engine_.snapshot().bytes()==unchangedBeforeImport;
        checks["valid_import_updates_production_ui"]=applyBackup(QJsonDocument(backup).toJson())&&unsaved_&&engine_.snapshot().bytes()==baseline.bytes();
        checks["all_hotkeys_checked_for_conflicts"]=hotkeyConflict(baseline,3,baseline.menu_key)==0&&hotkeyConflict(baseline,3,0)==-1;
        checks["phone_rejects_global_hotkey_collision"]=mobileUpdate({{"action","setGlobal"},{"values",QJsonObject{{"key3",baseline.menu_key}}}}).contains("error");
        engine_.edit([](r6::Config& config){config.neon[0]=.2f;config.neon[1]=.4f;config.neon[2]=.8f;});applyStyle();dirty();wait(180);
        const auto blueInk=[](const QImage& image){int count=0;for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x){const auto color=image.pixelColor(x,y);if(color.blue()>color.red()+40&&color.blue()>color.green()+20)++count;}return count;};
        checks["dirty_save_uses_custom_accent"]=unsaved_&&save_->property("unsaved").toBool()&&blueInk(save_->grab().toImage())>12;
        save_blocked_=true;save();checks["failed_save_retains_accent_and_dirty_state"]=unsaved_&&save_->property("unsaved").toBool()&&blueInk(save_->grab().toImage())>12;
        save_blocked_=false;save();wait(180);checks["successful_save_returns_to_neutral"]=!unsaved_&&!save_->property("unsaved").toBool()&&blueInk(save_->grab().toImage())==0;
        editOperator([](r6::Operator& op){op.vertical=13.1f;});
        QTimer::singleShot(80,this,[this,output]{if(auto* modal=QApplication::activeModalWidget()){modal->grab().save(output+"/unsaved-close.png");if(auto* cancel=modal->findChild<QPushButton*>("cancelClose"))cancel->click();}});
        close();checks["cancel_close_keeps_unsaved_app_running"]=isVisible()&&engine_.running&&unsaved_;
        bool repeat_surfaced=false,repeat_kept_app=false;
        QTimer::singleShot(80,this,[this]{close();});
        QTimer::singleShot(130,this,[&]{repeat_surfaced=close_alert_!=nullptr&&QApplication::activeModalWidget()==close_alert_;});
        QTimer::singleShot(180,this,[]{if(auto* modal=QApplication::activeModalWidget())if(auto* cancel=modal->findChild<QPushButton*>("cancelClose"))cancel->click();});
        close();repeat_kept_app=isVisible()&&engine_.running&&unsaved_;
        checks["repeat_close_surfaces_prompt_again"]=repeat_surfaced&&repeat_kept_app;
        save_blocked_=true;QTimer::singleShot(80,this,[]{if(auto* modal=QApplication::activeModalWidget())if(auto* button=modal->findChild<QPushButton*>("save"))button->click();});
        close();checks["failed_save_during_close_keeps_window_open"]=isVisible()&&engine_.running&&unsaved_;save_blocked_=false;
        engine_.edit([&](r6::Config& config){config=baseline;});applyStyle();syncSelection();dirty();save();wait(1800);
        status_->setFocus(Qt::OtherFocusReason);paint_.stop();const QPoint stationary(-20000,-20000);trackCursor(stationary);wait(100);const int idleRequests=cursor_repaints_;
        for(int sample=0;sample<12;++sample)trackCursor(stationary);wait(350);
        checks["stationary_cursor_does_not_schedule_background_repaints"]=cursor_repaints_==idleRequests;
        trackCursor(stationary+QPoint(1,0));checks["cursor_movement_schedules_one_background_repaint"]=cursor_repaints_==idleRequests+1;paint_.start();
        // Wait for delayed feedback (including the three-second scrollbar fade)
        // to finish, then require the same strict 350 ms without a repaint.
        // Continuous repainting still fails after the bounded settle period.
        QCursor::setPos(mapToGlobal(QPoint(-20,-20)));QElapsedTimer idleClock;idleClock.start();qint64 quietSince=0;int idlePaints=paint_count_;
        while(idleClock.elapsed()<6000&&idleClock.elapsed()-quietSince<350){wait(25);if(paint_count_!=idlePaints){idlePaints=paint_count_;quietSince=idleClock.elapsed();}}
        checks["stationary_cursor_does_not_repaint_window"]=idleClock.elapsed()-quietSince>=350;
        capture(output+"/save-neutral.png");editOperator([](r6::Operator& op){op.vertical=13.2f;});capture(output+"/save-unsaved.png");save();
        // Load previous default themes and a custom theme through the production loader.
        // The fixture lives in its own directory so no azec_weapons.json sidecar applies.
        const auto themeDirectory=std::filesystem::path(output.toStdWString())/L"theme-fixture";std::error_code themeError;std::filesystem::create_directories(themeDirectory,themeError);const auto themeFixture=themeDirectory/L"theme-test.bin";r6::Config theme;
        bool migrationPassed=true;for(const auto& legacy:std::array<std::array<float,3>,4>{{{0,1,.4f},{5.f/255,134.f/255,105.f/255},{1.f,91.f/255,99.f/255},{210.f/255,210.f/255,210.f/255}}}){for(int i=0;i<3;++i)theme.neon[i]=legacy[i];r6::Engine migrated(true);migrationPassed&=theme.save(themeFixture)&&migrated.load(themeFixture);const auto loaded=migrated.snapshot();migrationPassed&=QColor::fromRgbF(loaded.neon[0],loaded.neon[1],loaded.neon[2]).name()=="#7248d7"&&loaded.operators[4].vertical==theme.operators[4].vertical;}
        checks["previous_default_accents_migrate_without_profile_changes"]=migrationPassed;
        theme.neon[0]=.2f;theme.neon[1]=.4f;theme.neon[2]=.8f;r6::Engine customTheme(true);const bool customLoaded=theme.save(themeFixture)&&customTheme.load(themeFixture);checks["custom_accent_is_preserved"]=customLoaded&&customTheme.snapshot().bytes()==theme.bytes();
        showPreferences();wait(220);
        checks["interface_picker_has_only_goofy_and_rally"]=interface_choice_->count()==2&&interface_choice_->itemText(0)=="Goofy"&&interface_choice_->itemText(1)=="Rally";
        checks["active_interface_needs_no_restart"]=interface_choice_->currentData().toInt()==int(presentation::active)&&!interface_apply_->isEnabled();
        capture(output+"/interface-settings.png");
        const auto originalUiFile=favoritesPath();QFile uiBefore(QString::fromStdWString(originalUiFile.wstring()));uiBefore.open(QIODevice::ReadOnly);const auto oldUiBytes=uiBefore.readAll();uiBefore.close();
        const auto target=rally?presentation::Interface::Goofy:presentation::Interface::Rally;const auto beforeChoice=engine_.snapshot().bytes();
        interface_choice_->setCurrentIndex(interface_choice_->findData(int(target)));
        checks["choosing_interface_does_not_rebuild_or_edit_profiles"]=interface_apply_->isEnabled()&&engine_.snapshot().bytes()==beforeChoice&&presentation::isRally()==rally;
        bool restartCalled=false;const auto restartAction=restart_action_;restart_action_=[&]{restartCalled=true;};
        editOperator([](r6::Operator& op){op.vertical=14.7f;});save_blocked_=true;interface_apply_->click();
        checks["failed_profile_save_blocks_interface_restart"]=!restartCalled&&!restart_requested_&&unsaved_&&interface_feedback_->isVisible();save_blocked_=false;
        interface_apply_->click();r6::Config restartSaved;QJsonObject selectedUi;presentation::readDocument(presentation::filePath(config_path_),selectedUi);
        checks["interface_apply_saves_pending_profiles_before_restart"]=restartCalled&&restart_requested_&&!unsaved_&&restartSaved.load(config_path_)&&std::abs(restartSaved.operators[engine_.snapshot().selected].vertical-14.7f)<.001;
        checks["interface_choice_is_persisted_with_favorites"]=presentation::read(config_path_)==target&&selectedUi["interface"].toString()==presentation::name(target)&&readFavorites()==favorites_;
        persistFavorites();checks["favorite_save_preserves_interface_choice"]=presentation::read(config_path_)==target;
        const auto uiFixture=std::filesystem::path(output.toStdWString())/L"interface-preferences"/L"r6_config.bin";QDir().mkpath(QString::fromStdWString(uiFixture.parent_path().wstring()));
        checks["missing_interface_defaults_to_goofy"]=presentation::read(uiFixture)==presentation::Interface::Goofy;
        QFile invalidUi(presentation::filePath(uiFixture));invalidUi.open(QIODevice::WriteOnly);invalidUi.write("{broken");invalidUi.close();
        checks["invalid_ui_preferences_are_not_overwritten"]=presentation::read(uiFixture)==presentation::Interface::Goofy&&!presentation::write(uiFixture,presentation::Interface::Rally);
        invalidUi.open(QIODevice::WriteOnly|QIODevice::Truncate);invalidUi.write("{\"version\":1,\"interface\":\"atelier\",\"favorites\":[\"Ash\"]}");invalidUi.close();
        checks["unsupported_interface_defaults_to_goofy"]=presentation::read(uiFixture)==presentation::Interface::Goofy;
        presentation::write(uiFixture,presentation::Interface::Rally);QJsonObject uiFixtureRead;presentation::readDocument(presentation::filePath(uiFixture),uiFixtureRead);
        checks["interface_write_preserves_other_ui_preferences"]=uiFixtureRead["favorites"].toArray().contains("Ash");
        // A blocked UI-preference destination must not close the running app.
        restartCalled=false;restart_requested_=false;const auto uiName=presentation::filePath(config_path_),uiBackup=uiName+".test-backup";QFile::rename(uiName,uiBackup);QDir().mkdir(uiName);interface_apply_->click();
        checks["failed_interface_save_keeps_app_open"]=!restartCalled&&!restart_requested_&&isVisible()&&interface_feedback_->isVisible();QDir().rmdir(uiName);QFile::rename(uiBackup,uiName);
        presentation::write(config_path_,presentation::active);pending_interface_=presentation::active;restart_action_=restartAction;restart_requested_=false;buildPreferences();
        QJsonObject report;report["interface"]=presentation::name(presentation::active);report["checks"]=checks;report["rounding_method"]=clipped?"native rounded window region (Windows 10)":"DWM rounded corners";report["icon_failures"]=QJsonArray::fromStringList(icon_failures_);report["font_family"]=QFontInfo(operator_name_->font()).family();report["heading_font_style"]=QFontInfo(operator_name_->font()).styleName();report["body_font_style"]=QFontInfo(humanize_->parentWidget()->findChild<QLabel*>()->font()).styleName();report["save_font_family"]=QFontInfo(save_->font()).family();report["paint_events"]=paint_count_;report["mouse_api"]="SendInput";report["input_injection"]="disabled";report["width"]=width();report["height"]=height();report["status_text"]=status_->text();report["display_scale"]=devicePixelRatioF();bool passed=true;for(const auto& check:checks)passed&=check.toBool();report["passed"]=passed;return report;
    }
