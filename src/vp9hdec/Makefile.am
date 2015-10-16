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

SUBDIRS =

AM_CPPFLAGS = \
	-DPTHREADS		\
	$(DRM_CFLAGS)		\
	$(DRM_INTEL_CFLAGS)	\
	$(LIBVA_DEPS_CFLAGS)	\
	-I$(top_srcdir)/src	\
	-I$(top_srcdir)/src/cmrt	\
	-D__linux__		\
	-DLINUX		\
	$(NULL)


driver_cflags = \
	-Wall			\
	-Wno-unused-variable      \
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
	$(LIBVA_DEPS_LIBS)	\
	$(NULL)

driver_files = \
	decode_hybrid_vp9.cpp	\
	intel_hybrid_hostvld_vp9.cpp	\
	intel_hybrid_hostvld_vp9_loopfilter.cpp	\
	intel_hybrid_hostvld_vp9_parser.cpp	\
	intel_hybrid_hostvld_vp9_engine.cpp	\
	intel_hybrid_hostvld_vp9_context.cpp	\
	intel_hybrid_vp9_kernel_g75.cpp	\
	intel_hybrid_vp9_kernel_g8.cpp	\
	intel_hybrid_vp9_kernel_g9.cpp	\
	intel_hybrid_vp9_kernel_g8lp.cpp	\
	intel_hybrid_debug_dump.cpp	\
	$(NULL)

driver_headers = \
	decode_hybrid_vp9.h	\
	intel_hybrid_common_vp9.h	\
	intel_hybrid_hostvld_vp9.h	\
	intel_hybrid_hostvld_vp9_loopfilter.h	\
	intel_hybrid_hostvld_vp9_parser.h	\
	intel_hybrid_hostvld_vp9_parser_tables.h	\
	intel_hybrid_hostvld_vp9_engine.h	\
	intel_hybrid_hostvld_vp9_context.h	\
	intel_hybrid_hostvld_vp9_context_tables.h	\
	intel_hybrid_hostvld_vp9_internal.h	\
	intel_hybrid_debug_dump.h	\
	$(NULL)

noinst_LTLIBRARIES		= vp9hdec.la
vp9hdec_la_CFLAGS		= $(driver_cflags)
vp9hdec_la_CXXFLAGS		= -fpermissive $(driver_cflags)
vp9hdec_la_LDFLAGS		= $(driver_ldflags)
vp9hdec_la_LIBADD		= $(driver_libs)
vp9hdec_la_SOURCES		= $(driver_files)
noinst_HEADERS			= $(driver_headers)

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in config.h.in
