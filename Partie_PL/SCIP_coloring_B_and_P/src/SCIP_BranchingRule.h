#ifndef BranchingRuleH
#define BranchingRuleH

#include <scip/scip.h>
#include <objscip/objscip.h>

#include"SCIP_master.h"
#include "Graph.h"

using namespace std;


/**
 * Manage the rule decisions
 **/
class BranchingRule : public scip::ObjBranchrule {
  public:

  C_Graph *G;
  C_master_coloring *M;
  //  Cplex_pricing_algo A_cplex;

 BranchingRule(SCIP* scip, C_Graph *GG, C_master_coloring *MM ) : scip::ObjBranchrule(scip, "Branching_Color_Stable_Rule", "", 2000000, -1, 1.0)
    {
      M=MM;
      G=GG;
    };

   virtual ~BranchingRule(){};
   
   /*
     * Exec the branching rule
     *  Possible return values for *result (if more than one applies, the first in the list should be used):
     *  - SCIP_CUTOFF     : the current node was detected to be infeasible
     *  - SCIP_CONSADDED  : an additional constraint (e.g. a conflict clause) was generated; this result code must not be
     *                      returned, if allowaddcons is FALSE
     *  - SCIP_REDUCEDDOM : a domain was reduced that rendered the current LP solution infeasible
     *  - SCIP_SEPARATED  : a cutting plane was generated
     *  - SCIP_BRANCHED   : branching was applied
     *  - SCIP_DIDNOTRUN  : the branching rule was skipped
     */
    SCIP_RETCODE scip_execlp(
	SCIP*              scip,               /** SCIP data structure */
	SCIP_BRANCHRULE*   branchrule,         /** the branching rule itself */
	SCIP_Bool          allowaddcons,       /** should adding constraints be allowed to avoid a branching? */
	SCIP_RESULT*       result              /** pointer to store the result of the branching call */
	);

};


#endif
