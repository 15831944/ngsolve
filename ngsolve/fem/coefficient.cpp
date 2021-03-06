/*********************************************************************/
/* File:   coefficient.cpp                                           */
/* Author: Joachim Schoeberl                                         */
/* Date:   24. Jan. 2002                                             */
/*********************************************************************/

/* 
   Finite Element Coefficient Function
*/

#include <fem.hpp>
#include <../ngstd/evalfunc.hpp>

namespace ngfem
{
  
  CoefficientFunction :: CoefficientFunction ()
  { ; }

  CoefficientFunction :: ~CoefficientFunction ()
  { ; }


  void CoefficientFunction :: PrintReport (ostream & ost) const
  {
    ost << "CoefficientFunction is " << typeid(*this).name() << endl;
  }
  
  void CoefficientFunction :: 
  Evaluate (const BaseMappedIntegrationRule & ir, FlatMatrix<double> values) const
  {
    for (int i = 0; i < ir.Size(); i++)
      Evaluate (ir[i], values.Row(i)); // values(i, 0) = Evaluate (ir[i]);
  }

  void CoefficientFunction :: 
  Evaluate (const BaseMappedIntegrationRule & ir, FlatMatrix<Complex> values) const
  {
    for (int i = 0; i < ir.Size(); i++)
      Evaluate (ir[i], values.Row(i)); 
  }


  ///
  ConstantCoefficientFunction ::   
  ConstantCoefficientFunction (double aval) 
    : val(aval) 
  { ; }

  ConstantCoefficientFunction ::
  ~ConstantCoefficientFunction ()
  { ; }

  void ConstantCoefficientFunction :: PrintReport (ostream & ost) const
  {
    ost << "ConstantCF, val = " << val << endl;
  }

  DomainConstantCoefficientFunction :: 
  DomainConstantCoefficientFunction (const Array<double> & aval)
    : val(aval) { ; }
  /*
    .Size())
    {
    for (int i = 0; i < val.Size(); i++)
    val[i] = aval[i];
    }
  */
 
  double DomainConstantCoefficientFunction :: Evaluate (const BaseMappedIntegrationPoint & ip) const
  {
    int elind = ip.GetTransformation().GetElementIndex();
    
    if (elind < 0 || elind >= val.Size())
      {
	ostringstream ost;
	ost << "DomainConstantCoefficientFunction: Element index "
	    << elind << " out of range 0 - " << val.Size()-1 << endl;
	throw Exception (ost.str());
      }
    
    return val[elind]; 
  }

  void DomainConstantCoefficientFunction :: Evaluate (const BaseMappedIntegrationRule & ir, FlatMatrix<double> values) const
  {
    int elind = ir[0].GetTransformation().GetElementIndex();
    
    if (elind < 0 || elind >= val.Size())
      {
	ostringstream ost;
	ost << "DomainConstantCoefficientFunction: Element index "
	    << elind << " out of range 0 - " << val.Size()-1 << endl;
	throw Exception (ost.str());
      }
    
    values = val[elind]; 
  }

  void DomainConstantCoefficientFunction :: Evaluate (const BaseMappedIntegrationRule & ir, FlatMatrix<Complex> values) const
  {
    int elind = ir[0].GetTransformation().GetElementIndex();
    
    if (elind < 0 || elind >= val.Size())
      {
	ostringstream ost;
	ost << "DomainConstantCoefficientFunction: Element index "
	    << elind << " out of range 0 - " << val.Size()-1 << endl;
	throw Exception (ost.str());
      }
    
    values = val[elind]; 
  }


  DomainConstantCoefficientFunction :: 
  ~DomainConstantCoefficientFunction ()
  { ; }



  DomainVariableCoefficientFunction ::
  DomainVariableCoefficientFunction (const EvalFunction & afun)
    : fun(1)
  {
    fun[0] = make_shared<EvalFunction> (afun);
    numarg = 3;
  }

  DomainVariableCoefficientFunction ::
  DomainVariableCoefficientFunction (const EvalFunction & afun,
				     const Array<shared_ptr<CoefficientFunction>> & adepends_on)
    : fun(1), depends_on(adepends_on)
  {
    fun[0] = make_shared<EvalFunction> (afun);
    numarg = 3;
    for (int i = 0; i < depends_on.Size(); i++)
      numarg += depends_on[i]->Dimension();
  }


  DomainVariableCoefficientFunction ::
  DomainVariableCoefficientFunction (const Array<shared_ptr<EvalFunction>> & afun)
    : fun(afun.Size())
  {
    for (int i = 0; i < fun.Size(); i++)
      if (afun[i])
        // fun[i] = new EvalFunction (*afun[i]);
        fun[i] = afun[i];
      else
        fun[i] = nullptr;
    numarg = 3;
  }

  DomainVariableCoefficientFunction ::
  DomainVariableCoefficientFunction (const Array<shared_ptr<EvalFunction>> & afun,
				     const Array<shared_ptr<CoefficientFunction>> & adepends_on)
    : fun(afun.Size()), depends_on(adepends_on)
  {
    for (int i = 0; i < fun.Size(); i++)
      if (afun[i])
        // fun[i] = new EvalFunction (*afun[i]);
        fun[i] = afun[i];
      else
        fun[i] = nullptr;

    numarg = 3;
    for (int i = 0; i < depends_on.Size(); i++)
      numarg += depends_on[i]->Dimension();
  }


  DomainVariableCoefficientFunction ::
  ~DomainVariableCoefficientFunction ()
  {
    ;
    /*
    for (int i = 0; i < fun.Size(); i++)
      delete fun[i];
    */
  }

  double DomainVariableCoefficientFunction ::
  Evaluate (const BaseMappedIntegrationPoint & ip) const
  {
    Vec<1> result;
    Evaluate (ip, result);
    return result(0);
    /*
      int numarg = max(3, depends_on.Size());
      VectorMem<10> args(numarg);
      args.Range(0,DIM) = static_cast<const DimMappedIntegrationPoint<DIM>&>(ip).GetPoint();
    
      for (int i = 3; i < depends_on.Size(); i++)
      args(i) = depends_on[i] -> Evaluate (ip);

      int elind = ip.GetTransformation().GetElementIndex();
      if (fun.Size() == 1) elind = 0;
      double val = fun[elind]->Eval (&args(0));
      return val;
    */
  }

  bool DomainVariableCoefficientFunction :: IsComplex() const 
  {
    for (int i = 0; i < fun.Size(); i++)
      if (fun[i]->IsResultComplex()) return true;
    return false;
  }
  
  int DomainVariableCoefficientFunction :: Dimension() const
  { 
    return fun[0]->Dimension(); 
  }


  Complex DomainVariableCoefficientFunction ::
  EvaluateComplex (const BaseMappedIntegrationPoint & ip) const
  {
    Vec<1, Complex> result;
    Evaluate (ip, result);
    return result(0);
    /*
      int elind = ip.GetTransformation().GetElementIndex();
      Vec<DIM, Complex> hp;
      for (int i = 0; i < DIM; i++)
      hp(i) = static_cast<const DimMappedIntegrationPoint<DIM>&>(ip).GetPoint()(i);
      return fun[elind]->Eval (&hp(0));
    */
  }
  
  void DomainVariableCoefficientFunction ::
  Evaluate(const BaseMappedIntegrationPoint & ip,
	   FlatVector<> result) const
  {
    int elind = ip.GetTransformation().GetElementIndex();
    if (fun.Size() == 1) elind = 0;
    
    if (! fun[elind] -> IsComplex ())
      {
	VectorMem<10> args(numarg);
	// args.Range(0,DIM) = static_cast<const DimMappedIntegrationPoint<DIM>&>(ip).GetPoint();
        args.Range(0,ip.Dim()) = ip.GetPoint();
	
	for (int i = 0, an = 3; i < depends_on.Size(); i++)
	  {
	    int dim = depends_on[i]->Dimension();
	    depends_on[i] -> Evaluate (ip, args.Range(an,an+dim));
	    an += dim;
	  }
	fun[elind]->Eval (&args(0), &result(0), result.Size());      
      }
    else
      {
	VectorMem<10, Complex> args(numarg);
	// args.Range(0,DIM) = static_cast<const DimMappedIntegrationPoint<DIM>&>(ip).GetPoint();
        args.Range(0,ip.Dim()) = ip.GetPoint();
	
	for (int i = 0, an = 3; i < depends_on.Size(); i++)
	  {
	    int dim = depends_on[i]->Dimension();
	    depends_on[i] -> Evaluate (ip, args.Range(an,an+dim));
	    an += dim;
	  }
	fun[elind]->Eval (&args(0), &result(0), result.Size());      
      }
  }


  void DomainVariableCoefficientFunction ::
  Evaluate(const BaseMappedIntegrationPoint & ip,
           FlatVector<Complex> result) const
  {
    VectorMem<10,Complex> args(numarg);
    args = -47;
    // args.Range(0,DIM) = static_cast<const DimMappedIntegrationPoint<DIM>&>(ip).GetPoint();
    args.Range(0,ip.Dim()) = ip.GetPoint();
    for (int i = 0, an = 3; i < depends_on.Size(); i++)
      {
        int dim = depends_on[i]->Dimension();
        depends_on[i] -> Evaluate (ip, args.Range(an,an+dim));
        an += dim;
      }
    
    int elind = ip.GetTransformation().GetElementIndex();
    if (fun.Size() == 1) elind = 0;
    fun[elind]->Eval (&args(0), &result(0), result.Size());
  }
  
  
void DomainVariableCoefficientFunction ::
Evaluate (const BaseMappedIntegrationRule & ir, 
	  FlatMatrix<double> values) const
{
  int elind = ir.GetTransformation().GetElementIndex();
  if (fun.Size() == 1) elind = 0;

  if (! fun[elind] -> IsComplex ())
    {
      ArrayMem<double,2000> mem(ir.Size()*numarg);
      FlatMatrix<> args(ir.Size(), numarg, &mem[0]);
      for (int i = 0; i < ir.Size(); i++)
        args.Row(i).Range(0,ir[i].Dim()) = ir[i].GetPoint();
      /*
	args.Row(i).Range(0,DIM) = 
	  static_cast<const DimMappedIntegrationPoint<DIM> & > (ir[i]).GetPoint();
      */
      for (int i = 0, an = 3; i < depends_on.Size(); i++)
	{
	  int dim = depends_on[i]->Dimension();
	  Matrix<> hmat(ir.Size(), dim);
	  depends_on[i] -> Evaluate (ir, hmat);
	  args.Cols(an,an+dim) = hmat;
	  an += dim;
	}
      for (int i = 0; i < ir.Size(); i++)
	fun[elind]->Eval (&args(i,0), &values(i,0), values.Width());
    }
  else
    {
      Matrix<Complex> args(ir.Size(), numarg);
      for (int i = 0; i < ir.Size(); i++)
	args.Row(i).Range(0,ir[i].Dim()) = ir[i].GetPoint();
      
      for (int i = 0, an = 3; i < depends_on.Size(); i++)
	{
	  int dim = depends_on[i]->Dimension();
	  Matrix<Complex> hmat(ir.Size(), dim);
	  depends_on[i] -> Evaluate (ir, hmat);
	  args.Cols(an,an+dim) = hmat;
	  an += dim;
	}
    
      for (int i = 0; i < ir.Size(); i++)
	fun[elind]->Eval (&args(i,0), &values(i,0), values.Width());
    }
}

  
void DomainVariableCoefficientFunction :: PrintReport (ostream & ost) const
{
  *testout << "DomainVariableCoefficientFunction, functios are: " << endl;
  for (int i = 0; i < fun.Size(); i++)
    fun[i] -> Print(ost);
}


  /*
  template class DomainVariableCoefficientFunction<1>;
template class DomainVariableCoefficientFunction<2>;
template class DomainVariableCoefficientFunction<3>;
  */

PolynomialCoefficientFunction::PolynomialCoefficientFunction(const Array < Array< Array<double>* >* > & polycoeffs_in,
							     const Array < Array<double>* > & polybounds_in)
  : polycoeffs(polycoeffs_in), polybounds(polybounds_in)
{}

PolynomialCoefficientFunction::PolynomialCoefficientFunction(const Array < Array<double>* > & polycoeffs_in)
{
  polycoeffs.SetSize(polycoeffs_in.Size());
  polybounds.SetSize(polycoeffs_in.Size());
    
  for(int i=0; i<polycoeffs_in.Size(); i++)
    {
      polycoeffs[i] = new Array< Array<double>* >(1);
      (*polycoeffs[i])[0] = polycoeffs_in[i];
      polybounds[i] = new Array<double>(0);
    } 
}


PolynomialCoefficientFunction::~PolynomialCoefficientFunction()
{
  for(int i=0; i<polycoeffs.Size(); i++)
    {
      delete polybounds[i];
      for(int j=0; j<polycoeffs[i]->Size(); j++)
	{
	  delete (*polycoeffs[i])[j];
	}
      delete polycoeffs[i];
    }
  polycoeffs.DeleteAll();
  polybounds.DeleteAll();
}
    
  
  
double PolynomialCoefficientFunction::Evaluate (const BaseMappedIntegrationPoint & ip) const
{
  return Evaluate(ip,0);
}



double PolynomialCoefficientFunction::EvalPoly(const double t, const Array<double> & coeffs) const
{
  const int last = coeffs.Size()-1;
    
  double retval = coeffs[last];
  for(int i=last-1; i>=0; i--)
    {
      retval *= t;
      retval += coeffs[i];
    }

  return retval;    
}


double PolynomialCoefficientFunction::EvalPolyDeri(const double t, const Array<double> & coeffs) const
{
  const int last = coeffs.Size()-1;

  double retval = last*coeffs[last];
  for(int i=last-1; i>=1; i--)
    {
      retval *= t;
      retval += i*coeffs[i];
    }  

  return retval;    
}


double PolynomialCoefficientFunction::Evaluate (const BaseMappedIntegrationPoint & ip, const double & t) const
{
  const int elind = ip.GetTransformation().GetElementIndex();
    
  if (elind < 0 || elind >= polycoeffs.Size())
    {
      ostringstream ost;
      ost << "PolynomialCoefficientFunction: Element index "
	  << elind << " out of range 0 - " << polycoeffs.Size()-1 << endl;
      throw Exception (ost.str());
    }
 
  int pos;
  for(pos=0; pos < polybounds[elind]->Size() && t > (*polybounds[elind])[pos]; pos++){}
   
  return EvalPoly(t,*((*(polycoeffs[elind]))[pos]));

    
}


 
double PolynomialCoefficientFunction::EvaluateDeri (const BaseMappedIntegrationPoint & ip, const double & t) const
{
  const int elind = ip.GetTransformation().GetElementIndex();
    
  if (elind < 0 || elind >= polycoeffs.Size())
    {
      ostringstream ost;
      ost << "PolynomialCoefficientFunction: Element index "
	  << elind << " out of range 0 - " << polycoeffs.Size()-1 << endl;
      throw Exception (ost.str());
    }

  int pos;
  for(pos=0; pos < polybounds[elind]->Size() && t > (*polybounds[elind])[pos]; pos++){}

  return EvalPolyDeri(t,*((*(polycoeffs[elind]))[pos]));
}


double PolynomialCoefficientFunction::EvaluateConst () const
{
  return (*(*polycoeffs[0])[0])[0];
}



//////////////////

FileCoefficientFunction :: FileCoefficientFunction ()
{
  writeips = false;
}

  
FileCoefficientFunction :: FileCoefficientFunction (const string & filename)
{
  StartWriteIps(filename);
}

FileCoefficientFunction :: FileCoefficientFunction (const string & aipfilename,
						    const string & ainfofilename,
						    const string & avaluesfilename,
						    const bool loadvalues)
{
  ipfilename = aipfilename;
  infofilename = ainfofilename;
  valuesfilename = avaluesfilename;

  if(loadvalues)
    {
      writeips = false;
      LoadValues();
    }
  else
    StartWriteIps();
}
    

  
void FileCoefficientFunction :: EmptyValues(void)
{
  for(int i=0; i<ValuesAtIps.Size(); i++)
    delete ValuesAtIps[i];

  ValuesAtIps.SetSize(0);
}

void FileCoefficientFunction :: Reset(void)
{
  EmptyValues();
}

FileCoefficientFunction :: ~FileCoefficientFunction()
{
  if(writeips)
    StopWriteIps(); 

  EmptyValues();
}


void FileCoefficientFunction :: LoadValues(const string & filename)
{
  cout << "Loading values for coefficient function ..."; cout.flush();

  if(writeips) cerr << "WARNING: CoefficientFunction still writing points to \"" 
		    << ipfilename << "\"" << endl;

  ifstream infile(filename.c_str());
    
  int numels,numips,numentries,eln,ipn;
  double val;

  infile >> numels;
  infile >> numips;
  infile >> numentries;
    
  EmptyValues();
    
  ValuesAtIps.SetSize(numels);
    
  for(int i=0; i<numels; i++)
    {
      ValuesAtIps[i] = new Array<double>(numips);
      *(ValuesAtIps[i]) = 0.;
    }

  for(int i=0; i<numentries; i++)
    {
      infile >> eln;
      infile >> ipn;
      infile >> val;
      (*(ValuesAtIps[eln]))[ipn] = val;
    }

  infile.close();
  cout << "done\n";
}



double FileCoefficientFunction :: Evaluate (const BaseMappedIntegrationPoint & ip) const
{
  const ElementTransformation & eltrans = ip.GetTransformation();
  const int elnum = eltrans.GetElementNr();
  const int ipnum = ip.GetIPNr();

  if(writeips)
    {
      if(elnum > maxelnum) const_cast<int&> (maxelnum) = elnum;
      if(ipnum > maxipnum) const_cast<int&> (maxipnum) = ipnum;
      const_cast<int&> (totalipnum)++;

      Vec<3> point;
      eltrans.CalcPoint(ip.IP(),point);

      const_cast<ofstream&> (outfile) << elnum << " " << ipnum << " " << point << "\n";
    }

  if(elnum < ValuesAtIps.Size())
    {
      return (*(ValuesAtIps[elnum]))[ipnum];
    }

  return 0.;
}

void FileCoefficientFunction :: StartWriteIps(const string & filename)
{
  writeips = true;
  maxelnum = 0;
  maxipnum = 0;
  totalipnum = 0;

  outfile.open(filename.c_str());
  outfile.precision(12);
    
}

void FileCoefficientFunction :: StopWriteIps(const string & infofilename)
{
  writeips = false;

  outfile.close();

    
  cout << "Stopped writing to " << ipfilename << endl;
  cout << "Writing info file to " << infofilename << endl;

  ofstream info(infofilename.c_str());

  info << "numelts " << maxelnum+1 << endl
       << "maxnumips " << maxipnum+1 << endl
       << "totalipnum " << totalipnum << endl;

  info.close();

}

}

 
