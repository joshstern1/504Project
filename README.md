#Parallel Preflow-Push with MPI

## Setting up MPI
If you are running the code on a system with MPI already installed, you may skip this section. If you are running this code on your PC, and you do not have MPI installed, download MPICH-3.2 from:

http://www.mpich.org/static/downloads/3.2.1/mpich-3.2.1.tar.gz

Installation instructions: (from: https://www.mpich.org/static/downloads/3.2/mpich-3.2-installguide.pdf)

1. Unpack the tar file:     

```tar xfz mpich.tar.gz```

2. Create an installation directory:     

```mkdir /home/you/mpich-install```

3. Create a build directory:    

```mkdir /tmp/you/mpich-3.0.2```

4. Configure MPICH:     

```cd /tmp/you/mpich-3.0.2 /home/you/libraries/mpich-3.0.2/configure -prefix=/home/you/mpich-install |& tee c.txt```

5.  Build MPICH:

```make 2>&1 | tee m.txt```

6. Install MPICH commands:

```make install 2>&1 | tee mi.txt```


