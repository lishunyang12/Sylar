#include "config.h"
#include "log.h"
#include <yaml-cpp/yaml.h>

sylar::ConfigVar<int>::ptr g_int_value_config =
     sylar::Config::Lookup("system.port", (int)8080, "system port");
sylar::ConfigVar<float>::ptr g_int_float_value_config =
     sylar::Config::Lookup("system.value", (float)10.5f, "system value");

void test_yaml() {
     YAML::Node root = YAML::LoadFile("/home/li/Desktop/Sylar/High-Performance-Sylar-Server/sylar/config/log.yaml");

     SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root;
}

int main(int argc, char** argv) {
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_float_value_config->toString();
    test_yaml();
    return 0;
}