/* =========================================================================
 * File: $Id: FIFOComparator.java,v 1.1 2010/06/18 17:01:09 smhuang Exp $FIFOComparator.java,v$
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
 * Класс FIFOComparator используется для сорировки объектов в соответствии с
 * алгоритмом FIFO (First In First Out).
 * Если при помещении нового объекта кеш переполняется, по размеру или количеству
 * объектов, то удаляется объект который был помещён первым.
 *
 * @version $Revision: 1.1 $ $Date: 2010/06/18 17:01:09 $
 * @author Yuriy Stepovoy. <a href="mailto:stepovoy@gmail.com">stepovoy@gmail.com</a>
 **/

public class FIFOComparator implements Comparator {
// ----------------------------------------------------------------------------- Константы
// ----------------------------------------------------------------------------- Атрибуты класса
// ----------------------------------------------------------------------------- Статические переменные
// ----------------------------------------------------------------------------- Конструкторы
// ----------------------------------------------------------------------------- Public методы

    public int compare(Object o1, Object o2) {
        CacheObject co1 = (CacheObject)o1;
        CacheObject co2 = (CacheObject)o2;
        return co1.getCreateTime()<co2.getCreateTime() ?
                  -1
                : co1.getCreateTime()==co2.getCreateTime() ?
                    ( co1.getId()<co2.getId() ? -1 : (co1.getId()==co2.getId() ? 0 : 1) )
                     : 1;
    }


    public boolean equals(Object obj) {
        return obj==null ? false : (obj instanceof FIFOComparator);
    }

// ----------------------------------------------------------------------------- Package scope методы
// ----------------------------------------------------------------------------- Protected методы
// ----------------------------------------------------------------------------- Private методы
// ----------------------------------------------------------------------------- Inner классы
}

/*
$Log: FIFOComparator.java,v $
Revision 1.1  2010/06/18 17:01:09  smhuang
*** empty log message ***

*/
