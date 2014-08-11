#ifdef CUDA 

/*********************************************************************/
/* File:   cuda_linalg.cpp                                           */
/* Author: Joachim Schoeberl, Matthias Hochsteger                    */
/* Date:   11. Aug. 2014                                             */
/*********************************************************************/


#include <la.hpp>
#include <cublas_v2.h>
#include <cusparse.h>

namespace ngla
{
  cublasHandle_t handle;
  cusparseHandle_t cusparseHandle;



  UnifiedVector :: UnifiedVector (int asize)
  {
    size = asize;
    host_data = new double[asize];
    cudaMalloc(&dev_data, size*sizeof(double));
    host_uptodate = false;
    dev_uptodate = false;
  }

  BaseVector & UnifiedVector :: operator= (double d)
  {
    for (int i = 0; i < size; i++) host_data[i] = d;
    host_uptodate = true;
    dev_uptodate = false;
    UpdateDevice();
    return *this;

    /*
    if (dev_uptodate || !host_uptodate)
      {
        // dev_dat = d  // howto ????
        dev_uptodate = true;
        host_uptodate = false;
      }
    else
      {
        for (int i = 0; i < size; i++)
          host_data[i] = d;

        dev_uptodate = false;
        host_uptodate = true;
      }
    */
  }

  BaseVector & UnifiedVector :: operator= (BaseVector & v2)
  {
    UnifiedVector * uv2 = dynamic_cast<UnifiedVector*> (&v2);
    if (uv2)
      {
        if (uv2->dev_uptodate)
          {
            cudaMemcpy (dev_data, uv2->dev_data, sizeof(double)*size, cudaMemcpyDeviceToDevice);    
            dev_uptodate = true;
            host_uptodate = false;
          }
        else if (uv2->host_uptodate)
          {
            VFlatVector<double> fv(size, host_data);
            fv = 1.0*v2;
            host_uptodate = true;
            dev_uptodate = false;
          }
        else
          {
            cerr << "operator= (BaseVector) : undefined vector" << endl;
          }
        return *this;
      }

    VFlatVector<double> fv(size, host_data);
    fv = 1.0*v2;

    host_uptodate = true;
    dev_uptodate = false;
    return *this;
  }



  
  BaseVector & UnifiedVector :: Scale (double scal)
  {
    UpdateDevice();
    cublasDscal (handle, size, &scal, dev_data, 1);
    host_uptodate = false;
  }

  BaseVector & UnifiedVector :: SetScalar (double scal)
  {
    (*this) = scal;
  }
  
  BaseVector & UnifiedVector :: Set (double scal, const BaseVector & v)
  {
    (*this) = 0.0;
    Add (scal, v);
  }
  
  
  BaseVector & UnifiedVector :: Add (double scal, const BaseVector & v)
  {
    const UnifiedVector * v2 = dynamic_cast<const UnifiedVector*> (&v);
    if (v2)
      {
	UpdateDevice();
	v2->UpdateDevice();
	
	cublasDaxpy (handle, size, &scal, v2->dev_data, 1, dev_data, 1);
	host_uptodate = false;
      }
    else
      {
	VFlatVector<> (size, host_data) = scal * v;
	host_uptodate = true;
	dev_uptodate = false;
      }
  }
  
  ostream & UnifiedVector :: Print (ostream & ost) const
  {
    cout << "output unified vector, host = " << host_uptodate << ", dev = " << dev_uptodate << endl;
    ost << FVDouble();
    return ost;
  }
  
  void UnifiedVector :: UpdateHost () const
  {
    if (host_uptodate) return;
    if (!dev_uptodate) cout << "device not uptodate" << endl;
    cudaMemcpy (host_data, dev_data, sizeof(double)*size, cudaMemcpyDeviceToHost);    
    host_uptodate = true;
  }

  void UnifiedVector :: UpdateDevice () const
  {
    if (dev_uptodate) return;
    if (!host_uptodate) cout << "host not uptodate" << endl;
    cudaMemcpy (dev_data, host_data, sizeof(double)*size, cudaMemcpyHostToDevice);
    dev_uptodate = true;
  }

  
  FlatVector<double> UnifiedVector :: FVDouble () const
  {
    UpdateHost();
    return FlatVector<> (size, host_data);
  }
  
  FlatVector<Complex> UnifiedVector :: FVComplex () const
  {
    throw Exception ("unified complex not yet supported");
  }
    
  void * UnifiedVector :: Memory() const throw()
  { 
    UpdateHost(); 
    return host_data;
  }

  BaseVector * UnifiedVector :: CreateVector () const
  {
    return new UnifiedVector (size);
  }

  
  void UnifiedVector :: GetIndirect (const FlatArray<int> & ind, 
			      const FlatVector<double> & v) const
  {
    cout << "UnifiedVector :: GetIndirect not supported" << endl;
  }
  void UnifiedVector :: GetIndirect (const FlatArray<int> & ind, 
			      const FlatVector<Complex> & v) const
  {
    cout << "UnifiedVector :: GetIndirect not supported" << endl;
  }








  DevSparseMatrix :: DevSparseMatrix (const SparseMatrix<double> & mat)
  {
    height = mat.Height();
    width = mat.Width();
    nze = mat.NZE();

    descr = new cusparseMatDescr_t;
    cusparseCreateMatDescr (descr);

    cusparseSetMatType(*descr, CUSPARSE_MATRIX_TYPE_GENERAL);
    cusparseSetMatIndexBase(*descr, CUSPARSE_INDEX_BASE_ZERO);

    cout << "create device sparse matrix, n = " << height << ", nze = " << nze << endl;
    
    Array<int> temp_ind (mat.Height()+1); 
    for (int i = 0; i <= mat.Height(); i++) temp_ind[i] = mat.First(i); // conversion to 32-bit integer

    cudaMalloc ((void**)&dev_ind, (mat.Height()+1) * sizeof(int));
    cudaMalloc ((void**)&dev_col, (mat.NZE()) * sizeof(int));
    cudaMalloc ((void**)&dev_val, (mat.NZE()) * sizeof(double));

    cudaMemcpy (dev_ind, &temp_ind[0], (mat.Height()+1)*sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy (dev_col, &mat.GetRowIndices(0)[0], mat.NZE()*sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy (dev_val, &mat.GetRowValues(0)[0], mat.NZE()*sizeof(double), cudaMemcpyHostToDevice);
    

  }
  
  void DevSparseMatrix :: Mult (const BaseVector & x, BaseVector & y) const
  {
    cout << "device mult" << endl;
    const UnifiedVector & ux = dynamic_cast<const UnifiedVector&> (x);
    UnifiedVector & uy = dynamic_cast<UnifiedVector&> (y);

    ux.UpdateDevice();
    uy = 0.0;
    uy.UpdateDevice();

    double alpha= 1;
    double beta = 0;
    cusparseDcsrmv (cusparseHandle, CUSPARSE_OPERATION_NON_TRANSPOSE, height, width, nze, 
		    &alpha, *descr, 
		    dev_val, dev_ind, dev_col, 
		    ux.dev_data, &beta, uy.dev_data);

    uy.host_uptodate = false;
  }


  void DevSparseMatrix :: MultAdd (double s, const BaseVector & x, BaseVector & y) const
  {
    cout << "device multadd" << endl;
    const UnifiedVector & ux = dynamic_cast<const UnifiedVector&> (x);
    UnifiedVector & uy = dynamic_cast<UnifiedVector&> (y);

    ux.UpdateDevice();
    uy.UpdateDevice();

    double alpha= 1;
    double beta = 1;
    cusparseDcsrmv (cusparseHandle, CUSPARSE_OPERATION_NON_TRANSPOSE, height, width, nze, 
		    &alpha, *descr, 
		    dev_val, dev_ind, dev_col, 
		    ux.dev_data, &beta, uy.dev_data);

    uy.host_uptodate = false;
  }
  









  class InitCuBlasHandle
  {
  public:
    InitCuBlasHandle ()
    {
      cout << "create cublas handle" << endl;
      cublasCreate (&handle);
      cout << "creaet cusparse handle" << endl;
      cusparseCreate(&cusparseHandle);
    }
  };

  InitCuBlasHandle init;


}




#endif