/*
 * Copyright © 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *     Wei Lin<wei.w.lin@intel.com>
 *     Yuting Yang<yuting.yang@intel.com>
 *     Lina Sun<lina.sun@intel.com>
 *     Zhao Yakui <yakui.zhao@intel.com>
 */

#pragma once
#ifndef _Cm_H_
#define _Cm_H_


#ifdef __cplusplus
#   define EXTERN_C     extern "C"
#else
#   define EXTERN_C
#endif

#include "cm_rt_linux.h"


#ifndef CM_1_0
#define CM_1_0          100
#endif

#ifndef CM_2_0
#define CM_2_0          200
#endif

#ifndef CM_2_1
#define CM_2_1          201
#endif
#ifndef CM_2_2
#define CM_2_2          202
#endif
#ifndef CM_2_3
#define CM_2_3          203
#endif
#ifndef CM_2_4
#define CM_2_4          204
#endif
#ifndef CM_3_0
#define CM_3_0          300
#endif
#ifndef CM_4_0
#define CM_4_0          400
#endif

//Define MDF version for MDF 4.0
#ifndef __INTEL_MDF
#define __INTEL_MDF     CM_4_0
#endif

#define CM_SUCCESS                                  0
#define CM_FAILURE                                  -1
#define CM_NOT_IMPLEMENTED                          -2
#define CM_SURFACE_ALLOCATION_FAILURE               -3
#define CM_OUT_OF_HOST_MEMORY                       -4
#define CM_SURFACE_FORMAT_NOT_SUPPORTED             -5
#define CM_EXCEED_SURFACE_AMOUNT                    -6
#define CM_EXCEED_KERNEL_ARG_AMOUNT                 -7
#define CM_EXCEED_KERNEL_ARG_SIZE_IN_BYTE           -8
#define CM_INVALID_ARG_INDEX                        -9
#define CM_INVALID_ARG_VALUE                        -10
#define CM_INVALID_ARG_SIZE                         -11
#define CM_INVALID_THREAD_INDEX                     -12
#define CM_INVALID_WIDTH                            -13
#define CM_INVALID_HEIGHT                           -14
#define CM_INVALID_DEPTH                            -15
#define CM_INVALID_COMMON_ISA                       -16
#define CM_D3DDEVICEMGR_OPEN_HANDLE_FAIL            -17 // IDirect3DDeviceManager9::OpenDeviceHandle fail
#define CM_D3DDEVICEMGR_DXVA2_E_NEW_VIDEO_DEVICE    -18 // IDirect3DDeviceManager9::LockDevice return DXVA2_E_VIDEO_DEVICE_LOCKED
#define CM_D3DDEVICEMGR_LOCK_DEVICE_FAIL            -19 // IDirect3DDeviceManager9::LockDevice return other fail than DXVA2_E_VIDEO_DEVICE_LOCKED
#define CM_EXCEED_SAMPLER_AMOUNT                    -20
#define CM_EXCEED_MAX_KERNEL_PER_ENQUEUE            -21
#define CM_EXCEED_MAX_KERNEL_SIZE_IN_BYTE           -22
#define CM_EXCEED_MAX_THREAD_AMOUNT_PER_ENQUEUE     -23
#define CM_EXCEED_VME_STATE_G6_AMOUNT               -24
#define CM_INVALID_THREAD_SPACE                     -25
#define CM_EXCEED_MAX_TIMEOUT                       -26
#define CM_JITDLL_LOAD_FAILURE                      -27
#define CM_JIT_COMPILE_FAILURE                      -28
#define CM_JIT_COMPILESIM_FAILURE                   -29
#define CM_INVALID_THREAD_GROUP_SPACE               -30
#define CM_THREAD_ARG_NOT_ALLOWED                   -31
#define CM_INVALID_GLOBAL_BUFFER_INDEX              -32
#define CM_INVALID_BUFFER_HANDLER                   -33
#define CM_EXCEED_MAX_SLM_SIZE                      -34
#define CM_JITDLL_OLDER_THAN_ISA                    -35
#define CM_INVALID_HARDWARE_THREAD_NUMBER           -36
#define CM_GTPIN_INVOKE_FAILURE                     -37
#define CM_INVALIDE_L3_CONFIGURATION                -38
#define CM_INVALID_D3D11_TEXTURE2D_USAGE            -39
#define CM_INTEL_GFX_NOTFOUND                       -40
#define CM_GPUCOPY_INVALID_SYSMEM                   -41
#define CM_GPUCOPY_INVALID_WIDTH                    -42
#define CM_GPUCOPY_INVALID_STRIDE                   -43
#define CM_EVENT_DRIVEN_FAILURE                     -44
#define CM_LOCK_SURFACE_FAIL                        -45 // Lock surface failed
#define CM_INVALID_GENX_BINARY                      -46
#define CM_FEATURE_NOT_SUPPORTED_IN_DRIVER          -47 // driver out-of-sync
#define CM_QUERY_DLL_VERSION_FAILURE                -48 //Fail in getting DLL file version
#define CM_KERNELPAYLOAD_PERTHREADARG_MUTEX_FAIL    -49
#define CM_KERNELPAYLOAD_PERKERNELARG_MUTEX_FAIL    -50
#define CM_KERNELPAYLOAD_SETTING_FAILURE            -51
#define CM_KERNELPAYLOAD_SURFACE_INVALID_BTINDEX    -52
#define CM_NOT_SET_KERNEL_ARGUMENT                  -53
#define CM_GPUCOPY_INVALID_SURFACES                 -54
#define CM_GPUCOPY_INVALID_SIZE                     -55
#define CM_GPUCOPY_OUT_OF_RESOURCE                  -56
#define CM_DEVICE_INVALID_D3DDEVICE                 -57
#define CM_SURFACE_DELAY_DESTROY                    -58
#define	CM_INVALID_VEBOX_STATE                      -59
#define CM_INVALID_VEBOX_SURFACE                    -60
#define CM_FEATURE_NOT_SUPPORTED_BY_HARDWARE	    -61
#define CM_RESOURCE_USAGE_NOT_SUPPORT_READWRITE     -62
#define CM_MULTIPLE_MIPLEVELS_NOT_SUPPORTED         -63
#define CM_INVALID_UMD_CONTEXT                      -64
#define CM_INVALID_LIBVA_SURFACE                    -65
#define CM_INVALID_LIBVA_INITIALIZE                 -66
#define CM_KERNEL_THREADSPACE_NOT_SET               -67
#define CM_INVALID_KERNEL_THREADSPACE               -68
#define CM_KERNEL_THREADSPACE_THREADS_NOT_ASSOCIATED -69
#define CM_KERNEL_THREADSPACE_INTEGRITY_FAILED      -70
#define CM_INVALID_USERPROVIDED_GENBINARY           -71
#define CM_INVALID_PRIVATE_DATA                     -72
#define CM_VA_SURFACE_NOT_SUPPORTED                 -73
#define CM_SURFACE_CACHED                           -74
#define CM_SURFACE_IN_USE                           -75
#define CM_INVALID_GPUCOPY_KERNEL                   -76
#define CM_INVALID_DEPENDENCY_WITH_WALKING_PATTERN  -77
#define CM_INVALID_MEDIA_WALKING_PATTERN            -78
#define CM_FAILED_TO_ALLOCATE_SVM_BUFFER            -79
#define CM_EXCEED_MAX_POWER_OPTION_FOR_PLATFORM     -80
#define CM_INVALID_KERNEL_THREADGROUPSPACE          -81
#define CM_INVALID_KERNEL_SPILL_CODE                -82
#define CM_UMD_DRIVER_NOT_SUPPORTED                 -83
#define CM_INVALID_GPU_FREQUENCY_VALUE              -84
#define CM_SYSTEM_MEMORY_NOT_4KPAGE_ALIGNED         -85
#define CM_KERNEL_ARG_SETTING_FAILED                -86
#define CM_NO_AVAILABLE_SURFACE                     -87
#define CM_PLATFORM_NOT_SUPPORTED                   -88

#define CM_MIN_SURF_WIDTH   1
#define CM_MIN_SURF_HEIGHT  1
#define CM_MIN_SURF_DEPTH   2

#define CM_MAX_1D_SURF_WIDTH 0X8000000 // 2^27

//IVB+
#define CM_MAX_2D_SURF_WIDTH_IVB_PLUS   16384
#define CM_MAX_2D_SURF_HEIGHT_IVB_PLUS  16384

#define CM_MAX_2D_SURF_WIDTH    CM_MAX_2D_SURF_WIDTH_IVB_PLUS
#define CM_MAX_2D_SURF_HEIGHT   CM_MAX_2D_SURF_HEIGHT_IVB_PLUS

#define CM_MAX_3D_SURF_WIDTH    2048
#define CM_MAX_3D_SURF_HEIGHT   2048
#define CM_MAX_3D_SURF_DEPTH    2048

#define CM_MAX_OPTION_SIZE_IN_BYTE          512
#define CM_MAX_KERNEL_NAME_SIZE_IN_BYTE     256
#define CM_MAX_ISA_FILE_NAME_SIZE_IN_BYTE   256

#define CM_MAX_THREADSPACE_WIDTH        511
#define CM_MAX_THREADSPACE_HEIGHT       511

#define IVB_MAX_SLM_SIZE_PER_GROUP   16 // 64KB PER Group on Gen7

//Time in seconds before kernel should timeout
#define CM_MAX_TIMEOUT 2
//Time in milliseconds before kernel should timeout
#define CM_MAX_TIMEOUT_MS CM_MAX_TIMEOUT*1000

#define CM_NO_EVENT  ((CmEvent *)(-1))	//NO Event

typedef enum _CM_STATUS
{
    CM_STATUS_QUEUED         ,
    CM_STATUS_FLUSHED        = 1,
    CM_STATUS_FINISHED       = 2,
    CM_STATUS_STARTED        = 3
} CM_STATUS;


// CM RT DLL File Version
typedef struct _CM_DLL_FILE_VERSION
{
    WORD    wMANVERSION;
    WORD    wMANREVISION;
    WORD    wSUBREVISION;
    WORD    wBUILD_NUMBER;
    //Version constructed as : "wMANVERSION.wMANREVISION.wSUBREVISION.wBUILD_NUMBER"
} CM_DLL_FILE_VERSION, *PCM_DLL_FILE_VERSION;

// Cm Device Create Option
#define CM_DEVICE_CREATE_OPTION_DEFAULT                     0
#define CM_DEVICE_CREATE_OPTION_SCRATCH_SPACE_DISABLE       1
#define CM_DEVICE_CREATE_OPTION_TDR_DISABLE                 64  //Work only for DX11 so far

#define CM_DEVICE_CREATE_OPTION_SURFACE_REUSE_ENABLE        1024


#define CM_MAX_DEPENDENCY_COUNT         8

typedef enum _CM_DEPENDENCY_PATTERN
{
    CM_NONE_DEPENDENCY          ,    //All threads run parallel, scanline dispatch
    CM_WAVEFRONT                = 1,
    CM_WAVEFRONT26              = 2,
    CM_VERTICAL_DEPENDENCY      = 3,
    CM_HORIZONTAL_DEPENDENCY    = 4,
    CM_WAVEFRONT26Z             = 5,
    CM_WAVEFRONT26ZI            = 8
} CM_DEPENDENCY_PATTERN;

typedef enum _CM_WALKING_PATTERN
{
    CM_WALK_DEFAULT      ,
    CM_WALK_WAVEFRONT    = 1,
    CM_WALK_WAVEFRONT26  = 2,
    CM_WALK_VERTICAL     = 3,
    CM_WALK_HORIZONTAL   = 4
} CM_WALKING_PATTERN;

typedef struct _CM_DEPENDENCY
{
    UINT    count;
    INT     deltaX[CM_MAX_DEPENDENCY_COUNT];
    INT     deltaY[CM_MAX_DEPENDENCY_COUNT];
}CM_DEPENDENCY;

typedef enum _CM_26ZI_DISPATCH_PATTERN
{
    VVERTICAL_HVERTICAL_26           = 0,
    VVERTICAL_HHORIZONTAL_26         = 1,
    VVERTICAL26_HHORIZONTAL26        = 2,
    VVERTICAL1X26_HHORIZONTAL1X26    = 3
} CM_26ZI_DISPATCH_PATTERN;


class SurfaceIndex
{
public:
	SurfaceIndex() { index = 0; };
	SurfaceIndex(const SurfaceIndex& _src) { index = _src.index; };
	SurfaceIndex(const unsigned int& _n) { index = _n; };
	SurfaceIndex& operator = (const unsigned int& _n) { this->index = _n; return *this; };
	SurfaceIndex& operator + (const unsigned int& _n) { this->index += _n; return *this; };
    virtual unsigned int get_data(void) { return index; };

private:
    unsigned int index;
    unsigned char extra_byte;
};

class CmEvent
{
public:
        INT GetStatus( CM_STATUS &status);
        INT GetExecutionTime(UINT64 &time);
        INT WaitForTaskFinished(DWORD dwTimeOutMs = CM_MAX_TIMEOUT_MS);

};

class CmThreadSpace;
class CmThreadGroupSpace
{
public:
	INT GetThreadGroupSpaceSize(UINT & threadSpaceWidth, UINT & threadSpaceHeight, UINT & groupSpaceWidth, UINT & groupSpaceHeight);

};

class CmKernel
{
public:
        INT SetThreadCount(UINT count ) ;
        INT SetKernelArg(UINT index, size_t size, const void * pValue ) ;

        INT SetThreadArg(UINT threadId, UINT index, size_t size, const void * pValue ) ;

        INT AssociateThreadSpace(CmThreadSpace* & pTS) ;
        INT AssociateThreadGroupSpace(CmThreadGroupSpace* & pTGS) ;

};

class CmTask
{
public:
        INT AddKernel(CmKernel *pKernel) ;
        INT Reset(void) ;
	INT AddSync(void);
};

class CmProgram
{
public:
	INT GetKernelCount( UINT& kernelCount );
};

class SurfaceIndex;

class CmBuffer
{
public:
        virtual INT GetIndex( SurfaceIndex*& pIndex ) ;
	/* The following Read/Write of buffer is not required as when it is
	 * shared between Cm and other component.
	 */
        virtual INT ReadSurface( unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL ) ;
        virtual INT WriteSurface( const unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL ) ;
};


class CmSurface2D
{
public:
        INT GetIndex( SurfaceIndex*& pIndex ) ;
        INT SetSurfaceStateDimensions(UINT iWidth, UINT iHeight, SurfaceIndex* pSurfIndex = NULL) ;

	/* The following Read/Write of 2D surface is not required as when it is
	 * shared between Cm and other component.
	 */

        INT ReadSurface( unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL ) ;
        INT WriteSurface( const unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL ) ;
        INT ReadSurfaceStride( unsigned char* pSysMem, CmEvent* pEvent, const UINT stride, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL ) ;
        INT WriteSurfaceStride( const unsigned char* pSysMem, CmEvent* pEvent, const UINT stride, UINT64 sysMemSize = 0xFFFFFFFFFFFFFFFFULL ) ;
};



class CmThreadSpace
{
public:
        INT SelectThreadDependencyPattern ( CM_DEPENDENCY_PATTERN pattern ) ;
        INT AssociateThread( UINT x, UINT y, CmKernel* pKernel , UINT threadId, BYTE nDependencyMask = 0 ) ;
        INT Set26ZIDispatchPattern( CM_26ZI_DISPATCH_PATTERN pattern ) ;
        INT Set26ZIMacroBlockSize( UINT width, UINT height )  ;
};

class CmThreadGroupSpace;

class CmQueue
{
public:
        INT Enqueue( CmTask* pTask, CmEvent* & pEvent, const CmThreadSpace* pTS = NULL ) ;
        INT DestroyEvent( CmEvent* & pEvent ) ;

public:
        INT EnqueueWithGroup( CmTask* pTask, CmEvent* & pEvent, const CmThreadGroupSpace* pTGS = NULL ) ;
        INT EnqueueWithHints( CmTask* pTask, CmEvent* & pEvent, UINT hints ) ;
};


class CmDevice
{
public:

        INT CreateBuffer(UINT size, CmBuffer* & pSurface );
        INT CreateSurface2D(UINT width, UINT height, CM_SURFACE_FORMAT format, CmSurface2D* & pSurface ) ;

        /* The following are preferred API that is used to create the buffer/2d surface */
        INT CreateSurface2D( CmOsResource *pOsResource, CmSurface2D* & pSurface ) ;
        INT CreateBuffer(CmOsResource *pOsResource, CmBuffer* & pSurface );

        INT DestroySurface( CmBuffer* & pSurface) ;
        INT DestroySurface( CmSurface2D* & pSurface) ;


        INT GetSurface2DInfo( UINT width, UINT height, CM_SURFACE_FORMAT format, UINT & pitch, UINT & physicalSize);

        INT CreateQueue( CmQueue* & pQueue) ;
        INT LoadProgram( void* pCommonISACode, const UINT size, CmProgram*& pProgram, const char* options = NULL ) ;
        INT CreateKernel( CmProgram* pProgram, const char* kernelName, CmKernel* & pKernel, const char* options = NULL) ;
        INT DestroyKernel( CmKernel*& pKernel) ;
        INT DestroyProgram( CmProgram*& pProgram ) ;

        INT CreateTask(CmTask *& pTask) ;
        INT DestroyTask(CmTask*& pTask) ;

        INT CreateThreadGroupSpace( UINT thrdSpaceWidth, UINT thrdSpaceHeight, UINT grpSpaceWidth, UINT grpSpaceHeight, CmThreadGroupSpace*& pTGS ) ;
        INT DestroyThreadGroupSpace(CmThreadGroupSpace*& pTGS) ;

        INT CreateThreadSpace( UINT width, UINT height, CmThreadSpace* & pTS ) ;
        INT DestroyThreadSpace( CmThreadSpace* & pTS ) ;

        INT GetRTDllVersion(CM_DLL_FILE_VERSION* pFileVersion) ;
};

/*
 * This function depends on which parameter is required by low-level Cm-device.
 * a. whether CM is using its own DRM buffer management or sharing the buffer management with other driver.
 * b. Device ID is passed.
 * c. The CmDriverContext will provide the enough info for the low-level driver
 */
EXTERN_C INT CreateCmDevice(CmDevice* &pDevice, UINT & version, CmDriverContext *drivercontext, UINT DevCreateOption = CM_DEVICE_CREATE_OPTION_DEFAULT);

EXTERN_C INT DestroyCmDevice(CmDevice* &pDevice);

#endif /* _Cm_H_ */
