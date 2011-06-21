/*
 * Grid.h
 *
 *  Created on: Nov 25, 2010
 *      Author: gasst
 */

#ifndef GRAPH_H
#define GRAPH_H
 
#include <vector>
#include <assert.h>
#include "itkVectorImage.h"
#include "itkConstNeighborhoodIterator.h"
#include <itkVectorResampleImageFilter.h>
#include <itkResampleImageFilter.h>
#include "itkLinearInterpolateImageFunction.h"
#include "itkBSplineInterpolateImageFunction.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkVectorNearestNeighborInterpolateImageFunction.h"
#include "itkBSplineResampleImageFunction.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkSignedMaurerDistanceMapImageFilter.h"
#include <itkImageToImageFilter.h>
#include <itkBSplineDeformableTransform.h>

using namespace std;
/*
 * Isotropic Graph
 * Returns current/next position in a grid based on size and resolution
 */

template<class TImage, 
         class TUnaryRegistrationFunction, 
         class TPairwiseRegistrationFunction, 
         class TPairwiseSegmentationFunction, 
         class TPairwiseSegmentationRegistrationFunction,
         class TLabelMapper>

class GraphModel{
public:
    typedef GraphModel self;
   	typedef SmartPointer<Self>        Pointer;
	typedef SmartPointer<const Self>  ConstPointer;
    itkNewMacro(Self);

    //    typedef  itk::ImageToimageFilter<TImage,TImage> Superclass;
	typedef TImage ImageType;
	typedef typename TImage::IndexType IndexType;
	typedef typename TImage::OffsetType OffsetType;
	typedef typename TImage::PointType PointType;
	typedef typename TImage::SizeType SizeType;
	typedef typename TImage::SpacingType SpacingType;
	typedef typename TImage::Pointer ImagePointerType;
	
   
    typedef TUnaryRegistrationFunction UnaryRegistrationFunctionType;
    typedef typename UnaryRegistrationFunctionType::Pointer UnaryRegistrationFunctionPointerType;
    typedef TPairwiseRegistrationFunction PairwiseRegistrationFunctionType;
    typedef typename PairwiseRegistrationFunctionType::Pointer PairwiseRegistrationFunctionPointerType;
    typedef TPairwiseSegmentationFunction PairwiseSegmentationFunctionType;
    typedef typename PairwiseSegmentationFunctionType::Pointer PairwiseSegmentationFunctionPointerType;
    typedef TPairwiseSegmentationRegistrationFunction PairwiseSegmentationRegistrationFunctionType;
    typedef typename PairwiseSegmentationRegistrationFunctionType::Pointer PairwiseSegmentationRegistrationFunctionPointerType;
    
	typedef typename LabelMapperType::RegistrationLabelType RegistrationLabelType;
	typedef typename itk::Image<RegistrationLabelType,ImageType::ImageDimension> RegistrationLabelImageType;
	typedef typename RegistrationLabelImageType::Pointer RegistrationLabelImagePointerType;

	typedef typename LabelMapperType::SegmentationLabelType SegmentationLabelType;
	typedef typename itk::Image<SegmentationLabelType,ImageType::ImageDimension> SegmentationLabelImageType;
	typedef typename SegmentationLabelImageType::Pointer SegmentationLabelImagePointerType;
    
    typedef typename itk::BSplineTransform<float,ImageType::ImageDimension,3> BSplineTransformType;
    typedef typename BSplineTransformType::Pointer BSplineTransformPointerType;
    
protected:
    
	SizeType m_totalSize,m_imageLevelDivisors,m_gridSize;
    
    //grid spacing in unit pixels
    SpacingType m_gridSpacing;
    //grid spacing in terms of input image spacing
	SpacingType m_spacing;
    
    //spacing of the displacement labels
    SpacingType m_labelSpacing;

	PointType m_origin;
	double m_DisplacementScalingFactor;
	static const unsigned int m_dim=TImage::ImageDimension;
	int m_nNodes,m_nVertices, m_nRegistrationNodes, m_nSegmentationNodes;
	//ImageInterpolatorType m_ImageInterpolator,m_SegmentationInterpolator,m_BoneConfidenceInterploator;
	UnaryRegistrationFunctionPointerType m_unaryRegFunction;
	UnarySegmentationFunctionPointerType m_unarySegFunction;
    PairwiseSegmentationRegistrationFunctionPointerType m_pairwiseSegRegFunction;
    PairwiseSegmentationFunctionPointerType m_pairwiseSegFunction;
    PairwiseRegistrationFunctionPointerType m_pairwiseRegFunction;
  
    //PairwiseFunctionPointerType m_pairwiseFunction;
	bool verbose;
	double m_segmentationWeight, m_registrationWeight;
	bool m_haveLabelMap;
    ImagePointerType m_fixedImage=NULL;
public:

    GraphModel(){
        assert(m_dim>1);
		assert(m_dim<4);
		m_haveLabelMap=false;
		verbose=true;
    };

    void setFixedImage(ImagePointerType fixedImage){m_fixedImage=fixedImage;}
    void setGraph(int nGraphNodesPerEdge){
        assert(m_fixedImage);
		m_totalSize=m_fixedimage->GetLargestPossibleRegion().GetSize();
		std::cout<<"Full image resolution: "<<m_totalSize<<endl;
		m_nSegmentationNodes=1;
		m_nRegistrationNodes=1;
		setSpacing(nGraphNodesPerEdge);
		if (LabelMapperType::nDisplacementSamples){
			m_labelSpacing=0.4*m_gridSpacing/(LabelMapperType::nDisplacementSamples);
			if (verbose) std::cout<<"Spacing :"<<m_gridSpacing<<" "<<LabelMapperType::nDisplacementSamples<<" labelSpacing :"<<m_labelSpacing<<std::endl;
		}
		for (int d=0;d<(int)m_dim;++d){
			if (verbose) std::cout<<"total size divided by spacing :"<<1.0*m_totalSize[d]/m_spacing[d]<<std::endl;

            //origin is original origin
			m_origin[d]=m_fixedImage->GetOrigin()[d];
            //
			m_gridSize[d]=m_totalSize[d]/m_gridSpacing[d];
			if (m_spacing!=1.0)
				m_gridSize[d]++;
			m_nRegistrationNodes*=m_gridSize[d];
            m_nSegmentationNodes*=m_totalSize[d];
			if (d>0){
				m_imageLevelDivisors[d]=m_imageLevelDivisors[d-1]*m_gridSize[d-1];
			}else{
				m_imageLevelDivisors[d]=1;
			}
		}
        m_nNodes=m_nRegistrationNodes+m_nSegmentationNodes;
		if (verbose) std::cout<<"GridSize: "<<m_dim<<" ";
        
		if (m_dim>=2){
			if (verbose) std::cout<<m_gridSize[0]<<" "<<m_gridSize[1];
			m_nVertices=m_gridSize[1]*(m_gridSize[0]-1)+m_gridSize[0]*(m_gridSize[1]-1);
		}
		if (m_dim==3){
			std::cout<<" "<<m_gridSize[2];
			m_nVertices=this->m_nVertices*this->m_gridSize[2]+(this->m_gridSize[2]-1)*this->m_gridSize[1]*this->m_gridSize[0];
		}
		if (verbose) std::cout<<" nodes:"<<m_nNodes<<" vertices:"<<m_nVertices<<" labels:"<<LabelMapperType::nLabels<<std::endl;
		//		m_ImageInterpolator.SetInput(m_movingImage);
	}
	void setRegistrationWeight(double registrationWeight){
		m_registrationWeight=registrationWeight;
	}
	void setSegmentationWeight(double segmentationWeight){
		m_segmentationWeight=segmentationWeight;
	}
	virtual void setSpacing(int divisor){
        assert(m_fixedImage);
		int minSpacing=999999;
		for (int d=0;d<ImageType::ImageDimension;++d){
			if(m_fixedImage->GetLargestPossibleRegion().GetSize()[d]/(divisor-1) < minSpacing){
				minSpacing=(m_fixedImage->GetLargestPossibleRegion().GetSize()[d]/(divisor-1)-1);
			}
		}
		minSpacing=minSpacing>=1?minSpacing:1.0;
		for (int d=0;d<ImageType::ImageDimension;++d){
			int div=m_fixedImage->GetLargestPossibleRegion().GetSize()[d]/minSpacing;
			div=div>0?div:1;
			double spacing=(1.0*m_fixedImage->GetLargestPossibleRegion().GetSize()[d]/div);
			if (spacing>1.0) spacing-=1.0/(div);
			m_gridSpacing[d]=spacing;
			m_spacing[d]=spacing*m_fixedImage->GetSpacing()[d];
		}
 
	}
    
	typename ImageType::DirectionType getDirection(){return m_fixedImage->GetDirection();}
	void setLabelImage(LabelImagePointerType limg){m_fullLabelImage=limg;m_haveLabelMap=true;}
	void setGradientImage(ImagePointerType limg){m_fixedGradientImage=limg;}
    
	void setDisplacementFactor(double fac){m_DisplacementScalingFactor=fac;}
	SpacingType getDisplacementFactor(){return m_labelSpacing*m_DisplacementScalingFactor;}
	SpacingType getSpacing(){return m_spacing;}
	PointType getOrigin(){return m_origin;}

    
    double getUnaryRegistrationPotential(int nodeIndex,int labelIndex){
        IndexType graphIndex=getGraphIndex(nodeIndex);
        LabelType l=LabelMapperType::getLabel(labelIndex);
        return m_unaryRegistrationPotential->getPotential(graphIndex,l);
    }
    double getUnarySegmentationPotential(int nodeIndex,int labelIndex){
        IndexType imageIndex=getImageIndex(nodeIndex);
        //Segmentation:labelIndex==segmentationlabel
        return m_segmentationPotential->getPotential(imageIndex,labelIndex);
    };
    double getPairwiseRegistrationPotential(int nodeIndex1, int nodeIndex2, int labelIndex1, int labelIndex2){
        IndexType graphIndex1=getGraphIndex(nodeIndex1);
        LabelType l1=LabelMapperType::getLabel(labelIndex1);
        IndexType graphIndex2=getGraphIndex(nodeIndex2);
        LabelType l2=LabelMapperType::getLabel(labelIndex2);
        return m_pairwiseRegistrationPotential->getPotential(graphIndex1, graphIndex2, l1,l2);
    };
    double getPairwiseSegRegPotential(int nodeIndex1, int nodeIndex2, int labelIndex1, int segmentationLabel){
        IndexType graphIndex=getGraphIndex(nodeIndex1);
        IndexType imageIndex=getImageIndex(nodeIndex2);
        LabelType registrationLabel=LabelMapperType::getLabel(labelIndex1);
        return m_PairwiseSegmentationRegistrationPotential->getPotential(graphIndex,imageIndex,registrationLabel,segmentationLabel);
    }
    double getSegmentationWeight(int nodeIndex1, int nodeIndex2){
        IndexType imageIndex1=getImageIndex(nodeIndex1);
        IndexType imageIndex2=getImageIndex(nodeIndex2);
        return m_segmentationPotential->getWeight(imageIndex1,imageIndex2);
    }
    
	virtual IndexType gridToImageIndex(IndexType gridIndex){
		IndexType imageIndex;
		for (unsigned int d=0;d<m_dim;++d){
			int t=gridIndex[d]*m_gridSpacing[d];
            if ( t<0 ){ t=0;}
                else if (t>=m_totalSize[d]){ t=	m_totalSize[d]-1;}
			imageIndex[d]=t;
		}
		return imageIndex;
	}

	virtual IndexType imageToGridIndex(IndexType imageIndex){
		IndexType gridIndex;
		for (int d=0;d<m_dim;++d){
			gridIndex[d]=(imageIndex[d])/m_gridSpacing[d];
		}
		return gridIndex;
	}
	virtual IndexType getGridPositionAtIndex(int idx){
		IndexType position;
		for ( int d=m_dim-1;d>=0;--d){
			position[d]=idx/m_imageLevelDivisors[d];
			idx-=position[d]*m_imageLevelDivisors[d];
		}
		return position;
	}
	virtual IndexType getImagePositionAtIndex(int idx){
		return gridToImageIndex(getGridPositionAtIndex(idx));
	}

	virtual int  getIntegerIndex(IndexType gridIndex){
		int i=0;
		for (unsigned int d=0;d<m_dim;++d){
			i+=gridIndex[d]*m_imageLevelDivisors[d];
		}
		return i;
	}

	int nNodes(){return m_nNodes;}

    //	LabelType getResolution(){return m_spacing;}
	int nVertices(){return m_nVertices;}

	std::vector<int> getForwardNeighbours(int index){
		IndexType position=getGridPositionAtIndex(index);
		std::vector<int> neighbours;
		for ( int d=0;d<(int)m_dim;++d){
			OffsetType off;
			off.Fill(0);
			if ((int)position[d]<(int)m_gridSize[d]-1){
				off[d]+=1;
				neighbours.push_back(getIntegerIndex(position+off));
			}
		}
		return neighbours;
	}

	SizeType getTotalSize() const
	{
		return m_totalSize;
	}
	SizeType getGridSize() const
	{return m_gridSize;}

	void setResolution(LabelType m_resolution)
	{
		this->m_resolution = m_resolution;
	}
	ImagePointerType getFixedImage(){
		return m_fixedImage;
	}
	void checkConstraints(LabelImagePointerType labelImage, std::string filename='costs.png'){
		ImagePointerType costMap=ImageType::New();
		costMap->SetRegions(labelImage->GetLargestPossibleRegion());
		costMap->Allocate();
		int vCount=0,totalCount=0;
		for (int n=0;n<m_nNodes;++n){
			IndexType gridIdx=getGridPositionAtIndex(n);
			LabelType l1=labelImage->GetPixel(gridIdx);
			std::vector<int> nb=getForwardNeighbours(n);
			double localSum=0.0;
			for (unsigned int i=0;i<nb.size();++i){
				IndexType neighbGridIdx=getGridPositionAtIndex(nb[i]);
				LabelType l2=labelImage->GetPixel(neighbGridIdx);
				//				std::cout<<gridIdx<<" "<<neighbGridIdx<<std::endl;
				double pp=getPairwisePotential(n,i,(l1),(l2),true);
				if (pp>9999 ){
					vCount++;
				}
				totalCount++;
				localSum+=pp;
			}
			if (nb.size()) localSum/=nb.size();
			costMap->SetPixel(gridIdx,localSum);
			//			std::cout<<gridIdx<<" "<<65535*localSum<<std::endl;

		}
		ImageUtils<ImageType>::writeImage(filename,costMap);

		std::cout<<1.0*vCount/(totalCount+0.0000000001)<<" ratio of violated constraints"<<std::endl;
	}

    LabelImagePointerType scaleLabelImage(LabelImagePointerType labelImg, SpacingType scalingFactors){
        typedef typename  itk::ImageRegionIterator<LabelImageType> LabelIterator;
        LabelIterator lIt(labelImg,labelImg->GetLargestPossibleRegion());
		lIt.GoToBegin();
		for (;!lIt.IsAtEnd();++lIt){
			lIt.Set(LabelMapperType::scaleDisplacement(lIt.Get(),scalingFactors));
        }
        return labelImg;
    }
    LabelImagePointerType getFullLabelImage(LabelImagePointerType labelImg){
        typedef typename  itk::ImageRegionIterator<LabelImageType> LabelIterator;
#if 1
        const unsigned int SplineOrder = 3;

        typedef typename itk::Image<float,ImageType::ImageDimension> ParamImageType;
        typedef typename itk::ResampleImageFilter<ParamImageType,ParamImageType> ResamplerType;
        typedef typename itk::BSplineResampleImageFunction<ParamImageType,double> FunctionType;
        typedef typename itk::BSplineDecompositionImageFilter<ParamImageType,ParamImageType>			DecompositionType;
        typedef typename  itk::ImageRegionIterator<ParamImageType> Iterator;
        std::vector<typename ParamImageType::Pointer> newImages(ImageType::ImageDimension+1);
        //interpolate deformation
        for ( unsigned int k = 0; k < ImageType::ImageDimension+1; k++ )
            {
                //			std::cout<<k<<" setup"<<std::endl;
                typename ParamImageType::Pointer paramsK=ParamImageType::New();
                paramsK->SetRegions(labelImg->GetLargestPossibleRegion());
                paramsK->SetOrigin(labelImg->GetOrigin());
                paramsK->SetSpacing(labelImg->GetSpacing());
                paramsK->SetDirection(labelImg->GetDirection());
                paramsK->Allocate();
                Iterator itCoarse( paramsK, paramsK->GetLargestPossibleRegion() );
                LabelIterator itOld(labelImg,labelImg->GetLargestPossibleRegion());
                for (itCoarse.GoToBegin(),itOld.GoToBegin();!itCoarse.IsAtEnd();++itOld,++itCoarse){
                    itCoarse.Set((itOld.Get()[k]));//*(k<ImageType::ImageDimension?getDisplacementFactor()[k]:1));
                    //				std::cout<<itCoarse.Get()<<std::endl;
                }
                if (k<ImageType::ImageDimension){
                    //bspline interpolation for the displacements
                    typename ResamplerType::Pointer upsampler = ResamplerType::New();
                    typename FunctionType::Pointer function = FunctionType::New();
                    upsampler->SetInput( paramsK );
                    upsampler->SetInterpolator( function );
                    upsampler->SetSize(m_fixedImage->GetLargestPossibleRegion().GetSize() );
                    upsampler->SetOutputSpacing( m_fixedImage->GetSpacing() );
                    upsampler->SetOutputOrigin( m_fixedImage->GetOrigin());
                    upsampler->SetOutputDirection( m_fixedImage->GetDirection());
                    typename DecompositionType::Pointer decomposition = DecompositionType::New();
                    decomposition->SetSplineOrder( SplineOrder );
                    decomposition->SetInput( upsampler->GetOutput() );
                    decomposition->Update();
                    newImages[k] = decomposition->GetOutput();
                }
                else{
                    //linear interpolation for the segmentation label
                    typedef typename itk::NearestNeighborInterpolateImageFunction<ParamImageType, double> InterpolatorType;
                    //				typedef typename itk::LinearInterpolateImageFunction<ParamImageType, double> InterpolatorType;
                    typedef typename InterpolatorType::Pointer InterpolatorPointerType;
                    typedef typename itk::ResampleImageFilter< ParamImageType , ParamImageType>	ParamResampleFilterType;
                    InterpolatorPointerType interpolator=InterpolatorType::New();
                    interpolator->SetInputImage(paramsK);
                    typename ParamResampleFilterType::Pointer resampler = ParamResampleFilterType::New();
                    //resample deformation field to fixed image dimension
                    resampler->SetInput( paramsK );
                    resampler->SetInterpolator( interpolator );
                    resampler->SetOutputOrigin(m_fixedImage->GetOrigin());
                    resampler->SetOutputSpacing ( m_fixedImage->GetSpacing() );
                    resampler->SetOutputDirection ( m_fixedImage->GetDirection() );
                    resampler->SetSize ( m_fixedImage->GetLargestPossibleRegion().GetSize() );
                    resampler->Update();
                    newImages[k] = resampler->GetOutput();
                }
            }
        std::vector< Iterator*> iterators(ImageType::ImageDimension+1);
        for ( unsigned int k = 0; k < ImageType::ImageDimension+1; k++ )
            {
                iterators[k]=new Iterator(newImages[k],newImages[k]->GetLargestPossibleRegion());
                iterators[k]->GoToBegin();
            }
        LabelImagePointerType fullLabelImage=LabelImageType::New();
        fullLabelImage->SetRegions(m_fixedImage->GetLargestPossibleRegion());
        fullLabelImage->SetOrigin(m_fixedImage->GetOrigin());
        fullLabelImage->SetSpacing(m_fixedImage->GetSpacing());
        fullLabelImage->SetDirection(m_fixedImage->GetDirection());
        fullLabelImage->Allocate();
        LabelIterator lIt(fullLabelImage,fullLabelImage->GetLargestPossibleRegion());
        lIt.GoToBegin();
        for (;!lIt.IsAtEnd();++lIt){
            LabelType l;
            for ( unsigned int k = 0; k < ImageType::ImageDimension+1; k++ ){
                //				std::cout<<k<<" label: "<<iterators[k]->Get()<<std::endl;
                l[k]=iterators[k]->Get();
                ++(*(iterators[k]));
            }

            //			lIt.Set(LabelMapperType::scaleDisplacement(l,getDisplacementFactor()));
            lIt.Set(l);
        }
        for ( unsigned int k = 0; k < ImageType::ImageDimension+1; k++ )
            {
                delete iterators[k];
            }
        return fullLabelImage;

#else
#if 1
        //typedef typename itk::VectorLinearInterpolateImageFunction<LabelImageType, double> LabelInterpolatorType;
        typedef typename itk::VectorNearestNeighborInterpolateImageFunction<LabelImageType, double> LabelInterpolatorType;
        typedef typename LabelInterpolatorType::Pointer LabelInterpolatorPointerType;
        typedef typename itk::VectorResampleImageFilter< LabelImageType , LabelImageType>	LabelResampleFilterType;
        LabelInterpolatorPointerType labelInterpolator=LabelInterpolatorType::New();
        labelInterpolator->SetInputImage(labelImg);
        //initialise resampler

        typename LabelResampleFilterType::Pointer resampler = LabelResampleFilterType::New();
        //resample deformation field to fixed image dimension
        resampler->SetInput( labelImg );
        resampler->SetInterpolator( labelInterpolator );
        resampler->SetOutputOrigin(m_fixedImage->GetOrigin());
        resampler->SetOutputSpacing ( m_fixedImage->GetSpacing() );
        resampler->SetOutputDirection ( m_fixedImage->GetDirection() );
        resampler->SetSize ( m_fixedImage->GetLargestPossibleRegion().GetSize() );
        if (verbose) std::cout<<"interpolating deformation field"<<std::endl;
        resampler->Update();
        LabelImagePointerType fullLabelImage=resampler->GetOutput();
        LabelIterator lIt(fullLabelImage,fullLabelImage->GetLargestPossibleRegion());
        lIt.GoToBegin();
        for (;!lIt.IsAtEnd();++lIt){
            LabelType l=lIt.Get();
            lIt.Set(LabelMapperType::scaleDisplacement(l,getDisplacementFactor()));
        }
        return fullLabelImage;
#else
        LabelIterator lIt(labelImg,labelImg->GetLargestPossibleRegion());
        lIt.GoToBegin();
        for (;!lIt.IsAtEnd();++lIt){
            LabelType l=lIt.Get();
            lIt.Set(LabelMapperType::scaleDisplacement(l,getDisplacementFactor()));
        }
        return labelImg;
#endif
#endif
    }

    void calculateBackProjections(){
        bool zero=false;
        for (int d=0;d<ImageType::ImageDimension;++d){
            if (this->m_unaryFunction->getRadius()[d]==0){
                zero=true;
                break;
            }
        }
        if (zero){
            m_backProjFixedImage=m_fixedGradientImage;
            m_backProjLabelImage=m_fullLabelImage;
            return;
        }
        m_backProjFixedImage=ImageType::New();
        typename ImageType::RegionType imRegion;
        imRegion.SetSize(m_gridSize);
        m_backProjFixedImage->SetOrigin(m_origin);
        m_backProjFixedImage->SetRegions(imRegion);
        m_backProjFixedImage->SetSpacing(m_spacing);
        m_backProjFixedImage->Allocate();
        m_backProjLabelImage=LabelImageType::New();
        typename LabelImageType::RegionType region;
        region.SetSize(m_gridSize);
        m_backProjLabelImage->SetOrigin(m_origin);
        m_backProjLabelImage->SetRegions(region);
        m_backProjLabelImage->SetSpacing(m_spacing);
        m_backProjLabelImage->Allocate();
        bool inBounds;
        typename itk::ConstNeighborhoodIterator<LabelImageType>::RadiusType radius=this->m_unaryFunction->getRadius();
        typename itk::ConstNeighborhoodIterator<LabelImageType> nIt(radius,this->m_fullLabelImage, this->m_fullLabelImage->GetLargestPossibleRegion());
        typename itk::ConstNeighborhoodIterator<ImageType> fixedIt(radius,this->m_fixedGradientImage, this->m_fixedImage->GetLargestPossibleRegion());

        for (int i=0;i<m_nNodes;++i){

            IndexType gridIndex=getGridPositionAtIndex(i);
            IndexType imageIndex=gridToImageIndex(gridIndex);
            nIt.SetLocation(imageIndex);
            fixedIt.SetLocation(imageIndex);

            double weightSum=0.0;
            LabelType labelSum;
            float valSum=0.0;
            for (unsigned int n=0;n<nIt.Size();++n){
                LabelType l=nIt.GetPixel(n,inBounds);
                float val=fixedIt.GetPixel(n);

                //				std::cout<<val<<std::endl;
                if (inBounds){
                    IndexType neighborIndex=nIt.GetIndex(n);
                    double weight=1.0;
                    double maxD=0.0;
                    double dist=0.0;
                    for (int d=0;d<ImageType::ImageDimension;++d){
                        double tmp=1.0*fabs(neighborIndex[d]-imageIndex[d]);
                        dist+=tmp*tmp;
                        maxD+=radius[d]*radius[d];
                        ////					weight*=1-(1.0*fabs(neighborIndex[d]-fixedIndex[d]))/m_radius[d];
                        //					weight*=1-(1.0*fabs(neighborIndex[d]-fixedIndex[d]))/m_radius[d];

                    }
                    //					weight=1-(dist/maxD);
                    //					for (int d=0;d<ImageType::ImageDimension;++d){
                    //						weight*=1-(1.0*fabs(neighborIndex[d]-imageIndex[d]))/(m_spacing[d]); //uhuh radius==spacing?
                    //						//weight+=w/ImageType::ImageDimension;
                    //					}
                    //					weight=1;
                    //					std::cout<<n<<" "<<neighborIndex<<" "<<imageIndex<<" "<<weight<<" "<<val<<std::endl;
                    labelSum+=l*weight;
                    valSum+=val*weight;
                    weightSum+=weight;
                }

            }
            labelSum/=weightSum;
            m_backProjLabelImage->SetPixel(gridIndex,labelSum);
            //			std::cout<<gridIndex<<" "<<imageIndex<<" "<<valSum<<" "<<weightSum<<" "<<valSum/weightSum<<std::endl;
            valSum/=weightSum;
            m_backProjFixedImage->SetPixel(gridIndex,valSum);
        }
        typedef typename itk::RescaleIntensityImageFilter<
        ImageType, ImageType >  RescaleFilterType;
        typename RescaleFilterType::Pointer rescaler = RescaleFilterType::New();
        rescaler->SetOutputMinimum(  0 );
        rescaler->SetOutputMaximum( 65535 );
        rescaler->SetInput(m_backProjFixedImage);
        rescaler->Update();
        m_backProjFixedImage=rescaler->GetOutput();
    }
    void saveBackProj(std::string Filename){
        typedef typename itk::NearestNeighborInterpolateImageFunction<ImageType, double> InterpolatorType;
        typedef typename itk::ResampleImageFilter< ImageType , ImageType>	ResampleFilterType;
        typename InterpolatorType::Pointer interpolator=InterpolatorType::New();
        interpolator->SetInputImage(m_backProjFixedImage);
        typename ResampleFilterType::Pointer resampler = ResampleFilterType::New();
        //resample deformation field to fixed image dimension
        resampler->SetInput( m_backProjFixedImage );
        resampler->SetInterpolator( interpolator );
        resampler->SetOutputOrigin(m_fixedImage->GetOrigin());
        resampler->SetOutputSpacing ( m_fixedImage->GetSpacing() );
        resampler->SetOutputDirection ( m_fixedImage->GetDirection() );
        resampler->SetSize ( m_fixedImage->GetLargestPossibleRegion().GetSize() );
        resampler->Update();
        ImageUtils<ImageType>::writeImage(Filename,resampler->GetOutput());
    }

    ImagePointerType deformImage(ImagePointerType segmentationImage, LabelImagePointerType deformation){
        typedef typename  itk::ImageRegionIterator<LabelImageType> LabelIterator;
        typedef typename  itk::ImageRegionIterator<ImageType> ImageIterator;
        typename LabelIterator::PointerType deformationIt(deformation,deformation->GetLargestPossibleRegion());
        typename ImageIterator::PointerType imageIt(segmentationImage,segmentationImage->GetLargestPossibleRegion());
        typedef typename itk::LinearInterpolateImageFunction<ImageType, double> ImageInterpolatorType;
        typename ImageInterpolatorType::Pointer interpolator=ImageInterpolatorType::New();
        interpolator->SetInput(segmentationImage);
        ImagePointerType deformed=ImageUtils<ImageType>::createEmpty(segmentationImage);
        for (imageIt.GoToBegin(),deformationIt.GoToBegin();!imageIt.IsAtEnd();++imageIt,++deformationIt){
            IndexType index=deformationIt.GetIndex();
            typename ImageInterpolatorType::ContinuousIndexType idx=this->m_unaryPotential->getMovingIndex(index);
            LabelType displacement=deformationIt.Get();
            idx+=LabelMapperType::getDisplacement(displacement);
            if (interpolator->IsInsideBuffer(idx)){
                deformed->SetPixel(imageIt.GetIndex(),interpolator->EvaluateAtContinuousIndex(idx));

            }else{
                deformed->SetPixel(imageIt.GetIndex(),0);
            }
        }
        return deformed;
    }
        
    void setUpDistanceTransformImage(ImagePointerType segmentationImage){
        typedef typename itk::SignedMaurerDistanceMapImageFilter< ImageType, ImageType > DistanceTransformType;
        typename DistanceTransformType::Pointer distanceTransform=DistanceTransformType::New();
        distanceTransform->SetInput(segmentationImage);
        distanceTransform->Update();
        //ImageUtils<ImageType>::writeImage("dt.nii",distanceTransform->GetOutput());
        m_distanceToDeformedSegmentationMap=distanceTransform->GetOutput();
    

    }
};



#endif /* GRIm_dim_H_ */
