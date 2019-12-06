#include <ilcplex/ilocplex.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

#include"../../Graph/Graph.h"

#define epsilon 0.00001

using namespace std;

#define OUTPUT

///////////////////////////////
///////////////////////////////

// The "famous" formulation for the TSP
// Given a complete undirected graph G=(V,E)

// Min sum_{i=1 to n} sum{j=1 to n, i!=j}  c_ij x_ij
//   sum_{j=1 to n, j!=i} x_ij = 2   for all node i=1 to n
//   sum_{i in W} sum_{j not in W} x_ij>=2 for every subset of node W  (W!= 0 and W!= V)
//   x_ij in {0,1} for every arc (i,j) i=1 to n, j=1 to n i!=j

///////////////////////////////
///////////////////////////////


#define MAX_ITER 0


///////////////////////////////
////// SEPARATION FCT ////////
/////////////////////////////

// declaration of the functions that are dispatched outstide this file
bool find_ViolatedMengerCutCst_INTEGER(IloEnv env, C_Graph & G,  vector<vector<IloNumVar> >& x, IloRange & ViolatedCst);


bool find_ViolatedMengerCutCst(IloEnv env, C_Graph & G,  vector<vector<IloNumVar> >& x, IloRange & ViolatedCst);


// Cplex callback that needs to be in the same file as the main one

// Necessary inequalities
ILOLAZYCONSTRAINTCALLBACK2(LazyMengerCutSeparation,
			   C_Graph &, G,			 
			   vector<vector<IloNumVar> >&,x
		    ){
  #ifdef OUTPUT
  cout<<"*********** Lazy separation Callback *************"<<endl;
  #endif

  int i;
  list<C_link *>::const_iterator it;
  IloRange ViolatedCst;

  #ifdef OUTPUT
  cout<<"Solution a couper "<<endl;
  for (i=0;i<G.nb_nodes;i++)
    for (it=G.V_nodes[i].L_adjLinks.begin();it!=G.V_nodes[i].L_adjLinks.end();it++){
      if (i<(*it)->return_other_extrem(i)){
	if (getValue(x[i][(*it)->return_other_extrem(i)])>=1-epsilon)
	  cout<<"("<<i<<","<<(*it)->return_other_extrem(i)<<") ";
      }
    }
  cout<<endl;
  #endif
  
  // Put the INTEGER values x on the edges of graph G
  // Be carefull... Cplex can send -0.00001 for 0 or 0.000099 for 1

  for (i=0;i<G.nb_nodes;i++)
    for (it=G.V_nodes[i].L_adjLinks.begin();it!=G.V_nodes[i].L_adjLinks.end();it++){
      if (i<(*it)->return_other_extrem(i)){
	(*it)->algo_cost= getValue(x[i][(*it)->return_other_extrem(i)]);
	if((*it)->algo_cost<epsilon) (*it)->algo_cost=0 ;
	else (*it)->algo_cost=1 ;
      }
    }

  /* Separation of Cut inequalities for integer inequalities*/

  if (find_ViolatedMengerCutCst_INTEGER(getEnv(),G,x, ViolatedCst)){
    #ifdef OUTPUT
    cout << "Adding constraint : "<<endl;
    cout<< ViolatedCst << endl;
    #endif
    add(ViolatedCst,IloCplex::UseCutPurge);   // UseCutForce UseCutPurge UseCutFilter
  }
  #ifdef OUTPUT
    else {
      cout<<"No Cst found"<<endl;
    }
  #endif 

}


// Usefull inequalities (here are the same as the necessary ones)
ILOUSERCUTCALLBACK2(UsercutMengerCutSeparation,
			   C_Graph &, G,
			   vector<vector<IloNumVar> >&,x
		    ){
  #ifdef OUTPUT
  cout<<"********* UserCut separation Callback *************"<<endl;
  #endif

  int i;
  list<C_link *>::const_iterator it;
  IloRange ViolatedCst;
  
  // Put the linear relaxation values on the edges of graph G

  for (i=0;i<G.nb_nodes;i++)
    for (it=G.V_nodes[i].L_adjLinks.begin();it!=G.V_nodes[i].L_adjLinks.end();it++){
      if (i<(*it)->return_other_extrem(i)){
	(*it)->algo_cost= getValue(x[i][(*it)->return_other_extrem(i)]);
	if((*it)->algo_cost<epsilon)(*it)->algo_cost=0 ;
      }
    }

  /* Separation of Cut inequalities for fractional point*/

  if (find_ViolatedMengerCutCst(getEnv(),G,x, ViolatedCst)){

    #ifdef OUTPUT
    cout << "Adding constraint : "<<endl;
    cout<< ViolatedCst << endl;
    #endif
    add(ViolatedCst,IloCplex::UseCutPurge);   // UseCutForce UseCutPurge UseCutFilter
  }
  #ifdef OUTPUT
    else {
      cout<<"No Cst found"<<endl;
    }
  #endif
}




//////////////////////////////////////
/////   PRIMAL HEURISTIC          ////
//////////////////////////////////////

class sortingEdge{
public:
  int v1,v2;
  double val;

  bool operator<(sortingEdge& e){
    return (val>e.val);
  }
};


// Try to produce a heuristic solution from fractional solution
ILOHEURISTICCALLBACK2(PrimalHeuristic,
		      C_Graph &, G,
		      vector<vector<IloNumVar> >&,x
		      ){

  
  #ifdef OUTPUT
  cout << "******* Primal Heuristic ********" << endl;  
  #endif
  
  int i,j,oldi;
  list<sortingEdge> L;
  double value;

  
  
  for (i=0;i<G.nb_nodes;i++)
    for (j=i+1;j<G.nb_nodes;j++){
      sortingEdge e;
      e.v1=i;
      e.v2=j;
      e.val=getValue(x[i][j]);
      L.push_back(e);
    }
  L.sort();

  vector<int> CC;
  CC.resize(G.nb_nodes);
  for (i=0;i<G.nb_nodes;i++) CC[i]=-1;

  vector<pair<int,int> > tour;
  tour.resize(G.nb_nodes);
  for (i=0;i<G.nb_nodes;i++){
    tour[i].first=-1;
    tour[i].second=-1;
  }
  
  int cpt=0;
  sortingEdge e;
  while ( (cpt!=G.nb_nodes-1) && (!L.empty()) ){

    e=L.front();
    L.pop_front();

    if (  ( (CC[e.v1]==-1) || (CC[e.v2]==-1) || (CC[e.v1]!=CC[e.v2]) )
	  && (tour[e.v1].second==-1) && (tour[e.v2].second==-1) ) {
      cpt++;

      if (tour[e.v1].first==-1){
	tour[e.v1].first=e.v2;
	CC[e.v1]=e.v1;
      }
      else{
	tour[e.v1].second=e.v2;
      }

      if (tour[e.v2].first==-1){
	tour[e.v2].first=e.v1;
	CC[e.v2]=CC[e.v1];
      }
      else{	
	tour[e.v2].second=e.v1;
	oldi=e.v1;
	i=e.v2;
	while (tour[i].second!=-1){

	  if (tour[i].second==oldi) j=tour[i].first;
    	                       else j=tour[i].second;
	  CC[j]=CC[e.v1];
	  oldi=i;
	  i=j;
	}
		
      }
    }
  }

 
  int i1=-1,i2;
  for (i=0;i<G.nb_nodes;i++)
    if (tour[i].second==-1)
       if (i1==-1) i1=i;
              else i2=i;

  tour[i1].second=i2;
  tour[i2].second=i1;

  IloIntVarArray solx(getEnv(),(G.nb_nodes*(G.nb_nodes-1))/2,0.0,1.0);
  IloNumArray valx(getEnv(),(G.nb_nodes*(G.nb_nodes-1))/2);

 
  value=0;
  cpt=0;
  int cc=0;
  for (i=0;i<G.nb_nodes;i++){

    for (j=i+1;j<G.nb_nodes;j++){
      solx[cpt]=x[i][j];
      if ((j!=tour[i].first)&&(j!=tour[i].second))
  	valx[cpt]=0;      
      else {
  	valx[cpt]=1;  
  	//cout<<"("<<i<<","<<j<<") ";
  	value+=G.lengthTSP(i,j);
  	cc++;
      }
      cpt++;
    }

    
  }
  
  

  //cout<<"Heuristic solution found: "<<value<<endl;
  setSolution(solx,valx,value);

  if (value<getIncumbentObjValue()){
    ofstream ficsol("att48.tour");
    for (i=0;i<G.nb_nodes;i++){
      if (i<tour[i].first)
	ficsol<<i<<" "<<tour[i].first<<endl;
      if (i<tour[i].second)
	ficsol<<i<<" "<<tour[i].second<<endl;
    }  
    ficsol.close();
  }



}


void greedy_heuristic(C_Graph &G, vector<int>& sol){

  int i,k;
  
  vector<int> added;
  added.resize(G.nb_nodes);    
  for (i=0;i<G.nb_nodes;i++) added[i]=0;
  
  int curr=0;
  float next;
  int nexti;
  
  sol[0]=0;
  added[0]=1;
  k=1;
  while (k<G.nb_nodes){

    next=10e6;
    for (i=0;i<G.nb_nodes;i++)
      if (added[i]==0)
	if (next>G.lengthTSP(curr,i)){
	  next=G.lengthTSP(curr,i);
	  nexti=i;
	}
    sol[k]=nexti;
    added[nexti]=1;
    curr=nexti;
    k++;
  }

}

//////////////////////////////////////
/////      MAIN                   ////
//////////////////////////////////////

int main (int argc, char**argv){

  string name, nameext, nameextsol;
  int i,j,k;

  vector<int> sol;


  //////////////
  //////  DATA
  //////////////

  if(argc!=2){
    cerr<<"usage: "<<argv[0]<<" <TSP file name>   (without .tsp)"<<endl; 
    return 1;
  }

  name=argv[1];
  nameext=name+".tsp";
  nameextsol=name+".tour";

  ifstream fic(nameext.c_str());

  if (!fic){
    cerr<<"file "<<nameext<<" not found"<<endl;
    return 1;
  }

  C_Graph G;

  G.read_undirected_complete_TSP(fic);

  fic.close();


  //////////////
  //////  CPLEX INITIALIZATION
  //////////////


  IloEnv   env;
  IloModel model(env);



  ////////////////////////
  //////  VAR
  ////////////////////////


  vector<vector<IloNumVar> > x;
  x.resize(G.nb_nodes);
  
  for (i=0;i<G.nb_nodes;i++)
    x[i].resize(G.nb_nodes);
  
  for (i=0;i<G.nb_nodes;i++){
    for (j=i+1;j<G.nb_nodes;j++) {
	x[i][j]=IloNumVar(env, 0.0, 1.0, ILOINT);
	ostringstream varname;
	varname.str("");
	varname<<"x_"<<i<<"_"<<j;
	x[i][j].setName(varname.str().c_str());
	x[j][i]=x[i][j];
    }
  }
  


  //////////////
  //////  CST
  //////////////

  IloRangeArray CC(env);
  int nbcst=0;
   
  //   sum_{j=1 to n, j!=i} x_ij = 2  for all node i=1 to n
  for (i=0;i<G.nb_nodes;i++){
    IloExpr c1(env);
    for (j=0;j<G.nb_nodes;j++)
      if (i!=j)
	c1+=x[i][j];
    CC.add(c1==2);
    ostringstream nomcst;
    nomcst.str("");
    nomcst<<"CstDeg_"<<i;
    CC[nbcst].setName(nomcst.str().c_str());
    nbcst++;
  }

  IloExpr c2(env);
  for (i=0;i<G.nb_nodes;i++)
    for (j=i+1;j<G.nb_nodes;j++) 
      c2+=x[i][j];
  CC.add(c2==G.nb_nodes);
  ostringstream nomcst;
  nomcst.str("");
  nomcst<<"CstNm1";
  CC[nbcst].setName(nomcst.str().c_str());
  nbcst++;


  model.add(CC);

   // And Finally
  IloCplex cplex(model);

  
  /// ADD SEPARATION CALLBACK
  cplex.use(LazyMengerCutSeparation(env,G,x));
  //cplex.use(UsercutMengerCutSeparation(env,G,x));



  //////////////
  ////// OBJ
  //////////////
  
  IloObjective obj=IloAdd(model, IloMinimize(env, 0.0));
  
  for (i=0;i<G.nb_nodes;i++)
    for (j=i+1;j<G.nb_nodes;j++)
	obj.setLinearCoef(x[i][j],G.lengthTSP(i,j));
 

  ///////////
  //// RESOLUTION
  //////////

  // ADD CHECK SOLUTION FEASABILITY

  cplex.use(CheckSolFeas(env,G,x));


  // ADD A PRIMAL HEURISTIC
  cplex.use(PrimalHeuristic(env,G,x));
  
  // cplex.setParam(IloCplex::Cliques,-1);
  // cplex.setParam(IloCplex::Covers,-1);
  // cplex.setParam(IloCplex::DisjCuts,-1);
  // cplex.setParam(IloCplex::FlowCovers,-1);
  // cplex.setParam(IloCplex::FlowPaths,-1);
  // cplex.setParam(IloCplex::FracCuts,-1);
  // cplex.setParam(IloCplex::GUBCovers,-1);
  // cplex.setParam(IloCplex::ImplBd,-1);
  // cplex.setParam(IloCplex::MIRCuts,-1);
  // cplex.setParam(IloCplex::ZeroHalfCuts,-1);
  // cplex.setParam(IloCplex::MCFCuts,-1);
  // cplex.setParam(IloCplex::MIPInterval,1);
  // cplex.setParam(IloCplex::HeurFreq,-1);
  // cplex.setParam(IloCplex::ClockType,1);
  // cplex.setParam(IloCplex::RINSHeur,-1);


  #ifdef OUTPUT
  cout<<"Wrote LP on file"<<endl;
  cplex.exportModel("sortie.lp");
  #endif

  //START FROM A HEURISTIC SOLUTION
  vector<int> start_sol;
  start_sol.resize(G.nb_nodes);
  
  greedy_heuristic(G,start_sol);

  // Translate from encoding by a list of nodes to variable x
  vector<vector<int> > startx;
  startx.resize(G.nb_nodes);

  for (i=0;i<G.nb_nodes;i++)  startx[i].resize(G.nb_nodes);

  for (i=0;i<G.nb_nodes;i++)
    for (j=i+1;j<G.nb_nodes;j++)
      startx[i][j]=0;

  for(k=1; k<G.nb_nodes;k++) {
    if (start_sol[k-1]<start_sol[k])
      startx[start_sol[k-1]][start_sol[k]]=1;
    else
      startx[start_sol[k]][start_sol[k-1]]=1;
  }
  startx[start_sol[0]][start_sol[G.nb_nodes-1]]=1;

 
  IloNumVarArray startVar(env);
  IloNumArray startVal(env);
  for (i=0;i<G.nb_nodes;i++)
    for (j=i+1;j<G.nb_nodes;j++) {
             startVar.add(x[i][j]);
             startVal.add(startx[i][j]); // startx is your heuristic values
         }
  cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartCheckFeas);
  startVal.end();
  startVar.end();

  
  // LAUNCH CPLEX
  
  if ( !cplex.solve() ) {
    env.error() << "Failed to optimize LP" << endl;
    exit(1);
  }
    
 
  env.out() << "Solution status = " << cplex.getStatus() << endl;
  env.out() << "Solution value  = " << cplex.getObjValue() << endl;


  list<pair<int,int> >   Lsol;
  for(i = 0; i < G.nb_nodes; i++)
     for (j=i+1;j<G.nb_nodes;j++)
	if (cplex.getValue(x[i][j])>1-epsilon)
	  Lsol.push_back(make_pair(i,j));
  

  //////////////
  //////  CPLEX's ENDING
  //////////////

  env.end();

  //////////////
  //////  OUTPUT
  //////////////


  list<pair<int,int> >::const_iterator itp;
 
  ofstream ficsol(nameextsol.c_str());
  double best_length=0;
  for(itp = Lsol.begin(); itp!=Lsol.end();itp++) {
    best_length+=G.lengthTSP(itp->first,itp->second);
    ficsol<<itp->first<<" "<<itp->second<<endl;
  }
 
  ficsol.close();

  cout<<"Tour found of value : "<<best_length<<endl;



  return 0;
}
