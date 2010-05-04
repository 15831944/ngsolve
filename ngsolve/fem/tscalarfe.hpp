#ifndef FILE_TSCALARFE
#define FILE_TSCALARFE

/*********************************************************************/
/* File:   tscalarfe.hpp                                             */
/* Author: Joachim Schoeberl                                         */
/* Date:   25. Mar. 2000                                             */
/*********************************************************************/

namespace ngfem
{

  // Barton and Nackman Trick

  
  
  template <int DIM>
  class DShapeElement
  {
    double * data;
  public:
    DShapeElement (double * adata) : data(adata) { ; }
    void operator= (AutoDiff<DIM> ad) 
    { 
      for (int i = 0; i < DIM; i++) 
        data[i] = ad.DValue(i); 
    }
  };



  template <int DIM>
  class DShapeAssign
  {
    double * dshape;
  public:
    DShapeAssign (FlatMatrixFixWidth<DIM> mat)
    { dshape = &mat(0,0); }

    DShapeAssign (double * adshape)
    { dshape = adshape; }

    DShapeElement<DIM> operator[] (int i) const
    { return DShapeElement<DIM> (dshape + i*DIM); }

    const DShapeAssign Addr (int i) const
    { return DShapeAssign (dshape+i*DIM); } 
  };

  


  class EvaluateShapeElement
  {
    double coef;
    double * sum;
  public:
    EvaluateShapeElement (double acoef, double * asum)
      : coef(acoef), sum(asum) { ; }

    void operator= (double val) 
    {
      *sum += coef * val;
    }
  };

  class EvaluateShape
  {
    const double * coefs;
    double * sum;
  public:
    EvaluateShape (FlatVector<> acoefs, double * asum)
      : coefs(&acoefs(0)), sum(asum) { ; }
    
    EvaluateShape (const double * acoefs, double * asum)
      : coefs(acoefs), sum(asum) { ; }

    EvaluateShapeElement operator[] (int i) const
    { return EvaluateShapeElement (coefs[i], sum); }

    const EvaluateShape Addr (int i) const
    { return EvaluateShape (coefs+i, sum); } 
  };





  
  
  template <int DIM>
  class EvaluateShapeTransElement
  {
    double & data;
    const double & fac;
  public:
    EvaluateShapeTransElement (double & adata, const double & afac) : data(adata), fac(afac) { ; }
    void operator= (double ad) 
    { 
      data += ad * fac;
    }
  };

  template <int DIM>
  class EvaluateShapeTrans
  {
    double * coefs;
    const double & fac;
  public:
    EvaluateShapeTrans (FlatVector<> acoefs, const double & afac)
      : coefs(&acoefs(0)), fac(afac) { ; }

    EvaluateShapeTrans (double * acoefs, const double & afac)
      : coefs(acoefs), fac(afac) { ; }

    EvaluateShapeTransElement<DIM> operator[] (int i) const
    { return EvaluateShapeTransElement<DIM> (coefs[i], fac); }

    const EvaluateShapeTrans Addr (int i) const
    { return EvaluateShapeTrans (coefs+i, fac); }
  };








  
  template <int DIM>
  class EvaluateDShapeElement
  {
    double data;
    Vec<DIM> & sum;
  public:
    EvaluateDShapeElement (double adata, Vec<DIM> & asum) : data(adata), sum(asum) { ; }
    void operator= (AutoDiff<DIM> ad) 
    { 
      for (int i = 0; i < DIM; i++) 
        sum(i) += ad.DValue(i) * data;
    }
  };

  template <int DIM>
  class EvaluateDShape
  {
    double * coefs;
    Vec<DIM> & sum;
  public:
    EvaluateDShape (FlatVector<> acoefs, Vec<DIM> & asum)
      : coefs(&acoefs(0)), sum(asum) { ; }

    EvaluateDShape (double * acoefs, Vec<DIM> & asum)
      : coefs(acoefs), sum(asum) { ; }

    EvaluateDShapeElement<DIM> operator[] (int i) const
    { return EvaluateDShapeElement<DIM> (coefs[i], sum); }

    const EvaluateDShape Addr (int i) const
    { return EvaluateDShape (coefs+i, sum); }
  };





  
  
  template <int DIM>
  class EvaluateDShapeTransElement
  {
    double & data;
    const Vec<DIM> & fac;
  public:
    EvaluateDShapeTransElement (double & adata, const Vec<DIM> & afac) : data(adata), fac(afac) { ; }
    void operator= (AutoDiff<DIM> ad) 
    { 
      for (int i = 0; i < DIM; i++) 
        data += ad.DValue(i) * fac(i);
    }
  };

  template <int DIM>
  class EvaluateDShapeTrans
  {
    double * coefs;
    const Vec<DIM> & fac;
  public:
    EvaluateDShapeTrans (FlatVector<> acoefs, const Vec<DIM> & afac)
      : coefs(&acoefs(0)), fac(afac) { ; }

    EvaluateDShapeTrans (double * acoefs, const Vec<DIM> & afac)
      : coefs(acoefs), fac(afac) { ; }

    EvaluateDShapeTransElement<DIM> operator[] (int i) const
    { return EvaluateDShapeTransElement<DIM> (coefs[i], fac); }

    const EvaluateDShapeTrans Addr (int i) const
    { return EvaluateDShapeTrans (coefs+i, fac); }
  };









  /**
     Base-element for template polymorphism.
     Barton and Nackman Trick for elements with static CalcShape method
  */

  template <class FEL, ELEMENT_TYPE ET, int NDOF, int ORDER>
  class T_ScalarFiniteElement : public ScalarFiniteElement<ET_trait<ET>::DIM>
  {

  public:
    
  protected:
    enum { DIM = ET_trait<ET>::DIM };
    
    T_ScalarFiniteElement ()
      : ScalarFiniteElement<DIM> (ET, NDOF, ORDER) { ; }

    virtual ~T_ScalarFiniteElement() { ; }

  public:

    virtual void CalcShape (const IntegrationPoint & ip, 
			    FlatVector<> shape) const
    {
      double pt[DIM];
      for (int i = 0; i < DIM; i++) pt[i] = ip(i);
      FEL::T_CalcShape (pt, shape); 
    }

    virtual double
    Evaluate (const IntegrationPoint & ip, FlatVector<double> x) const
    {
      double pt[DIM];
      for (int i = 0; i < DIM; i++) pt[i] = ip(i);

      double sum = 0.0;
      EvaluateShape eval(x, &sum); 

      FEL::T_CalcShape (pt, eval); 
      return sum;
    }  


    static void CalcShapeStat (const IntegrationPoint & ip, 
                               FlatVector<> shape)
    {
      double pt[DIM];
      for (int i = 0; i < DIM; i++) pt[i] = ip(i);
      FEL::T_CalcShape (pt, shape); 
    }
    
    virtual void CalcDShape (const IntegrationPoint & ip, 
			     FlatMatrixFixWidth<DIM> dshape) const
    {
      AutoDiff<DIM> adp[DIM];
      for (int i = 0; i < DIM; i++)
        adp[i] = AutoDiff<DIM> (ip(i), i);
      
      DShapeAssign<DIM> ds(dshape); 
      FEL::T_CalcShape (adp, ds);
    }

    static void CalcDShapeStat (const IntegrationPoint & ip, 
				FlatMatrixFixWidth<DIM> dshape)
    {
      AutoDiff<DIM> adp[DIM];
      for (int i = 0; i < DIM; i++)
        adp[i] = AutoDiff<DIM> (ip(i), i);
      
      DShapeAssign<DIM> ds(dshape); 
      FEL::T_CalcShape (adp, ds);
    }

    virtual void 
    CalcMappedDShape (const SpecificIntegrationPoint<DIM,DIM> & sip, 
                      FlatMatrixFixWidth<DIM> dshape) const
    {
      AutoDiff<DIM> adp[DIM];
      
      for (int i = 0; i < DIM; i++)
        adp[i].Value() = sip.IP()(i);
      
      for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++)
          adp[i].DValue(j) = sip.GetJacobianInverse()(i,j);
      
      DShapeAssign<DIM> ds(dshape); 
      FEL::T_CalcShape (adp, ds);
    }
  };




























  /**
     Base-element for template polymorphism.
     Barton and Nackman Trick for elements with non-static CalcShape method
  */

  template <class FEL, ELEMENT_TYPE ET>
  class NGS_DLL_HEADER T_ScalarFiniteElement2 : virtual public ScalarFiniteElement<ET_trait<ET>::DIM>
  {
  public:
    enum { DIM = ET_trait<ET>::DIM };
    using ScalarFiniteElement<DIM>::eltype;

    T_ScalarFiniteElement2 () { eltype = ET; }

    virtual void CalcShape (const IntegrationPoint & ip, 
			    FlatVector<> shape) const;

    virtual double Evaluate (const IntegrationPoint & ip, FlatVector<double> x) const;
    
    virtual void Evaluate (const IntegrationRule & ir, FlatVector<double> coefs, FlatVector<double> vals) const;

    virtual void EvaluateTrans (const IntegrationRule & ir, FlatVector<> vals, FlatVector<double> coefs) const;

    virtual void EvaluateGrad (const IntegrationRule & ir, FlatVector<double> coefs, FlatMatrixFixWidth<DIM> vals) const;

    virtual void EvaluateGradTrans (const IntegrationRule & ir, FlatMatrixFixWidth<DIM> vals, FlatVector<double> coefs) const;

    virtual void CalcDShape (const IntegrationPoint & ip, 
			     FlatMatrixFixWidth<DIM> dshape) const;

    virtual void 
    CalcMappedDShape (const SpecificIntegrationPoint<DIM,DIM> & sip, 
                      FlatMatrixFixWidth<DIM> dshape) const;
  };



}


#endif
