/*=========================================================================
 *
 *  Copyright NumFOCUS
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

#include "itkDescoteauxEigenToScalarFunctorImageFilter.h"
#include "itkUnaryFunctorImageFilter.h"
#include "itkTestingMacros.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkMath.h"

int
itkDescoteauxEigenToScalarFunctorImageTest(int argc, char * argv[])
{
  /* type alias, instantiate filter */
  constexpr unsigned int Dimension = 3;
  using ImagePixelType = double;
  using ImageType = itk::Image<ImagePixelType, Dimension>;

  using EigenValueType = double;
  using EigenValueArrayType = itk::FixedArray<EigenValueType, Dimension>;
  using EigenValueImageType = itk::Image<EigenValueArrayType, Dimension>;

  using FilterType = itk::DescoteauxEigenToScalarFunctorImageFilter<EigenValueImageType, ImageType>;
  FilterType::Pointer descoFilter = FilterType::New();

  /* Basic tests. Need to set parameters first. */
  descoFilter->SetAlpha(0.5);
  descoFilter->SetBeta(0.5);
  descoFilter->SetC(0.25);
  ITK_EXERCISE_BASIC_OBJECT_METHODS(descoFilter, DescoteauxEigenToScalarFunctorImageFilter, UnaryFunctorImageFilter);

  /* Exercise basic set/get methods */
  descoFilter->SetAlpha(0.5);
  ITK_TEST_SET_GET_VALUE(0.5, descoFilter->GetAlpha());
  descoFilter->SetBeta(0.5);
  ITK_TEST_SET_GET_VALUE(0.5, descoFilter->GetBeta());
  descoFilter->SetC(0.25);
  ITK_TEST_SET_GET_VALUE(0.25, descoFilter->GetC());
  // Default should be 1
  ITK_TEST_SET_GET_VALUE(-1.0, descoFilter->GetEnhanceType());
  descoFilter->SetEnhanceDarkObjects();
  ITK_TEST_SET_GET_VALUE(1.0, descoFilter->GetEnhanceType());
  descoFilter->SetEnhanceBrightObjects();
  ITK_TEST_SET_GET_VALUE(-1.0, descoFilter->GetEnhanceType());

  /* Create some test data which is computable */
  EigenValueArrayType simpleEigenPixel;
  for (unsigned int i = 0; i < Dimension; ++i)
  {
    simpleEigenPixel.SetElement(i, 0);
  }

  EigenValueImageType::RegionType region;
  EigenValueImageType::IndexType  start;
  start[0] = 0;
  start[1] = 0;
  start[2] = 0;

  EigenValueImageType::SizeType size;
  size[0] = 10;
  size[1] = 10;
  size[2] = 10;

  region.SetSize(size);
  region.SetIndex(start);

  EigenValueImageType::Pointer image = EigenValueImageType::New();
  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(simpleEigenPixel);

  descoFilter->SetInput(image);
  ITK_TRY_EXPECT_NO_EXCEPTION(descoFilter->Update());

  itk::ImageRegionIteratorWithIndex<ImageType> input(descoFilter->GetOutput(), region);

  input.GoToBegin();
  while (!input.IsAtEnd())
  {
    ITK_TEST_EXPECT_TRUE(itk::Math::FloatAlmostEqual(input.Get(), 0.0, 6, 0.000001));
    ++input;
  }

  /* Create some test data which is computable */
  simpleEigenPixel.SetElement(0, 0.25);
  simpleEigenPixel.SetElement(1, 1);
  simpleEigenPixel.SetElement(2, -1);

  EigenValueImageType::Pointer image2 = EigenValueImageType::New();
  image2->SetRegions(region);
  image2->Allocate();
  image2->FillBuffer(simpleEigenPixel);

  descoFilter->SetInput(image2);
  ITK_TRY_EXPECT_NO_EXCEPTION(descoFilter->Update());

  itk::ImageRegionIteratorWithIndex<ImageType> input2(descoFilter->GetOutput(), region);

  input2.GoToBegin();
  while (!input2.IsAtEnd())
  {
    ITK_TEST_EXPECT_TRUE(itk::Math::FloatAlmostEqual(input2.Get(), 0.0913983433747, 6, 0.000001));
    ++input2;
  }

  return EXIT_SUCCESS;
}
