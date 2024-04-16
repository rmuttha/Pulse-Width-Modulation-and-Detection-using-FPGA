# Usage with Vitis IDE:
# In Vitis IDE create a Single Application Debug launch configuration,
# change the debug type to 'Attach to running target' and provide this 
# tcl script in 'Execute Script' option.
# Path of this script: D:\ECE544\project\PROJ_TEST2\TESTING2_system\_ide\scripts\debugger_testing2-default.tcl
# 
# 
# Usage with xsct:
# To debug using xsct, launch xsct and run below command
# source D:\ECE544\project\PROJ_TEST2\TESTING2_system\_ide\scripts\debugger_testing2-default.tcl
# 
connect -url tcp:127.0.0.1:3121
targets -set -nocase -filter {name =~ "*microblaze*#0" && bscan=="USER2" }
loadhw -hw D:/ECE544/project/PROJ_TEST2/TEST2/export/TEST2/hw/nexysa7fpga.xsa -regs
configparams mdm-detect-bscan-mask 2
targets -set -nocase -filter {name =~ "*microblaze*#0" && bscan=="USER2" }
rst -processor
targets -set -nocase -filter {name =~ "*microblaze*#0" && bscan=="USER2" }
dow D:/ECE544/project/PROJ_TEST2/TESTING2/Debug/TESTING2.elf
targets -set -nocase -filter {name =~ "*microblaze*#0" && bscan=="USER2" }
con
