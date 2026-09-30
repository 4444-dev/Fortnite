Unicode true
RequestExecutionLevel user
SetCompressor /SOLID lzma

!include "MUI2.nsh"

!ifndef VERSION
!error "VERSION define is required"
!endif

!ifndef SOURCEDIR
!error "SOURCEDIR define is required"
!endif

!ifndef OUTDIR
!error "OUTDIR define is required"
!endif

!define PRODUCT_NAME "Luvkrimes"
!define PRODUCT_PUBLISHER "Luvkrimes"
!define PRODUCT_REGKEY "Software\Luvkrimes"
!define PRODUCT_UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Luvkrimes"

Name "${PRODUCT_NAME}"
OutFile "${OUTDIR}\Luvkrimes-Setup-${VERSION}.exe"
InstallDir "$LocalAppData\Programs\Luvkrimes"
InstallDirRegKey HKCU "${PRODUCT_REGKEY}" "InstallDir"

BrandingText "Luvkrimes ${VERSION}"
ShowInstDetails show
ShowUninstDetails show

VIAddVersionKey /LANG=1033 "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey /LANG=1033 "ProductVersion" "${VERSION}"
VIAddVersionKey /LANG=1033 "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey /LANG=1033 "FileDescription" "Luvkrimes Windows installer"
VIAddVersionKey /LANG=1033 "LegalCopyright" "Copyright ${PRODUCT_PUBLISHER}"

!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\Luvkrimes.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch Luvkrimes"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "French"

Section "Luvkrimes" SEC_MAIN
    SetShellVarContext current

    SetOutPath "$INSTDIR"
    File "${SOURCEDIR}\Luvkrimes.exe"
    File "${SOURCEDIR}\README.txt"
    File "${SOURCEDIR}\VERSION.txt"

    SetOutPath "$INSTDIR\projects\fortnite"
    File "${SOURCEDIR}\projects\fortnite\Luvkrimes-Fortnite.exe"

    WriteUninstaller "$INSTDIR\Uninstall.exe"

    WriteRegStr HKCU "${PRODUCT_REGKEY}" "InstallDir" "$INSTDIR"

    WriteRegStr HKCU "${PRODUCT_UNINSTALL_KEY}" "DisplayName" "${PRODUCT_NAME}"
    WriteRegStr HKCU "${PRODUCT_UNINSTALL_KEY}" "DisplayVersion" "${VERSION}"
    WriteRegStr HKCU "${PRODUCT_UNINSTALL_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegStr HKCU "${PRODUCT_UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKCU "${PRODUCT_UNINSTALL_KEY}" "DisplayIcon" "$INSTDIR\Luvkrimes.exe"
    WriteRegStr HKCU "${PRODUCT_UNINSTALL_KEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegDWORD HKCU "${PRODUCT_UNINSTALL_KEY}" "NoModify" 1
    WriteRegDWORD HKCU "${PRODUCT_UNINSTALL_KEY}" "NoRepair" 1

    CreateDirectory "$SMPROGRAMS\Luvkrimes"
    CreateShortcut "$SMPROGRAMS\Luvkrimes\Luvkrimes.lnk" "$INSTDIR\Luvkrimes.exe"
    CreateShortcut "$SMPROGRAMS\Luvkrimes\Uninstall Luvkrimes.lnk" "$INSTDIR\Uninstall.exe"
    CreateShortcut "$DESKTOP\Luvkrimes.lnk" "$INSTDIR\Luvkrimes.exe"
SectionEnd

Section "Uninstall"
    SetShellVarContext current

    Delete "$DESKTOP\Luvkrimes.lnk"
    Delete "$SMPROGRAMS\Luvkrimes\Luvkrimes.lnk"
    Delete "$SMPROGRAMS\Luvkrimes\Uninstall Luvkrimes.lnk"
    RMDir "$SMPROGRAMS\Luvkrimes"

    Delete "$INSTDIR\projects\fortnite\Luvkrimes-Fortnite.exe"
    RMDir "$INSTDIR\projects\fortnite"
    RMDir "$INSTDIR\projects"

    Delete "$INSTDIR\Luvkrimes.exe"
    Delete "$INSTDIR\README.txt"
    Delete "$INSTDIR\VERSION.txt"
    Delete "$INSTDIR\Uninstall.exe"
    RMDir "$INSTDIR"

    DeleteRegKey HKCU "${PRODUCT_UNINSTALL_KEY}"
    DeleteRegKey HKCU "${PRODUCT_REGKEY}"

    ; User settings, logs and remembered licenses live under
    ; %LOCALAPPDATA%\luvkrimes and are intentionally preserved.
SectionEnd
