package net.sf.cache4j.test;

import net.sf.cache4j.CacheCleaner;
import net.sf.cache4j.CacheConfig;
import net.sf.cache4j.CacheFactory;
import net.sf.cache4j.impl.BlockingCache;
import net.sf.cache4j.impl.CacheConfigImpl;

public class Cache4jBugDriver {

	public static void main(String[] args) throws Exception {
		long start, end;
		start = System.nanoTime(); //start timestamp
		
		CacheCleaner cache = new CacheCleaner(200);
		cache.start();
		int nThreads = 1;
		
		WorkerThread[] threads = new WorkerThread[nThreads];
		
		try{Thread.currentThread().sleep(10);}catch(Exception e){}
		for(int i = 0; i < nThreads; i++)
		{
			threads[i] = new WorkerThread(cache);
			threads[i].start();
		}
		
		for(int i = 0; i < nThreads; i++)
		{
			threads[i].join();
		}
		
		end = System.nanoTime(); //** end timestamp
		double time = (((double)(end - start)/1000000000));
		System.out.println("\nEXECUTION TIME: "+time+"s");
	}

}
