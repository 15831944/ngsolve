print ("hello from ale.py")


from ngsolve.solve import *
from ngsolve.comp import *
from ngsolve.fem import *
from ngsolve.la import *
from ngsolve.bla import *
from ngsolve.ngstd import *

from math import sin
from time import sleep
 

class npALE(PyNumProc):
        
    def Do(self, heap):
        print ("ALE - Laplace")

        pde = self.pde

        d = pde.gridfunctions["def"]
        d.Set(VariableCF("(2*y*(1-y)*x*(1-x),(3*y*(1-y)*x*(1-x))"))
        pde.Mesh().SetDeformation (d)

        u = pde.gridfunctions["u"]
        # first test:
        # u.Set(VariableCF("x"))

        
        v = pde.spaces["v"]
        a = pde.bilinearforms["a"]
        f = pde.linearforms["f"]

        a.Assemble()
        f.Assemble()
        inv = a.mat.Inverse(v.FreeDofs())

        u.vec.data = inv * f.vec
        






class npALE_instat(PyNumProc):
        
    def Do(self, heap):
        print ("ALE - Laplace")

        pde = self.pde

        d = pde.gridfunctions["def"]
        dp = pde.gridfunctions["defp"]
        u = pde.gridfunctions["u"]
        v = pde.spaces["v"]
        a = pde.bilinearforms["a"]
        b = pde.bilinearforms["b"]
        m = pde.bilinearforms["m"]
        f = pde.linearforms["f"]

    
        mstar = a.mat.CreateMatrix()

        # d.Set(VariableCF("(4*y*(1-y)*x*(1-x),(4*y*(1-y)*x*(1-x))"))
        d.Set(VariableCF("(1*y*x*(1-x),(1*y*x*(1-x))"))
        dmax = d.vec.CreateVector()
        dold = d.vec.CreateVector()
        
        dmax.data = d.vec
        d.vec.data = 0.0 * dmax
        dold.data = d.vec
        
        inv = a.mat.Inverse (v.FreeDofs())
        u.vec.data = inv * f.vec
        # u.Set(VariableCF ("exp (-1000 *((x-0.5)*(x-0.5)+(y-0.5)*(y-0.5)))"))

        Redraw()
        sleep (3)

        tau = 0.001
        r = u.vec.CreateVector()
        
        for j in range(10000):
            t = tau * j
            print ("t = ", t)

            dold.data = d.vec
            d.vec.data = sin(10*t) * dmax
            dp.vec.data = 1.0/tau * d.vec - 1.0/tau * dold

            pde.Mesh().SetDeformation (d)

            a.Assemble()
            b.Assemble()
            m.Assemble()
            f.Assemble(1000000)

            mstar.AsVector().data = m.mat.AsVector() + tau * a.mat.AsVector()
            inv = mstar.Inverse(v.FreeDofs())

            r.data = a.mat * u.vec + b.mat * u.vec
            u.vec.data -= tau * inv * r

            Redraw()
            sleep (0.1)

