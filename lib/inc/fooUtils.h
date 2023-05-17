#ifndef FOOBAR_FOOUTILS_H
#define FOOBAR_FOOUTILS_H

#include <iostream>
#include <ios>
#include <string>
#include <sstream>
#include <vector>
#include <cctype>
#include <iomanip>
#include <vector>
#include <mutex>
#include <cxxabi.h>
#include <atomic>

#include <boost/tokenizer.hpp>

#define FOOLOG(x) {  std::lock_guard<std::mutex> guard(coutMutex);  std::cerr << __FILE__ << ":" << __LINE__ << " | " << (x) << std::endl; }

extern std::mutex coutMutex;

#ifndef NDEBUG
#define DEBUG(x)  (x)
#else
#define DEBUG(x) /* (x) */
#endif

namespace foo {
    namespace utils {
        /**
         * Dumps a buffer to a hex & ascii string.
         *
         * @param buffer Pointer to byte data
         * @param len Length of data to dump
         * @return string containing hex & ascii string.
         */
        std::string               dump_buffer_hex(uint8_t * buffer, size_t len);
        std::string               dump_vector(std::vector<uint8_t> vec);
        std::vector<std::string>  tokenize_string(const std::string & str);

        std::string               demangle_symbol( const std::string & mangled_name);

        template<typename T>
        std::string addr_to_hex_string(T addr, bool uc = false) {
          std::ostringstream oss;
          auto hex_chars = sizeof(T) * 2; // 2 nibbles per bytes
          if (uc)
            oss.setf(std::ios::uppercase);
          oss << std::showbase << std::internal << std::hex << std::setw(hex_chars + 2) << std::setfill('0') << addr;
          return oss.str();
        }

        template<typename T>
        void dump_vector_elements(const T & v, std::ostream & os = std::cout) {
          for (auto & item : v) {
            os << item << std::endl;
          }
        }

        template<typename T>
        std::string type_name( T var )
        {
          std::string mangled_name = typeid(var).name();
//          int status = 0;
//          char *realname = abi::__cxa_demangle(mangled_name.c_str(), 0, 0, &status);
//          std::string demangled_name{realname};
//          free(realname);
          return demangle_symbol(mangled_name);
        }

        class flagContainer {
            using flagVec = std::vector<std::string>;

            std::atomic<uint32_t>   flag_value_{0};
            flagVec                 flags_;
            std::mutex              mtx_;

            bool setFlagImpl(const std::string & flagName, bool value = true);

        public:
            flagContainer() = default;
            flagContainer(const flagContainer &copy);
            flagContainer(flagContainer &&move) noexcept;

            bool operator[](const std::string & flagName);

            /**
             * Add a named flag to the vector of flags.
             *
             * @param flagName
             * @return
             */
            bool addFlag(const std::string & flagName);

            /**
             * Add several flags in one call.
             *
             * @param list
             * @return
             */
            bool addFlags(std::initializer_list<std::string> list);

            /**
             * Set the named flag in the container.
             *
             * @param flagName
             * @param value
             * @return
             */
            bool setFlag(const std::string & flagName, bool value = true);

            /**
             * Set all flags in initializer list to value.
             *
             * @param list Initializer list of strings
             * @param value true to set the flag, false to clear it
             * @return true if all flags are set successfully, otherwise false.
             */
            bool setFlags(std::initializer_list<std::string> list, bool value = true);
            /**
             * Get the named flag from container.
             *
             * @param flagName
             * @return
             */
            bool getFlag(const std::string & flagName);

            uint32_t operator()() const;

            friend std::ostream & operator << (std::ostream & os, const flagContainer & container);
        };

        /**
         * Class used to display a uniform heading
         */
        class ReportHeading {
            std::string     heading_;
            std::ostream &  os_;
            int             width_;
            char            fill_{'-'};
            void            output(const std::string & prefix);

        public:
            explicit ReportHeading(std::ostream & os, std::string  heading, int width = 80);
            virtual ~ReportHeading();
        };
    };
};

#endif //FOOBAR_FOOUTILS_H
