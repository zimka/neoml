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

#pragma once

#include <NeoML/NeoMLDefs.h>
#include <NeoML/TraditionalML/FloatVector.h>
#include <NeoML/TraditionalML/ClassificationResult.h>
#include <NeoML/TraditionalML/TrainingModel.h>
#include <NeoML/Random.h>

namespace NeoML {

class IRegressionTreeNode;
template<class T>
class CGradientBoostFullTreeBuilder;
class CGradientBoostStatisticsSingle;
class CGradientBoostStatisticsMulti;
template<class T>
class CGradientBoostFastHistTreeBuilder;
class IGradientBoostingLossFunction;
class CGradientBoostModel;
class CGradientBoostFullProblem;
class CGradientBoostFastHistProblem;

// Decision tree ensemble that has been built by gradient boosting
using CGradientBoostEnsemble = CObjectArray<IRegressionTreeNode>;

inline void ArrayMemMoveElement( CGradientBoostEnsemble* dest, CGradientBoostEnsemble* src )
{
	NeoPresume( dest != src );
	::new( dest ) CGradientBoostEnsemble;
	src->MoveTo( *dest );
	src->~CGradientBoostEnsemble();
}

// The type of tree builder used in the gradient boosting algorithm
enum TGradientBoostTreeBuilder {
	// All feature values may be used for subtree splitting
	// This algorithm can provide better quality 
	// and may be used even in the case where the whole problem data does not fit into memory
	GBTB_Full = 0,
	// The steps of a histogram built on the feature values will be used for subtree splitting
	// The params.MaxBins value sets the histogram size
	// The algorithm will cache all the problem data
	// This algorithm is faster and works best for binary problems that fit into memory
	GBTB_FastHist,
	// Similar to GBTB_Full but with multiclass trees,
	// which leaves contain a vector of values for all classes
	GBTB_MultiFull,
	// Similar to GBTB_FastHist but with multiclass trees,
	// which leaves contain a vector of values for all classes
	GBTB_MultiFastHist,
	GBTB_Count
};

// Different model representations CGradientBoost can produce.
enum TGradientBoostModelRepresentation {
	// Straightforward representation used during training and for backward compatibility.
	GBMR_Linked,
	// Optimized for low memory impact.
	// Limited to 64K nodes per tree and (64K - 1) features.
	GBMR_Compact,
	// Optimized for large numbers of trees of moderate depths.
	GBMR_QuickScorer
};

// Gradient tree boosting
class NEOML_API CGradientBoost : public ITrainingModel, public IRegressionTrainingModel {
public:
	// Supported loss functions
	enum TLossFunction {
		LF_Exponential, // AdaBoost - classification only
		LF_Binomial, // LogitBoost - classification only
		LF_SquaredHinge, // smoothed squared hinge - classification only
		LF_L2, // quadratic function
		LF_Undefined
	};

	// Classification parameters
	struct CParams {
		TLossFunction LossFunction; // the loss function
		int IterationsCount; // the maximum number of iterations (the number of trees in the ensemble)
		float LearningRate; // the multiplier of each classifier
		float Subsample; // the fraction of input data that is used for building one tree; may be from 0 to 1
		float Subfeature; // the fraction of features that is used for building one tree; may be from 0 to 1
		CRandom* Random; // the random numbers generator for selecting Subsample vectors and Subfeature features out of the whole
		int MaxTreeDepth; // the maximum depth of each tree
		int MaxNodesCount; // the maximum number of nodes in a tree (set to -1 for no limitation)
		// Note that the L1RegFactor, L2RegFactor, PruneCriterionValue parameters are applied 
		// to the values depending on the total vector weight in the corresponding tree node. 
		// Therefore when setting up these parameters, you need to take into consideration 
		// the number and weights of the vectors in your training data set.
		float L1RegFactor; // the L1 regularization factor
		float L2RegFactor; // the L2 regularization factor
		// The value of criterion difference when the nodes should be merged (set to 0 to never merge)
		float PruneCriterionValue;
		int ThreadCount; // the number of processing threads to be used while training the model
		TGradientBoostTreeBuilder TreeBuilder; // the type of tree builder used
		int MaxBins; // the largest possible histogram size to be used in *GBTB_FastHist* mode
		float MinSubsetWeight; // the minimum subtree weight (set to 0 to have no lower limit)
		float DenseTreeBoostCoefficient; // the dense tree boost coefficient (only for GBTB_MultiFull)
		// Representation of training result.
		TGradientBoostModelRepresentation Representation;

		CParams() :
			LossFunction( LF_Binomial ),
			IterationsCount( 100 ),
			LearningRate( 0.1f ),
			Subsample( 1.f ),
			Subfeature( 1.f ),
			Random( 0 ),
			MaxTreeDepth( 10 ),
			MaxNodesCount( NotFound ),
			L1RegFactor( 0.f ),
			L2RegFactor( 1.f ),
			PruneCriterionValue( 0.f ),
			ThreadCount( 1 ),
			TreeBuilder( GBTB_Full ),
			MaxBins( 32 ),
			MinSubsetWeight( 0.f ),
			DenseTreeBoostCoefficient( 0.f ),
			Representation( GBMR_Compact )
		{
		}
	};

	explicit CGradientBoost( const CParams& params );
	~CGradientBoost() override;

	// Sets a text stream for logging processing
	void SetLog( CTextStream* newLog ) { logStream = newLog; }

	// Trains the multivariate regression model
	CPtr<IMultivariateRegressionModel> TrainRegression(
		const IMultivariateRegressionProblem& problem );

	// IRegressionTrainingModel interface methods:
	CPtr<IRegressionModel> TrainRegression( const IRegressionProblem& problem ) override;

	// ITrainingModel interface methods:
	CPtr<IModel> Train( const IProblem& problem ) override;

	// Returns the last loss mean
	double GetLastLossMean() const { return loss; }

	// Train one iteration
	// returns true if currentIteration >= params.IterationsCount
	bool TrainStep( const IProblem& _problem );
	bool TrainStep( const IRegressionProblem& _problem );
	bool TrainStep( const IMultivariateRegressionProblem& _problem );

	// Save/load checkpoint
	void Serialize( CArchive& archive );

	// Get final model
	CPtr<IModel> GetClassificationModel( const IProblem& _problem );
	CPtr<IRegressionModel> GetRegressionModel( const IRegressionProblem& _problem );
	CPtr<IMultivariateRegressionModel> GetMultivariateRegressionModel( const IMultivariateRegressionProblem& _problem );

private:
	// A cache element that contains the ensemble predictions for a vector on a given step
	struct CPredictionCacheItem {
		int Step; // the number of the step on which the value was calculated
		double Value; // the calculated value

		friend inline CArchive& operator <<( CArchive& archive, CPredictionCacheItem& item )
		{
			archive << item.Step << item.Value;
			return archive;
		}

		friend inline CArchive& operator >>( CArchive& archive, CPredictionCacheItem& item )
		{
			archive >> item.Step >> item.Value;
			return archive;
		}
	};

	const CParams params; // the classification parameters
	CRandom defaultRandom; // the default random number generator
	CTextStream* logStream; // the logging stream
	CPtr<CGradientBoostFullTreeBuilder<CGradientBoostStatisticsSingle>> fullSingleClassTreeBuilder; // TGBT_Full tree builder for single class
	CPtr<CGradientBoostFullTreeBuilder<CGradientBoostStatisticsMulti>> fullMultiClassTreeBuilder; // TGBT_Full tree builder for multi class
	CPtr<CGradientBoostFastHistTreeBuilder<CGradientBoostStatisticsSingle>> fastHistSingleClassTreeBuilder; // TGBT_FastHist tree builder for single class
	CPtr<CGradientBoostFastHistTreeBuilder<CGradientBoostStatisticsMulti>> fastHistMultiClassTreeBuilder; // TGBT_MultiFastHist tree builder for multi class
	CPtr<IMultivariateRegressionProblem> baseProblem; // base problem
	CPtr<CGradientBoostFullProblem> fullProblem; // the problem data for TGBT_Full mode
	CPtr<CGradientBoostFastHistProblem> fastHistProblem; // the problem data for TGBT_FastHist mode
	CArray< CArray<CPredictionCacheItem> > predictCache; // the cache for predictions of the models being built
	// In the predicts, answers, gradients, hessians arrays the first index corresponds to the number of the class
	// if you are training a multi-class classifier; 
	// for a binary classifier, these arrays have the length of 1, and the first index is always 0
	// The second index represents the vector number in the truncated training set (see usedVectors)
	CArray< CArray<double> > predicts; // the current algorithm predictions on each step
	CArray< CArray<double> > answers; // the correct answers for the vectors used on each step
	CArray< CArray<double> > gradients; // the gradients on each step
	CArray< CArray<double> > hessians; // the hessians on each step
	double loss; // the last loss mean
	// The vectors used on each step
	// Contains the mapping of the index in the truncated training set for the given step to the index in the full set
	// The array length is N * CParams::Subsample, where N is the original training set length
	CArray<int> usedVectors;
	// The features used on each step
	// Contains the mapping of the index in the truncated feature set for the given step to the index in the full set
	// The array length is N * CParams::Subfeature, where N is the total number of features
	CArray<int> usedFeatures;
	// The inverse mapping of features
	// The array length is equal to the total number of features
	CArray<int> featureNumbers;
	// The current models ensemble (ensembles are used for multi-class classification)
	CArray<CGradientBoostEnsemble> models;
	// Loss function
	CPtr<IGradientBoostingLossFunction> lossFunction;

	void createTreeBuilder( const IMultivariateRegressionProblem* problem );
	void destroyTreeBuilder();
	CPtr<IGradientBoostingLossFunction> createLossFunction() const;
	void prepareProblem( const IProblem& _problem );
	void prepareProblem( const IRegressionProblem& _problem );
	void prepareProblem( const IMultivariateRegressionProblem& _problem );
	void initialize();
	bool trainStep();
	void executeStep( IGradientBoostingLossFunction& lossFunction,
		const IMultivariateRegressionProblem* problem, CGradientBoostEnsemble& curModels );
	void buildPredictions( const IMultivariateRegressionProblem& problem, const CArray<CGradientBoostEnsemble>& models, int curStep );
	void buildFullPredictions( const IMultivariateRegressionProblem& problem, const CArray<CGradientBoostEnsemble>& models );
	CPtr<IObject> createOutputRepresentation(
		CArray<CGradientBoostEnsemble>& models, int predictionSize );
	bool isMultiTreesModel() { return params.TreeBuilder == GBTB_MultiFull || params.TreeBuilder == GBTB_MultiFastHist; }
	template<typename T>
	CPtr<T> getModel();
};

//------------------------------------------------------------------------------------------------------------

DECLARE_NEOML_MODEL_NAME( GradientBoostModelName, "FmlGradientBoostModel" )

// Gradient boosting classification model interface
class NEOML_API IGradientBoostModel : public IModel {
public:
	~IGradientBoostModel() override;

	// Gets the tree ensemble
	virtual const CArray<CGradientBoostEnsemble>& GetEnsemble() const = 0;

	// Serializes the model
	void Serialize( CArchive& ) override = 0;

	// Gets the learning rate
	virtual double GetLearningRate() const = 0;

	// Gets the loss function
	virtual CGradientBoost::TLossFunction GetLossFunction() const = 0;

	// Gets the classification results for all tree ensembles [1..k], 
	// with k taking values from 1 to the total number of trees
	virtual bool ClassifyEx( const CSparseFloatVector& data, CArray<CClassificationResult>& results ) const = 0;
	virtual bool ClassifyEx( const CFloatVectorDesc& data, CArray<CClassificationResult>& results ) const = 0;

	// Calculates feature usage statistics
	// Returns the number of times each feature was used for node splitting
	virtual void CalcFeatureStatistics( int maxFeature, CArray<int>& result ) const = 0;

	// Reduces the number of trees in the ensemble to the given cutoff value
	virtual void CutNumberOfTrees( int numberOfTrees ) = 0;

	// Converts model to the compact representation (GBMR_Compact).
	virtual void ConvertToCompact() = 0;
};

DECLARE_NEOML_MODEL_NAME( GradientBoostRegressionModelName, "FmlGradientBoostModel" )

// Gradient boosting regression model interface
class NEOML_API IGradientBoostRegressionModel : public IRegressionModel, public IMultivariateRegressionModel {
public:
	~IGradientBoostRegressionModel() override;
	
    // Gets the tree ensemble
	virtual const CArray<CGradientBoostEnsemble>& GetEnsemble() const = 0;

	// Serializes the model
	void Serialize( CArchive& ) override = 0;

	// Gets the learning rate
	virtual double GetLearningRate() const = 0;

	// Gets the loss function
	virtual CGradientBoost::TLossFunction GetLossFunction() const = 0;

	// Calculates feature usage statistics
	// Returns the number of times each feature was used for node splitting
	virtual void CalcFeatureStatistics( int maxFeature, CArray<int>& result ) const = 0;

	// Reduces the number of trees in the ensemble to the given cutoff value
	virtual void CutNumberOfTrees( int numberOfTrees ) = 0;

	// Converts model to the compact representation (GBMR_Compact).
	virtual void ConvertToCompact() = 0;
};

//------------------------------------------------------------------------------------------------------------

// Regression tree node types
enum TRegressionTreeNodeType {
	RTNT_Undefined = 0,
	RTNT_Const, // a constant node
	RTNT_Continuous, // a node that uses a continuous feature for splitting into subtrees
	RTNT_MultiConst, // a constant node with multiple values
	RTNT_Count
};

// Regression tree node information
struct CRegressionTreeNodeInfo {
	TRegressionTreeNodeType Type; // the node type
	// The index of the feature used for splitting - only for RTNT_Continuous
	int FeatureIndex;
	// The Value[0] of the feature used for splitting - only for RTNT_Continuous
	// For RTNT_Const/RTNT_MultiConst - the result
	CFastArray<double, 1> Value;

	CRegressionTreeNodeInfo() : Type( RTNT_Undefined ), FeatureIndex( NotFound ), Value( { 0 } ) {}

	// Copies the node information to another node
	void CopyTo( CRegressionTreeNodeInfo& newInfo ) const;

	CRegressionTreeNodeInfo( const CRegressionTreeNodeInfo& other )
	{
		Type = other.Type;
		FeatureIndex = other.FeatureIndex;
		other.Value.CopyTo( Value );
	}

	CRegressionTreeNodeInfo& operator=( const CRegressionTreeNodeInfo& other )
	{
		Type = other.Type;
		FeatureIndex = other.FeatureIndex;
		other.Value.CopyTo( Value );
		return *this;
	}
};

inline void CRegressionTreeNodeInfo::CopyTo( CRegressionTreeNodeInfo& newInfo ) const
{
	newInfo.Type = Type;
	newInfo.FeatureIndex = FeatureIndex;
	Value.CopyTo( newInfo.Value );
}

inline CArchive& operator<<( CArchive& archive, const CRegressionTreeNodeInfo& info )
{
	archive.SerializeEnum( const_cast<CRegressionTreeNodeInfo&>( info ).Type );
	archive << info.FeatureIndex;
	if( info.Type == RTNT_MultiConst ) {
		const_cast< CRegressionTreeNodeInfo& >( info ).Value.Serialize( archive );
	} else {
		archive << info.Value[0];
	}
	return archive;
}

inline CArchive& operator >> ( CArchive& archive, CRegressionTreeNodeInfo& info )
{
	archive.SerializeEnum( info.Type );
	archive >> info.FeatureIndex;
	if( info.Type == RTNT_MultiConst ) {
		info.Value.Serialize( archive );
	} else {
		double value;
		archive >> value;
		info.Value = { value };
	}
	return archive;
}

// The regression tree node interface.
// Can be used for iterating through the boosting results.
class NEOML_API IRegressionTreeNode : virtual public IObject {
public:
	~IRegressionTreeNode() override;

	// Gets the child nodes
	virtual CPtr<const IRegressionTreeNode> GetLeftChild() const = 0;
	virtual CPtr<const IRegressionTreeNode> GetRightChild() const = 0;

	// Gets the node information
	virtual void GetNodeInfo( CRegressionTreeNodeInfo& info ) const = 0;
};

// For backward compatibility.
using IRegressionTreeModel = IRegressionTreeNode;

} // namespace NeoML
