
target("city_hash")
    set_default(false)
    set_kind("static")
    add_files("city.cc")
    add_includedirs(".", {public = true})
target_end()
