#ifndef FILE_INTRULE
#define FILE_INTRULE

/*********************************************************************/
/* File:   intrule.hpp                                               */
/* Author: Joachim Schoeberl                                         */
/* Date:   25. Mar. 2000                                             */
/*********************************************************************/


/// An integration point 
class IntegrationPoint
{
private:
  /// number within intergration Rule
  int nr; 
  /// enumerates ALL intergration points for element type
  int glob_nr; 
  /// coordinates (empty values for 1D and 2D)
  double pi[3];
  /// weight of integration point
  double weight;
public:
  ///
  bool precomputed_geometry;
public:
  ///
  IntegrationPoint & operator=(const IntegrationPoint & aip)
  {
    glob_nr = aip.IPNr();
    nr = aip.Nr();
    pi[0] = aip(0);
    pi[1] = aip(1);
    pi[2] = aip(2);
    weight = aip.Weight();
    precomputed_geometry = aip.precomputed_geometry;
    return *this;
  }

  ///
  IntegrationPoint (const double api[3], double aw)
  {
    glob_nr = -1;
    nr = -1;
    pi[0] = api[0];
    pi[1] = api[1];
    pi[2] = api[2];
    weight = aw;
    precomputed_geometry = 0;
  }

  ///
  IntegrationPoint (double p1 = 0, double p2 = 0, double p3 = 0, double aw = 0)
  {
    glob_nr = -1;
    nr = -1;
    pi[0] = p1;
    pi[1] = p2;
    pi[2] = p3;
    weight = aw;
    precomputed_geometry = 0;
  }

  ///
  IntegrationPoint (const FlatVector<double> & ap, double aw)
  {
    glob_nr = -1;
    nr = -1;
    pi[0] = (ap.Size() >= 1) ? ap(0) : 0;
    pi[1] = (ap.Size() >= 2) ? ap(1) : 0;
    pi[2] = (ap.Size() >= 3) ? ap(2) : 0;
    weight = aw;
    precomputed_geometry = 0;
  }


  template <int D>
  IntegrationPoint (const Vec<D> & ap, double aw = -1)
  {
    glob_nr = -1;
    nr = -1;
    for (int j = 0; j < D; j++)
      pi[j] = ap(j);
    weight = aw;
    precomputed_geometry = 0;
  }

  ///
  IntegrationPoint (const IntegrationPoint & aip)
  { *this = aip; }

  ///
  void SetNr (int anr) { nr = anr; }
  ///
  void SetGlobNr (int aipnr) { glob_nr = aipnr; }
  ///
  const double * Point () const { return pi; }
  ///
  int Size() const { return 3; }
  ///
  const double & operator() (int i) const { return pi[i]; }
  ///
  double & operator() (int i) { return pi[i]; }
  ///
  double Weight () const { return weight; }
  ///
  void SetWeight (double w) { weight = w; }

  /// number within integration rule
  int Nr () const { return nr; }

  /// global number of ip
  int IPNr () const { return glob_nr; }

  ///
  friend ostream & operator<< (ostream & ost, const IntegrationPoint & ip);
};



class ElementTransformation;

/**
   Base class for SpecificIntegrationPoint.
   A specific integration point is the mapped point, and stores
   point coordinates, the Jacobimatrix, and the determinant of the Jacobimatrix.
*/
class BaseSpecificIntegrationPoint
{
protected:
  /// IP on the reference element
  const IntegrationPoint & ip;
  /// 
  const ElementTransformation & eltrans;

public:
  ///
  BaseSpecificIntegrationPoint (const IntegrationPoint & aip,
				const ElementTransformation & aeltrans)
    : ip(aip), eltrans(aeltrans)  { ; }

  ///
  const IntegrationPoint & IP () const { return ip; }
  ///
  const ElementTransformation & GetTransformation () const { return eltrans; }
  ///
  int GetIPNr() const { return ip.Nr(); }
};


template <int R, typename SCAL = double>
class DimSpecificIntegrationPoint : public BaseSpecificIntegrationPoint
{
protected:
  ///
  Vec<R,SCAL> point;
public:
  ///
  DimSpecificIntegrationPoint (const IntegrationPoint & aip,
			       const ElementTransformation & aeltrans)
    : BaseSpecificIntegrationPoint (aip, aeltrans)
  { ; }
  ///
  const Vec<R,SCAL> GetPoint () const { return point; }
  ///
  SCAL operator() (int i) const 
  { return point(i); }
};


/// ip, dimension source, dimension range
template <int DIMS = 2, int DIMR = 2, typename SCAL = double> 
class SpecificIntegrationPoint : public DimSpecificIntegrationPoint<DIMR,SCAL>
{
private:
  /// Jacobi matrix
  Mat<DIMR,DIMS,SCAL> dxdxi;
  /// (pseudo)inverse of Jacobi matrix
  Mat<DIMS,DIMR,SCAL> dxidx;
  /// Jacobian 
  SCAL det;
  /// for boundary points
  Vec<DIMR,SCAL> normalvec;
  Vec<DIMR,SCAL> tangentialvec;
  
 
public:
  typedef SCAL TSCAL;
  ///
  SpecificIntegrationPoint (const IntegrationPoint & aip,
			    const ElementTransformation & aeltrans,
			    LocalHeap & lh);
  ///
  SpecificIntegrationPoint (const IntegrationPoint & aip,
			    const ElementTransformation & aeltrans,
			    const Vec<DIMR, SCAL> & ax,
			    const Mat<DIMR, DIMS, SCAL> & adxdxi, 
			    LocalHeap & lh)
    : DimSpecificIntegrationPoint<DIMR,SCAL> (aip, aeltrans)
  {
    this->point = ax;
    dxdxi = adxdxi;
      
    if (DIMS == DIMR)
      {
	det = Det (dxdxi);
	dxidx = Inv (dxdxi);
      }
    else
      {
	if (DIMR == 3)
	  {
            normalvec = Cross (Vec<3,SCAL> (dxdxi.Col(0)),
                               Vec<3,SCAL> (dxdxi.Col(1)));
            det = L2Norm (normalvec);
            normalvec /= det;
	  }
	else
	  {
	    det = sqrt ( sqr (dxdxi(0,0)) + sqr (dxdxi(1,0)));

	    normalvec(0) = -dxdxi(1,0) / det;
	    normalvec(1) = dxdxi(0,0) / det;
	  }
	
	Mat<DIMS,DIMS,SCAL> ata, iata;
	
	ata = Trans (dxdxi) * dxdxi;
	iata = Inv (ata);
	dxidx = iata * Trans (dxdxi);
      }
  }
  
  ///
  const Mat<DIMR,DIMS,SCAL> & GetJacobian() const { return dxdxi; }
  ///
  SCAL GetJacobiDet() const { return det; }
  ///
  const Mat<DIMS,DIMR,SCAL> & GetJacobianInverse() const { return dxidx; }
  ///
  const Vec<DIMR,SCAL> GetNV () const { return normalvec; }
  /// 
  void SetNV ( const Vec<DIMR,SCAL> & vec) { normalvec = vec; }
  ///
  void SetTV ( const Vec<DIMR,SCAL> & vec) { tangentialvec = vec; }
  ///
  const Vec<DIMR,SCAL> GetTV () const { return tangentialvec; }
  ///
  int IsBoundary () const { return DIMS != DIMR; }

  ///
  void CalcHesse (Mat<2> & ddx1, Mat<2> & ddx2) const;
  void CalcHesse (Mat<2> & ddx1, Mat<2> & ddx2, Mat<2> & ddx3) const; 
  void CalcHesse (Mat<3> & ddx1, Mat<3> & ddx2, Mat<3> & ddx3) const;
};


template <int DIMS, int DIMR, typename SCAL> 
inline ostream & operator<< (ostream & ost, const SpecificIntegrationPoint<DIMS,DIMR,SCAL> & sip)
{
  ost << sip.GetPoint() << ", dxdxi = " << sip.GetJacobian();
  return ost;
}





/**
   An integration rule.
   Contains array of integration points
*/
class IntegrationRule
{
  ///
  ARRAY<IntegrationPoint*> ipts;
public:
  ///
  IntegrationRule ();
  ///
  IntegrationRule (int nips);
  ///
  ~IntegrationRule ();
  ///
  void AddIntegrationPoint (IntegrationPoint * ip);

  /// number of integration points
  int GetNIP() const { return ipts.Size(); }

  /// returns i-th integration point
  const IntegrationPoint & operator[] (int i) const
  {
#ifdef CHECK_RANGE
    if (i < 0 || i >= ipts.Size())
      {
	stringstream st;
	st << "Integration Point out of range " << i << endl;
	throw Exception (st.str());
      }
#endif
    return *ipts[i]; 
  }


  // old style, should be replaced by []
  const IntegrationPoint & GetIP(int i) const
  {
#ifdef CHECK_RANGE
    if (i < 0 || i >= ipts.Size())
      {
	stringstream st;
	st << "Integration Point out of range " << i << endl;
	throw Exception (st.str());
      }
#endif
    return *ipts[i]; 
  }
};

inline ostream & operator<< (ostream & ost, const IntegrationRule & ir)
{
  for (int i = 0; i < ir.GetNIP(); i++)
    ost << ir[i] << endl;
  return ost;
}




class FiniteElement;


#ifdef OLD

class TensorProductIntegrationRule : public IntegrationRule
{
  bool useho;
  ELEMENT_TYPE type;
  const IntegrationRule & seg_rule;
  int n;
  unsigned int nx;
  int vnums[8];
  int sort[8], isort[8];
  Vec<3> vertices[8];
public:
  TensorProductIntegrationRule (const FiniteElement & el, const IntegrationRule & aseg_rule);
  TensorProductIntegrationRule (ELEMENT_TYPE atype, const int * avnums, const IntegrationRule & aseg_rule);

  unsigned int GetNIP() const { return n; }
  unsigned int GetNIPX() const { return nx; }
  unsigned int GetNIPY() const { return nx; }
  unsigned int GetNIPZ() const { return nx; }
  
  IntegrationPoint GetIP(int i) const;
  IntegrationPoint GetIP(int ix, int iy, int iz) const;
  void GetIP_DTet_DHex(int i, Mat<3> & dtet_dhex) const;
  void GetIP_DTrig_DQuad(int i, Mat<2> & dtrig_dquad) const;

  double GetIPX (int ix) const { return seg_rule[ix](0); }
  double GetIPY (int iy) const { return seg_rule[iy](0); }
  double GetIPZ (int iz) const { return seg_rule[iz](0); }

  double GetWeight (int i)
  {
    switch (type)
      {
      case ET_TET:
	{
	  int ix = i % nx;
	  i /= nx;
	  int iy = i % nx;
	  i /= nx;
	  int iz = i;
	  
	  double eta = seg_rule[iy](0);
	  double zeta = seg_rule[iz](0);
	  
	  return seg_rule[ix].Weight() * (1-eta)*(1-zeta) *
	    seg_rule[iy].Weight() * (1-zeta) *
	    seg_rule[iz].Weight();
	}
      case ET_TRIG:
	{
	  int ix = i % nx;
	  i /= nx;
	  int iy = i;
	  
	  double eta = seg_rule[iy](0);
	  return seg_rule[ix].Weight() * (1-eta)* seg_rule[iy].Weight();
	}
      }
  }
	


  void GetWeights (FlatArray<double> weights);



  IntegrationPoint operator[] (int i) const
  {
    return GetIP(i);
  }
};
#endif




template <int D>
class IntegrationRuleTP
{
  const IntegrationRule *irx, *iry, *irz;
  // int sort[8];

  ArrayMem<Vec<D>, 100> xi;
  ArrayMem<double, 100> weight;
  ArrayMem<Vec<D>, 100> x;
  ArrayMem<Mat<D,D>, 100> dxdxi;
  ArrayMem<Mat<D,D>, 100> dxdxi_duffy;

public:
  IntegrationRuleTP (const ElementTransformation & eltrans,
                     int order, bool compute_mapping, LocalHeap & lh);

  // tensor product rule for a facet
  IntegrationRuleTP (ELEMENT_TYPE eltype, FlatArray<int> sort, 
                     NODE_TYPE nt, int nodenr, int order, LocalHeap & lh);

  /*
  IntegrationRuleTP (ElementType & etype, int classnr, 
                     int order, LocalHeap & lh);
  */

  const IntegrationRule & GetIRX() const { return *irx; }
  const IntegrationRule & GetIRY() const { return *iry; }
  const IntegrationRule & GetIRZ() const { return *irz; }

  int GetNIP() const { return xi.Size(); }
  double GetWeight (int i) const { return weight[i]; }
  const Vec<D> GetXi(int i) const { return xi[i]; }
  const Vec<D> GetPoint(int i) const { return x[i]; }
  const Mat<D,D> & GetJacobian(int i) const { return dxdxi[i]; }
  const Mat<D,D> & GetDuffyJacobian(int i) const { return dxdxi_duffy[i]; }
  IntegrationPoint operator[] (int i) const
  { 
    if (D == 2)
      return IntegrationPoint(xi[i](0), xi[i](1), 0, weight[i]); 
    else
      return IntegrationPoint(xi[i](0), xi[i](1), xi[i](2), weight[i]); 
  }
};








/** 
   Integration Rules.
   A global class maintaining integration rules. If a rule of specific
   order is requested for the first time, than the rule is generated.
*/
class IntegrationRules
{
  ///
  ARRAY<IntegrationRule*> segmentrules;
  ///
  IntegrationRule segmentlumping;
  ///
  ARRAY<IntegrationPoint*> segmentpoints;
  ///
  ARRAY<IntegrationRule*> jacobirules10;
  ///
  ARRAY<IntegrationRule*> jacobirules20;
  ///
  ARRAY<IntegrationRule*> trigrules;
  ///
  ARRAY<IntegrationRule*> trignodalrules;
  ///
  IntegrationRule triglumping;
  ///
  IntegrationRule triglumping2;
  ///
  ARRAY<IntegrationPoint*> trigpoints;

  ///
  ARRAY<IntegrationRule*> quadrules;
  ///
  IntegrationRule quadlumping;
  ///
  ARRAY<IntegrationPoint*> quadpoints;

  ///
  ARRAY<IntegrationRule*> tetrules;
  ///
  IntegrationRule tetlumping;
  ///
  ARRAY<IntegrationRule*> tetnodalrules;
  ///
  ARRAY<IntegrationPoint*> tetpoints;

  ///
  ARRAY<IntegrationRule*> prismrules;
  ///
  IntegrationRule prismlumping;
  ///
  IntegrationRule prismfacemidpoint;
  ///
  ARRAY<IntegrationPoint*> prismpoints;

  ///
  ARRAY<IntegrationRule*> pyramidrules;
  ///
  IntegrationRule pyramidlumping;
  ///
  ARRAY<IntegrationPoint*> pyramidpoints;

  ///
  ARRAY<IntegrationRule*> hexrules;
  ///
  ARRAY<IntegrationPoint*> hexpoints;

public:
  ///
  IntegrationRules ();
  ///
  ~IntegrationRules ();
  /// returns the integration rule for the element type and integration order
  const IntegrationRule & SelectIntegrationRule (ELEMENT_TYPE eltyp, int order) const;
  ///
  const IntegrationRule & SelectLumpingIntegrationRule (ELEMENT_TYPE eltyp) const;
  ///
  const IntegrationRule & SelectNodalIntegrationRule (ELEMENT_TYPE eltyp, int order) const;
  ///
  const IntegrationRule & SelectIntegrationRuleJacobi10 (int order) const;
  ///
  const IntegrationRule & SelectIntegrationRuleJacobi20 (int order) const;
  ///
  const IntegrationRule & PrismFaceMidPoint () const
  { return prismfacemidpoint; }
  ///
  const ARRAY<IntegrationPoint*> & GetIntegrationPoints (ELEMENT_TYPE eltyp) const;
  ///
  const IntegrationRule & GenerateIntegrationRule (ELEMENT_TYPE eltyp, int order);

  const IntegrationRule & GenerateIntegrationRuleJacobi10 (int order);
  const IntegrationRule & GenerateIntegrationRuleJacobi20 (int order);
};


/**
   Computes Gaussean integration.
   Integration rule on (0,1), contains n points,
   Exact for polynomials up to order 2n-1
*/ 
extern void ComputeGaussRule (int n, 
			      ARRAY<double> & xi, 
			      ARRAY<double> & wi);




/**
   Computes Gauss-Jacobi integration rule.
   Integration rule on (0,1), contains n points.
   Assumes that the polynomial has alpha roots in 1, and beta roots in 0.
   Exact for polynomials up to order 2n-1 + alpha + beta
*/ 
extern void ComputeGaussJacobiRule (int n, 
				    ARRAY<double> & xi, 
				    ARRAY<double> & wi,
				    double alf,
				    double bet);


/**
   Computes Gauss-Hermite integration rule.
   Integration rule on R, contains n points.
   Exact for exp{-x*x} * p(x),  with polynomials p(x) up to order 2n-1
*/ 
extern void ComputeHermiteRule (int n, 
                                ARRAY<double> & x,
                                ARRAY<double> & w);
  



/// Get a reference to the integration-rules container
extern const IntegrationRules & GetIntegrationRules ();

extern const IntegrationRule & SelectIntegrationRule (ELEMENT_TYPE eltype, int order);
extern const IntegrationRule & SelectIntegrationRuleJacobi10 (int order);
extern const IntegrationRule & SelectIntegrationRuleJacobi20 (int order);


// transformation of (d-1) dimensional integration points on facets to 
// d-dimensional point in volumes
class Facet2ElementTrafo
{
protected:
  mutable IntegrationPoint elpoint;  
  ELEMENT_TYPE eltype;
  const POINT3D * points;
  const EDGE * edges;
  const FACE * faces;
  EDGE hedges[4];
  FACE hfaces[4];
public:
  Facet2ElementTrafo(ELEMENT_TYPE aeltype) : eltype(aeltype) 
  {
    points = ElementTopology::GetVertices (eltype);
    edges = ElementTopology::GetEdges (eltype);
    faces = ElementTopology::GetFaces (eltype);
  }
  
  Facet2ElementTrafo(ELEMENT_TYPE aeltype, FlatArray<int> & vnums) : eltype(aeltype) 
  {
    points = ElementTopology::GetVertices (eltype);
    edges = ElementTopology::GetEdges (eltype);
    faces = ElementTopology::GetFaces (eltype);

    if (eltype == ET_TRIG)
      {
        for (int i = 0; i < 3; i++)
          {
            hedges[i][0] = edges[i][0];
            hedges[i][1] = edges[i][1];
            if (vnums[hedges[i][0]] > vnums[hedges[i][1]])
              swap (hedges[i][0], hedges[i][1]);
          }
        edges = &hedges[0];
      }

    if (eltype == ET_QUAD)
      {
        for (int i = 0; i < 4; i++)
          {
            hedges[i][0] = edges[i][0];
            hedges[i][1] = edges[i][1];
            if (vnums[hedges[i][0]] > vnums[hedges[i][1]])
              swap (hedges[i][0], hedges[i][1]);
          }
        edges = &hedges[0];
      }

    if (eltype == ET_TET)
      {
        for (int i = 0; i < 4; i++)
          {
            hfaces[i][0] = faces[i][0];
            hfaces[i][1] = faces[i][1];
            hfaces[i][2] = faces[i][2];
            if (vnums[hfaces[i][0]] > vnums[hfaces[i][1]]) swap (hfaces[i][0], hfaces[i][1]);
            if (vnums[hfaces[i][1]] > vnums[hfaces[i][2]]) swap (hfaces[i][1], hfaces[i][2]);
            if (vnums[hfaces[i][0]] > vnums[hfaces[i][1]]) swap (hfaces[i][0], hfaces[i][1]);
          }
        faces = &hfaces[0];
      }


  }


    void operator()(int fnr, const IntegrationPoint &ipfac, IntegrationPoint & ipvol) const 
    {
      ELEMENT_TYPE facettype;
      switch (eltype) 
      {
        case ET_TRIG:
        case ET_QUAD:
          facettype = ET_SEGM;
          break;
        case ET_TET:
          facettype = ET_TRIG;
          break;
        case ET_HEX:
          facettype = ET_QUAD;
          break;
        case ET_PRISM:
          if (fnr < 2) facettype = ET_TRIG;
          else facettype = ET_QUAD;
          break;
        case ET_PYRAMID:
          if (fnr < 4) facettype = ET_TRIG;
          else facettype = ET_QUAD;
          break;
        default:
          cerr << "undefined element type in Facet2ElementTrafo()" << endl;
          exit(0);
      }
      
      switch (facettype)
      {
        case ET_SEGM:
        {
          const POINT3D & p1 = points[edges[fnr][0]];
          const POINT3D & p2 = points[edges[fnr][1]];
          ipvol(0) = p1[0] + (ipfac(0))*(p2[0]-p1[0]);
          ipvol(1) = p1[1] + (ipfac(0))*(p2[1]-p1[1]);
          ipvol(2) = p1[2] + (ipfac(0))*(p2[2]-p1[2]);
          break;
        }
        case ET_TRIG:
        {
          const POINT3D & p0 = points[faces[fnr][0]];
          const POINT3D & p1 = points[faces[fnr][1]];
          const POINT3D & p2 = points[faces[fnr][2]];
          ipvol(0) = p2[0] + ipfac(0)*(p0[0]-p2[0]) + ipfac(1)*(p1[0]-p2[0]);
          ipvol(1) = p2[1] + ipfac(0)*(p0[1]-p2[1]) + ipfac(1)*(p1[1]-p2[1]);
          ipvol(2) = p2[2] + ipfac(0)*(p0[2]-p2[2]) + ipfac(1)*(p1[2]-p2[2]);
          break;
        }
        case ET_QUAD:
        {
          const POINT3D & p0 = points[faces[fnr][0]];
          const POINT3D & p1 = points[faces[fnr][1]];
          const POINT3D & p2 = points[faces[fnr][3]];
          ipvol(0) = p0[0] + ipfac(0)*(p1[0]-p0[0]) + ipfac(1)*(p2[0]-p0[0]);
          ipvol(1) = p0[1] + ipfac(0)*(p1[1]-p0[1]) + ipfac(1)*(p2[1]-p0[1]);
          ipvol(2) = p0[2] + ipfac(0)*(p1[2]-p0[2]) + ipfac(1)*(p2[2]-p0[2]);
          break;
        }
      } 
/*      cerr << "*** mapping integrationpoint for element " << eltype << " and facel " << fnr << " of type " << facettype << endl;
      cerr << "  * ipfac = " << ipfac;
      cerr << "  * ipvol = " << ipvol;*/
    }
    const IntegrationPoint & operator()(int fnr, const IntegrationPoint &ip1d) const 
    {
      operator()(fnr, ip1d, elpoint);
      return elpoint;
    }
};

#endif
