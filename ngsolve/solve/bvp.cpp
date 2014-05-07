#include "../include/solve.hpp"
#include <parallelngs.hpp>

namespace ngsolve
{


  /* *************************** Numproc BVP ********************** */


  ///
  class NumProcBVP : public NumProc
  {
  protected:
    ///
    BilinearForm * bfa;
    ///
    LinearForm * lff;
    ///
    GridFunction * gfu;
    ///
    Preconditioner * pre;
    ///
    int maxsteps;
    ///
    double prec;
    ///
    double tau, taui;
    ///
    bool print;
    ///
    enum SOLVER { CG, GMRES, QMR/*, NCG */, SIMPLE, DIRECT, BICGSTAB };
    ///
    enum IP_TYPE { SYMMETRIC, HERMITEAN, CONJ_HERMITEAN };
    ///
    SOLVER solver;

    IP_TYPE ip_type;
    ///
    bool useseedvariant;
  public:
    ///
    NumProcBVP (PDE & apde, const Flags & flags);
    ///
    virtual ~NumProcBVP();

    ///
    virtual void Do(LocalHeap & lh);
    ///
    virtual string GetClassName () const
    {
      return "Boundary Value Problem";
    }

    virtual void PrintReport (ostream & ost)
    {
      ost << GetClassName() << endl
	  << "Bilinear-form = " << bfa->GetName() << endl
	  << "Linear-form   = " << lff->GetName() << endl
	  << "Gridfunction  = " << gfu->GetName() << endl
	  << "Preconditioner = " << ((pre) ? pre->ClassName() : "None") << endl
	  << "solver        = "; 
      
      switch(solver)
        {
        case CG:
          ost << "CG" << endl; break;
        case QMR:
          ost << "QMR" << endl; break;
        case GMRES:
          ost << "GMRES" << endl; break;
          // case NCG:
          // ost << "NCG" << endl; break;
        case SIMPLE:
          ost << "Simple" << endl; break;
        case DIRECT:
          ost << "DIRECT" << endl; break;
	case BICGSTAB:
	  ost << "BiCGStab" << endl; break;
        default:
          ost << "Unkown solver-type" << endl;
        }
      ost << "precision     = " << prec << endl
	  << "maxsteps      = " << maxsteps << endl;
    }

    ///
    static void PrintDoc (ostream & ost);
  };



  NumProcBVP :: NumProcBVP (PDE & apde, const Flags & flags)
    : NumProc (apde)
  {
    bfa = pde.GetBilinearForm (flags.GetStringFlag ("bilinearform", ""));
    lff = pde.GetLinearForm (flags.GetStringFlag ("linearform", ""));
    gfu = pde.GetGridFunction (flags.GetStringFlag ("gridfunction", ""));
    if (flags.StringFlagDefined("preconditioner"))
      pre = pde.GetPreconditioner (flags.GetStringFlag ("preconditioner", ""));
    else
      pre = NULL;
    maxsteps = int(flags.GetNumFlag ("maxsteps", 200));
    prec = flags.GetNumFlag ("prec", 1e-12);
    tau = flags.GetNumFlag ("tau", 1);
    taui = flags.GetNumFlag ("taui", 0);
    solver = CG;
    
    if (flags.GetDefineFlag ("qmr")) solver = QMR;
// old flag-style: no longer supported. only qmr for compatibility 
    if (flags.GetDefineFlag ("gmres")) 
      cout << "*** warning: flag -gmres deprecated: use -solver=gmres instead" << endl;
    if (flags.GetDefineFlag ("ncg"))
      cout << "*** warning: flag -ncg deprecated: use -solver=ncg instead" << endl;
    if (flags.GetDefineFlag ("direct"))
      cout << "*** warning: flag -direct deprecated: use -solver=direct instead" << endl;

    
    // new style: -solver=cg|qmr|gmres|direct|bicgstab
    {
      string solvername = flags.GetStringFlag("solver","cg");
      if (solvername == "cg")     solver = CG;
      if (solvername == "qmr")    solver = QMR;
      if (solvername == "gmres")  solver = GMRES;
      if (solvername == "simple") solver = SIMPLE;
      if (solvername == "direct") solver = DIRECT;
      if (solvername == "bicgstab") solver = BICGSTAB;
    }       
    
    string ipflag = flags.GetStringFlag("innerproduct","symmetric");
    ip_type = SYMMETRIC;
    if (ipflag == "symmetric") ip_type = SYMMETRIC;
    if (ipflag == "hermitean") ip_type = HERMITEAN;
    if (ipflag == "conj_hermitean") ip_type = CONJ_HERMITEAN;

    print = flags.GetDefineFlag ("print");
    useseedvariant = flags.GetDefineFlag ("seed");

    if (solver != DIRECT)
      pde.AddVariable (string("bvp.")+flags.GetStringFlag ("name",NULL)+".its", 0.0, 6);
  }

  NumProcBVP :: ~NumProcBVP()
  {
    ;
  }

  void NumProcBVP :: PrintDoc (ostream & ost)
  {
    ost << 
      "\n\nNumproc BVP:\n" \
      "------------\n" \
      "Solves the linear system resulting from a boundary value problem\n\n" \
      "Required flags:\n" 
      "-bilinearform=<bfname>\n" 
      "    bilinear-form providing the matrix\n" \
      "-linearform=<lfname>\n" \
      "    linear-form providing the right hand side\n" \
      "-gridfunction=<gfname>\n" \
      "    grid-function to store the solution vector\n" 
      "\nOptional flags:\n"\
      "\n-solver=<solvername> (cg|qmr|gmres|direct|bicgstab)\n"\
      "-seed\n"\
      "    use seed variant for multiple rhs\n"\
      "-preconditioner=<prename>\n"
      "-maxsteps=n\n"
      "-prec=eps\n"
      "-print\n"
      "    write matrices and vectors into logfile\n"
	<< endl;
  }


  void NumProcBVP :: Do(LocalHeap & lh)
  {
    static Timer timer("Equation solving");
    timer.Start();

    if (!lff->IsAssembled()) lff->Assemble(lh);

    cout << IM(1) << "solve bvp" << endl;

    const BaseMatrix & mat = bfa->GetMatrix();
    const BaseVector & vecf = lff->GetVector();
    BaseVector & vecu = gfu->GetVector();

    bool eliminate_internal = bfa -> UsesEliminateInternal();

    if (print)
      {
	(*testout) << "MatrixHeight = " << endl << mat.VHeight() << endl;
	(*testout) << "MatrixWidth = " << endl << mat.VWidth() << endl;
	(*testout) << "Matrix = " << endl << mat << endl;
	(*testout) << "RHS-Vector = " << endl << vecf << endl;
      }

    const BaseMatrix * premat = NULL;
    if (pre)  premat = &(pre->GetMatrix());
    
    KrylovSpaceSolver * invmat = NULL;
    BaseMatrix * invmat2 = NULL; 

    if (!bfa->GetFESpace().IsComplex())
      {
	switch (solver)
	  {
          case CG:
	    cout << IM(1) << "cg solve for real system" << endl;
	    invmat = new CGSolver<double>(mat, *premat);
	    break;
          case BICGSTAB:
	    cout << IM(1) << "bicgstab solve for real system" << endl;
	    invmat = new BiCGStabSolver<double>(mat, *premat);
	    break;
          case QMR:
            cout << IM(1) << "qmr solve for real system" << endl;
            invmat = new QMRSolver<double>(mat, *premat);
            break;
	  case GMRES:
            cout << IM(1) << "gmres solve for real system" << endl;
            invmat = new GMRESSolver<double>(mat, *premat);
	    break;
	  case SIMPLE:
            {
              cout << IM(1) << "simple solve for real system" << endl;
              SimpleIterationSolver<double> * hinv = new SimpleIterationSolver<double>(mat, *premat);
              hinv -> SetTau (tau);
              invmat = hinv;
              break;
            }
          case DIRECT:
            cout << IM(1) << "direct solve for real system" << endl;
            invmat2 = dynamic_cast<const BaseSparseMatrix&> (mat) . InverseMatrix(bfa->GetFESpace().GetFreeDofs(eliminate_internal)); 
            break;
	  }
      }
    else if ( ip_type == SYMMETRIC )
      {
 	switch (solver)
	  {
	  case CG:
            cout << IM(1) << "cg solve for complex system" << endl;
            invmat = new CGSolver<Complex>(mat, *premat);
	    break;
          case BICGSTAB:
	    cout << IM(1) << "bicgstab solve for complex system" << endl;
	    invmat = new BiCGStabSolver<Complex>(mat, *premat);
	    break;
	  case QMR:
            cout << IM(1) << "qmr solve for complex system" << endl;
            invmat = new QMRSolver<Complex>(mat, *premat);
	    break;
	  case GMRES:
            cout << IM(1) << "gmres solve for complex system" << endl;
            invmat = new GMRESSolver<Complex>(mat, *premat);
	    break;
	  case SIMPLE:
            {
              cout << IM(1) << "simple solve for complex system" << endl;
              SimpleIterationSolver<Complex> * hinv = new SimpleIterationSolver<Complex>(mat, *premat);
              hinv -> SetTau (Complex (tau, taui));
              invmat = hinv;
              break;
            }
          case DIRECT:
            cout << IM(1) << "direct solve for complex system" << endl;
            invmat2 = dynamic_cast<const BaseSparseMatrix&> (mat) . InverseMatrix(bfa->GetFESpace().GetFreeDofs(eliminate_internal)); 
            break;
          }
      }
    else if ( ip_type == HERMITEAN )
      {
 	switch (solver)
	  {
	  case CG:
            cout << IM(1) << "cg solve for complex system" << endl;
            invmat = new CGSolver<ComplexConjugate>(mat, *premat);
	    break;
          case BICGSTAB:
	    cout << IM(1) << "bicgstab solve for complex system" << endl;
	    invmat = new BiCGStabSolver<ComplexConjugate>(mat, *premat);
	    break;
	  case QMR:
            cout << IM(1) << "qmr solve for complex system" << endl;
            invmat = new QMRSolver<ComplexConjugate>(mat, *premat);
	    break;
	  case GMRES:
            cout << IM(1) << "gmres solve for complex system" << endl;
            invmat = new GMRESSolver<ComplexConjugate>(mat, *premat);
	    break;
	  case SIMPLE:
            {
              cout << IM(1) << "simple solve for complex system" << endl;
              SimpleIterationSolver<ComplexConjugate> * hinv = new SimpleIterationSolver<ComplexConjugate>(mat, *premat);
              hinv -> SetTau (Complex (tau, taui));
              invmat = hinv;
              break;
            }
          case DIRECT:
            cout << IM(1) << "direct solve for complex system" << endl;
            invmat2 = dynamic_cast<const BaseSparseMatrix&> (mat) . InverseMatrix(bfa->GetFESpace().GetFreeDofs(eliminate_internal)); 
            break;
          }
      }
    else if ( ip_type == CONJ_HERMITEAN )
      {
 	switch (solver)
	  {
	  case CG:
            cout << IM(1) << "cg solve for complex system" << endl;
            invmat = new CGSolver<ComplexConjugate2>(mat, *premat);
	    break;
          case BICGSTAB:
	    cout << IM(1) << "bicgstab solve for complex system" << endl;
	    invmat = new BiCGStabSolver<ComplexConjugate2>(mat, *premat);
	    break;
	  case QMR:
            cout << IM(1) << "qmr solve for complex system" << endl;
            invmat = new QMRSolver<ComplexConjugate2>(mat, *premat);
	    break;
	  case GMRES:
            cout << IM(1) << "gmres solve for complex system" << endl;
            invmat = new GMRESSolver<ComplexConjugate2>(mat, *premat);
	    break;
	  case SIMPLE:
            {
              cout << IM(1) << "simple solve for complex system" << endl;
              SimpleIterationSolver<ComplexConjugate2> * hinv = new SimpleIterationSolver<ComplexConjugate2>(mat, *premat);
              hinv -> SetTau (Complex (tau, taui));
              invmat = hinv;
              break;
            }
          case DIRECT:
            cout << IM(1) << "direct solve for complex system" << endl;
            invmat2 = dynamic_cast<const BaseSparseMatrix&> (mat) . InverseMatrix(bfa->GetFESpace().GetFreeDofs(eliminate_internal)); 
            break;
          }
      }

    if (solver != DIRECT)  
      {
        ma.PushStatus ("Iterative solver");
        invmat->SetMaxSteps (maxsteps);
        invmat->SetPrecision (prec);
        invmat->SetPrintRates ();
        invmat->SetInitialize (0);
        invmat->SetStatusHandler(ma);
	invmat->UseSeed(useseedvariant);
      }
    else
      ma.PushStatus ("Direct solver");

      
    double starttime, endtime;
    starttime = WallTime(); // clock();


    BaseVector & hv = *vecu.CreateVector();
    if (solver != DIRECT)
      {
	hv = vecf;
        bfa->ModifyRHS (hv);
        invmat->Mult (hv, vecu);
      }
    else 
      {
	hv = vecf - mat * vecu;
        bfa->ModifyRHS (hv);
	vecu += (*invmat2) * hv;
      }
    delete &hv;


    ma.PopStatus ();
    
    if (print)
      (*testout) << "Solution = " << endl << vecu << endl;

    endtime = WallTime(); // clock();
    
    cout << IM(1) << "Solution time = " << endtime - starttime << " sec wall time" << endl;
    if (solver != DIRECT)
      {
	cout << IM(1) << "Iterations: " << invmat->GetSteps() << endl;
	pde.AddVariable (string("bvp.")+GetName()+".its", invmat->GetSteps(), 6);
      }

    if (solver != DIRECT)
      delete invmat;
    else
      delete invmat2;

    timer.Stop();

    bfa -> ComputeInternal (vecu, vecf, lh);

    if (print)
      (*testout) << "Solution = " << endl << vecu << endl;
  }















  /* *************************** Numproc ConstrainedBVP ********************** */


  ///
  class NumProcConstrainedBVP : public NumProc
  {
  protected:
    ///
    BilinearForm * bfa;
    ///
    LinearForm * lff;
    ///
    GridFunction * gfu;
    ///
    Preconditioner * pre;
    ///
    int maxsteps;
    ///
    double prec;
    ///
    bool print;
    ///
    enum SOLVER { CG, QMR /* , NCG */ };
    ///
    SOLVER solver;
    ///
    Array<LinearForm*> constraints;
    
  public:
    ///
    NumProcConstrainedBVP (PDE & apde, const Flags & flags);
    ///
    virtual ~NumProcConstrainedBVP();

    ///
    virtual void Do(LocalHeap & lh);
    ///
    virtual string GetClassName () const
    {
      return "Boundary Value Problem";
    }

    virtual void PrintReport (ostream & ost)
    {
      ost << GetClassName() << endl
	  << "Bilinear-form = " << bfa->GetName() << endl
	  << "Linear-form   = " << lff->GetName() << endl
	  << "Gridfunction  = " << gfu->GetName() << endl
	  << "Preconditioner = " << ((pre) ? pre->ClassName() : "None") << endl
	  << "solver        = " << ((solver==CG) ? "CG" : "QMR") << endl
	  << "precision     = " << prec << endl
	  << "maxsteps      = " << maxsteps << endl;
    }

    ///
    static void PrintDoc (ostream & ost);
  };



  class ConstrainedPrecondMatrix : public BaseMatrix
  {
    const BaseMatrix * c1;
    Array<const BaseVector*> constraints;
    Array<BaseVector*> c1constraints;
    Matrix<double> projection, invprojection;
    int ncnt;
  public:
    ConstrainedPrecondMatrix (const BaseMatrix * ac1)
      : c1(ac1) { ncnt = 0; }
    
    virtual ~ConstrainedPrecondMatrix () { ; }

    void AddConstraint (const BaseVector * hv)
    {
      constraints.Append (hv);
      c1constraints.Append (hv->CreateVector());
      *c1constraints.Last() = (*c1) * *constraints.Last();

      ncnt = constraints.Size();
      projection.SetSize(ncnt);
      invprojection.SetSize(ncnt);

      for (int i = 0; i < ncnt; i++)
	for (int j = 0; j < ncnt; j++)
	  projection(i,j) = InnerProduct (*constraints[i], *c1constraints[j]);
      for (int i = 0; i < ncnt; i++)
	projection(i,i) += 1;

      CalcInverse (projection, invprojection);
      // cout << "projection = " << endl << projection << endl;
      // cout << "invprojection = " << endl << invprojection << endl;
    }

    virtual void Mult (const BaseVector & x, BaseVector & y) const
    {
      c1 -> Mult (x, y);
      Vector<double> hv1(ncnt), hv2(ncnt);
      
      for (int i = 0; i < ncnt; i++)
	hv1(i) = InnerProduct (x, *c1constraints[i]);

      hv2 = invprojection * hv1;

      //      cout << "hv1 = " << hv1 << ", hv2 = " << hv2 << endl;

      for (int i = 0; i < ncnt; i++)
	y -= hv2(i) * *c1constraints[i];
    }
  };



  class ConstrainedMatrix : public BaseMatrix
  {
    const BaseMatrix * a1;
    Array<const BaseVector*> constraints;
    int ncnt;
  public:
    ConstrainedMatrix (const BaseMatrix * aa1)
      : a1(aa1) { ncnt = 0; }
    
    virtual ~ConstrainedMatrix () { ; }

    void AddConstraint (const BaseVector * hv)
    {
      constraints.Append (hv);
      ncnt = constraints.Size();
    }

    virtual BaseVector * CreateVector () const
    {
      return a1->CreateVector();
    }

    virtual void Mult (const BaseVector & x, BaseVector & y) const
    {
      a1 -> Mult (x, y);
      Vector<double> hv1(ncnt);
      
      for (int i = 0; i < ncnt; i++)
	hv1(i) = InnerProduct (x, *constraints[i]);
      for (int i = 0; i < ncnt; i++)
	y += hv1(i) * *constraints[i];
    }

    virtual void MultAdd (double s, const BaseVector & x, BaseVector & y) const
    {
      a1 -> MultAdd (s, x, y);
      Vector<double> hv1(ncnt);
      
      for (int i = 0; i < ncnt; i++)
	hv1(i) = InnerProduct (x, *constraints[i]);
      for (int i = 0; i < ncnt; i++)
	y += (s*hv1(i)) * *constraints[i];
    }
  };





  NumProcConstrainedBVP :: NumProcConstrainedBVP (PDE & apde, const Flags & flags)
    : NumProc (apde)
  {
    bfa = pde.GetBilinearForm (flags.GetStringFlag ("bilinearform", ""));
    lff = pde.GetLinearForm (flags.GetStringFlag ("linearform", ""));
    gfu = pde.GetGridFunction (flags.GetStringFlag ("gridfunction", ""));
    pre = pde.GetPreconditioner (flags.GetStringFlag ("preconditioner", ""), 1);
    maxsteps = int(flags.GetNumFlag ("maxsteps", 200));
    prec = flags.GetNumFlag ("prec", 1e-12);
    solver = CG;
    if (flags.GetDefineFlag ("qmr")) solver = QMR;
    // if (flags.GetDefineFlag ("ncg")) solver = NCG;
    print = flags.GetDefineFlag ("print");

    const Array<char*> & cnts = flags.GetStringListFlag ("constraints");
    for (int i = 0; i < cnts.Size(); i++)
      constraints.Append (apde.GetLinearForm (cnts[i]));
  }

  NumProcConstrainedBVP :: ~NumProcConstrainedBVP()
  {
    ;
  }

  void NumProcConstrainedBVP :: PrintDoc (ostream & ost)
  {
    ost << 
      "\n\nNumproc ConstrainedBVP:\n" \
      "------------\n" \
      "Solves the linear system resulting from a boundary value problem\n\n" \
      "Required flags:\n" 
      "-bilinearform=<bfname>\n" 
      "    bilinear-form providing the matrix\n" \
      "-linearform=<lfname>\n" \
      "    linear-form providing the right hand side\n" \
      "-gridfunction=<gfname>\n" \
      "    grid-function to store the solution vector\n" 
      "\nOptional flags:\n"
      "-preconditioner=<prename>\n"
      "-maxsteps=n\n"
      "-prec=eps\n"
      "-qmr\n"
      "-print\n"
      "    write matrices and vectors into logfile\n"
	<< endl;
  }


  void NumProcConstrainedBVP :: Do(LocalHeap & lh)
  {
    cout << "solve constrained bvp" << endl;

    const BaseMatrix & mat = bfa->GetMatrix();
    const BaseVector & vecf = lff->GetVector();
    BaseVector & vecu = gfu->GetVector();

    if (print)
      {
	(*testout) << "MatrixHeight = " << endl << mat.VHeight() << endl;
	(*testout) << "MatrixWidth = " << endl << mat.VWidth() << endl;
	(*testout) << "Matrix = " << endl << mat << endl;
	(*testout) << "RHS-Vector = " << endl << vecf << endl;
      }

    const BaseMatrix * premat = NULL;
    if (pre)  
      {
	premat = &(pre->GetMatrix());

	ConstrainedPrecondMatrix * hpre = new ConstrainedPrecondMatrix (premat);
	premat = hpre;
	for (int i = 0; i < constraints.Size(); i++)
	  hpre->AddConstraint(&constraints[i]->GetVector());
      }

    ConstrainedMatrix * hmat = new ConstrainedMatrix (&mat);
    for (int i = 0; i < constraints.Size(); i++)
      hmat->AddConstraint(&constraints[i]->GetVector());
    

    KrylovSpaceSolver * invmat = NULL;

    if (!bfa->GetFESpace().IsComplex())
      {
	switch (solver)
	  {
	  case CG:
	    invmat = new CGSolver<double>(*hmat, *premat);
	    break;
	  case QMR:
	    invmat = new QMRSolver<double>(*hmat, *premat);
	    break;
	  }
      }
    else
      {
	switch (solver)
	  {
	  case CG:
	    invmat = new CGSolver<Complex>(*hmat, *premat);
	    break;
	  case QMR:
	    invmat = new QMRSolver<Complex>(*hmat, *premat);
	    break;
	  }
      }

    ma.PushStatus ("Iterative solver");

    invmat->SetMaxSteps (maxsteps);
    invmat->SetPrecision (prec);
    invmat->SetPrintRates ();
    invmat->SetInitialize (0);

    clock_t starttime, endtime;
    starttime = clock();

    invmat->Mult (vecf, vecu);

    ma.PopStatus ();
    
    if (print)
      (*testout) << "Solution = " << endl << vecu << endl;

    endtime = clock();
    cout << "Solution time = " << double(endtime - starttime)/CLOCKS_PER_SEC << endl;
    cout << "Iterations: " << invmat->GetSteps() << endl;
    *testout << "Solution time = " << double(endtime - starttime)/CLOCKS_PER_SEC << endl;
    *testout << "Iterations: " << invmat->GetSteps() << endl;

    
    delete invmat;


    bfa -> ComputeInternal (vecu, vecf, lh);
  }



  static RegisterNumProc<NumProcBVP> npinitbvp("bvp");
  static RegisterNumProc<NumProcConstrainedBVP> npinitbvp2("constrainedbvp");
}
