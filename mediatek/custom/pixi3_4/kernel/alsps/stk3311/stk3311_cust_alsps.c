/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#include <linux/types.h>
#include <stk3311_cust_alsps.h>
#ifdef MT6573
#include <mach/mt6573_pll.h>
#endif
#ifdef MT6575
#include <mach/mt6575_pm_ldo.h>
#endif
#ifdef MT6577
#include <mach/mt6577_pm_ldo.h>
#endif
#ifdef MT6589
#include <mach/mt_pm_ldo.h>
#endif
#ifdef MT6572
#include <mach/mt_pm_ldo.h>
#endif

static struct stk_alsps_hw stk_cust_alsps_hw = {
	/* i2c bus number, for mt657x, default=0. For mt6589, default=3 */
#ifdef MT6589	
    .i2c_num    = 3,
#elif defined(MT6572)	
    .i2c_num    = 1,
#else	
    .i2c_num    = 0,	
#endif	
	//.polling_mode =1,
		.polling_mode_ps =0,
		.polling_mode_als =1,
		.power_id   = MT65XX_POWER_NONE,    /*LDO is not used*/
		.power_vol  = VOL_DEFAULT,          /*LDO is not used*/
		.i2c_addr   = {0x90, 0x00, 0x00, 0x00},	/*STK3x1x*/

	  //	.als_level  = { 3, 40,  80,   120,   160, 250,  400, 800, 1200,  1600, 2000, 3000, 5000, 10000, 65535},
	  	.als_level  = { 5, 30,  60,   100,   120, 190,  300, 600, 900,  1200, 1500, 2400, 3800, 7500, 65535},
    		.als_value  = {10, 250,250,  250, 2100, 2100,  2100,  2100, 2100,  3100,  4100,  8100,  10240, 10240,  10240, 10240},
	//	.als_level  = {6, 12, 20, 30, 30, 30, 55, 55, 55, 93, 93, 170, 260, 512, 768},	/* als_code */
	//	.als_value  = {10, 300, 300, 300, 300, 300, 550, 550, 550, 930, 930, 1700, 1900, 1900, 7680, 10240},    /* lux */
		.state_val = 0x0,		/* disable all */
		.psctrl_val = 0x71,		/* ps_persistance=4, ps_gain=64X, PS_IT=0.391ms */
	//	.alsctrl_val = 0x2A,
		.alsctrl_val = 0x39,	/* als_persistance=1, als_gain=64X, ALS_IT=50ms def*/
		.ledctrl_val = 0xFF,	/* 100mA IRDR, 64/64 LED duty */
		.wait_val = 0x7,		/* 50 ms */
		.ps_high_thd_val = 1700,
		.ps_low_thd_val = 1500,
};
struct stk_alsps_hw *stk_get_cust_alsps_hw(void) {
    return &stk_cust_alsps_hw;
}
