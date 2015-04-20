package twostage;
/**
 * @author Xuan
 * Created on 2005-1-18
 */
public class TwoStage {
	
    public Data data1,data2;

    public TwoStage (Data data1, Data data2) {
	this.data1=data1;
	this.data2=data2;
    }
   
    /*
     * This method is used to simulate two stage access
     * In first stage, it modify the value of data1
     * In the second stage, it modify the value of data2 according to data1
     * It assumes that data2.value=data1.value (This assumption is used to
     * simulate in some database application, the two values in different
     * tables must be consistent.
     */
    public void A () {
		
       // This is the first stage
       synchronized (data1) {
           data1.value=1;
       }
	    
       //reading may happen here, data will be inconsistent...
	   try{Thread.currentThread().sleep(10);}catch(Exception e){} 
       
       // This is the second stage, using the result of stage1 to calculate 
       synchronized (data2) {   		
           //if other threads modify data1.value, inconsistency will happen
           data2.value=data1.value+1;
       }
    }
	
    public void B () {
		
	int t1=-1, t2=-1;
        synchronized (data1) {
	    if (data1.value==0) return ; //The first stage has not begun.
	        t1=data1.value;
	}
	synchronized (data2) {
	    t2=data2.value;
	}
		assert (data2.value==data1.value+1);
        if (t2 != (t1+1))
        	throw new RuntimeException("[Bug Found]");
        else
        	System.out.println("[OK]");
    }
}
