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
#ifndef itkPyBuffer_hxx
#define itkPyBuffer_hxx

#include "itkPyBuffer.h"

namespace itk
{

template<class TImage>
PyObject *
PyBuffer<TImage>
::_GetArrayFromImage( ImageType * image)
{
  PyObject *                  memoryView    = NULL;
  Py_buffer                   pyBuffer;
  memset(&pyBuffer, 0, sizeof(Py_buffer));

  Py_ssize_t                  len           = 1;
  size_t                      pixelSize     = sizeof(ComponentType);
  int                         res           = 0;

  if( !image )
    {
    throw std::runtime_error("Input image is null");
    }

  image->Update();

  ComponentType *buffer =  const_cast < ComponentType *> (reinterpret_cast< const ComponentType* > ( image->GetBufferPointer() ) );

  void * itkImageBuffer = (void *)( buffer );

  // Computing the length of data
  const int numberOfComponents = image->GetNumberOfComponentsPerPixel();
  SizeType size = image->GetBufferedRegion().GetSize();

  for( unsigned int dim = 0; dim < ImageDimension; ++dim )
    {
    std::cout << "size["<<dim <<"]::" << size[dim] << std::endl;
    len *= size[dim];
    }

  len *= numberOfComponents;
  len *= pixelSize;

  res = PyBuffer_FillInfo(&pyBuffer, NULL, (void*)itkImageBuffer, len, 0, PyBUF_CONTIG);
  memoryView = PyMemoryView_FromBuffer(&pyBuffer);

  PyBuffer_Release(&pyBuffer);

  return memoryView;
}

template<class TImage>
const typename PyBuffer<TImage>::OutputImagePointer
PyBuffer<TImage>
::_GetImageFromArray( PyObject *arr, PyObject *shape, PyObject *numOfComponent)
{
  PyObject *                  obj           = NULL;
  PyObject *                  shapeseq      = NULL;
  PyObject *                  item          = NULL;

  Py_ssize_t                  buffer_len;
  Py_buffer                   pyBuffer;
  memset(&pyBuffer, 0, sizeof(Py_buffer));

  SizeType size;
  SizeValueType numberOfPixels = 1;

  const void *                buffer;

  int                         NumOfComponent= 1;
  unsigned int                dimension     = 0;


  size_t                      pixelSize     = sizeof(ComponentType);
  size_t                      len           = 1;

  if(PyObject_GetBuffer(arr, &pyBuffer, PyBUF_CONTIG) == -1)
    {
    PyErr_SetString( PyExc_RuntimeError, "Cannot get an instance of NumPy array." );
    PyErr_Clear();
    return NULL;
    }
  else
    {
    buffer_len = pyBuffer.len;
    buffer     = pyBuffer.buf;
    }

  obj        = shape;
  shapeseq   = PySequence_Fast(obj, "expected sequence");
  dimension  = PySequence_Size(obj);

  NumOfComponent=(int)PyInt_AsLong(numOfComponent);

  for(unsigned int i=0 ; i< dimension; ++i)
    {
    item = PySequence_Fast_GET_ITEM(shapeseq,i);
    size[i] = (SizeValueType)PyInt_AsLong(item);
    numberOfPixels *= size[i];
    }

  len = numberOfPixels*NumOfComponent*pixelSize;
  if ( buffer_len != len )
    {
    PyErr_SetString( PyExc_RuntimeError, "Size mismatch of image and Buffer." );
    }

  IndexType start;
  start.Fill( 0 );

  RegionType region;
  region.SetIndex( start );
  region.SetSize( size );

  PointType origin;
  origin.Fill( 0.0 );

  SpacingType spacing;
  spacing.Fill( 1.0 );

  typedef ImportImageFilter< ComponentType, ImageDimension > ImporterType;
  typename ImporterType::Pointer importer = ImporterType::New();
  importer->SetRegion( region );
  importer->SetOrigin( origin );
  importer->SetSpacing( spacing );
  const bool importImageFilterWillOwnTheBuffer = false;

  ComponentType * data = (ComponentType *)buffer;

  importer->SetImportPointer( data,
                              numberOfPixels,
                              importImageFilterWillOwnTheBuffer );

  importer->Update();
  OutputImagePointer output = importer->GetOutput();
  output->DisconnectPipeline();

  return output;
}

} // namespace itk

#endif
