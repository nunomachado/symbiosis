/* =========================================================================
 * File: $Id: LFUComparator.java,v 1.1 2010/06/18 17:01:10 smhuang Exp $LFUComparator.java,v$
 *
 * Copyright (c) 2006, Yuriy Stepovoy. All rights reserved.
 * email: stepovoy@gmail.com
 *
 * =========================================================================
 */

package net.sf.cache4j.impl;

import net.sf.cache4j.impl.CacheObject;

import java.util.Comparator;

/**
 * Класс LFUComparator используется для сорировки объектов в соответствии с
 * алгоритмом LFU (Least Frequently Used).
 * Если при помещении нового объекта кеш переполняется, по размеру или количеству
 * объектов, то удаляется объект который использовался наименьшее количество раз.
 *
 * @version $Revision: 1.1 $ $Date: 2010/06/18 17:01:10 $
 * @author Yuriy Stepovoy. <a href="mailto:stepovoy@gmail.com">stepovoy@gmail.com</a>
 **/

public class LFUComparator implements Comparator {
// ----------------------------------------------------------------------------- Константы
// ----------------------------------------------------------------------------- Атрибуты класса
// ----------------------------------------------------------------------------- Статические переменные
// ----------------------------------------------------------------------------- Конструкторы
// ----------------------------------------------------------------------------- Public методы

    public int compare(Object o1, Object o2) {
        CacheObject co1 = (CacheObject)o1;
        CacheObject co2 = (CacheObject)o2;
        return co1.getAccessCount()<co2.getAccessCount() ?
                  -1
                : co1.getAccessCount()==co2.getAccessCount() ?
                    ( co1.getId()<co2.getId() ? -1 : (co1.getId()==co2.getId() ? 0 : 1) )
                     : 1;
    }


    public boolean equals(Object obj) {
        return obj==null ? false : (obj instanceof LFUComparator);
    }

// ----------------------------------------------------------------------------- Package scope методы
// ----------------------------------------------------------------------------- Protected методы
// ----------------------------------------------------------------------------- Private методы
// ----------------------------------------------------------------------------- Inner классы
}

/*
$Log: LFUComparator.java,v $
Revision 1.1  2010/06/18 17:01:10  smhuang
*** empty log message ***

*/
