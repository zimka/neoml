/* Copyright © 2017-2024 ABBYY

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
--------------------------------------------------------------------------------------------------------------*/

#include "../common.h"
#pragma hdrstop

#include "DropoutOperator.h"
#include "NeoOnnxCheck.h"

#include "onnx.pb.h"

using namespace NeoML;

namespace NeoOnnx {

CDropoutOperator::CDropoutOperator( const onnx::NodeProto& dropout, int opsetVersion ) :
	CLayerOperator( dropout, opsetVersion )
{
	// v1 - original
	// v6 - removed legacy optimization attribute
	// v7 - removed "is_test" attribute
	// v10 - changed second output data type
	// v12 - added "seed" attribute, "ratio" moved from attributes to inputs, "training_mode" added
	// v13 - bfloat16 is supported
	CheckNeoOnnxSupport( OpsetVersion >= 1 && OpsetVersion <= MaxOpsetVersion, "opset version", *this );

	if( OpsetVersion < 12 ) {
		CheckOnnxProtocol( InputCount() == 1, "operator must have 1 input", *this );
	} else {
		CheckOnnxProtocol( InputCount() >= 1 && InputCount() <= 3, "operator must have from 1 up to 3 inputs", *this );
	}
	CheckOnnxProtocol( OutputCount() == 1 || OutputCount() == 2, "operator must have 1 or 2 outputs", *this );
}

void CDropoutOperator::AddLayers( const CTensorArray& inputs, CDnn& dnn, CTensorArray& outputs ) const
{
	CheckNoShapeInputs( inputs );
	CheckOnnxProtocol( inputs[0] != nullptr, "input can't be optional", *this );
	CPtr<const CUserTensor> userInput = AsUserTensor( *inputs[0], Name() + "_Source", dnn );

	CPtr<CDropoutLayer> dropout = new CDropoutLayer( dnn.GetMathEngine() );
	dropout->SetName( Name() );
	dropout->SetDropoutRate( getRatio( inputs ) );
	dropout->Connect( 0, *userInput->Layer(), userInput->OutputIndex() );
	dnn.AddLayer( *dropout );

	outputs.Add( new CUserTensor( userInput->Layout(), CLayerOutput( dropout, 0 ) ) );
	if( OutputCount() == 2 ) {
		// neoml::CDropoutLayer doesn't support mask as output
		outputs.Add( nullptr );
	}
}

// Gets dropout rate
float CDropoutOperator::getRatio( const CTensorArray& inputs ) const
{
	if( OpsetVersion < 12 ) {
		// Before opset 12 ratio is stored as optional attribute with default value 0.5f
		float ratio = 0.5f;
		GetAttribute( "ratio", ratio );
		return ratio;
	} else if( inputs.Size() < 2 || inputs[1] == nullptr ) {
		// If "ratio" input is omitted, default value is 0.5f
		return 0.5f;
	}

	// Extracting data from input
	CheckNeoOnnxSupport( inputs[1]->Type() == TTensorType::Data, "User-provided ratio", *this );
	return dynamic_cast<const CDataTensor*>( inputs[1].Ptr() )->Data()->GetData().GetValue();
}

} // namespace NeoOnnx


#include "DepthToSpaceOperator.h"


namespace NeoOnnx {

CDepthToSpaceOperator::CDepthToSpaceOperator( const onnx::NodeProto& depthToSpace, int opsetVersion ) :
    CLayerOperator( depthToSpace, opsetVersion )
{
    // v1 - original
    // v11 - added "mode" attribute 
    // v13 - bfloat16 is supported
    CheckNeoOnnxSupport( OpsetVersion >= 1 && OpsetVersion <= MaxOpsetVersion, "opset version", *this );
    CheckOnnxProtocol( OutputCount() == 1, "operator must have 1 output", *this );
    CheckOnnxProtocol( InputCount() == 1, "operator must have 1 input", *this );
    GetAttribute( "blocksize", blockSize );
    CheckNeoOnnxSupport( blockSize > 1, "blocksize attribute must > 1", *this );

    CString mode {"DCR"};
    if( opsetVersion > 11 ) {
        // before v11 DCR was used by default
        GetAttribute( "mode", mode );
    };

    // For DepthToSpace 'mode' can be either DCR (depth-column-row) or CRD (column-row-depth)
    // ONNX uses channel-first layout, so depending on mode operator reshapes input as following:
    //    CRD: 'B (C block_size block_size) H W -> B C (H block_size) (W block_size)'
    //    DCR: 'B (block_size block_size C) H W -> B C (H block_size) (W block_size)'
    // NeoML CDepthToSpaceLayer uses channel-last layout and  rearranges input as following:
    //    'batch_length B list_size H W 1 (block_size block_size C) -> 
    //    batch_length B list_size (H block_size) (W block_size) C'
    // CDepthToSpaceLayer is equivalent to DCR, while CRD is not supported by NeoML
    CheckNeoOnnxSupport( mode == "DCR", "DCR is supported, CRD is not supported", *this );
}

void CDepthToSpaceOperator::AddLayers( const CTensorArray& inputs, CDnn& dnn, CTensorArray& outputs ) const
{
    CheckNoShapeInputs( inputs );
    CheckOnnxProtocol( inputs[0] != nullptr, "input can't be optional", *this );
    CheckNeoOnnxSupport( inputs[0]->DimCount() == 4, "can convert only 4d input", *this );

    CPtr<const CUserTensor> userInput = AsUserTensor( 
    	*ConvertTensor( *inputs[0], CNeoMLImageLayoutValidator() ), Name() + "_Source", dnn );

    CPtr<CDepthToSpaceLayer> depthToSpace = new CDepthToSpaceLayer( dnn.GetMathEngine() );
    depthToSpace->SetName( Name() );
    depthToSpace->SetBlockSize( blockSize );
    depthToSpace->Connect( 0, *userInput->Layer(), userInput->OutputIndex() );
    dnn.AddLayer( *depthToSpace );

    outputs.Add( new CUserTensor( userInput->Layout(), CLayerOutput( depthToSpace, 0 ) ) );
}

} // namespace NeoOnnx
