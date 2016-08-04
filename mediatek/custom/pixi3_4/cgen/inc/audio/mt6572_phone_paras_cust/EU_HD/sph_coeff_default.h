#ifndef SPEECH_COEFF_DEFAULT_H
#define SPEECH_COEFF_DEFAULT_H

#ifndef FALSE
#define FALSE 0
#endif

//speech parameter depen on BT_CHIP cersion
#if defined(MTK_MT6611)

#define BT_COMP_FILTER (1 << 15)
#define BT_SYNC_DELAY  86

#elif defined(MTK_MT6612)

#define BT_COMP_FILTER (1 << 15)
#define BT_SYNC_DELAY  86

#elif defined(MTK_MT6616) || defined(MTK_MT6620) || defined(MTK_MT6622) || defined(MTK_MT6626) || defined(MTK_MT6628)

#define BT_COMP_FILTER (1 << 15)
#define BT_SYNC_DELAY  86

#else // MTK_MT6620

#define BT_COMP_FILTER (0 << 15)
#define BT_SYNC_DELAY  86

#endif

#ifdef MTK_DUAL_MIC_SUPPORT
#define SPEECH_MODE_PARA13 (371)
#define SPEECH_MODE_PARA14 (23)
#else
#define SPEECH_MODE_PARA13 (0)
#define SPEECH_MODE_PARA14 (0)
#endif

#ifdef NXP_SMARTPA_SUPPORT
	#define MANUAL_CLIPPING (1 << 15)
	#define NXP_DELAY_REF   (1 << 6)
	#define PRE_CLIPPING_LEVEL 32767
#else
	#define MANUAL_CLIPPING (0 << 15)
	#define NXP_DELAY_REF   (0 << 6)
	#define PRE_CLIPPING_LEVEL 10752
#endif



#define DEFAULT_SPEECH_NORMAL_MODE_PARA \
    96,   253, 16388,    30, 57351,   799,   400,    17,\
   216,  4325,   611,     0, 20488,     0,     0,  8192

#define DEFAULT_SPEECH_EARPHONE_MODE_PARA \
     0,   189, 10756,    31, 57351,    31,   400,    64,\
    80,  4325,   611,     0, 20488,     0,     0,     0

#define DEFAULT_SPEECH_BT_EARPHONE_MODE_PARA \
     0,   253, 10756,    31, 53255,    31,   400,     0,\
    80,  4325,   611,     0, 20488|BT_COMP_FILTER,     0,     0,BT_SYNC_DELAY

#define DEFAULT_SPEECH_LOUDSPK_MODE_PARA \
    96,   224,  5256,    31, 57351, 24607,   400,   131,\
    84,  4325,   611,     0, 20488,     0,     0,     0

#define DEFAULT_SPEECH_CARKIT_MODE_PARA \
    96,   224,  5256,    31, 57351, 24607,   400,   132,\
    84,  4325,   611,     0, 20488,     0,     0,     0

#define DEFAULT_SPEECH_BT_CORDLESS_MODE_PARA \
     0,     0,     0,     0,     0,     0,     0,     0,\
     0,     0,     0,     0,     0,     0,     0,     0

#define DEFAULT_SPEECH_AUX1_MODE_PARA \
     0,     0,     0,     0,     0,     0,     0,     0,\
     0,     0,     0,     0,     0,     0,     0,     0

#define DEFAULT_SPEECH_AUX2_MODE_PARA \
     0,     0,     0,     0,     0,     0,     0,     0,\
     0,     0,     0,     0,     0,     0,     0,     0

#define DEFAULT_SPEECH_COMMON_PARA \
     0, 55997, 31000, 10752, 32769,     0,     0,     0, \
     0,     0,     0,     0

#define DEFAULT_SPEECH_VOL_PARA \
     0,     0,     0,     0

#define DEFAULT_AUDIO_DEBUG_INFO \
     0,     0,     0,     0,     0,     0,     0,     0, \
     0,     0,     0,     0,     0,     0,     0,     0

#define DEFAULT_VM_SUPPORT FALSE

#define DEFAULT_AUTO_VM FALSE

#define DEFAULT_WB_SPEECH_NORMAL_MODE_PARA \
   128,   253, 16388,    30, 57607,   799,   400,    64,\
   400,  4325,   611,     0, 16392,     0,     0,  8192

#define DEFAULT_WB_SPEECH_EARPHONE_MODE_PARA \
     0,   189, 10756,    31, 57607,    31,   400,    80,\
    80,  4325,   611,     0, 16392,     0,     0,     0

#define DEFAULT_WB_SPEECH_BT_EARPHONE_MODE_PARA \
     0,   253, 10756,    31, 53511,    31,   400,     0,\
    80,  4325,   611,     0, 16392|BT_COMP_FILTER,     0,     0,BT_SYNC_DELAY

#define DEFAULT_WB_SPEECH_LOUDSPK_MODE_PARA \
    96,   224,  5256,    31, 57607, 24607,   400,   129,\
    84,  4325,   611,     0, 16392,     0,     0,     0

#define DEFAULT_WB_SPEECH_CARKIT_MODE_PARA \
 65276, 65064, 65489,   104, 65079, 65400,   176,   646,\
 64756, 64908, 64877,   327, 65125, 64603, 65281,   221

#define DEFAULT_WB_SPEECH_BT_CORDLESS_MODE_PARA \
     8,   128,   148,   160,     8,     1, 65498, 65491,\
   124, 65493,   128, 65433, 65445,    24,   134, 65461

#define DEFAULT_WB_SPEECH_AUX1_MODE_PARA \
  1402, 63758, 62839, 65111,  2284, 62577, 64800, 61780,\
  5393, 61842,   467, 55850, 23197, 23197, 55850,   467

#define DEFAULT_WB_SPEECH_AUX2_MODE_PARA \
 61842,  5393, 61780, 64800, 62577,  2284, 65111, 62839,\
 63758,  1402,   221, 65281, 64603, 65125,   327, 64877

#define MICBAIS  1900

/* The Bluetooth PCM digital volume */
/* default_bt_pcm_in_vol : uplink, only for enlarge volume,
                       0x100 : 0dB  gain
                       0x200 : 6dB  gain
                       0x300 : 9dB  gain
                       0x400 : 12dB gain
                       0x800 : 18dB gain
                       0xF00 : 24dB gain             */

#define DEFAULT_BT_PCM_IN_VOL  0x100
/* default_bt_pcm_out_vol : downlink gain,
                       0x1000 : 0dB; maximum 0x7FFF  */
#define DEFAULT_BT_PCM_OUT_VOL  0x1000

#endif
