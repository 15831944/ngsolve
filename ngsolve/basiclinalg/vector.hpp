#ifndef FILE_VECTOR_EXPR
#define FILE_VECTOR_EXPR

/**************************************************************************/
/* File:   vector.hpp                                                     */
/* Author: Joachim Schoeberl                                              */
/* Date:   01. Jan. 02                                                    */
/**************************************************************************/

// #define FLATVECTOR_WITH_DIST

namespace ngbla
{


  template <int S, class T> class Vec;
  template <int S, typename T> class FlatVec;
  template <class T> class SysVector;
  template <class T> class FlatVector;
  template <class T> class Vector;
  template <class T> class SliceVector;
  template <int DIST, typename T> class FixSliceVector;


  extern void CheckVecRange(int s, int i);
  extern void CheckVecRange(int s, int i, int j);



  /**
     new option: 
     A FlatVector becomes the old SliceVector ! 
     overhead can be neglected, but needs more testing, results are welcome ...

     A simple vector.
     Has size and generic data-pointer. 
     No memory allocation/deallocation. User must provide memory.
  */
  template <typename T = double>
  class FlatVector : public CMCPMatExpr<FlatVector<T> > 
  {
  protected:
    /// vector size
    int size;
#ifdef FLATVECTOR_WITH_DIST
    /// distance between entries
    int dist;
#else
    enum { dist = 1 };
#endif
    /// the data
    T * data;
  public:
    /// element type
    typedef T TELEM;
    typedef T& TREF;
    /// scalar of element type
    typedef typename mat_traits<T>::TSCAL TSCAL;
    enum { IS_LINEAR = 0 };

    /// default constructor does nothing
    FlatVector () { ; }
    /// set size and mem
    FlatVector (unsigned int as, T * adata) : size(as), data(adata)
    { 
#ifdef FLATVECTOR_WITH_DIST
      dist = 1;
#endif
    }


#ifdef FLATVECTOR_WITH_DIST
    /// set size and mem
    FlatVector (unsigned int as, int ad, T * adata) : size(as), dist(ad), data(adata) { ; }
#endif

    /// set size and mem
    FlatVector (unsigned int as, void * adata) : size(as), data(static_cast<TELEM*> (adata)) 
    {
#ifdef FLATVECTOR_WITH_DIST
      dist = 1;
#endif
    }

    /// put FlatVector over fixed size vector
    template <int S>
    FlatVector (const Vec<S,TSCAL> & v)
      : size(v.Size()), data(const_cast<T*>(&v(0)))
    { 
#ifdef FLATVECTOR_WITH_DIST
      dist = 1;
#endif
    }

    template <int S>
    FlatVector (const FlatVec<S,TSCAL> & v)
      : size(v.Size()), data(const_cast<T*>(&v(0)))
    { 
#ifdef FLATVECTOR_WITH_DIST
      dist = 1;
#endif
    }

    /// allocate FlatVector on local heap
    FlatVector (int as, LocalHeap & lh) 
      : size(as), data(lh.Alloc<T> (as)) 
    {
#ifdef FLATVECTOR_WITH_DIST
      dist = 1;
#endif
    }

    /// put FlatVector over systemvector
    FlatVector (const SysVector<TSCAL> & sv)
      : size(sv.Size()*sv.BlockSize() / mat_traits<T>::VDIM), 
	data (sv(0))
    {
#ifdef FLATVECTOR_WITH_DIST
      dist = 1;
#endif
    }



    /// allocate and compute 
    template<typename TB>
    FlatVector (const LocalHeapExpr<TB> & m2) 
    {
      size = m2.A().Height();
#ifdef FLATVECTOR_WITH_DIST
      dist = 1;
#endif
      LocalHeap & lh = m2.GetLocalHeap();
      data = lh.Alloc<T> (size);

      // does not copy FlatVectors and don't know why
      // CMCPMatExpr<FlatVector<T> >::operator= (m2.A()); 

      for (int j = 0; j < size; j++) data[j] = m2.A()(j);
    }

    /// assign memory for vector on local heap
    void AssignMemory (int as, LocalHeap & lh) 
    {
      size = as;
#ifdef FLATVECTOR_WITH_DIST
      dist = 1;
#endif
      data = lh.Alloc<T>(size);
    }

    /// assign memory for vector
    void AssignMemory (int as, T * mem) 
    {
      size = as;
#ifdef FLATVECTOR_WITH_DIST
      dist = 1;
#endif
      data = mem;
    }

    /// copy vector. sizes must match
    const FlatVector & operator= (const FlatVector & v) const
    {
      for (int i = 0; i < size; i++)
	data[i*dist] = v(i);
      return *this;
    }

    /// evaluate matrix expression
    template<typename TB>
    const FlatVector & operator= (const Expr<TB> & v) const
    {
      return CMCPMatExpr<FlatVector>::operator= (v);
    }

    /// assign constant value
    const FlatVector & operator= (TSCAL scal) const
    {
      for (int i = 0; i < size; i++)
	data[i*dist] = scal; 
      return *this;
    }

    template<typename TB>
    ALWAYS_INLINE const FlatVector & operator+= (const Expr<TB> & v) const
    {
      if (TB::IS_LINEAR)
	for (int i = 0; i < size; i++)
	  data[i*dist] += v.Spec()(i);
      else
	for (int i = 0; i < size; i++)
	  data[i*dist] += v.Spec()(i,0);
      return *this;
    }

    /// constant element access
    TELEM & operator() (int i) const
    {
#ifdef CHECK_RANGE
      CheckVecRange(size,i);
#endif
      return data[i*dist]; 
    }

    RowsArrayExpr<FlatVector> operator() (FlatArray<int> rows) const
    { 
      return RowsArrayExpr<FlatVector> (*this, rows);
    }
    
    
    /// element access. index j is ignored
    TELEM & operator() (int i, int j) const
    {
#ifdef CHECK_RANGE 
      CheckVecRange(size,i);
#endif
      return data[i*dist]; 
    }

    /// constant element access
    TELEM & operator[] (int i) const
    {
#ifdef CHECK_RANGE
      CheckVecRange(size,i);
#endif
      return data[i*dist]; 
    }

    // shape functions had a problem with icc v9.1
    // const CArray<T> Addr(int i) const { return CArray<T> (data+i*dist); }
    T * Addr(int i) const { return data+i*dist; }
    /*
    const CArray<T> operator+(int i) const
    { return CArray<T> (data+i*dist); }
    */

    T * operator+(int i) const { return data+i*dist; }


    /// sub-vector of size next-first, starting at first
#ifdef FLATVECTOR_WITH_DIST
    const FlatVector<T> Range (int first, int next) const
    { return FlatVector<T> (next-first, dist, data+dist*first); }
#else
    const FlatVector<T> Range (int first, int next) const
    { return FlatVector<T> (next-first, data+dist*first); }
#endif


    /// sub-vector given by range
    const FlatVector<T> Range (IntRange range) const
    { return Range (range.First(), range.Next()); }


    /// vector size
    int Size () const { return size; }

    /// vector is matrix of height size
    int Height () const { return size; }

    /// vector is matrix of with 1
    int Width () const { return 1; }
    

#ifdef FLATVECTOR_WITH_DIST
    /// take a slice of the vector. Take elements first+i * dist. 
    const FlatVector Slice (int first, int dist2) const
    {
      return FlatVector(size/dist2, dist*dist2, data+dist*first);
    }
#else
    /// take a slice of the vector. Take elements first+i * dist. 
    const SliceVector<T> Slice (int first, int dist2) const
    {
      return SliceVector<T> (size/dist2, dist*dist2, data+first);
    }
#endif

    FlatMatrix<T> AsMatrix (int h, int w)
    {
      return FlatMatrix<T> (h,w, data);
    }

    T * Data () const { return data; }

    // new for SysVectors:
    typedef FlatVector<T> TV_COL;
    typedef double TV_ROW;
    enum { HEIGHT = 1 };
    enum { WIDTH = 1 };
  };












  template <int S, typename T>
  class FlatVector<Vec<S, T> > : public CMCPMatExpr<FlatVector<Vec<S, T> > > 
  {
  protected:
    /// vector size
    int size;
    /// the data
    T * data;
  public:
    /// element type
    typedef Vec<S,T> TELEM;
    typedef FlatVec<S,T> TREF;
    /// scalar of element type
    typedef typename mat_traits<T>::TSCAL TSCAL;

    /// default constructor does nothing
    FlatVector () { ; }  
    /// copy pointer
    FlatVector (const FlatVector & v2) : size(v2.size), data(v2.data) { ; }
    /// set size and mem
    FlatVector (unsigned int as, T * adata) : size(as), data(adata) { ; }
    
    /// set size and mem
    FlatVector (unsigned int as, void * adata) : size(as), data(static_cast<T*> (adata)) { ; }
    
    /*
    /// put FlatVector over fixed size vector
    template <int S>
    FlatVector (const Vec<S,TSCAL> & v)
      : s(v.Size()), data(const_cast<T*>(&v(0)))
    { ; }
    */

    /// allocate FlatVector on local heap
    FlatVector (int as, LocalHeap & lh) 
      : size(as), data((T*)lh.Alloc(size*S*sizeof(T))) { ; }

    /*
    /// put FlatVector over systemvector
    FlatVector (const SysVector<TSCAL> & sv)
      : s(sv.Size()*sv.BlockSize() / mat_traits<T>::VDIM), 
	data (sv(0))
    {
      ;
    }
    */

    /// assign memory for vector on local heap
    void AssignMemory (int as, LocalHeap & lh) 
    {
      size = as;
      data = (T*)lh.Alloc(size*S*sizeof(T));
    }

    /// assign memory for vector
    void AssignMemory (int as, T * mem) 
    {
      size = as;
      data = mem;
    }

    /// copy vector. sizes must match
    const FlatVector & operator= (const FlatVector & v) const
    {
      for (int i = 0; i < size; i++)
	(*this)(i) = v(i);
      return *this;
    }

    /// evaluate matrix expression
    template<typename TB>
    const FlatVector & operator= (const Expr<TB> & v) const
    {
      return CMCPMatExpr<FlatVector>::operator= (v);
    }

    /// assign constant value
    const FlatVector & operator= (TSCAL scal) const
    {
      for (int i = 0; i < size; i++)
	(*this)(i) = scal; 
      return *this;
    }

    template<typename TB>
    ALWAYS_INLINE const FlatVector & operator+= (const Expr<TB> & v) const
    {
      if (TB::IS_LINEAR)
	for (int i = 0; i < size; i++)
	  (*this)(i) += v.Spec()(i);
      else
	for (int i = 0; i < size; i++)
	  (*this)(i) += v.Spec()(i,0);
      return *this;
    }

    /// constant element access
    const FlatVec<S,T> operator() (int i) const
    {
      return FlatVec<S,T> (data+i*S); 
    }

    /// element access. index j is ignored
    const FlatVec<S,T> operator() (int i, int j) const
    {
      return FlatVec<S,T> (data+i*S); 
    }

    /// constant element access
    const FlatVec<S,T> operator[] (int i) const
    {
      return FlatVec<S,T> (data+i*S); 
    }

    RowsArrayExpr<FlatVector> operator() (FlatArray<int> rows) const
    { 
      return RowsArrayExpr<FlatVector> (*this, rows);
    }

    /*
    // shape functions had a problem with icc v9.1
    Vec<S,T> * Addr(int i) const
    {
      return static_cast<Vec<S,T>*> ((void*) (data+i*S)); 
    }
    */

    /*
    const CArray<Vec<S,T> > Addr(int i) const
    {
      return CArray<Vec<S,T> > (static_cast<Vec<S,T>*> ((void*) (data+i*S))); 
    }
    */
    /*
    const CArray<T> Addr(int i) const
    {
      return CArray<T> (data+i*S); 
    }
    */

    /// sub-vector of size next-first, starting at first
    const FlatVector<Vec<S,T> > Range (int first, int next) const
    { return FlatVector<Vec<S,T> > (next-first, data+S*first); }

    /// sub-vector given by range
    const FlatVector<Vec<S,T> > Range (IntRange range) const
    { return FlatVector<Vec<S,T> > (range.Next()-range.First(), data+S*range.First()); }

    /// vector size
    int Size () const { return size; }

    /// vector is matrix of height size
    int Height () const { return size; }

    /// vector is matrix of with 1
    int Width () const { return 1; }

    /*
    /// take a slice of the vector. Take elements first+i * dist. 
    SliceVector<T> Slice (int first, int dist)
    {
      return SliceVector<T> (s/dist, dist, data+first);
    }

    /// take a slice of the vector. Take elements first+i * dist. 
    const SliceVector<T> Slice (int first, int dist) const
    {
      return SliceVector<T> (s/dist, dist, data+first);
    }
    */

    /// access to data
    const void * Data () const { return static_cast<const void*>(data); }
    /// access to data
    void * Data () { return static_cast<void*>(data); }

    // new for SysVectors:
    // typedef FlatVector<T> TV_COL;
    // typedef double TV_ROW;
    enum { HEIGHT = 1 };
    enum { WIDTH = 1 };
  };





  

  /**
     A Vector class with memory allocation/deallocation
  */
  template <typename T = double>
  class Vector : public FlatVector<T>
  {
  public:
    typedef typename mat_traits<T>::TSCAL TSCAL;

    /// default constructor
    Vector () : FlatVector<T> (0, (T*)0) { ; }

    /// allocate vector
    explicit Vector (int as) : FlatVector<T> (as, new T[as]) { ; }


    /// allocate and copy matrix  
    Vector (const Vector & v2) 
      : FlatVector<T> (v2.Size(), new T[v2.Size()]) 
    {
      FlatVector<T>::operator= (v2);
    }
    
    /// allocate and compute 
    template<typename TB>
    Vector (const Expr<TB> & v2) 
      : FlatVector<T> (v2.Height(), new T[v2.Height()]) 
    {
      FlatVector<T>::operator= (v2);
    }


    /// deallocate vector
    ~Vector() { delete [] this->data; }

    /// set vector to constant values
    Vector & operator= (TSCAL scal)
    {
      FlatVector<T>::operator= (scal);
      return *this;
    }

    /// set vector size
    void SetSize(int as)
    {
      if (this->size == as) return;
      delete [] this->data;
      this->size = as;
      this->data = new T[this->size];
    }

    /// evaluate matrix expression
    template<typename TB>
    Vector & operator= (const Expr<TB> & v)
    {
      MatExpr<FlatVector<T> >::operator= (v);
      return *this;
    }

    Vector & operator= (const Vector & v2)
    {
      FlatVector<T>::operator= (static_cast<FlatVector<T> >(v2));
      // MatExpr<FlatVector<T> >::operator= (v2);  // does not work, we don't know why
      return *this;
    }

  };





  template <int S, typename T>
  class Vector<Vec<S,T> > : public FlatVector<Vec<S,T> >
  {
  public:
    typedef typename mat_traits<T>::TSCAL TSCAL;

    /// default constructor
    Vector () : FlatVector<Vec<S,T> > (0, (T*)0) { ; }

    /// allocate vector
    explicit Vector (int as) : FlatVector<Vec<S,T> > (as, new T[as*S]) { ; }


    /// allocate and copy matrix  
    Vector (const Vector & v2) 
      : FlatVector<Vec<S,T> > (v2.Size(), new T[S*v2.Size()]) 
    {
      FlatVector<Vec<S,T> >::operator= (v2);
    }
    
    /// allocate and compute 
    template<typename TB>
    Vector (const Expr<TB> & v2) 
      : FlatVector<Vec<S,T> > (v2.Height(), new T[S*v2.Height()]) 
    {
      FlatVector<Vec<S,T> >::operator= (v2);
    }

    /// deallocate vector
    ~Vector() { delete [] this->data; }

    /// set vector to constant values
    Vector & operator= (TSCAL scal)
    {
      FlatVector<Vec<S,T> >::operator= (scal);
      return *this;
    }

    /// set vector size
    void SetSize(int as)
    {
      if (this->size == as) return;
      delete [] this->data;
      this->size = as;
      this->data = new T[S*this->size];
    }

    /// evaluate matrix expression
    template<typename TB>
    Vector & operator= (const Expr<TB> & v)
    {
      MatExpr<FlatVector<Vec<S,T> > >::operator= (v);
      return *this;
    }

    Vector & operator= (const Vector & v2)
    {
      FlatVector<Vec<S,T> >::operator= (static_cast<FlatVector<Vec<S,T> > >(v2));
      // MatExpr<FlatVector<Vec<S,T> > >::operator= (v2);  // does not work, we don't know why
      return *this;
    }

  };







  /**
     A Vector class with memory allocation/deallocation.  At
     compile-time, a certain amount of vector entries is defined. If the
     dynamic size fits into this memory, the vector is allocated in this memory.
     Otherwise, dynamic memory is allocated.
  */
  template <int S, typename T = double>
  class VectorMem : public FlatVector<T>
  {
    /// a predfined amount of memory
    double mem[(S*sizeof(T)+7) / 8];   // alignment (on ia64 machines)
    // T mem[S];                     // should be best, but calls trivial default constructor 
  public:
    /// the scalar type
    typedef typename mat_traits<T>::TSCAL TSCAL;

    /** allocate vector. 
	If the dynamic size fits into the static size, use static memory. Otherwise use dynamic alloation
    */
    explicit VectorMem (int as) : FlatVector<T> (as, (as <= S) ? 
						 static_cast<void*> (&mem[0]) : 
						 static_cast<void*> (new T[as])) { ; }

    /// deallocates dynamic memory
    ~VectorMem() { if (this->size > S) delete [] this->data; }

    /// assigns constant value
    VectorMem & operator= (TSCAL scal)
    {
      FlatVector<T>::operator= (scal);
      return *this;
    }

    /// evaluates matrix expression
    template<typename TB>
    VectorMem & operator= (const Expr<TB> & v)
    {
      MatExpr<FlatVector<T> >::operator= (v);
      return *this;
    }
  };















  // A system vector (not completely functional)
  template <typename T = double>
  class FlatSysVector : public MatExpr<FlatSysVector<T> >
  {
  protected:
    int s;
    int blocksize;
    T * data;
  public:
    typedef FlatVector<T> TELEM;
    typedef typename mat_traits<T>::TSCAL TSCAL;
  
    FlatSysVector (int as, int bs, T * adata) 
      : s(as), blocksize(bs), data(adata) { ; }
  
    FlatSysVector (int as, int bs, LocalHeap & lh) 
      : s(as), blocksize(bs), data (new (lh) T[as*bs])
    { ; }
  
    /*
      FlatSysVector (const FlatSysVector<TSCAL> & sv)
      : s(sv.Size()),
      blocksize(sv.BlockSize()),
      data (sv(0))
      {
      ;
      }
    */
  
    FlatSysVector & operator= (const FlatSysVector & v)
    {
      for (int i = 0; i < this->s * this->bs; i++)
	data[i] = v.data[i];
      return *this;
    }
  
    template<typename TB>
    FlatSysVector & operator= (const Expr<TB> & v)
    {
      return MatExpr<FlatSysVector>::operator= (v);
    }
  
    FlatSysVector & operator= (TSCAL s)
    {
      for (int i = 0; i < this->s*this->bs; i++)
	data[i] = s;
      return *this;
    }

  

    TELEM operator() (int i) 
    {
      return FlatVector<T> (blocksize, &data[i*blocksize]); 
    }

    const TELEM operator() (int i) const
    {
      return FlatVector<T> (blocksize, &data[i*blocksize]); 
    }

    const TELEM operator() (int i, int j) const
    {
      return FlatVector<T> (blocksize, &data[i*blocksize]); 
    }


    FlatSysVector<T> Range(int first, int last)
    { return FlatSysVector<T> (last-first+1, blocksize, data+(first*blocksize)); }
    const FlatSysVector<T> Range(int first, int last) const
    { return FlatSysVector<T> (last-first+1, blocksize, data+(first*blocksize)); }

    int Size () const { return s; }
    int Height () const { return s; }
    int Width () const { return 1; }
  };



  /**
     A vector of fixed size.
     Useful as entry type in system vectors.
  */
  template <int S, typename T = double>
  class Vec : public MatExpr<Vec<S,T> > // , protected BaseVec
  {
    /// the values
    T data[(S>0)?S:1];
  public:
    /// type of the elements
    typedef T TELEM;
    /// is the element double or complex ?
    typedef typename mat_traits<T>::TSCAL TSCAL;
    /// a vec is a S times 1 matrix, the according colume vector
    typedef Vec<S, typename mat_traits<T>::TV_COL> TV_COL;
    /// a vec is a S times 1 matrix, the according row vector
    typedef Vec<1, typename mat_traits<T>::TV_ROW> TV_ROW;

    enum { SIZE = S };
    /// height of matrix
    enum { HEIGHT = S };
    /// with of matrix
    enum { WIDTH  = 1 };

    /// constructor, no initialization
    Vec () { ; }
    /// copy vector
    Vec (const Vec & v) : MatExpr<Vec> ()
    {
      for (int i = 0; i < S; i++)
	data[i] = v.data[i];
    }

    /// initialize with values
    Vec (const TELEM & scal)
    {
      for (int i = 0; i < S; i++)
	data[i] = scal;
    }

    /// initialize with expression
    template<typename TB>
    Vec (const Expr<TB> & v)
    {
      (*this) = v;
    }

    /// initialize zeroth and first elements
    Vec (const T & s1, const T & s2)
    {
      data[0] = s1;
      data[1] = s2;
    }

    /// initialize zeroth, first, and second elements
    Vec (const T & s1, const T & s2, const T & s3)
    {
      data[0] = s1;
      data[1] = s2;
      data[2] = s3;
    }

    /// initialize zeroth, first, and second elements
    Vec (const T & s1, const T & s2, const T & s3, const T & s4)
    {
      data[0] = s1;
      data[1] = s2;
      data[2] = s3;
      data[3] = s4;
    }


  
    /// copy vector
    Vec & operator= (const Vec & v)
    {
      for (int i = 0; i < S; i++)
	data[i] = v.data[i];
      return *this;
    }
  

    /// assign scalar value
    Vec & operator= (const TELEM & scal)
    {
      for (int i = 0; i < S; i++)
	data[i] = scal;
      return *this;
    }

    /// assign expression
    template<typename TB>
    Vec & operator= (const Expr<TB> & v)
    {
      for (int i = 0; i < S; i++)
	data[i] = v.Spec()(i,0);
      return *this;
    }

    /// access vector
    TELEM & operator() (int i) 
    {
#ifdef CHECK_RANGE
      CheckVecRange(S,i);
#endif
      return data[i]; 
    }

    /// access vector
    const TELEM & operator() (int i) const 
    {
#ifdef CHECK_RANGE
      CheckVecRange(S,i);
#endif
      return data[i]; 
    }


    /// access vector
    TELEM & operator[] (int i) 
    {
#ifdef CHECK_RANGE
      CheckVecRange(S,i);
#endif
      return data[i]; 
    }

    /// access vector
    const TELEM & operator[] (int i) const 
    {
#ifdef CHECK_RANGE
      CheckVecRange(S,i);
#endif
      return data[i]; 
    }

    /// access vector
    TELEM & operator() (int i, int j) 
    {
#ifdef CHECK_RANGE
      CheckVecRange(S,i);
#endif
      return data[i]; 
    }

    /// access vector
    const TELEM & operator() (int i, int j) const 
    {
#ifdef CHECK_RANGE
      CheckVecRange(S,i);
#endif
      return data[i]; 
    }

    /// vector size
    int Size () const { return S; }
    /// corresponding matrix height
    int Height () const { return S; }
    /// corresponding matrix with
    int Width () const { return 1; }

    const FlatVector<const T> Range(int first, int next) const
    { return FlatVector<const T> (next-first, data+first); }

    const FlatVector<T> Range(int first, int next) 
    { return FlatVector<T> (next-first, data+first); }
  };





  /// cross product
  template <typename S>
  inline Vec<3,S> Cross (const Vec<3,S> & a, const Vec<3,S> & b)
  {
    Vec<3,S> prod;
    prod(0) = a(1) * b(2) - a(2) * b(1);
    prod(1) = a(2) * b(0) - a(0) * b(2);
    prod(2) = a(0) * b(1) - a(1) * b(0);
    return prod;
  }

  /// output vector
  template<int S, typename T>
  inline ostream & operator<< (ostream & ost, const Vec<S,T> & v)
  {
    for (int i = 0; i < S; i++)
      ost << " " << setw(7) << v(i);
    return ost;
  }


  template<int S, typename TB>
  ALWAYS_INLINE inline Vec<S> & operator+= (Vec<S> & v, const Expr<TB> & v2)
  {
    for (int i = 0; i < S; i++)
      v(i) += v2.Spec()(i,0);
    return v;
  }








  /**
     A pointer to a vector of fixed size.
  */
  template <int S, typename T = double>
  class FlatVec : public CMCPMatExpr<FlatVec<S,T> > 
  {
    /// the values
    T * data;
  public:
    /// type of the elements
    typedef T TELEM;
    /// is the element double or complex ?
    typedef typename mat_traits<T>::TSCAL TSCAL;
    /// a vec is a S times 1 matrix, the according colume vector
    typedef Vec<S, typename mat_traits<T>::TV_COL> TV_COL;
    /// a vec is a S times 1 matrix, the according row vector
    typedef Vec<1, typename mat_traits<T>::TV_ROW> TV_ROW;

    enum { SIZE = S };
    /// height of matrix
    enum { HEIGHT = S };
    /// with of matrix
    enum { WIDTH  = 1 };

    /// constructor, no initialization
    FlatVec (T * adata) : data(adata) { ; }

    /// constructor, no initialization
    FlatVec (Vec<S,T> & v2) : data(&v2(0)) { ; }


    /// copy vector
    const FlatVec & operator= (const FlatVec & v) const
    {
      for (int i = 0; i < S; i++)
	data[i] = v.data[i];
      return *this;
    }

    /// assign scalar value
    const FlatVec & operator= (TSCAL scal) const
    {
      for (int i = 0; i < S; i++)
	data[i] = scal;
      return *this;
    }

    /// assign expression
    template<typename TB>
    const FlatVec & operator= (const Expr<TB> & v) const
    {
      for (int i = 0; i < S; i++)
	data[i] = v.Spec()(i,0);
      return *this;
    }

    template<typename TB>
    ALWAYS_INLINE const FlatVec & operator+= (const Expr<TB> & v) const
    {
      for (int i = 0; i < S; i++)
	data[i] += v.Spec()(i,0);
      return *this;
    }

    /// access vector
    TELEM & operator() (int i) const 
    {
#ifdef CHECK_RANGE
      CheckVecRange(S,i);
#endif
      return data[i]; 
    }

    /// access vector
    TELEM & operator[] (int i) const 
    {
#ifdef CHECK_RANGE
      CheckVecRange(S,i);
#endif
      return data[i]; 
    }

    /// access vector
    TELEM & operator() (int i, int j) const 
    {
#ifdef CHECK_RANGE
      CheckVecRange(S,i);
#endif
      return data[i]; 
    }

    const FlatVector<T> Range(int first, int next) const
    { return FlatVector<T> (next-first, data+first); }

    /// vector size
    int Size () const { return S; }
    /// corresponding matrix height
    int Height () const { return S; }
    /// corresponding matrix with
    int Width () const { return 1; }
  };

  /// output vector.
  template<int S, typename T>
  inline ostream & operator<< (ostream & ost, const FlatVec<S,T> & v)
  {
    for (int i = 0; i < S; i++)
      ost << " " << setw(7) << v(i);
    return ost;
  }















  /**
     A vector with non-linear data access.
     Has size and generic data-pointer. 
     No memory allocation/deallocation. User must provide memory.
  */
  template <typename T = double>
  class SliceVector : public CMCPMatExpr<SliceVector<T> > 
  {
  protected:
    /// vector size
    int s;
    /// distance between entries
    int dist;
    /// the data
    T * data;
  public:
    /// the entry type
    typedef T TELEM;
    /// the scalar type of the vector
    typedef typename mat_traits<T>::TSCAL TSCAL;

    /// element access is not linear
    enum { IS_LINEAR = 0 };

    /// set size, distance and memory
    SliceVector (unsigned int as, unsigned int ad, T * adata) 
      : s(as), dist(ad), data(adata) { ; }
    
    /// SV from FlatVector
    SliceVector (FlatVector<T> fv)
      : s(fv.Size()), dist(&fv(1)-&fv(0)), data((T*)fv.Data()) { ; }
    
    ///
    template <int D>
    SliceVector (FixSliceVector<D,T> fsv)
      : s(fsv.Size()), dist(D), data(&fsv(0)) { ; }


    /// evaluates matrix expression
    template<typename TB>
    const SliceVector & operator= (const Expr<TB> & v) const
    {
      return CMCPMatExpr<SliceVector>::operator= (v);
    }

    /// assignes constant value
    const SliceVector & operator= (TSCAL scal) const
    {
      for (int i = 0; i < s; i++)
	data[i*dist] = scal; 
      return *this;
    }

    /// copies contents of vector
    const SliceVector & operator= (const SliceVector & v2) const
    {
      for (int i = 0; i < s; i++)
	data[i*dist] = v2(i);
      return *this;
    }


    template<typename TB>
    ALWAYS_INLINE const SliceVector & operator+= (const Expr<TB> & v) const
    {
      if (TB::IS_LINEAR)
	for (int i = 0; i < s; i++)
	  data[i*dist] += v.Spec()(i);
      else
	for (int i = 0; i < s; i++)
	  data[i*dist] += v.Spec()(i,0);
      return *this;
    }



    /// access element
    TELEM & operator() (int i) 
    {
#ifdef CHECK_RANGE
      CheckVecRange(s,i);
#endif
      return data[i*dist]; 
    }

    /// access element
    TELEM & operator() (int i) const
    {
#ifdef CHECK_RANGE
      CheckVecRange(s,i);
#endif
      return data[i*dist]; 
    }

    /// access element, index j is unused
    TELEM & operator() (int i, int j) const
    {
#ifdef CHECK_RANGE
      CheckVecRange(s,i);
#endif
      return data[i*dist]; 
    }

    /// access element, index j is unused
    TELEM & operator() (int i, int j) 
    {
#ifdef CHECK_RANGE
      CheckVecRange(s,i);
#endif
      return data[i*dist]; 
    }

    /// access element
    TELEM & operator[] (int i) 
    {
#ifdef CHECK_RANGE
      CheckVecRange(s,i);
#endif
      return data[i*dist]; 
    }

    /// access element
    TELEM & operator[] (int i) const
    {
#ifdef CHECK_RANGE
      CheckVecRange(s,i);
#endif
      return data[i*dist]; 
    }

    TELEM * Addr (int i) const
    {
      return data+i*dist;
    }


    /// vector size
    int Size () const { return s; }
    int Dist () const { return dist; }
    /// vector is a matrix of hight size
    int Height () const { return s; }
    /// vector is a matrix of width 1
    int Width () const { return 1; }

    T * Data () const { return data; }


    const SliceVector<T> Range (int first, int next) const
    {
      return SliceVector<T> (next-first, dist, data+first*dist);
    }

    const SliceVector<T> Range (IntRange range) const
    {
      return Range (range.First(), range.Next());
    }


    const SliceVector<T> Slice (int first, int adist) const
    {
      return SliceVector<T> (s/adist, dist*adist, data+first*dist);
    }
  };



















  /**
     A vector with non-linear data access.
     Has size and generic data-pointer. 
     No memory allocation/deallocation. User must provide memory.
  */
  template <int DIST, typename T = double>
  class FixSliceVector : public CMCPMatExpr<FixSliceVector<DIST, T> > 
  {
  protected:
    /// vector size
    int s;
    /// the data
    T * data;
  public:
    /// the entry type
    typedef T TELEM;
    /// the scalar type of the vector
    typedef typename mat_traits<T>::TSCAL TSCAL;

    /// element access is not linear
    enum { IS_LINEAR = 0 };

    /// set size, distance and memory
    FixSliceVector (unsigned int as, T * adata) 
      : s(as), data(adata) { ; }

    /// evaluates matrix expression
    template<typename TB>
    FixSliceVector & operator= (const Expr<TB> & v)
    {
      return MatExpr<FixSliceVector>::operator= (v);
    }

    /// assignes constant value
    const FixSliceVector & operator= (TSCAL scal) const
    {
      for (int i = 0; i < s; i++)
	data[i*DIST] = scal; 
      return *this;
    }

    /// copies contents of vector
    const FixSliceVector & operator= (const FixSliceVector & v2) const
    {
      for (int i = 0; i < s; i++)
	data[i*DIST] = v2(i);
      return *this;
    }

    template<typename TB>
    const FixSliceVector & operator= (const Expr<TB> & v) const
    {
      if (TB::IS_LINEAR)
	for (int i = 0; i < s; i++)
	  data[i*DIST] = v.Spec()(i);
      else
	for (int i = 0; i < s; i++)
	  data[i*DIST] = v.Spec()(i,0);
      return *this;
    }


    template<typename TB>
    ALWAYS_INLINE const FixSliceVector & operator+= (const Expr<TB> & v) const
    {
      if (TB::IS_LINEAR)
	for (int i = 0; i < s; i++)
	  data[i*DIST] += v.Spec()(i);
      else
	for (int i = 0; i < s; i++)
	  data[i*DIST] += v.Spec()(i,0);
      return *this;
    }



    /// access element
    TELEM & operator() (int i) 
    {
#ifdef CHECK_RANGE
      CheckVecRange(s,i);
#endif
      return data[i*DIST]; 
    }

    /// access element
    TELEM & operator() (int i) const
    {
#ifdef CHECK_RANGE
      CheckVecRange(s,i);
#endif
      return data[i*DIST]; 
    }

    /// access element, index j is unused
    TELEM & operator() (int i, int j) const
    {
#ifdef CHECK_RANGE
      CheckVecRange(s,i);
#endif
      return data[i*DIST]; 
    }

    /// access element, index j is unused
    TELEM & operator() (int i, int j) 
    {
#ifdef CHECK_RANGE
      CheckVecRange(s,i);
#endif
      return data[i*DIST]; 
    }

    /// access element
    TELEM & operator[] (int i) 
    {
#ifdef CHECK_RANGE
      CheckVecRange(s,i);
#endif
      return data[i*DIST]; 
    }

    /// access element
    TELEM & operator[] (int i) const
    {
#ifdef CHECK_RANGE
      CheckVecRange(s,i);
#endif
      return data[i*DIST]; 
    }

    /*
    TELEM * Addr (int i) const
    {
      return data+i*DIST;
    }
    */

    /// vector size
    int Size () const { return s; }

    /// vector is a matrix of hight size
    int Height () const { return s; }
    /// vector is a matrix of width 1
    int Width () const { return 1; }

    const FixSliceVector Range (int first, int next) const
    {
      return FixSliceVector (next-first, data+first*DIST);
    }

    const FixSliceVector Range (IntRange range) const
    {
      return Range (range.First(), range.Next());
    }


    const SliceVector<T> Slice (int first, int adist) const
    {
      return SliceVector<T> (s/adist, DIST*adist, data+first*DIST);
    }
  };










  template <class TV, class TSCAL> class Scalar2ElemVector
  {
  public:
    const FlatVector<TSCAL> & vec;
    Scalar2ElemVector (const FlatVector<TSCAL> & avec) : vec(avec) { ; }

    enum { H = mat_traits<TV>::HEIGHT };

    FlatVec<H,TSCAL> operator() (int i) const
    {
      return FlatVec<H,TSCAL> (&vec(i*H));
    }

  };
  

  template <class TSCAL> class Scalar2ElemVector<TSCAL,TSCAL>
  {
  public:
    const FlatVector<TSCAL> & vec;
    Scalar2ElemVector (const FlatVector<TSCAL> & avec) : vec(avec) { ; }


    const TSCAL & operator() (int i) const
    {
      return vec(i);
    }

    TSCAL & operator() (int i)
    {
      return vec(i);
    }

  };






  template <class T>
  class mat_traits<FlatVector<T> >
  {
  public:
    typedef T TELEM;
    typedef T TSCAL;
  };


}

#ifdef PARALLEL
namespace ngstd
{
  template<int S, typename T>
  class MPI_Traits<ngbla::Vec<S, T> >
  {
  public:
    static MPI_Datatype MPIType () 
    { 
      static MPI_Datatype MPI_T = 0;
      if (!MPI_T)
	{
	  MPI_Type_contiguous ( S, MPI_Traits<T>::MPIType(), &MPI_T);
	  MPI_Type_commit ( &MPI_T );
	}
      return MPI_T;
    }
  };
}
#endif

#endif
