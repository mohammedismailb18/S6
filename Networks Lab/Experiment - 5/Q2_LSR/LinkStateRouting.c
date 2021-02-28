#include<stdio.h>
#include<stdbool.h>
#include<limits.h>

void InitializeSingleSource(int V,int distance[],bool isSetPath[], int src,int predecessor[]) {
    for(int i=0;i<V;i++) {
        distance[i] = INT_MAX;
        isSetPath[i] = false;
        predecessor[i] = -1;
    }
    distance[src] = 0;
    predecessor[src] = src;
}

int minDistance(int V,int distance[], bool isSetPath[]) 
{  
    int min = INT_MAX, min_index; 
    for (int v = 0; v < V; v++) 
        if (isSetPath[v] == false && distance[v] <= min) { 
            min = distance[v];
            min_index = v; 
        }
    return min_index; 
} 

void printNext(int V, int predecessor[],int src,int dst) {
    if(predecessor[dst] == -1)
        printf("Host unreachable");
    else {
        while(predecessor[dst]!=src) {
            dst = predecessor[dst];
        }
        printf("  %d",dst);
    }
}

void printPath(int V,int predecessor[],int src,int dst) {
    if(predecessor[dst] == -1)
        printf("Host unreachable");
    else if(predecessor[dst] == dst)
        printf("%d ",dst);
    else{
        printPath(V,predecessor,src,predecessor[dst]);
        printf("%d ",dst);
    }
}

void print(int distance[], int V, int src,int predecessor) 
{ 
    printf("-------------------------------\n\tRouter - %d\n-------------------------------\n",src);
    printf("Destination \t Hop \t Distance \t Path\n"); 
    for (int i = 0; i < V; i++) {
            printf("     %d \t\t", i);
            printNext(V,predecessor,src,i);
            printf("    \t %d \t\t",distance[i]); 
            printPath(V,predecessor,src,i);
            printf("\n");
    }
    printf("\n\n");
} 

void dijkstra(int V, int graph[][V], int src) {
    int distance[V];    //distance matrix for storing shortest path to each node form source
    bool isSetPath[V];
    int predecessor[V];
    InitializeSingleSource(V,distance,isSetPath,src,predecessor);
    for (int count = 0; count < V - 1; count++) { 
        //Taking the minimum node among the rest whose shortest path is not set
        int u = minDistance(V,distance, isSetPath);  
        isSetPath[u] = true; 
    
        for (int v = 0; v < V; v++) {
            if (!isSetPath[v] && graph[u][v] && (distance[u] != INT_MAX) 
                && (distance[u] + graph[u][v] < distance[v])) {
                distance[v] = distance[u] + graph[u][v];
                predecessor[v] = u;
            }
        }
    }
    print(distance,V,src,predecessor);
}

int main() {
    //input file 
    FILE* F1 = fopen("input.txt","r");

    int V,E,u,v,w;
    fscanf(F1,"%d%d",&V,&E);
    int graph[V][V];

    for(int i=0;i<V;i++) 
        for(int j=0;j<V;j++)
            graph[i][j] = 0;
    for(int i=0;i<E;i++) {
            fscanf(F1,"%d%d%d",&u,&v,&w);
            graph[u][v] = w;
            graph[v][u] = w;
    }
    
    for(int i=0;i<V;i++) {
        dijkstra(V,graph,i);
    }
    return 0;
}