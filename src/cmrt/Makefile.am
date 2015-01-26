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

CC = g++ -g

SUBDIRS =

AM_CPPFLAGS = \
	-DPTHREADS		\
	$(DRM_CFLAGS)		\
	$(LIBVA_DEPS_CFLAGS)	\
	-I$(top_srcdirc)/src/cmrt	\
	$(NULL)


cmrt_cflags = \
	-Wall			\
	-Wno-missing-braces \
	-fvisibility=default 	\
	-DLINUX			\
	-I/usr/include/libdrm   \
	-msse                   \
	-msse2                  \
	-msse3                  \
	-mssse3                 \
	-msse4.1                \
	-msse4.2                \
	-msse4                  \
	-fPIC                  \
	$(NULL)

cmrt_ldflags = \
	-module -avoid-version	\
	-no-undefined		\
	-Wl,--no-undefined	\
	-fPIC \
	$(NULL)

cmrt_libs = \
	-lpthread -lm -ldl	\
	$(DRM_LIBS) -ldrm_intel	\
	$(LIBVA_DEPS_LIBS)	\
	$(NULL)

cmrt_files = \
	hal_cm.c  \
	hal_cm_g75.c  \
	hal_cm_g8.c  \
	hw_interface_base.c \
	hw_interface.c \
	hw_interface_g75.c \
	hw_interface_g8.c \
	os_utilities.c \
	os_interface.c \
	hw_cmd.c \
	os_util_debug.c \
	oscl_impl_linux.c \
	cm_device.cpp \
	cm_buffer.cpp   \
	cm_event.cpp \
	cm_array.cpp \
	cm_basic_list.cpp \
	cm_basic_queue.cpp \
	cm_perf.cpp  \
	cm_kernel.cpp \
	cm_kernel_data.cpp \
	cm_surface_manager.cpp  \
	cm_surface.cpp  \
	cm_task.cpp  \
	cm_task_internal.cpp  \
	cm_program.cpp  \
	cm_thread_space.cpp  \
	cm_group_space.cpp  \
	cm_queue.cpp   \
	cm_surface_2d.cpp \
	cm_surface_2d_up.cpp \
	$(NULL)

cmrt_headers = \
	cm_rt.h	\
	cm_rt_linux.h	\
	$(NULL)

cmrt_la_LTLIBRARIES     = cmrt.la
cmrt_ladir              = $(libdir)
cmrt_la_CFLAGS          = $(cmrt_cflags)
cmrt_la_CXXFLAGS        = -fpermissive $(cmrt_cflags)
cmrt_la_LDFLAGS         = $(cmrt_ldflags)
cmrt_la_LIBADD          = $(cmrt_libs)
cmrt_la_SOURCES         = $(cmrt_files)
cmrt_la_HEADERS         = $(cmrt_headers)

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in config.h.in
