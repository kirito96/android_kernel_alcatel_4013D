/*****************************************************************************
 *
 * Filename:
 * ---------
 *   CFG_file_info_custom.h
 *
 * Project:
 * --------
 *   YuSu
 *
 * Description:
 * ------------
 *   Configuration File List for Customer
 *
 *
 * Author:
 * -------
 *   Nick Huang (mtk02183)
 *
 ****************************************************************************/

#ifndef __CFG_FILE_INFO_CUSTOM_H__
#define __CFG_FILE_INFO_CUSTOM_H__

#include "CFG_file_public.h"
#include "CFG_file_lid.h"
#include "Custom_NvRam_LID.h"
#include "CFG_AUDIO_File.h"
#include "CFG_Audio_Default.h"
#include "CFG_GPS_File.h"
#include "CFG_GPS_Default.h"
#include "CFG_Wifi_File.h"
#include "CFG_WIFI_Default.h"
#include "CFG_PRODUCT_INFO_File.h"
#include "CFG_PRODUCT_INFO_Default.h"
#ifdef JRD_AUDIO_PARAMS_CUST
#include "audio/mt6572_phone_paras_cust/EU_HD/CFG_Audio_Param_Cust_EU_HD.h"
#include "audio/mt6572_phone_paras_cust/US_receiver/CFG_Audio_Param_Cust_US_receiver.h"
#include "audio/mt6572_phone_paras_cust/O2_UK/CFG_Audio_Param_Cust_O2_UK.h"  //Add by fangjie for PR942640
#include "audio/mt6572_phone_paras_cust/US_Relook/CFG_Audio_Param_Cust_US_Relook.h"  //Add by ersen.shang for PR1005561
#endif
#include <stdio.h>
#ifdef __cplusplus
extern "C"
{
#endif
#ifdef JRD_AUDIO_PARAMS_CUST
    const TCFG_FILE g_akCFG_File_Custom1[]=
    {
        {
            "/data/nvram/APCFG/APRDCL/Audio_Sph",       VER(AP_CFG_RDCL_FILE_AUDIO_LID),         CFG_FILE_SPEECH_REC_SIZE,
            CFG_FILE_SPEECH_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                                   (char *)&speech_custom_default, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDEB/GPS",         VER(AP_CFG_CUSTOM_FILE_GPS_LID),	           CFG_FILE_GPS_CONFIG_SIZE,
            CFG_FILE_GPS_CONFIG_TOTAL,                  SIGNLE_DEFUALT_REC,                (char *)&stGPSConfigDefault, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_CompFlt",       VER(AP_CFG_RDCL_FILE_AUDIO_COMPFLT_LID),         CFG_FILE_AUDIO_COMPFLT_REC_SIZE,
            CFG_FILE_AUDIO_COMPFLT_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_custom_default, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Effect",       VER(AP_CFG_RDCL_FILE_AUDIO_EFFECT_LID),         CFG_FILE_AUDIO_EFFECT_REC_SIZE,
            CFG_FILE_AUDIO_EFFECT_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_effect_custom_default, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDEB/WIFI",	    	VER(AP_CFG_RDEB_FILE_WIFI_LID),		    CFG_FILE_WIFI_REC_SIZE,
            CFG_FILE_WIFI_REC_TOTAL,		    	SIGNLE_DEFUALT_REC,				    (char *)&stWifiCfgDefault, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDEB/WIFI_CUSTOM",	VER(AP_CFG_RDEB_WIFI_CUSTOM_LID),	CFG_FILE_WIFI_CUSTOM_REC_SIZE,
            CFG_FILE_WIFI_CUSTOM_REC_TOTAL,		    SIGNLE_DEFUALT_REC,				    (char *)&stWifiCustomDefault, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Sph_Med",       VER(AP_CFG_RDCL_FILE_AUDIO_PARAM_MED_LID),         CFG_FILE_AUDIO_PARAM_MED_REC_SIZE,
            CFG_FILE_AUDIO_PARAM_MED_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_param_med_default, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Vol_custom",       VER(AP_CFG_RDCL_FILE_AUDIO_VOLUME_CUSTOM_LID),         CFG_FILE_AUDIO_VOLUME_CUSTOM_REC_SIZE,
            CFG_FILE_AUDIO_VOLUME_CUSTOM_REC_TOTAL,           SIGNLE_DEFUALT_REC,                (char *)&audio_volume_custom_default, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Sph_Dual_Mic",       VER(AP_CFG_RDCL_FILE_DUAL_MIC_CUSTOM_LID),         CFG_FILE_SPEECH_DUAL_MIC_SIZE,
            CFG_FILE_SPEECH_DUAL_MIC_TOTAL,           SIGNLE_DEFUALT_REC,                (char *)&dual_mic_custom_default, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Wb_Sph",       VER(AP_CFG_RDCL_FILE_AUDIO_WB_PARAM_LID),         CFG_FILE_WB_SPEECH_REC_SIZE,
            CFG_FILE_WB_SPEECH_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                                   (char *)&wb_speech_custom_default, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDEB/PRODUCT_INFO",       VER(AP_CFG_REEB_PRODUCT_INFO_LID),         CFG_FILE_PRODUCT_INFO_SIZE,
            CFG_FILE_PRODUCT_INFO_TOTAL,                   SIGNLE_DEFUALT_REC,                                   (char *)&stPRODUCT_INFOConfigDefault,DataReset, NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Headphone_CompFlt",       VER(AP_CFG_RDCL_FILE_HEADPHONE_COMPFLT_LID),         CFG_FILE_AUDIO_COMPFLT_REC_SIZE,
            CFG_FILE_HEADPHONE_COMPFLT_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_hcf_custom_default, DataReset , NULL
        },

        { "/data/nvram/APCFG/APRDCL/Audio_gain_table",   VER(AP_CFG_RDCL_FILE_AUDIO_GAIN_TABLE_LID), CFG_FILE_AUDIO_GAIN_TABLE_CUSTOM_REC_SIZE,
            CFG_FILE_AUDIO_GAIN_TABLE_CUSTOM_REC_TOTAL, SIGNLE_DEFUALT_REC  ,	 (char *)&Gain_control_table_default, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_ver1_Vol_custom",       VER(AP_CFG_RDCL_FILE_AUDIO_VER1_VOLUME_CUSTOM_LID),         CFG_FILE_AUDIO_VER1_VOLUME_CUSTOM_REC_SIZE,
            CFG_FILE_AUDIO_VER1_VOLUME_CUSTOM_REC_TOTAL,           SIGNLE_DEFUALT_REC,                (char *)&audio_ver1_custom_default, DataReset , NULL
        },

        { "/data/nvram/APCFG/APRDCL/Audio_Hd_Record_Param",   VER(AP_CFG_RDCL_FILE_AUDIO_HD_REC_PAR_LID), CFG_FILE_AUDIO_HD_REC_PAR_SIZE,
            CFG_FILE_AUDIO_HD_REC_PAR_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Hd_Recrod_Par_default, DataReset , NULL
        },

        { "/data/nvram/APCFG/APRDCL/Audio_Hd_Record_Scene_Table",   VER(AP_CFG_RDCL_FILE_AUDIO_HD_REC_SCENE_LID), CFG_FILE_AUDIO_HD_REC_SCENE_TABLE_SIZE,
            CFG_FILE_AUDIO_HD_REC_SCENE_TABLE_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Hd_Recrod_Scene_Table_default, DataReset , NULL
        },

		{ "/data/nvram/APCFG/APRDCL/Audio_Hd_Record_48k_Param",   VER(AP_CFG_RDCL_FILE_AUDIO_HD_REC_48K_PAR_LID), CFG_FILE_AUDIO_HD_REC_48K_PAR_SIZE,
            CFG_FILE_AUDIO_HD_REC_48K_PAR_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Hd_Recrod_48k_Par_default, DataReset , NULL
        },

        { "/data/nvram/APCFG/APRDCL/Audio_Buffer_DC_Calibration_Param",   VER(AP_CFG_RDCL_FILE_AUDIO_BUFFER_DC_CALIBRATION_PAR_LID), CFG_FILE_AUDIO_BUFFER_DC_CALIBRATION_PAR_SIZE,
            CFG_FILE_AUDIO_BUFFER_DC_CALIBRATION_PAR_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Audio_Buffer_DC_Calibration_Par_default, DataReset , NULL
        },
        { "/data/nvram/APCFG/APRDCL/VibSpk_CompFlt",   VER(AP_CFG_RDCL_FILE_VIBSPK_COMPFLT_LID), CFG_FILE_AUDIO_COMPFLT_REC_SIZE,
            CFG_FILE_VIBSPK_COMPFLT_REC_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&audio_vibspk_custom_default, DataReset , NULL
        },
    };
/* Add another audio parameters set here.
      * The size of g_akCFG_File_Custom1 & g_akCFG_File_Custom2 must be the same.*/
    /*
      * For example:
      * Assume "/data/nvram/APCFG/APRDCL/Audio_Sph" is different.
      * 1. Change pDefaultValue of g_akCFG_File_Custom2[0] to "speech_custom_default2";
      * 2. Add an "AUDIO_CUSTOM_PARAM_STRUCT speech_custom_default2" in CFG_Audio_Default.h;
      * 3. Change the values in speech_custom_default2 which are different with speech_custom_default.*/
    const TCFG_FILE g_akCFG_File_Custom_eu_hd[]=
    {
        {
            "/data/nvram/APCFG/APRDCL/Audio_Sph",       VER(AP_CFG_RDCL_FILE_AUDIO_LID),         CFG_FILE_SPEECH_REC_SIZE,
            CFG_FILE_SPEECH_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                                   (char *)&speech_custom_default_cust_eu_hd, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDEB/GPS",         VER(AP_CFG_CUSTOM_FILE_GPS_LID),	           CFG_FILE_GPS_CONFIG_SIZE,
            CFG_FILE_GPS_CONFIG_TOTAL,                  SIGNLE_DEFUALT_REC,                (char *)&stGPSConfigDefault, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_CompFlt",       VER(AP_CFG_RDCL_FILE_AUDIO_COMPFLT_LID),         CFG_FILE_AUDIO_COMPFLT_REC_SIZE,
            CFG_FILE_AUDIO_COMPFLT_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_custom_default_cust_eu_hd, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Effect",       VER(AP_CFG_RDCL_FILE_AUDIO_EFFECT_LID),         CFG_FILE_AUDIO_EFFECT_REC_SIZE,
            CFG_FILE_AUDIO_EFFECT_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_effect_custom_default_cust_eu_hd, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDEB/WIFI",	    	VER(AP_CFG_RDEB_FILE_WIFI_LID),		    CFG_FILE_WIFI_REC_SIZE,
            CFG_FILE_WIFI_REC_TOTAL,		    	SIGNLE_DEFUALT_REC,				    (char *)&stWifiCfgDefault, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDEB/WIFI_CUSTOM",	VER(AP_CFG_RDEB_WIFI_CUSTOM_LID),	CFG_FILE_WIFI_CUSTOM_REC_SIZE,
            CFG_FILE_WIFI_CUSTOM_REC_TOTAL,		    SIGNLE_DEFUALT_REC,				    (char *)&stWifiCustomDefault, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Sph_Med",       VER(AP_CFG_RDCL_FILE_AUDIO_PARAM_MED_LID),         CFG_FILE_AUDIO_PARAM_MED_REC_SIZE,
            CFG_FILE_AUDIO_PARAM_MED_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_param_med_default_cust_eu_hd, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Vol_custom",       VER(AP_CFG_RDCL_FILE_AUDIO_VOLUME_CUSTOM_LID),         CFG_FILE_AUDIO_VOLUME_CUSTOM_REC_SIZE,
            CFG_FILE_AUDIO_VOLUME_CUSTOM_REC_TOTAL,           SIGNLE_DEFUALT_REC,                (char *)&audio_volume_custom_default_cust_eu_hd, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Sph_Dual_Mic",       VER(AP_CFG_RDCL_FILE_DUAL_MIC_CUSTOM_LID),         CFG_FILE_SPEECH_DUAL_MIC_SIZE,
            CFG_FILE_SPEECH_DUAL_MIC_TOTAL,           SIGNLE_DEFUALT_REC,                (char *)&dual_mic_custom_default_cust_eu_hd, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Wb_Sph",       VER(AP_CFG_RDCL_FILE_AUDIO_WB_PARAM_LID),         CFG_FILE_WB_SPEECH_REC_SIZE,
            CFG_FILE_WB_SPEECH_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                                   (char *)&wb_speech_custom_default_cust_eu_hd, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDEB/PRODUCT_INFO",       VER(AP_CFG_REEB_PRODUCT_INFO_LID),         CFG_FILE_PRODUCT_INFO_SIZE,
            CFG_FILE_PRODUCT_INFO_TOTAL,                   SIGNLE_DEFUALT_REC,                                   (char *)&stPRODUCT_INFOConfigDefault,DataReset, NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Headphone_CompFlt",       VER(AP_CFG_RDCL_FILE_HEADPHONE_COMPFLT_LID),         CFG_FILE_AUDIO_COMPFLT_REC_SIZE,
            CFG_FILE_HEADPHONE_COMPFLT_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_hcf_custom_default_cust_eu_hd, DataReset , NULL
        },

        { "/data/nvram/APCFG/APRDCL/Audio_gain_table",   VER(AP_CFG_RDCL_FILE_AUDIO_GAIN_TABLE_LID), CFG_FILE_AUDIO_GAIN_TABLE_CUSTOM_REC_SIZE,
            CFG_FILE_AUDIO_GAIN_TABLE_CUSTOM_REC_TOTAL, SIGNLE_DEFUALT_REC  ,	 (char *)&Gain_control_table_default_cust_eu_hd, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_ver1_Vol_custom",       VER(AP_CFG_RDCL_FILE_AUDIO_VER1_VOLUME_CUSTOM_LID),         CFG_FILE_AUDIO_VER1_VOLUME_CUSTOM_REC_SIZE,
            CFG_FILE_AUDIO_VER1_VOLUME_CUSTOM_REC_TOTAL,           SIGNLE_DEFUALT_REC,                (char *)&audio_ver1_custom_default_cust_eu_hd, DataReset , NULL
        },
 
        { "/data/nvram/APCFG/APRDCL/Audio_Hd_Record_Param",   VER(AP_CFG_RDCL_FILE_AUDIO_HD_REC_PAR_LID), CFG_FILE_AUDIO_HD_REC_PAR_SIZE,
            CFG_FILE_AUDIO_HD_REC_PAR_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Hd_Recrod_Par_default_cust_eu_hd, DataReset , NULL
        },

        { "/data/nvram/APCFG/APRDCL/Audio_Hd_Record_Scene_Table",   VER(AP_CFG_RDCL_FILE_AUDIO_HD_REC_SCENE_LID), CFG_FILE_AUDIO_HD_REC_SCENE_TABLE_SIZE,
            CFG_FILE_AUDIO_HD_REC_SCENE_TABLE_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Hd_Recrod_Scene_Table_default_cust_eu_hd, DataReset , NULL
        },

		{ "/data/nvram/APCFG/APRDCL/Audio_Hd_Record_48k_Param",   VER(AP_CFG_RDCL_FILE_AUDIO_HD_REC_48K_PAR_LID), CFG_FILE_AUDIO_HD_REC_48K_PAR_SIZE,
            CFG_FILE_AUDIO_HD_REC_48K_PAR_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Hd_Recrod_48k_Par_default_cust_eu_hd, DataReset , NULL
        },

        { "/data/nvram/APCFG/APRDCL/Audio_Buffer_DC_Calibration_Param",   VER(AP_CFG_RDCL_FILE_AUDIO_BUFFER_DC_CALIBRATION_PAR_LID), CFG_FILE_AUDIO_BUFFER_DC_CALIBRATION_PAR_SIZE,
            CFG_FILE_AUDIO_BUFFER_DC_CALIBRATION_PAR_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Audio_Buffer_DC_Calibration_Par_default_cust_eu_hd, DataReset , NULL
        },
        { "/data/nvram/APCFG/APRDCL/VibSpk_CompFlt",   VER(AP_CFG_RDCL_FILE_VIBSPK_COMPFLT_LID), CFG_FILE_AUDIO_COMPFLT_REC_SIZE,
            CFG_FILE_VIBSPK_COMPFLT_REC_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&audio_vibspk_custom_default_cust_eu_hd, DataReset , NULL
        },
    };
const TCFG_FILE g_akCFG_File_Custom_us_receiver[]=
{
	{
		"/data/nvram/APCFG/APRDCL/Audio_Sph",		VER(AP_CFG_RDCL_FILE_AUDIO_LID),		 CFG_FILE_SPEECH_REC_SIZE,
		CFG_FILE_SPEECH_REC_TOTAL,					 SIGNLE_DEFUALT_REC,								   (char *)&speech_custom_default_cust_us_receiver, DataReset , NULL
	},

	{
		"/data/nvram/APCFG/APRDEB/GPS", 		VER(AP_CFG_CUSTOM_FILE_GPS_LID),			   CFG_FILE_GPS_CONFIG_SIZE,
		CFG_FILE_GPS_CONFIG_TOTAL,					SIGNLE_DEFUALT_REC, 			   (char *)&stGPSConfigDefault, DataReset , NULL
	},

	{
		"/data/nvram/APCFG/APRDCL/Audio_CompFlt",		VER(AP_CFG_RDCL_FILE_AUDIO_COMPFLT_LID),		 CFG_FILE_AUDIO_COMPFLT_REC_SIZE,
		CFG_FILE_AUDIO_COMPFLT_REC_TOTAL,					SIGNLE_DEFUALT_REC, 			   (char *)&audio_custom_default_cust_us_receiver, DataReset , NULL
	},

	{
		"/data/nvram/APCFG/APRDCL/Audio_Effect",	   VER(AP_CFG_RDCL_FILE_AUDIO_EFFECT_LID),		   CFG_FILE_AUDIO_EFFECT_REC_SIZE,
		CFG_FILE_AUDIO_EFFECT_REC_TOTAL,				   SIGNLE_DEFUALT_REC,				  (char *)&audio_effect_custom_default_cust_us_receiver, DataReset , NULL
	},

	{
		"/data/nvram/APCFG/APRDEB/WIFI",			VER(AP_CFG_RDEB_FILE_WIFI_LID), 		CFG_FILE_WIFI_REC_SIZE,
		/*BEGIN: modify by fangjie for PR884947,  US branch change the wifi  b/g mode tx power, EU branch keep default.*/
		//old: CFG_FILE_WIFI_REC_TOTAL,				SIGNLE_DEFUALT_REC, 				(char *)&stWifiCfgDefault, DataReset , NULL
		CFG_FILE_WIFI_REC_TOTAL,				SIGNLE_DEFUALT_REC, 				(char *)&stWifiCfgDefault_us, DataReset , NULL
		/*END: modify by fangjie for PR884947,  US branch change the wifi  b/g mode tx power, EU branch keep default.*/

	},

	{
		"/data/nvram/APCFG/APRDEB/WIFI_CUSTOM", VER(AP_CFG_RDEB_WIFI_CUSTOM_LID),	CFG_FILE_WIFI_CUSTOM_REC_SIZE,
		CFG_FILE_WIFI_CUSTOM_REC_TOTAL, 		SIGNLE_DEFUALT_REC, 				(char *)&stWifiCustomDefault, DataReset , NULL
	},

	{
		"/data/nvram/APCFG/APRDCL/Audio_Sph_Med",		VER(AP_CFG_RDCL_FILE_AUDIO_PARAM_MED_LID),		   CFG_FILE_AUDIO_PARAM_MED_REC_SIZE,
		CFG_FILE_AUDIO_PARAM_MED_REC_TOTAL, 				  SIGNLE_DEFUALT_REC,				 (char *)&audio_param_med_default_cust_us_receiver, DataReset , NULL
	},

	{
		"/data/nvram/APCFG/APRDCL/Audio_Vol_custom",	   VER(AP_CFG_RDCL_FILE_AUDIO_VOLUME_CUSTOM_LID),		  CFG_FILE_AUDIO_VOLUME_CUSTOM_REC_SIZE,
		CFG_FILE_AUDIO_VOLUME_CUSTOM_REC_TOTAL, 		  SIGNLE_DEFUALT_REC,				 (char *)&audio_volume_custom_default_cust_us_receiver, DataReset , NULL
	},

	{
		"/data/nvram/APCFG/APRDCL/Sph_Dual_Mic",	   VER(AP_CFG_RDCL_FILE_DUAL_MIC_CUSTOM_LID),		  CFG_FILE_SPEECH_DUAL_MIC_SIZE,
		CFG_FILE_SPEECH_DUAL_MIC_TOTAL, 		  SIGNLE_DEFUALT_REC,				 (char *)&dual_mic_custom_default_cust_us_receiver, DataReset , NULL
	},

	{
		"/data/nvram/APCFG/APRDCL/Audio_Wb_Sph",	   VER(AP_CFG_RDCL_FILE_AUDIO_WB_PARAM_LID),		 CFG_FILE_WB_SPEECH_REC_SIZE,
		CFG_FILE_WB_SPEECH_REC_TOTAL,					SIGNLE_DEFUALT_REC, 								  (char *)&wb_speech_custom_default_cust_us_receiver, DataReset , NULL
	},

	{
		"/data/nvram/APCFG/APRDEB/PRODUCT_INFO",	   VER(AP_CFG_REEB_PRODUCT_INFO_LID),		  CFG_FILE_PRODUCT_INFO_SIZE,
		CFG_FILE_PRODUCT_INFO_TOTAL,				   SIGNLE_DEFUALT_REC,									 (char *)&stPRODUCT_INFOConfigDefault,DataReset, NULL
	},

	{
		"/data/nvram/APCFG/APRDCL/Headphone_CompFlt",		VER(AP_CFG_RDCL_FILE_HEADPHONE_COMPFLT_LID),		 CFG_FILE_AUDIO_COMPFLT_REC_SIZE,
		CFG_FILE_HEADPHONE_COMPFLT_REC_TOTAL,					SIGNLE_DEFUALT_REC, 			   (char *)&audio_hcf_custom_default_cust_us_receiver, DataReset , NULL
	},

	{ "/data/nvram/APCFG/APRDCL/Audio_gain_table",	 VER(AP_CFG_RDCL_FILE_AUDIO_GAIN_TABLE_LID), CFG_FILE_AUDIO_GAIN_TABLE_CUSTOM_REC_SIZE,
		CFG_FILE_AUDIO_GAIN_TABLE_CUSTOM_REC_TOTAL, SIGNLE_DEFUALT_REC	,	 (char *)&Gain_control_table_default_cust_us_receiver, DataReset , NULL
	},

	{
		"/data/nvram/APCFG/APRDCL/Audio_ver1_Vol_custom",		VER(AP_CFG_RDCL_FILE_AUDIO_VER1_VOLUME_CUSTOM_LID), 		CFG_FILE_AUDIO_VER1_VOLUME_CUSTOM_REC_SIZE,
		CFG_FILE_AUDIO_VER1_VOLUME_CUSTOM_REC_TOTAL,		   SIGNLE_DEFUALT_REC,				  (char *)&audio_ver1_custom_default_cust_us_receiver, DataReset , NULL
	},

	{ "/data/nvram/APCFG/APRDCL/Audio_Hd_Record_Param",   VER(AP_CFG_RDCL_FILE_AUDIO_HD_REC_PAR_LID), CFG_FILE_AUDIO_HD_REC_PAR_SIZE,
		CFG_FILE_AUDIO_HD_REC_PAR_TOTAL, SIGNLE_DEFUALT_REC  ,	  (char *)&Hd_Recrod_Par_default_cust_us_receiver, DataReset , NULL
	},

	{ "/data/nvram/APCFG/APRDCL/Audio_Hd_Record_Scene_Table",	VER(AP_CFG_RDCL_FILE_AUDIO_HD_REC_SCENE_LID), CFG_FILE_AUDIO_HD_REC_SCENE_TABLE_SIZE,
		CFG_FILE_AUDIO_HD_REC_SCENE_TABLE_TOTAL, SIGNLE_DEFUALT_REC  ,	  (char *)&Hd_Recrod_Scene_Table_default_cust_us_receiver, DataReset , NULL
	},

	{ "/data/nvram/APCFG/APRDCL/Audio_Hd_Record_48k_Param",   VER(AP_CFG_RDCL_FILE_AUDIO_HD_REC_48K_PAR_LID), CFG_FILE_AUDIO_HD_REC_48K_PAR_SIZE,
		CFG_FILE_AUDIO_HD_REC_48K_PAR_TOTAL, SIGNLE_DEFUALT_REC  ,	  (char *)&Hd_Recrod_48k_Par_default_cust_us_receiver, DataReset , NULL
	},

	{ "/data/nvram/APCFG/APRDCL/Audio_Buffer_DC_Calibration_Param",   VER(AP_CFG_RDCL_FILE_AUDIO_BUFFER_DC_CALIBRATION_PAR_LID), CFG_FILE_AUDIO_BUFFER_DC_CALIBRATION_PAR_SIZE,
		CFG_FILE_AUDIO_BUFFER_DC_CALIBRATION_PAR_TOTAL, SIGNLE_DEFUALT_REC	,	 (char *)&Audio_Buffer_DC_Calibration_Par_default_cust_us_receiver, DataReset , NULL
	},
	{ "/data/nvram/APCFG/APRDCL/VibSpk_CompFlt",   VER(AP_CFG_RDCL_FILE_VIBSPK_COMPFLT_LID), CFG_FILE_AUDIO_COMPFLT_REC_SIZE,
		CFG_FILE_VIBSPK_COMPFLT_REC_TOTAL, SIGNLE_DEFUALT_REC  ,	(char *)&audio_vibspk_custom_default_cust_us_receiver, DataReset , NULL
	},
};

/* Add another audio parameters set here.*/
//BEGIN: Add by fangjie for PR942640, add O2 UK audio parameters
    const TCFG_FILE g_akCFG_File_Custom_O2_UK[]=
    {
        {
            "/data/nvram/APCFG/APRDCL/Audio_Sph",       VER(AP_CFG_RDCL_FILE_AUDIO_LID),         CFG_FILE_SPEECH_REC_SIZE,
            CFG_FILE_SPEECH_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                                   (char *)&speech_custom_default_cust_O2_UK, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDEB/GPS",         VER(AP_CFG_CUSTOM_FILE_GPS_LID),	           CFG_FILE_GPS_CONFIG_SIZE,
            CFG_FILE_GPS_CONFIG_TOTAL,                  SIGNLE_DEFUALT_REC,                (char *)&stGPSConfigDefault, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_CompFlt",       VER(AP_CFG_RDCL_FILE_AUDIO_COMPFLT_LID),         CFG_FILE_AUDIO_COMPFLT_REC_SIZE,
            CFG_FILE_AUDIO_COMPFLT_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_custom_default_cust_O2_UK, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Effect",       VER(AP_CFG_RDCL_FILE_AUDIO_EFFECT_LID),         CFG_FILE_AUDIO_EFFECT_REC_SIZE,
            CFG_FILE_AUDIO_EFFECT_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_effect_custom_default_cust_O2_UK, DataReset , NULL
        },

		/*begin ersen.shang 20150522 use us wifi config follow hw's requirment switch stWifiCfgDefault to stWifiCfgDefault_us*/
        {
            "/data/nvram/APCFG/APRDEB/WIFI",	    	VER(AP_CFG_RDEB_FILE_WIFI_LID),		    CFG_FILE_WIFI_REC_SIZE,
            CFG_FILE_WIFI_REC_TOTAL,		    	SIGNLE_DEFUALT_REC,				    (char *)&/*stWifiCfgDefault*/stWifiCfgDefault_us, DataReset , NULL
        },
		/*end   ersen.shang 20150522 use us wifi config follow hw's requirment switch stWifiCfgDefault to stWifiCfgDefault_us*/
			
        {
            "/data/nvram/APCFG/APRDEB/WIFI_CUSTOM",	VER(AP_CFG_RDEB_WIFI_CUSTOM_LID),	CFG_FILE_WIFI_CUSTOM_REC_SIZE,
            CFG_FILE_WIFI_CUSTOM_REC_TOTAL,		    SIGNLE_DEFUALT_REC,				    (char *)&stWifiCustomDefault, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Sph_Med",       VER(AP_CFG_RDCL_FILE_AUDIO_PARAM_MED_LID),         CFG_FILE_AUDIO_PARAM_MED_REC_SIZE,
            CFG_FILE_AUDIO_PARAM_MED_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_param_med_default_cust_O2_UK, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Vol_custom",       VER(AP_CFG_RDCL_FILE_AUDIO_VOLUME_CUSTOM_LID),         CFG_FILE_AUDIO_VOLUME_CUSTOM_REC_SIZE,
            CFG_FILE_AUDIO_VOLUME_CUSTOM_REC_TOTAL,           SIGNLE_DEFUALT_REC,                (char *)&audio_volume_custom_default_cust_O2_UK, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Sph_Dual_Mic",       VER(AP_CFG_RDCL_FILE_DUAL_MIC_CUSTOM_LID),         CFG_FILE_SPEECH_DUAL_MIC_SIZE,
            CFG_FILE_SPEECH_DUAL_MIC_TOTAL,           SIGNLE_DEFUALT_REC,                (char *)&dual_mic_custom_default_cust_O2_UK, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Wb_Sph",       VER(AP_CFG_RDCL_FILE_AUDIO_WB_PARAM_LID),         CFG_FILE_WB_SPEECH_REC_SIZE,
            CFG_FILE_WB_SPEECH_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                                   (char *)&wb_speech_custom_default_cust_O2_UK, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDEB/PRODUCT_INFO",       VER(AP_CFG_REEB_PRODUCT_INFO_LID),         CFG_FILE_PRODUCT_INFO_SIZE,
            CFG_FILE_PRODUCT_INFO_TOTAL,                   SIGNLE_DEFUALT_REC,                                   (char *)&stPRODUCT_INFOConfigDefault,DataReset, NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Headphone_CompFlt",       VER(AP_CFG_RDCL_FILE_HEADPHONE_COMPFLT_LID),         CFG_FILE_AUDIO_COMPFLT_REC_SIZE,
            CFG_FILE_HEADPHONE_COMPFLT_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_hcf_custom_default_cust_O2_UK, DataReset , NULL
        },

        { "/data/nvram/APCFG/APRDCL/Audio_gain_table",   VER(AP_CFG_RDCL_FILE_AUDIO_GAIN_TABLE_LID), CFG_FILE_AUDIO_GAIN_TABLE_CUSTOM_REC_SIZE,
            CFG_FILE_AUDIO_GAIN_TABLE_CUSTOM_REC_TOTAL, SIGNLE_DEFUALT_REC  ,	 (char *)&Gain_control_table_default_cust_O2_UK, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_ver1_Vol_custom",       VER(AP_CFG_RDCL_FILE_AUDIO_VER1_VOLUME_CUSTOM_LID),         CFG_FILE_AUDIO_VER1_VOLUME_CUSTOM_REC_SIZE,
            CFG_FILE_AUDIO_VER1_VOLUME_CUSTOM_REC_TOTAL,           SIGNLE_DEFUALT_REC,                (char *)&audio_ver1_custom_default_cust_O2_UK, DataReset , NULL
        },
 
        { "/data/nvram/APCFG/APRDCL/Audio_Hd_Record_Param",   VER(AP_CFG_RDCL_FILE_AUDIO_HD_REC_PAR_LID), CFG_FILE_AUDIO_HD_REC_PAR_SIZE,
            CFG_FILE_AUDIO_HD_REC_PAR_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Hd_Recrod_Par_default_cust_O2_UK, DataReset , NULL
        },

        { "/data/nvram/APCFG/APRDCL/Audio_Hd_Record_Scene_Table",   VER(AP_CFG_RDCL_FILE_AUDIO_HD_REC_SCENE_LID), CFG_FILE_AUDIO_HD_REC_SCENE_TABLE_SIZE,
            CFG_FILE_AUDIO_HD_REC_SCENE_TABLE_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Hd_Recrod_Scene_Table_default_cust_O2_UK, DataReset , NULL
        },

		{ "/data/nvram/APCFG/APRDCL/Audio_Hd_Record_48k_Param",   VER(AP_CFG_RDCL_FILE_AUDIO_HD_REC_48K_PAR_LID), CFG_FILE_AUDIO_HD_REC_48K_PAR_SIZE,
            CFG_FILE_AUDIO_HD_REC_48K_PAR_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Hd_Recrod_48k_Par_default_cust_O2_UK, DataReset , NULL
        },

        { "/data/nvram/APCFG/APRDCL/Audio_Buffer_DC_Calibration_Param",   VER(AP_CFG_RDCL_FILE_AUDIO_BUFFER_DC_CALIBRATION_PAR_LID), CFG_FILE_AUDIO_BUFFER_DC_CALIBRATION_PAR_SIZE,
            CFG_FILE_AUDIO_BUFFER_DC_CALIBRATION_PAR_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Audio_Buffer_DC_Calibration_Par_default_cust_O2_UK, DataReset , NULL
        },
        { "/data/nvram/APCFG/APRDCL/VibSpk_CompFlt",   VER(AP_CFG_RDCL_FILE_VIBSPK_COMPFLT_LID), CFG_FILE_AUDIO_COMPFLT_REC_SIZE,
            CFG_FILE_VIBSPK_COMPFLT_REC_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&audio_vibspk_custom_default_cust_O2_UK, DataReset , NULL
        },
    };
//END: Add by fangjie for PR942640, add O2 UK audio parameters

//BEGIN: Add by ersen.shang for PR1005561, add US_Relook audio parameters
    const TCFG_FILE g_akCFG_File_Custom_US_Relook[]=
    {
        {
            "/data/nvram/APCFG/APRDCL/Audio_Sph",       VER(AP_CFG_RDCL_FILE_AUDIO_LID),         CFG_FILE_SPEECH_REC_SIZE,
            CFG_FILE_SPEECH_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                                   (char *)&speech_custom_default_cust_US_Relook, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDEB/GPS",         VER(AP_CFG_CUSTOM_FILE_GPS_LID),	           CFG_FILE_GPS_CONFIG_SIZE,
            CFG_FILE_GPS_CONFIG_TOTAL,                  SIGNLE_DEFUALT_REC,                (char *)&stGPSConfigDefault, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_CompFlt",       VER(AP_CFG_RDCL_FILE_AUDIO_COMPFLT_LID),         CFG_FILE_AUDIO_COMPFLT_REC_SIZE,
            CFG_FILE_AUDIO_COMPFLT_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_custom_default_cust_US_Relook, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Effect",       VER(AP_CFG_RDCL_FILE_AUDIO_EFFECT_LID),         CFG_FILE_AUDIO_EFFECT_REC_SIZE,
            CFG_FILE_AUDIO_EFFECT_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_effect_custom_default_cust_US_Relook, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDEB/WIFI",	    	VER(AP_CFG_RDEB_FILE_WIFI_LID),		    CFG_FILE_WIFI_REC_SIZE,
            CFG_FILE_WIFI_REC_TOTAL,		    	SIGNLE_DEFUALT_REC,				    (char *)&stWifiCfgDefault_us, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDEB/WIFI_CUSTOM",	VER(AP_CFG_RDEB_WIFI_CUSTOM_LID),	CFG_FILE_WIFI_CUSTOM_REC_SIZE,
            CFG_FILE_WIFI_CUSTOM_REC_TOTAL,		    SIGNLE_DEFUALT_REC,				    (char *)&stWifiCustomDefault, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Sph_Med",       VER(AP_CFG_RDCL_FILE_AUDIO_PARAM_MED_LID),         CFG_FILE_AUDIO_PARAM_MED_REC_SIZE,
            CFG_FILE_AUDIO_PARAM_MED_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_param_med_default_cust_US_Relook, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Vol_custom",       VER(AP_CFG_RDCL_FILE_AUDIO_VOLUME_CUSTOM_LID),         CFG_FILE_AUDIO_VOLUME_CUSTOM_REC_SIZE,
            CFG_FILE_AUDIO_VOLUME_CUSTOM_REC_TOTAL,           SIGNLE_DEFUALT_REC,                (char *)&audio_volume_custom_default_cust_US_Relook, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Sph_Dual_Mic",       VER(AP_CFG_RDCL_FILE_DUAL_MIC_CUSTOM_LID),         CFG_FILE_SPEECH_DUAL_MIC_SIZE,
            CFG_FILE_SPEECH_DUAL_MIC_TOTAL,           SIGNLE_DEFUALT_REC,                (char *)&dual_mic_custom_default_cust_US_Relook, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Wb_Sph",       VER(AP_CFG_RDCL_FILE_AUDIO_WB_PARAM_LID),         CFG_FILE_WB_SPEECH_REC_SIZE,
            CFG_FILE_WB_SPEECH_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                                   (char *)&wb_speech_custom_default_cust_US_Relook, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDEB/PRODUCT_INFO",       VER(AP_CFG_REEB_PRODUCT_INFO_LID),         CFG_FILE_PRODUCT_INFO_SIZE,
            CFG_FILE_PRODUCT_INFO_TOTAL,                   SIGNLE_DEFUALT_REC,                                   (char *)&stPRODUCT_INFOConfigDefault,DataReset, NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Headphone_CompFlt",       VER(AP_CFG_RDCL_FILE_HEADPHONE_COMPFLT_LID),         CFG_FILE_AUDIO_COMPFLT_REC_SIZE,
            CFG_FILE_HEADPHONE_COMPFLT_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_hcf_custom_default_cust_US_Relook, DataReset , NULL
        },

        { "/data/nvram/APCFG/APRDCL/Audio_gain_table",   VER(AP_CFG_RDCL_FILE_AUDIO_GAIN_TABLE_LID), CFG_FILE_AUDIO_GAIN_TABLE_CUSTOM_REC_SIZE,
            CFG_FILE_AUDIO_GAIN_TABLE_CUSTOM_REC_TOTAL, SIGNLE_DEFUALT_REC  ,	 (char *)&Gain_control_table_default_cust_US_Relook, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_ver1_Vol_custom",       VER(AP_CFG_RDCL_FILE_AUDIO_VER1_VOLUME_CUSTOM_LID),         CFG_FILE_AUDIO_VER1_VOLUME_CUSTOM_REC_SIZE,
            CFG_FILE_AUDIO_VER1_VOLUME_CUSTOM_REC_TOTAL,           SIGNLE_DEFUALT_REC,                (char *)&audio_ver1_custom_default_cust_US_Relook, DataReset , NULL
        },
 
        { "/data/nvram/APCFG/APRDCL/Audio_Hd_Record_Param",   VER(AP_CFG_RDCL_FILE_AUDIO_HD_REC_PAR_LID), CFG_FILE_AUDIO_HD_REC_PAR_SIZE,
            CFG_FILE_AUDIO_HD_REC_PAR_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Hd_Recrod_Par_default_cust_US_Relook, DataReset , NULL
        },

        { "/data/nvram/APCFG/APRDCL/Audio_Hd_Record_Scene_Table",   VER(AP_CFG_RDCL_FILE_AUDIO_HD_REC_SCENE_LID), CFG_FILE_AUDIO_HD_REC_SCENE_TABLE_SIZE,
            CFG_FILE_AUDIO_HD_REC_SCENE_TABLE_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Hd_Recrod_Scene_Table_default_cust_US_Relook, DataReset , NULL
        },

		{ "/data/nvram/APCFG/APRDCL/Audio_Hd_Record_48k_Param",   VER(AP_CFG_RDCL_FILE_AUDIO_HD_REC_48K_PAR_LID), CFG_FILE_AUDIO_HD_REC_48K_PAR_SIZE,
            CFG_FILE_AUDIO_HD_REC_48K_PAR_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Hd_Recrod_48k_Par_default_cust_US_Relook, DataReset , NULL
        },

        { "/data/nvram/APCFG/APRDCL/Audio_Buffer_DC_Calibration_Param",   VER(AP_CFG_RDCL_FILE_AUDIO_BUFFER_DC_CALIBRATION_PAR_LID), CFG_FILE_AUDIO_BUFFER_DC_CALIBRATION_PAR_SIZE,
            CFG_FILE_AUDIO_BUFFER_DC_CALIBRATION_PAR_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Audio_Buffer_DC_Calibration_Par_default_cust_US_Relook, DataReset , NULL
        },
        { "/data/nvram/APCFG/APRDCL/VibSpk_CompFlt",   VER(AP_CFG_RDCL_FILE_VIBSPK_COMPFLT_LID), CFG_FILE_AUDIO_COMPFLT_REC_SIZE,
            CFG_FILE_VIBSPK_COMPFLT_REC_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&audio_vibspk_custom_default_cust_US_Relook, DataReset , NULL
        },
    };
//END: Add by ersen.shang for PR1005562, add US_Relook audio parameters

#else
    const TCFG_FILE g_akCFG_File_Custom[]=
    {
        {
            "/data/nvram/APCFG/APRDCL/Audio_Sph",       VER(AP_CFG_RDCL_FILE_AUDIO_LID),         CFG_FILE_SPEECH_REC_SIZE,
            CFG_FILE_SPEECH_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                                   (char *)&speech_custom_default, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDEB/GPS",         VER(AP_CFG_CUSTOM_FILE_GPS_LID),	           CFG_FILE_GPS_CONFIG_SIZE,
            CFG_FILE_GPS_CONFIG_TOTAL,                  SIGNLE_DEFUALT_REC,                (char *)&stGPSConfigDefault, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_CompFlt",       VER(AP_CFG_RDCL_FILE_AUDIO_COMPFLT_LID),         CFG_FILE_AUDIO_COMPFLT_REC_SIZE,
            CFG_FILE_AUDIO_COMPFLT_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_custom_default, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Effect",       VER(AP_CFG_RDCL_FILE_AUDIO_EFFECT_LID),         CFG_FILE_AUDIO_EFFECT_REC_SIZE,
            CFG_FILE_AUDIO_EFFECT_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_effect_custom_default, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDEB/WIFI",	    	VER(AP_CFG_RDEB_FILE_WIFI_LID),		    CFG_FILE_WIFI_REC_SIZE,
            CFG_FILE_WIFI_REC_TOTAL,		    	SIGNLE_DEFUALT_REC,				    (char *)&stWifiCfgDefault, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDEB/WIFI_CUSTOM",	VER(AP_CFG_RDEB_WIFI_CUSTOM_LID),	CFG_FILE_WIFI_CUSTOM_REC_SIZE,
            CFG_FILE_WIFI_CUSTOM_REC_TOTAL,		    SIGNLE_DEFUALT_REC,				    (char *)&stWifiCustomDefault, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Sph_Med",       VER(AP_CFG_RDCL_FILE_AUDIO_PARAM_MED_LID),         CFG_FILE_AUDIO_PARAM_MED_REC_SIZE,
            CFG_FILE_AUDIO_PARAM_MED_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_param_med_default, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Vol_custom",       VER(AP_CFG_RDCL_FILE_AUDIO_VOLUME_CUSTOM_LID),         CFG_FILE_AUDIO_VOLUME_CUSTOM_REC_SIZE,
            CFG_FILE_AUDIO_VOLUME_CUSTOM_REC_TOTAL,           SIGNLE_DEFUALT_REC,                (char *)&audio_volume_custom_default, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Sph_Dual_Mic",       VER(AP_CFG_RDCL_FILE_DUAL_MIC_CUSTOM_LID),         CFG_FILE_SPEECH_DUAL_MIC_SIZE,
            CFG_FILE_SPEECH_DUAL_MIC_TOTAL,           SIGNLE_DEFUALT_REC,                (char *)&dual_mic_custom_default, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_Wb_Sph",       VER(AP_CFG_RDCL_FILE_AUDIO_WB_PARAM_LID),         CFG_FILE_WB_SPEECH_REC_SIZE,
            CFG_FILE_WB_SPEECH_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                                   (char *)&wb_speech_custom_default, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDEB/PRODUCT_INFO",       VER(AP_CFG_REEB_PRODUCT_INFO_LID),         CFG_FILE_PRODUCT_INFO_SIZE,
            CFG_FILE_PRODUCT_INFO_TOTAL,                   SIGNLE_DEFUALT_REC,                                   (char *)&stPRODUCT_INFOConfigDefault,DataReset, NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Headphone_CompFlt",       VER(AP_CFG_RDCL_FILE_HEADPHONE_COMPFLT_LID),         CFG_FILE_AUDIO_COMPFLT_REC_SIZE,
            CFG_FILE_HEADPHONE_COMPFLT_REC_TOTAL,                   SIGNLE_DEFUALT_REC,                (char *)&audio_hcf_custom_default, DataReset , NULL
        },

        { "/data/nvram/APCFG/APRDCL/Audio_gain_table",   VER(AP_CFG_RDCL_FILE_AUDIO_GAIN_TABLE_LID), CFG_FILE_AUDIO_GAIN_TABLE_CUSTOM_REC_SIZE,
            CFG_FILE_AUDIO_GAIN_TABLE_CUSTOM_REC_TOTAL, SIGNLE_DEFUALT_REC  ,	 (char *)&Gain_control_table_default, DataReset , NULL
        },

        {
            "/data/nvram/APCFG/APRDCL/Audio_ver1_Vol_custom",       VER(AP_CFG_RDCL_FILE_AUDIO_VER1_VOLUME_CUSTOM_LID),         CFG_FILE_AUDIO_VER1_VOLUME_CUSTOM_REC_SIZE,
            CFG_FILE_AUDIO_VER1_VOLUME_CUSTOM_REC_TOTAL,           SIGNLE_DEFUALT_REC,                (char *)&audio_ver1_custom_default, DataReset , NULL
        },

        { "/data/nvram/APCFG/APRDCL/Audio_Hd_Record_Param",   VER(AP_CFG_RDCL_FILE_AUDIO_HD_REC_PAR_LID), CFG_FILE_AUDIO_HD_REC_PAR_SIZE,
            CFG_FILE_AUDIO_HD_REC_PAR_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Hd_Recrod_Par_default, DataReset , NULL
        },

        { "/data/nvram/APCFG/APRDCL/Audio_Hd_Record_Scene_Table",   VER(AP_CFG_RDCL_FILE_AUDIO_HD_REC_SCENE_LID), CFG_FILE_AUDIO_HD_REC_SCENE_TABLE_SIZE,
            CFG_FILE_AUDIO_HD_REC_SCENE_TABLE_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Hd_Recrod_Scene_Table_default, DataReset , NULL
        },

		{ "/data/nvram/APCFG/APRDCL/Audio_Hd_Record_48k_Param",   VER(AP_CFG_RDCL_FILE_AUDIO_HD_REC_48K_PAR_LID), CFG_FILE_AUDIO_HD_REC_48K_PAR_SIZE,
            CFG_FILE_AUDIO_HD_REC_48K_PAR_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Hd_Recrod_48k_Par_default, DataReset , NULL
        },

        { "/data/nvram/APCFG/APRDCL/Audio_Buffer_DC_Calibration_Param",   VER(AP_CFG_RDCL_FILE_AUDIO_BUFFER_DC_CALIBRATION_PAR_LID), CFG_FILE_AUDIO_BUFFER_DC_CALIBRATION_PAR_SIZE,
            CFG_FILE_AUDIO_BUFFER_DC_CALIBRATION_PAR_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Audio_Buffer_DC_Calibration_Par_default, DataReset , NULL
        },
        { "/data/nvram/APCFG/APRDCL/VibSpk_CompFlt",   VER(AP_CFG_RDCL_FILE_VIBSPK_COMPFLT_LID), CFG_FILE_AUDIO_COMPFLT_REC_SIZE,
            CFG_FILE_VIBSPK_COMPFLT_REC_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&audio_vibspk_custom_default, DataReset , NULL
        },
    };
#endif
    int iNvRamFileMaxLID=AP_CFG_CUSTOM_FILE_MAX_LID;
    extern int iNvRamFileMaxLID;
#ifdef JRD_AUDIO_PARAMS_CUST
    const TCFG_FILE *g_akCFG_File_Custom = NULL;
    const TCFG_FILE *g_akCFG_File_Custom_jrd[] = {
        //g_akCFG_File_Custom1
        /* add another audio parameters set here: */
        /*
               * For example: 
                     ,g_akCFG_File_Custom2
               */
          g_akCFG_File_Custom_eu_hd
         ,g_akCFG_File_Custom_us_receiver
	,g_akCFG_File_Custom_O2_UK //Add by fangjie for PR942640
	,g_akCFG_File_Custom_US_Relook//Add by ersen.shang for PR1005562
    };
    const unsigned int g_i4CFG_File_Custom_Count = sizeof(g_akCFG_File_Custom1)/sizeof(TCFG_FILE);
    const unsigned int g_akCFG_File_Custom_jrd_Count = \
        sizeof(g_akCFG_File_Custom_jrd)/sizeof(g_akCFG_File_Custom_jrd[0]);
    extern const TCFG_FILE *g_akCFG_File_Custom_jrd[];
    extern const unsigned int g_akCFG_File_Custom_jrd_Count;
#else
    const unsigned int g_i4CFG_File_Custom_Count = sizeof(g_akCFG_File_Custom)/sizeof(TCFG_FILE);

    extern const TCFG_FILE g_akCFG_File_Custom[];
#endif
    extern const unsigned int g_i4CFG_File_Custom_Count;

    int iFileWIFILID=AP_CFG_RDEB_FILE_WIFI_LID;
    extern int iFileWIFILID;
    int iFileCustomWIFILID=AP_CFG_RDEB_WIFI_CUSTOM_LID;
    extern int iFileCustomWIFILID;
    int iFilePRODUCT_INFOLID=AP_CFG_REEB_PRODUCT_INFO_LID;
    extern int iFilePRODUCT_INFOLID;

#ifdef __cplusplus
}
#endif

#endif
