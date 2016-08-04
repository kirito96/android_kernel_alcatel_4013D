ifeq ($(PROJECT),soul35)
EXTRA_CFLAGS += -DSOUL3P5_CHARGING_TEMPERATURE_PLOICY
endif

#add by yanqi.liu
ifeq ($(PROJECT),angola)
EXTRA_CFLAGS += -DANGOLA_CHARGING_TEMPERATURE_PLOICY
endif

##zhongyang add for soul4 compile error 20140227 begin
ifeq ($(PROJECT),soul4)
EXTRA_CFLAGS += -DSOUL4P0_CHARGING_TEMPERATURE_PLOICY
endif
##zhongyang add for soul4 compile error 20140227 end

##zhongyang add for pixi3_4 compile error 20140827 begin
ifeq ($(PROJECT),pixi3_4)
EXTRA_CFLAGS += -DPIXI34_CHARGING_TEMPERATURE_PLOICY
endif

ifeq ($(PROJECT),pixi3_45)
EXTRA_CFLAGS += -DSOUL4P0_CHARGING_TEMPERATURE_PLOICY
endif

##zhongyang add for pixi3_4 compile error 20140827 end


ifeq ($(PROJECT),jrdcd72_we_vf_kk)
EXTRA_CFLAGS += -DSIMBA_KK_CHARGING_TEMPERATURE_PLOICY
endif

ifeq ($(TARGET_BUILD_MMITEST),true)
EXTRA_CFLAGS += -DCHARGING_META
EXTRA_CFLAGS += -DTARGET_BUILD_MMITEST
endif

obj-$(CONFIG_MTK_SMART_BATTERY) += battery_meter.o battery_common.o

ifeq ($(MTK_FAN5402_SUPPORT),yes)
    obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
else ifeq ($(MTK_FAN5405_SUPPORT),yes)
        obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o 
else
    ifeq ($(MTK_SWCHR_SUPPORT),yes)
        obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
    else
        ifeq ($(MTK_BQ24158_SUPPORT),yes)
            obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o 
        else
            ifeq ($(MTK_BQ24196_SUPPORT),yes)
                obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
            else
                ifeq ($(MTK_BQ24296_SUPPORT),yes)
                    obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
                else
                    ifeq ($(MTK_NCP1851_SUPPORT),yes)
                        obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
                    else
                        ifeq ($(MTK_NCP1854_SUPPORT),yes)
                            obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
                        else
                            ifeq ($(MTK_BQ24160_SUPPORT),yes)
                                obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
                            else
                                ifeq ($(MTK_BQ24157_SUPPORT),yes)
                                    obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
                                else
                                    obj-$(CONFIG_MTK_SMART_BATTERY) += linear_charging.o
                                endif        
                            endif        
                        endif
                   endif
                endif
            endif
        endif  
    endif    
endif
