
package("drogon")
    set_homepage("https://github.com/an-tao/drogon/")
    set_description("Drogon: A C++14/17 based HTTP web application framework running on Linux/macOS/Unix/Windows")
    set_license("MIT")

    add_urls("https://github.com/an-tao/drogon.git")
    add_versions("v1.7.5", "e2af7c55dcabafef16f26f5b3242692f5a2b54c19b7b626840bf9132d24766f6")

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
        import("package.tools.cmake").install(package, configs)
    end)
package_end()