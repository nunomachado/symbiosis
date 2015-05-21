package net.sf.cache4j.test;

public class Cache4jDriver {
	public static void main(String[] args)
	{
		BlockingCacheTest btest = new BlockingCacheTest();
		SynchronizedCacheTest stest=  new SynchronizedCacheTest();
		try {
			btest.test_THREAD1() ;
			//stest.test_THREAD1();
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
}
