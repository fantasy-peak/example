
package("redis-plus-plus")
set_homepage("https://github.com/sewenew/redis-plus-plus.git")
set_description("")
set_license("Apache-2.0 License")

add_urls("https://github.com/sewenew/redis-plus-plus.git")

add_deps("cmake")
add_deps("hiredis", {system = false})
add_deps("boost 1.78.0", {configs={system=true,thread=true}})
add_syslinks("uv")

on_install("linux", function (package)
    local configs = {}
    table.insert(configs, "-DREDIS_PLUS_PLUS_BUILD_ASYNC=libuv")
    table.insert(configs, "-DREDIS_PLUS_PLUS_ASYNC_FUTURE=boost")
    table.insert(configs, "-DREDIS_PLUS_PLUS_CXX_STANDARD=20")
    table.insert(configs, "-DREDIS_PLUS_PLUS_BUILD_TEST=OFF")
    table.insert(configs, "-DREDIS_PLUS_PLUS_BUILD_SHARED=OFF")
    import("package.tools.cmake").install(package, configs)
end)
package_end()