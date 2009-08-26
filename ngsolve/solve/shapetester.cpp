
#include <solve.hpp>
// #include <nginterface.h>


namespace ngsolve
{
  using namespace ngsolve;

  int NGS_DrawShape (ClientData clientData,
		     Tcl_Interp * interp,
		     int argc, tcl_const char *argv[]);

  class NumProcShapeTester;


  
  static NumProcShapeTester * shapetester;
  ///
  class NumProcShapeTester : public NumProc
  {
  protected:
    ///
    GridFunction * gfu;
    ///
    BilinearForm * bfa;
    ///
    int dof;
  public:
    ///
    NumProcShapeTester (PDE & apde, const Flags & flags)
      : NumProc (apde)
    {
      gfu = pde.GetGridFunction (flags.GetStringFlag ("gridfunction", ""));
      // bfa = pde.GetBilinearForm (flags.GetStringFlag ("bilinearform", ""));
      dof = int(flags.GetNumFlag ("dof", 0));

      Tcl_Eval (pde.tcl_interpreter,
		"set w .shapetester;"
		"toplevel $w;" 
		"wm withdraw $w\n"
		"wm geom $w +100+100;"
		"wm deiconify $w;"
		"wm title $w \"Shape Tester\"\n"
		"set dofnr 0;"
		"tixControl $w.dof -label \"Dof number: \" -integer true -variable dofnr -min 0 "
		"-options { entry.width 5 label.width 12 label.anchor e } "
		"-command { NGS_DrawShape };"
		"pack $w.dof\n"
		"focus .options_dlg\n"
		);

      Tcl_CreateCommand (pde.tcl_interpreter, "NGS_DrawShape", NGS_DrawShape,
			 (ClientData)NULL,
			 (Tcl_CmdDeleteProc*) NULL);
      shapetester = this;
    }

    ///
    virtual ~NumProcShapeTester()
    {
      ;
    }
    
    static NumProc * Create (PDE & pde, const Flags & flags)
    {
      return new NumProcShapeTester (pde, flags);
    }

    void SetDof (int adof)
    {
      dof = adof;
      DoIt();
    }
    
    ///
    virtual void Do(LocalHeap & lh)
    {
      DoIt();
    }

    void DoIt ()
    {
      BaseVector & vecu = gfu->GetVector();
      vecu = 0;
      Array<int> dnums(1);
      if (dof >= vecu.Size())
	dof = vecu.Size()-1;
      dnums[0] = dof;
       
      Vector<> elu(1);
      elu(0) = 1;
      vecu.SetIndirect (dnums, elu);
      //gfu->Visualize(gfu->GetName());
      Ng_Redraw ();
    }


    ///
    virtual string GetClassName () const
    {
      return "Shape tester";
    }

    virtual void PrintReport (ostream & ost)
    {
      ;
    }

    static void PrintDoc (ostream & ost);
  };



  int NGS_DrawShape (ClientData clientData,
		     Tcl_Interp * interp,
		     int argc, tcl_const char *argv[])
  {
    cout << "draw shape nr " << argv[1] << endl;
    shapetester->SetDof (atoi(argv[1]));
    return TCL_OK;
  }





  namespace
  {
    class Init
    { 
    public: 
      Init ();
    };
    
    Init::Init()
    {
      GetNumProcs().AddNumProc ("shapetester", NumProcShapeTester::Create);
    }
    
    
    Init init;
    
  }
}
