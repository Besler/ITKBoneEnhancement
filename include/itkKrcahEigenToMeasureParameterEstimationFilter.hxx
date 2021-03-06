/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#ifndef itkKrcahEigenToMeasureParameterEstimationFilter_hxx
#define itkKrcahEigenToMeasureParameterEstimationFilter_hxx

#include "itkKrcahEigenToMeasureParameterEstimationFilter.h"

namespace itk {

template< typename TInputImage, typename TOutputImage >
KrcahEigenToMeasureParameterEstimationFilter< TInputImage, TOutputImage >
::KrcahEigenToMeasureParameterEstimationFilter() :
  m_ParameterSet(UseImplementationParameters)
{
  /* Set parameter size to 3 */
  ParameterArrayType parameters = this->GetParametersOutput()->Get();
  parameters.SetSize(3);
  parameters[0] = 0.5;
  parameters[1] = 0.5;
  parameters[2] = 1;
  this->GetParametersOutput()->Set(parameters);
}

template< typename TInputImage, typename TOutputImage >
void
KrcahEigenToMeasureParameterEstimationFilter< TInputImage, TOutputImage >
::BeforeThreadedGenerateData()
{
  m_ThreadAccumulatedTrace = NumericTraits< RealType >::ZeroValue();
  m_ThreadCount = NumericTraits< RealType >::ZeroValue();
}

template< typename TInputImage, typename TOutputImage >
void
KrcahEigenToMeasureParameterEstimationFilter< TInputImage, TOutputImage >
::AfterThreadedGenerateData()
{
  /* Determine default parameters */
  RealType alpha, beta, gamma;
  switch(m_ParameterSet)
  {
    case UseImplementationParameters:
      alpha = Math::sqrt2 * 0.5f;
      beta = Math::sqrt2 * 0.5f;
      gamma = Math::sqrt2 * 0.5f;
      break;
    case UseJournalParameters:
      alpha = 0.5f;
      beta = 0.5f;
      gamma = 0.25f;
      break;
    default:
      itkExceptionMacro(<< "Have bad parameterset enumeration " << m_ParameterSet);
      break;
  }

  /* Do derived measures */
  const RealType  accum(m_ThreadAccumulatedTrace);
  const RealType  count(m_ThreadCount);
  if (count > 0) {
    RealType averageTrace = accum / count;
    gamma = gamma * averageTrace;
  } else {
    gamma = NumericTraits< RealType >::ZeroValue();
  }

  /* Assign outputs parameters */
  ParameterArrayType parameters;
  parameters.SetSize(3);
  parameters[0] = alpha;
  parameters[1] = beta;
  parameters[2] = gamma;
  this->GetParametersOutput()->Set(parameters);
}

template< typename TInputImage, typename TOutputImage >
void
KrcahEigenToMeasureParameterEstimationFilter< TInputImage, TOutputImage >
::DynamicThreadedGenerateData(const OutputImageRegionType & outputRegionForThread)
{
  /* If size is zero, return */
  const SizeValueType size0 = outputRegionForThread.GetSize(0);
  if (size0 == 0)
  {
    return;
  }

  /* Determine which function to call */
  RealType (Self::*traceFunction)(InputImagePixelType);
  switch(m_ParameterSet)
  {
    case UseImplementationParameters:
      traceFunction = &Self::CalculateTraceAccordingToImplementation;
      break;
    case UseJournalParameters:
      traceFunction = &Self::CalculateTraceAccordingToJournalArticle;
      break;
    default:
      itkExceptionMacro(<< "Have bad parameterset enumeration " << m_ParameterSet);
      break;
  }

  /* Keep track of the current max */
  RealType accum = NumericTraits< RealType >::ZeroValue();
  RealType count = NumericTraits< RealType >::ZeroValue();

  /* Get input and mask pointer */
  InputImageConstPointer inputPointer = this->GetInput();
  MaskSpatialObjectTypeConstPointer maskPointer = this->GetMask();
  typename InputImageType::PointType point;

  OutputImageType      *outputPtr = this->GetOutput(0);
  
  // Define the portion of the input to walk for this thread, using
  // the CallCopyOutputRegionToInputRegion method allows for the input
  // and output images to be different dimensions
  InputImageRegionType inputRegionForThread;

  this->CallCopyOutputRegionToInputRegion(inputRegionForThread, outputRegionForThread);

  /* Setup iterator */
  ImageRegionConstIteratorWithIndex< TInputImage > inputIt(inputPointer, inputRegionForThread);
  ImageRegionIterator< OutputImageType > outputIt(outputPtr, outputRegionForThread);

  /* Iterate and count */
  inputIt.GoToBegin();
  outputIt.GoToBegin();
  while ( !inputIt.IsAtEnd() )
  {
    // Process point
    inputPointer->TransformIndexToPhysicalPoint(inputIt.GetIndex(), point);
    if ( (!maskPointer) ||  (maskPointer->IsInsideInObjectSpace(point)) )
    {
      /* Compute trace */
      count++;
      accum += (this->*traceFunction)(inputIt.Get());
    }

    // Set 
    outputIt.Set( static_cast< OutputImagePixelType >( inputIt.Get() ) );

    // Increment
    ++inputIt;
    ++outputIt;
  }

  /* Block and store */
  std::lock_guard<std::mutex> mutexHolder(m_Mutex);
  m_ThreadCount += count;
  m_ThreadAccumulatedTrace += accum;
}

template< typename TInputImage, typename TOutputImage >
typename KrcahEigenToMeasureParameterEstimationFilter< TInputImage, TOutputImage >::RealType
KrcahEigenToMeasureParameterEstimationFilter< TInputImage, TOutputImage >
::CalculateTraceAccordingToImplementation(InputImagePixelType pixel) {
  /* Sum of the absolute value of the eigenvalues */
  RealType trace = 0;
  for( unsigned int i = 0; i < pixel.Length; ++i) {
    trace += Math::abs(pixel[i]);
  }
  return trace;
}

template< typename TInputImage, typename TOutputImage >
typename KrcahEigenToMeasureParameterEstimationFilter< TInputImage, TOutputImage >::RealType
KrcahEigenToMeasureParameterEstimationFilter< TInputImage, TOutputImage >
::CalculateTraceAccordingToJournalArticle(InputImagePixelType pixel) {
  /* Sum of the eigenvalues */
  RealType trace = 0;
  for( unsigned int i = 0; i < pixel.Length; ++i) {
    trace += pixel[i];
  }
  return trace;
}

template< typename TInputImage, typename TOutputImage >
void
KrcahEigenToMeasureParameterEstimationFilter< TInputImage, TOutputImage >
::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
  os << indent << "ParameterSet: " << GetParameterSet() << std::endl;
}

} /* end namespace */

#endif /* itkKrcahEigenToMeasureParameterEstimationFilter_hxx */
