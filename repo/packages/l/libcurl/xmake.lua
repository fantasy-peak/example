package("libcurl")
    set_homepage("https://github.com/curl/curl.git")
    set_description("")

    add_urls("https://github.com/curl/curl.git")

    add_deps("openssl 1.1.1n", {system=false})
    add_deps("c-ares", {optional = true})

    on_install("linux", function (package)
        local configs = {}
        table.insert(configs, "-DBUILD_CURL_EXE=OFF")
        table.insert(configs, "-DBUILD_SHARED_LIBS=OFF")
        table.insert(configs, "-DENABLE_ARES=ON")
        import("package.tools.cmake").install(package, configs)
    end)
package_end()