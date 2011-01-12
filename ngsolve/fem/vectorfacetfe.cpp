/*********************************************************************/
/* File:   vectorfacetfe.cpp                                         */
/* Author: A. Sinwel, (J. Schoeberl)                                 */
/* Date:   2008                                                      */
/*********************************************************************/


#include <fem.hpp>

namespace ngfem {


  using namespace ngfem;

  VectorFacetFacetFiniteElement ::
  VectorFacetFacetFiniteElement (int dim, ELEMENT_TYPE aeltype) :
    FiniteElement (dim, aeltype, -1, -1 )
  {
    for (int i=0; i<8; i++)
      vnums[i] = -1; 
    order_inner = INT<2> (-1, -1);
  }

  void VectorFacetFacetFiniteElement ::
  SetVertexNumbers (FlatArray<int> & avnums)
  {
    for ( int i = 0; i < avnums.Size(); i++ )
      vnums[i] = avnums[i];
  }

  void VectorFacetFacetFiniteElement ::
  SetOrder (int aorder)
  {
    order = aorder;
    order_inner = INT<2> (aorder, aorder);
    ComputeNDof();
  }
  
  void VectorFacetFacetFiniteElement ::
  SetOrder (INT<2> oi)
  {
    order = max2 (oi[0], oi[1]);
    order_inner = oi;
    ComputeNDof();
  }


  /* **************************** Facet Segm ********************************* */

  VectorFacetFacetSegm :: VectorFacetFacetSegm (int aorder) :
    VectorFacetFacetFiniteElement (1, ET_SEGM )
  {
    order = aorder;
    order_inner = INT<2> (aorder, aorder);
    ComputeNDof();
  }

  void VectorFacetFacetSegm :: ComputeNDof()
  {
    order = order_inner[0];
    ndof = order+1;
  }


  void VectorFacetFacetSegm :: CalcShape (const IntegrationPoint & ip, 
					  FlatMatrix<> shape) const
  {
    AutoDiff<1> x (ip(0),0);
    ArrayMem<double, 10>  polx(order_inner[0]+1);
    // orient
    if ( vnums[0] > vnums[1])
      x = 1-x;
    
    int ii = 0;

    LegendrePolynomial (order_inner[0], 2*x.Value()-1, polx);
    for ( int i = 0; i <= order_inner[0]; i++ )
      shape(ii++,0) = 2 * polx[i] * x.DValue(0);
  }


  /* **************************** Facet Trig ********************************* */

  VectorFacetFacetTrig :: VectorFacetFacetTrig (int aorder) :
    VectorFacetFacetFiniteElement (2, ET_TRIG)
  {
    order = aorder;
    order_inner = INT<2> ( aorder, aorder);
    ComputeNDof();
  }

  void VectorFacetFacetTrig :: ComputeNDof()
  {
    order = order_inner[0];
    int p = order_inner[0];
    ndof = (p+1)*(p+2);
  }

  void VectorFacetFacetTrig :: CalcShape (const IntegrationPoint & ip, 
					  FlatMatrix<> shape) const
  {
    AutoDiff<2> x (ip(0), 0);
    AutoDiff<2> y (ip(1), 1);

    int p = order_inner[0];
    ArrayMem< double, 10> polx(p+1), poly(p+1);
    int ii = 0;

    AutoDiff<2> lami[3] = { x, y, 1-x-y };
    int fav[3] = { 0, 1, 2};
    if(vnums[fav[0]] > vnums[fav[1]]) swap(fav[0],fav[1]); 
    if(vnums[fav[1]] > vnums[fav[2]]) swap(fav[1],fav[2]);
    if(vnums[fav[0]] > vnums[fav[1]]) swap(fav[0],fav[1]); 	


    double xi = lami[fav[0]].Value();
    double eta = lami[fav[1]].Value();

    AutoDiff<2> adxi  = lami[fav[0]]-lami[fav[2]];
    AutoDiff<2> adeta = lami[fav[1]]-lami[fav[2]];

    ScaledLegendrePolynomial (p, 2*xi+eta-1, 1-eta, polx);

    Matrix<> polsy(p+1, p+1);
    DubinerJacobiPolynomials<1,0> (p, 2*eta-1, polsy);

    // LegendrePolynomial (p, 2*eta-1, poly);

    // *testout << "surface trig, orderinner = " << order_inner[0] << endl;

    for (int i = 0; i <= order_inner[0]; i++)
      for (int j = 0; j <= order_inner[0]-i; j++)
	{
	  // double val = polx[i] * poly[j];
	  double val = polx[i] * polsy(i,j);
	  shape(ii,0) = val * adxi.DValue(0);  // lami[fav[0]].DValue(0);
	  shape(ii,1) = val * adxi.DValue(1);  // lami[fav[0]].DValue(1);
	  ii++;
	  shape(ii,0) = val * adeta.DValue(0);  // lami[fav[1]].DValue(0);
	  shape(ii,1) = val * adeta.DValue(1);  // lami[fav[1]].DValue(1);
	  ii++;
	}
    // *testout << "surface trig " << endl << shape << endl;
  }



  /* **************************** Facet Quad ********************************* */

  VectorFacetFacetQuad :: VectorFacetFacetQuad (int aorder) :
    VectorFacetFacetFiniteElement (2, ET_QUAD)
  {
    order = aorder;
    order_inner = INT<2> (aorder, aorder);
    ComputeNDof();
  }

  void VectorFacetFacetQuad :: ComputeNDof()
  {
    order = max2( order_inner[0], order_inner[2] );
    ndof = 2 * (order_inner[0]+1) * (order_inner[1]+1);
  }

  /// compute shape
  void VectorFacetFacetQuad :: CalcShape (const IntegrationPoint & ip, 
					  FlatMatrix<> shape) const
  {
    AutoDiff<2> x (ip(0), 0);
    AutoDiff<2> y (ip(1), 1);

    // orient: copied from h1
    AutoDiff<2> sigma[4] = {(1-x)+(1-y),x+(1-y),x+y,(1-x)+y};  
    int fmax = 0; 
    for (int j = 1; j < 4; j++)
      if (vnums[j] > vnums[fmax]) fmax = j;
    int f1 = (fmax+3)%4; 
    int f2 = (fmax+1)%4; 
    if(vnums[f2] > vnums[f1]) swap(f1,f2);  // fmax > f1 > f2; 

    AutoDiff<2> xi  = sigma[fmax] - sigma[f1]; 
    AutoDiff<2> eta = sigma[fmax] - sigma[f2]; 
    
    int ii = 0;
    
    int n = max(order_inner[0],order_inner[1]);
    ArrayMem<double, 20> polx(n+1), poly(n+1);

    LegendrePolynomial (n, xi.Value(), polx);
    LegendrePolynomial (n, eta.Value(), poly);
    
    for (int i = 0; i <= order_inner[0]; i++)
      for (int j = 0; j <= order_inner[1]; j++)
	{
	  double val = polx[i] * poly[j];
	  shape(ii,0) = val * xi.DValue(0);
	  shape(ii,1) = val * xi.DValue(1);
	  ii++;
	  shape(ii,0) = val * eta.DValue(0);
	  shape(ii,1) = val * eta.DValue(1);
	  ii++;
	}
  }


  ///+++++++++++++++++++++++++

  template <int D>
  VectorFacetVolumeFiniteElement<D> :: 
  VectorFacetVolumeFiniteElement (ELEMENT_TYPE aeltype) :
    HCurlFiniteElement<D>(aeltype, -1, -1)
  {
    for ( int i = 0; i < 8; i++ )
      vnums[i] = -1;
    for ( int i = 0; i < 6; i++ )
      facet_order[i] = INT<2> (-1, -1);
    for ( int i = 0; i < 7; i++ )
      first_facet_dof[i] = 0;
    /*
    switch (eltype)
      {
      case ET_TRIG:
	nv=3; nf=3; break; 
      case ET_QUAD:
	nv=4; nf=4; break;
      case ET_TET:
	nv=4; nf=4; break;
      case ET_PRISM:
	nv=6; nf=5; break;
      case ET_HEX:
	nv=8; nf=6; break;
      default:
	cerr << "*** error in FacetVolumeFiniteElement::FacetVolumeFiniteElement(int, ELEMENT_TYPE):unknown element type" << endl;
	exit(0);
      }
    */
  }

  /// degrees of freedom sitting inside the element, used for static condensation
  
  template <int D>
  void VectorFacetVolumeFiniteElement<D>::
  GetInternalDofs (Array<int> & idofs) const
  {
    idofs.SetSize(0);
    if (highest_order_dc)
      {
	if (D==2)
	  {
	    for (int i=0; i<ElementTopology::GetNFacets(eltype); i++)
	      idofs.Append(first_facet_dof[i+1]-1);   
	  }
	else
	  {
	    for (int i=0; i<ElementTopology::GetNFacets(eltype); i++)
	      {
		int pos = first_facet_dof[i]-2;
		int fac = 4 - ElementTopology::GetNVertices(ElementTopology::GetFaceType(eltype,i));
		for (int k = 0; k <= facet_order[i][0]; k++){
		  pos += 2*(facet_order[i][0]+1-fac*k);
		  idofs.Append(pos);
		  idofs.Append(pos+1);
		}
	      }
	  }//end if Dimension
      }
  }


  template <int D>
  void VectorFacetVolumeFiniteElement<D> :: 
  SetVertexNumbers (FlatArray<int> & avnums)
  {
    for (int i = 0; i < avnums.Size(); i++)
      vnums[i] = avnums[i];
  }

  template <int D>
  void VectorFacetVolumeFiniteElement<D> :: 
  SetOrder(int ao)
  {
    order = ao;
    for ( int i = 0; i < 6; i++ )
      facet_order[i] = INT<2> (ao, ao);
    ComputeNDof();
  }

  template <int D>
  void VectorFacetVolumeFiniteElement<D> :: 
  SetOrder(FlatArray<INT<2> > & ao)
  {
    for ( int i = 0; i < ao.Size(); i++ )
      {
        order = max3 ( order, ao[i][0], ao[i][1] );
	facet_order[i] = ao[i];
      }
    ComputeNDof();
  }

  template <int D>
  void VectorFacetVolumeFiniteElement<D> :: 
  SetOrder(FlatArray<int> & ao)
  {
    for ( int i = 0; i < ao.Size(); i++ )
      {
        order = max2 ( order, ao[i] );
	facet_order[i] = INT<2> (ao[i], ao[i]);
      }
    ComputeNDof();
  }

  template <int D>
  void VectorFacetVolumeFiniteElement<D> :: 
  GetFacetDofNrs(int afnr, Array<int>& fdnums) const
  {
    fdnums.SetSize(0);
    int first = first_facet_dof[afnr];
    int next = first_facet_dof[afnr+1];
    for ( int i = first; i < next; i++ )
      fdnums.Append(i); 
  }
  
    /*  template <int D>
  void VectorFacetVolumeFiniteElement<D> :: 
  GetVertexNumbers(Array<int>& avnums) const
  {
    cerr << "vectorfacetelement getvertexnumbers not supported" << endl;

    avnums.SetSize(nv);
    for ( int i = 0; i < nv; i++ )
      avnums[i] = vnums[i];
  }
  
  template <int D>
  void VectorFacetVolumeFiniteElement<D> :: 
  GetFacetOrders(Array<INT<2> >& forder) const
  {
    forder.SetSize(6);
    for ( int i = 0; i < 6; i++ )
      forder[i] = facet_order[i];
  }
    */
  

  template class VectorFacetVolumeFiniteElement<2>;
  template class VectorFacetVolumeFiniteElement<3>;






  void VectorFacetVolumeTrig :: 
  CalcShape (const IntegrationPoint & ip, FlatMatrixFixWidth<2> shape) const
  {
    double x=ip(0), y=ip(1);
    int fanr; 

    shape = 0.0;
    if (fabs(y) < 1e-4) 
      {
	fanr = 0;
      }
    else if (fabs(x) < 1e-4)
      {
	fanr = 1;
      }
    else 
      {
	fanr = 2;
      }

    CalcShape ( ip, fanr, shape);
  }

  void VectorFacetVolumeTrig ::
  CalcShape ( const IntegrationPoint & ip, int fanr, FlatMatrixFixWidth<2> shape ) const
  {
    shape = 0.0;
    int first = first_facet_dof[fanr];

    AutoDiff<2> x(ip(0), 0), y(ip(1),1);

    const EDGE * edges = ElementTopology :: GetEdges ( eltype );

    int  fav[2] = {edges[fanr][0], edges[fanr][1] };
    int j1 = 0; 
    int j2 = 1; 
    if(vnums[fav[j1]] > vnums[fav[j2]]) swap(j1,j2); 

    AutoDiff<2> lami[3] = {x, y, 1-x-y};  

    int p = facet_order[fanr][0];
    ArrayMem< double, 10> polx(p+1);
    int ii = first;

    AutoDiff<2> xi = lami[fav[j1]] - lami[fav[j2]];

    LegendrePolynomial (p, xi.Value(), polx);
    for (int i = 0; i <= facet_order[fanr][0]; i++)
      {
	double val = polx[i];
	shape(ii,0) = val * xi.DValue(0);
	shape(ii,1) = val * xi.DValue(1);
	ii++;
      }
  }

  void VectorFacetVolumeTrig ::
  CalcExtraShape ( const IntegrationPoint & ip, int fanr, FlatMatrixFixWidth<2> xshape ) const
  {
    xshape = 0.0;
    // int first = first_facet_dof[fanr];

    AutoDiff<2> x(ip(0), 0), y(ip(1),1);

    const EDGE * edges = ElementTopology :: GetEdges ( eltype );

    int  fav[2] = {edges[fanr][0], edges[fanr][1] };
    int j1 = 0; 
    int j2 = 1; 
    if(vnums[fav[j1]] > vnums[fav[j2]]) swap(j1,j2); 

    AutoDiff<2> lami[3] = {x, y, 1-x-y};  

    int p = facet_order[fanr][0];
    
    ArrayMem< double, 10> polx(p+2);
    // int ii = first;

    AutoDiff<2> xi = lami[fav[j1]] - lami[fav[j2]];

    LegendrePolynomial (p+1, xi.Value(), polx);
    
    double val = polx[p+1];
    xshape(0,0) = val * xi.DValue(0);
    xshape(0,1) = val * xi.DValue(1);
  }


    
  void VectorFacetVolumeTrig :: ComputeNDof()
  {
    ndof = 0;
    for ( int i = 0; i < 3; i++ )
      {
	first_facet_dof[i] = ndof;
	ndof += facet_order[i][0] + 1;
      }
    first_facet_dof[3] = ndof;
  }

  

  //--------------------------------------------------

  void VectorFacetVolumeQuad :: 
  CalcShape (const IntegrationPoint & ip, FlatMatrixFixWidth<2> shape) const
  {
    // topology: points = 0:(0,0), 1:(1,0), 2:(1,1), 3:(0,1)
    //           edges = 0:(0,1), 1:(2,3), 2:(3,0), 4:(1,2)
    IntegrationPoint ip1d;  
    double x=ip(0), y=ip(1);
    int fnr=-1;
    
    Matrix<double> shape1d (order+1, 1);

    shape = 0.0;
    if (fabs(y)<1e-12)
      {
	fnr=0;
      }
    else if (fabs(y)>1-(1e-12))
      {
	fnr=1;
      }
    else if (fabs(x)<1e-12)
      {
	fnr=2;
      }
    else if (fabs(x)>1-(1e-12))
      {
	fnr=3;
      }
    CalcShape(ip, fnr, shape);
  }

  void VectorFacetVolumeQuad ::
  CalcShape ( const IntegrationPoint & ip, int fanr, FlatMatrixFixWidth<2> shape ) const
  {
    shape = 0.0;

    AutoDiff<2> x(ip(0), 0), y(ip(1),1);

    const EDGE * faces = ElementTopology :: GetEdges ( eltype );
    // const POINT3D * points = ElementTopology :: GetVertices (eltype);
    // const ELEMENT_TYPE facettype = ElementTopology :: GetFacetType (eltype, fanr);
    // int nvert = ElementTopology :: GetNVertices (facettype);

    int  fav[2] = {faces[fanr][0], faces[fanr][1] };
    int j1 = 0; 
    int j2 = 1; 
    if(vnums[fav[j1]] > vnums[fav[j2]]) swap(j1,j2);  // fmax > f2 > f1; 

    AutoDiff<2> sigma[4] = {(1-x)+(1-y),x+(1-y),x+y,(1-x)+y};  

    int p = facet_order[fanr][0];
    ArrayMem< double, 10> polx(p+1);
    int ii = first_facet_dof[fanr];

    AutoDiff<2> xi = sigma[fav[j1]] - sigma[fav[j2]];

    // LegendrePolynomial (p, 2*xi.Value()-1, polx);
    LegendrePolynomial (p, xi.Value(), polx);
    for (int i = 0; i <= facet_order[fanr][0]; i++)
      {
	double val = polx[i];
	shape(ii,0) = val * xi.DValue(0);
	shape(ii,1) = val * xi.DValue(1);
	ii++;
      }
  }



  void VectorFacetVolumeQuad ::
  CalcExtraShape ( const IntegrationPoint & ip, int fanr, FlatMatrixFixWidth<2> xshape ) const
  {
    xshape = 0.0;

    AutoDiff<2> x(ip(0), 0), y(ip(1),1);

    const EDGE * faces = ElementTopology :: GetEdges ( eltype );
    // const POINT3D * points = ElementTopology :: GetVertices (eltype);
    // const ELEMENT_TYPE facettype = ElementTopology :: GetFacetType (eltype, fanr);
    // int nvert = ElementTopology :: GetNVertices (facettype);

    int  fav[2] = {faces[fanr][0], faces[fanr][1] };
    int j1 = 0; 
    int j2 = 1; 
    if(vnums[fav[j1]] > vnums[fav[j2]]) swap(j1,j2);  // fmax > f2 > f1; 

    AutoDiff<2> sigma[4] = {(1-x)+(1-y),x+(1-y),x+y,(1-x)+y};  

    int p = facet_order[fanr][0];
    ArrayMem< double, 10> polx(p+2);
    // int ii = first_facet_dof[fanr];

    AutoDiff<2> xi = sigma[fav[j1]] - sigma[fav[j2]];

    // LegendrePolynomial (p, 2*xi.Value()-1, polx);
    LegendrePolynomial (p+1, xi.Value(), polx);
    double val = polx[p+1];
    xshape(0,0) = val * xi.DValue(0);
    xshape(0,1) = val * xi.DValue(1);
  }


  void VectorFacetVolumeQuad :: ComputeNDof()
  {
    ndof = 0;
    for ( int i = 0; i < 4; i++ )
      {
	first_facet_dof[i] = ndof;
	ndof += facet_order[i][0] + 1;
      }
    first_facet_dof[4] = ndof;
  }

  



  void VectorFacetVolumeTet::
  CalcShape (const IntegrationPoint & ip, FlatMatrixFixWidth<3> shape) const
  {
    // topology: points = 0:(1,0,0), 1:(0,1,0), 2:(0,0,1), 3:(0,0,0)
    //           faces = 0:(3,1,2,-1), 1:(3,2,0,-1), 2:(3,0,1,-1), 4:(0,2,1,-1)
    
    double x=ip(0), y=ip(1), z=ip(2);
    int fanr;

    if (fabs(x)<1e-8)
      {
	// (0,0,0)->(1,0); (0,1,0)->(0,1), (0,0,1)->(0,0)
	fanr=0;
      }
    else if (fabs(y)<1e-8)
      {
	// (0,0,0)->(1,0); (0,0,1)->(0,1); (1,0,0)->(0,0)
	fanr=1;
      }
    else if (fabs(z)<1e-8) 
      {
	// (0,0,0)->(1,0); (1,0,0)->(0,1); (0,1,0)->(0,0)
	fanr=2;
      }
    else if (fabs(1-x-y-z)<1e-8)
      {
	// (1,0,0)->(1,0); (0,0,1)->(0,1); (0,1,0)->(0,0)
	fanr=3;
      }
    else
      {
	throw Exception ("VectorFacetVolumeTet: illegal evaluation");
      }

    CalcShape ( ip, fanr, shape );
  }   

  void VectorFacetVolumeTet ::
  CalcShape ( const IntegrationPoint & ip, int fanr, FlatMatrixFixWidth<3> shape ) const
  {
    shape = 0.0;


    AutoDiff<3> x(ip(0), 0), y(ip(1),1), z(ip(2),2);
    AutoDiff<3> lami[4] = { x, y, z, 1-x-y-z };

    const FACE * faces = ElementTopology :: GetFaces ( eltype );


    int fav[3] = {faces[fanr][0], faces[fanr][1], faces[fanr][2]};
    
    if(vnums[fav[0]] > vnums[fav[1]]) swap(fav[0],fav[1]); 
    if(vnums[fav[1]] > vnums[fav[2]]) swap(fav[1],fav[2]);
    if(vnums[fav[0]] > vnums[fav[1]]) swap(fav[0],fav[1]); 	  	
    
    AutoDiff<3> adxi = lami[fav[0]]-lami[fav[2]];
    AutoDiff<3> adeta = lami[fav[1]]-lami[fav[2]];
    double xi = lami[fav[0]].Value();
    double eta = lami[fav[1]].Value();


    int p = facet_order[fanr][0];
    ArrayMem< double, 10> polx(p+1), poly(p+1);
    Matrix<> polsy(p+1, p+1);
    int ii = first_facet_dof[fanr];

    ScaledLegendrePolynomial (p, 2*xi+eta-1, 1-eta, polx);
    DubinerJacobiPolynomials<1,0> (p, 2*eta-1, polsy);

    for (int i = 0; i <= facet_order[fanr][0]; i++)
      for (int j = 0; j <= facet_order[fanr][0]-i; j++)
	{
	  double val = polx[i] * polsy(i, j);
	  shape(ii,0) = val * adxi.DValue(0);
	  shape(ii,1) = val * adxi.DValue(1);
	  shape(ii,2) = val * adxi.DValue(2);
	  ii++;
	  shape(ii,0) = val * adeta.DValue(0);
	  shape(ii,1) = val * adeta.DValue(1);
	  shape(ii,2) = val * adeta.DValue(2);
	  ii++;
	}
  }


  void VectorFacetVolumeTet ::
  CalcExtraShape ( const IntegrationPoint & ip, int fanr, FlatMatrixFixWidth<3> xshape ) const
  {
    xshape = 0.0;


    AutoDiff<3> x(ip(0), 0), y(ip(1),1), z(ip(2),2);
    AutoDiff<3> lami[4] = { x, y, z, 1-x-y-z };

    const FACE * faces = ElementTopology :: GetFaces ( eltype );


    int fav[3] = {faces[fanr][0], faces[fanr][1], faces[fanr][2]};
    
    if(vnums[fav[0]] > vnums[fav[1]]) swap(fav[0],fav[1]); 
    if(vnums[fav[1]] > vnums[fav[2]]) swap(fav[1],fav[2]);
    if(vnums[fav[0]] > vnums[fav[1]]) swap(fav[0],fav[1]); 	  	
    
    AutoDiff<3> adxi = lami[fav[0]]-lami[fav[2]];
    AutoDiff<3> adeta = lami[fav[1]]-lami[fav[2]];
    double xi = lami[fav[0]].Value();
    double eta = lami[fav[1]].Value();


    int p = facet_order[fanr][0];
    ArrayMem< double, 10> polx(p+2), poly(p+2);
    Matrix<> polsy(p+2, p+2);
//     int ii = first_facet_dof[fanr];
    int ii = 0;

    ScaledLegendrePolynomial (p+1, 2*xi+eta-1, 1-eta, polx);
    DubinerJacobiPolynomials<1,0> (p+1, 2*eta-1, polsy);

    for (int i = 0; i <= p+1; i++)
    {
      int j = p+1-i;
      double val = polx[i] * polsy(i, j);
      xshape(ii,0) = val * adxi.DValue(0);
      xshape(ii,1) = val * adxi.DValue(1);
      xshape(ii,2) = val * adxi.DValue(2);
      ii++;
      xshape(ii,0) = val * adeta.DValue(0);
      xshape(ii,1) = val * adeta.DValue(1);
      xshape(ii,2) = val * adeta.DValue(2);
      ii++;
    }

  }





  void VectorFacetVolumeTet :: ComputeNDof() 
  {
    ndof = 0;
    for (int i = 0; i < 4; i++)
      {
	first_facet_dof[i] = ndof;
	ndof += (facet_order[i][0]+1) * (facet_order[i][0]+2);
      }
    first_facet_dof[4] = ndof;
  }




  // -------------------------------------------------------------------------------

  void VectorFacetVolumePrism::
  CalcShape (const IntegrationPoint & ip, FlatMatrixFixWidth<3> shape) const
  {  
    int fanr = -1;
    double x=ip(0), y=ip(1), z=ip(2);
    
    shape = 0.0;

    if (fabs(y)<1e-12) // front
      {
	fanr = 2;
      }
    else if (fabs(x)<1e-12) // left
      {
	fanr = 4;
      }
    else if (fabs(1-x-y)<1e-12) // right
      {
	fanr = 3;
      }
    else if (fabs(z)<1e-12) // botom
      {
	fanr = 0;
      }
    else if (fabs(z)>1-1e-12) // top
      {
	fanr = 1;
      }

    CalcShape ( ip, fanr, shape );
    ;
  } 
   
  void VectorFacetVolumePrism ::
  CalcShape ( const IntegrationPoint & ip, int fanr, FlatMatrixFixWidth<3> shape ) const
  {
    AutoDiff<3> x(ip(0), 0), y(ip(1),1), z(ip(2),2);

    AutoDiff<3> lami[6] = { x, y, 1-x-y, x, y, 1-x-y };
    AutoDiff<3> muz[6]  = { 1-z, 1-z, 1-z, z, z, z }; 

    const FACE * faces = ElementTopology::GetFaces (ET_PRISM); 
    shape = 0.0;

    // trig face shapes
    if (fanr < 2)
      {
	int p = facet_order[fanr][0];

	int fav[3] = {faces[fanr][0], faces[fanr][1], faces[fanr][2]};

	if(vnums[fav[0]] > vnums[fav[1]]) swap(fav[0],fav[1]); 
	if(vnums[fav[1]] > vnums[fav[2]]) swap(fav[1],fav[2]);
	if(vnums[fav[0]] > vnums[fav[1]]) swap(fav[0],fav[1]); 	  	
	
	AutoDiff<3> adxi = lami[fav[0]]-lami[fav[2]];
	AutoDiff<3> adeta = lami[fav[1]]-lami[fav[2]];
	double xi = lami[fav[0]].Value();
	double eta = lami[fav[1]].Value();
	
	ArrayMem< double, 10> polx(p+1), poly(p+1);
	Matrix<> polsy(p+1, p+1);
	
	ScaledLegendrePolynomial (p, 2*xi+eta-1, 1-eta, polx);
	DubinerJacobiPolynomials<1,0> (p, 2*eta-1, polsy);
	
	int ii = first_facet_dof[fanr];
	for (int i = 0; i <= facet_order[fanr][0]; i++)
	  for (int j = 0; j <= facet_order[fanr][0]-i; j++)
	    {
	      double val = polx[i] * polsy(i, j);
	      shape(ii,0) = val * adxi.DValue(0);
	      shape(ii,1) = val * adxi.DValue(1);
	      shape(ii,2) = val * adxi.DValue(2);
	      ii++;
	      shape(ii,0) = val * adeta.DValue(0);
	      shape(ii,1) = val * adeta.DValue(1);
	      shape(ii,2) = val * adeta.DValue(2);
	      ii++;
	    }
      }
    else
      // quad faces
      {
	int p = facet_order[fanr][0];
	 
	int fmax = 0;
	for (int j = 1; j < 4; j++)
	  if (vnums[faces[fanr][j]] > vnums[faces[fanr][fmax]]) fmax = j;

	int fz = 3-fmax; 
	int ftrig = fmax^1; 
	AutoDiff<3> xi = lami[faces[fanr][fmax]]-lami[faces[fanr][ftrig]]; 
	AutoDiff<3> eta = 1-lami[faces[fanr][fmax]]-lami[faces[fanr][ftrig]]; 
	AutoDiff<3> zeta = muz[faces[fanr][fmax]]-muz[faces[fanr][fz]]; 
	
	ArrayMem< double, 10> polx(p+1), poly(p+1);
	LegendrePolynomial (p, xi.Value(), polx);
	LegendrePolynomial (p, zeta.Value(), poly);

	int ii = first_facet_dof[fanr];
	for (int i = 0; i <= p; i++)
	  for (int j = 0; j <= p; j++)
	    {
	      double val = polx[i] * poly[j];
	      shape(ii,0) = val * xi.DValue(0);
	      shape(ii,1) = val * xi.DValue(1);
	      shape(ii,2) = val * xi.DValue(2);
	      ii++;
	      shape(ii,0) = val * zeta.DValue(0);
	      shape(ii,1) = val * zeta.DValue(1);
	      shape(ii,2) = val * zeta.DValue(2);
	      ii++;
	    }	
      }
  }
  /*
  int VectorFacetVolumePrism ::
  GetNExtraShapes ( int fanr) const
  {
    if (fanr < 2) //trig shape
      return 2*(facet_order[facet][0]+2);
    else //quad shape
      return 2*(2*facet_order[facet][0]+3);
  }
  */
  /*
  void VectorFacetVolumePrism ::
  CalcExtraShape ( const IntegrationPoint & ip, int fanr, FlatMatrixFixWidth<3> shape ) const
  {
    AutoDiff<3> x(ip(0), 0), y(ip(1),1), z(ip(2),2);

    AutoDiff<3> lami[6] = { x, y, 1-x-y, x, y, 1-x-y };
    AutoDiff<3> muz[6]  = { 1-z, 1-z, 1-z, z, z, z }; 

    const FACE * faces = ElementTopology::GetFaces (ET_PRISM); 
    xshape = 0.0;

    // trig face shapes
    if (fanr < 2)
      {
	int p = facet_order[fanr][0];

	int fav[3] = {faces[fanr][0], faces[fanr][1], faces[fanr][2]};

	if(vnums[fav[0]] > vnums[fav[1]]) swap(fav[0],fav[1]); 
	if(vnums[fav[1]] > vnums[fav[2]]) swap(fav[1],fav[2]);
	if(vnums[fav[0]] > vnums[fav[1]]) swap(fav[0],fav[1]); 	  	
	
	AutoDiff<3> adxi = lami[fav[0]]-lami[fav[2]];
	AutoDiff<3> adeta = lami[fav[1]]-lami[fav[2]];
	double xi = lami[fav[0]].Value();
	double eta = lami[fav[1]].Value();
	
	ArrayMem< double, 10> polx(p+2), poly(p+2);
	Matrix<> polsy(p+2, p+2);
	
	ScaledLegendrePolynomial (p+1, 2*xi+eta-1, 1-eta, polx);
	DubinerJacobiPolynomials<1,0> (p+1, 2*eta-1, polsy);
	
	int ii = 0;
	for (int i = 0; i <= p+1; i++)
	{
	  int j= p+1-i;
	  double val = polx[i] * polsy(i, j);
	  xshape(ii,0) = val * adxi.DValue(0);
	  xshape(ii,1) = val * adxi.DValue(1);
	  xshape(ii,2) = val * adxi.DValue(2);
	  ii++;
	  xshape(ii,0) = val * adeta.DValue(0);
	  xshape(ii,1) = val * adeta.DValue(1);
	  xshape(ii,2) = val * adeta.DValue(2);
	  ii++;
	}
      }
    else
      // quad faces
      {
	int p = facet_order[fanr][0];
	 
	int fmax = 0;
	for (int j = 1; j < 4; j++)
	  if (vnums[faces[fanr][j]] > vnums[faces[fanr][fmax]]) fmax = j;

	int fz = 3-fmax; 
	int ftrig = fmax^1; 
	AutoDiff<3> xi = lami[faces[fanr][fmax]]-lami[faces[fanr][ftrig]]; 
	AutoDiff<3> eta = 1-lami[faces[fanr][fmax]]-lami[faces[fanr][ftrig]]; 
	AutoDiff<3> zeta = muz[faces[fanr][fmax]]-muz[faces[fanr][fz]]; 
	
	ArrayMem< double, 10> polx(p+2), poly(p+2);
	LegendrePolynomial (p+1, xi.Value(), polx);
	LegendrePolynomial (p+1, zeta.Value(), poly);

	int ii = 0;
	for (int i = 0; i <= p+1; i++)
	{
	  for (int j = (i==p+1)?0:p+1; j <= p+1; j++)
	  {
	    double val = polx[i] * poly[j];
	    shape(ii,0) = val * xi.DValue(0);
	    shape(ii,1) = val * xi.DValue(1);
	    shape(ii,2) = val * xi.DValue(2);
	    ii++;
	    shape(ii,0) = val * zeta.DValue(0);
	    shape(ii,1) = val * zeta.DValue(1);
	    shape(ii,2) = val * zeta.DValue(2);
	    ii++;
	  }
	}	
      }
  }
  */
  
  
  void VectorFacetVolumePrism::ComputeNDof() 
  {
    ndof = 0;
    // triangles
    for (int i=0; i<2; i++)
      {
	first_facet_dof[i] = ndof;
	ndof += ( (facet_order[i][0]+1) * (facet_order[i][0]+2) );
      }
    //quads
    for (int i=2; i<5; i++)
      {
	first_facet_dof[i] = ndof;
	ndof += 2 * (facet_order[i][0]+1) * (facet_order[i][1]+1);
      }
  
    first_facet_dof[5] = ndof;
  }





  // --------------------------------------------

  void VectorFacetVolumeHex::
  CalcShape (const IntegrationPoint & ip, FlatMatrixFixWidth<3> shape) const
  {
    //   topology: points = 0:()
    int fanr = -1;
    double x=ip(0), y=ip(1), z=ip(2);
    
    shape = 0.0;

    if (fabs(z)<1e-6)
      {
	// (0,0) -> (0,0,0), (1,0) -> (0,1,0), (1,1) -> (1,1,0), (0,1) -> (1,0,0)
	fanr=0;
      }
    else if (fabs(z)>1-1e-6)
      {
	// (0,0) -> (0,0,1), (1,0) -> (1,0,0), (1,1) -> (1,1,1), (0,1) -> (0,1,1)
	fanr=1;
      }
    else if (fabs(y)<1e-6) 
      {
	// (0,0) -> (0,0,0), (1,0) -> (1,0,0), (1,1) -> (1,0,1), (0,1) -> (0,0,1)
	fanr=2;
      }
    else if (fabs(x)> 1-1e-6)
      {
	// (0,0) -> (1,0,0), (1,0) -> (1,1,0), (1,1) -> (1,1,1), (0,1) -> (1,0,1)
	fanr=3;
      }
    else if (fabs(y)> 1-1e-6)
      {
	// (0,0) -> (1,1,0), (1,0) -> (0,1,0), (1,1) -> (0,1,1), (0,1) -> (1,1,1)
	fanr=4;
      }
    else if (fabs(x)<1e-6)
      {
	// (0,0) -> (0,1,0), (1,0) -> (0,0,0), (1,1) -> (0,0,1), (0,1) -> (0,1,1)
	fanr=5;
      }

    CalcShape ( ip, fanr, shape );
    ;
  }

  void VectorFacetVolumeHex ::
  CalcShape ( const IntegrationPoint & ip, int facet, FlatMatrixFixWidth<3> shape ) const
  {
    cout << "VectorFacetVolumeHex::CalcShape not implemented!" << endl;
    exit(0);
  }

  
  void VectorFacetVolumeHex::ComputeNDof()  
  {
    ndof = 0;
    for (int i=0; i<6; i++)
      {
	first_facet_dof[i] = ndof;
	ndof += 2* (facet_order[i][0]+1) * (facet_order[i][0]+1);
      }
    first_facet_dof[6] = ndof;
  }


  // -------------------------------------------------------------------------------
  void VectorFacetVolumePyramid::
  CalcShape (const IntegrationPoint & ip, FlatMatrixFixWidth<3> shape) const
  {
    ;
    cout << "error in VectorFacetVolumePyramid::CalcShape: not implemented!" << endl;
    exit(0);
  }
   
  void VectorFacetVolumePyramid ::
  CalcShape ( const IntegrationPoint & ip, int facet, FlatMatrixFixWidth<3> shape ) const
  {
    cout << "error in VectorFacetVolumePyramid::CalcShape: not implemented!" << endl;
    exit(0);
  }

//   void VectorFacetVolumePyramid::CalcFacetShape (int afnr, const IntegrationPoint & ip, FlatMatrix<> shape) const
//   {
//     ;
//   }
   
//   void VectorFacetVolumePyramid::SetFacet(int afnr) const
//   {
//     if (qnr == afnr || tnr == afnr) return;
  
//     VectorFacetVolumePyramid * pyramid=const_cast<VectorFacetVolumePyramid*>(this);
//     if (afnr < 4) // triangles
//       {
// 	pyramid->tnr = afnr;
// 	Array<int> fvnums(3);  const FACE * faces = ElementTopology::GetFaces (eltype);

// 	fvnums[0] = vnums[faces[tnr][0]]; 
// 	fvnums[1] = vnums[faces[tnr][1]]; 
// 	fvnums[2] = vnums[faces[tnr][2]]; 
   
// 	pyramid->trig.SetVertexNumbers(fvnums);
// 	pyramid->trig.SetOrder(facet_order[tnr]);
//       }
//     else // quad face
//       {
// 	pyramid->qnr = afnr;
// 	Array<int> fvnums(4);  const FACE * faces = ElementTopology::GetFaces (eltype);

// 	fvnums[0] = vnums[faces[qnr][0]]; 
// 	fvnums[1] = vnums[faces[qnr][1]]; 
// 	fvnums[2] = vnums[faces[qnr][2]]; 
// 	fvnums[3] = vnums[faces[qnr][3]]; 
   
// 	pyramid->quad.SetVertexNumbers(fvnums);
// 	pyramid->quad.SetOrder(facet_order[qnr]);
//       }
//   }  


  void VectorFacetVolumePyramid::ComputeNDof() 
  {
    ndof = 0;
    // triangles
    for (int i=0; i<4; i++)
      {
	first_facet_dof[i] = ndof;
	ndof += ( (facet_order[i][0]+1) * (facet_order[i][0]+2) );
      }
    //quad - basis
    first_facet_dof[4] = ndof;
    ndof += 2 * (facet_order[4][0]+1) * (facet_order[4][0]+1);
  
    // final
    first_facet_dof[4] = ndof;
  }

}



namespace ngfem
{

  using namespace ngfem;


  namespace vectorfacetint {

    class Init
    { 
    public: 
      Init ();

    };
    
    Init::Init()
    {
             
      /*
      GetIntegrators().AddBFIntegrator ("massvectorfacet", 3, 1,
					MassEdgeIntegrator<3, VectorFacetVolumeFiniteElement<3> >::Create);
      GetIntegrators().AddBFIntegrator ("massvectorfacet", 2, 1,
					MassEdgeIntegrator<2, VectorFacetVolumeFiniteElement<2> >::Create);
      */


      GetIntegrators().AddBFIntegrator ("robinvectorfacet", 3, 1,
					RobinEdgeIntegrator<3, VectorFacetFacetFiniteElement>::Create);
      GetIntegrators().AddBFIntegrator ("robinvectorfacet", 2, 1,
					RobinEdgeIntegrator<2, VectorFacetFacetFiniteElement>::Create);      
      
      /*
      GetIntegrators().AddLFIntegrator ("sourcevectorfacet", 3, 3,
					SourceEdgeIntegrator<3, VectorFacetVolumeFiniteElement<3> >::Create);
      GetIntegrators().AddLFIntegrator ("sourcevectorfacet", 2, 2,
					SourceEdgeIntegrator<2, VectorFacetVolumeFiniteElement<2> >::Create);
       */
      GetIntegrators().AddLFIntegrator ("neumannvectorfacet", 3, 3,
					NeumannEdgeIntegrator<3, VectorFacetFacetFiniteElement>::Create);
      GetIntegrators().AddLFIntegrator ("neumannvectorfacet", 2, 2,
					NeumannEdgeIntegrator<2, VectorFacetFacetFiniteElement>::Create);
    }

    Init init;
  }
}


