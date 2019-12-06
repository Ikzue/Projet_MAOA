#include <ilcplex/ilocplex.h>
#include <vector>
#include"../../Graph/Graph.h"

#define epsilon 0.00001

using namespace::std;


//Circuit inequality separation algorithm when x is integer
void  find_ViolatedAcyclicCst_INTEGER(IloEnv env, C_Graph & G,  vector<IloNumVar>& x, vector<float>&fracsol, list<IloRange> & L_ViolatedCst){

  vector<int> sol;
  list<int> L;
  int i;

  sol.resize(G.nb_nodes);

  // Some "integer" value of CPLEX are not exactly integer...
  for (i=0;i<G.nb_nodes;i++)
    if (fracsol[i]>epsilon) sol[i]=1;
    else sol[i]=0;

  if (G.return_circuit(sol,L)){

    // Found a violated inequality -> add to violatedCte structure
    IloExpr expr(env);
    list<int>::const_iterator it;
    for (it=L.begin();it!=L.end();it++)      
      expr+=x[*it];

    i=L.size();
    IloRange newCte = IloRange(expr <= i - 1);
    L_ViolatedCst.push_back(newCte);

  }

}



//Circuit inequality separation algorithm
void  find_ViolatedAcyclicCst(IloEnv env, C_Graph & G,  vector<IloNumVar>& x, vector<float>&fracsol, list<IloRange> & L_ViolatedCst){

  int nbcte=0;
  int i,j,u,k;
  vector<int> T;
  vector<float> dist;
  vector<float> sol2sep;
  float test;
  vector<int> lotohat;
  int circuitsize;

  T.resize(G.nb_nodes);
  dist.resize(G.nb_nodes);
  lotohat.resize(G.nb_nodes);

  // Put the linear relaxation values on the edges of graph G

  list<C_link *>::const_iterator it;
  
  for (i=0;i<G.nb_nodes;i++)
    for (it=G.V_nodes[i].L_adjLinks.begin();it!=G.V_nodes[i].L_adjLinks.end();it++){
      (*it)->algo_cost=(2-fracsol[i]-fracsol[(*it)->v2])/2.0;
      if((*it)->algo_cost<epsilon)(*it)->algo_cost=0 ;
    }


  // Randomly scrumble the nodes of G 

  for (i=0;i<G.nb_nodes;i++){
    lotohat[i]=i;
  }

  for (i=0;i<G.nb_nodes;i++){
    j=rand()%G.nb_nodes;
    k=lotohat[i];
    lotohat[i]=lotohat[j];
    lotohat[j]=k;
  }


  // Find a shortest circuit for every node i

  i=0;
  while ((L_ViolatedCst.size()<200) && (i<G.nb_nodes)){
    
    G.Directed_ShortestPathTree(lotohat[i],T,dist);

    for (j=0;j<G.nb_nodes;j++){

      if ( (j!=lotohat[i]) && (T[j]!=-2) && (G.V_nodes[j].test_successor(lotohat[i]) ) ){

	test=dist[j]+(2-fracsol[lotohat[i]]-fracsol[j])/2.0;

	  if (test<1-epsilon) {
	    // Found a violated inequality -> add to violatedCte structure
	    IloExpr expr(env);
	    circuitsize=0;
	    u=j;

	    while (u!=-1){
	      expr+=x[u];
	      u=T[u];
	      circuitsize++;
	    }

	    IloRange newCte = IloRange(expr <= circuitsize - 1);
	    L_ViolatedCst.push_back(newCte);

	    nbcte++;
	  }
      }

    }
    i++;

  }


}
