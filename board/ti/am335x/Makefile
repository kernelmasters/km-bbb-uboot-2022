# SPDX-License-Identifier: GPL-2.0+
#
# Makefile
#
# Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/

ifeq ($(CONFIG_$(SPL_)SKIP_LOWLEVEL_INIT),)
obj-y	:= mux.o
endif

obj-y	+= board.o
obj-$(CONFIG_KM_BBB_2x16_CHAR_LCD) += km_char_lcd.o
