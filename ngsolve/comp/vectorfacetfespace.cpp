/*********************************************************************/
/* File:   vectorfacetfespace.cpp                                    */
/* Author: A. Sinwel, Joachim Schoeberl                              */
/* Date:   2008                                                      */
/*********************************************************************/


#include <comp.hpp>
#include <fem.hpp>

#include <parallelngs.hpp>



namespace ngcomp
{
  using namespace ngfem;
  using namespace ngparallel;



  VectorFacetFESpace :: VectorFacetFESpace ( const MeshAccess & ama, const Flags & flags, 
		       bool parseflags )
    : FESpace(ama, flags )
  {
    name = "VectorFESpace";
    DefineNumFlag("relorder");
    DefineDefineFlag("print");
    DefineDefineFlag("variableorder");

    if ( parseflags) CheckFlags(flags);

    print = flags.GetDefineFlag("print");

    ndlevel.SetSize(0);
    Flags loflags;
    loflags.SetFlag("order", 0.0);
    if ( this->IsComplex() )
      loflags.SetFlag("complex");
    if (order != 0)
      low_order_space = new VectorFacetFESpace(ma, loflags);
    else
      low_order_space = 0;
        

    order =  int (flags.GetNumFlag ("order",0)); 
    
    if(flags.NumFlagDefined("relorder") && !flags.NumFlagDefined("order")) 
       var_order = 1; 
    else 
      var_order = 0;
    
    rel_order=int(flags.GetNumFlag("relorder",order-1)); 


    if(flags.NumFlagDefined("order") && flags.NumFlagDefined("relorder")) 
      {
	if(var_order)
	  cerr << " WARNING: VectorFacetFESpace: inconsistent flags: variableorder, order and relorder "
	       << "-> variable order space with rel_order " << rel_order << "is used, but order is ignored " << endl; 
	else 
	  cerr << " WARNING: VectorFacetFESpace: inconsistent flags: order and rel_order "
	       << "-> uniform order space with order " << order << " is used " << endl; 
      }

    if (flags.NumFlagDefined("order")) 
      { 
	if(var_order) 
	  { 
	    rel_order = int(flags.GetNumFlag("relorder",order-1)); 
	    order = rel_order + 1;
	  }
	else 
	  order =  int (flags.GetNumFlag ("order",0));
      }
    else if(flags.NumFlagDefined("relorder"))
      {
	var_order=1; 
	rel_order = int (flags.GetNumFlag ("relorder",-1));
	order=1+rel_order; 
      }
    else // neither rel_order nor order is given  
      {
	rel_order = -1;  
	order = 0;  
      }

    // Evaluator for shape tester 
    static ConstantCoefficientFunction one(1);
    if (ma.GetDimension() == 2)
      {
	Array<CoefficientFunction*> coeffs(1);
	coeffs[0] = &one;
	evaluator = GetIntegrators().CreateBFI("massvectorfacet", 2, coeffs);
	boundary_evaluator = GetIntegrators().CreateBFI("robinvectorfacet",2,coeffs); 
      }
    else if(ma.GetDimension() == 3) 
      {
	Array<CoefficientFunction*> coeffs(1); 
	coeffs[0] = &one;
	evaluator = GetIntegrators().CreateBFI("massvectorfacet",3,coeffs); 
	boundary_evaluator = GetIntegrators().CreateBFI("robinvectorfacet",3,coeffs); 
      }


    highest_order_dc = flags.GetDefineFlag("highest_order_dc");


    // Update();
  }
  

  FESpace * VectorFacetFESpace :: Create ( const MeshAccess & ma, const Flags & flags )
  {
    return new VectorFacetFESpace ( ma, flags, true);
  }

  void VectorFacetFESpace :: Update(LocalHeap& lh)
  {
    if ( print ) 
      *testout << "VectorFacetFESpace, order " << order << endl 
	       << "rel_order " << rel_order << ", var_order " << var_order << endl;

    if ( low_order_space ) 
      low_order_space -> Update(lh);

    nel = ma.GetNE();
    nfacets = (ma.GetDimension() == 2 ? ma.GetNEdges() : ma.GetNFaces()); 
 
    int p = 0; 
    if(!var_order) p = order; 
    
    order_facet.SetSize(nfacets);
    order_facet = INT<2> (p,p);
    fine_facet.SetSize(nfacets);
    fine_facet = 0; 
    
    Array<int> fanums;
        
    for (int i = 0; i < nel; i++)
      {
	INT<3> el_orders = ma.GetElOrders(i); 

	ELEMENT_TYPE eltype=ma.GetElType(i); 
	const POINT3D * points = ElementTopology :: GetVertices (eltype);	
	
	if (ma.GetDimension() == 2)
	  {
	    ma.GetElEdges (i, fanums);
	    for (int j=0;j<fanums.Size();j++) 
	      fine_facet[fanums[j]] = 1; 
	    
	    if(var_order)
	      {
		const EDGE * edges = ElementTopology::GetEdges (eltype);
		for(int j=0; j<fanums.Size(); j++)
		  for(int k=0;k<2;k++)
		    if(points[edges[j][0]][k] != points[edges[j][1]][k])
		      { 
			order_facet[fanums[j]] = INT<2>(max(el_orders[k]+rel_order, order_facet[fanums[j]][0]),0);
			break; 
		      }
	      }
	  }
	else
	  {
	    Array<int> elfaces,vnums;
	    ma.GetElFaces(i,elfaces);
	    for (int j=0;j<elfaces.Size();j++) fine_facet[elfaces[j]] = 1; 
	    
	    if(var_order) 
	      {
		ma.GetElVertices (i, vnums);
		const FACE * faces = ElementTopology::GetFaces (eltype);
		for(int j=0;j<elfaces.Size();j++)
		  {
		    if(faces[j][3]==-1) // trig  
		      {
			order_facet[elfaces[j]][0] = max(order_facet[elfaces[j]][0],el_orders[0]+rel_order);
			order_facet[elfaces[j]][1] = order_facet[elfaces[j]][0]; 
		      }
		    else //quad_face
		      {
			int fmax = 0;
			for(int k = 1; k < 4; k++) 
			  if(vnums[faces[j][k]] > vnums[faces[j][fmax]]) fmax = k;   
					
			INT<2> f((fmax+3)%4,(fmax+1)%4); 
			if(vnums[faces[j][f[1]]] > vnums[faces[j][f[0]]]) swap(f[0],f[1]);
			
			// fmax > f[0] > f[1]
			// p[0] for direction fmax,f[0] 
			// p[1] for direction fmax,f[1] 
			for(int l=0;l<2;l++)
			  for(int k=0;k<3;k++)
			    if(points[faces[j][fmax]][k] != points[faces[j][f[l] ]][k])
			      {
				order_facet[elfaces[j]][l] = max(order_facet[elfaces[j]][l], rel_order + el_orders[k]);
				break; 
			      } 
			
		      }
		  }
	      }
	    
	  }
	
      }
    
    // update dofs

    ncfacets = 0;
    ndof_lo = ( ma.GetDimension() == 2 ) ? nfacets : 2*nfacets;
    ndof = ndof_lo;

    first_facet_dof.SetSize(nfacets+1);
    first_facet_dof = ndof_lo;

    if ( ma.GetDimension() == 2 )
      {
	for ( int i = 0; i < nfacets; i++ )
	  {
	    first_facet_dof[i] = ndof;
	    ndof += order_facet[i][0];
	    if (highest_order_dc) ndof--;
	  }
	first_facet_dof[nfacets] = ndof;
	
	if (highest_order_dc)
	  {
	    int ne = ma.GetNE();
	    first_inner_dof.SetSize(ne+1);
	    for (int i = 0; i < ne; i++)
	      {
		first_inner_dof[i] = ndof;
		
		// only trigs supported:
		ndof += 3;
	      }
	    first_inner_dof[ne] = ndof;
	  }
      }

    else // 3D
      {
	int inci = 0;
	Array<int> pnums;

	for ( int i = 0; i < nfacets; i++ )
	  {
	    INT<2> p = order_facet[i];
	    if (highest_order_dc)
	      { p[0]--; p[1]--; }

	    ma.GetFacePNums(i, pnums);

	    inci = ( pnums.Size() == 3 ) ?
	      ( ((p[0]+1)*(p[0]+2)) - 2) :  ( 2 * (p[0]+1) * (p[1]+1) - 2 );

	    first_facet_dof[i] = ndof;
	    ndof += inci;
	  }
	first_facet_dof[nfacets] = ndof;


	if (highest_order_dc)
	  {
	    int ne = ma.GetNE();
	    first_inner_dof.SetSize(ne+1);
	    for (int i = 0; i < ne; i++)
	      {
		first_inner_dof[i] = ndof;
		switch (ma.GetElType(i))
		  {
		  case ET_TET: ndof += 4*(order+1)*2; break;
		  case ET_PRISM: ndof += 2 * (2*(order+1)+3*(2*order+1)); break;
		  default: throw Exception (string("VectorFacetFESpace: Element type not implemented"));
		  }
	      }
	    first_inner_dof[ne] = ndof;
	  }
      }

    while (ma.GetNLevels() > ndlevel.Size())
      ndlevel.Append (ndof);
    ndlevel.Last() = ndof;
      
      //no prolongation so far       
      //prol->Update();

    if(print)
      {
	*testout << "*** Update VectorFacetFESpace: General Information" << endl;
	*testout << " order facet (facet) " << order_facet << endl; 
	*testout << " first_facet_dof (facet)  " << first_facet_dof << endl; 
	*testout << " first_inner_dof (facet)  " << first_inner_dof << endl; 
      } 


    FinalizeUpdate (lh);

#ifdef PARALLEL
    *testout << "update parallel dofs in facet-fespace, ndof " << ndof << endl;
    UpdateParallelDofs();
    *testout << "juhu, my paralleldofs " << endl;
    paralleldofs -> Print();
#endif

  }

  const FiniteElement & VectorFacetFESpace :: GetFE ( int elnr, LocalHeap & lh ) const
  {
    VectorFacetVolumeFiniteElement<2> * fe2d = 0;
    VectorFacetVolumeFiniteElement<3> * fe3d = 0;
    switch (ma.GetElType(elnr))
      {
      case ET_TRIG: fe2d = new (lh)  VectorFacetVolumeTrig (); break;
      case ET_QUAD: fe2d = new (lh) VectorFacetVolumeQuad (); break;
      case ET_TET:  fe3d = new (lh) VectorFacetVolumeTet (); break;
      case ET_PYRAMID: fe3d = new (lh)  VectorFacetVolumePyramid (); break;
      case ET_PRISM: fe3d = new (lh)  VectorFacetVolumePrism (); break;
      case ET_HEX:   fe3d = new (lh) VectorFacetVolumeHex (); break;
      default:
        throw Exception (string("VectorFacetFESpace::GetFE: unsupported element ")+
                         ElementTopology::GetElementName(ma.GetElType(elnr)));
      }

    Array<int> vnums;
    ArrayMem<int, 6> fanums, order_fa;
    
    ma.GetElVertices(elnr, vnums);

    if (fe2d)
      {
	ma.GetElEdges(elnr, fanums);
    
	order_fa.SetSize(fanums.Size());
	for (int j = 0; j < fanums.Size(); j++)
	  order_fa[j] = order_facet[fanums[j]][0]; 
	
	fe2d -> SetVertexNumbers (vnums);
	fe2d -> SetOrder (order_fa);
	fe2d -> ComputeNDof();
	
	return *fe2d;
      }    
    else
      {
	ma.GetElFaces(elnr, fanums);
    
	order_fa.SetSize(fanums.Size());
	for (int j = 0; j < fanums.Size(); j++)
	  order_fa[j] = order_facet[fanums[j]][0]; 
	
	fe3d -> SetVertexNumbers (vnums);
	fe3d -> SetOrder (order_fa);
	fe3d -> ComputeNDof();
    
	return *fe3d;
      }
  }


  const FiniteElement & VectorFacetFESpace :: GetSFE ( int selnr, LocalHeap & lh ) const
  {
    
    VectorFacetFacetFiniteElement * fe = 0;

    switch (ma.GetSElType(selnr))
    {
      case ET_SEGM: fe = new (lh) VectorFacetFacetSegm (); break;
      case ET_TRIG: fe = new (lh) VectorFacetFacetTrig (); break;
      case ET_QUAD: fe = new (lh) VectorFacetFacetQuad (); break;
      default:
        fe = 0;
    }
     
    if (!fe)
    {
      stringstream str;
      str << "VectorFacetFESpace " << GetClassName()
          << ", undefined eltype "
          << ElementTopology::GetElementName(ma.GetSElType(selnr))
          << ", order = " << order << endl;
      throw Exception (str.str());
    }
     
    ArrayMem<int,4> vnums;
    ArrayMem<int, 4> ednums;
    
    ma.GetSElVertices(selnr, vnums);
    int reduceorder = highest_order_dc ? 1 : 0;
    switch (ma.GetSElType(selnr))
    {
      case ET_SEGM:
        fe -> SetVertexNumbers (vnums);
        ma.GetSElEdges(selnr, ednums);
        fe -> SetOrder (order_facet[ednums[0]][0]-reduceorder); 
        fe -> ComputeNDof();
        break;
      case ET_TRIG: 
      case ET_QUAD:
        fe -> SetVertexNumbers (vnums);
        fe -> SetOrder (order_facet[ma.GetSElFace(selnr)][0]-reduceorder);
        fe -> ComputeNDof();
        break;
    default:
      throw Exception ("VectorFacetFESpace::GetSFE: unsupported element");
    }
    return *fe;
    
  }

  void VectorFacetFESpace :: GetDofNrs(int elnr, Array<int> & dnums) const
  {
    if (!highest_order_dc)
      {
	Array<int> fanums; // facet numbers
	int first,next;
	
	fanums.SetSize(0);
	dnums.SetSize(0);
	
	
	if(ma.GetDimension() == 3)
	  ma.GetElFaces (elnr, fanums);
	else // dim=2
	  ma.GetElEdges (elnr, fanums);
	
	for(int i=0; i<fanums.Size(); i++)
	  {
	    if ( ma.GetDimension() == 2 )
	      dnums.Append(fanums[i]); // low_order
	    else
	      {
		dnums.Append(2*fanums[i]);
		dnums.Append(2*fanums[i]+1);
	      }
	    
	    first = first_facet_dof[fanums[i]];
	    next = first_facet_dof[fanums[i]+1];
	    for(int j=first ; j<next; j++)
	      dnums.Append(j);
	  }
      }
    else
      {
	if (ma.GetDimension() == 2)
	  {
	    Array<int> fanums; // facet numbers
	    
	    fanums.SetSize(0);
	    dnums.SetSize(0);
	    
	    ma.GetElEdges (elnr, fanums);
	    
	    for(int i=0; i<fanums.Size(); i++)
	      {
		int facetdof = first_facet_dof[fanums[i]];
		int innerdof = first_inner_dof[elnr];
		
		for (int j = 0; j <= order; j++)
		  {
		    if (j == 0)
		      {
			dnums.Append(fanums[i]);
		      }
		    else if (j < order)
		      {
			dnums.Append (facetdof++);
		      }
		    else
		      {
			dnums.Append (innerdof++);
		      }
		  }
	      }
	  }
	else
	  {
	    Array<int> fanums; // facet numbers
	    
	    fanums.SetSize(0);
	    dnums.SetSize(0);
	    
	    ELEMENT_TYPE et = ma.GetElType (elnr);
	    ma.GetElFaces (elnr, fanums);
	    
	    int innerdof = first_inner_dof[elnr];
	    for(int i=0; i<fanums.Size(); i++)
	      {
		ELEMENT_TYPE ft = ElementTopology::GetFacetType (et, i);
		
		int facetdof = first_facet_dof[fanums[i]];
		
		if (ft == ET_TRIG)
		  {
		    for (int j = 0; j <= order; j++)
		      for (int k = 0; k <= order-j; k++)
			{
			  if (j+k == 0)
			    {
			      dnums.Append(2*fanums[i]);
			      dnums.Append(2*fanums[i]+1);
			    }
			  else if (j+k < order)
			    {
			      dnums.Append (facetdof++);
			      dnums.Append (facetdof++);
			    }
			  else
			    {
			      dnums.Append (innerdof++);
			      dnums.Append (innerdof++);
			    }
			}
		  }
		else
		  {
		    for (int j = 0; j <= order; j++)
		      for (int k = 0; k <= order; k++)
			{
			  if (j+k == 0)
			    {
			      dnums.Append(2*fanums[i]);
			      dnums.Append(2*fanums[i]+1);
			    }
			  else if ( (j < order) && (k < order) )
			    {
			      dnums.Append (facetdof++);
			      dnums.Append (facetdof++);
			    }
			  else
			    {
			      dnums.Append (innerdof++);
			      dnums.Append (innerdof++);
			    }
			}
		  }
	      }
	  }
      }
    // *testout << "dnums = " << endl << dnums << endl;
  }

  void VectorFacetFESpace :: GetWireBasketDofNrs(int elnr, Array<int> & dnums) const
  {
    dnums.SetSize(0);
    Array<int> facets;

    if ( ma.GetDimension() == 2 )
      {
	ma.GetElEdges ( elnr, dnums );
      }
    else // 3D
      {
	ma.GetElFaces ( elnr, facets );
	for ( int i = 0; i < facets.Size(); i++ )
	  {
	    dnums.Append( 2 * facets[i] );
	    dnums.Append ( 2*facets[i] + 1 );
	  }
      }
    return;
  }

  ///
  void VectorFacetFESpace :: GetSDofNrs (int selnr, Array<int> & dnums) const
  {
    dnums.SetSize(0);
    ArrayMem<int, 1> fanums(1);
    int first, next;


    if ( ma.GetDimension() == 2 )
      {
	ma.GetSElEdges ( selnr, fanums );
	dnums.Append(fanums[0]);

	first = first_facet_dof[fanums[0]];
	next = first_facet_dof[fanums[0]+1];
	for ( int j = first; j < next; j++ )
	  dnums.Append(j);
      } 
    else // 3D
      {
	fanums[0] = ma.GetSElFace(selnr);
	dnums.Append( 2*fanums[0] );
	dnums.Append( 2*fanums[0]+1 );

	first = first_facet_dof[fanums[0]];
	next = first_facet_dof[fanums[0]+1];
	for ( int j = first; j < next; j++ )
	  dnums.Append(j);

      }
  }

  Table<int> * VectorFacetFESpace :: CreateSmoothingBlocks (const Flags & precflags) const
  { 
    return NULL;
  }

  Array<int> * VectorFacetFESpace :: CreateDirectSolverClusters (const Flags & precflags) const
  {
    return NULL;
  }
  
  void VectorFacetFESpace :: GetFacetDofNrs ( int felnr, Array<int> & dnums ) const
  {
    dnums.SetSize(0);
    if ( dimension == 3 )
      {
	dnums.Append( 2*felnr );
	dnums.Append (2*felnr+1);
      }
    else
      dnums.Append ( felnr );

    int first = first_facet_dof[felnr];
    int next = first_facet_dof[felnr+1];
    for ( int j = first; j < next; j++ )
      {
	dnums.Append(j);
      }
  }


  void VectorFacetFESpace :: GetInnerDofNrs ( int felnr, Array<int> & dnums ) const
  {
    dnums.SetSize(0);
  }

   int VectorFacetFESpace :: GetNFacetDofs ( int felnr ) const
  {
    // number of low_order_dofs = dimension - 1
    return ( first_facet_dof[felnr+1] - first_facet_dof[felnr] + dimension - 1);
  }

   void VectorFacetFESpace :: GetExternalDofNrs (int elnr, Array<int> & dnums) const
  {
    GetDofNrs ( elnr, dnums);
  }
  ///
   void VectorFacetFESpace :: GetVertexNumbers(int elnr, Array<int>& vnums) 
  { ma.GetElVertices(elnr, vnums); };
  ///
   INT<2> VectorFacetFESpace :: GetFacetOrder(int fnr) 
  { return order_facet[fnr]; };

   int VectorFacetFESpace :: GetFirstFacetDof(int fanr) const {return (first_facet_dof[fanr]);}; 


   void VectorFacetFESpace :: GetVertexDofNrs ( int elnum, Array<int> & dnums ) const
  {
    dnums.SetSize(0);
  }

   void VectorFacetFESpace :: GetEdgeDofNrs ( int elnum, Array<int> & dnums ) const
  {
    dnums.SetSize(0);
    if ( ma.GetDimension() == 3 )
      return;

    dnums.Append(elnum);
    for (int j=first_facet_dof[elnum]; j<first_facet_dof[elnum+1]; j++)
      dnums.Append(j);
  }

   void VectorFacetFESpace :: GetFaceDofNrs (int felnr, Array<int> & dnums) const
  {
    dnums.SetSize(0);
    if ( ma.GetDimension() == 2 ) return;

    dnums.Append( 2*felnr);
    dnums.Append (2*felnr+1);
    for (int j=first_facet_dof[felnr]; j<first_facet_dof[felnr+1]; j++)
      dnums.Append(j);
  }

#ifdef PARALLEL
   void VectorFacetFESpace :: UpdateParallelDofs_hoproc()
   { 
     // ******************************
     // update exchange dofs 
     // ******************************
    *testout << "VectorFacetFESpace::UpdateParallelDofs_hoproc" << endl;
    // Find number of exchange dofs
    Array<int> nexdof(ntasks);
    nexdof = 0;

    MPI_Status status;
    MPI_Request * sendrequest = new MPI_Request[ntasks];
    MPI_Request * recvrequest = new MPI_Request[ntasks];

    // number of face exchange dofs
    for ( int face = 0; face < ma.GetNFaces(); face++ )
      {
	if ( !parallelma->IsExchangeFace ( face ) ) continue;
	
	Array<int> dnums;
	GetFaceDofNrs ( face, dnums );
	nexdof[id] += dnums.Size() ; 

	for ( int dest = 1; dest < ntasks; dest ++ )
	  if (  parallelma -> GetDistantFaceNum ( dest, face ) >= 0 )
	    nexdof[dest] += dnums.Size() ; 
      }

    // + number of inner exchange dofs --> set eliminate internal

    nexdof[0] = ma.GetNFaces() * 2;

    paralleldofs->SetNExDof(nexdof);

//     paralleldofs->localexchangedof = new Table<int> (nexdof);
//     paralleldofs->distantexchangedof = new Table<int> (nexdof);
    paralleldofs->sorted_exchangedof = new Table<int> (nexdof);

    Array<int> ** owndofs, ** distantdofs;
    owndofs = new Array<int>* [ntasks];
    distantdofs = new Array<int>* [ntasks];

    for ( int i = 0; i < ntasks; i++ )
      {
	owndofs[i] = new Array<int>(1);
	(*owndofs[i])[0] = ndof;
	distantdofs[i] = new Array<int>(0);
      }

    Array<int> cnt_nexdof(ntasks);
    cnt_nexdof = 0;
    int exdof = 0;
    int ii;

    // *****************
    // Parallel Face dofs
    // *****************


   for ( int face = 0; face < ma.GetNFaces(); face++ )
      if ( parallelma->IsExchangeFace ( face ) )
      {
	Array<int> dnums;
	GetFaceDofNrs ( face, dnums );
	if ( dnums.Size() == 0 ) continue;

 	for ( int i=0; i<dnums.Size(); i++ )
 	  (*(paralleldofs->sorted_exchangedof))[id][exdof++] = dnums[i];

	for ( int dest = 1; dest < ntasks; dest++ )
	  {
	    int distface = parallelma -> GetDistantFaceNum ( dest, face );
	    if( distface < 0 ) continue;

	    owndofs[dest]->Append ( distface );
	    owndofs[dest]->Append (int(paralleldofs->IsGhostDof(dnums[0])) );
	    for ( int i=0; i<dnums.Size(); i++)
	      {
		paralleldofs->SetExchangeDof ( dest, dnums[i] );
		paralleldofs->SetExchangeDof ( dnums[i] );
		owndofs[dest]->Append ( dnums[i] );
	      }
  	  }
      }   


    for ( int sender = 1; sender < ntasks; sender ++ )
      {
        if ( id == sender )
          for ( int dest = 1; dest < ntasks; dest ++ )
            if ( dest != id )
              {
		MyMPI_ISend ( *owndofs[dest], dest, sendrequest[dest]);
              }
	  
        if ( id != sender )
          {
	    MyMPI_IRecv ( *distantdofs[sender], sender, recvrequest[sender]);
          }
 	  
	  
      }

     for( int dest=1; dest<ntasks; dest++) 
      {
	if ( dest == id ) continue;
	MPI_Wait(recvrequest+dest, &status);
	paralleldofs -> SetDistNDof( dest, (*distantdofs[dest])[0]) ;
	// low order dofs first
	ii = 1;
	while ( ii < distantdofs[dest]->Size() )
	  {
	    int fanum = (*distantdofs[dest])[ii++];
	    int isdistghost = (*distantdofs[dest])[ii++];
	    Array<int> dnums;
	    GetFaceDofNrs (fanum, dnums);
// 	    (*(paralleldofs->localexchangedof))[dest][ cnt_nexdof[dest] ] = dnums[0];
// 	    (*(paralleldofs->distantexchangedof))[dest][ cnt_nexdof[dest] ] = (*distantdofs[dest])[ii];
	    (*(paralleldofs->sorted_exchangedof))[dest][ cnt_nexdof[dest] ] = dnums[0];
	    cnt_nexdof[dest]++;
// 	    (*(paralleldofs->localexchangedof))[dest][ cnt_nexdof[dest] ] = dnums[1];
// 	    (*(paralleldofs->distantexchangedof))[dest][ cnt_nexdof[dest] ] = (*distantdofs[dest])[ii+1];
	    (*(paralleldofs->sorted_exchangedof))[dest][ cnt_nexdof[dest] ] = dnums[1];
	    if ( dest < id && !isdistghost )
	      {
		paralleldofs->ismasterdof.Clear ( dnums[0] ) ;
		paralleldofs->ismasterdof.Clear ( dnums[1] ) ;
	      }
	    ii += dnums.Size();
	    cnt_nexdof[dest]++;
	  }
	// then the high order dofs
	ii = 1;
	while ( ii < distantdofs[dest]->Size() )
	  {
	    int fanum = (*distantdofs[dest])[ii++];
	    int isdistghost = (*distantdofs[dest])[ii++];
	    Array<int> dnums;
	    GetFaceDofNrs (fanum, dnums);
	    ii += 2; 
	    for ( int i=2; i<dnums.Size(); i++)
	      {
// 		(*(paralleldofs->localexchangedof))[dest][ cnt_nexdof[dest] ] = dnums[i];
// 		(*(paralleldofs->distantexchangedof))[dest][ cnt_nexdof[dest] ] = (*distantdofs[dest])[ii];
		(*(paralleldofs->sorted_exchangedof))[dest][ cnt_nexdof[dest] ] = dnums[i];
		if ( dest < id && !isdistghost )
		  paralleldofs->ismasterdof.Clear ( dnums[i] ) ;
		ii++; cnt_nexdof[dest]++;
	      }
	  }
      }



     /*******************************

         update low order space

     *****************************/

     if ( order == 0 ) return;

     int ndof_lo = 2*ma.GetNFaces();

     // all dofs are exchange dofs
     nexdof = ndof_lo;
     
     exdof = 0;
     cnt_nexdof = 0;
     

     // *****************
     // Parallel Face dofs
     // *****************
     
     owndofs[0]->SetSize(1);
     (*owndofs[0])[0] = ndof;
     distantdofs[0]->SetSize(0);
     
     // find local and distant dof numbers for face exchange dofs
     for ( int face = 0; face < ma.GetNFaces(); face++ )
       {
	 int dest = 0;
	 
	 int distface = parallelma -> GetDistantFaceNum ( dest, face );
	owndofs[0]->Append ( distface );
	paralleldofs->SetExchangeDof ( dest, face );
	owndofs[0]->Append ( face );
      }   
    
    int dest = 0;
    MyMPI_ISend ( *owndofs[0], dest, sendrequest[dest]);
    MyMPI_IRecv ( *distantdofs[0], dest, recvrequest[dest]);
   
    MPI_Wait ( recvrequest+dest, &status);

    paralleldofs -> SetDistNDof( dest, (*distantdofs[dest])[0]) ;
    ii = 1;
    while ( ii < distantdofs[0]->Size() )
      {
	int fanum = (*distantdofs[0])[ii++];
// 	(*(paralleldofs->localexchangedof))[dest][ cnt_nexdof[dest] ] = 2*fanum;
// 	(*(paralleldofs->distantexchangedof))[dest][ cnt_nexdof[dest] ] = 2*(*distantdofs[0])[ii];
	(*(paralleldofs->sorted_exchangedof))[dest][ cnt_nexdof[dest] ] = 2*fanum;
	cnt_nexdof[dest]++;
// 	(*(paralleldofs->localexchangedof))[dest][ cnt_nexdof[dest] ] = 2*fanum+1;
// 	(*(paralleldofs->distantexchangedof))[dest][ cnt_nexdof[dest] ] = 2*(*distantdofs[0])[ii]+1;
	(*(paralleldofs->sorted_exchangedof))[dest][ cnt_nexdof[dest] ] = 2*fanum+1;
	ii++; cnt_nexdof[dest]++;
      }

    for ( int dest = id+1; dest < ntasks; dest++ )
      QuickSort ( (*(paralleldofs->sorted_exchangedof))[dest] );

    for ( int i = 0; i < ntasks; i++ )
       delete distantdofs[i], owndofs[i];

     delete [] owndofs, distantdofs;
     delete [] sendrequest, recvrequest;


   }

   void VectorFacetFESpace :: UpdateParallelDofs_loproc()
   { 
    *testout << "VectorFacetFESpace::UpdateParallelDofs_loproc" << endl;

    const MeshAccess & ma = (*this). GetMeshAccess();

    int ndof = GetNDof();

    // Find number of exchange dofs
    Array<int> nexdof(ntasks); 
    nexdof = 0;

    MPI_Status status;
    MPI_Request * sendrequest = new MPI_Request[ntasks];
    MPI_Request * recvrequest = new MPI_Request[ntasks];

    // number of face exchange dofs
    for ( int face = 0; face < ma.GetNFaces(); face++ )
      {
	nexdof[id] ++;//= dnums.Size() ; 
	for ( int dest = 1; dest < ntasks; dest ++ )
	  if (  parallelma -> GetDistantFaceNum ( dest, face ) >= 0 )
	    { 
	      nexdof[dest] += 2; 
	    }
      }

    paralleldofs->SetNExDof(nexdof);

//     paralleldofs->localexchangedof = new Table<int> (nexdof);
//     paralleldofs->distantexchangedof = new Table<int> (nexdof);
    paralleldofs->sorted_exchangedof = new Table<int> (nexdof);



    Array<int> ** owndofs,** distantdofs;
    owndofs = new Array<int> * [ntasks];
    distantdofs = new Array<int> * [ntasks];

    for ( int i = 0; i < ntasks; i++)
      {
	owndofs[i] = new Array<int> (1);
	(*owndofs[i])[0] = ndof;
	distantdofs[i] = new Array<int> (0);
      }

    int exdof = 0;
    Array<int> cnt_nexdof(ntasks);
    cnt_nexdof = 0;

    // *****************
    // Parallel Face dofs
    // *****************


    // find local and distant dof numbers for face exchange dofs
    for ( int face = 0; face < ma.GetNFaces(); face++ )
      {
// 	(*(paralleldofs->localexchangedof))[id][exdof++] = 2*face;
// 	(*(paralleldofs->localexchangedof))[id][exdof++] = 2*face+1;
	(*(paralleldofs->sorted_exchangedof))[id][exdof++] = 2*face;
	(*(paralleldofs->sorted_exchangedof))[id][exdof++] = 2*face+1;

	for ( int dest = 1; dest < ntasks; dest++ )
	  {
	    int distface = parallelma -> GetDistantFaceNum ( dest, face );
	    if( distface < 0 ) continue;

	    owndofs[dest]->Append ( distface );
	    paralleldofs->SetExchangeDof ( dest, 2*face );
	    paralleldofs->SetExchangeDof ( 2*face );
	    paralleldofs->SetExchangeDof ( dest, 2*face+1 );
	    paralleldofs->SetExchangeDof ( 2*face+1 );
	    owndofs[dest]->Append ( face );
  	  }
      }   


    for ( int dest = 1; dest < ntasks; dest ++ )
      {
	MyMPI_ISend ( *owndofs[dest], dest, sendrequest[dest]);
	MyMPI_IRecv ( *distantdofs[dest], dest, recvrequest[dest]);
      }
    


    int ii = 1;
    for( int dest=1; dest<ntasks; dest++) 
      {
	if ( dest == id ) continue;
	MPI_Wait ( recvrequest+dest, &status );
	paralleldofs -> SetDistNDof( dest, (*distantdofs[dest])[0]) ;
	ii = 1;
	while ( ii < distantdofs[dest]->Size() )
	  {
	    int fanum = (*distantdofs[dest])[ii++];
// 	    (*(paralleldofs->localexchangedof))[dest][ cnt_nexdof[dest] ] = 2*fanum;
// 	    (*(paralleldofs->distantexchangedof))[dest][ cnt_nexdof[dest] ] = 2*(*distantdofs[dest])[ii];
	    (*(paralleldofs->sorted_exchangedof))[dest][ cnt_nexdof[dest] ] = 2*fanum;
	    cnt_nexdof[dest]++;
// 	    (*(paralleldofs->localexchangedof))[dest][ cnt_nexdof[dest] ] = 2*fanum+1;
// 	    (*(paralleldofs->distantexchangedof))[dest][ cnt_nexdof[dest] ] = 2*(*distantdofs[dest])[ii]+1;
	    (*(paralleldofs->sorted_exchangedof))[dest][ cnt_nexdof[dest] ] = 2*fanum+1;
	    ii++; cnt_nexdof[dest]++;
	     
	  }
      }

    for ( int dest = id+1; dest < ntasks; dest++ )
      QuickSort ( (*(paralleldofs->sorted_exchangedof))[dest] );

     for ( int i = 0; i < ntasks; i++ )
       delete distantdofs[i], owndofs[i];

     delete [] owndofs, distantdofs;
     delete [] sendrequest, recvrequest;

   }

#endif

  // register FESpaces
  namespace vectorfacetfespace_cpp
  {
    class Init
    { 
    public: 
      Init ();
    };
    
    Init::Init()
    {
      GetFESpaceClasses().AddFESpace ("vectorfacet", VectorFacetFESpace::Create);
    }
    
    Init init;

  }

}



