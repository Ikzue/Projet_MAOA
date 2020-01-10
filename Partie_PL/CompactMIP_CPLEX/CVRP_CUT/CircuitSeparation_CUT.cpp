#include <ilcplex/ilocplex.h>
#include <vector>
#include"../../Graph/Graph.h"

#define epsilon 0.00001

using namespace::std;


//Circuit inequality separation algorithm when x is integer
void  find_ViolatedCycle_INTEGER(IloEnv env, C_Graph & G,  vector<vector<IloNumVar>>& x, vector<vector<float>>&fracsol, list<IloRange> & L_ViolatedCst){

  list<pair<int,int>> sol;
  list<pair<int,int>> L;
  int i,j;

  // Some "integer" value of CPLEX are not exactly integer...
  for (i=0;i<G.nb_nodes;i++){
      for(j=0;j<G.nb_nodes;j++){
            if (fracsol[i][j]>epsilon) sol.push_back(make_pair(i,j));
      }
  }

  if (G.return_cycle_CVRP(sol,L)){
      cout<<"cycle" << endl;
    list<int> W;
    list<int> notW;
    bool is_in_cycle;
    list<pair<int,int> >::const_iterator it;
    for (it=L.begin();it!=L.end();it++){
        cout << it->first << " " << it->second << endl;
    }
    for(i=0;i<G.nb_nodes;i++){
        is_in_cycle = false;
            for (it=L.begin();it!=L.end();it++){
                if(it->first == i || it->second == i){
                is_in_cycle = true;
                break;
                }
            }
            if(is_in_cycle){
            W.push_back(i);
            }
            else{
            notW.push_back(i);
            }
    }

    // Found a violated inequality -> add to violatedCte structure
    IloExpr expr(env);
    list<int>::const_iterator itW;
    list<int>::const_iterator itnotW;
    int demande = 0;
    int Q;
    cout << "W" << endl;
    for (itW = W.begin(); itW != W.end(); itW++){
        cout << *itW << endl;
        demande += G.V_nodes[*itW].weight;
    }
    cout << "notW" << endl;
    for (itnotW = notW.begin(); itnotW != notW.end(); itnotW++){
        cout << *itnotW << endl;
    }
    if(!notW.empty()){
        for (itW = W.begin(); itW != W.end(); itW++){
            for (itnotW = notW.begin(); itnotW != notW.end(); itnotW++){
                expr+=x[*itW][*itnotW];
            }
        }
        Q = G.truck_capacity;
        cout << "demande ";
        cout << demande << endl;
        cout << "Q ";
        cout << Q << endl;
        cout << "sup(demande/Q) ";
        cout << ceil(float(demande)/Q) << endl;
        IloRange newCte = IloRange(expr >= ceil(float(demande)/Q));
        L_ViolatedCst.push_back(newCte);
        }
    else{
        IloExpr expr2(env);
        for (j=1;j<G.nb_nodes;j++){
            expr+=x[0][j];
            expr2+=x[j][0];
        }
        IloRange newCte = IloRange(expr >= 2);
        cout << expr2 << endl;
        IloRange newCte2 = IloRange(expr2 >= 2);
        L_ViolatedCst.push_back(newCte);
        L_ViolatedCst.push_back(newCte2);
    }
    }
}
