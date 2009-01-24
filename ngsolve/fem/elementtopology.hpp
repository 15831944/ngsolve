#ifndef FILE_TOPOLOGY
#define FILE_TOPOLOGY

/*********************************************************************/
/* File:   topology.hpp                                              */
/* Author: Joachim Schoeberl                                         */
/* Date:   25. Mar. 2000                                             */
/*********************************************************************/

/*
    Toplogy of reference elements
*/


/**
   Geometry of element.
   Possible are ET_SEGM, ET_TRIG, ET_QUAD, ET_TET, ET_PYRAMID, ET_PRISM, ET_HEX
 */
enum ELEMENT_TYPE { ET_SEGM = 1,
		    ET_TRIG = 10, ET_QUAD = 11, 
		    ET_TET = 20, ET_PYRAMID = 21, ET_PRISM = 22, ET_HEX = 24 };



/**
   Type of node. 
   vertex nodes are 0 dimensional, 
   edge nodes are 1 dimensional
   face nodes are 2 dimensional
   cell nodes are 3 dimensional

   2D elements have vertex, edge, and face nodes

   3D elements have vertex, edge, face and cell nodes
 */

enum NODE_TYPE { NT_VERTEX = 0, NT_EDGE = 1, NT_FACE = 2, NT_CELL = 3 };

inline void operator++(NODE_TYPE & nt, int)  { nt = NODE_TYPE(nt+1); } 



/// point coordinates
typedef double POINT3D[3];  

/// initial point, end point
typedef int EDGE[2];      

/// points, last one is -1 for trig
typedef int FACE[4];      

/// normal vector
typedef double NORMAL[3];


/// Topology and coordinate information of master element:
class ElementTopology
{
public:
  /// returns name of element type
  static const char * GetElementName (ELEMENT_TYPE et);

  /// returns space dimension of element type
  static int GetSpaceDim (ELEMENT_TYPE et)
  {
    switch (et)
      {
      case ET_SEGM: return 1;
      case ET_TRIG: return 2;
      case ET_QUAD: return 2;
      case ET_TET: return 3;
      case ET_PYRAMID: return 3;
      case ET_PRISM: return 3;
      case ET_HEX: return 3;
      }
    return 0;
  }

  /// returns number of vertices
  static int GetNVertices (ELEMENT_TYPE et)
  {
    switch (et)
      {
      case ET_SEGM: return 2;
      case ET_TRIG: return 3;
      case ET_QUAD: return 4;
      case ET_TET: return 4;
      case ET_PYRAMID: return 5;
      case ET_PRISM: return 6;
      case ET_HEX: return 8;
      }
    return 0;
  }

  /// returns number of edges
  static int GetNEdges (ELEMENT_TYPE et)
  { 
    switch (et)
      {
      case ET_SEGM: return 1;
      case ET_TRIG: return 3;
      case ET_QUAD: return 4;
      case ET_TET: return 6;
      case ET_PYRAMID: return 8;
      case ET_PRISM: return 9;
      case ET_HEX: return 12;
      }
    return 0;
  }


  /// returns number of faces
  static int GetNFaces (ELEMENT_TYPE et)
  {
    switch (et)
      {
      case ET_SEGM: return 0;
      case ET_TRIG: return 1;
      case ET_QUAD: return 1;
      case ET_TET: return 4;
      case ET_PYRAMID: return 5;
      case ET_PRISM: return 5;
      case ET_HEX: return 6;
      }  
    return 0;
  }


  /// returns number of nodes of type nt
  static int GetNNodes (ELEMENT_TYPE et, NODE_TYPE nt)
  {
    static const int nn_segm[] = { 2, 1, 0, 0 };
    static const int nn_trig[] = { 3, 3, 1, 0 };
    static const int nn_quad[] = { 4, 4, 1, 0 };
    static const int nn_tet[] = { 4, 6, 4, 1 };
    static const int nn_pyramid[] = { 5, 8, 5, 1 };
    static const int nn_prism[] = { 6, 9, 5, 1 };
    static const int nn_hex[] = { 8, 12, 6, 1 };
    switch (et)
    {
    case ET_SEGM: return nn_segm[nt];
    case ET_TRIG: return nn_trig[nt];
    case ET_QUAD: return nn_quad[nt];
    case ET_TET: return nn_tet[nt];
    case ET_PYRAMID: return nn_pyramid[nt];
    case ET_PRISM: return nn_prism[nt];
    case ET_HEX: return nn_hex[nt];
    }  
    return 0;
  }







  /// returns number of facets: == GetNFaces in 3D, GetNEdges in 2D
  static int GetNFacets (ELEMENT_TYPE et)
  {
    switch (et)
    {
      case ET_SEGM: return 0;
      case ET_TRIG: return 3;
      case ET_QUAD: return 4;
      case ET_TET: return 4;
      case ET_PYRAMID: return 5;
      case ET_PRISM: return 5;
      case ET_HEX: return 6;
    }  
    return 0;
  }




  /// returns number of facets: == GetNFaces in 3D, GetNEdges in 2D
  static ELEMENT_TYPE GetFacetType (ELEMENT_TYPE et, int k)
  {
    switch (et)
    {
      case ET_TRIG: return ET_SEGM;
      case ET_QUAD: return ET_SEGM;
      case ET_TET: return ET_TRIG;
      case ET_PYRAMID: return (k<4 ? ET_TRIG : ET_QUAD); 
      case ET_PRISM: return (k<2 ? ET_TRIG : ET_QUAD);
      case ET_HEX: return ET_QUAD;
      default:
        cerr << "*** error in GetFacetType: Unhandled Elementtype" << endl;
        return ET_SEGM;
    }  
  }

  /// returns vertex coordinates (as 3D points)
  static const POINT3D * GetVertices (ELEMENT_TYPE et);

  /// returns edges of elements. zero-based pairs of integers
  static const EDGE * GetEdges (ELEMENT_TYPE et)
  {
    static const int segm_edges[1][2] =
      { { 0, 1 }};
    

    static const int trig_edges[3][2] =
      { { 2, 0 },
	{ 1, 2 },
	{ 0, 1 }};

    static const int quad_edges[4][2] =
      { { 0, 1 },
	{ 2, 3 },
	{ 3, 0 },
	{ 1, 2 }};
    
    static const int tet_edges[6][2] =
      { { 3, 0 },
	{ 3, 1 },
	{ 3, 2 }, 
	{ 0, 1 }, 
	{ 0, 2 },
	{ 1, 2 }};
    
    static const int prism_edges[9][2] =
      { { 2, 0 },
	{ 0, 1 },
	{ 2, 1 },
	{ 5, 3 },
	{ 3, 4 },
	{ 5, 4 },
	{ 2, 5 },
	{ 0, 3 },
	{ 1, 4 }};

    static const int pyramid_edges[8][2] =
      { { 0, 1 },
	{ 1, 2 },
	{ 0, 3 },
	{ 3, 2 },
	{ 0, 4 },
	{ 1, 4 },
	{ 2, 4 },
	{ 3, 4 }};

    static const int hex_edges[12][2] =
      {
        { 0, 1 },
        { 2, 3 },
        { 3, 0 },
        { 1, 2 },
        { 4, 5 },
        { 6, 7 },
        { 7, 4 },
        { 5, 6 },
        { 0, 4 },
        { 1, 5 },
        { 2, 6 },
        { 3, 7 },
      };
    
    switch (et)
      {
      case ET_SEGM: return segm_edges;
      case ET_TRIG: return trig_edges;
      case ET_QUAD: return quad_edges;
      case ET_TET:  return tet_edges;
      case ET_PYRAMID: return pyramid_edges;
      case ET_PRISM: return prism_edges;
      case ET_HEX: return hex_edges;
      default:
	break;
      }
    cerr << "Ng_GetEdges, illegal element type " << et << endl;
    return 0;  
  }

  /// returns faces of elements. zero-based array of 4 integers, last one is -1 for triangles
  static const FACE * GetFaces (ELEMENT_TYPE et)
  {
    static int tet_faces[4][4] =
      { { 3, 1, 2, -1 },
	{ 3, 2, 0, -1 },
	{ 3, 0, 1, -1 },
        { 0, 2, 1, -1 } }; // all faces point into interior!
  
    static int prism_faces[5][4] =
      {
	{ 0, 2, 1, -1 },
	{ 3, 4, 5, -1 },
	{ 2, 0, 3, 5 },
	{ 0, 1, 4, 3 },
	{ 1, 2, 5, 4 } 
      };

    static int pyramid_faces[5][4] =
      {
	{ 0, 1, 4, -1 },
	{ 1, 2, 4, -1 },
	{ 2, 3, 4, -1 },
	{ 3, 0, 4, -1 },
        { 0, 1, 2, 3 } // points into interior!
      };
  
    static int hex_faces[6][4] =
      {
        { 0, 3, 2, 1 },
        { 4, 5, 6, 7 },
        { 0, 1, 5, 4 },
        { 1, 2, 6, 5 },
        { 2, 3, 7, 6 },
        { 3, 0, 4, 7 }
    };
    
    static int trig_faces[1][4] = 
      {
	{ 0, 1, 2, -1 },
      };
    
    static int quad_faces[1][4] = 
      {
	{ 0, 1, 2, 3 },
      };
    
    switch (et)
      {
      case ET_TET: return tet_faces;
      case ET_PRISM: return prism_faces;
      case ET_PYRAMID: return pyramid_faces;
      case ET_HEX: return hex_faces;

      case ET_TRIG: return trig_faces;
      case ET_QUAD: return quad_faces;
        
      case ET_SEGM:
      default:
	break;
      }
    
    cerr << "Ng_GetFaces, illegal element type " << et << endl;
    return 0;
  }

  /// return normals on facets
  static const NORMAL * GetNormals(ELEMENT_TYPE et);
  
  /// returns number of edge from vertex v1 to vertex v2
  static int GetEdgeNr (ELEMENT_TYPE et, int v1, int v2);

  /// returns number of face containing vertices v1, v2, v3.  (trig only ?)
  static int GetFaceNr (ELEMENT_TYPE et, int v1, int v2, int v3);
};



/**
   A Node of an element.  

   A Node has a node type such such NT_VERTEX or NT_FACE, and a node
   number. The number can be with respect to the local element
   numbering, or can be the global numbering on the mesh.
 */
class Node
{
  NODE_TYPE nt;
  int nodenr;

public:
  /// do nothing
  Node () { ; }
  
  /// construct node from type and number
  Node (NODE_TYPE ant, int anodenr)
    : nt(ant), nodenr(anodenr) { ; }

  /// copy constructor
  Node (const Node & n2)
  { nt = n2.nt; nodenr = n2.nodenr; }

  /// returns type of the node
  NODE_TYPE GetType () const { return nt; }

  /// returns number of the node
  int GetNodeNr() const { return nodenr; }
};

inline int CalcNodeId (ELEMENT_TYPE et, const Node & node)
{
  switch (et)
    {
    case ET_TRIG: 
      {
        static const int nodebase[] = { 0, 3, 6 };
        return nodebase[node.GetType()] + node.GetNodeNr();
      }

    case ET_TET: 
      {
        static const int nodebase[] = { 0, 4, 10, 14 };
        return nodebase[node.GetType()] + node.GetNodeNr();
      }
    default:
      throw Exception (string ("CalcNodeId not implemented for element ") +
                       ElementTopology::GetElementName (et));
    }
}

inline Node CalcNodeFromId (ELEMENT_TYPE et, int nodeid)
{
  switch (et)
    {
    case ET_TRIG: 
      {
        static const int nodetypes[] = { 0, 0, 0, 1, 1, 1, 2 };
        static const int nodenrs[]   = { 0, 1, 2, 0, 1, 2, 0 };
        return Node (NODE_TYPE(nodetypes[nodeid]), nodenrs[nodeid]);
      }
    case ET_TET: 
      {
        static const int nodetypes[] = { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3 };
        static const int nodenrs[]   = { 0, 1, 2, 3, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 0 };
        return Node (NODE_TYPE(nodetypes[nodeid]), nodenrs[nodeid]);
      }

    default:
      throw Exception (string ("CalcNodeFromId not implemented for element ") +
                       ElementTopology::GetElementName (et));
    }
}

ostream & operator<< (ostream & ost, const Node & node);

typedef int NODE_SET;

inline NODE_SET NodeSet (NODE_TYPE nt1) 
{
  return 1 << nt1;
}

inline NODE_SET NodeSet (NODE_TYPE nt1, NODE_TYPE nt2) 
{
  return (1 << nt1) + (1 << nt2);
}

inline NODE_SET NodeSet (NODE_TYPE nt1, NODE_TYPE nt2, NODE_TYPE nt3) 
{
  return (1 << nt1) + (1 << nt2) + (1 << nt3);
}

inline NODE_SET NodeSet (NODE_TYPE nt1, NODE_TYPE nt2, NODE_TYPE nt3, NODE_TYPE nt4) 
{
  return (1 << nt1) + (1 << nt2) + (1 << nt3) + (1 << nt4);
}




class TopologicElement
{
  ELEMENT_TYPE et;
  ArrayMem<Node, 27> nodes;

public:
  void SetElementType (ELEMENT_TYPE aet) { et = aet; }
  void Clear() { nodes.SetSize(0); }
  void AddNode (const Node & node) { nodes.Append (node); }


  ELEMENT_TYPE GetType () const { return et; }
  int GetNNodes () const { return nodes.Size(); }
  const Node & GetNode (int i) const { return nodes[i]; }

  Node GlobalNode (const Node & locnode)
  { return Node (locnode.GetType(), nodes[CalcNodeId (et, locnode)].GetNodeNr() ); }
};

ostream & operator<< (ostream & ost, const TopologicElement & etop);




template <int ET> class ET_trait { };

template<> class ET_trait<ET_SEGM>
{
public:
  enum { DIM = 1 };
  enum { NV = 2 };
};

template<> class ET_trait<ET_TRIG>
{
public:
  enum { DIM = 2 };
  enum { NV = 3 };
};

template<> class ET_trait<ET_QUAD>
{
public:
  enum { DIM = 2 };
  enum { NV = 4 };
};


template<> class ET_trait<ET_TET>
{
public:
  enum { DIM = 3 };
  enum { NV = 4 };
};

template<> class ET_trait<ET_PRISM>
{
public:
  enum { DIM = 3 };
  enum { NV = 6 };
};

template<> class ET_trait<ET_PYRAMID>
{
public:
  enum { DIM = 3 };
  enum { NV = 5 };
};

template<> class ET_trait<ET_HEX>
{
public:
  enum { DIM = 3 };
  enum { NV = 8 };
};


#endif
