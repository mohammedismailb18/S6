#include<stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>

struct graphNode {
    int no,d;   //Node no and distance
    struct graphNode* p;    //parent pointer
};

struct graphEdge{
    int wt; //weight
    struct graphNode *u,*v;
};

typedef struct graphNode Node;
typedef struct graphEdge Edge;

void InitializeSingleSource(int V,Node* NodeList,int s) {
    for(int i=0;i<V;i++) {
        NodeList[i].d = INT_MAX;
        NodeList[i].p = NULL;
    }
    NodeList[s].d = 0;
    NodeList[s].p = &NodeList[s];
}

void Relax(Node* u, Node* v, int w) {
    if(u->d!=INT_MAX) {
        if(v->d > u->d + w) {
            v->d = u->d + w;
            v->p = u;
        }
    }
}

bool BELLMAN_FORD(Node *NodeList, int V, Edge *EdgeList, int E, int s) {
    
    InitializeSingleSource(V,NodeList,s);
    
    for(int i=0;i<V-1;i++) {
        for(int j=0;j<E;j++) {
            Relax(EdgeList[j].u,EdgeList[j].v,EdgeList[j].wt);
            Relax(EdgeList[j].v,EdgeList[j].u,EdgeList[j].wt);
        }
    }

    for(int i=0;i<E;i++) {
        if(EdgeList[i].v->d > EdgeList[i].u->d + EdgeList[i].wt)
            return false;
        if(EdgeList[i].u->d > EdgeList[i].v->d + EdgeList[i].wt)
            return false;
    }
    return true;
}

void printNext(Node *src, Node * dst) {

	if(dst->p==NULL)
		printf("Not Reachable");
	else {
		while(dst->p !=src) 
			dst = dst->p;
		printf("%d", dst->no);
	}

}

void traceRoute(Node *dst) {

	if(dst->p == NULL) {
		printf("Not Reachable");
	}else if(dst->p == dst) {
		printf("%d ", dst->no); 
	}
	else {
		traceRoute(dst->p);
		printf("%u ", dst->no);
	}
}

int main() {
    
    FILE* F1 = fopen("input","r");
    int V,E;

    //reading no.of vertices and edges from input file
    fscanf(F1,"%d%d",&V,&E);
    
    Node* NodeList = (Node*)malloc(V*sizeof(Node));
    Edge* EdgeList = (Edge*)malloc(E*sizeof(Edge));

    for(int i=0;i<E;i++) {
        int u,v,w;
        fscanf(F1,"%d%d%d",&u,&v,&w);
        EdgeList[i].wt = w;
        EdgeList[i].u = &NodeList[u];
        EdgeList[i].v = &NodeList[v];
    }

    for(int i=0;i<V;i++) {
        NodeList[i].no = i;
    }

    for(int i=0;i<V;i++) {
        if(!BELLMAN_FORD(NodeList, V, EdgeList, E, i)) {
			printf("\n Graph contains negative weighted edge\n");
			break;
		}

        printf("-------------------------------\n\tRouter - %d\n-------------------------------\n\n",i);
        printf(" Destination \tHop \tCost \tPath");
		for(int j=0; j<V; j++) {
			printf("\n %d \t\t",j); 
            printNext(NodeList+i, NodeList+j);
			printf("\t%d\t ", NodeList[j].d); 
            traceRoute(NodeList+j);
		}
		printf("\n\n");
    }
    return 0;
}