#ifndef FILE_H1HOFE_IMPL
#define FILE_H1HOFE_IMPL

/*********************************************************************/
/* File:   h1hofe_impl.hpp                                           */
/* Author: Start                                                     */
/* Date:   6. Feb. 2003                                              */
/*********************************************************************/

#include "recursive_pol_tet.hpp"

namespace ngfem
{

  template <ELEMENT_TYPE ET> 
  class H1HighOrderFE_Shape : public H1HighOrderFE<ET, H1HighOrderFE_Shape<ET>>
  {
    using H1HighOrderFE<ET>::vnums;
    using H1HighOrderFE<ET>::order;
    using H1HighOrderFE<ET>::order_edge;
    using H1HighOrderFE<ET>::order_face;
    using H1HighOrderFE<ET>::order_cell;
    using H1HighOrderFE<ET>::GetFaceSort;
    using H1HighOrderFE<ET>::GetEdgeSort;
    // using H1HighOrderFE<ET>::EdgeOrthoPol;

    using H1HighOrderFE<ET>::N_VERTEX;
    using H1HighOrderFE<ET>::N_EDGE;
    using H1HighOrderFE<ET>::N_FACE;

    // typedef LegendrePolynomial EdgeOrthoPol;
    // typedef IntLegNoBubble EdgeOrthoPol;  // Integrated Legendre divided by bubble
    typedef ChebyPolynomial EdgeOrthoPol; 
    typedef ChebyPolynomial QuadOrthoPol; 

  public:
    template<typename Tx, typename TFA>  
    INLINE void T_CalcShape (Tx hx[], TFA & shape) const;
  };
  


  /* *********************** Point  **********************/

  template<> template<typename Tx, typename TFA>  
  void H1HighOrderFE_Shape<ET_POINT> :: T_CalcShape (Tx x[], TFA & shape) const
  {
    shape[0] = 1.0;
  }



  /* *********************** Segment  **********************/  

  template <> template<typename Tx, typename TFA>  
  void H1HighOrderFE_Shape<ET_SEGM> :: T_CalcShape (Tx x[], TFA & shape) const
  {
    Tx lam[2] = { x[0], 1-x[0] };
    
    shape[0] = lam[0];
    shape[1] = lam[1];
    
    if (order_edge[0] >= 2)
      {
        INT<2> e = GetEdgeSort (0, vnums);
        EdgeOrthoPol::
          EvalMult (order_edge[0]-2, 
                    lam[e[1]]-lam[e[0]], lam[e[0]]*lam[e[1]], shape+2);
      }
  }


  /* *********************** Triangle  **********************/

  template<> template<typename Tx, typename TFA>  
  void H1HighOrderFE_Shape<ET_TRIG> :: T_CalcShape (Tx x[], TFA & shape) const
  {
    Tx lam[3] = { x[0], x[1], 1-x[0]-x[1] };

    for (int i = 0; i < 3; i++) shape[i] = lam[i];

    int ii = 3;
    
    // edge-based shapes
    for (int i = 0; i < N_EDGE; i++)
      if (order_edge[i] >= 2)
	{ 
          INT<2> e = GetEdgeSort (i, vnums);
          EdgeOrthoPol::
            EvalScaledMult (order_edge[i]-2, 
                            lam[e[1]]-lam[e[0]], lam[e[0]]+lam[e[1]], 
                            lam[e[0]]*lam[e[1]], shape+ii);
	  ii += order_edge[i]-1;
	}

    // inner shapes
    if (order_face[0][0] >= 3)
      {
        INT<4> f = GetFaceSort (0, vnums);
	DubinerBasis3::EvalMult (order_face[0][0]-3, 
				 lam[f[0]], lam[f[1]], 
				 lam[f[0]]*lam[f[1]]*lam[f[2]], shape+ii);
      }
  }


  /* *********************** Quadrilateral  **********************/

  template<> template<typename Tx, typename TFA>  
  void H1HighOrderFE_Shape<ET_QUAD> :: T_CalcShape (Tx hx[], TFA & shape) const
  {
    Tx x = hx[0], y = hx[1];
    Tx lam[4] = {(1-x)*(1-y),x*(1-y),x*y,(1-x)*y};  
    
    // vertex shapes
    for(int i=0; i < N_VERTEX; i++) shape[i] = lam[i]; 
    int ii = 4;
     
    // edge-based shapes
    for (int i = 0; i < N_EDGE; i++)
      if (order_edge[i] >= 2)
        {
          int p = order_edge[i];

          Tx xi = ET_trait<ET_QUAD>::XiEdge(i, hx, vnums);
          Tx lam_e = ET_trait<ET_QUAD>::LamEdge(i, hx);
          
          Tx bub = 0.25 * lam_e * (1 - xi*xi);
          EdgeOrthoPol::EvalMult (p-2, xi, bub, shape+ii);
          ii += p-1;
        }
    
    // inner shapes
    INT<2> p = order_face[0];
    if (p[0] >= 2 && p[1] >= 2)
      {
        Vec<2,Tx> xi = ET_trait<ET_QUAD>::XiFace(0, hx, vnums);

	Tx bub = 1.0/16 * (1-xi(0)*xi(0))*(1-xi(1)*xi(1));
        
        /*
	ArrayMem<Tx,20> polxi(order+1), poleta(order+1);
        
	LegendrePolynomial::EvalMult(p[0]-2, xi(0), bub, polxi);
	LegendrePolynomial::Eval(p[1]-2, xi(1), poleta);
	
	for (int k = 0; k <= p[0]-2; k++)
	  for (int j = 0; j <= p[1]-2; j++)
	    shape[ii++] = polxi[k] * poleta[j];
        */

        QuadOrthoPol::EvalMult1Assign(p[0]-2, xi(0), bub,
          SBLambda ([&](int i, Tx val) LAMBDA_INLINE 
                    {  
                      QuadOrthoPol::EvalMult (p[1]-2, xi(1), val, shape+ii);
                      ii += p[1]-1;
                    }));
      }
  }


  /* *********************** Tetrahedron  **********************/

  template<> template<typename Tx, typename TFA>  
  INLINE void H1HighOrderFE_Shape<ET_TET> :: T_CalcShape (Tx x[], TFA & shape) const
  {
    Tx lam[4] = { x[0], x[1], x[2], 1-x[0]-x[1]-x[2] };

    // vertex shapes
    for (int i = 0; i < 4; i++) shape[i] = lam[i];

    int ii = 4; 

    // edge dofs
    for (int i = 0; i < N_EDGE; i++)
      if (order_edge[i] >= 2)
	{
          INT<2> e = GetEdgeSort (i, vnums);
	  EdgeOrthoPol::EvalScaledMult (order_edge[i]-2, 
                                        lam[e[1]]-lam[e[0]], lam[e[0]]+lam[e[1]], 
                                        lam[e[0]]*lam[e[1]], shape+ii);
	  ii += order_edge[i]-1;
	}

    // face dofs
    for (int i = 0; i < N_FACE; i++)
      if (order_face[i][0] >= 3)
	{
          INT<4> f = GetFaceSort (i, vnums);
	  int vop = 6 - f[0] - f[1] - f[2];  	
          
	  int p = order_face[i][0];
	  DubinerBasis3::EvalScaledMult (p-3, lam[f[0]], lam[f[1]], 1-lam[vop], 
					 lam[f[0]]*lam[f[1]]*lam[f[2]], shape+ii);
	  ii += (p-2)*(p-1)/2;
	}

    if (order_cell[0][0] >= 4)
      ii += TetShapesInnerLegendre::
	Calc (order_cell[0][0], 
	      lam[0]-lam[3], lam[1], lam[2], 
	      shape+ii);
  }




  /* *********************** Prism  **********************/

  template<> template<typename Tx, typename TFA>  
  void  H1HighOrderFE_Shape<ET_PRISM> :: T_CalcShape (Tx hx[], TFA & shape) const
  {
    Tx x = hx[0], y = hx[1], z = hx[2];
    Tx lam[6] = { x, y, 1-x-y, x, y, 1-x-y };
    Tx muz[6]  = { 1-z, 1-z, 1-z, z, z, z };

    Tx sigma[6];
    for (int i = 0; i < 6; i++) sigma[i] = lam[i] + muz[i];

    // vertex shapes
    for (int i = 0; i < 6; i++) shape[i] = lam[i] * muz[i];

    int ii = 6;

    // horizontal edge dofs
    for (int i = 0; i < 6; i++)
      if (order_edge[i] >= 2)
	{
          INT<2> e = GetEdgeSort (i, vnums);

	  Tx xi = lam[e[1]]-lam[e[0]]; 
	  Tx eta = lam[e[0]]+lam[e[1]]; 
	  Tx bub = lam[e[0]]*lam[e[1]]*muz[e[1]];

	  EdgeOrthoPol::
	    EvalScaledMult (order_edge[i]-2, xi, eta, bub, shape+ii);
	  ii += order_edge[i]-1;
	}
    
    // vertical edges
    for (int i = 6; i < 9; i++)
      if (order_edge[i] >= 2)
	{
          INT<2> e = GetEdgeSort (i, vnums);

	  EdgeOrthoPol::
	    EvalMult (order_edge[i]-2, 
		      muz[e[1]]-muz[e[0]], 
		      muz[e[0]]*muz[e[1]]*lam[e[1]], shape+ii);

	  ii += order_edge[i]-1;
	}
    

    ArrayMem<Tx,20> polx(order+1), poly(order+1), polz(order+1);

    // trig face dofs
    for (int i = 0; i < 2; i++)
      if (order_face[i][0] >= 3)
	{
          INT<4> f = GetFaceSort (i, vnums);
	  int p = order_face[i][0];
	  
	  Tx bub = lam[0]*lam[1]*lam[2]*muz[f[2]];
	  
	  DubinerBasis3::
	    EvalMult (p-3, lam[f[0]], lam[f[1]], bub, shape+ii);

	  ii += (p-2)*(p-1)/2; 
	}
   
    // quad face dofs
    for (int i = 2; i < 5; i++)
      if (order_face[i][0] >= 2 && order_face[i][1] >= 2)
	{
	  INT<2> p = order_face[i];
          INT<4> f = GetFaceSort (i, vnums);	  

	  Tx xi  = sigma[f[0]] - sigma[f[1]]; 
	  Tx eta = sigma[f[0]] - sigma[f[3]];

	  Tx scalexi(1.0), scaleeta(1.0);
	  if (f[0] / 3 == f[1] / 3)  
	    scalexi = lam[f[0]]+lam[f[1]];  // xi is horizontal
	  else
	    scaleeta = lam[f[0]]+lam[f[3]];

	  Tx bub = (1.0/16)*(scaleeta*scaleeta-eta*eta)*(scalexi*scalexi-xi*xi);
	  QuadOrthoPol::EvalScaled     (p[0]-2, xi, scalexi, polx);
	  QuadOrthoPol::EvalScaledMult (p[1]-2, eta, scaleeta, bub, poly);
	    
	  for (int k = 0; k < p[0]-1; k++) 
            for (int j = 0; j < p[1]-1; j++) 
              shape[ii++] = polx[k] * poly[j];
	}
    
    // volume dofs:
    INT<3> p = order_cell[0];
    if (p[0] > 2 && p[2] > 1)
      {
	int nf = (p[0]-1)*(p[0]-2)/2;
	ArrayMem<Tx,20> pol_trig(nf);

	DubinerBasis3::EvalMult (p[0]-3, x, y, x*y*(1-x-y),pol_trig);
	LegendrePolynomial::EvalMult (p[2]-2, 2*z-1, z*(1-z), polz);

	for (int i = 0; i < nf; i++)
	  for (int k = 0; k < p[2]-1; k++)
	    shape[ii++] = pol_trig[i] * polz[k];
      }
  }



 

  /* *********************** Hex  **********************/

  template<> template<typename Tx, typename TFA>  
  void  H1HighOrderFE_Shape<ET_HEX> :: T_CalcShape (Tx hx[], TFA & shape) const
  { 
    Tx x = hx[0], y = hx[1], z = hx[2];

    Tx lam[8]={(1-x)*(1-y)*(1-z),x*(1-y)*(1-z),x*y*(1-z),(1-x)*y*(1-z),
		(1-x)*(1-y)*z,x*(1-y)*z,x*y*z,(1-x)*y*z}; 
    Tx sigma[8]={(1-x)+(1-y)+(1-z),x+(1-y)+(1-z),x+y+(1-z),(1-x)+y+(1-z),
		 (1-x)+(1-y)+z,x+(1-y)+z,x+y+z,(1-x)+y+z}; 

    // vertex shapes
    for (int i = 0; i < 8; i++) shape[i] = lam[i]; 
    int ii = 8;

    ArrayMem<Tx,30> polx(order+1), poly(order+1), polz(order+1);
    
    // edge dofs
    for (int i = 0; i < N_EDGE; i++)
      if (order_edge[i] >= 2)
	{
	  int p = order_edge[i];

          INT<2> e = GetEdgeSort (i, vnums);	  
          Tx xi = sigma[e[1]]-sigma[e[0]]; 
          Tx lam_e = lam[e[0]]+lam[e[1]];
	  Tx bub = 0.25 * lam_e * (1 - xi*xi);
	  
	  EdgeOrthoPol::EvalMult (p-2, xi, bub, shape+ii);
	  ii += p-1;
	}

    for (int i = 0; i < N_FACE; i++)
      if (order_face[i][0] >= 2 && order_face[i][1] >= 2)
	{
	  INT<2> p = order_face[i];
          INT<4> f = GetFaceSort (i, vnums);	  

	  Tx lam_f(0.0);
	  for (int j = 0; j < 4; j++) lam_f += lam[f[j]];
          
	  Tx xi  = sigma[f[0]] - sigma[f[1]]; 
	  Tx eta = sigma[f[0]] - sigma[f[3]];

	  Tx bub = 1.0/16 * (1-xi*xi)*(1-eta*eta) * lam_f;
	  QuadOrthoPol::EvalMult(p[0]-2, xi, bub, polx);
	  QuadOrthoPol::Eval(p[1]-2, eta, poly);

	  for (int k = 0; k < p[0]-1; k++) 
            for (int j = 0; j < p[1]-1; j++) 
              shape[ii++]= polx[k] * poly[j];
	}

    // volume dofs:
    INT<3> p = order_cell[0];
    if (p[0] >= 2 && p[1] >= 2 && p[2] >= 2)
      {
	QuadOrthoPol::EvalMult (p[0]-2, 2*x-1, x*(1-x), polx);
	QuadOrthoPol::EvalMult (p[1]-2, 2*y-1, y*(1-y), poly);
	QuadOrthoPol::EvalMult (p[2]-2, 2*z-1, z*(1-z), polz);

	for (int i = 0; i < p[0]-1; i++)
	  for (int j = 0; j < p[1]-1; j++)
	    {
	      Tx pxy = polx[i] * poly[j];
	      for (int k = 0; k < p[2]-1; k++)
		shape[ii++] = pxy * polz[k];
	    }
      }
  }

  /* ******************************** Pyramid  ************************************ */

  template<> template<typename Tx, typename TFA>  
  void  H1HighOrderFE_Shape<ET_PYRAMID> :: T_CalcShape (Tx hx[], TFA & shape) const
  {
    Tx x = hx[0], y = hx[1], z = hx[2];

    // if (z == 1.) z -= 1e-10;
    z *= (1-1e-10);

    Tx xt = x / (1-z);
    Tx yt = y / (1-z);
    
    Tx sigma[4]  = { (1-xt)+(1-yt), xt+(1-yt), xt+yt, (1-xt)+yt };
    Tx lambda[4] = { (1-xt)*(1-yt), xt*(1-yt), xt*yt, (1-xt)*yt };
    Tx lambda3d[5];

    for (int i = 0; i < 4; i++)  
      lambda3d[i] = lambda[i] * (1-z);
    lambda3d[4] = z;


    for (int i = 0; i < 5; i++)  
      shape[i] = lambda3d[i];

    int ii = 5;


    // horizontal edge dofs 
    for (int i = 0; i < 4; i++)
      if (order_edge[i] >= 2)
	{
	  int p = order_edge[i];
	  INT<2> e = GetEdgeSort (i, vnums);	  

	  Tx xi = sigma[e[1]]-sigma[e[0]]; 
	  Tx lam_e = lambda[e[0]]+lambda[e[1]];
	  Tx bub = 0.25 * lam_e * (1 - xi*xi)*(1-z)*(1-z);
	  Tx ximz = xi*(1-z);
	  LegendrePolynomial::
	    EvalScaledMult (p-2, ximz, 1-z, bub, shape+ii);
	  ii += p-1;
	}
    
    // vertical edges
    for (int i = 4; i < 8; i++) 
      if (order_edge[i] >= 2)
	{
	  int p = order_edge[i];
	  INT<2> e = GetEdgeSort (i, vnums);	  

	  Tx xi = lambda3d[e[1]]-lambda3d[e[0]]; 
	  Tx lam_e = lambda3d[e[0]]+lambda3d[e[1]];
	  Tx bub = 0.25 * (lam_e*lam_e-xi*xi);
	  
	  LegendrePolynomial::
	    EvalScaledMult (p-2, xi, lam_e, bub, shape+ii);
	  ii += p-1;
	}


    ArrayMem<Tx,20> polx(order+1), poly(order+1), polz(order+1);
    const FACE * faces = ElementTopology::GetFaces (ET_PYRAMID);

    // trig face dofs
    for (int i = 0; i < 4; i++)
      if (order_face[i][0] >= 3)
	{
	  int p = order_face[i][0];
	  Tx lam_face = lambda[faces[i][0]] + lambda[faces[i][1]];  // vertices on quad    

	  Tx bary[5] = 
	    {(sigma[0]-lam_face)*(1-z), (sigma[1]-lam_face)*(1-z), 
	     (sigma[2]-lam_face)*(1-z), (sigma[3]-lam_face)*(1-z), z };
	  
	  INT<4> f = GetFaceSort (i, vnums);

	  Tx bub = lam_face * bary[f[0]]*bary[f[1]]*bary[f[2]];

	  DubinerBasis3::
	    EvalMult (p-3, bary[f[0]], bary[f[1]], bub, shape+ii);
	  ii += (p-2)*(p-1)/2;
	}
    
    // quad face dof
    if (order_face[4][0] >= 2 && order_face[4][1] >= 2)
      {	  
	INT<2> p = order_face[4];

	int pmax = max2(p[0], p[1]);
	Tx fac(1.0);
	for (int k = 1; k <= pmax; k++)
	  fac *= (1-z);

	INT<4> f = GetFaceSort (4, vnums);	  
	
	Tx xi  = sigma[f[0]] - sigma[f[1]]; 
	Tx eta = sigma[f[0]] - sigma[f[3]];

	QuadOrthoPol::EvalMult (p[0]-2, xi,  0.25*(1-xi*xi), polx);
	QuadOrthoPol::EvalMult (p[1]-2, eta, 0.25*(1-eta*eta), poly);
	for (int k = 0; k < p[0]-1; k++) 
	  for (int j = 0; j < p[1]-1; j++) 
	    shape[ii++] = polx[k] * poly[j] * fac; 
      }

    
    if (order_cell[0][0] >= 3)
      {
	LegendrePolynomial::EvalMult (order_cell[0][0]-2, 2*xt-1, xt*(1-xt), polx);
        LegendrePolynomial::EvalMult (order_cell[0][0]-2, 2*yt-1, yt*(1-yt), poly);

	Tx pz = z*(1-z)*(1-z);
	
	for(int k = 0; k <= order_cell[0][0]-3; k++)
	  {
	    for(int i = 0; i <= k; i++)
	      { 
		Tx bubpik = pz * polx[i];
		for (int j = 0; j <= k; j++)
		  shape[ii++] = bubpik * poly[j];
	      }
	    pz *= 1-z;  
	  }
      }
  }

}

#endif
