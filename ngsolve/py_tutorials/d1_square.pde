geometry = square.in2d
mesh = square.vol

# python module
pymodule = module1


fespace v -type=h1ho -order=3 -dirichlet=[1]
gridfunction u -fespace=v -nested

define coefficient cf  (x*y)
define coefficient one 1

linearform f -fespace=v
source cf

bilinearform a -fespace=v -symmetric -xxeliminate_internal
laplace one
# robin 100

preconditioner c -type=local -bilinearform=a

# numproc bvp np1 -bilinearform=a -linearform=f -gridfunction=u -preconditioner=c -maxsteps=1000

# numproc drawflux np2 -bilinearform=a -solution=u -label=flux -applyd
# numproc visualization npv1 -scalarfunction=u -subdivision=2 -nolineartexture


pynumproc pyNP1  myname1 -flag1=27

pynumproc pybvp np2




