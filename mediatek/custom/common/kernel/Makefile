include $(MTK_PATH_BUILD)/common.mk
$(call all-modules-src-or-makefile,$(obj),CUSTOM_KERNEL_)
obj-n := dummy.o

ifeq ($(MTK_ALPS_BOX_SUPPORT), yes)
ccflags-y += -DMTK_ALPS_BOX_SUPPORT
endif

#add by fangjie, when compile ./makeMtk -t -o=TARGET_BUILD_MMITEST=true r k
ifeq ($(TARGET_BUILD_MMITEST),true)
ccflags-y += -DTARGET_BUILD_MMITEST
endif

#add by ersen.shang for pixi3_4_relook 
ifeq ($(RELOOK_SUPPORT),TRUE)
ccflags-y += -DTARGET_RELOOK_SUPPORT
endif
