/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
 *
 * akvirtualcamera is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * akvirtualcamera is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with akvirtualcamera. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#include <windows.h>

#include "mutex.h"

namespace AkVCam
{
    class MutexPrivate
    {
        public:
            HANDLE m_mutex;
            std::string m_name;
    };
}

AkVCam::Mutex::Mutex(const std::string &name)
{
    this->d = new MutexPrivate();
    this->d->m_mutex = CreateMutexA(nullptr,
                                    FALSE,
                                    name.empty()? nullptr: name.c_str());
    this->d->m_name = name;
}

AkVCam::Mutex::Mutex(const Mutex &other)
{
    this->d = new MutexPrivate();
    this->d->m_mutex = CreateMutexA(nullptr,
                                    FALSE,
                                    other.d->m_name.empty()?
                                        nullptr: other.d->m_name.c_str());
    this->d->m_name = other.d->m_name;
}

AkVCam::Mutex::~Mutex()
{
    if (this->d->m_mutex)
        CloseHandle(this->d->m_mutex);

    delete this->d;
}

AkVCam::Mutex &AkVCam::Mutex::operator =(const Mutex &other)
{
    if (this != &other) {
        this->unlock();

        if (this->d->m_mutex)
            CloseHandle(this->d->m_mutex);

        this->d->m_mutex = CreateMutexA(nullptr,
                                        FALSE,
                                        other.d->m_name.empty()?
                                            nullptr: other.d->m_name.c_str());
        this->d->m_name = other.d->m_name;
    }

    return *this;
}

std::string AkVCam::Mutex::name() const
{
    return this->d->m_name;
}

void AkVCam::Mutex::lock()
{
    if (!this->d->m_mutex)
        return;

    WaitForSingleObject(this->d->m_mutex, INFINITE);
}

bool AkVCam::Mutex::tryLock(int timeout)
{
    if (!this->d->m_mutex)
        return false;

    DWORD waitResult = WaitForSingleObject(this->d->m_mutex,
                                           !timeout? INFINITE: DWORD(timeout));

    return waitResult != WAIT_FAILED && waitResult != WAIT_TIMEOUT;
}

void AkVCam::Mutex::unlock()
{
    if (!this->d->m_mutex)
        return;

    ReleaseMutex(this->d->m_mutex);
}
