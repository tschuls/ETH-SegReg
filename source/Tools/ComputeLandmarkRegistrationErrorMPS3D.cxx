#include "Log.h"

#include <stdio.h>
#include <iostream>
#include "ArgumentParser.h"
#include "ImageUtils.h"
#include "TransformationUtils.h"
#include <itkWarpImageFilter.h>
#include <fstream>

#include <itkInverseDisplacementFieldImageFilter.h>
#include "itkVectorLinearInterpolateImageFunction.h"
#include "itkVTKPolyDataReader.h"
 
#include "itkMesh.h"
#include "itkPointSet.h"

using namespace std;
using namespace itk;



int main(int argc, char ** argv)
{
    

	feraiseexcept(FE_INVALID|FE_DIVBYZERO|FE_OVERFLOW);
    typedef unsigned short PixelType;
    const unsigned int D=3;
    typedef Image<PixelType,D> ImageType;
    typedef  ImageType::IndexType IndexType;
    typedef  ImageType::PointType PointType;
    typedef  ImageType::DirectionType DirectionType;

    typedef ImageType::Pointer ImagePointerType;
    typedef ImageType::ConstPointer ImageConstPointerType;
    typedef Vector<float,D> LabelType;
    typedef Image<LabelType,D> LabelImageType;
    typedef LabelImageType::Pointer LabelImagePointerType;
    typedef ImageType::IndexType IndexType;
    ArgumentParser * as=new ArgumentParser(argc,argv);
    string refLandmarks,targetLandmarks,target="",def,output;
    bool NN=false;
    as->parameter ("refLandmarks", refLandmarks, " filename...", true);
    as->parameter ("targetLandmarks", targetLandmarks, " filename...", true);
    as->parameter ("def", def, " filename of deformation", true);
    as->parameter ("target", target, " filename of target image", true);
    as->parse();
    
    
    LabelImagePointerType deformation = ImageUtils<LabelImageType>::readImage(def);
    ImageConstPointerType referenceImage =(ImageConstPointerType) ImageUtils<ImageType>::readImage(target);
    deformation=TransfUtils<ImageType>::bSplineInterpolateDeformationField(deformation,referenceImage);

    DirectionType targetDir=deformation->GetDirection();
    typedef itk::VectorLinearInterpolateImageFunction<LabelImageType> DefInterpolatorType;
    DefInterpolatorType::Pointer defInterpol=DefInterpolatorType::New();
    defInterpol->SetInputImage(deformation);
    
    typedef DefInterpolatorType::ContinuousIndexType CIndexType;

    PointType p;
    p.Fill(0.0);
    deformation->SetOrigin(0.0);
    DirectionType refDir=referenceImage->GetDirection();

    //ImagePointerType targetLandmarkImage=ImageUtils<ImageType>::createEmpty(referenceImage);
    //ImagePointerType deformedReferenceLandmarkImage=ImageUtils<ImageType>::createEmpty(referenceImage);
    vector<PointType> landmarksReference, landmarksTarget;
    ifstream ifs(refLandmarks.c_str());
    int i=0;
    
    while ( not ifs.eof() ) {
        PointType point;
        for (int d=0;d<D;++d){
            ifs>>point[d];
            point[d]=point[d]*refDir[d][d];
        }
        LOG<<point<<endl;
        landmarksReference.push_back(point);
       
    } 
    std::cout<<"read "<<landmarksReference.size()<<" landmarks"<<std::endl;
    double sumSquareError=0.0;
    ifstream ifs2(targetLandmarks.c_str());
    i=0;
    for (;i<landmarksReference.size()-1;++i){
        PointType pointTarget;
        for (int d=0;d<D;++d){
            ifs2>>pointTarget[d];
            pointTarget[d]=pointTarget[d]*targetDir[d][d];
        }        
        IndexType indexTarget,indexReference;
        deformation->TransformPhysicalPointToIndex(pointTarget,indexTarget);
        LOG<<VAR(deformation->GetOrigin())<<endl;
        LOG<<VAR(pointTarget)<<" "<<VAR(indexTarget)<<endl;
        PointType deformedReferencePoint;
        referenceImage->TransformPhysicalPointToIndex(landmarksReference[i],indexReference);
        
        //std::cout<<VAR(targetPoint)<<endl;
        //deformedReferencePoint= pointTarget+deformation->GetPixel(indexTarget);
        CIndexType cindex;
        deformation->TransformPhysicalPointToContinuousIndex(pointTarget,cindex);
        LOG<<VAR(landmarksReference[i])<<" "<<VAR(indexReference)<<" "<<VAR(cindex)<<endl;

        deformedReferencePoint= pointTarget+defInterpol->EvaluateAtContinuousIndex(cindex);

        LOG<< VAR(pointTarget) << endl;
        double localSquaredError=(deformedReferencePoint - landmarksReference[i]).GetNorm();
        for (int d=0;d<D;++d){
            deformedReferencePoint[d]=deformedReferencePoint[d]*targetDir[d][d];
        }    
        LOG<< VAR(deformedReferencePoint) << endl;

        std::cout<<"pt"<<i<<": "<<(localSquaredError)<<endl;;
        sumSquareError+=localSquaredError;
    }
    std::cout<<std::endl<<"totalAverage: "<<(sumSquareError)/(i)<<std::endl;
   
    if (argc>6){
        // ImageUtils<ImageType>::writeImage(argv[5],  (ImageConstPointerType) deformedReferenceLandmarkImage );
        //ImageUtils<ImageType>::writeImage(argv[6],  (ImageConstPointerType) targetLandmarkImage );
    }

	return 1;
}
