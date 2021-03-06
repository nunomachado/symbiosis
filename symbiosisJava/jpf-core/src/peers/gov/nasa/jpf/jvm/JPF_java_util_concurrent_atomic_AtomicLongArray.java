//
//Copyright (C) 2009 United States Government as represented by the
//Administrator of the National Aeronautics and Space Administration
//(NASA).  All Rights Reserved.
//
//This software is distributed under the NASA Open Source Agreement
//(NOSA), version 1.3.  The NOSA has been approved by the Open Source
//Initiative.  See the file NOSA-1.3-JPF at the top of the distribution
//directory tree for the complete NOSA document.
//
//THE SUBJECT SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY OF ANY
//KIND, EITHER EXPRESSED, IMPLIED, OR STATUTORY, INCLUDING, BUT NOT
//LIMITED TO, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL CONFORM TO
//SPECIFICATIONS, ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
//A PARTICULAR PURPOSE, OR FREEDOM FROM INFRINGEMENT, ANY WARRANTY THAT
//THE SUBJECT SOFTWARE WILL BE ERROR FREE, OR ANY WARRANTY THAT
//DOCUMENTATION, IF PROVIDED, WILL CONFORM TO THE SUBJECT SOFTWARE.
//
package gov.nasa.jpf.jvm;

/**
 * native peer for java.util.concurrent.atomic.AtomicLongArray
 */
public class JPF_java_util_concurrent_atomic_AtomicLongArray {

  public static long getNative__I__J (MJIEnv env, int objRef, int index) {
    int arrayRef = env.getReferenceField(objRef, "array");
    return env.getLongArrayElement(arrayRef, index);
  }

  public static boolean compareAndSetNative__IJJ__Z (MJIEnv env, int objRef, int index, long expect, long update) {
    int arrayRef = env.getReferenceField(objRef, "array");
    long value = env.getLongArrayElement(arrayRef, index);
    if (value == expect) {
      env.setLongArrayElement(arrayRef, index, update);
      return true;
    } else {
      return false;
    }
  }
}
