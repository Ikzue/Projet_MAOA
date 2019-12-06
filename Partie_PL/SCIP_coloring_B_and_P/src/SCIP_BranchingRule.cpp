#include"SCIP_BranchingRule.h"
#include"SCIP_BranchingHandler.h"

//#define OUTPUT_BRANCHRULE

#define eps 1e-6

using namespace std;

SCIP_RETCODE BranchingRule::scip_execlp(SCIP* scip, SCIP_BRANCHRULE* branchrule, SCIP_Bool allowaddcons, SCIP_RESULT* result) {

#ifdef OUTPUT_BRANCHRULE
  cout << " --------------------- Branching Rule EXECLP ---------------  \n";
  cout << "Nombre de noeuds actuel : " << SCIPgetNNodes(scip) << std::endl;
#endif

  SCIP_ConsData *consdata;
  
  SCIP_NODE* node = SCIPgetCurrentNode(scip);


 if (node->conssetchg!=NULL) { 
    consdata=SCIPconsGetData(node->conssetchg->addedconss[0]);
    #ifdef OUTPUT_BRANCHRULE
    cout<<"Consdata non null"<<endl;
    #endif
  }
  else {
    consdata=NULL;
    #ifdef OUTPUT_BRANCHRULE
    cout<<"Consdata null"<<endl;
    #endif

  }

#ifdef OUTPUT_BRANCHRULE
    
  cout<<"Treated Node : ";
  if (consdata!=NULL) {
    list<int>::iterator it;
    for (it=consdata->var->L_nodes.begin();it!=consdata->var->L_nodes.end();it++)
      cout<<*it<<" ";
    cout<<endl;
  }
  else{
    cout<<"root"<<endl;
  }
#endif

  // Search the "most fractional" variables
  C_master_var *branching_var=NULL;
  list<C_master_var*>::const_iterator itv;
  SCIP_Real tmp, bestfrac = 1;
  for(itv = M->L_var.begin(); itv!=M->L_var.end(); itv++){
    tmp = fabs(SCIPgetVarSol(scip,(*itv)->ptr));
    if( (tmp > eps )&& (tmp < 1-eps) && (fabs(tmp - 0.5) < bestfrac) ){
      bestfrac = fabs(tmp - 0.5);
      branching_var=*itv;
    }
  }
  if (branching_var!=NULL){

    #ifdef OUTPUT_BRANCHRULE
    cout<<"Branch on var : ";
    list<int>::const_iterator iti;
    for (iti=branching_var->L_nodes.begin();iti!=branching_var->L_nodes.end();iti++)
      cout<<*iti<<" ";
    cout<<" of value : "<<SCIPgetVarSol(scip,branching_var->ptr)<<endl;
    #endif
  
    SCIP_NODE *newnode;
    SCIP_CONS *newcons;

	
    // first node
    SCIPcreateChild(scip, &newnode, 1000.0, SCIPgetLocalTransEstimate(scip));
    create_CteBranch(scip, branching_var, 0 ,consdata, &newcons );
    SCIPaddConsNode(scip, newnode, newcons, NULL);
    SCIPreleaseCons(scip, &newcons);
  
    // second node
    SCIPcreateChild(scip, &newnode, 1000.0, SCIPgetLocalTransEstimate(scip));
    create_CteBranch(scip, branching_var, 1 ,consdata, &newcons );
    SCIPaddConsNode(scip, newnode, newcons, NULL);
    SCIPreleaseCons(scip, &newcons);

    *result = SCIP_BRANCHED;
  }
  else{
    cout<<"Every variable is integer!!!!!!"<<endl;
    *result = SCIP_CUTOFF;
  }
  
  #ifdef DEBUG
  cout << "\n*****END OF Branching Rule EXECLP ****\n";
  cout << "****************************************\n";
  #endif
    
  return SCIP_OKAY;

}
