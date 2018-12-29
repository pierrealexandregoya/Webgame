#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>

#include <nlohmann/json.hpp>

#include "config.hpp"

#define WEBGAME_REGISTER(base, derived)\
namespace webgame {\
std::shared_ptr<base> load_##derived(nlohmann::json const& j)\
{\
    auto ptr = std::make_shared<derived>();\
    ptr->load(j);\
    return ptr;\
}\
class derived##_loader_adder\
{\
private:\
    derived##_loader_adder()\
    {\
        base##_loaders()[#derived] = load_##derived;\
    }\
    static derived##_loader_adder derived##_loader_adder_instance;\
};\
derived##_loader_adder derived##_loader_adder::derived##_loader_adder_instance;\
}

#define WEBGAME_POLY_LOADER_DECL(base)\
namespace webgame {\
WEBGAME_API std::shared_ptr<base> load_##base(nlohmann::json const& j);\
WEBGAME_API std::map<std::string, std::function<std::shared_ptr<base>(nlohmann::json const& j)>> & base##_loaders();\
}

#define WEBGAME_POLY_LOADER_DEF(base)\
namespace webgame {\
std::shared_ptr<base> load_##base(nlohmann::json const& j)\
{\
    if (!j.is_object())\
        throw std::runtime_error("load_"#base": json is not an object");\
    if (j.count("type") != 1 )\
        throw std::runtime_error("load_"#base": json does not contain \"type\" field");\
    if (!j["type"].is_string())\
        throw std::runtime_error("load_"#base": \"type\" field is not a string");\
    if (base##_loaders().count(j["type"]) == 0)\
        throw std::runtime_error("load_"#base": \"" + std::string(j["type"]) + "\" type not registered");\
    return base##_loaders()[j["type"]](j);\
}\
std::map<std::string, std::function<std::shared_ptr<base>(nlohmann::json const& j)>> & base##_loaders()\
{\
    static std::map<std::string, std::function<std::shared_ptr<base>(nlohmann::json const& j)>> loaders;\
    return loaders;\
}\
}

#include "behavior.hpp"
#include "entity.hpp"

WEBGAME_POLY_LOADER_DECL(entity);
WEBGAME_POLY_LOADER_DECL(behavior);
