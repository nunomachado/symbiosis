package pt.tecnico.jpf.symbiosis;

import gov.nasa.jpf.Config;
import gov.nasa.jpf.JPF;
import gov.nasa.jpf.PropertyListenerAdapter;
import gov.nasa.jpf.jvm.ChoiceGenerator;
import gov.nasa.jpf.jvm.ElementInfo;
import gov.nasa.jpf.jvm.FieldInfo;
import gov.nasa.jpf.jvm.JVM;
import gov.nasa.jpf.jvm.MethodInfo;
import gov.nasa.jpf.jvm.StackFrame;
import gov.nasa.jpf.jvm.SystemState;
import gov.nasa.jpf.jvm.ThreadChoiceGenerator;
import gov.nasa.jpf.jvm.ThreadInfo;
import gov.nasa.jpf.jvm.bytecode.FieldInstruction;
import gov.nasa.jpf.jvm.bytecode.GETFIELD;
import gov.nasa.jpf.jvm.bytecode.GETSTATIC;
import gov.nasa.jpf.jvm.bytecode.INVOKEINTERFACE;
import gov.nasa.jpf.jvm.bytecode.INVOKESPECIAL;
import gov.nasa.jpf.jvm.bytecode.INVOKESTATIC;
import gov.nasa.jpf.jvm.bytecode.INVOKEVIRTUAL;
import gov.nasa.jpf.jvm.bytecode.IfInstruction;
import gov.nasa.jpf.jvm.bytecode.Instruction;
import gov.nasa.jpf.jvm.bytecode.MONITORENTER;
import gov.nasa.jpf.jvm.bytecode.MONITOREXIT;
import gov.nasa.jpf.jvm.bytecode.PUTFIELD;
import gov.nasa.jpf.jvm.bytecode.PUTSTATIC;
import gov.nasa.jpf.jvm.bytecode.ReturnInstruction;
import gov.nasa.jpf.jvm.choice.ThreadChoiceFromSet;
import gov.nasa.jpf.search.Search;
import gov.nasa.jpf.symbc.numeric.PCChoiceGenerator;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map.Entry;
import java.util.Stack;
import java.util.Vector;

import com.sun.corba.se.impl.ior.WireObjectKeyTemplate;

import pt.tecnico.jpf.symbiosis.util.Type;
import pt.tecnico.jpf.symbiosis.util.Utilities;

public class SymbiosisListener extends PropertyListenerAdapter{

	public static Config config; //configuration parameters
	public static boolean DEBUG = true; 
	public static HashMap<String,Vector<String>> bbtrace; 	//"symbiosis.bbtrace" - path to the log containing each thread's execution path, in terms of basic block ids 
	public static HashSet<String> sharedAccesses;			//"symbiosis.sharedAccesses" - set of strings indicating the shared accesses identified by the static analysis
	public static String symbTraceFolder;					//"symbiosis.tracefolder" - path to the output folder where we will store the symbolic event traces
	public static HashMap<String,String> threadSymbTraces; 		// map: thread id -> output trace with the symbolic operations observed
	public static HashSet<String> threadsFinished;				//set used to mark threads as finished, i.e. threads which have already printed their Path Conditions to the file
	public static HashSet<String> threadsStarted;				//set used to count threads that already started, i.e. threads which have already printed their start event into the file	
	public static HashSet<String> deamonThreads;			//set that stores the deamon threads that are still alive; this is important to prevent all user threads from finishing before the deamon threads have completed their execution paths 
	public boolean hasForked = false;	//bool indicating whether a thread has forked a children or not (used to ensure that all threads are executed right after being forked)
	
	//data structures to handle consistent state identification
	//public static HashMap<String,HashSet<String>> symbVarsCreated;		//map: thread's state path id -> set of the symbolic var created so far (this is used to avoid creating different symbolic variables for the same access, when we have to backtrack multiple times)
	public static HashMap<String,Integer> lastNumBBs; 		//map: symb var name -> number of BBs (consumed from the trace) when the symbolic variable was created for the first time 
	public static HashMap<Integer,Boolean> stateOkToLog;		//map: state id -> boolean indicating whether it is ok to log (i.e. if the state corresponds to a basic block that conforms with the trace)
	public static HashMap<String,String> writtenValues;		//map: write operation -> written value -> used to resolve write operations whose values are references to other writes
	
	//data structures to handle object monitors
	public static HashMap<String, Stack<String>> methodMonitor; //map: thread id -> stack with the last monitor acquired (used to identify the monitor of a given synchronized method, when leaving that method)
	
	//data structures to guide symbolic execution	
	public Search pointerToSearch=null;
	public SystemState pointerToSS = null;
	
	//measure elapsed time
	long startTime, endTime;
	
	public SymbiosisListener(Config conf, JPF jpf)
	{
		startTime = System.nanoTime();
		
		config = conf;
		pointerToSearch = jpf.getSearch();
		pointerToSS = jpf.getVM().getSystemState();
		//initialize data structures
		bbtrace = new HashMap<String, Vector<String>>();
		sharedAccesses = new HashSet<String>();
		threadsFinished = new HashSet<String>();
		threadsStarted = new HashSet<String>();
		lastNumBBs = new HashMap<String, Integer>();
		threadSymbTraces = new HashMap<String, String>();
		methodMonitor = new HashMap<String, Stack<String>>();
		stateOkToLog = new HashMap<Integer, Boolean>();
		writtenValues = new HashMap<String, String>();
		deamonThreads = new HashSet<String>();
		
		//create output folder if it doesn't exist
		symbTraceFolder = config.getString("symbiosis.tracefolder");
		File tempFile = new File(symbTraceFolder);
		if(!(tempFile.exists()))
			tempFile.mkdir();

		//load basic block trace and shared access locations
		loadBBTrace();
		loadSharedAccesses();
	}


	/**
	 * Loads the file containing the references to the shared accesses identified by the static analysis
	 */
	private void loadSharedAccesses() {
		String fname = config.getString("symbiosis.sharedAccesses");
		System.out.println("[SymbiosisListener] Loading shared accesses from "+fname);
		try{
			BufferedReader br = new BufferedReader(new FileReader(fname));
			String line;
			while ((line = br.readLine()) != null) {
				sharedAccesses.add(line);
			}
			br.close();

			if(DEBUG)
			{
				for(String acc : sharedAccesses)
				{
					System.out.println("\t"+acc);
				}
			}
		} 
		catch (IOException e) {
			e.printStackTrace();
		}
	}


	/**
	 * Loads the thread execution paths recorded at runtime.
	 */
	private void loadBBTrace()
	{
		String fname = config.getString("symbiosis.bbtrace");
		System.out.println("[SymbiosisListener] Loading BB trace from "+fname);
		try{
			BufferedReader br = new BufferedReader(new FileReader(fname));
			String line;
			while ((line = br.readLine()) != null) {
				String[] vals = line.split(" "); 
				String tid = vals[0];
				String bbid = vals[1]; 

				if(bbtrace.containsKey(tid)){
					bbtrace.get(tid).add(bbid);
				}
				else{
					Vector<String> tmp = new Vector<String>();
					tmp.add(bbid);
					bbtrace.put(tid, tmp);
				}
			}
			br.close();

			if(DEBUG)
			{
				for(Entry<String, Vector<String>> entry : bbtrace.entrySet())
				{
					System.out.print("\tT"+entry.getKey()+": ");
					for(String bbid : entry.getValue())
					{
						System.out.print(bbid+" ");
					}
					System.out.println("");
				}
			}
		} 
		catch (IOException e) {
			e.printStackTrace();
		}
	}

	/**
	 * Returns the value written by a write operation on a symbolic variable
	 * @param lastIns
	 * @param ei
	 * @param vm
	 * @return
	 */
	public Object extractWrittenValue(FieldInstruction lastIns, ElementInfo ei, JVM vm)
	{
		FieldInfo fi = lastIns.getFieldInfo();
		String type = fi.getType();
		Object attr = null;
		long lvalue = lastIns.getLastValue();

		if (lastIns instanceof PUTSTATIC){	
			attr = fi.getClassInfo().getStaticElementInfo().getFieldAttr(fi);
		}
		else if (lastIns instanceof PUTFIELD){
			ei = lastIns.getLastElementInfo();
			attr = ei.getFieldAttr(fi);			
		}

		Type itype = Utilities.typeToInteger(type);
		Type rtype = Utilities.simplifyTypeFromType(itype);
		Object value = null;
		// Giving the proper shape to the read value.
		if (itype==Type.INT){
			if (attr != null){
				rtype = Type.SYMINT;
				value = attr;
				//System.out.println("Writing symint "+((SymbolicInteger)((BinaryLinearIntegerExpression)value).getLeft())._max);
			}else{
				value = Utilities.transformValueFromLong(vm, lvalue, type);
			}
		}else if (itype==Type.BOOLEAN){
			if (attr != null){
				rtype = Type.SYMINT;
				value = attr;
				//System.out.println("Writing symint "+((SymbolicInteger)((BinaryLinearIntegerExpression)value).getLeft())._max);
			}else{
				value = Utilities.transformValueFromLong(vm, lvalue, type);
			}
		}else if (itype==Type.BYTE){
			if (attr != null){
				rtype = Type.SYMINT;
				value = attr;
			}else{
				value = Utilities.transformValueFromLong(vm, lvalue, type);
			}
		}else if (itype==Type.CHAR){
			if (attr != null){
				rtype = Type.SYMINT;
				value = attr;
			}else{
				value = Utilities.transformValueFromLong(vm, lvalue, type);
			}
		}else if (itype==Type.LONG){
			if (attr != null){
				rtype = Type.SYMINT;
				value = attr;
			}else{
				value = Utilities.transformValueFromLong(vm, lvalue, type);
			}
		}else if (itype==Type.SHORT){
			if (attr != null){
				rtype = Type.SYMINT;
				value = attr;
			}else{
				value = Utilities.transformValueFromLong(vm, lvalue, type);
			}
		}else if (itype==Type.REAL){
			if (attr != null){
				rtype = Type.SYMREAL;
				value = attr;
			}else{
				value = Utilities.transformValueFromLong(vm, lvalue, type);
			}
		}else if (itype==Type.FLOAT){
			if (attr != null){
				rtype = Type.SYMREAL;
				value = attr;
			}else{
				value = Utilities.transformValueFromLong(vm, lvalue, type);
			}
			// TODO: Not working for String. lastValue is going to be the object reference.
		}else if (itype==Type.STRING){
			if (attr != null){
				rtype = Type.SYMSTRING;
				value = attr;
			}else{
				value = Utilities.transformValueFromLong(vm, lvalue, type);
			}
			System.out.println("WARNING: String variable. Not ready for it yet.");
		}else if (itype==Type.REFERENCE){
			if (attr != null){
				rtype = Type.SYMREF;
				value = attr;
			}else{
				value = Utilities.transformValueFromLong(vm, lvalue, type);
			}
			System.out.println("WARNING: Ref value for field "+fi.getName()+": "+type);
		}

		return value;
	}

	public void stateAdvanced(Search search) 
	{
		ThreadInfo ti = search.getVM().getCurrentThread();
		String tid = search.getVM().getCurrentThread().getName();

		//account for calls to monitor inside <clini> and <init> methods,
		//where t-main name has not yet been changed
		if(tid.equals("main"))
			tid = "0";

		/*if(search.getVM().getChoiceGenerator() instanceof PCChoiceGenerator){
			PCChoiceGenerator pccg = (PCChoiceGenerator) search.getVM().getChoiceGenerator();

			System.out.println("["+getPathStateId(tid, "")+"] PC: "+pccg.getCurrentPC().toString());

		}//*/
		
		if(!stateOkToLog.containsKey(pointerToSS.getId()))
			stateOkToLog.put(pointerToSS.getId(),true);

		if(bbtrace.get(tid).isEmpty() && !threadsFinished.contains(tid)){
			
			// && search.getVM().getChoiceGenerator() instanceof PCChoiceGenerator){
			PCChoiceGenerator pccg = search.getVM().getLastChoiceGeneratorOfType(PCChoiceGenerator.class);
			if (pccg != null && pccg.getThreadInfo().getName()==tid) {
				System.out.println("PCCG THREAD INFO:" +pccg.getThreadInfo());
				String cond = pccg.getCurrentPC().toString();
				cond = cond.substring(cond.indexOf('\n')+1, cond.length());

				//edit condition to match symbiosis' format
				String[] andConds = cond.split("&&\n");
				cond = "<pathjpf>";
				//reverse order of conditions to correspond to that of the execution path
				for(int i = andConds.length-1; i >= 0; i--)
				{
					String c = andConds[i];
					cond += "\nT"+tid+":("+c+")";
				}			
				
				System.out.println("["+getPathStateId(tid,search.getVM().getLastInstruction().getFileLocation())+"] Thread finished execution. Save PC: \n"+ cond);

				//log event and save the complete thread log into file
				logSymbEvent(tid, cond);	
				//Utilities.logSymbLog(symbTraceFolder, tid, threadSymbTraces.get(tid));
			}
			search.getVM().getCurrentThread().setTerminated();
			threadsFinished.add(tid);

			//if all threads have already finished, then exit
			if(threadsFinished.size() == bbtrace.keySet().size()){
				System.out.println("[SymbiosisJPF] All thread have consumed their logs. Terminate execution.");
				endTime = System.nanoTime();
				double time = (((double)(endTime - startTime)/1000000000));
				System.out.println("[SymbiosisJPF] EXECUTION TIME: "+time+"s\n");
				System.exit(1);
			}
		}
	}

	public void stateBacktracked (Search search)
	{
		String tid = search.getVM().getCurrentThread().getName();
		
		System.out.println("["+getPathStateId(tid,search.getVM().getLastInstruction().getFileLocation())+"] state backtracked");
		/*
		if(search.getVM().getChoiceGenerator() instanceof PCChoiceGenerator){
			PCChoiceGenerator pccg = (PCChoiceGenerator) search.getVM().getChoiceGenerator();

			System.out.println("["+getPathStateId(tid,search.getVM().getLastInstruction().getFileLocation())+"] state backtracked");
			pointerToSS.backtrackPathId(tid);
		}*/
	}

	/**
	 * 	This method is called every time there is a decision to take regarding the execution path.
	 */
	public void choiceGeneratorAdvanced(JVM jvm) 
	{
		SystemState ss = jvm.getSystemState();
		ChoiceGenerator<?> cg = ss.getChoiceGenerator();
		ThreadInfo ti = jvm.getCurrentThread();
		String tid = jvm.getCurrentThread().getName();
		
		// In case the choice is related to a symbolic variable
		if (cg instanceof PCChoiceGenerator) {
			int line = jvm.getLastInstruction().getLineNumber();

			PCChoiceGenerator pccg = (PCChoiceGenerator) cg;
			int choice = pccg.getNextChoice();

			if(bbtrace.containsKey(tid) && bbtrace.get(tid).size() > 0)
			{
				System.out.println("["+getPathStateId(tid,jvm.getLastInstruction().getFileLocation())+"]  update state with "+choice);
				pointerToSS.updatePathId(tid,choice);
			}
		}
		else if (cg instanceof ThreadChoiceGenerator) {
			
			if(hasForked){
				hasForked = false;
				ThreadChoiceFromSet threadCg = (ThreadChoiceFromSet) cg;
				ThreadInfo[] threads = threadCg.getChoices();
				for (ThreadInfo t : threads){
					//System.out.println("      - choice: "+t.getName());
					if(t.isDaemon() && !deamonThreads.contains(t.getName())){
						deamonThreads.add(t.getName());
						System.out.println("[SymbiosisJPF] add new deamon thread "+t.getName()+" (size: "+deamonThreads.size()+")");
					}
				}
			}
			
			//if thread is deamon thread, then terminate and remove entry from deamonThreads
			//otherwise, wait until all deamon threads have finished
			if(bbtrace.get(tid)!=null && bbtrace.get(tid).isEmpty()){
				
				System.out.println(" THREAD IS ABOUT TO FINISH - CHOICE GEN");
				/*if(ti.isDaemon()){
					deamonThreads.remove(tid);
				}
				else{
					System.out.println("["+getPathStateId(tid,search.getVM().getLastInstruction().getFileLocation())+"] there are deamon threads running. Delay finishing this thread.");
					while(!deamonThreads.isEmpty()){
						ti.yield();
					}
				}	*/
			}
		}
	}


	public void executeInstruction(JVM vm)
	{
		// Gets the next instruction scheduled in the VM
		Instruction nextIns = vm.getNextInstruction();

		ThreadInfo ti = vm.getCurrentThread();
		String tid = ti.getName();
		
		if(threadsFinished.contains(tid)){
			ti.setTerminated();
			ti.skipInstruction();
			return;
		}
		
		if (nextIns instanceof INVOKESTATIC){
			INVOKESTATIC virtualIns = (INVOKESTATIC) nextIns;
			String method = virtualIns.getInvokedMethod().getName();
			SystemState ss = vm.getSystemState();
			String file = nextIns.getFileLocation();

			if(method.contains("symbiosisBBEntry"))
			{	
				if(tid.equals("main"))
					tid = "0";
			
				String bbid = virtualIns.getArgumentValues(ti)[0].toString();
				if(!bbtrace.get(tid).isEmpty())
				{

					String nextbbid = bbtrace.get(tid).firstElement();
					if(bbid.equals(nextbbid))
					{
						if(DEBUG)
							System.out.println("["+getPathStateId(tid,file)+"] bbid: "+bbid+" == nextbbid: "+nextbbid+" -> OK ("+(bbtrace.get(tid).size()-1)+" left)");
						bbtrace.get(tid).remove(0);
						pointerToSS.setInteresting(true);
						stateOkToLog.put(pointerToSS.getId(), true);
					}
					else
					{
						if(DEBUG)
							System.out.println("["+getPathStateId(tid,file)+"] bbid: "+bbid+" != nextbbid: "+nextbbid+" -> STOP");
						//pointerToSS.setBoring(true); 
						pointerToSearch.setIgnoredState(true); //having this, there are cases where JPF cannot proceed (dunno why...), but it has to be like this, otherwise it is
						stateOkToLog.put(pointerToSS.getId(), false);
					}
				}
				else //thread has no more BBs in the trace, thus should stop executing
				{
					if(!threadsFinished.contains(tid))
						System.out.println("["+getPathStateId(tid,file)+"] Thread finished execution.");
					
					ti.setTerminated();
					//System.out.println("["+getPathStateId(tid,file)+"] holds lock: "+ti.holdsLock(vm.getLastElementInfo()));
				}
			}
		}
	}

	/**
	 * Called after the execution of an instruction.
	 * - Marks shared variables as symbolic (according to the shared accesses log)
	 * - Generates the log file with the symbolic operations executed
	 */
	public void instructionExecuted(JVM vm) 
	{
		// Info that it is going to be used throughout the method
		Instruction lastIns = vm.getLastInstruction();	// The last instruction executed
		ThreadInfo ti = vm.getLastThreadInfo();	// Information regarding the thread that executed the last instruction
		String tid = ti.getName();
		ElementInfo ei = vm.getLastElementInfo(); // Information regarding the last element used. It completely depends on the instruction.
		MethodInfo mi = lastIns.getMethodInfo(); // Information regarding the method to which the last instruction belongs
		
		if (mi == null){
			System.out.println("[SymbiosisJPF] There might be a problem, MehotdInfo is not set for the last instruction!");
			return;
		}

		// We always need to check whether the instruction is completed or not in order to avoid transition breaks
		// These breaks sometimes forces an instruction to be re-executed
		if (lastIns.isCompleted(ti)){
			int line = lastIns.getLineNumber();
			String file = lastIns.getFileLocation();
			
			//for some weird reason, it might happen that a thread executes an instruction after being terminated
			if(threadsFinished.contains(tid)){
				return;
			}
			
			//account for calls to monitor inside <clini> and <init> methods,
			//where t-main name has not yet been changed
			if(tid.equals("main"))
				tid = "0";

			if ((lastIns instanceof GETFIELD) || (lastIns instanceof GETSTATIC)) {
				FieldInstruction getfieldIns = (FieldInstruction) lastIns;
				String access = getfieldIns.getFieldInfo().getFullName()+"@"+line;

				if(getfieldIns.getLastElementInfo() == null){
					System.out.println("["+getPathStateId(tid,file)+"] There might be a problem, field info is null!");
					return;
				}
				
				if(sharedAccesses.contains(access))
				{					
					String type = getfieldIns.getFieldInfo().getType();
					String symbvar = Symbolic.getRWSymbName(getfieldIns, tid, false);

					//check if the symbolic access has not yet been logged
					boolean isNewSV = isNewSymbolicVar(file, symbvar,tid);

					//mark variable as symbolic
					Symbolic.newSymbolic(type,symbvar,vm);

					if(isNewSV && stateOkToLog.get(pointerToSS.getId()))
					{
						Symbolic.incrementSymbVarId(symbvar); //update id for next symbolic var

						//log event
						String event = Utilities.getFileShortName(getfieldIns.getFileLocation()).replace(':', '@')+":"+symbvar;
						logSymbEvent(tid, event);	

						if(DEBUG) 
							System.out.println("["+getPathStateId(tid,file)+"] Log event "+event);
					}
				}
				if(getfieldIns.getFieldInfo().getFullName().contains("assertionsDisabled")){
					System.out.println("Assert Thread: "+tid);
					logSymbEvent(tid, "<assertThread_ok>"); //TODO: this is not correct, as it always logs assertThread_Ok, regardless of whether the program fails or not 
				}
				//*/
			}
			else if ((lastIns instanceof PUTSTATIC)||(lastIns instanceof PUTFIELD)){
				FieldInstruction putfieldIns = (FieldInstruction) lastIns;
				String access = putfieldIns.getFieldInfo().getFullName()+"@"+line;

				if(putfieldIns.getLastElementInfo() == null){
					System.out.println("["+getPathStateId(tid,file)+"] There might be a problem, field info is null!");
					return;
				}
				
				if(sharedAccesses.contains(access))
				{
					String type = putfieldIns.getFieldInfo().getType();		
					String symbvar = Symbolic.getRWSymbName(putfieldIns, tid, true);

					//check if the symbolic access has not yet been logged
					boolean isNewSV = isNewSymbolicVar(file, symbvar,tid);

					//mark variable as symbolic
					//System.out.println("["+getPathStateId(tid,file)+"] symbvar: "+symbvar);
					Symbolic.newSymbolic(type,symbvar,vm);

					if(!stateOkToLog.containsKey(pointerToSS.getId())  //it might happen that some accesses occur before executing symbiosisBBentry
							|| (isNewSV && stateOkToLog.get(pointerToSS.getId())))
					{
						Symbolic.incrementSymbVarId(symbvar); //update id for next symbolic var

						//log event
						Object valueObj = extractWrittenValue(putfieldIns, ei, vm);
						String value = valueObj.toString();

						//check whether written value is a reference to other write
						if(value.contains("W-")){
							int initW = value.indexOf("W-");
							while(initW!=-1){
								int endW = value.indexOf(' ', initW);
								if(endW==-1){
									endW = value.indexOf(')', initW);
									if(endW==-1){
										endW = value.length();
									}
								}
								String writeRef = value.substring(initW,endW); 
								//System.out.print("\t-- writeRef = "+writeRef+" writtenValue = "+writtenValues.get(writeRef)+"; reference to "+value+" translated to ");
								value = value.replace(writeRef, writtenValues.get(writeRef));
								//System.out.println(value);
								initW = value.indexOf("W-");
							}
							writtenValues.put(symbvar, value);
						}
						else{
							System.out.println("\t-- "+symbvar+" -> "+value);
							writtenValues.put(symbvar, value);
						}

						String event = Utilities.getFileShortName(putfieldIns.getFileLocation()).replace(':', '@')+":"+symbvar+"\n$"+value+"$";
						logSymbEvent(tid, event);	

						if(DEBUG) 
							System.out.println("["+getPathStateId(tid,file)+"] Log event "+event);
					}
				}
			}
			else if(lastIns instanceof MONITORENTER){
				MONITORENTER monEnterIns = (MONITORENTER) lastIns;
				if (monEnterIns.getSourceLine()!=null){					
					ElementInfo obj = vm.getElementInfo(monEnterIns.getLastLockRef()); 
					String object = Integer.toHexString(obj.getObjectRef());
					logLockSyncEvent(monEnterIns, line, ti, vm, "lock",object);
				}
			}
			else if(lastIns instanceof MONITOREXIT){
				MONITOREXIT monExitIns = (MONITOREXIT) lastIns;
				if (monExitIns.getSourceLine()!=null){
					ElementInfo obj = vm.getElementInfo(monExitIns.getLastLockRef()); 
					String object = Integer.toHexString(obj.getObjectRef());
					logLockSyncEvent(monExitIns, line, ti, vm, "unlock",object);
				}
			}

			//STATIC INVOCATIONS *** Used to detect calls to symbiosisBBEntry
			else if (lastIns instanceof INVOKESTATIC){
				INVOKESTATIC virtualIns = (INVOKESTATIC) lastIns;
				String method = virtualIns.getInvokedMethod().getName();


			}
			//Used to detect the run method of a new thread
			if (lastIns.isFirstInstruction()){
				String methodName = ti.getMethod().getName()+ti.getMethod().getSignature();

				// Identifying when a new thread is starting in order to trace start events
				if (methodName.equals("run()V") && !lastIns.getFileLocation().contains("synthetic"))
				{
					//make sure we only log one start event per thread
					if(!threadsStarted.contains(tid))
					{
						HashSet<String> vars = new HashSet<String>();
						vars.add("start-"+tid);
						threadsStarted.add(tid);
						
						//log event
						String event = Utilities.getFileShortName(lastIns.getFileLocation()).replace(':', '@')+":S-start-"+tid;
						logSymbEvent(tid, event);		

						if(DEBUG) 
							System.out.println("["+getPathStateId(tid,String.valueOf(line))+"] Log event "+event);	
					}
				}
			}
			//VIRTUAL INVOCATIONS *** Used to detect start, join, lock, unlock, newCondition
			else if (lastIns instanceof INVOKEVIRTUAL){

				INVOKEVIRTUAL virtualIns = (INVOKEVIRTUAL) lastIns;
				String method = virtualIns.getInvokedMethod().getName();
				String invokedMethod = virtualIns.getInvokedMethodName();

				// Start method invocation
				if ((method.equals("start")) && (virtualIns.getInvokedMethod().getClassInfo().getName().equals("java.lang.Thread")))
				{
					logPOSyncEvent(virtualIns, line, ti, vm, "fork");
					hasForked = true;
				}
				//Join method invocation
				else if ((method.equals("join")) && (virtualIns.getInvokedMethod().getClassInfo().getName().equals("java.lang.Thread")))
				{
					logPOSyncEvent(virtualIns, line, ti, vm, "join");
					
					System.out.println("["+ti.getName()+"] skip JOIN");
					/*StackFrame sf = ti.popFrame();
					Instruction nextIns = sf.getPC().getNext();*/
				    vm.getCurrentThread().skipInstruction();
				}
				//Wait method invocation
				else if (invokedMethod.equals("wait()V")||invokedMethod.equals("wait(I)V")||invokedMethod.equals("wait(IJ)V"))
				{
					ElementInfo obj = vm.getElementInfo(virtualIns.getCalleeThis(ti)); 
					String object = Integer.toHexString(obj.getObjectRef());
					logLockSyncEvent(virtualIns, line, ti, vm, "wait",object);
					
					System.out.println("["+ti.getName()+"] skip WAIT");
					/*StackFrame sf = ti.popFrame();
					Instruction nextIns = sf.getPC().getNext();*/
				    vm.getCurrentThread().skipInstruction();
				}
				//notify method invocation
				else if (invokedMethod.equals("notify()V")||invokedMethod.equals("notifyAll()V"))
				{
					ElementInfo obj = vm.getElementInfo(virtualIns.getCalleeThis(ti)); 
					String object = Integer.toHexString(obj.getObjectRef());
					
					if(invokedMethod.equals("notify()V"))
						logLockSyncEvent(virtualIns, line, ti, vm, "signal",object);
					else
						logLockSyncEvent(virtualIns, line, ti, vm, "signalall",object);
				}
				//Lock method invocation
				else if(invokedMethod.equals("lock()V"))
				{
					ElementInfo obj = vm.getElementInfo(virtualIns.getCalleeThis(ti)); 
					String object = Integer.toHexString(obj.getObjectRef());
					logLockSyncEvent(virtualIns, line, ti, vm, "lock",object);
				}
				//Unlock method invocation
				else if(invokedMethod.equals("unlock()V"))
				{
					ElementInfo obj = vm.getElementInfo(virtualIns.getCalleeThis(ti)); 
					String object = Integer.toHexString(obj.getObjectRef());
					logLockSyncEvent(virtualIns, line, ti, vm, "unlock",object);
				}
				else if(!virtualIns.getInvokedMethod().getClassName().startsWith("java."))
				{
					//System.out.println("-- method "+virtualIns.getInvokedMethod()+" is sync? "+virtualIns.getInvokedMethod().isSynchronized());
					// Synchronized method invocation
					if (virtualIns.getInvokedMethod().isSynchronized()){
						
						ElementInfo obj = vm.getElementInfo(virtualIns.getCalleeThis(ti)); 
						String object = Integer.toHexString(obj.getObjectRef());
						logLockSyncEvent(virtualIns, line, ti, vm, "lock",object);
						
						//save monitor obj to store the unlock operation when returning from the sync method
						if(methodMonitor.containsKey(tid)){
							methodMonitor.get(tid).push(object);
						}
						else{
							Stack<String> tmp = new Stack<String>();
							tmp.push(object);
							methodMonitor.put(tid, tmp);
						}
					}
				}

			}//end if invokevirtual

			else if(lastIns instanceof INVOKEINTERFACE)
			{
				INVOKEINTERFACE interfaceIns = (INVOKEINTERFACE) lastIns;
				String invokedMethod = interfaceIns.getInvokedMethodName();
				if (invokedMethod.equals("await()V")||invokedMethod.equals("awaitNanos(J)V"))
				{
					ElementInfo obj = vm.getElementInfo(interfaceIns.getCalleeThis(ti)); 
					String object = Integer.toHexString(obj.getObjectRef());
					logLockSyncEvent(interfaceIns, line, ti, vm, "wait",object);
				}
				else if (invokedMethod.equals("signal()V")||invokedMethod.equals("signalAll()V"))
				{
					ElementInfo obj = vm.getElementInfo(interfaceIns.getCalleeThis(ti)); 
					String object = Integer.toHexString(obj.getObjectRef());
					
					if(invokedMethod.equals("signal()V"))
						logLockSyncEvent(interfaceIns, line, ti, vm, "signal",object);
					else
						logLockSyncEvent(interfaceIns, line, ti, vm, "signalall",object);
				}
			}
			//RETURN INSTRUCTION *** Used to detect the end of synchronized methods
			else if (lastIns instanceof ReturnInstruction){
				ReturnInstruction genReturnIns = (ReturnInstruction) lastIns;
				MethodInfo me = genReturnIns.getMethodInfo();
				if(!me.getClassName().startsWith("java.")){
					if (me.isSynchronized() && methodMonitor.containsKey(tid) && !methodMonitor.get(tid).isEmpty()){
						String object = methodMonitor.get(tid).pop();
						logLockSyncEvent(genReturnIns, line, ti, vm, "unlock", object);
					}
				}
			}
			//SPECIAL INVOCATIONS *** Used to detect AssertionError invocations
			else if(lastIns instanceof INVOKESPECIAL){
				//delay assertion error instruction if all threads haven't finished yet
				if(((INVOKESPECIAL) lastIns).toString().contains("java.lang.AssertionError"))
				{
					logSymbEvent(tid, "<assertThread_fail>");
					System.out.println("["+getPathStateId(tid,file)+"] Assertion Error -> proceed");
					//check if all threads have already finished
					/*if(threadsFinished.size()!=bbtrace.keySet().size())
					{
						vm.getSystemState().setBoring(true);
					}*/
				}
			}
		}//end instruction.isCompleted()
	}


	/**
	 * Checks whether a new symbolic var created is redundant or not (i.e. it has been created before
	 * for that very same access by a non-backtracked state)
	 * @param tid
	 * @param symbvar
	 * @return
	 */
	private boolean isNewSymbolicVar(String file, String symbvar, String tid)
	{
		symbvar = symbvar.substring(0,symbvar.lastIndexOf('-'));
		
		//create key of format 'symbvar@file' to index symb vars
		String key = symbvar+"@"+file;
		
		if(!lastNumBBs.containsKey(key))
		{
			lastNumBBs.put(key, bbtrace.get(tid).size());
			return true;
		}
		else
		{
			//if we are repeating the same access but with fewer blocks to go in the trace
			//it means that we are in a loop and should create new symb vars
			if(lastNumBBs.get(key) > bbtrace.get(tid).size())
			{
				lastNumBBs.put(key, bbtrace.get(tid).size());
				return true;
			}
			else{
				//System.out.println(" -- key "+key+" is redundant ; isfinished: "+threadsFinished.contains(tid));
				return false; //symbvar is redundant
			}

		}//*/
		//return true;
	}

	/**
	 * Stores into the log file a given partial order synchronization event (e.g. join, fork)
	 * @param virtualIns
	 * @param line
	 * @param ti
	 * @param vm
	 * @param synctype
	 */
	protected void logPOSyncEvent(INVOKEVIRTUAL virtualIns, int line, ThreadInfo ti, JVM vm, String synctype)
	{
		String tid = ti.getName();
		String child = vm.getThreadList().getThreadInfoForObjRef(virtualIns.getCalleeThis(ti)).getName();

		if(threadsFinished.contains(tid))
			return;
		
		//lazy way of making sure that we don't log the fork operation when states backtrack
		String symbvar = "S-"+synctype+"_"+child+"-"+tid;
		if(isNewSymbolicVar(virtualIns.getFileLocation(), (symbvar+"-"),tid)){
			//log event
			String event = Utilities.getFileShortName(virtualIns.getFileLocation()).replace(':', '@')+":S-"+synctype+"_"+child+"-"+tid;
			logSymbEvent(ti.getName(), event);		

			if(DEBUG) 
				System.out.println("["+getPathStateId(tid,String.valueOf(line))+"] Log event "+event);
		}
	}


	protected void logLockSyncEvent(Instruction virtualIns, int line, ThreadInfo ti, JVM vm, String synctype, String lockobj)
	{
		String tid = ti.getName();

		if(threadsFinished.contains(tid))
			return;
		
		String symbvar = "S-"+synctype+"_"+lockobj+"-"+tid;

		//check if the symbolic access has not yet been logged
		if(isNewSymbolicVar(virtualIns.getFileLocation(), symbvar+"-",tid) && stateOkToLog.get(pointerToSS.getId())){
			//log event
			String event = Utilities.getFileShortName(virtualIns.getFileLocation()).replace(':', '@')+":"+symbvar;
			logSymbEvent(ti.getName(), event);	

			if(DEBUG) 
				System.out.println("["+getPathStateId(tid,String.valueOf(line))+"] Log event "+event);
		}
	}

	protected void logSymbEvent(String tid, String event)
	{

		if(threadsFinished.contains(tid))
			return;
		
		//we are tracing directly to file because sometimes the symbolic execution 
		//cannot finish properly, so we should store the info so far
		if(threadSymbTraces.containsKey(tid))
		{
			//String log = threadSymbTraces.get(tid);
			//log += event+"\n";
			//threadSymbTraces.put(tid, log);
			Utilities.logSymbLog(symbTraceFolder, tid, event, true);
		}
		else
		{
			String log = event+"\n";
			threadSymbTraces.put(tid, log);
			Utilities.logSymbLog(symbTraceFolder, tid, event, false);
		}
	}

	public void searchFinished(Search search) {
		//System.out.println("---------search finished---------");
	}
	
	private String getPathStateId(String tid, String file)
	{
		return (tid+"_"+pointerToSS.getId()+"@"+file);
	}
}
