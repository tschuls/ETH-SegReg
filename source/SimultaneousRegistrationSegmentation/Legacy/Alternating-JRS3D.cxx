#include "Log.h"

#include <stdio.h>
#include <iostream>

#include "ArgumentParser.h"

#include "SRSConfig.h"
#include "HierarchicalJRSImageToImageFilter.h"
#include "Graph.h"
#include "BaseLabel.h"
#include "Potential-Registration-Unary.h"
#include "Potential-Registration-Pairwise.h"
#include "Potential-Segmentation-Unary.h"
#include "Potential-Segmentation-Pairwise.h"
#include "Potential-SegmentationRegistration-Pairwise.h"
#include "MRF-FAST-PD.h"

using namespace std;
using namespace itk;

int main(int argc, char ** argv)
{

	feenableexcept(FE_INVALID|FE_DIVBYZERO|FE_OVERFLOW);

	SRSConfig filterConfig;
	filterConfig.parseParams(argc,argv);
	//define types.
	typedef unsigned char PixelType;
	const unsigned int D=3;
	typedef Image<PixelType,D> ImageType;
	typedef itk::Vector<float,D> BaseLabelType;
    typedef SparseRegistrationLabelMapper<ImageType,BaseLabelType> LabelMapperType;
    typedef UnaryPotentialSegmentationMarcelWithPrior< ImageType > SegmentationUnaryPotentialType;
    typedef UnaryPotentialRegistrationNCCWithSegmentationPrior< LabelMapperType, ImageType > RegistrationUnaryPotentialType;
    typedef PairwisePotentialRegistration< LabelMapperType, ImageType > RegistrationPairwisePotentialType;
    typedef PairwisePotentialSegmentationMarcel<ImageType> SegmentationPairwisePotentialType;
  	typedef HierarchicalJRSImageToImageFilter<ImageType,
                                              LabelMapperType,
                                              RegistrationUnaryPotentialType,
                                              SegmentationUnaryPotentialType,
                                              SegmentationPairwisePotentialType,
                                              RegistrationPairwisePotentialType
                                              >        FilterType;
    
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
	LOG<<"Finished computation after "<<t<<" seconds"<<std::endl;
	return 1;
}
