/*********************************************************************/
/* File:   meshaccess.cpp                                            */
/* Author: Joachim Schoeberl                                         */
/* Date:   25. Mar. 2000                                             */
/*********************************************************************/

/* 
   Access to fe mesh
*/

#include <comp.hpp>
#include "../fem/h1lofe.hpp"


namespace ngcomp
{

  
  
  template <int DIMS, int DIMR>
  class Ng_ElementTransformation : public ElementTransformation
  {
    // const netgen::Ngx_Mesh * mesh;
    const MeshAccess * mesh;	
  public:
    Ng_ElementTransformation (const MeshAccess * amesh) : mesh(amesh) { ; }


    virtual void SetElement (bool aboundary, int aelnr, int aelindex)
    {
      elnr = aelnr;
      elindex = aelindex;
      iscurved = true;
    }

    virtual int SpaceDim () const
    {
      return DIMR;
    }
    
    virtual bool Boundary() const
    {
      return DIMS < DIMR;
    }

    virtual bool BelongsToMesh (const void * mesh2) const 
    {
      // return mesh == &(static_cast<const MeshAccess*> (mesh2) -> mesh);
      return mesh == mesh2;
    }

    virtual const void * GetMesh () const { return mesh; }

    virtual void GetSort (FlatArray<int> sort) const
    {
      int vnums[12];

      Ngs_Element nel = mesh -> GetElement<DIMS, (DIMS==DIMR)?VOL:BND> (elnr);
      for (int j = 0; j  < nel.vertices.Size(); j++)
        vnums[j] = nel.vertices[j];

      switch (eltype)
	{
	case ET_TRIG:
	  for (int i = 0; i < 3; i++) sort[i] = i;
	  if (vnums[sort[0]] > vnums[sort[1]]) Swap (sort[0], sort[1]);
	  if (vnums[sort[1]] > vnums[sort[2]]) Swap (sort[1], sort[2]);
	  if (vnums[sort[0]] > vnums[sort[1]]) Swap (sort[0], sort[1]);
	  // vnums[sort[0]] < vnums[sort[1]] < vnums[sort[2]]
	  break; 

	case ET_TET:
	  for (int i = 0; i < 4; i++) sort[i] = i;
	  if (vnums[sort[0]] > vnums[sort[1]]) Swap (sort[0], sort[1]);
	  if (vnums[sort[2]] > vnums[sort[3]]) Swap (sort[2], sort[3]);
	  if (vnums[sort[0]] > vnums[sort[2]]) Swap (sort[0], sort[2]);
	  if (vnums[sort[1]] > vnums[sort[3]]) Swap (sort[1], sort[3]);
	  if (vnums[sort[1]] > vnums[sort[2]]) Swap (sort[1], sort[2]);

	  // vnums[sort[0]] < vnums[sort[1]] < vnums[sort[2]] < vnums[sort[3]]
	  break; 

	case ET_PRISM:
	  for (int i = 0; i < 6; i++) sort[i] = i;

	  if (vnums[sort[0]] > vnums[sort[1]]) Swap (sort[0], sort[1]);
	  if (vnums[sort[1]] > vnums[sort[2]]) Swap (sort[1], sort[2]);
	  if (vnums[sort[0]] > vnums[sort[1]]) Swap (sort[0], sort[1]);
        
	  if (vnums[sort[3]] > vnums[sort[4]]) Swap (sort[3], sort[4]);
	  if (vnums[sort[4]] > vnums[sort[5]]) Swap (sort[4], sort[5]);
	  if (vnums[sort[3]] > vnums[sort[4]]) Swap (sort[3], sort[4]);
	  break;

	default:
	  throw Exception ("undefined eltype in ElementTransformation::GetSort()\n");
	}
      
    }


    virtual void CalcJacobian (const IntegrationPoint & ip,
			       FlatMatrix<> dxdxi) const
    {
      mesh->mesh.ElementTransformation <DIMS,DIMR> (elnr, &ip(0), NULL, &dxdxi(0));
    }
    
    virtual void CalcPoint (const IntegrationPoint & ip,
			    FlatVector<> point) const
    {
      mesh->mesh.ElementTransformation <DIMS,DIMR> (elnr, &ip(0), &point(0), NULL);
    }

    virtual void CalcPointJacobian (const IntegrationPoint & ip,
				    FlatVector<> point, FlatMatrix<> dxdxi) const
    {
      mesh->mesh.ElementTransformation <DIMS,DIMR> (elnr, &ip(0), &point(0), &dxdxi(0));
    }

    virtual BaseMappedIntegrationPoint & operator() (const IntegrationPoint & ip, LocalHeap & lh) const
    {
      return *new (lh) MappedIntegrationPoint<DIMS,DIMR> (ip, *this);
    }

    virtual BaseMappedIntegrationRule & operator() (const IntegrationRule & ir, LocalHeap & lh) const
    {
      return *new (lh) MappedIntegrationRule<DIMS,DIMR> (ir, *this, lh);
    }    

    virtual void CalcMultiPointJacobian (const IntegrationRule & ir,
					 BaseMappedIntegrationRule & bmir) const
    {
      if (sizeof(IntegrationPoint) % 8 != 0)
        {
          cerr << "Integration must should have 8-byte alignment" << endl;
          exit(1);
        }

      // static Timer t("eltrans::multipointjacobian"); RegionTimer reg(t);
      MappedIntegrationRule<DIMS,DIMR> & mir = 
	static_cast<MappedIntegrationRule<DIMS,DIMR> &> (bmir);
      mesh->mesh.MultiElementTransformation <DIMS,DIMR> (elnr, ir.Size(),
                                                         &ir[0](0), &ir[1](0)-&ir[0](0),
                                                         &mir[0].Point()(0), 
                                                         &mir[1].Point()(0)-&mir[0].Point()(0), 
                                                         &mir[0].Jacobian()(0,0), 
                                                         &mir[1].Jacobian()(0,0)-&mir[0].Jacobian()(0,0));
    
      for (int i = 0; i < ir.Size(); i++)
        mir[i].Compute();
  }
};
  




  template <int DIMS, int DIMR>
  class ALE_ElementTransformation : public Ng_ElementTransformation<DIMS,DIMR>
  {
    const GridFunction * deform;
    LocalHeap & lh;
    const ScalarFiniteElement<DIMR> * fel;
    FlatVector<> elvec;
    FlatMatrix<> elvecs;
  public:
    ALE_ElementTransformation (const MeshAccess * amesh, 
                               const GridFunction * adeform,
                               LocalHeap & alh)
      : Ng_ElementTransformation<DIMS,DIMR> (amesh), deform(adeform), lh(alh) { ; }

    virtual void SetElement (bool aboundary, int aelnr, int aelindex)
    {
      Ng_ElementTransformation<DIMS,DIMR> :: SetElement (aboundary, aelnr, aelindex);

      ElementId id(aboundary ? BND : VOL, aelnr);
      fel = dynamic_cast<const ScalarFiniteElement<DIMR>*> (&deform->GetFESpace()->GetFE(id, lh));

      Array<int> dnums;
      deform->GetFESpace()->GetDofNrs(id, dnums);

      elvec.AssignMemory(DIMR*dnums.Size(), lh);
      deform->GetElementVector(dnums, elvec);

      elvecs.AssignMemory(DIMR, dnums.Size(), lh);
      for (int j = 0; j < DIMR; j++)
        elvecs.Row(j) = elvec.Slice(j,DIMR);
    }


    virtual ~ALE_ElementTransformation() { ; }



    virtual void CalcJacobian (const IntegrationPoint & ip,
			       FlatMatrix<> dxdxi) const
    {
      /*
      Mat<DIMR,DIMS> tmp, itmp;
      Ng_ElementTransformation<DIMS,DIMR>::CalcJacobian (ip, tmp);
      itmp = Inv (tmp);

      Mat<DIMR,DIMR> def, mdef;
      for (int i = 0; i < DIMR; i++)
        def.Row(i) = fel->EvaluateGrad (ip, elvecs.Row(i));
      
      mdef = def*itmp;
      mdef += Id<DIMR>();
      dxdxi = mdef * tmp;
      */

      Mat<DIMR,DIMS> tmp;
      Ng_ElementTransformation<DIMS,DIMR>::CalcJacobian (ip, tmp);

      Mat<DIMR,DIMR> def;
      for (int i = 0; i < DIMR; i++)
        def.Row(i) = fel->EvaluateGrad (ip, elvecs.Row(i));
      dxdxi = def + tmp;
    }
    
    virtual void CalcPoint (const IntegrationPoint & ip,
			    FlatVector<> point) const
    {
      Vec<DIMR> tmp;
      Ng_ElementTransformation<DIMS,DIMR>::CalcPoint (ip, tmp);

      Vec<DIMR> def;
      for (int i = 0; i < DIMR; i++)
        def(i) = fel->Evaluate (ip, elvecs.Row(i));
      point = tmp + def;
    }

    virtual void CalcPointJacobian (const IntegrationPoint & ip,
				    FlatVector<> point, FlatMatrix<> dxdxi) const
    {
      CalcJacobian (ip, dxdxi);
      CalcPoint (ip, point);
      // this->mesh->mesh.ElementTransformation <DIMS,DIMR> (elnr, &ip(0), &point(0), &dxdxi(0));
    }

    virtual void CalcMultiPointJacobian (const IntegrationRule & ir,
					 BaseMappedIntegrationRule & bmir) const
    {
      if (sizeof(IntegrationPoint) % 8 != 0)
        {
          cerr << "Integration must should have 8-byte alignment" << endl;
          exit(1);
        }

      MappedIntegrationRule<DIMS,DIMR> & mir = 
	static_cast<MappedIntegrationRule<DIMS,DIMR> &> (bmir);

      for (int i = 0; i < ir.Size(); i++)
        {
          CalcPointJacobian (ir[i], mir[i].Point(), mir[i].Jacobian());
          mir[i].Compute();
        }

      /*
      MappedIntegrationRule<DIMS,DIMR> & mir = 
	static_cast<MappedIntegrationRule<DIMS,DIMR> &> (bmir);
      mesh->mesh.MultiElementTransformation <DIMS,DIMR> (elnr, ir.Size(),
                                                         &ir[0](0), &ir[1](0)-&ir[0](0),
                                                         &mir[0].Point()(0), 
                                                         &mir[1].Point()(0)-&mir[0].Point()(0), 
                                                         &mir[0].Jacobian()(0,0), 
                                                         &mir[1].Jacobian()(0,0)-&mir[0].Jacobian()(0,0));
    
      for (int i = 0; i < ir.Size(); i++)
        mir[i].Compute();
      */
  }

  };




  
  template <int DIMS, int DIMR>
  class Ng_ConstElementTransformation : public ElementTransformation
  {
    const MeshAccess * mesh;
    Vec<DIMR> p0;
    Mat<DIMR,DIMS> mat;
  public:
    Ng_ConstElementTransformation (const MeshAccess * amesh) 
      : mesh(amesh) 
    { ; }

    virtual void SetElement (bool aboundary, int aelnr, int aelindex)
    {
      elnr = aelnr;
      elindex = aelindex;
      iscurved = false;

      if (eltype == ET_TET)
        {
          Ngs_Element nel = mesh -> GetElement<DIMS,VOL> (elnr);
          // p0 = FlatVec<3> (point0[point_delta*nel.Vertices()[0]]);
          p0 = FlatVec<3, const double> (mesh->mesh.GetPoint (nel.Vertices()[3]));
	  for (int j = 0; j < 3; j++)
	    {
	      Vec<3> pj = FlatVec<3, const double>(mesh->mesh.GetPoint(nel.Vertices()[j])) -p0;
	      for (int k = 0; k < 3; k++)
		mat(k,j) = pj(k);
	    }
          //mat.Col(0) = FlatVec<3, const double> (mesh -> GetPoint (nel.Vertices()[0])) - p0;
          //mat.Col(1) = FlatVec<3, const double> (mesh -> GetPoint (nel.Vertices()[1])) - p0;
          //mat.Col(2) = FlatVec<3, const double> (mesh -> GetPoint (nel.Vertices()[2])) - p0;
        }
      else
        {
          Vec<DIMS> pref = 0.0;
          mesh->mesh.ElementTransformation <DIMS,DIMR> (elnr, &pref(0), &p0(0), &mat(0));
        }
    }

    virtual int SpaceDim () const
    {
      return DIMR;
    }
    
    virtual bool Boundary() const
    {
      return DIMS < DIMR;
    }

    virtual bool BelongsToMesh (const void * mesh2) const 
    {
      // return mesh == &(static_cast<const MeshAccess*> (mesh2) -> mesh);
      return mesh == mesh2;
    }

    virtual const void * GetMesh () const { return mesh; }

    virtual void GetSort (FlatArray<int> sort) const
    {
      int vnums[12];

      Ngs_Element nel = mesh -> GetElement<DIMS, (DIMS==DIMR)?VOL:BND> (elnr);
      for (int j = 0; j  < nel.vertices.Size(); j++)
        vnums[j] = nel.vertices[j];

      switch (eltype)
	{
	case ET_TRIG:
	  for (int i = 0; i < 3; i++) sort[i] = i;
	  if (vnums[sort[0]] > vnums[sort[1]]) Swap (sort[0], sort[1]);
	  if (vnums[sort[1]] > vnums[sort[2]]) Swap (sort[1], sort[2]);
	  if (vnums[sort[0]] > vnums[sort[1]]) Swap (sort[0], sort[1]);
	  // vnums[sort[0]] < vnums[sort[1]] < vnums[sort[2]]
	  break; 

	case ET_TET:
	  for (int i = 0; i < 4; i++) sort[i] = i;
	  if (vnums[sort[0]] > vnums[sort[1]]) Swap (sort[0], sort[1]);
	  if (vnums[sort[2]] > vnums[sort[3]]) Swap (sort[2], sort[3]);
	  if (vnums[sort[0]] > vnums[sort[2]]) Swap (sort[0], sort[2]);
	  if (vnums[sort[1]] > vnums[sort[3]]) Swap (sort[1], sort[3]);
	  if (vnums[sort[1]] > vnums[sort[2]]) Swap (sort[1], sort[2]);

	  // vnums[sort[0]] < vnums[sort[1]] < vnums[sort[2]] < vnums[sort[3]]
	  break; 

	case ET_PRISM:
	  for (int i = 0; i < 6; i++) sort[i] = i;

	  if (vnums[sort[0]] > vnums[sort[1]]) Swap (sort[0], sort[1]);
	  if (vnums[sort[1]] > vnums[sort[2]]) Swap (sort[1], sort[2]);
	  if (vnums[sort[0]] > vnums[sort[1]]) Swap (sort[0], sort[1]);
        
	  if (vnums[sort[3]] > vnums[sort[4]]) Swap (sort[3], sort[4]);
	  if (vnums[sort[4]] > vnums[sort[5]]) Swap (sort[4], sort[5]);
	  if (vnums[sort[3]] > vnums[sort[4]]) Swap (sort[3], sort[4]);
	  break;

	default:
	  throw Exception ("undefined eltype in ElementTransformation::GetSort()\n");
	}
      
    }


    virtual void CalcJacobian (const IntegrationPoint & ip,
			       FlatMatrix<> dxdxi) const
    {
      dxdxi = mat;
    }
    
    virtual void CalcPoint (const IntegrationPoint & ip,
			    FlatVector<> point) const
    {
      point = p0 + mat * FlatVec<DIMS, const double> (&ip(0));
    }

    virtual void CalcPointJacobian (const IntegrationPoint & ip,
				    FlatVector<> point, FlatMatrix<> dxdxi) const
    {
      point = p0 + mat * FlatVec<DIMS, const double> (&ip(0));
      dxdxi = mat;
    }

    virtual BaseMappedIntegrationPoint & operator() (const IntegrationPoint & ip, LocalHeap & lh) const
    {
      return *new (lh) MappedIntegrationPoint<DIMS,DIMR> (ip, *this);
    }

    virtual BaseMappedIntegrationRule & operator() (const IntegrationRule & ir, LocalHeap & lh) const
    {
      return *new (lh) MappedIntegrationRule<DIMS,DIMR> (ir, *this, lh);
    }    

    virtual void CalcMultiPointJacobian (const IntegrationRule & ir,
					 BaseMappedIntegrationRule & bmir) const
    {
      MappedIntegrationRule<DIMS,DIMR> & mir = static_cast<MappedIntegrationRule<DIMS,DIMR> &> (bmir);
      for (int i = 0; i < ir.Size(); i++)
        {
          const IntegrationPoint & ip = ir[i];
          mir[i].Point() = p0 + mat * FlatVec<DIMS, const double> (&ip(0));
          mir[i].Jacobian() = mat;
          mir[i].Compute();
        }
    }
  };
  














  MeshAccess :: MeshAccess (shared_ptr<netgen::Mesh> amesh)
    : mesh(amesh)
  {
    // the connection to netgen global variables
    ngstd::testout = netgen::testout;
    ngstd::printmessage_importance = netgen::printmessage_importance;

    mesh.SelectMesh();
    mesh.UpdateTopology();  // for netgen/ngsolve stand alone
    UpdateBuffers();
  }


  MeshAccess :: ~MeshAccess ()
  {
    // delete mesh;
    // Ng_LoadGeometry("");
  }


  void MeshAccess :: LoadMesh (const string & filename)
  {
    mesh.LoadMesh (filename);
    UpdateBuffers();
    if (!mesh.Valid())
      throw Exception ("could not load mesh from '" + filename + "'");
  }

  void MeshAccess :: LoadMesh (istream & str)
  {
    mesh.LoadMesh (str);
    UpdateBuffers();
  }

  void MeshAccess :: SaveMesh (ostream & str) const
  {
    mesh.SaveMesh (str);
  }

  void MeshAccess :: ArchiveMesh (Archive & archive) 
  {
    mesh.DoArchive (archive);
    if (archive.Input()) UpdateBuffers();
  }

  void MeshAccess :: SelectMesh() const
  {
    mesh.SelectMesh();
  }


  /*
  void MeshAccess :: LoadMeshFromString(const string & str)
  {
    Ng_LoadMeshFromString(const_cast<char*>(str.c_str()));
    UpdateBuffers();
  }
  */


  void MeshAccess :: UpdateBuffers()
  {
    if (!mesh.Valid())
      {
        for (int i = 0; i < 4; i++)  
          {
            nnodes[i] = 0;
            nelements[i] = 0;
            nnodes_cd[i] = 0;
            nelements_cd[i] = 0;
          }
        dim = -1;
        ne_vb[VOL] = ne_vb[BND] = 0;
        return;
      }

    dim = mesh.GetDimension();
    nlevels = mesh.GetNLevels(); 

    if (MyMPI_GetNTasks() > 1 && MyMPI_GetId() == 0)
      {
        for (int i = 0; i < 4; i++)  
          {
            nnodes[i] = 0;
            nelements[i] = 0;
            nnodes_cd[i] = 0;
            nelements_cd[i] = 0;
          }
        ne_vb[VOL] = ne_vb[BND] = 0;
      }
    else
      {
	for (int i = 0; i < 4; i++)  
	  {
	    nnodes[i] = mesh.GetNNodes(i);
	    nelements[i] = mesh.GetNElements(i);
	  }
	for (int i = 0; i <= dim; i++)
	  {
	    nnodes_cd[i] = nnodes[dim-i];
	    nelements_cd[i] = nelements[dim-i];
	  }
        ne_vb[VOL] = nelements_cd[0];
        ne_vb[BND] = nelements_cd[1];
      }

    ndomains = -1;
    int ne = GetNE(); 
    for (int i = 0; i < ne; i++)
      ndomains = max2(ndomains, GetElIndex(i));

    ndomains++;
    ndomains = MyMPI_AllReduce (ndomains, MPI_MAX);

    nboundaries = -1;
    int nse = GetNSE(); 
    for (int i = 0; i < nse; i++)
      nboundaries = max2(nboundaries, GetSElIndex(i));

    nboundaries++;
    nboundaries = MyMPI_AllReduce (nboundaries, MPI_MAX);
  }


#ifdef ABC
  void MeshAccess::GetTopologicElement (int elnr, TopologicElement & topel) const
  {
    int help[54];
    int nnodes;

    nnodes = Ng_GetElementClosureNodes (dim, elnr, 
                                        NodeSet (NT_VERTEX, NT_EDGE, NT_FACE, NT_CELL), help);
        
    topel.SetElementType (GetElType(elnr));
    topel.Clear();
    for (int i = 0; i < nnodes; i++)
      topel.AddNode (Node (NODE_TYPE (help[2*i]), help[2*i+1]));
    
    /*    
    // 2D case
    ArrayMem<int, 12> nums;
    
    topel.SetElementType (GetElType(elnr));
    topel.Clear();

    GetElVertices (elnr, nums);
    for (int j = 0; j < nums.Size(); j++)
    topel.AddNode (Node (NT_VERTEX, nums[j]));

    GetElEdges (elnr, nums);
    for (int j = 0; j < nums.Size(); j++)
    topel.AddNode (Node (NT_EDGE, nums[j]));

    if (GetDimension() == 3)
    {
    GetElFaces (elnr, nums);
    for (int j = 0; j < nums.Size(); j++)
    topel.AddNode (Node (NT_FACE, nums[j]));
        
    topel.AddNode (Node (NT_CELL, elnr));
    }
    else
    topel.AddNode (Node (NT_FACE, elnr));
    */
  }
#endif


#ifdef ABC
  void MeshAccess :: GetSegmentPNums (int snr, Array<int> & pnums) const
  {
    pnums = GetElement<1> (snr).Points();
    /*
    pnums.SetSize(3);
    int np;
    Ng_GetSegment (snr+1, &pnums[0], &np);
    pnums.SetSize(np);

    for (int i = 0; i < np; i++)
      pnums[i]--;
    */
  }


  int MeshAccess :: GetSegmentIndex (int snr) const
  {
    return mesh -> GetElementIndex<1> (snr);
    //return Ng_GetSegmentIndex(snr+1);
  }
#endif


  void MeshAccess :: 
  GetElEdges (int elnr, Array<int> & ednums, Array<int> & orient) const
  {
    ednums.SetSize (12);
    orient.SetSize (12);
    int ned = 
      Ng_GetElement_Edges (elnr+1, &ednums[0], &orient[0]);
    ednums.SetSize (ned);
    orient.SetSize (ned);
    for (int i = 0; i < ned; i++)
      ednums[i]--;
  }

  void MeshAccess :: 
  GetSElEdges (int selnr, Array<int> & ednums, Array<int> & orient) const
  {
    ednums.SetSize (4);
    orient.SetSize (4);
    int ned = 
      Ng_GetSurfaceElement_Edges (selnr+1, &ednums[0], &orient[0]);
    ednums.SetSize (ned);
    orient.SetSize (ned);

    for (int i = 0; i < ned; i++)
      ednums[i]--;
  }


  void MeshAccess :: GetEdgeElements (int enr, Array<int> & elnums) const
  {
    elnums.SetSize(0);
    ArrayMem<int,3> pnums;
    ArrayMem<int,50> velems0, velems1; 
    GetEdgePNums(enr, pnums);
    GetVertexElements(pnums[0], velems0);
    GetVertexElements(pnums[1], velems1);
  
    // now compare
    for (int i=0; i<velems0.Size(); i++) 
      for (int j=0; j<velems1.Size(); j++) 
	if (velems0[i] == velems1[j]) 
	  {
	    elnums.Append(velems0[i]);
	    continue;
	  }
  }

  void MeshAccess :: GetEdgeSurfaceElements (int enr, Array<int> & elnums) const
  {
    elnums.SetSize(0);
    ArrayMem<int,3> pnums;
    ArrayMem<int,50> velems0, velems1; 
    GetEdgePNums(enr, pnums);
    GetVertexSurfaceElements(pnums[0], velems0);
    GetVertexSurfaceElements(pnums[1], velems1);
  
    // now compare
    for (int i=0; i<velems0.Size(); i++) 
      for (int j=0; j<velems1.Size(); j++) 
	if (velems0[i] == velems1[j]) 
	  {
	    elnums.Append(velems0[i]);
	    continue;
	  }
  }

  

  void MeshAccess :: 
  GetElFaces (int elnr, Array<int> & fnums, Array<int> & orient) const
  {
    fnums.SetSize (6);
    orient.SetSize (6);
    int nfa = 
      Ng_GetElement_Faces (elnr+1, &fnums[0], &orient[0]);
    fnums.SetSize (nfa);
    orient.SetSize (nfa);

    for (int i = 0; i < nfa; i++)
      fnums[i]--;
  }

  int MeshAccess :: 
  GetSElFace (int selnr) const
  {
    return Ng_GetSurfaceElement_Face (selnr+1, 0)-1;
  }
  
  void MeshAccess :: 
  GetSElFace (int selnr, int & fnum, int & orient) const
  {
    fnum = Ng_GetSurfaceElement_Face (selnr+1, &orient);
    fnum--;
  }

  void MeshAccess :: GetFacePNums (int fnr, Array<int> & pnums) const
  {
    pnums.SetSize(4);
    int nv = Ng_GetFace_Vertices (fnr+1, &pnums[0]);
    pnums.SetSize(nv);
    for (int i = 0; i < nv; i++) pnums[i]--;
  }

 
  void MeshAccess :: GetFaceEdges (int fnr, Array<int> & edges) const
  {
    edges.SetSize(4);
    int ned = Ng_GetFace_Edges (fnr+1, &edges[0]);
    edges.SetSize(ned);
    for (int i = 0; i < ned; i++) edges[i]--;
  }
 

  void MeshAccess :: GetFaceElements (int fnr, Array<int> & elnums) const
  {
    ArrayMem<int, 9> vnums;
    GetFacePNums(fnr, vnums);

    ArrayMem<int, 50> vels;
    GetVertexElements (vnums[0], vels);

    int faces[8];
    elnums.SetSize (0);
    for (int i = 0; i < vels.Size(); i++)
      {
	int nfa = Ng_GetElement_Faces (vels[i]+1, faces, 0);
	for (int j = 0; j < nfa; j++)
	  if (faces[j]-1 == fnr)
	    elnums.Append (vels[i]);
      }
  }


  void MeshAccess :: GetFaceSurfaceElements (int fnr, Array<int> & elnums) const
  {
    ArrayMem<int, 9> vnums;
    GetFacePNums(fnr, vnums);

    ArrayMem<int, 50> vels;
    GetVertexSurfaceElements (vnums[0], vels);

    elnums.SetSize (0);
    for (int i = 0; i < vels.Size(); i++)
      {
	int sface = Ng_GetSurfaceElement_Face (vels[i]+1)-1;
        if (sface == fnr)
          elnums.Append (vels[i]);
      }
  }



  void MeshAccess :: GetEdgePNums (int enr, int & pn1, int & pn2) const
  {
    int v2[2];
    Ng_GetEdge_Vertices (enr+1, v2);
    pn1 = v2[0]-1;
    pn2 = v2[1]-1;
  }

  void MeshAccess :: GetEdgePNums (int enr, Array<int> & pnums) const
  {
    pnums.SetSize(2);
    Ng_GetEdge_Vertices (enr+1, &pnums[0]);
    pnums[0] -= 1;
    pnums[1] -= 1;
  }

  // some utility for Facets
  void MeshAccess :: GetElFacets (int elnr, Array<int> & fnums) const
  {
    switch (dim)
      {
      case 1: fnums = GetElement<1,VOL> (elnr).Vertices(); break;
      case 2: fnums = GetElement<2,VOL> (elnr).Edges(); break;
      default:
        fnums = GetElement<3,VOL> (elnr).Faces();
      }
  } 
    
  void MeshAccess :: GetSElFacets (int selnr, Array<int> & fnums) const
  {
    if (dim == 2) 
      GetSElEdges(selnr, fnums);
    else
      {
	fnums.SetSize(1);
	fnums[0] = GetSElFace(selnr);
      }
  }

  void MeshAccess :: GetFacetPNums (int fnr, Array<int> & pnums) const
  {
    switch (dim)
      {
      case 1: pnums.SetSize(1); pnums[0] = fnr; break;
      case 2: GetEdgePNums(fnr, pnums); break;
      case 3: GetFacePNums(fnr, pnums); break;
      }
  }

  ELEMENT_TYPE MeshAccess :: GetFacetType (int fnr) const
  {
    switch (dim)
      {
      case 1: return ET_POINT; 
      case 2: return ET_SEGM;
      default:  // i.e. dim = 3
	ArrayMem<int, 4> pnums;
	GetFacePNums(fnr, pnums);
	return (pnums.Size() == 3) ? ET_TRIG : ET_QUAD;
      }
  }

  void MeshAccess::PushStatus (const char * str) const
  { 
    Ng_PushStatus (str);
  }
  void MeshAccess::PopStatus () const
  { 
    Ng_PopStatus ();
  }
  void MeshAccess::SetThreadPercentage (double percent) const
  { 
    Ng_SetThreadPercentage (percent); 
  }
  void MeshAccess :: GetStatus (string & str, double & percent) const
  {
    char * s;
    Ng_GetStatus(&s,percent);
    str = s;
  }

  void MeshAccess :: SetTerminate(void) const
  {
    Ng_SetTerminate();
  }
  void MeshAccess :: UnSetTerminate(void) const
  {
    Ng_UnSetTerminate();
  }
  bool MeshAccess :: ShouldTerminate(void) const
  {
    return (Ng_ShouldTerminate() != 0);
  }


  void MeshAccess :: GetVertexElements (int vnr, Array<int> & elnrs) const
  {
    int nel = Ng_GetNVertexElements (vnr+1);
    elnrs.SetSize (nel);
    Ng_GetVertexElements (vnr+1, &elnrs[0]);
    for (int j = 0; j < nel; j++)
      elnrs[j]--;
  }




  ElementTransformation & MeshAccess :: GetTrafo (int elnr, bool boundary, LocalHeap & lh) const

  {
    // static Timer t("GetTrafo"); RegionTimer reg(t);

    ElementTransformation * eltrans;
    
    if (!boundary)
      {
        if (deformation)
          {
            switch (dim)
              {
              case 1: eltrans = new (lh) ALE_ElementTransformation<1,1> (this, deformation.get(), lh); break;
              case 2: eltrans = new (lh) ALE_ElementTransformation<2,2> (this, deformation.get(), lh); break;
              case 3: eltrans = new (lh) ALE_ElementTransformation<3,3> (this, deformation.get(), lh); break;
              default:
                throw Exception ("MeshAccess::GetTrafo, illegal dimension");
              }
          }

        else if (Ng_IsElementCurved (elnr+1))
          {
            switch (dim)
              {
              case 1: eltrans = new (lh) Ng_ElementTransformation<1,1> (this); break;
              case 2: eltrans = new (lh) Ng_ElementTransformation<2,2> (this); break;
              case 3: eltrans = new (lh) Ng_ElementTransformation<3,3> (this); break;
              default:
                throw Exception ("MeshAccess::GetTrafo, illegal dimension");
              }
          }
        else
          {
            switch (dim)
              {
              case 1: eltrans = new (lh) Ng_ConstElementTransformation<1,1> (this); break;
              case 2: eltrans = new (lh) Ng_ConstElementTransformation<2,2> (this); break;
              case 3: eltrans = new (lh) Ng_ConstElementTransformation<3,3> (this); break;
              default:
                throw Exception ("MeshAccess::GetTrafo, illegal dimension");
              }
          }

	eltrans->SetElementType (GetElType(elnr));
        int elind = GetElIndex (elnr);
	eltrans->SetElement (0, elnr, elind);
      
	if(higher_integration_order.Size() == GetNE() && higher_integration_order[elnr])
	  eltrans->SetHigherIntegrationOrder();
	else
	  eltrans->UnSetHigherIntegrationOrder();
      }
    else
      {
        if (Ng_IsSurfaceElementCurved (elnr+1))
          {
            switch (dim)
              {
              case 1: eltrans = new (lh) Ng_ElementTransformation<0,1> (this); break;
              case 2: eltrans = new (lh) Ng_ElementTransformation<1,2> (this); break;
              case 3: eltrans = new (lh) Ng_ElementTransformation<2,3> (this); break;
              default:
                throw Exception ("MeshAccess::GetTrafo, illegal dimension");
              }
          }
        else
          {
            switch (dim)
              {
              case 1: eltrans = new (lh) Ng_ConstElementTransformation<0,1> (this); break;
              case 2: eltrans = new (lh) Ng_ConstElementTransformation<1,2> (this); break;
              case 3: eltrans = new (lh) Ng_ConstElementTransformation<2,3> (this); break;
              default:
                throw Exception ("MeshAccess::GetTrafo, illegal dimension");
              }
          }
        
        int elind = GetSElIndex (elnr);
	eltrans->SetElementType (GetSElType(elnr));
	eltrans->SetElement (1, elnr, elind);
      }

    return *eltrans;
  }


  double MeshAccess :: ElementVolume (int elnr) const
  {
    static FE_Segm0 segm0;
    static ScalarFE<ET_TRIG,0> trig0;
    static ScalarFE<ET_QUAD,0> quad0;
    static ScalarFE<ET_TET,0> tet0;
    static FE_Prism0 prism0;
    static FE_Pyramid0 pyramid0;
    FE_Hex0 hex0;
  
    const FiniteElement * fe = NULL;
    switch (GetElType (elnr))
      {
      case ET_SEGM: fe = &segm0; break;
      case ET_TRIG: fe = &trig0; break;
      case ET_QUAD: fe = &quad0; break;
      case ET_TET: fe = &tet0; break;
      case ET_PYRAMID: fe = &pyramid0; break;
      case ET_PRISM: fe = &prism0; break;
	// case ET_HEX: fe = &hex0; break;
      default:
	{
	  cerr << "ElementVolume not implemented for el " << GetElType(elnr) << endl;
	}
      }
  
    LocalHeapMem<10000> lh("MeshAccess - elementvolume");

    ElementTransformation & trans = GetTrafo (elnr, false, lh);
    ConstantCoefficientFunction ccf(1);

    if (GetDimension() == 1)
      {
	SourceIntegrator<1> si (&ccf);
	FlatVector<> elvec(fe->GetNDof(), lh);
	si.CalcElementVector (*fe, trans, elvec, lh);
	return elvec(0);
      }
    else if (GetDimension() == 2)
      {
	SourceIntegrator<2> si (&ccf);
	FlatVector<> elvec(fe->GetNDof(), lh);
	si.CalcElementVector (*fe, trans, elvec, lh);
	return elvec(0);
      }
    else
      {
	SourceIntegrator<3> si(&ccf);
	FlatVector<> elvec(fe->GetNDof(), lh);
	si.CalcElementVector (*fe, trans, elvec, lh);
	return elvec(0);
      }
  }


  
  double MeshAccess :: SurfaceElementVolume (int selnr) const
  {
    static ScalarFE<ET_TRIG,0> trig0;
    static ScalarFE<ET_QUAD,0> quad0;

    const FiniteElement * fe;
    switch (GetSElType (selnr))
      {
      case ET_TRIG: fe = &trig0; break;
      case ET_QUAD: fe = &quad0; break;
      default:
	{
	  cerr << "SurfaceElementVolume not implemented for el " << GetElType(selnr) << endl;
	  return 0;
	}
      }

    LocalHeapMem<10000> lh("MeshAccess - surfaceelementvolume");

    ElementTransformation & trans = GetTrafo (selnr, true, lh);
    ConstantCoefficientFunction ccf(1);

    if (GetDimension() == 2)
      {
	NeumannIntegrator<2> si( &ccf );
	FlatVector<> elvec (fe->GetNDof(), lh);
	si.CalcElementVector (*fe, trans, elvec, lh);
	return elvec(0);
      }
    else
      {
	NeumannIntegrator<3> si( &ccf );
	FlatVector<> elvec (fe->GetNDof(), lh);
	si.CalcElementVector (*fe, trans, elvec, lh);
	return elvec(0);
      }
  }


  void MeshAccess :: SetPointSearchStartElement(const int el) const
  {
    Ng_SetPointSearchStartElement(el+1);
  }

  int MeshAccess :: FindElementOfPoint (FlatVector<double> point,
					IntegrationPoint & ip, 
					bool build_searchtree,
					int index) const
  {
    ArrayMem<int,1> dummy(1);
    dummy[0] = index;
    return FindElementOfPoint(point,ip,build_searchtree,&dummy);
  }

  int MeshAccess :: FindElementOfPoint (FlatVector<double> point,
					IntegrationPoint & ip,
					bool build_searchtree,
					const Array<int> * const indices) const
  {
    static Timer t("FindElementOfPonit");
    RegionTimer reg(t);


    if (indices != NULL)
      {
        switch (dim)
          {
          case 2:
            return mesh.FindElementOfPoint<2> (&point(0), &ip(0), build_searchtree, 
                                               &(*indices)[0],indices->Size());
          case 3:
            return mesh.FindElementOfPoint<3> (&point(0), &ip(0), build_searchtree,
                                               &(*indices)[0],indices->Size());
          }
      }
    else
      {  
        switch (dim)
          {
          case 2: return mesh.FindElementOfPoint<2> (&point(0), &ip(0), build_searchtree, NULL, 0);
          case 3: return mesh.FindElementOfPoint<3> (&point(0), &ip(0), build_searchtree, NULL, 0);
          }
      }

    return -1;
  }


  int MeshAccess :: FindSurfaceElementOfPoint (FlatVector<double> point,
					       IntegrationPoint & ip, 
					       bool build_searchtree,
					       int index) const
  {
    ArrayMem<int,1> dummy(1);
    dummy[0] = index;
    return FindSurfaceElementOfPoint(point,ip,build_searchtree,&dummy);
  }

  int MeshAccess :: FindSurfaceElementOfPoint (FlatVector<double> point,
					       IntegrationPoint & ip,
					       bool build_searchtree,
					       const Array<int> * const indices) const
  {
    static Timer t("FindSurfaceElementOfPonit");
    RegionTimer reg(t);
    int elnr;
    double lami[3];
    if(indices != NULL && indices->Size()>0)
      {
	int * dummy = new int[indices->Size()];
	for(int i=0; i<indices->Size(); i++) dummy[i] = (*indices)[i]+1;

	elnr = Ng_FindSurfaceElementOfPoint (&point(0), lami, build_searchtree,dummy,indices->Size());

	delete [] dummy;
      }
    else
      {  
	elnr = Ng_FindSurfaceElementOfPoint (&point(0), lami, build_searchtree);
      }

    if (elnr == 0) return -1;

    for (int k = 0; k < 3; k++)
      ip(k) = lami[k];


    return elnr-1;
  }


  int MeshAccess :: GetNPairsPeriodicVertices () const 
  {
    return Ng_GetNPeriodicVertices(0);
  }

  int MeshAccess :: GetNPairsPeriodicVertices (int idnr) const 
  {
    return Ng_GetNPeriodicVertices(idnr);
  }
 
  void MeshAccess :: GetPeriodicVertices ( Array<INT<2> > & pairs) const
  {
    int npairs;

    npairs = Ng_GetNPeriodicVertices (0);
    pairs.SetSize (npairs);

    Ng_GetPeriodicVertices (0,&pairs[0][0]);
    for (int i = 0; i < pairs.Size(); i++)
      {
	pairs[i][0]--;
	pairs[i][1]--;
      }
  }
 
  void MeshAccess :: GetPeriodicVertices (int idnr, Array<INT<2> > & pairs) const
  {
    int npairs;

    npairs = Ng_GetNPeriodicVertices (idnr);

    pairs.SetSize (npairs);

    Ng_GetPeriodicVertices (idnr,&pairs[0][0]);

    for (int i = 0; i < pairs.Size(); i++)
      {
	pairs[i][0]--;
	pairs[i][1]--;
      }
  }




  int MeshAccess :: GetNPairsPeriodicEdges () const 
  {
    return Ng_GetNPeriodicEdges(0);
  }
 
  void MeshAccess :: GetPeriodicEdges ( Array<INT<2> > & pairs) const
  {
    int npairs;

    npairs = Ng_GetNPeriodicEdges (0);
    pairs.SetSize (npairs);

    Ng_GetPeriodicEdges (0,&pairs[0][0]);
    for (int i = 0; i < pairs.Size(); i++)
      {
	pairs[i][0]--;
	pairs[i][1]--;
      }
  }

  int MeshAccess :: GetNPairsPeriodicEdges (int idnr) const 
  {
    return Ng_GetNPeriodicEdges(idnr);
  }
 
  void MeshAccess :: GetPeriodicEdges (int idnr, Array<INT<2> > & pairs) const
  {
    int npairs;

    npairs = Ng_GetNPeriodicEdges (idnr);
    pairs.SetSize (npairs);

    Ng_GetPeriodicEdges (idnr,&pairs[0][0]);
    for (int i = 0; i < pairs.Size(); i++)
      {
	pairs[i][0]--;
	pairs[i][1]--;
      }
  }


///// Added by Roman Stainko ....
void MeshAccess::GetVertexSurfaceElements( int vnr, Array<int>& elems) const
{
  int nel = Ng_GetVertex_NSurfaceElements( vnr+1 );
  elems.SetSize( nel );
  
  Ng_GetVertex_SurfaceElements( vnr+1, &elems[0] );

  for( int i=0; i<nel; i++ )
    elems[i]--;
}



  void MeshAccess::SetHigherIntegrationOrder(int elnr)
  {
    if(higher_integration_order.Size() != GetNE())
      {
	higher_integration_order.SetSize(GetNE());
	higher_integration_order = false;
      }
    higher_integration_order[elnr] = true;
  }
  void MeshAccess::UnSetHigherIntegrationOrder(int elnr)
  {
    if(higher_integration_order.Size() != GetNE())
      {
	higher_integration_order.SetSize(GetNE());
	higher_integration_order = false;
      }
    higher_integration_order[elnr] = false;
  }


  void MeshAccess :: InitPointCurve(double red, double green, double blue) const
  {
    Ng_InitPointCurve(red, green, blue);
  }

  void MeshAccess :: AddPointCurvePoint(const Vec<3> & point) const
  {
    Ng_AddPointCurvePoint(&(point(0)));
  }

 

#ifdef PARALLEL
 
  int MeshAccess ::GetGlobalNodeNum (Node node) const
  {
    int glob = NgPar_GetGlobalNodeNum (node.GetType(), node.GetNr());
    return glob;
  }
  
  void MeshAccess :: GetDistantProcs (Node node, Array<int> & procs) const
  {
    procs.SetSize( NgPar_GetNDistantNodeNums(node.GetType(), node.GetNr()) );
    NgPar_GetDistantNodeNums ( node.GetType(), node.GetNr(), &procs[0] );
  }


#else

  int MeshAccess ::GetGlobalNodeNum (Node node) const  
  {
    return -1;
  }

  void MeshAccess :: GetDistantProcs (Node node, Array<int> & procs) const
  {
    procs.SetSize (0);
  }
#endif






  
  ProgressOutput :: ProgressOutput (shared_ptr<MeshAccess> ama,
				    string atask, int atotal)
    : ma(ama), task(atask), total(atotal)
  {
    is_root = (MyMPI_GetId() == 0);
    prevtime = WallTime();
    int glob_total = MyMPI_Reduce (total);
    if (is_root) total = glob_total;

    done_called = false;
    cnt = 0;
  }

  ProgressOutput :: ~ProgressOutput ()
  {
    Done();
  }  

  void ProgressOutput :: Update ()
  {
#pragma omp atomic
    cnt++;
    Update(cnt);
  }

  void ProgressOutput :: Update (int nr)
  {
    double time = WallTime();
    if (time > prevtime+0.05)
      {
#pragma omp critical(progressupdate) 
	{
	  if (is_root)
	    {
	      cout << IM(3) << "\r" << task << " " << nr << "/" << total << flush;
	      ma->SetThreadPercentage ( 100.0*nr / total);
	    }
#ifdef PARALLEL
	  else
	    {
	      static Timer t("dummy - progressreport"); RegionTimer r(t);
	      MPI_Send (&nr, 1, MPI_INT, 0, MPI_TAG_SOLVE, ngs_comm);
              // changed from BSend (VSC-problem)
	    }
#endif
	  
	  prevtime = WallTime();
	}
      }
  }
  


  void ProgressOutput :: Done()
  {
    if (done_called) return;
    done_called = true;

    if (is_root)
      {
#ifdef PARALLEL	  
	int ntasks = MyMPI_GetNTasks();
	if (ntasks > 1)
	  {
	    Array<int> working(ntasks), computed(ntasks);
	    working = 1;
	    computed = 0;
	    while (1)
	      {
		int flag, data, num_working = 0, got_flag = false;
		for (int source = 1; source < ntasks; source++)
		  {
		    if (!working[source]) continue;
		    num_working++;
		    MPI_Iprobe (source, MPI_TAG_SOLVE, ngs_comm, &flag, MPI_STATUS_IGNORE);
		    if (flag)
		      {
			got_flag = true;
			MPI_Recv (&data, 1, MPI_INT, source, MPI_TAG_SOLVE, ngs_comm, MPI_STATUS_IGNORE);
			if (data == -1) 
			  working[source] = 0;
			else
			  computed[source] = data;
		      }
		  }
		int sum = 0;
		for (int j = 1; j < ntasks; j++) 
		  sum += computed[j];
		cout << IM(3) 
		     << "\r" << task << " " << sum << "/" << total
		     << " (" << num_working << " procs working) " << flush;
		ma->SetThreadPercentage ( 100.0*sum / total );
		if (!num_working) break;
		if (!got_flag) usleep (1000);
	      }
	  }
#endif
	cout << IM(3) << "\r" << task << " " << total << "/" << total
	     << "                                 " << endl;
      }
    else
      {
#ifdef PARALLEL
	MPI_Send (&total, 1, MPI_INT, 0, MPI_TAG_SOLVE, ngs_comm);
	int final = -1;
	MPI_Send (&final, 1, MPI_INT, 0, MPI_TAG_SOLVE, ngs_comm);
#endif
      }
  }
  


}




