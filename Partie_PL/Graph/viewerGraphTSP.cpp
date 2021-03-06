#include<iostream>
#include<string>
#include<sstream>
#include<iomanip>

#include"Graph.h"

using namespace std;


int main(int argc,char**argv){

  string name, nameext;
  int i,j;

  if(argc!=2){
    cerr<<"usage: "<<argv[0]<<" <TSP file name without exension>"<<endl;
    cerr<<"    Graph instance having the TSP format (*.vrp)"<<endl;
    return 1;
  }

  name=argv[1];
  nameext=name+".vrp";


  ifstream fic(nameext.c_str());

  if (!fic){
    cerr<<"file "<<nameext<<" "<<" not found"<<endl;
    return 1;
  }

  C_Graph G;

  G.read_undirected_complete_CVRP(fic);

  fic.close();

  G.write_SVG_node_cloud(name.c_str());
  
  list<pair<int,int>> L;
  list<pair<int,int>> sol;
  sol.clear();

  sol.push_back(make_pair(0,20));
  sol.push_back(make_pair(20,27));
  sol.push_back(make_pair(27,23));
  sol.push_back(make_pair(23,3));
  sol.push_back(make_pair(3,2));
  sol.push_back(make_pair(2,13));
  sol.push_back(make_pair(13,7));
  sol.push_back(make_pair(7,1));
  sol.push_back(make_pair(1,0));
  G.return_cycle_CVRP(sol,L);
  
  
  list<pair<int,int> >::const_iterator it;
  for (it=L.begin();it!=L.end();it++){
    cout << it->first << ", " << it->second << endl;
  }



  return 0;
}
