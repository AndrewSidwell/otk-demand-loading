//
// Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include "PagingSystemTestKernels.h"
#include "CudaCheck.h"

#include <OptiXToolkit/DemandLoading/Paging.h>

using namespace demandLoading;

__global__ static void pageRequester( DeviceContext       context,
                                      unsigned int        numPages,
                                      const unsigned int* pageIds,
                                      unsigned long long* outputPages,
                                      bool*               pagesResident )
{
    unsigned int index = blockIdx.x + threadIdx.x;
    if( index >= numPages )
        return;

    outputPages[index] = pagingMapOrRequest( context, pageIds[index], &pagesResident[index] );
}

__host__ void launchPageRequester( CUstream             stream,
                                   const DeviceContext& context,
                                   unsigned int         numPages,
                                   const unsigned int*  pageIds,
                                   unsigned long long*  outputPages,
                                   bool*                pagesResident )
{
    unsigned int threadsPerBlock = 32;
    unsigned int numBlocks       = ( numPages + threadsPerBlock - 1 ) / threadsPerBlock;
    pageRequester<<<numBlocks, threadsPerBlock, 0U, stream>>>( context, numPages, pageIds, outputPages, pagesResident );
    DEMAND_CUDA_CHECK( cudaStreamSynchronize( stream ) );
    DEMAND_CUDA_CHECK( cudaGetLastError() );
}
