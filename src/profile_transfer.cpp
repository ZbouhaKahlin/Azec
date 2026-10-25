#include "main_window.hpp"
#include "profile_transfer.hpp"

bool MainWindow::confirmProfileAction(const QString& title,const QString& body,const QString& action){
    QDialog dialog(this,Qt::Dialog|Qt::FramelessWindowHint);dialog.setObjectName("helpDialog");dialog.setAttribute(Qt::WA_StyledBackground);dialog.setWindowTitle(title);dialog.setFixedWidth(420);
    auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(24,20,24,22);layout->setSpacing(16);auto* heading=label(title,"cardTitle");heading->setWordWrap(true);layout->addWidget(heading);auto* description=label(body);description->setTextFormat(Qt::PlainText);description->setWordWrap(true);layout->addWidget(description);
    auto* row=new QHBoxLayout;auto* cancel=new MonoButton(text(UiText::Cancel));cancel->setObjectName("secondary");auto* apply=new MonoButton(action);apply->setObjectName("save");row->addWidget(cancel);row->addWidget(apply);layout->addLayout(row);
    connect(cancel,&QPushButton::clicked,&dialog,&QDialog::reject);connect(apply,&QPushButton::clicked,&dialog,&QDialog::accept);dialog.adjustSize();roundWindow(&dialog);dialog.move(frameGeometry().center()-dialog.rect().center());return dialog.exec()==QDialog::Accepted;
}
void MainWindow::exportProfiles(){
    const auto path=QFileDialog::getSaveFileName(this,text(UiText::ExportBackup),"azec-backup.json","JSON (*.json)",nullptr,QFileDialog::DontUseNativeDialog);
    if(path.isEmpty())return;
    const auto bytes=QJsonDocument(makeBackup(engine_.snapshot(),favorites_,engine_.game_focus_only.load(),
        engine_.ads_only.load())).toJson(QJsonDocument::Compact);QSaveFile file(path);
    if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){
        diagnostic("export profiles",file.errorString());showNotice(text(UiText::CouldNotExportProfilesCheckTheDestinationPermissions),true,true);
    }else showNotice(text(UiText::ProfilesExported));
}
bool MainWindow::applyBackup(const QByteArray& bytes){
    if(bytes.size()>1024*1024)return false;
    QJsonParseError error;const auto document=QJsonDocument::fromJson(bytes,&error);const auto existing=engine_.snapshot();r6::Config config;QSet<QString> favorites;BackupOptions options;
    if(error.error!=QJsonParseError::NoError||!document.isObject()||!parseBackup(document.object(),config,favorites,options,&existing)){
        diagnostic("import profiles","Invalid backup or unsupported version");return false;
    }
    // Keep imported UI choices in memory alongside the unsaved profiles.
    // Save commits all three files; Discard (including panic exit) writes none.
    QJsonObject ui;
    if(!uiprefs::readDocument(uiprefs::filePath(config_path_),ui)){
        diagnostic("import profiles","Could not read existing UI preferences; no changes applied");return false;
    }
    imported_ui_pending_=true;favorites_=favorites;
    engine_.edit([&](r6::Config& current){current=config;});
    if(options.gameFocusOnly)engine_.game_focus_only=*options.gameFocusOnly;
    if(options.adsOnly)engine_.ads_only=*options.adsOnly;
    if(config.phone_enabled)phone_->start();else phone_->stop();
    search_->clear();refreshFavorites();buildPreferences();buildSettings();updateLanguage();applyStyle();syncSidebarSelection();dirty();return true;
}
void MainWindow::importProfiles(){
    if(save_blocked_)return;
    const auto path=QFileDialog::getOpenFileName(this,text(UiText::ImportBackup),{},"JSON (*.json)",nullptr,QFileDialog::DontUseNativeDialog);
    if(path.isEmpty())return;QFile file(path);QByteArray bytes;QJsonDocument document;
    r6::Config imported;QSet<QString> favorites;BackupOptions options;const auto existing=engine_.snapshot();
    if(file.open(QIODevice::ReadOnly)&&file.size()<=1024*1024){bytes=file.readAll();document=QJsonDocument::fromJson(bytes);}
    const bool valid=file.error()==QFile::NoError&&document.isObject()&&parseBackup(document.object(),imported,favorites,options,&existing);file.close();
    if(!valid){
        diagnostic("import profiles",file.error()!=QFile::NoError?file.errorString():QStringLiteral("Invalid backup, unsupported version, or file exceeds 1 MiB"));
        showNotice(text(UiText::ImportFailedCheckTheBackupFormatAndFilePermissionsExistingProfile),true,true);return;
    }
    const auto summary=text(UiText::ImportBackupPreview).arg(QFileInfo(path).fileName()).arg(document.object()["operators"].toArray().size());
    if(!confirmProfileAction(text(UiText::ImportBackup),summary,text(UiText::ImportBackup)))return;
    if(!applyBackup(bytes)){
        showNotice(text(UiText::ImportFailedCheckTheBackupFormatAndFilePermissionsExistingProfile),true,true);
    }else showNotice(text(UiText::ProfilesImportedSaveToKeepTheChanges));
}
