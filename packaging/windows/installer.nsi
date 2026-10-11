; UVC Studio — Windows installer (NSIS, Unicode)
Unicode true
Target amd64-unicode
!include "MUI2.nsh"
!ifndef VERSION
  !define VERSION "1.1.0"
!endif
!ifndef SRCDIR
  !define SRCDIR "UVCStudio"
!endif
Name "UVC Studio"
!ifndef VERFILE
  !define VERFILE "v${VERSION}"
!endif
!ifndef VERNUM
  !define VERNUM "1.0.0.0"
!endif
OutFile "UVCStudio-Setup-${VERFILE}.exe"
InstallDir "$PROGRAMFILES64\UVC Studio"
InstallDirRegKey HKLM "Software\UVC Studio" "InstallDir"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
!define MUI_ICON "uvc-studio.ico"
!define MUI_UNICON "uvc-studio.ico"
!define MUI_FINISHPAGE_RUN "$INSTDIR\UVCStudio.exe"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "TradChinese"
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "Korean"
VIProductVersion "${VERNUM}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "UVC Studio"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "UVC Studio installer"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${VERSION}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" "${VERSION}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "Personal build"

Section "Install"
  SetShellVarContext all
  SetOutPath "$INSTDIR"
  File "${SRCDIR}\UVCStudio.exe"
  SetOutPath "$INSTDIR\assets"
  File /r "${SRCDIR}\assets\*.*"
  SetOutPath "$INSTDIR"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  CreateShortCut "$SMPROGRAMS\UVC Studio.lnk" "$INSTDIR\UVCStudio.exe"
  CreateShortCut "$DESKTOP\UVC Studio.lnk" "$INSTDIR\UVCStudio.exe"
  WriteRegStr HKLM "Software\UVC Studio" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\UVCStudio" "DisplayName" "UVC Studio"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\UVCStudio" "DisplayVersion" "${VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\UVCStudio" "DisplayIcon" "$INSTDIR\UVCStudio.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\UVCStudio" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\UVCStudio" "InstallLocation" "$INSTDIR"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\UVCStudio" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\UVCStudio" "NoRepair" 1
SectionEnd

Section "Uninstall"
  SetShellVarContext all
  Delete "$SMPROGRAMS\UVC Studio.lnk"
  Delete "$DESKTOP\UVC Studio.lnk"
  RMDir /r "$INSTDIR\assets"
  Delete "$INSTDIR\UVCStudio.exe"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\UVCStudio"
  DeleteRegKey HKLM "Software\UVC Studio"
SectionEnd
