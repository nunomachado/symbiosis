package pt.tecnico.symbiosis.transformer;

import java.util.Iterator;
import java.util.Map;

import pt.tecnico.symbiosis.context.RefContext;
import soot.Body;
import soot.BodyTransformer;
import soot.SootField;
import soot.SootMethod;
import soot.Value;
import soot.jimple.AssignStmt;
import soot.jimple.ConcreteRef;
import soot.jimple.InstanceFieldRef;
import soot.jimple.StaticFieldRef;
import soot.jimple.Stmt;
import soot.util.Chain;


/**
 * Body transformer that finds shared variables.
 * This pass is used as a pre-processing phase to
 * log information on accesses to shared variables.
 * @author nunomachado
 *
 */
public class SymbFindSVPass extends BodyTransformer{
	
	public static SymbFindSVPass instance = new SymbFindSVPass();
	public SymbFindSVPass() {}

	public static SymbFindSVPass v() { return instance; }

	@Override
	protected void internalTransform(Body body, String phase, Map options) 
	{
		Chain units = body.getUnits();
		Iterator stmtIt = units.snapshotIterator();  

		SootMethod m = body.getMethod();
		
		while (stmtIt.hasNext()) 
		{
			Stmt s = (Stmt) stmtIt.next();
			checkForSharedVars(m,s);
		}
	}

	
	/**
	 * Checks whether the current stmt has an access to a shared variable.
	 * @param s
	 */
	public static void checkForSharedVars(SootMethod sm, Stmt s)
	{
		if(s instanceof AssignStmt)
		{
			Value left = ((AssignStmt)s).getLeftOp();
			Value right = ((AssignStmt)s).getRightOp();
			RefContext context;
			
			//handle write accesses
			if (left instanceof ConcreteRef) {
				
				//context = LHSContextImpl.getInstance();
				//if (context != RHSContextImpl.getInstance())
				//{
					if (((ConcreteRef) left) instanceof InstanceFieldRef)
					{
						InstanceFieldRef fieldRef = ((InstanceFieldRef) left);
						SootField field  = fieldRef.getField();
						String sig = field.getDeclaringClass().getName()+"."+fieldRef.getField().getName()+".INSTANCE";
						
						if(SymbiosisTransformer.ftea.isFieldThreadShared(field) && !SymbiosisTransformer.tlo.isObjectThreadLocal(fieldRef, sm))
						{
							SymbiosisTransformer.sharedVars.add(sig);
						}
					}
					else if(((ConcreteRef) left) instanceof StaticFieldRef)
					{
						StaticFieldRef fieldRef = ((StaticFieldRef) left);
						SootField field  = fieldRef.getField();
						String sig = field.getDeclaringClass().getName()+"."+fieldRef.getField().getName()+".STATIC";
						
						if(SymbiosisTransformer.ftea.isFieldThreadShared(field) && !SymbiosisTransformer.tlo.isObjectThreadLocal(fieldRef, sm))
						{
							SymbiosisTransformer.sharedVars.add(sig);
						}
					}
				//}
		    }
		    
		  //handle read accesses
		    if (right instanceof ConcreteRef) {
		    	if (((ConcreteRef) right) instanceof InstanceFieldRef)
		    	{
		    		InstanceFieldRef fieldRef = ((InstanceFieldRef) right);
		    		SootField field  = fieldRef.getField();
		    		String sig = field.getDeclaringClass().getName()+"."+fieldRef.getField().getName()+".INSTANCE";
		    		
		    		if(SymbiosisTransformer.ftea.isFieldThreadShared(field) && !SymbiosisTransformer.tlo.isObjectThreadLocal(fieldRef, sm))
		    		{
		    			SymbiosisTransformer.sharedVars.add(sig);
		    		}
		    	}
		    	else if(((ConcreteRef) right) instanceof StaticFieldRef)
		    	{
		    		StaticFieldRef fieldRef = ((StaticFieldRef) right);
		    		SootField field  = fieldRef.getField();
		    		String sig = field.getDeclaringClass().getName()+"."+fieldRef.getField().getName()+".STATIC";
		    		
		    		if(SymbiosisTransformer.ftea.isFieldThreadShared(field) && !SymbiosisTransformer.tlo.isObjectThreadLocal(fieldRef, sm))
		    		{
		    			SymbiosisTransformer.sharedVars.add(sig);
		    		}
		    	}
		    }
		}
	}
	
}
