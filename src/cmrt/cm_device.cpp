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
#include <string.h>

#include "cm_rt.h"

/* The following API is only used when the created buffer is used by
 * Cm internally.
 */
INT CmDevice::CreateBuffer(UINT size, CmBuffer* & pSurface )
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmDevice::CreateSurface2D(UINT width, UINT height, CM_SURFACE_FORMAT format, CmSurface2D* & pSurface )
{
	INT result = CM_SUCCESS;
	return result;
}

/*-----------------------------------------------------------------------------
 Purpose:    Create shared Surface 2D (OS agnostic)
 Arguments :
               pMosResource      [in]     Pointer to CM os resource 
               pSurface          [out]    Reference to Pointer to CmSurface2D
 Returns:    Result of the operation.
*/

INT CmDevice::CreateSurface2D( CmOsResource *pOsResource, CmSurface2D* & pSurface )
{
	INT result = CM_SUCCESS;
	return result;
}

/*-----------------------------------------------------------------------------
 Purpose:    Create shared Buffer 2D (OS agnostic)
 Arguments :
               pMosResource      [in]     Pointer to CM os resource 
               pSurface          [out]    Reference to Pointer to CmBuffer
 Returns:    Result of the operation.
*/
INT CmDevice::CreateBuffer(CmOsResource *pOsResource, CmBuffer* & pSurface )
{
	INT result = CM_SUCCESS;
	return result;
}

/* Surface destroy */
INT CmDevice::DestroySurface( CmBuffer* & pSurface)
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmDevice::DestroySurface( CmSurface2D* & pSurface)
{
	INT result = CM_SUCCESS;
	return result;
}

/*
 * The following API are also used when it needs to share buffer between
 * other components and Cm. But it relies on the mapping from user-ptr
 * to gfx memory. 
 */

/*----------------------------------------------------------------------------
 Purpose:    Destroy BufferUp
 Arguments :  pSurface          [in]    Reference to Pointer to CmBuffer 
 Returns:    Result of the operation.
 */
/*----------------------------------------------------------------------------
 Purpose:    Get Surface2D information: pitch and physical size in Video memory

 Returns:    Result of the operation.
 */
INT CmDevice::GetSurface2DInfo( UINT width, UINT height, CM_SURFACE_FORMAT format, UINT & pitch, UINT & physicalSize)
{
	INT result = CM_SUCCESS;
	return result;
}

/*--------------------------------------------------------------------------------------------
 Purpose:    Create Surface 2D UP
 Arguments :   width             [in]     width of the  CmSurface2DUP
               height            [in]     height of the CmSurface2DUP
               format            [in]     format of the CmSurface2DUP

               pSysMem           [in]     Pointer to host memory, must be page(4K bytes)-aligned.
               pSurface          [in/out]  Reference to  Pointer to CmSurface2DUP
 Returns:    Result of the operation.
*/


/*-----------------------------------------------------------------------------
 Purpose:    Load program from memory
 Arguments :
               pCommonISACode    [in]       pointer to memory where common isa locates
               size              [in]       size of common isa
               pProgram          [in/out]   Pointer to CmProgram
               options           [in]       options : non-jitter,jitter

 Returns:    Result of the operation.
*/
INT CmDevice::LoadProgram( void* pCommonISACode, const UINT size, CmProgram*& pProgram, const char* options)
{
	INT result = CM_SUCCESS;
	return result;
}

/*-----------------------------------------------------------------------------
| Purpose:    Destroy Program
| Returns:    Result of the operation.
*/
INT DestroyProgram( CmProgram*& pProgram )
{
	if (pProgram) {
		delete pProgram;
		pProgram = NULL;
	}
	
	INT result = CM_SUCCESS;
	return result;
}


/*-----------------------------------------------------------------------------
 Purpose:    Create Kernel
 Arguments :
               pKernel           [out]      pointer to CmKernel
               kernelName        [in]       string of kernel's name
               pProgram          [in/out]   Pointer to CmProgram
               options           [in]       options : non-jitter,jitter

 Returns:    Result of the operation.
 */
INT CmDevice::CreateKernel( CmProgram* pProgram, const char* kernelName, CmKernel* & pKernel, const char* options )
{
	INT result = CM_SUCCESS;
	return result;
}


/*-----------------------------------------------------------------------------
 Purpose:    Destroy Kernel
 Returns:    Result of the operation.
*/
INT CmDevice::DestroyKernel( CmKernel*& pKernel)
{
	INT result = CM_SUCCESS;
	if( pKernel == NULL ) {
		return CM_FAILURE;	
	} else {
		delete pKernel;
		pKernel = NULL;
	}

	return result;
}

/*
 * Create one CmQueue for one device
 */
INT CmDevice::CreateQueue( CmQueue* & pQueue)
{
	INT result = CM_SUCCESS;
	return result;
}

/* Create one Task for one device */
INT CmDevice::CreateTask(CmTask *& pTask)
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmDevice::DestroyTask(CmTask*& pTask)
{
	INT result = CM_SUCCESS;

        if (pTask) {
		delete pTask;
		pTask = NULL;
	}
	return result;
}


/*-----------------------------------------------------------------------------
 Create a 2-dimensional dependency board. Each board is corrsponding to a task.
 Each board unit is notated as a pair of X/Y coordinates, which is in the range of [0, width -1] or [0. heigh-1]
 Each board uint is correspinding to a thread in the task.
 Input :
     1) width and height of the dependency board
 OUTPUT :
     CM_SUCCESS if CmThreadSpace is successfully created.
*/
INT CmDevice::CreateThreadSpace( UINT width, UINT height, CmThreadSpace* & pTS )
{
	INT result = CM_SUCCESS;
	return result;
}

/*-----------------------------------------------------------------------------
 Purpose:    Destroy Thread Space
 Returns:    Result of the operation.
 */
INT CmDevice::DestroyThreadSpace( CmThreadSpace* & pTS )
{
	INT result = CM_SUCCESS;
	return result;
}


/*-----------------------------------------------------------------------------
 Function to create a thread group space
 Arguments: 
        1. Width/height (in unit of thread ) of each thread group
        2. Width/height(in unit of group) of thread group space. 
        3. Reference to the point to CmThreadGroupSpace object to created.
 Return Value: 
        CM_SUCCESS if the CmThreadGroupSpace is successfully created 
 Notes: 
        The total thread count is width*height*grpWidth*grpHeight. 
        The thread count will check against the thread count set by CmKernel::SetThreadCount if CmKernel::SetThreadCount is called. 
        CmKernel::SetThreadCount needs to be called if CmKernel::SetThreadArg is to be called.
*/
INT CmDevice::CreateThreadGroupSpace( UINT thrdSpaceWidth, UINT thrdSpaceHeight, UINT grpSpaceWidth, UINT grpSpaceHeight, CmThreadGroupSpace*& pTGS )
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmDevice::DestroyThreadGroupSpace(CmThreadGroupSpace*& pTGS)
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmDevice::GetRTDllVersion(CM_DLL_FILE_VERSION* pFileVersion)
{
	INT result = CM_SUCCESS;
	return result;

}

/*-----------------------------------------------------------------------------
 Purpose:    Create Cm Device 
 Returns:    Result of the operation.
 */
INT CreateCmDevice(CmDevice* &pDevice, UINT & version, CmDriverContext *drivercontext, UINT  DevCreateOption)
{
	INT result = CM_SUCCESS;
	pDevice = new CmDevice();

	return result;
}

/*-----------------------------------------------------------------------------
 Purpose:    Destroy Cm Device 
 Returns:    Result of the operation.
 */
INT DestroyCmDevice(CmDevice* &pDevice)
{
	INT result = CM_SUCCESS;
	if (pDevice) {
		delete pDevice;
		pDevice = NULL; 
	}
	return result;
}



INT CmEvent::GetStatus( CM_STATUS& status)
{
	INT result = CM_SUCCESS;
	return result;
}
INT CmEvent::GetExecutionTime(UINT64& time)
{
	INT result = CM_SUCCESS;
	return result;
}
INT CmEvent::WaitForTaskFinished(DWORD dwTimeOutMs)
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmKernel::SetThreadCount(UINT count )
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmKernel::SetKernelArg(UINT index, size_t size, const void * pValue )
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmKernel::SetThreadArg(UINT threadId, UINT index, size_t size, const void * pValue )
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmKernel::AssociateThreadSpace(CmThreadSpace* & pTS)
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmKernel::AssociateThreadGroupSpace(CmThreadGroupSpace* & pTGS)
{
	INT result = CM_SUCCESS;
	return result;
}


INT CmProgram::GetKernelCount( UINT& kernelCount )
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmBuffer::GetIndex( SurfaceIndex*& pIndex )
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmBuffer::ReadSurface( unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize)
{
	INT result = CM_SUCCESS;
	return result;

}

INT CmBuffer::WriteSurface( const unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize)
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmSurface2D::GetIndex( SurfaceIndex*& pIndex )
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmSurface2D::ReadSurface( unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize)
{

	INT result = CM_SUCCESS;
	return result;
}

INT CmSurface2D::WriteSurface( const unsigned char* pSysMem, CmEvent* pEvent, UINT64 sysMemSize )
{

	INT result = CM_SUCCESS;
	return result;
}

INT CmSurface2D::ReadSurfaceStride( unsigned char* pSysMem, CmEvent* pEvent, const UINT stride, UINT64 sysMemSize)
{

	INT result = CM_SUCCESS;
	return result;
}

INT CmSurface2D::WriteSurfaceStride( const unsigned char* pSysMem, CmEvent* pEvent,
					const UINT stride, UINT64 sysMemSize)
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmSurface2D::SetSurfaceStateDimensions(UINT iWidth, UINT iHeight, SurfaceIndex* pSurfIndex)
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmThreadSpace:: AssociateThread( UINT x, UINT y, CmKernel* pKernel , UINT threadId, BYTE nDependecnyMask )
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmThreadSpace::SelectThreadDependencyPattern ( CM_DEPENDENCY_PATTERN pattern )
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmThreadSpace::Set26ZIDispatchPattern( CM_26ZI_DISPATCH_PATTERN pattern ) 
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmThreadSpace::Set26ZIMacroBlockSize( UINT width, UINT height ) 
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmThreadGroupSpace::GetThreadGroupSpaceSize(UINT & threadSpaceWidth, UINT & threadSpaceHeight, UINT & groupSpaceWidth, UINT & groupSpaceHeight)
{

	INT result = CM_SUCCESS;
	return result;
}


INT CmTask::AddKernel(CmKernel *pKernel)
{

	INT result = CM_SUCCESS;
	return result;
}

INT CmTask::Reset(void)
{

	INT result = CM_SUCCESS;
	return result;
}

INT CmTask::AddSync(void)
{
	INT result = CM_SUCCESS;
	return result;
}



INT CmQueue::Enqueue( CmTask* pTask, CmEvent* & pEvent, const CmThreadSpace* pTS )
{
	INT result = CM_SUCCESS;
	return result;
}
INT CmQueue::DestroyEvent( CmEvent* & pEvent )
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmQueue::EnqueueWithHints( CmTask* pTask, CmEvent* & pEvent, UINT hints )
{
	INT result = CM_SUCCESS;
	return result;
}

INT CmQueue::EnqueueWithGroup( CmTask* pTask, CmEvent* & pEvent, const CmThreadGroupSpace* pTGS )
{
	INT result = CM_SUCCESS;
	return result;
}
