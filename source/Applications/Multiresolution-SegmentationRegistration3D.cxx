
#include <stdio.h>
#include <iostream>

#include "argstream.h"

#include "SRSConfig.h"
#include "HierarchicalSRSImageToImageFilter.h"
#include "Graph.h"
#include "BaseLabel.h"
#include "Potential-Registration-Unary.h"
#include "Potential-Registration-Pairwise.h"
#include "Potential-Segmentation-Unary.h"
#include "Potential-SegmentationRegistration-Pairwise.h"
#include "Potential-Segmentation-Pairwise.h"
#include "MRF-TRW-S.h"

using namespace std;
using namespace itk;

int main(int argc, char ** argv)
{

	feenableexcept(FE_INVALID|FE_DIVBYZERO|FE_OVERFLOW);

	SRSConfig filterConfig;
	filterConfig.parseParams(argc,argv);
	//define types.
	typedef float PixelType;
	const unsigned int D=3;
	typedef Image<PixelType,D> ImageType;
	typedef itk::Vector<float,D> BaseLabelType;
    //typedef SparseRegistrationLabelMapper<ImageType,BaseLabelType> LabelMapperType;
    typedef DenseRegistrationLabelMapper<ImageType,BaseLabelType> LabelMapperType;

    //unary seg
    typedef HandcraftedBoneSegmentationClassifierGradient<ImageType> ClassifierType;
    //typedef SegmentationClassifierGradient<ImageType> ClassifierType;
    //typedef SegmentationClassifier<ImageType> ClassifierType;
    typedef UnaryPotentialSegmentationClassifier< ImageType, ClassifierType > SegmentationUnaryPotentialType;
    
    //pairwise seg
//    typedef UnaryPotentialSegmentation< ImageType > SegmentationUnaryPotentialType;
    //typedef SmoothnessClassifierGradient<ImageType> SegmentationSmoothnessClassifierType;
    typedef SmoothnessClassifierGradientContrast<ImageType> SegmentationSmoothnessClassifierType;
    typedef PairwisePotentialSegmentationClassifier<ImageType,SegmentationSmoothnessClassifierType> SegmentationPairwisePotentialType;

    //reg
    typedef UnaryPotentialRegistrationNCC< LabelMapperType, ImageType > RegistrationUnaryPotentialType;
    typedef PairwisePotentialRegistration< LabelMapperType, ImageType > RegistrationPairwisePotentialType;
    typedef PairwisePotentialSegmentationRegistration< ImageType > SegmentationRegistrationPairwisePotentialType;
  typedef GraphModel<
        ImageType,
        RegistrationUnaryPotentialType,
        RegistrationPairwisePotentialType,
        SegmentationUnaryPotentialType,
        SegmentationPairwisePotentialType,
        SegmentationRegistrationPairwisePotentialType,
        LabelMapperType>        GraphType;
    
	typedef HierarchicalSRSImageToImageFilter<GraphType>        FilterType;    
	//create filter
    FilterType::Pointer filter=FilterType::New();
    filter->setConfig(filterConfig);
    filter->setFixedImage(ImageUtils<ImageType>::readImage(filterConfig.targetFilename));
    filter->setMovingImage(ImageUtils<ImageType>::readImage(filterConfig.movingFilename));
    filter->setMovingSegmentation(ImageUtils<ImageType>::readImage(filterConfig.movingSegmentationFilename));
    filter->setFixedGradientImage(ImageUtils<ImageType>::readImage(filterConfig.fixedGradientFilename));

	clock_t start = clock();
	//DO IT!
	filter->Update();
	clock_t end = clock();
	float t = (float) ((double)(end - start) / CLOCKS_PER_SEC);
	std::cout<<"Finished computation after "<<t<<" seconds"<<std::endl;
	std::cout<<"Interpolation: "<<tInterpolation<<" Optimization: "<<tOpt<<std::endl;
	return 1;
}
