#include "main_window.hpp"
#include "profile_transfer.hpp"

void MainWindow::exportProfiles(){
    const auto path=QFileDialog::getSaveFileName(this,text(UiText::ExportProfiles),"kingboss-profiles.json","JSON (*.json)",nullptr,QFileDialog::DontUseNativeDialog);
    if(path.isEmpty())return;
    const auto bytes=QJsonDocument(makeBackup(engine_.snapshot(),favorites_)).toJson();QSaveFile file(path);
    if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){
        diagnostic("export profiles",file.errorString());notice_->setText(text(UiText::CouldNotExportProfilesCheckTheDestinationPermissions));
    }else notice_->setText(text(UiText::ProfilesExported));
    notice_->show();
}
bool MainWindow::applyBackup(const QByteArray& bytes){
    if(bytes.size()>1024*1024)return false;
    QJsonParseError error;const auto document=QJsonDocument::fromJson(bytes,&error);r6::Config config;QSet<QString> favorites;
    if(error.error!=QJsonParseError::NoError||!document.isObject()||!parseBackup(document.object(),config,favorites))return false;
    const auto old=favorites_;favorites_=favorites;
    if(!persistFavorites()){favorites_=old;return false;}
    engine_.edit([&](r6::Config& current){current=config;});engine_.rapid_master=bool(config.rapid_master);
    if(config.phone_enabled)phone_->start();else phone_->stop();
    refreshFavorites();buildPreferences();buildSettings();updateLanguage();applyStyle();dirty();return true;
}
void MainWindow::importProfiles(){
    const auto path=QFileDialog::getOpenFileName(this,text(UiText::ImportProfiles),{},"JSON (*.json)",nullptr,QFileDialog::DontUseNativeDialog);
    if(path.isEmpty())return;QFile file(path);
    if(!file.open(QIODevice::ReadOnly)||file.size()>1024*1024||!applyBackup(file.readAll())){
        diagnostic("import profiles","Invalid backup, unsupported version, read error, or favorites write failure");
        notice_->setText(text(UiText::ImportFailedCheckTheBackupFormatAndFilePermissionsExistingProfile));
    }else notice_->setText(text(UiText::ProfilesImportedSaveToKeepTheChanges));
    notice_->show();
}
