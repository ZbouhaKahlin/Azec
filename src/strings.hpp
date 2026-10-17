#pragma once
#include <QtCore>
#include <functional>

// All desktop presentation strings, indexed independently of the active language.
enum class UiText {
    OpenHideWindow,
    RecoilActive,
    GlobalRapidFire,
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
    MasterControlForEveryOperator,
    EnableForThisOperator,
    ShotDelay,
    DelayBetweenRepeatedShots,
    Ms,
    SystemSettings,
    FeedbackSound,
    BeepWhenHotkeyStatesChange,
    Language,
    RecoilToggle,
    RapidFireToggle,
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
    AccentColor,
    UsedForSwitchesAndSelectedStates,
    ReactiveRadius,
    MouseResponseInTheBackground,
    ResetAppearance,
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
    ApplyColor,
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
    EnableTheGlobalControlThenEnableRapidFireForTheSelectedOperator,
    OpenTheSettingsGearToChangeTheAccentColorAndReactiveRadius,
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
    InterfaceStyle,
    InterfaceRestartHint,
    ApplyAndRestart,
    InterfaceSaveFailed,
};
inline constexpr int UiTextCount=116;

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

// Translate bundled Qt color/file dialogs without external translation packs.
class DesktopTranslator final : public QTranslator {
    std::function<int()> language_;
public:
    explicit DesktopTranslator(std::function<int()> language):language_(std::move(language)){}
    bool isEmpty() const override{return false;}
    QString translate(const char*,const char* source,const char* =nullptr,int =-1) const override{
        if(language_())return {};
        static const QHash<QString,QString> portuguese={
            {QString::fromUtf8("Hue:"),QString::fromUtf8("Matiz:")},
            {QString::fromUtf8("Sat:"),QString::fromUtf8("Saturação:")},
            {QString::fromUtf8("Val:"),QString::fromUtf8("Valor:")},
            {QString::fromUtf8("Red:"),QString::fromUtf8("Vermelho:")},
            {QString::fromUtf8("Green:"),QString::fromUtf8("Verde:")},
            {QString::fromUtf8("Blue:"),QString::fromUtf8("Azul:")},
            {QString::fromUtf8("Alpha channel:"),QString::fromUtf8("Canal alfa:")},
            {QString::fromUtf8("HTML:"),QString::fromUtf8("HTML:")},
            {QString::fromUtf8("Basic colors"),QString::fromUtf8("Cores básicas")},
            {QString::fromUtf8("Custom colors"),QString::fromUtf8("Cores personalizadas")},
            {QString::fromUtf8("Pick Screen Color"),QString::fromUtf8("Selecionar cor da tela")},
            {QString::fromUtf8("Add to Custom Colors"),QString::fromUtf8("Adicionar às cores personalizadas")},
            {QString::fromUtf8("Select Color"),QString::fromUtf8("Selecionar cor")},
            {QString::fromUtf8("Color"),QString::fromUtf8("Cor")},
            {QString::fromUtf8("Look in:"),QString::fromUtf8("Procurar em:")},
            {QString::fromUtf8("File name:"),QString::fromUtf8("Nome do arquivo:")},
            {QString::fromUtf8("Files of type:"),QString::fromUtf8("Tipo de arquivo:")},
            {QString::fromUtf8("Save in:"),QString::fromUtf8("Salvar em:")},
            {QString::fromUtf8("Open"),QString::fromUtf8("Abrir")},
            {QString::fromUtf8("Save"),QString::fromUtf8("Salvar")},
            {QString::fromUtf8("Cancel"),QString::fromUtf8("Cancelar")},
            {QString::fromUtf8("Close"),QString::fromUtf8("Fechar")},
            {QString::fromUtf8("Choose"),QString::fromUtf8("Escolher")},
            {QString::fromUtf8("All Files (*)"),QString::fromUtf8("Todos os arquivos (*)")},
            {QString::fromUtf8("File"),QString::fromUtf8("Arquivo")},
            {QString::fromUtf8("Name"),QString::fromUtf8("Nome")},
            {QString::fromUtf8("Size"),QString::fromUtf8("Tamanho")},
            {QString::fromUtf8("Type"),QString::fromUtf8("Tipo")},
            {QString::fromUtf8("Date Modified"),QString::fromUtf8("Data de modificação")},
            {QString::fromUtf8("New Folder"),QString::fromUtf8("Nova pasta")},
            {QString::fromUtf8("Back"),QString::fromUtf8("Voltar")},
            {QString::fromUtf8("Forward"),QString::fromUtf8("Avançar")},
            {QString::fromUtf8("Parent Directory"),QString::fromUtf8("Pasta superior")},
            {QString::fromUtf8("List View"),QString::fromUtf8("Lista")},
            {QString::fromUtf8("Detail View"),QString::fromUtf8("Detalhes")},
            {QString::fromUtf8("Show hidden files"),QString::fromUtf8("Mostrar arquivos ocultos")},
            {QString::fromUtf8("Directories"),QString::fromUtf8("Pastas")},
            {QString::fromUtf8("Computer"),QString::fromUtf8("Computador")},
            {QString::fromUtf8("Home"),QString::fromUtf8("Início")},
            {QString::fromUtf8("Recent Places"),QString::fromUtf8("Locais recentes")},
            {QString::fromUtf8("File not found.\nPlease verify the correct file name was given."),QString::fromUtf8("Arquivo não encontrado.\nVerifique o nome do arquivo.")},
            {QString::fromUtf8("%1 already exists.\nDo you want to replace it?"),QString::fromUtf8("%1 já existe.\nDeseja substituí-lo?")},
            {QString::fromUtf8("Confirm Save As"),QString::fromUtf8("Confirmar salvamento")},
            {QString::fromUtf8("Yes"),QString::fromUtf8("Sim")},
            {QString::fromUtf8("No"),QString::fromUtf8("Não")},
        };
        QString key=QString::fromUtf8(source);key.remove('&');return portuguese.value(key);
    }
};
