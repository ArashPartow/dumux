#!/bin/sh

# First create and enter a folder for the dune and dumux modules

# download Dune core modules
git clone http://git.dune-project.org/repositories/dune-common
cd dune-common
git checkout releases/2.3
cd ..

git clone http://git.dune-project.org/repositories/dune-geometry
cd dune-geometry
git checkout releases/2.3
cd ..

git clone http://git.dune-project.org/repositories/dune-grid
cd dune-grid
git checkout releases/2.3
cd ..

git clone http://git.dune-project.org/repositories/dune-istl
cd dune-istl
git checkout releases/2.3
cd ..

git clone http://git.dune-project.org/repositories/dune-localfunctions
cd dune-localfunctions
git checkout releases/2.3
cd ..

# download dune-typetree
git clone http://git.dune-project.org/repositories/dune-typetree
cd dune-typetree
git checkout releases/2.3
cd ..

# download dune-PDELab
git clone http://git.dune-project.org/repositories/dune-pdelab
cd dune-pdelab
git checkout releases/2.0
cd ..

# download dune-multidomaingrid
git clone git://github.com/smuething/dune-multidomaingrid.git
cd dune-multidomaingrid
git checkout releases/2.3
cd ..

# download dune-multidomain
git clone git://github.com/smuething/dune-multidomain.git
cd dune-multidomain
git checkout releases/2.0
cd ..

# download DuMuX
svn co svn://svn.iws.uni-stuttgart.de/DUMUX/dumux/trunk dumux
