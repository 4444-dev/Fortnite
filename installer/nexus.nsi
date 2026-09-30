Unicode true
RequestExecutionLevel user
SetCompressor /SOLID lzma

!include "MUI2.nsh"

!ifndef VERSION
!error "VERSION define is required"
!endif

!ifndef NUMERICVERSION
!error "NUMERICVERSION define is required"
!endif

!ifndef SOURCEDIR
!error "SOURCEDIR define is required"
!endif

!ifndef OUTDIR
!error "OUTDIR define is required"
!endif

!define PRODUCT_NAME "Nexus"
!define PRODUCT_PUBLISHER "Nexus"
!define PRODUCT_REGKEY "Software\Nexus"
!define PRODUCT_UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Nexus"

Name "${PRODUCT_NAME}"
OutFile "${OUTDIR}\Nexus-Setup-${VERSION}.exe"
InstallDir "$LocalAppData\Programs\Nexus"
InstallDirRegKey HKCU "${PRODUCT_REGKEY}" "InstallDir"

BrandingText "Nexus ${VERSION}"
ShowInstDetails show
ShowUninstDetails show

VIProductVersion "${NUMERICVERSION}"
VIAddVersionKey /LANG=1033 "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey /LANG=1033 "ProductVersion" "${VERSION}"
VIAddVersionKey /LANG=1033 "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey /LANG=1033 "FileDescription" "Nexus Windows installer"
VIAddVersionKey /LANG=1033 "LegalCopyright" "Copyright ${PRODUCT_PUBLISHER}"

!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\Nexus.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch Nexus"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "French"

Section "Nexus" SEC_MAIN
    SetShellVarContext current

    SetOutPath "$INSTDIR"
    File "${SOURCEDIR}\Nexus.exe"
    File "${SOURCEDIR}\README.txt"
    File "${SOURCEDIR}\VERSION.txt"

    SetOutPath "$INSTDIR\projects\fortnite"
    File "${SOURCEDIR}\projects\fortnite\Nexus-Fortnite.exe"

    WriteUninstaller "$INSTDIR\Uninstall.exe"

    WriteRegStr HKCU "${PRODUCT_REGKEY}" "InstallDir" "$INSTDIR"

    WriteRegStr HKCU "${PRODUCT_UNINSTALL_KEY}" "DisplayName" "${PRODUCT_NAME}"
    WriteRegStr HKCU "${PRODUCT_UNINSTALL_KEY}" "DisplayVersion" "${VERSION}"
    WriteRegStr HKCU "${PRODUCT_UNINSTALL_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegStr HKCU "${PRODUCT_UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKCU "${PRODUCT_UNINSTALL_KEY}" "DisplayIcon" "$INSTDIR\Nexus.exe"
    WriteRegStr HKCU "${PRODUCT_UNINSTALL_KEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegDWORD HKCU "${PRODUCT_UNINSTALL_KEY}" "NoModify" 1
    WriteRegDWORD HKCU "${PRODUCT_UNINSTALL_KEY}" "NoRepair" 1

    CreateDirectory "$SMPROGRAMS\Nexus"
    CreateShortcut "$SMPROGRAMS\Nexus\Nexus.lnk" "$INSTDIR\Nexus.exe"
    CreateShortcut "$SMPROGRAMS\Nexus\Uninstall Nexus.lnk" "$INSTDIR\Uninstall.exe"
    CreateShortcut "$DESKTOP\Nexus.lnk" "$INSTDIR\Nexus.exe"
SectionEnd

Section "Uninstall"
    SetShellVarContext current

    Delete "$DESKTOP\Nexus.lnk"
    Delete "$SMPROGRAMS\Nexus\Nexus.lnk"
    Delete "$SMPROGRAMS\Nexus\Uninstall Nexus.lnk"
    RMDir "$SMPROGRAMS\Nexus"

    Delete "$INSTDIR\projects\fortnite\Nexus-Fortnite.exe"
    RMDir "$INSTDIR\projects\fortnite"
    RMDir "$INSTDIR\projects"

    Delete "$INSTDIR\Nexus.exe"
    Delete "$INSTDIR\README.txt"
    Delete "$INSTDIR\VERSION.txt"
    Delete "$INSTDIR\Uninstall.exe"
    RMDir "$INSTDIR"

    DeleteRegKey HKCU "${PRODUCT_UNINSTALL_KEY}"
    DeleteRegKey HKCU "${PRODUCT_REGKEY}"

    ; User settings, logs and remembered licenses live under
    ; %LOCALAPPDATA%\Nexus and are intentionally preserved.
SectionEnd
