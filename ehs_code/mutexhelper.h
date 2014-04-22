/* $Id: mutexhelper.h 43 2010-06-14 02:42:17Z felfert $
 *
 * EHS is a library for embedding HTTP(S) support into a C++ application
 *
 * Copyright (C) 2004 Zachary J. Hansen
 *
 * Code cleanup, new features and bugfixes: Copyright (C) 2010 Fritz Elfert
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License version 2.1 as published by the Free Software Foundation;
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with this library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    This can be found in the 'COPYING' file.
 *
 */

#ifndef _MUTEXHELPER_H_
#define _MUTEXHELPER_H_
#include <pthread.h>

/**
 * Automatically unlocks a mutex if destroyed.
 */
class MutexHelper {
    public:
        MutexHelper(pthread_mutex_t *mutex, bool locknow = true) :
            m_pMutex(mutex), m_bLocked(false)
        {
            if (locknow)
                Lock();
        }

        ~MutexHelper()
        {
            if (m_bLocked)
                pthread_mutex_unlock(m_pMutex);
        }

        void Lock()
        {
            pthread_mutex_lock(m_pMutex);
            m_bLocked = true;
        }

        void Unlock()
        {
            m_bLocked = false;
            pthread_mutex_unlock(m_pMutex);
        }
    private:
        pthread_mutex_t *m_pMutex;
        bool m_bLocked;

        MutexHelper(const MutexHelper &);
        MutexHelper & operator=(const MutexHelper &);
};
#endif
