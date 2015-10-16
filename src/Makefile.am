# Copyright (c) 2007 Intel Corporation. All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sub license, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
# 
# The above copyright notice and this permission notice (including the
# next paragraph) shall be included in all copies or substantial portions
# of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
# IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


SUBDIRS = vp9hdec

AM_CPPFLAGS = \
	-DPTHREADS		\
        -DVA_DRIVERS_PATH="\"$(LIBVA_DRIVERS_PATH)\"" \
	$(DRM_CFLAGS)		\
	$(DRM_INTEL_CFLAGS)	\
	$(CMRT_CFLAGS)		\
	$(LIBVA_DEPS_CFLAGS)	\
	-I$(top_srcdir)/src/cmrt	\
	-I$(top_srcdir)/src/vp9hdec	\
	$(NULL)

driver_cflags = \
	-Wall			\
	-fvisibility=hidden	\
	$(NULL)

driver_ldflags = \
	-module -avoid-version	\
	-no-undefined		\
	-Wl,--no-undefined	\
	$(NULL)

driver_libs = \
	-lpthread -lm -ldl	\
	$(DRM_LIBS)		\
	$(DRM_INTEL_LIBS)	\
	$(CMRT_LIBS)		\
	$(LIBVA_DEPS_LIBS)	\
	$(NULL)

driver_files = \
        dso_utils.c            \
	object_heap.c		\
	media_drv_util.c   \
        media_drv_gpe_utils.c   \
        media_drv_batchbuffer.c \
        media_drv_common.c \
        media_drv_driver.c  \
        media_drv_encoder.c \
        media_drv_encoder_vp8.c \
        media_drv_encoder_vp8_g7.c \
        media_drv_hw.c	\
        media_drv_hwcmds.c  \
        media_drv_hwcmds_g8.c \
        media_drv_hw_g9.c  \
        media_drv_hw_g8.c  \
        media_drv_hw_g75.c  \
        media_drv_hw_g7.c  \
        media_drv_init.c   \
        media_drv_kernels.c  \
        media_drv_kernels_g7.c  \
        media_drv_render.c   \
        media_drv_surface.c  \
        media_drv_decoder.c  \
        media_drv_gen75_render.c   \
        media_drv_gen8_render.c   \
        media_drv_gen9_render.c   \
        media_drv_hybrid_vp9_common.cpp   \
        $(NULL)

if USE_X11
driver_files	+=media_drv_output_dri.c
endif

driver_headers = \
        dso_utils.h             \
	i965_mutext.h		\
	intel_compiler.h	\
	object_heap.h           \
	sysdeps.h		\
	va_backend_compat.h	\
        media_drv_defines.h  \
        media_drv_util.h  \
        media_drv_gpe_utils.h  \
        media_drv_batchbuffer.h  \
        media_drv_common.h  \
        media_drv_data.h  \
        media_drv_driver.h  \
        media_drv_encoder.h  \
        media_drv_encoder_vp8.h  \
        media_drv_encoder_vp8_g7.h \
        media_drv_hwcmds.h  \
        media_drv_hwcmds_g8.h \
        media_drv_hw_g9.h  \
        media_drv_hw_g8.h  \
        media_drv_hw_g75.h  \
        media_drv_hw_g7.h  \
        media_drv_hw.h     \
        media_drv_init.h   \
        media_drv_kernels.h \
        media_drv_kernels_g7.h \
        media_drv_output_dri.h  \
        media_drv_render.h  \
        media_drv_surface.h   \
        media_drv_decoder.h   \
        media_render_common.h  \
        media_drv_hybrid_vp9_common.h   \
       $(NULL)

if USE_X11
driver_headers	+=media_drv_output_dri.h
endif

hybrid_drv_video_la_LTLIBRARIES= hybrid_drv_video.la
hybrid_drv_video_ladir= $(LIBVA_DRIVERS_PATH)
hybrid_drv_video_la_CFLAGS	= $(driver_cflags)
hybrid_drv_video_la_CXXFLAGS	= -fpermissive $(driver_cflags)
hybrid_drv_video_la_LDFLAGS= $(driver_ldflags)
hybrid_drv_video_la_LIBADD	= $(driver_libs) vp9hdec/vp9hdec.la
hybrid_drv_video_la_SOURCES= $(driver_files)
noinst_HEADERS = $(driver_headers)

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in config.h.in
