PWD=$(shell pwd)

include ../../.config

MALI_DRV_ROOT=DX910-SW-99002-r4p0-00rel0/driver/src/devicedrv/mali
MALI_UMP_ROOT=DX910-SW-99002-r4p0-00rel0/driver/src/devicedrv/ump

#MALI_DRV_ROOT=DX910-SW-99002-r3p2-01rel2/driver/src/devicedrv/mali
#MALI_UMP_ROOT=DX910-SW-99002-r3p2-01rel2/driver/src/devicedrv/ump

# MALI_DRV_ROOT=DX910-SW-99002-r3p2-01rel1/driver/src/devicedrv/mali
# MALI_UMP_ROOT=DX910-SW-99002-r3p2-01rel1/driver/src/devicedrv/ump
EGL_ROOT=DX910-SW-99002-r4p0-00rel0/driver/src/egl/x11/drm_module/mali_drm

ifeq ($(LICHEE_PLATFORM),linux)
UMP_ENABLE=1
else
UMP_ENABLE=0
endif

install: build
	cp $(MALI_DRV_ROOT)/mali.ko $(LICHEE_MOD_DIR)/
	if [ $(UMP_ENABLE) -eq 1 ]; then \
		cp $(MALI_UMP_ROOT)/ump.ko $(LICHEE_MOD_DIR)/; \
	fi
	
build:
	@echo $(LICHEE_KDIR)
	if [ $(UMP_ENABLE) -eq 1 ]; then \
		$(MAKE) -C $(MALI_UMP_ROOT) CONFIG=ca8-virtex820-m400-1 BUILD=release KDIR=${LICHEE_KDIR}; \
	fi
	$(MAKE) -C $(MALI_DRV_ROOT) USING_MMU=1 USING_UMP=0 USING_PMM=1 BUILD=release \
		KDIR=${LICHEE_KDIR}

clean:
	$(MAKE) -C $(MALI_DRV_ROOT) USING_MMU=1 USING_UMP=0 USING_PMM=1 BUILD=release \
		KDIR=${LICHEE_KDIR} clean
	if [ $(UMP_ENABLE) -eq 1 ]; then \
		$(MAKE) -C $(MALI_UMP_ROOT) CONFIG=ca8-virtex820-m400-1 BUILD=release KDIR=${LICHEE_KDIR} clean;
	fi
