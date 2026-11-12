`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// ALU_32
//////////////////////////////////////////////////////////////////////////////////

module alu_32(
   input  [31:0] A_in,B_in,        // ALU 32 bit inputs
   input  [3:0]  ALU_Sel,          // ALU 4 bits selection
   output [31:0] ALU_Out,          // ALU 32 bits output
   output reg    Carry_Out,
   output        Zero,             // 1 bit Zero Flag
   output reg    Overflow = 1'b0   // 1 bit Overflow Flag 
    );

reg [31:0] ALU_Result;
reg [32:0] temp;
reg [32:0] twos_com;

// Use the code from the lab manual or use your code if it received full points.
assign ALU_Out = ALU_Result;
assign Zero = (ALU_Result == 32'b0);

always @(*) begin
    Overflow = 1'b0;
    Carry_Out = 1'b0;
    ALU_Result = 32'b0;

    case (ALU_Sel)
        4'b0000: begin
            ALU_Result = A_in & B_in;
        end

        4'b0001: begin
            ALU_Result = A_in | B_in;
        end

        4'b0010: begin
            ALU_Result = A_in + B_in;
            temp = {1'b0, A_in} + {1'b0, B_in};
            Carry_Out = temp[32];
            if ((A_in[31] == B_in[31]) && (ALU_Result[31] != A_in[31]))
                Overflow = 1'b1;
        end

        4'b0110: begin
            ALU_Result = A_in - B_in;
            twos_com = {1'b0, (~B_in + 1'b1)};
            temp = {1'b0, A_in} + twos_com;
            Carry_Out = temp[32];
            if ((A_in[31] != B_in[31]) && (ALU_Result[31] != A_in[31]))
                Overflow = 1'b1;
        end

        4'b0111: begin
            ALU_Result = ($signed(A_in) < $signed(B_in)) ? 32'd1 : 32'd0;
        end

        4'b1100: begin
            ALU_Result = ~(A_in | B_in);
        end

        4'b1111: begin
            ALU_Result = (A_in == B_in) ? 32'd1 : 32'd0;
        end

        default: begin
            ALU_Result = 32'b0;
        end
    endcase
end

endmodule
