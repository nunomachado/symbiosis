//Copyright (C) 2007 United States Government as represented by the
//Administrator of the National Aeronautics and Space Administration
//(NASA).  All Rights Reserved.

//This software is distributed under the NASA Open Source Agreement
//(NOSA), version 1.3.  The NOSA has been approved by the Open Source
//Initiative.  See the file NOSA-1.3-JPF at the top of the distribution
//directory tree for the complete NOSA document.

//THE SUBJECT SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY OF ANY
//KIND, EITHER EXPRESSED, IMPLIED, OR STATUTORY, INCLUDING, BUT NOT
//LIMITED TO, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL CONFORM TO
//SPECIFICATIONS, ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
//A PARTICULAR PURPOSE, OR FREEDOM FROM INFRINGEMENT, ANY WARRANTY THAT
//THE SUBJECT SOFTWARE WILL BE ERROR FREE, OR ANY WARRANTY THAT
//DOCUMENTATION, IF PROVIDED, WILL CONFORM TO THE SUBJECT SOFTWARE.

package gov.nasa.jpf.symbc.bytecode;

import gov.nasa.jpf.jvm.KernelState;
import gov.nasa.jpf.jvm.StackFrame;
import gov.nasa.jpf.jvm.SystemState;
import gov.nasa.jpf.jvm.ThreadInfo;
import gov.nasa.jpf.jvm.bytecode.Instruction;
import gov.nasa.jpf.symbc.numeric.Expression;


// we should factor out some of the code and put it in a parent class for all "if statements"
//TODO: to review: approximation

public class IFNULL extends gov.nasa.jpf.jvm.bytecode.IFNULL {
	public IFNULL (int targetPc) {
	    super(targetPc);
	  }
	@Override
	public Instruction execute (SystemState ss, KernelState ks, ThreadInfo ti) {

		StackFrame sf = ti.getTopFrame();
		Expression sym_v = (Expression) sf.getOperandAttr();
		if(sym_v == null) { // the condition is concrete
			//System.out.println("Execute IFEQ: The condition is concrete");
			return super.execute(ss, ks, ti);
		}
		else { // the condition is symbolic
			ti.pop();
			return getNext(ti);
			}
		}
	}
