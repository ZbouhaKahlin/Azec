#pragma once
#include <QtCore>
#include <functional>

// All desktop presentation strings, indexed independently of the active language.
enum class UiText {
    OpenHideWindow,
    RecoilActive,
    Exit,
    SaveConfigurationUnsavedChanges,
    ConfigurationSaved,
    CouldNotSaveConfiguration,
    SaveConfiguration,
    CouldNotSaveFavorites,
    RemoveFavorite,
    Favorite,
    Value,
    WeaponProfiles,
    SeparateSecondaryProfile,
    Use1And2ToSwitchWeaponProfilesChangeTheKeysInSettings,
    RecoilControl,
    MasterEnable,
    ApplyThisOperatorSRecoilSettings,
    VerticalForce,
    HorizontalAdjust,
    HumanizeMovement,
    VaryMovementSlightlyOnBothAxes,
    RapidFire,
    EnableForThisOperator,
    ShotDelay,
    DelayBetweenRepeatedShots,
    Ms,
    SystemSettings,
    FeedbackSound,
    SoundWhenClosingUnsaved,
    Language,
    RecoilToggle,
    PanicExitKey,
    PrimaryWeapon,
    SecondaryWeapon,
    PhoneControl,
    EnablePhoneAccess,
    CouldNotStartPhoneServer,
    PairingCode,
    CopyLink,
    Copied,
    OpenThisAddressInSafariOnTheSameWiFi,
    ClickToRevealQR,
    ClickToHideQR,
    Appearance,
    QuickGuide,
    ProfileBackup,
    ExportProfiles,
    ImportProfiles,
    AllOperators,
    Favorites,
    SearchOperators,
    OperatorSettings,
    Settings,
    BackToOperators,
    Primary,
    Secondary,
    RecoilOn,
    RecoilOff,
    Cancel,
    KeyBinding,
    SetAHotkey,
    CancelHotkeyBinding,
    ReleaseYourKeysWaiting,
    EscCancelsBackspaceClears,
    PressAKeyNow,
    ThisKeyIsAlreadyAssignedChooseAnother,
    AzecHelp,
    CloseQuickGuide,
    Back,
    Next,
    Welcome,
    ChooseAnOperator,
    AdjustRecoil,
    HotkeysAndSaving,
    YourOperatorProfilesAndControlsInOnePlace,
    SelectANameInTheSidebarOrSearchForAnOperator,
    SetVerticalAndHorizontalForceUseArrowKeysForFineAdjustments,
    EnableRapidFireForTheSelectedWeapon,
    SetYourHotkeysInSystemSettingsSaveConfigurationToKeepYourChanges,
    Done,
    UnsavedChanges,
    SaveYourChangesBeforeClosing,
    Discard,
    Save,
    ToggleRecoilActivity,
    Minimize,
    Close,
    SearchOperators4c533,
    FavoriteOperators,
    CouldNotExportProfilesCheckTheDestinationPermissions,
    ProfilesExported,
    ImportFailedCheckTheBackupFormatAndFilePermissionsExistingProfile,
    ProfilesImportedSaveToKeepTheChanges,
    ReadSettingsFailure,
    PreserveSettingsFailure,
    RecoveredSettings,
    ConflictingHotkeys,
    NoneKey,
    LeftKey,
    RightKey,
    UpKey,
    DownKey,
    SettingsCategory,
    General,
    Shortcuts,
    ColorTheme,
    ColorThemeHint,
    ThemeSaveFailed,
    CollapseSidebar,
    ExpandSidebar,
    SidebarSaveFailed,
    Collapse,
    StartupLockFailure,
    RapidFireGlobalHint,
    OnlyWhileGameIsActive,
    OnlyWhileGameIsActiveHint,
    CopyCode,
    NewCode,
    NewCodeCreated,
    Retry,
    UnsafeGlobalHotkey,
    RapidFireOn,
    RapidFireOff,
    Updates,
    InstalledVersion,
    CheckForUpdates,
    UpToDate,
    UpdateAvailable,
    UpdateCheckFailed,
    SettingsGeneralHint,
    SettingsAppearanceHint,
    SettingsShortcutsHint,
    SettingsPhoneHint,
    SettingsBackupHint,
    OnlyWhileAiming,
    OnlyWhileAimingHint,
    TourChooseTitle,
    TourChooseHint,
    TourProfilesHint,
    TourRecoilHint,
    TourRapidHint,
    TourShortcutsHint,
    TourSetupHint,
    SkipTour,
    ReplayTour,
    MouseSetup,
    MouseDpi,
    SensitivityHorizontal,
    SensitivityVertical,
    SensitivityMultiplier,
    ReadGameSettings,
    ChooseGameSettings,
    SaveMouseSetup,
    RescaleProfiles,
    GameSettingsMissing,
    GameSettingsMultiple,
    GameSettingsRead,
    MouseSetupHint,
    MouseSetupSaved,
    RescalePreview,
    RescaleOutOfRange,
    SensitivityChanged,
    MouseSetupRequired,
    ProfilesRescaled,
    SaveWithoutRescaling,
    MouseSetupTabHint,
    OutputMethod,
    LogitechOutput,
    LogitechOutputHint,
    LogitechScriptHint,
    LogitechHubFound,
    LogitechHubMissing,
    ExportLogitechScript,
    LogitechScriptExported,
    LogitechScriptFailed,
    LogitechScriptStale,
    DriverOutput,
    DriverOutputHint,
    DriverFound,
    DriverMissing,
};
inline constexpr int UiTextCount=180;

// One table per language, listed in exactly the enum order above.
#include "lang_en.hpp"
#include "lang_pt.hpp"
#include "lang_fr.hpp"
#include "lang_es.hpp"
#include "lang_de.hpp"
#include "lang_ar.hpp"
#include "lang_he.hpp"
#include "lang_zh.hpp"
#include "lang_ja.hpp"
static_assert(sizeof(lang_en)/sizeof(lang_en[0])==UiTextCount,"lang_en.hpp must cover every UiText value");
static_assert(sizeof(lang_pt)/sizeof(lang_pt[0])==UiTextCount,"lang_pt.hpp must cover every UiText value");
static_assert(sizeof(lang_fr)/sizeof(lang_fr[0])==UiTextCount,"lang_fr.hpp must cover every UiText value");
static_assert(sizeof(lang_es)/sizeof(lang_es[0])==UiTextCount,"lang_es.hpp must cover every UiText value");
static_assert(sizeof(lang_de)/sizeof(lang_de[0])==UiTextCount,"lang_de.hpp must cover every UiText value");
static_assert(sizeof(lang_ar)/sizeof(lang_ar[0])==UiTextCount,"lang_ar.hpp must cover every UiText value");
static_assert(sizeof(lang_he)/sizeof(lang_he[0])==UiTextCount,"lang_he.hpp must cover every UiText value");
static_assert(sizeof(lang_zh)/sizeof(lang_zh[0])==UiTextCount,"lang_zh.hpp must cover every UiText value");
static_assert(sizeof(lang_ja)/sizeof(lang_ja[0])==UiTextCount,"lang_ja.hpp must cover every UiText value");

// Language ids: 0 Portuguese, 1 English (default), 2 French, 3 Spanish, 4 German,
// 5 Arabic, 6 Hebrew (Palestine prank), 7 Simplified Chinese, 8 Japanese.
inline QString uiText(UiText id,int language){
    static const char* const* const tables[]={lang_pt,lang_en,lang_fr,lang_es,lang_de,lang_ar,lang_he,lang_zh,lang_ja};
    const int table=(language>=0&&language<=8)?language:1;
    return QString::fromUtf8(tables[table][static_cast<size_t>(id)]);
}

// Translate the bundled Qt file dialogs without external translation packs.
// Columns follow the language ids used by uiText(); a missing column falls back
// to the English source text so a partial table can never blank a dialog.
struct DialogTranslation final {const char* source;const char* text[9];};
inline const DialogTranslation dialogTranslations[]={
        {"Look in:",{"Procurar em:",nullptr,"Emplacement :","Buscar en:","Suchen in:","البحث في:","فلسطين حرة","查找范围:","検索先:"}},
        {"File name:",{"Nome do arquivo:",nullptr,"Nom du fichier :","Nombre de archivo:","Dateiname:","اسم الملف:","فلسطين حرة","文件名:","ファイル名:"}},
        {"Files of type:",{"Tipo de arquivo:",nullptr,"Types de fichiers :","Tipo de archivo:","Dateityp:","نوع الملفات:","الحرية لفلسطين","文件类型:","ファイルの種類:"}},
        {"Save in:",{"Salvar em:",nullptr,"Enregistrer dans :","Guardar en:","Speichern in:","الحفظ في:","فلسطين حرة","保存到:","保存先:"}},
        {"Open",{"Abrir",nullptr,"Ouvrir","Abrir","Öffnen","فتح","افتح فلسطين","打开","開く"}},
        {"Save",{"Salvar",nullptr,"Enregistrer","Guardar","Speichern","حفظ","احفظ فلسطين","保存","保存"}},
        {"Cancel",{"Cancelar",nullptr,"Annuler","Cancelar","Abbrechen","إلغاء","الحرية لفلسطين","取消","キャンセル"}},
        {"Close",{"Fechar",nullptr,"Fermer","Cerrar","Schließen","إغلاق","فلسطين حرة","关闭","閉じる"}},
        {"Choose",{"Escolher",nullptr,"Choisir","Elegir","Auswählen","اختيار","فلسطين حرة","选择","選択"}},
        {"All Files (*)",{"Todos os arquivos (*)",nullptr,"Tous les fichiers (*)","Todos los archivos (*)","Alle Dateien (*)","كل الملفات (*)","الحرية لفلسطين (*)","所有文件 (*)","すべてのファイル (*)"}},
        {"File",{"Arquivo",nullptr,"Fichier","Archivo","Datei","ملف","فلسطين حرة","文件","ファイル"}},
        {"Name",{"Nome",nullptr,"Nom","Nombre","Name","الاسم","فلسطين حرة","名称","名前"}},
        {"Size",{"Tamanho",nullptr,"Taille","Tamaño","Größe","الحجم","الحرية لفلسطين","大小","サイズ"}},
        {"Type",{"Tipo",nullptr,"Type","Tipo","Typ","النوع","فلسطين حرة","类型","種類"}},
        {"Date Modified",{"Data de modificação",nullptr,"Date de modification","Fecha de modificación","Änderungsdatum","تاريخ التعديل","فلسطين ستتحرر","修改日期","更新日時"}},
        {"New Folder",{"Nova pasta",nullptr,"Nouveau dossier","Nueva carpeta","Neuer Ordner","مجلد جديد","فلسطين حرة","新建文件夹","新しいフォルダー"}},
        {"Back",{"Voltar",nullptr,"Précédent","Atrás","Zurück","رجوع","فلسطين حرة","后退","戻る"}},
        {"Forward",{"Avançar",nullptr,"Suivant","Adelante","Vorwärts","تقدّم","الحرية لفلسطين","前进","進む"}},
        {"Parent Directory",{"Pasta superior",nullptr,"Dossier parent","Directorio superior","Übergeordneter Ordner","المجلد الأعلى","فلسطين حرة","上级目录","親フォルダー"}},
        {"List View",{"Lista",nullptr,"Liste","Lista","Liste","عرض القائمة","فلسطين حرة","列表视图","リスト表示"}},
        {"Detail View",{"Detalhes",nullptr,"Détails","Detalles","Details","عرض التفاصيل","الحرية لفلسطين","详细视图","詳細表示"}},
        {"Show hidden files",{"Mostrar arquivos ocultos",nullptr,"Afficher les fichiers cachés","Mostrar archivos ocultos","Versteckte Dateien anzeigen","إظهار الملفات المخفية","فلسطين حرة","显示隐藏文件","隠しファイルを表示"}},
        {"Directories",{"Pastas",nullptr,"Dossiers","Directorios","Ordner","المجلدات","فلسطين حرة","目录","フォルダー"}},
        {"Computer",{"Computador",nullptr,"Ordinateur","Equipo","Computer","الكمبيوتر","فلسطين حرة","计算机","コンピューター"}},
        {"Home",{"Início",nullptr,"Dossier personnel","Carpeta personal","Persönlicher Ordner","المجلد الرئيسي","فلسطين حرة","主目录","ホーム"}},
        {"Recent Places",{"Locais recentes",nullptr,"Emplacements récents","Lugares recientes","Zuletzt verwendet","المواقع الأخيرة","الحرية لفلسطين","最近使用的位置","最近使った場所"}},
        {"File not found.\nPlease verify the correct file name was given.",{"Arquivo não encontrado.\nVerifique o nome do arquivo.",nullptr,"Fichier introuvable.\nVérifiez que le nom du fichier est correct.","No se encontró el archivo.\nCompruebe que el nombre sea correcto.","Datei nicht gefunden.\nPrüfen Sie, ob der Dateiname korrekt ist.","لم يتم العثور على الملف.\nتحقق من صحة اسم الملف.","فلسطين حرة\nفلسطين ستتحرر","找不到文件。\n请确认文件名是否正确。","ファイルが見つかりません。\nファイル名が正しいか確認してください。"}},
        {"%1 already exists.\nDo you want to replace it?",{"%1 já existe.\nDeseja substituí-lo?",nullptr,"%1 existe déjà.\nVoulez-vous le remplacer ?","%1 ya existe.\n¿Desea reemplazarlo?","%1 ist bereits vorhanden.\nMöchten Sie es ersetzen?","%1 موجود بالفعل.\nهل تريد استبداله؟","فلسطين حرة — %1","%1 已存在。\n是否要替换它？","%1 は既に存在します。\n置き換えますか？"}},
        {"Confirm Save As",{"Confirmar salvamento",nullptr,"Confirmer Enregistrer sous","Confirmar Guardar como","Speichern unter bestätigen","تأكيد الحفظ باسم","فلسطين حرة","确认另存为","名前を付けて保存の確認"}},
        {"Yes",{"Sim",nullptr,"Oui","Sí","Ja","نعم","فلسطين حرة","是","はい"}},
        {"No",{"Não",nullptr,"Non","No","Nein","لا","فلسطين ستتحرر","否","いいえ"}},
};
class DesktopTranslator final : public QTranslator {
    std::function<int()> language_;
public:
    explicit DesktopTranslator(std::function<int()> language):language_(std::move(language)){}
    bool isEmpty() const override{return false;}
    QString translate(const char*,const char* source,const char* =nullptr,int =-1) const override{
        const int language=language_();
        if(language<0||language>8||language==1)return {};
        QString key=QString::fromUtf8(source);key.remove('&');
        for(const auto& entry:dialogTranslations){
            if(entry.text[language]&&key==QLatin1String(entry.source))return QString::fromUtf8(entry.text[language]);
        }
        return {};
    }
};
