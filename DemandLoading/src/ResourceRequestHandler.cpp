//
// Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
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

#include "ResourceRequestHandler.h"
#include "DemandLoaderImpl.h"

namespace demandLoading {

void ResourceRequestHandler::fillRequest( CUstream stream, unsigned int pageIndex ) 
{
    // We use MutexArray to ensure mutual exclusion on a per-page basis.  This is necessary because
    // multiple streams might race to fill the same tile (or the mip tail).
    unsigned int index = pageIndex - m_startPage;
    MutexArrayLock lock( m_mutex.get(), index);

    // Do nothing if the request has already been filled.
    PagingSystem* pagingSystem = m_loader->getPagingSystem();
    if( pagingSystem->isResident( pageIndex ) )
        return;

    // Invoke the callback that was provided when the resource was created, which returns a new page table entry.
    void* pageTableEntry;
    if( m_callback( stream, pageIndex, m_callbackContext, &pageTableEntry ) )
    {
        // Add a page table mapping from the requested page index to the new page table entry.
        // Page table updates are accumulated in the PagingSystem until launchPrepare is called, which
        // sends them to the device (via PagingSystem::pushMappings).
        m_loader->setPageTableEntry( pageIndex, false, pageTableEntry );
    }
}

}
