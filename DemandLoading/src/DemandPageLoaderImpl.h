//
// Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#pragma once

#include <OptiXToolkit/DemandLoading/DemandPageLoader.h>

#include <OptiXToolkit/Memory/Allocators.h>
#include "Memory/DeviceMemoryManager.h"
#include <OptiXToolkit/Memory/MemoryPool.h>
#include <OptiXToolkit/Memory/RingSuballocator.h>
#include "PageTableManager.h"
#include "PagingSystem.h"
#include "ResourceRequestHandler.h"
#include "Textures/DemandTextureImpl.h"
#include "Textures/SamplerRequestHandler.h"
#include "TransferBufferDesc.h"
#include "Util/PerContextData.h"
#include "Util/TraceFile.h"

#include <cuda.h>

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace imageSource {
class ImageSource;
}

namespace demandLoading {

struct DeviceContext;
class DemandTexture;
class RequestProcessor;
struct TextureDescriptor;
class TraceFileWriter;

/// DemandLoader demonstrates how to implement demand-loaded textures using the OptiX paging library.
class DemandPageLoaderImpl : public DemandPageLoader
{
  public:
    /// Construct demand loading sytem.
    DemandPageLoaderImpl( RequestProcessor* requestProcessor, const Options& options );

    DemandPageLoaderImpl( std::shared_ptr<PageTableManager> pageTableManager, RequestProcessor *requestProcessor, const Options& options );

    /// Destroy demand loading system.
    ~DemandPageLoaderImpl() override = default;

    static bool supportsSparseTextures( unsigned int deviceIndex );

    unsigned int allocatePages( unsigned int numPages, bool backed ) override;

    void setPageTableEntry( unsigned int pageId, bool evictable, void* pageTableEntry ) override;

    /// Prepare for launch.  The caller must ensure that the current CUDA context matches the given
    /// stream.  Returns false if the current device does not support sparse textures.  If
    /// successful, returns a DeviceContext via result parameter, which should be copied to device
    /// memory (typically along with OptiX kernel launch parameters), so that it can be passed to
    /// Tex2D().
    bool pushMappings( CUstream stream, DeviceContext& demandTextureContext ) override;

    /// Fetch page requests from the given device context and enqueue them for background
    /// processing.  The caller must ensure that the current CUDA context matches the given stream.
    /// The given stream is used when copying tile data to the device.  Returns a ticket that is
    /// notified when the requests have been filled.
    void pullRequests( CUstream stream, const DeviceContext& deviceContext, unsigned int id ) override;

    /// Replay the given page requests (from a trace file), adding them to the page request queue
    /// for asynchronous processing.  The caller must ensure that the current CUDA context matches
    /// the given stream.
    void replayRequests( CUstream stream, unsigned int id, const unsigned int* pageIds, unsigned int numPageIds );

    /// Get the demand loading configuration options.
    const Options& getOptions() const { return m_options; }

    unsigned int getNumDevices() const { return m_numDevices; }

    /// Get indices of the devices that can be employed by the DemandLoader.
    std::vector<unsigned int> getDevices() const override { return m_devices; }

    /// Turn on or off eviction
    void enableEviction( bool evictionActive ) override { m_options.evictionActive = evictionActive; }

    /// Get the DeviceMemoryManager for the current CUDA context.
    DeviceMemoryManager* getDeviceMemoryManager() const;

    otk::MemoryPool<otk::PinnedAllocator, otk::RingSuballocator> *getPinnedMemoryPool() { return &m_pinnedMemoryPool; }

    /// Get the PagingSystem for the current CUDA context.
    PagingSystem* getPagingSystem() const;

    void setMaxTextureMemory( size_t maxMem );

    void invalidatePageRange( unsigned int startPage, unsigned int endPage, PageInvalidatorPredicate* predicate );

    double getTotalProcessingTime() const { return m_totalProcessingTime; }

    /// Accumulate statistics into the given struct.
    void accumulateStatistics( Statistics& stats ) const;

  private:
    mutable std::mutex        m_mutex;
    Options                   m_options;
    unsigned int              m_numDevices;
    std::vector<unsigned int> m_devices;  // Indices of supported devices.

    mutable PerContextData<DeviceMemoryManager> m_deviceMemoryManagers;  // Manages device memory (one per CUDA context)
    mutable PerContextData<PagingSystem>        m_pagingSystems;  // Manages device interaction (one per CUDA context)

    mutable std::mutex m_deviceMemoryManagersMutex;
    mutable std::mutex m_pagingSystemsMutex;

    struct InvalidationRange
    {
        unsigned int startPage;
        unsigned int endPage;
        PageInvalidatorPredicate* predicate;
    };
    PerContextData<std::vector<InvalidationRange>> m_pagesToInvalidate;
    std::mutex m_pagesToInvalidateMutex;

    std::shared_ptr<PageTableManager> m_pageTableManager;  // Allocates ranges of virtual pages.
    RequestProcessor*   m_requestProcessor;  // Processes page requests.

    mutable otk::MemoryPool<otk::PinnedAllocator, otk::RingSuballocator> m_pinnedMemoryPool;

    double m_totalProcessingTime{};

    std::vector<std::unique_ptr<RequestHandler>> m_requestHandlers;

    // Invalidate the pages for current device in m_pagesToInvalidate
    void invalidatePages( CUstream stream, DeviceContext& context );

    std::vector<InvalidationRange>* getPagesToInvalidate();
};

}  // namespace demandLoading
