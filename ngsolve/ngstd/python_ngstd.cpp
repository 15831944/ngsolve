#ifdef NGS_PYTHON

#include <string>
#include <ostream>
#include <type_traits>

#include "python_ngstd.hpp"

using std::ostringstream;




struct PyExportNgStd {
  PyExportNgStd(BasePythonEnvironment & py_env);
};


PyExportNgStd ::
PyExportNgStd(BasePythonEnvironment & py_env) {
  
  bp::scope sc(py_env.main_module);  

      
    // Export ngstd classes
    py_env["FlatArrayD"] = bp::class_<FlatArray<double> >("FlatArrayD")
        .def(PyDefVector<FlatArray<double>, double>()) 
        .def(PyDefToString<FlatArray<double> >())
        .def(bp::init<int, double *>())
        ;
    
    py_env["ArrayD"] = bp::class_<Array<double>, bp::bases<FlatArray<double> > >("ArrayD")
        .def(bp::init<int>())
        ;

    py_env["FlatArrayI"] = bp::class_<FlatArray<int> >("FlatArrayI")
        .def(PyDefVector<FlatArray<int>, int>()) 
        .def(PyDefToString<FlatArray<int> >())
        .def(bp::init<int, int *>())
        ;

    py_env["ArrayI"] = bp::class_<Array<int>, bp::bases<FlatArray<int> > >("ArrayI")
        .def(bp::init<int>())
        ;
    
    

    bp::class_<ngstd::LocalHeap>
      ("LocalHeap",bp::no_init)
      .def(bp::init<size_t,const char*>())
      ;

    bp::class_<ngstd::Flags>
      ("Flags")
      ;

    bp::class_<ngstd::IntRange>
      ("IntRange", bp::init<int,int>())
      .def(PyDefToString<IntRange>())
      ;

    
    }


// static PyExportNgStd python_export_ngstd (PythonEnvironment::getInstance());


#endif // NGS_PYTHON
