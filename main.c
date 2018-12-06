#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <string.h>


int main(){

  MPI_Init(NULL, NULL);
  int rank, world_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  if(rank==0){
    printf("world size = %d\n", world_size);
  }


  MPI_Barrier(MPI_COMM_WORLD);

  FILE *fp = fopen("graph.txt", "r");
  int vertex_count, edge_count, start, end;
  char buf[100];

  fgets (buf, 20, fp);
  buf[strlen(buf)-1] = '\0';
  vertex_count = atoi(buf);

  fgets (buf, 20, fp);
  buf[strlen(buf)-1] = '\0';
  edge_count = atoi(buf);

  fgets (buf, 20, fp);
  buf[strlen(buf)-1] = '\0';
  start = atoi(buf);

  fgets (buf, 20, fp);
  buf[strlen(buf)-1] = '\0';
  end = atoi(buf);

  if(rank==0){
    printf("vertex_count = %d\nedge_count = %d\nstart = %d\nend = %d\n\n", vertex_count, edge_count, start, end);
  }

  int adj_matrix[vertex_count][vertex_count];
  int m = 0, n = 0, j = 0, i = 0;
  while (fgets(buf, 100, fp)!=NULL){
    n = 0;
    char temp_buf[20];
    memset(temp_buf, '\0', 20);
    int k = 0;
    for (j = 0; j < strlen(buf); j++){
      if(buf[j] >= '0'){
        temp_buf[k] = buf[j];
        k++;
      }
      else{
        k = 0;
        adj_matrix[m][n] = atoi(temp_buf);
        memset(temp_buf, '\0', 20);
        n++;
      }
    }
    m++;
  }

  if(rank==0){
    for(i = 0; i < vertex_count; i++){
      for(j = 0; j < vertex_count; j++){
        printf("%d ", adj_matrix[i][j]);
      }
      printf("\n");
    }
  }

  fclose(fp);

  //have each node get its local information
  int in_edges[vertex_count];
  int out_edges[vertex_count];
  for(i = 0; i<vertex_count; i++){
    in_edges[i] = adj_matrix[i][rank];
    out_edges[i] = adj_matrix[rank][i];
  }


  //BFS
  int height = -1;
  if(rank == end){
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
  printf("rank: %d, height: %d\n", rank, height);

  //preflow push
  int in_flow = 0, out_flow = 0, excess = 0, total_excess = -1;

  int cur_flow_out[vertex_count];
  int cur_flow_in[vertex_count];
  for (i = 0; i < vertex_count; i++) {
    if (out_edges[i] > 0) {
      cur_flow_out[i] = 0;
    }    else {
      cur_flow_out[i] = -1;
    }
    if (in_edges[i] > 0) {
      cur_flow_in[i] = 0;
    }    else {
      cur_flow_in[i] = -1;
    }
  }

  if(rank == start){
    for (i = 0; i< vertex_count; i++){
      if(out_edges[i] > 0){
        cur_flow_out[i] = out_edges[i];
      }
    }
  }
  if(in_edges[start] > 0){
    cur_flow_in[start] = in_edges[start];
    excess = in_edges[start];
  }

  MPI_Group world_group, intermediate_nodes;
  MPI_Comm INTERMEDIATE_COMM;
  int members[vertex_count-2];
  j = 0;
  for(i = 0; i < vertex_count; i++){
    if((i != start) && (i != end)){
      members[j] = i;
      j++;
    }
  }
  MPI_Comm_group(MPI_COMM_WORLD, &world_group);
  MPI_Group_incl(world_group, vertex_count-2, members, &intermediate_nodes);
  MPI_Comm_create(MPI_COMM_WORLD, intermediate_nodes, &INTERMEDIATE_COMM);

  if((rank != start)&&(rank != end)){
    MPI_Allreduce(&excess, &total_excess, 1, MPI_INT, MPI_SUM, INTERMEDIATE_COMM);
    if(rank == 1){
      printf("excess = %d\n", total_excess);
    }
  }

  while (total_excess != 0){

    break;
  }

  MPI_Finalize();
  return 0;
}
