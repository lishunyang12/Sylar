#include "config.h"

namespace sylar {

Config::ConfigVarMap Config::m_datas;

ConfigVarBase::ptr Config::LookupBase(const std::string& name) {
    auto it = m_datas.find(name);
    return it == m_datas.end() ? nullptr : it->second;
}


//"A.B", 10
//A：
//  B: 10
//  C: str

static void ListAllMemeber(const std::string& prefix, 
                            const YAML::Node& node,
                            std::list<std::pair<std::string, const YAML::Node >> &output){
        if(prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._0123456789")
                != std::string::npos) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
            return;
        }
        output.push_back(std::make_pair(prefix, node));
        if(node.IsMap()) {
            for (auto it = node.begin(); it != node.end(); ++it) {
                ListAllMemeber(prefix.empty() ? (it->first.Scalar()) : (prefix + "." + it->first.Scalar()), 
                                it->second, 
                                output);        
            }
        } else if(node.IsSequence()) {
            for (size_t i = 0; i < node.size(); ++i) {
                ListAllMemeber(prefix.empty() ? std::to_string(i) : prefix + "." + std::to_string(i), 
                                node[i],
                                output);
            }
        }
}

void Config::LoadFromYaml(const YAML::Node& root) {
    std::list<std::pair<std::string, const YAML::Node>> all_nodes;
    ListAllMemeber("", root, all_nodes);

    for(const auto& i : all_nodes) {
        std::string key = i.first;
        auto node = i.second;
        if(key.empty()) {
            continue;
        }

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        ConfigVarBase::ptr var = LookupBase(key);

        if(var) {
            if(node.IsScalar()) {
                var->fromString(node.Scalar());
            } else {
                std::stringstream ss;
                ss << node;
                var->fromString(ss.str());
            }
        }
    }
}

}