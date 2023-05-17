#include <string>
#include <sstream>
#include <vector>
#include <cctype>
#include <iomanip>
#include <vector>
#include <atomic>
#include <boost/predef.h>
#include <boost/tokenizer.hpp>
#if BOOST_PREDEF_VERSION >= BOOST_VERSION_NUMBER(1,11,0)
#include <boost/io/quoted.hpp>
#else
#include <boost/io/detail/quoted_manip.hpp>
#endif

#include "fooUtils.h"


namespace foo {
    namespace utils {
        std::string dump_vector(std::vector<uint8_t> vec) {
          std::ostringstream outss;
          // Dump vector
          outss << "( ";
          for_each(vec.begin(), vec.end(), [&](uint8_t ch) {
              outss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') <<(int)ch;
          });
          outss << " : '";
          for_each(vec.begin(), vec.end(), [&](uint8_t c) {
              outss << (isprint(c)?(char)c:'.');
          });
          outss << "' )";
          return outss.str();
        }
        /**
         * Dumps a buffer to a hex & ascii string.
         *
         * @param buffer Pointer to byte data
         * @param len Length of data to dump
         * @return string containing hex & ascii string.
         */
        std::string  dump_buffer_hex(uint8_t * buffer, size_t len)
        {
//          std::ostringstream oss;
//          oss << "( ";
          std::vector<uint8_t> storage(buffer, buffer + len);
          return dump_vector(storage);
//          for_each(storage.begin(), storage.end(), [&](uint8_t c) {
//              oss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)c;
//          });
//          oss << " : '";
//          for_each(storage.begin(), storage.end(), [&](uint8_t c) {
//              oss << (isprint(c)?(char)c:'.');
//          });
//          oss << "' )";
//          return oss.str();
        }

        std::vector<std::string> tokenize_string(const std::string & str)
        {
          std::vector<std::string> tokens;
          std::stringstream iss(str);
          std::string s;

          while (iss >> boost::io::quoted(s)) {
            tokens.push_back(s);
          }

          return tokens;
        }


        ReportHeading::ReportHeading( std::ostream &os, std::string heading, int width )
                : heading_{std::move(heading)},
                  os_{os},
                  width_{width}
        {
          std::string prefix = "+--Begin ";
          output(prefix);
        }

        ReportHeading::~ReportHeading()
        {
          std::string prefix = "+--End ";
          output(prefix);
          os_ << std::endl;
        }

        void ReportHeading::output( const std::string &prefix )
        {
          std::ios oldState{nullptr};
          oldState.copyfmt(os_);
          os_ << prefix << std::setw(width_-prefix.length()) << std::setfill(fill_) << std::left << heading_ << std::endl;
          os_.copyfmt(oldState);
        }

        /**
         * Flag Container Class
         */
        flagContainer::flagContainer( const flagContainer &copy )
        {
          flag_value_.store(copy.flag_value_);
          flags_      = copy.flags_;
        }

        flagContainer::flagContainer( flagContainer &&move ) noexcept
        {
          flag_value_.store(move.flag_value_);
          flags_      = std::move(move.flags_);
        }

        bool flagContainer::operator[]( const std::string &flagName )
        {
          return getFlag(flagName);
        }

        bool flagContainer::setFlagImpl( const std::string &flagName, bool value )
        {
          bool result = false;
          flagVec::iterator it;

          if ((it = std::find(flags_.begin(), flags_.end(), flagName)) != flags_.end())
          {
            uint32_t index = it - flags_.begin();
            if ((index >= 0) && (index <= 31))
            {
              uint32_t bitflag = (1 << index);
              if (value)
              {
                flag_value_ |= bitflag;
              }
              else
              {
                flag_value_ &= ~bitflag;
              }

              result = true;
            }
          }
          return result;
        }

        bool flagContainer::addFlag( const std::string &flagName )
        {
          std::lock_guard<std::mutex> guard(mtx_);

          bool result = false;
          // If flag name does not exist in the vector, add it...
          if (std::find(flags_.begin(), flags_.end(), flagName) == flags_.end())
          {
            flags_.push_back(flagName);
            result = true;
          }
          return result;
        }

        bool flagContainer::addFlags( std::initializer_list<std::string> list )
        {
          std::lock_guard<std::mutex> guard(mtx_);
          flags_.insert(flags_.end(), list.begin(), list.end());
          return true;
        }

        bool flagContainer::setFlag( const std::string &flagName, bool value )
        {
          std::lock_guard<std::mutex> guard(mtx_);
          return setFlagImpl(flagName, value);
        }

        bool flagContainer::setFlags( std::initializer_list<std::string> list, bool value )
        {
          std::lock_guard<std::mutex> guard(mtx_);
          bool result = true;

          for (const auto & flag : list)
          {
            result &= setFlagImpl(flag, value);
          }

          return result;
        }

        bool flagContainer::getFlag( const std::string &flagName )
        {
          std::lock_guard<std::mutex> guard(mtx_);

          bool result = false;
          flagVec::iterator it;

          if ((it = std::find(flags_.begin(), flags_.end(), flagName)) != flags_.end())
          {
            uint32_t index = it - flags_.begin();
            if ((index >= 0) && (index <= 31))
            {
              uint32_t bitflag = (1 << index);

              result = ((flag_value_ & bitflag) != 0);
            }
          }

          return result;
        }

        uint32_t flagContainer::operator()() const
        {
          return flag_value_;
        }

        std::ostream &operator<<( std::ostream &os, const flagContainer &container )
        {
          bool first = true;

          os << "flags ( ";
          for (size_t index = 0 ; index < 31 ; index++)
          {
            uint32_t bitmask = (1 << index);
            if (container.flag_value_ & bitmask)
            {
              if (!first)
              {
                os << " ";
              }
              os << container.flags_[index];
              first = false;
            }
          }
          os << " )";

          return os;
        }

        std::string demangle_symbol( const std::string &mangled_name )
        {
          int status{0};
          char *realname = abi::__cxa_demangle(mangled_name.c_str(), 0, 0, &status);
          std::string demangled_name{realname};
          free(realname);
          return demangled_name;
        }
    }
}
