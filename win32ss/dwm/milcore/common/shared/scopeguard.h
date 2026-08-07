// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once
#include <stdexcept>
#include <functional>

namespace wpf
{
    namespace util
    {
        /// <summary>
        /// Non-copyable base class
        /// </summary>
        struct noncopyable
        {
            noncopyable() = default;
            noncopyable(const noncopyable&) = delete;
            noncopyable& operator=(const noncopyable&) = delete;
        };

        /// <summary>
        /// Generic RAII pattern to automatically 
        /// acquire and release a resource at the end
        /// of a scope
        /// </summary>
        template<typename Resource>
        class scopeguard : public noncopyable
        {
        private:
            /// <summary>
            /// The resource being protected and
            /// automatically released at the end 
            /// of the scope
            /// </summary>
            Resource data;

            /// <summary>
            /// Function used to release the resource
            /// </summary>
            std::function<void(Resource&)> release;
           

            /// <summary>
            /// Function used to test the goodness of the 
            /// resource
            /// </summary>
            std::function<bool(const Resource&)> good;

        private:

            /// <summary>
            /// Returns a reference to the resource
            /// </summary>
            /// <returns></returns>
            inline Resource& get_data()
            {
                return data;
            }

            /// <summary>
            /// Returns a const-reference to the resource
            /// </summary>
            /// <returns></returns>
            inline const Resource& get_data() const
            {
                return data;
            }

        public:

            /// <summary>
            /// Constructor
            /// </summary>
            /// <param name="acquire">Function used to acquire the resource</param>
            /// <param name="release">Function used to release the resource. This will be called at the end of the scope</param>
            /// <param name="good">Function used to test the goodness of the resource</param>
            inline scopeguard(std::function<Resource()> acquire, std::function<void(Resource&)> release, std::function<bool(const Resource&)> good)
                : data(acquire()), release(release), good(good)
            {
                if (!release || !good)
                {
                    throw std::logic_error("release and good functions must be supplied");
                }
            }

            /// <summary>
            /// Destructor - calls <see cref="release()"/>
            /// </summary>
            inline ~scopeguard()
            {
                if (valid())
                {
                    release(data);
                }
            }

            /// <summary>
            /// Tests whether the resource being protected is valid
            /// </summary>
            inline bool valid() const
            {
                return good(data);
            }

            /// <summary>
            /// Returns a reference to the resource
            /// </summary>
            inline Resource& resource()
            {
                return get_data();
            }

            /// <summary>
            /// Returns a const reference to the resource
            /// </summary>
            inline const Resource& resource() const
            {
                return get_data();
            }

            /// <summary>
            /// Implicitly converts to a reference to the resource
            /// </summary>
            inline operator Resource&()
            {
                return get_data();
            }

            /// <summary>
            /// Implicitly converts to a const reference to the resource
            /// </summary>
            inline operator const Resource&() const
            {
                return get_data();
            }
        };
    }
}
