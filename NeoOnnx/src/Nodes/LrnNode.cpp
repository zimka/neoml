/* Copyright © 2017-2020 ABBYY Production LLC

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

#include "LrnNode.h"
#include "NeoOnnxCheck.h"
#include "TensorUtils.h"

#include "onnx.pb.h"

namespace NeoOnnx {

CLrnNode::CLrnNode( const onnx::NodeProto& lrn, int opsetVersion ) :
	COpNode( lrn, opsetVersion )
{
	// Introduce in opset v1
	CheckNeoOnnxSupport( OpsetVersion >= 1 && OpsetVersion <= MaxOpsetVersion, "opset version", lrn );

	CheckOnnxProtocol( InputCount() == 1, "node must have 1 input", lrn );
	CheckOnnxProtocol( OutputCount() == 1, "node must have 1 output", lrn );
}

void CLrnNode::AddLayers( const CObjectArray<const CTensorBase>& inputs,
	CObjectArray<const CTensorBase>& outputs, CDnn& dnn )
{
	CheckNeoOnnxInternal( inputs[0] != nullptr && !inputs[0]->IsCalculated(), "Input must be provided by user", OnnxNode );
	CheckNeoOnnxSupport( inputs[0]->Shape().Size() <= 5, "6+ dimensional input", OnnxNode );

	CDimOrder dimOrder( { BD_BatchWidth, BD_Channels, BD_Height, BD_Width, BD_Depth } );
	dimOrder.SetSize( inputs[0]->Shape().Size() );

	CPtr<const CUserTensor> input = dynamic_cast<const CUserTensor*>( ConvertTensor( *inputs[0], CTensorLayout( dimOrder ) ).Ptr() );

	CPtr<CLrnLayer> lrn = new CLrnLayer( dnn.GetMathEngine() );
	lrn->SetName( Name() );
	lrn->SetWindowSize( Attributes.GetRequiredInt( "size" ) );
	lrn->SetBias( Attributes.GetOptionalFloat( "bias", 1 ) );
	lrn->SetAlpha( Attributes.GetOptionalFloat( "alpha", 1e-4f ) );
	lrn->SetBeta( Attributes.GetOptionalFloat( "beta", 0.75f ) );
	lrn->Connect( 0, *input->Layer(), input->OutputIndex() );
	dnn.AddLayer( *lrn );

	outputs[0] = new CUserTensor( input->Shape(), input->Layout(), CLayerOutput( lrn, 0 ) );
}


} // namespace NeoOnnx