// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include "Common/CommonPaths.h"
#include "Common/CommonTypes.h"
#include "Common/FileUtil.h"
#include "Common/IniFile.h"
#include "Core/ConfigManager.h"
#include "Core/HW/SI.h"
#include "DiscIO/NANDContentLoader.h"

SConfig* SConfig::m_Instance;

static const struct
{
	const char* IniText;
	const bool  KBM;
	const int   DefaultKey;
	const int   DefaultModifier;
	const u32   XInputMapping;
} g_HKData[] = {
	{ "Open",                 true, 79 /* 'O' */,        2 /* wxMOD_CONTROL */, 0 },
	{ "ChangeDisc",           true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "RefreshList",          true, 0,                   0 /* wxMOD_NONE */,    0 },
#ifdef __APPLE__
	{ "PlayPause",            true, 80 /* 'P' */,        2 /* wxMOD_CMD */,     0 },
	{ "Stop",                 true, 87 /* 'W' */,        2 /* wxMOD_CMD */,     0 },
	{ "Reset",                true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "FrameAdvance",         true, 0,                   0 /* wxMOD_NONE */,    0 },

	{ "StartRecording",       true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "PlayRecording",        true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "ExportRecording",      true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "Readonlymode",         true, 0,                   0 /* wxMOD_NONE */,    0 },

	{ "ToggleFullscreen",     true, 70 /* 'F' */,        2 /* wxMOD_CMD */,     0 },
	{ "Screenshot",           true, 83 /* 'S' */,        2 /* wxMOD_CMD */,     0 },
	{ "Exit",                 true, 0,                   0 /* wxMOD_NONE */,    0 },

	{ "Wiimote1Connect",      true, 49 /* '1' */,        2 /* wxMOD_CMD */,     0 },
	{ "Wiimote2Connect",      true, 50 /* '2' */,        2 /* wxMOD_CMD */,     0 },
	{ "Wiimote3Connect",      true, 51 /* '3' */,        2 /* wxMOD_CMD */,     0 },
	{ "Wiimote4Connect",      true, 52 /* '4' */,        2 /* wxMOD_CMD */,     0 },
	{ "BalanceBoardConnect",  true, 53 /* '4' */,        2 /* wxMOD_CMD */,     0 },
#else
	{ "PlayPause",            true, 349 /* WXK_F10 */,   0 /* wxMOD_NONE */,    0 },
	{ "Stop",                 true, 27 /* WXK_ESCAPE */, 0 /* wxMOD_NONE */,    0 },
	{ "Reset",                true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "FrameAdvance",         true, 0,                   0 /* wxMOD_NONE */,    0 },

	{ "StartRecording",       true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "PlayRecording",        true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "ExportRecording",      true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "Readonlymode",         true, 0,                   0 /* wxMOD_NONE */,    0 },

	{ "ToggleFullscreen",     true, 13 /* WXK_RETURN */, 1 /* wxMOD_ALT */,     0 },
	{ "Screenshot",           true, 348 /* WXK_F9 */,    0 /* wxMOD_NONE */,    0 },
	{ "Exit",                 true, 0,                   0 /* wxMOD_NONE */,    0 },

	{ "Wiimote1Connect",      true, 344 /* WXK_F5 */,    1 /* wxMOD_ALT */,     0 },
	{ "Wiimote2Connect",      true, 345 /* WXK_F6 */,    1 /* wxMOD_ALT */,     0 },
	{ "Wiimote3Connect",      true, 346 /* WXK_F7 */,    1 /* wxMOD_ALT */,     0 },
	{ "Wiimote4Connect",      true, 347 /* WXK_F8 */,    1 /* wxMOD_ALT */,     0 },
	{ "BalanceBoardConnect",  true, 348 /* WXK_F9 */,    1 /* wxMOD_ALT */,     0 },
#endif
	{ "ToggleIR",             true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "ToggleAspectRatio",    true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "ToggleEFBCopies",      true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "ToggleFog",            true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "ToggleThrottle",       true, 9 /* '\t' */,        0 /* wxMOD_NONE */,    0 },
	{ "IncreaseFrameLimit",   true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "DecreaseFrameLimit",   true, 0,                   0 /* wxMOD_NONE */,    0 },

	{ "FreelookIncreaseSpeed", true, 49 /* '1' */,       4 /* wxMOD_SHIFT */,   0 },
	{ "FreelookDecreaseSpeed", true, 50 /* '2' */,       4 /* wxMOD_SHIFT */,   0 },
	{ "FreelookResetSpeed",    true, 70 /* 'F' */,       4 /* wxMOD_SHIFT */,   0 },
	{ "FreelookUp",            true, 69 /* 'E' */,       4 /* wxMOD_SHIFT */,   0 },
	{ "FreelookDown",          true, 81 /* 'Q' */,       4 /* wxMOD_SHIFT */,   0 },
	{ "FreelookLeft",          true, 65 /* 'A' */,       4 /* wxMOD_SHIFT */,   0 },
	{ "FreelookRight",         true, 68 /* 'D' */,       4 /* wxMOD_SHIFT */,   0 },
	{ "FreelookZoomIn",        true, 87 /* 'W' */,       4 /* wxMOD_SHIFT */,   0 },
	{ "FreelookZoomOut",       true, 83 /* 'S' */,       4 /* wxMOD_SHIFT */,   0 },
	{ "FreelookReset",         true, 82 /* 'R' */,       4 /* wxMOD_SHIFT */,   0 },

	{ "IncreaseSeparation",   true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "DecreaseSeparation",   true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "IncreaseConvergence",  true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "DecreaseConvergence",  true, 0,                   0 /* wxMOD_NONE */,    0 },

	{ "LoadStateSlot1",       true, 340 /* WXK_F1 */,    0 /* wxMOD_NONE */,    0 },
	{ "LoadStateSlot2",       true, 341 /* WXK_F2 */,    0 /* wxMOD_NONE */,    0 },
	{ "LoadStateSlot3",       true, 342 /* WXK_F3 */,    0 /* wxMOD_NONE */,    0 },
	{ "LoadStateSlot4",       true, 343 /* WXK_F4 */,    0 /* wxMOD_NONE */,    0 },
	{ "LoadStateSlot5",       true, 344 /* WXK_F5 */,    0 /* wxMOD_NONE */,    0 },
	{ "LoadStateSlot6",       true, 345 /* WXK_F6 */,    0 /* wxMOD_NONE */,    0 },
	{ "LoadStateSlot7",       true, 346 /* WXK_F7 */,    0 /* wxMOD_NONE */,    0 },
	{ "LoadStateSlot8",       true, 347 /* WXK_F8 */,    0 /* wxMOD_NONE */,    0 },
	{ "LoadStateSlot9",       true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "LoadStateSlot10",      true, 0,                   0 /* wxMOD_NONE */,    0 },

	{ "SaveStateSlot1",       true, 340 /* WXK_F1 */,    4 /* wxMOD_SHIFT */,   0 },
	{ "SaveStateSlot2",       true, 341 /* WXK_F2 */,    4 /* wxMOD_SHIFT */,   0 },
	{ "SaveStateSlot3",       true, 342 /* WXK_F3 */,    4 /* wxMOD_SHIFT */,   0 },
	{ "SaveStateSlot4",       true, 343 /* WXK_F4 */,    4 /* wxMOD_SHIFT */,   0 },
	{ "SaveStateSlot5",       true, 344 /* WXK_F5 */,    4 /* wxMOD_SHIFT */,   0 },
	{ "SaveStateSlot6",       true, 345 /* WXK_F6 */,    4 /* wxMOD_SHIFT */,   0 },
	{ "SaveStateSlot7",       true, 346 /* WXK_F7 */,    4 /* wxMOD_SHIFT */,   0 },
	{ "SaveStateSlot8",       true, 347 /* WXK_F8 */,    4 /* wxMOD_SHIFT */,   0 },
	{ "SaveStateSlot9",       true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "SaveStateSlot10",      true, 0,                   0 /* wxMOD_NONE */,    0 },

	{ "SelectStateSlot1",	  true, 0,                   0,                     0 },
	{ "SelectStateSlot2",	  true, 0,	                 0,                     0 },
	{ "SelectStateSlot3",	  true, 0,	                 0,                     0 },
	{ "SelectStateSlot4",	  true, 0,	                 0,                     0 },
	{ "SelectStateSlot5",	  true, 0,	                 0,                     0 },
	{ "SelectStateSlot6",	  true, 0,	                 0,                     0 },
	{ "SelectStateSlot7",	  true, 0,	                 0,                     0 },
	{ "SelectStateSlot8",	  true, 0,	                 0,                     0 },
	{ "SelectStateSlot9",	  true, 0,	                 0,                     0 },
	{ "SelectStateSlot10",	  true, 0,	                 0,                     0 },
	{ "SaveSelectedSlot",	  true, 0,	                 0,                     0 },
	{ "LoadSelectedSlot",	  true, 0,	                 0,                     0 },
	 
	{ "LoadLastState1",       true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "LoadLastState2",       true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "LoadLastState3",       true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "LoadLastState4",       true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "LoadLastState5",       true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "LoadLastState6",       true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "LoadLastState7",       true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "LoadLastState8",       true, 0,                   0 /* wxMOD_NONE */,    0 },

	{ "SaveFirstState",       true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "UndoLoadState",        true, 351 /* WXK_F12 */,   0 /* wxMOD_NONE */,    0 },
	{ "UndoSaveState",        true, 351 /* WXK_F12 */,   4 /* wxMOD_SHIFT */,   0 },
	{ "SaveStateFile",        true, 0,                   0 /* wxMOD_NONE */,    0 },
	{ "LoadStateFile",        true, 0,                   0 /* wxMOD_NONE */,    0 },
};

static const struct
{
	const char* IniText;
	const bool  KBM;
	const int   DefaultKey;
	const int   DefaultModifier;
	const u32   XInputMapping;
} g_VRData[] = {
		{ "FreelookReset",              true, 82, 4 /* wxMOD_SHIFT */, 0 },
		{ "FreelookZoomIn",             true, 87, 4 /* wxMOD_SHIFT */, 0 },
		{ "FreelookZoomOut",            true, 83, 4 /* wxMOD_SHIFT */, 0 },
		{ "FreelookLeft",               true, 65, 4 /* wxMOD_SHIFT */, 0 },
		{ "FreelookRight",              true, 68, 4 /* wxMOD_SHIFT */, 0 },
		{ "FreelookUp",                 true, 69, 4 /* wxMOD_SHIFT */, 0 },
		{ "FreelookDown",               true, 81, 4 /* wxMOD_SHIFT */, 0 },
		{ "VRPermanentCameraForward",   true, 80, 4 /* wxMOD_SHIFT */, 0 },
		{ "VRPermanentCameraBackward",  true, 59, 4 /* wxMOD_SHIFT */, 0 },
		{ "VRLargerScale",              true, 61, 4 /* wxMOD_SHIFT */, 0 },
		{ "VRSmallerScale",	            true, 45, 4 /* wxMOD_SHIFT */, 0 },
		{ "VRCameraTiltUp",             true, 79, 4 /* wxMOD_SHIFT */, 0 },
		{ "VRCameraTiltDown",           true, 76, 4 /* wxMOD_SHIFT */, 0 },

		{ "VRHUDForward",               true, 47, 4 /* wxMOD_SHIFT */, 0 },
		{ "VRHUDBackward",              true, 46, 4 /* wxMOD_SHIFT */, 0 },
		{ "VRHUDThicker",               true, 93, 4 /* wxMOD_SHIFT */, 0 },
		{ "VRHUDThinner",               true, 91, 4 /* wxMOD_SHIFT */, 0 },
		{ "VRHUD3DCloser",              true,  0, 0 /* wxMOD_NONE */, 0 },
		{ "VRHUD3DFurther",             true,  0, 0 /* wxMOD_NONE */, 0 },

		{ "VR2DScreenLarger",           true, 44, 4 /* wxMOD_SHIFT */, 0 },
		{ "VR2DScreenSmaller",          true, 77, 4 /* wxMOD_SHIFT */, 0 },
		{ "VR2DCameraForward",          true, 74, 4 /* wxMOD_SHIFT */, 0 },
		{ "VR2DCameraBackward",         true, 85, 4 /* wxMOD_SHIFT */, 0 },
		//{ "VR2DScreenLeft",             true, 0, 0 /* wxMOD_NONE */, 0 }, //doesn't_exist_right_now?
		//{ "VR2DScreenRight",            true, 0, 0 /* wxMOD_NONE */, 0 }, //doesn't_exist_right_now?
		{ "VR2DCameraUp",               true, 72, 4 /* wxMOD_SHIFT */, 0 },
		{ "VR2DCameraDown",             true, 89, 4 /* wxMOD_SHIFT */, 0 },
		{ "VR2DCameraTiltUp",           true, 73, 4 /* wxMOD_SHIFT */, 0 },
		{ "VR2DCameraTiltDown",         true, 75, 4 /* wxMOD_SHIFT */, 0 },
		{ "VR2DScreenThicker",          true, 84, 4 /* wxMOD_SHIFT */, 0 },
		{ "VR2DScreenThinner",          true, 71, 4 /* wxMOD_SHIFT */, 0 },

};

SConfig::SConfig()
{
	// Make sure we have log manager
	LoadSettings();
}

void SConfig::Init()
{
	m_Instance = new SConfig;
}

void SConfig::Shutdown()
{
	delete m_Instance;
	m_Instance = nullptr;
}

SConfig::~SConfig()
{
	SaveSettings();
	delete m_SYSCONF;
}


void SConfig::SaveSettings()
{
	NOTICE_LOG(BOOT, "Saving settings to %s", File::GetUserPath(F_DOLPHINCONFIG_IDX).c_str());
	IniFile ini;
	ini.Load(File::GetUserPath(F_DOLPHINCONFIG_IDX)); // load first to not kill unknown stuff

	SaveGeneralSettings(ini);
	SaveInterfaceSettings(ini);
	SaveHotkeySettings(ini);
	SaveDisplaySettings(ini);
	SaveGameListSettings(ini);
	SaveCoreSettings(ini);
	SaveMovieSettings(ini);
	SaveDSPSettings(ini);
	SaveInputSettings(ini);
	SaveFifoPlayerSettings(ini);
	SaveVRSettings(ini);

	ini.Save(File::GetUserPath(F_DOLPHINCONFIG_IDX));
	m_SYSCONF->Save();
}

void SConfig::SaveGeneralSettings(IniFile& ini)
{
	IniFile::Section* general = ini.GetOrCreateSection("General");

	// General
	general->Set("LastFilename", m_LastFilename);
	general->Set("ShowLag", m_ShowLag);
	general->Set("ShowFrameCount", m_ShowFrameCount);

	// ISO folders
	// Clear removed folders
	int oldPaths;
	int numPaths = (int)m_ISOFolder.size();
	//general->Get("GCMPathes", &oldPaths, 0);
	//for (int i = numPaths; i < oldPaths; i++)
	//{
	//	ini.DeleteKey("General", StringFromFormat("GCMPath%i", i));
	//}
	//ini.DeleteKey("General", "GCMPathes");

	general->Get("ISOPaths", &oldPaths, 0);
	for (int i = numPaths; i < oldPaths; i++)
	{
		ini.DeleteKey("General", StringFromFormat("ISOPath%i", i));
	}

	general->Set("ISOPaths", numPaths);
	for (int i = 0; i < numPaths; i++)
	{
		general->Set(StringFromFormat("ISOPath%i", i), m_ISOFolder[i]);
	}

	general->Set("RecursiveISOPaths", m_RecursiveISOFolder);
	general->Set("NANDRootPath", m_NANDPath);
	general->Set("WirelessMac", m_WirelessMac);

#ifdef USE_GDBSTUB
	general->Set("GDBPort", m_LocalCoreStartupParameter.iGDBPort);
#endif
}

void SConfig::SaveInterfaceSettings(IniFile& ini)
{
	IniFile::Section* interface = ini.GetOrCreateSection("Interface");

	interface->Set("ConfirmStop", m_LocalCoreStartupParameter.bConfirmStop);
	interface->Set("UsePanicHandlers", m_LocalCoreStartupParameter.bUsePanicHandlers);
	interface->Set("OnScreenDisplayMessages", m_LocalCoreStartupParameter.bOnScreenDisplayMessages);
	interface->Set("HideCursor", m_LocalCoreStartupParameter.bHideCursor);
	interface->Set("AutoHideCursor", m_LocalCoreStartupParameter.bAutoHideCursor);
	interface->Set("MainWindowPosX", (m_LocalCoreStartupParameter.iPosX == -32000) ? 0 : m_LocalCoreStartupParameter.iPosX); // TODO - HAX
	interface->Set("MainWindowPosY", (m_LocalCoreStartupParameter.iPosY == -32000) ? 0 : m_LocalCoreStartupParameter.iPosY); // TODO - HAX
	interface->Set("MainWindowWidth", m_LocalCoreStartupParameter.iWidth);
	interface->Set("MainWindowHeight", m_LocalCoreStartupParameter.iHeight);
	interface->Set("Language", m_InterfaceLanguage);
	interface->Set("ShowToolbar", m_InterfaceToolbar);
	interface->Set("ShowStatusbar", m_InterfaceStatusbar);
	interface->Set("ShowLogWindow", m_InterfaceLogWindow);
	interface->Set("ShowLogConfigWindow", m_InterfaceLogConfigWindow);
	interface->Set("ExtendedFPSInfo", m_InterfaceExtendedFPSInfo);
	interface->Set("ThemeName40", m_LocalCoreStartupParameter.theme_name);
}

void SConfig::SaveHotkeySettings(IniFile& ini)
{
	IniFile::Section* hotkeys = ini.GetOrCreateSection("Hotkeys");

	hotkeys->Set("XInputPolling", m_LocalCoreStartupParameter.bHotkeysXInput);
	hotkeys->Set("FreeLookSensitivity", m_LocalCoreStartupParameter.fFreeLookSensitivity);

	for (int i = 0; i < NUM_HOTKEYS; i++)
	{
		hotkeys->Set(g_HKData[i].IniText, m_LocalCoreStartupParameter.iHotkey[i]);
		hotkeys->Set(std::string(g_HKData[i].IniText) + "Modifier",
			m_LocalCoreStartupParameter.iHotkeyModifier[i]);
		hotkeys->Set(std::string(g_HKData[i].IniText) + "KBM",
			m_LocalCoreStartupParameter.iHotkeyKBM[i]);
		hotkeys->Set(std::string(g_HKData[i].IniText) + "XInputMapping",
			m_LocalCoreStartupParameter.iHotkeyXInputMapping[i]);
	}
}

void SConfig::SaveVRSettings(IniFile& ini)
{
	IniFile::Section* vrsettings = ini.GetOrCreateSection("Hotkeys");

	for (int i = 0; i < NUM_VR_HOTKEYS; i++)
	{
		vrsettings->Set(g_VRData[i].IniText, m_LocalCoreStartupParameter.iVRSettings[i]);
		vrsettings->Set(std::string(g_VRData[i].IniText) + "Modifier",
			m_LocalCoreStartupParameter.iVRSettingsModifier[i]);
		vrsettings->Set(std::string(g_VRData[i].IniText) + "KBM",
			m_LocalCoreStartupParameter.iVRSettingsKBM[i]);
		vrsettings->Set(std::string(g_VRData[i].IniText) + "XInputMapping",
			m_LocalCoreStartupParameter.iVRSettingsXInputMapping[i]);
	}
}

void SConfig::SaveDisplaySettings(IniFile& ini)
{
	IniFile::Section* display = ini.GetOrCreateSection("Display");

	if (!m_special_case)
		display->Set("FullscreenResolution", m_LocalCoreStartupParameter.strFullscreenResolution);
	display->Set("Fullscreen", m_LocalCoreStartupParameter.bFullscreen);
	display->Set("RenderToMain", m_LocalCoreStartupParameter.bRenderToMain);
	if (!m_special_case)
	{
		display->Set("RenderWindowXPos", m_LocalCoreStartupParameter.iRenderWindowXPos);
		display->Set("RenderWindowYPos", m_LocalCoreStartupParameter.iRenderWindowYPos);
		display->Set("RenderWindowWidth", m_LocalCoreStartupParameter.iRenderWindowWidth);
		display->Set("RenderWindowHeight", m_LocalCoreStartupParameter.iRenderWindowHeight);
	}
	display->Set("RenderWindowAutoSize", m_LocalCoreStartupParameter.bRenderWindowAutoSize);
	display->Set("KeepWindowOnTop", m_LocalCoreStartupParameter.bKeepWindowOnTop);
	display->Set("ProgressiveScan", m_LocalCoreStartupParameter.bProgressive);
	display->Set("DisableScreenSaver", m_LocalCoreStartupParameter.bDisableScreenSaver);
	display->Set("ForceNTSCJ", m_LocalCoreStartupParameter.bForceNTSCJ);

	IniFile::Section* vr = ini.GetOrCreateSection("VR");
	vr->Set("AsynchronousTimewarp", m_LocalCoreStartupParameter.bAsynchronousTimewarp);
}

void SConfig::SaveGameListSettings(IniFile& ini)
{
	IniFile::Section* gamelist = ini.GetOrCreateSection("GameList");

	gamelist->Set("ListDrives", m_ListDrives);
	gamelist->Set("ListWad", m_ListWad);
	gamelist->Set("ListWii", m_ListWii);
	gamelist->Set("ListGC", m_ListGC);
	gamelist->Set("ListJap", m_ListJap);
	gamelist->Set("ListPal", m_ListPal);
	gamelist->Set("ListUsa", m_ListUsa);
	gamelist->Set("ListAustralia", m_ListAustralia);
	gamelist->Set("ListFrance", m_ListFrance);
	gamelist->Set("ListGermany", m_ListGermany);
	gamelist->Set("ListInternational", m_ListInternational);
	gamelist->Set("ListItaly", m_ListItaly);
	gamelist->Set("ListKorea", m_ListKorea);
	gamelist->Set("ListNetherlands", m_ListNetherlands);
	gamelist->Set("ListRussia", m_ListRussia);
	gamelist->Set("ListSpain", m_ListSpain);
	gamelist->Set("ListTaiwan", m_ListTaiwan);
	gamelist->Set("ListUnknown", m_ListUnknown);
	gamelist->Set("ListSort", m_ListSort);
	gamelist->Set("ListSortSecondary", m_ListSort2);

	gamelist->Set("ColorCompressed", m_ColorCompressed);

	gamelist->Set("ColumnPlatform", m_showSystemColumn);
	gamelist->Set("ColumnBanner", m_showBannerColumn);
	gamelist->Set("ColumnNotes", m_showNotesColumn);
	gamelist->Set("ColumnID", m_showIDColumn);
	gamelist->Set("ColumnRegion", m_showRegionColumn);
	gamelist->Set("ColumnSize", m_showSizeColumn);
	gamelist->Set("ColumnState", m_showStateColumn);
}

void SConfig::SaveCoreSettings(IniFile& ini)
{
	IniFile::Section* core = ini.GetOrCreateSection("Core");

	core->Set("HLE_BS2", m_LocalCoreStartupParameter.bHLE_BS2);
	core->Set("CPUCore", m_LocalCoreStartupParameter.iCPUCore);
	core->Set("Fastmem", m_LocalCoreStartupParameter.bFastmem);
	core->Set("CPUThread", m_LocalCoreStartupParameter.bCPUThread);
	core->Set("DSPThread", m_LocalCoreStartupParameter.bDSPThread);
	core->Set("DSPHLE", m_LocalCoreStartupParameter.bDSPHLE);
	core->Set("SkipIdle", m_LocalCoreStartupParameter.bSkipIdle);
	core->Set("DefaultISO", m_LocalCoreStartupParameter.m_strDefaultISO);
	core->Set("DVDRoot", m_LocalCoreStartupParameter.m_strDVDRoot);
	core->Set("Apploader", m_LocalCoreStartupParameter.m_strApploader);
	core->Set("EnableCheats", m_LocalCoreStartupParameter.bEnableCheats);
	core->Set("SelectedLanguage", m_LocalCoreStartupParameter.SelectedLanguage);
	core->Set("DPL2Decoder", m_LocalCoreStartupParameter.bDPL2Decoder);
	core->Set("Latency", m_LocalCoreStartupParameter.iLatency);
	core->Set("MemcardAPath", m_strMemoryCardA);
	core->Set("MemcardBPath", m_strMemoryCardB);
	core->Set("SlotA", m_EXIDevice[0]);
	core->Set("SlotB", m_EXIDevice[1]);
	core->Set("SerialPort1", m_EXIDevice[2]);
	core->Set("BBA_MAC", m_bba_mac);
	for (int i = 0; i < MAX_SI_CHANNELS; ++i)
	{
		core->Set(StringFromFormat("SIDevice%i", i), m_SIDevice[i]);
	}
	core->Set("WiiSDCard", m_WiiSDCard);
	core->Set("WiiKeyboard", m_WiiKeyboard);
	core->Set("WiimoteContinuousScanning", m_WiimoteContinuousScanning);
	core->Set("WiimoteEnableSpeaker", m_WiimoteEnableSpeaker);
	core->Set("RunCompareServer", m_LocalCoreStartupParameter.bRunCompareServer);
	core->Set("RunCompareClient", m_LocalCoreStartupParameter.bRunCompareClient);
	core->Set("FrameLimit", m_Framelimit);
	core->Set("FrameSkip", m_FrameSkip);
	core->Set("GFXBackend", m_LocalCoreStartupParameter.m_strVideoBackend);
	core->Set("GPUDeterminismMode", m_LocalCoreStartupParameter.m_strGPUDeterminismMode);
}

void SConfig::SaveMovieSettings(IniFile& ini)
{
	IniFile::Section* movie = ini.GetOrCreateSection("Movie");

	movie->Set("PauseMovie", m_PauseMovie);
	movie->Set("Author", m_strMovieAuthor);
	movie->Set("DumpFrames", m_DumpFrames);
	movie->Set("ShowInputDisplay", m_ShowInputDisplay);
}

void SConfig::SaveDSPSettings(IniFile& ini)
{
	IniFile::Section* dsp = ini.GetOrCreateSection("DSP");

	dsp->Set("EnableJIT", m_DSPEnableJIT);
	dsp->Set("DumpAudio", m_DumpAudio);
	dsp->Set("Backend", sBackend);
	dsp->Set("Volume", m_Volume);
	dsp->Set("CaptureLog", m_DSPCaptureLog);
}

void SConfig::SaveInputSettings(IniFile& ini)
{
	IniFile::Section* input = ini.GetOrCreateSection("Input");

	input->Set("BackgroundInput", m_BackgroundInput);
}

void SConfig::SaveFifoPlayerSettings(IniFile& ini)
{
	IniFile::Section* fifoplayer = ini.GetOrCreateSection("FifoPlayer");

	fifoplayer->Set("LoopReplay", m_LocalCoreStartupParameter.bLoopFifoReplay);
}

void SConfig::LoadSettings()
{
	INFO_LOG(BOOT, "Loading Settings from %s", File::GetUserPath(F_DOLPHINCONFIG_IDX).c_str());
	IniFile ini;
	ini.Load(File::GetUserPath(F_DOLPHINCONFIG_IDX));

	LoadGeneralSettings(ini);
	LoadInterfaceSettings(ini);
	LoadHotkeySettings(ini);
	LoadDisplaySettings(ini);
	LoadGameListSettings(ini);
	LoadCoreSettings(ini);
	LoadMovieSettings(ini);
	LoadDSPSettings(ini);
	LoadInputSettings(ini);
	LoadFifoPlayerSettings(ini);
	LoadVRSettings(ini);

	m_SYSCONF = new SysConf();
}

void SConfig::LoadGeneralSettings(IniFile& ini)
{
	IniFile::Section* general = ini.GetOrCreateSection("General");

	general->Get("LastFilename", &m_LastFilename);
	general->Get("ShowLag", &m_ShowLag, false);
	general->Get("ShowFrameCount", &m_ShowFrameCount, false);
#ifdef USE_GDBSTUB
	general->Get("GDBPort", &(m_LocalCoreStartupParameter.iGDBPort), -1);
#endif

	m_ISOFolder.clear();
	int numISOPaths;

	if (general->Get("ISOPaths", &numISOPaths, 0))
	{
		for (int i = 0; i < numISOPaths; i++)
		{
			std::string tmpPath;
			general->Get(StringFromFormat("ISOPath%i", i), &tmpPath, "");
			m_ISOFolder.push_back(std::move(tmpPath));
		}
	}

	if (general->Get("GCMPathes", &numISOPaths, 0))
	{
		for (int i = 0; i < numISOPaths; i++)
		{
			std::string tmpPath;
			general->Get(StringFromFormat("GCMPath%i", i), &tmpPath, "");
			bool found = false;
			for (int j = 0; j < m_ISOFolder.size(); ++j)
			{
				if (m_ISOFolder[j] == tmpPath)
				{
					found = true;
					break;
				}
			}
			if (!found)
				m_ISOFolder.push_back(std::move(tmpPath));
		}
	}

	general->Get("RecursiveISOPaths", &m_RecursiveISOFolder, false);
	if (!m_RecursiveISOFolder)
		general->Get("RecursiveGCMPaths", &m_RecursiveISOFolder, false);

	general->Get("NANDRootPath", &m_NANDPath);
	m_NANDPath = File::GetUserPath(D_WIIROOT_IDX, m_NANDPath);
	DiscIO::cUIDsys::AccessInstance().UpdateLocation();
	DiscIO::CSharedContent::AccessInstance().UpdateLocation();
	general->Get("WirelessMac", &m_WirelessMac);
}

void SConfig::LoadInterfaceSettings(IniFile& ini)
{
	IniFile::Section* interface = ini.GetOrCreateSection("Interface");

	interface->Get("ConfirmStop",             &m_LocalCoreStartupParameter.bConfirmStop,      true);
	interface->Get("UsePanicHandlers",        &m_LocalCoreStartupParameter.bUsePanicHandlers, true);
	interface->Get("OnScreenDisplayMessages", &m_LocalCoreStartupParameter.bOnScreenDisplayMessages, true);
	interface->Get("HideCursor",              &m_LocalCoreStartupParameter.bHideCursor,       false);
	interface->Get("AutoHideCursor",          &m_LocalCoreStartupParameter.bAutoHideCursor,   false);
	interface->Get("MainWindowPosX",          &m_LocalCoreStartupParameter.iPosX,             100);
	interface->Get("MainWindowPosY",          &m_LocalCoreStartupParameter.iPosY,             100);
	interface->Get("MainWindowWidth",         &m_LocalCoreStartupParameter.iWidth,            800);
	interface->Get("MainWindowHeight",        &m_LocalCoreStartupParameter.iHeight,           600);
	interface->Get("Language",                &m_InterfaceLanguage,                           0);
	interface->Get("ShowToolbar",             &m_InterfaceToolbar,                            true);
	interface->Get("ShowStatusbar",           &m_InterfaceStatusbar,                          true);
	interface->Get("ShowLogWindow",           &m_InterfaceLogWindow,                          false);
	interface->Get("ShowLogConfigWindow",     &m_InterfaceLogConfigWindow,                    false);
	interface->Get("ExtendedFPSInfo",         &m_InterfaceExtendedFPSInfo,                    false);
	interface->Get("ThemeName40",             &m_LocalCoreStartupParameter.theme_name,        "Clean");
}

void SConfig::LoadHotkeySettings(IniFile& ini)
{
	IniFile::Section* hotkeys = ini.GetOrCreateSection("Hotkeys");

	hotkeys->Get("XInputPolling", &m_LocalCoreStartupParameter.bHotkeysXInput, true);
	hotkeys->Get("FreeLookSensitivity", &m_LocalCoreStartupParameter.fFreeLookSensitivity, 1.00);

	for (int i = 0; i < NUM_HOTKEYS; i++)
	{
		hotkeys->Get(g_HKData[i].IniText,
		    &m_LocalCoreStartupParameter.iHotkey[i], g_HKData[i].DefaultKey);
		hotkeys->Get(std::string(g_HKData[i].IniText) + "Modifier",
		    &m_LocalCoreStartupParameter.iHotkeyModifier[i], g_HKData[i].DefaultModifier);
		hotkeys->Get(std::string(g_HKData[i].IniText) + "KBM",
			&m_LocalCoreStartupParameter.iHotkeyKBM[i], g_HKData[i].KBM);
		hotkeys->Get(std::string(g_HKData[i].IniText) + "XInputMapping",
			&m_LocalCoreStartupParameter.iHotkeyXInputMapping[i], g_HKData[i].XInputMapping);
	}
}

void SConfig::LoadVRSettings(IniFile& ini)
{
	IniFile::Section* vrsettings = ini.GetOrCreateSection("Hotkeys");


	for (int i = 0; i < NUM_VR_HOTKEYS; i++)
	{
		vrsettings->Get(g_VRData[i].IniText,
			&m_LocalCoreStartupParameter.iVRSettings[i], g_VRData[i].DefaultKey);
		vrsettings->Get(std::string(g_VRData[i].IniText) + "Modifier",
			&m_LocalCoreStartupParameter.iVRSettingsModifier[i], g_VRData[i].DefaultModifier);
		vrsettings->Get(std::string(g_VRData[i].IniText) + "KBM",
			&m_LocalCoreStartupParameter.iVRSettingsKBM[i], g_VRData[i].KBM);
		vrsettings->Get(std::string(g_VRData[i].IniText) + "XInputMapping",
			&m_LocalCoreStartupParameter.iVRSettingsXInputMapping[i], g_VRData[i].XInputMapping);
	}
}

void SConfig::LoadDisplaySettings(IniFile& ini)
{
	IniFile::Section* display = ini.GetOrCreateSection("Display");

	display->Get("Fullscreen",           &m_LocalCoreStartupParameter.bFullscreen,             false);
	display->Get("FullscreenResolution", &m_LocalCoreStartupParameter.strFullscreenResolution, "Auto");
	display->Get("RenderToMain",         &m_LocalCoreStartupParameter.bRenderToMain,           false);
	display->Get("RenderWindowXPos",     &m_LocalCoreStartupParameter.iRenderWindowXPos,       -1);
	display->Get("RenderWindowYPos",     &m_LocalCoreStartupParameter.iRenderWindowYPos,       -1);
	display->Get("RenderWindowWidth",    &m_LocalCoreStartupParameter.iRenderWindowWidth,      640);
	display->Get("RenderWindowHeight",   &m_LocalCoreStartupParameter.iRenderWindowHeight,     480);
	display->Get("RenderWindowAutoSize", &m_LocalCoreStartupParameter.bRenderWindowAutoSize,   false);
	display->Get("KeepWindowOnTop",      &m_LocalCoreStartupParameter.bKeepWindowOnTop,        false);
	display->Get("ProgressiveScan",      &m_LocalCoreStartupParameter.bProgressive,            false);
	display->Get("DisableScreenSaver",   &m_LocalCoreStartupParameter.bDisableScreenSaver,     true);
	display->Get("ForceNTSCJ",           &m_LocalCoreStartupParameter.bForceNTSCJ,             false);

	IniFile::Section* vr = ini.GetOrCreateSection("VR");
	vr->Get("AsynchronousTimewarp",      &m_LocalCoreStartupParameter.bAsynchronousTimewarp,   false);
}

void SConfig::LoadGameListSettings(IniFile& ini)
{
	IniFile::Section* gamelist = ini.GetOrCreateSection("GameList");

	gamelist->Get("ListDrives",        &m_ListDrives,  false);
	gamelist->Get("ListWad",           &m_ListWad,     true);
	gamelist->Get("ListWii",           &m_ListWii,     true);
	gamelist->Get("ListGC",            &m_ListGC,      true);
	gamelist->Get("ListJap",           &m_ListJap,     true);
	gamelist->Get("ListPal",           &m_ListPal,     true);
	gamelist->Get("ListUsa",           &m_ListUsa,     true);

	gamelist->Get("ListAustralia",     &m_ListAustralia,     true);
	gamelist->Get("ListFrance",        &m_ListFrance,        true);
	gamelist->Get("ListGermany",       &m_ListGermany,       true);
	gamelist->Get("ListInternational", &m_ListInternational, true);
	gamelist->Get("ListItaly",         &m_ListItaly,         true);
	gamelist->Get("ListKorea",         &m_ListKorea,         true);
	gamelist->Get("ListNetherlands",   &m_ListNetherlands,   true);
	gamelist->Get("ListRussia",        &m_ListRussia,        true);
	gamelist->Get("ListSpain",         &m_ListSpain,         true);
	gamelist->Get("ListTaiwan",        &m_ListTaiwan,        true);
	gamelist->Get("ListUnknown",       &m_ListUnknown,       true);
	gamelist->Get("ListSort",          &m_ListSort,       3);
	gamelist->Get("ListSortSecondary", &m_ListSort2,      0);

	// Determines if compressed games display in blue
	gamelist->Get("ColorCompressed", &m_ColorCompressed, true);

	// Gamelist columns toggles
	gamelist->Get("ColumnPlatform",   &m_showSystemColumn,  true);
	gamelist->Get("ColumnBanner",     &m_showBannerColumn,  true);
	gamelist->Get("ColumnNotes",      &m_showNotesColumn,   true);
	gamelist->Get("ColumnID",         &m_showIDColumn,      false);
	gamelist->Get("ColumnRegion",     &m_showRegionColumn,  true);
	gamelist->Get("ColumnSize",       &m_showSizeColumn,    true);
	gamelist->Get("ColumnState",      &m_showStateColumn,   true);
}

void SConfig::LoadCoreSettings(IniFile& ini)
{
	IniFile::Section* core = ini.GetOrCreateSection("Core");

	core->Get("HLE_BS2",      &m_LocalCoreStartupParameter.bHLE_BS2, false);
#ifdef _M_X86
	core->Get("CPUCore",      &m_LocalCoreStartupParameter.iCPUCore, SCoreStartupParameter::CORE_JIT64);
#elif _M_ARM_32
	core->Get("CPUCore",      &m_LocalCoreStartupParameter.iCPUCore, SCoreStartupParameter::CORE_JITARM);
#else
	core->Get("CPUCore",      &m_LocalCoreStartupParameter.iCPUCore, SCoreStartupParameter::CORE_INTERPRETER);
#endif
	core->Get("Fastmem",           &m_LocalCoreStartupParameter.bFastmem,      true);
	core->Get("DSPThread",         &m_LocalCoreStartupParameter.bDSPThread,    false);
	core->Get("DSPHLE",            &m_LocalCoreStartupParameter.bDSPHLE,       true);
	core->Get("CPUThread",         &m_LocalCoreStartupParameter.bCPUThread,    true);
	core->Get("SkipIdle",          &m_LocalCoreStartupParameter.bSkipIdle,     true);
	core->Get("DefaultISO",        &m_LocalCoreStartupParameter.m_strDefaultISO);
	core->Get("DVDRoot",           &m_LocalCoreStartupParameter.m_strDVDRoot);
	core->Get("Apploader",         &m_LocalCoreStartupParameter.m_strApploader);
	core->Get("EnableCheats",      &m_LocalCoreStartupParameter.bEnableCheats, false);
	core->Get("SelectedLanguage",  &m_LocalCoreStartupParameter.SelectedLanguage, 0);
	core->Get("DPL2Decoder",       &m_LocalCoreStartupParameter.bDPL2Decoder, false);
	core->Get("Latency",           &m_LocalCoreStartupParameter.iLatency, 2);
	core->Get("MemcardAPath",      &m_strMemoryCardA);
	core->Get("MemcardBPath",      &m_strMemoryCardB);
	core->Get("SlotA",       (int*)&m_EXIDevice[0], EXIDEVICE_MEMORYCARD);
	core->Get("SlotB",       (int*)&m_EXIDevice[1], EXIDEVICE_NONE);
	core->Get("SerialPort1", (int*)&m_EXIDevice[2], EXIDEVICE_NONE);
	core->Get("BBA_MAC",           &m_bba_mac);
	core->Get("TimeProfiling",     &m_LocalCoreStartupParameter.bJITILTimeProfiling, false);
	core->Get("OutputIR",          &m_LocalCoreStartupParameter.bJITILOutputIR,      false);
	for (int i = 0; i < MAX_SI_CHANNELS; ++i)
	{
		core->Get(StringFromFormat("SIDevice%i", i), (u32*)&m_SIDevice[i], (i == 0) ? SIDEVICE_GC_CONTROLLER : SIDEVICE_NONE);
	}
	core->Get("WiiSDCard",                 &m_WiiSDCard,                                   false);
	core->Get("WiiKeyboard",               &m_WiiKeyboard,                                 false);
	core->Get("WiimoteContinuousScanning", &m_WiimoteContinuousScanning,                   false);
	core->Get("WiimoteEnableSpeaker",      &m_WiimoteEnableSpeaker,                        false);
	core->Get("RunCompareServer",          &m_LocalCoreStartupParameter.bRunCompareServer, false);
	core->Get("RunCompareClient",          &m_LocalCoreStartupParameter.bRunCompareClient, false);
	core->Get("MMU",                       &m_LocalCoreStartupParameter.bMMU,              false);
	core->Get("BBDumpPort",                &m_LocalCoreStartupParameter.iBBDumpPort,       -1);
	core->Get("VBeam",                     &m_LocalCoreStartupParameter.bVBeamSpeedHack,   false);
	core->Get("SyncGPU",                   &m_LocalCoreStartupParameter.bSyncGPU,          false);
	core->Get("FastDiscSpeed",             &m_LocalCoreStartupParameter.bFastDiscSpeed,    false);
	core->Get("DCBZ",                      &m_LocalCoreStartupParameter.bDCBZOFF,          false);
	core->Get("FrameLimit",                &m_Framelimit,                                  1); // auto frame limit by default
	core->Get("FrameSkip",                 &m_FrameSkip,                                   0);
	core->Get("GFXBackend",                &m_LocalCoreStartupParameter.m_strVideoBackend, "");
	core->Get("GPUDeterminismMode",        &m_LocalCoreStartupParameter.m_strGPUDeterminismMode, "auto");
}

void SConfig::LoadMovieSettings(IniFile& ini)
{
	IniFile::Section* movie = ini.GetOrCreateSection("Movie");

	movie->Get("PauseMovie", &m_PauseMovie, false);
	movie->Get("Author", &m_strMovieAuthor, "");
	movie->Get("DumpFrames", &m_DumpFrames, false);
	movie->Get("ShowInputDisplay", &m_ShowInputDisplay, false);
}

void SConfig::LoadDSPSettings(IniFile& ini)
{
	IniFile::Section* dsp = ini.GetOrCreateSection("DSP");

	dsp->Get("EnableJIT", &m_DSPEnableJIT, true);
	dsp->Get("DumpAudio", &m_DumpAudio, false);
#if defined __linux__ && HAVE_ALSA
	dsp->Get("Backend", &sBackend, BACKEND_ALSA);
#elif defined __APPLE__
	dsp->Get("Backend", &sBackend, BACKEND_COREAUDIO);
#elif defined _WIN32
	dsp->Get("Backend", &sBackend, BACKEND_XAUDIO2);
#elif defined ANDROID
	dsp->Get("Backend", &sBackend, BACKEND_OPENSLES);
#else
	dsp->Get("Backend", &sBackend, BACKEND_NULLSOUND);
#endif
	dsp->Get("Volume", &m_Volume, 100);
	dsp->Get("CaptureLog", &m_DSPCaptureLog, false);
}

void SConfig::LoadInputSettings(IniFile& ini)
{
	IniFile::Section* input = ini.GetOrCreateSection("Input");

	input->Get("BackgroundInput", &m_BackgroundInput, false);
}

void SConfig::LoadFifoPlayerSettings(IniFile& ini)
{
	IniFile::Section* fifoplayer = ini.GetOrCreateSection("FifoPlayer");

	fifoplayer->Get("LoopReplay", &m_LocalCoreStartupParameter.bLoopFifoReplay, true);
}
