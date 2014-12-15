/*
 * Copyright © 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *     Wei Lin<wei.w.lin@intel.com>
 *     Yuting Yang<yuting.yang@intel.com>
 */

#include "os_util_debug.h"

#if GENOS_MESSAGES_ENABLED
#include "os_utilities.h"

extern INT32 GenOsMemAllocCounter;

const PCCHAR GenOsLogPathTemplate = "%s/igd_%u.%s";

const PCCHAR DDILogPathTemplate = "%s\\ddi_dump_%d.%s";

const PCCHAR GENOS_LogLevelName[GENOS_MESSAGE_LVL_COUNT] = {
	"",
	"CRITICAL",
	"NORMAL  ",
	"VERBOSE ",
	"ENTER   ",
	"EXIT    ",
	"ENTER   ",
	"EXIT    ",
};

const PCCHAR GENOS_ComponentName[GENOS_COMPONENT_COUNT] = {
	"[GENOS]:  ",
	"[GENHW]:  ",
	"[LIBVA]:",
	"[CM]:   "
};

GENOS_MESSAGE_PARAMS g_GenOsMsgParams;
GENOS_MESSAGE_PARAMS g_GenOsMsgParams_DDI_Dump;

PCCHAR pcComponentRegKeys[GENOS_COMPONENT_COUNT][3] = {
	{
	 __GENOS_USER_FEATURE_KEY_MESSAGE_OS_TAG,
	 __GENOS_USER_FEATURE_KEY_BY_SUB_COMPONENT_OS,
	 __GENOS_USER_FEATURE_KEY_SUB_COMPONENT_OS_TAG},

	{
	 __GENOS_USER_FEATURE_KEY_MESSAGE_CODEC_TAG,
	 __GENOS_USER_FEATURE_KEY_BY_SUB_COMPONENT_CODEC,
	 __GENOS_USER_FEATURE_KEY_SUB_COMPONENT_CODEC_TAG},

	{
	 __GENOS_USER_FEATURE_KEY_MESSAGE_VP_TAG,
	 __GENOS_USER_FEATURE_KEY_BY_SUB_COMPONENT_VP,
	 __GENOS_USER_FEATURE_KEY_SUB_COMPONENT_VP_TAG},

	{
	 __GENOS_USER_FEATURE_KEY_MESSAGE_CP_TAG,
	 __GENOS_USER_FEATURE_KEY_BY_SUB_COMPONENT_CP,
	 __GENOS_USER_FEATURE_KEY_SUB_COMPONENT_CP_TAG},

	{
	 __GENOS_USER_FEATURE_KEY_MESSAGE_DDI_TAG,
	 __GENOS_USER_FEATURE_KEY_BY_SUB_COMPONENT_DDI,
	 __GENOS_USER_FEATURE_KEY_SUB_COMPONENT_DDI_TAG},

	{
	 __GENOS_USER_FEATURE_KEY_MESSAGE_CM_TAG,
	 __GENOS_USER_FEATURE_KEY_BY_SUB_COMPONENT_CM,
	 __GENOS_USER_FEATURE_KEY_SUB_COMPONENT_CM_TAG}
};

UINT8 subComponentCount[GENOS_COMPONENT_COUNT] = {
	GENOS_SUBCOMP_COUNT,
	GENOS_CODEC_SUBCOMP_COUNT,
	GENOS_VP_SUBCOMP_COUNT,
	GENOS_CP_SUBCOMP_COUNT,
	GENOS_DDI_SUBCOMP_COUNT,
	GENOS_CM_SUBCOMP_COUNT
};

VOID GENOS_SetSubCompMessageLevel(GENOS_COMPONENT_ID compID, UINT8 subCompID,
				  GENOS_MESSAGE_LEVEL msgLevel)
{
	if (compID >= GENOS_COMPONENT_COUNT) {
		GENOS_OS_ASSERTMESSAGE("Invalid component %d.", compID);
		return;
	}

	if (subCompID >= GENOS_MAX_SUBCOMPONENT_COUNT) {
		GENOS_OS_ASSERTMESSAGE("Invalid sub-component %d.", subCompID);
		return;
	}

	g_GenOsMsgParams.components[compID].subComponents[subCompID].
	    uiMessageLevel = msgLevel;
}

VOID GENOS_SetCompMessageLevel(GENOS_COMPONENT_ID compID,
			       GENOS_MESSAGE_LEVEL msgLevel)
{
	if (compID >= GENOS_COMPONENT_COUNT) {
		GENOS_OS_ASSERTMESSAGE("Invalid component %d.", compID);
		return;
	}

	g_GenOsMsgParams.components[compID].component.uiMessageLevel = msgLevel;
}

VOID GENOS_SetCompMessageLevelAll(GENOS_MESSAGE_LEVEL msgLevel)
{
	UINT32 i;

	for (i = 0; i < GENOS_COMPONENT_COUNT; i++) {
		GENOS_SetCompMessageLevel((GENOS_COMPONENT_ID) i, msgLevel);
	}
}

VOID GENOS_SubCompAssertEnableDisable(GENOS_COMPONENT_ID compID,
				      UINT8 subCompID, BOOL bEnable)
{
	if (compID >= GENOS_COMPONENT_COUNT) {
		GENOS_OS_ASSERTMESSAGE("Invalid component %d.", compID);
		return;
	}

	if (subCompID >= GENOS_MAX_SUBCOMPONENT_COUNT) {
		GENOS_OS_ASSERTMESSAGE("Invalid sub-component %d.", subCompID);
		return;
	}

	g_GenOsMsgParams.components[compID].subComponents[subCompID].
	    bAssertEnabled = bEnable;
}

VOID GENOS_CompAssertEnableDisable(GENOS_COMPONENT_ID compID, BOOL bEnable)
{
	if (compID >= GENOS_COMPONENT_COUNT) {
		GENOS_OS_ASSERTMESSAGE("Invalid component %d.", compID);
		return;
	}

	g_GenOsMsgParams.components[compID].component.bAssertEnabled = bEnable;
}

VOID GENOS_MessageInitComponent(GENOS_COMPONENT_ID compID)
{
	GENOS_STATUS eStatus = GENOS_STATUS_SUCCESS;
	GENOS_USER_FEATURE UserFeature;
	GENOS_USER_FEATURE_VALUE UserFeatureValue =
	    { 0, GENOS_USER_FEATURE_VALUE_TYPE_INVALID, {0}
	};
	UINT32 uiCompRegSetting;
	UINT64 uiSubCompRegSetting;
	UINT8 i;
	PCCHAR pcMessageKey = NULL;
	PCCHAR pcBySubComponentsKey = NULL;
	PCCHAR pcSubComponentsKey = NULL;

	if (compID >= GENOS_COMPONENT_COUNT) {
		GENOS_OS_ASSERTMESSAGE("Invalid component %d.", compID);
		return;
	}

	pcMessageKey = pcComponentRegKeys[compID][0];
	pcBySubComponentsKey = pcComponentRegKeys[compID][1];
	pcSubComponentsKey = pcComponentRegKeys[compID][2];

	UserFeatureValue.u32Data =
	    __GENOS_USER_FEATURE_KEY_MESSAGE_DEFAULT_VALUE;
	UserFeature.Type = GENOS_USER_FEATURE_TYPE_USER;
	UserFeature.pPath = __MEDIA_REGISTRY_SUBKEY_INTERNAL;
	UserFeature.pValues = &UserFeatureValue;
	UserFeature.uiNumValues = 1;

	eStatus = GENOS_UserFeature_ReadValue(NULL,
					      &UserFeature,
					      pcMessageKey,
					      GENOS_USER_FEATURE_VALUE_TYPE_UINT32);

	if (eStatus == GENOS_STATUS_READ_REGISTRY_FAILED) {
		GENOS_UserFeature_WriteValues(NULL, &UserFeature);
	}

	uiCompRegSetting = UserFeatureValue.u32Data;

	GENOS_SetCompMessageLevel(compID,
				  (GENOS_MESSAGE_LEVEL) (uiCompRegSetting &
							 0x7));
	GENOS_CompAssertEnableDisable(compID, (uiCompRegSetting >> 3) & 0x1);

	UserFeatureValue.bData = FALSE;

	eStatus = GENOS_UserFeature_ReadValue(NULL,
					      &UserFeature,
					      pcBySubComponentsKey,
					      GENOS_USER_FEATURE_VALUE_TYPE_UINT32);
	if (eStatus == GENOS_STATUS_READ_REGISTRY_FAILED) {
		GENOS_UserFeature_WriteValues(NULL, &UserFeature);
	}

	g_GenOsMsgParams.components[compID].bBySubComponent =
	    UserFeatureValue.bData;

	if (g_GenOsMsgParams.components[compID].bBySubComponent) {
		UserFeatureValue.u64Data = 0;

		eStatus = GENOS_UserFeature_ReadValue(NULL,
						      &UserFeature,
						      pcSubComponentsKey,
						      GENOS_USER_FEATURE_VALUE_TYPE_UINT64);

		if (eStatus == GENOS_STATUS_READ_REGISTRY_FAILED) {
			GENOS_UserFeature_WriteValues(NULL, &UserFeature);
		}

		uiSubCompRegSetting = UserFeatureValue.u64Data;

		for (i = 0; i < subComponentCount[compID]; i++) {
			GENOS_SetSubCompMessageLevel(compID, i,
						     (GENOS_MESSAGE_LEVEL)
						     (uiSubCompRegSetting &
						      0x7));
			GENOS_SubCompAssertEnableDisable(compID, i,
							 (uiSubCompRegSetting >>
							  3) & 0x1);

			uiSubCompRegSetting = (uiSubCompRegSetting >> 4);
		}
	}
}

GENOS_STATUS GENOS_HLTInit()
{
	UINT32 nPID = 0;
	CHAR hltFileName[GENOS_MAX_HLT_FILENAME_LEN] = { 0 };
	GENOS_USER_FEATURE UserFeature;
	GENOS_USER_FEATURE_VALUE UserFeatureValue =
	    { 0, GENOS_USER_FEATURE_VALUE_TYPE_INVALID, {0}
	};
	CHAR fileNamePrefix[GENOS_MAX_HLT_FILENAME_LEN];
	BOOL bUseHybridLogTrace = FALSE;
	CHAR cDDIDumpFilePath[GENOS_MAX_HLT_FILENAME_LEN] = { 0 };
	CHAR cStringData[GENOS_USER_CONTROL_MAX_DATA_SIZE];
	PFILE pDDILogFile;
	GENOS_STATUS eStatus;

	if (g_GenOsMsgParams.uiCounter != 0) {
		GENOS_OS_NORMALMESSAGE("HLT settings already set.");
		return GENOS_STATUS_UNKNOWN;
	}

	g_GenOsMsgParams.bUseHybridLogTrace = FALSE;
	g_GenOsMsgParams.pLogFile = NULL;
	g_GenOsMsgParams.pTraceFile = NULL;

	g_GenOsMsgParams_DDI_Dump.bUseHybridLogTrace = FALSE;
	g_GenOsMsgParams_DDI_Dump.pLogFile = NULL;
	g_GenOsMsgParams_DDI_Dump.pTraceFile = NULL;

	UserFeatureValue.bData = FALSE;
	UserFeature.Type = GENOS_USER_FEATURE_TYPE_USER;
	UserFeature.pPath = __MEDIA_REGISTRY_SUBKEY_INTERNAL;
	UserFeature.pValues = &UserFeatureValue;
	UserFeature.uiNumValues = 1;

	eStatus = GENOS_UserFeature_ReadValue(NULL,
					      &UserFeature,
					      __GENOS_USER_FEATURE_KEY_MESSAGE_HLT_ENABLED,
					      GENOS_USER_FEATURE_VALUE_TYPE_UINT32);

	if (eStatus == GENOS_STATUS_READ_REGISTRY_FAILED) {
		GENOS_UserFeature_WriteValues(NULL, &UserFeature);
	}

	bUseHybridLogTrace = UserFeatureValue.bData;

	g_GenOsMsgParams.bEnableMaps = 0;

	if (!bUseHybridLogTrace) {
		GENOS_OS_NORMALMESSAGE("HLT not enabled.");
		return GENOS_STATUS_SUCCESS;
	}

	nPID = GENOS_GetPid();

	GENOS_LogFileNamePrefix(fileNamePrefix);
	GENOS_SecureStringPrint(hltFileName, GENOS_MAX_HLT_FILENAME_LEN,
				GENOS_MAX_HLT_FILENAME_LEN - 1,
				GenOsLogPathTemplate, fileNamePrefix, nPID,
				"log");

	eStatus = GENOS_CreateDirectory(fileNamePrefix);
	if (GENOS_FAILED(eStatus)) {
		GENOS_OS_NORMALMESSAGE
		    ("Failed to create output directory. Status = %d", eStatus);
	}

	eStatus =
	    GENOS_SecureFileOpen(&g_GenOsMsgParams.pLogFile, hltFileName, "w");

	if (GENOS_FAILED(eStatus)) {
		GENOS_OS_NORMALMESSAGE("Failed to open log file '%s'.",
				       hltFileName);
		g_GenOsMsgParams.pLogFile = NULL;
	}

	if (g_GenOsMsgParams.pLogFile == NULL) {
		return GENOS_STATUS_HLT_INIT_FAILED;
	}
	g_GenOsMsgParams.bUseHybridLogTrace = TRUE;

	GENOS_HltpPreface(g_GenOsMsgParams.pLogFile);
	GENOS_OS_NORMALMESSAGE("HLT initialized successfuly (%s).",
			       hltFileName);

	GENOS_SecureStringPrint(hltFileName, GENOS_MAX_HLT_FILENAME_LEN,
				GENOS_MAX_HLT_FILENAME_LEN - 1,
				GenOsLogPathTemplate, fileNamePrefix, nPID,
				"hlt");

	eStatus =
	    GENOS_SecureFileOpen(&g_GenOsMsgParams.pTraceFile, hltFileName,
				 "w");

	if (GENOS_FAILED(eStatus)) {
		GENOS_OS_NORMALMESSAGE("Failed to open trace file '%s'.",
				       hltFileName);
	}
	GENOS_ZeroMemory(&UserFeatureValue, sizeof(UserFeatureValue));
	UserFeature.Type = GENOS_USER_FEATURE_TYPE_USER;
	UserFeature.pPath = __MEDIA_REGISTRY_SUBKEY_INTERNAL;
	UserFeature.pValues = &UserFeatureValue;
	UserFeature.uiNumValues = 1;

	UserFeatureValue.StringData.pStringData = cStringData;
	UserFeatureValue.StringData.uMaxSize = GENOS_USER_CONTROL_MAX_DATA_SIZE;
	UserFeatureValue.StringData.uSize = 0;
	eStatus = GENOS_UserFeature_ReadValue(NULL,
					      &UserFeature,
					      __MEDIA_REGISTRY_VALUE_DDI_DUMP_DIRECTORY,
					      GENOS_USER_FEATURE_VALUE_TYPE_STRING);

	GENOS_SecureStringPrint(cDDIDumpFilePath, GENOS_MAX_HLT_FILENAME_LEN,
				GENOS_MAX_HLT_FILENAME_LEN - 1,
				DDILogPathTemplate,
				(UserFeatureValue.StringData.uSize >
				 0) ? UserFeatureValue.StringData.
				pStringData : fileNamePrefix, nPID, "log");

	eStatus =
	    GENOS_SecureFileOpen(&g_GenOsMsgParams_DDI_Dump.pLogFile,
				 cDDIDumpFilePath, "w");
	if (GENOS_FAILED(eStatus)) {
		GENOS_OS_NORMALMESSAGE("Failed to open log file '%s'.",
				       cDDIDumpFilePath);
		g_GenOsMsgParams_DDI_Dump.pLogFile = NULL;
	}
	return GENOS_STATUS_SUCCESS;
}

VOID GENOS_MessageInit()
{
	UINT8 i;
	GENOS_USER_FEATURE UserFeature;
	GENOS_USER_FEATURE_VALUE UserFeatureValue =
	    { 0, GENOS_USER_FEATURE_VALUE_TYPE_INVALID, {0}
	};
	GENOS_STATUS eStatus = GENOS_STATUS_SUCCESS;

	if (g_GenOsMsgParams.uiCounter == 0) {
		GenOsMemAllocCounter = 0;

		GENOS_SetCompMessageLevelAll(GENOS_MESSAGE_LVL_CRITICAL);

		for (i = 0; i < GENOS_COMPONENT_COUNT; i++) {
			GENOS_MessageInitComponent((GENOS_COMPONENT_ID) i);
		}

		GENOS_ZeroMemory(&UserFeatureValue, sizeof(UserFeatureValue));
		UserFeatureValue.bData = TRUE;
		UserFeature.Type = GENOS_USER_FEATURE_TYPE_USER;
		UserFeature.pPath = __MEDIA_REGISTRY_SUBKEY_INTERNAL;
		UserFeature.pValues = &UserFeatureValue;
		UserFeature.uiNumValues = 1;

		eStatus = GENOS_UserFeature_ReadValue(NULL,
						      &UserFeature,
						      __GENOS_USER_FEATURE_KEY_MESSAGE_PRINT_ENABLED,
						      GENOS_USER_FEATURE_VALUE_TYPE_INT32);

		if (eStatus == GENOS_STATUS_READ_REGISTRY_FAILED) {
			GENOS_UserFeature_WriteValues(NULL, &UserFeature);
		}

		g_GenOsMsgParams.bUseOutputDebugString = UserFeatureValue.bData;

		GENOS_HLTInit();
	}
	g_GenOsMsgParams.uiCounter++;

}

VOID GENOS_HLTClose()
{
	if (g_GenOsMsgParams.pTraceFile != NULL) {
		fclose(g_GenOsMsgParams.pTraceFile);
		g_GenOsMsgParams.pTraceFile = NULL;

		GENOS_OS_NORMALMESSAGE("Trace file is closed.");
	}

	if (g_GenOsMsgParams.pLogFile != NULL) {
		GENOS_OS_NORMALMESSAGE
		    ("Log file is closing, total services %d.",
		     g_GenOsMsgParams.uiCounter);

		fclose(g_GenOsMsgParams.pLogFile);
		g_GenOsMsgParams.pLogFile = NULL;
	}

	if (g_GenOsMsgParams_DDI_Dump.pLogFile != NULL) {
		fclose(g_GenOsMsgParams_DDI_Dump.pLogFile);
		g_GenOsMsgParams_DDI_Dump.pLogFile = NULL;

		GENOS_OS_NORMALMESSAGE("DDI dump file closed.");
	}
}

VOID GENOS_MessageClose()
{
	if (g_GenOsMsgParams.uiCounter == 1) {
		GENOS_HLTClose();
		GENOS_ZeroMemory(&g_GenOsMsgParams,
				 sizeof(GENOS_MESSAGE_PARAMS));
	} else {
		g_GenOsMsgParams.uiCounter--;
	}
}

BOOL GENOS_ShouldPrintMessage(GENOS_MESSAGE_LEVEL level,
			      GENOS_COMPONENT_ID compID,
			      UINT8 subCompID, const PCCHAR message)
{
	if (message == NULL) {
		return FALSE;
	}

	if (compID >= GENOS_COMPONENT_COUNT ||
	    subCompID >= GENOS_MAX_SUBCOMPONENT_COUNT) {
		return FALSE;
	}
	if (g_GenOsMsgParams.components[compID].component.uiMessageLevel <
	    level) {
		return FALSE;
	}

	if (g_GenOsMsgParams.components[compID].bBySubComponent &&
	    g_GenOsMsgParams.components[compID].subComponents[subCompID].
	    uiMessageLevel < level) {
		return FALSE;
	}

	return TRUE;

}

#if GENOS_ASSERT_ENABLED
BOOL GENOS_ShouldAssert(GENOS_COMPONENT_ID compID, UINT8 subCompID)
{
	if (compID >= GENOS_COMPONENT_COUNT ||
	    subCompID >= GENOS_MAX_SUBCOMPONENT_COUNT) {
		return FALSE;
	}

	if (!g_GenOsMsgParams.components[compID].component.bAssertEnabled) {
		return FALSE;
	}

	if (g_GenOsMsgParams.components[compID].bBySubComponent &&
	    !g_GenOsMsgParams.components[compID].subComponents[subCompID].
	    bAssertEnabled) {
		return FALSE;
	}

	return TRUE;

}
#endif

#endif
