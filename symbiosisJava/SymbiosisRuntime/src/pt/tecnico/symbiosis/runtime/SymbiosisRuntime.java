package pt.tecnico.symbiosis.runtime;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.util.HashMap;
import java.util.Map;
import java.util.Vector;
import java.util.concurrent.locks.ReentrantLock;


public class SymbiosisRuntime {
	public static boolean isCrashed = false;
	
	//** data structures for thread consistent identification
	public volatile static HashMap<String,Integer> threadChildrenCounter;	//** allows to generate deterministic thread identifiers by counting the number of children threads spawned
	public static Map<Thread, String> MapBackupThreadName;					//** used to keep the thread name consistent during the execution (because the name can be reset by the target program after the thread initialization)

	//** data structures for tracing log file events
	public static Vector<String> traceBB;
	public static ReentrantLock l; //** to avoid concurrent modifications when writing the trace, for some programs
	//public static HashMap<String,Boolean> skipBB; //** map: tid -> skipBB : used to skip a given BB entry when we arrive from a goto stmt in a catch exception block (the symbolic execution cannot guide threads towards catch blocks)
	
	public static void initialize()
	{
		//** initialize thread consistent identification data structures
		threadChildrenCounter = new HashMap<String, Integer>();
		MapBackupThreadName = new HashMap<Thread, String>();
		//skipBB = new HashMap<String, Boolean>();
		
		traceBB = new Vector<String>(); 		
		l = new ReentrantLock();
	}
	
	public static void symbiosisBBEntry(long bbid)
	{	
		if(Thread.currentThread().getName().equals("main"))
			Thread.currentThread().setName("0");

		String tid = Thread.currentThread().getName();
	/*	if(skipBB.get(tid)){
			skipBB.put(tid, false);
			return;
		}*/
		
		String entry = tid + " " + bbid;
		l.lock();
		traceBB.add(entry);
		l.unlock();
	}

	
	public static void symbiosisCaughtException()
	{
		//skipBB.put(Thread.currentThread().getName(), true);
	}

	//** thread handling
	public static void mainThreadStartRun()
	{
		try{
			Thread.currentThread().setName("0");
			String mainthreadname = Thread.currentThread().getName();
			MapBackupThreadName.put(Thread.currentThread(),mainthreadname);	//** save the name for handling future setNames

			//**to generate deterministic thread identifiers
			threadChildrenCounter.put("0", 1);
			//skipBB.put("0", false);

		}catch(Exception e)
		{
			System.err.println("[SymbiosisRuntime] "+e.getMessage());
			e.printStackTrace();
		}
	}

	/**
	 * Override thread creation for consistent thread identification across runs. 
	 * The new thread identifier will be the thread's parent thread ID associated with the counter value. 
	 * For instance, suppose a thread ti forks its j-th child thread, this child thread will be identified as ti:j .
	 * @param t
	 * @param parentId
	 */
	public synchronized static void threadStartRunBefore(Thread t)
	{	
		try{
			String parentId = Thread.currentThread().getName();  //** as the instrumented code to get the thread name is executed before changing the name, we need to do this
			//System.out.println(parentId+" "+threadChildrenCounter.containsKey(parentId));
			int childCounter = threadChildrenCounter.get(parentId);
			String newThreadName;

			//** the ith thread spawned by the main thread should have thread id = i
			if(!parentId.equals("0"))
				newThreadName= parentId+"."+childCounter;
			else
				newThreadName = String.valueOf(childCounter);

			t.setName(newThreadName);
			childCounter++;
			threadChildrenCounter.put(parentId,childCounter);

		}catch(Exception e)
		{
			System.err.println("[SymbiosisRuntime] "+e.getMessage());
			e.printStackTrace();
		}
	}

	
	public synchronized static void threadStartRun()
	{
		try{
			String threadId = Thread.currentThread().getName();
			threadChildrenCounter.put(threadId, 1);
			MapBackupThreadName.put(Thread.currentThread(),threadId);
			//skipBB.put(threadId, false);

			System.out.println("[SymbiosisRuntime] T"+threadId+" started running");
		}catch(Exception e)
		{
			System.err.println("[SymbiosisRuntime] "+e.getMessage());
			e.printStackTrace();
		}
	}
	
	
	public static void saveTrace()
	{
		try {
			OutputStreamWriter outstream = new OutputStreamWriter(new FileOutputStream(Main.tracefile));
			
			l.lock();
			for(String s : traceBB)
			{
				outstream.write(s+"\n");
			}
			outstream.close();
			l.unlock();
			
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
}
