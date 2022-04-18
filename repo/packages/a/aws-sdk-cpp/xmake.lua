package("aws-sdk-cpp")

set_homepage("https://github.com/aws/aws-sdk-cpp.git")
set_description("")

add_urls("https://github.com/aws/aws-sdk-cpp.git")

add_deps("openssl 1.1.1n", {system=false})
add_deps("libcurl", {system=false})
add_deps("c-ares", {optional = true})

on_install("linux", function (package)
    local configs = {}
    table.insert(configs, "-DCMAKE_BUILD_TYPE=Release")
    table.insert(configs, "-DBUILD_SHARED_LIBS=OFF")
    table.insert(configs, "-DBUILD_ONLY=core;dynamodb")
    table.insert(configs, "-DENABLE_TESTING=OFF")
    table.insert(configs, "-DCMAKE_INSTALL_LIBDIR=lib64")
    table.insert(configs, "-UTORUN_UNIT_TESTS=OFF")
    import("package.tools.cmake").install(package, configs)
    local l = path.join(package:installdir(), "lib64/*.a")
    os.cp(l, package:installdir("lib"))
end)

package_end()