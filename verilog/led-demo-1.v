`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company:
// Engineer:
//
// Create Date: 02/20/2021 10:41:04 AM
// Design Name:
// Module Name: led_1
// Project Name:
// Target Devices:
// Tool Versions:
// Description:
//
// Dependencies:
//
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
//
//////////////////////////////////////////////////////////////////////////////////


module led_1(
    input CLK,
    output LD0,
    output LD1,
    output LD2,
    output LD3,
    output LD4,
    output LD5,
    output LD6,
    output LD7
    );
    reg [31:0] count = 0;
    assign LD0 = count[24];
    assign LD1 = count[25];
    assign LD2 = count[26];
    assign LD3 = count[27];
    assign LD4 = count[28];
    assign LD5 = count[29];
    assign LD6 = count[30];
    assign LD7 = count[31];
    always @ (posedge(CLK)) count <= count + 1;
endmodule
