`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/03/10 18:39:40
// Design Name: 
// Module Name: processor
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


module processor
(
    input clk, reset,
    output [31:0] Result
);

wire [6:0] opcode;
wire [6:0] funct7;
wire [2:0] funct3;

wire ALUSrc;
wire MemtoReg;
wire RegWrite;
wire MemRead;
wire MemWrite;
wire [1:0] ALUOp;
wire [3:0] Operation;

Controller controller_inst (
    .Opcode   (opcode),
    .ALUSrc   (ALUSrc),
    .MemtoReg (MemtoReg),
    .RegWrite (RegWrite),
    .MemRead  (MemRead),
    .MemWrite (MemWrite),
    .ALUOp    (ALUOp)
);

ALUController alucontroller_inst (
    .ALUOp     (ALUOp),
    .Funct7    (funct7),
    .Funct3    (funct3),
    .Operation (Operation)
);

data_path datapath_inst (
    .clk        (clk),
    .reset      (reset),
    .reg_write  (RegWrite),
    .mem2reg    (MemtoReg),
    .alu_src    (ALUSrc),
    .mem_write  (MemWrite),
    .mem_read   (MemRead),
    .alu_cc     (Operation),
    .opcode     (opcode),
    .funct7     (funct7),
    .funct3     (funct3),
    .alu_result (Result)
);

endmodule
