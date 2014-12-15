# Copyright (c) 2014 Intel Corporation. All Rights Reserved.
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

SUBDIRS =

AM_CPPFLAGS = \
	-DPTHREADS		\
	$(DRM_CFLAGS)		\
	$(LIBVA_DEPS_CFLAGS)	\
	-I$(top_srcdirc)/src/cmrt	\
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
	$(DRM_LIBS) -ldrm_intel	\
	$(LIBVA_DEPS_LIBS)	\
	$(NULL)

driver_files = \
	cm_device.cpp	\
	$(NULL)

driver_headers = \
	cm_rt.h	\
	cm_rt_linux.h	\
	$(NULL)

noinst_LTLIBRARIES      = cmrt.la
#cmrt_ladir             = $(LIBVA_DRIVERS_PATH)
cmrt_la_CFLAGS          = $(driver_cflags)
cmrt_la_CXXFLAGS        = -fpermissive $(driver_cflags)
cmrt_la_LDFLAGS         = $(driver_ldflags)
cmrt_la_LIBADD          = $(driver_libs)
cmrt_la_SOURCES         = $(driver_files)
noinst_HEADERS          = $(driver_headers)

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in config.h.in
