#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include <memory>
#include <string>
#include <sstream>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include "log.h"
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <list>

namespace sylar {

/**
 * @class ConfigVarBase
 * @brief Base class for type-erased configuration variables
 * 
 * Provides common interface for all configuration variables regardless of type.
 * Implements name normalization (to lowercase) and basic metadata storage.
 */
class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    
    /**
     * @brief Construct a new configuration variable
     * @param name Variable identifier (automatically converted to lowercase)
     * @param description Human-readable documentation
     */
    ConfigVarBase(const std::string& name, const std::string& description = "") 
        : m_name(name), m_description(description) {
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }
    
    virtual ~ConfigVarBase() = default;

    /// @return Normalized name of this configuration variable
    const std::string& getName() const { return m_name; }
    
    /// @return Description of this configuration variable  
    const std::string& getDescription() const { return m_description; }

    /// Serialize value to string representation (pure virtual)
    virtual std::string toString() = 0;
    
    /// Deserialize value from string (pure virtual)
    virtual bool fromString(const std::string& val) = 0;
    
    /// @return Name of the value type (pure virtual)
    virtual std::string getTypeName() const = 0;

private:
    std::string m_name;         ///< Normalized configuration key
    std::string m_description;  ///< Human-readable documentation
};

/**
 * @class LexicalCast
 * @brief Policy-based type conversion system for configuration values
 * 
 * Provides bidirectional conversion between string representations and native types,
 * with specializations for STL containers using YAML serialization format.
 */

/**
 * @brief Primary template for generic type conversion
 * @tparam F Source type
 * @tparam T Destination type
 * 
 * Default implementation uses boost::lexical_cast for basic type conversions.
 * Specializations provide custom conversions for container types.
 */
template<class F, class T>
class LexicalCast {
public:
    T operator()(const F& v) {
        return boost::lexical_cast<T>(v);
    }
};

// Container Specializations ///////////////////////////////////////////////////

/**
 * @brief Vector serialization/deserialization
 * @tparam T Element type
 * 
 * Format: YAML sequence (e.g., "[value1, value2, value3]")
 */
template<class T>
class LexicalCast<std::string, std::vector<T>> {
public:
    std::vector<T> operator() (const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator() (const std::vector<T>& v) {
        YAML::Node node;
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief List serialization/deserialization
 * @tparam T Element type
 * 
 * Preserves insertion order. Format same as vector.
 */
template<class T>
class LexicalCast<std::string, std::list<T>> {
public:
    std::list<T> operator() (const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::list<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::list<T>, std::string> {
public:
    std::string operator() (const std::list<T>& v) {
        YAML::Node node;
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief Set serialization/deserialization
 * @tparam T Element type
 * 
 * Enforces uniqueness. Format same as vector.
 */
template<class T>
class LexicalCast<std::string, std::set<T>> {
public:
    std::set<T> operator() (const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::set<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::set<T>, std::string> {
public:
    std::string operator() (const std::set<T>& v) {
        YAML::Node node;
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief Unordered_set serialization/deserialization
 * @tparam T Element type
 * 
 * Hash-based variant of set. Format same as vector.
 */
template<class T>
class LexicalCast<std::string, std::unordered_set<T>> {
public:
    std::unordered_set<T> operator() (const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
    std::string operator() (const std::unordered_set<T>& v) {
        YAML::Node node;
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief Map serialization/deserialization
 * @tparam T Value type
 * 
 * Format: YAML mapping (e.g., "{key1: value1, key2: value2}")
 * Keys must be strings. Values use T's conversion.
 */
template<class T>
class LexicalCast<std::string, std::map<std::string, T>> {
public:
    std::map<std::string, T>  operator() (const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::map<std::string, T> vec;
        std::stringstream ss;
        for(auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
    std::string operator() (const std::map<std::string, T>& v) {
        YAML::Node node;
        for(auto& i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief Unordered_map serialization/deserialization
 * @tparam T Value type
 * 
 * Hash-based variant of map. Format same as ordered map.
 */
template<class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
public:
    std::unordered_map<std::string, T>  operator() (const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_map<std::string, T> vec;
        std::stringstream ss;
        for(auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator() (const std::unordered_map<std::string, T>& v) {
        YAML::Node node;
        for(auto& i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @class ConfigVar
 * @brief Typed configuration variable container with serialization support
 * 
 * @tparam T The value type stored in this configuration variable
 * @tparam FromStr Conversion policy from string to T (default: LexicalCast<std::string, T>)
 * @tparam ToStr Conversion policy from T to string (default: LexicalCast<T, std::string>)
 *
 * Implements type-safe configuration storage with bidirectional string conversion
 * capabilities for serialization/deserialization.
 */
template<class T, 
         class FromStr = LexicalCast<std::string, T>,
         class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVar> ptr;

    /**
     * @brief Construct a new ConfigVar object
     * @param name Unique identifier for this configuration variable
     * @param default_value Initial value
     * @param description Human-readable description
     */
    ConfigVar(const std::string& name, 
             const T& default_value, 
             const std::string& description = "")
        : ConfigVarBase(name, description)
        , m_val(default_value) {}

    /**
     * @brief Serialize the stored value to string representation
     * @return std::string Serialized value
     * 
     * Uses the ToStr conversion policy. Catches and logs conversion errors.
     */
    std::string toString() override {
        try {
            return ToStr()(m_val);
        } catch(std::exception& e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) 
                << "ConfigVar::toString exception "
                << e.what() << " converting: " 
                << typeid(m_val).name() << " to string";
        }
        return ""; 
    }

    /**
     * @brief Deserialize from string and update stored value
     * @param val String representation to parse
     * @return bool True if conversion and update succeeded
     * 
     * Uses the FromStr conversion policy. Catches and logs conversion errors.
     */
    bool fromString(const std::string& val) override {
        try {
            setValue(FromStr()(val));
            return true;
        } catch (std::exception& e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) 
                << "ConfigVar::fromString exception "
                << e.what() << " converting string to " 
                << typeid(m_val).name();
        }
        return false;
    }
/**
 * @brief Unordered_map serialization/deserialization
 * @tparam T Value type
 * 
 * Hash-based variant of map. Format same as ordered map.
 */
    /**
     * @brief Get the current stored value
     */
    const T getValue() const { return m_val; }

    /**
     * @brief Update the stored value
     * @param v New value to store
     */
    void setValue(const/**
 * @brief Unordered_map serialization/deserialization
 * @tparam T Value type
 * 
 * Hash-based variant of map. Format same as ordered map.
 */ T& v) { m_val = v; }

    /**
     * @brief Get the type name of the stored value
     * @return std::string Type name as reported by typeid
     */
    std::string getTypeName() const override { 
        return typeid(T).name(); 
    }

private:
    T m_val; ///< The actual stored configuration value
};

/**
 * @class Config
 * @brief Central configuration management system for storing and accessing application settings
 * 
 * Maintains a registry of configuration variables that can be accessed by name.
 * Supports type-safe operations and dynamic loading from YAML configuration files.
 */
class Config {
public:
    /// Type alias for the configuration variable registry
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    /**
     * @brief Lookup or create a typed configuration variable
     * @tparam T The value type of the configuration variable
     * @param name Unique identifier for the configuration variable
     * @param default_value Initial value if creating a new variable
     * @param description Human-readable description of the variable
     * @return Shared pointer to the configuration variable
     * @throws std::invalid_argument If the name contains invalid characters
     * 
     * This method implements the registry pattern with the following behavior:
     * 1. First attempts to find an existing variable with the given name
     * 2. Validates the name format (only alphanumeric with underscores/dots allowed)
     * 3. Creates and registers a new variable if none exists
     * 
     * The name validation prevents potential security issues and parsing problems.
     */
    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name,
            const T& default_value, const std::string& description = "") {   
        // Check for existing configuration
        auto tmp = Lookup<T>(name);
        if(tmp) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name:" << name << " exists";
            return tmp;
        } 

        // Validate naming convention
        if(name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._0123456789")
                != std::string::npos) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid:" << name;
            throw std::invalid_argument(name);
        }

        // Instantiate and register new configuration variable
        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
        GetDatas[name] = v;
        return v;
    }

    /**
     * @brief Lookup an existing configuration variable with type checking
     * @tparam T The expected value type
     * @param name Name of the configuration variable to find
     * @return Shared pointer to the variable if found and type matches, nullptr otherwise
     * 
     * Performs a safe dynamic cast to verify type compatibility at runtime.
     * Provides detailed logging in case of type mismatches to aid debugging.
     */
    template<class T> 
    static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
            auto it = GetDatas().find(name);
            if(it == GetDatas().end()) {
                return nullptr;
            } 
            
            // Runtime type verification
            auto res = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
            if(res) {
                return res;
            }
            
            // Detailed type mismatch reporting
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) 
                << "Type mismatch for config '" << name << "'"
                << "(expected: " << typeid(T).name()
                << ") (real_type= " << it->second->getTypeName() << ")"
                << " " << it->second->toString();
            return nullptr;
    }

    /**
     * @brief Load configuration values from a YAML document
     * @param root The root node of a parsed YAML document
     * 
     * Walks through the YAML node hierarchy and updates all registered
     * configuration variables with matching names.
     */
    static void LoadFromYaml(const YAML::Node& root);

    /**
     * @brief Lookup a configuration variable without type information
     * @param name Name of the configuration variable
     * @return Base class pointer to the variable
     * 
     * This is the untyped version for when the concrete type is unknown.
     */
    static ConfigVarBase::ptr LookupBase(const std::string& name);

private:
    /// Static registry storing all configuration variables
    static ConfigVarMap& GetDatas() {
        static ConfigVarMap m_datas;
        return m_datas;
    }
};

}

#endif
