; -- main.nsh --
; Created: JohnE, 2008-03-15


; DISCLAIMER:
; The author(s) of this file's contents release it into the public domain,
; without express or implied warranty of any kind. You may use, modify, and
; redistribute this file freely.


; Ephemeral settings
!searchparse /file SETUP-VERSION.txt "" SETUP_VER
!define SHORTNAME "TDM-GCC"
!define DEF_INST_DIR_32 "\TDM-GCC-32"
!define DEF_INST_DIR_64 "\TDM-GCC-64"
!define LOCAL_NET_MANIFEST "net-manifest.txt"
!define NET_MANIFEST_URL "https://jmeubank.github.io/tdm-gcc/net-manifest.txt"
;!define NET_MANIFEST_URL "http://localhost:4000/net-manifest.txt"
!define DEFAULT_BASE_URL "http://downloads.sourceforge.net/project/"
!define APPDATA_SUBFOLDER "TDM-GCC"
!define STARTMENU_ENTRY_32 "TDM-GCC-32"
!define STARTMENU_ENTRY_64 "TDM-GCC-64"
!define UNINSTKEY "TDM-GCC"
!define READMEFILE_32 "README-gcc-tdm.md"
!define READMEFILE_64 "README-gcc-tdm64.md"
!define INFOURL "http://tdm-gcc.tdragon.net/"
!define UPDATEURL "http://tdm-gcc.tdragon.net/"
!define PUBLISHER "TDM"


; Plugin settings
!addplugindir "plugins"
!addplugindir "${OUTPUT_DIR}"


!addincludedir ${OUTPUT_DIR}


!ifdef INNER_COMPONENTS
SetCompressor zlib
SetCompress off
!else
SetCompressor /SOLID lzma
SetCompress auto
!endif

; Basic attributes
Name "${SHORTNAME}"
XPStyle On
BrandingText "${SHORTNAME} Setup ${SETUP_VER}"
CompletedText "$comp_text"


; Includes
!include LogicLib.nsh
!include MUI2.nsh
!include FileFunc.nsh
!insertmacro GetRoot
!insertmacro Locate
!insertmacro GetParameters
!insertmacro un.GetParameters
!include StrFunc.nsh
;${StrRep}

!define MULTIUSER_EXECUTIONLEVEL Highest
!include MultiUser.nsh


;;; Globals
Var setup_type
Var stype_shortdesc
Var inst_dir
Var dlupdates
Var comp_text
Var inst_ftext
Var inst_fsubtext
Var working_manifest
Var num_prev_insts
Var man_msg
Var uninst
Var have_allusers_mode
Var system_id


;;; UI settings
!define MUI_ICON "tdm_icon_48.ico"
!define MUI_UNICON "tdm_icon_48.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "header1.bmp"
!define MUI_HEADERIMAGE_RIGHT
!define MUI_WELCOMEFINISHPAGE_BITMAP "wfpage1.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "wfpage1.bmp"
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_NOAUTOCLOSE


;;; Installer Pages
Page custom WizardAction_Create WizardAction_Leave

Page custom EditionSelect_Create EditionSelect_Leave

Page custom Tidbits_Create Tidbits_Leave

!define MUI_DIRECTORYPAGE_VARIABLE $inst_dir
!define MUI_PAGE_CUSTOMFUNCTION_SHOW InstDir_Show
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE InstDir_Leave
!insertmacro MUI_PAGE_DIRECTORY

Section
	${If} ${Silent}
		${If} "$INSTDIR" == ""
			${GetRoot} "$PROGRAMFILES" $0
			${If} "$system_id" == "tdm64"
				StrCpy $inst_dir "$0${DEF_INST_DIR_64}"
			${Else}
				StrCpy $inst_dir "$0${DEF_INST_DIR_32}"
			${EndIf}
		${Else}
			StrCpy $inst_dir "$INSTDIR"
		${EndIf}
		tdminstall::SetInstLocation /NOUNLOAD "$inst_dir"
	${EndIf}
SectionEnd

Page custom Components_Create

!define MUI_INSTFILESPAGE_FINISHHEADER_TEXT "$inst_ftext"
!define MUI_INSTFILESPAGE_FINISHHEADER_SUBTEXT "$inst_fsubtext"
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Read the TDM-GCC README file (recommended)"
!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchReadme"
!define MUI_PAGE_CUSTOMFUNCTION_SHOW "Finish_Show"
!insertmacro MUI_PAGE_FINISH


;;; Uninstaller pages
!define MUI_INSTFILESPAGE_FINISHHEADER_TEXT "$inst_ftext"
!define MUI_INSTFILESPAGE_FINISHHEADER_SUBTEXT "$inst_fsubtext"
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_UNPAGE_FINISH


;;; Languages
!insertmacro MUI_LANGUAGE "English"


;;; Reserved files
!insertmacro MUI_RESERVEFILE_LANGDLL ;Language selection dialog


;;; Sections
Section "Install Components" SEC_INSTALL_COMPONENTS
	Var /GLOBAL ar_dl_index

	RealProgress::SetProgress /NOUNLOAD 0

	SetOutPath "$inst_dir"
	CreateDirectory "$inst_dir\__installer"
	tdminstall::AddManMiscFile /NOUNLOAD "__installer/"

	; This executable
	${IfNot} ${FileExists} "$inst_dir\__installer\$EXEFILE"
		CopyFiles /SILENT "$EXEPATH" "$inst_dir\__installer"
		tdminstall::AddManMiscFile /NOUNLOAD "__installer/$EXEFILE"
	${EndIf}

	CreateDirectory "$inst_dir\__installer\downloaded"
	tdminstall::AddManMiscFile /NOUNLOAD "__installer/downloaded/"

	; Unpack any inner archives
!ifdef INNER_COMPONENTS
!system 'echo inner-manifest-${INNER_COMPONENTS_SYS}.txt>"${OUTPUT_DIR}/arcout.${INNER_COMPONENTS_SYS}.template.txt"'
!system 'echo System:id:${INNER_COMPONENTS_SYS}>>"${OUTPUT_DIR}/arcout.${INNER_COMPONENTS_SYS}.template.txt"'
!system 'echo Archive:path:.*/([^^?]+)>>"${OUTPUT_DIR}/arcout.${INNER_COMPONENTS_SYS}.template.txt"'
!system 'echo ^]^]^>^]^]^>>>"${OUTPUT_DIR}/arcout.${INNER_COMPONENTS_SYS}.template.txt"'
!system 'echo File "/oname=$PLUGINSDIR\@()@" "${ARCHIVES_DIR}\@()@">>"${OUTPUT_DIR}/arcout.${INNER_COMPONENTS_SYS}.template.txt"'
!system '${OUTPUT_DIR}/ctemplate.exe <"${OUTPUT_DIR}/arcout.${INNER_COMPONENTS_SYS}.template.txt" >${OUTPUT_DIR}/arcout.${INNER_COMPONENTS_SYS}.nsh' = 0
!include arcout.${INNER_COMPONENTS_SYS}.nsh
!endif

	; Download archives
	${If} $dlupdates == "yes"
		StrCpy $ar_dl_index 0
		GetFunctionAddress $0 "ArchiveCallbackForDownload"
		tdminstall::EnumArchives /NOUNLOAD $0
		Pop $0
		${If} $0 != "OK"
			DetailPrint "$0"
			Goto onerror
		${EndIf}
	${EndIf}

	; Install (and/or remove) archives
	GetFunctionAddress $0 "ItemBeforeActionCallback"
	tdminstall::RemoveAndAdd /NOUNLOAD \
	 "$inst_dir\__installer\downloaded|$EXEDIR\downloaded|$PLUGINSDIR" $0
	Pop $0
	${If} $0 != "OK"
		DetailPrint "$0"
		Goto onerror
	${EndIf}

	; MinGW environment vars batch file
	File "mingwvars.bat"
	tdminstall::AddManMiscFile /NOUNLOAD "mingwvars.bat"

	; Start menu shortcuts
	tdminstall::GetComponentSelected /NOUNLOAD "startmenu"
	Pop $0
	${If} $0 == 1
		${If} "$system_id" == "tdm64"
			StrCpy $1 "$SMPROGRAMS\${STARTMENU_ENTRY_64}"
		${Else}
			StrCpy $1 "$SMPROGRAMS\${STARTMENU_ENTRY_32}"
		${EndIf}
		CreateDirectory "$1"
		CreateShortCut "$1\MinGW Command Prompt.lnk" \
		 '%comspec%' '/k ""$inst_dir\mingwvars.bat""' \
		 "" "" "" "" "Open ${SHORTNAME} Command Prompt"
		CreateShortCut "$1\Modify or Remove MinGW.lnk" \
		 "$inst_dir\__installer\$EXEFILE"
	${EndIf}

	; PATH addition
	tdminstall::GetComponentSelected /NOUNLOAD "addpath"
	Pop $0
	${If} $0 == 1
		DetailPrint "Adding '$inst_dir\bin' to PATH"
		tdminstall::EnsureInPathEnv /NOUNLOAD "$inst_dir\bin" \
		 "$MultiUser.InstallMode"
		Pop $1
		${If} "$1" != "OK"
			DetailPrint "$1"
			Goto onerror
		${EndIf}
	${EndIf}

	Goto instend

onerror:
	StrCpy $inst_ftext "Installation Failed"
	StrCpy $inst_fsubtext "Setup was not completed successfully."
	StrCpy $comp_text "One or more errors occurred"

instend:
	; Write installation manifest
	DetailPrint "Writing installation manifest"
	tdminstall::AddManMiscFile /NOUNLOAD "__installer/installed_man.txt"
	tdminstall::WriteInstManifest /NOUNLOAD

	; Add installation to list
	DetailPrint "Updating list of ${SHORTNAME} installations"
	tdminstall::WriteInstList /NOUNLOAD "$APPDATA\${APPDATA_SUBFOLDER}" \
	 "installations.txt"

	; Add/Remove Programs entry
	WriteRegStr SHELL_CONTEXT \
	 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTKEY}" \
	 "DisplayName" "TDM-GCC"
	WriteRegStr SHELL_CONTEXT \
	 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTKEY}" \
	 "UninstallString" '"$inst_dir\__installer\$EXEFILE" /tdmu'
	WriteRegDWORD SHELL_CONTEXT \
	 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTKEY}" \
	 "NoRepair" 1
	WriteRegStr SHELL_CONTEXT \
	 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTKEY}" \
	 "ModifyPath" '"$inst_dir\__installer\$EXEFILE"'
	WriteRegStr SHELL_CONTEXT \
	 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTKEY}" \
	 "InstallLocation" "$inst_dir"
	WriteRegStr SHELL_CONTEXT \
	 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTKEY}" \
	 "Publisher" "${PUBLISHER}"
	WriteRegStr SHELL_CONTEXT \
	 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTKEY}" \
	 "HelpLink" "${INFOURL}"
	WriteRegStr SHELL_CONTEXT \
	 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTKEY}" \
	 "URLUpdateInfo" "${UPDATEURL}"
	WriteRegStr SHELL_CONTEXT \
	 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTKEY}" \
	 "URLInfoAbout" "${INFOURL}"
	WriteRegStr SHELL_CONTEXT \
	 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTKEY}" \
	 "DisplayVersion" "${SETUP_VER}"

	RealProgress::SetProgress /NOUNLOAD 100
SectionEnd


Section "un.Uninstall"
	RealProgress::SetProgress /NOUNLOAD 0

	StrCpy $comp_text "Completed successfully"
	StrCpy $inst_ftext "Uninstall Complete"
	StrCpy $inst_fsubtext "Uninstallation was completed successfully."

	${If} "$uninst" == ""
		DetailPrint "Error: Uninstaller called without location"
		Goto onerrorun
	${EndIf}

	UserInfo::GetName
	${IfNot} ${Errors}
		Pop $0
		tdminstall::CheckIfUserInstall /NOUNLOAD "$uninst" "$0"
		Pop $0
		${If} "$0" == 1
			Call un.MultiUser.InstallMode.CurrentUser
		${Else}
			Call un.MultiUser.InstallMode.AllUsers
		${EndIf}
		tdminstall::GetSystemID /NOUNLOAD "$uninst"
		Pop $system_id
		GetFunctionAddress $0 "un.ItemBeforeActionCallback"
		tdminstall::RemoveInst /NOUNLOAD "$uninst" \
		 "$APPDATA\${APPDATA_SUBFOLDER}\installations.txt" $0
		Pop $0
		${If} $0 != "OK"
			DetailPrint "$0"
			Goto onerrorun
		${EndIf}
		${If} "$system_id" == "tdm64"
			StrCpy $1 "${STARTMENU_ENTRY_64}"
		${Else}
			StrCpy $1 "${STARTMENU_ENTRY_32}"
		${EndIf}
		${If} ${FileExists} "$SMPROGRAMS\$1\MinGW Command Prompt.lnk"
			ShellLink::GetShortCutArgs "$SMPROGRAMS\$1\MinGW Command Prompt.lnk"
			Pop $0
			tdminstall::StringInString /NOUNLOAD "$uninst" "$0"
			Pop $0
			${If} "$0" == 1
				Delete "$SMPROGRAMS\$1\MinGW Command Prompt.lnk"
				Delete "$SMPROGRAMS\$1\Modify or Remove MinGW.lnk"
				RMDir "$SMPROGRAMS\$1"
				DetailPrint "Removed '$1' Start Menu entry"
			${EndIf}
		${EndIf}
		DetailPrint "Removing any instances of '$uninst\bin' from PATH"
		tdminstall::EnsureNotInPathEnv /NOUNLOAD "$uninst\bin"
		ReadRegStr $0 SHELL_CONTEXT \
		 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTKEY}" \
		 "UninstallString"
		tdminstall::StringInString /NOUNLOAD "$uninst" "$0"
		Pop $0
		${If} "$0" == 1
			DeleteRegKey SHELL_CONTEXT \
			 "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTKEY}"
		${EndIf}
		SetDetailsPrint none
		SetOutPath "$TEMP"
		SetDetailsPrint both
		RMDir "$uninst"
		${IfNot} ${FileExists} "$uninst"
			DetailPrint "Removed '$uninst'"
		${Else}
			DetailPrint "(Some files left in '$uninst')"
		${EndIf}
	${EndIf}

	Goto uninstend

onerrorun:
	StrCpy $inst_ftext "Uninstall Failed"
	StrCpy $inst_fsubtext "Uninstallation was not completed successfully."
	StrCpy $comp_text "One or more errors occurred"

uninstend:
	RealProgress::SetProgress /NOUNLOAD 100
SectionEnd


;;; Special functions
Function .onInit
	InitPluginsDir
	!insertmacro MULTIUSER_INIT
	${GetParameters} $0
	tdminstall::GetUninst /NOUNLOAD "$0"
	Pop $uninst
	${If} "$uninst" != "true"
		StrCpy $uninst ""
	${EndIf}
	StrCpy $setup_type "undefined"
	StrCpy $inst_dir ""
	${If} ${Silent}
		StrCpy $dlupdates "no"
	${Else}
		StrCpy $dlupdates "yes"
	${EndIf}
	StrCpy $comp_text "Completed successfully"
	StrCpy $inst_ftext "Installation Complete"
	StrCpy $inst_fsubtext "Setup was completed successfully."
	StrCpy $num_prev_insts 0
	StrCpy $man_msg ""
!ifdef INNER_COMPONENTS
	StrCpy $working_manifest "$PLUGINSDIR\${INNER_COMPONENTS}"
	StrCpy $system_id "${INNER_COMPONENTS_SYS}"
!else
	StrCpy $system_id "tdm32"
	StrCpy $working_manifest ""
!endif
	tdminstall::BeginInstFindBanner /NOUNLOAD
	tdminstall::UpdateFoundInsts /NOUNLOAD \
	 "$APPDATA\${APPDATA_SUBFOLDER}\installations.txt"
	Pop $num_prev_insts
	${If} "$MultiUser.InstallMode" == "AllUsers"
		StrCpy $have_allusers_mode 1
		Call MultiUser.InstallMode.CurrentUser
		tdminstall::UpdateFoundInsts /NOUNLOAD \
		 "$APPDATA\${APPDATA_SUBFOLDER}\installations.txt"
		Pop $num_prev_insts
	${Else}
		StrCpy $have_allusers_mode 0
	${EndIf}
	tdminstall::EndInstFindBanner /NOUNLOAD
!ifdef INNER_COMPONENTS
!system 'echo inner-manifest-${INNER_COMPONENTS_SYS}.txt>"${OUTPUT_DIR}/arcreg.${INNER_COMPONENTS_SYS}.template.txt"'
!system 'echo System:id:${INNER_COMPONENTS_SYS}>>"${OUTPUT_DIR}/arcreg.${INNER_COMPONENTS_SYS}.template.txt"'
!system 'echo Archive:path:.*/([^^?]+)>>"${OUTPUT_DIR}/arcreg.${INNER_COMPONENTS_SYS}.template.txt"'
!system 'echo ^]^]^>^]^]^>>>"${OUTPUT_DIR}/arcreg.${INNER_COMPONENTS_SYS}.template.txt"'
!system 'echo tdminstall::RegisterInnerArchive /NOUNLOAD "@()@">>"${OUTPUT_DIR}/arcreg.${INNER_COMPONENTS_SYS}.template.txt"'
!system '${OUTPUT_DIR}/ctemplate.exe <"${OUTPUT_DIR}/arcreg.${INNER_COMPONENTS_SYS}.template.txt" >${OUTPUT_DIR}/arcreg.${INNER_COMPONENTS_SYS}.nsh' = 0
!include arcreg.${INNER_COMPONENTS_SYS}.nsh
!endif
FunctionEnd

Function un.onInit
	InitPluginsDir
	!insertmacro MULTIUSER_UNINIT
	${un.GetParameters} $0
	tdminstall::GetUninst /NOUNLOAD "$0"
	Pop $uninst
	StrCpy $comp_text "Completed successfully"
	StrCpy $inst_ftext "Uninstall Complete"
	StrCpy $inst_fsubtext "Uninstallation was completed successfully."
FunctionEnd

Function .onGUIEnd
	tdminstall::Unload
	RealProgress::Unload
FunctionEnd

Function un.onGUIEnd
	tdminstall::Unload
	RealProgress::Unload
FunctionEnd

Function .onVerifyInstDir
	${If} "$setup_type" != "create"
		${IfNot} ${FileExists} "$inst_dir\__installer\installed_man.txt"
			${If} $man_msg != ""
				SendMessage $man_msg ${WM_SETTEXT} 0 \
				 'STR:(No installation manifest present in this directory)'
			${EndIf}
			Abort
		${Else}
			${If} $man_msg != ""
				SendMessage $man_msg ${WM_SETTEXT} 0 'STR:'
			${EndIf}
		${EndIf}
	${EndIf}
FunctionEnd


;;; Utility functions
Function ArchiveCallbackForDownload
	Var /GLOBAL apc_url
	Var /GLOBAL apc_file
	Pop $apc_url
	Pop $apc_file
	Push $0

	; Check if already available
	${If} ${FileExists} "$EXEDIR\downloaded\$apc_file"
		DetailPrint "Using local archive '$EXEDIR\downloaded\$apc_file'"
		Push "OK"
		Return
	${ElseIf} ${FileExists} "$inst_dir\__installer\downloaded\$apc_file"
		DetailPrint "Using local archive '$inst_dir\__installer\downloaded\$apc_file'"
		Push "OK"
		Return
	${ElseIf} ${FileExists} "$PLUGINSDIR\$apc_file"
		DetailPrint "Using inner archive '$apc_file'"
		Push "OK"
		Return
	${EndIf}

dlfile:
	;${StrRep} $0 "$apc_url" "+" "%252B"
	StrCpy $1 "$apc_url" 4
	${If} "$1" == "http"
		StrCpy $1 "$apc_url"
	${Else}
		StrCpy $1 "${DEFAULT_BASE_URL}$apc_url"
	${EndIf}
	DetailPrint "Downloading '$1'"
	tdminstall::GetDownloadProgress /NOUNLOAD $ar_dl_index ; Pushes...
	RealProgress::SetProgress /NOUNLOAD ; ...then pops.
	Push "$1"
	Push "$apc_file"
	Call DownloadArchive
	Pop $0
	${If} $0 != "OK"
		MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION|MB_DEFBUTTON1 \
		 "Couldn't download \
		 '$1':$\r$\n$0$\r$\n$\r$\nWould you like to try again?" \
		 IDRETRY dlfile
		DetailPrint "$0"
		StrCpy $0 "Couldn't download '$1'"
		Goto errorend
	${EndIf}
	IntOp $ar_dl_index $ar_dl_index + 1
	tdminstall::AddManMiscFile /NOUNLOAD "__installer/downloaded/$apc_file"

errorend:
	Exch $0
FunctionEnd

Function ItemBeforeActionCallback
	Var /GLOBAL ibec_path
	Var /GLOBAL ibec_isdir
	Var /GLOBAL ibec_isdel
	Var /GLOBAL ibec_isarchive
	Pop $ibec_path
	Pop $ibec_isdir
	Pop $ibec_isdel
	Pop $ibec_isarchive

	${If} "$ibec_isdel" == 1
		DetailPrint "Removing '$ibec_path'"
	${ElseIf} "$ibec_isarchive" == 1
        DetailPrint "Unpacking '$ibec_path'"
	${ElseIf} "$ibec_isdir" == 1
		DetailPrint "Creating '$ibec_path'"
	${Else}
		DetailPrint "Writing '$ibec_path'"
	${EndIf}

	tdminstall::GetInstallProgress /NOUNLOAD ; Pushes...
	RealProgress::SetProgress /NOUNLOAD ; ...then pops.
FunctionEnd

Function un.ItemBeforeActionCallback
	Var /GLOBAL unibec_path
	Var /GLOBAL unibec_isdir
	Var /GLOBAL unibec_isdel
	Pop $unibec_path
	Pop $unibec_isdir
	Pop $unibec_isdel

	${If} "$unibec_isdel" == 1
		DetailPrint "Removing '$unibec_path'"
	${Else}
		${If} "$unibec_isdir" == 1
			DetailPrint "Creating '$unibec_path'"
		${Else}
			DetailPrint "Extracting '$unibec_path'"
		${EndIf}
	${EndIf}

	tdminstall::GetInstallProgress /NOUNLOAD ; Pushes...
	RealProgress::SetProgress /NOUNLOAD ; ...then pops.
FunctionEnd

Function DownloadArchive
	Var /GLOBAL da_file
	Var /GLOBAL da_url
	Pop $da_file
	Pop $da_url
	INetC::get \
	 /CAPTION "Download '$da_file'" \
	 /QUESTION "Are you sure you want to cancel the download?" \
	 /USERAGENT "curl/7" \
	 "$da_url" "$inst_dir\__installer\downloaded\$da_file" \
	 /END
	; Leave result on stack for caller to use
FunctionEnd


;;; Page functions
Function WizardAction_Create
	${If} "$uninst" == "true"
		StrCpy $setup_type "remove"
		StrCpy $stype_shortdesc "Uninstall"
		tdminstall::GetFirstPrevInst /NOUNLOAD
		Pop $inst_dir
		${If} "$inst_dir" != ""
			tdminstall::SetInstLocation /NOUNLOAD "$inst_dir"
		${EndIf}
		Abort
	${EndIf}

	${If} ${Silent}
		StrCpy $setup_type "create"
		StrCpy $stype_shortdesc "New Installation"
		Abort
	${EndIf}

	!insertmacro MUI_HEADER_TEXT "Wizard Action" "Choose which action you want \
	  the setup wizard to perform."

	nsDialogs::Create /NOUNLOAD 1018
	Pop $2

	IntOp $0 $(^FontSize) + 2
	CreateFont $0 "$(^Font)" $0 "700"

	; Action buttons
	${NSD_CreateButton} 0u 0u 150u 17u "Create"
	Pop $1
	SendMessage $1 ${WM_SETFONT} $0 1
	${NSD_OnClick} $1 WizardAction_OnClickCreate
	${NSD_CreateLabel} 15u 19u 150u 20u \
	 ": Create a new ${SHORTNAME} installation"
	Pop $1
	${NSD_CreateButton} 0u 41u 150u 17u "Manage"
	Pop $1
	SendMessage $1 ${WM_SETFONT} $0 1
	${NSD_OnClick} $1 WizardAction_OnClickManage
	${NSD_CreateLabel} 15u 60u 150u 20u \
	 ": Manage an existing ${SHORTNAME} installation"
	Pop $1
	${NSD_CreateButton} 0u 82u 150u 17u "Remove"
	Pop $1
	SendMessage $1 ${WM_SETFONT} $0 1
	${NSD_OnClick} $1 WizardAction_OnClickRemove
	${NSD_CreateLabel} 15u 101u 150u 20u \
	 ": Remove a ${SHORTNAME} installation"
	Pop $1

	; Previous installations
	${If} "$num_prev_insts" > 0
		tdminstall::ReplaceDirProc /NOUNLOAD $2
		${NSD_CreateGroupBox} 170u 0u 129u 126u "Previous Installations"
		Pop $1
		GetFunctionAddress $1 "WizardAction_OnInstSel"
		tdminstall::CreateInstDirPrevList /NOUNLOAD $2 1 $1
	${EndIf}

	; Download updates checkbox
!ifdef INNER_COMPONENTS
	Var /GLOBAL WizardAction.DLUpdates
	${NSD_CreateCheckBox} 0u 128u 290u 12u \
	 "Check for updated files on the ${SHORTNAME} server"
	Pop $WizardAction.DLUpdates
	${If} $dlupdates == "yes"
		SendMessage $WizardAction.DLUpdates ${BM_SETCHECK} ${BST_CHECKED} 0
	${Else}
		SendMessage $WizardAction.DLUpdates ${BM_SETCHECK} ${BST_UNCHECKED} 0
	${EndIf}
!endif

	; Hide Next button
	GetDlgItem $0 $HWNDPARENT 1
	EnableWindow $0 0
	ShowWindow $0 ${SW_HIDE}

	nsDialogs::Show
FunctionEnd

Function WizardAction_Leave
	; Set $inst_dir
	${If} "$setup_type" != "create"
	${AndIf} "$inst_dir" == ""
		tdminstall::GetFirstPrevInst /NOUNLOAD
		Pop $inst_dir
	${EndIf}
	${If} "$inst_dir" != ""
		tdminstall::SetInstLocation /NOUNLOAD "$inst_dir"
	${EndIf}

	; Check Download updates checkbox state
!ifdef INNER_COMPONENTS
	${NSD_GetState} $WizardAction.DLUpdates $0
	${If} $0 == ${BST_CHECKED}
		StrCpy $dlupdates "yes"
	${Else}
		StrCpy $dlupdates "no"
	${EndIf}
!endif

	; Download updated manifest
	${If} "$setup_type" != "remove"
		${If} $dlupdates == "yes"
			${IfNot} ${FileExists} "$PLUGINSDIR\${LOCAL_NET_MANIFEST}"
				StrCpy $0 "${NET_MANIFEST_URL}" 4
				${If} "$0" == "http"
					Push /END
					Push "$PLUGINSDIR\${LOCAL_NET_MANIFEST}"
					Push "${NET_MANIFEST_URL}"
					Push "Are you sure you want to cancel the download?"
					Push /QUESTION
					Push "Downloading '${NET_MANIFEST_URL}'"
					Push /BANNER
					Push \
					"An error occurred while downloading '${NET_MANIFEST_URL}'. Would \
					you like to try again?"
					Push /RESUME
					Push "Download updated manifest"
					Push /CAPTION
					INetC::get
					Pop $0
					${If} "$0" != "OK"
						MessageBox MB_OK|MB_ICONEXCLAMATION \
						"An updated manifest file could not be \
						downloaded:$\r$\n$0$\r$\n(${NET_MANIFEST_URL})"
						Abort
					${EndIf}
				${Else}
					CopyFiles /SILENT "${NET_MANIFEST_URL}" "$PLUGINSDIR\${LOCAL_NET_MANIFEST}"
				${EndIf}
			${EndIf}
			StrCpy $working_manifest "$PLUGINSDIR\${LOCAL_NET_MANIFEST}"
			tdminstall::SetManifest /NOUNLOAD "$working_manifest" $HWNDPARENT
			Pop $0
			${If} "$0" == "update"
				Abort
			${ElseIf} "$0" != "OK"
				MessageBox MB_OK|MB_ICONEXCLAMATION \
				 "The downloaded manifest file could not be loaded:$\r$\n$0"
				Delete "$working_manifest"
				Abort
			${EndIf}
!ifdef INNER_COMPONENTS
		${Else}
			${IfNot} ${FileExists} "$PLUGINSDIR\${INNER_COMPONENTS}"
				File "/oname=$PLUGINSDIR\${INNER_COMPONENTS}" \
				 "${INNER_COMPONENTS}"
			${EndIf}
			StrCpy $working_manifest "$PLUGINSDIR\${INNER_COMPONENTS}"
			tdminstall::SetManifest /NOUNLOAD "$working_manifest" $HWNDPARENT
			Pop $0
			${If} "$0" != "OK"
				MessageBox MB_OK|MB_ICONEXCLAMATION \
				 "The inner manifest file could not be loaded."
				Abort
			${EndIf}
!endif
		${EndIf}
	${EndIf}
FunctionEnd

Function WizardAction_OnClickCreate
	Pop $0
	StrCpy $setup_type "create"
	StrCpy $stype_shortdesc "New Installation"
	Call WizardActionNext
FunctionEnd

Function WizardAction_OnClickManage
	Pop $0
	StrCpy $setup_type "manage"
	StrCpy $stype_shortdesc "Existing Installation"
	Call WizardActionNext
FunctionEnd

Function WizardAction_OnClickRemove
	Pop $0
	StrCpy $setup_type "remove"
	StrCpy $stype_shortdesc "Uninstall"
	Call WizardActionNext
FunctionEnd

Function WizardActionNext
	; Show Next button
	GetDlgItem $0 $HWNDPARENT 1
	EnableWindow $0 1
	ShowWindow $0 ${SW_SHOW}
	; "Click" Next button
	SendMessage $0 ${BM_CLICK} 0 0
FunctionEnd

Function WizardAction_OnInstSel
	Pop $inst_dir
	Push 0
	Call WizardAction_OnClickManage
FunctionEnd


Function EditionSelect_Create
	${If} "$setup_type" == "manage"
	${OrIf} "$setup_type" == "remove"
!ifdef INNER_COMPONENTS
	${OrIf} "$dlupdates" != "yes"
		StrCpy $system_id "${INNER_COMPONENTS_SYS}"
		${GetRoot} "$PROGRAMFILES" $0
		${If} "$system_id" == "tdm64"
			StrCpy $0 "$0${DEF_INST_DIR_64}"
		${Else}
			StrCpy $0 "$0${DEF_INST_DIR_32}"
		${EndIf}
		tdminstall::IsPrevInst /NOUNLOAD "$0"
		Pop $1
		${If} "$1" == 1
			StrCpy $inst_dir ""
		${Else}
			StrCpy $inst_dir "$0"
		${EndIf}
!endif
		Abort
	${EndIf}

	!insertmacro MUI_HEADER_TEXT "Select Edition" \
	 "Choose which edition of TDM-GCC you want to install."

	nsDialogs::Create /NOUNLOAD 1018
	Pop $2

	IntOp $0 $(^FontSize) + 2
	CreateFont $0 "$(^Font)" $0 "700"

	; Edition radio buttons
	${NSD_CreateRadioButton} 0u 30u 250u 10u "MinGW/TDM (32-bit)"
	Pop $1
	SendMessage $1 ${WM_SETFONT} $0 1
	${If} "$system_id" == "tdm32"
		${NSD_Check} $1
	${EndIf}
	${NSD_OnClick} $1 EditionSelect_OnClick32
	${NSD_CreateLabel} 15u 42u 190u 20u \
	 "Create a MinGW-based installation"
	Pop $1
	${NSD_CreateRadioButton} 0u 71u 250u 10u "MinGW-w64/TDM64 (32-bit and 64-bit)"
	Pop $1
	SendMessage $1 ${WM_SETFONT} $0 1
	${If} "$system_id" == "tdm64"
		${NSD_Check} $1
	${EndIf}
	${NSD_OnClick} $1 EditionSelect_OnClick64
	${NSD_CreateLabel} 15u 83u 190u 20u \
	 "Create a MinGW-w64-based installation"
	Pop $1

	nsDialogs::Show
FunctionEnd

Function EditionSelect_Leave
	${GetRoot} "$PROGRAMFILES" $0
	${If} "$system_id" == "tdm64"
		StrCpy $0 "$0${DEF_INST_DIR_64}"
	${Else}
		StrCpy $0 "$0${DEF_INST_DIR_32}"
	${EndIf}
	tdminstall::IsPrevInst /NOUNLOAD "$0"
	Pop $1
	${If} "$1" == 1
		StrCpy $inst_dir ""
	${Else}
		StrCpy $inst_dir "$0"
	${EndIf}
FunctionEnd

Function EditionSelect_OnClick32
	StrCpy $system_id "tdm32"
FunctionEnd

Function EditionSelect_OnClick64
	StrCpy $system_id "tdm64"
FunctionEnd


Function Tidbits_Create
	${If} "$setup_type" == "manage"
	${OrIf} "$setup_type" == "remove"
		Abort
	${EndIf}

	tdminstall::GetTidbits /NOUNLOAD "$system_id" "$PLUGINSDIR"
	Pop $3
	Pop $4
	Pop $5
	${If} "$5" == ""
		Abort
	${EndIf}

	!insertmacro MUI_HEADER_TEXT "$5" "$4"

	nsDialogs::Create /NOUNLOAD 1018
	Pop $0

	nsDialogs::CreateControl /NOUNLOAD RichEdit20A \
	 ${WS_VISIBLE}|${WS_CHILD}|${WS_TABSTOP}|${WS_VSCROLL}|${ES_MULTILINE}|${ES_WANTRETURN} \
	 ${__NSD_Text_EXSTYLE} 0 0 100% -24u ""
	Pop $0
	nsRichEdit::Load $0 "$3"

	nsDialogs::Show
FunctionEnd

Function Tidbits_Leave
FunctionEnd


Function InstDir_Show
	!insertmacro MUI_HEADER_TEXT "$stype_shortdesc: Installation Directory" \
	  "Choose the installation directory to use."
	tdminstall::ReplaceDirProc /NOUNLOAD $mui.DirectoryPage
	ShowWindow $mui.DirectoryPage.SpaceRequired ${SW_HIDE}
	SendMessage $mui.DirectoryPage.DirectoryBox ${WM_SETTEXT} 0 'STR:Installation Directory'
	${If} $setup_type == "manage"
		SendMessage $mui.DirectoryPage.Text ${WM_SETTEXT} 0 \
		 "STR:Setup will operate on the installation of ${SHORTNAME} in the \
		  following folder. To use a different installation, click Browse and \
		  select its location. $_CLICK"
	${ElseIf} $setup_type == "remove"
		SendMessage $mui.DirectoryPage.Text ${WM_SETTEXT} 0 \
		 "STR:Setup will remove the installation of ${SHORTNAME} in the \
		  following folder. To remove a different installation, click Browse \
		  and select its location. Click Uninstall to begin removal."
		ShowWindow $mui.DirectoryPage.SpaceAvailable ${SW_HIDE}
		tdminstall::CreateInstDirUninstNote /NOUNLOAD $mui.DirectoryPage
		Pop $0
		GetDlgItem $0 $HWNDPARENT 1
		SendMessage $0 ${WM_SETTEXT} 0 'STR:Uninstall'
	${EndIf}
	${If} "$setup_type" == "manage"
	${OrIf} "$setup_type" == "remove"
		tdminstall::CreateInstDirManMsg /NOUNLOAD $mui.DirectoryPage
		Pop $man_msg
		${If} "$num_prev_insts" > 0
			GetFunctionAddress $0 "InstDir_OnInstSel"
			tdminstall::CreateInstDirPrevList /NOUNLOAD $mui.DirectoryPage 2 $0
		${EndIf}
	${EndIf}
FunctionEnd

Function InstDir_Leave
	Var /GLOBAL idl_empty

	StrLen $0 "$inst_dir"
	IntOp $0 $0 - 1
	StrCpy $1 "$inst_dir" 1 $0
	${If} "$1" == "\"
	${OrIf} "$1" == "/"
		StrCpy $inst_dir "$inst_dir" $0
	${EndIf}

	${If} "$setup_type" == "remove"
		GetFunctionAddress $0 "WriteUninstCallback"
		tdminstall::RunUninstall /NOUNLOAD "$inst_dir" "$TEMP" "$EXEPATH" \
		 "$EXEFILE" $0
		Pop $0
		${If} "$0" != "OK"
			MessageBox MB_OK|MB_ICONEXCLAMATION "$0"
		${EndIf}
		Quit
	${EndIf}

	StrCpy $0 1
	UserInfo::GetName
	${IfNot} ${Errors}
		Pop $0
		tdminstall::CheckIfUserInstall /NOUNLOAD "$inst_dir" "$0"
		Pop $0
	${EndIf}
	${If} "$0" == 1
		Call MultiUser.InstallMode.CurrentUser
	${Else}
		Call MultiUser.InstallMode.AllUsers
	${EndIf}

	${If} "$setup_type" == "create"
		StrCpy $idl_empty "true"
		${Locate} "$inst_dir" "/L=FD /G=0" "InstDir_NotEmpty"
		${If} $idl_empty != "true"
			MessageBox MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2 \
			 "The directory '$inst_dir' is not empty!$\r$\nAre you sure you \
			  want to install here?" IDYES noabort
			Abort
noabort:
		${EndIf}
		tdminstall::SetPrevInstMan /NOUNLOAD ""
		Pop $0
	${Else}
		tdminstall::SetPrevInstMan /NOUNLOAD "$inst_dir\__installer\installed_man.txt"
		Pop $0
		${If} "$0" != 1
			MessageBox MB_OK|MB_ICONEXCLAMATION \
			 "The installation manifest file for '$inst_dir' could not be \
			  loaded."
			Abort
		${EndIf}
		tdminstall::GetSystemID /NOUNLOAD "$inst_dir"
		Pop $system_id
	${EndIf}
	tdminstall::SetInstLocation /NOUNLOAD "$inst_dir"
FunctionEnd

Function InstDir_NotEmpty
	StrCpy $idl_empty "false"
	Push "StopLocate"
FunctionEnd

Function InstDir_OnInstSel
	Exch $0

	; SendMessage $mui.DirectoryPage.Directory ${WM_SETTEXT} 0 'STR:$0'

	; "Click" Next button
	GetDlgItem $0 $HWNDPARENT 1
	SendMessage $0 ${BM_CLICK} 0 0

	Pop $0
FunctionEnd

Function WriteUninstCallback
	Exch $0
	WriteUninstaller "$0"
	Pop $0
FunctionEnd


Function Components_Create
	${If} ${Silent}
		Abort
	${EndIf}

	!insertmacro MUI_HEADER_TEXT "$stype_shortdesc: Choose Components" \
	 "Choose which features of ${SHORTNAME} you want installed."

	nsDialogs::Create /NOUNLOAD 1018
	Pop $0

	${NSD_CreateLabel} 0u 0u 300u 18u \
	 "Check the components you want installed and uncheck the components you \
	 don't want installed. $_CLICK"
	Pop $1

	${NSD_CreateLabel} 0u 23u 90u 9u \
	 "Select the type of install:"
	Pop $1

	${NSD_CreateDropList} 92u 21u 206u 75u ""
	Pop $1
	${If} "$setup_type" == "manage"
		tdminstall::PopulateInstallTypeList /NOUNLOAD "$system_id" $1 1
	${Else}
		tdminstall::PopulateInstallTypeList /NOUNLOAD "$system_id" $1 0
	${EndIf}

	${NSD_CreateLabel} 0u 38u 300u 9u \
	 "Or, select the optional components you wish to have installed:"
	Pop $1

	tdminstall::CreateComponentsTree /NOUNLOAD $0 "$system_id"

	${NSD_CreateGroupBox} 210u 45u 89u 94u "Description"
	Pop $1

	${NSD_CreateLabel} 215u 55u 79u 79u ""
	Pop $1
	tdminstall::SetDescLabel /NOUNLOAD $1

	${NSD_CreateLabel} 0u 128u 200u 12u ""
	Pop $1
	tdminstall::SetSpaceReqLabel /NOUNLOAD $1

	nsDialogs::Show
FunctionEnd


Function Finish_Show
	${IfNot} ${FileExists} "$inst_dir\${READMEFILE_32}"
	${AndIfNot} ${FileExists} "$inst_dir\${READMEFILE_64}"
		SendMessage $mui.FinishPage.Run ${BM_SETCHECK} 0 ${BST_UNCHECKED}
		ShowWindow $mui.FinishPage.Run ${SW_HIDE}
	${EndIf}
FunctionEnd

Function LaunchReadme
	${If} "$system_id" == "tdm64"
		Push "$inst_dir\${READMEFILE_64}"
		Call ConvertUnixNewLines
		Exec 'notepad.exe "$inst_dir\${READMEFILE_64}"'
	${Else}
		Push "$inst_dir\${READMEFILE_32}"
		Call ConvertUnixNewLines
		Exec 'notepad.exe "$inst_dir\${READMEFILE_32}"'
	${EndIf}
FunctionEnd

Function ConvertUnixNewLines
Exch $R0 ;file #1 path
Push $R1 ;file #1 handle
Push $R2 ;file #2 path
Push $R3 ;file #2 handle
Push $R4 ;data
Push $R5

 FileOpen $R1 $R0 r
 GetTempFileName $R2
 FileOpen $R3 $R2 w

 loopRead:
  ClearErrors
  FileRead $R1 $R4
  IfErrors doneRead

   StrCpy $R5 $R4 1 -1
   StrCmp $R5 $\n 0 +4
   StrCpy $R5 $R4 1 -2
   StrCmp $R5 $\r +3
   StrCpy $R4 $R4 -1
   StrCpy $R4 "$R4$\r$\n"

  FileWrite $R3 $R4

 Goto loopRead
 doneRead:

 FileClose $R3
 FileClose $R1

 SetDetailsPrint none
 Delete $R0
 Rename $R2 $R0
 SetDetailsPrint both

Pop $R5
Pop $R4
Pop $R3
Pop $R2
Pop $R1
Pop $R0
FunctionEnd

; end main.nsh
