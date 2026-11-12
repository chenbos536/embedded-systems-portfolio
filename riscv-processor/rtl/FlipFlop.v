`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Module Name: FlipFlop
//////////////////////////////////////////////////////////////////////////////////

// Module definition
module FlipFlop(
   input  clk , reset,
   input  [7:0] d,
   output [7:0] q
   );
 
   
// Write your code
reg [7:0] q_reg;

assign q = q_reg;

always @(posedge clk or posedge reset) begin
    if (reset)
        q_reg <= 8'b00000000;
    else
        q_reg <= d;
end

endmodule 




