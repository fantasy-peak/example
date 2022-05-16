package("libmysql")
    set_homepage("https://github.com/anarthal/mysql.git")
    set_description("")
    set_license("")

    add_urls("https://github.com/anarthal/mysql.git")

    add_deps("boost 1.79.0", {system=false, configs = {all = true}})

    on_install("linux", function (package)
        os.cp("include", package:installdir())
    end)

package_end()