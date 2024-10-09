from pymake.bare import *

build_dir = "build"
target_name = "show-tree"

add_source_directory("src")

build(
    output_directory=build_dir,
    output_file=target_name,
    compiler="g++",
    flags=["-std=c++17"],
)
run(output_file=target_name, args=["."])
