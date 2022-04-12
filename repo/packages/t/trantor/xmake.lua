package("trantor")
    set_homepage("https://github.com/an-tao/trantor/")
    set_description("a non-blocking I/O tcp network lib based on c++14/17")
    set_license("BSD-3-Clause")

    add_urls("https://github.com/an-tao/trantor.git")
    add_versions("v1.5.5", "5a549c6efebe7ecba73a944cfba4a9713130704d4ccc82af534a2e108b9a0e71")

    add_deps("cmake")
    add_deps("openssl 1.1.1m", {system=false})
    add_deps("c-ares", {optional = true})
    on_install("linux", function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=Release")
        print("888888888888****************************************************")
        import("package.tools.cmake").install(package, configs)
    end)
package_end()