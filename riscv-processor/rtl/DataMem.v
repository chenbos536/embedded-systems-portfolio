`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
/// data memory
//////////////////////////////////////////////////////////////////////////////////

module DataMem(MemRead, MemWrite, addr, write_data, read_data);

// Define I/O ports
input MemRead, MemWrite;
input [8:0] addr;
input [31:0] write_data;
output [31:0] read_data;

// Describe DataMem behavior 
reg [31:0] memory [0:127];
assign read_data = (MemRead) ? memory[addr[8:2]] : 32'b0;
always @(*) begin
    if (MemWrite)
        memory[addr[8:2]] = write_data;
end

endmodule
     
