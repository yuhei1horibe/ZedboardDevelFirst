`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 04/11/2019 09:15:52 PM
// Design Name: 
// Module Name: io_test
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

module half_adder(
    input a,
    input b,
    output y,
    output cout
);
    assign cout = a & b; // Carry over
    assign y    = a ^ b;
endmodule

module adder(
    input  a,
    input  b,
    input  cin,
    output y,
    output cout
);
    wire xor_ab;

    assign xor_ab   = a ^ b;
    assign y        = xor_ab ^ cin;
    assign cout     = (xor_ab & cin) | (a & b);
endmodule

module adder4(
    input  [3:0]a,
    input  [3:0]b,
    output [4:0]y
);
    wire carry[3:0];

    half_adder U_0(
        .a    (a[0]),
        .b    (b[0]),
        .y    (y[0]),
        .cout (carry[0]));

    generate
        genvar i;
        for(i = 1; i < 4; i = i + 1) begin
            adder U_i(
                .a    (a[i]),
                .b    (b[i]),
                .cin  (carry[i - 1]),
                .y    (y[i]),
                .cout (carry[i]));
        end
    endgenerate
    assign y[4] = carry[3];
endmodule
