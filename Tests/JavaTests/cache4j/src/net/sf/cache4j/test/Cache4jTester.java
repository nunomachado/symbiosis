/* =========================================================================
 * File: Tester.java$
 *
 * Copyright 2006 by Yuriy Stepovoy.
 * email: stepovoy@gmail.com
 * All rights reserved.
 *
 * =========================================================================
 */

package net.sf.cache4j.test;

import net.sf.cache4j.test.Test;

import java.util.List;
import java.util.ArrayList;
import java.util.Properties;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

/**
 * Класс Tester запускает все тесты
 *
 * @version $Revision: 1.1 $ $Date: 2010/06/22 16:21:10 $
 * @author Yuriy Stepovoy. <a href="mailto:stepovoy@gmail.com">stepovoy@gmail.com</a>
 **/

public class Cache4jTester {
// ----------------------------------------------------------------------------- Константы
// ----------------------------------------------------------------------------- Атрибуты класса
// ----------------------------------------------------------------------------- Статические переменные
// ----------------------------------------------------------------------------- Конструкторы
// ----------------------------------------------------------------------------- Public методы
    public static void main(String[] args) {

        try {
            //подгружаем список тестов
//            InputStream in = Tester.class.getResourceAsStream("classes.txt");
            Properties props = new Properties();
//            props.load(in);
//            in.close();

	    props.setProperty("net.sf.cache4j.test.BlockingCacheTest","");
//	    props.setProperty("net.sf.cache4j.test.SynchronizedCacheTest","");

            //запускаем тесты
            runTest(newInstances(new ArrayList(props.keySet())));
        } catch (Exception e){
            e.printStackTrace();
            return;
        }
    }
// ----------------------------------------------------------------------------- Package scope методы
// ----------------------------------------------------------------------------- Protected методы
// ----------------------------------------------------------------------------- Private методы

    //OH IT USES REFLECTION!!
    /**
     * Вызывает тестовые методы всех объектов
     * @param testList список тестов
     */
    private static void runTest(List testList){

        log("---------------------------------------------------------------");
        log("java.version="+System.getProperty("java.version"));
        log("java.vm.version="+System.getProperty("java.vm.version"));
        log("java.runtime.version="+System.getProperty("java.runtime.version"));
        log("java.vm.name="+System.getProperty("java.vm.name"));
        log("java.vm.info="+System.getProperty("java.vm.info"));
        log("java.vm.vendor="+System.getProperty("java.vm.vendor"));
        log("---------------------------------------------------------------");

        boolean success = true;
        for (int i = 0, indx = testList==null ? 0 : testList.size(); i <indx; i++) {
            Test test = (Test)testList.get(i);
            boolean testSuccess = true;
            log(test.getClass().getName());
            log("---------------------------------------------------------------");
            log(" STATUS |    TIME   | METHOD");
            log("---------------------------------------------------------------");
            try {
                Method[] mtds = test.getClass().getDeclaredMethods();

                //инициализируем тест
                test.init();

                //запускаем все test методы
                for (int j = 0, jindx = mtds==null ? 0 : mtds.length; j <jindx; j++) {
                    Method m = mtds[j];
                    int modifier = m.getModifiers();
                    //Метод будет вызван если:
                    //  метод возвращает boolean
                    //и метод начинается с "test"
                    //и метод не имеет входных параметров
                    //и метод объявлен как public
                    if(m.getReturnType()==Boolean.TYPE &&
                       m.getName().startsWith("test") &&
                      (m.getParameterTypes()==null || m.getParameterTypes().length==0) &&
                       Modifier.isPublic(modifier) ) {

                        Boolean rez = null;
                        Throwable th = null;
                        long start = 0;
                        long stop = 0;
                        try {

                            start = System.currentTimeMillis();
                            rez = (Boolean)m.invoke(test, null);
                            stop = System.currentTimeMillis();

                        } catch (Throwable t){
                            stop = System.currentTimeMillis();
                            th = t;
                        }
                        if(rez!=null && rez.booleanValue()){
                            log("SUCCESS | "+fill(""+(stop-start), 9)+" | "+m.getName()+"()");
                        } else {
                            success = false;
                            testSuccess = false;
                            log("FAILED  | "+fill(""+(stop-start), 9)+" | "+m.getName()+"()");
                            if(th!=null){
                                th.printStackTrace();
                            }
                        }

                        try {
                            test.afterEachMethod();
                        } catch (Throwable t){
                            t.printStackTrace();
                        }
                    }
                }

                //деинициализируем тест
                test.destroy();

            } catch (Exception e) {
                e.printStackTrace();
            }


            log("---------------------------------------------------------------");
            if(testSuccess){
                log("SUCCESS");
            } else {
                log("FAILED");
            }
            log("---------------------------------------------------------------");
        }
        if(success){
            log("TEST SUCCESS");
        } else {
            log("TEST FAILED");
            //System.exit(1);
        }
    }

    /**
     * Создаём тексты
     * @param classList список строк с названиями классов
     * @return список экземпляров тестов
     */
    private static List newInstances(List classList) {
        List rez = new ArrayList();
        for (int i = 0, indx = classList==null ? 0 : classList.size(); i <indx; i++) {
            String cl = (String)classList.get(i);
            Object obj = null;
            try {
                obj = Class.forName(cl).newInstance();
                obj = new net.sf.cache4j.test.SynchronizedCacheTest();
            } catch (Exception e){
                e.printStackTrace();
            }

            if((obj instanceof Test)) {
                rez.add(obj);
            } else {
                log("Class:"+cl+" not instance of "+Test.class.getName());
            }
        }

        return rez;
    }

    /**
     * Вывод на консоль
     */
    private static void log(String s){
        System.out.println(s);
    }

    /**
     * Дополняет строку src пробелами пока длина строки не будет больше либо равна count
     * @param src исходная строка
     * @param count минимальный размер результирующей строки
     */
    private static String fill(String src, int count){
        src = src==null ? "" : src;
        StringBuffer buf = new StringBuffer(src);
        while(buf.length()<count){
            buf.append(' ');
        }

        return buf.toString();
    }

// ----------------------------------------------------------------------------- Inner классы
}

/*
$Log: Cache4jTester.java,v $
Revision 1.1  2010/06/22 16:21:10  smhuang
*** empty log message ***

Revision 1.1  2010/06/18 17:01:16  smhuang
*** empty log message ***

*/
