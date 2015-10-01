/* =========================================================================
 * File: $Id: CacheException.java,v 1.1 2010/06/18 17:01:12 smhuang Exp $CacheException.java,v$
 *
 * Copyright (c) 2006, Yuriy Stepovoy. All rights reserved.
 * email: stepovoy@gmail.com
 *
 * =========================================================================
 */

package net.sf.cache4j;

/**
 * CacheException ���������� �������� �����
 *
 * @version $Revision: 1.1 $ $Date: 2010/06/18 17:01:12 $
 * @author Yuriy Stepovoy. <a href="mailto:stepovoy@gmail.com">stepovoy@gmail.com</a>
 **/

public class CacheException extends Exception {
// ----------------------------------------------------------------------------- ���������
// ----------------------------------------------------------------------------- �������� ������
// ----------------------------------------------------------------------------- ����������� ����������
// ----------------------------------------------------------------------------- ������������
// ----------------------------------------------------------------------------- Public ������

    /**
     * �����������
     */
    public CacheException() {
    }

    /**
     * ����������� � ��������� ���������
     * @param msg ���������
     */
    public CacheException(String msg) {
        super(msg);
    }

// ----------------------------------------------------------------------------- Package scope ������
// ----------------------------------------------------------------------------- Protected ������
// ----------------------------------------------------------------------------- Private ������
// ----------------------------------------------------------------------------- Inner ������
}

/*
$Log: CacheException.java,v $
Revision 1.1  2010/06/18 17:01:12  smhuang
*** empty log message ***

*/