# Parallel Preflow-Push with MPI
This is my final project for EC504, in which my goal was to implement a parallel version of the pre-flow push algorithm for finding maximum flow. The version of the pre-flow push algorithm I chose to implement is based on the Pulse Algorithm developed by Andrew Vladislav. Vladislav published his algorithm, which is used to implement pre-flow push in a parallel environment, in his 1987 Ph.D. dissertation Efficient Graph Algorithms for Sequential and Parallel Computers by Andrew Vladislav Goldberg (Pg 33-34). The full text can be found at http://hdl.handle.net/1721.1/14912.

I also provide in instructions for running my code on the BU Shared Computing Cluster (SCC), so that you can have every process run on a separate node. My performance results from this are shared in my report.

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


## Input graph file (I have provided graph.txt as an example)

The program expects one argument: a text file containing an adjacency matrix for the input graph. The format of the text file is as follows:
 ```   
  #Vertices
  #Edges
  Rank of source
  Rank of sink
  Adjacency matrix (Each entry in the adjacency matrix contains either the capacity of an edge, or a 0 if the edge does not exist)
  The number 0 (used to indicate the end)
 ```
 
## Compilation and Execution

This code was written in C. However, you must be careful to adhere to the following compilation instructions, or else the code will not run properly.
    
#### To compile: 

```mpicc parallel_preflowpush.c```

#### To run on a PC: 

```mpiexec -n #numprocs a.out graph.txt```

Replace #numprocs with the number of vertices in the graph
Ex: To run with 8 vertices:
    
 ```mpiexec -n 8 a.out graph.txt```

 (if mpiexec does not work, replace with mpirun)

#### To run on the SCC: 
Let V = total number of vertices
Let N = V/28

```qsub -pe mpi_28_tasks_per_node V -b y "mpirun -npernode N ./a.out graph.txt"```

Ex: to run with 448 vertices (V = 448, N = 28)

```qsub -pe mpi_28_tasks_per_node 448 -b y "mpirun -npernode 28 ./a.out graph.txt"```


