#include "main_window.hpp"
#include "profile_transfer.hpp"

void MainWindow::exportProfiles(){
    const auto path=QFileDialog::getSaveFileName(this,text(UiText::ExportProfiles),"azec-profiles.json","JSON (*.json)",nullptr,QFileDialog::DontUseNativeDialog);
    if(path.isEmpty())return;
    const auto bytes=QJsonDocument(makeBackup(engine_.snapshot(),favorites_,engine_.game_focus_only.load(),
        kbtheme::activeColorTheme,sidebar_collapsed_,engine_.ads_only.load())).toJson();QSaveFile file(path);
    if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){
        diagnostic("export profiles",file.errorString());showNotice(text(UiText::CouldNotExportProfilesCheckTheDestinationPermissions),true,true);
    }else showNotice(text(UiText::ProfilesExported));
}
bool MainWindow::applyBackup(const QByteArray& bytes){
    if(bytes.size()>1024*1024)return false;
    QJsonParseError error;const auto document=QJsonDocument::fromJson(bytes,&error);r6::Config config;QSet<QString> favorites;BackupOptions options;
    if(error.error!=QJsonParseError::NoError||!document.isObject()||!parseBackup(document.object(),config,favorites,options))return false;
    // Keep imported UI choices in memory alongside the unsaved profiles.
    // Save commits all three files; Discard (including panic exit) writes none.
    QJsonObject ui;
    if(!uiprefs::readDocument(uiprefs::filePath(config_path_),ui))return false;
    imported_ui_pending_=true;favorites_=favorites;
    engine_.edit([&](r6::Config& current){current=config;});
    mouse_draft_.reset();mouse_source_.clear();mouse_setup_dirty_=false;
    if(options.gameFocusOnly)engine_.game_focus_only=*options.gameFocusOnly;
    if(options.adsOnly)engine_.ads_only=*options.adsOnly;
    if(config.phone_enabled)phone_->start();else phone_->stop();
    if(!options.colorTheme.isEmpty())chooseColorTheme(options.colorTheme);
    if(options.sidebarCollapsed)setSidebarCollapsed(*options.sidebarCollapsed,false);
    search_->clear();refreshFavorites();buildPreferences();buildSettings();updateLanguage();applyStyle();syncSidebarSelection();dirty();return true;
}
void MainWindow::importProfiles(){
    const auto path=QFileDialog::getOpenFileName(this,text(UiText::ImportProfiles),{},"JSON (*.json)",nullptr,QFileDialog::DontUseNativeDialog);
    if(path.isEmpty())return;QFile file(path);
    if(!file.open(QIODevice::ReadOnly)||file.size()>1024*1024||!applyBackup(file.readAll())){
        diagnostic("import profiles","Invalid backup, unsupported version, read error, or favorites write failure");
        showNotice(text(UiText::ImportFailedCheckTheBackupFormatAndFilePermissionsExistingProfile),true,true);
    }else showNotice(text(UiText::ProfilesImportedSaveToKeepTheChanges));
}
