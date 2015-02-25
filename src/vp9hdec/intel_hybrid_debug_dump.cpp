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
 *     Zhao Yakui <yakui.zhao@intel.com>
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <intel_hybrid_debug_dump.h>
#include <errno.h>
#include <stdlib.h>

#include <malloc.h>

static int intel_hybrid_createfile(
    int		*fd,
    char           *lpFileName,
    uint32_t                iOpenFlag)
{
    int               iFileDescriptor;
    uint32_t              mode;
    int error_index;

    if((lpFileName == NULL) || (fd == NULL))
    {
        return -EINVAL;
    }
    //set read/write access right for usr/group, mMode only takes effect when
    //O_CREAT is set
    mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
    if ( (iFileDescriptor = open(lpFileName, iOpenFlag, mode)) < 0 )
    {
        *fd = 0;
        error_index = errno;
        return -error_index;
    }

    *fd = iFileDescriptor;
    return 0;
}

static int intel_hybrid_writefile(
    int           fd,
    void            *lpBuffer,
    uint32_t           bytesToWrite,
    uint32_t          *pbytesWritten)
{
    size_t    nNumBytesToWrite;
    ssize_t   nNumBytesWritten;

    if((fd == NULL) || (lpBuffer == NULL) || (pbytesWritten == NULL))
    {
        return -EINVAL;
    }

    nNumBytesToWrite = (size_t)bytesToWrite;
    nNumBytesWritten = 0;


    if ((nNumBytesWritten = write(fd, lpBuffer, nNumBytesToWrite)) < 0)
    {
        int ret_val = -errno;
        *pbytesWritten = 0;
        return ret_val;
    }

    *pbytesWritten = (uint32_t)nNumBytesWritten;
    return 0;
}

int intel_hybrid_writefilefromptr(
    char                  *pFilename,
    void                   *lpBuffer,
    uint32_t                  writeSize)
{
    int         fd;
    uint32_t          bytesWritten;
    int ret_val = 0;

    bytesWritten    = 0;

    ret_val = intel_hybrid_createfile(&fd, pFilename, O_WRONLY|O_CREAT | O_TRUNC);

    if (ret_val) {
        printf("failed to open %s, err %d\n", pFilename, ret_val);
        return ret_val;
    }

    if((ret_val = intel_hybrid_writefile(fd, lpBuffer, writeSize, &bytesWritten)))
    {
        printf("failed to write %s, err %d\n", pFilename, ret_val);
        close(fd);
        return ret_val;
    }

    close(fd);
    return 0;
}


int intel_hybrid_appendfilefromptr(
    char                     *pFilename,
    void                    *pData,
    uint32_t                    dwSize)
{
    int ret_val = 0;
    int fd;
    uint32_t      dwWritten;

    dwWritten   = 0;

    ret_val = intel_hybrid_createfile(&fd, pFilename, O_WRONLY|O_CREAT);
    if (ret_val)
    {
        printf("Failed to Create file %s\n", pFilename);
        return ret_val;
    }

    if (lseek(fd, 0, SEEK_END) < 0)
    {
        printf("Failed to seek %s, err %d\n", pFilename, ret_val);
        close(fd);
        return ret_val;
    }

    // Write the file
    if(intel_hybrid_writefile(fd, pData, dwSize, &dwWritten))
    {
        printf("Failed to write to file %s ", pFilename);
        close(fd);
        return -1;
    }

    close(fd);
    return ret_val;
}


char *intel_alloc_zero_aligned_memory(uint32_t size, unsigned int alignment)
{
     char *ptr;
     size_t tmp_size;

     tmp_size = ALIGN(size, alignment);
     ptr = memalign(alignment, tmp_size);

     if (ptr) {
         memset(ptr, 0, tmp_size);
     }

     return ptr;
}


