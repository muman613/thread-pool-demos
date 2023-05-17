//
// Created by Michael Uman on 12/29/20.
//

#ifndef FOOBAR_FOOVARIANTMAP_H
#define FOOBAR_FOOVARIANTMAP_H

#include <map>
#include <boost/variant.hpp>
#include <boost/algorithm/string.hpp>

namespace foo {
    using variant_type  = boost::variant<uint32_t, std::string>;
    using variant_map   = std::map<std::string, variant_type>;
    using string_vec    = std::vector<std::string>;

    std::ostream &operator<<( std::ostream &os, const variant_map &map );

    class VariantMap
    {
        variant_map         map_;
        mutable std::mutex  mutex_;

        friend std::ostream &operator<<( std::ostream &os, const VariantMap &map );

        /**
         * Iterator class
         */
        class VariantMapIter
        {
            friend VariantMap;

            VariantMap &vm_;
            string_vec keys_;
            size_t index_;

            explicit VariantMapIter( VariantMap &map, size_t i = 0 );

        public:
            VariantMapIter &operator++( int i );

            VariantMapIter &operator++();

            const variant_type &operator*();

            bool operator!=( const VariantMapIter &other );

            std::string key() const;
        };

        friend VariantMapIter;

        /**
         * Throw std::runtime_error if the key already exists.
         *
         * @param key
         */
        void verify_unique_key(const std::string & key);

    public:
        VariantMap();

        virtual ~VariantMap();

        /**
         * Add a string variable to map.
         *
         * @param key Name of key
         * @param value String value
         */
        void add_variable( const std::string &key, const std::string &value );

        /**
         * Add an integer variable to map.
         *
         * @param key Name of key
         * @param value Integer value
         */
        void add_variable( const std::string &key, uint32_t value );

        /**
         * Return a string vector of map keys in alphabetical order.
         *
         * @return vector of strings.
         */
        string_vec get_keys( const std::string &search = "" ) const;


        size_t size() const;

        VariantMapIter begin();

        VariantMapIter end();

        // Load & Save variable map to ini file
        void load_map_from_ini( const std::string &filename, const std::string &section );

        void save_map_to_ini( const std::string &filename, const std::string &section );

        void lock();

        void unlock();

        class VariantMapLocker
        {
        private:
            VariantMap &vm_;
        public:
          explicit VariantMapLocker(VariantMap & map);
          virtual ~VariantMapLocker();
        };

        class VariantAccessor {
        private:
            friend class VariantMap;

            variant_type &vt_;
            explicit VariantAccessor(variant_type & variant) : vt_{variant} {}
        public:
            VariantAccessor(const VariantMap::VariantAccessor & copy) : vt_{copy.vt_} {  }

            virtual ~VariantAccessor() {}

            VariantAccessor & operator=(uint32_t value);
            VariantAccessor & operator=(const std::string & value);
            const variant_type & operator()() const;

            /**
             * Cast operators (will throw if the variant doesnt contain specified type)
             *
             * @return
             */
            explicit operator uint32_t();
            explicit operator std::string();

            const std::type_info & type() const;

            friend std::ostream & operator << (std::ostream & os, const VariantMap::VariantAccessor & accessor);
        };

        VariantAccessor operator[](const std::string & key);

//        variant_type & operator[](const std::string & key);
    };

}
#endif //FOOBAR_FOOVARIANTMAP_H
