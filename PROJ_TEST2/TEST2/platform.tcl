# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct E:\EMBEDDED_DESIGN\PROJ_TEST2\TEST2\platform.tcl
# 
# OR launch xsct and run below command.
# source E:\EMBEDDED_DESIGN\PROJ_TEST2\TEST2\platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {TEST2}\
-hw {E:\EMBEDDED_DESIGN\project_2\nexysa7fpga.xsa}\
-proc {microblaze_0} -os {standalone} -out {E:/EMBEDDED_DESIGN/PROJ_TEST2}

platform write
platform generate -domains 
platform active {TEST2}
platform generate
platform clean
platform generate
platform generate
platform active {TEST2}
platform generate -domains 
platform generate -domains standalone_domain 
platform generate
