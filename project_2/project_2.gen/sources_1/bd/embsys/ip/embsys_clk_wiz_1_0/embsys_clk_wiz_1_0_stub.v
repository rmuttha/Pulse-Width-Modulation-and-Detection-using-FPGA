// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// --------------------------------------------------------------------------------
// Tool Version: Vivado v.2022.2 (win64) Build 3671981 Fri Oct 14 05:00:03 MDT 2022
// Date        : Fri Feb  9 20:29:57 2024
// Host        : LAPTOP-I10N6P43 running 64-bit major release  (build 9200)
// Command     : write_verilog -force -mode synth_stub
//               e:/EMBEDDED_DESIGN/project_2/project_2.gen/sources_1/bd/embsys/ip/embsys_clk_wiz_1_0/embsys_clk_wiz_1_0_stub.v
// Design      : embsys_clk_wiz_1_0
// Purpose     : Stub declaration of top-level module interface
// Device      : xc7a100tcsg324-1
// --------------------------------------------------------------------------------

// This empty module with port declaration file causes synthesis tools to infer a black box for IP.
// The synthesis directives are for Synopsys Synplify support to prevent IO buffer insertion.
// Please paste the declaration into a Verilog source file or add the file as an additional source.
module embsys_clk_wiz_1_0(clk_out1, clkPWM_10MHz, resetn, locked, clk_in1)
/* synthesis syn_black_box black_box_pad_pin="clk_out1,clkPWM_10MHz,resetn,locked,clk_in1" */;
  output clk_out1;
  output clkPWM_10MHz;
  input resetn;
  output locked;
  input clk_in1;
endmodule
