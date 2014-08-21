#ifdef NGS_PYTHON
#include "../ngstd/python_ngstd.hpp"
#include <bla.hpp>
using namespace ngbla;

struct PyExportNgBla {
  PyExportNgBla(BasePythonEnvironment & py_env);
};

PyExportNgBla ::
    PyExportNgBla(BasePythonEnvironment & py_env) {
  cout << "a" << endl;
      // auto py_env = PythonEnvironment::getInstance();
        typedef FlatVector<double> FVD;
        py_env["FlatVector"] = bp::class_<FVD >("FlatVector")
            .def(PyDefVector<FVD, double>()) 
            .def(PyDefToString<FVD >())
            .def("Assign", FunctionPointer( [](FVD &self, FVD &v, double s) { self  = s*v; }) )
            .def("Add",    FunctionPointer( [](FVD &self, FVD &v, double s) { self += s*v; }) )
          .def("Range",    static_cast</* const */ FVD (FVD::*)(int,int) const> (&FVD::Range ) )
            .def(bp::init<int, double *>())
            .def(bp::self+=bp::self)
            .def(bp::self-=bp::self)
            .def(bp::self*=double())
            ;
        cout << " type enablevecexpr " << endl;
        PyEnableVecExpr(py_env, "FlatVector");
        cout << " succ" << endl;
        py_env["Vector"] = bp::class_<Vector<double>, bp::bases<FlatVector<double> > >("Vector")
            .def(bp::init<int>())
            ;

  cout << "b" << endl;
        typedef FlatMatrix<double> FMD;
        py_env["FlatMatrix"] = bp::class_<FlatMatrix<double> >("FlatMatrix")
            .def(PyDefToString<FMD>())
            .def("Mult",        FunctionPointer( [](FMD &m, FVD &x, FVD &y, double s) { y  = s*m*x; }) )
            .def("MultAdd",     FunctionPointer( [](FMD &m, FVD &x, FVD &y, double s) { y += s*m*x; }) )
            .def("MultTrans",   FunctionPointer( [](FMD &m, FVD &x, FVD &y, double s) { y  = s*Trans(m)*x; }) )
            .def("MultTransAdd",FunctionPointer( [](FMD &m, FVD &x, FVD &y, double s) { y += s*Trans(m)*x; }) )
            .def("Get", FunctionPointer( [](FMD &m, int i, int j)->double { return m(i,j); } ) )
            .def("Set", FunctionPointer( [](FMD &m, int i, int j, double val) { m(i,j) = val; }) )
            .def(bp::self+=bp::self)
            .def(bp::self-=bp::self)
            .def(bp::self*=double())
            ;

        // PyEnableMatExpr(py_env, "FlatMatrix");

        py_env["Matrix"] = bp::class_<Matrix<double>, bp::bases<FMD> >("Matrix")
            .def(bp::init<int, int>())
            ;

        // execute bla.py
        // py_env.exec("globals().update(run_module('bla',globals()))");

  cout << "c" << endl;
    }


// Call constructor to export python classes
// static PyExportBla python_export_bla (PythonEnvironment::getInstance());

#endif // NGS_PYTHON