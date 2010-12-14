
#include <stdio.h>
#include <iostream>

#include "argstream.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "ImageUtils.h"
#include "itkImage.h"
#include "Potentials.h"
#include "MRF.h"
#include "Grid.h"
#include "Label.h"
#include "RegistrationSegmentationLabel.h"
#include "FAST-PD-Registration-mrf.h"
#include <fenv.h>
#include "TRW-S-Registration.h"

using namespace std;
using namespace itk;

#define _MANY_LABELS_

int main(int argc, char ** argv)
{
	feenableexcept(FE_INVALID|FE_DIVBYZERO|FE_OVERFLOW);


	argstream as(argc, argv);
	string targetFilename,movingFilename,outputFilename,deformableFilename,defFilename="";
	double pairwiseWeight=1;
	int displacementSampling=-1;
	double unaryWeight=1;
	int maxDisplacement=10;

	as >> parameter ("t", targetFilename, "target image (file name)", true);
	as >> parameter ("m", movingFilename, "moving image (file name)", true);
	as >> parameter ("o", outputFilename, "output image (file name)", true);
	as >> parameter ("d", deformableFilename, "deformable image (file name)", false);
	as >> parameter ("f", defFilename,"deformation field filename", false);
	as >> parameter ("p", pairwiseWeight,"weight for pairwise potentials", false);
	as >> parameter ("u", unaryWeight,"weight for unary potentials", false);
	as >> parameter ("max", maxDisplacement,"maximum displacement in pixels per axis", false);
	as >> parameter ("n", displacementSampling,"number of samples for each displacement axis", false);
	as >> help();
	as.defaultErrorHandling();

	if (displacementSampling==-1) displacementSampling=maxDisplacement;

	//typedefs
	typedef unsigned short PixelType;
	const unsigned int D=2;
	typedef Image<PixelType,D> ImageType;
	typedef ImageType::IndexType IndexType;
	typedef ImageType::Pointer ImagePointerType;
	//	typedef Image<LabelType> LabelImageType;
	//read input images
	ImageType::Pointer targetImage =
			ImageUtils<ImageType>::readImage(targetFilename);
	ImageType::Pointer movingImage =
			ImageUtils<ImageType>::readImage(movingFilename);

	//typedef int LabelType;
	typedef Offset<D> LabelType;

	//create Grid
	typedef Grid<ImageType> GridType;
	GridType::LabelType resolution;
	for (int d=0;d<D;++d){
		resolution[d]=1;
	}

	GridType fullimageGrid(targetImage,resolution);
	typedef RegistrationSegmentationLabel<ImageType> RegistrationSegmentationLabelType;
	typedef RegistrationSegmentationLabelConverter<ImageType, RegistrationSegmentationLabelType> RLCType;
	RLCType * RLC=new RLCType(targetImage,movingImage,2*maxDisplacement+1,2*maxDisplacement+1);


	//	PairwisePotential
	typedef EuclideanPairwisePotential<RLCType> PairwisePotentialType;
	PairwisePotentialType::Pointer potentialFunction=PairwisePotentialType::New();


	typedef UnaryPotential<RLCType> UnaryPotentialType;
	UnaryPotentialType::Pointer unaryFunction=UnaryPotentialType::New();
	unaryFunction->SetMovingImage(movingImage);
	unaryFunction->SetFixedImage(targetImage);
	unaryFunction->setLabelConverter(RLC);


	//	ok what now: create graph! solve graph! save result!Z

	typedef FastPDMRFSolver<UnaryPotentialType,PairwisePotentialType> MRFSolverType;
//		typedef TRWS_MRFSolver<UnaryPotentialType,PairwisePotentialType> MRFSolverType;
	MRFSolverType mrfSolver(targetImage,movingImage,&fullimageGrid,potentialFunction,unaryFunction,unaryWeight,pairwiseWeight);
	std::cout<<"run"<<std::endl;
	mrfSolver.optimize();

	ImagePointerType transformedImage;
	if (deformableFilename!=""){
		ImageType::Pointer deformableImage =
				ImageUtils<ImageType>::readImage(deformableFilename);
		transformedImage=mrfSolver.transformImage(deformableImage);
	}else{
		transformedImage=mrfSolver.transformImage(movingImage);
	}

	ImageUtils<ImageType>::writeImage(outputFilename, transformedImage);

	if (defFilename!=""){
		typedef MRFSolverType::LabelFieldType LabelFieldType;
		typedef LabelFieldType::Pointer LabelFieldPointerType;

		LabelFieldPointerType LabelField=mrfSolver.getLabelField();
		ImageUtils<LabelFieldType>::writeImage(defFilename,LabelField);

	}
	std::cout<<"wtf"<<std::endl;
	return 1;
}
