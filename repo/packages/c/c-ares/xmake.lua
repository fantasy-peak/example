package("c-ares")

set_homepage("https://c-ares.haxx.se/")
set_description("A C library for asynchronous DNS requests")

add_urls("https://c-ares.haxx.se/download/c-ares-$(version).tar.gz")
add_versions("1.16.1", "d08312d0ecc3bd48eee0a4cc0d2137c9f194e0a28de2028928c0f6cae85f86ce")
add_versions("1.17.1", "d73dd0f6de824afd407ce10750ea081af47eba52b8a6cb307d220131ad93fc40")

on_install("linux", function (package)
    local configs = {}
    table.insert(configs, "-DCARES_SHARED:BOOL=OFF")
    table.insert(configs, "-DCARES_STATIC:BOOL=ON")
    table.insert(configs, "-DCARES_STATIC_PIC:BOOL=ON")
    import("package.tools.cmake").install(package, configs)
end)

package_end()