package("hiredis")
set_homepage("https://github.com/redis/hiredis.git")
set_description("Minimalistic C client for Redis >= 1.2")
set_license("BSD-3-Clause License")

add_urls("https://github.com/redis/hiredis.git")
add_deps("cmake")

on_install(function (package)
    local configs = {}
    import("package.tools.cmake").install(package)
end)
