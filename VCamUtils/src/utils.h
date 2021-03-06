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

#ifndef AKVCAMUTILS_UTILS_H
#define AKVCAMUTILS_UTILS_H

#include <cstdint>
#include <string>
#include <vector>

#ifndef UNUSED
    #define UNUSED(x) (void)(x)
#endif

#define GLOBAL_STATIC(type, variableName) \
    type *variableName() \
    { \
        static type _##variableName; \
        \
        return &_##variableName; \
    }

#define GLOBAL_STATIC_WITH_ARGS(type, variableName, ...) \
    type *variableName() \
    { \
        static type _##variableName {__VA_ARGS__}; \
        \
        return &_##variableName; \
    }

#define AKVCAM_CALLBACK(CallbackName, ...) \
    using CallbackName##CallbackT = void (*)(void *userData, __VA_ARGS__); \
    using CallbackName##Callback = std::pair<void *, CallbackName##CallbackT>;

#define AKVCAM_CALLBACK_NOARGS(CallbackName) \
    using CallbackName##CallbackT = void (*)(void *userData); \
    using CallbackName##Callback = std::pair<void *, CallbackName##CallbackT>;

#define AKVCAM_SIGNAL(CallbackName, ...) \
    public: \
        using CallbackName##CallbackT = void (*)(void *userData, __VA_ARGS__); \
        using CallbackName##Callback = std::pair<void *, CallbackName##CallbackT>; \
        \
        void connect##CallbackName(void *userData, \
                                   CallbackName##CallbackT callback) \
        { \
            if (!callback) \
                return; \
            \
            for (auto &func: this->m_##CallbackName##Callback) \
                if (func.first == userData \
                    && func.second == callback) \
                    return; \
            \
            this->m_##CallbackName##Callback.push_back({userData, callback});\
        } \
        \
        void disconnect##CallbackName(void *userData, \
                                      CallbackName##CallbackT callback) \
        { \
            if (!callback) \
                return; \
            \
            for (auto it = this->m_##CallbackName##Callback.begin(); \
                it != this->m_##CallbackName##Callback.end(); \
                it++) \
                if (it->first == userData \
                    && it->second == callback) { \
                    this->m_##CallbackName##Callback.erase(it); \
                    \
                    return; \
                } \
        } \
    \
    private: \
        std::vector<CallbackName##Callback> m_##CallbackName##Callback;

#define AKVCAM_SIGNAL_NOARGS(CallbackName) \
    public: \
        using CallbackName##CallbackT = void (*)(void *userData); \
        using CallbackName##Callback = std::pair<void *, CallbackName##CallbackT>; \
        \
        void connect##CallbackName(void *userData, \
                                   CallbackName##CallbackT callback) \
        { \
            if (!callback) \
                return; \
            \
            for (auto &func: this->m_##CallbackName##Callback) \
                if (func.first == userData \
                    && func.second == callback) \
                    return; \
            \
            this->m_##CallbackName##Callback.push_back({userData, callback});\
        } \
        \
        void disconnect##CallbackName(void *userData, \
                                      CallbackName##CallbackT callback) \
        { \
            if (!callback) \
                return; \
            \
            for (auto it = this->m_##CallbackName##Callback.begin(); \
                it != this->m_##CallbackName##Callback.end(); \
                it++) \
                if (it->first == userData \
                    && it->second == callback) { \
                    this->m_##CallbackName##Callback.erase(it); \
                    \
                    return; \
                } \
        } \
    \
    private: \
        std::vector<CallbackName##Callback> m_##CallbackName##Callback;

#define AKVCAM_EMIT(owner, CallbackName, ...) \
    for (auto &callback: owner->m_##CallbackName##Callback) \
        if (callback.second) \
            callback.second(callback.first, __VA_ARGS__); \

#define AKVCAM_EMIT_NOARGS(owner, CallbackName) \
    for (auto &callback: owner->m_##CallbackName##Callback) \
        if (callback.second) \
            callback.second(callback.first); \

namespace AkVCam
{
    uint64_t id();
    std::string timeStamp();
    std::string replace(const std::string &str,
                        const std::string &from,
                        const std::string &to);
    std::string trimmed(const std::string &str);
    std::string fill(const std::string &str, size_t maxSize);
    std::string join(const std::vector<std::string> &strs,
                     const std::string &separator);
    std::vector<std::string> split(const std::string &str, char separator);
    std::pair<std::string, std::string> splitOnce(const std::string &str,
                                                  const std::string &separator);
}

#endif // AKVCAMUTILS_UTILS_H
