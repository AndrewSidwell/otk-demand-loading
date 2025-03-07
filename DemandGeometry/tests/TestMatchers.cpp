//
//  Copyright (c) 2023 NVIDIA Corporation.  All rights reserved.
//
//  NVIDIA Corporation and its licensors retain all intellectual property and proprietary
//  rights in and to this software, related documentation and any modifications thereto.
//  Any use, reproduction, disclosure or distribution of this software and related
//  documentation without an express license agreement from NVIDIA Corporation is strictly
//  prohibited.
//
//  TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED *AS IS*
//  AND NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES, EITHER EXPRESS OR IMPLIED,
//  INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
//  PARTICULAR PURPOSE.  IN NO EVENT SHALL NVIDIA OR ITS SUPPLIERS BE LIABLE FOR ANY
//  SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT
//  LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF
//  BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS) ARISING OUT OF THE USE OF OR
//  INABILITY TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF
//  SUCH DAMAGES
//

#include <OptiXToolkit/DemandGeometry/Mocks/Matchers.h>

#include <optix.h>

#include <gtest/gtest.h>

using namespace testing;
using namespace otk::testing;

namespace {

class TestIsInstanceBuildInput : public Test
{
  protected:
    void SetUp() override
    {
        //
        m_input.type = OPTIX_BUILD_INPUT_TYPE_INSTANCES;
    }

    OptixBuildInput m_input{};
};

}  // namespace

TEST_F( TestIsInstanceBuildInput, differentBuiltType )
{
    m_input.type = OPTIX_BUILD_INPUT_TYPE_CURVES;

    EXPECT_THAT( &m_input, Not( isInstanceBuildInput( 0 ) ) );
}

TEST_F( TestIsInstanceBuildInput, matchesBuildInput )
{
    EXPECT_THAT( &m_input, isInstanceBuildInput( 0 ) );
}
