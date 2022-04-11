add_requires("fmt 8.1.1", "spdlog v1.9.2", "nlohmann_json v3.10.5")
add_repositories("repo repo")
add_requires("hiredis", {system = false})
add_requires("redis-plus-plus 1.3.3", "trantor", "drogon")
add_syslinks("pthread", "dl", "ssl", "crypto", "uuid")

set_languages("c++20")
--[[https://xmake.io/mirror/zh-cn/manual/project_target.html]]--
--[[set_optimize("faster")]]--
set_policy("check.auto_ignore_flags", false)
add_cxflags("-O2 -Wall -Wextra -Werror -pedantic-errors -Wno-missing-field-initializers -Wno-ignored-qualifiers")
add_includedirs("include")

target("xmake-example")
    set_kind("binary")
    add_files("src/*.cpp")
    add_packages("hiredis", "redis-plus-plus", "drogon", "trantor", "fmt", "spdlog", "nlohmann_json")
target_end()