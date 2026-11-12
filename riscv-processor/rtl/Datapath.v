////   Data Path /////
module data_path #(
    parameter PC_W = 8,       // Program Counter
    parameter INS_W = 32,     // Instruction Width
    parameter RF_ADDRESS = 5, // Register File Address
    parameter DATA_W = 32,    // Data WriteData
    parameter DM_ADDRESS = 9, // Data Memory Address
    parameter ALU_CC_W = 4    // ALU Control Code Width
 )(
    input                  clk ,    // CLK in datapath figure
    input                  reset,   // Reset in datapath figure      
    input                  reg_write,   // RegWrite in datapath figure
    input                  mem2reg,     // MemtoReg in datapath figure
    input                  alu_src,     // ALUSrc in datapath figure 
    input                  mem_write,   // MemWrite in datapath figure  
    input                  mem_read,    // MemRead in datapath figure          
    input  [ALU_CC_W-1:0]  alu_cc,      // ALUCC in datapath figure
    output          [6:0]  opcode,      // opcode in dataptah figure
    output          [6:0]  funct7,      // Funct7 in datapath figure
    output          [2:0]  funct3,      // Funct3 in datapath figure
    output   [DATA_W-1:0]  alu_result   // Datapath_Result in datapath figure
 );


// Write your code here
wire [PC_W-1:0] pc_current;
wire [PC_W-1:0] pc_next;

wire [INS_W-1:0] instruction;

wire [RF_ADDRESS-1:0] rd_rg_wrt_wire;
wire [RF_ADDRESS-1:0] rd_rg_addr_wire1;
wire [RF_ADDRESS-1:0] rd_rg_addr_wire2;

wire [DATA_W-1:0] rg_rd_data1;
wire [DATA_W-1:0] rg_rd_data2;
wire [DATA_W-1:0] imm_out;
wire [DATA_W-1:0] alu_in2;
wire [DATA_W-1:0] mem_read_data;
wire [DATA_W-1:0] writeback_data;

wire carry_out_wire;
wire zero_wire;
wire overflow_wire;

assign pc_next = pc_current + 8'd4;

assign opcode = instruction[6:0];
assign funct3 = instruction[14:12];
assign funct7 = instruction[31:25];

assign rd_rg_wrt_wire  = instruction[11:7];
assign rd_rg_addr_wire1 = instruction[19:15];
assign rd_rg_addr_wire2 = instruction[24:20];

FlipFlop pc_reg (
    .clk   (clk),
    .reset (reset),
    .d     (pc_next),
    .q     (pc_current)
);

InstMem instr_mem (
    .addr        (pc_current),
    .instruction (instruction)
);

RegFile rf (
    .clk         (clk),
    .reset       (reset),
    .rg_wrt_en   (reg_write),
    .rg_wrt_addr (rd_rg_wrt_wire),
    .rg_rd_addr1 (rd_rg_addr_wire1),
    .rg_rd_addr2 (rd_rg_addr_wire2),
    .rg_wrt_data (writeback_data),
    .rg_rd_data1 (rg_rd_data1),
    .rg_rd_data2 (rg_rd_data2)
);

ImmGen imm_gen (
    .InstCode (instruction),
    .ImmOut   (imm_out)
);

MUX21 alu_mux (
    .D1 (rg_rd_data2),
    .D2 (imm_out),
    .S  (alu_src),
    .Y  (alu_in2)
);

alu_32 alu (
    .A_in      (rg_rd_data1),
    .B_in      (alu_in2),
    .ALU_Sel   (alu_cc),
    .ALU_Out   (alu_result),
    .Carry_Out (carry_out_wire),
    .Zero      (zero_wire),
    .Overflow  (overflow_wire)
);

DataMem data_mem (
    .MemRead    (mem_read),
    .MemWrite   (mem_write),
    .addr       (alu_result[DM_ADDRESS-1:0]),
    .write_data (rg_rd_data2),
    .read_data  (mem_read_data)
);

MUX21 wb_mux (
    .D1 (alu_result),
    .D2 (mem_read_data),
    .S  (mem2reg),
    .Y  (writeback_data)
);

endmodule
