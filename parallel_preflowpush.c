// Copyright 2019 Joshua Stern
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <string.h>
#include <time.h>

#define ITERATIONS 1

int findmin(int a, int b) {
  if (a < b) {
    return a;
  } else {
    return b;
  }
}


int main(int argc, char **argv) {
  if (argc < 2) {
    printf("please provide an input graph file\n");
    exit(1);
  }

  // Initialize MPI
  MPI_Init(NULL, NULL);
  int rank, world_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  double total_time = 0;

  int y;
  for (y = 0; y < ITERATIONS + 1; y++) {

    MPI_Barrier(MPI_COMM_WORLD);

    // Open the input graph file
    FILE *fp = fopen(argv[1], "r");
    int vertex_count, edge_count, start, end;
    char buf[100];

    // get graph parameters
    fgets(buf, 20, fp);
    buf[strlen(buf)-1] = '\0';
    vertex_count = atoi(buf);

    if((vertex_count != world_size)&&(rank == 0)){
      printf("Please make sure that the number of vertices in the graph is equal to the number of MPI processes\n");
      exit(1);
    }

    fgets(buf, 20, fp);
    buf[strlen(buf)-1] = '\0';
    edge_count = atoi(buf);

    fgets(buf, 20, fp);
    buf[strlen(buf)-1] = '\0';
    start = atoi(buf);

    fgets(buf, 20, fp);
    buf[strlen(buf)-1] = '\0';
    end = atoi(buf);

    if (rank == 0) {
      //printf("vertex_count = %d\nedge_count = %d\nsource = %d\nsink = %d\n\n", vertex_count, edge_count, start, end);
    }

    // read in adjacency matrix
    int adj_matrix[vertex_count][vertex_count];
    int m = 0, n = 0, j = 0, i = 0;
    while (fgets(buf, 100, fp) != NULL) {
      n = 0;
      char temp_buf[20];
      memset(temp_buf, '\0', 20);
      int k = 0;
      for (j = 0; j < strlen(buf); j++) {
        if (buf[j] >= '0') {
          temp_buf[k] = buf[j];
          k++;
        }
        else {
          k = 0;
          adj_matrix[m][n] = atoi(temp_buf);
          memset(temp_buf, '\0', 20);
          n++;
        }
      }
      m++;
    }

    // to print adjacency matrix
    /*if (rank == 0) {
      for (i = 0; i < vertex_count; i++) {
        for (j = 0; j < vertex_count; j++) {
          printf("%d ", adj_matrix[i][j]);
        }
        printf("\n");
      }
    }*/

    fclose(fp);


    // have each node get a list of vertices edges that it is connected to and their capacities
    int in_edges[vertex_count];
    int out_edges[vertex_count];
    for (i = 0; i < vertex_count; i++) {
      in_edges[i] = adj_matrix[i][rank];
      out_edges[i] = adj_matrix[rank][i];
    }

    double total_time = 0;
    clock_t t_begin = clock();

    // Use Breadth First Search to determine height labels
    int height = -1;
    if (rank == end) {
      height = 0;
    }
    int temp_height[vertex_count];
    for (i = 0; i < vertex_count; i++) {
      temp_height[i] = -1;
    }

    MPI_Request request[vertex_count];
    MPI_Status status[vertex_count];
    if (rank != end) {
      for (i = 0; i < vertex_count; i++) {
        if (out_edges[i] > 0) {
            MPI_Irecv(&temp_height[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD, &request[i]);
        }
      }
      for (i = 0; i < vertex_count; i++) {
        if (out_edges[i] > 0) {
          MPI_Wait(&request[i], &status[i]);
          if ((height == -1) || (temp_height[i] < height)) {
            height = temp_height[i] + 1;
          }
        }
      }
    }
    for (i = 0; i < vertex_count; i++) {
      if (in_edges[i] > 0) {
        MPI_Send(&height, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      }
    }
    if (rank == start) {
      height = vertex_count;
    }

    // each node has a list of the heights of all adjacent vertices
    int heights[vertex_count];
    heights[rank] = height;
    for (i = 0; i < vertex_count; i++) {
      heights[i] = 100 * vertex_count;
      if (i == rank) {
        continue;
      }
      if (out_edges[i] > 0) {
        MPI_Send(&height, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        MPI_Recv(&heights[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      if (in_edges[i] > 0) {
        MPI_Send(&height, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        MPI_Recv(&heights[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
    }

    // preflow push from root
    int excess = 0, total_excess = -1;

    if (rank == start) {
      for (i = 0; i < vertex_count; i++) {
        if (out_edges[i] > 0) {
          in_edges[i] = out_edges[i];
          out_edges[i] = 0;
        }
      }
    }
    if (in_edges[start] > 0) {
      excess = in_edges[start];
      out_edges[start] = excess;
      in_edges[start] = 0;
    }

    // create new communicator without start and end nodes
    MPI_Group world_group, intermediate_nodes;
    MPI_Comm INTERMEDIATE_COMM;
    int members[vertex_count-2];
    j = 0;
    for (i = 0; i < vertex_count; i++) {
      if ((i != start) && (i != end)) {
        members[j] = i;
        j++;
      }
    }

    // Create a new communicator that includes all nodes except the source and sink nodes
    // We use this new communicator to calculate if any excess flow exists in the graph
    int inter_rank;
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);
    MPI_Group_incl(world_group, vertex_count-2, members, &intermediate_nodes);
    MPI_Comm_create(MPI_COMM_WORLD, intermediate_nodes, &INTERMEDIATE_COMM);
    if ((rank != start) && (rank != end)) {
      MPI_Comm_rank(INTERMEDIATE_COMM, &inter_rank);
    }

    // obtain total excess from intermediate nodes (not including source and sink nodes)
    if ((rank != start) && (rank != end)) {
      MPI_Allreduce(&excess, &total_excess, 1, MPI_INT, MPI_SUM, INTERMEDIATE_COMM);
    }

    int min, max_height;
    int pulse3[vertex_count];
    int counter = 0;

    // run loop while total excess > 0
    while ((total_excess > 0) || (rank == start) || (rank == end)) {

      // Step 1 of Vladislav's Algorithm: Each node pushes its excess flow to its neighbors (send flow data)
      for (i = 0; i < vertex_count; i++) {
        if (i == rank) {
          continue;
        }
        min = 0;
        if ((excess > 0) && (out_edges[i] > 0) && (heights[i] < height)) {
          min = findmin(out_edges[i], excess);
          excess = excess - min;
        }
        MPI_Send(&min, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
        in_edges[i] += min;
        out_edges[i] -= min;
      }

      // Step 2 of Vladislav's Algorithm: Each node recalculates its height
      max_height = height - 1;
      if ((excess > 0) && (rank != end)) {
        max_height = 100*vertex_count;
        for (i = 0; i < vertex_count; i++) {
          if (i == rank) {
            continue;
          }
          if ((heights[i] < max_height) && (out_edges[i] > 0)) {
            max_height = heights[i];
          }
        }
      }
      height = max_height + 1;

      // Step 3 of Vladislav's Algorithm: Each node receives the flow from step 1, and adds any received flow to its own excess
      for (i = 0; i < vertex_count; i++) {
        if (i == rank) {
          continue;
        }
        if (in_edges[i] > 0) {
          MPI_Recv(&pulse3[i], 1, MPI_INT, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          excess += pulse3[i];
          out_edges[i] += pulse3[i];
          in_edges[i] -= pulse3[i];
        }
      }

      // Step 4 of Vladislav's Algorithm: Each node sends its new height to all of its neighbors
      for (i = 0; i < vertex_count; i++) {
        if (i == rank) {
          heights[i] = height;
          continue;
        }
        if ((out_edges[i] > 0) || (in_edges[i] > 0)) {
          MPI_Send(&height, 1, MPI_INT, i, 4, MPI_COMM_WORLD);
        }
      }

      for (i = 0; i < vertex_count; i++) {
        if (i == rank) {
          heights[i] = height;
          continue;
        }
        if ((out_edges[i] > 0) || (in_edges[i] > 0)) {
          MPI_Recv(&heights[i], 1, MPI_INT, i, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
      }

      MPI_Barrier(MPI_COMM_WORLD);


      // Check if there is still any excess flow remaining
      // If yes, break. If no, repeat the loop
      if ((rank != start) && (rank != end)) {
        MPI_Allreduce(&excess, &total_excess, 1, MPI_INT, MPI_SUM, INTERMEDIATE_COMM);
        if (inter_rank == 0) {
          if (total_excess == 0) {
            int marker = -1;
            // If there is no more excess, notify the source and sink nodes so that they know to break
            MPI_Send(&marker, 1, MPI_INT, start, 5, MPI_COMM_WORLD);
            MPI_Send(&marker, 1, MPI_INT, end, 5, MPI_COMM_WORLD);
          }
          else {
            int marker = 1;
            MPI_Send(&marker, 1, MPI_INT, start, 5, MPI_COMM_WORLD);
            MPI_Send(&marker, 1, MPI_INT, end, 5, MPI_COMM_WORLD);
          }
        }
        if (total_excess == 0) {
          break;
        }
      }

      if ((rank == start) || (rank == end)) {
        int buf = 2;
        MPI_Recv(&buf, 1, MPI_INT, MPI_ANY_SOURCE, 5, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (buf == -1) {
          break;
        }
      }

      MPI_Barrier(MPI_COMM_WORLD);
    }

    clock_t t_end = clock();
    double time_spent = (double)(t_end - t_begin) / CLOCKS_PER_SEC;
    if(y != 0){
      total_time += time_spent;
    }

    if ((rank == 0) && (y == ITERATIONS)) {
        printf("time = %fs\n", total_time / ITERATIONS);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    int final_capacity;
    if (rank == end) {
      MPI_Send(&excess, 1, MPI_INT, 0, 6, MPI_COMM_WORLD);
    }

    if ((rank == 0) && (y == ITERATIONS)) {
      MPI_Recv(&final_capacity, 1, MPI_INT, end, 6, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      printf("capcity = %d\n", final_capacity);
    }

    MPI_Barrier(MPI_COMM_WORLD);
  }

  MPI_Finalize();
  return 0;
}
