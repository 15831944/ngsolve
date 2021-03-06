#ifndef FILE_HCURLHOFE_
#define FILE_HCURLHOFE_  

/*********************************************************************/
/* File:   hcurlhofe.hpp                                             */
/* Author: Sabine Zaglmayr                                           */
/* Date:   20. Maerz 2003                                            */
/*                                                                   */
/* AutoCurl - revision: J. Schoeberl, March 2009                     */
/*********************************************************************/
   
namespace ngfem
{



  /** 
      HCurlHighOrderFE of shape ET.
      The template specialization provides the shape functions.
  */
  template <ELEMENT_TYPE ET> class HCurlHighOrderFE_Shape;


  /**
     High order finite elements for H(curl).  
     These are the actual finite element classes to be used.  
     Shape functions are provided by the shape template
   */

  template <ELEMENT_TYPE ET, 
            template <ELEMENT_TYPE ET2> class TSHAPES = HCurlHighOrderFE_Shape,
            typename BASE = T_HCurlHighOrderFiniteElement<ET, TSHAPES<ET>> >

  class HCurlHighOrderFE : public BASE, public ET_trait<ET>
  {
  protected:
    using ET_trait<ET>::N_VERTEX;
    using ET_trait<ET>::N_EDGE;
    using ET_trait<ET>::N_FACE;
    using ET_trait<ET>::N_CELL;
    using ET_trait<ET>::FaceType;

    enum { DIM = ET_trait<ET>::DIM };

    typedef short TORDER;

    int vnums[N_VERTEX]; 
    INT<N_EDGE, TORDER> order_edge;
    INT<N_FACE, INT<2, TORDER>> order_face;
    INT<3, TORDER> order_cell;
    
    //bool usegrad_edge[N_EDGE]; 
    INT<N_EDGE, bool> usegrad_edge;
    INT<N_FACE, bool> usegrad_face;
    bool usegrad_cell; 

    using BASE::ndof;
    using BASE::order;

    typedef IntegratedLegendreMonomialExt T_ORTHOPOL;

  public:
    /* INLINE */ HCurlHighOrderFE () { ; } 

    /* INLINE */ HCurlHighOrderFE (int aorder) 
    {
      order_edge = aorder;
      order_face = aorder;
      // for (int i = 0; i < N_EDGE; i++) order_edge[i] = aorder;
      // for (int i = 0; i < N_FACE; i++) order_face[i] = aorder;
      if (DIM == 3) order_cell = aorder;
      
      for(int i = 0; i < N_EDGE; i++) usegrad_edge[i] = 1;
      for(int i = 0; i < N_FACE; i++) usegrad_face[i] = 1;
      if (DIM == 3) usegrad_cell = 1;
      
      for (int i = 0; i < N_VERTEX; i++) vnums[i] = i;
      
      ComputeNDof();
    }

    /// assignes vertex numbers
    template <typename TA> 
    INLINE void SetVertexNumbers (const TA & avnums)
    { for (int i = 0; i < N_VERTEX; i++) vnums[i] = avnums[i]; }


    // void SetVertexNumber (int nr, int vnum) { vnums[nr] = vnum; }
    INLINE void SetOrderEdge (int nr, int order) { order_edge[nr] = order; }
    INLINE void SetOrderFace (int nr, INT<2> order) { order_face[nr] = order; }

    INLINE void SetUseGradEdge(int nr, bool uge) { usegrad_edge[nr] = uge; }
    INLINE void SetUseGradFace(int nr, bool ugf) { usegrad_face[nr] = ugf; }


    INLINE void SetOrderCell (INT<3> oi) { order_cell = oi; }

    /// set isotropic or anisotropic face orders
    template <typename TA>
    INLINE void SetOrderFace (const TA & of)
    { for (int i = 0; i < N_FACE; i++) order_face[i] = of[i]; }

    /// set edge orders
    template <typename TA>
    INLINE void SetOrderEdge (const TA & oe)
    { for (int i = 0; i < N_EDGE; i++) order_edge[i] = oe[i]; }

    /// use edge-gradients
    template <typename TA>
    INLINE void SetUseGradEdge (const TA & uge)
    { for (int i = 0; i < N_EDGE; i++) usegrad_edge[i] = uge[i]; }

    /// use face-gradients
    template <typename TA>
    INLINE void SetUseGradFace (const TA & ugf)
    { for (int i = 0; i < N_FACE; i++) usegrad_face[i] = ugf[i]; }

    INLINE void SetUseGradCell (bool ugc) 
    { usegrad_cell = ugc; }

    void ComputeNDof();
  };

}  


#ifdef FILE_HCURLHOFE_CPP

#define HCURLHOFE_EXTERN
#include <thcurlfe_impl.hpp>
#include <hcurlhofe_impl.hpp>

#else

#define HCURLHOFE_EXTERN extern

#endif


namespace ngfem
{
  // HCURLHOFE_EXTERN template class HCurlHighOrderFE<ET_POINT>;
  HCURLHOFE_EXTERN template class HCurlHighOrderFE<ET_SEGM>;
  HCURLHOFE_EXTERN template class HCurlHighOrderFE<ET_TRIG>;
  HCURLHOFE_EXTERN template class HCurlHighOrderFE<ET_QUAD>;
  HCURLHOFE_EXTERN template class HCurlHighOrderFE<ET_TET>;
  HCURLHOFE_EXTERN template class HCurlHighOrderFE<ET_PRISM>;
  HCURLHOFE_EXTERN template class HCurlHighOrderFE<ET_PYRAMID>;
  HCURLHOFE_EXTERN template class HCurlHighOrderFE<ET_HEX>;

  // HCURLHOFE_EXTERN template class 
  // T_HCurlHighOrderFiniteElement<ET_POINT, HCurlHighOrderFE_Shape<ET_POINT>>;
  HCURLHOFE_EXTERN template class 
  T_HCurlHighOrderFiniteElement<ET_SEGM, HCurlHighOrderFE_Shape<ET_SEGM>>;
  HCURLHOFE_EXTERN template class 
  T_HCurlHighOrderFiniteElement<ET_TRIG, HCurlHighOrderFE_Shape<ET_TRIG>>;
  HCURLHOFE_EXTERN template class 
  T_HCurlHighOrderFiniteElement<ET_QUAD, HCurlHighOrderFE_Shape<ET_QUAD>>;

  HCURLHOFE_EXTERN template class 
  T_HCurlHighOrderFiniteElement<ET_TET, HCurlHighOrderFE_Shape<ET_TET>>;
  HCURLHOFE_EXTERN template class 
  T_HCurlHighOrderFiniteElement<ET_PRISM, HCurlHighOrderFE_Shape<ET_PRISM>>;
  HCURLHOFE_EXTERN template class 
  T_HCurlHighOrderFiniteElement<ET_PYRAMID, HCurlHighOrderFE_Shape<ET_PYRAMID>>;
  HCURLHOFE_EXTERN template class 
  T_HCurlHighOrderFiniteElement<ET_HEX, HCurlHighOrderFE_Shape<ET_HEX>>;
  
}

#endif

