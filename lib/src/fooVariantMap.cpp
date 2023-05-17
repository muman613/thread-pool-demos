//
// Created by Michael Uman on 12/29/20.
//

#include <iostream>
#include <atomic>
#include "fooUtils.h"
#include "fooVariantMap.h"


namespace foo {
    std::ostream &operator<<( std::ostream &os, const variant_map &map )
    {
      string_vec elements;
      for (const auto &map_pair : map) {
        std::ostringstream oss;
        oss << map_pair.first << "=" << map_pair.second;
        elements.push_back(oss.str());
      }
      auto line = boost::algorithm::join(elements, ";");

      return os << "(" << line << ")";
    }

    VariantMap::VariantMap()
    {
#if !defined(NDEBUG) && ENABLE_TRACE
      std::cout << __PRETTY_FUNCTION__ << std::endl;
#endif
    }

    VariantMap::~VariantMap()
    {
#if !defined(NDEBUG) && ENABLE_TRACE
      std::cout << __PRETTY_FUNCTION__ << std::endl;
#endif
    }

    std::ostream &operator<<( std::ostream &os, const VariantMap &map )
    {
      return os << "VariantMap" << map.map_;
    }

    void VariantMap::load_map_from_ini( const std::string &filename, const std::string &section )
    {

    }

    void VariantMap::save_map_to_ini( const std::string &filename, const std::string &section )
    {

    }

    void VariantMap::add_variable( const std::string &key, const std::string &value )
    {
      verify_unique_key(key);
      map_[key] = value;
    }

    void VariantMap::add_variable( const std::string &key, uint32_t value )
    {
      verify_unique_key(key);
      map_[key] = value;
    }

    string_vec VariantMap::get_keys( const std::string &search ) const
    {
      string_vec sv;

      for (const auto &var_pair : map_) {
        if (search.empty()) {
          sv.push_back(var_pair.first);
        } else if (var_pair.first.rfind(search, 0) == 0) {
          sv.push_back(var_pair.first);
        }
      }
      std::sort(sv.begin(), sv.end());

      return sv;
    }

    void VariantMap::lock()
    {
      mutex_.lock();
    }

    void VariantMap::unlock()
    {
      mutex_.unlock();
    }

    VariantMap::VariantMapIter VariantMap::begin()
    {
      return VariantMap::VariantMapIter(*this, 0);
    }

    VariantMap::VariantMapIter VariantMap::end()
    {
      return VariantMap::VariantMapIter(*this, map_.size());
    }

    const variant_type &VariantMap::VariantMapIter::operator*()
    {
      auto key_name = keys_[index_];
      return vm_.map_[key_name];
    }

    VariantMap::VariantMapIter &VariantMap::VariantMapIter::operator++( int i )
    {
      index_++;
      return *this;
    }

    VariantMap::VariantMapIter &VariantMap::VariantMapIter::operator++()
    {
      index_++;
      return *this;
    }

    bool VariantMap::VariantMapIter::operator!=( const VariantMap::VariantMapIter &other )
    {
      return index_ != other.index_;
    }

    VariantMap::VariantMapIter::VariantMapIter( VariantMap &map, size_t i )
            : vm_{map}, index_{i}, keys_{map.get_keys()}
    {

    }

    std::string VariantMap::VariantMapIter::key() const
    {
      return keys_[index_];
    }

    size_t VariantMap::VariantMap::size() const
    {
      return map_.size();
    }

    VariantMap::VariantAccessor VariantMap::operator[]( const std::string &key )
    {
      return VariantAccessor(map_.at(key));
    }

    /**
     * VariantMapLocker Class
     */

    VariantMap::VariantMapLocker::VariantMapLocker( VariantMap &map )
            : vm_{map}
    {
      vm_.lock();
    }

    VariantMap::VariantMapLocker::~VariantMapLocker()
    {
      vm_.unlock();
    }

    const variant_type &VariantMap::VariantAccessor::operator()() const
    {
      return vt_;
    }

    VariantMap::VariantAccessor & VariantMap::VariantAccessor::operator=( uint32_t value )
    {
      auto typeName = utils::type_name(vt_);

      if (vt_.type() == typeid(std::string)) {
        throw std::invalid_argument("Attempt to set integer on string property");
      }
      vt_ = value;
      return *this;
    }

    VariantMap::VariantAccessor & VariantMap::VariantAccessor::operator=( const std::string & value )
    {
      auto typeName = utils::type_name(vt_);

      if (vt_.type() == typeid(uint32_t)) {
        throw std::invalid_argument("Attempt to set string on integer property");
      }
      vt_ = value;
      return *this;
    }

    std::ostream &operator<<( std::ostream &os, const VariantMap::VariantAccessor &accessor )
    {
      return os << accessor();
    }

    void VariantMap::verify_unique_key( const std::string &key )
    {
      if (map_.find(key) != map_.end()) {
        throw std::runtime_error("Duplicate key");
      }
    }

    VariantMap::VariantAccessor::operator uint32_t()
    {
      return boost::get<uint32_t>(vt_);
    }

    VariantMap::VariantAccessor::operator std::string()
    {
      return boost::get<std::string>(vt_);
    }

    const std::type_info & VariantMap::VariantAccessor::type() const
    {
      return vt_.type();
    }
}