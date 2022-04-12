set_project("xmake-example")
set_version("0.0.1", {build = "%Y%m%d%H%M"})
set_xmakever("2.1.0")

add_repositories("repo repo")
add_requires("redis-plus-plus 1.3.3", "trantor", "drogon")
add_requires("fmt 8.1.1", "spdlog v1.9.2", "nlohmann_json v3.10.5")

set_languages("c++20")
--[[https://xmake.io/mirror/zh-cn/manual/project_target.html]]--
--[[set_optimize("faster")]]--
set_policy("check.auto_ignore_flags", false)
add_cxflags("-O2 -Wall -Wextra -pedantic-errors -Wno-missing-field-initializers -Wno-ignored-qualifiers")
add_includedirs("include")

target("xmake-example")
    set_kind("binary")
    add_files("src/*.cpp")
    add_packages("fmt", "spdlog", "nlohmann_json", "drogon", "trantor", "redis-plus-plus")
    add_links("uuid")
    add_syslinks("pthread")
target_end()