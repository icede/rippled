//------------------------------------------------------------------------------
/*
    This file is part of Beast: https://github.com/vinniefalco/Beast
    Copyright 2013, Vinnie Falco <vinnie.falco@gmail.com>

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef BEAST_UTILITY_STATICOBJECT_H_INCLUDED
#define BEAST_UTILITY_STATICOBJECT_H_INCLUDED

#include <atomic>
#include <chrono>
#include <thread>

namespace beast {

// Spec: N2914=09-0104
//
// [3.6.2] Initialization of non-local objects
//
//         Objects with static storage duration (3.7.1) or thread storage
//         duration (3.7.2) shall be zero-initialized (8.5) before any
//         other initialization takes place.
//

/** Wrapper to produce an object with static storage duration.
    
    The object is constructed in a thread-safe fashion when the get function
    is first called. Note that the destructor for Object is never called. To
    invoke the destructor, use the AtExitHook facility (with caution).

    The Tag parameter allows multiple instances of the same Object type, by
    using different tags.

    Object must meet these requirements:
        DefaultConstructible

    @see AtExitHook
*/
template <class Object, typename Tag = void>
class StaticObject
{
public:
    static Object& get ()
    {
        StaticData& staticData (StaticData::get());

        if (staticData.state.load () != initialized)
        {
            if (staticData.state.exchange (initializing) == uninitialized)
            {
                // Initialize the object.
                new (&staticData.object) Object;
                staticData.state = initialized;
            }
            else
            {
                std::size_t n = 0;

                while (staticData.state.load () != initialized)
                {
                    ++n;

                    std::this_thread::yield ();

                    if (n > 10)
                    {
                        std::chrono::milliseconds duration (1);

                        if (n > 100)
                            duration *= 10;

                        std::this_thread::sleep_for (duration);
                    }
                }
            }
        }

        assert (staticData.state.load () == initialized);
        return staticData.object;
    }

private:
    static int const uninitialized = 0;
    static int const initializing = 1;
    static int const initialized = 2;

    // This structure gets zero-filled at static initialization time.
    // No constructors are called.
    //
    class StaticData
    {
    public:
        std::atomic <int> state;
        Object object;

        static StaticData& get ()
        {
            static std::uint8_t storage [sizeof (StaticData)];
            return *(reinterpret_cast <StaticData*> (&storage [0]));
        }

        StaticData() = delete;
        StaticData(StaticData const&) = delete;
        StaticData& operator= (StaticData const&) = delete;
        ~StaticData() = delete;
    };
};

}

#endif
