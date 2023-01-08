
package("drogon")
    set_homepage("https://github.com/drogonframework/drogon.git")
    set_description("Drogon: A C++14/17 based HTTP web application framework running on Linux/macOS/Unix/Windows")
    set_license("MIT")

    add_urls("https://github.com/drogonframework/drogon.git")

    add_deps("cmake")
    add_deps("trantor")
    add_deps("jsoncpp 1.9.5", {system=false})
    add_deps("zlib v1.2.12", {system=false})
    add_deps("brotli 1.0.9", {system=false})
    on_install("linux", function (package)
        local configs = {}
        table.insert(configs, "-DBUILD_POSTGRESQL=OFF")
        table.insert(configs, "-DLIBPQ_BATCH_MODE=OFF")
        table.insert(configs, "-DBUILD_MYSQL=OFF")
        table.insert(configs, "-DBUILD_SQLITE=OFF")
        table.insert(configs, "-DBUILD_REDIS=OFF")
        table.insert(configs, "-DBUILD_TESTING=OFF")
        import("package.tools.cmake").install(package, configs)
    end)
package_end()